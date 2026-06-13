[[nodiscard]] ScenarioResult build_three_page_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: three page";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 720.0, "Three page sample page 1", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page1.text_runs.push_back(make_text_run(
        72.0, 694.0, "First page keeps text flowing", 12.0,
        featherdoc::pdf::PdfRgbColor{0.16, 0.16, 0.4}, false, false, false,
        false));

    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 720.0, "Three page sample page 2", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page2.text_runs.push_back(make_text_run(
        72.0, 694.0, "Second page keeps text flowing", 12.0,
        featherdoc::pdf::PdfRgbColor{0.16, 0.16, 0.4}, false, false, false,
        false));

    auto page3 = make_letter_page();
    page3.text_runs.push_back(make_text_run(
        72.0, 720.0, "Three page sample page 3", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page3.text_runs.push_back(make_text_run(
        72.0, 694.0, "Third page closes the smoke path", 12.0,
        featherdoc::pdf::PdfRgbColor{0.16, 0.16, 0.4}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    sample.layout.pages.push_back(std::move(page3));
    return sample;
}

[[nodiscard]] ScenarioResult build_landscape_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: landscape text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_landscape_page();
    page.text_runs.push_back(make_text_run(
        72.0, 540.0, "Landscape text regression sample", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 512.0, "Wide page keeps the text path visible", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.24, 0.44}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 484.0, "PDFium still extracts landscape text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.10, 0.10}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_title_body_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: title body text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Title line", 24.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 694.0, "Subtitle line", 14.0,
        featherdoc::pdf::PdfRgbColor{0.15, 0.15, 0.15}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 666.0, "Body paragraph line one", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 646.0, "Body paragraph line two", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 620.0, "Footer line", 9.0,
        featherdoc::pdf::PdfRgbColor{0.22, 0.22, 0.22}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_dense_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: dense text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Dense line 1", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 704.0, "Dense line 2", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.12, 0.36}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 684.0, "Dense line 3", 12.0,
        featherdoc::pdf::PdfRgbColor{0.36, 0.12, 0.12}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 664.0, "Dense line 4", 12.0,
        featherdoc::pdf::PdfRgbColor{0.10, 0.40, 0.10}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 644.0, "Dense line 5", 12.0,
        featherdoc::pdf::PdfRgbColor{0.42, 0.18, 0.08}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 624.0, "Dense line 6", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.24, 0.42}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_four_page_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: four page";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 720.0, "Four page sample page 1", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 720.0, "Four page sample page 2", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    auto page3 = make_letter_page();
    page3.text_runs.push_back(make_text_run(
        72.0, 720.0, "Four page sample page 3", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    auto page4 = make_letter_page();
    page4.text_runs.push_back(make_text_run(
        72.0, 720.0, "Four page sample page 4", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    sample.layout.pages.push_back(std::move(page3));
    sample.layout.pages.push_back(std::move(page4));
    return sample;
}

[[nodiscard]] ScenarioResult build_underline_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: underline text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Underlined line 1", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, true,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 694.0, "Underlined line 2", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78}, false, false, true,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 668.0, "Bold underlined line", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.08, 0.08}, true, false, true,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 642.0, "Italic underlined line", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.10, 0.10}, false, true, true,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_punctuation_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: punctuation text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Alpha, beta, gamma.", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 694.0, "Numbers: 12345", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 668.0, "Symbols: ( ) [ ] / -", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.18, 0.18}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 642.0, "Quote FeatherDoc", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_latin_ligature_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: Latin ligature text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "office affinity flow", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 692.0, "file fixture flings", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 666.0, "efficient workflow", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.12, 0.08}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_two_page_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title = "FeatherDoc regression sample: two page";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 720.0, "Two page sample page 1", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page1.text_runs.push_back(make_text_run(
        72.0, 694.0, "First page keeps extracted text stable", 12.0,
        featherdoc::pdf::PdfRgbColor{0.16, 0.16, 0.4}, false, false, false,
        false));

    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 720.0, "Two page sample page 2", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page2.text_runs.push_back(make_text_run(
        72.0, 694.0, "Second page keeps extracted text stable", 12.0,
        featherdoc::pdf::PdfRgbColor{0.16, 0.16, 0.4}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    return sample;
}

[[nodiscard]] ScenarioResult build_repeat_phrase_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: repeat phrase text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Repeat phrase alpha", 12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 694.0, "Repeat phrase beta", 12.0,
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 668.0, "Repeat phrase gamma", 12.0,
        featherdoc::pdf::PdfRgbColor{0.18, 0.10, 0.10}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 642.0, "Repeat phrase delta", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.18, 0.38}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_bordered_box_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: bordered box text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.rectangles.push_back(featherdoc::pdf::PdfRectangle{
        featherdoc::pdf::PdfRect{72.0, 604.0, 360.0, 96.0},
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32},
        featherdoc::pdf::PdfRgbColor{0.94, 0.97, 1.0},
        1.25,
        true,
        true,
    });
    page.rectangles.push_back(featherdoc::pdf::PdfRectangle{
        featherdoc::pdf::PdfRect{90.0, 622.0, 324.0, 22.0},
        featherdoc::pdf::PdfRgbColor{0.72, 0.78, 0.86},
        featherdoc::pdf::PdfRgbColor{1.0, 1.0, 1.0},
        0.75,
        true,
        false,
    });
    page.text_runs.push_back(make_text_run(
        90.0, 676.0, "Bordered box sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        90.0, 642.0, "Box body text", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_line_primitive_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: line primitive text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{72.0, 690.0},
        featherdoc::pdf::PdfPoint{432.0, 690.0},
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        1.0,
        0.0,
        0.0,
        0.0,
        featherdoc::pdf::PdfLineCap::butt,
    });
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{72.0, 664.0},
        featherdoc::pdf::PdfPoint{432.0, 664.0},
        featherdoc::pdf::PdfRgbColor{0.12, 0.28, 0.78},
        1.25,
        0.0,
        6.0,
        4.0,
        featherdoc::pdf::PdfLineCap::round,
    });
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{72.0, 638.0},
        featherdoc::pdf::PdfPoint{432.0, 638.0},
        featherdoc::pdf::PdfRgbColor{0.78, 0.16, 0.16},
        2.0,
        0.0,
        0.0,
        0.0,
        featherdoc::pdf::PdfLineCap::square,
    });
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Line primitive sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 612.0, "Dashed and solid lines", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_table_like_grid_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: table-like grid text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    constexpr double left = 72.0;
    constexpr double top = 700.0;
    constexpr double cell_width = 120.0;
    constexpr double cell_height = 32.0;
    constexpr double columns = 3.0;
    constexpr double rows = 3.0;
    const auto right = left + cell_width * columns;
    const auto bottom = top - cell_height * rows;

    for (int column = 0; column <= static_cast<int>(columns); ++column) {
        const auto x = left + cell_width * static_cast<double>(column);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, bottom},
            featherdoc::pdf::PdfPoint{x, top},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }
    for (int row = 0; row <= static_cast<int>(rows); ++row) {
        const auto y = top - cell_height * static_cast<double>(row);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{left, y},
            featherdoc::pdf::PdfPoint{right, y},
            featherdoc::pdf::PdfRgbColor{0.12, 0.16, 0.22},
            0.75,
        });
    }

    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Grid sample header", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        86.0, 678.0, "Cell A1", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        206.0, 646.0, "Cell B2", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        326.0, 614.0, "Cell C3", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_metadata_long_title_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample with a deliberately long metadata title";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.text_runs.push_back(make_text_run(
        72.0, 720.0, "Long metadata title sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 692.0, "Metadata path stays parseable", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_header_footer_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: header footer text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{72.0, 704.0},
        featherdoc::pdf::PdfPoint{540.0, 704.0},
        featherdoc::pdf::PdfRgbColor{0.64, 0.68, 0.74},
        0.75,
    });
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{72.0, 92.0},
        featherdoc::pdf::PdfPoint{540.0, 92.0},
        featherdoc::pdf::PdfRgbColor{0.64, 0.68, 0.74},
        0.75,
    });
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Document header area", 10.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 660.0, "Header footer sample", 15.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 640.0, "Body text stays between page bands", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 72.0, "Page 1 footer marker", 10.0,
        featherdoc::pdf::PdfRgbColor{0.22, 0.26, 0.34}, false, true, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_header_footer_rtl_text_sample(
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
        !title.set_text("Header footer RTL bidi sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center)) {
        return sample;
    }

    auto body = title.insert_paragraph_after("");
    if (!body.has_next() ||
        !body.set_text(
            "Body anchor paragraph keeps header and footer rendering active.")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    if (!header.has_next() ||
        !document.set_paragraph_style(header, "DocumentPdfRtlParagraph") ||
        !header.set_bidi(true) ||
        !header.set_alignment(featherdoc::paragraph_alignment::right) ||
        !header.add_run("Header mixed: ").has_next()) {
        return sample;
    }
    auto header_prefix =
        header.add_run(utf8_from_u8(u8"\u0627\u0644\u0646\u0633\u062e\u0629"));
    if (!header_prefix.has_next() ||
        !document.set_run_style(header_prefix,
                                "DocumentPdfRtlArabicEmphasis") ||
        !header.add_run(" HF-2048 ").has_next()) {
        return sample;
    }
    auto header_suffix = header.add_run(
        utf8_from_u8(u8"\u062c\u0627\u0647\u0632\u0629 \u0644\u0644\u0645\u0631\u0627\u062c\u0639\u0629"));
    if (!header_suffix.has_next() ||
        !document.set_run_style(header_suffix, "DocumentPdfRtlArabic")) {
        return sample;
    }

    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!footer.has_next() ||
        !document.set_paragraph_style(footer, "DocumentPdfRtlParagraph") ||
        !footer.set_bidi(true) ||
        !footer.set_alignment(featherdoc::paragraph_alignment::right) ||
        !footer.add_run("Footer page ").has_next() ||
        !footer.add_run("{{page}}").has_next() ||
        !footer.add_run(" / ").has_next() ||
        !footer.add_run("{{total_pages}}").has_next() ||
        !footer.add_run(" ").has_next()) {
        return sample;
    }
    auto footer_suffix = footer.add_run(
        utf8_from_u8(u8"\u0645\u0631\u0627\u062c\u0639\u0629 \u0646\u0647\u0627\u0626\u064a\u0629"));
    if (!footer_suffix.has_next() ||
        !document.set_run_style(footer_suffix, "DocumentPdfRtlArabic")) {
        return sample;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{520.0, 360.0};
    options.metadata.title =
        "FeatherDoc regression sample: header footer RTL bidi";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Document RTL Arabic", font_path},
    };
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 9.0;
    options.line_height_points = 18.0;
    options.paragraph_spacing_after_points = 6.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_two_column_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: two column text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{306.0, 608.0},
        featherdoc::pdf::PdfPoint{306.0, 704.0},
        featherdoc::pdf::PdfRgbColor{0.82, 0.84, 0.88},
        0.75,
    });
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Two column sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 684.0, "Left column alpha", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        72.0, 660.0, "Left column beta", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        330.0, 684.0, "Right column alpha", 12.0,
        featherdoc::pdf::PdfRgbColor{0.20, 0.12, 0.08}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        330.0, 660.0, "Right column beta", 12.0,
        featherdoc::pdf::PdfRgbColor{0.20, 0.12, 0.08}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_invoice_grid_text_sample() {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: invoice grid text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    constexpr double left = 72.0;
    constexpr double top = 690.0;
    constexpr double right = 540.0;
    constexpr double row_height = 30.0;

    for (int row = 0; row <= 4; ++row) {
        const auto y = top - row_height * static_cast<double>(row);
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{left, y},
            featherdoc::pdf::PdfPoint{right, y},
            featherdoc::pdf::PdfRgbColor{0.16, 0.20, 0.28},
            row == 0 || row == 4 ? 1.1 : 0.65,
        });
    }
    for (const auto x : {left, 300.0, 420.0, right}) {
        page.lines.push_back(featherdoc::pdf::PdfLine{
            featherdoc::pdf::PdfPoint{x, top - row_height * 4.0},
            featherdoc::pdf::PdfPoint{x, top},
            featherdoc::pdf::PdfRgbColor{0.16, 0.20, 0.28},
            0.65,
        });
    }

    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Invoice grid sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        84.0, 670.0, "Line item alpha", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        84.0, 640.0, "Line item beta", 11.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        432.0, 580.0, "Total USD 42", 11.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, true,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_image_caption_text_sample(
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: image caption text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    auto page = make_letter_page();
    const auto image_path = write_tiny_rgb_png(asset_dir);
    page.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 602.0, 96.0, 96.0},
        image_path,
        "image/png",
        "Generated inline image asset",
        true,
        true,
    });
    page.rectangles.push_back(featherdoc::pdf::PdfRectangle{
        featherdoc::pdf::PdfRect{70.0, 600.0, 100.0, 100.0},
        featherdoc::pdf::PdfRgbColor{0.20, 0.24, 0.30},
        featherdoc::pdf::PdfRgbColor{1.0, 1.0, 1.0},
        0.75,
        true,
        false,
    });
    page.text_runs.push_back(make_text_run(
        72.0, 724.0, "Image caption sample", 14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        190.0, 674.0, "Generated inline image asset", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page.text_runs.push_back(make_text_run(
        190.0, 650.0, "PDFium reads caption beside image", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page));
    return sample;
}

[[nodiscard]] ScenarioResult build_image_report_text_sample(
    const std::filesystem::path &asset_dir) {
    ScenarioResult sample;
    sample.layout.metadata.title =
        "FeatherDoc regression sample: image report text";
    sample.layout.metadata.creator = "FeatherDoc regression tests";

    const auto image_path = write_tiny_rgb_png(
        asset_dir, "featherdoc-pdf-regression-image-report.png");

    auto page1 = make_letter_page();
    page1.text_runs.push_back(make_text_run(
        72.0, 724.0, "Image report sample", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page1.text_runs.push_back(make_text_run(
        72.0, 698.0, "Page 1 keeps text readable beside an image", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page1.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 540.0, 108.0, 108.0},
        image_path,
        "image/png",
        "Report figure one",
        true,
        true,
    });
    page1.text_runs.push_back(make_text_run(
        198.0, 610.0, "Figure 1: inline image on the first page", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page1.text_runs.push_back(make_text_run(
        198.0, 586.0, "PDFium still extracts text across the figure", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    auto page2 = make_letter_page();
    page2.text_runs.push_back(make_text_run(
        72.0, 724.0, "Image report sample page 2", 16.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0}, true, false, false,
        false));
    page2.text_runs.push_back(make_text_run(
        72.0, 698.0, "Second page confirms the image path is stable", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page2.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 540.0, 108.0, 108.0},
        image_path,
        "image/png",
        "Report figure two",
        true,
        true,
    });
    page2.text_runs.push_back(make_text_run(
        198.0, 610.0, "Figure 2: reused image asset on the second page", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));
    page2.text_runs.push_back(make_text_run(
        198.0, 586.0, "Page 2 text extraction remains intact", 12.0,
        featherdoc::pdf::PdfRgbColor{0.08, 0.16, 0.32}, false, false, false,
        false));

    sample.layout.pages.push_back(std::move(page1));
    sample.layout.pages.push_back(std::move(page2));
    return sample;
}
