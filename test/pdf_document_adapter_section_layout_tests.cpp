#include "pdf_document_adapter_font_test_support.hpp"

TEST_CASE("document PDF adapter emits default section header and footer text") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto body = document.paragraphs();
    REQUIRE(body.has_next());
    CHECK(body.set_text("Body text"));

    auto &header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Header text"));

    auto &footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Footer text"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.header_footer_font_size_points = 9.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);

    bool found_body = false;
    bool found_header = false;
    bool found_footer = false;
    double body_y = 0.0;
    double header_y = 0.0;
    double footer_y = 0.0;
    double header_font_size = 0.0;
    double footer_font_size = 0.0;

    for (const auto &text_run : layout.pages.front().text_runs) {
        if (text_run.text == "Body text") {
            found_body = true;
            body_y = text_run.baseline_origin.y_points;
        } else if (text_run.text == "Header text") {
            found_header = true;
            header_y = text_run.baseline_origin.y_points;
            header_font_size = text_run.font_size_points;
        } else if (text_run.text == "Footer text") {
            found_footer = true;
            footer_y = text_run.baseline_origin.y_points;
            footer_font_size = text_run.font_size_points;
        }
    }

    CHECK(found_body);
    CHECK(found_header);
    CHECK(found_footer);
    CHECK(header_y > body_y);
    CHECK(footer_y < body_y);
    CHECK_EQ(header_y, doctest::Approx(options.page_size.height_points -
                                       options.header_y_offset_points));
    CHECK_EQ(footer_y, doctest::Approx(options.footer_y_offset_points));
    CHECK_EQ(header_font_size,
             doctest::Approx(options.header_footer_font_size_points));
    CHECK_EQ(footer_font_size,
             doctest::Approx(options.header_footer_font_size_points));
}

TEST_CASE("document PDF adapter expands header and footer page placeholders") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto body = document.paragraphs();
    REQUIRE(body.has_next());
    CHECK(body.set_text("Placeholder body line 0"));
    for (auto index = 1U; index < 18U; ++index) {
        auto paragraph = append_body_paragraph(
            document, "Placeholder body line " + std::to_string(index));
        REQUIRE(paragraph.has_next());
    }

    auto &header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Long header {{section_page}}/"
                          "{{section_total_pages}}"));

    auto &footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Page {{page}} of {{total_pages}}"));

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 4320U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    REQUIRE(document.set_section_page_setup(0U, setup));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;
    options.header_footer_font_size_points = 8.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    REQUIRE_GE(layout.pages.size(), 2U);

    const auto total_pages = std::to_string(layout.pages.size());
    const auto first_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages.front()}});
    const auto last_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages.back()}});
    const auto all_text = collect_layout_text(layout);

    CHECK_NE(first_page_text.find("Long header 1/" + total_pages),
             std::string::npos);
    CHECK_NE(first_page_text.find("Page 1 of " + total_pages),
             std::string::npos);
    CHECK_NE(last_page_text.find("Long header " + total_pages + "/" +
                                 total_pages),
             std::string::npos);
    CHECK_NE(last_page_text.find("Page " + total_pages + " of " +
                                 total_pages),
             std::string::npos);
    CHECK_EQ(all_text.find("{{page}}"), std::string::npos);
    CHECK_EQ(all_text.find("{{total_pages}}"), std::string::npos);
}

TEST_CASE("document PDF adapter applies first section page setup") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-section-page-setup.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto body = document.paragraphs();
    REQUIRE(body.has_next());
    CHECK(body.set_text("Section page setup body"));

    auto &header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Section setup header"));

    auto &footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Section setup footer"));

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 15840U;
    setup.height_twips = 12240U;
    setup.margins.top_twips = 1440U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1800U;
    setup.margins.right_twips = 900U;
    setup.margins.header_twips = 360U;
    setup.margins.footer_twips = 420U;
    REQUIRE(document.set_section_page_setup(0U, setup));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    const auto &page = layout.pages.front();
    CHECK_EQ(page.size.width_points, doctest::Approx(792.0));
    CHECK_EQ(page.size.height_points, doctest::Approx(612.0));

    bool found_body = false;
    bool found_header = false;
    bool found_footer = false;
    double body_x = 0.0;
    double body_y = 0.0;
    double header_y = 0.0;
    double footer_y = 0.0;
    for (const auto &text_run : page.text_runs) {
        if (text_run.text == "Section page setup body") {
            found_body = true;
            body_x = text_run.baseline_origin.x_points;
            body_y = text_run.baseline_origin.y_points;
        } else if (text_run.text == "Section setup header") {
            found_header = true;
            header_y = text_run.baseline_origin.y_points;
        } else if (text_run.text == "Section setup footer") {
            found_footer = true;
            footer_y = text_run.baseline_origin.y_points;
        }
    }

    CHECK(found_body);
    CHECK(found_header);
    CHECK(found_footer);
    CHECK_EQ(body_x, doctest::Approx(90.0));
    CHECK_EQ(body_y, doctest::Approx(528.0));
    CHECK_EQ(header_y, doctest::Approx(594.0));
    CHECK_EQ(footer_y, doctest::Approx(21.0));

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);
    CHECK_EQ(parse_result.document.pages.front().size.width_points,
             doctest::Approx(792.0));
    CHECK_EQ(parse_result.document.pages.front().size.height_points,
             doctest::Approx(612.0));

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Section setup header"), std::string::npos);
    CHECK_NE(extracted_text.find("Section page setup body"), std::string::npos);
    CHECK_NE(extracted_text.find("Section setup footer"), std::string::npos);
