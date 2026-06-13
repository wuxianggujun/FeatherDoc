auto create_column_width_table(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(3, 3);
    if (!require_step(configure_table(table, 7800U), "column width table configure") ||
        !require_step(table.set_layout_mode(featherdoc::table_layout_mode::autofit),
                      "column width table temporary autofit") ||
        !require_step(set_table_column_widths(table,
                                             std::array<std::uint32_t, 3>{1200U, 2200U, 4400U}),
                      "column width table grid widths") ||
        !require_step(table.clear_column_width(1U), "column width table clear middle width") ||
        !require_step(table.set_column_width_twips(1U, 2200U),
                      "column width table restore middle width") ||
        !require_step(table.clear_width(), "column width table clear tblW") ||
        !require_step(table.set_width_twips(7800U), "column width table restore tblW") ||
        !require_step(table.clear_layout_mode(), "column width table clear layout mode") ||
        !require_step(table.set_layout_mode(featherdoc::table_layout_mode::fixed),
                      "column width table normalize tcW from grid")) {
        return false;
    }

    auto override_row = table.rows();
    override_row.next();
    if (!require_step(override_row.has_next(), "column width override row exists")) {
        return false;
    }

    auto override_cell = override_row.cells();
    override_cell.next();
    if (!require_step(override_cell.has_next(), "column width override middle cell exists") ||
        !require_step(override_cell.set_width_twips(900U),
                      "column width temporary tcW override")) {
        return false;
    }
    override_cell.next();
    if (!require_step(override_cell.has_next(), "column width clear target cell exists") ||
        !require_step(override_cell.clear_width(), "column width temporary tcW clear") ||
        !require_step(table.set_layout_mode(featherdoc::table_layout_mode::fixed),
                      "column width restore tcW from grid after manual cell edits")) {
        return false;
    }

    auto row = table.rows();
    if (!require_step(row.has_next(), "column width header exists") ||
        !require_step(row.set_height_twips(520U, featherdoc::row_height_rule::exact),
                      "column width header height")) {
        return false;
    }

    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Key", featherdoc::formatting_flag::bold)) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FFF2CC") ||
        !add_cell_text(cell, "Review state", featherdoc::formatting_flag::bold)) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("E2F0D9") ||
        !add_cell_text(cell, "Evidence / notes", featherdoc::formatting_flag::bold)) {
        return false;
    }

    row.next();
    if (!require_step(row.has_next(), "column width body row one exists") ||
        !require_step(row.set_height_twips(1050U, featherdoc::row_height_rule::at_least),
                      "column width body row one height")) {
        return false;
    }

    cell = row.cells();
    if (!cell.has_next() || !add_cell_text(cell, "A-7")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_lines(cell, {"Waiting", "Middle column stays readable"})) {
        return false;
    }
    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell,
                        {"Wide evidence column",
                         "This green column should remain visibly wider than the blue key column "
                         "after Table::set_column_width_twips(...), even after a temporary "
                         "tcW override/clear cycle." })) {
        return false;
    }

    row.next();
    if (!require_step(row.has_next(), "column width body row two exists") ||
        !require_step(row.set_height_twips(1050U, featherdoc::row_height_rule::at_least),
                      "column width body row two height")) {
        return false;
    }

    cell = row.cells();
    if (!cell.has_next() || !add_cell_text(cell, "B-2")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_lines(cell, {"Ready", "Center column should stay medium"})) {
        return false;
    }
    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell,
                        {"Regression cue",
                         "If all three columns look nearly equal in Word, tblGrid column-width "
                         "editing likely regressed."})) {
        return false;
    }

    return true;
}

auto create_unmerge_table(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(4, 3);
    if (!require_step(configure_table(table, 9000U), "unmerge table configure")) {
        return false;
    }

    auto row = table.rows();
    if (!require_step(row.has_next(), "unmerge header exists") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 3>{3000U, 3000U, 3000U}),
                      "unmerge header widths")) {
        return false;
    }

    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Case", featherdoc::formatting_flag::bold)) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Expected final layout", featherdoc::formatting_flag::bold)) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Review notes", featherdoc::formatting_flag::bold)) {
        return false;
    }

    row.next();
    if (!require_step(row.has_next(), "unmerge horizontal row exists") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 3>{3000U, 3000U, 3000U}),
                      "unmerge horizontal row widths")) {
        return false;
    }

    auto horizontal_anchor = row.cells();
    if (!horizontal_anchor.has_next() || !horizontal_anchor.set_fill_color("DDEBF7") ||
        !add_cell_lines(horizontal_anchor, {"Horizontal merge anchor",
                                           "This row is merged first, then split again"}) ||
        !horizontal_anchor.merge_right(1U) ||
        !horizontal_anchor.unmerge_right()) {
        return false;
    }

    auto restored_horizontal = row.cells();
    restored_horizontal.next();
    if (!restored_horizontal.has_next() || !restored_horizontal.set_fill_color("FCE4D6") ||
        !add_cell_lines(restored_horizontal, {"Restored right cell",
                                             "Should be a standalone orange cell"})) {
        return false;
    }

    auto horizontal_tail = restored_horizontal;
    horizontal_tail.next();
    if (!horizontal_tail.has_next() || !horizontal_tail.set_fill_color("E2F0D9") ||
        !add_cell_lines(horizontal_tail, {"Tail", "Must stay on the far right"})) {
        return false;
    }

    row.next();
    if (!require_step(row.has_next(), "unmerge vertical anchor row exists") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 3>{3000U, 3000U, 3000U}),
                      "unmerge vertical anchor row widths")) {
        return false;
    }

    auto vertical_anchor = row.cells();
    if (!vertical_anchor.has_next() || !vertical_anchor.set_fill_color("FFF2CC") ||
        !add_cell_lines(vertical_anchor, {"Vertical merge anchor",
                                         "Upper cell keeps the original text"}) ||
        !vertical_anchor.merge_down(1U)) {
        return false;
    }

    cell = row.cells();
    cell.next();
    if (!cell.has_next() || !add_cell_lines(cell, {"Upper row detail", "Should remain visible"})) {
        return false;
    }
    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell, {"Review border continuity", "No stray merge markers after split"})) {
        return false;
    }

    row.next();
    if (!require_step(row.has_next(), "unmerge vertical continuation row exists") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 3>{3000U, 3000U, 3000U}),
                      "unmerge vertical continuation row widths")) {
        return false;
    }

    cell = row.cells();
    if (!cell.has_next() || !cell.unmerge_down() || !cell.set_fill_color("E2F0D9") ||
        !add_cell_lines(cell, {"Restored lower cell", "Should be an independent green cell"})) {
        return false;
    }
    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell, {"Lower row detail", "Middle content must stay in place"})) {
        return false;
    }
    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell, {"Check horizontal rules", "No collapsed row height"})) {
        return false;
    }

    return true;
}

