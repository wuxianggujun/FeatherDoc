bool Document::set_paragraph_style(Paragraph paragraph_handle,
                                   std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing paragraph styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style id must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    auto owning_part_entry_name = std::string_view{};
    if (!this->owns_paragraph_handle(paragraph_handle,
                                     owning_part_entry_name)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle does not belong to this "
                       "document or is no longer valid",
                       std::string{document_xml_entry});
        return false;
    }

    auto properties_transaction = applied_style_properties_transaction{};
    if (!properties_transaction.prepare(paragraph_handle.current, "w:pPr")) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare paragraph properties for a style "
                       "update",
                       std::string{owning_part_entry_name});
        return false;
    }
    if (!set_applied_style_value(properties_transaction.working_properties(),
                                 "w:pStyle", style_id)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate paragraph style metadata",
                       std::string{owning_part_entry_name});
        return false;
    }

    auto attachment_work =
        featherdoc::detail::singleton_part_attachment_work{};
    if (const auto error =
            this->prepare_styles_part_attachment(attachment_work)) {
        return false;
    }
    if (!properties_transaction.commit()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to commit paragraph style metadata",
                       std::string{owning_part_entry_name});
        return false;
    }
    this->publish_styles_part_attachment(std::move(attachment_work));
    this->last_error_info.clear();
    return true;
}

bool Document::clear_paragraph_style(Paragraph paragraph_handle) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing paragraph styles");
        return false;
    }

    auto owning_part_entry_name = std::string_view{};
    if (!this->owns_paragraph_handle(paragraph_handle,
                                     owning_part_entry_name)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target paragraph handle does not belong to this "
                       "document or is no longer valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (auto paragraph_properties = paragraph_handle.current.child("w:pPr");
        paragraph_properties != pugi::xml_node{}) {
        for (auto style_node = paragraph_properties.child("w:pStyle");
             style_node != pugi::xml_node{};) {
            const auto next_style_node = style_node.next_sibling("w:pStyle");
            paragraph_properties.remove_child(style_node);
            style_node = next_style_node;
        }
        remove_empty_paragraph_properties(paragraph_handle.current);
    }

    this->last_error_info.clear();
    return true;
}

bool Document::set_run_style(Run run_handle, std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing run styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "run style id must not be empty",
                       std::string{document_xml_entry});
        return false;
    }

    auto owning_part_entry_name = std::string_view{};
    if (!this->owns_run_handle(run_handle, owning_part_entry_name)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target run handle does not belong to this document or "
                       "is no longer valid",
                       std::string{document_xml_entry});
        return false;
    }

    auto properties_transaction = applied_style_properties_transaction{};
    if (!properties_transaction.prepare(run_handle.current, "w:rPr")) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to prepare run properties for a style update",
                       std::string{owning_part_entry_name});
        return false;
    }
    if (!set_applied_style_value(properties_transaction.working_properties(),
                                 "w:rStyle", style_id)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to populate run style metadata",
                       std::string{owning_part_entry_name});
        return false;
    }

    auto attachment_work =
        featherdoc::detail::singleton_part_attachment_work{};
    if (const auto error =
            this->prepare_styles_part_attachment(attachment_work)) {
        return false;
    }
    if (!properties_transaction.commit()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to commit run style metadata",
                       std::string{owning_part_entry_name});
        return false;
    }
    this->publish_styles_part_attachment(std::move(attachment_work));
    this->last_error_info.clear();
    return true;
}

bool Document::clear_run_style(Run run_handle) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing run styles");
        return false;
    }

    auto owning_part_entry_name = std::string_view{};
    if (!this->owns_run_handle(run_handle, owning_part_entry_name)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target run handle does not belong to this document or "
                       "is no longer valid",
                       std::string{document_xml_entry});
        return false;
    }

    if (auto run_properties = run_handle.current.child("w:rPr");
        run_properties != pugi::xml_node{}) {
        for (auto style_node = run_properties.child("w:rStyle");
             style_node != pugi::xml_node{};) {
            const auto next_style_node = style_node.next_sibling("w:rStyle");
            run_properties.remove_child(style_node);
            style_node = next_style_node;
        }
        remove_empty_run_properties(run_handle.current);
    }

    this->last_error_info.clear();
    return true;
}
