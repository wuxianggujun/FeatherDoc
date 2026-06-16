[[nodiscard]] ScenarioResult build_cjk_image_report_text_sample(
    const std::filesystem::path &font_path,
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: CJK image report text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    const auto image_path = write_tiny_rgb_png(
        asset_dir, "featherdoc-pdf-regression-cjk-image-report.png");

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 724.0, utf8_from_u8(u8"中文图文报告样本"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page1.text_runs.push_back(make_text_run(
        72.0, 698.0, utf8_from_u8(u8"第一页包含图片和中文段落"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));
    page1.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 540.0, 108.0, 108.0},
        image_path,
        "image/png",
        utf8_from_u8(u8"图一"),
        true,
        true,
    });
    page1.text_runs.push_back(make_text_run(
        198.0, 610.0, utf8_from_u8(u8"PDFium 回读中文和图片"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));

    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 724.0, utf8_from_u8(u8"第二页继续图文回读"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page2.text_runs.push_back(make_text_run(
        72.0, 698.0, utf8_from_u8(u8"第二页复用同一张位图"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));
    page2.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 540.0, 108.0, 108.0},
        image_path,
        "image/png",
        utf8_from_u8(u8"图二"),
        true,
        true,
    });
    page2.text_runs.push_back(make_text_run(
        198.0, 610.0, utf8_from_u8(u8"中文图文回归保持稳定"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    return sample;
}

[[nodiscard]] featherdoc::Paragraph append_document_paragraph(
    featherdoc::Document &document,
    std::string_view text) {
    auto paragraph = document.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text});
}

[[nodiscard]] bool append_document_text_paragraph(
    featherdoc::Document &document,
    std::string_view text) {
    return append_document_paragraph(document, text).has_next();
}

[[nodiscard]] bool append_document_list_item(featherdoc::Document &document,
                                             std::string_view text,
                                             featherdoc::list_kind kind,
                                             bool restart) {
    auto paragraph = append_document_paragraph(document, text);
    if (!paragraph.has_next()) {
        return false;
    }

    if (restart) {
        return document.restart_paragraph_list(paragraph, kind);
    }
    return document.set_paragraph_list(paragraph, kind);
}

[[nodiscard]] bool append_document_list_item_at_level(
    featherdoc::Document &document,
    std::string_view text,
    featherdoc::list_kind kind,
    bool restart,
    std::uint32_t level) {
    auto paragraph = append_document_paragraph(document, text);
    if (!paragraph.has_next()) {
        return false;
    }

    if (restart) {
        return document.restart_paragraph_list(paragraph, kind, level);
    }
    return document.set_paragraph_list(paragraph, kind, level);
}

[[nodiscard]] ScenarioResult build_header_footer_rtl_variants_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !define_document_rtl_bidi_styles(document)) {
        return sample;
    }

    auto title = document.paragraphs();
    if (!title.has_next() ||
        !title.set_text("Header footer RTL variants sample")) {
        return sample;
    }

    for (std::size_t index = 1U; index <= 17U; ++index) {
        const auto paragraph_text =
            "Variant body line " + std::to_string(index);
        if (!append_document_text_paragraph(document, paragraph_text)) {
            return sample;
        }
    }

    auto configure_variant_header =
        [&](auto &paragraph, std::string_view label, std::string_view code,
            std::u8string_view suffix) {
            if (!paragraph.has_next() ||
                !document.set_paragraph_style(paragraph,
                                              "DocumentPdfRtlParagraph") ||
                !paragraph.set_bidi(true) ||
                !paragraph.set_alignment(
                    featherdoc::paragraph_alignment::right) ||
                !paragraph.add_run(std::string{label}).has_next()) {
                return false;
            }

            auto prefix = paragraph.add_run(
                utf8_from_u8(u8"\u0627\u0644\u0646\u0633\u062e\u0629"));
            if (!prefix.has_next() ||
                !document.set_run_style(prefix,
                                        "DocumentPdfRtlArabicEmphasis") ||
                !paragraph.add_run(std::string{code}).has_next()) {
                return false;
            }

            auto suffix_run = paragraph.add_run(utf8_from_u8(suffix));
            return suffix_run.has_next() &&
                   document.set_run_style(suffix_run, "DocumentPdfRtlArabic");
        };

    auto configure_variant_footer =
        [&](auto &paragraph, std::string_view label,
            std::u8string_view suffix) {
            if (!paragraph.has_next() ||
                !document.set_paragraph_style(paragraph,
                                              "DocumentPdfRtlParagraph") ||
                !paragraph.set_bidi(true) ||
                !paragraph.set_alignment(
                    featherdoc::paragraph_alignment::right) ||
                !paragraph.add_run(std::string{label}).has_next() ||
                !paragraph.add_run("{{page}}").has_next() ||
                !paragraph.add_run(" / ").has_next() ||
                !paragraph.add_run("{{total_pages}}").has_next() ||
                !paragraph.add_run(" | section ").has_next() ||
                !paragraph.add_run("{{section_page}}").has_next() ||
                !paragraph.add_run(" / ").has_next() ||
                !paragraph.add_run("{{section_total_pages}}").has_next() ||
                !paragraph.add_run(" - ").has_next()) {
                return false;
            }

            auto suffix_run = paragraph.add_run(utf8_from_u8(suffix));
            return suffix_run.has_next() &&
                   document.set_run_style(suffix_run, "DocumentPdfRtlArabic");
        };

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!configure_variant_header(
            default_header, "Default header ", " HF-303 ",
            u8"\u062c\u0627\u0647\u0632\u0629") ||
        !configure_variant_footer(
            default_footer, "Default footer page ",
            u8"\u0646\u0647\u0627\u0626\u064a\u0629") ||
        !configure_variant_header(
            first_header, "First header ", " HF-101 ",
            u8"\u0627\u0644\u0627\u0641\u062a\u062a\u0627\u062d\u064a\u0629") ||
        !configure_variant_footer(
            first_footer, "First footer page ",
            u8"\u0627\u0644\u0623\u0648\u0644\u0649") ||
        !configure_variant_header(
            even_header, "Even header ", " HF-202 ",
            u8"\u0627\u0644\u0632\u0648\u062c\u064a\u0629") ||
        !configure_variant_footer(
            even_footer, "Even footer page ",
            u8"\u0627\u0644\u0645\u0632\u062f\u0648\u062c\u0629")) {
        return sample;
    }

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 4320U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 720U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 240U;
    setup.margins.footer_twips = 240U;
    if (!document.set_section_page_setup(0U, setup)) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title =
        "FeatherDoc regression sample: header footer RTL variants";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document RTL Arabic", font_path},
    };
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.font_size_points = 12.0;
    options.line_height_points = 16.0;
    options.header_footer_font_size_points = 8.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}
