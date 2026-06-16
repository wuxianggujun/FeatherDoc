void remove_empty_run_fonts_node(pugi::xml_node run_properties) {
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts == pugi::xml_node{}) {
        return;
    }

    if (run_fonts.first_child() == pugi::xml_node{} &&
        !node_has_attributes(run_fonts)) {
        run_properties.remove_child(run_fonts);
    }
}

void remove_empty_run_language_node(pugi::xml_node run_properties) {
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    auto run_language = run_properties.child("w:lang");
    if (run_language == pugi::xml_node{}) {
        return;
    }

    if (run_language.first_child() == pugi::xml_node{} &&
        !node_has_attributes(run_language)) {
        run_properties.remove_child(run_language);
    }
}

auto clear_font_family_attributes(pugi::xml_node run_properties) -> bool {
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts == pugi::xml_node{}) {
        return false;
    }

    bool removed = false;
    removed = run_fonts.remove_attribute("w:ascii") || removed;
    removed = run_fonts.remove_attribute("w:hAnsi") || removed;
    removed = run_fonts.remove_attribute("w:cs") || removed;
    removed = run_fonts.remove_attribute("w:eastAsia") || removed;
    remove_empty_run_fonts_node(run_properties);
    return removed;
}

auto clear_font_family_attribute(pugi::xml_node run_properties,
                                 const char *attribute_name) -> bool {
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts == pugi::xml_node{}) {
        return false;
    }

    const auto removed = run_fonts.remove_attribute(attribute_name);
    remove_empty_run_fonts_node(run_properties);
    return removed;
}

auto clear_language_attributes(pugi::xml_node run_properties) -> bool {
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    auto run_language = run_properties.child("w:lang");
    if (run_language == pugi::xml_node{}) {
        return false;
    }

    bool removed = false;
    removed = run_language.remove_attribute("w:val") || removed;
    removed = run_language.remove_attribute("w:eastAsia") || removed;
    removed = run_language.remove_attribute("w:bidi") || removed;
    remove_empty_run_language_node(run_properties);
    return removed;
}

auto clear_language_attribute(pugi::xml_node run_properties,
                              const char *attribute_name) -> bool {
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    auto run_language = run_properties.child("w:lang");
    if (run_language == pugi::xml_node{}) {
        return false;
    }

    const auto removed = run_language.remove_attribute(attribute_name);
    remove_empty_run_language_node(run_properties);
    return removed;
}

void set_on_off_value(pugi::xml_node node, bool enabled) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute("w:val");
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute("w:val");
    }
    attribute.set_value(enabled ? "1" : "0");
}

auto clear_on_off_child(pugi::xml_node parent, const char *child_name) -> bool {
    if (parent == pugi::xml_node{}) {
        return false;
    }

    const auto child = parent.child(child_name);
    return child != pugi::xml_node{} && parent.remove_child(child);
}

auto clear_run_text_color(pugi::xml_node run_properties) -> bool {
    return clear_on_off_child(run_properties, "w:color");
}

auto clear_run_bold(pugi::xml_node run_properties) -> bool {
    return clear_on_off_child(run_properties, "w:b");
}

auto clear_run_italic(pugi::xml_node run_properties) -> bool {
    return clear_on_off_child(run_properties, "w:i");
}

auto clear_run_underline(pugi::xml_node run_properties) -> bool {
    return clear_on_off_child(run_properties, "w:u");
}

auto clear_run_strikethrough(pugi::xml_node run_properties) -> bool {
    return clear_on_off_child(run_properties, "w:strike");
}

auto clear_run_vertical_align(pugi::xml_node run_properties) -> bool {
    return clear_on_off_child(run_properties, "w:vertAlign");
}

auto clear_run_font_size_points(pugi::xml_node run_properties) -> bool {
    bool removed = false;
    removed = clear_on_off_child(run_properties, "w:sz") || removed;
    removed = clear_on_off_child(run_properties, "w:szCs") || removed;
    return removed;
}

void set_style_flag_node(pugi::xml_node style, const char *child_name,
                         bool enabled) {
    if (style == pugi::xml_node{}) {
        return;
    }

    if (!enabled) {
        clear_on_off_child(style, child_name);
        return;
    }

    const auto flag = ensure_style_child_before_properties(style, child_name);
    if (flag == pugi::xml_node{}) {
        return;
    }

    ensure_attribute_value(flag, "w:val", "1");
}

