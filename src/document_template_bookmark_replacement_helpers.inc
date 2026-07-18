template <typename Collector>
auto collect_bookmark_mutation_inputs(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, std::string_view allocation_failure_detail,
    Collector &&collector) -> bool {
    try {
        return std::forward<Collector>(collector)();
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            last_error_info, allocation_failure_detail, entry_name);
    } catch (const std::length_error &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            last_error_info, allocation_failure_detail, entry_name);
    }
    return false;
}

struct bookmark_image_append_input final {
    std::string image_data;
    std::string extension;
    std::string content_type;
    std::string display_name;
};

template <typename ImageParts>
class bookmark_image_package_transaction final {
public:
    bookmark_image_package_transaction(
        pugi::xml_document &relationships_document,
        pugi::xml_document &content_types_document, pugi::xml_node part_root,
        bool &has_relationships_part, bool &relationships_dirty,
        bool &content_types_dirty, ImageParts &image_parts) noexcept
        : relationships_document_(relationships_document),
          content_types_document_(content_types_document), part_root_(part_root),
          part_root_last_attribute_(part_root.last_attribute()),
          has_relationships_part_(has_relationships_part),
          relationships_dirty_(relationships_dirty),
          content_types_dirty_(content_types_dirty), image_parts_(image_parts),
          had_relationships_part_(has_relationships_part),
          relationships_were_dirty_(relationships_dirty),
          content_types_were_dirty_(content_types_dirty),
          image_parts_size_(image_parts.size()) {}

    bookmark_image_package_transaction(
        const bookmark_image_package_transaction &) = delete;
    auto operator=(const bookmark_image_package_transaction &)
        -> bookmark_image_package_transaction & = delete;

    ~bookmark_image_package_transaction() noexcept {
        if (active_) {
            rollback();
        }
    }

    auto prepare(featherdoc::document_error_info &last_error_info,
                 std::string_view entry_name,
                 std::size_t replacement_count) -> bool {
        if (replacement_count > image_parts_.max_size() - image_parts_.size()) {
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                last_error_info,
                "bookmark image replacement exceeds the image part limit",
                entry_name);
            return false;
        }

        if (featherdoc::detail::checked_clone_xml_document(
                relationships_document_, relationships_snapshot_) !=
                featherdoc::detail::xml_document_clone_status::success ||
            featherdoc::detail::checked_clone_xml_document(
                content_types_document_, content_types_snapshot_) !=
                featherdoc::detail::xml_document_clone_status::success) {
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                last_error_info,
                "bookmark image package snapshot ran out of memory",
                entry_name);
            return false;
        }

        try {
            image_parts_.reserve(image_parts_.size() + replacement_count);
        } catch (...) {
            (void)featherdoc::detail::set_xml_document_allocation_failure(
                last_error_info,
                "bookmark image part reservation ran out of memory",
                entry_name);
            return false;
        }

        active_ = true;
        return true;
    }

    void commit() noexcept { active_ = false; }

    [[nodiscard]] auto initial_image_part_count() const noexcept
        -> std::size_t {
        return image_parts_size_;
    }

private:
    void rollback_root_attributes() noexcept {
        auto attribute =
            part_root_last_attribute_ != pugi::xml_attribute{}
                ? part_root_last_attribute_.next_attribute()
                : part_root_.first_attribute();
        while (attribute != pugi::xml_attribute{}) {
            const auto next = attribute.next_attribute();
            (void)part_root_.remove_attribute(attribute);
            attribute = next;
        }
    }

    void rollback() noexcept {
        rollback_root_attributes();
        image_parts_.resize(image_parts_size_);
        has_relationships_part_ = had_relationships_part_;
        relationships_dirty_ = relationships_were_dirty_;
        content_types_dirty_ = content_types_were_dirty_;
        relationships_document_ = std::move(relationships_snapshot_);
        content_types_document_ = std::move(content_types_snapshot_);
        active_ = false;
    }

    pugi::xml_document &relationships_document_;
    pugi::xml_document &content_types_document_;
    pugi::xml_node part_root_;
    pugi::xml_attribute part_root_last_attribute_;
    bool &has_relationships_part_;
    bool &relationships_dirty_;
    bool &content_types_dirty_;
    ImageParts &image_parts_;
    bool had_relationships_part_;
    bool relationships_were_dirty_;
    bool content_types_were_dirty_;
    std::size_t image_parts_size_;
    pugi::xml_document relationships_snapshot_;
    pugi::xml_document content_types_snapshot_;
    bool active_{false};
};

auto bookmark_image_attribute_is_nonempty(pugi::xml_node node,
                                          const char *name) noexcept -> bool {
    const auto attribute = node.attribute(name);
    return attribute != pugi::xml_attribute{} && *attribute.value() != '\0';
}

auto bookmark_image_ascii_case_insensitive_equal(std::string_view left,
                                                 std::string_view right) noexcept
    -> bool {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0U; index < left.size(); ++index) {
        const auto left_character = static_cast<unsigned char>(left[index]);
        const auto right_character = static_cast<unsigned char>(right[index]);
        if (std::tolower(left_character) != std::tolower(right_character)) {
            return false;
        }
    }
    return true;
}

