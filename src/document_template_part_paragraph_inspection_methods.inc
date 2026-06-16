std::vector<featherdoc::paragraph_inspection_summary> TemplatePart::inspect_paragraphs() {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto summaries = std::vector<featherdoc::paragraph_inspection_summary>{};
    auto paragraph_handle = this->paragraphs();
    for (std::size_t paragraph_index = 0U; paragraph_handle.has_next();
         ++paragraph_index, paragraph_handle.next()) {
        auto summary = summarize_part_paragraph_node(paragraph_handle.current, paragraph_index);
        enrich_part_paragraph_numbering(this->owner, summary);
        summaries.push_back(std::move(summary));
    }

    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::paragraph_inspection_summary>
TemplatePart::inspect_paragraph(std::size_t paragraph_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next(); ++current_index) {
        paragraph_handle.next();
    }

    if (!paragraph_handle.has_next()) {
        this->last_error_info->clear();
        return std::nullopt;
    }

    auto summary = summarize_part_paragraph_node(paragraph_handle.current, paragraph_index);
    enrich_part_paragraph_numbering(this->owner, summary);
    this->last_error_info->clear();
    return summary;
}

std::vector<featherdoc::run_inspection_summary>
TemplatePart::inspect_paragraph_runs(std::size_t paragraph_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return {};
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next(); ++current_index) {
        paragraph_handle.next();
    }

    if (!paragraph_handle.has_next()) {
        this->last_error_info->clear();
        return {};
    }

    auto summaries = std::vector<featherdoc::run_inspection_summary>{};
    auto run = paragraph_handle.runs();
    for (std::size_t run_index = 0U; run.has_next(); ++run_index, run.next()) {
        summaries.push_back(summarize_part_run_handle(run, run_index));
    }

    this->last_error_info->clear();
    return summaries;
}

std::optional<featherdoc::run_inspection_summary>
TemplatePart::inspect_paragraph_run(std::size_t paragraph_index, std::size_t run_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           std::string{unavailable_template_part_detail},
                           this->entry_name_storage);
        }
        return std::nullopt;
    }

    const auto runs = this->inspect_paragraph_runs(paragraph_index);
    if (run_index >= runs.size()) {
        this->last_error_info->clear();
        return std::nullopt;
    }

    return runs[run_index];
}

