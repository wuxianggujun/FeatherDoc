#include "pdf_document_adapter_font_test_support.hpp"

TEST_CASE("document PDF adapter preserves body order when laying out tables") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto before = document.paragraphs();
    REQUIRE(before.has_next());
    CHECK(before.set_text("Before table"));

    auto table = document.append_table(2U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(7200U));
    CHECK(table.set_column_width_twips(0U, 2400U));
    CHECK(table.set_column_width_twips(1U, 4800U));
    CHECK(table.set_cell_text(0U, 0U, "Name"));
    CHECK(table.set_cell_text(0U, 1U, "Amount"));
    CHECK(table.set_cell_text(1U, 0U, "Service"));
    CHECK(table.set_cell_text(1U, 1U, "100"));

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto after = body_template.append_paragraph("After table");
    REQUIRE(after.has_next());

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 3U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[0].item_index, 0U);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);
    CHECK_EQ(blocks[1].item_index, 0U);
    CHECK_EQ(blocks[2].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[2].item_index, 1U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    CHECK_GE(layout.pages.front().rectangles.size(), 4U);

    std::vector<std::string> text_runs;
    for (const auto &run : layout.pages.front().text_runs) {
        if (!run.text.empty()) {
            text_runs.push_back(run.text);
        }
    }

    REQUIRE_GE(text_runs.size(), 6U);
    CHECK_EQ(text_runs[0], "Before table");
    CHECK_NE(std::find(text_runs.begin(), text_runs.end(), "Name"),
             text_runs.end());
    CHECK_NE(std::find(text_runs.begin(), text_runs.end(), "Amount"),
             text_runs.end());
    CHECK_EQ(text_runs.back(), "After table");

    const auto before_y =
        layout.pages.front().text_runs.front().baseline_origin.y_points;
    const auto after_y =
        layout.pages.front().text_runs.back().baseline_origin.y_points;
    CHECK_GT(before_y, after_y);
}

TEST_CASE("document PDF adapter keeps text after positioned tables below them") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto before = document.paragraphs();
    REQUIRE(before.has_next());
    CHECK(before.set_text("Before positioned table"));

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));
    CHECK(table.set_cell_text(0U, 0U, "Positioned cell"));

    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));

    auto position = featherdoc::table_position{};
    position.horizontal_reference =
        featherdoc::table_position_horizontal_reference::margin;
    position.horizontal_offset_twips = 0;
    position.vertical_reference =
        featherdoc::table_position_vertical_reference::paragraph;
    position.vertical_offset_twips = 0;
    position.bottom_from_text_twips = 240U;
    CHECK(table.set_position(position));

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto after = body_template.append_paragraph("After positioned table");
    REQUIRE(after.has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 6.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

    const auto &table_rect = layout.pages.front().rectangles.front();
    const featherdoc::pdf::PdfTextRun *after_text = nullptr;
    for (const auto &run : layout.pages.front().text_runs) {
        if (run.text == "After positioned table") {
            after_text = &run;
            break;
        }
    }

    REQUIRE(after_text != nullptr);
    CHECK_LT(after_text->baseline_origin.y_points, table_rect.bounds.y_points);
    CHECK(after_text->baseline_origin.y_points ==
          doctest::Approx(table_rect.bounds.y_points - 12.0 -
                          options.paragraph_spacing_after_points -
                          options.font_size_points));
}

TEST_CASE("document PDF adapter repeats table header rows across pages") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(6U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));
    auto position = featherdoc::table_position{};
    position.horizontal_reference =
        featherdoc::table_position_horizontal_reference::margin;
    position.horizontal_offset_twips = 0;
    position.vertical_reference =
        featherdoc::table_position_vertical_reference::page;
    position.vertical_offset_twips = 720;
    CHECK(table.set_position(position));
    CHECK(table.set_cell_text(0U, 0U, "Header"));
    CHECK(table.set_cell_text(0U, 1U, "Amount"));
    for (std::size_t row_index = 1U; row_index < 6U; ++row_index) {
        CHECK(table.set_cell_text(row_index, 0U,
                                  "Row " + std::to_string(row_index)));
        CHECK(table.set_cell_text(row_index, 1U,
                                  std::to_string(row_index * 10U)));
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        REQUIRE(row.has_next());
        CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));
        if (row_index == 0U) {
            CHECK(row.set_repeats_header());
        }
        row.next();
    }

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    REQUIRE_EQ(table_summary->row_repeats_header.size(), 6U);
    CHECK(table_summary->row_repeats_header[0]);
    CHECK_FALSE(table_summary->row_repeats_header[1]);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 0.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_GE(layout.pages.size(), 2U);

    auto page_text_count = [](const featherdoc::pdf::PdfPageLayout &page,
                              std::string_view text) {
        auto count = 0U;
        for (const auto &run : page.text_runs) {
            if (run.text == text) {
                ++count;
            }
        }
        return count;
    };
    auto page_contains_row_text =
        [](const featherdoc::pdf::PdfPageLayout &page) {
            for (const auto &run : page.text_runs) {
                if (run.text.rfind("Row ", 0U) == 0U) {
                    return true;
                }
            }
            return false;
        };

    CHECK_EQ(page_text_count(layout.pages[0], "Header"), 1U);
    CHECK_EQ(page_text_count(layout.pages[1], "Header"), 1U);
    CHECK(page_contains_row_text(layout.pages[1]));
    for (const auto &page : layout.pages) {
        for (const auto &rectangle : page.rectangles) {
            CHECK_GE(rectangle.bounds.y_points,
                     options.margin_bottom_points - 0.01);
        }
    }
}