#endif
}

TEST_CASE("document PDF adapter switches page setup at section boundaries") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-multi-section-page-setup.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first = document.paragraphs();
    REQUIRE(first.has_next());
    CHECK(first.set_text("Section one body"));

    auto &section0_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(section0_header.has_next());
    CHECK(section0_header.set_text("Section one header"));

    CHECK(document.append_section(false));
    REQUIRE_EQ(document.section_count(), 2U);

    auto second = append_body_paragraph(document, "Section two body");
    REQUIRE(second.has_next());

    auto &section1_header = document.ensure_section_header_paragraphs(1U);
    REQUIRE(section1_header.has_next());
    CHECK(section1_header.set_text("Section two header"));

    featherdoc::section_page_setup first_setup{};
    first_setup.orientation = featherdoc::page_orientation::portrait;
    first_setup.width_twips = 12240U;
    first_setup.height_twips = 15840U;
    first_setup.margins.top_twips = 1440U;
    first_setup.margins.bottom_twips = 1440U;
    first_setup.margins.left_twips = 1440U;
    first_setup.margins.right_twips = 1440U;
    first_setup.margins.header_twips = 360U;
    first_setup.margins.footer_twips = 360U;

    featherdoc::section_page_setup second_setup{};
    second_setup.orientation = featherdoc::page_orientation::landscape;
    second_setup.width_twips = 15840U;
    second_setup.height_twips = 12240U;
    second_setup.margins.top_twips = 720U;
    second_setup.margins.bottom_twips = 720U;
    second_setup.margins.left_twips = 1800U;
    second_setup.margins.right_twips = 900U;
    second_setup.margins.header_twips = 300U;
    second_setup.margins.footer_twips = 300U;
    REQUIRE(document.set_section_page_setup(0U, first_setup));
    REQUIRE(document.set_section_page_setup(1U, second_setup));

    const auto body_blocks = document.inspect_body_blocks();
    REQUIRE_GE(body_blocks.size(), 3U);
    CHECK_EQ(body_blocks.front().section_index, 0U);
    CHECK_EQ(body_blocks.back().section_index, 1U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_GE(layout.pages.size(), 2U);
    CHECK_EQ(layout.pages[0].section_index, 0U);
    CHECK_EQ(layout.pages[0].size.width_points, doctest::Approx(612.0));
    CHECK_EQ(layout.pages[0].size.height_points, doctest::Approx(792.0));
    CHECK_EQ(layout.pages[1].section_index, 1U);
    CHECK_EQ(layout.pages[1].size.width_points, doctest::Approx(792.0));
    CHECK_EQ(layout.pages[1].size.height_points, doctest::Approx(612.0));

    const auto first_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[0]}});
    const auto second_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[1]}});
    CHECK_NE(first_page_text.find("Section one body"), std::string::npos);
    CHECK_NE(first_page_text.find("Section one header"), std::string::npos);
    CHECK_EQ(first_page_text.find("Section two body"), std::string::npos);
    CHECK_NE(second_page_text.find("Section two body"), std::string::npos);
    CHECK_NE(second_page_text.find("Section two header"), std::string::npos);
    CHECK_EQ(second_page_text.find("Section one header"), std::string::npos);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_GE(parse_result.document.pages.size(), 2U);
    CHECK_EQ(parse_result.document.pages[0].size.width_points,
             doctest::Approx(612.0));
    CHECK_EQ(parse_result.document.pages[0].size.height_points,
             doctest::Approx(792.0));
    CHECK_EQ(parse_result.document.pages[1].size.width_points,
             doctest::Approx(792.0));
    CHECK_EQ(parse_result.document.pages[1].size.height_points,
             doctest::Approx(612.0));

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Section one body"), std::string::npos);
    CHECK_NE(extracted_text.find("Section two body"), std::string::npos);
#endif
}