auto bookmark_image_drawing_is_complete(
    pugi::xml_node paragraph, bool floating,
    const std::optional<featherdoc::floating_image_options> &options,
    std::string_view expected_display_name, std::string_view expected_extension,
    std::string_view expected_content_type,
    pugi::xml_document &relationships_document,
    pugi::xml_document &content_types_document) noexcept -> bool {
    const auto run = paragraph.child("w:r");
    const auto drawing = run.child("w:drawing");
    const auto container =
        drawing.child(floating ? "wp:anchor" : "wp:inline");
    if (run == pugi::xml_node{} || drawing == pugi::xml_node{} ||
        container == pugi::xml_node{} ||
        !bookmark_image_attribute_is_nonempty(drawing, "xmlns:r") ||
        !bookmark_image_attribute_is_nonempty(drawing, "xmlns:wp") ||
        !bookmark_image_attribute_is_nonempty(drawing, "xmlns:a") ||
        !bookmark_image_attribute_is_nonempty(drawing, "xmlns:pic") ||
        !bookmark_image_attribute_is_nonempty(container, "distT") ||
        !bookmark_image_attribute_is_nonempty(container, "distB") ||
        !bookmark_image_attribute_is_nonempty(container, "distL") ||
        !bookmark_image_attribute_is_nonempty(container, "distR")) {
        return false;
    }

    if (floating) {
        const auto simple_position = container.child("wp:simplePos");
        const auto position_horizontal = container.child("wp:positionH");
        const auto position_vertical = container.child("wp:positionV");
        if (!options.has_value() || simple_position == pugi::xml_node{} ||
            position_horizontal == pugi::xml_node{} ||
            position_vertical == pugi::xml_node{} ||
            !bookmark_image_attribute_is_nonempty(container, "simplePos") ||
            !bookmark_image_attribute_is_nonempty(container,
                                                  "relativeHeight") ||
            !bookmark_image_attribute_is_nonempty(container, "behindDoc") ||
            !bookmark_image_attribute_is_nonempty(container, "locked") ||
            !bookmark_image_attribute_is_nonempty(container, "layoutInCell") ||
            !bookmark_image_attribute_is_nonempty(container,
                                                  "allowOverlap") ||
            !bookmark_image_attribute_is_nonempty(simple_position, "x") ||
            !bookmark_image_attribute_is_nonempty(simple_position, "y") ||
            !bookmark_image_attribute_is_nonempty(position_horizontal,
                                                  "relativeFrom") ||
            *position_horizontal.child("wp:posOffset").text().as_string() ==
                '\0' ||
            !bookmark_image_attribute_is_nonempty(position_vertical,
                                                  "relativeFrom") ||
            *position_vertical.child("wp:posOffset").text().as_string() ==
                '\0') {
            return false;
        }

        switch (options->wrap_mode) {
        case featherdoc::floating_image_wrap_mode::none:
            if (container.child("wp:wrapNone") == pugi::xml_node{}) {
                return false;
            }
            break;
        case featherdoc::floating_image_wrap_mode::square:
            if (!bookmark_image_attribute_is_nonempty(
                    container.child("wp:wrapSquare"), "wrapText")) {
                return false;
            }
            break;
        case featherdoc::floating_image_wrap_mode::top_bottom:
            if (container.child("wp:wrapTopAndBottom") == pugi::xml_node{}) {
                return false;
            }
            break;
        }
    }

    const auto extent = container.child("wp:extent");
    const auto effect_extent = container.child("wp:effectExtent");
    const auto doc_properties = container.child("wp:docPr");
    const auto frame_locks =
        container.child("wp:cNvGraphicFramePr")
            .child("a:graphicFrameLocks");
    const auto graphic_data =
        container.child("a:graphic").child("a:graphicData");
    const auto picture = graphic_data.child("pic:pic");
    const auto non_visual_picture = picture.child("pic:nvPicPr");
    const auto non_visual_properties =
        non_visual_picture.child("pic:cNvPr");
    const auto blip_fill = picture.child("pic:blipFill");
    const auto blip = blip_fill.child("a:blip");
    const auto stretch = blip_fill.child("a:stretch");
    const auto shape_properties = picture.child("pic:spPr");
    const auto transform = shape_properties.child("a:xfrm");
    const auto offset = transform.child("a:off");
    const auto extents = transform.child("a:ext");
    const auto geometry = shape_properties.child("a:prstGeom");
    if (!bookmark_image_attribute_is_nonempty(extent, "cx") ||
        !bookmark_image_attribute_is_nonempty(extent, "cy") ||
        !bookmark_image_attribute_is_nonempty(effect_extent, "l") ||
        !bookmark_image_attribute_is_nonempty(effect_extent, "t") ||
        !bookmark_image_attribute_is_nonempty(effect_extent, "r") ||
        !bookmark_image_attribute_is_nonempty(effect_extent, "b") ||
        !bookmark_image_attribute_is_nonempty(doc_properties, "id") ||
        std::string_view{doc_properties.attribute("name").value()} !=
            expected_display_name ||
        !bookmark_image_attribute_is_nonempty(frame_locks,
                                              "noChangeAspect") ||
        !bookmark_image_attribute_is_nonempty(graphic_data, "uri") ||
        !bookmark_image_attribute_is_nonempty(non_visual_properties, "id") ||
        std::string_view{non_visual_properties.attribute("name").value()} !=
            expected_display_name ||
        non_visual_picture.child("pic:cNvPicPr") == pugi::xml_node{} ||
        !bookmark_image_attribute_is_nonempty(blip, "r:embed") ||
        stretch.child("a:fillRect") == pugi::xml_node{} ||
        !bookmark_image_attribute_is_nonempty(offset, "x") ||
        !bookmark_image_attribute_is_nonempty(offset, "y") ||
        !bookmark_image_attribute_is_nonempty(extents, "cx") ||
        !bookmark_image_attribute_is_nonempty(extents, "cy") ||
        !bookmark_image_attribute_is_nonempty(geometry, "prst") ||
        geometry.child("a:avLst") == pugi::xml_node{}) {
        return false;
    }

    if (floating && options->crop.has_value()) {
        const auto source_rectangle = blip_fill.child("a:srcRect");
        if (!bookmark_image_attribute_is_nonempty(source_rectangle, "l") ||
            !bookmark_image_attribute_is_nonempty(source_rectangle, "t") ||
            !bookmark_image_attribute_is_nonempty(source_rectangle, "r") ||
            !bookmark_image_attribute_is_nonempty(source_rectangle, "b")) {
            return false;
        }
    }

    const auto relationship_id =
        std::string_view{blip.attribute("r:embed").value()};
    auto relationship = featherdoc::detail::first_package_relationship(
        featherdoc::detail::package_relationships_root(
            relationships_document));
    for (; relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Id").value()} ==
            relationship_id) {
            break;
        }
    }
    if (relationship == pugi::xml_node{} ||
        std::string_view{relationship.attribute("Type").value()} !=
            "http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" ||
        !bookmark_image_attribute_is_nonempty(relationship, "Target")) {
        return false;
    }

    auto default_node = featherdoc::detail::first_package_content_type_default(
        featherdoc::detail::package_content_types_root(content_types_document));
    for (; default_node != pugi::xml_node{};
         default_node =
             featherdoc::detail::next_package_content_type_default(
                 default_node)) {
        if (bookmark_image_ascii_case_insensitive_equal(
                default_node.attribute("Extension").value(),
                expected_extension) &&
            std::string_view{default_node.attribute("ContentType").value()} ==
                expected_content_type) {
            return true;
        }
    }
    return false;
}

