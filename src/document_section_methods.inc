bool Document::append_section(bool inherit_header_footer) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    auto body_section_properties = body.child("w:sectPr");
    if (body_section_properties == pugi::xml_node{}) {
        body_section_properties = body.append_child("w:sectPr");
    }
    if (body_section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create body-level w:sectPr while appending a section",
                       std::string{document_xml_entry});
        return false;
    }

    const auto first_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page);
    const bool body_has_first_page_header = section_has_reference_type(
        body_section_properties, "w:headerReference", first_page_reference_type);
    const bool body_has_first_page_footer = section_has_reference_type(
        body_section_properties, "w:footerReference", first_page_reference_type);
    if ((body_has_first_page_header || body_has_first_page_footer) &&
        !this->ensure_title_page_enabled(body_section_properties)) {
        return false;
    }

    auto section_break_paragraph = featherdoc::detail::append_paragraph_node(body);
    if (section_break_paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert a paragraph for the new section break",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_properties = section_break_paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        paragraph_properties = section_break_paragraph.append_child("w:pPr");
    }
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for the new section break paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    if (paragraph_properties.append_copy(body_section_properties) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to copy body-level w:sectPr onto the new section break paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    if (!inherit_header_footer) {
        clear_section_header_footer_references(body_section_properties);
    }

    this->cleanup_first_page_section_markers();
    const auto even_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page);
    const bool has_any_even_header = document_has_reference_type(
        this->document, "w:headerReference", even_page_reference_type);
    const bool has_any_even_footer = document_has_reference_type(
        this->document, "w:footerReference", even_page_reference_type);
    if (has_any_even_header || has_any_even_footer) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return false;
        }
    } else if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::insert_section(std::size_t section_index, bool inherit_header_footer) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inserting a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    const auto count = this->section_count();
    if (section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for insertion",
                       std::string{document_xml_entry});
        return false;
    }

    if (section_index + 1U == count) {
        return this->append_section(inherit_header_footer);
    }

    auto source_section_properties = this->section_properties(section_index);
    if (source_section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the source section break for insertion",
                       std::string{document_xml_entry});
        return false;
    }

    const auto source_break_paragraph = section_break_paragraph_for(source_section_properties);
    if (source_break_paragraph == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, std::make_error_code(std::errc::invalid_argument),
            "the target section does not end with a paragraph-level w:sectPr node",
            std::string{document_xml_entry});
        return false;
    }

    const auto first_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page);
    const bool source_has_first_page_header = section_has_reference_type(
        source_section_properties, "w:headerReference", first_page_reference_type);
    const bool source_has_first_page_footer = section_has_reference_type(
        source_section_properties, "w:footerReference", first_page_reference_type);
    if ((source_has_first_page_header || source_has_first_page_footer) &&
        !this->ensure_title_page_enabled(source_section_properties)) {
        return false;
    }

    auto section_break_paragraph = body.insert_child_after("w:p", source_break_paragraph);
    if (section_break_paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert a paragraph for the new section break",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_properties = section_break_paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        paragraph_properties = section_break_paragraph.append_child("w:pPr");
    }
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for the inserted section break paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    auto inserted_section_properties = paragraph_properties.append_copy(source_section_properties);
    if (inserted_section_properties == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, std::make_error_code(std::errc::not_enough_memory),
            "failed to copy w:sectPr onto the inserted section break paragraph",
            std::string{document_xml_entry});
        return false;
    }

    if (!inherit_header_footer) {
        clear_section_header_footer_references(inserted_section_properties);
    }

    this->cleanup_first_page_section_markers();
    const auto even_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page);
    const bool has_any_even_header = document_has_reference_type(
        this->document, "w:headerReference", even_page_reference_type);
    const bool has_any_even_footer = document_has_reference_type(
        this->document, "w:footerReference", even_page_reference_type);
    if (has_any_even_header || has_any_even_footer) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return false;
        }
    } else if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::remove_section(std::size_t section_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    const auto count = this->section_count();
    if (count <= 1U) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "cannot remove the only section in the document",
                       std::string{document_xml_entry});
        return false;
    }

    if (section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for removal",
                       std::string{document_xml_entry});
        return false;
    }

    if (section_index + 1U < count) {
        auto section_properties = this->section_properties(section_index);
        if (section_properties == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the section break for removal",
                           std::string{document_xml_entry});
            return false;
        }

        auto paragraph_properties = section_properties.parent();
        auto section_break_paragraph_node = section_break_paragraph_for(section_properties);
        if (paragraph_properties == pugi::xml_node{} ||
            section_break_paragraph_node == pugi::xml_node{} ||
            !paragraph_properties.remove_child(section_properties)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the section break from the target paragraph",
                           std::string{document_xml_entry});
            return false;
        }

        remove_empty_paragraph(section_break_paragraph_node);
    } else {
        auto previous_section_properties = this->section_properties(section_index - 1U);
        auto body_section_properties = body.child("w:sectPr");
        if (previous_section_properties == pugi::xml_node{} ||
            body_section_properties == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the final section break for removal",
                           std::string{document_xml_entry});
            return false;
        }

        if (!replace_section_properties_contents(body_section_properties,
                                                previous_section_properties)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to copy the previous section properties onto body-level w:sectPr",
                           std::string{document_xml_entry});
            return false;
        }

        auto previous_paragraph_properties = previous_section_properties.parent();
        auto previous_break_paragraph =
            section_break_paragraph_for(previous_section_properties);
        if (previous_paragraph_properties == pugi::xml_node{} ||
            previous_break_paragraph == pugi::xml_node{} ||
            !previous_paragraph_properties.remove_child(previous_section_properties)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the previous section break while collapsing the final section",
                           std::string{document_xml_entry});
            return false;
        }

        remove_empty_paragraph(previous_break_paragraph);
    }

    this->cleanup_first_page_section_markers();
    if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::move_section(std::size_t source_section_index,
                            std::size_t target_section_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before moving a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    const auto count = this->section_count();
    if (source_section_index >= count || target_section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for move",
                       std::string{document_xml_entry});
        return false;
    }

    if (source_section_index == target_section_index) {
        this->last_error_info.clear();
        return true;
    }

    auto snapshots = collect_section_snapshots(body);
    if (!snapshots.has_value() || snapshots->size() != count) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to snapshot document sections for reordering",
                       std::string{document_xml_entry});
        return false;
    }

    auto reordered_snapshots = std::move(*snapshots);
    auto moved_snapshot =
        std::move(reordered_snapshots[source_section_index]);
    reordered_snapshots.erase(reordered_snapshots.begin() +
                              static_cast<std::ptrdiff_t>(source_section_index));
    reordered_snapshots.insert(reordered_snapshots.begin() +
                                   static_cast<std::ptrdiff_t>(target_section_index),
                               std::move(moved_snapshot));

    if (!rebuild_body_from_section_snapshots(body, reordered_snapshots)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to rebuild w:body while reordering sections",
                       std::string{document_xml_entry});
        return false;
    }

    this->paragraph.set_parent(body);
    this->table.set_owner(this);
    this->table.set_parent(body);
    this->cleanup_first_page_section_markers();

    const auto even_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::even_page);
    const bool has_any_even_header = document_has_reference_type(
        this->document, "w:headerReference", even_page_reference_type);
    const bool has_any_even_footer = document_has_reference_type(
        this->document, "w:footerReference", even_page_reference_type);
    if (has_any_even_header || has_any_even_footer) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return false;
        }
    } else if (const auto error = this->cleanup_even_and_odd_headers_setting()) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}


