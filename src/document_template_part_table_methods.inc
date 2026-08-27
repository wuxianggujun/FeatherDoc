Table TemplatePart::tables() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return {};
    }

    this->last_error_info->clear();
    auto table =
        Table(this->owner->tracked_node(container), container.child("w:tbl"));
    table.set_owner(this->owner.get());
    return table;
}

Table TemplatePart::append_table(std::size_t row_count, std::size_t column_count) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto container = template_part_block_container(*this->xml_document);
    if (container == pugi::xml_node{}) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part does not contain a supported block container",
                       this->entry_name_storage);
        return {};
    }

    if (row_count == 0U || column_count == 0U) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table row and column counts must be greater "
                       "than zero",
                       this->entry_name_storage);
        return {};
    }

    if (column_count > detail::max_table_grid_columns) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template table column count exceeds supported limit",
                       this->entry_name_storage);
        return {};
    }

    const auto table_node = detail::append_table_node(container);
    if (table_node == pugi::xml_node{}) {
        (void)detail::set_xml_document_allocation_failure(
            *this->last_error_info,
            "failed to append table to template part",
            this->entry_name_storage);
        return {};
    }

    try {
        for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
            if (detail::append_row_node(table_node, column_count) ==
                pugi::xml_node{}) {
                (void)container.remove_child(table_node);
                (void)detail::set_xml_document_allocation_failure(
                    *this->last_error_info,
                    "failed to append table row to template part",
                    this->entry_name_storage);
                return {};
            }
        }
    } catch (const std::bad_alloc &) {
        (void)container.remove_child(table_node);
        (void)detail::set_xml_document_mutation_allocation_failure(
            *this->last_error_info, this->entry_name_storage);
        return {};
    } catch (...) {
        (void)container.remove_child(table_node);
        throw;
    }

    auto created_table =
        Table(this->owner->tracked_node(container), table_node);
    created_table.set_owner(this->owner.get());
    this->last_error_info->clear();
    return created_table;
}

std::vector<featherdoc::table_inspection_summary> TemplatePart::inspect_tables() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto summaries = std::vector<featherdoc::table_inspection_summary>{};
    auto table_handle = this->tables();
    for (std::size_t table_index = 0U; table_handle.has_next();
         ++table_index, table_handle.next()) {
        summaries.push_back(summarize_part_table_handle(table_handle, table_index));
    }

    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::table_inspection_summary>
TemplatePart::inspect_table(std::size_t table_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        this->last_error_info->clear();
        return std::nullopt;
    }

    auto summary = summarize_part_table_handle(table_handle, table_index);
    this->last_error_info->clear();
    return summary;
}

std::vector<featherdoc::table_cell_inspection_summary>
TemplatePart::inspect_table_cells(std::size_t table_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next(); ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        this->last_error_info->clear();
        return {};
    }

    auto summaries = collect_part_table_cell_summaries(table_handle);
    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::table_cell_inspection_summary>
TemplatePart::inspect_table_cell(std::size_t table_index, std::size_t row_index,
                                  std::size_t cell_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    const auto cells = this->inspect_table_cells(table_index);
    for (const auto &cell : cells) {
        if (cell.row_index == row_index && cell.cell_index == cell_index) {
            this->last_error_info->clear();
            return cell;
        }
    }

    this->last_error_info->clear();
    return std::nullopt;
}

std::optional<featherdoc::table_cell_inspection_summary>
TemplatePart::inspect_table_cell_by_grid_column(std::size_t table_index,
                                                std::size_t row_index,
                                                std::size_t grid_column) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    const auto cells = this->inspect_table_cells(table_index);
    for (const auto &cell : cells) {
        if (cell.row_index == row_index && grid_column >= cell.column_index &&
            grid_column - cell.column_index < cell.column_span) {
            this->last_error_info->clear();
            return cell;
        }
    }

    this->last_error_info->clear();
    return std::nullopt;
}

