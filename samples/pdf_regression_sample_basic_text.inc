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
