#include "pdf_document_adapter_font_test_support.hpp"

TEST_CASE("document PDF adapter preserves table cell run styling") {
    const auto regular_font =
        make_temp_font_file("featherdoc-adapter-table-regular.ttf");
    const auto bold_font =
        make_temp_font_file("featherdoc-adapter-table-bold.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Table"));

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF Table Cell Accent";
    style_definition.run_text_color = std::string{"C00000"};
    style_definition.run_font_size_points = 16.0;
    REQUIRE(document.ensure_character_style("PdfTableCellAccent",
                                            style_definition));
    REQUIRE(document.materialize_style_run_properties("PdfTableCellAccent"));

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    auto paragraph = cell->paragraphs();
    REQUIRE(paragraph.has_next());
    auto strong_run = paragraph.add_run(
        "Strong ", featherdoc::formatting_flag::bold |
                       featherdoc::formatting_flag::underline);
    REQUIRE(strong_run.has_next());
    auto accent_run = paragraph.add_run("Accent");
    REQUIRE(accent_run.has_next());
    REQUIRE(document.set_run_style(accent_run, "PdfTableCellAccent"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Table", regular_font},
        featherdoc::pdf::PdfFontMapping{"Unit Table", bold_font, true, false},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &cell_rectangle = layout.pages.front().rectangles.front();
    CHECK_GT(cell_rectangle.bounds.height_points, 24.0);

    const auto &strong_text = layout.pages.front().text_runs[0];
    CHECK_EQ(strong_text.text, "Strong ");
    CHECK_EQ(strong_text.font_family, "Unit Table");
    CHECK_EQ(strong_text.font_file_path, bold_font);
    CHECK(strong_text.bold);
    CHECK(strong_text.underline);

    const auto &accent_text = layout.pages.front().text_runs[1];
    CHECK_EQ(accent_text.text, "Accent");
    CHECK_EQ(accent_text.font_family, "Unit Table");
    CHECK_EQ(accent_text.font_file_path, regular_font);
    CHECK_EQ(accent_text.font_size_points, doctest::Approx(16.0));
    CHECK_EQ(accent_text.fill_color.red, doctest::Approx(192.0 / 255.0));
    CHECK_EQ(accent_text.fill_color.green, doctest::Approx(0.0));
    CHECK_EQ(accent_text.fill_color.blue, doctest::Approx(0.0));
    const auto row_top =
        cell_rectangle.bounds.y_points + cell_rectangle.bounds.height_points;
    CHECK(accent_text.baseline_origin.y_points ==
          doctest::Approx(row_top - 4.0 - 16.0));
}

TEST_CASE("document PDF adapter preserves table cell paragraph breaks") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1U, 1U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 2880U));

    auto cell = table.find_cell(0U, 0U);
    REQUIRE(cell.has_value());
    auto first_paragraph = cell->paragraphs();
    REQUIRE(first_paragraph.has_next());
    CHECK(first_paragraph.set_text("First"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("Second");
    REQUIRE(second_paragraph.has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 2U);
    CHECK_EQ(layout.pages.front().text_runs[0].text, "First");
    CHECK_EQ(layout.pages.front().text_runs[1].text, "Second");

    const auto &first_run = layout.pages.front().text_runs[0];
    const auto &second_run = layout.pages.front().text_runs[1];
    CHECK(first_run.baseline_origin.y_points ==
          doctest::Approx(second_run.baseline_origin.y_points + 14.0));
    CHECK_EQ(layout.pages.front().rectangles.front().bounds.height_points,
             doctest::Approx(36.0));
}

TEST_CASE("document PDF adapter maps paragraph alignment and indentation") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-paragraph-layout.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto centered = document.paragraphs();
    REQUIRE(centered.has_next());
    CHECK(centered.set_text("Centered paragraph"));
    CHECK(centered.set_alignment(featherdoc::paragraph_alignment::center));

    auto right = append_body_paragraph(document, "Right paragraph");
    REQUIRE(right.has_next());
    CHECK(right.set_alignment(featherdoc::paragraph_alignment::right));

    auto hanging = append_body_paragraph(
        document,
        "Hanging indentation wraps across multiple lines with a visible "
        "second line offset.");
    REQUIRE(hanging.has_next());
    CHECK(hanging.set_indent_left_twips(720U));
    CHECK(hanging.set_hanging_indent_twips(360U));

    auto first_line = append_body_paragraph(
        document,
        "First line indentation demonstrates a visible inset only on the "
        "first rendered line.");
    REQUIRE(first_line.has_next());
    CHECK(first_line.set_indent_left_twips(360U));
    CHECK(first_line.set_first_line_indent_twips(360U));

    const auto centered_summary = document.inspect_paragraph(0U);
    REQUIRE(centered_summary.has_value());
    CHECK_EQ(centered_summary->alignment,
             std::optional<featherdoc::paragraph_alignment>{
                 featherdoc::paragraph_alignment::center});

    const auto hanging_summary = document.inspect_paragraph(2U);
    REQUIRE(hanging_summary.has_value());
    CHECK_EQ(hanging_summary->indent_left_twips.value_or(0U), 720U);
    CHECK_EQ(hanging_summary->hanging_indent_twips.value_or(0U), 360U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.page_size = featherdoc::pdf::PdfPageSize{240.0, 360.0};
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    const auto &text_runs = layout.pages.front().text_runs;
    REQUIRE_GE(text_runs.size(), 7U);

    const auto content_width = options.page_size.width_points -
                               options.margin_left_points -
                               options.margin_right_points;
    const auto centered_width =
        featherdoc::pdf::measure_text_width_points(
            "Centered paragraph", options.font_size_points, "Helvetica");
    const auto right_width = featherdoc::pdf::measure_text_width_points(
        "Right paragraph", options.font_size_points, "Helvetica");

    CHECK_EQ(text_runs[0].text, "Centered paragraph");
    CHECK(text_runs[0].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points +
                          (content_width - centered_width) / 2.0));
    CHECK_EQ(text_runs[1].text, "Right paragraph");
    CHECK(text_runs[1].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points + content_width -
                          right_width));

    auto hanging_index = std::size_t{0U};
    for (; hanging_index < text_runs.size(); ++hanging_index) {
        if (text_runs[hanging_index].text.find("Hanging") !=
            std::string::npos) {
            break;
        }
    }
    REQUIRE_LT(hanging_index + 1U, text_runs.size());
    CHECK(text_runs[hanging_index].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points + 18.0));
    CHECK(text_runs[hanging_index + 1U].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points + 36.0));

    auto first_line_index = std::size_t{0U};
    for (; first_line_index < text_runs.size(); ++first_line_index) {
        if (text_runs[first_line_index].text.find("First line") !=
            std::string::npos) {
            break;
        }
    }
    REQUIRE_LT(first_line_index + 1U, text_runs.size());
    CHECK(text_runs[first_line_index].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points + 36.0));
    CHECK(text_runs[first_line_index + 1U].baseline_origin.x_points ==
          doctest::Approx(options.margin_left_points + 18.0));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
}

