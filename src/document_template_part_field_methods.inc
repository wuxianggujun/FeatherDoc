bool TemplatePart::append_page_number_field(featherdoc::field_state_options state) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return false;
    }

    const auto paragraph = select_field_paragraph(container);
    if (paragraph == pugi::xml_node{} ||
        !append_simple_field_run(paragraph, page_number_field_instruction, "1", state)) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append page number field to template part",
                       this->entry_name_storage);
        return false;
    }

    this->last_error_info->clear();
    return true;
}

bool TemplatePart::append_total_pages_field(featherdoc::field_state_options state) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return false;
    }

    const auto paragraph = select_field_paragraph(container);
    if (paragraph == pugi::xml_node{} ||
        !append_simple_field_run(paragraph, total_pages_field_instruction, "1", state)) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append total pages field to template part",
                       this->entry_name_storage);
        return false;
    }

    this->last_error_info->clear();
    return true;
}

std::vector<featherdoc::field_summary> TemplatePart::list_fields() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return summarize_fields_in_part(*this->last_error_info, *this->xml_document);
}

bool TemplatePart::append_field(std::string_view instruction,
                                std::string_view result_text,
                                featherdoc::field_state_options state) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, instruction,
                                result_text, state);
}

bool TemplatePart::append_complex_field(
    std::string_view instruction, std::string_view result_text,
    featherdoc::complex_field_options options) {
    return this->append_complex_field(
        std::initializer_list<featherdoc::complex_field_instruction_fragment>{
            featherdoc::complex_field_text_fragment(instruction)},
        result_text, options);
}

bool TemplatePart::append_complex_field(
    std::span<const featherdoc::complex_field_instruction_fragment> instruction_fragments,
    std::string_view result_text, featherdoc::complex_field_options options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return append_complex_field_to_part(
        *this->last_error_info, *this->xml_document, this->entry_name_storage,
        instruction_fragments, result_text, options);
}

bool TemplatePart::append_complex_field(
    std::initializer_list<featherdoc::complex_field_instruction_fragment>
        instruction_fragments,
    std::string_view result_text, featherdoc::complex_field_options options) {
    return this->append_complex_field(
        std::span<const featherdoc::complex_field_instruction_fragment>{
            instruction_fragments.begin(), instruction_fragments.size()},
        result_text, options);
}

bool TemplatePart::append_table_of_contents_field(
    featherdoc::table_of_contents_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_table_of_contents_instruction(
        options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_reference_field(
    std::string_view bookmark_name,
    featherdoc::reference_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_reference_instruction(
        bookmark_name, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_page_reference_field(
    std::string_view bookmark_name,
    featherdoc::page_reference_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_page_reference_instruction(
        bookmark_name, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_style_reference_field(
    std::string_view style_name,
    featherdoc::style_reference_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_style_reference_instruction(
        style_name, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_document_property_field(
    std::string_view property_name,
    featherdoc::document_property_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_document_property_instruction(
        property_name, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_date_field(featherdoc::date_field_options options,
                                     std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_date_instruction(
        options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_hyperlink_field(
    std::string_view target,
    featherdoc::hyperlink_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_hyperlink_instruction(
        target, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_sequence_field(
    std::string_view identifier,
    featherdoc::sequence_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_sequence_instruction(
        identifier, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}


bool TemplatePart::append_caption(std::string_view label,
                                  std::string_view caption_text,
                                  featherdoc::caption_field_options options,
                                  std::string_view number_result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_caption_sequence_instruction(
        label, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    const auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return false;
    }

    const auto paragraph = featherdoc::detail::append_paragraph_node(container);
    if (paragraph == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append caption paragraph to template part",
                       this->entry_name_storage);
        return false;
    }

    std::string label_text{label};
    label_text.push_back(' ');
    if (!featherdoc::detail::append_plain_text_run(paragraph, label_text) ||
        !append_simple_field_run(paragraph, *instruction, number_result_text,
                                 options.state)) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to append caption field to template part",
                       this->entry_name_storage);
        return false;
    }

    if (!caption_text.empty()) {
        std::string trailing_text = options.separator;
        trailing_text += caption_text;
        if (!featherdoc::detail::append_plain_text_run(paragraph, trailing_text)) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::io_error),
                           "failed to append caption text to template part",
                           this->entry_name_storage);
            return false;
        }
    }

    this->last_error_info->clear();
    return true;
}

bool TemplatePart::append_index_field(featherdoc::index_field_options options,
                                      std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_index_instruction(
        options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::append_index_entry_field(
    std::string_view entry_text,
    featherdoc::index_entry_field_options options,
    std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    const auto instruction = build_index_entry_instruction(
        entry_text, options, *this->last_error_info, this->entry_name_storage);
    if (!instruction.has_value()) {
        return false;
    }

    return append_field_to_part(*this->last_error_info, *this->xml_document,
                                this->entry_name_storage, *instruction,
                                result_text, options.state);
}

bool TemplatePart::replace_field(std::size_t field_index,
                                 std::string_view instruction,
                                 std::string_view result_text) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return false;
    }

    return replace_field_in_part(*this->last_error_info, *this->xml_document,
                                 this->entry_name_storage, field_index,
                                 instruction, result_text);
}
