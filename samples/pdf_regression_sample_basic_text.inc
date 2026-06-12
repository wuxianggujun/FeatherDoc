[[nodiscard]] ScenarioResult build_document_strikethrough_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto style_definition = featherdoc::character_style_definition{};
    style_definition.name = "PDF Style Strikethrough";
    style_definition.run_strikethrough = true;
    if (!ensure_document_contract_style(document, "PdfStyleStrikethrough",
                                        std::move(style_definition))) {
        return sample;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("Strikethrough sample") ||
        !paragraph.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() || !paragraph.add_run("Direct ").has_next()) {
        return sample;
    }
    auto direct_strike = paragraph.add_run(
        "strikethrough", featherdoc::formatting_flag::strikethrough);
    if (!direct_strike.has_next() ||
        !paragraph.add_run(" demonstrates run formatting.").has_next()) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() || !paragraph.add_run("Style ").has_next()) {
        return sample;
    }
    auto styled_strike = paragraph.add_run("strikethrough");
    if (!styled_strike.has_next() ||
        !document.set_run_style(styled_strike, "PdfStyleStrikethrough") ||
        !paragraph
             .add_run(" demonstrates inherited style formatting.")
             .has_next()) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title = "FeatherDoc regression sample: strikethrough";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 40.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 10.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_superscript_subscript_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("Superscript and subscript sample") ||
        !paragraph.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() || !paragraph.add_run("E = mc").has_next()) {
        return sample;
    }
    auto superscript_run =
        paragraph.add_run("2", featherdoc::formatting_flag::superscript);
    if (!superscript_run.has_next() ||
        !paragraph.add_run(" demonstrates superscript support.").has_next()) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() || !paragraph.add_run("H").has_next()) {
        return sample;
    }
    auto subscript_run =
        paragraph.add_run("2", featherdoc::formatting_flag::subscript);
    if (!subscript_run.has_next() ||
        !paragraph.add_run("O demonstrates subscript support.").has_next()) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title =
        "FeatherDoc regression sample: superscript subscript";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 40.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 10.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult
