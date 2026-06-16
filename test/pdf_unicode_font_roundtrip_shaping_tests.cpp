#include "pdf_unicode_font_roundtrip_test_support.hpp"

TEST_CASE("PDFio writes shaped glyph CID text without duplicate extraction") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph PDF writer smoke: configure test Latin "
                "font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping shaped glyph PDF writer smoke: HarfBuzz unavailable");
        return;
    }

    const std::string expected_text = "office affinity efficient";
    constexpr double font_size = 18.0;
    auto glyph_run = featherdoc::pdf::shape_pdf_text(
        expected_text,
        featherdoc::pdf::PdfTextShaperOptions{font_path, font_size});
    if (!glyph_run.used_harfbuzz || !glyph_run.error_message.empty() ||
        glyph_run.glyphs.empty()) {
        MESSAGE("skipping shaped glyph PDF writer smoke: shaping failed");
        return;
    }

    const auto output_path =
        std::filesystem::current_path() / "featherdoc-shaped-glyph-text.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph text";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "Latin Test Font",
        font_path,
        font_size,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
        0.0,
        false,
        false,
        std::move(glyph_run),
    });
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/CIDToGIDMap"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/ToUnicode"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, expected_text), 1U);
}

TEST_CASE("PDFio subsets shaped glyph CID font data") {
    const auto font_path = find_cjk_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph subset smoke: no CJK font found; set "
                "FEATHERDOC_TEST_CJK_FONT to run it");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping shaped glyph subset smoke: HarfBuzz unavailable");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"涓枃 shaped glyph subset ABC 123");
    constexpr double font_size = 18.0;
    auto glyph_run = featherdoc::pdf::shape_pdf_text(
        expected_text,
        featherdoc::pdf::PdfTextShaperOptions{font_path, font_size});
    if (!glyph_run.used_harfbuzz || !glyph_run.error_message.empty() ||
        glyph_run.glyphs.empty() ||
        glyph_run.direction != featherdoc::pdf::PdfGlyphDirection::left_to_right) {
        MESSAGE("skipping shaped glyph subset smoke: shaping failed or did "
                "not produce an LTR run");
        return;
    }

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph subset";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "CJK Shaped Test Font",
        font_path,
        font_size,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
        0.0,
        false,
        false,
        std::move(glyph_run),
    });
    layout.pages.push_back(std::move(page));

    const auto subset_path =
        std::filesystem::current_path() / "featherdoc-shaped-glyph-subset.pdf";
    const auto full_path =
        std::filesystem::current_path() / "featherdoc-shaped-glyph-full.pdf";

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
    CHECK_NE(subset_pdf_bytes.find("/CIDToGIDMap"), std::string::npos);
    CHECK_NE(subset_pdf_bytes.find("/ToUnicode"), std::string::npos);
    CHECK_NE(subset_pdf_bytes.find("/Identity-H"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(subset_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, expected_text), 1U);
}

TEST_CASE("PDFio positions shaped glyph CIDs when glyph offsets are present") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph positioned writer smoke: configure "
                "test Latin font");
        return;
    }
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping shaped glyph positioned writer smoke: HarfBuzz "
                "unavailable");
        return;
    }

    const std::string expected_text = "offset office";
    constexpr double font_size = 18.0;
    auto glyph_run = featherdoc::pdf::shape_pdf_text(
        expected_text,
        featherdoc::pdf::PdfTextShaperOptions{font_path, font_size});
    if (!glyph_run.used_harfbuzz || !glyph_run.error_message.empty() ||
        glyph_run.glyphs.size() < 2U) {
        MESSAGE("skipping shaped glyph positioned writer smoke: shaping "
                "failed");
        return;
    }

    glyph_run.glyphs[0].x_offset_points += 1.25;
    glyph_run.glyphs[0].y_offset_points += 2.0;
    glyph_run.glyphs[0].y_advance_points += 0.5;

    const auto glyph_count = glyph_run.glyphs.size();
    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-shaped-glyph-positioned.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph positioned";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "Latin Test Font",
        font_path,
        font_size,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
        0.0,
        false,
        false,
        std::move(glyph_run),
    });
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);

    const auto streams = inflated_pdf_streams(pdf_bytes);
    REQUIRE_FALSE(streams.empty());
    const auto content_stream = find_actual_text_content_stream(streams);
    REQUIRE_MESSAGE(!content_stream.empty(),
                    "shaped glyph positioned content stream not found");
    CHECK_GE(count_occurrences(content_stream, " Tm\n"), glyph_count);
    CHECK_EQ(count_occurrences(content_stream, "> Tj\n"), glyph_count);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, expected_text), 1U);
}

