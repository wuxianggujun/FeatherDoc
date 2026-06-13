constexpr double kCrossPageTableTopOffsetThresholdPoints = 160.0;

[[nodiscard]] bool has_importable_text(const PdfParsedParagraph &paragraph) {
    return !paragraph.text.empty();
}

[[nodiscard]] double rect_right(const PdfRect &rect) noexcept {
    return rect.x_points + rect.width_points;
}

[[nodiscard]] double rect_top(const PdfRect &rect) noexcept {
    return rect.y_points + rect.height_points;
}

[[nodiscard]] bool rects_intersect(const PdfRect &left,
                                   const PdfRect &right) noexcept {
    return left.x_points < rect_right(right) &&
           rect_right(left) > right.x_points &&
           left.y_points < rect_top(right) &&
           rect_top(left) > right.y_points;
}

[[nodiscard]] double rect_area(const PdfRect &rect) noexcept {
    return (std::max)(rect.width_points, 0.0) *
           (std::max)(rect.height_points, 0.0);
}

[[nodiscard]] double rect_intersection_area(const PdfRect &left,
                                            const PdfRect &right) noexcept {
    if (!rects_intersect(left, right)) {
        return 0.0;
    }

    const double intersection_left = (std::max)(left.x_points, right.x_points);
    const double intersection_right =
        (std::min)(rect_right(left), rect_right(right));
    const double intersection_bottom = (std::max)(left.y_points, right.y_points);
    const double intersection_top = (std::min)(rect_top(left), rect_top(right));
    return (std::max)(intersection_right - intersection_left, 0.0) *
           (std::max)(intersection_top - intersection_bottom, 0.0);
}

[[nodiscard]] bool rect_contains_point(const PdfRect &rect, double x_points,
                                       double y_points) noexcept {
    return x_points >= rect.x_points && x_points <= rect_right(rect) &&
           y_points >= rect.y_points && y_points <= rect_top(rect);
}

void expand_rect(PdfRect &target, const PdfRect &source) noexcept {
    if (target.width_points <= 0.0 && target.height_points <= 0.0) {
        target = source;
        return;
    }

    const double left = (std::min)(target.x_points, source.x_points);
    const double bottom = (std::min)(target.y_points, source.y_points);
    const double right = (std::max)(rect_right(target), rect_right(source));
    const double top = (std::max)(rect_top(target), rect_top(source));
    target.x_points = left;
    target.y_points = bottom;
    target.width_points = right - left;
    target.height_points = top - bottom;
}

[[nodiscard]] bool rect_meaningfully_overlaps_rect(const PdfRect &subject,
                                                   const PdfRect &rect) noexcept {
    const double subject_area = rect_area(subject);
    if (subject_area <= 0.0) {
        return false;
    }

    const double center_x = subject.x_points + subject.width_points / 2.0;
    const double center_y = subject.y_points + subject.height_points / 2.0;
    const bool center_inside_rect =
        rect_contains_point(rect, center_x, center_y);
    const double overlap_ratio =
        rect_intersection_area(subject, rect) / subject_area;
    return center_inside_rect || overlap_ratio >= 0.5;
}

[[nodiscard]] bool paragraph_meaningfully_overlaps_rect(
    const PdfParsedParagraph &paragraph, const PdfRect &rect) noexcept {
    if (!paragraph.lines.empty()) {
        std::size_t overlapping_line_count = 0U;
        for (const auto &line : paragraph.lines) {
            if (rect_meaningfully_overlaps_rect(line.bounds, rect)) {
                ++overlapping_line_count;
            }
        }
        return overlapping_line_count > paragraph.lines.size() / 2U;
    }

    return rect_meaningfully_overlaps_rect(paragraph.bounds, rect);
}

