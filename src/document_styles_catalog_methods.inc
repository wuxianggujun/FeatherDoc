std::vector<featherdoc::style_summary> Document::list_styles() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before listing styles",
                       std::string{styles_xml_entry});
        return {};
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return {};
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return {};
    }

    auto numbering_ready = false;
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        if (!style_has_numbering(style)) {
            continue;
        }
        if (!this->ensure_numbering_loaded()) {
            numbering_ready =
                this->numbering.child("w:numbering") != pugi::xml_node{};
        }
        break;
    }

    const auto apply_numbering_lookup =
        [this, numbering_ready](featherdoc::style_summary &summary) {
            if (!numbering_ready || !summary.numbering.has_value() ||
                !summary.numbering->num_id.has_value()) {
                return;
            }

            const auto lookup =
                this->find_numbering_instance(*summary.numbering->num_id);
            if (!lookup.has_value()) {
                this->last_error_info.clear();
                return;
            }

            summary.numbering->definition_id = lookup->definition_id;
            if (!lookup->definition_name.empty()) {
                summary.numbering->definition_name = lookup->definition_name;
            }
            summary.numbering->instance = lookup->instance;
            this->last_error_info.clear();
        };

    auto summaries = std::vector<featherdoc::style_summary>{};
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        auto summary = summarize_style_node(style);
        apply_numbering_lookup(summary);
        summaries.push_back(std::move(summary));
    }

    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::style_summary>
Document::find_style(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before reading style metadata",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when reading style metadata",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto numbering_ready =
        style_has_numbering(style) && !this->ensure_numbering_loaded() &&
        this->numbering.child("w:numbering") != pugi::xml_node{};

    auto summary = summarize_style_node(style);
    if (numbering_ready && summary.numbering.has_value() &&
        summary.numbering->num_id.has_value()) {
        const auto lookup =
            this->find_numbering_instance(*summary.numbering->num_id);
        if (lookup.has_value()) {
            summary.numbering->definition_id = lookup->definition_id;
            if (!lookup->definition_name.empty()) {
                summary.numbering->definition_name = lookup->definition_name;
            }
            summary.numbering->instance = lookup->instance;
        }
    }

    this->last_error_info.clear();
    return summary;
}

std::optional<featherdoc::resolved_style_properties_summary>
Document::resolve_style_properties(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before resolving style properties",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when resolving style properties",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    std::vector<pugi::xml_node> chain;
    if (!build_style_inheritance_chain(styles_root, style,
                                       this->last_error_info, chain)) {
        return std::nullopt;
    }

    featherdoc::resolved_style_properties_summary summary{};
    summary.style_id = std::string{style_id};
    summary.type_name = style.attribute("w:type").value();
    summary.kind = decode_style_kind(summary.type_name);
    if (const auto based_on = style.child("w:basedOn").attribute("w:val");
        based_on != pugi::xml_attribute{} && based_on.value()[0] != '\0') {
        summary.based_on = std::string{based_on.value()};
    }

    summary.inheritance_chain.reserve(chain.size());
    for (const auto &visited_style : chain) {
        summary.inheritance_chain.emplace_back(
            std::string{style_node_id(visited_style)});
    }

    summary.run_font_family =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_primary_font_family(node.child("w:rPr"));
        });
    summary.run_east_asia_font_family =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_east_asia_font_family(node.child("w:rPr"));
        });
    summary.run_text_color =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_run_text_color(node.child("w:rPr"));
        });
    summary.run_bold =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_run_bold(node.child("w:rPr"));
        });
    summary.run_italic =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_run_italic(node.child("w:rPr"));
        });
    summary.run_strikethrough =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_run_strikethrough(node.child("w:rPr"));
        });
    summary.run_underline =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_run_underline(node.child("w:rPr"));
        });
    summary.run_superscript =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_vertical_align_value(
                node.child("w:rPr").child("w:vertAlign"), "superscript");
        });
    summary.run_subscript =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_vertical_align_value(
                node.child("w:rPr").child("w:vertAlign"), "subscript");
        });
    summary.run_font_size_points =
        resolve_style_double_property(chain, [](pugi::xml_node node) {
            return read_run_font_size_points(node.child("w:rPr"));
        });
    summary.run_language =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_primary_language(node.child("w:rPr"));
        });
    summary.run_east_asia_language =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_east_asia_language(node.child("w:rPr"));
        });
    summary.run_bidi_language =
        resolve_style_string_property(chain, [](pugi::xml_node node) {
            return read_bidi_language(node.child("w:rPr"));
        });
    summary.run_rtl =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_run_rtl(node.child("w:rPr"));
        });
    summary.paragraph_bidi =
        resolve_style_bool_property(chain, [](pugi::xml_node node) {
            return read_paragraph_bidi(node.child("w:pPr"));
        });

    this->last_error_info.clear();
    return summary;
}