std::optional<section_page_setup> Document::get_section_page_setup(
    std::size_t section_index) const {
    this->last_error_info.clear();

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inspecting section page setup");
        return std::nullopt;
    }

    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    if (section_index >= this->section_count()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for page setup inspection",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    const auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    const auto page_size = section_properties.child("w:pgSz");
    const auto page_margins = section_properties.child("w:pgMar");
    if (page_size == pugi::xml_node{} || page_margins == pugi::xml_node{}) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    auto require_uint32_attribute =
        [this](pugi::xml_node node, const char *attribute_name, std::uint32_t &value)
        -> bool {
        const auto status = parse_xml_uint32_attribute(node, attribute_name, value);
        if (status == xml_uint_attribute_status::ok) {
            return true;
        }

        std::string detail = "invalid section page setup attribute ";
        detail += node.name();
        detail += '/';
        detail += attribute_name;
        if (status == xml_uint_attribute_status::missing) {
            detail += " (missing)";
        } else {
            detail += " (expected unsigned integer)";
        }

        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::move(detail), std::string{document_xml_entry});
        return false;
    };

    featherdoc::section_page_setup result;
    if (!require_uint32_attribute(page_size, "w:w", result.width_twips) ||
        !require_uint32_attribute(page_size, "w:h", result.height_twips) ||
        !require_uint32_attribute(page_margins, "w:top", result.margins.top_twips) ||
        !require_uint32_attribute(page_margins, "w:bottom",
                                  result.margins.bottom_twips) ||
        !require_uint32_attribute(page_margins, "w:left", result.margins.left_twips) ||
        !require_uint32_attribute(page_margins, "w:right",
                                  result.margins.right_twips) ||
        !require_uint32_attribute(page_margins, "w:header",
                                  result.margins.header_twips) ||
        !require_uint32_attribute(page_margins, "w:footer",
                                  result.margins.footer_twips)) {
        return std::nullopt;
    }

    const auto orientation =
        std::string_view{page_size.attribute("w:orient").value()};
    if (orientation.empty() || orientation == "portrait") {
        result.orientation = featherdoc::page_orientation::portrait;
    } else if (orientation == "landscape") {
        result.orientation = featherdoc::page_orientation::landscape;
    } else {
        std::string detail = "unsupported section page orientation: ";
        detail += orientation;
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::move(detail), std::string{document_xml_entry});
        return std::nullopt;
    }

    const auto page_numbering = section_properties.child("w:pgNumType");
    if (page_numbering != pugi::xml_node{}) {
        std::uint32_t page_number_start = 0U;
        switch (parse_xml_uint32_attribute(page_numbering, "w:start",
                                           page_number_start)) {
        case xml_uint_attribute_status::ok:
            result.page_number_start = page_number_start;
            break;
        case xml_uint_attribute_status::missing:
            break;
        case xml_uint_attribute_status::invalid:
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "invalid section page setup attribute w:pgNumType/w:start "
                           "(expected unsigned integer)",
                           std::string{document_xml_entry});
            return std::nullopt;
        }
    }

    this->last_error_info.clear();
    return result;
}