TEST_CASE("document PDF adapter emits bullet list prefixes") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(
        document.set_paragraph_list(paragraph, featherdoc::list_kind::bullet));
    CHECK(paragraph.add_run("Bullet item").has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 1U);
    CHECK_EQ(collect_layout_text(layout),
             utf8_from_u8(u8"\u2022\tBullet item"));
}

TEST_CASE(
    "document PDF adapter falls back bullet prefixes to east Asia fonts") {
    const auto cjk_font = first_existing_path(candidate_cjk_fonts());
    if (cjk_font.empty()) {
        MESSAGE("skipping bullet prefix font fallback test: configure test CJK "
                "fonts");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Helvetica"));
    CHECK(document.set_default_run_east_asia_font_family("Unit CJK"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(
        document.set_paragraph_list(paragraph, featherdoc::list_kind::bullet));
    CHECK(paragraph.add_run("Bullet item").has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit CJK", cjk_font},
    };
    options.cjk_font_file_path = cjk_font;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 1U);

    const auto bullet_prefix = utf8_from_u8(u8"\u2022\t");
    const auto bullet_run = std::find_if(
        layout.pages.front().text_runs.begin(),
        layout.pages.front().text_runs.end(),
        [&](const auto &text_run) {
            return text_run.text.find(bullet_prefix) != std::string::npos;
        });
    REQUIRE(bullet_run != layout.pages.front().text_runs.end());
    CHECK_EQ(bullet_run->font_family, "Unit CJK");
    CHECK_EQ(bullet_run->font_file_path, cjk_font);
    CHECK(bullet_run->unicode);
}

TEST_CASE("document PDF adapter emits decimal list prefixes") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first = document.paragraphs();
    REQUIRE(first.has_next());
    CHECK(document.set_paragraph_list(first, featherdoc::list_kind::decimal));
    CHECK(first.add_run("First").has_next());

    auto second = first.insert_paragraph_after("");
    REQUIRE(second.has_next());
    CHECK(document.set_paragraph_list(second, featherdoc::list_kind::decimal));
    CHECK(second.add_run("Second").has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);
    CHECK_EQ(layout.pages.front().text_runs[0].text, "1.\tFirst");
    CHECK_EQ(layout.pages.front().text_runs[1].text, "2.\tSecond");
}

TEST_CASE("document PDF adapter emits nested decimal list prefixes") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first = document.paragraphs();
    REQUIRE(first.has_next());
    CHECK(document.set_paragraph_list(first, featherdoc::list_kind::decimal));
    CHECK(first.add_run("First").has_next());

    auto first_child = first.insert_paragraph_after("");
    REQUIRE(first_child.has_next());
    CHECK(document.set_paragraph_list(first_child,
                                      featherdoc::list_kind::decimal, 1U));
    CHECK(first_child.add_run("First child").has_next());

    auto second = first_child.insert_paragraph_after("");
    REQUIRE(second.has_next());
    CHECK(document.set_paragraph_list(second, featherdoc::list_kind::decimal));
    CHECK(second.add_run("Second").has_next());

    auto second_child = second.insert_paragraph_after("");
    REQUIRE(second_child.has_next());
    CHECK(document.set_paragraph_list(second_child,
                                      featherdoc::list_kind::decimal, 1U));
    CHECK(second_child.add_run("Second child").has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 4U);
    CHECK_EQ(layout.pages.front().text_runs[0].text, "1.\tFirst");
    CHECK_EQ(layout.pages.front().text_runs[1].text, "1.1.\tFirst child");
    CHECK_EQ(layout.pages.front().text_runs[2].text, "2.\tSecond");
    CHECK_EQ(layout.pages.front().text_runs[3].text, "2.1.\tSecond child");
}

TEST_CASE("document PDF adapter honors custom decimal numbering patterns") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "PdfAdapterLegalOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{featherdoc::list_kind::decimal,
                                               3U, 0U, "Article %1"},
        featherdoc::numbering_level_definition{featherdoc::list_kind::decimal,
                                               1U, 1U, "%1.%2)"},
    };

    const auto numbering_id = document.ensure_numbering_definition(definition);
    REQUIRE(numbering_id.has_value());

    auto first = document.paragraphs();
    REQUIRE(first.has_next());
    CHECK(document.set_paragraph_numbering(first, *numbering_id, 0U));
    CHECK(first.add_run("Scope").has_next());

    auto child = first.insert_paragraph_after("");
    REQUIRE(child.has_next());
    CHECK(document.set_paragraph_numbering(child, *numbering_id, 1U));
    CHECK(child.add_run("Details").has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);
    CHECK_EQ(layout.pages.front().text_runs[0].text, "Article 3\tScope");
    CHECK_EQ(layout.pages.front().text_runs[1].text, "3.1)\tDetails");
}
