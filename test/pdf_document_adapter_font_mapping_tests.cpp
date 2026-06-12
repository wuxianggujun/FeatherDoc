#include "pdf_document_adapter_font_test_support.hpp"

TEST_CASE("document PDF adapter resolves run-level CJK font mappings") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    const auto cjk_font = first_existing_path(candidate_cjk_fonts());
    if (latin_font.empty() || cjk_font.empty()) {
        MESSAGE("skipping adapter font mapping test: configure test Latin/CJK "
                "fonts");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Latin"));
    CHECK(document.set_default_run_east_asia_font_family("Unit CJK"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Contract ABC ").has_next());
    REQUIRE(paragraph.add_run(utf8_from_u8(u8"中文测试")).has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit CJK", cjk_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &latin_run = layout.pages.front().text_runs[0];
    CHECK_EQ(latin_run.text, "Contract ABC ");
    CHECK_EQ(latin_run.font_family, "Unit Latin");
    CHECK_EQ(latin_run.font_file_path, latin_font);
    CHECK_FALSE(latin_run.unicode);

    const auto &cjk_run = layout.pages.front().text_runs[1];
    CHECK_EQ(cjk_run.text, utf8_from_u8(u8"中文测试"));
    CHECK_EQ(cjk_run.font_family, "Unit CJK");
    CHECK_EQ(cjk_run.font_file_path, cjk_font);
    CHECK(cjk_run.unicode);
    CHECK_GT(cjk_run.baseline_origin.x_points,
             latin_run.baseline_origin.x_points + 24.0);
    CHECK_EQ(cjk_run.baseline_origin.y_points,
             doctest::Approx(latin_run.baseline_origin.y_points));
}

TEST_CASE("document PDF adapter carries shaped glyph run for file-backed text") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (latin_font.empty()) {
        MESSAGE("skipping adapter glyph run test: configure test Latin font");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto first_run = paragraph.add_run("office");
    REQUIRE(first_run.has_next());
    CHECK(first_run.set_font_family("Unit Latin A"));
    auto second_run = paragraph.add_run(" after");
    REQUIRE(second_run.has_next());
    CHECK(second_run.set_font_family("Unit Latin B"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin A", latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit Latin B", latin_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 2U);

    const auto &text_run = layout.pages.front().text_runs[0];
    CHECK_EQ(text_run.text, "office");
    CHECK_EQ(text_run.font_file_path, latin_font);
    const auto &after_run = layout.pages.front().text_runs[1];
    CHECK_EQ(after_run.text, " after");
    CHECK_EQ(after_run.font_file_path, latin_font);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(text_run.glyph_run.used_harfbuzz);
        CHECK(text_run.glyph_run.glyphs.empty());
        return;
    }

    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.text, "office");
    CHECK_EQ(text_run.glyph_run.font_file_path, latin_font);
    CHECK(text_run.glyph_run.font_size_points == doctest::Approx(12.0));
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::left_to_right);
    CHECK_EQ(text_run.glyph_run.script_tag, "Latn");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());

    double total_advance = 0.0;
    for (const auto &glyph : text_run.glyph_run.glyphs) {
        total_advance += glyph.x_advance_points;
        CHECK_LT(glyph.cluster,
                 std::char_traits<char>::length("office"));
    }
    CHECK_GT(total_advance, 1.0);
    CHECK(after_run.baseline_origin.x_points ==
          doctest::Approx(text_run.baseline_origin.x_points +
                          featherdoc::pdf::glyph_run_x_advance_points(
                              text_run.glyph_run)));
}

TEST_CASE("document PDF adapter carries RTL shaped direction metadata") {
    const auto rtl_font = first_existing_path(candidate_rtl_fonts());
    if (rtl_font.empty()) {
        MESSAGE("skipping adapter RTL glyph direction test: configure test "
                "RTL font");
        return;
    }

    const auto expected_text = utf8_from_u8(u8"\u05E9\u05DC\u05D5\u05DD");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto rtl_run = paragraph.add_run(expected_text);
    REQUIRE(rtl_run.has_next());
    CHECK(rtl_run.set_font_family("Unit RTL"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit RTL", rtl_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, expected_text);
    CHECK_EQ(text_run.font_file_path, rtl_font);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(text_run.glyph_run.used_harfbuzz);
        CHECK(text_run.glyph_run.glyphs.empty());
        return;
    }

    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.text, expected_text);
    CHECK_EQ(text_run.glyph_run.font_file_path, rtl_font);
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(text_run.glyph_run.script_tag, "Hebr");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());
}

