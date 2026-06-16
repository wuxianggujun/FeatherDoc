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
