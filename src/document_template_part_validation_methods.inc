template_validation_result TemplatePart::validate_template(
    std::span<const template_slot_requirement> requirements) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    return validate_template_in_part(*this->last_error_info, *this->xml_document,
                                     this->entry_name_storage, requirements);
}

template_validation_result TemplatePart::validate_template(
    std::initializer_list<template_slot_requirement> requirements) const {
    return this->validate_template(
        std::span<const template_slot_requirement>{requirements.begin(),
                                                   requirements.size()});
}
