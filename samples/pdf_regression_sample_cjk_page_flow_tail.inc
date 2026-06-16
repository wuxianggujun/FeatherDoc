[[nodiscard]] ScenarioResult
build_document_cjk_page_boundary_lite_text_sample(
    const std::filesystem::path &font_path,
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;

    const auto image_path = write_quadrant_rgb_png(
        asset_dir,
        "featherdoc-pdf-regression-document-cjk-page-boundary-lite.png");

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
        !title.set_text("Document CJK page boundary lite sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto intro = title.insert_paragraph_after("");
    if (!intro.has_next() ||
        !intro.add_run("Page boundary lite: ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"PB-101 \u4e34\u754c\u5206\u9875\u8fb9\u754c"),
            "DocumentPdfCjkFontEmbedLiteAccent") ||
        !intro.add_run(" / ").has_next() ||
        !add_styled_contract_run(
            document, intro,
            utf8_from_u8(u8"FE-PB-202 \u9875\u7709\u9875\u811a\u56de\u6d41"),
            "DocumentPdfCjkFontEmbedLiteNote")) {
        return sample;
    }

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    if (!default_header.has_next() ||
        !default_header.set_text("Boundary header PB-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("Boundary footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    if (!append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"PB-404 \u5185\u8054\u56fe\u7247\u524d\u7684 CJK \u6b63\u6587\u4fdd\u7559 copy search")) ||
        !document.append_image(image_path, 72U, 36U) ||
        !append_document_text_paragraph(
            document,
            utf8_from_u8(
                u8"FE-PB-505 \u5185\u8054\u56fe\u7247\u540e\u6bb5\u843d\u7acb\u5373\u6062\u590d\u6574\u680f\u5bbd\u5ea6"))) {
        return sample;
    }

    featherdoc::floating_image_options square_wrap_options;
    square_wrap_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    square_wrap_options.horizontal_offset_px = 0;
    square_wrap_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    square_wrap_options.vertical_offset_px = 0;
    square_wrap_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::square;
    square_wrap_options.wrap_distance_right_px = 12U;
    square_wrap_options.wrap_distance_bottom_px = 8U;
    square_wrap_options.crop =
        featherdoc::floating_image_crop{160U, 0U, 80U, 80U};
    if (!document.append_floating_image(image_path, 96U, 72U,
                                        square_wrap_options)) {
        return sample;
    }

    const std::array<std::string, 4> boundary_paragraphs{
        utf8_from_u8(
            u8"PB-601 \u56fe\u50cf\u73af\u7ed5\u5206\u9875 lite \u6bb5\u843d ABC 123 \u4fdd\u7559\u68c0\u7d22\u952e"),
        utf8_from_u8(
            u8"PB-602 \u56fe\u50cf\u73af\u7ed5\u5206\u9875 lite \u6bb5\u843d ABC 123 \u4fdd\u7559\u68c0\u7d22\u952e"),
        utf8_from_u8(
            u8"PB-603 \u56fe\u50cf\u73af\u7ed5\u5206\u9875 lite \u6bb5\u843d ABC 123 \u4fdd\u7559\u68c0\u7d22\u952e"),
        utf8_from_u8(
            u8"PB-604 \u56fe\u50cf\u73af\u7ed5\u5206\u9875 lite \u6bb5\u843d ABC 123 \u4fdd\u7559\u68c0\u7d22\u952e"),
    };
    for (const auto &text : boundary_paragraphs) {
        if (!append_document_text_paragraph(document, text)) {
            return sample;
        }
    }

    auto closing = append_document_paragraph(document, "");
    if (!closing.has_next() ||
        !closing.add_run("Boundary close lite: ").has_next() ||
        !add_styled_contract_run(
            document, closing,
            utf8_from_u8(u8"FE-PB-999 \u672b\u9875\u6536\u53e3"),
            "DocumentPdfCjkFontEmbedLiteLarge")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document CJK page boundary lite text";
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
    options.render_inline_images = true;
    options.line_height_points = 16.0;
    options.paragraph_spacing_after_points = 4.0;
    options.image_spacing_after_points = 5.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_long_report_text_sample() {
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
        !title.set_text("Long report sample")) {
        return sample;
    }

    const std::array<std::string_view, 3> sections{
        "Background section",
        "Operational findings",
        "Action plan",
    };
    for (const auto section_title : sections) {
        if (!append_document_text_paragraph(document, section_title) ||
            !append_document_text_paragraph(
                document,
                "This paragraph is intentionally verbose so that the document "
                "spills across multiple pages while still staying within the "
                "existing Document to PDF adapter.") ||
            !append_document_text_paragraph(
                document,
                "It exercises wrapped body text, repeated page flow, and the "
                "same PDFium text extraction path used by the real regression "
                "suite.") ||
            !append_document_text_paragraph(
                document,
                "The content remains plain text on purpose so the sample stays "
                "focused on pagination rather than a new rendering primitive.") ||
            !append_document_text_paragraph(
                document,
                "A short closing paragraph for this subsection keeps the page "
                "flow consistent and makes the sample more report-like.")) {
            return sample;
        }
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: long report text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_long_flow_text_sample() {
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
        !title.set_text("Document long flow sample")) {
        return sample;
    }

    auto &header = document.ensure_section_header_paragraphs(0U);
    auto &footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() || !header.set_text("Long flow header") ||
        !footer.has_next() ||
        !footer.set_text("Page {{page}} of {{total_pages}}")) {
        return sample;
    }

    for (int index = 1; index <= 120; ++index) {
        const auto text = "Long flow paragraph " + std::to_string(index) +
                          " keeps pagination and paragraph continuity stable.";
        if (!append_document_text_paragraph(document, text)) {
            return sample;
        }
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document long flow text";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout = featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}