TEST_CASE("document PDF adapter maps run RTL formatting into shaping options") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (latin_font.empty()) {
        MESSAGE("skipping adapter run RTL shaping test: configure test Latin "
                "font");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto rtl_run = paragraph.add_run("office");
    REQUIRE(rtl_run.has_next());
    CHECK(rtl_run.set_font_family("Unit Latin RTL"));
    CHECK(rtl_run.set_rtl());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin RTL", latin_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, "office");
    CHECK_EQ(text_run.font_file_path, latin_font);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(text_run.glyph_run.used_harfbuzz);
        CHECK(text_run.glyph_run.glyphs.empty());
        return;
    }

    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(text_run.glyph_run.script_tag, "Latn");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());
}

TEST_CASE("document PDF adapter maps run language into shaping options") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (latin_font.empty()) {
        MESSAGE("skipping adapter run language shaping test: configure test "
                "Latin font");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run("office");
    REQUIRE(run.has_next());
    CHECK(run.set_font_family("Unit Latin Lang"));
    CHECK(run.set_language("fr"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin Lang", latin_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, "office");
    CHECK_EQ(text_run.font_file_path, latin_font);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(text_run.glyph_run.used_harfbuzz);
        CHECK(text_run.glyph_run.glyphs.empty());
        return;
    }

    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::left_to_right);
    CHECK_EQ(text_run.glyph_run.script_tag, "Latn");
    CHECK_EQ(text_run.glyph_run.language_tag, "fr");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());
}

TEST_CASE(
    "document PDF adapter prefers bidi language for RTL shaping options") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (latin_font.empty()) {
        MESSAGE("skipping adapter RTL language shaping test: configure test "
                "Latin font");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run("office");
    REQUIRE(run.has_next());
    CHECK(run.set_font_family("Unit Latin Bidi Lang"));
    CHECK(run.set_language("en"));
    CHECK(run.set_bidi_language("he"));
    CHECK(run.set_rtl());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin Bidi Lang", latin_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, "office");
    CHECK_EQ(text_run.font_file_path, latin_font);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(text_run.glyph_run.used_harfbuzz);
        CHECK(text_run.glyph_run.glyphs.empty());
        return;
    }

    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::right_to_left);
    CHECK_EQ(text_run.glyph_run.script_tag, "Latn");
    CHECK_EQ(text_run.glyph_run.language_tag, "he");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());
}

TEST_CASE(
    "document PDF adapter maps default run language into shaping options") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (latin_font.empty()) {
        MESSAGE("skipping adapter default language shaping test: configure "
                "test Latin font");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Default Lang"));
    CHECK(document.set_default_run_language("de"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("office").has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Default Lang", latin_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, "office");
    CHECK_EQ(text_run.font_file_path, latin_font);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(text_run.glyph_run.used_harfbuzz);
        CHECK(text_run.glyph_run.glyphs.empty());
        return;
    }

    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::left_to_right);
    CHECK_EQ(text_run.glyph_run.script_tag, "Latn");
    CHECK_EQ(text_run.glyph_run.language_tag, "de");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());
}

