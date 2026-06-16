[[nodiscard]] ScenarioResult build_document_table_cjk_merged_repeat_text_sample(
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
        !title.set_text("Document table CJK merged repeat sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"MR-CJK-101 \u5408\u5e76\u5355\u5143\u683c\u91cd\u590d\u8868\u5934"))) {
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
            utf8_from_u8(u8"CJK merged repeat header MR-CJK-303 page {{page}}")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            utf8_from_u8(
                u8"CJK merged repeat footer {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(
            utf8_from_u8(
                u8"CJK merged repeat first MR-CJK-101 page {{page}}")) ||
        !even_header.has_next() ||
        !even_header.set_text(
            utf8_from_u8(
                u8"CJK merged repeat even MR-CJK-202 page {{page}}")) ||
        !first_footer.has_next() ||
        !first_footer.set_text(utf8_from_u8(
            u8"CJK merged repeat first footer {{page}} / {{total_pages}}")) ||
        !even_footer.has_next() ||
        !even_footer.set_text(utf8_from_u8(
            u8"CJK merged repeat even footer {{page}} / {{total_pages}}"))) {
        return sample;
    }

    auto table = document.append_table(5U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 2300U) ||
        !table.set_column_width_twips(2U, 3400U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u53d1\u5e03\u770b\u677f")) ||
        !table.set_cell_text(1U, 0U, utf8_from_u8(u8"\u6848\u4f8b")) ||
        !table.set_cell_text(1U, 1U, utf8_from_u8(u8"\u8d1f\u8d23\u4eba")) ||
        !table.set_cell_text(1U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(2U, 0U, "MR-CJK-201") ||
        !table.set_cell_text(2U, 1U,
                             utf8_from_u8(u8"\u7edf\u7b79")) ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"\u8d1f\u8d23\u4eba\u5757\u9700\u8981\u548c\u91cd\u590d\u8868\u5934\u4e00\u8d77\u4fdd\u6301\u5bf9\u9f50\u3002")) ||
        !table.set_cell_text(3U, 0U, "MR-CJK-303") ||
        !table.set_cell_text(
            3U, 2U,
            utf8_from_u8(
                u8"\u68c0\u7d22\u5355\u5143\u683c\u8981\u4fdd\u7559 FE-MR-CJK-303 \u548c ABC 123\u3002")) ||
        !table.set_cell_text(4U, 0U, "MR-CJK-999") ||
        !table.set_cell_text(4U, 1U, utf8_from_u8(u8"\u5c3e\u90e8")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(
                u8"\u5408\u5e76\u5c3e\u884c\u5728\u540e\u7eed\u53ef\u89c6\u5316\u9a8c\u8bc1\u4e2d\u5fc5\u987b\u6e05\u6670\u3002"))) {
        return sample;
    }

    auto board_header = table.find_cell(0U, 0U);
    auto owner_block = table.find_cell(2U, 1U);
    auto tail_merge = table.find_cell(4U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_note = table.find_cell(1U, 2U);
    if (!board_header.has_value() || !owner_block.has_value() ||
        !tail_merge.has_value() || !heading_case.has_value() ||
        !heading_owner.has_value() || !heading_note.has_value() ||
        !board_header->merge_right(2U) || !owner_block->merge_down(1U) ||
        !tail_merge->merge_right(1U)) {
        return sample;
    }

    if (!board_header->set_text(utf8_from_u8(u8"\u53d1\u5e03\u770b\u677f")) ||
        !tail_merge->set_text(utf8_from_u8(
            u8"\u5408\u5e76\u5c3e\u884c MR-CJK-999 \u5728\u5206\u9875\u540e\u4ecd\u7136\u6e05\u6670\u3002"))) {
        return sample;
    }

    if (!board_header->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_note->set_fill_color("EAF2F8") ||
        !owner_block->set_fill_color("E8F3E8") ||
        !tail_merge->set_fill_color("F6E7D8") ||
        !owner_block->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center) ||
        !tail_merge->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index < 2U ? 480U : 720U,
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

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document table CJK merged repeat";
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

[[nodiscard]] ScenarioResult build_document_table_cant_split_text_sample() {
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
        !title.set_text("Document table cant split sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "A tall no-split row must move as a single block when the "
            "remaining page space is too small.")) {
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
        !default_header.set_text("Cant split header S-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            "Cant split footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Cant split first S-101 page {{page}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Cant split even S-202 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            "Cant split first footer {{page}} / {{total_pages}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            "Cant split even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 7200U;
    setup.height_twips = 5200U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 720U;
    setup.margins.right_twips = 720U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    constexpr std::size_t row_count = 4U;
    auto table = document.append_table(row_count, 2U);
    if (!table.has_next() || !table.set_width_twips(5520U) ||
        !table.set_column_width_twips(0U, 1320U) ||
        !table.set_column_width_twips(1U, 4200U) ||
        !table.set_cell_text(0U, 0U, "Case") ||
        !table.set_cell_text(0U, 1U, "Notes") ||
        !table.set_cell_text(1U, 0U, "FE-801") ||
        !table.set_cell_text(
            1U, 1U,
            "Lead row leaves just enough space to expose the cant-split "
            "behavior on the next row.") ||
        !table.set_cell_text(2U, 0U, "FE-802") ||
        !table.set_cell_text(
            2U, 1U,
            "This tall row must move as a single block. It should not leave "
            "the first half of the paragraph on one page and the rest on the "
            "next page. Visual checks need to see the entire note start below "
            "the repeated header after the break.") ||
        !table.set_cell_text(3U, 0U, "FE-803") ||
        !table.set_cell_text(
            3U, 1U,
            "Tail row confirms the table continues normally after the large "
            "row moves intact.")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(600U, featherdoc::row_height_rule::at_least)) {
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
    options.page_size = featherdoc::pdf::PdfPageSize{360.0, 250.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table cant split";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}
