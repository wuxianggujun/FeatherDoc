[[nodiscard]] ScenarioResult build_document_eastasia_style_probe_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document EastAsia Probe CJK") ||
        !define_document_eastasia_style_probe_styles(document)) {
        return sample;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("Document eastAsia style probe")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Styled Latin run: ").has_next()) {
        return sample;
    }
    auto latin_run = paragraph.add_run("bold italic blue 16pt");
    if (!latin_run.has_next() ||
        !document.set_run_style(latin_run, "DocumentPdfEastAsiaProbeLatin")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("East Asia run mapping: ").has_next()) {
        return sample;
    }
    auto accent_run = paragraph.add_run(utf8_from_u8(u8"中文样式映射 ABC 123"));
    if (!accent_run.has_next() ||
        !document.set_run_style(accent_run,
                                "DocumentPdfEastAsiaProbeAccent")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("East Asia note style: ").has_next()) {
        return sample;
    }
    auto note_run = paragraph.add_run(utf8_from_u8(u8"蓝色 14pt 中文备注"));
    if (!note_run.has_next() ||
        !document.set_run_style(note_run, "DocumentPdfEastAsiaProbeNote")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("East Asia symbol probe: ").has_next()) {
        return sample;
    }
    auto symbol_run = paragraph.add_run(utf8_from_u8(u8"※ ㊟ ㊣"));
    if (!symbol_run.has_next() ||
        !document.set_run_style(symbol_run,
                                "DocumentPdfEastAsiaProbeAccent")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document eastAsia style probe";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document EastAsia Probe CJK",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 24.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_rtl_bidi_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !define_document_rtl_bidi_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document RTL bidi sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto paragraph = title.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !document.set_paragraph_style(paragraph, "DocumentPdfRtlParagraph") ||
        !paragraph.set_alignment(featherdoc::paragraph_alignment::right) ||
        !paragraph.add_run("Arabic paragraph: ").has_next()) {
        return sample;
    }
    auto arabic_run = paragraph.add_run(
        utf8_from_u8(u8"\u0645\u0631\u062d\u0628\u0627 \u0628\u0643\u0645 \u0641\u064a FeatherDoc 2026"));
    if (!arabic_run.has_next() ||
        !document.set_run_style(arabic_run, "DocumentPdfRtlArabic")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !document.set_paragraph_style(paragraph, "DocumentPdfRtlParagraph") ||
        !paragraph.set_alignment(featherdoc::paragraph_alignment::right) ||
        !paragraph.add_run("Mixed order: ").has_next()) {
        return sample;
    }
    auto mixed_prefix =
        paragraph.add_run(utf8_from_u8(u8"\u0627\u0644\u0646\u0633\u062e\u0629"));
    if (!mixed_prefix.has_next() ||
        !document.set_run_style(mixed_prefix,
                                "DocumentPdfRtlArabicEmphasis") ||
        !paragraph.add_run(" FE-2048 ").has_next()) {
        return sample;
    }
    auto mixed_suffix = paragraph.add_run(
        utf8_from_u8(u8"\u062c\u0627\u0647\u0632\u0629 \u0644\u0644\u0645\u0631\u0627\u062c\u0639\u0629"));
    if (!mixed_suffix.has_next() ||
        !document.set_run_style(mixed_suffix, "DocumentPdfRtlArabic")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{520.0, 680.0};
    options.metadata.title = "FeatherDoc regression sample: document RTL bidi";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document RTL Arabic", font_path},
    };
    options.use_system_font_fallbacks = false;
    options.line_height_points = 22.0;
    options.paragraph_spacing_after_points = 8.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_style_gallery_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !define_document_style_gallery_styles(document)) {
        return sample;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("Document style gallery sample") ||
        !paragraph.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Style coverage: ").has_next()) {
        return sample;
    }
    auto accent_run = paragraph.add_run("bold italic blue 16pt");
    auto separator_run = paragraph.add_run(", ");
    auto note_run = paragraph.add_run("underlined note");
    if (!accent_run.has_next() || !separator_run.has_next() ||
        !note_run.has_next() ||
        !document.set_run_style(accent_run, "DocumentPdfStyleGalleryAccent") ||
        !document.set_run_style(note_run, "DocumentPdfStyleGalleryNote") ||
        !paragraph
             .add_run(", and neutral body text keeps mixed inline styling "
                      "stable.")
             .has_next()) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Right-aligned summary marker").has_next() ||
        !paragraph.set_alignment(featherdoc::paragraph_alignment::right)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after(
        "Hanging indent paragraph wraps across multiple lines so the export "
        "path keeps a narrower first-line offset and a deeper follow-on body "
        "column.");
    if (!paragraph.has_next() || !paragraph.set_indent_left_twips(720U) ||
        !paragraph.set_hanging_indent_twips(360U)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after(
        "First line indent paragraph keeps the opening clause inset while the "
        "remaining lines align back to the body column for visual comparison.");
    if (!paragraph.has_next() || !paragraph.set_indent_left_twips(360U) ||
        !paragraph.set_first_line_indent_twips(360U)) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 420.0};
    options.metadata.title =
        "FeatherDoc regression sample: document style gallery";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 40.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 10.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_font_matrix_text_sample(
    const std::filesystem::path &cjk_font_path,
    const std::filesystem::path &arabic_font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Font Matrix CJK") ||
        !define_document_rtl_bidi_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document font matrix sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto paragraph = title.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Body matrix: ").has_next() ||
        !paragraph
             .add_run(utf8_from_u8(u8"\u4e2d\u6587"))
             .has_next() ||
        !paragraph.add_run(" / ").has_next()) {
        return sample;
    }
    auto body_arabic = paragraph.add_run(
        utf8_from_u8(u8"\u0645\u0631\u062d\u0628\u0627 \u0628\u0627\u0644\u0645\u0631\u0627\u062c\u0639\u064a\u0646"));
    if (!body_arabic.has_next() ||
        !document.set_run_style(body_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Identifier matrix: FE-300 / ").has_next() ||
        !paragraph
             .add_run(utf8_from_u8(u8"\u7f16\u53f7"))
             .has_next() ||
        !paragraph.add_run(" / ").has_next()) {
        return sample;
    }
    auto identifier_arabic =
        paragraph.add_run(utf8_from_u8(u8"\u0627\u0644\u0646\u0633\u062e\u0629"));
    if (!identifier_arabic.has_next() ||
        !document.set_run_style(identifier_arabic,
                                "DocumentPdfRtlArabicEmphasis")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    if (!header.has_next() ||
        !document.set_paragraph_style(header, "DocumentPdfRtlParagraph") ||
        !header.set_bidi(true) ||
        !header.set_alignment(featherdoc::paragraph_alignment::right) ||
        !header.add_run("Header matrix ").has_next() ||
        !header
             .add_run(utf8_from_u8(u8"\u9875\u7709"))
             .has_next() ||
        !header.add_run(" ").has_next()) {
        return sample;
    }
    auto header_arabic =
        header.add_run(utf8_from_u8(u8"\u0645\u0635\u0641\u0648\u0641\u0629"));
    if (!header_arabic.has_next() ||
        !document.set_run_style(header_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!footer.has_next() ||
        !document.set_paragraph_style(footer, "DocumentPdfRtlParagraph") ||
        !footer.set_bidi(true) ||
        !footer.set_alignment(featherdoc::paragraph_alignment::right) ||
        !footer.add_run("Footer matrix ").has_next() ||
        !footer.add_run("{{page}}").has_next() ||
        !footer.add_run(" / ").has_next() ||
        !footer.add_run("{{total_pages}}").has_next() ||
        !footer.add_run(" ").has_next() ||
        !footer
             .add_run(utf8_from_u8(u8"\u9875\u811a"))
             .has_next() ||
        !footer.add_run(" ").has_next()) {
        return sample;
    }
    auto footer_arabic =
        footer.add_run(utf8_from_u8(u8"\u0645\u0631\u0627\u062c\u0639\u0629"));
    if (!footer_arabic.has_next() ||
        !document.set_run_style(footer_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto table = document.append_table(3U, 2U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 5400U) ||
        !table.set_cell_text(0U, 0U, "Zone") ||
        !table.set_cell_text(0U, 1U, "Value") ||
        !table.set_cell_text(1U, 0U, "Body") ||
        !table.set_cell_text(2U, 0U, "Cell")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 3U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(420U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        row.next();
    }

    auto body_cell = table.find_cell(1U, 1U);
    auto mixed_cell = table.find_cell(2U, 1U);
    if (!body_cell.has_value() || !mixed_cell.has_value()) {
        return sample;
    }

    auto body_cell_paragraph = body_cell->paragraphs();
    if (!body_cell_paragraph.has_next() ||
        !body_cell_paragraph
             .add_run("Header/footer preview ")
             .has_next() ||
        !body_cell_paragraph
             .add_run(utf8_from_u8(u8"\u9875\u9762"))
             .has_next() ||
        !body_cell_paragraph.add_run(" ").has_next()) {
        return sample;
    }
    auto body_cell_arabic =
        body_cell_paragraph.add_run(utf8_from_u8(u8"\u062c\u0627\u0647\u0632"));
    if (!body_cell_arabic.has_next() ||
        !document.set_run_style(body_cell_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto mixed_cell_paragraph = mixed_cell->paragraphs();
    if (!mixed_cell_paragraph.has_next() ||
        !document.set_paragraph_style(mixed_cell_paragraph,
                                      "DocumentPdfRtlParagraph") ||
        !mixed_cell_paragraph.set_bidi(true) ||
        !mixed_cell_paragraph.set_alignment(
            featherdoc::paragraph_alignment::right) ||
        !mixed_cell_paragraph.add_run("Cell ").has_next() ||
        !mixed_cell_paragraph
             .add_run(utf8_from_u8(u8"\u5355\u5143"))
             .has_next() ||
        !mixed_cell_paragraph.add_run(" ").has_next()) {
        return sample;
    }
    auto mixed_cell_arabic =
        mixed_cell_paragraph.add_run(utf8_from_u8(u8"\u062e\u0644\u064a\u0629"));
    if (!mixed_cell_arabic.has_next() ||
        !document.set_run_style(mixed_cell_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document font matrix";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Font Matrix CJK",
                                        cjk_font_path},
        featherdoc::pdf::PdfFontMapping{"Document RTL Arabic",
                                        arabic_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_font_matrix_text_sample(
    const std::filesystem::path &cjk_font_path,
    const std::filesystem::path &arabic_font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Font Matrix CJK") ||
        !define_document_rtl_bidi_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document table font matrix sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Table matrix: ").has_next() ||
        !intro
             .add_run(utf8_from_u8(u8"\u4e2d\u6587\u8868\u683c"))
             .has_next() ||
        !intro.add_run(" / FE-420 / ").has_next()) {
        return sample;
    }
    auto intro_arabic =
        intro.add_run(utf8_from_u8(u8"\u0645\u0631\u0627\u062c\u0639\u0629"));
    if (!intro_arabic.has_next() ||
        !document.set_run_style(intro_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    if (!header.has_next() ||
        !document.set_paragraph_style(header, "DocumentPdfRtlParagraph") ||
        !header.set_bidi(true) ||
        !header.set_alignment(featherdoc::paragraph_alignment::right) ||
        !header.add_run("Table matrix header ").has_next() ||
        !header.add_run("{{page}}").has_next() ||
        !header.add_run(" ").has_next() ||
        !header
             .add_run(utf8_from_u8(u8"\u9875\u7709"))
             .has_next() ||
        !header.add_run(" ").has_next()) {
        return sample;
    }
    auto header_arabic =
        header.add_run(utf8_from_u8(u8"\u0627\u0644\u062c\u062f\u0648\u0644"));
    if (!header_arabic.has_next() ||
        !document.set_run_style(header_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!footer.has_next() ||
        !document.set_paragraph_style(footer, "DocumentPdfRtlParagraph") ||
        !footer.set_bidi(true) ||
        !footer.set_alignment(featherdoc::paragraph_alignment::right) ||
        !footer.add_run("Table matrix footer ").has_next() ||
        !footer.add_run("{{page}}").has_next() ||
        !footer.add_run(" / ").has_next() ||
        !footer.add_run("{{total_pages}}").has_next() ||
        !footer.add_run(" ").has_next() ||
        !footer
             .add_run(utf8_from_u8(u8"\u9875\u811a"))
             .has_next() ||
        !footer.add_run(" ").has_next()) {
        return sample;
    }
    auto footer_arabic =
        footer.add_run(utf8_from_u8(u8"\u062a\u062f\u0642\u064a\u0642"));
    if (!footer_arabic.has_next() ||
        !document.set_run_style(footer_arabic, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto table = document.append_table(4U, 2U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 5400U) ||
        !table.set_cell_text(0U, 0U, "Zone") ||
        !table.set_cell_text(
            0U, 1U, utf8_from_u8(u8"Value \u77e9\u9635")) ||
        !table.set_cell_text(1U, 0U, "FE-421") ||
        !table.set_cell_text(2U, 0U, "FE-422") ||
        !table.set_cell_text(3U, 0U, "FE-423")) {
        return sample;
    }

    auto configure_matrix_cell =
        [&](std::size_t row_index, std::string_view latin,
            const char8_t *cjk, const char8_t *arabic,
            bool bidi) -> bool {
        auto cell = table.find_cell(row_index, 1U);
        if (!cell.has_value()) {
            return false;
        }
        auto paragraph = cell->paragraphs();
        if (!paragraph.has_next()) {
            return false;
        }
        if (bidi &&
            (!document.set_paragraph_style(paragraph,
                                           "DocumentPdfRtlParagraph") ||
             !paragraph.set_bidi(true) ||
             !paragraph.set_alignment(
                 featherdoc::paragraph_alignment::right))) {
            return false;
        }
        if (!paragraph.add_run(std::string(latin)).has_next() ||
            !paragraph.add_run(" ").has_next() ||
            !paragraph.add_run(utf8_from_u8(cjk)).has_next() ||
            !paragraph.add_run(" ").has_next()) {
            return false;
        }
        auto arabic_run = paragraph.add_run(utf8_from_u8(arabic));
        if (!arabic_run.has_next()) {
            return false;
        }
        return document.set_run_style(
            arabic_run,
            bidi ? "DocumentPdfRtlArabicEmphasis" :
                   "DocumentPdfRtlArabic");
    };

    if (!configure_matrix_cell(1U, "Body cell", u8"\u6b63\u6587",
                               u8"\u0646\u0633\u062e\u0629", false) ||
        !configure_matrix_cell(2U, "Bidi cell", u8"\u5355\u5143",
                               u8"\u062e\u0644\u064a\u0629", true) ||
        !configure_matrix_cell(3U, "Search cell", u8"\u68c0\u7d22",
                               u8"\u0628\u062d\u062b", false)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 4U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 420U : 560U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 2U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document table font matrix";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Font Matrix CJK",
                                        cjk_font_path},
        featherdoc::pdf::PdfFontMapping{"Document RTL Arabic",
                                        arabic_font_path},
    };
    options.cjk_font_file_path = cjk_font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}
