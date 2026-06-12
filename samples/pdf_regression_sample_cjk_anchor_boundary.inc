[[nodiscard]] ScenarioResult
build_document_cjk_style_overlay_page_flow_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    const auto overlay_key = utf8_from_u8(u8"SO-888 \u6837\u5f0f\u68c0\u7d22\u952e");

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
        !title.set_text("Document CJK style overlay page flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Style overlay flow: ").has_next() ||
        !add_styled_contract_run(
            document, intro, utf8_from_u8(u8"\u6df7\u6392\u53e0\u52a0"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-SO-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"\u4e0a\u4e0b\u6807\u590d\u5236"),
            "DocumentPdfCjkFontEmbedLiteNote") ||
        !intro.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"\u4e0a\u6807SO1"),
                                 "DocumentPdfCjkOverlayLiteSuperscript") ||
        !intro.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, intro, utf8_from_u8(u8"\u4e0b\u6807SO2"),
                                 "DocumentPdfCjkOverlayLiteSubscript")) {
        return sample;
    }

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    auto &first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto &first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto &even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text(utf8_from_u8(
            u8"Overlay header SO-303 \u6837\u5f0f\u68c0\u7d22\u952e page {{page}}")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(utf8_from_u8(
            u8"Overlay footer SO-888 \u6837\u5f0f\u68c0\u7d22\u952e {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(utf8_from_u8(
            u8"Overlay first header SO-101 \u6837\u5f0f\u68c0\u7d22\u952e page {{page}}")) ||
        !first_footer.has_next() ||
        !first_footer.set_text(utf8_from_u8(
            u8"Overlay first footer SO-888 \u6837\u5f0f\u68c0\u7d22\u952e {{page}} / {{total_pages}}")) ||
        !even_header.has_next() ||
        !even_header.set_text(utf8_from_u8(
            u8"Overlay even header SO-202 \u6837\u5f0f\u68c0\u7d22\u952e page {{page}}")) ||
        !even_footer.has_next() ||
        !even_footer.set_text(utf8_from_u8(
            u8"Overlay even footer SO-888 \u6837\u5f0f\u68c0\u7d22\u952e {{page}} / {{total_pages}}"))) {
        return sample;
    }

    auto matrix = append_document_paragraph(document, "");
    if (!matrix.has_next() ||
        !matrix.add_run("Overlay matrix: ").has_next() ||
        !add_styled_contract_run(document, matrix, overlay_key,
                                 "DocumentPdfCjkFontEmbedLiteAccent") ||
        !matrix.add_run(" / FE-SO-921 / ").has_next() ||
        !add_styled_contract_run(document, matrix, utf8_from_u8(u8"\u4e0a\u6807A1"),
                                 "DocumentPdfCjkOverlayLiteSuperscript") ||
        !matrix.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, matrix, utf8_from_u8(u8"\u4e0b\u6807B2"),
                                 "DocumentPdfCjkOverlayLiteSubscript") ||
        !matrix.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, matrix, utf8_from_u8(u8"\u5220\u9664\u7ebf\u4ecd\u53ef\u641c"),
            "DocumentPdfCjkOverlayLiteStrike")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"\u7b2c 1 \u7ec4\u6837\u5f0f\u53e0\u52a0\u5206\u9875\u9a8c\u8bc1\uff1aSO-A-03 / token FE-SO-921 / \u540c\u4e00\u884c\u5185\u7a33\u5b9a\u5904\u7406 CJK\u3001Latin\u3001\u6570\u5b57\u3001\u4e0a\u4e0b\u6807\u548c\u5220\u9664\u7ebf\u3002")) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"\u6b63\u6587\u7ee7\u7eed\u4fdd\u7559 SO-888 \u6837\u5f0f\u68c0\u7d22\u952e \u548c ABC 123\uff0c\u786e\u8ba4\u9875\u7709\u9875\u811a\u4e0e\u6b63\u6587\u5171\u4eab\u6587\u672c\u5c42\u5951\u7ea6\u3002"))) {
        return sample;
    }

    auto header_contract = append_document_paragraph(document, "");
    if (!header_contract.has_next() ||
        !header_contract.add_run("Overlay header contract: ").has_next() ||
        !add_styled_contract_run(
            document, header_contract,
            utf8_from_u8(u8"SO-202 \u5076\u6570\u9875\u6837\u5f0f\u952e"),
            "DocumentPdfCjkFontEmbedLiteNote") ||
        !header_contract.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, header_contract,
            utf8_from_u8(u8"SO-303 \u9ed8\u8ba4\u9875\u6837\u5f0f\u952e"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto table = document.append_table(6U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 2100U) ||
        !table.set_column_width_twips(2U, 3600U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u53e0\u52a0\u533a")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "SO-A-03") ||
        !table.set_cell_text(1U, 1U, overlay_key) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"FE-SO-961 \u8868\u683c\u6837\u5f0f\u6536\u53e3\uff0c\u4fdd\u7559\u53ef\u590d\u5236 CJK \u952e")) ||
        !table.set_cell_text(2U, 0U, "FE-SO-961") ||
        !table.set_cell_text(2U, 1U, overlay_key) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"\u7eb5\u5411\u5408\u5e76\u5757\u7b2c\u4e00\u884c\uff0c\u548c\u91cd\u590d\u8868\u5934\u5171\u5b58")) ||
        !table.set_cell_text(3U, 0U, "FE-SO-981") ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(
                u8"\u7eb5\u5411\u5408\u5e76\u5757\u7b2c\u4e8c\u884c\uff0c\u4e0d\u7834\u574f\u6837\u5f0f\u53e0\u52a0")) ||
        !table.set_cell_text(4U, 0U, "SO-CANT") ||
        !table.set_cell_text(4U, 1U,
                             utf8_from_u8(u8"\u7981\u6b62\u62c6\u5206\u884c")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(
                u8"FE-SO-981 \u56de\u6d41\u884c\u9700\u8981\u4fdd\u6301\u6574\u884c\u5b8c\u6574")) ||
        !table.set_cell_text(5U, 0U, "FE-SO-999") ||
        !table.set_cell_text(5U, 1U, overlay_key)) {
        return sample;
    }

    auto merged_key = table.find_cell(2U, 1U);
    auto styled_cell = table.find_cell(5U, 2U);
    if (!merged_key.has_value() || !styled_cell.has_value() ||
        !merged_key->merge_down(1U) || !merged_key->set_fill_color("FCE4D6") ||
        !merged_key->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto styled_paragraph = styled_cell->paragraphs();
    if (!styled_paragraph.has_next() ||
        !styled_paragraph.add_run("Overlay close: ").has_next() ||
        !add_styled_contract_run(document, styled_paragraph, overlay_key,
                                 "DocumentPdfCjkFontEmbedLiteAccent") ||
        !styled_paragraph.add_run(" / FE-SO-999 / ").has_next() ||
        !add_styled_contract_run(document, styled_paragraph,
                                 utf8_from_u8(u8"\u7ec8\u9875\u590d\u68c0"),
                                 "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 440U : 660U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 4U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Overlay settle: ").has_next() ||
        !add_styled_contract_run(document, closing, overlay_key,
                                 "DocumentPdfCjkFontEmbedLiteAccent") ||
        !closing.add_run(" / FE-SO-981 / FE-SO-999").has_next()) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK style overlay page flow "
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
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_anchor_matrix_lite_text_sample(
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
        !title.set_text("Document CJK anchor matrix lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Anchor matrix lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"AM-101 \u8de8\u754c\u951a\u70b9"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto density = intro.insert_paragraph_after("");
    if (!density.has_next() ||
        !density.add_run("Font density: ").has_next() ||
        !add_styled_contract_run(
            document, density,
            utf8_from_u8(u8"AM-202 \u5b57\u4f53\u5bc6\u5ea6\u77e9\u9635"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto stripe = density.insert_paragraph_after("");
    if (!stripe.has_next() ||
        !stripe.add_run("Anchor stripe: ").has_next() ||
        !add_styled_contract_run(
            document, stripe,
            utf8_from_u8(u8"AM-303 \u68c0\u7d22\u5bc6\u6392 ABC 123"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto closing = stripe.insert_paragraph_after("");
    if (!closing.has_next() ||
        !closing.add_run("Anchor close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"AM-999 \u951a\u70b9\u77e9\u9635\u7ed3\u675f"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK anchor matrix lite text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 22.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_search_density_lite_text_sample(
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
        !title.set_text("Document CJK search density lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Search density lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"SD-101 \u5bc6\u6392\u68c0\u7d22"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto width = intro.insert_paragraph_after("");
    if (!width.has_next() ||
        !width.add_run("Width matrix: ").has_next() ||
        !add_styled_contract_run(
            document, width,
            utf8_from_u8(u8"SD-202 \u5b57\u5bbd\u8054\u6d4b copy search"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto ladder = width.insert_paragraph_after("");
    if (!ladder.has_next() ||
        !ladder.add_run("Density ladder: ").has_next() ||
        !add_styled_contract_run(
            document, ladder,
            utf8_from_u8(u8"SD-303 \u6ce8\u8bb0\u56de\u8bfb ABC 123"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto closing = ladder.insert_paragraph_after("");
    if (!closing.has_next() ||
        !closing.add_run("Density close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"SD-999 \u5bc6\u5ea6\u56de\u8bfb\u7ed3\u675f"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK search density lite text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 22.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_cjk_font_search_density_flow_text_sample(
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
        !title.set_text("Document CJK font search density flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Font search density: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"SD-101 \u5bc6\u6392\u68c0\u7d22"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-SD-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"SD-202 \u5b57\u53f7\u56de\u8bfb\u77e9\u9635"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    auto &first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto &first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto &even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Search density header SD-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            "Search density footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text(
            "Search density first header SD-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Search density first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text(
            "Search density even header SD-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            "Search density even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto stripe = append_document_paragraph(document, "");
    if (!stripe.has_next() ||
        !stripe.set_alignment(featherdoc::paragraph_alignment::right) ||
        !stripe.add_run("Density stripe: ").has_next() ||
        !add_styled_contract_run(
            document, stripe,
            utf8_from_u8(u8"SD-303 \u5b57\u5bbd\u8054\u6d4b"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !stripe.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, stripe,
            utf8_from_u8(u8"FE-SD-921 \u590d\u5236\u56de\u8bfb ABC 123"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto ladder = append_document_paragraph(document, "");
    if (!ladder.has_next() ||
        !ladder.add_run("Density ladder: ").has_next() ||
        !add_styled_contract_run(
            document, ladder, utf8_from_u8(u8"\u6ce8\u8bb0\u56de\u8bfb"),
            "DocumentPdfCjkFontEmbedLiteNote") ||
        !ladder.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, ladder, utf8_from_u8(u8"\u5408\u540c\u77e9\u9635"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !ladder.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, ladder, utf8_from_u8(u8"\u5927\u5b57\u5d4c\u5b57"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    const std::array<std::string, 2> density_markers{
        utf8_from_u8(
            u8"\u7b2c 1 \u7ec4\u5bc6\u6392\u68c0\u7d22\u9a8c\u8bc1 FE-SD-931"),
        utf8_from_u8(
            u8"\u7b2c 2 \u7ec4\u5bc6\u6392\u68c0\u7d22\u9a8c\u8bc1 FE-SD-932"),
    };
    for (const auto &marker_text : density_markers) {
        if (!append_document_text_paragraph(document, marker_text)) {
            return sample;
        }
    }

    auto table = document.append_table(5U, 4U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1200U) ||
        !table.set_column_width_twips(1U, 1200U) ||
        !table.set_column_width_twips(2U, 1600U) ||
        !table.set_column_width_twips(3U, 3200U) ||
        !table.set_cell_text(0U, 0U,
                             utf8_from_u8(u8"\u68c0\u7d22\u5bc6\u5ea6\u8868")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"\u533a\u57df")) ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"\u5b57\u53f7")) ||
        !table.set_cell_text(1U, 2U, utf8_from_u8(u8"\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(1U, 3U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(2U, 0U, "SD-A-04") ||
        !table.set_cell_text(2U, 1U, utf8_from_u8(u8"\u5c0f\u5b57")) ||
        !table.set_cell_text(2U, 2U, "Density checkpoint A:") ||
        !table.set_cell_text(
            2U, 3U,
            utf8_from_u8(
                u8"\u8868\u683c\u8854\u63a5 FE-SD-931 \u987b\u4fdd\u6301 copy search")) ||
        !table.set_cell_text(3U, 0U, "SD-B-04") ||
        !table.set_cell_text(3U, 1U, utf8_from_u8(u8"\u5927\u5b57")) ||
        !table.set_cell_text(3U, 2U, "Density checkpoint B:") ||
        !table.set_cell_text(
            3U, 3U,
            utf8_from_u8(
                u8"\u53f3\u4fa7\u6536\u53e3\u3001\u9876\u5e95\u5207\u6362\u548c\u7ec8\u9875\u68c0\u7d22")) ||
        !table.set_cell_text(4U, 0U, "FE-SD-999") ||
        !table.set_cell_text(4U, 1U, utf8_from_u8(u8"\u7ec8\u70b9")) ||
        !table.set_cell_text(4U, 2U, utf8_from_u8(u8"\u56de\u8bfb\u590d\u68c0")) ||
        !table.set_cell_text(
            4U, 3U,
            utf8_from_u8(
                u8"\u6062\u590d\u6574\u680f\u5e76\u786e\u8ba4\u56fe\u7247\u4e0b\u65b9\u56de\u6d41"))) {
        return sample;
    }

    auto density_header = table.find_cell(0U, 0U);
    auto close_cell = table.find_cell(4U, 2U);
    auto heading_region = table.find_cell(1U, 0U);
    auto heading_size = table.find_cell(1U, 1U);
    auto heading_key = table.find_cell(1U, 2U);
    auto heading_note = table.find_cell(1U, 3U);
    if (!density_header.has_value() || !close_cell.has_value() ||
        !heading_region.has_value() || !heading_size.has_value() ||
        !heading_key.has_value() || !heading_note.has_value() ||
        !density_header->merge_right(3U) || !close_cell->merge_right(1U)) {
        return sample;
    }

    if (!density_header->set_text(
            utf8_from_u8(u8"\u68c0\u7d22\u5bc6\u5ea6\u8868")) ||
        !close_cell->set_text(utf8_from_u8(
            u8"\u56de\u8bfb\u590d\u68c0 FE-SD-999 \u6062\u590d\u6574\u680f"))) {
        return sample;
    }

    if (!density_header->set_fill_color("D9EAF7") ||
        !heading_region->set_fill_color("EAF2F8") ||
        !heading_size->set_fill_color("EAF2F8") ||
        !heading_key->set_fill_color("EAF2F8") ||
        !heading_note->set_fill_color("EAF2F8") ||
        !close_cell->set_fill_color("F6E7D8") ||
        !close_cell->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index < 2U ? 420U : 560U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index < 2U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 4U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Density close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-SD-999 \u56de\u8bfb\u590d\u68c0"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK font search density flow "
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
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 16.5;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_repeated_key_lite_text_sample(
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
        !title.set_text("Document CJK repeated key lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Repeated key lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"RK-101 \u8de8\u533a\u68c0\u7d22\u952e"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto shared = intro.insert_paragraph_after("");
    if (!shared.has_next() ||
        !shared.add_run("Shared marker: ").has_next() ||
        !add_styled_contract_run(
            document, shared,
            utf8_from_u8(u8"RK-777 \u5171\u7528\u68c0\u7d22\u952e"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto repeat = shared.insert_paragraph_after("");
    if (!repeat.has_next() ||
        !repeat.add_run("Repeat marker: ").has_next() ||
        !add_styled_contract_run(
            document, repeat,
            utf8_from_u8(u8"RK-777 \u5171\u7528\u68c0\u7d22\u952e ABC 123"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto closing = repeat.insert_paragraph_after("");
    if (!closing.has_next() ||
        !closing.add_run("Repeat close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"RK-999 \u91cd\u590d\u952e\u7ed3\u675f"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK repeated key lite text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 22.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_cjk_repeated_key_boundary_flow_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    const auto repeated_key =
        utf8_from_u8(u8"RK-777 \u5171\u7528\u68c0\u7d22\u952e");

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
        !title.set_text("Document CJK repeated key boundary flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Repeated key flow: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"RK-101 \u8de8\u533a\u68c0\u7d22\u952e"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-RK-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"RK-202 \u8fb9\u754c\u590d\u73b0\u77e9\u9635"),
            "DocumentPdfCjkFontEmbedLiteNote") ||
        !intro.add_run(" / RK-303").has_next()) {
        return sample;
    }

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    auto &first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto &first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto &even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text(
            utf8_from_u8(u8"Repeat key header RK-303 \u5171\u7528\u952e page {{page}}")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            utf8_from_u8(u8"Repeat key footer RK-777 \u5171\u7528\u952e {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(
            utf8_from_u8(u8"Repeat key first header RK-101 \u8de8\u533a page {{page}}")) ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            utf8_from_u8(u8"Repeat key first footer RK-777 \u5171\u7528\u952e {{page}} / {{total_pages}}")) ||
        !even_header.has_next() ||
        !even_header.set_text(
            utf8_from_u8(u8"Repeat key even header RK-202 \u8fb9\u754c page {{page}}")) ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            utf8_from_u8(u8"Repeat key even footer RK-777 \u5171\u7528\u952e {{page}} / {{total_pages}}"))) {
        return sample;
    }

    auto marker = append_document_paragraph(document, "");
    if (!marker.has_next() ||
        !marker.set_alignment(featherdoc::paragraph_alignment::right) ||
        !marker.add_run("Repeat marker: ").has_next() ||
        !add_styled_contract_run(document, marker, repeated_key,
                                 "DocumentPdfCjkFontEmbedLiteAccent") ||
        !marker.add_run(" / FE-RK-911 / ").has_next() ||
        !add_styled_contract_run(document, marker,
                                 utf8_from_u8(u8"\u6b63\u6587\u590d\u73b0"),
                                 "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto table = document.append_table(6U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 1900U) ||
        !table.set_column_width_twips(2U, 3800U) ||
        !table.set_cell_text(0U, 0U,
                             utf8_from_u8(u8"\u540c\u952e\u590d\u73b0\u8868")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "RK-A-01") ||
        !table.set_cell_text(1U, 1U, repeated_key) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"Repeat checkpoint: FE-RK-941 \u8868\u5934\u4e4b\u540e\u6587\u672c\u5c42\u8fde\u7eed")) ||
        !table.set_cell_text(2U, 0U, "RK-A-04") ||
        !table.set_cell_text(2U, 1U, repeated_key) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"FE-RK-951 \u5408\u5e76\u5757\u7b2c\u4e00\u5904\u590d\u73b0")) ||
        !table.set_cell_text(3U, 0U, "RK-A-05") ||
        !table.set_cell_text(3U, 2U,
                             utf8_from_u8(u8"\u5408\u5e76\u5757\u7b2c\u4e8c\u884c")) ||
        !table.set_cell_text(4U, 0U, "RK-A-06") ||
        !table.set_cell_text(4U, 1U,
                             utf8_from_u8(u8"\u7981\u62c6\u6781\u9650")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(
                u8"FE-RK-961 \u8868\u540e\u6062\u590d \u4e0d\u80fd\u628a\u5171\u7528\u952e\u538b\u5230\u9875\u811a")) ||
        !table.set_cell_text(5U, 0U, "FE-RK-981") ||
        !table.set_cell_text(5U, 1U, repeated_key) ||
        !table.set_cell_text(
            5U, 2U,
            utf8_from_u8(
                u8"Repeat settle: \u7ec8\u9875\u518d\u6b21\u590d\u73b0 ABC 123"))) {
        return sample;
    }

    auto merged_key = table.find_cell(2U, 1U);
    if (!merged_key.has_value() || !merged_key->merge_down(1U) ||
        !merged_key->set_fill_color("FCE4D6") ||
        !merged_key->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 440U : 680U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 4U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Repeat close: ").has_next() ||
        !add_styled_contract_run(document, closing, repeated_key,
                                 "DocumentPdfCjkFontEmbedLiteAccent") ||
        !closing.add_run(" / FE-RK-999 / ").has_next() ||
        !add_styled_contract_run(document, closing,
                                 utf8_from_u8(u8"\u7ec8\u9875\u590d\u68c0"),
                                 "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK repeated key boundary "
        "flow text";
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
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_vertical_merge_lite_text_sample(
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
        !title.set_text("Document CJK vertical merge lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"VM-101 \u7eb5\u5411\u5408\u5e76\u8054\u52a8"))) {
        return sample;
    }

    auto table = document.append_table(4U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 2600U) ||
        !table.set_column_width_twips(2U, 2800U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u6848\u4f8b")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u8d1f\u8d23\u4eba")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "VM-202") ||
        !table.set_cell_text(1U, 1U,
                             utf8_from_u8(u8"\u7edf\u7b79\u5757")) ||
        !table.set_cell_text(1U, 2U,
                             utf8_from_u8(u8"\u6574\u5757\u8fc1\u79fb")) ||
        !table.set_cell_text(2U, 0U, "VM-303") ||
        !table.set_cell_text(2U, 2U,
                             utf8_from_u8(u8"\u7ffb\u9875\u56de\u6d41")) ||
        !table.set_cell_text(3U, 0U, "VM-999") ||
        !table.set_cell_text(3U, 1U,
                             utf8_from_u8(u8"\u5c3e\u884c\u590d\u68c0")) ||
        !table.set_cell_text(3U, 2U,
                             utf8_from_u8(u8"\u6574\u5757\u7ed3\u675f"))) {
        return sample;
    }

    auto owner_block = table.find_cell(1U, 1U);
    if (!owner_block.has_value() || !owner_block->merge_down(1U) ||
        !owner_block->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center) ||
        !owner_block->set_fill_color("E8F3E8")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 4U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 480U : 680U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 1U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(u8"VM-777 \u8868\u540e\u56de\u6d41\u68c0\u7d22\u952e"))) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK vertical merge lite text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}
