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

auto replace_placeholder_with_table_rows(
    featherdoc::document_error_info &last_error_info,
    std::string_view entry_name, table_row_bookmark_placeholder placeholder,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    if (placeholder.table == pugi::xml_node{} || placeholder.row == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder row for table row replacement",
                       std::string{entry_name});
        return false;
    }

    if (placeholder.row.parent() != placeholder.table) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder row parent table",
                       std::string{entry_name});
        return false;
    }

    for (const auto &row_values : rows) {
        if (row_values.size() != placeholder.cell_count) {
            set_last_error(
                last_error_info, std::make_error_code(std::errc::invalid_argument),
                "replacement table row cell count must exactly match the template row cell "
                "count",
                std::string{entry_name});
            return false;
        }
    }

    for (const auto &row_values : rows) {
        auto row_copy = placeholder.table.insert_copy_before(placeholder.row, placeholder.row);
        if (row_copy == pugi::xml_node{}) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to clone the template row before the bookmark row",
                           std::string{entry_name});
            return false;
        }

        std::size_t cell_index = 0U;
        for (auto cell = row_copy.child("w:tc"); cell != pugi::xml_node{};
             cell = featherdoc::detail::next_named_sibling(cell, "w:tc"), ++cell_index) {
            if (!rewrite_table_cell_plain_text(cell, row_values[cell_index])) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to rewrite a cloned template row cell",
                               std::string{entry_name});
                return false;
            }
        }
    }

    if (!placeholder.table.remove_child(placeholder.row)) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove the bookmark placeholder row after table row "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto insert_table_before_placeholder(
    featherdoc::document_error_info &last_error_info, std::string_view entry_name,
    pugi::xml_node placeholder_paragraph,
    const std::vector<std::vector<std::string>> &rows) -> bool {
    auto parent = placeholder_paragraph.parent();
    if (placeholder_paragraph == pugi::xml_node{} || parent == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder paragraph for table replacement",
                       std::string{entry_name});
        return false;
    }

    auto table_node = parent.insert_child_before("w:tbl", placeholder_paragraph);
    if (table_node == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert the replacement table before the bookmark paragraph",
                       std::string{entry_name});
        return false;
    }

    featherdoc::Table table(parent, table_node);
    for (const auto &row_values : rows) {
        auto row = table.append_row(row_values.size());
        if (!row.has_next()) {
            set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                           "failed to append a replacement table row",
                           std::string{entry_name});
            return false;
        }

        auto cell = row.cells();
        for (const auto &cell_text : row_values) {
            if (!cell.has_next()) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to resolve a replacement table cell",
                               std::string{entry_name});
                return false;
            }

            if (!cell_text.empty() && !cell.paragraphs().add_run(cell_text).has_next()) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to append replacement table text",
                               std::string{entry_name});
                return false;
            }

            cell.next();
        }
    }

    if (!parent.remove_child(placeholder_paragraph)) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove the bookmark placeholder paragraph after table "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto replace_placeholder_with_plain_text_paragraphs(
    featherdoc::document_error_info &last_error_info, std::string_view entry_name,
    pugi::xml_node placeholder_paragraph,
    const std::vector<std::string> &paragraphs) -> bool {
    auto parent = placeholder_paragraph.parent();
    if (placeholder_paragraph == pugi::xml_node{} || parent == pugi::xml_node{}) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the placeholder paragraph for paragraph "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    for (const auto &paragraph_text : paragraphs) {
        if (!featherdoc::detail::insert_plain_text_paragraph(
                parent, placeholder_paragraph, paragraph_text)) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to insert a replacement paragraph before the bookmark "
                           "placeholder",
                           std::string{entry_name});
            return false;
        }
    }

    if (!parent.remove_child(placeholder_paragraph)) {
        set_last_error(last_error_info, std::make_error_code(std::errc::not_enough_memory),
                       "failed to remove the bookmark placeholder paragraph after paragraph "
                       "replacement",
                       std::string{entry_name});
        return false;
    }

    return true;
}

auto replace_bookmark_text_in_part(featherdoc::document_error_info &last_error_info,
                                   pugi::xml_document &document, std::string_view entry_name,
                                   std::string_view bookmark_name,
                                   std::string_view replacement) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<pugi::xml_node> bookmark_starts;
    collect_named_bookmark_starts(document, bookmark_name, bookmark_starts);

    std::size_t replaced = 0U;
    for (const auto bookmark_start : bookmark_starts) {
        if (replace_bookmark_range(bookmark_start, replacement)) {
            ++replaced;
        }
    }

    last_error_info.clear();
    return replaced;
}

