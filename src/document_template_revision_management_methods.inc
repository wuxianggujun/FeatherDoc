std::optional<featherdoc::text_range_preview> Document::preview_text_range(
    std::size_t start_paragraph_index, std::size_t start_text_offset,
    std::size_t end_paragraph_index, std::size_t end_text_offset) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before previewing a text range",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    std::vector<pugi::xml_node> paragraphs;
    auto body =
        const_cast<pugi::xml_document &>(this->document)
            .child("w:document")
            .child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:p") {
            paragraphs.push_back(child);
        }
    }

    paragraph_revision_range_plan plan;
    std::string range_error;
    if (!build_paragraph_revision_range_plan(
            paragraphs, start_paragraph_index, start_text_offset,
            end_paragraph_index, end_text_offset, plan, range_error)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       range_error, std::string{document_xml_entry});
        return std::nullopt;
    }

    auto preview = preview_paragraph_revision_range_segments(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset, plan);
    this->last_error_info.clear();
    return preview;
}

std::vector<featherdoc::text_range_preview> Document::find_text_ranges(
    std::string_view text) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before finding text ranges",
                       std::string{document_xml_entry});
        return {};
    }
    if (text.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "search text must not be empty",
                       std::string{document_xml_entry});
        return {};
    }

    struct paragraph_text_range {
        std::size_t paragraph_index{};
        std::size_t global_start{};
        std::size_t text_length{};
    };

    std::vector<pugi::xml_node> paragraphs;
    std::vector<paragraph_text_range> paragraph_ranges;
    std::string document_text;
    auto body =
        const_cast<pugi::xml_document &>(this->document)
            .child("w:document")
            .child("w:body");
    std::size_t paragraph_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        paragraphs.push_back(child);
        paragraph_text_range range;
        range.paragraph_index = paragraph_index;
        range.global_start = document_text.size();
        for (const auto &span : collect_paragraph_revision_run_spans(child)) {
            document_text += span.text;
        }
        range.text_length = document_text.size() - range.global_start;
        paragraph_ranges.push_back(range);
        ++paragraph_index;
    }

    const auto map_match_start =
        [&paragraph_ranges](std::size_t global_offset, std::size_t &mapped_paragraph,
                            std::size_t &mapped_offset) -> bool {
        if (paragraph_ranges.empty()) {
            return false;
        }
        for (const auto &range : paragraph_ranges) {
            if (range.text_length == 0U) {
                continue;
            }
            const auto range_end = range.global_start + range.text_length;
            if (global_offset < range_end) {
                mapped_paragraph = range.paragraph_index;
                mapped_offset = global_offset - range.global_start;
                return true;
            }
        }

        const auto &last = paragraph_ranges.back();
        mapped_paragraph = last.paragraph_index;
        mapped_offset = last.text_length;
        return true;
    };

    const auto map_match_end =
        [&paragraph_ranges](std::size_t global_offset, std::size_t &mapped_paragraph,
                            std::size_t &mapped_offset) -> bool {
        if (paragraph_ranges.empty()) {
            return false;
        }

        std::optional<paragraph_text_range> previous_text_range;
        for (const auto &range : paragraph_ranges) {
            if (range.text_length == 0U) {
                continue;
            }
            const auto range_end = range.global_start + range.text_length;
            if (global_offset > range.global_start && global_offset <= range_end) {
                mapped_paragraph = range.paragraph_index;
                mapped_offset = global_offset - range.global_start;
                return true;
            }
            if (global_offset == range.global_start &&
                previous_text_range.has_value()) {
                mapped_paragraph = previous_text_range->paragraph_index;
                mapped_offset = previous_text_range->text_length;
                return true;
            }
            previous_text_range = range;
        }

        if (previous_text_range.has_value()) {
            mapped_paragraph = previous_text_range->paragraph_index;
            mapped_offset = previous_text_range->text_length;
            return true;
        }

        const auto &last = paragraph_ranges.back();
        mapped_paragraph = last.paragraph_index;
        mapped_offset = last.text_length;
        return true;
    };

    std::vector<featherdoc::text_range_preview> matches;
    std::size_t search_offset = 0U;
    while (search_offset <= document_text.size()) {
        const auto match_offset = document_text.find(text, search_offset);
        if (match_offset == std::string::npos) {
            break;
        }

        std::size_t start_paragraph_index = 0U;
        std::size_t start_text_offset = 0U;
        std::size_t end_paragraph_index = 0U;
        std::size_t end_text_offset = 0U;
        if (!map_match_start(match_offset, start_paragraph_index,
                             start_text_offset) ||
            !map_match_end(match_offset + text.size(), end_paragraph_index,
                           end_text_offset)) {
            break;
        }

        paragraph_revision_range_plan plan;
        std::string range_error;
        if (!build_paragraph_revision_range_plan(
                paragraphs, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset, plan, range_error)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           range_error, std::string{document_xml_entry});
            return {};
        }

        matches.push_back(preview_paragraph_revision_range_segments(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset, plan));
        search_offset = match_offset + 1U;
    }

    this->last_error_info.clear();
    return matches;
}