void set_style_custom_flag(pugi::xml_node style, bool enabled) {
    if (style == pugi::xml_node{}) {
        return;
    }

    if (enabled) {
        ensure_attribute_value(style, "w:customStyle", "1");
        return;
    }

    style.remove_attribute("w:customStyle");
}

void remove_empty_style_paragraph_properties(pugi::xml_node style) {
    if (style == pugi::xml_node{}) {
        return;
    }

    auto paragraph_properties = style.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        return;
    }

    if (paragraph_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(paragraph_properties)) {
        style.remove_child(paragraph_properties);
    }
}

auto ensure_style_metadata_node(pugi::xml_node style,
                                const char *child_name) -> pugi::xml_node {
    const auto name = std::string_view{child_name};
    if (name == "w:name") {
        return ensure_style_name_node(style);
    }
    if (name == "w:basedOn") {
        return ensure_style_based_on_node(style);
    }
    if (name == "w:next") {
        return ensure_style_next_node(style);
    }
    return ensure_style_child_before_properties(style, child_name);
}

auto set_style_string_node(pugi::xml_node style, const char *child_name,
                           const std::optional<std::string> &value) -> bool {
    if (style == pugi::xml_node{}) {
        return false;
    }

    if (!value.has_value()) {
        if (const auto child = style.child(child_name);
            child != pugi::xml_node{}) {
            style.remove_child(child);
        }
        return true;
    }

    const auto child = ensure_style_metadata_node(style, child_name);
    if (child == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(child, "w:val", *value);
    return true;
}

auto set_style_outline_level(pugi::xml_node style,
                             const std::optional<std::uint32_t> &outline_level)
    -> bool {
    if (style == pugi::xml_node{}) {
        return false;
    }

    if (!outline_level.has_value()) {
        if (auto paragraph_properties = style.child("w:pPr");
            paragraph_properties != pugi::xml_node{}) {
            clear_on_off_child(paragraph_properties, "w:outlineLvl");
            remove_empty_style_paragraph_properties(style);
        }
        return true;
    }

    const auto paragraph_properties =
        ensure_style_paragraph_properties_node(style);
    if (paragraph_properties == pugi::xml_node{}) {
        return false;
    }

    const auto outline = ensure_style_outline_level_node(paragraph_properties);
    if (outline == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(outline, "w:val", std::to_string(*outline_level));
    return true;
}

auto validate_optional_style_value(
    featherdoc::document_error_info &last_error_info,
    const std::optional<std::string> &value, std::string_view detail) -> bool {
    if (!value.has_value() || !value->empty()) {
        return true;
    }

    set_last_error(last_error_info,
                   std::make_error_code(std::errc::invalid_argument),
                   std::string{detail}, std::string{styles_xml_entry});
    return false;
}

auto apply_common_style_definition(
    pugi::xml_node style, std::string_view style_id, std::string_view type_name,
    std::string_view name, const std::optional<std::string> &based_on,
    bool is_custom, bool is_semi_hidden, bool is_unhide_when_used,
    bool is_quick_format) -> bool {
    if (style == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(style, "w:styleId", style_id);
    ensure_attribute_value(style, "w:type", type_name);
    set_style_custom_flag(style, is_custom);

    const auto name_node = ensure_style_name_node(style);
    if (name_node == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(name_node, "w:val", name);
    if (!set_style_string_node(style, "w:basedOn", based_on)) {
        return false;
    }

    set_style_flag_node(style, "w:semiHidden", is_semi_hidden);
    set_style_flag_node(style, "w:unhideWhenUsed", is_unhide_when_used);
    set_style_flag_node(style, "w:qFormat", is_quick_format);
    return true;
}

void remove_empty_paragraph_properties(pugi::xml_node paragraph) {
    if (paragraph == pugi::xml_node{}) {
        return;
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        return;
    }

    if (paragraph_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(paragraph_properties)) {
        paragraph.remove_child(paragraph_properties);
    }
}

void remove_empty_run_properties(pugi::xml_node run) {
    if (run == pugi::xml_node{}) {
        return;
    }

    auto run_properties = run.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    if (run_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(run_properties)) {
        run.remove_child(run_properties);
    }
}

void remove_empty_style_run_properties(pugi::xml_node style) {
    if (style == pugi::xml_node{}) {
        return;
    }

    auto run_properties = style.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    if (run_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(run_properties)) {
        style.remove_child(run_properties);
    }
}