auto collect_table_row_bookmark_placeholders(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    std::string_view bookmark_name, std::vector<table_row_bookmark_placeholder> &placeholders)
    -> bool {
    std::vector<block_bookmark_placeholder> block_placeholders;
    if (!collect_block_bookmark_placeholders(last_error_info, document, entry_name,
                                             bookmark_name,
                                             block_placeholders)) {
        return false;
    }

    for (const auto &placeholder : block_placeholders) {
        const auto cell = placeholder.paragraph.parent();
        if (cell == pugi::xml_node{} || std::string_view{cell.name()} != "w:tc") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the bookmark paragraph to live "
                           "inside a table cell",
                           std::string{entry_name});
            return false;
        }

        const auto row = cell.parent();
        if (row == pugi::xml_node{} || std::string_view{row.name()} != "w:tr") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the bookmark paragraph to live "
                           "inside a table row",
                           std::string{entry_name});
            return false;
        }

        const auto table = row.parent();
        if (table == pugi::xml_node{} || std::string_view{table.name()} != "w:tbl") {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the bookmark row to belong to a "
                           "table",
                           std::string{entry_name});
            return false;
        }

        for (const auto &existing : placeholders) {
            if (existing.row == row) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "table row replacement requires each template row to contain "
                               "the target bookmark at most once",
                               std::string{entry_name});
                return false;
            }
        }

        const auto cell_count = count_named_children(row, "w:tc");
        if (cell_count == 0U) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "table row replacement requires the template row to contain at "
                           "least one cell",
                           std::string{entry_name});
            return false;
        }

        placeholders.push_back({table, row, cell_count});
    }

    return true;
}

