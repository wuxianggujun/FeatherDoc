[[nodiscard]] ScenarioResult build_document_cjk_style_overlay_lite_text_sample(
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
        !title.set_text("Document CJK style overlay lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Style overlay lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"SO-101 \u6837\u5f0f\u4e0a\u6807"),
            "DocumentPdfCjkOverlayLiteSuperscript")) {
        return sample;
    }

    auto baseline = intro.insert_paragraph_after("");
    if (!baseline.has_next() ||
        !baseline.add_run("Overlay baseline: ").has_next() ||
        !add_styled_contract_run(
            document, baseline,
            utf8_from_u8(u8"SO-202 \u6837\u5f0f\u4e0b\u6807"),
            "DocumentPdfCjkOverlayLiteSubscript")) {
        return sample;
    }

    auto strike = baseline.insert_paragraph_after("");
    if (!strike.has_next() ||
        !strike.add_run("Overlay strike: ").has_next() ||
        !add_styled_contract_run(
            document, strike,
            utf8_from_u8(u8"SO-303 \u5220\u9664\u7ebf\u4ecd\u53ef\u641c"),
            "DocumentPdfCjkOverlayLiteStrike")) {
        return sample;
    }

    auto closing = strike.insert_paragraph_after("");
    if (!closing.has_next() ||
        !closing.add_run("Overlay close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"SO-999 \u6837\u5f0f\u53e0\u52a0\u7ed3\u675f"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK style overlay lite text";
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

[[nodiscard]] ScenarioResult build_document_cjk_complex_layout_text_sample(
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
        !title.set_text("Document CJK complex layout sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Complex layout overview: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"\u9875\u7709\u9875\u811a\u5206\u9875\u53d8\u4f53"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-CL-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"\u591a\u951a\u70b9\u56fe\u6587\u6df7\u6392"),
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
        !default_header.set_text("Complex header CL-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            "Complex footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text(
            "Complex first header CL-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Complex first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Complex even header CL-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            "Complex even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto style_matrix = append_document_paragraph(document, "");
    if (!style_matrix.has_next() ||
        !style_matrix.add_run("Wrap matrix: ").has_next() ||
        !add_styled_contract_run(document, style_matrix,
                                 utf8_from_u8(u8"\u56fe\u6587\u951a\u70b9"),
                                 "DocumentPdfCjkFontEmbedLiteAccent") ||
        !style_matrix.add_run(" + FE-CL-902 + ").has_next() ||
        !add_styled_contract_run(document, style_matrix,
                                 utf8_from_u8(u8"\u88c1\u526a\u73af\u7ed5"),
                                 "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"Top-bottom \u6d6e\u52a8\u56fe\u4e4b\u540e\u7684\u6b63\u6587\u9700\u8981\u6574\u4f53\u56de\u5230\u5168\u5bbd\u5217\uff0c\u4fdd\u7559 FE-CL-903 \u4e0e\u4e2d\u6587\u68c0\u7d22\u56fe\u5c42\u3002")) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"\u7b2c 1 \u6bb5\u7ee7\u7eed\u9a8c\u8bc1\u590d\u6742\u7248\u5f0f\u7a33\u5b9a\u6027\uff0c\u6df7\u6392 English token FE-CL-921 \u548c ABC 123\u3002")) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"\u7b2c 18 \u6bb5\u7ee7\u7eed\u9a8c\u8bc1\u590d\u6742\u7248\u5f0f\u7a33\u5b9a\u6027\uff0c\u9875\u7709\u9875\u811a\u5360\u4f4d\u7b26\u4fdd\u6301\u53ef\u8ffd\u8e2a\u3002"))) {
        return sample;
    }

    auto header_contract = append_document_paragraph(document, "");
    if (!header_contract.has_next() ||
        !header_contract.add_run("Complex header contract: ").has_next() ||
        !add_styled_contract_run(document, header_contract, "CL-202",
                                 "DocumentPdfCjkFontEmbedLiteNote") ||
        !header_contract.add_run(" / ").has_next() ||
        !add_styled_contract_run(document, header_contract, "CL-303",
                                 "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto table = document.append_table(5U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 2200U) ||
        !table.set_column_width_twips(2U, 3500U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u533a\u57df")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u951a\u70b9")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "CL-A-01") ||
        !table.set_cell_text(1U, 1U,
                             utf8_from_u8(u8"\u56fe\u6587\u951a\u70b9")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"FE-CL-941 \u8868\u683c\u4e2d\u4fdd\u7559\u590d\u6742\u7248\u5f0f\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(2U, 0U, "CL-A-02") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u88c1\u526a\u73af\u7ed5")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"FE-CL-951 \u7eb5\u5411\u5408\u5e76\u5757\u4fdd\u7559\u6574\u5757\u8bed\u4e49")) ||
        !table.set_cell_text(3U, 0U, "CL-A-03") ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(
                u8"\u5408\u5e76\u5757\u7b2c\u4e8c\u884c\u7ee7\u7eed\u68c0\u67e5\u56de\u6d41\u3002")) ||
        !table.set_cell_text(4U, 0U, "FE-CL-999") ||
        !table.set_cell_text(4U, 1U,
                             utf8_from_u8(u8"\u7ec8\u9875\u6536\u53e3")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(
                u8"\u590d\u6742\u7248\u5f0f\u6536\u53e3\u884c\u4fdd\u7559 ABC 123 \u5e76\u7981\u6b62\u62c6\u5206\u3002"))) {
        return sample;
    }

    auto merged_anchor = table.find_cell(2U, 1U);
    if (!merged_anchor.has_value() || !merged_anchor->merge_down(1U) ||
        !merged_anchor->set_fill_color("E8F3E8") ||
        !merged_anchor->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
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
        !closing.add_run("Complex close: ").has_next() ||
        !add_styled_contract_run(document, closing,
                                 utf8_from_u8(u8"FE-CL-999 \u7ec8\u9875\u6536\u53e3"),
                                 "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK complex layout text";
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

[[nodiscard]] ScenarioResult build_document_cjk_image_wrap_stress_text_sample(
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
        !title.set_text("Document CJK image wrap stress sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Image wrap stress: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"\u591a\u951a\u70b9\u56fe\u50cf\u6d41"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-IW-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"\u88c1\u526a\u73af\u7ed5\u77e9\u9635"),
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
        !default_header.set_text("Image header IW-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Image footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text(
            "Image first header IW-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Image first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Image even header IW-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            "Image even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto header_contract = append_document_paragraph(document, "");
    if (!header_contract.has_next() ||
        !header_contract.add_run("Image header contract: ").has_next() ||
        !add_styled_contract_run(
            document, header_contract, "Image first header IW-101 page 1",
            "DocumentPdfCjkFontEmbedLiteNote") ||
        !header_contract
             .add_run(" / Image even header IW-202 page 2 / Image header "
                      "IW-303 page 3")
             .has_next()) {
        return sample;
    }

    auto footer_contract = append_document_paragraph(document, "");
    if (!footer_contract.has_next() ||
        !footer_contract.add_run("Image footer contract: ").has_next() ||
        !add_styled_contract_run(
            document, footer_contract, "Image first footer 1 / 4",
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !footer_contract
             .add_run(" / Image even footer 2 / 4 / Image footer 4 / 4")
             .has_next()) {
        return sample;
    }

    auto search_stripe = append_document_paragraph(document, "");
    if (!search_stripe.has_next() ||
        !search_stripe.add_run("Search stripe: ").has_next() ||
        !add_styled_contract_run(
            document, search_stripe,
            utf8_from_u8(u8"\u56de\u8bfb\u70ed\u533a"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !search_stripe.add_run(" / FE-IW-921 / ").has_next() ||
        !add_styled_contract_run(
            document, search_stripe,
            utf8_from_u8(u8"\u7ec8\u9875\u951a\u70b9"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"\u7b2c 1 \u7ec4\u56fe\u50cf\u73af\u7ed5\u9a8c\u8bc1\uff1a\u591a\u951a\u70b9\u56fe\u50cf\u6d41\u4fdd\u7559\u641c\u7d22\u70ed\u533a\uff0c\u6df7\u6392 English token FE-IW-921 \u548c ABC 123\u3002")) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"\u7b2c 18 \u7ec4\u56fe\u50cf\u73af\u7ed5\u9a8c\u8bc1\uff1a\u88c1\u526a\u73af\u7ed5\u77e9\u9635\u4fdd\u6301\u9875\u7709\u9875\u811a\u7d22\u5f15\uff0c\u68c0\u7d22\u952e FE-IW-938\u3002")) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"\u7b2c 36 \u7ec4\u56fe\u50cf\u73af\u7ed5\u9a8c\u8bc1\uff1a\u7ec8\u9875\u951a\u70b9\u4e0e\u88c1\u526a\u56de\u6d41\u9700\u8981\u5728\u6587\u672c\u5c42\u7a33\u5b9a\uff0c\u68c0\u7d22\u952e FE-IW-956\u3002"))) {
        return sample;
    }

    auto table = document.append_table(6U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 2200U) ||
        !table.set_column_width_twips(2U, 3500U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u533a\u57df")) ||
        !table.set_cell_text(0U, 1U,
                             utf8_from_u8(u8"\u73af\u7ed5\u5951\u7ea6")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "IW-A-01") ||
        !table.set_cell_text(
            1U, 1U, utf8_from_u8(u8"\u591a\u951a\u70b9\u56fe\u50cf\u6d41")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"FE-IW-901 \u8868\u683c\u4e2d\u4fdd\u7559\u56fe\u50cf\u73af\u7ed5\u4e3b\u9898\u952e")) ||
        !table.set_cell_text(2U, 0U, "FE-IW-921") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u56de\u8bfb\u70ed\u533a")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"\u7eb5\u5411\u5408\u5e76\u5757\u7b2c\u4e00\u884c\u56fa\u5b9a\u641c\u7d22\u533a")) ||
        !table.set_cell_text(3U, 0U, "FE-IW-938") ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(
                u8"\u7eb5\u5411\u5408\u5e76\u5757\u7b2c\u4e8c\u884c\u7ee7\u7eed\u68c0\u67e5\u56de\u6d41")) ||
        !table.set_cell_text(4U, 0U, "FE-IW-956") ||
        !table.set_cell_text(4U, 1U,
                             utf8_from_u8(u8"\u88c1\u526a\u56de\u6d41")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(
                u8"\u7981\u6b62\u62c6\u5206\u884c\u4fdd\u6301\u7ec8\u9875\u56de\u6d41\u8bed\u4e49")) ||
        !table.set_cell_text(5U, 0U, "FE-IW-999") ||
        !table.set_cell_text(5U, 1U,
                             utf8_from_u8(u8"\u7ec8\u9875\u951a\u70b9")) ||
        !table.set_cell_text(
            5U, 2U,
            utf8_from_u8(
                u8"Closing matrix \u6536\u53e3\u884c\u4fdd\u7559 ABC 123 \u548c\u88c1\u526a\u56de\u6d41\u3002"))) {
        return sample;
    }

    auto merged_hotspot = table.find_cell(2U, 1U);
    if (!merged_hotspot.has_value() || !merged_hotspot->merge_down(1U) ||
        !merged_hotspot->set_fill_color("E2F0D9") ||
        !merged_hotspot->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 440U : 620U,
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
        !closing.add_run("Closing matrix: ").has_next() ||
        !add_styled_contract_run(document, closing, "FE-IW-999",
                                 "DocumentPdfCjkFontEmbedLiteAccent") ||
        !closing.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"\u7ec8\u9875\u951a\u70b9"),
            "DocumentPdfCjkFontEmbedLiteNote") ||
        !closing.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"\u88c1\u526a\u56de\u6d41"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK image wrap stress text";
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
    options.line_height_points = 15.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_extreme_page_breaks_text_sample(
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
        !title.set_text("Document CJK extreme page breaks sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Extreme page breaks: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"\u4e34\u754c\u5206\u9875\u8fb9\u754c"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-PB-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"\u6bb5\u843d\u56de\u6d41\u77e9\u9635"),
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
        !default_header.set_text("Break header PB-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Break footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text(
            "Break first header PB-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Break first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Break even header PB-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            "Break even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto header_contract = append_document_paragraph(document, "");
    if (!header_contract.has_next() ||
        !header_contract.add_run("Break header contract: ").has_next() ||
        !add_styled_contract_run(
            document, header_contract, "Break first header PB-101 page 1",
            "DocumentPdfCjkFontEmbedLiteNote") ||
        !header_contract
             .add_run(" / Break even header PB-202 page 2 / Break header "
                      "PB-303 page 3")
             .has_next()) {
        return sample;
    }

    auto footer_contract = append_document_paragraph(document, "");
    if (!footer_contract.has_next() ||
        !footer_contract.add_run("Break footer contract: ").has_next() ||
        !add_styled_contract_run(
            document, footer_contract, "Break first footer 1 / 5",
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !footer_contract
             .add_run(" / Break even footer 2 / 5 / Break footer 5 / 5")
             .has_next()) {
        return sample;
    }

    auto marker = append_document_paragraph(document, "");
    if (!marker.has_next() ||
        !marker.set_alignment(featherdoc::paragraph_alignment::right) ||
        !marker.add_run("Boundary stripe: ").has_next() ||
        !add_styled_contract_run(
            document, marker, utf8_from_u8(u8"\u7ec8\u9875\u6536\u53e3"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !marker.add_run(" / FE-PB-921 / ").has_next() ||
        !add_styled_contract_run(
            document, marker, utf8_from_u8(u8"\u56de\u6d41\u68c0\u6d4b"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"\u7b2c 1 \u7ec4\u5206\u9875\u8fb9\u754c\u9a8c\u8bc1\uff1a\u5de6\u4fa7\u5305\u56f4\u7f29\u8fdb\u4fdd\u7559\u9875\u7709\u7d22\u5f15\u3001\u9875\u811a\u7d22\u5f15\u548c English token FE-PB-921\u3002")) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"\u7b2c 12 \u7ec4\u5206\u9875\u8fb9\u754c\u9a8c\u8bc1\uff1a\u6574\u680f\u56de\u6d41\u5207\u6362\u4fdd\u6301\u6587\u672c\u5c42\u8fde\u7eed\uff0c\u68c0\u7d22\u952e FE-PB-932\u3002")) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"\u7b2c 18 \u7ec4\u5206\u9875\u8fb9\u754c\u9a8c\u8bc1\uff1a\u672b\u9875\u4e34\u754c\u5206\u9875\u4e0d\u5e94\u7f3a\u5b57\u3001\u91cd\u53e0\u6216\u9519\u8bef\u7f29\u8fdb\uff0c\u68c0\u7d22\u952e FE-PB-938\u3002"))) {
        return sample;
    }

    auto table = document.append_table(6U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 2200U) ||
        !table.set_column_width_twips(2U, 3500U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u533a\u57df")) ||
        !table.set_cell_text(0U, 1U,
                             utf8_from_u8(u8"\u5206\u9875\u5951\u7ea6")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "PB-A-01") ||
        !table.set_cell_text(
            1U, 1U,
            utf8_from_u8(u8"\u4e34\u754c\u5206\u9875\u8fb9\u754c")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"FE-PB-901 \u8868\u683c\u4e2d\u4fdd\u7559\u5206\u9875\u8fb9\u754c\u4e3b\u9898\u952e")) ||
        !table.set_cell_text(2U, 0U, "FE-PB-921") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u56de\u6d41\u68c0\u6d4b")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"\u7eb5\u5411\u5408\u5e76\u5757\u7b2c\u4e00\u884c\u56fa\u5b9a\u4e34\u754c\u8fb9\u754c")) ||
        !table.set_cell_text(3U, 0U, "FE-PB-932") ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(
                u8"\u7eb5\u5411\u5408\u5e76\u5757\u7b2c\u4e8c\u884c\u7ee7\u7eed\u68c0\u67e5\u6574\u680f\u56de\u6d41")) ||
        !table.set_cell_text(4U, 0U, "FE-PB-938") ||
        !table.set_cell_text(4U, 1U,
                             utf8_from_u8(u8"\u672b\u9875\u6536\u53e3")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(
                u8"\u7981\u6b62\u62c6\u5206\u884c\u4fdd\u6301\u4e34\u754c\u5206\u9875\u8bed\u4e49")) ||
        !table.set_cell_text(5U, 0U, "FE-PB-999") ||
        !table.set_cell_text(5U, 1U,
                             utf8_from_u8(u8"\u7ec8\u9875\u6536\u53e3")) ||
        !table.set_cell_text(
            5U, 2U,
            utf8_from_u8(
                u8"Boundary close \u6536\u53e3\u884c\u4fdd\u7559 ABC 123 \u548c\u672b\u9875\u6536\u53e3\u3002"))) {
        return sample;
    }

    auto merged_boundary = table.find_cell(2U, 1U);
    if (!merged_boundary.has_value() || !merged_boundary->merge_down(1U) ||
        !merged_boundary->set_fill_color("DDEBF7") ||
        !merged_boundary->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 440U : 600U,
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
        !closing.add_run("Boundary close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"\u4e34\u754c\u5206\u9875\u8fb9\u754c"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !closing.add_run(" / FE-PB-999 / ").has_next() ||
        !add_styled_contract_run(
            document, closing, utf8_from_u8(u8"\u672b\u9875\u6536\u53e3"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK extreme page breaks text";
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
    options.line_height_points = 15.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}
