#include "pdf_document_adapter_font_test_support.hpp"

TEST_CASE("document PDF adapter resolves linked section headers and footers") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-linked-section-header-footer."
                             "pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first_body = document.paragraphs();
    REQUIRE(first_body.has_next());
    CHECK(first_body.set_text("Linked section one body"));

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.has_next());
    CHECK(default_header.set_text("Linked inherited default header"));

    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(default_footer.has_next());
    CHECK(default_footer.set_text("Linked inherited default footer"));

    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.set_text("Linked inherited even header"));

    auto &even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_footer.has_next());
    CHECK(even_footer.set_text("Linked inherited even footer"));

    CHECK(document.append_section(false));
    REQUIRE_EQ(document.section_count(), 2U);

    for (auto index = 0U; index < 18U; ++index) {
        auto paragraph = append_body_paragraph(
            document, "Linked section two body line " + std::to_string(index));
        REQUIRE(paragraph.has_next());
    }

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
    REQUIRE(document.set_section_page_setup(1U, setup));

    const auto section = document.inspect_section(1U);
    REQUIRE(section.has_value());
    CHECK_FALSE(section->header.has_default);
    CHECK_FALSE(section->header.has_even);
    CHECK(section->header.default_linked_to_previous);
    CHECK(section->header.even_linked_to_previous);
    REQUIRE(section->header.resolved_default_section_index.has_value());
    CHECK_EQ(*section->header.resolved_default_section_index, 0U);
    REQUIRE(section->header.resolved_even_section_index.has_value());
    CHECK_EQ(*section->header.resolved_even_section_index, 0U);
    CHECK(section->footer.default_linked_to_previous);
    CHECK(section->footer.even_linked_to_previous);
    REQUIRE(section->footer.resolved_default_section_index.has_value());
    CHECK_EQ(*section->footer.resolved_default_section_index, 0U);
    REQUIRE(section->footer.resolved_even_section_index.has_value());
    CHECK_EQ(*section->footer.resolved_even_section_index, 0U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;
    options.header_footer_font_size_points = 8.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_GE(layout.pages.size(), 3U);
    CHECK_EQ(layout.pages[0].section_index, 0U);
    CHECK_EQ(layout.pages[1].section_index, 1U);
    CHECK_EQ(layout.pages[2].section_index, 1U);

    const auto first_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[0]}});
    const auto second_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[1]}});
    const auto third_page_text = collect_layout_text(
        featherdoc::pdf::PdfDocumentLayout{{}, {layout.pages[2]}});
    CHECK_NE(first_page_text.find("Linked inherited default header"),
             std::string::npos);
    CHECK_NE(second_page_text.find("Linked inherited even header"),
             std::string::npos);
    CHECK_NE(second_page_text.find("Linked inherited even footer"),
             std::string::npos);
    CHECK_EQ(second_page_text.find("Linked inherited default header"),
             std::string::npos);
    CHECK_NE(third_page_text.find("Linked inherited default header"),
             std::string::npos);
    CHECK_NE(third_page_text.find("Linked inherited default footer"),
             std::string::npos);
    CHECK_EQ(third_page_text.find("Linked inherited even header"),
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

    const auto parsed_second_page_text =
        collect_text(parse_result.document.pages[1]);
    const auto parsed_third_page_text =
        collect_text(parse_result.document.pages[2]);

    CHECK_NE(parsed_second_page_text.find("Linked inherited even header"),
             std::string::npos);
    CHECK_EQ(parsed_second_page_text.find("Linked inherited default header"),
             std::string::npos);
    CHECK_NE(parsed_third_page_text.find("Linked inherited default header"),
             std::string::npos);
    CHECK_EQ(parsed_third_page_text.find("Linked inherited even header"),
             std::string::npos);
#endif
}

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
TEST_CASE("document PDF adapter header and footer output round-trips through "
          "PDFium") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-header-footer-roundtrip.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto body = document.paragraphs();
    REQUIRE(body.has_next());
    CHECK(body.set_text("Roundtrip body text"));

    auto &header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Roundtrip header text"));

    auto &footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Roundtrip footer text"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Roundtrip header text"), std::string::npos);
    CHECK_NE(extracted_text.find("Roundtrip body text"), std::string::npos);
    CHECK_NE(extracted_text.find("Roundtrip footer text"), std::string::npos);
}
#endif

TEST_CASE("PDF writer accepts standard font style variants and underlines") {
    const auto output_path =
        std::filesystem::current_path() / "featherdoc-styled-base-fonts.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc underlined text writer test";
    layout.metadata.creator = "FeatherDoc tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize{};
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        "Underlined PDF text",
        "Helvetica",
        {},
        14.0,
        featherdoc::pdf::PdfRgbColor{0.1, 0.2, 0.3},
        false,
        false,
        true,
        false,
    });
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 690.0},
        "Bold italic PDF text",
        "Helvetica",
        {},
        14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        true,
        true,
        false,
        false,
    });
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (!latin_font.empty()) {
        page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
            featherdoc::pdf::PdfPoint{72.0, 660.0},
            "Synthetic file font style",
            "Unit File Font",
            latin_font,
            14.0,
            featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
            true,
            true,
            false,
            false,
            0.0,
            true,
            true,
        });
    }
    layout.pages.push_back(std::move(page));

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
    CHECK_NE(extracted_text.find("Underlined PDF text"), std::string::npos);
    CHECK_NE(extracted_text.find("Bold italic PDF text"), std::string::npos);
    if (!latin_font.empty()) {
        CHECK_NE(extracted_text.find("Synthetic file font style"),
                 std::string::npos);
    }
#endif
}