auto replace_placeholders_with_table_rows(
    featherdoc::document_error_info &last_error_info,
    featherdoc::detail::xml_handle_lifetime &handle_lifetime,
    std::string_view entry_name,
    std::span<const table_row_bookmark_placeholder> placeholders,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    auto retired_rows = std::vector<pugi::xml_node>{};
    auto prepared_rows = std::vector<pugi::xml_node>{};
    try {
        if (!rows.empty() &&
            placeholders.size() >
                std::numeric_limits<std::size_t>::max() / rows.size()) {
            throw std::length_error{"replacement table row count overflow"};
        }
        retired_rows.reserve(placeholders.size());
        prepared_rows.reserve(placeholders.size() * rows.size());
        for (const auto &placeholder : placeholders) {
            if (placeholder.table == pugi::xml_node{} ||
                placeholder.row == pugi::xml_node{}) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "failed to resolve the placeholder row for table row "
                               "replacement",
                               std::string{entry_name});
                return false;
            }
            if (placeholder.row.parent() != placeholder.table) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "failed to resolve the placeholder row parent table",
                               std::string{entry_name});
                return false;
            }
            for (const auto &row_values : rows) {
                if (row_values.size() != placeholder.cell_count) {
                    set_last_error(
                        last_error_info,
                        std::make_error_code(std::errc::invalid_argument),
                        "replacement table row cell count must exactly match the "
                        "template row cell count",
                        std::string{entry_name});
                    return false;
                }
            }
            append_outermost_removal_root(retired_rows, placeholder.row);
        }
    } catch (const std::bad_alloc &) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare replacement table row tracking",
                       std::string{entry_name});
        return false;
    } catch (const std::length_error &) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::value_too_large),
                       "replacement table row count is too large",
                       std::string{entry_name});
        return false;
    }

    const auto rollback_prepared_rows = [&]() noexcept {
        for (const auto row : prepared_rows) {
            auto table = row.parent();
            if (table != pugi::xml_node{}) {
                (void)table.remove_child(row);
            }
        }
    };

    const auto checked_insert_row_copy_before = [](
                                                    pugi::xml_node table,
                                                    pugi::xml_node source_row)
        -> pugi::xml_node {
        if (table == pugi::xml_node{} || source_row == pugi::xml_node{} ||
            source_row.parent() != table ||
            source_row.type() == pugi::node_document) {
            return {};
        }

        auto row_copy =
            table.insert_child_before(source_row.type(), source_row);
        if (row_copy == pugi::xml_node{}) {
            return {};
        }

        const auto rollback = [&]() noexcept {
            (void)table.remove_child(row_copy);
        };
        try {
            if (!featherdoc::detail::xml_document_clone_detail::
                    copy_node_contents(source_row, row_copy)) {
                rollback();
                return {};
            }
            for (auto child = source_row.first_child();
                 child != pugi::xml_node{}; child = child.next_sibling()) {
                if (featherdoc::detail::checked_append_copy_xml_node(
                        child, row_copy) !=
                    featherdoc::detail::xml_document_clone_status::success) {
                    rollback();
                    return {};
                }
            }
        } catch (...) {
            rollback();
            return {};
        }
        return row_copy;
    };

    for (const auto &placeholder : placeholders) {
        auto table = placeholder.table;
        for (const auto &row_values : rows) {
            auto row_copy =
                checked_insert_row_copy_before(table, placeholder.row);
            if (row_copy == pugi::xml_node{}) {
                rollback_prepared_rows();
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to clone the template row before the bookmark row",
                               std::string{entry_name});
                return false;
            }
            prepared_rows.push_back(row_copy);

            if (count_named_children(row_copy, "w:tc") !=
                row_values.size()) {
                rollback_prepared_rows();
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to clone every template row cell before bookmark "
                    "table-row replacement",
                    std::string{entry_name});
                return false;
            }

            std::size_t cell_index = 0U;
            for (auto cell = row_copy.child("w:tc"); cell != pugi::xml_node{};
                 cell = featherdoc::detail::next_named_sibling(cell, "w:tc"),
                      ++cell_index) {
                if (cell_index >= row_values.size()) {
                    rollback_prepared_rows();
                    set_last_error(
                        last_error_info,
                        std::make_error_code(std::errc::not_enough_memory),
                        "cloned template row contains an unexpected cell count",
                        std::string{entry_name});
                    return false;
                }
                if (!rewrite_table_cell_plain_text(cell,
                                                   row_values[cell_index])) {
                    rollback_prepared_rows();
                    set_last_error(last_error_info,
                                   std::make_error_code(std::errc::not_enough_memory),
                                   "failed to rewrite a cloned template row cell",
                                   std::string{entry_name});
                    return false;
                }
            }
            if (cell_index != row_values.size()) {
                rollback_prepared_rows();
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to rewrite every cloned template row cell",
                    std::string{entry_name});
                return false;
            }
        }
    }

    if (!retire_bookmark_subtrees(last_error_info, handle_lifetime,
                                  retired_rows, entry_name,
                                  "bookmark table-row replacement")) {
        rollback_prepared_rows();
        return false;
    }

    for (const auto row : retired_rows) {
        auto table = row.parent();
        if (table != pugi::xml_node{}) {
            (void)table.remove_child(row);
        }
    }
    return true;
}

auto insert_tables_before_placeholders(
    featherdoc::document_error_info &last_error_info,
    featherdoc::detail::xml_handle_lifetime &handle_lifetime,
    std::string_view entry_name,
    std::span<const block_bookmark_placeholder> placeholders,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    auto retired_paragraphs = std::vector<pugi::xml_node>{};
    auto prepared_tables = std::vector<pugi::xml_node>{};
    try {
        retired_paragraphs.reserve(placeholders.size());
        prepared_tables.reserve(placeholders.size());
        for (const auto &placeholder : placeholders) {
            if (placeholder.paragraph == pugi::xml_node{} ||
                placeholder.paragraph.parent() == pugi::xml_node{}) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "failed to resolve the placeholder paragraph for table "
                               "replacement",
                               std::string{entry_name});
                return false;
            }
            append_outermost_removal_root(retired_paragraphs,
                                          placeholder.paragraph);
        }
    } catch (const std::bad_alloc &) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare replacement table tracking",
                       std::string{entry_name});
        return false;
    }

    const auto rollback_prepared_tables = [&]() noexcept {
        for (const auto table : prepared_tables) {
            auto parent = table.parent();
            if (parent != pugi::xml_node{}) {
                (void)parent.remove_child(table);
            }
        }
    };

    for (const auto &placeholder : placeholders) {
        auto parent = placeholder.paragraph.parent();
        auto table_node =
            parent.insert_child_before("w:tbl", placeholder.paragraph);
        if (table_node == pugi::xml_node{}) {
            rollback_prepared_tables();
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to insert the replacement table before the bookmark "
                           "paragraph",
                           std::string{entry_name});
            return false;
        }
        prepared_tables.push_back(table_node);

        for (const auto &row_values : rows) {
            const auto row =
                featherdoc::detail::append_row_node(table_node, row_values.size());
            if (row == pugi::xml_node{}) {
                rollback_prepared_tables();
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append a replacement table row",
                               std::string{entry_name});
                return false;
            }

            auto cell = row.child("w:tc");
            for (const auto &cell_text : row_values) {
                if (cell == pugi::xml_node{}) {
                    rollback_prepared_tables();
                    set_last_error(last_error_info,
                                   std::make_error_code(std::errc::not_enough_memory),
                                   "failed to resolve a replacement table cell",
                                   std::string{entry_name});
                    return false;
                }

                if (!cell_text.empty() &&
                    !featherdoc::detail::replace_table_cell_text(
                        cell, cell_text.c_str())) {
                    rollback_prepared_tables();
                    set_last_error(last_error_info,
                                   std::make_error_code(std::errc::not_enough_memory),
                                   "failed to append replacement table text",
                                   std::string{entry_name});
                    return false;
                }

                cell = featherdoc::detail::next_named_sibling(cell, "w:tc");
            }
        }
    }

    if (!retire_bookmark_subtrees(last_error_info, handle_lifetime,
                                  retired_paragraphs, entry_name,
                                  "bookmark table replacement")) {
        rollback_prepared_tables();
        return false;
    }

    for (const auto retired_paragraph : retired_paragraphs) {
        auto parent = retired_paragraph.parent();
        if (parent != pugi::xml_node{}) {
            (void)parent.remove_child(retired_paragraph);
        }
    }
    return true;
}

