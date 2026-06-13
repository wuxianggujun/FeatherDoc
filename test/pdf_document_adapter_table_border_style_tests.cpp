#include "pdf_document_adapter_font_test_support.hpp"

TEST_CASE("document PDF adapter maps table cell spacing") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));
    CHECK(table.set_cell_spacing_twips(240U));
    CHECK(table.set_cell_text(0U, 0U, "A"));
    CHECK(table.set_cell_text(0U, 1U, "B"));
    CHECK(table.set_cell_text(1U, 0U, "C"));
    CHECK(table.set_cell_text(1U, 1U, "D"));

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    REQUIRE(table_summary->cell_spacing_twips.has_value());
    CHECK_EQ(*table_summary->cell_spacing_twips, 240U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 4U);

    const auto &top_left = layout.pages.front().rectangles[0];
    const auto &top_right = layout.pages.front().rectangles[1];
    const auto &bottom_left = layout.pages.front().rectangles[2];

    CHECK(top_right.bounds.x_points ==
          doctest::Approx(top_left.bounds.x_points +
                          top_left.bounds.width_points + 12.0));
    CHECK(bottom_left.bounds.y_points + bottom_left.bounds.height_points ==
          doctest::Approx(top_left.bounds.y_points - 12.0));
}

TEST_CASE("document PDF adapter maps table and cell borders") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(1440U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_border(
        featherdoc::table_border_edge::top,
        featherdoc::border_definition{featherdoc::border_style::single, 8U,
                                      "4472C4", 0U}));
    CHECK(table.set_border(
        featherdoc::table_border_edge::left,
        featherdoc::border_definition{featherdoc::border_style::single, 4U,
                                      "666666", 0U}));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    CHECK(cell->set_text("Bordered"));
    CHECK(cell->set_border(
        featherdoc::cell_border_edge::top,
        featherdoc::border_definition{featherdoc::border_style::single, 16U,
                                      "C00000", 0U}));

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    REQUIRE(table_summary->borders.has_value());
    REQUIRE(table_summary->borders->left.has_value());
    CHECK_EQ(table_summary->borders->left->color, "666666");

    const auto cells = document.inspect_table_cells(0U);
    REQUIRE_EQ(cells.size(), 1U);
    REQUIRE(cells.front().border_top.has_value());
    CHECK_EQ(cells.front().border_top->size_eighth_points, 16U);
    CHECK_EQ(cells.front().border_top->color, "C00000");

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().lines.size(), 2U);

    const auto &top_border = layout.pages.front().lines[0];
    CHECK(top_border.line_width_points == doctest::Approx(2.0));
    CHECK(top_border.stroke_color.red == doctest::Approx(192.0 / 255.0));
    CHECK(top_border.stroke_color.green == doctest::Approx(0.0));
    CHECK(top_border.stroke_color.blue == doctest::Approx(0.0));

    const auto &left_border = layout.pages.front().lines[1];
    CHECK(left_border.line_width_points == doctest::Approx(0.5));
    CHECK(left_border.stroke_color.red == doctest::Approx(102.0 / 255.0));
    CHECK(left_border.stroke_color.green == doctest::Approx(102.0 / 255.0));
    CHECK(left_border.stroke_color.blue == doctest::Approx(102.0 / 255.0));
}

TEST_CASE("document PDF adapter maps dashed and dotted table borders") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));
    CHECK(table.set_border(
        featherdoc::table_border_edge::inside_vertical,
        featherdoc::border_definition{featherdoc::border_style::dashed, 8U,
                                      "808080", 0U}));

    auto first_cell = table.find_cell(0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK(first_cell->set_text("Dashed"));

    auto second_cell = table.find_cell(0U, 1U);
    REQUIRE(second_cell.has_value());
    CHECK(second_cell->set_text("Dotted"));
    CHECK(second_cell->set_border(
        featherdoc::cell_border_edge::top,
        featherdoc::border_definition{featherdoc::border_style::dotted, 4U,
                                      "0000FF", 0U}));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    auto dashed_count = 0U;
    auto dotted_count = 0U;
    for (const auto &line : layout.pages.front().lines) {
        if (line.line_width_points == doctest::Approx(1.0) &&
            line.dash_on_points == doctest::Approx(3.0) &&
            line.dash_off_points == doctest::Approx(2.0) &&
            line.line_cap == featherdoc::pdf::PdfLineCap::butt) {
            ++dashed_count;
        }
        if (line.line_width_points == doctest::Approx(0.5) &&
            line.dash_on_points == doctest::Approx(0.5) &&
            line.dash_off_points == doctest::Approx(1.0) &&
            line.line_cap == featherdoc::pdf::PdfLineCap::round) {
            ++dotted_count;
        }
    }

    CHECK_GE(dashed_count, 1U);
    CHECK_EQ(dotted_count, 1U);
}

