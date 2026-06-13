[[nodiscard]] ScenarioResult build_document_contract_cjk_style_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica") ||
        !document.set_default_run_east_asia_font_family(
            "Document Contract CJK") ||
        !define_document_contract_styles(document)) {
        return sample;
    }

    const std::vector<std::pair<std::string, std::string>> lines{
        {"SERVICE AGREEMENT", "DocumentPdfContractTitle"},
        {utf8_from_u8(u8"中文合同样本"), "DocumentPdfContractCjkTitle"},
        {"Contract No: FD-2026-0507", "DocumentPdfContractParty"},
        {"Party A: FeatherDoc", "DocumentPdfContractParty"},
        {utf8_from_u8(u8"甲方：羽文档"), "DocumentPdfContractParty"},
        {"Amount: USD 12,000", "DocumentPdfContractAmount"},
        {utf8_from_u8(u8"生效日期：2026年05月06日"),
         "DocumentPdfContractDate"},
        {utf8_from_u8(u8"第一条：服务范围覆盖文档生成 PDF。"),
         "DocumentPdfContractParty"},
        {utf8_from_u8(u8"第二条：双方确认输出可回读、可验证。"),
         "DocumentPdfContractParty"},
        {"Authorized signature", "DocumentPdfContractSignature"},
    };

    auto paragraph = document.paragraphs();
    for (std::size_t index = 0U; index < lines.size(); ++index) {
        if (index != 0U) {
            paragraph = paragraph.insert_paragraph_after("");
        }
        if (!add_styled_contract_run(document, paragraph, lines[index].first,
                                     lines[index].second)) {
            return sample;
        }
    }

    auto &header = document.ensure_section_header_paragraphs(0U);
    auto &footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() || !header.set_text("Contract header") ||
        !footer.has_next() ||
        !footer.set_text("Page {{page}} of {{total_pages}}")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document contract CJK style";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document Contract CJK", font_path},
    };
    options.cjk_font_file_path = font_path;
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 24.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] bool define_document_eastasia_style_probe_styles(
    featherdoc::Document &document) {
    auto latin_emphasis = featherdoc::character_style_definition{};
    latin_emphasis.name = "Document PDF EastAsia Probe Latin Emphasis";
    latin_emphasis.run_font_family = std::string{"Helvetica"};
    latin_emphasis.run_text_color = std::string{"1F4E79"};
    latin_emphasis.run_bold = true;
    latin_emphasis.run_italic = true;
    latin_emphasis.run_font_size_points = 16.0;
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfEastAsiaProbeLatin",
                                        std::move(latin_emphasis))) {
        return false;
    }

    auto east_asia_accent = featherdoc::character_style_definition{};
    east_asia_accent.name = "Document PDF EastAsia Probe Accent";
    east_asia_accent.run_font_family = std::string{"Helvetica"};
    east_asia_accent.run_east_asia_font_family =
        std::string{"Document EastAsia Probe CJK"};
    east_asia_accent.run_text_color = std::string{"C00000"};
    east_asia_accent.run_bold = true;
    east_asia_accent.run_underline = true;
    east_asia_accent.run_font_size_points = 18.0;
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfEastAsiaProbeAccent",
                                        std::move(east_asia_accent))) {
        return false;
    }

    auto east_asia_note = featherdoc::character_style_definition{};
    east_asia_note.name = "Document PDF EastAsia Probe Note";
    east_asia_note.run_font_family = std::string{"Helvetica"};
    east_asia_note.run_east_asia_font_family =
        std::string{"Document EastAsia Probe CJK"};
    east_asia_note.run_text_color = std::string{"336699"};
    east_asia_note.run_italic = true;
    east_asia_note.run_font_size_points = 14.0;
    return ensure_document_contract_style(document,
                                          "DocumentPdfEastAsiaProbeNote",
                                          std::move(east_asia_note));
}

[[nodiscard]] bool define_document_style_gallery_styles(
    featherdoc::Document &document) {
    auto accent = featherdoc::character_style_definition{};
    accent.name = "Document PDF Style Gallery Accent";
    accent.run_font_family = std::string{"Helvetica"};
    accent.run_font_size_points = 16.0;
    accent.run_bold = true;
    accent.run_italic = true;
    accent.run_text_color = std::string{"2F5597"};
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfStyleGalleryAccent",
                                        std::move(accent))) {
        return false;
    }

    auto note = featherdoc::character_style_definition{};
    note.name = "Document PDF Style Gallery Note";
    note.run_font_family = std::string{"Helvetica"};
    note.run_font_size_points = 12.0;
    note.run_underline = true;
    note.run_text_color = std::string{"7A3E00"};
    return ensure_document_contract_style(document,
                                          "DocumentPdfStyleGalleryNote",
                                          std::move(note));
}

[[nodiscard]] bool define_document_rtl_bidi_styles(
    featherdoc::Document &document) {
    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Document PDF RTL Bidi Paragraph";
    paragraph_style.paragraph_bidi = true;
    if (!document.ensure_paragraph_style("DocumentPdfRtlParagraph",
                                         paragraph_style)) {
        return false;
    }

    auto arabic = featherdoc::character_style_definition{};
    arabic.name = "Document PDF RTL Arabic";
    arabic.run_font_family = std::string{"Document RTL Arabic"};
    arabic.run_bidi_language = std::string{"ar-SA"};
    arabic.run_rtl = true;
    arabic.run_font_size_points = 15.0;
    arabic.run_text_color = std::string{"1F3A5F"};
    if (!ensure_document_contract_style(document, "DocumentPdfRtlArabic",
                                        std::move(arabic))) {
        return false;
    }

    auto emphasis = featherdoc::character_style_definition{};
    emphasis.name = "Document PDF RTL Arabic Emphasis";
    emphasis.based_on = std::string{"DocumentPdfRtlArabic"};
    emphasis.run_underline = true;
    emphasis.run_text_color = std::string{"8A2D3B"};
    return ensure_document_contract_style(document,
                                          "DocumentPdfRtlArabicEmphasis",
                                          std::move(emphasis));
}