auto replace_placeholders_with_plain_text_paragraphs(
    featherdoc::document_error_info &last_error_info,
    featherdoc::detail::xml_handle_lifetime &handle_lifetime,
    std::string_view entry_name,
    std::span<const block_bookmark_placeholder> placeholders,
    const std::vector<std::string> &paragraphs) -> bool {
    auto retired_paragraphs = std::vector<pugi::xml_node>{};
    auto prepared_paragraphs = std::vector<pugi::xml_node>{};
    try {
        if (!paragraphs.empty() &&
            placeholders.size() >
                std::numeric_limits<std::size_t>::max() / paragraphs.size()) {
            throw std::length_error{"replacement paragraph count overflow"};
        }
        retired_paragraphs.reserve(placeholders.size());
        prepared_paragraphs.reserve(placeholders.size() * paragraphs.size());
        for (const auto &placeholder : placeholders) {
            if (placeholder.paragraph == pugi::xml_node{} ||
                placeholder.paragraph.parent() == pugi::xml_node{}) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "failed to resolve the placeholder paragraph for paragraph "
                               "replacement",
                               std::string{entry_name});
                return false;
            }
            append_outermost_removal_root(retired_paragraphs,
                                          placeholder.paragraph);
        }
    } catch (const std::bad_alloc &) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare replacement paragraph tracking",
                       std::string{entry_name});
        return false;
    } catch (const std::length_error &) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::value_too_large),
                       "replacement paragraph count is too large",
                       std::string{entry_name});
        return false;
    }

    const auto rollback_prepared_paragraphs = [&]() noexcept {
        for (const auto prepared_paragraph : prepared_paragraphs) {
            auto parent = prepared_paragraph.parent();
            if (parent != pugi::xml_node{}) {
                (void)parent.remove_child(prepared_paragraph);
            }
        }
    };

    for (const auto &placeholder : placeholders) {
        auto parent = placeholder.paragraph.parent();
        for (const auto &paragraph_text : paragraphs) {
            if (!featherdoc::detail::insert_plain_text_paragraph(
                    parent, placeholder.paragraph, paragraph_text)) {
                rollback_prepared_paragraphs();
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to insert a replacement paragraph before the "
                               "bookmark placeholder",
                               std::string{entry_name});
                return false;
            }
            prepared_paragraphs.push_back(
                placeholder.paragraph.previous_sibling());
        }
    }

    if (!retire_bookmark_subtrees(last_error_info, handle_lifetime,
                                  retired_paragraphs, entry_name,
                                  "bookmark paragraph replacement")) {
        rollback_prepared_paragraphs();
        return false;
    }

    for (const auto retired_paragraph : retired_paragraphs) {
        auto parent = retired_paragraph.parent();
        if (parent != pugi::xml_node{}) {
            (void)parent.remove_child(retired_paragraph);
        }
    }
    return true;
}

auto replace_bookmark_text_in_part(featherdoc::document_error_info &last_error_info,
                                   featherdoc::detail::xml_handle_lifetime &handle_lifetime,
                                   pugi::xml_document &document, std::string_view entry_name,
                                   std::string_view bookmark_name,
                                   std::string_view replacement) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<pugi::xml_node> bookmark_starts;
    if (!collect_bookmark_mutation_inputs(
            last_error_info, entry_name,
            "bookmark text replacement ran out of memory while collecting targets",
            [&]() {
                collect_named_bookmark_starts(document, bookmark_name,
                                              bookmark_starts);
                return true;
            })) {
        return 0U;
    }

    const auto replaced = replace_bookmark_ranges(
        last_error_info, handle_lifetime, entry_name, bookmark_starts,
        replacement);
    if (!replaced.has_value()) {
        return 0U;
    }

    last_error_info.clear();
    return *replaced;
}