bool Document::set_section_page_setup(std::size_t section_index,
                                      const section_page_setup &setup) {
    this->last_error_info.clear();

    if (setup.width_twips == 0U || setup.height_twips == 0U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "section page width and height must be greater than zero",
                       std::string{document_xml_entry});
        return false;
    }

    auto section_properties = this->ensure_section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return false;
    }

    auto page_size = ensure_section_property_node(section_properties, "w:pgSz");
    if (page_size == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pgSz for section page setup",
                       std::string{document_xml_entry});
        return false;
    }

    auto page_margins = ensure_section_property_node(section_properties, "w:pgMar");
    if (page_margins == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pgMar for section page setup",
                       std::string{document_xml_entry});
        return false;
    }

    if (!ensure_xml_uint32_attribute(page_size, "w:w", setup.width_twips) ||
        !ensure_xml_uint32_attribute(page_size, "w:h", setup.height_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:top", setup.margins.top_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:bottom",
                                     setup.margins.bottom_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:left",
                                     setup.margins.left_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:right",
                                     setup.margins.right_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:header",
                                     setup.margins.header_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:footer",
                                     setup.margins.footer_twips)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to write section page setup attributes",
                       std::string{document_xml_entry});
        return false;
    }

    if (setup.orientation == featherdoc::page_orientation::landscape) {
        auto orientation_attribute = page_size.attribute("w:orient");
        if (orientation_attribute == pugi::xml_attribute{}) {
            orientation_attribute = page_size.append_attribute("w:orient");
        }
        if (orientation_attribute == pugi::xml_attribute{} ||
            !orientation_attribute.set_value("landscape")) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to write w:pgSz/w:orient for section page setup",
                           std::string{document_xml_entry});
            return false;
        }
    } else {
        page_size.remove_attribute("w:orient");
    }

    auto page_numbering = section_properties.child("w:pgNumType");
    if (setup.page_number_start.has_value()) {
        if (page_numbering == pugi::xml_node{}) {
            page_numbering = ensure_section_property_node(section_properties, "w:pgNumType");
        }
        if (page_numbering == pugi::xml_node{} ||
            !ensure_xml_uint32_attribute(page_numbering, "w:start",
                                         *setup.page_number_start)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to write w:pgNumType/w:start for section page setup",
                           std::string{document_xml_entry});
            return false;
        }
    } else if (page_numbering != pugi::xml_node{}) {
        page_numbering.remove_attribute("w:start");
        remove_empty_node(page_numbering);
    }

    this->last_error_info.clear();
    return true;
}

