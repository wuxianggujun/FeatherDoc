[[nodiscard]] ScenarioResult
build_document_cjk_bullet_overlay_page_flow_text_sample(
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
        !title.set_text("Document CJK bullet overlay page flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Bullet overlay flow: ").has_next() ||
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

    auto &header = document.ensure_section_header_paragraphs(0U);
    auto &footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("Bullet overlay header BO-303 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text(
            "Bullet overlay footer BO-777 {{page}} / {{total_pages}}")) {
        return sample;
    }

    const std::array<std::string, 4> bullets{
        utf8_from_u8(u8"BO-777 \u53e0\u52a0\u9879\u76ee\u7b26\u53f7\u68c0\u7d22\u952e"),
        utf8_from_u8(u8"FE-BO-921 \u7b2c 1 \u7ec4 CJK \u9879\u76ee\u7b26\u53f7\u6d41"),
        utf8_from_u8(u8"BO-A-03 \u4e09\u7ea7\u65b9\u5757"),
        utf8_from_u8(u8"FE-BO-999 \u8986\u76d6\u6536\u53e3"),
    };
    for (std::size_t index = 0U; index < bullets.size(); ++index) {
        if (!append_document_list_item_at_level(
                document, bullets[index], featherdoc::list_kind::bullet,
                index == 0U, static_cast<std::uint32_t>(index % 3U))) {
            return sample;
        }
    }

    auto checkpoint = append_document_paragraph(document, "");
    if (!checkpoint.has_next() ||
        !checkpoint.add_run("Bullet checkpoint: ").has_next() ||
        !add_styled_contract_run(
            document, checkpoint,
            utf8_from_u8(u8"BO-777 \u53e0\u52a0\u9879\u76ee\u7b26\u53f7\u68c0\u7d22\u952e"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !checkpoint.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, checkpoint, utf8_from_u8(u8"\u4e0a\u6807\u56de\u8bfb"),
            "DocumentPdfCjkOverlayLiteSuperscript") ||
        !checkpoint.add_run(" + ").has_next() ||
        !add_styled_contract_run(
            document, checkpoint, utf8_from_u8(u8"\u4e0b\u6807\u56de\u8bfb"),
            "DocumentPdfCjkOverlayLiteSubscript") ||
        !checkpoint.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, checkpoint, utf8_from_u8(u8"\u5220\u7ebf\u4fdd\u7559"),
            "DocumentPdfCjkOverlayLiteStrike")) {
        return sample;
    }

    auto table = document.append_table(3U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1600U) ||
        !table.set_column_width_twips(1U, 2200U) ||
        !table.set_column_width_twips(2U, 3400U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u5c42\u7ea7")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "BO-A-03") ||
        !table.set_cell_text(
            1U, 1U,
            utf8_from_u8(
                u8"BO-777 \u53e0\u52a0\u9879\u76ee\u7b26\u53f7\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"\u8868\u683c\u4e2d\u7ee7\u7eed\u4fdd\u7559 bullet \u3001CJK \u548c overlay \u6837\u5f0f")) ||
        !table.set_cell_text(2U, 0U, "FE-BO-999") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u8986\u76d6\u6536\u53e3")) ||
        !table.set_cell_text(2U, 2U,
                             utf8_from_u8(u8"\u53e0\u52a0\u9879\u76ee\u7b26\u53f7\u5951\u7ea6\u7ed3\u675f"))) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK bullet overlay page flow "
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

[[nodiscard]] ScenarioResult build_document_cjk_copy_search_lite_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Copy Search Lite") ||
        !define_document_cjk_copy_search_lite_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK copy search lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Copy search lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"CS-101 \u590d\u5236\u68c0\u7d22\u952e"),
            "DocumentPdfCjkCopySearchLiteAccent") ||
        !intro.add_run(" / FE-CS-LITE-101").has_next()) {
        return sample;
    }

    auto body = intro.insert_paragraph_after("");
    if (!body.has_next() ||
        !body.add_run("Search anchor: ").has_next() ||
        !add_styled_contract_run(
            document, body,
            utf8_from_u8(u8"CS-202 \u4e2d\u82f1\u6df7\u6392 copy search ABC 123"),
            "DocumentPdfCjkCopySearchLiteNote")) {
        return sample;
    }

    auto closing = body.insert_paragraph_after("");
    if (!closing.has_next() ||
        !closing.add_run("Copy search close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"CS-999 \u6587\u672c\u5c42\u7ed3\u675f"),
            "DocumentPdfCjkCopySearchLiteAccent")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK copy search lite text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Copy Search Lite",
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

[[nodiscard]] ScenarioResult build_document_cjk_copy_search_matrix_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document CJK Copy Search Lite") ||
        !define_document_cjk_copy_search_lite_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document CJK copy search matrix sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Copy search matrix: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"CS-101 \u68c0\u7d22\u951a\u70b9\u7532"),
            "DocumentPdfCjkCopySearchLiteAccent") ||
        !intro.add_run(" / FE-CS-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"CS-202 \u9875\u7801\u77e9\u9635 ABC 123"),
            "DocumentPdfCjkCopySearchLiteNote")) {
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
            utf8_from_u8(u8"\u68c0\u7d22\u9ed8\u8ba4\u9875\u7709 CS-303 \u7b2c {{page}} \u9875")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            utf8_from_u8(u8"\u68c0\u7d22\u9ed8\u8ba4\u9875\u811a {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(
            utf8_from_u8(u8"\u68c0\u7d22\u9996\u9875\u9875\u7709 CS-101 \u7b2c {{page}} \u9875")) ||
        !even_header.has_next() ||
        !even_header.set_text(
            utf8_from_u8(u8"\u68c0\u7d22\u5076\u6570\u9875\u7709 CS-202 \u7b2c {{page}} \u9875"))) {
        return sample;
    }

    auto marker = append_document_paragraph(document, "");
    if (!marker.has_next() ||
        !marker.set_alignment(featherdoc::paragraph_alignment::right) ||
        !marker.add_run("Search marker: ").has_next() ||
        !add_styled_contract_run(
            document, marker,
            utf8_from_u8(u8"FE-CS-921 \u4e2d\u6587\u68c0\u7d22\u77e9\u9635"),
            "DocumentPdfCjkCopySearchLiteAccent") ||
        !marker.add_run(" / FE-CS-922").has_next()) {
        return sample;
    }

    for (int index = 1; index <= 4; ++index) {
        const auto paragraph_text =
            utf8_from_u8(u8"\u7b2c ") + std::to_string(index) +
            utf8_from_u8(
                u8" \u7ec4\u590d\u5236\u68c0\u7d22\u9a8c\u8bc1\uff1a\u5408\u540c\u68c0\u7d22\u3001\u9875\u7709\u7d22\u5f15\u3001\u56de\u8bfb\u77e9\u9635\uff0c\u6df7\u6392 English token FE-CS-") +
            std::to_string(930 + index) + utf8_from_u8(u8"\u3002");
        if (!append_document_text_paragraph(document, paragraph_text)) {
            return sample;
        }
    }

    auto table = document.append_table(5U, 2U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 5400U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u533a\u57df")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"\u6b63\u6587")) ||
        !table.set_cell_text(1U, 1U, "FE-CS-931 copy search body") ||
        !table.set_cell_text(2U, 0U, utf8_from_u8(u8"\u9875\u7709")) ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"CS-303 \u9875\u7709\u7d22\u5f15")) ||
        !table.set_cell_text(3U, 0U, utf8_from_u8(u8"\u9875\u811a")) ||
        !table.set_cell_text(3U, 1U, "FE-CS-941 footer search") ||
        !table.set_cell_text(4U, 0U, "CS-999") ||
        !table.set_cell_text(
            4U, 1U,
            utf8_from_u8(
                u8"\u7ec8\u9875\u68c0\u7d22\u548c\u6587\u672c\u5c42\u6536\u53e3"))) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(420U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        row.next();
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Copy search close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-CS-999 \u6587\u672c\u5c42\u7ed3\u675f"),
            "DocumentPdfCjkCopySearchLiteAccent")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK copy search matrix text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document CJK Copy Search Lite",
                                        font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 17.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_cjk_font_embed_lite_text_sample(
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
        !title.set_text("Document CJK font embed lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Font embed lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"FE-101 \u5d4c\u5b57\u77e9\u9635\u7532"),
            "DocumentPdfCjkFontEmbedLiteAccent")) {
        return sample;
    }

    auto metrics = intro.insert_paragraph_after("");
    if (!metrics.has_next() ||
        !metrics.add_run("Metrics marker: ").has_next() ||
        !add_styled_contract_run(
            document, metrics,
            utf8_from_u8(u8"FE-202 \u5b57\u5bbd\u6821\u9a8c copy search"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto scale = metrics.insert_paragraph_after("");
    if (!scale.has_next() ||
        !scale.add_run("Run scale: ").has_next() ||
        !add_styled_contract_run(
            document, scale,
            utf8_from_u8(u8"FE-999 \u7ec8\u9875\u5d4c\u5b57"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK font embed lite text";
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

[[nodiscard]] ScenarioResult build_document_cjk_font_embed_matrix_text_sample(
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
        !title.set_text("Document CJK font embed matrix sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Font embed matrix: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"FM-101 \u5d4c\u5b57\u77e9\u9635\u7532"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-FM-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"FM-202 \u590d\u5236\u641c\u7d22\u8def\u5f84"),
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
        !default_header.set_text("Embed header FM-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Embed footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Embed first header FM-101 page {{page}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Embed even header FM-202 page {{page}}")) {
        return sample;
    }

    auto marker = append_document_paragraph(document, "");
    if (!marker.has_next() ||
        !marker.set_alignment(featherdoc::paragraph_alignment::right) ||
        !marker.add_run("Metrics marker: ").has_next() ||
        !add_styled_contract_run(
            document, marker,
            utf8_from_u8(u8"FE-FM-921 \u5b57\u5bbd\u6821\u9a8c"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !marker.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, marker,
            utf8_from_u8(u8"FE-FM-922 \u641c\u7d22\u56de\u8bfb"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto scale = append_document_paragraph(document, "");
    if (!scale.has_next() || !scale.add_run("Run scale: ").has_next() ||
        !add_styled_contract_run(
            document, scale,
            utf8_from_u8(u8"FE-FM-931 \u5c0f\u5b57\u56de\u8bfb"),
            "DocumentPdfCjkFontEmbedLiteNote") ||
        !scale.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, scale,
            utf8_from_u8(u8"FE-FM-932 \u5408\u540c\u68c0\u7d22"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !scale.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, scale,
            utf8_from_u8(u8"FE-FM-933 \u7ec8\u9875\u5d4c\u5b57 ABC 123"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    auto table = document.append_table(6U, 2U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 5400U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u533a\u57df")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u5d4c\u5b57\u952e")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"\u6b63\u6587")) ||
        !table.set_cell_text(1U, 1U, "FE-FM-941 copy search body") ||
        !table.set_cell_text(2U, 0U, utf8_from_u8(u8"\u9875\u7709")) ||
        !table.set_cell_text(2U, 1U, "FM-303 header index") ||
        !table.set_cell_text(3U, 0U, utf8_from_u8(u8"\u9875\u811a")) ||
        !table.set_cell_text(3U, 1U, "FE-FM-951 footer index") ||
        !table.set_cell_text(4U, 0U, utf8_from_u8(u8"\u7ec8\u9875")) ||
        !table.set_cell_text(4U, 1U,
                             utf8_from_u8(u8"FE-FM-961 \u7ec8\u9875\u641c\u7d22")) ||
        !table.set_cell_text(5U, 0U, utf8_from_u8(u8"\u6837\u5f0f")) ||
        !table.set_cell_text(5U, 1U,
                             utf8_from_u8(u8"FE-FM-971 \u5927\u5b57\u5c0f\u5b57"))) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(420U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        row.next();
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Embed close: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-FM-999 \u5b57\u5bbd\u56de\u8bfb"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK font embed matrix text";
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
    options.line_height_points = 17.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_cjk_anchor_font_matrix_boundary_text_sample(
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
        !title.set_text("Document CJK anchor font matrix boundary sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Anchor font matrix: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"AM-101 \u8de8\u754c\u951a\u70b9"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-AM-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"AM-202 \u5b57\u4f53\u5bc6\u5ea6\u77e9\u9635"),
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
        !default_header.set_text("Anchor matrix header AM-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            "Anchor matrix footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text(
            "Anchor matrix first header AM-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Anchor matrix first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text(
            "Anchor matrix even header AM-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            "Anchor matrix even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto stripe = append_document_paragraph(document, "");
    if (!stripe.has_next() ||
        !stripe.set_alignment(featherdoc::paragraph_alignment::right) ||
        !stripe.add_run("Anchor matrix stripe: ").has_next() ||
        !add_styled_contract_run(
            document, stripe,
            utf8_from_u8(u8"AM-303 \u5b57\u53f7\u8054\u6d4b"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !stripe.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, stripe,
            utf8_from_u8(u8"FE-AM-921 \u68c0\u7d22\u5bc6\u6392 ABC 123"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto ladder = append_document_paragraph(document, "");
    if (!ladder.has_next() || !ladder.add_run("Run ladder: ").has_next() ||
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

    auto table = document.append_table(5U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1600U) ||
        !table.set_column_width_twips(1U, 2200U) ||
        !table.set_column_width_twips(2U, 3400U) ||
        !table.set_cell_text(0U, 0U,
                             utf8_from_u8(u8"\u951a\u70b9\u77e9\u9635\u8868")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"\u8fb9\u754c")) ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"\u68c0\u7d22\u952e")) ||
        !table.set_cell_text(1U, 2U, utf8_from_u8(u8"\u56de\u6d41\u8981\u6c42")) ||
        !table.set_cell_text(2U, 0U, "AM-A-04") ||
        !table.set_cell_text(2U, 1U, "Boundary checkpoint A:") ||
        !table.set_cell_text(2U, 2U,
                             utf8_from_u8(u8"\u8de8\u8868\u5207\u6362")) ||
        !table.set_cell_text(3U, 0U, "AM-B-04") ||
        !table.set_cell_text(3U, 1U, "Boundary checkpoint B:") ||
        !table.set_cell_text(3U, 2U,
                             utf8_from_u8(u8"\u53f3\u4fa7\u6536\u53e3")) ||
        !table.set_cell_text(4U, 0U, "FE-AM-999") ||
        !table.set_cell_text(4U, 1U, utf8_from_u8(u8"\u7ec8\u70b9")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(
                u8"\u6062\u590d\u6574\u680f\u5e76\u4fdd\u7559\u56fe\u7247\u4e0b\u65b9\u5168\u5bbd"))) {
        return sample;
    }

    auto matrix_header = table.find_cell(0U, 0U);
    auto close_cell = table.find_cell(4U, 1U);
    auto heading_boundary = table.find_cell(1U, 0U);
    auto heading_key = table.find_cell(1U, 1U);
    auto heading_flow = table.find_cell(1U, 2U);
    if (!matrix_header.has_value() || !close_cell.has_value() ||
        !heading_boundary.has_value() || !heading_key.has_value() ||
        !heading_flow.has_value() || !matrix_header->merge_right(2U) ||
        !close_cell->merge_right(1U)) {
        return sample;
    }

    if (!matrix_header->set_text(
            utf8_from_u8(u8"\u951a\u70b9\u77e9\u9635\u8868")) ||
        !close_cell->set_text(utf8_from_u8(
            u8"\u6062\u590d\u6574\u680f FE-AM-999 \u56fe\u7247\u4e0b\u65b9\u5168\u5bbd"))) {
        return sample;
    }

    if (!matrix_header->set_fill_color("D9EAF7") ||
        !heading_boundary->set_fill_color("EAF2F8") ||
        !heading_key->set_fill_color("EAF2F8") ||
        !heading_flow->set_fill_color("EAF2F8") ||
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
            utf8_from_u8(
                u8"FE-AM-999 \u6062\u590d\u6574\u680f \u56fe\u7247\u4e0b\u65b9\u5168\u5bbd"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK anchor font matrix "
        "boundary text";
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
    options.line_height_points = 17.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}