auto fill_bookmarks_in_part(featherdoc::document_error_info &last_error_info,
                            featherdoc::detail::xml_handle_lifetime &handle_lifetime,
                            pugi::xml_document &document, std::string_view entry_name,
                            std::span<const featherdoc::bookmark_text_binding> bindings)
    -> featherdoc::bookmark_fill_result {
    featherdoc::bookmark_fill_result result;
    result.requested = bindings.size();
    const auto failure_result = [&]() {
        featherdoc::bookmark_fill_result failure;
        failure.requested = bindings.size();
        return failure;
    };

    auto targets = std::vector<bookmark_text_replacement_target>{};
    try {
        std::unordered_set<std::string> seen_bookmarks;
        seen_bookmarks.reserve(bindings.size());
        result.missing_bookmarks.reserve(bindings.size());

        for (const auto &binding : bindings) {
            if (binding.bookmark_name.empty()) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "bookmark binding name must not be empty",
                    std::string{entry_name});
                return failure_result();
            }

            const auto [_, inserted] =
                seen_bookmarks.emplace(binding.bookmark_name);
            if (!inserted) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "duplicate bookmark binding name: " +
                        binding.bookmark_name,
                    std::string{entry_name});
                return failure_result();
            }

            auto bookmark_starts = std::vector<pugi::xml_node>{};
            collect_named_bookmark_starts(document, binding.bookmark_name,
                                          bookmark_starts);
            if (bookmark_starts.empty()) {
                result.missing_bookmarks.push_back(binding.bookmark_name);
                continue;
            }

            if (bookmark_starts.size() > targets.max_size() - targets.size()) {
                throw std::length_error{
                    "bookmark binding target count overflow"};
            }
            if (result.matched ==
                    std::numeric_limits<std::size_t>::max() ||
                bookmark_starts.size() >
                    std::numeric_limits<std::size_t>::max() -
                        result.replaced) {
                throw std::length_error{
                    "bookmark batch result count overflow"};
            }
            targets.reserve(targets.size() + bookmark_starts.size());
            for (const auto bookmark_start : bookmark_starts) {
                targets.push_back({bookmark_start, binding.text});
            }

            ++result.matched;
            result.replaced += bookmark_starts.size();
        }
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            last_error_info,
            "bookmark batch replacement ran out of memory while collecting "
            "targets",
            entry_name);
        return failure_result();
    } catch (const std::length_error &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            last_error_info,
            "bookmark batch replacement target count is too large", entry_name);
        return failure_result();
    }

    if (!targets.empty() &&
        !replace_bookmark_ranges(last_error_info, handle_lifetime, entry_name,
                                 targets)
             .has_value()) {
        return failure_result();
    }

    last_error_info.clear();
    return result;
}

auto validate_placeholder_requirement(
    pugi::xml_document &document, std::string_view entry_name,
    const featherdoc::template_slot_requirement &requirement) -> bool {
    if (requirement.source != featherdoc::template_slot_source_kind::bookmark) {
        return true;
    }

    featherdoc::document_error_info validation_error_info;

    switch (requirement.kind) {
    case featherdoc::template_slot_kind::text:
        return true;
    case featherdoc::template_slot_kind::table_rows: {
        std::vector<table_row_bookmark_placeholder> placeholders;
        return collect_table_row_bookmark_placeholders(
            validation_error_info, document, entry_name, requirement.bookmark_name,
            placeholders);
    }
    case featherdoc::template_slot_kind::table:
    case featherdoc::template_slot_kind::image:
    case featherdoc::template_slot_kind::floating_image:
    case featherdoc::template_slot_kind::block: {
        std::vector<block_bookmark_placeholder> placeholders;
        return collect_block_bookmark_placeholders(
            validation_error_info, document, entry_name, requirement.bookmark_name,
            placeholders);
    }
    }

    return false;
}

auto set_bookmark_block_visibility_in_part(
    featherdoc::document_error_info &last_error_info,
    featherdoc::detail::xml_handle_lifetime &handle_lifetime,
    pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name, bool visible)
    -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<bookmark_block_placeholder> placeholders;
    if (!collect_bookmark_mutation_inputs(
            last_error_info, entry_name,
            "bookmark block visibility ran out of memory while collecting targets",
            [&]() {
                return collect_bookmark_block_placeholders(
                    last_error_info, document, entry_name, bookmark_name,
                    placeholders);
            })) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    auto retired_nodes = std::vector<pugi::xml_node>{};
    try {
        if (visible) {
            if (placeholders.size() >
                std::numeric_limits<std::size_t>::max() / 2U) {
                throw std::length_error{"bookmark block retirement root count overflow"};
            }
            retired_nodes.reserve(placeholders.size() * 2U);
        }

        for (auto it = placeholders.rbegin(); it != placeholders.rend(); ++it) {
            const auto placeholder = *it;
            if (placeholder.container == pugi::xml_node{} ||
                placeholder.start_paragraph.parent() != placeholder.container ||
                placeholder.end_paragraph.parent() != placeholder.container) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "failed to resolve the bookmark block marker paragraphs",
                               std::string{entry_name});
                return 0U;
            }

            if (visible) {
                append_outermost_removal_root(retired_nodes,
                                              placeholder.end_paragraph);
                append_outermost_removal_root(retired_nodes,
                                              placeholder.start_paragraph);
            } else {
                auto node = placeholder.start_paragraph;
                bool found_end = false;
                while (node != pugi::xml_node{}) {
                    const auto next = node.next_sibling();
                    const bool is_end = node == placeholder.end_paragraph;
                    append_outermost_removal_root(retired_nodes, node);
                    if (is_end) {
                        found_end = true;
                        break;
                    }
                    node = next;
                }

                if (!found_end) {
                    set_last_error(last_error_info,
                                   std::make_error_code(std::errc::invalid_argument),
                                   "failed to locate the bookmark block end marker while "
                                   "removing the block",
                                   std::string{entry_name});
                    return 0U;
                }
            }
        }
    } catch (const std::bad_alloc &) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare bookmark block handle retirement",
                       std::string{entry_name});
        return 0U;
    } catch (const std::length_error &) {
        set_last_error(last_error_info,
                       std::make_error_code(std::errc::value_too_large),
                       "bookmark block handle retirement root count is too large",
                       std::string{entry_name});
        return 0U;
    }

    if (!retire_bookmark_subtrees(last_error_info, handle_lifetime,
                                  retired_nodes, entry_name,
                                  "bookmark block visibility update")) {
        return 0U;
    }

    for (const auto node : retired_nodes) {
        auto parent = node.parent();
        if (parent != pugi::xml_node{}) {
            (void)parent.remove_child(node);
        }
    }

    last_error_info.clear();
    return placeholders.size();
}