auto create_direction_stress_table(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(1, 4);
    if (!require_step(configure_table(table, 7200U), "direction stress table configure")) {
        return false;
    }

    auto row = table.rows();
    if (!require_step(row.has_next(), "direction stress header exists") ||
        !require_step(row.set_height_twips(480U, featherdoc::row_height_rule::exact),
                      "direction stress header height") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 4>{850U, 1450U, 1750U, 3150U}),
                      "direction stress header widths")) {
        return false;
    }

    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Cell", featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Mode", featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Stress content", featherdoc::formatting_flag::bold)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_text(cell, "Expected review points", featherdoc::formatting_flag::bold)) {
        return false;
    }

    row = table.append_row(4);
    if (!row.has_next() ||
        !set_row_widths(row, std::array<std::uint32_t, 4>{850U, 1450U, 1750U, 3150U}) ||
        !row.set_height_twips(2100U, featherdoc::row_height_rule::at_least)) {
        return false;
    }

    cell = row.cells();
    if (!cell.has_next() ||
        !cell.set_fill_color("E4DFEC") ||
        !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center) ||
        !cell.set_text_direction(
            featherdoc::cell_text_direction::top_to_bottom_right_to_left) ||
        !add_cell_lines(cell, {utf8_from_u8(u8"\u7EB5\u6392"), "TBRL"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FCE4D6") ||
        !add_cell_lines(cell, {"w:textDirection", "topToBottom-rl"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FFF2CC") ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::left, 54U) ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::right, 54U)) {
        return false;
    }
    {
        auto mixed_paragraph = cell.paragraphs();
        auto mixed_run = mixed_paragraph.add_run(
            utf8_from_u8(u8"ABC-2026 \u0645\u0631\u062D\u0628\u0627 \u4E2D\u6587"));
        if (!mixed_run.has_next() || !mixed_paragraph.set_bidi() || !mixed_run.set_rtl()) {
            return false;
        }

        mixed_paragraph = mixed_paragraph.insert_paragraph_after(
            utf8_from_u8(u8"\u7A84\u683C wrap \u987A\u5E8F\u68C0\u67E5"));
        if (!mixed_paragraph.has_next()) {
            return false;
        }
    }

    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell, {"Vertical text should stay readable.",
                               "Borders must stay continuous.",
                               "No clipped glyphs or reversed order."})) {
        return false;
    }

    row = table.append_row(4);
    if (!row.has_next() ||
        !set_row_widths(row, std::array<std::uint32_t, 4>{850U, 1450U, 1750U, 3150U}) ||
        !row.set_height_twips(2300U, featherdoc::row_height_rule::at_least)) {
        return false;
    }

    cell = row.cells();
    if (!cell.has_next() ||
        !cell.set_fill_color("DDEBF7") ||
        !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center) ||
        !cell.set_text_direction(
            featherdoc::cell_text_direction::top_to_bottom_left_to_right_rotated) ||
        !add_cell_lines(cell, {utf8_from_u8(u8"\u65CB\u8F6C"), "TLR rot"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("E2F0D9") ||
        !add_cell_lines(cell, {"Style inheritance", "Quote + Emphasis"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FDF2E9") ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::left, 54U) ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::right, 54U)) {
        return false;
    }
    {
        auto styled_paragraph = cell.paragraphs();
        auto styled_run = styled_paragraph.add_run(
            utf8_from_u8(u8"Quote/Emphasis \u0627\u0644\u0639\u0631\u0628\u064A\u0629 ABC \u4E2D\u6587"));
        if (!styled_run.has_next() || !doc.set_paragraph_style(styled_paragraph, "Quote") ||
            !doc.set_run_style(styled_run, "Emphasis")) {
            return false;
        }

        styled_paragraph = styled_paragraph.insert_paragraph_after(
            utf8_from_u8(
                u8"\u89C2\u5BDF\u65CB\u8F6C\u5217\u65C1\u8FB9\u662F\u5426\u51FA\u73B0\u9519\u4F4D"));
        if (!styled_paragraph.has_next()) {
            return false;
        }
    }

    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell, {"Check wrap stability beside rotated text.",
                               "No fallback-font drift or overlap.",
                               "Keep borders aligned with neighboring cells."})) {
        return false;
    }

    return true;
}
