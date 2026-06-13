[[nodiscard]] ScenarioResult build_sectioned_report_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Sectioned report sample")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            "Executive summary paragraph one keeps the opening section readable "
            "while PDFium verifies a normal body flow.") ||
        !append_document_text_paragraph(
            document,
            "Executive summary paragraph two adds a second line of realistic "
            "report content and keeps the section header visible.") ||
        !append_document_text_paragraph(
            document,
            "Executive summary paragraph three closes the portrait section with "
            "a stable footer marker.")) {
        return sample;
    }

    auto section0_header = document.ensure_section_header_paragraphs(0U);
    auto section0_footer = document.ensure_section_footer_paragraphs(0U);
    if (!section0_header.has_next() ||
        !section0_header.set_text("Portrait section header") ||
        !section0_footer.has_next() ||
        !section0_footer.set_text("Portrait section footer")) {
        return sample;
    }

    featherdoc::section_page_setup portrait_setup{};
    portrait_setup.orientation = featherdoc::page_orientation::portrait;
    portrait_setup.width_twips = 12240U;
    portrait_setup.height_twips = 15840U;
    portrait_setup.margins.top_twips = 1440U;
    portrait_setup.margins.bottom_twips = 1440U;
    portrait_setup.margins.left_twips = 1440U;
    portrait_setup.margins.right_twips = 1440U;
    portrait_setup.margins.header_twips = 720U;
    portrait_setup.margins.footer_twips = 720U;
    if (!document.set_section_page_setup(0U, portrait_setup)) {
        return sample;
    }

    if (!document.append_section(false)) {
        return sample;
    }

    auto appendix_header = document.ensure_section_header_paragraphs(1U);
    auto appendix_footer = document.ensure_section_footer_paragraphs(1U);
    if (!appendix_header.has_next() ||
        !appendix_header.set_text("Landscape appendix header") ||
        !appendix_footer.has_next() ||
        !appendix_footer.set_text("Landscape appendix footer")) {
        return sample;
    }

    featherdoc::section_page_setup landscape_setup{};
    landscape_setup.orientation = featherdoc::page_orientation::landscape;
    landscape_setup.width_twips = 15840U;
    landscape_setup.height_twips = 12240U;
    landscape_setup.margins.top_twips = 1080U;
    landscape_setup.margins.bottom_twips = 1080U;
    landscape_setup.margins.left_twips = 1440U;
    landscape_setup.margins.right_twips = 1440U;
    landscape_setup.margins.header_twips = 540U;
    landscape_setup.margins.footer_twips = 540U;
    if (!document.set_section_page_setup(1U, landscape_setup)) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            "Appendix paragraph one keeps the second section on a wider page.") ||
        !append_document_text_paragraph(
            document,
            "Appendix paragraph two verifies that the layout still emits the "
            "correct text after the section break.") ||
        !append_document_text_paragraph(
            document,
            "Appendix paragraph three closes the wider section and provides a "
            "stable footer anchor.")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: sectioned report text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_list_report_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("List report sample")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            "The first list uses bullet markers and exercises prefix extraction.")) {
        return sample;
    }

    for (int index = 1; index <= 8; ++index) {
        const auto text = "Bullet item " + std::to_string(index) +
                          ": confirm the PDF text path still sees bullet "
                          "content beside the visible list marker.";
        if (!append_document_list_item(document, text,
                                       featherdoc::list_kind::bullet,
                                       false)) {
            return sample;
        }
    }

    if (!append_document_text_paragraph(
            document,
            "The second list restarts numbering so PDFium sees list restarts in "
            "the extracted text.")) {
        return sample;
    }

    for (int index = 1; index <= 10; ++index) {
        const auto text = "Numbered item " + std::to_string(index) +
                          ": keep the report long enough for a real two-page "
                          "flow and preserve readable numbering.";
        if (!append_document_list_item(document, text,
                                       featherdoc::list_kind::decimal,
                                       index == 1)) {
            return sample;
        }
    }

    if (!append_document_text_paragraph(
            document,
            "Restarted list section demonstrates a fresh sequence after the "
            "numbered block.")) {
        return sample;
    }

    for (int index = 1; index <= 10; ++index) {
        const auto text = "Restarted item " + std::to_string(index) +
                          ": repeat the numbering sequence after a restart so "
                          "the regression keeps that path covered.";
        if (!append_document_list_item(document, text,
                                       featherdoc::list_kind::decimal,
                                       index == 1)) {
            return sample;
        }
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: list report text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.line_height_points = 17.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_bullet_list_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK List")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK bullet list sample")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"\u9879\u76ee\u7b26\u53f7\u5217\u8868\u5f00\u59cb"))) {
        return sample;
    }

    const std::vector<std::string> items{
        utf8_from_u8(u8"BL-101 \u9879\u76ee\u7b26\u53f7\u68c0\u7d22\u952e"),
        utf8_from_u8(u8"BL-102 \u9879\u76ee\u7b26\u53f7\u5b57\u4f53\u56de\u9000"),
        utf8_from_u8(u8"BL-103 East Asia bullet fallback"),
        utf8_from_u8(u8"BL-104 \u4e2d\u82f1\u6df7\u6392 bullet ABC 123"),
    };
    for (const auto &text : items) {
        if (!append_document_list_item(document, text,
                                       featherdoc::list_kind::bullet,
                                       false)) {
            return sample;
        }
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"\u9879\u76ee\u7b26\u53f7\u7ed3\u675f"))) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK bullet list text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK List", font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 20.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_numbered_list_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK List")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK numbered list sample")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"\u7f16\u53f7\u5217\u8868\u5f00\u59cb"))) {
        return sample;
    }

    const std::vector<std::string> items{
        utf8_from_u8(u8"NL-101 \u7f16\u53f7\u68c0\u7d22\u952e"),
        utf8_from_u8(u8"NL-102 \u7f16\u53f7\u5b57\u4f53\u56de\u9000"),
        utf8_from_u8(u8"NL-103 East Asia numbering fallback"),
        utf8_from_u8(u8"NL-104 \u4e2d\u82f1\u6df7\u6392 numbering ABC 123"),
    };
    for (std::size_t index = 0U; index < items.size(); ++index) {
        if (!append_document_list_item(document, items[index],
                                       featherdoc::list_kind::decimal,
                                       index == 0U)) {
            return sample;
        }
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"\u7f16\u53f7\u5217\u8868\u7ed3\u675f"))) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK numbered list text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK List", font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 20.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_cjk_numbered_list_page_flow_lite_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Font Embed Lite") ||
        !define_document_cjk_font_embed_lite_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK numbered list page flow lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Numbered list flow lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"NL-101 \u7f16\u53f7\u91cd\u542f"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"FE-NL-202 \u5d4c\u5957\u5217\u8868"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    const std::vector<std::string> first_block{
        utf8_from_u8(u8"NL-888 \u7f16\u53f7\u68c0\u7d22\u952e"),
        utf8_from_u8(u8"FE-NL-921 \u7b2c 1 \u9879 CJK \u7f16\u53f7\u6d41"),
        utf8_from_u8(u8"NL-A-03 \u7f16\u53f7\u5206\u9875\u8868"),
    };
    for (std::size_t index = 0U; index < first_block.size(); ++index) {
        if (!append_document_list_item(document, first_block[index],
                                       featherdoc::list_kind::decimal,
                                       index == 0U)) {
            return sample;
        }
    }

    auto marker = append_document_paragraph(document, "");
    if (!marker.has_next() ||
        !marker.add_run("List marker lite: ").has_next() ||
        !add_styled_contract_run(
            document, marker,
            utf8_from_u8(u8"NL-888 \u7f16\u53f7\u68c0\u7d22\u952e"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !marker.add_run(" / FE-NL-961 / ").has_next() ||
        !add_styled_contract_run(
            document, marker,
            utf8_from_u8(u8"\u8868\u540e\u7f16\u53f7\u6062\u590d"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    const std::vector<std::string> restart_block{
        utf8_from_u8(u8"NL-501 \u91cd\u542f\u7f16\u53f7"),
        utf8_from_u8(u8"NL-502 \u9879\u76ee\u7b26\u53f7\u5207\u6362"),
        utf8_from_u8(u8"FE-NL-999 \u7ec8\u9875\u590d\u68c0"),
    };
    for (std::size_t index = 0U; index < restart_block.size(); ++index) {
        const auto list_kind =
            index == 1U ? featherdoc::list_kind::bullet
                        : featherdoc::list_kind::decimal;
        if (!append_document_list_item(document, restart_block[index],
                                       list_kind, index != 2U)) {
            return sample;
        }
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("List close lite: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-NL-999 \u7ec8\u9875\u590d\u68c0"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK numbered list page flow "
        "lite text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 19.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_numbered_list_page_flow_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Font Embed Lite") ||
        !define_document_cjk_font_embed_lite_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK numbered list page flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Numbered list flow: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"NL-101 \u7f16\u53f7\u91cd\u542f"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"FE-NL-202 \u5d4c\u5957\u5217\u8868"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto &header = document.ensure_section_header_paragraphs(0U);
    auto &footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("List header NL-303 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text("List footer NL-888 {{page}} / {{total_pages}}")) {
        return sample;
    }

    const std::vector<std::string> list_items{
        utf8_from_u8(u8"NL-888 \u7f16\u53f7\u68c0\u7d22\u952e"),
        utf8_from_u8(u8"FE-NL-921 \u7b2c 1 \u9879 CJK \u7f16\u53f7\u6d41"),
        utf8_from_u8(u8"NL-A-03 \u7f16\u53f7\u5206\u9875\u8868"),
        utf8_from_u8(u8"NL-501 \u91cd\u542f\u7f16\u53f7"),
    };
    for (std::size_t index = 0U; index < list_items.size(); ++index) {
        const auto level = index == 2U ? 1U : 0U;
        if (!append_document_list_item_at_level(
                document, list_items[index], featherdoc::list_kind::decimal,
                index == 0U || index == 3U, level)) {
            return sample;
        }
    }

    auto table = document.append_table(4U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1600U) ||
        !table.set_column_width_twips(1U, 2200U) ||
        !table.set_column_width_twips(2U, 3400U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u9636\u6bb5")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u7f16\u53f7\u952e")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "NL-A-03") ||
        !table.set_cell_text(
            1U, 1U, utf8_from_u8(u8"NL-888 \u7f16\u53f7\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"\u8868\u683c\u63d2\u5165\u540e\u4fdd\u7559 CJK \u7f16\u53f7\u6587\u672c\u5c42")) ||
        !table.set_cell_text(2U, 0U, "FE-NL-961") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u8868\u540e\u7f16\u53f7\u6062\u590d")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"\u91cd\u542f\u540e\u7ee7\u7eed\u68c0\u67e5 decimal \u548c\u5d4c\u5957\u5c42\u7ea7")) ||
        !table.set_cell_text(3U, 0U, "FE-NL-999") ||
        !table.set_cell_text(3U, 1U,
                             utf8_from_u8(u8"\u7ec8\u9875\u590d\u68c0")) ||
        !table.set_cell_text(3U, 2U,
                             utf8_from_u8(u8"\u5217\u8868\u5206\u9875\u5951\u7ea6\u6536\u53e3"))) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 4U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 460U : 620U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        row.next();
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("List close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-NL-999 \u7ec8\u9875\u590d\u68c0"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK numbered list page flow "
        "text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.line_height_points = 17.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_bullet_page_flow_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Font Embed Lite") ||
        !define_document_cjk_font_embed_lite_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK bullet page flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Bullet flow: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"BL-101 \u9879\u76ee\u7b26\u53f7\u68c0\u7d22\u952e"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-BL-202 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"\u591a\u5c42\u7b26\u53f7"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto &header = document.ensure_section_header_paragraphs(0U);
    auto &footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("Bullet header BL-303 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text("Bullet footer BL-777 {{page}} / {{total_pages}}")) {
        return sample;
    }

    const std::array<std::string, 4> bullets{
        utf8_from_u8(u8"BL-777 \u9879\u76ee\u7b26\u53f7\u68c0\u7d22\u952e"),
        utf8_from_u8(u8"FE-BL-921 \u7b2c 1 \u7ec4 CJK \u9879\u76ee\u7b26\u53f7\u6d41"),
        utf8_from_u8(u8"BL-A-03 \u4e09\u7ea7\u65b9\u5757"),
        utf8_from_u8(u8"FE-BL-999 \u9879\u76ee\u7b26\u53f7\u6536\u53e3"),
    };
    for (std::size_t index = 0U; index < bullets.size(); ++index) {
        if (!append_document_list_item_at_level(
                document, bullets[index], featherdoc::list_kind::bullet,
                index == 0U, static_cast<std::uint32_t>(index % 3U))) {
            return sample;
        }
    }

    auto table = document.append_table(3U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1600U) ||
        !table.set_column_width_twips(1U, 2200U) ||
        !table.set_column_width_twips(2U, 3400U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u5c42\u7ea7")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "BL-A-03") ||
        !table.set_cell_text(
            1U, 1U,
            utf8_from_u8(u8"BL-777 \u9879\u76ee\u7b26\u53f7\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"\u8868\u683c\u4e2d\u7ee7\u7eed\u4fdd\u7559 bullet \u548c CJK \u6587\u672c\u5c42")) ||
        !table.set_cell_text(2U, 0U, "FE-BL-999") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u5217\u8868\u6536\u53e3")) ||
        !table.set_cell_text(2U, 2U,
                             utf8_from_u8(u8"\u9879\u76ee\u7b26\u53f7\u5206\u9875\u5951\u7ea6\u7ed3\u675f"))) {
        return sample;
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Bullet close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-BL-999 \u9879\u76ee\u7b26\u53f7\u6536\u53e3"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK bullet page flow text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.line_height_points = 17.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_bullet_overlay_lite_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Font Embed Lite") ||
        !define_document_cjk_style_overlay_lite_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK bullet overlay lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Bullet overlay lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"BO-101 \u9879\u76ee\u7b26\u53f7\u4e0a\u6807"),
            "DocumentPdfCjkOverlayLiteSuperscript") ||
        !intro.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"BO-202 \u9879\u76ee\u7b26\u53f7\u4e0b\u6807"),
            "DocumentPdfCjkOverlayLiteSubscript")) {
        return sample;
    }

    const std::array<std::string, 4> bullets{
        utf8_from_u8(u8"BO-303 \u6837\u5f0f\u53e0\u52a0 bullet \u68c0\u7d22"),
        utf8_from_u8(u8"BO-404 \u4e2d\u82f1\u6df7\u6392 bullet ABC 123"),
        utf8_from_u8(u8"BO-777 \u5171\u7528\u56de\u8bfb\u952e"),
        utf8_from_u8(u8"FE-BO-999 \u8986\u76d6\u6536\u53e3"),
    };
    for (std::size_t index = 0U; index < bullets.size(); ++index) {
        if (!append_document_list_item(document, bullets[index],
                                       featherdoc::list_kind::bullet,
                                       index == 0U)) {
            return sample;
        }
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Bullet overlay close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"BO-777 \u8986\u76d6\u5217\u8868\u56de\u8bfb"),
            "DocumentPdfCjkOverlayLiteStrike")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK bullet overlay lite text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 19.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}
