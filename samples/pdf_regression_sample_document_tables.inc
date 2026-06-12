[[nodiscard]] bool append_document_text_paragraph(
    featherdoc::Document &document,
    std::string_view text);

[[nodiscard]] featherdoc::Paragraph append_document_paragraph(
    featherdoc::Document &document,
    std::string_view text);

[[nodiscard]] ScenarioResult build_document_image_semantics_text_sample(
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;

    const auto image_path = write_quadrant_rgb_png(
        asset_dir, "featherdoc-pdf-regression-document-image-semantics.png");

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Document image semantics sample") ||
        !append_document_text_paragraph(
            document,
            "Inline image intro keeps body flow above the image block.") ||
        !document.append_image(image_path, 64U, 32U) ||
        !append_document_text_paragraph(
            document,
            "Text after inline image remains below the image block.")) {
        return sample;
    }

    featherdoc::floating_image_options square_wrap_options;
    square_wrap_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    square_wrap_options.horizontal_offset_px = 0;
    square_wrap_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    square_wrap_options.vertical_offset_px = 0;
    square_wrap_options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    square_wrap_options.wrap_distance_right_px = 16U;
    square_wrap_options.crop = featherdoc::floating_image_crop{250U, 0U, 0U, 0U};
    if (!document.append_floating_image(image_path, 96U, 72U,
                                        square_wrap_options) ||
        !append_document_text_paragraph(
            document,
            "Square wrap text flows beside the anchored image before returning "
            "to the margin.")) {
        return sample;
    }

    featherdoc::floating_image_options behind_text_options;
    behind_text_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    behind_text_options.horizontal_offset_px = 420;
    behind_text_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    behind_text_options.vertical_offset_px = 0;
    behind_text_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    behind_text_options.behind_text = true;
    if (!document.append_floating_image(image_path, 96U, 48U,
                                        behind_text_options) ||
        !append_document_text_paragraph(
            document,
            "Behind text image anchor paragraph stays readable.")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document image semantics text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;
    options.image_spacing_after_points = 6.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_semantics_text_sample() {
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
        !title.set_text("Document table semantics sample")) {
        return sample;
    }

    auto table = document.append_table(6U, 2U);
    if (!table.has_next() ||
        !table.set_width_twips(2880U) ||
        !table.set_column_width_twips(0U, 1440U) ||
        !table.set_column_width_twips(1U, 1440U) ||
        !table.set_cell_text(0U, 0U, "Header") ||
        !table.set_cell_text(0U, 1U, "Amount")) {
        return sample;
    }
    for (std::size_t row_index = 1U; row_index < 6U; ++row_index) {
        if (!table.set_cell_text(row_index, 0U,
                                 "Row " + std::to_string(row_index)) ||
            !table.set_cell_text(row_index, 1U,
                                 std::to_string(row_index * 10U))) {
            return sample;
        }
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(720U, featherdoc::row_height_rule::exact)) {
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

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table semantics text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_invoice_table_text_sample() {
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
        !title.set_text("Document invoice table sample") ||
        !append_document_text_paragraph(document,
                                        "Invoice No: INV-2026-0507") ||
        !append_document_text_paragraph(document,
                                        "Bill to: FeatherDoc QA")) {
        return sample;
    }

    auto table = document.append_table(5U, 4U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 3000U) ||
        !table.set_column_width_twips(1U, 720U) ||
        !table.set_column_width_twips(2U, 1800U) ||
        !table.set_column_width_twips(3U, 1680U) ||
        !table.set_cell_text(0U, 0U, "Item") ||
        !table.set_cell_text(0U, 1U, "Qty") ||
        !table.set_cell_text(0U, 2U, "Unit") ||
        !table.set_cell_text(0U, 3U, "Total") ||
        !table.set_cell_text(1U, 0U, "PDF export design") ||
        !table.set_cell_text(1U, 1U, "2") ||
        !table.set_cell_text(1U, 2U, "USD 50") ||
        !table.set_cell_text(1U, 3U, "USD 100") ||
        !table.set_cell_text(2U, 0U, "Visual validation") ||
        !table.set_cell_text(2U, 1U, "1") ||
        !table.set_cell_text(2U, 2U, "USD 25") ||
        !table.set_cell_text(2U, 3U, "USD 25") ||
        !table.set_cell_text(3U, 0U, "Regression evidence") ||
        !table.set_cell_text(3U, 1U, "1") ||
        !table.set_cell_text(3U, 2U, "USD 10") ||
        !table.set_cell_text(3U, 3U, "USD 10") ||
        !table.set_cell_text(4U, 0U, "Grand total") ||
        !table.set_cell_text(4U, 3U, "USD 135")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 5U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(360U, featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document invoice table text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_table_header_footer_variants_text_sample() {
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
        !title.set_text("Document table header footer variants sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Paged table keeps header footer variants visible across table "
            "pagination.")) {
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
        !default_header.set_text("Variant header HF-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Default footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("First variant HF-101 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("First footer {{page}} / {{total_pages}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Even variant HF-202 page {{page}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("Even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 6000U;
    setup.height_twips = 4400U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 720U;
    setup.margins.right_twips = 720U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    constexpr std::size_t row_count = 8U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(4560U) ||
        !table.set_column_width_twips(0U, 900U) ||
        !table.set_column_width_twips(1U, 1140U) ||
        !table.set_column_width_twips(2U, 2520U) ||
        !table.set_cell_text(0U, 0U, "Item") ||
        !table.set_cell_text(0U, 1U, "Batch") ||
        !table.set_cell_text(0U, 2U, "Status") ||
        !table.set_cell_text(1U, 0U, "FE-501") ||
        !table.set_cell_text(1U, 1U, "Alpha") ||
        !table.set_cell_text(1U, 2U, "Draft review") ||
        !table.set_cell_text(2U, 0U, "FE-502") ||
        !table.set_cell_text(2U, 1U, "Beta") ||
        !table.set_cell_text(2U, 2U, "Header sync") ||
        !table.set_cell_text(3U, 0U, "FE-503") ||
        !table.set_cell_text(3U, 1U, "Gamma") ||
        !table.set_cell_text(3U, 2U, "Footer sync") ||
        !table.set_cell_text(4U, 0U, "FE-504") ||
        !table.set_cell_text(4U, 1U, "Delta") ||
        !table.set_cell_text(4U, 2U, "Repeat header") ||
        !table.set_cell_text(5U, 0U, "FE-505") ||
        !table.set_cell_text(5U, 1U, "Epsilon") ||
        !table.set_cell_text(5U, 2U, "Stable page flow") ||
        !table.set_cell_text(6U, 0U, "FE-506") ||
        !table.set_cell_text(6U, 1U, "Zeta") ||
        !table.set_cell_text(6U, 2U, "Visual gate") ||
        !table.set_cell_text(7U, 0U, "FE-507") ||
        !table.set_cell_text(7U, 1U, "Omega") ||
        !table.set_cell_text(7U, 2U, "Release ready")) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(600U, featherdoc::row_height_rule::exact)) {
            return sample;
        }
        if (row_index == 0U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 5U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table header footer variants";
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

[[nodiscard]] ScenarioResult build_document_table_wrap_flow_text_sample() {
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
        !title.set_text("Document table wrap flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Long table cells must wrap cleanly while page headers, repeated "
            "table headers, and footers stay stable.")) {
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
        !default_header.set_text("Wrap header W-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Wrap footer {{page}} / {{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text("Wrap first W-101 page {{page}}") ||
        !even_header.has_next() ||
        !even_header.set_text("Wrap even W-202 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("Wrap first footer {{page}} / {{total_pages}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("Wrap even footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 6000U;
    setup.height_twips = 4400U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 720U;
    setup.margins.right_twips = 720U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    constexpr std::size_t row_count = 6U;
    auto table = document.append_table(row_count, 2U);
    if (!table.has_next() || !table.set_width_twips(6960U) ||
        !table.set_column_width_twips(0U, 1440U) ||
        !table.set_column_width_twips(1U, 5520U) ||
        !table.set_cell_text(0U, 0U, "Case") ||
        !table.set_cell_text(0U, 1U, "Notes") ||
        !table.set_cell_text(1U, 0U, "FE-601") ||
        !table.set_cell_text(
            1U, 1U,
            "Header footer placeholders must stay visible while the first "
            "note wraps.") ||
        !table.set_cell_text(2U, 0U, "FE-602") ||
        !table.set_cell_text(
            2U, 1U,
            "Repeated table headers must stay aligned after page breaks.") ||
        !table.set_cell_text(3U, 0U, "FE-603") ||
        !table.set_cell_text(
            3U, 1U,
            "Visual checks should catch clipped status text before automated "
            "tests pass.") ||
        !table.set_cell_text(4U, 0U, "FE-604") ||
        !table.set_cell_text(
            4U, 1U,
            "Long note blocks need predictable wrapping so screenshots stay "
            "easy to compare.") ||
        !table.set_cell_text(5U, 0U, "FE-605") ||
        !table.set_cell_text(
            5U, 1U,
            "Release ready evidence should move intact instead of breaking "
            "between pages.")) {
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
        if (row_index == 5U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{420.0, 320.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table wrap flow";
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

[[nodiscard]] ScenarioResult build_document_table_cjk_wrap_flow_text_sample(
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
        !title.set_text("Document table CJK wrap flow sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"CW-101 \u4e2d\u6587\u8868\u683c\u6362\u884c\u6d41"))) {
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
            utf8_from_u8(u8"CJK wrap header CW-303 page {{page}}")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            utf8_from_u8(u8"CJK wrap footer {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(
            utf8_from_u8(u8"CJK wrap first CW-101 page {{page}}")) ||
        !even_header.has_next() ||
        !even_header.set_text(
            utf8_from_u8(u8"CJK wrap even CW-202 page {{page}}")) ||
        !first_footer.has_next() ||
        !first_footer.set_text(
            utf8_from_u8(u8"CJK wrap first footer {{page}} / {{total_pages}}")) ||
        !even_footer.has_next() ||
        !even_footer.set_text(
            utf8_from_u8(u8"CJK wrap even footer {{page}} / {{total_pages}}"))) {
        return sample;
    }

    auto table = document.append_table(5U, 2U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 5700U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u6848\u4f8b")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "CW-201") ||
        !table.set_cell_text(
            1U, 1U,
            utf8_from_u8(
                u8"\u9875\u7709\u9875\u811a\u5360\u4f4d\u7b26\u9700\u8981\u5728\u4e2d\u6587\u8868\u683c\u6362\u884c\u540e\u4fdd\u6301\u53ef\u89c1\u3002")) ||
        !table.set_cell_text(2U, 0U, "CW-202") ||
        !table.set_cell_text(
            2U, 1U,
            utf8_from_u8(
                u8"\u91cd\u590d\u8868\u5934\u548c\u7f51\u683c\u5bf9\u9f50\u662f\u540e\u7eed\u53ef\u89c6\u5316\u9a8c\u8bc1\u7684\u5173\u952e\u3002")) ||
        !table.set_cell_text(3U, 0U, "CW-303") ||
        !table.set_cell_text(
            3U, 1U,
            utf8_from_u8(
                u8"\u957f\u6587\u672c\u5355\u5143\u683c\u8981\u4fdd\u7559\u7a33\u5b9a\u68c0\u7d22\u952e FE-CW-303 \u548c ABC 123\u3002")) ||
        !table.set_cell_text(4U, 0U, "CW-999") ||
        !table.set_cell_text(
            4U, 1U,
            utf8_from_u8(
                u8"\u6536\u53e3\u884c\u4f7f\u7528 cant-split \u9632\u6b62\u5c0f\u6279\u91cf\u642c\u5165\u540e\u9000\u5316\u3002"))) {
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

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document table CJK wrap flow";
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
build_document_table_cjk_vertical_merged_cant_split_text_sample(
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
        !title.set_text(
            "Document table CJK vertical merged cant split sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"CVC-101 \u7eb5\u5411\u5408\u5e76\u8868\u683c\u4e0d\u62c6\u5206"))) {
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
            utf8_from_u8(
                u8"CJK vertical merge header CVC-303 page {{page}}")) ||
        !default_footer.has_next() ||
        !default_footer.set_text(
            utf8_from_u8(
                u8"CJK vertical merge footer {{page}} / {{total_pages}}")) ||
        !first_header.has_next() ||
        !first_header.set_text(
            utf8_from_u8(
                u8"CJK vertical merge first CVC-101 page {{page}}")) ||
        !even_header.has_next() ||
        !even_header.set_text(
            utf8_from_u8(
                u8"CJK vertical merge even CVC-202 page {{page}}")) ||
        !first_footer.has_next() ||
        !first_footer.set_text(utf8_from_u8(
            u8"CJK vertical merge first footer {{page}} / {{total_pages}}")) ||
        !even_footer.has_next() ||
        !even_footer.set_text(utf8_from_u8(
            u8"CJK vertical merge even footer {{page}} / {{total_pages}}"))) {
        return sample;
    }

    auto table = document.append_table(5U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 2200U) ||
        !table.set_column_width_twips(2U, 3500U) ||
        !table.set_cell_text(0U, 0U, utf8_from_u8(u8"\u9636\u6bb5")) ||
        !table.set_cell_text(0U, 1U, utf8_from_u8(u8"\u8d1f\u8d23")) ||
        !table.set_cell_text(0U, 2U, utf8_from_u8(u8"\u8bf4\u660e")) ||
        !table.set_cell_text(1U, 0U, "CVC-202") ||
        !table.set_cell_text(1U, 1U,
                             utf8_from_u8(u8"\u7edf\u7b79\u5757")) ||
        !table.set_cell_text(
            1U, 2U,
            utf8_from_u8(
                u8"\u7eb5\u5411\u5408\u5e76\u5757\u9700\u8981\u548c cant-split \u884c\u4e00\u8d77\u4fdd\u6301\u53ef\u8bfb\u3002")) ||
        !table.set_cell_text(2U, 0U, "CVC-303") ||
        !table.set_cell_text(
            2U, 2U,
            utf8_from_u8(
                u8"\u7ffb\u9875\u56de\u6d41\u65f6\u4e0d\u80fd\u7834\u574f\u5408\u5e76\u5355\u5143\u683c\u8fb9\u754c\u3002")) ||
        !table.set_cell_text(3U, 0U, "FE-CVC-404") ||
        !table.set_cell_text(3U, 1U, utf8_from_u8(u8"\u9a8c\u6536")) ||
        !table.set_cell_text(4U, 0U, "CVC-999") ||
        !table.set_cell_text(4U, 1U, utf8_from_u8(u8"\u6536\u53e3")) ||
        !table.set_cell_text(
            4U, 2U,
            utf8_from_u8(
                u8"\u6536\u53e3\u884c\u4fdd\u7559 ABC 123 \u68c0\u7d22\u952e\u5e76\u7981\u6b62\u62c6\u5206\u3002"))) {
        return sample;
    }

    auto owner_block = table.find_cell(1U, 1U);
    auto merged_note = table.find_cell(3U, 1U);
    auto heading_phase = table.find_cell(0U, 0U);
    auto heading_owner = table.find_cell(0U, 1U);
    auto heading_note = table.find_cell(0U, 2U);
    if (!owner_block.has_value() || !merged_note.has_value() ||
        !heading_phase.has_value() || !heading_owner.has_value() ||
        !heading_note.has_value() || !owner_block->merge_down(1U) ||
        !merged_note->merge_right(1U)) {
        return sample;
    }

    if (!merged_note->set_text(utf8_from_u8(
            u8"\u6a2a\u5411\u5408\u5e76\u884c FE-CVC-404 \u9700\u8981\u4fdd\u7559\u6574\u6bb5\u4e2d\u6587\u548c ABC 123\u3002"))) {
        return sample;
    }

    if (!heading_phase->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_note->set_fill_color("EAF2F8") ||
        !owner_block->set_fill_color("E8F3E8") ||
        !merged_note->set_fill_color("F6E7D8") ||
        !owner_block->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center) ||
        !merged_note->set_vertical_alignment(
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
        if ((row_index == 1U || row_index == 4U) && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document table CJK vertical merged "
        "cant split";
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