auto bookmark_kind_to_template_slot_kind(featherdoc::bookmark_kind kind)
    -> std::optional<featherdoc::template_slot_kind> {
    switch (kind) {
    case featherdoc::bookmark_kind::text:
        return featherdoc::template_slot_kind::text;
    case featherdoc::bookmark_kind::block:
        return featherdoc::template_slot_kind::block;
    case featherdoc::bookmark_kind::table_rows:
        return featherdoc::template_slot_kind::table_rows;
    case featherdoc::bookmark_kind::block_range:
    case featherdoc::bookmark_kind::malformed:
    case featherdoc::bookmark_kind::mixed:
        return std::nullopt;
    }

    return std::nullopt;
}

auto apply_bookmark_block_visibility_in_part(
    featherdoc::document_error_info &last_error_info,
    featherdoc::detail::xml_handle_lifetime &handle_lifetime,
    pugi::xml_document &document,
    std::string_view entry_name,
    std::span<const featherdoc::bookmark_block_visibility_binding> bindings)
    -> featherdoc::bookmark_block_visibility_result {
    featherdoc::bookmark_block_visibility_result result;
    result.requested = bindings.size();
    const auto failure_result = [&]() {
        featherdoc::bookmark_block_visibility_result failure;
        failure.requested = bindings.size();
        return failure;
    };

    struct visibility_target final {
        bookmark_block_placeholder placeholder;
        std::string_view bookmark_name;
        bool visible;
        std::size_t start_position{};
        std::size_t end_position{};
    };

    auto targets = std::vector<visibility_target>{};
    auto retired_nodes = std::vector<pugi::xml_node>{};
    try {
        std::unordered_set<std::string> seen_bookmarks;
        seen_bookmarks.reserve(bindings.size());
        result.missing_bookmarks.reserve(bindings.size());

        for (const auto &binding : bindings) {
            if (binding.bookmark_name.empty()) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "bookmark block visibility binding name must not be empty",
                    std::string{entry_name});
                return failure_result();
            }

            const auto [_, inserted] =
                seen_bookmarks.emplace(binding.bookmark_name);
            if (!inserted) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "duplicate bookmark block visibility binding name: " +
                        binding.bookmark_name,
                    std::string{entry_name});
                return failure_result();
            }

            auto placeholders = std::vector<bookmark_block_placeholder>{};
            if (!collect_bookmark_block_placeholders(
                    last_error_info, document, entry_name,
                    binding.bookmark_name, placeholders)) {
                return failure_result();
            }
            if (placeholders.empty()) {
                result.missing_bookmarks.push_back(binding.bookmark_name);
                continue;
            }

            if (placeholders.size() > targets.max_size() - targets.size()) {
                throw std::length_error{
                    "bookmark visibility target count overflow"};
            }
            auto &updated_count =
                binding.visible ? result.kept : result.removed;
            if (result.matched ==
                    std::numeric_limits<std::size_t>::max() ||
                placeholders.size() >
                    std::numeric_limits<std::size_t>::max() -
                        updated_count) {
                throw std::length_error{
                    "bookmark visibility result count overflow"};
            }
            targets.reserve(targets.size() + placeholders.size());
            for (const auto &placeholder : placeholders) {
                targets.push_back(
                    {placeholder, binding.bookmark_name, binding.visible});
            }

            ++result.matched;
            updated_count += placeholders.size();
        }

        for (auto &target : targets) {
            const auto placeholder = target.placeholder;
            if (placeholder.container == pugi::xml_node{} ||
                placeholder.start_paragraph.parent() !=
                    placeholder.container ||
                placeholder.end_paragraph.parent() != placeholder.container) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "failed to resolve the bookmark block marker paragraphs",
                    std::string{entry_name});
                return failure_result();
            }

            bool found_start = false;
            bool found_end = false;
            std::size_t position = 0U;
            for (auto sibling = placeholder.container.first_child();
                 sibling != pugi::xml_node{};
                 sibling = sibling.next_sibling(), ++position) {
                if (sibling == placeholder.start_paragraph) {
                    target.start_position = position;
                    found_start = true;
                }
                if (sibling == placeholder.end_paragraph) {
                    target.end_position = position;
                    found_end = true;
                }
            }
            if (!found_start || !found_end ||
                target.start_position >= target.end_position) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "bookmark block visibility requires the start marker to "
                    "precede the end marker",
                    std::string{entry_name});
                return failure_result();
            }
        }

        for (std::size_t left_index = 0U; left_index < targets.size();
             ++left_index) {
            for (std::size_t right_index = left_index + 1U;
                 right_index < targets.size(); ++right_index) {
                const auto &left = targets[left_index];
                const auto &right = targets[right_index];
                if (left.visible == right.visible ||
                    left.placeholder.container !=
                        right.placeholder.container) {
                    continue;
                }

                const auto &hidden = left.visible ? right : left;
                const auto &visible = left.visible ? left : right;
                const bool disjoint =
                    hidden.end_position < visible.start_position ||
                    visible.end_position < hidden.start_position;
                const bool hidden_strictly_inside_visible =
                    visible.start_position < hidden.start_position &&
                    hidden.end_position < visible.end_position;
                if (disjoint || hidden_strictly_inside_visible) {
                    continue;
                }

                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "bookmark block visibility conflict: hidden bookmark '" +
                        std::string{hidden.bookmark_name} +
                        "' overlaps visible bookmark '" +
                        std::string{visible.bookmark_name} + "'",
                    std::string{entry_name});
                return failure_result();
            }
        }

        for (auto it = targets.rbegin(); it != targets.rend(); ++it) {
            const auto &target = *it;
            const auto placeholder = target.placeholder;
            if (target.visible) {
                append_outermost_removal_root(retired_nodes,
                                              placeholder.end_paragraph);
                append_outermost_removal_root(retired_nodes,
                                              placeholder.start_paragraph);
                continue;
            }

            auto node = placeholder.start_paragraph;
            bool found_end = false;
            while (node != pugi::xml_node{}) {
                const auto next = node.next_sibling();
                const bool is_end = node == placeholder.end_paragraph;
                append_outermost_removal_root(retired_nodes, node);
                if (is_end) {
                    found_end = true;
                    break;
                }
                node = next;
            }

            if (!found_end) {
                set_last_error(
                    last_error_info,
                    std::make_error_code(std::errc::invalid_argument),
                    "failed to locate the bookmark block end marker while "
                    "removing the block",
                    std::string{entry_name});
                return failure_result();
            }
        }
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            last_error_info,
            "bookmark block visibility batch ran out of memory while "
            "preparing targets",
            entry_name);
        return failure_result();
    } catch (const std::length_error &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            last_error_info,
            "bookmark block visibility batch target count is too large",
            entry_name);
        return failure_result();
    }

    if (!retire_bookmark_subtrees(last_error_info, handle_lifetime,
                                  retired_nodes, entry_name,
                                  "bookmark block visibility batch update")) {
        return failure_result();
    }

    for (const auto node : retired_nodes) {
        auto parent = node.parent();
        if (parent != pugi::xml_node{}) {
            (void)parent.remove_child(node);
        }
    }

    last_error_info.clear();
    return result;
}