pugi::xml_node Document::section_properties(std::size_t section_index) const {
    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        return {};
    }

    std::size_t current_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        const auto paragraph_section_properties = child.child("w:pPr").child("w:sectPr");
        if (paragraph_section_properties == pugi::xml_node{}) {
            continue;
        }

        if (current_index == section_index) {
            return paragraph_section_properties;
        }
        ++current_index;
    }

    if (current_index == section_index) {
        return body.child("w:sectPr");
    }

    return {};
}

pugi::xml_node Document::ensure_section_properties(std::size_t section_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing section properties");
        return {};
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return {};
    }

    const auto count = this->section_count();
    if (section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for section editing",
                       std::string{document_xml_entry});
        return {};
    }

    if (const auto existing = this->section_properties(section_index);
        existing != pugi::xml_node{}) {
        return existing;
    }

    if (section_index + 1U != count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "the target section does not expose an editable w:sectPr node",
                       std::string{document_xml_entry});
        return {};
    }

    auto section_properties = body.child("w:sectPr");
    if (section_properties != pugi::xml_node{}) {
        return section_properties;
    }

    section_properties = body.append_child("w:sectPr");
    if (section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:sectPr for section editing",
                       std::string{document_xml_entry});
        return {};
    }

    return section_properties;
}

Paragraph &Document::header_paragraphs(std::size_t index) {
    if (index >= this->header_parts.size()) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    auto &part = *this->header_parts[index];
    part.paragraph.set_parent(part.xml.child("w:hdr"));
    return part.paragraph;
}

Paragraph &Document::footer_paragraphs(std::size_t index) {
    if (index >= this->footer_parts.size()) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    auto &part = *this->footer_parts[index];
    part.paragraph.set_parent(part.xml.child("w:ftr"));
    return part.paragraph;
}

Paragraph &Document::related_part_paragraphs_by_relationship_id(
    std::string_view relationship_id, std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *part_root_name) {
    if (relationship_id.empty()) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    for (auto &part : parts) {
        if (part->relationship_id == relationship_id) {
            part->paragraph.set_parent(part->xml.child(part_root_name));
            return part->paragraph;
        }
    }

    this->detached_paragraph.set_parent({});
    return this->detached_paragraph;
}

