[[nodiscard]] bool configure_imported_table(featherdoc::Table &table,
                                            const PdfParsedTableCandidate &source) {
    const auto column_count = table_column_count(source);
    if (!has_consistent_row_widths(source, column_count)) {
        return false;
    }

    if (!table.has_next()) {
        return false;
    }

    if (!table.set_style_id("TableGrid") ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !table.set_width_twips(table_width_twips(source))) {
        return false;
    }

    for (std::size_t column_index = 0U; column_index < column_count;
         ++column_index) {
        if (!table.set_column_width_twips(
                column_index, column_width_twips(source, column_index))) {
            return false;
        }
    }

    for (std::size_t row_index = 0U; row_index < source.rows.size();
         ++row_index) {
        const auto &row = source.rows[row_index];
        if (row.cells.size() != column_count) {
            return false;
        }

        for (const auto &cell : row.cells) {
            if (cell.has_text &&
                !table.set_cell_text_by_grid_column(row_index,
                                                    cell.column_index,
                                                    cell.text)) {
                return false;
            }
        }

        auto row_handle = table.find_row(row_index);
        if (!row_handle.has_value() ||
            !row_handle->set_height_twips(
                table_row_height_twips(source, row),
                featherdoc::row_height_rule::exact)) {
            return false;
        }

        if (!apply_imported_table_column_spans(table, row_index, row)) {
            return false;
        }
    }

    if (!apply_imported_table_row_span_column_spans(table, 0U, source, 0U,
                                                    source.rows.size())) {
        return false;
    }

    if (!apply_imported_table_row_spans(table, 0U, source, 0U,
                                        source.rows.size())) {
        return false;
    }

    if (should_mark_repeating_header_row(source)) {
        auto header_row = table.find_row(0U);
        if (!header_row.has_value() || !header_row->set_repeats_header()) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::uint32_t continuation_confidence(
    const PdfTableContinuationDiagnostic &diagnostic) {
    switch (diagnostic.blocker) {
    case PdfTableContinuationBlocker::none:
        return diagnostic.skipped_repeating_header ? 95U : 85U;
    case PdfTableContinuationBlocker::no_previous_table:
        return 0U;
    case PdfTableContinuationBlocker::not_first_block_on_page:
        return 35U;
    case PdfTableContinuationBlocker::not_near_page_top:
        return 45U;
    case PdfTableContinuationBlocker::inconsistent_source_rows:
        return 25U;
    case PdfTableContinuationBlocker::column_count_mismatch:
        return 30U;
    case PdfTableContinuationBlocker::column_anchors_mismatch:
        return 55U;
    case PdfTableContinuationBlocker::repeated_header_mismatch:
        return 70U;
    case PdfTableContinuationBlocker::continuation_confidence_below_threshold:
        return diagnostic.continuation_confidence;
    }

    return 0U;
}

[[nodiscard]] PdfTableContinuationDiagnostic evaluate_table_continuation(
    const ImportCursor &cursor, std::size_t page_index, std::size_t block_index,
    double block_top_points, double page_height_points,
    const PdfParsedTableCandidate &source,
    std::uint32_t min_continuation_confidence) {
    PdfTableContinuationDiagnostic diagnostic;
    diagnostic.page_index = page_index;
    diagnostic.block_index = block_index;
    diagnostic.minimum_continuation_confidence = min_continuation_confidence;
    diagnostic.is_first_block_on_page = block_index == 0U;
    diagnostic.is_near_page_top =
        block_top_points >= 0.0 &&
        (page_height_points - block_top_points) <=
            kCrossPageTableTopOffsetThresholdPoints;
    diagnostic.has_previous_table =
        cursor.table.has_value() && cursor.table_row_count > 0U;
    diagnostic.previous_has_repeating_header =
        !cursor.table_header_cell_texts.empty();
    diagnostic.source_has_repeating_header =
        !source.rows.empty() && should_mark_repeating_header_row(source);

    const auto column_count = table_column_count(source);
    diagnostic.source_rows_consistent =
        has_consistent_row_widths(source, column_count);
    diagnostic.column_count_matches =
        diagnostic.has_previous_table &&
        cursor.table_column_count == column_count;
    diagnostic.column_anchors_match =
        diagnostic.has_previous_table &&
        have_compatible_column_anchors(cursor.table_column_anchor_x_points,
                                       source.column_anchor_x_points);
    if (!diagnostic.previous_has_repeating_header ||
        !diagnostic.source_has_repeating_header) {
        diagnostic.header_match_kind =
            PdfTableContinuationHeaderMatchKind::not_required;
    } else {
        diagnostic.header_match_kind = row_cell_texts_match_kind(
            cursor.table_header_cell_texts, source.rows.front());
    }
    diagnostic.header_matches_previous =
        diagnostic.header_match_kind != PdfTableContinuationHeaderMatchKind::none;

    if (!diagnostic.has_previous_table) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::no_previous_table;
    } else if (!diagnostic.is_first_block_on_page) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker =
            PdfTableContinuationBlocker::not_first_block_on_page;
    } else if (!diagnostic.is_near_page_top) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::not_near_page_top;
    } else if (!diagnostic.source_rows_consistent) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::inconsistent_source_rows;
    } else if (!diagnostic.column_count_matches) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::column_count_mismatch;
    } else if (!diagnostic.column_anchors_match) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::column_anchors_mismatch;
    } else if (diagnostic.previous_has_repeating_header &&
               diagnostic.source_has_repeating_header &&
               !diagnostic.header_matches_previous) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::repeated_header_mismatch;
    } else {
        diagnostic.disposition =
            PdfTableContinuationDisposition::merged_with_previous_table;
        diagnostic.source_row_offset =
            diagnostic.previous_has_repeating_header &&
                    diagnostic.source_has_repeating_header &&
                    diagnostic.header_matches_previous
                ? 1U
                : 0U;
        diagnostic.skipped_repeating_header =
            diagnostic.source_row_offset > 0U;
        diagnostic.blocker = PdfTableContinuationBlocker::none;
    }

    diagnostic.continuation_confidence = continuation_confidence(diagnostic);
    if (diagnostic.disposition ==
            PdfTableContinuationDisposition::merged_with_previous_table &&
        diagnostic.continuation_confidence < min_continuation_confidence) {
        diagnostic.disposition =
            PdfTableContinuationDisposition::created_new_table;
        diagnostic.source_row_offset = 0U;
        diagnostic.skipped_repeating_header = false;
        diagnostic.blocker =
            PdfTableContinuationBlocker::continuation_confidence_below_threshold;
    }

    return diagnostic;
}

