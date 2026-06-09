std::optional<std::size_t> TemplatePart::find_table_index_by_bookmark(
    std::string_view bookmark_name) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    return find_table_index_by_bookmark_in_part(*this->last_error_info,
                                                *this->xml_document,
                                                this->entry_name_storage,
                                                bookmark_name);
}

std::optional<std::size_t> TemplatePart::find_table_index(
    const featherdoc::template_table_selector &selector) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    return find_table_index_by_selector_in_part(*this->last_error_info,
                                                *this->xml_document,
                                                this->entry_name_storage,
                                                selector);
}

std::optional<featherdoc::Table> TemplatePart::find_table_by_bookmark(
    std::string_view bookmark_name) {
    const auto resolved_index = this->find_table_index_by_bookmark(bookmark_name);
    if (!resolved_index.has_value()) {
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < *resolved_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::state_not_recoverable),
                           "bookmark-resolved table index '" +
                               std::to_string(*resolved_index) +
                               "' is no longer present in template part",
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    this->last_error_info->clear();
    return table_handle;
}

std::optional<featherdoc::Table> TemplatePart::find_table(
    const featherdoc::template_table_selector &selector) {
    const auto resolved_index = this->find_table_index(selector);
    if (!resolved_index.has_value()) {
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < *resolved_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::state_not_recoverable),
                           "selector-resolved table index '" +
                               std::to_string(*resolved_index) +
                               "' is no longer present in template part",
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    this->last_error_info->clear();
    return table_handle;
}
