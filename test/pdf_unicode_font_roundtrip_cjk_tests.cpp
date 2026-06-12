#include "pdf_unicode_font_roundtrip_test_support.hpp"

TEST_CASE("PDFio embeds Unicode CJK font and PDFium extracts text") {
    const auto font_path = find_cjk_font();
    if (font_path.empty()) {
        MESSAGE("skipping CJK PDF smoke: no CJK font found; set "
                "FEATHERDOC_TEST_CJK_FONT to run it");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"中文测试 ABC 123，标点。");
    const auto output_path =
        std::filesystem::current_path() / "featherdoc-cjk-font-roundtrip.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc CJK font roundtrip";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "CJK Test Font",
        font_path,
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/Type0"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/CIDFontType2"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/ToUnicode"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/Identity-H"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find(expected_text), std::string::npos);
}

TEST_CASE("PDFio CJK ToUnicode supports PDFium substring search surrogate") {
    const auto font_path = find_cjk_font();
    if (font_path.empty()) {
        MESSAGE("skipping CJK PDF copy/search surrogate: no CJK font found; set "
                "FEATHERDOC_TEST_CJK_FONT to run it");
        return;
    }

    const auto title_line =
        utf8_from_u8(u8"\u4E2D\u6587\u641C\u7D22\u6D4B\u8BD5 Alpha");
    const auto body_line =
        utf8_from_u8(u8"\u5408\u540C\u7F16\u53F7 FD-2026-05\uFF0C\u91D1\u989D 123\u3002");
    const auto title_query =
        utf8_from_u8(u8"\u641C\u7D22\u6D4B\u8BD5");
    const auto body_query =
        utf8_from_u8(u8"\u5408\u540C\u7F16\u53F7");
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-cjk-copy-search-surrogate.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc CJK copy search surrogate";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        title_line,
        "CJK Test Font",
        font_path,
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 692.0},
        body_line,
        "CJK Test Font",
        font_path,
        14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/ToUnicode"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/Identity-H"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 1U);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find(title_line), std::string::npos);
    CHECK_NE(extracted_text.find(body_line), std::string::npos);
    CHECK_NE(extracted_text.find(title_query), std::string::npos);
    CHECK_NE(extracted_text.find(body_query), std::string::npos);
    CHECK_NE(extracted_text.find("FD-2026-05"), std::string::npos);

    const auto &parsed_page = parse_result.document.pages.front();
    REQUIRE_GE(parsed_page.text_lines.size(), 2U);
    CHECK_NE(parsed_page.text_lines[0].text.find(title_query),
             std::string::npos);
    CHECK_NE(parsed_page.text_lines[1].text.find(body_query),
             std::string::npos);
}

TEST_CASE("PDFio subsets embedded Unicode CJK font data") {
    const auto font_path = find_cjk_font();
    if (font_path.empty()) {
        MESSAGE("skipping CJK PDF subset smoke: no CJK font found; set "
                "FEATHERDOC_TEST_CJK_FONT to run it");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"中文合同测试 ABC 123，标点。");

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc CJK font subset";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "CJK Test Font",
        font_path,
        16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
    });
    layout.pages.push_back(std::move(page));

    const auto subset_path =
        std::filesystem::current_path() / "featherdoc-cjk-font-subset.pdf";
    const auto full_path =
        std::filesystem::current_path() / "featherdoc-cjk-font-full.pdf";

    featherdoc::pdf::PdfioGenerator generator;
    featherdoc::pdf::PdfWriterOptions full_options;
    full_options.subset_unicode_fonts = false;
    const auto full_result = generator.write(layout, full_path, full_options);
    REQUIRE_MESSAGE(full_result.success, full_result.error_message);

    featherdoc::pdf::PdfWriterOptions subset_options;
    subset_options.subset_unicode_fonts = true;
    const auto subset_result =
        generator.write(layout, subset_path, subset_options);
    REQUIRE_MESSAGE(subset_result.success, subset_result.error_message);

#if defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
    CHECK_GT(full_result.bytes_written, subset_result.bytes_written);
#endif

    const auto subset_pdf_bytes = read_file_bytes(subset_path);
    CHECK_NE(subset_pdf_bytes.find("/Type0"), std::string::npos);
    CHECK_NE(subset_pdf_bytes.find("/CIDFontType2"), std::string::npos);
    CHECK_NE(subset_pdf_bytes.find("/ToUnicode"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(subset_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find(expected_text), std::string::npos);
}