build_document_style_superscript_subscript_text_sample() {
    ScenarioResult sample;

    featherdoc::Document document;
    if (document.create_empty()) {
        return sample;
    }
    if (!document.set_default_run_font_family("Helvetica")) {
        return sample;
    }

    auto superscript_style = featherdoc::character_style_definition{};
    superscript_style.name = "PDF Style Superscript";
    superscript_style.run_superscript = true;
    if (!ensure_document_contract_style(document, "PdfStyleSuperscript",
                                        std::move(superscript_style))) {
        return sample;
    }

    auto subscript_style = featherdoc::character_style_definition{};
    subscript_style.name = "PDF Style Subscript";
    subscript_style.run_subscript = true;
    if (!ensure_document_contract_style(document, "PdfStyleSubscript",
                                        std::move(subscript_style))) {
        return sample;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("Style superscript and subscript sample") ||
        !paragraph.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() ||
        !paragraph.add_run("Style E = mc").has_next()) {
        return sample;
    }
    auto superscript_run = paragraph.add_run("2");
    if (!superscript_run.has_next() ||
        !document.set_run_style(superscript_run, "PdfStyleSuperscript") ||
        !paragraph.add_run(" inherits superscript style.").has_next()) {
        return sample;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next() || !paragraph.add_run("Style H").has_next()) {
        return sample;
    }
    auto subscript_run = paragraph.add_run("2");
    if (!subscript_run.has_next() ||
        !document.set_run_style(subscript_run, "PdfStyleSubscript") ||
        !paragraph.add_run("O inherits subscript style.").has_next()) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title =
        "FeatherDoc regression sample: style superscript subscript";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 40.0;
    options.margin_bottom_points = 40.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 10.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_single_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: single text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "FeatherDoc PDF regression sample", 18.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 690.0, "Single-page text smoke", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_multi_page_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: multi page";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    featherdoc::pdf::PdfPageLayout page1;
    page1.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page1.text_runs.push_back(make_text_run(
        72.0, 720.0, "Page 1 regression sample", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page1.text_runs.push_back(make_text_run(
        72.0, 694.0, "Paragraph flow continues across pages", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        false));

    featherdoc::pdf::PdfPageLayout page2;
    page2.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page2.text_runs.push_back(make_text_run(
        72.0, 720.0, "Page 2 regression sample", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page2.text_runs.push_back(make_text_run(
        72.0, 694.0, "Second page keeps the text path alive", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.12, 0.08}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    return sample;
}

[[nodiscard]] ScenarioResult build_cjk_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: CJK text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, utf8_from_u8(u8"中文文本路径回归样本"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 690.0, utf8_from_u8(u8"PDFium 回读"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_mixed_cjk_punctuation_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: mixed CJK punctuation text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, utf8_from_u8(u8"中英混排标点样本"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 692.0, utf8_from_u8(u8"FeatherDoc，PDF；office 123。"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 666.0, utf8_from_u8(u8"括号（中文）与引号“mixed”。"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.12, 0.08}, false, false, false,
        true, "CJK Regression Font", font_path));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_cjk_report_text_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: CJK report text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 720.0, utf8_from_u8(u8"中文报告样本"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page1.text_runs.push_back(make_text_run(
        72.0, 694.0, utf8_from_u8(u8"第一页正文保持可读"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));
    page1.text_runs.push_back(make_text_run(
        72.0, 668.0, utf8_from_u8(u8"PDFium 回读中文文本"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));

    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 720.0, utf8_from_u8(u8"第二页中文标题"), 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Regression Font", font_path));
    page2.text_runs.push_back(make_text_run(
        72.0, 694.0, utf8_from_u8(u8"第二页继续验证分页"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));
    page2.text_runs.push_back(make_text_run(
        72.0, 668.0, utf8_from_u8(u8"中文报告样本结束"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Regression Font", font_path));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    return sample;
}

[[nodiscard]] ScenarioResult build_styled_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: styled text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Small blue text", 10.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 695.0, "Large red text", 18.0,
        featherdoc::pdf::PdfRgbColor{0.78, 0.16, 0.16}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 664.0, "Bold italic text", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, true, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 638.0, "Underlined text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.1, 0.1, 0.1}, false, false, true,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_font_size_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: font size";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Tiny text 8pt", 8.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.18, 0.18}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 698.0, "Body text 12pt", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 666.0, "Display text 20pt", 20.0,
        featherdoc::pdf::PdfRgbColor{0.42, 0.08, 0.08}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_color_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: color text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Black text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 694.0, "Blue text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 668.0, "Green text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.10, 0.50, 0.20}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 642.0, "Red text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.78, 0.16, 0.16}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_mixed_style_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: mixed style";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Plain text", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.08, 0.08}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 696.0, "Bold text", 11.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 672.0, "Italic text", 11.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, true, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 648.0, "Bold italic underline", 11.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, true, true,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_contract_cjk_style_sample(
    const std::filesystem::path &font_path) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: contract CJK style";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "SERVICE AGREEMENT", 20.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 688.0, utf8_from_u8(u8"中文合同样本"), 18.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        true, "CJK Contract Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 650.0, "Party A: FeatherDoc", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 626.0, utf8_from_u8(u8"甲方：羽文档"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        true, "CJK Contract Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 596.0, "Amount: USD 12,000", 14.0,
        featherdoc::pdf::PdfRgbColor{0.48, 0.08, 0.08}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 568.0, utf8_from_u8(u8"生效日期：2026年05月06日"), 12.0,
        featherdoc::pdf::PdfRgbColor{0.10, 0.34, 0.18}, false, false, false,
        true, "CJK Contract Font", font_path));
    page.text_runs.push_back(make_text_run(
        72.0, 532.0, "Authorized signature", 12.0,
        featherdoc::pdf::PdfRgbColor{0.05, 0.05, 0.05}, false, true, true,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] bool ensure_document_contract_style(
    featherdoc::Document &document,
    std::string_view style_id,
    featherdoc::character_style_definition definition) {
    return document.ensure_character_style(style_id, definition) &&
           document.materialize_style_run_properties(style_id);
}

[[nodiscard]] bool add_styled_contract_run(featherdoc::Document &document,
                                           featherdoc::Paragraph &paragraph,
                                           const std::string &text,
                                           std::string_view style_id) {
    if (!paragraph.has_next()) {
        return false;
    }

    auto run = paragraph.add_run(text);
    return run.has_next() && document.set_run_style(run, style_id);
}

[[nodiscard]] bool define_document_contract_styles(
    featherdoc::Document &document) {
    auto title = featherdoc::character_style_definition{};
    title.name = "Document PDF Contract Title";
    title.run_font_family = std::string{"Helvetica"};
    title.run_font_size_points = 20.0;
    title.run_bold = true;
    if (!ensure_document_contract_style(document, "DocumentPdfContractTitle",
                                        std::move(title))) {
        return false;
    }

    auto cjk_title = featherdoc::character_style_definition{};
    cjk_title.name = "Document PDF Contract CJK Title";
    cjk_title.run_east_asia_font_family =
        std::string{"Document Contract CJK"};
    cjk_title.run_font_size_points = 18.0;
    cjk_title.run_text_color = std::string{"142E61"};
    if (!ensure_document_contract_style(document, "DocumentPdfContractCjkTitle",
                                        std::move(cjk_title))) {
        return false;
    }

    auto party = featherdoc::character_style_definition{};
    party.name = "Document PDF Contract Party";
    party.run_font_family = std::string{"Helvetica"};
    party.run_east_asia_font_family = std::string{"Document Contract CJK"};
    party.run_font_size_points = 12.0;
    party.run_bold = true;
    if (!ensure_document_contract_style(document, "DocumentPdfContractParty",
                                        std::move(party))) {
        return false;
    }

    auto amount = featherdoc::character_style_definition{};
    amount.name = "Document PDF Contract Amount";
    amount.run_font_family = std::string{"Helvetica"};
    amount.run_font_size_points = 14.0;
    amount.run_text_color = std::string{"7A1414"};
    amount.run_bold = true;
    if (!ensure_document_contract_style(document, "DocumentPdfContractAmount",
                                        std::move(amount))) {
        return false;
    }

    auto date = featherdoc::character_style_definition{};
    date.name = "Document PDF Contract Date";
    date.run_east_asia_font_family = std::string{"Document Contract CJK"};
    date.run_font_size_points = 12.0;
    date.run_text_color = std::string{"19572E"};
    if (!ensure_document_contract_style(document, "DocumentPdfContractDate",
                                        std::move(date))) {
        return false;
    }

    auto signature = featherdoc::character_style_definition{};
    signature.name = "Document PDF Contract Signature";
    signature.run_font_family = std::string{"Helvetica"};
    signature.run_font_size_points = 12.0;
    signature.run_italic = true;
    signature.run_underline = true;
    if (!ensure_document_contract_style(document,
                                        "DocumentPdfContractSignature",
                                        std::move(signature))) {
        return false;
    }

    return true;
}

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