auto fill_bookmarks_in_part(featherdoc::document_error_info &last_error_info,
                            pugi::xml_document &document, std::string_view entry_name,
                            std::span<const featherdoc::bookmark_text_binding> bindings)
    -> featherdoc::bookmark_fill_result {
    featherdoc::bookmark_fill_result result;
    result.requested = bindings.size();

    std::unordered_set<std::string> seen_bookmarks;
    for (const auto &binding : bindings) {
        if (binding.bookmark_name.empty()) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark binding name must not be empty",
                           std::string{entry_name});
            return result;
        }

        const auto [_, inserted] = seen_bookmarks.emplace(binding.bookmark_name);
        if (!inserted) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "duplicate bookmark binding name: " + binding.bookmark_name,
                           std::string{entry_name});
            return result;
        }
    }

    for (const auto &binding : bindings) {
        const auto replaced = replace_bookmark_text_in_part(
            last_error_info, document, entry_name, binding.bookmark_name, binding.text);
        if (replaced == 0U) {
            result.missing_bookmarks.push_back(binding.bookmark_name);
            continue;
        }

        ++result.matched;
        result.replaced += replaced;
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
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name, bool visible)
    -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<bookmark_block_placeholder> placeholders;
    if (!collect_bookmark_block_placeholders(last_error_info, document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (auto it = placeholders.rbegin(); it != placeholders.rend(); ++it) {
        auto placeholder = *it;
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
            if (!placeholder.container.remove_child(placeholder.end_paragraph) ||
                !placeholder.container.remove_child(placeholder.start_paragraph)) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::not_enough_memory),
                               "failed to remove bookmark block marker paragraphs",
                               std::string{entry_name});
                return 0U;
            }
        } else {
            auto node = placeholder.start_paragraph;
            bool removed_end = false;
            while (node != pugi::xml_node{}) {
                const auto next = node.next_sibling();
                const bool is_end = node == placeholder.end_paragraph;
                if (!placeholder.container.remove_child(node)) {
                    set_last_error(last_error_info,
                                   std::make_error_code(std::errc::not_enough_memory),
                                   "failed to remove nodes inside the bookmark block range",
                                   std::string{entry_name});
                    return 0U;
                }
                if (is_end) {
                    removed_end = true;
                    break;
                }
                node = next;
            }

            if (!removed_end) {
                set_last_error(last_error_info,
                               std::make_error_code(std::errc::invalid_argument),
                               "failed to locate the bookmark block end marker while "
                               "removing the block",
                               std::string{entry_name});
                return 0U;
            }
        }

        ++replaced;
    }

    last_error_info.clear();
    return replaced;
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
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name,
    std::span<const featherdoc::bookmark_block_visibility_binding> bindings)
    -> featherdoc::bookmark_block_visibility_result {
    featherdoc::bookmark_block_visibility_result result;
    result.requested = bindings.size();

    std::unordered_set<std::string> seen_bookmarks;
    for (const auto &binding : bindings) {
        if (binding.bookmark_name.empty()) {
            set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                           "bookmark block visibility binding name must not be empty",
                           std::string{entry_name});
            return result;
        }

        const auto [_, inserted] = seen_bookmarks.emplace(binding.bookmark_name);
        if (!inserted) {
            set_last_error(last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "duplicate bookmark block visibility binding name: " +
                               binding.bookmark_name,
                           std::string{entry_name});
            return result;
        }
    }

    for (const auto &binding : bindings) {
        const auto replaced = set_bookmark_block_visibility_in_part(
            last_error_info, document, entry_name, binding.bookmark_name, binding.visible);
        if (replaced == 0U) {
            result.missing_bookmarks.push_back(binding.bookmark_name);
            continue;
        }

        ++result.matched;
        if (binding.visible) {
            result.kept += replaced;
        } else {
            result.removed += replaced;
        }
    }

    last_error_info.clear();
    return result;
}

auto replace_bookmark_with_paragraphs_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::string> &paragraphs) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<block_bookmark_placeholder> placeholders;
    if (!collect_block_bookmark_placeholders(last_error_info, document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        if (!replace_placeholder_with_plain_text_paragraphs(
                last_error_info, entry_name, placeholder.paragraph, paragraphs)) {
            return 0U;
        }
        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto remove_bookmark_block_in_part(featherdoc::document_error_info &last_error_info,
                                   pugi::xml_document &document,
                                   std::string_view entry_name,
                                   std::string_view bookmark_name)
    -> std::size_t {
    return replace_bookmark_with_paragraphs_in_part(last_error_info, document, entry_name,
                                                    bookmark_name, {});
}

auto replace_bookmark_with_table_rows_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
    std::string_view entry_name, std::string_view bookmark_name,
    const std::vector<std::vector<std::string>> &rows) -> std::size_t {
    if (bookmark_name.empty()) {
        set_last_error(last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "bookmark name must not be empty", std::string{entry_name});
        return 0U;
    }

    std::vector<table_row_bookmark_placeholder> placeholders;
    if (!collect_table_row_bookmark_placeholders(last_error_info, document, entry_name,
                                                 bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        if (!replace_placeholder_with_table_rows(last_error_info, entry_name, placeholder,
                                                 rows)) {
            return 0U;
        }
        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}

auto replace_bookmark_with_table_in_part(
    featherdoc::document_error_info &last_error_info, pugi::xml_document &document,
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
    if (!collect_block_bookmark_placeholders(last_error_info, document, entry_name,
                                             bookmark_name, placeholders)) {
        return 0U;
    }

    if (placeholders.empty()) {
        last_error_info.clear();
        return 0U;
    }

    std::size_t replaced = 0U;
    for (const auto &placeholder : placeholders) {
        if (!insert_table_before_placeholder(last_error_info, entry_name,
                                             placeholder.paragraph, rows)) {
            return 0U;
        }
        ++replaced;
    }

    last_error_info.clear();
    return replaced;
}