Paragraph &Document::section_related_part_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *reference_name,
    const char *part_root_name) {
    auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        this->detached_paragraph.set_parent({});
        return this->detached_paragraph;
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    for (auto reference = section_properties.child(reference_name);
         reference != pugi::xml_node{};
         reference = reference.next_sibling(reference_name)) {
        if (std::string_view{reference.attribute("w:type").value()} != xml_reference_type) {
            continue;
        }

        return this->related_part_paragraphs_by_relationship_id(
            reference.attribute("r:id").value(), parts, part_root_name);
    }

    this->detached_paragraph.set_parent({});
    return this->detached_paragraph;
}

Document::xml_part_state *Document::section_related_part_state(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *reference_name) {
    const auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return nullptr;
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    for (auto reference = section_properties.child(reference_name);
         reference != pugi::xml_node{};
         reference = reference.next_sibling(reference_name)) {
        if (std::string_view{reference.attribute("w:type").value()} != xml_reference_type) {
            continue;
        }

        const auto relationship_id = std::string_view{reference.attribute("r:id").value()};
        if (relationship_id.empty()) {
            return nullptr;
        }

        for (auto &part : parts) {
            if (part->relationship_id == relationship_id) {
                return part.get();
            }
        }

        return nullptr;
    }

    return nullptr;
}

Document::xml_part_state *Document::find_related_part_state(std::string_view entry_name) {
    for (auto &part : this->header_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    for (auto &part : this->footer_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    return nullptr;
}

const Document::xml_part_state *Document::find_related_part_state(
    std::string_view entry_name) const {
    for (const auto &part : this->header_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    for (const auto &part : this->footer_parts) {
        if (part->entry_name == entry_name) {
            return part.get();
        }
    }

    return nullptr;
}

Paragraph &Document::section_header_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->section_related_part_paragraphs(
        section_index, reference_kind, this->header_parts, "w:headerReference", "w:hdr");
}

Paragraph &Document::section_footer_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind) {
    return this->section_related_part_paragraphs(
        section_index, reference_kind, this->footer_parts, "w:footerReference", "w:ftr");
}

std::error_code Document::ensure_even_and_odd_headers_enabled() {
    if (const auto error = this->ensure_settings_part_attached()) {
        return error;
    }

    auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::settings_xml_parse_failed,
                              "word/settings.xml does not contain the expected w:settings root",
                              std::string{settings_xml_entry});
    }

    if (ensure_on_off_node_enabled(settings_root, "w:evenAndOddHeaders") ==
        pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              std::make_error_code(std::errc::not_enough_memory),
                              "failed to enable w:evenAndOddHeaders in word/settings.xml",
                              std::string{settings_xml_entry});
    }

    this->settings_dirty = true;
    return {};
}

bool Document::ensure_title_page_enabled(pugi::xml_node section_properties) {
    if (section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "cannot enable first-page header/footer without a w:sectPr node",
                       std::string{document_xml_entry});
        return false;
    }

    if (ensure_section_title_page_node(section_properties) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to enable w:titlePg for the target section",
                       std::string{document_xml_entry});
        return false;
    }

    return true;
}

Paragraph &Document::ensure_related_part_paragraphs(
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
    const char *reference_name, const char *relationship_type,
    const char *content_type) {
    this->detached_paragraph.set_parent({});

    const auto count = this->section_count();
    const auto last_section_index = count == 0U ? 0U : count - 1U;
    return this->ensure_section_related_part_paragraphs(
        last_section_index, featherdoc::section_reference_kind::default_reference, parts,
        part_root_name, reference_name, relationship_type, content_type);
}