TEST_CASE("document PDF adapter keeps cant-split table rows on one page") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto before = document.paragraphs();
    REQUIRE(before.has_next());
    CHECK(before.set_text("Before cant-split table"));

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));
    CHECK(table.set_cell_text(0U, 0U, "Cant split row"));

    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.set_height_twips(1600U, featherdoc::row_height_rule::exact));
    CHECK(row.set_cant_split());

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    REQUIRE_EQ(table_summary->row_cant_split.size(), 1U);
    CHECK(table_summary->row_cant_split[0]);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 160.0};
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 6.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 2U);

    auto page_has_text = [](const featherdoc::pdf::PdfPageLayout &page,
                            std::string_view text) {
        for (const auto &run : page.text_runs) {
            if (run.text == text) {
                return true;
            }
        }
        return false;
    };

    CHECK(page_has_text(layout.pages[0], "Before cant-split table"));
    CHECK_FALSE(page_has_text(layout.pages[0], "Cant split row"));
    CHECK(page_has_text(layout.pages[1], "Cant split row"));
    REQUIRE_FALSE(layout.pages[1].rectangles.empty());
    CHECK_GE(layout.pages[1].rectangles.front().bounds.y_points,
             options.margin_bottom_points - 0.01);
}

TEST_CASE("document PDF adapter lays out merged table cells") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2U, 3U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(9000U));
    CHECK(table.set_column_width_twips(0U, 3000U));
    CHECK(table.set_column_width_twips(1U, 3000U));
    CHECK(table.set_column_width_twips(2U, 3000U));
    auto top_left = table.find_cell(0U, 0U);
    REQUIRE(top_left.has_value());
    CHECK(top_left->set_text("Merged heading"));
    CHECK(top_left->merge_right());
    CHECK(table.set_cell_text(0U, 1U, "Tail"));
    CHECK(table.set_cell_text(1U, 0U, "A"));
    CHECK(table.set_cell_text(1U, 1U, "B"));
    CHECK(table.set_cell_text(1U, 2U, "C"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().rectangles.size(), 5U);

    const auto &first_cell = layout.pages.front().rectangles.front();
    const auto &second_cell = layout.pages.front().rectangles[1];
    CHECK_GT(first_cell.bounds.width_points, second_cell.bounds.width_points);
    CHECK_EQ(layout.pages.front().text_runs.front().text, "Merged heading");
}

TEST_CASE("document PDF adapter lays out vertically merged table cells") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));

    auto first_row = table.rows();
    REQUIRE(first_row.has_next());
    CHECK(first_row.set_height_twips(720U, featherdoc::row_height_rule::exact));
    first_row.next();
    REQUIRE(first_row.has_next());
    CHECK(first_row.set_height_twips(720U, featherdoc::row_height_rule::exact));

    auto merged_cell = table.find_cell(0U, 0U);
    REQUIRE(merged_cell.has_value());
    CHECK(merged_cell->set_text("Merged down"));
    CHECK(merged_cell->merge_down());
    CHECK(table.set_cell_text(0U, 1U, "Top right"));
    CHECK(table.set_cell_text(1U, 1U, "Bottom right"));

    const auto cells = document.inspect_table_cells(0U);
    REQUIRE_EQ(cells.size(), 4U);
    CHECK_EQ(cells[0].vertical_merge, featherdoc::cell_vertical_merge::restart);
    CHECK_EQ(cells[0].row_span, 2U);
    CHECK_EQ(cells[2].vertical_merge,
             featherdoc::cell_vertical_merge::continue_merge);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 3U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 3U);

    const auto &merged_rectangle = layout.pages.front().rectangles.front();
    CHECK(merged_rectangle.bounds.height_points == doctest::Approx(72.0));
    CHECK_EQ(layout.pages.front().text_runs[0].text, "Merged");
    CHECK_EQ(layout.pages.front().text_runs[1].text, "down");
}