TEST_CASE("document PDF adapter maps double table borders") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(1440U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_border(
        featherdoc::table_border_edge::top,
        featherdoc::border_definition{featherdoc::border_style::double_line,
                                      12U, "1F4E79", 0U}));
    CHECK(table.set_cell_text(0U, 0U, "Double"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().lines.size(), 2U);

    const auto &outer_line = layout.pages.front().lines[0];
    const auto &inner_line = layout.pages.front().lines[1];
    CHECK(outer_line.line_width_points == doctest::Approx(0.5));
    CHECK(inner_line.line_width_points == doctest::Approx(0.5));
    CHECK(outer_line.start.y_points ==
          doctest::Approx(inner_line.start.y_points + 0.5));
    CHECK(outer_line.end.y_points ==
          doctest::Approx(inner_line.end.y_points + 0.5));
    CHECK(outer_line.stroke_color.red == doctest::Approx(31.0 / 255.0));
    CHECK(outer_line.stroke_color.green == doctest::Approx(78.0 / 255.0));
    CHECK(outer_line.stroke_color.blue == doctest::Approx(121.0 / 255.0));
}

TEST_CASE("document PDF adapter maps table row height and vertical alignment") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.set_height_twips(1200U, featherdoc::row_height_rule::exact));
    CHECK(row.set_cant_split());
    CHECK(row.set_repeats_header());

    auto left_cell = table.find_cell(0U, 0U);
    REQUIRE(left_cell.has_value());
    CHECK(left_cell->set_text("Center"));
    CHECK(left_cell->set_vertical_alignment(
        featherdoc::cell_vertical_alignment::center));

    auto right_cell = table.find_cell(0U, 1U);
    REQUIRE(right_cell.has_value());
    CHECK(right_cell->set_text("Bottom"));
    CHECK(right_cell->set_vertical_alignment(
        featherdoc::cell_vertical_alignment::bottom));

    const auto table_summary = document.inspect_table(0U);
    REQUIRE(table_summary.has_value());
    REQUIRE_EQ(table_summary->row_height_twips.size(), 1U);
    CHECK_EQ(table_summary->row_height_twips[0].value_or(0U), 1200U);
    REQUIRE_EQ(table_summary->row_height_rules.size(), 1U);
    REQUIRE(table_summary->row_height_rules[0].has_value());
    CHECK_EQ(*table_summary->row_height_rules[0],
             featherdoc::row_height_rule::exact);
    REQUIRE_EQ(table_summary->row_cant_split.size(), 1U);
    CHECK(table_summary->row_cant_split[0]);
    REQUIRE_EQ(table_summary->row_repeats_header.size(), 1U);
    CHECK(table_summary->row_repeats_header[0]);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 2U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 2U);

    const auto &left_rectangle = layout.pages.front().rectangles[0];
    CHECK(left_rectangle.bounds.height_points == doctest::Approx(60.0));

    const auto &center_text = layout.pages.front().text_runs[0];
    CHECK_EQ(center_text.text, "Center");
    const auto row_top =
        left_rectangle.bounds.y_points + left_rectangle.bounds.height_points;
    CHECK(center_text.baseline_origin.y_points ==
          doctest::Approx(row_top - 23.0 - options.font_size_points));

    const auto &bottom_text = layout.pages.front().text_runs[1];
    CHECK_EQ(bottom_text.text, "Bottom");
    CHECK(bottom_text.baseline_origin.y_points ==
          doctest::Approx(row_top - 42.0 - options.font_size_points));
}

TEST_CASE("document PDF adapter maps table cell text direction") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-table-text-direction.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    CHECK(row.set_height_twips(1800U, featherdoc::row_height_rule::exact));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    CHECK(cell->set_fill_color("EAF3F8"));
    CHECK(cell->set_text_direction(
        featherdoc::cell_text_direction::top_to_bottom_right_to_left));
    auto paragraph = cell->paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Vert").has_next());
    REQUIRE(paragraph.add_run("ical", featherdoc::formatting_flag::bold)
                .has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

    const auto &cell_rectangle = layout.pages.front().rectangles.front();
    std::vector<const featherdoc::pdf::PdfTextRun *> vertical_text_runs;
    for (const auto &text_run : layout.pages.front().text_runs) {
        if (text_run.text == "Vert" || text_run.text == "ical") {
            vertical_text_runs.push_back(&text_run);
        }
    }

    REQUIRE_EQ(vertical_text_runs.size(), 2U);
    CHECK_EQ(vertical_text_runs[0]->rotation_degrees, doctest::Approx(90.0));
    CHECK_EQ(vertical_text_runs[1]->rotation_degrees, doctest::Approx(90.0));
    CHECK(vertical_text_runs[1]->bold);
    CHECK_EQ(vertical_text_runs[1]->baseline_origin.x_points,
             doctest::Approx(vertical_text_runs[0]->baseline_origin.x_points));
    CHECK_GT(vertical_text_runs[1]->baseline_origin.y_points,
             vertical_text_runs[0]->baseline_origin.y_points + 12.0);
    CHECK_GT(vertical_text_runs[0]->baseline_origin.x_points,
             cell_rectangle.bounds.x_points +
                 cell_rectangle.bounds.width_points - 8.0);
    CHECK_GT(vertical_text_runs[0]->baseline_origin.y_points,
             cell_rectangle.bounds.y_points + 3.0);
    CHECK_LT(vertical_text_runs[0]->baseline_origin.y_points,
             cell_rectangle.bounds.y_points +
                 cell_rectangle.bounds.height_points - 16.0);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Vert"), std::string::npos);
    CHECK_NE(extracted_text.find("ical"), std::string::npos);
#endif
}
