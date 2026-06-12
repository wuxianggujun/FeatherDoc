[[nodiscard]] std::vector<PdfParsedTableCandidate>
detect_table_candidates(std::vector<PdfParsedTextLine> &lines) {
    const auto candidate_lines = collect_candidate_lines(lines);
    std::vector<PdfParsedTableCandidate> tables;

    if (candidate_lines.size() < 3U) {
        return tables;
    }

    for (std::size_t gap_index = 0U; gap_index + 1U < candidate_lines.size();
         ++gap_index) {
        const double first_gap =
            candidate_lines[gap_index].center_y -
            candidate_lines[gap_index + 1U].center_y;
        if (first_gap <= 0.0) {
            continue;
        }

        const double gap_tolerance = (std::max)(4.0, first_gap * 0.15);
        std::size_t run_end = gap_index;
        while (run_end + 1U < candidate_lines.size() - 1U) {
            const double next_gap =
                candidate_lines[run_end + 1U].center_y -
                candidate_lines[run_end + 2U].center_y;
            if (std::abs(next_gap - first_gap) > gap_tolerance) {
                break;
            }
            ++run_end;
        }

        std::vector<CandidateLine> rows;
        rows.reserve(run_end - gap_index + 2U);
        for (std::size_t row_index = gap_index; row_index <= run_end + 1U;
             ++row_index) {
            rows.push_back(candidate_lines[row_index]);
        }

        PdfParsedTableCandidate table;
        if (build_table_candidate(rows, table)) {
            const auto table_index = tables.size();
            for (const auto &row : rows) {
                lines[row.line_index].table_candidate_index = table_index;
            }
            tables.push_back(std::move(table));
            gap_index = run_end;
        }
    }

    return tables;
}

[[nodiscard]] std::vector<PdfParsedParagraph>
group_lines_into_paragraphs(const std::vector<PdfParsedTextLine> &lines) {
    std::vector<PdfParsedParagraph> paragraphs;
    std::optional<std::size_t> previous_table_candidate_index;
    for (const auto &line : lines) {
        const auto current_table_candidate_index = line.table_candidate_index;
        if (paragraphs.empty() ||
            should_start_new_paragraph(paragraphs.back().lines.back(), line,
                                       previous_table_candidate_index,
                                       current_table_candidate_index)) {
            paragraphs.emplace_back();
        }
        paragraphs.back().table_candidate_index = current_table_candidate_index;
        append_line_to_paragraph(paragraphs.back(), line);
        previous_table_candidate_index = current_table_candidate_index;
    }
    return paragraphs;
}

[[nodiscard]] std::vector<PdfParsedContentBlock> build_content_blocks(
    const PdfParsedPage &page) {
    std::vector<PdfParsedContentBlock> blocks;
    blocks.reserve(page.paragraphs.size() + page.table_candidates.size());

    std::vector<bool> emitted_tables(page.table_candidates.size(), false);
    for (std::size_t paragraph_index = 0U;
         paragraph_index < page.paragraphs.size(); ++paragraph_index) {
        const auto &paragraph = page.paragraphs[paragraph_index];
        if (paragraph.table_candidate_index.has_value() &&
            *paragraph.table_candidate_index < page.table_candidates.size()) {
            const auto table_index = *paragraph.table_candidate_index;
            if (!emitted_tables[table_index]) {
                blocks.push_back(PdfParsedContentBlock{
                    PdfParsedContentBlockKind::table_candidate,
                    page.table_candidates[table_index].bounds, table_index});
                emitted_tables[table_index] = true;
            }
            continue;
        }

        blocks.push_back(PdfParsedContentBlock{
            PdfParsedContentBlockKind::paragraph, paragraph.bounds,
            paragraph_index});
    }

    for (std::size_t table_index = 0U; table_index < page.table_candidates.size();
         ++table_index) {
        if (emitted_tables[table_index]) {
            continue;
        }

        blocks.push_back(PdfParsedContentBlock{
            PdfParsedContentBlockKind::table_candidate,
            page.table_candidates[table_index].bounds, table_index});
    }

    return blocks;
}

void enrich_text_structure(PdfParsedPage &page) {
    page.text_lines = group_text_spans_into_lines(page.text_spans);
    sort_lines_by_reading_order(page.text_lines);
    page.table_candidates = detect_table_candidates(page.text_lines);
    page.paragraphs = group_lines_into_paragraphs(page.text_lines);
    page.content_blocks = build_content_blocks(page);
}