[[nodiscard]] bool append_rows_to_imported_table(
    featherdoc::Table &table, std::size_t start_row_index,
    std::size_t column_count, const PdfParsedTableCandidate &source,
    std::size_t source_row_offset, std::size_t &appended_row_count) {
    if (!has_consistent_row_widths(source, column_count)) {
        return false;
    }

    if (source_row_offset > source.rows.size()) {
        return false;
    }

    appended_row_count = 0U;
    for (std::size_t row_index = source_row_offset; row_index < source.rows.size();
         ++row_index) {
        auto appended_row = table.append_row(column_count);
        if (!appended_row.has_next()) {
            return false;
        }

        const auto &row = source.rows[row_index];
        for (const auto &cell : row.cells) {
            if (cell.has_text &&
                !table.set_cell_text_by_grid_column(
                    start_row_index + appended_row_count, cell.column_index,
                    cell.text)) {
                return false;
            }
        }

        if (!appended_row.set_height_twips(
                table_row_height_twips(source, row),
                featherdoc::row_height_rule::exact)) {
            return false;
        }

        if (!apply_imported_table_column_spans(
                table, start_row_index + appended_row_count, row)) {
            return false;
        }

        ++appended_row_count;
    }

    if (!apply_imported_table_row_span_column_spans(
            table, start_row_index, source, source_row_offset,
            appended_row_count)) {
        return false;
    }

    if (!apply_imported_table_row_spans(table, start_row_index, source,
                                        source_row_offset, appended_row_count)) {
        return false;
    }

    return true;
}

[[nodiscard]] bool append_imported_paragraph(ImportCursor &cursor,
                                             const std::string &text) {
    if (!cursor.has_written_block) {
        if (!cursor.paragraph.has_next()) {
            return false;
        }
        if (!cursor.paragraph.set_text(text)) {
            return false;
        }
        cursor.has_written_block = true;
        cursor.table.reset();
        cursor.table_row_count = 0U;
        cursor.table_column_count = 0U;
        cursor.table_column_anchor_x_points.clear();
        cursor.table_header_cell_texts.clear();
        return true;
    }

    if (cursor.table.has_value()) {
        cursor.paragraph = cursor.table->insert_paragraph_after(text);
    } else {
        cursor.paragraph = cursor.paragraph.insert_paragraph_after(text);
    }

    if (!cursor.paragraph.has_next()) {
        return false;
    }
    cursor.table.reset();
    cursor.table_row_count = 0U;
    cursor.table_column_count = 0U;
    cursor.table_column_anchor_x_points.clear();
    cursor.table_header_cell_texts.clear();
    return true;
}

[[nodiscard]] TableAppendResult append_imported_table(
    featherdoc::Document &document, ImportCursor &cursor,
    const PdfParsedTableCandidate &source,
    const PdfTableContinuationDiagnostic &continuation) {
    const auto column_count = table_column_count(source);
    const bool is_first_written_block = !cursor.has_written_block;
    if (!has_consistent_row_widths(source, column_count)) {
        return TableAppendResult::failed;
    }

    if (continuation.disposition ==
        PdfTableContinuationDisposition::merged_with_previous_table) {
        std::size_t appended_row_count = 0U;
        if (!append_rows_to_imported_table(*cursor.table, cursor.table_row_count,
                                           column_count, source,
                                           continuation.source_row_offset,
                                           appended_row_count)) {
            return TableAppendResult::failed;
        }

        cursor.table_row_count += appended_row_count;
        cursor.has_written_block = true;
        return TableAppendResult::merged;
    }

    featherdoc::Table table;
    if (!cursor.has_written_block) {
        table = document.append_table(source.rows.size(), column_count);
    } else if (cursor.table.has_value()) {
        table = cursor.table->insert_table_after(source.rows.size(), column_count);
    } else {
        table = document.append_table(source.rows.size(), column_count);
    }

    if (!configure_imported_table(table, source)) {
        return TableAppendResult::failed;
    }

    cursor.table = std::move(table);
    cursor.table_row_count = source.rows.size();
    cursor.table_column_count = column_count;
    cursor.table_column_anchor_x_points = source.column_anchor_x_points;
    cursor.table_header_cell_texts =
        should_mark_repeating_header_row(source)
            ? collect_row_cell_texts(source.rows.front())
            : std::vector<std::string>{};
    cursor.has_written_block = true;

    if (is_first_written_block) {
        // 首个导入块如果是表格，删除 create_empty() 带来的占位段落，
        // 避免 body 头部残留一个空白段落。
        if (!cursor.paragraph.remove()) {
            return TableAppendResult::failed;
        }
    }

    return TableAppendResult::created;
}
