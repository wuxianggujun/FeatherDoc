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
