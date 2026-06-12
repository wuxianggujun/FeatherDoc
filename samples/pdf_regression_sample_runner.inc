int run_program(const std::vector<std::string> &args) {
    SampleConfig config;
    bool show_help = false;
    bool require_cjk_font = false;
    if (!parse_arguments(args, config, show_help, require_cjk_font)) {
        print_usage();
        return 2;
    }
    if (show_help) {
        print_usage();
        return 0;
    }

    const auto output_parent = config.output_path.parent_path();
    if (!output_parent.empty()) {
        std::filesystem::create_directories(output_parent);
    }

    const auto cjk_font = resolve_cjk_font();
    const auto arabic_font = resolve_arabic_font();
    ScenarioResult sample;
    if (config.scenario == "single_text") {
        sample = build_single_text_sample();
    } else if (config.scenario == "multi_page_text") {
        sample = build_multi_page_text_sample();
    } else if (config.scenario == "cjk_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario cjk_text\n";
            return 1;
        }
        sample = build_cjk_text_sample(cjk_font);
    } else if (config.scenario == "mixed_cjk_punctuation_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario mixed_cjk_punctuation_text\n";
            return 1;
        }
        sample = build_mixed_cjk_punctuation_text_sample(cjk_font);
    } else if (config.scenario == "styled_text") {
        sample = build_styled_text_sample();
    } else if (config.scenario == "font_size_text") {
        sample = build_font_size_text_sample();
    } else if (config.scenario == "color_text") {
        sample = build_color_text_sample();
    } else if (config.scenario == "mixed_style_text") {
        sample = build_mixed_style_text_sample();
    } else if (config.scenario == "contract_cjk_style") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario contract_cjk_style\n";
            return 1;
        }
        sample = build_contract_cjk_style_sample(cjk_font);
    } else if (config.scenario == "document_contract_cjk_style") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario document_contract_cjk_style\n";
            return 1;
        }
        sample = build_document_contract_cjk_style_sample(cjk_font);
    } else if (config.scenario == "document_eastasia_style_probe") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario document_eastasia_style_probe\n";
            return 1;
        }
        sample = build_document_eastasia_style_probe_sample(cjk_font);
    } else if (config.scenario == "document_rtl_bidi_text") {
        if (arabic_font.empty() || !std::filesystem::exists(arabic_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping RTL regression sample: no usable Arabic "
                             "font found; set FEATHERDOC_TEST_ARABIC_FONT or "
                             "install a common Arabic font\n";
                return 77;
            }
            std::cerr
                << "missing Arabic font for scenario document_rtl_bidi_text\n";
            return 1;
        }
        sample = build_document_rtl_bidi_text_sample(arabic_font);
    } else if (config.scenario == "document_style_gallery_text") {
        sample = build_document_style_gallery_text_sample();
    } else if (config.scenario == "document_font_matrix_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario document_font_matrix_text\n";
            return 1;
        }
        if (arabic_font.empty() || !std::filesystem::exists(arabic_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping RTL regression sample: no usable Arabic "
                             "font found; set FEATHERDOC_TEST_ARABIC_FONT or "
                             "install a common Arabic font\n";
                return 77;
            }
            std::cerr
                << "missing Arabic font for scenario document_font_matrix_text\n";
            return 1;
        }
        sample = build_document_font_matrix_text_sample(cjk_font, arabic_font);
    } else if (config.scenario == "document_table_font_matrix_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_table_font_matrix_text\n";
            return 1;
        }
        if (arabic_font.empty() || !std::filesystem::exists(arabic_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping RTL regression sample: no usable Arabic "
                             "font found; set FEATHERDOC_TEST_ARABIC_FONT or "
                             "install a common Arabic font\n";
                return 77;
            }
            std::cerr << "missing Arabic font for scenario "
                         "document_table_font_matrix_text\n";
            return 1;
        }
        sample =
            build_document_table_font_matrix_text_sample(cjk_font, arabic_font);
    } else if (config.scenario == "three_page_text") {
        sample = build_three_page_text_sample();
    } else if (config.scenario == "landscape_text") {
        sample = build_landscape_text_sample();
    } else if (config.scenario == "title_body_text") {
        sample = build_title_body_text_sample();
    } else if (config.scenario == "dense_text") {
        sample = build_dense_text_sample();
    } else if (config.scenario == "four_page_text") {
        sample = build_four_page_text_sample();
    } else if (config.scenario == "underline_text") {
        sample = build_underline_text_sample();
    } else if (config.scenario == "strikethrough_text") {
        sample = build_document_strikethrough_text_sample();
    } else if (config.scenario == "superscript_subscript_text") {
        sample = build_document_superscript_subscript_text_sample();
    } else if (config.scenario == "style_superscript_subscript_text") {
        sample = build_document_style_superscript_subscript_text_sample();
    } else if (config.scenario == "punctuation_text") {
        sample = build_punctuation_text_sample();
    } else if (config.scenario == "latin_ligature_text") {
        sample = build_latin_ligature_text_sample();
    } else if (config.scenario == "two_page_text") {
        sample = build_two_page_text_sample();
    } else if (config.scenario == "repeat_phrase_text") {
        sample = build_repeat_phrase_text_sample();
    } else if (config.scenario == "bordered_box_text") {
        sample = build_bordered_box_text_sample();
    } else if (config.scenario == "line_primitive_text") {
        sample = build_line_primitive_text_sample();
    } else if (config.scenario == "table_like_grid_text") {
        sample = build_table_like_grid_text_sample();
    } else if (config.scenario == "metadata_long_title_text") {
        sample = build_metadata_long_title_text_sample();
    } else if (config.scenario == "header_footer_text") {
        sample = build_header_footer_text_sample();
    } else if (config.scenario == "header_footer_rtl_text") {
        if (arabic_font.empty() || !std::filesystem::exists(arabic_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping RTL regression sample: no usable Arabic "
                             "font found; set FEATHERDOC_TEST_ARABIC_FONT or "
                             "install a common Arabic font\n";
                return 77;
            }
            std::cerr
                << "missing Arabic font for scenario header_footer_rtl_text\n";
            return 1;
        }
        sample = build_header_footer_rtl_text_sample(arabic_font);
    } else if (config.scenario == "header_footer_rtl_variants_text") {
        if (arabic_font.empty() || !std::filesystem::exists(arabic_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping RTL regression sample: no usable Arabic "
                             "font found; set FEATHERDOC_TEST_ARABIC_FONT or "
                             "install a common Arabic font\n";
                return 77;
            }
            std::cerr << "missing Arabic font for scenario "
                         "header_footer_rtl_variants_text\n";
            return 1;
        }
        sample = build_header_footer_rtl_variants_text_sample(arabic_font);
    } else if (config.scenario == "two_column_text") {
        sample = build_two_column_text_sample();
    } else if (config.scenario == "invoice_grid_text") {
        sample = build_invoice_grid_text_sample();
    } else if (config.scenario == "image_caption_text") {
        sample = build_image_caption_text_sample(output_parent);
    } else if (config.scenario == "sectioned_report_text") {
        sample = build_sectioned_report_text_sample();
    } else if (config.scenario == "list_report_text") {
        sample = build_list_report_text_sample();
    } else if (config.scenario == "document_cjk_bullet_list_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_bullet_list_text\n";
            return 1;
        }
        sample = build_document_cjk_bullet_list_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_numbered_list_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_numbered_list_text\n";
            return 1;
        }
        sample = build_document_cjk_numbered_list_text_sample(cjk_font);
    } else if (config.scenario ==
               "document_cjk_numbered_list_page_flow_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_numbered_list_page_flow_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_numbered_list_page_flow_lite_text_sample(
            cjk_font);
    } else if (config.scenario ==
               "document_cjk_numbered_list_page_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_numbered_list_page_flow_text\n";
            return 1;
        }
        sample = build_document_cjk_numbered_list_page_flow_text_sample(
            cjk_font);
    } else if (config.scenario == "document_cjk_bullet_page_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_bullet_page_flow_text\n";
            return 1;
        }
        sample = build_document_cjk_bullet_page_flow_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_bullet_overlay_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_bullet_overlay_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_bullet_overlay_lite_text_sample(cjk_font);
    } else if (config.scenario ==
               "document_cjk_bullet_overlay_page_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_bullet_overlay_page_flow_text\n";
            return 1;
        }
        sample = build_document_cjk_bullet_overlay_page_flow_text_sample(
            cjk_font);
    } else if (config.scenario == "document_cjk_copy_search_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_copy_search_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_copy_search_lite_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_copy_search_matrix_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_copy_search_matrix_text\n";
            return 1;
        }
        sample = build_document_cjk_copy_search_matrix_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_font_embed_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_font_embed_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_font_embed_lite_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_font_embed_matrix_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_font_embed_matrix_text\n";
            return 1;
        }
        sample = build_document_cjk_font_embed_matrix_text_sample(cjk_font);
    } else if (
        config.scenario == "document_cjk_anchor_font_matrix_boundary_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_anchor_font_matrix_boundary_text\n";
            return 1;
        }
        sample =
            build_document_cjk_anchor_font_matrix_boundary_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_style_overlay_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_style_overlay_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_style_overlay_lite_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_anchor_matrix_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_anchor_matrix_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_anchor_matrix_lite_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_search_density_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_search_density_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_search_density_lite_text_sample(cjk_font);
    } else if (
        config.scenario == "document_cjk_font_search_density_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_font_search_density_flow_text\n";
            return 1;
        }
        sample = build_document_cjk_font_search_density_flow_text_sample(
            cjk_font);
    } else if (config.scenario == "document_cjk_repeated_key_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_repeated_key_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_repeated_key_lite_text_sample(cjk_font);
    } else if (
        config.scenario == "document_cjk_repeated_key_boundary_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_repeated_key_boundary_flow_text\n";
            return 1;
        }
        sample =
            build_document_cjk_repeated_key_boundary_flow_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_complex_layout_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_complex_layout_text\n";
            return 1;
        }
        sample = build_document_cjk_complex_layout_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_image_wrap_stress_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_image_wrap_stress_text\n";
            return 1;
        }
        sample = build_document_cjk_image_wrap_stress_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_extreme_page_breaks_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_extreme_page_breaks_text\n";
            return 1;
        }
        sample = build_document_cjk_extreme_page_breaks_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_vertical_merge_wrap_cant_split_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_vertical_merge_wrap_cant_split_text\n";
            return 1;
        }
        sample =
            build_document_cjk_vertical_merge_wrap_cant_split_text_sample(
                cjk_font);
    } else if (config.scenario == "document_cjk_style_overlay_page_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_style_overlay_page_flow_text\n";
            return 1;
        }
        sample =
            build_document_cjk_style_overlay_page_flow_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_vertical_merge_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_vertical_merge_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_vertical_merge_lite_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_table_wrap_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_table_wrap_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_table_wrap_lite_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_table_wrap_page_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_table_wrap_page_flow_text\n";
            return 1;
        }
        sample =
            build_document_cjk_table_wrap_page_flow_text_sample(cjk_font);
    } else if (config.scenario ==
               "document_cjk_font_embed_wrap_mix_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_font_embed_wrap_mix_lite_text\n";
            return 1;
        }
        sample =
            build_document_cjk_font_embed_wrap_mix_lite_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_font_embed_wrap_mix_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_font_embed_wrap_mix_text\n";
            return 1;
        }
        sample = build_document_cjk_font_embed_wrap_mix_text_sample(cjk_font);
    } else if (config.scenario ==
               "document_cjk_multi_anchor_table_flow_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_multi_anchor_table_flow_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_multi_anchor_table_flow_lite_text_sample(
            cjk_font);
    } else if (config.scenario ==
               "document_cjk_multi_anchor_table_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_multi_anchor_table_flow_text\n";
            return 1;
        }
        sample =
            build_document_cjk_multi_anchor_table_flow_text_sample(cjk_font);
    } else if (config.scenario == "document_cjk_page_boundary_lite_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario "
                   "document_cjk_page_boundary_lite_text\n";
            return 1;
        }
        sample = build_document_cjk_page_boundary_lite_text_sample(
            cjk_font, output_parent);
    } else if (config.scenario == "long_report_text") {
        sample = build_long_report_text_sample();
    } else if (config.scenario == "document_long_flow_text") {
        sample = build_document_long_flow_text_sample();
    } else if (config.scenario == "image_report_text") {
        sample = build_image_report_text_sample(output_parent);
    } else if (config.scenario == "document_image_semantics_text") {
        sample = build_document_image_semantics_text_sample(output_parent);
    } else if (config.scenario == "document_table_semantics_text") {
        sample = build_document_table_semantics_text_sample();
    } else if (config.scenario == "document_invoice_table_text") {
        sample = build_document_invoice_table_text_sample();
    } else if (config.scenario ==
               "document_table_header_footer_variants_text") {
        sample = build_document_table_header_footer_variants_text_sample();
    } else if (config.scenario == "document_table_wrap_flow_text") {
        sample = build_document_table_wrap_flow_text_sample();
    } else if (config.scenario == "document_table_cjk_wrap_flow_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_table_cjk_wrap_flow_text\n";
            return 1;
        }
        sample = build_document_table_cjk_wrap_flow_text_sample(cjk_font);
    } else if (config.scenario ==
               "document_table_cjk_vertical_merged_cant_split_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_table_cjk_vertical_merged_cant_split_text\n";
            return 1;
        }
        sample =
            build_document_table_cjk_vertical_merged_cant_split_text_sample(
                cjk_font);
    } else if (config.scenario == "document_table_cjk_merged_repeat_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario "
                         "document_table_cjk_merged_repeat_text\n";
            return 1;
        }
        sample = build_document_table_cjk_merged_repeat_text_sample(cjk_font);
    } else if (config.scenario == "document_table_cant_split_text") {
        sample = build_document_table_cant_split_text_sample();
    } else if (config.scenario == "document_table_merged_cells_text") {
        sample = build_document_table_merged_cells_text_sample();
    } else if (config.scenario == "document_table_merged_header_repeat_text") {
        sample = build_document_table_merged_header_repeat_text_sample();
    } else if (config.scenario ==
               "document_table_merged_header_footer_variants_text") {
        sample =
            build_document_table_merged_header_footer_variants_text_sample();
    } else if (config.scenario ==
               "document_table_vertical_merged_cant_split_text") {
        sample = build_document_table_vertical_merged_cant_split_text_sample();
    } else if (config.scenario == "document_table_merged_cant_split_text") {
        sample = build_document_table_merged_cant_split_text_sample();
    } else if (config.scenario == "cjk_image_report_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr
                << "missing CJK font for scenario cjk_image_report_text\n";
            return 1;
        }
        sample = build_cjk_image_report_text_sample(cjk_font, output_parent);
    } else if (config.scenario == "cjk_report_text") {
        if (cjk_font.empty() || !std::filesystem::exists(cjk_font)) {
            if (require_cjk_font) {
                std::cerr << "skipping CJK regression sample: no usable CJK font "
                             "found; set FEATHERDOC_TEST_CJK_FONT or install a "
                             "common CJK font\n";
                return 77;
            }
            std::cerr << "missing CJK font for scenario cjk_report_text\n";
            return 1;
        }
        sample = build_cjk_report_text_sample(cjk_font);
    } else {
        std::cerr << "unknown scenario: " << config.scenario << '\n';
        print_usage();
        return 2;
    }

    return write_and_validate_sample(config, sample, require_cjk_font);
}