auto replace_bookmark_with_paragraphs_in_part(
    featherdoc::document_error_info &last_error_info,
    featherdoc::detail::xml_handle_lifetime &handle_lifetime,
    pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::string> &paragraphs) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_bookmark_mutation_inputs(
            last_error_info, entry_name,
            "bookmark paragraph replacement ran out of memory while collecting targets",
            [&]() {
                return collect_block_bookmark_placeholders(
                    last_error_info, document, entry_name, bookmark_name,
                    placeholders);
            })) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    if (!replace_placeholders_with_plain_text_paragraphs(
            last_error_info, handle_lifetime, entry_name, placeholders,
            paragraphs)) {
        return 0U;
    }

    last_error_info.clear();
    return placeholders.size();
}

auto remove_bookmark_block_in_part(featherdoc::document_error_info &last_error_info,
                                   featherdoc::detail::xml_handle_lifetime &handle_lifetime,
                                   pugi::xml_document &document,
                                   std::string_view entry_name,
                                   std::string_view bookmark_name)
    -> std::size_t {
    return replace_bookmark_with_paragraphs_in_part(
        last_error_info, handle_lifetime, document, entry_name, bookmark_name,
        {});
}

auto replace_bookmark_with_table_rows_in_part(
    featherdoc::document_error_info &last_error_info,
    featherdoc::detail::xml_handle_lifetime &handle_lifetime,
    pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::vector<std::string>> &rows) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<table_row_bookmark_placeholder> placeholders;
    if (!collect_bookmark_mutation_inputs(
            last_error_info, entry_name,
            "bookmark table-row replacement ran out of memory while collecting targets",
            [&]() {
                return collect_table_row_bookmark_placeholders(
                    last_error_info, document, entry_name, bookmark_name,
                    placeholders);
            })) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    if (!replace_placeholders_with_table_rows(
            last_error_info, handle_lifetime, entry_name, placeholders, rows)) {
        return 0U;
    }

    last_error_info.clear();
    return placeholders.size();
}

auto replace_bookmark_with_table_in_part(
    featherdoc::document_error_info &last_error_info,
    featherdoc::detail::xml_handle_lifetime &handle_lifetime,
    pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::vector<std::string>> &rows) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    if (rows.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "replacement table must contain at least one row",
                       std::string{entry_name});
        return 0U;
    }

    for (const auto &row : rows) {
        if (row.empty()) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "replacement table rows must contain at least one cell",
                           std::string{entry_name});
            return 0U;
        }
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_bookmark_mutation_inputs(
            last_error_info, entry_name,
            "bookmark table replacement ran out of memory while collecting targets",
            [&]() {
                return collect_block_bookmark_placeholders(
                    last_error_info, document, entry_name, bookmark_name,
                    placeholders);
            })) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    if (!insert_tables_before_placeholders(last_error_info, handle_lifetime,
                                           entry_name, placeholders, rows)) {
        return 0U;
    }

    last_error_info.clear();
    return placeholders.size();
}