Paragraph &Document::ensure_section_related_part_paragraphs(
    std::size_t section_index, featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
    const char *reference_name, const char *relationship_type,
    const char *content_type) {
    this->detached_paragraph.set_parent({});

    const auto section_properties = this->ensure_section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::first_page &&
        !this->ensure_title_page_enabled(section_properties)) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::even_page) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return this->detached_paragraph;
        }
    }

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    auto reference =
        find_section_reference(section_properties, reference_name, xml_reference_type);
    if (reference != pugi::xml_node{}) {
        const auto relationship_id = std::string_view{reference.attribute("r:id").value()};
        if (!relationship_id.empty()) {
            for (const auto &part : parts) {
                if (part->relationship_id == relationship_id) {
                    part->paragraph.set_parent(part->xml.child(part_root_name));
                    this->last_error_info.clear();
                    return part->paragraph;
                }
            }
        }
    }

    if (const auto error = this->ensure_content_types_loaded()) {
        return this->detached_paragraph;
    }

    auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document root",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    if (this->document_relationships.child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(this->document_relationships)) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "failed to initialize default relationships XML",
                       std::string{document_relationships_xml_entry});
        return this->detached_paragraph;
    }

    auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::relationships_xml_parse_failed,
                       "word/_rels/document.xml.rels does not contain a Relationships root",
                       std::string{document_relationships_xml_entry});
        return this->detached_paragraph;
    }

    auto types = this->content_types.child("Types");
    if (types == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::content_types_xml_parse_failed,
                       "[Content_Types].xml does not contain a Types root",
                       std::string{content_types_xml_entry});
        return this->detached_paragraph;
    }

    auto relationships_namespace = document_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = document_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    std::unordered_set<std::string> used_relationship_ids;
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        const auto id = relationship.attribute("Id").value();
        if (*id != '\0') {
            used_relationship_ids.emplace(id);
        }
    }

    std::string relationship_id;
    for (std::size_t next_index = 1U; relationship_id.empty(); ++next_index) {
        auto candidate = "rId" + std::to_string(next_index);
        if (!used_relationship_ids.contains(candidate)) {
            relationship_id = std::move(candidate);
        }
    }

    const auto part_prefix =
        std::string_view{part_root_name} == "w:hdr" ? "header" : "footer";
    std::unordered_set<std::string> used_part_entries;
    for (const auto &part : parts) {
        used_part_entries.insert(part->entry_name);
    }

    std::string entry_name;
    for (std::size_t next_index = 1U; entry_name.empty(); ++next_index) {
        auto candidate = std::string{"word/"} + part_prefix +
                         std::to_string(next_index) + ".xml";
        if (!used_part_entries.contains(candidate)) {
            entry_name = std::move(candidate);
        }
    }

    auto part = std::make_unique<xml_part_state>();
    part->relationship_id = relationship_id;
    part->entry_name = entry_name;
    part->relationships_entry_name = make_part_relationships_entry(entry_name);
    const auto empty_part_xml =
        std::string_view{part_root_name} == "w:hdr" ? empty_header_xml : empty_footer_xml;
    const auto parse_result =
        part->xml.load_buffer(empty_part_xml.data(), empty_part_xml.size());
    if (!parse_result) {
        set_last_error(this->last_error_info, document_errc::related_part_parse_failed,
                       parse_result.description(), entry_name,
                       parse_result.offset >= 0
                           ? std::optional<std::ptrdiff_t>{parse_result.offset}
                           : std::nullopt);
        return this->detached_paragraph;
    }

    auto relationship = relationships.append_child("Relationship");
    if (relationship == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create a new document relationship for header/footer",
                       std::string{document_relationships_xml_entry});
        return this->detached_paragraph;
    }

    relationship.append_attribute("Id").set_value(relationship_id.c_str());
    relationship.append_attribute("Type").set_value(relationship_type);
    const auto relationship_target = make_document_relationship_target(entry_name);
    relationship.append_attribute("Target").set_value(relationship_target.c_str());

    const auto override_part_name = make_override_part_name(entry_name);
    auto override_node = pugi::xml_node{};
    for (auto existing_override = types.child("Override");
         existing_override != pugi::xml_node{};
         existing_override = existing_override.next_sibling("Override")) {
        if (std::string_view{existing_override.attribute("PartName").value()} ==
            override_part_name) {
            override_node = existing_override;
            break;
        }
    }
    if (override_node == pugi::xml_node{}) {
        override_node = types.append_child("Override");
    }
    if (override_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append a content type override for header/footer",
                       std::string{content_types_xml_entry});
        return this->detached_paragraph;
    }
    auto part_name_attribute = override_node.attribute("PartName");
    if (part_name_attribute == pugi::xml_attribute{}) {
        part_name_attribute = override_node.append_attribute("PartName");
    }
    part_name_attribute.set_value(override_part_name.c_str());

    auto content_type_attribute = override_node.attribute("ContentType");
    if (content_type_attribute == pugi::xml_attribute{}) {
        content_type_attribute = override_node.append_attribute("ContentType");
    }
    content_type_attribute.set_value(content_type);

    if (reference == pugi::xml_node{}) {
        reference = append_section_reference(section_properties, reference_name);
    }

    if (reference == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to attach a section header/footer reference to w:sectPr",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    auto type_attribute = reference.attribute("w:type");
    if (type_attribute == pugi::xml_attribute{}) {
        type_attribute = reference.append_attribute("w:type");
    }
    type_attribute.set_value(xml_reference_type.data());

    auto relationship_attribute = reference.attribute("r:id");
    if (relationship_attribute == pugi::xml_attribute{}) {
        relationship_attribute = reference.append_attribute("r:id");
    }
    relationship_attribute.set_value(relationship_id.c_str());

    parts.push_back(std::move(part));
    auto &created_part = *parts.back();
    created_part.paragraph.set_parent(created_part.xml.child(part_root_name));

    this->has_document_relationships_part = true;
    this->document_relationships_dirty = true;
    this->content_types_dirty = true;
    this->last_error_info.clear();
    return created_part.paragraph;
}