[[nodiscard]] bool define_document_cjk_copy_search_lite_styles(
    featherdoc::Document &document) {
    auto accent = featherdoc::character_style_definition{};
    accent.name = "Document PDF CJK Copy Search Lite Accent";
    accent.run_font_family = std::string{"Helvetica"};
    accent.run_east_asia_font_family =
        std::string{"Document CJK Copy Search Lite"};
    accent.run_text_color = std::string{"1F4E79"};
    accent.run_bold = true;
    accent.run_font_size_points = 14.0;
    if (!ensure_document_contract_style(
            document, "DocumentPdfCjkCopySearchLiteAccent",
            std::move(accent))) {
        return false;
    }

    auto note = featherdoc::character_style_definition{};
    note.name = "Document PDF CJK Copy Search Lite Note";
    note.run_font_family = std::string{"Helvetica"};
    note.run_east_asia_font_family =
        std::string{"Document CJK Copy Search Lite"};
    note.run_text_color = std::string{"19572E"};
    note.run_italic = true;
    note.run_font_size_points = 12.0;
    return ensure_document_contract_style(
        document, "DocumentPdfCjkCopySearchLiteNote", std::move(note));
}

[[nodiscard]] bool define_document_cjk_font_embed_lite_styles(
    featherdoc::Document &document) {
    auto accent = featherdoc::character_style_definition{};
    accent.name = "Document PDF CJK Font Embed Lite Accent";
    accent.run_font_family = std::string{"Helvetica"};
    accent.run_east_asia_font_family =
        std::string{"Document CJK Font Embed Lite"};
    accent.run_text_color = std::string{"1F4E79"};
    accent.run_bold = true;
    accent.run_underline = true;
    accent.run_font_size_points = 14.0;
    if (!ensure_document_contract_style(
            document, "DocumentPdfCjkFontEmbedLiteAccent",
            std::move(accent))) {
        return false;
    }

    auto large = featherdoc::character_style_definition{};
    large.name = "Document PDF CJK Font Embed Lite Large";
    large.run_font_family = std::string{"Helvetica"};
    large.run_east_asia_font_family =
        std::string{"Document CJK Font Embed Lite"};
    large.run_text_color = std::string{"7A1414"};
    large.run_bold = true;
    large.run_font_size_points = 18.0;
    if (!ensure_document_contract_style(
            document, "DocumentPdfCjkFontEmbedLiteLarge",
            std::move(large))) {
        return false;
    }

    auto note = featherdoc::character_style_definition{};
    note.name = "Document PDF CJK Font Embed Lite Note";
    note.run_font_family = std::string{"Helvetica"};
    note.run_east_asia_font_family =
        std::string{"Document CJK Font Embed Lite"};
    note.run_text_color = std::string{"19572E"};
    note.run_italic = true;
    note.run_font_size_points = 10.5;
    return ensure_document_contract_style(
        document, "DocumentPdfCjkFontEmbedLiteNote", std::move(note));
}

[[nodiscard]] bool define_document_cjk_style_overlay_lite_styles(
    featherdoc::Document &document) {
    if (!define_document_cjk_font_embed_lite_styles(document)) {
        return false;
    }

    auto superscript = featherdoc::character_style_definition{};
    superscript.name = "Document PDF CJK Style Overlay Lite Superscript";
    superscript.run_font_family = std::string{"Helvetica"};
    superscript.run_east_asia_font_family =
        std::string{"Document CJK Font Embed Lite"};
    superscript.run_text_color = std::string{"5B2C6F"};
    superscript.run_font_size_points = 11.5;
    superscript.run_superscript = true;
    if (!ensure_document_contract_style(
            document, "DocumentPdfCjkOverlayLiteSuperscript",
            std::move(superscript))) {
        return false;
    }

    auto subscript = featherdoc::character_style_definition{};
    subscript.name = "Document PDF CJK Style Overlay Lite Subscript";
    subscript.run_font_family = std::string{"Helvetica"};
    subscript.run_east_asia_font_family =
        std::string{"Document CJK Font Embed Lite"};
    subscript.run_text_color = std::string{"0F5C6E"};
    subscript.run_font_size_points = 11.5;
    subscript.run_subscript = true;
    if (!ensure_document_contract_style(
            document, "DocumentPdfCjkOverlayLiteSubscript",
            std::move(subscript))) {
        return false;
    }

    auto strike = featherdoc::character_style_definition{};
    strike.name = "Document PDF CJK Style Overlay Lite Strike";
    strike.run_font_family = std::string{"Helvetica"};
    strike.run_east_asia_font_family =
        std::string{"Document CJK Font Embed Lite"};
    strike.run_text_color = std::string{"6E2C00"};
    strike.run_font_size_points = 12.0;
    strike.run_strikethrough = true;
    return ensure_document_contract_style(
        document, "DocumentPdfCjkOverlayLiteStrike", std::move(strike));
}
