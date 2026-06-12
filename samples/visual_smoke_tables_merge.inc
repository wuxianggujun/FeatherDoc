auto create_merge_table(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(1, 4);
    if (!require_step(configure_table(table, 9000U), "merge table configure")) {
        return false;
    }

    auto row = table.rows();
    if (!require_step(row.has_next(), "merge first row exists") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 4>{2250U, 2250U, 2250U, 2250U}),
                      "merge first row widths")) {
        return false;
    }

    auto cell = row.cells();
    if (!require_step(cell.has_next(), "merge first row first cell exists") ||
        !require_step(cell.set_fill_color("D9EAF7"), "merge horizontal fill") ||
        !require_step(add_cell_text(cell, "Horizontal merge", featherdoc::formatting_flag::bold),
                      "merge horizontal text") ||
        !require_step(cell.merge_right(1U), "merge horizontal right")) {
        return false;
    }

    cell = row.cells();
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("E2F0D9") ||
        !add_cell_text(cell, "Reference")) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FCE4D6") ||
        !add_cell_text(cell, "Borders")) {
        return false;
    }

    row = table.append_row(4);
    if (!require_step(row.has_next(), "merge second row exists") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 4>{2250U, 2250U, 2250U, 2250U}),
                      "merge second row widths")) {
        return false;
    }

    auto third_row = table.append_row(4);
    if (!third_row.has_next() ||
        !set_row_widths(third_row, std::array<std::uint32_t, 4>{2250U, 2250U, 2250U, 2250U})) {
        return false;
    }

    cell = row.cells();
    if (!require_step(cell.has_next(), "merge second row first cell exists") ||
        !require_step(cell.set_fill_color("FFF2CC"), "merge vertical fill") ||
        !require_step(add_cell_text(cell, "Vertical merge", featherdoc::formatting_flag::bold),
                      "merge vertical text") ||
        !require_step(cell.merge_down(1U), "merge vertical down")) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center) ||
        !add_cell_lines(cell, {"Center aligned", "Cell-level margins active"}) ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::top, 180U) ||
        !cell.set_margin_twips(featherdoc::cell_margin_edge::bottom, 180U)) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_border(featherdoc::cell_border_edge::right,
                                             {featherdoc::border_style::dashed, 10, "C00000", 0}) ||
        !add_cell_lines(cell, {"Dashed right border", "Should align after export"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() ||
        !cell.set_fill_color("E4DFEC") ||
        !cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center) ||
        !cell.set_text_direction(
            featherdoc::cell_text_direction::top_to_bottom_right_to_left) ||
        !add_cell_text(cell, utf8_from_u8(u8"\u7EB5\u6392\u68C0\u67E5"))) {
        return false;
    }

    cell = third_row.cells();
    if (!cell.has_next()) {
        return false;
    }
    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell, {"Continuation under vertical merge",
                               "The left yellow block should span both rows"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("EDEDED") ||
        !add_cell_text(cell, "Shaded")) {
        return false;
    }

    cell.next();
    if (!cell.has_next()) {
        return false;
    }
    {
        auto mixed_paragraph = cell.paragraphs();
        auto mixed_run = mixed_paragraph.add_run(
            utf8_from_u8(
                u8"\u7A84\u683C mixed RTL/LTR: ABC 12 "
                u8"\u0645\u0631\u062D\u0628\u0627 "
                u8"\u4E2D\u6587"));
        if (!mixed_run.has_next() || !mixed_paragraph.set_bidi() || !mixed_run.set_rtl()) {
            return false;
        }
    }

    row = table.append_row(4);
    if (!row.has_next() ||
        !set_row_widths(row, std::array<std::uint32_t, 4>{2250U, 2250U, 2250U, 2250U})) {
        return false;
    }

    cell = row.cells();
    if (!cell.has_next() || !add_cell_text(cell, "Footer")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_text(cell, "No clipped borders")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_text(cell, "No missing fills")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_text(cell, "No overlap")) {
        return false;
    }

    return true;
}

auto create_fixed_grid_merge_right_table(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(2, 3);
    if (!require_step(configure_table(table, 7000U), "fixed merge table configure") ||
        !require_step(set_table_column_widths(
                          table, std::array<std::uint32_t, 3>{1000U, 1900U, 4100U}),
                      "fixed merge table grid widths")) {
        return false;
    }

    auto row = table.rows();
    if (!require_step(row.has_next(), "fixed merge row one exists") ||
        !require_step(row.set_height_twips(900U, featherdoc::row_height_rule::at_least),
                      "fixed merge row one height")) {
        return false;
    }

    auto merged_cell = row.cells();
    if (!require_step(merged_cell.has_next(), "fixed merge anchor exists") ||
        !require_step(merged_cell.set_fill_color("D9EAF7"), "fixed merge anchor fill") ||
        !require_step(add_cell_lines(merged_cell, {"Fixed-grid merge_right",
                                                   "Expected merged width: 1000 + 1900"}),
                      "fixed merge anchor text") ||
        !require_step(merged_cell.merge_right(1U), "fixed merge anchor merge right")) {
        return false;
    }

    auto tail_cell = merged_cell;
    tail_cell.next();
    if (!require_step(tail_cell.has_next(), "fixed merge tail exists") ||
        !require_step(tail_cell.set_fill_color("E2F0D9"), "fixed merge tail fill") ||
        !require_step(add_cell_lines(tail_cell, {"Tail grid column",
                                                 "4100 twips, should stay visibly widest"}),
                      "fixed merge tail text")) {
        return false;
    }

    row.next();
    if (!require_step(row.has_next(), "fixed merge row two exists") ||
        !require_step(row.set_height_twips(820U, featherdoc::row_height_rule::at_least),
                      "fixed merge row two height")) {
        return false;
    }

    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color("F2F2F2") ||
        !add_cell_lines(cell, {"1000", "Narrow base"})) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FFF2CC") ||
        !add_cell_lines(cell, {"1900", "Medium base"})) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("E2F0D9") ||
        !add_cell_lines(cell, {"4100", "Visual tail cue"})) {
        return false;
    }

    return true;
}