Paragraph &Document::assign_section_related_part_paragraphs(
    std::size_t section_index, std::size_t part_index,
    featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
    const char *reference_name) {
    this->detached_paragraph.set_parent({});

    if (part_index >= parts.size()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "header/footer part index is out of range for section assignment",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    const auto section_properties = this->ensure_section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::first_page &&
        !this->ensure_title_page_enabled(section_properties)) {
        return this->detached_paragraph;
    }

    if (reference_kind == featherdoc::section_reference_kind::even_page) {
        if (const auto error = this->ensure_even_and_odd_headers_enabled()) {
            return this->detached_paragraph;
        }
    }

    auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document root",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    auto relationships_namespace = document_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = document_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    const auto xml_reference_type = featherdoc::to_xml_reference_type(reference_kind);
    auto reference =
        find_section_reference(section_properties, reference_name, xml_reference_type);
    if (reference == pugi::xml_node{}) {
        reference = append_section_reference(section_properties, reference_name);
    }

    if (reference == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to attach a section header/footer reference to w:sectPr",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    auto &part = *parts[part_index];
    if (part.relationship_id.empty()) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "target header/footer part does not have a relationship id",
                       part.entry_name);
        return this->detached_paragraph;
    }

    auto type_attribute = reference.attribute("w:type");
    if (type_attribute == pugi::xml_attribute{}) {
        type_attribute = reference.append_attribute("w:type");
    }
    type_attribute.set_value(xml_reference_type.data());

    auto relationship_attribute = reference.attribute("r:id");
    if (relationship_attribute == pugi::xml_attribute{}) {
        relationship_attribute = reference.append_attribute("r:id");
    }
    relationship_attribute.set_value(part.relationship_id.c_str());

    part.paragraph.set_parent(part.xml.child(part_root_name));
    this->last_error_info.clear();
    return part.paragraph;
}

#include "document_section_related_part_methods.inc"

#include "document_section_text_replacement_methods.inc"