TEST_CASE(
    "document PDF adapter maps inherited style language into shaping options") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    if (latin_font.empty()) {
        MESSAGE("skipping adapter style language shaping test: configure test "
                "Latin font");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF Language Character";
    style_definition.run_font_family = std::string{"Unit Styled Lang"};
    style_definition.run_language = std::string{"it"};
    REQUIRE(document.ensure_character_style("PdfLanguageCharacter",
                                            style_definition));
    REQUIRE(document.materialize_style_run_properties("PdfLanguageCharacter"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run("office");
    REQUIRE(run.has_next());
    REQUIRE(document.set_run_style(run, "PdfLanguageCharacter"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Styled Lang", latin_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &text_run = layout.pages.front().text_runs.front();
    CHECK_EQ(text_run.text, "office");
    CHECK_EQ(text_run.font_family, "Unit Styled Lang");
    CHECK_EQ(text_run.font_file_path, latin_font);

    if (!featherdoc::pdf::pdf_text_shaper_has_harfbuzz()) {
        CHECK_FALSE(text_run.glyph_run.used_harfbuzz);
        CHECK(text_run.glyph_run.glyphs.empty());
        return;
    }

    CHECK(text_run.glyph_run.used_harfbuzz);
    CHECK(text_run.glyph_run.error_message.empty());
    CHECK_EQ(text_run.glyph_run.direction,
             featherdoc::pdf::PdfGlyphDirection::left_to_right);
    CHECK_EQ(text_run.glyph_run.script_tag, "Latn");
    CHECK_EQ(text_run.glyph_run.language_tag, "it");
    REQUIRE_FALSE(text_run.glyph_run.glyphs.empty());
}

TEST_CASE(
    "document PDF adapter prefers east Asia font mapping for mixed text") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    const auto cjk_font = first_existing_path(candidate_cjk_fonts());
    if (latin_font.empty() || cjk_font.empty()) {
        MESSAGE("skipping adapter east Asia preference test: configure test "
                "Latin/CJK fonts");
        return;
    }

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Latin"));
    CHECK(document.set_default_run_east_asia_font_family("Unit CJK"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Mixed font ").has_next());
    REQUIRE(paragraph.add_run(utf8_from_u8(u8"中文 Mixed")).has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit CJK", cjk_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &latin_run = layout.pages.front().text_runs[0];
    CHECK_EQ(latin_run.text, "Mixed font ");
    CHECK_EQ(latin_run.font_family, "Unit Latin");
    CHECK_EQ(latin_run.font_file_path, latin_font);
    CHECK_FALSE(latin_run.unicode);

    const auto &cjk_run = layout.pages.front().text_runs[1];
    CHECK_EQ(cjk_run.text, utf8_from_u8(u8"中文 Mixed"));
    CHECK_EQ(cjk_run.font_family, "Unit CJK");
    CHECK_EQ(cjk_run.font_file_path, cjk_font);
    CHECK(cjk_run.unicode);
}

TEST_CASE("document PDF adapter falls back to configured CJK font file path") {
    const auto latin_font =
        make_temp_font_file("featherdoc-adapter-latin-fallback.ttf");
    const auto cjk_font =
        make_temp_font_file("featherdoc-adapter-cjk-fallback.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Latin"));
    CHECK(document.set_default_run_east_asia_font_family("Missing CJK"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Contract ").has_next());
    REQUIRE(paragraph.add_run(utf8_from_u8(u8"中文")).has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font},
    };
    options.cjk_font_file_path = cjk_font;
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &latin_run = layout.pages.front().text_runs[0];
    CHECK_EQ(latin_run.text, "Contract ");
    CHECK_EQ(latin_run.font_family, "Unit Latin");
    CHECK_EQ(latin_run.font_file_path, latin_font);
    CHECK_FALSE(latin_run.unicode);

    const auto &cjk_run = layout.pages.front().text_runs[1];
    CHECK_EQ(cjk_run.text, utf8_from_u8(u8"中文"));
    CHECK_EQ(cjk_run.font_family, "Missing CJK");
    CHECK_EQ(cjk_run.font_file_path, cjk_font);
    CHECK(cjk_run.unicode);
}

TEST_CASE("document PDF adapter maps bold italic underline strikethrough run styling") {
    const auto regular_font =
        make_temp_font_file("featherdoc-adapter-regular.ttf");
    const auto bold_italic_font =
        make_temp_font_file("featherdoc-adapter-bold-italic.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Style"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(
        paragraph
            .add_run("Styled PDF", featherdoc::formatting_flag::bold |
                                       featherdoc::formatting_flag::italic |
                                       featherdoc::formatting_flag::strikethrough |
                                       featherdoc::formatting_flag::underline)
            .has_next());
    REQUIRE(paragraph.add_run(" Super",
                              featherdoc::formatting_flag::superscript)
                .has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Style", regular_font},
        featherdoc::pdf::PdfFontMapping{"Unit Style", bold_italic_font, true,
                                        true},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &styled_run = layout.pages.front().text_runs.front();
    CHECK_EQ(styled_run.text, "Styled PDF");
    CHECK_EQ(styled_run.font_family, "Unit Style");
    CHECK_EQ(styled_run.font_file_path, bold_italic_font);
    CHECK(styled_run.bold);
    CHECK(styled_run.italic);
    CHECK(styled_run.strikethrough);
    CHECK(styled_run.underline);
    CHECK_FALSE(styled_run.unicode);
    CHECK_FALSE(styled_run.synthetic_bold);
    CHECK_FALSE(styled_run.synthetic_italic);

    const auto &superscript_run = layout.pages.front().text_runs[1];
    CHECK_EQ(superscript_run.text, " Super");
    CHECK_EQ(superscript_run.font_size_points, doctest::Approx(7.8));
    CHECK_EQ(superscript_run.vertical_shift_points, doctest::Approx(4.2));
    CHECK_GT(superscript_run.baseline_origin.y_points,
             styled_run.baseline_origin.y_points);
}

TEST_CASE("document PDF adapter marks synthetic styles for missing file font "
          "variants") {
    const auto regular_font =
        make_temp_font_file("featherdoc-adapter-synthetic-regular.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Synthetic"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Synthetic styled PDF",
                              featherdoc::formatting_flag::bold |
                                  featherdoc::formatting_flag::italic)
                .has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Synthetic", regular_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 1U);

    const auto &styled_run = layout.pages.front().text_runs.front();
    CHECK_EQ(styled_run.text, "Synthetic styled PDF");
    CHECK_EQ(styled_run.font_family, "Unit Synthetic");
    CHECK_EQ(styled_run.font_file_path, regular_font);
    CHECK(styled_run.bold);
    CHECK(styled_run.italic);
    CHECK(styled_run.synthetic_bold);
    CHECK(styled_run.synthetic_italic);
}

TEST_CASE("document PDF adapter resolves inherited run style formatting") {
    const auto regular_font =
        make_temp_font_file("featherdoc-adapter-style-regular.ttf");
    const auto bold_italic_font =
        make_temp_font_file("featherdoc-adapter-style-bold-italic.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF Styled Character";
    style_definition.run_text_color = std::string{"336699"};
    style_definition.run_bold = true;
    style_definition.run_italic = true;
    style_definition.run_strikethrough = true;
    style_definition.run_underline = true;
    style_definition.run_subscript = true;
    style_definition.run_font_size_points = 15.5;
    style_definition.run_font_family = std::string{"Unit Styled"};
    REQUIRE(document.ensure_character_style("PdfStyledCharacter",
                                            style_definition));
    REQUIRE(document.materialize_style_run_properties("PdfStyledCharacter"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run("Inherited PDF");
    REQUIRE(run.has_next());
    REQUIRE(document.set_run_style(run, "PdfStyledCharacter"));

    const auto resolved =
        document.resolve_style_properties("PdfStyledCharacter");
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->run_font_family.value.has_value());
    CHECK_EQ(*resolved->run_font_family.value, "Unit Styled");
    REQUIRE(resolved->run_text_color.value.has_value());
    CHECK_EQ(*resolved->run_text_color.value, "336699");
    CHECK_EQ(resolved->run_bold.value.value_or(false), true);
    CHECK_EQ(resolved->run_italic.value.value_or(false), true);
    CHECK_EQ(resolved->run_strikethrough.value.value_or(false), true);
    CHECK_EQ(resolved->run_underline.value.value_or(false), true);
    CHECK_EQ(resolved->run_subscript.value.value_or(false), true);
    REQUIRE(resolved->run_font_size_points.value.has_value());
    CHECK_EQ(*resolved->run_font_size_points.value, doctest::Approx(15.5));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Styled", regular_font},
        featherdoc::pdf::PdfFontMapping{"Unit Styled", bold_italic_font, true,
                                        true},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &styled_run = layout.pages.front().text_runs.front();
    CHECK_EQ(styled_run.text, "Inherited PDF");
    CHECK_EQ(styled_run.font_family, "Unit Styled");
    CHECK_EQ(styled_run.font_file_path, bold_italic_font);
    CHECK_EQ(styled_run.font_size_points, doctest::Approx(10.075));
    CHECK_EQ(styled_run.fill_color.red, doctest::Approx(0x33 / 255.0));
    CHECK_EQ(styled_run.fill_color.green, doctest::Approx(0x66 / 255.0));
    CHECK_EQ(styled_run.fill_color.blue, doctest::Approx(0x99 / 255.0));
    CHECK(styled_run.bold);
    CHECK(styled_run.italic);
    CHECK(styled_run.strikethrough);
    CHECK(styled_run.underline);
    CHECK_EQ(styled_run.vertical_shift_points, doctest::Approx(-3.1));
}

TEST_CASE(
    "document PDF adapter resolves inherited east Asia run style formatting") {
    const auto latin_font =
        make_temp_font_file("featherdoc-adapter-style-eastasia-latin.ttf");
    const auto cjk_font =
        make_temp_font_file("featherdoc-adapter-style-eastasia-cjk.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF East Asia Character";
    style_definition.run_font_family = std::string{"Unit EastAsia Latin"};
    style_definition.run_east_asia_font_family =
        std::string{"Unit EastAsia CJK"};
    style_definition.run_font_size_points = 14.0;
    REQUIRE(document.ensure_character_style("PdfEastAsiaCharacter",
                                            style_definition));
    REQUIRE(document.materialize_style_run_properties("PdfEastAsiaCharacter"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(utf8_from_u8(u8"中文样式"));
    REQUIRE(run.has_next());
    REQUIRE(document.set_run_style(run, "PdfEastAsiaCharacter"));

    const auto resolved =
        document.resolve_style_properties("PdfEastAsiaCharacter");
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->run_font_family.value.has_value());
    CHECK_EQ(*resolved->run_font_family.value, "Unit EastAsia Latin");
    REQUIRE(resolved->run_east_asia_font_family.value.has_value());
    CHECK_EQ(*resolved->run_east_asia_font_family.value, "Unit EastAsia CJK");
    REQUIRE(resolved->run_font_size_points.value.has_value());
    CHECK_EQ(*resolved->run_font_size_points.value, doctest::Approx(14.0));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit EastAsia Latin", latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit EastAsia CJK", cjk_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &styled_run = layout.pages.front().text_runs.front();
    CHECK_EQ(styled_run.text, utf8_from_u8(u8"中文样式"));
    CHECK_EQ(styled_run.font_family, "Unit EastAsia CJK");
    CHECK_EQ(styled_run.font_file_path, cjk_font);
    CHECK_EQ(styled_run.font_size_points, doctest::Approx(14.0));
    CHECK(styled_run.unicode);
}

TEST_CASE("document PDF adapter resolves inherited east Asia run styling") {
    const auto latin_font = make_temp_font_file(
        "featherdoc-adapter-style-eastasia-combo-latin.ttf");
    const auto cjk_font =
        make_temp_font_file("featherdoc-adapter-style-eastasia-combo-cjk.ttf");
    const auto cjk_bold_italic_font = make_temp_font_file(
        "featherdoc-adapter-style-eastasia-combo-cjk-bold-italic.ttf");

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF East Asia Styled Character";
    style_definition.run_font_family = std::string{"Unit EastAsia Combo Latin"};
    style_definition.run_east_asia_font_family =
        std::string{"Unit EastAsia Combo CJK"};
    style_definition.run_text_color = std::string{"336699"};
    style_definition.run_bold = true;
    style_definition.run_italic = true;
    style_definition.run_font_size_points = 18.0;
    REQUIRE(document.ensure_character_style("PdfEastAsiaStyledCharacter",
                                            style_definition));
    REQUIRE(document.materialize_style_run_properties(
        "PdfEastAsiaStyledCharacter"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(utf8_from_u8(u8"中文样式"));
    REQUIRE(run.has_next());
    REQUIRE(document.set_run_style(run, "PdfEastAsiaStyledCharacter"));

    const auto resolved =
        document.resolve_style_properties("PdfEastAsiaStyledCharacter");
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->run_font_family.value.has_value());
    CHECK_EQ(*resolved->run_font_family.value, "Unit EastAsia Combo Latin");
    REQUIRE(resolved->run_east_asia_font_family.value.has_value());
    CHECK_EQ(*resolved->run_east_asia_font_family.value,
             "Unit EastAsia Combo CJK");
    REQUIRE(resolved->run_text_color.value.has_value());
    CHECK_EQ(*resolved->run_text_color.value, "336699");
    REQUIRE(resolved->run_font_size_points.value.has_value());
    CHECK_EQ(*resolved->run_font_size_points.value, doctest::Approx(18.0));
    CHECK_EQ(resolved->run_bold.value.value_or(false), true);
    CHECK_EQ(resolved->run_italic.value.value_or(false), true);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit EastAsia Combo Latin",
                                        latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit EastAsia Combo CJK", cjk_font},
        featherdoc::pdf::PdfFontMapping{"Unit EastAsia Combo CJK",
                                        cjk_bold_italic_font, true, true},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 1U);

    const auto &styled_run = layout.pages.front().text_runs.front();
    CHECK_EQ(styled_run.text, utf8_from_u8(u8"中文样式"));
    CHECK_EQ(styled_run.font_family, "Unit EastAsia Combo CJK");
    CHECK_EQ(styled_run.font_file_path, cjk_bold_italic_font);
    CHECK_EQ(styled_run.font_size_points, doctest::Approx(18.0));
    CHECK_EQ(styled_run.fill_color.red, doctest::Approx(0x33 / 255.0));
    CHECK_EQ(styled_run.fill_color.green, doctest::Approx(0x66 / 255.0));
    CHECK_EQ(styled_run.fill_color.blue, doctest::Approx(0x99 / 255.0));
    CHECK(styled_run.bold);
    CHECK(styled_run.italic);
    CHECK_FALSE(styled_run.underline);
    CHECK(styled_run.unicode);
}