TEST_CASE("PDFio maps shaped glyph clusters in ToUnicode CMap") {
    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        MESSAGE("skipping shaped glyph ToUnicode smoke: HarfBuzz unavailable");
        return;
    }

    const std::string expected_text = "office affinity efficient";
    constexpr double font_size = 18.0;
    std::filesystem::path font_path;
    featherdoc::pdf::PdfGlyphRun glyph_run;
    std::vector<ExpectedToUnicodeEntry> expected_entries;
    for (const auto &candidate : candidate_latin_fonts()) {
        if (!std::filesystem::exists(candidate)) {
            continue;
        }

        auto candidate_run = featherdoc::pdf::shape_pdf_text(
            expected_text,
            featherdoc::pdf::PdfTextShaperOptions{candidate, font_size});
        if (!candidate_run.used_harfbuzz ||
            !candidate_run.error_message.empty() ||
            candidate_run.glyphs.empty()) {
            continue;
        }

        auto candidate_entries = expected_to_unicode_entries(candidate_run);
        const bool has_multi_codepoint_cluster =
            std::any_of(candidate_entries.begin(), candidate_entries.end(),
                        [](const ExpectedToUnicodeEntry &entry) {
                            return has_multiple_codepoints(entry.unicode_text);
                        });
        if (has_multi_codepoint_cluster) {
            font_path = candidate;
            glyph_run = std::move(candidate_run);
            expected_entries = std::move(candidate_entries);
            break;
        }
    }

    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph ToUnicode smoke: no Latin font formed "
                "a multi-codepoint cluster");
        return;
    }
    REQUIRE_FALSE(expected_entries.empty());

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-shaped-glyph-tounicode.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph ToUnicode";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "Latin Test Font",
        font_path,
        font_size,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
        0.0,
        false,
        false,
        std::move(glyph_run),
    });
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);

    const auto streams = inflated_pdf_streams(pdf_bytes);
    REQUIRE_FALSE(streams.empty());
    const auto cmap = find_shaped_to_unicode_cmap(streams);
    REQUIRE_MESSAGE(!cmap.empty(), "shaped glyph ToUnicode CMap not found");

    std::size_t checked_entries = 0U;
    bool checked_multi_codepoint_cluster = false;
    for (const auto &entry : expected_entries) {
        const auto expected_cmap_entry =
            "<" + utf16be_hex_unit(entry.cid) + "> <" +
            utf16be_hex_payload_from_utf8(entry.unicode_text) + ">";
        CHECK_NE(cmap.find(expected_cmap_entry), std::string::npos);
        ++checked_entries;
        checked_multi_codepoint_cluster =
            checked_multi_codepoint_cluster ||
            has_multiple_codepoints(entry.unicode_text);
    }

    CHECK_GT(checked_entries, 0U);
    CHECK(checked_multi_codepoint_cluster);
}

TEST_CASE("PDFio maps only the first shaped glyph for repeated clusters") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping repeated shaped cluster smoke: configure test Latin "
                "font");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"a\u0301b");
    constexpr double font_size = 18.0;
    featherdoc::pdf::PdfGlyphRun glyph_run;
    glyph_run.text = expected_text;
    glyph_run.font_file_path = font_path;
    glyph_run.font_size_points = font_size;
    glyph_run.used_harfbuzz = true;
    glyph_run.glyphs = {
        featherdoc::pdf::PdfGlyphPosition{1U, 0U, 6.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{2U, 0U, 0.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{3U, 3U, 6.0, 0.0, 0.0, 0.0},
    };

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-shaped-glyph-repeated-cluster.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc repeated shaped cluster";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "Latin Test Font",
        font_path,
        font_size,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
        0.0,
        false,
        false,
        std::move(glyph_run),
    });
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_file_bytes(output_path);
    CHECK_NE(pdf_bytes.find("/FeatherDocGlyph"), std::string::npos);

    const auto streams = inflated_pdf_streams(pdf_bytes);
    REQUIRE_FALSE(streams.empty());
    const auto cmap = find_shaped_to_unicode_cmap(streams);
    REQUIRE_MESSAGE(!cmap.empty(), "shaped glyph ToUnicode CMap not found");

    CHECK_NE(cmap.find("<0001> <00610301>"), std::string::npos);
    CHECK_EQ(cmap.find("<0002> <"), std::string::npos);
    CHECK_NE(cmap.find("<0003> <0062>"), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_EQ(count_occurrences(extracted_text, expected_text), 1U);
}

