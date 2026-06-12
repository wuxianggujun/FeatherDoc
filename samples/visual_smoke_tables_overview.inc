auto create_overview_table(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(1, 3);
    if (!require_step(configure_table(table, 9000U), "overview table configure")) {
        return false;
    }

    auto first_row = table.rows();
    if (!require_step(first_row.has_next(), "overview first row exists") ||
        !require_step(first_row.set_height_twips(520U, featherdoc::row_height_rule::exact),
                      "overview first row height")) {
        return false;
    }
    if (!require_step(set_row_widths(first_row, std::array<std::uint32_t, 3>{3000U, 3000U, 3000U}),
                      "overview first row widths")) {
        return false;
    }

    auto cell = first_row.cells();
    if (!require_step(cell.has_next(), "overview first row first cell exists") ||
        !require_step(cell.set_fill_color("D9EAF7"), "overview banner fill") ||
        !require_step(cell.set_border(featherdoc::cell_border_edge::bottom,
                                      {featherdoc::border_style::thick, 12, "1F1F1F", 0}),
                      "overview banner border") ||
        !require_step(add_cell_text(cell,
                                    "Overview banner: fixed-width table, merged cells, borders, and margins",
                                    featherdoc::formatting_flag::bold),
                      "overview banner text") ||
        !require_step(cell.merge_right(2U), "overview banner merge right")) {
        return false;
    }

    auto second_row = table.append_row(3);
    if (!require_step(second_row.has_next(), "overview second row exists") ||
        !require_step(set_row_widths(second_row, std::array<std::uint32_t, 3>{3000U, 3000U, 3000U}),
                      "overview second row widths")) {
        return false;
    }

    auto third_row = table.append_row(3);
    if (!third_row.has_next() ||
        !set_row_widths(third_row, std::array<std::uint32_t, 3>{3000U, 3000U, 3000U})) {
        return false;
    }

    cell = second_row.cells();
    if (!require_step(cell.has_next(), "overview second row first cell exists") ||
        !require_step(cell.set_fill_color("FFF2CC"), "overview vertical merge fill") ||
        !require_step(cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center),
                      "overview vertical merge valign") ||
        !require_step(add_cell_text(cell, "Vertical merge sample", featherdoc::formatting_flag::bold),
                      "overview vertical merge text") ||
        !require_step(cell.merge_down(1U), "overview vertical merge down")) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("E2F0D9") ||
        !add_cell_lines(cell,
                        {"Cell fill + per-edge border",
                         "Expected: green background and thick dark bottom edge"}) ||
        !cell.set_border(featherdoc::cell_border_edge::bottom,
                         {featherdoc::border_style::thick, 12, "2F5597", 0})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FCE4D6") ||
        !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center) ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::left, 180U) ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::right, 180U)) {
        return false;
    }

    auto cjk_paragraph = cell.paragraphs();
    auto cjk_run = cjk_paragraph.add_run(
        utf8_from_u8(u8"\u4E2D\u6587/CJK \u6DF7\u6392\u68C0\u67E5"),
        featherdoc::formatting_flag::bold);
    if (!cjk_run.has_next()) {
        return false;
    }

    cjk_paragraph = cjk_paragraph.insert_paragraph_after(
        utf8_from_u8(
            u8"\u9884\u671F\uff1A\u8868\u683C\u5BBD\u5EA6\u4E0D\u88AB\u4E2D\u6587\u6491\u7206\uFF0C"
            u8"Invoice INV-2026-0001 \u6362\u884C\u4ECD\u7136\u7A33\u5B9A"));
    if (!cjk_paragraph.has_next()) {
        return false;
    }

    cell = third_row.cells();
    if (!cell.has_next()) {
        return false;
    }

    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell,
                        {"Horizontal merge preserved in first row",
                         "Vertical merge from left column should continue here"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell,
                        {"Inside borders should stay continuous",
                         "No clipped shading or unexpected spacing"})) {
        return false;
    }

    return true;
}

auto create_multipage_table(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(1, 4);
    if (!require_step(configure_table(table, 9800U), "multipage table configure")) {
        return false;
    }

    auto header = table.rows();
    if (!require_step(header.has_next(), "multipage header exists") ||
        !require_step(header.set_height_twips(480U, featherdoc::row_height_rule::exact),
                      "multipage header height") ||
        !require_step(header.set_repeats_header(), "multipage header repeat") ||
        !require_step(header.set_cant_split(), "multipage header cantSplit") ||
        !require_step(set_row_widths(header, std::array<std::uint32_t, 4>{900U, 2500U, 3600U, 2800U}),
                      "multipage header widths")) {
        return false;
    }

    auto cell = header.cells();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Case", featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Expected rendering", featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Stress content", featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Notes", featherdoc::formatting_flag::bold)) {
        return false;
    }

    for (std::size_t row_index = 1; row_index <= 24U; ++row_index) {
        auto row = table.append_row(4);
        if (!require_step(row.has_next(), "multipage row exists") ||
            !require_step(set_row_widths(row, std::array<std::uint32_t, 4>{900U, 2500U, 3600U, 2800U}),
                          "multipage row widths")) {
            return false;
        }

        if (row_index == 16U &&
            !require_step(row.set_cant_split(), "multipage highlighted row cantSplit")) {
            return false;
        }

        cell = row.cells();
        if (!cell.has_next()) {
            return false;
        }

        const auto case_label = std::string("R") + std::to_string(row_index);
        if (row_index == 16U && !cell.set_fill_color("FCE4D6")) {
            return false;
        }
        if (!add_cell_text(cell, case_label, featherdoc::formatting_flag::bold)) {
            return false;
        }

        cell.next();
        if (!cell.has_next()) {
            return false;
        }
        if (row_index % 2U == 0U && !cell.set_fill_color("F9F9F9")) {
            return false;
        }
        if (!add_cell_lines(cell,
                            {"Keep borders continuous",
                             row_index == 16U ? "Highlighted row should move as a whole"
                                              : "Header must repeat on later pages"})) {
            return false;
        }

        cell.next();
        if (!cell.has_next()) {
            return false;
        }
        std::vector<std::string> stress_lines;
        if (row_index == 16U) {
            stress_lines = {
                "Large cantSplit row:",
                "line 1 - this block should not break across pages.",
                "line 2 - Word should push the whole row forward when needed.",
                "line 3 - borders should remain intact around the tall content.",
                "line 4 - repeated header should still appear above the row.",
                "line 5 - no clipped fill or missing inner borders.",
                "line 6 - text should remain readable.",
                "line 7 - margin spacing should be preserved.",
                "line 8 - background shading should stay visible.",
                "line 9 - check that the row is fully on one page.",
                "line 10 - this is the row to inspect first.",
            };
            if (!cell.set_fill_color("FFF2CC")) {
                return false;
            }
        } else {
            stress_lines = {
                "Baseline content for row " + std::to_string(row_index),
                "Two-line cell keeps the table tall enough to span pages.",
            };
        }
        if (!add_cell_lines(cell, stress_lines)) {
            return false;
        }

        cell.next();
        if (!cell.has_next()) {
            return false;
        }
        if (row_index % 3U == 0U &&
            !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center)) {
            return false;
        }
        if (!add_cell_lines(cell,
                            {row_index == 16U ? "Inspect this row in the PNG review."
                                              : "Alternating rows help expose spacing drift.",
                             "No unexpected overlap or border gaps."})) {
            return false;
        }
    }

    return true;
}