TEST_CASE("document PDF adapter renders first page section header and footer") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-first-page-header-footer.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first_body = document.paragraphs();
    REQUIRE(first_body.has_next());
    CHECK(first_body.set_text("First body line 0"));
    for (auto index = 1U; index < 18U; ++index) {
        auto paragraph = append_body_paragraph(
            document, "First body line " + std::to_string(index));
        REQUIRE(paragraph.has_next());
    }

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.has_next());
    CHECK(default_header.set_text("Default section header"));

    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(default_footer.has_next());
    CHECK(default_footer.set_text("Default section footer"));

    auto &first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_header.has_next());
    CHECK(first_header.set_text("First page section header"));

    auto &first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.set_text("First page section footer"));

    const auto section = document.inspect_section(0U);
    REQUIRE(section.has_value());
    CHECK(section->different_first_page_enabled);

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 4320U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    REQUIRE(document.set_section_page_setup(0U, setup));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;
    options.header_footer_font_size_points = 8.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_GE(layout.pages.size(), 2U);
    CHECK_EQ(layout.pages[0].section_index, 0U);
    CHECK_EQ(layout.pages[0].section_page_index, 0U);
    CHECK_EQ(layout.pages[1].section_index, 0U);
    CHECK_EQ(layout.pages[1].section_page_index, 1U);

    const auto first_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[0]}});
    const auto second_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[1]}});
    CHECK_NE(first_page_text.find("First page section header"),
             std::string::npos);
    CHECK_NE(first_page_text.find("First page section footer"),
             std::string::npos);
    CHECK_EQ(first_page_text.find("Default section header"), std::string::npos);
    CHECK_EQ(first_page_text.find("Default section footer"), std::string::npos);
    CHECK_NE(second_page_text.find("Default section header"),
             std::string::npos);
    CHECK_NE(second_page_text.find("Default section footer"),
             std::string::npos);
    CHECK_EQ(second_page_text.find("First page section header"),
             std::string::npos);
    CHECK_EQ(second_page_text.find("First page section footer"),
             std::string::npos);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_GE(parse_result.document.pages.size(), 2U);

    auto parsed_first_page_text = std::string{};
    for (const auto &text_span : parse_result.document.pages[0].text_spans) {
        parsed_first_page_text += text_span.text;
    }
    auto parsed_second_page_text = std::string{};
    for (const auto &text_span : parse_result.document.pages[1].text_spans) {
        parsed_second_page_text += text_span.text;
    }

    CHECK_NE(parsed_first_page_text.find("First page section header"),
             std::string::npos);
    CHECK_EQ(parsed_first_page_text.find("Default section header"),
             std::string::npos);
    CHECK_NE(parsed_second_page_text.find("Default section header"),
             std::string::npos);
    CHECK_EQ(parsed_second_page_text.find("First page section header"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter renders even page section header and footer") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-even-page-header-footer.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first_body = document.paragraphs();
    REQUIRE(first_body.has_next());
    CHECK(first_body.set_text("Even body line 0"));
    for (auto index = 1U; index < 18U; ++index) {
        auto paragraph = append_body_paragraph(
            document, "Even body line " + std::to_string(index));
        REQUIRE(paragraph.has_next());
    }

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.has_next());
    CHECK(default_header.set_text("Odd default section header"));

    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(default_footer.has_next());
    CHECK(default_footer.set_text("Odd default section footer"));

    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.set_text("Even page section header"));

    auto &even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_footer.has_next());
    CHECK(even_footer.set_text("Even page section footer"));

    const auto section = document.inspect_section(0U);
    REQUIRE(section.has_value());
    REQUIRE(section->even_and_odd_headers_enabled.has_value());
    CHECK(*section->even_and_odd_headers_enabled);

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 4320U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    REQUIRE(document.set_section_page_setup(0U, setup));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;
    options.header_footer_font_size_points = 8.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_GE(layout.pages.size(), 3U);
    CHECK_EQ(layout.pages[0].section_page_index, 0U);
    CHECK_EQ(layout.pages[1].section_page_index, 1U);
    CHECK_EQ(layout.pages[2].section_page_index, 2U);

    const auto first_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[0]}});
    const auto second_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[1]}});
    const auto third_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[2]}});
    CHECK_NE(first_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_NE(first_page_text.find("Odd default section footer"),
             std::string::npos);
    CHECK_EQ(first_page_text.find("Even page section header"),
             std::string::npos);
    CHECK_NE(second_page_text.find("Even page section header"),
             std::string::npos);
    CHECK_NE(second_page_text.find("Even page section footer"),
             std::string::npos);
    CHECK_EQ(second_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_NE(third_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_NE(third_page_text.find("Odd default section footer"),
             std::string::npos);
    CHECK_EQ(third_page_text.find("Even page section header"),
             std::string::npos);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_GE(parse_result.document.pages.size(), 3U);

    const auto parsed_first_page_text =
        collect_text(parse_result.document.pages[0]);
    const auto parsed_second_page_text =
        collect_text(parse_result.document.pages[1]);
    const auto parsed_third_page_text =
        collect_text(parse_result.document.pages[2]);

    CHECK_NE(parsed_first_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_EQ(parsed_first_page_text.find("Even page section header"),
             std::string::npos);
    CHECK_NE(parsed_second_page_text.find("Even page section header"),
             std::string::npos);
    CHECK_EQ(parsed_second_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_NE(parsed_third_page_text.find("Odd default section header"),
             std::string::npos);
    CHECK_EQ(parsed_third_page_text.find("Even page section header"),
             std::string::npos);
#endif
}
