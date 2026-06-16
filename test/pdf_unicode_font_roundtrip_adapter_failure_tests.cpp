#include "pdf_unicode_font_roundtrip_test_support.hpp"

TEST_CASE("PDFio falls back for document adapter RTL shaped runs") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping document RTL fallback smoke: configure test Latin "
                "font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping document RTL fallback smoke: HarfBuzz unavailable");
        return;
    }

    const std::string expected_text = "abc";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto rtl_run = paragraph.add_run(expected_text);
    REQUIRE(rtl_run.has_next());
    CHECK(rtl_run.set_font_family("Latin RTL Fallback"));
    CHECK(rtl_run.set_rtl());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Latin RTL Fallback", font_path},
    };
    options.use_system_font_fallbacks = false;
    options.font_size_points = 18.0;

    auto layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, expected_text);
    CHECK_EQ(text_run.font_file_path, font_path);
    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(text_run.glyph_run.script_tag, "Latn");

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-document-rtl-glyph-fallback.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_EQ(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, expected_text), 1U);
}

TEST_CASE("PDFio reports a missing Unicode font path") {
    const auto missing_font_path =
        std::filesystem::current_path() / "featherdoc-missing-cjk-font.ttf";
    REQUIRE_FALSE(std::filesystem::exists(missing_font_path));

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc missing CJK font diagnostic";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        utf8_from_u8(u8"中文测试 ABC 123"),
        "Missing CJK Test Font",
        missing_font_path,
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-missing-cjk-font-diagnostic.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});

    CHECK_FALSE(write_result.success);
    CHECK_NE(write_result.error_message.find("Unable to create PDF font"),
             std::string::npos);
    CHECK_NE(write_result.error_message.find(missing_font_path.string()),
             std::string::npos);
}

TEST_CASE("PDFio reports an unresolved Unicode font") {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc unresolved Unicode font diagnostic";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        utf8_from_u8(u8"中文测试 ABC 123"),
        "Unresolved CJK Test Font",
        {},
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-unresolved-cjk-font-diagnostic.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});

    CHECK_FALSE(write_result.success);
    CHECK_NE(write_result.error_message.find(
                 "Unicode PDF text requires an embedded font file"),
             std::string::npos);
    CHECK_NE(write_result.error_message.find("Unresolved CJK Test Font"),
             std::string::npos);
}

TEST_CASE("PDFio reports a corrupt Unicode font file") {
    const auto corrupt_font_path =
        std::filesystem::current_path() / "featherdoc-corrupt-cjk-font.ttf";
    {
        std::ofstream output(corrupt_font_path, std::ios::binary);
        output << "not a font";
    }

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc corrupt CJK font diagnostic";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        utf8_from_u8(u8"中文测试 ABC 123"),
        "Corrupt CJK Test Font",
        corrupt_font_path,
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-corrupt-cjk-font-diagnostic.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});

    CHECK_FALSE(write_result.success);
    CHECK_NE(write_result.error_message.find("Unable to create PDF font"),
             std::string::npos);
    CHECK_NE(write_result.error_message.find(corrupt_font_path.string()),
             std::string::npos);
}
