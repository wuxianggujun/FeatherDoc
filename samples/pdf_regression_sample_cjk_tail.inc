[[nodiscard]] ScenarioResult build_document_cjk_table_wrap_lite_text_sample(
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
        !title.set_text("Document CJK table wrap lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(u8"TW-101 \u957f\u6587\u672c\u5355\u5143\u683c\u6362\u884c"))) {
        return sample;
    }

    auto table = document.append_table(4U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1600U) ||
        !table.set_column_width_twips(1U, 2500U) ||
        !table.set_column_width_twips(2U, 3100U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u9636\u6bb5")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u8bc1\u636e")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "TW-202") ||
        !table.set_cell_text(1U, 1U,
                             utf8_from_u8(u8"\u590d\u5236\u6821\u9a8c")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"\u957f\u6587\u672c\u5355\u5143\u683c\u9700\u8981\u7a33\u5b9a\u6362\u884c\u5e76\u4fdd\u7559 CJK \u641c\u7d22\u952e TW-303")) ||
        !table.set_cell_text(2U, 0U, "TW-404") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u7ec8\u9875\u7d22\u5f15")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"\u8868\u683c\u6d41\u8f6c\u770b\u677f\u7ee7\u7eed\u56de\u8bfb ABC 123")) ||
        !table.set_cell_text(3U, 0U, "TW-999") ||
        !table.set_cell_text(3U, 1U,
                             utf8_from_u8(u8"\u6536\u53e3")) ||
        !table.set_cell_text(3U, 2U,
                             utf8_from_u8(u8"\u6362\u884c\u5951\u7ea6\u7ed3\u675f"))) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 4U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 480U : 760U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 1U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK table wrap lite text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_cjk_table_wrap_page_flow_text_sample(
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
        !title.set_text("Document CJK table wrap page flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"Table wrap page flow: TF-101 \u4ea4\u9519\u6587\u6863\u6d41"))) {
        return sample;
    }

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    auto &first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto &first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto &even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text(
            utf8_from_u8(u8"Flow header TF-303 page {{page}}")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            utf8_from_u8(u8"Flow footer {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(
            utf8_from_u8(u8"Flow first header TF-101 page {{page}}")) ||
        !even_header.has_next() ||
        !even_header.set_text(
            utf8_from_u8(u8"Flow even header TF-202 page {{page}}")) ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            utf8_from_u8(u8"Flow first footer {{page}} / {{total_pages}}")) ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            utf8_from_u8(u8"Flow even footer {{page}} / {{total_pages}}"))) {
        return sample;
    }

    auto stripe = title.insert_paragraph_after("");
    if (!stripe.has_next() ||
        !stripe.add_run("Flow stripe: ").has_next() ||
        !stripe
             .add_run(utf8_from_u8(
                 u8"FE-TF-921 \u5206\u9875\u73af\u7ed5\u77e9\u9635 ABC 123"))
             .has_next()) {
        return sample;
    }

    auto table = document.append_table(5U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 2300U) ||
        !table.set_column_width_twips(2U, 3400U) ||
        !table.set_cell_text(0U, 0U,
                             utf8_from_u8(u8"\u6d41\u8f6c\u770b\u677f")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u9636\u6bb5")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "TF-A-01") ||
        !table.set_cell_text(1U, 1U,
                             utf8_from_u8(u8"\u8868\u5934\u590d\u7528")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"Flow checkpoint A: \u957f\u6587\u672c\u5355\u5143\u683c\u9700\u4fdd\u6301\u6362\u884c\u548c\u56de\u8bfb\u3002")) ||
        !table.set_cell_text(2U, 0U, "TF-A-05") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u5206\u9875\u8854\u63a5")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"\u8868\u683c\u6d41\u5728\u540e\u7eed\u53ef\u89c6\u5316\u9a8c\u8bc1\u4e2d\u8981\u4fdd\u7559\u53f3\u4fa7\u951a\u70b9\u3002")) ||
        !table.set_cell_text(3U, 0U, "TF-B-03") ||
        !table.set_cell_text(3U, 1U,
                             utf8_from_u8(u8"\u7ec8\u9875\u5408\u540c")) ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(
                u8"Flow checkpoint B: \u53f3\u4fa7\u951a\u70b9\u4e0e FE-TF-921 \u4fdd\u6301\u7a33\u5b9a\u3002")) ||
        !table.set_cell_text(4U, 0U, "FE-TF-999") ||
        !table.set_cell_text(4U, 1U,
                             utf8_from_u8(u8"\u56de\u8bfb\u590d\u68c0")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(
                u8"Flow close: \u56fe\u7247\u4e0b\u65b9\u6062\u590d\u5168\u5bbd\u7684\u9759\u6001\u68c0\u7d22\u952e\u3002"))) {
        return sample;
    }

    auto board_header = table.find_cell(0U, 0U);
    auto phase_block = table.find_cell(1U, 1U);
    if (!board_header.has_value() || !phase_block.has_value() ||
        !board_header->merge_right(2U) || !phase_block->merge_down(1U)) {
        return sample;
    }

    if (!board_header->set_text(utf8_from_u8(u8"\u6d41\u8f6c\u770b\u677f")) ||
        !board_header->set_fill_color("D9EAF7") ||
        !phase_block->set_fill_color("E8F3E8") ||
        !phase_block->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 480U : 720U,
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

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"Flow settle: \u5c3e\u9875\u56de\u6d41\u8865\u7ebf FE-TF-999"))) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK table wrap page flow";
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
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_cjk_font_embed_wrap_mix_lite_text_sample(
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
        !title.set_text("Document CJK font embed wrap mix lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Font embed wrap mix lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"WM-101 \u5d4c\u5b57\u73af\u7ed5\u7532"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto wrap = intro.insert_paragraph_after("");
    if (!wrap.has_next() ||
        !wrap.add_run("Wrap matrix: ").has_next() ||
        !add_styled_contract_run(
            document, wrap,
            utf8_from_u8(
                u8"FE-WM-202 \u957f\u53e5\u6362\u884c\u4fdd\u7559 copy search ABC 123"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto scale = wrap.insert_paragraph_after("");
    if (!scale.has_next() ||
        !scale.add_run("Wrap scale: ").has_next() ||
        !add_styled_contract_run(
            document, scale,
            utf8_from_u8(u8"WM-303 \u5927\u5b57\u5d4c\u5b57\u56de\u8bfb"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    auto closing = scale.insert_paragraph_after("");
    if (!closing.has_next() ||
        !closing.add_run("Wrap close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-WM-999 \u73af\u7ed5\u77e9\u9635\u6536\u53e3"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK font embed wrap mix lite "
        "text";
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

[[nodiscard]] ScenarioResult
build_document_cjk_font_embed_wrap_mix_text_sample(
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
        !title.set_text("Document CJK font embed wrap mix sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Font embed wrap mix: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"WM-101 \u5d4c\u5b57\u73af\u7ed5\u7532"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-WM-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"WM-202 \u88c1\u526a\u56de\u8bfb\u77e9\u9635"),
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
        !default_header.set_text("Wrap header WM-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Wrap footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Wrap first header WM-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("Wrap first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Wrap even header WM-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("Wrap even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto stripe = append_document_paragraph(document, "");
    if (!stripe.has_next() ||
        !stripe.add_run("Embed wrap stripe: ").has_next() ||
        !add_styled_contract_run(
            document, stripe,
            utf8_from_u8(u8"FE-WM-911 \u5b57\u4f53\u56de\u8bfb ABC 123"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto scale = append_document_paragraph(document, "");
    if (!scale.has_next() || !scale.add_run("Wrap scale: ").has_next() ||
        !add_styled_contract_run(
            document, scale,
            utf8_from_u8(u8"WM-303 \u5927\u5b57\u5d4c\u5b57"),
            "DocumentPdfCjkFontEmbedLiteLarge") ||
        !scale.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, scale,
            utf8_from_u8(u8"FE-WM-921 \u5408\u540c\u77e9\u9635"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto table = document.append_table(5U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 1900U) ||
        !table.set_column_width_twips(2U, 3800U) ||
        !table.set_cell_text(0U, 0U,
                             utf8_from_u8(u8"\u73af\u7ed5\u77e9\u9635\u8868")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u9636\u6bb5")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "FE-WM-941") ||
        !table.set_cell_text(1U, 1U,
                             utf8_from_u8(u8"\u56fe\u7247\u88c1\u526a")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"Wrap checkpoint A: \u73af\u7ed5\u77e9\u9635\u7532 \u5b57\u5bbd\u6821\u9a8c")) ||
        !table.set_cell_text(2U, 0U, "FE-WM-951") ||
        !table.set_cell_text(
            2U, 1U,
            utf8_from_u8(u8"\u590d\u5236\u641c\u7d22\u8def\u5f84")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"Wrap checkpoint B: \u7ec8\u9875\u951a\u70b9 copy search ABC 123")) ||
        !table.set_cell_text(3U, 0U, "FE-WM-961") ||
        !table.set_cell_text(3U, 1U,
                             utf8_from_u8(u8"\u5d4c\u5b57\u7ec4")) ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(
                u8"\u7b2c 1 \u7ec4\u73af\u7ed5\u5d4c\u5b57\u9a8c\u8bc1")) ||
        !table.set_cell_text(4U, 0U, "FE-WM-999") ||
        !table.set_cell_text(4U, 1U,
                             utf8_from_u8(u8"\u5b57\u5bbd\u590d\u68c0")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(u8"Wrap close: \u5b57\u5bbd\u590d\u68c0"))) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 460U : 700U,
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
        !closing.add_run("Wrap close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-WM-999 \u5b57\u5bbd\u590d\u68c0"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK font embed wrap mix text";
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

[[nodiscard]] ScenarioResult
build_document_cjk_multi_anchor_table_flow_lite_text_sample(
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
        !title.set_text("Document CJK multi anchor table flow lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Multi anchor flow lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"MA-101 \u8fde\u7eed\u6587\u6863\u6d41"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"FE-MA-202 \u951a\u70b9\u5207\u6362\u77e9\u9635"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto stripe = intro.insert_paragraph_after("");
    if (!stripe.has_next() ||
        !stripe.add_run("Anchor stripe lite: ").has_next() ||
        !add_styled_contract_run(
            document, stripe,
            utf8_from_u8(u8"MA-303 \u6837\u5f0f\u53e0\u52a0\u56de\u8bfb"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    auto table = document.append_table(5U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 1800U) ||
        !table.set_column_width_twips(2U, 3900U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u4e32\u8054\u770b\u677f")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u951a\u70b9")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "MA-A-04") ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"\u7981\u62c6\u5757")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"\u8868\u683c\u6d41\u4fdd\u7559 copy search \u952e FE-MA-404 \u5e76\u9a8c\u8bc1\u957f\u6587\u672c\u6362\u884c")) ||
        !table.set_cell_text(2U, 0U, "MA-B-04") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u5207\u6362\u77e9\u9635")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"\u7b2c\u4e8c\u4e2a\u951a\u70b9\u7ee7\u7eed\u56de\u8bfb ABC 123 \u548c CJK \u5b57\u4f53")) ||
        !table.set_cell_text(3U, 0U, "MA-C-04") ||
        !table.set_cell_text(3U, 1U,
                             utf8_from_u8(u8"\u6837\u5f0f\u590d\u6838")) ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(
                u8"\u5927\u5b57\u3001\u6ce8\u8bb0\u548c accent \u6df7\u6392\u540e\u4ecd\u9700\u53ef\u641c\u7d22")) ||
        !table.set_cell_text(4U, 0U, "FE-MA-999") ||
        !table.set_cell_text(4U, 1U,
                             utf8_from_u8(u8"\u6536\u53e3")) ||
        !table.set_cell_text(4U, 2U,
                             utf8_from_u8(u8"\u56de\u8bfb\u590d\u68c0\u7ed3\u675f"))) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 460U : 700U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 1U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Anchor close lite: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-MA-999 \u56de\u8bfb\u590d\u68c0"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK multi anchor table flow "
        "lite text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Font Embed Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_cjk_multi_anchor_table_flow_text_sample(
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
        !title.set_text("Document CJK multi anchor table flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Multi anchor flow: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"MA-101 \u8fde\u7eed\u6587\u6863\u6d41"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / MA-202 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"FE-MA-901 \u951a\u70b9\u5207\u6362\u77e9\u9635"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    auto &first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text(
            utf8_from_u8(u8"Multi flow header MA-303 page {{page}}")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            utf8_from_u8(u8"Multi flow footer {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(
            utf8_from_u8(u8"Multi first header MA-101 page {{page}}")) ||
        !even_header.has_next() ||
        !even_header.set_text(
            utf8_from_u8(u8"Multi even header MA-202 page {{page}}"))) {
        return sample;
    }

    auto stripe = intro.insert_paragraph_after("");
    if (!stripe.has_next() ||
        !stripe.add_run("Anchor stripe: ").has_next() ||
        !add_styled_contract_run(
            document, stripe,
            utf8_from_u8(u8"MA-303 \u6837\u5f0f\u53e0\u52a0\u56de\u8bfb ABC 123"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    auto table = document.append_table(5U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 1900U) ||
        !table.set_column_width_twips(2U, 3800U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u591a\u951a\u770b\u677f")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u951a\u70b9")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "MA-A-04") ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"\u7981\u62c6\u5757")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"Anchor checkpoint A: FE-MA-951 \u4fdd\u7559\u9996\u4e2a\u8868\u683c\u951a\u70b9\u548c\u957f\u6587\u672c\u6362\u884c\u3002")) ||
        !table.set_cell_text(2U, 0U, "MA-B-04") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u5207\u6362\u77e9\u9635")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"Anchor checkpoint B: FE-MA-971 \u4e0e FE-MA-981 \u7ee7\u7eed\u56de\u8bfb\u3002")) ||
        !table.set_cell_text(3U, 0U, "MA-C-04") ||
        !table.set_cell_text(3U, 1U,
                             utf8_from_u8(u8"\u6837\u5f0f\u590d\u6838")) ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(
                u8"Anchor settle: \u5927\u5b57\u3001\u6ce8\u8bb0\u548c accent \u6df7\u6392\u540e\u4ecd\u9700\u53ef\u641c\u7d22\u3002")) ||
        !table.set_cell_text(4U, 0U, "FE-MA-999") ||
        !table.set_cell_text(4U, 1U, utf8_from_u8(u8"\u6536\u53e3")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(
                u8"Anchor close: \u591a\u951a\u70b9\u8868\u683c\u6d41\u7ed3\u675f\u5e76\u4fdd\u7559\u68c0\u7d22\u952e\u3002"))) {
        return sample;
    }

    auto board_header = table.find_cell(0U, 0U);
    auto anchor_block = table.find_cell(1U, 1U);
    if (!board_header.has_value() || !anchor_block.has_value() ||
        !board_header->merge_right(2U) || !anchor_block->merge_down(1U)) {
        return sample;
    }
    if (!board_header->set_text(utf8_from_u8(u8"\u591a\u951a\u770b\u677f")) ||
        !board_header->set_fill_color("D9EAF7") ||
        !anchor_block->set_fill_color("E8F3E8") ||
        !anchor_block->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 460U : 700U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 1U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Anchor close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-MA-999 \u56de\u8bfb\u590d\u68c0"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK multi anchor table flow "
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
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}