[[nodiscard]] bool has_table_candidates(const PdfParsedDocument &document) {
    for (const auto &page : document.pages) {
        if (!page.table_candidates.empty()) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool overlaps_table_candidate(
    const PdfParsedParagraph &paragraph, const PdfParsedPage &page) {
    if (paragraph.table_candidate_index.has_value() &&
        *paragraph.table_candidate_index < page.table_candidates.size()) {
        return true;
    }

    return std::any_of(page.table_candidates.begin(),
                       page.table_candidates.end(),
                       [&paragraph](const PdfParsedTableCandidate &table) {
                           return std::any_of(
                               table.rows.begin(), table.rows.end(),
                               [&paragraph](const PdfParsedTableRow &row) {
                                   return paragraph_meaningfully_overlaps_rect(
                                       paragraph, row.bounds);
                               });
                       });
}

[[nodiscard]] bool line_overlaps_table_candidate(const PdfParsedTextLine &line,
                                                 const PdfParsedPage &page) {
    if (line.table_candidate_index.has_value() &&
        *line.table_candidate_index < page.table_candidates.size()) {
        return true;
    }

    return std::any_of(page.table_candidates.begin(),
                       page.table_candidates.end(),
                       [&line](const PdfParsedTableCandidate &table) {
                           return std::any_of(
                               table.rows.begin(), table.rows.end(),
                               [&line](const PdfParsedTableRow &row) {
                                   return rect_meaningfully_overlaps_rect(
                                       line.bounds, row.bounds);
                               });
                       });
}

struct ParagraphFragment {
    std::string text;
    PdfRect bounds;
};

[[nodiscard]] std::vector<ParagraphFragment> build_paragraph_fragments(
    const PdfParsedParagraph &paragraph, const PdfParsedPage &page,
    bool import_table_candidates_as_tables) {
    if (!has_importable_text(paragraph)) {
        return {};
    }

    if (!import_table_candidates_as_tables || page.table_candidates.empty()) {
        return {{paragraph.text, paragraph.bounds}};
    }

    if (paragraph.lines.empty()) {
        return overlaps_table_candidate(paragraph, page)
                   ? std::vector<ParagraphFragment>{}
                   : std::vector<ParagraphFragment>{
                         ParagraphFragment{paragraph.text, paragraph.bounds}};
    }

    const bool has_overlapping_line = std::any_of(
        paragraph.lines.begin(), paragraph.lines.end(),
        [&page](const PdfParsedTextLine &line) {
            return line_overlaps_table_candidate(line, page);
        });

    // 只有当段内存在明确落在表格里的行时，才按行切分并剔除重叠行；
    // 纯边界擦边的正文段落应优先作为完整段落保留，避免误丢正文。
    if (!has_overlapping_line) {
        return {{paragraph.text, paragraph.bounds}};
    }

    std::vector<ParagraphFragment> fragments;
    std::optional<ParagraphFragment> current_fragment;
    for (const auto &line : paragraph.lines) {
        if (line_overlaps_table_candidate(line, page)) {
            if (current_fragment.has_value()) {
                fragments.push_back(std::move(*current_fragment));
                current_fragment.reset();
            }
            continue;
        }

        if (!current_fragment.has_value()) {
            current_fragment = ParagraphFragment{line.text, line.bounds};
            continue;
        }

        current_fragment->text.push_back('\n');
        current_fragment->text += line.text;
        expand_rect(current_fragment->bounds, line.bounds);
    }

    if (current_fragment.has_value()) {
        fragments.push_back(std::move(*current_fragment));
    }

    return fragments;
}

[[nodiscard]] bool append_imported_paragraph(featherdoc::Paragraph &cursor,
                                             bool &has_written_paragraph,
                                             const std::string &text) {
    if (!has_written_paragraph) {
        if (!cursor.has_next()) {
            return false;
        }
        if (!cursor.set_text(text)) {
            return false;
        }
        has_written_paragraph = true;
        return true;
    }

    cursor = cursor.insert_paragraph_after(text);
    return cursor.has_next();
}

struct ImportBlock {
    enum class Kind { paragraph, table };

    Kind kind{Kind::paragraph};
    double top_points{0.0};
    double left_points{0.0};
    std::string paragraph_text;
    const PdfParsedTableCandidate *table{nullptr};
};

struct ImportCursor {
    featherdoc::Paragraph paragraph;
    std::optional<featherdoc::Table> table;
    std::size_t table_row_count{0U};
    std::size_t table_column_count{0U};
    std::vector<double> table_column_anchor_x_points;
    std::vector<std::string> table_header_cell_texts;
    bool has_written_block{false};
};

enum class TableAppendResult { failed, created, merged };

[[nodiscard]] std::vector<ImportBlock> build_import_blocks(
    const PdfParsedPage &page, bool import_table_candidates_as_tables) {
    std::vector<ImportBlock> blocks;
    blocks.reserve(page.paragraphs.size() + page.table_candidates.size());

    if (!page.content_blocks.empty()) {
        for (const auto &content_block : page.content_blocks) {
            if (content_block.kind ==
                PdfParsedContentBlockKind::paragraph) {
                if (content_block.source_index >= page.paragraphs.size()) {
                    continue;
                }

                const auto &paragraph = page.paragraphs[content_block.source_index];
                for (auto &fragment : build_paragraph_fragments(
                         paragraph, page, import_table_candidates_as_tables)) {
                    blocks.push_back(ImportBlock{
                        ImportBlock::Kind::paragraph, rect_top(fragment.bounds),
                        fragment.bounds.x_points, std::move(fragment.text),
                        nullptr});
                }
                continue;
            }

            if (!import_table_candidates_as_tables ||
                content_block.source_index >= page.table_candidates.size()) {
                continue;
            }

            const auto &table = page.table_candidates[content_block.source_index];
            blocks.push_back(ImportBlock{
                ImportBlock::Kind::table, rect_top(table.bounds),
                table.bounds.x_points, {}, &table});
        }
        return blocks;
    }

    for (const auto &paragraph : page.paragraphs) {
        for (auto &fragment : build_paragraph_fragments(
                 paragraph, page, import_table_candidates_as_tables)) {
            blocks.push_back(ImportBlock{
                ImportBlock::Kind::paragraph, rect_top(fragment.bounds),
                fragment.bounds.x_points, std::move(fragment.text), nullptr});
        }
    }

    if (import_table_candidates_as_tables) {
        for (const auto &table : page.table_candidates) {
            blocks.push_back(ImportBlock{
                ImportBlock::Kind::table, rect_top(table.bounds),
                table.bounds.x_points, {}, &table});
        }
    }

    std::stable_sort(blocks.begin(), blocks.end(),
                     [](const ImportBlock &left, const ImportBlock &right) {
                         const auto left_top_key =
                             std::llround(left.top_points * 100.0);
                         const auto right_top_key =
                             std::llround(right.top_points * 100.0);
                         if (left_top_key != right_top_key) {
                             return left_top_key > right_top_key;
                         }

                         const auto left_x_key =
                             std::llround(left.left_points * 100.0);
                         const auto right_x_key =
                             std::llround(right.left_points * 100.0);
                         if (left_x_key != right_x_key) {
                             return left_x_key < right_x_key;
                         }

                         return false;
                     });
    return blocks;
}