TEST_CASE("document PDF adapter keeps exact-height table header text visible") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(4U, 3U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(9000U));
    CHECK(table.set_column_width_twips(0U, 2400U));
    CHECK(table.set_column_width_twips(1U, 2400U));
    CHECK(table.set_column_width_twips(2U, 4200U));

    auto rows = table.rows();
    REQUIRE(rows.has_next());
    CHECK(rows.set_repeats_header());
    CHECK(rows.set_height_twips(360U, featherdoc::row_height_rule::exact));
    rows.next();
    REQUIRE(rows.has_next());
    CHECK(rows.set_repeats_header());
    CHECK(rows.set_height_twips(360U, featherdoc::row_height_rule::exact));

    auto merged_header = table.find_cell(0U, 0U);
    REQUIRE(merged_header.has_value());
    CHECK(merged_header->set_text("Merged banner"));
    CHECK(merged_header->merge_right(2U));

    CHECK(table.set_cell_text(1U, 0U, "Case"));
    CHECK(table.set_cell_text(1U, 1U, "Owner"));
    CHECK(table.set_cell_text(1U, 2U, "Notes"));
    CHECK(table.set_cell_text(2U, 0U, "A-01"));
    CHECK(table.set_cell_text(2U, 1U, "Ops"));
    CHECK(table.set_cell_text(2U, 2U, "Body row"));
    CHECK(table.set_cell_text(3U, 0U, "A-02"));
    CHECK(table.set_cell_text(3U, 1U, "QA"));
    CHECK(table.set_cell_text(3U, 2U, "Follow-up"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    const auto page_has_text = [](const featherdoc::pdf::PdfPageLayout &page,
                                  std::string_view text) {
        return std::any_of(page.text_runs.begin(), page.text_runs.end(),
                           [text](const featherdoc::pdf::PdfTextRun &run) {
                               return run.text == text;
                           });
    };

    CHECK(page_has_text(layout.pages.front(), "Merged banner"));
    CHECK(page_has_text(layout.pages.front(), "Case"));
    CHECK(page_has_text(layout.pages.front(), "Owner"));
    CHECK(page_has_text(layout.pages.front(), "Notes"));
}

TEST_CASE("document PDF adapter maps table cell fill and margins") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    CHECK(cell->set_text("Padded cell"));
    CHECK(cell->set_fill_color("D9EAF7"));
    CHECK(cell->set_margin_twips(featherdoc::cell_margin_edge::top, 120U));
    CHECK(cell->set_margin_twips(featherdoc::cell_margin_edge::left, 240U));
    CHECK(cell->set_margin_twips(featherdoc::cell_margin_edge::bottom, 360U));
    CHECK(cell->set_margin_twips(featherdoc::cell_margin_edge::right, 480U));

    const auto cells = document.inspect_table_cells(0U);
    REQUIRE_EQ(cells.size(), 1U);
    REQUIRE(cells.front().fill_color.has_value());
    CHECK_EQ(*cells.front().fill_color, "D9EAF7");
    CHECK_EQ(cells.front().margin_top_twips.value_or(0U), 120U);
    CHECK_EQ(cells.front().margin_left_twips.value_or(0U), 240U);
    CHECK_EQ(cells.front().margin_bottom_twips.value_or(0U), 360U);
    CHECK_EQ(cells.front().margin_right_twips.value_or(0U), 480U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &rectangle = layout.pages.front().rectangles.front();
    CHECK(rectangle.fill);
    CHECK(rectangle.fill_color.red == doctest::Approx(217.0 / 255.0));
    CHECK(rectangle.fill_color.green == doctest::Approx(234.0 / 255.0));
    CHECK(rectangle.fill_color.blue == doctest::Approx(247.0 / 255.0));
    CHECK(rectangle.bounds.x_points ==
          doctest::Approx(options.margin_left_points));
    CHECK(rectangle.bounds.width_points == doctest::Approx(144.0));
    CHECK(rectangle.bounds.height_points == doctest::Approx(38.0));

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, "Padded cell");
    CHECK(text_run.baseline_origin.x_points ==
          doctest::Approx(rectangle.bounds.x_points + 12.0));

    const auto row_top =
        rectangle.bounds.y_points + rectangle.bounds.height_points;
    CHECK(text_run.baseline_origin.y_points ==
          doctest::Approx(row_top - 6.0 - options.font_size_points));
}

TEST_CASE("document PDF adapter uses table default cell margins") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::top, 100U));
    CHECK(
        table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 200U));
    CHECK(table.set_cell_margin_twips(featherdoc::cell_margin_edge::bottom,
                                      300U));
    CHECK(
        table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 400U));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    CHECK(cell->set_text("Default margin"));

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    CHECK_EQ(table_summary->cell_margin_top_twips.value_or(0U), 100U);
    CHECK_EQ(table_summary->cell_margin_left_twips.value_or(0U), 200U);
    CHECK_EQ(table_summary->cell_margin_bottom_twips.value_or(0U), 300U);
    CHECK_EQ(table_summary->cell_margin_right_twips.value_or(0U), 400U);

    const auto cells = document.inspect_table_cells(0U);
    REQUIRE_EQ(cells.size(), 1U);
    CHECK_FALSE(cells.front().margin_left_twips.has_value());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &rectangle = layout.pages.front().rectangles.front();
    CHECK(rectangle.bounds.height_points == doctest::Approx(34.0));

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK(text_run.baseline_origin.x_points ==
          doctest::Approx(rectangle.bounds.x_points + 10.0));
    const auto row_top =
        rectangle.bounds.y_points + rectangle.bounds.height_points;
    CHECK(text_run.baseline_origin.y_points ==
          doctest::Approx(row_top - 5.0 - options.font_size_points));
}