std::vector<featherdoc::revision_summary> Document::list_revisions() const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing revisions",
                       std::string{document_xml_entry});
        return {};
    }

    std::vector<featherdoc::revision_summary> summaries;
    collect_revisions_from_node(const_cast<pugi::xml_document &>(this->document),
                                document_xml_entry, summaries);
    for (const auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revisions_from_node(part->xml, part->entry_name, summaries);
        }
    }
    for (const auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revisions_from_node(part->xml, part->entry_name, summaries);
        }
    }

    this->last_error_info.clear();
    return summaries;
}

bool Document::set_revision_metadata(
    std::size_t revision_index,
    const featherdoc::revision_metadata_update &metadata) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before setting revision metadata",
                       std::string{document_xml_entry});
        return false;
    }
    const auto has_change = metadata.author.has_value() ||
                            metadata.date.has_value() ||
                            metadata.clear_author || metadata.clear_date;
    if (!has_change) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision metadata update must contain at least one change",
                       std::string{document_xml_entry});
        return false;
    }
    if ((metadata.author.has_value() && metadata.clear_author) ||
        (metadata.date.has_value() && metadata.clear_date)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision metadata cannot be set and cleared in the same update",
                       std::string{document_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> revision_nodes;
    collect_revision_nodes_in_order(this->document, revision_nodes);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    if (revision_index >= revision_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }

    auto revision = revision_nodes[revision_index];
    if (metadata.author.has_value()) {
        ensure_attribute_value(revision, "w:author", *metadata.author);
    } else if (metadata.clear_author) {
        revision.remove_attribute("w:author");
    }
    if (metadata.date.has_value()) {
        ensure_attribute_value(revision, "w:date", *metadata.date);
    } else if (metadata.clear_date) {
        revision.remove_attribute("w:date");
    }

    this->last_error_info.clear();
    return true;
}

bool Document::accept_revision(std::size_t revision_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accepting a revision",
                       std::string{document_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> revision_nodes;
    collect_revision_nodes_in_order(this->document, revision_nodes);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    if (revision_index >= revision_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!apply_revision_decision(revision_nodes[revision_index], true)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to accept revision",
                       std::string{document_xml_entry});
        return false;
    }
    this->last_error_info.clear();
    return true;
}

bool Document::reject_revision(std::size_t revision_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before rejecting a revision",
                       std::string{document_xml_entry});
        return false;
    }

    std::vector<pugi::xml_node> revision_nodes;
    collect_revision_nodes_in_order(this->document, revision_nodes);
    for (auto &part : this->header_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    for (auto &part : this->footer_parts) {
        if (part != nullptr) {
            collect_revision_nodes_in_order(part->xml, revision_nodes);
        }
    }
    if (revision_index >= revision_nodes.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "revision index is out of range",
                       std::string{document_xml_entry});
        return false;
    }
    if (!apply_revision_decision(revision_nodes[revision_index], false)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to reject revision",
                       std::string{document_xml_entry});
        return false;
    }
    this->last_error_info.clear();
    return true;
}

std::size_t Document::accept_all_revisions() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before accepting revisions",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::size_t accepted = 0U;
    for (;;) {
        std::vector<pugi::xml_node> revision_nodes;
        collect_revision_nodes_in_order(this->document, revision_nodes);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        if (revision_nodes.empty()) {
            break;
        }
        if (!apply_revision_decision(revision_nodes.front(), true)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::io_error),
                           "failed to accept revision",
                           std::string{document_xml_entry});
            return accepted;
        }
        ++accepted;
    }

    this->last_error_info.clear();
    return accepted;
}

std::size_t Document::reject_all_revisions() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before rejecting revisions",
                       std::string{document_xml_entry});
        return 0U;
    }

    std::size_t rejected = 0U;
    for (;;) {
        std::vector<pugi::xml_node> revision_nodes;
        collect_revision_nodes_in_order(this->document, revision_nodes);
        for (auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        for (auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_revision_nodes_in_order(part->xml, revision_nodes);
            }
        }
        if (revision_nodes.empty()) {
            break;
        }
        if (!apply_revision_decision(revision_nodes.front(), false)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::io_error),
                           "failed to reject revision",
                           std::string{document_xml_entry});
            return rejected;
        }
        ++rejected;
    }

    this->last_error_info.clear();
    return rejected;
}
