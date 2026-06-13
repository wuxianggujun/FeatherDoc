[[nodiscard]] ScenarioResult build_document_table_merged_cells_text_sample() {
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
        !title.set_text("Document table merged cells sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Merged table cells should keep borders, text flow, and neighbor "
            "alignment stable in the rendered PDF.")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("Merged cells header M-101 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text("Merged cells footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto table = document.append_table(4U, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 2700U) ||
        !table.set_column_width_twips(2U, 2700U) ||
        !table.set_cell_text(0U, 0U, "Program summary") ||
        !table.set_cell_text(0U, 2U, "Status") ||
        !table.set_cell_text(1U, 0U, "Owner block") ||
        !table.set_cell_text(1U, 1U, "FE-901") ||
        !table.set_cell_text(1U, 2U, "Merged title row stays aligned") ||
        !table.set_cell_text(2U, 1U, "FE-902") ||
        !table.set_cell_text(2U, 2U, "Vertical merge block stays intact") ||
        !table.set_cell_text(3U, 0U, "Tail") ||
        !table.set_cell_text(3U, 1U, "Final release note") ||
        !table.set_cell_text(3U, 2U, "Merged footer row stays readable")) {
        return sample;
    }

    auto merged_title = table.find_cell(0U, 0U);
    auto vertical_block = table.find_cell(1U, 0U);
    auto tail_merge = table.find_cell(3U, 1U);
    if (!merged_title.has_value() || !vertical_block.has_value() ||
        !tail_merge.has_value() || !merged_title->merge_right(1U) ||
        !vertical_block->merge_down(1U) || !tail_merge->merge_right(1U)) {
        return sample;
    }

    if (!tail_merge->set_text("Merged footer row stays readable")) {
        return sample;
    }

    if (!merged_title->set_fill_color("D9EAF7") ||
        !vertical_block->set_fill_color("E8F3E8") ||
        !tail_merge->set_fill_color("F6E7D8") ||
        !vertical_block->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 4U; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index == 0U ? 520U : 720U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document table merged cells";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
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
build_document_table_merged_header_repeat_text_sample() {
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
        !title.set_text("Document table merged repeat sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Repeated header rows with merged cells should stay aligned across "
            "page breaks in the rendered PDF.")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("Merged repeat header R-303 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text("Merged repeat footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    constexpr std::size_t row_count = 8U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 1800U) ||
        !table.set_column_width_twips(2U, 3600U) ||
        !table.set_cell_text(0U, 0U, "Release board") ||
        !table.set_cell_text(1U, 0U, "Case") ||
        !table.set_cell_text(1U, 1U, "Owner") ||
        !table.set_cell_text(1U, 2U, "Notes") ||
        !table.set_cell_text(2U, 0U, "MR-01") ||
        !table.set_cell_text(2U, 1U, "Cluster lead") ||
        !table.set_cell_text(2U, 2U, "Merged owner block stays centered.") ||
        !table.set_cell_text(3U, 0U, "MR-02") ||
        !table.set_cell_text(3U, 2U,
                             "Second row remains aligned beneath the same "
                             "owner.") ||
        !table.set_cell_text(4U, 0U, "MR-03") ||
        !table.set_cell_text(4U, 1U, "Search") ||
        !table.set_cell_text(
            4U, 2U,
            "Repeated merged headers should reappear on page 2.") ||
        !table.set_cell_text(5U, 0U, "MR-04") ||
        !table.set_cell_text(5U, 1U, "Docs") ||
        !table.set_cell_text(
            5U, 2U,
            "Header/footer placeholders must keep page numbers stable.") ||
        !table.set_cell_text(6U, 0U, "Tail") ||
        !table.set_cell_text(6U, 1U, "Milestone note") ||
        !table.set_cell_text(
            6U, 2U, "Merged milestone row stays readable.") ||
        !table.set_cell_text(7U, 0U, "MR-05") ||
        !table.set_cell_text(7U, 1U, "Visual gate") ||
        !table.set_cell_text(
            7U, 2U, "Page 3 repeats the board header.")) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_owner = table.find_cell(2U, 1U);
    auto merged_tail = table.find_cell(6U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_owner.has_value() ||
        !merged_tail.has_value() || !heading_case.has_value() ||
        !heading_owner.has_value() || !heading_notes.has_value() ||
        !merged_banner->merge_right(2U) || !merged_owner->merge_down(1U) ||
        !merged_tail->merge_right(1U)) {
        return sample;
    }

    if (!merged_tail->set_text("Merged milestone row stays readable.")) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_owner->set_fill_color("E2F0D9") ||
        !merged_tail->set_fill_color("FCE4D6") ||
        !merged_owner->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!row.set_repeats_header() ||
                !row.set_height_twips(540U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (!row.set_height_twips(
                       820U, featherdoc::row_height_rule::exact)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table merged repeat";
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

[[nodiscard]] ScenarioResult
build_document_table_merged_header_footer_variants_text_sample() {
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
        !title.set_text("Document table merged header footer variants sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Merged cells should stay aligned while first, even, and default "
            "section headers and footers alternate across pages.")) {
        return sample;
    }

    auto default_header = document.ensure_section_header_paragraphs(0U);
    auto default_footer = document.ensure_section_footer_paragraphs(0U);
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto even_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    if (!default_header.has_next() ||
        !default_header.set_text("Merged variant header V-303 page {{page}}") ||
        !default_footer.has_next() ||
        !default_footer.set_text("V-303 {{page}}/{{total_pages}}") ||
        !first_header.has_next() ||
        !first_header.set_text(
            "Merged variant first header V-101 page {{page}}") ||
        !even_header.has_next() ||
        !even_header.set_text(
            "Merged variant even header V-202 page {{page}}") ||
        !first_footer.has_next() ||
        !first_footer.set_text("V-101 {{page}}/{{total_pages}}") ||
        !even_footer.has_next() ||
        !even_footer.set_text("V-202 {{page}}/{{total_pages}}")) {
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
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1800U) ||
        !table.set_column_width_twips(1U, 1800U) ||
        !table.set_column_width_twips(2U, 3600U) ||
        !table.set_cell_text(0U, 0U, "Release board") ||
        !table.set_cell_text(0U, 2U, "Notes") ||
        !table.set_cell_text(1U, 0U, "Case") ||
        !table.set_cell_text(1U, 1U, "Owner") ||
        !table.set_cell_text(1U, 2U, "Status") ||
        !table.set_cell_text(2U, 0U, "MR-01") ||
        !table.set_cell_text(2U, 1U, "Cluster lead") ||
        !table.set_cell_text(2U, 2U, "Merged owner block stays centered.") ||
        !table.set_cell_text(3U, 0U, "MR-02") ||
        !table.set_cell_text(3U, 2U,
                             "Second row remains aligned beneath the same "
                             "owner.") ||
        !table.set_cell_text(4U, 0U, "MR-03") ||
        !table.set_cell_text(4U, 1U, "Search") ||
        !table.set_cell_text(
            4U, 2U,
            "Repeated merged headers should reappear on page 2.") ||
        !table.set_cell_text(5U, 0U, "MR-04") ||
        !table.set_cell_text(5U, 1U, "Docs") ||
        !table.set_cell_text(
            5U, 2U,
            "Header/footer placeholders must keep page numbers stable.") ||
        !table.set_cell_text(6U, 0U, "Tail") ||
        !table.set_cell_text(6U, 1U, "Milestone note") ||
        !table.set_cell_text(
            6U, 2U, "Merged milestone row stays readable.") ||
        !table.set_cell_text(7U, 0U, "MR-05") ||
        !table.set_cell_text(7U, 1U, "Visual gate") ||
        !table.set_cell_text(
            7U, 2U, "Page 3 repeats the board header.")) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_owner = table.find_cell(2U, 1U);
    auto merged_tail = table.find_cell(6U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_owner.has_value() ||
        !merged_tail.has_value() || !heading_case.has_value() ||
        !heading_owner.has_value() || !heading_notes.has_value() ||
        !merged_banner->merge_right(2U) || !merged_owner->merge_down(1U) ||
        !merged_tail->merge_right(1U)) {
        return sample;
    }

    if (!merged_tail->set_text("Merged milestone row stays readable.")) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_owner->set_fill_color("E2F0D9") ||
        !merged_tail->set_fill_color("FCE4D6") ||
        !merged_owner->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!row.set_repeats_header() ||
                !row.set_height_twips(540U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (!row.set_height_twips(
                       720U, featherdoc::row_height_rule::exact)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.metadata.title = "FeatherDoc regression sample: document table "
                             "merged header footer variants";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
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
build_document_table_vertical_merged_cant_split_text_sample() {
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
        !title.set_text("Vertical merged cant split sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Repeated headers must keep a vertical merged owner block intact "
            "after the break.")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("Vertical merged header V-505 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text(
            "Vertical merged footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    auto header_contract = append_document_paragraph(document, "");
    if (!header_contract.has_next() ||
        !header_contract.add_run("Vertical merged header contract: ")
             .has_next() ||
        !header_contract
             .add_run(
                 "Vertical merged header V-505 page 1 / Vertical merged "
                 "header V-505 page 2 / Vertical merged header V-505 page 3 / "
                 "Vertical merged header V-505 page 4")
             .has_next()) {
        return sample;
    }

    auto footer_contract = append_document_paragraph(document, "");
    if (!footer_contract.has_next() ||
        !footer_contract.add_run("Vertical merged footer contract: ")
             .has_next() ||
        !footer_contract
             .add_run(
                 "Vertical merged footer 1 / 4 / Vertical merged footer 2 / "
                 "4 / Vertical merged footer 3 / 4 / Vertical merged footer "
                 "4 / 4")
             .has_next()) {
        return sample;
    }

    constexpr std::size_t row_count = 7U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1500U) ||
        !table.set_column_width_twips(1U, 1600U) ||
        !table.set_column_width_twips(2U, 4100U) ||
        !table.set_cell_text(0U, 0U, "Delivery board") ||
        !table.set_cell_text(1U, 0U, "Case") ||
        !table.set_cell_text(1U, 1U, "Owner") ||
        !table.set_cell_text(1U, 2U, "Notes") ||
        !table.set_cell_text(2U, 0U, "VMC-01") ||
        !table.set_cell_text(2U, 1U, "Queue") ||
        !table.set_cell_text(2U, 2U, "Short remainder.") ||
        !table.set_cell_text(3U, 0U, "VMC-02") ||
        !table.set_cell_text(3U, 1U, "Cluster") ||
        !table.set_cell_text(3U, 2U,
                             "Owner block restarts below header.") ||
        !table.set_cell_text(4U, 0U, "VMC-03") ||
        !table.set_cell_text(4U, 2U, "Second note stays with owner.") ||
        !table.set_cell_text(5U, 0U, "VMC-04") ||
        !table.set_cell_text(5U, 1U, "QA") ||
        !table.set_cell_text(
            5U, 2U, "Tail row confirms clean flow resumes.") ||
        !table.set_cell_text(6U, 0U, "VMC-05") ||
        !table.set_cell_text(6U, 1U, "Archive") ||
        !table.set_cell_text(6U, 2U, "Page numbering stays stable.")) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_owner = table.find_cell(3U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_owner.has_value() ||
        !heading_case.has_value() || !heading_owner.has_value() ||
        !heading_notes.has_value() || !merged_banner->merge_right(2U) ||
        !merged_owner->merge_down(1U)) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_owner->set_fill_color("E2F0D9") ||
        !merged_owner->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next() ||
            !row.set_height_twips(row_index < 2U ? 440U : 620U,
                                  featherdoc::row_height_rule::at_least)) {
            return sample;
        }
        if (row_index < 2U && !row.set_repeats_header()) {
            return sample;
        }
        if (row_index == 3U && !row.set_cant_split()) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize::letter_portrait();
    options.metadata.title =
        "FeatherDoc regression sample: document table vertical merged cant "
        "split";
    options.metadata.creator = "FeatherDoc regression tests";
    options.font_family = "Helvetica";
    options.use_system_font_fallbacks = false;
    options.render_headers_and_footers = true;
    options.expand_header_footer_page_placeholders = true;
    options.header_footer_font_size_points = 8.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 4.0;

    sample.layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    return sample;
}

[[nodiscard]] ScenarioResult build_document_table_merged_cant_split_text_sample() {
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
        !title.set_text("Merged cant split table sample") ||
        !title.set_alignment(featherdoc::paragraph_alignment::center) ||
        !append_document_text_paragraph(
            document,
            "Repeated headers must keep the merged row intact after the page "
            "break.")) {
        return sample;
    }

    auto header = document.ensure_section_header_paragraphs(0U);
    auto footer = document.ensure_section_footer_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("Merged cant split header T-404 page {{page}}") ||
        !footer.has_next() ||
        !footer.set_text(
            "Merged cant split footer {{page}} / {{total_pages}}")) {
        return sample;
    }

    constexpr std::size_t row_count = 6U;
    auto table = document.append_table(row_count, 3U);
    if (!table.has_next() || !table.set_width_twips(7200U) ||
        !table.set_column_width_twips(0U, 1600U) ||
        !table.set_column_width_twips(1U, 1800U) ||
        !table.set_column_width_twips(2U, 3800U) ||
        !table.set_cell_text(0U, 0U, "Release gate") ||
        !table.set_cell_text(1U, 0U, "Case") ||
        !table.set_cell_text(1U, 1U, "Owner") ||
        !table.set_cell_text(1U, 2U, "Notes") ||
        !table.set_cell_text(2U, 0U, "MCS-01") ||
        !table.set_cell_text(2U, 1U, "Queue") ||
        !table.set_cell_text(
            2U, 2U,
            "Lead row leaves a short remainder.") ||
        !table.set_cell_text(3U, 0U, "MCS-02") ||
        !table.set_cell_text(4U, 0U, "MCS-03") ||
        !table.set_cell_text(4U, 1U, "QA") ||
        !table.set_cell_text(
            4U, 2U,
            "Tail row confirms normal flow resumes.") ||
        !table.set_cell_text(5U, 0U, "MCS-04") ||
        !table.set_cell_text(5U, 1U, "Archive") ||
        !table.set_cell_text(
            5U, 2U,
            "Page numbering stays stable.")) {
        return sample;
    }

    auto merged_banner = table.find_cell(0U, 0U);
    auto merged_row = table.find_cell(3U, 1U);
    auto heading_case = table.find_cell(1U, 0U);
    auto heading_owner = table.find_cell(1U, 1U);
    auto heading_notes = table.find_cell(1U, 2U);
    if (!merged_banner.has_value() || !merged_row.has_value() ||
        !heading_case.has_value() || !heading_owner.has_value() ||
        !heading_notes.has_value() || !merged_banner->merge_right(2U) ||
        !merged_row->merge_right(1U)) {
        return sample;
    }

    if (!merged_banner->set_text("Release gate") ||
        !merged_row->set_text(
            "Merged cant-split row must restart beneath the repeated header "
            "and keep the whole note readable after the page break.")) {
        return sample;
    }

    if (!merged_banner->set_fill_color("D9EAF7") ||
        !heading_case->set_fill_color("EAF2F8") ||
        !heading_owner->set_fill_color("EAF2F8") ||
        !heading_notes->set_fill_color("EAF2F8") ||
        !merged_row->set_fill_color("E2F0D9") ||
        !merged_row->set_vertical_alignment(
            featherdoc::cell_vertical_alignment::center)) {
        return sample;
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < row_count; ++row_index) {
        if (!row.has_next()) {
            return sample;
        }
        if (row_index < 2U) {
            if (!row.set_repeats_header() ||
                !row.set_height_twips(520U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 2U) {
            if (!row.set_height_twips(920U,
                                      featherdoc::row_height_rule::exact)) {
                return sample;
            }
        } else if (row_index == 3U) {
            if (!row.set_cant_split() ||
                !row.set_height_twips(1500U,
                                      featherdoc::row_height_rule::at_least)) {
                return sample;
            }
        } else if (!row.set_height_twips(
                       760U, featherdoc::row_height_rule::exact)) {
            return sample;
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.page_size = featherdoc::pdf::PdfPageSize{320.0, 220.0};
    options.metadata.title =
        "FeatherDoc regression sample: document table merged cant split";
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
