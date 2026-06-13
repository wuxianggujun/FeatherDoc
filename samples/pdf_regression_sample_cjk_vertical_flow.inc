[[nodiscard]] ScenarioResult
build_document_cjk_vertical_merge_wrap_cant_split_text_sample(
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
        !title.set_text("Document CJK vertical merge wrap sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Vertical merge wrap: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"\u7eb5\u5411\u5408\u5e76\u8054\u52a8"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / FE-VM-901 / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"\u5c0f\u9875\u9ad8\u5206\u9875"),
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
        !default_header.set_text("Vertical header VM-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            "Vertical footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text(
            "Vertical first header VM-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Vertical first footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text(
            "Vertical even header VM-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            "Vertical even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto header_contract = append_document_paragraph(document, "");
    if (!header_contract.has_next() ||
        !header_contract.add_run("Vertical header contract: ").has_next() ||
        !add_styled_contract_run(
            document, header_contract,
            "Vertical first header VM-101 page 1",
            "DocumentPdfCjkFontEmbedLiteNote") ||
        !header_contract
             .add_run(" / Vertical even header VM-202 page 2 / Vertical "
                      "header VM-303 page 3")
             .has_next()) {
        return sample;
    }

    auto footer_contract = append_document_paragraph(document, "");
    if (!footer_contract.has_next() ||
        !footer_contract.add_run("Vertical footer contract: ").has_next() ||
        !add_styled_contract_run(
            document, footer_contract, "Vertical first footer 1 / 4",
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !footer_contract
             .add_run(" / Vertical even footer 2 / 4 / Vertical footer 4 / 4")
             .has_next()) {
        return sample;
    }

    auto merge_stripe = append_document_paragraph(document, "");
    if (!merge_stripe.has_next() ||
        !merge_stripe.add_run("Merge stripe: ").has_next() ||
        !add_styled_contract_run(
            document, merge_stripe, utf8_from_u8(u8"\u7eb5\u5411\u5757"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !merge_stripe.add_run(" / FE-VM-921 / ").has_next() ||
        !add_styled_contract_run(
            document, merge_stripe,
            utf8_from_u8(u8"\u6574\u5757\u8fc1\u79fb"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"\u7b2c 1 \u7ec4\u7eb5\u5411\u8054\u52a8\u9a8c\u8bc1\uff1a\u7eb5\u5411\u5408\u5e76\u8054\u52a8\u4fdd\u6301\u5c0f\u9875\u9ad8\u5206\u9875\u8bed\u4e49\uff0c\u68c0\u7d22\u952e FE-VM-921 \u548c ABC 123\u3002")) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"\u4ea4\u4ed8\u77e9\u9635\uff1a\u8d1f\u8d23\u4eba CVM-04 \u9700\u8981\u590d\u6838\u6574\u5757\u8fc1\u79fb\u548c\u8868\u683c cant-split\u3002"))) {
        return sample;
    }

    auto checkpoint = append_document_paragraph(document, "");
    if (!checkpoint.has_next() ||
        !checkpoint.add_run("Vertical checkpoint: ").has_next() ||
        !add_styled_contract_run(
            document, checkpoint,
            utf8_from_u8(u8"\u8868\u540e\u56de\u6d41"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !checkpoint.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, checkpoint, utf8_from_u8(u8"\u6574\u5757\u843d\u9875"),
            "DocumentPdfCjkFontEmbedLiteNote") ||
        !checkpoint.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, checkpoint,
            utf8_from_u8(
                u8"\u7b2c 5 \u7ec4\u8868\u540e\u56de\u6d41\u9a8c\u8bc1"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    auto table = document.append_table(6U, 4U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1300U) ||
        !table.set_column_width_twips(1U, 1600U) ||
        !table.set_column_width_twips(2U, 1900U) ||
        !table.set_column_width_twips(3U, 2400U) ||
        !table.set_cell_text(0U, 0U,
                             utf8_from_u8(u8"\u4ea4\u4ed8\u77e9\u9635")) ||
        !table.set_cell_text(0U, 1U,
                             utf8_from_u8(u8"\u8d1f\u8d23\u4eba")) ||
        !table.set_cell_text(0U, 2U,
                             utf8_from_u8(u8"\u5408\u5e76\u5757\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "VM-A-01") ||
        !table.set_cell_text(1U, 1U, "CVM-04") ||
        !table.set_cell_text(1U, 2U,
                             utf8_from_u8(u8"\u7eb5\u5411\u5757")) ||
        !table.set_cell_text(
            1U, 3U,
            utf8_from_u8(
                u8"\u7b2c 1 \u7ec4\u7eb5\u5411\u8054\u52a8\u9a8c\u8bc1")) ||
        !table.set_cell_text(2U, 0U, "FE-VM-921") ||
        !table.set_cell_text(2U, 2U,
                             utf8_from_u8(u8"\u6574\u5757\u8fc1\u79fb")) ||
        !table.set_cell_text(
            2U, 3U,
            utf8_from_u8(
                u8"\u5c0f\u9875\u9ad8\u5206\u9875\u4fdd\u7559 ABC 123")) ||
        !table.set_cell_text(3U, 0U, "VM-CHECK") ||
        !table.set_cell_text(3U, 1U,
                             utf8_from_u8(u8"\u8868\u540e\u56de\u6d41")) ||
        !table.set_cell_text(3U, 2U,
                             utf8_from_u8(u8"\u6574\u5757\u843d\u9875")) ||
        !table.set_cell_text(3U, 3U,
                             utf8_from_u8(
                                 u8"\u7b2c 5 \u7ec4\u8868\u540e\u56de\u6d41\u9a8c\u8bc1")) ||
        !table.set_cell_text(4U, 0U, "VM-CANT") ||
        !table.set_cell_text(4U, 1U,
                             utf8_from_u8(u8"\u8d1f\u8d23\u4eba")) ||
        !table.set_cell_text(4U, 2U,
                             utf8_from_u8(u8"\u6574\u5757\u843d\u9875")) ||
        !table.set_cell_text(
            4U, 3U,
            utf8_from_u8(
                u8"cant-split \u884c\u4fdd\u6301\u8868\u683c\u56de\u6d41\u5b8c\u6574")) ||
        !table.set_cell_text(5U, 0U, "FE-VM-999") ||
        !table.set_cell_text(5U, 1U,
                             utf8_from_u8(u8"\u6574\u5757\u590d\u68c0")) ||
        !table.set_cell_text(5U, 2U, "Vertical close:") ||
        !table.set_cell_text(
            5U, 3U,
            utf8_from_u8(
                u8"\u7eb5\u5411\u5408\u5e76\u8054\u52a8\u6536\u53e3"))) {
        return sample;
    }

    auto merged_header = table.find_cell(0U, 2U);
    auto owner_block = table.find_cell(1U, 1U);
    if (!merged_header.has_value() || !owner_block.has_value() ||
        !merged_header->merge_right(1U) || !owner_block->merge_down(1U) ||
        !owner_block->set_fill_color("E8F3E8") ||
        !owner_block->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 440U : 640U,
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
        !closing.add_run("Vertical close: ").has_next() ||
        !add_styled_contract_run(document, closing, "FE-VM-999",
                                 "DocumentPdfCjkFontEmbedLiteAccent") ||
        !closing.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"\u6574\u5757\u590d\u68c0"),
            "DocumentPdfCjkFontEmbedLiteLarge") ||
        !closing.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"\u8868\u540e\u56de\u6d41"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK vertical merge wrap "
        "cant split text";
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