TEST_CASE("PDFio falls back for shaped glyph clusters inside UTF-8 codepoints") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph UTF-8 boundary smoke: configure test "
                "Latin font");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"a\u0301b");
    constexpr double font_size = 18.0;
    featherdoc::pdf::PdfGlyphRun glyph_run;
    glyph_run.text = expected_text;
    glyph_run.font_file_path = font_path;
    glyph_run.font_size_points = font_size;
    glyph_run.used_harfbuzz = true;
    glyph_run.glyphs = {
        featherdoc::pdf::PdfGlyphPosition{1U, 0U, 6.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{2U, 2U, 6.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{3U, 3U, 6.0, 0.0, 0.0, 0.0},
    };

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-shaped-glyph-utf8-boundary-fallback.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph UTF-8 boundary";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "Latin Test Font",
        font_path,
        font_size,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
        0.0,
        false,
        false,
        std::move(glyph_run),
    });
    layout.pages.push_back(std::move(page));

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
    CHECK_NE(extracted_text.find(expected_text), std::string::npos);
}

TEST_CASE("PDFio falls back for non-forward shaped glyph clusters") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph fallback smoke: configure test Latin "
                "font");
        return;
    }

    const std::string expected_text = "abcdef";
    constexpr double font_size = 18.0;
    featherdoc::pdf::PdfGlyphRun glyph_run;
    glyph_run.text = expected_text;
    glyph_run.font_file_path = font_path;
    glyph_run.font_size_points = font_size;
    glyph_run.used_harfbuzz = true;
    glyph_run.glyphs = {
        featherdoc::pdf::PdfGlyphPosition{1U, 4U, 6.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{2U, 2U, 6.0, 0.0, 0.0, 0.0},
        featherdoc::pdf::PdfGlyphPosition{3U, 0U, 6.0, 0.0, 0.0, 0.0},
    };

    const auto output_path =
        std::filesystem::current_path() /
        "featherdoc-shaped-glyph-cluster-fallback.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc shaped glyph fallback";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        expected_text,
        "Latin Test Font",
        font_path,
        font_size,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        true,
        0.0,
        false,
        false,
        std::move(glyph_run),
    });
    layout.pages.push_back(std::move(page));

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
    CHECK_NE(extracted_text.find(expected_text), std::string::npos);
}

TEST_CASE("PDFio falls back for non-LTR shaped glyph directions") {
    const auto font_path = find_latin_font();
    if (font_path.empty()) {
        MESSAGE("skipping shaped glyph direction fallback smoke: configure "
                "test Latin font");
        return;
    }

    struct DirectionFallbackCase {
        featherdoc::pdf::PdfGlyphDirection direction{
            featherdoc::pdf::PdfGlyphDirection::unknown};
        std::string label;
    };

    const std::array<DirectionFallbackCase, 4U> fallback_cases{{
        {featherdoc::pdf::PdfGlyphDirection::right_to_left, "rtl"},
        {featherdoc::pdf::PdfGlyphDirection::top_to_bottom, "ttb"},
        {featherdoc::pdf::PdfGlyphDirection::bottom_to_top, "btt"},
        {featherdoc::pdf::PdfGlyphDirection::unknown, "unknown"},
    }};

    const std::string expected_text = "abc";
    constexpr double font_size = 18.0;

    for (const auto &fallback_case : fallback_cases) {
        CAPTURE(fallback_case.label);

        featherdoc::pdf::PdfGlyphRun glyph_run;
        glyph_run.text = expected_text;
        glyph_run.font_file_path = font_path;
        glyph_run.font_size_points = font_size;
        glyph_run.direction = fallback_case.direction;
        glyph_run.used_harfbuzz = true;
        glyph_run.glyphs = {
            featherdoc::pdf::PdfGlyphPosition{1U, 0U, 6.0, 0.0, 0.0, 0.0},
            featherdoc::pdf::PdfGlyphPosition{2U, 1U, 6.0, 0.0, 0.0, 0.0},
            featherdoc::pdf::PdfGlyphPosition{3U, 2U, 6.0, 0.0, 0.0, 0.0},
        };

        const auto output_path =
            std::filesystem::current_path() /
            ("featherdoc-shaped-glyph-direction-fallback-" +
             fallback_case.label + ".pdf");

        featherdoc::pdf::PdfDocumentLayout layout;
        layout.metadata.title = "FeatherDoc shaped glyph direction fallback";
        layout.metadata.creator = "FeatherDoc test";

        featherdoc::pdf::PdfPageLayout page;
        page.size = featherdoc::pdf::PdfPageSize::a4_portrait();
        page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
            featherdoc::pdf::PdfPoint{72.0, 720.0},
            expected_text,
            "Latin Test Font",
            font_path,
            font_size,
            featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
            false,
            false,
            false,
            true,
            0.0,
            false,
            false,
            std::move(glyph_run),
        });
        layout.pages.push_back(std::move(page));

        featherdoc::pdf::PdfioGenerator generator;
        const auto write_result = generator.write(
            layout, output_path, featherdoc::pdf::PdfWriterOptions{});
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
}
