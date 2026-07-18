namespace {

auto section_text_requires_preserved_space(std::string_view text) -> bool {
    if (text.empty()) {
        return false;
    }

    const auto is_space = [](char value) {
        return std::isspace(static_cast<unsigned char>(value)) != 0;
    };
    return is_space(text.front()) || is_space(text.back());
}

auto checked_append_section_text_paragraph(pugi::xml_node root,
                                           std::string_view text) -> bool {
    auto paragraph =
        featherdoc::detail::checked_append_xml_element(root, "w:p");
    if (paragraph == pugi::xml_node{}) {
        return false;
    }
    if (text.empty()) {
        return true;
    }

    auto run =
        featherdoc::detail::checked_append_xml_element(paragraph, "w:r");
    auto text_node =
        featherdoc::detail::checked_append_xml_element(run, "w:t");
    if (run == pugi::xml_node{} || text_node == pugi::xml_node{}) {
        return false;
    }

    if (section_text_requires_preserved_space(text) &&
        !featherdoc::detail::checked_append_xml_attribute(
            text_node, "xml:space", "preserve")) {
        return false;
    }

    auto text_content = text_node.append_child(pugi::node_pcdata);
    if (text_content == pugi::xml_node{}) {
        return false;
    }
    const auto text_buffer = std::string{text};
    return text_content.set_value(text_buffer.c_str());
}

} // namespace

bool Document::replace_section_related_part_text(
    xml_part_state &part, const char *part_root_name,
    std::string_view replacement_text) {
    try {
        this->ensure_xml_handle_lifetime();
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, part.entry_name);
        return false;
    }

    pugi::xml_document replacement_xml;
    if (featherdoc::detail::checked_clone_xml_document(part.xml,
                                                       replacement_xml) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, part.entry_name);
        return false;
    }

    auto replacement_root = replacement_xml.child(part_root_name);
    if (replacement_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::related_part_parse_failed,
                       "section header/footer part does not contain the "
                       "expected root",
                       part.entry_name);
        return false;
    }
    replacement_root.remove_children();

    try {
        const auto paragraphs = split_plain_text_paragraphs(replacement_text);
        for (const auto &paragraph_text : paragraphs) {
            if (!checked_append_section_text_paragraph(replacement_root,
                                                       paragraph_text)) {
                (void)featherdoc::detail::
                    set_xml_document_mutation_allocation_failure(
                        this->last_error_info, part.entry_name);
                return false;
            }
        }
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, part.entry_name);
        return false;
    }

    const auto published_root = part.xml.child(part_root_name);
    try {
        this->xml_handle_lifetime->retire_subtree(published_root);
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, part.entry_name);
        return false;
    }

    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>,
                  "section part publication must be allocation-free");
    part.xml = std::move(replacement_xml);
    part.paragraph.set_parent(
        this->tracked_node(part.xml.child(part_root_name)));
    this->last_error_info.clear();
    return true;
}

bool Document::replace_section_header_text(
    std::size_t section_index, std::string_view replacement_text,
    featherdoc::section_reference_kind reference_kind) try {
    auto &ensured_header =
        this->ensure_section_header_paragraphs(section_index, reference_kind);
    if (!ensured_header.valid()) {
        return false;
    }

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->header_parts,
                                                  "w:headerReference");
    if (part == nullptr) {
        if (!this->last_error_info) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the requested section header part",
                           std::string{document_xml_entry});
        }
        return false;
    }

    auto root = part->xml.child("w:hdr");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::related_part_parse_failed,
                       "header part does not contain the expected w:hdr root",
                       part->entry_name);
        return false;
    }

    return this->replace_section_related_part_text(
        *part, "w:hdr", replacement_text);
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}

bool Document::replace_section_footer_text(
    std::size_t section_index, std::string_view replacement_text,
    featherdoc::section_reference_kind reference_kind) try {
    auto &ensured_footer =
        this->ensure_section_footer_paragraphs(section_index, reference_kind);
    if (!ensured_footer.valid()) {
        return false;
    }

    auto *part = this->section_related_part_state(section_index, reference_kind,
                                                  this->footer_parts,
                                                  "w:footerReference");
    if (part == nullptr) {
        if (!this->last_error_info) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the requested section footer part",
                           std::string{document_xml_entry});
        }
        return false;
    }

    auto root = part->xml.child("w:ftr");
    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info, document_errc::related_part_parse_failed,
                       "footer part does not contain the expected w:ftr root",
                       part->entry_name);
        return false;
    }

    return this->replace_section_related_part_text(
        *part, "w:ftr", replacement_text);
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}