auto create_column_insertion_tables(featherdoc::Document &doc) -> bool {
    auto table = doc.append_table(2, 2);
    if (!require_step(configure_table(table, 8000U), "column insert table configure")) {
        return false;
    }

    auto row = table.rows();
    if (!require_step(row.has_next(), "column insert header exists") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 2>{2000U, 2000U}),
                      "column insert header widths")) {
        return false;
    }

    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") ||
        !add_cell_lines(cell, {"Base", "Insert after this blue column"})) {
        return false;
    }

    auto result_header = cell;
    result_header.next();
    if (!result_header.has_next() || !result_header.set_fill_color("E2F0D9") ||
        !add_cell_lines(result_header, {"Result", "Insert before this green column"})) {
        return false;
    }

    auto body_row = row;
    body_row.next();
    if (!body_row.has_next() ||
        !require_step(set_row_widths(body_row, std::array<std::uint32_t, 2>{2000U, 2000U}),
                      "column insert body widths")) {
        return false;
    }

    cell = body_row.cells();
    if (!cell.has_next() || !add_cell_text(cell, "Owner")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_text(cell, "Completed")) {
        return false;
    }

    auto inserted_after = table.rows().cells().insert_cell_after();
    if (!require_step(inserted_after.has_next(), "insert column after blue header") ||
        !require_step(inserted_after.set_fill_color("FFF2CC"), "color inserted-after header") ||
        !require_step(add_cell_lines(inserted_after, {"Priority", "Inserted after Base"}),
                      "write inserted-after header")) {
        return false;
    }

    auto inserted_after_body = table.rows();
    inserted_after_body.next();
    if (!inserted_after_body.has_next()) {
        return false;
    }
    cell = inserted_after_body.cells();
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FFF2CC") ||
        !add_cell_lines(cell, {"High", "Cloned column stays aligned"})) {
        return false;
    }

    auto inserted_before_anchor = table.rows().cells();
    inserted_before_anchor.next();
    inserted_before_anchor.next();
    if (!inserted_before_anchor.has_next()) {
        return false;
    }
    auto inserted_before = inserted_before_anchor.insert_cell_before();
    if (!require_step(inserted_before.has_next(), "insert column before green header") ||
        !require_step(inserted_before.set_fill_color("FCE4D6"), "color inserted-before header") ||
        !require_step(add_cell_lines(inserted_before, {"Status", "Inserted before Result"}),
                      "write inserted-before header")) {
        return false;
    }

    auto inserted_before_body = table.rows();
    inserted_before_body.next();
    if (!inserted_before_body.has_next()) {
        return false;
    }
    cell = inserted_before_body.cells();
    cell.next();
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FCE4D6") ||
        !add_cell_lines(cell, {"Ready", "No border drift after second insert"})) {
        return false;
    }

    cell.next();
    if (!cell.has_next() ||
        !add_cell_lines(cell, {"Completed", "Green result column stays rightmost"})) {
        return false;
    }

    auto spacer = append_body_paragraph(doc, "");
    if (!spacer.has_next()) {
        return false;
    }

    table = doc.append_table(2, 3);
    if (!require_step(configure_table(table, 8000U), "merged-boundary insert table configure")) {
        return false;
    }

    row = table.rows();
    if (!require_step(row.has_next(), "merged-boundary header exists") ||
        !require_step(set_row_widths(row, std::array<std::uint32_t, 3>{2000U, 2000U, 2000U}),
                      "merged-boundary header widths")) {
        return false;
    }

    auto merged_header = row.cells();
    if (!merged_header.has_next() || !merged_header.set_fill_color("D9EAF7") ||
        !add_cell_lines(merged_header, {"Merged block", "Safe insert goes to the right"}) ||
        !merged_header.merge_right(1U)) {
        return false;
    }

    auto tail_header = merged_header;
    tail_header.next();
    if (!tail_header.has_next() || !tail_header.set_fill_color("E2F0D9") ||
        !add_cell_lines(tail_header, {"Tail", "Should stay after the new yellow cell"})) {
        return false;
    }

    body_row = row;
    body_row.next();
    if (!body_row.has_next() ||
        !require_step(set_row_widths(body_row, std::array<std::uint32_t, 3>{2000U, 2000U, 2000U}),
                      "merged-boundary body widths")) {
        return false;
    }

    cell = body_row.cells();
    if (!cell.has_next() || !add_cell_text(cell, "R2C1")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_text(cell, "R2C2")) {
        return false;
    }
    cell.next();
    if (!cell.has_next() || !add_cell_text(cell, "Tail body")) {
        return false;
    }

    auto merged_inserted = table.rows().cells().insert_cell_after();
    if (!require_step(merged_inserted.has_next(), "insert after merged block") ||
        !require_step(merged_inserted.set_fill_color("FFF2CC"),
                      "color merged-boundary inserted header") ||
        !require_step(add_cell_lines(merged_inserted,
                                     {"Inserted after merge", "No stray gridSpan or vMerge"}),
                      "write merged-boundary inserted header")) {
        return false;
    }

    auto merged_body_row = table.rows();
    merged_body_row.next();
    if (!merged_body_row.has_next()) {
        return false;
    }
    cell = merged_body_row.cells();
    cell.next();
    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("FFF2CC") ||
        !add_cell_lines(cell, {"Boundary insert", "Should sit between blue and green"})) {
        return false;
    }

    return true;
}
