bool Document::materialize_style_run_properties(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before materializing "
                       "style run properties",
                       std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when materializing style "
                       "run properties",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto resolved = this->resolve_style_properties(style_id);
    if (!resolved.has_value()) {
        return false;
    }

    if (resolved->kind != featherdoc::style_kind::paragraph &&
        resolved->kind != featherdoc::style_kind::character) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id '" + std::string{style_id} + "' with type '" +
                resolved->type_name +
                "' does not support style run property materialization",
            std::string{styles_xml_entry});
        return false;
    }

    const auto style_id_string = std::string{style_id};
    const auto materialize_string =
        [&](const featherdoc::resolved_style_string_property &property,
            auto setter) -> bool {
        if (!property.value.has_value() ||
            !property.source_style_id.has_value() ||
            *property.source_style_id == style_id_string) {
            return true;
        }

        return (this->*setter)(style_id_string, *property.value);
    };

    const auto materialize_bool =
        [&](const featherdoc::resolved_style_bool_property &property,
            auto setter) -> bool {
        if (!property.value.has_value() ||
            !property.source_style_id.has_value() ||
            *property.source_style_id == style_id_string) {
            return true;
        }

        return (this->*setter)(style_id_string, *property.value);
    };

    const auto materialize_double =
        [&](const featherdoc::resolved_style_double_property &property,
            auto setter) -> bool {
        if (!property.value.has_value() ||
            !property.source_style_id.has_value() ||
            *property.source_style_id == style_id_string) {
            return true;
        }

        return (this->*setter)(style_id_string, *property.value);
    };

    const auto materialize_vertical_align = [&]() -> bool {
        const auto inherited_superscript =
            resolved->run_superscript.value.has_value() &&
            resolved->run_superscript.source_style_id.has_value() &&
            *resolved->run_superscript.source_style_id != style_id_string;
        const auto inherited_subscript =
            resolved->run_subscript.value.has_value() &&
            resolved->run_subscript.source_style_id.has_value() &&
            *resolved->run_subscript.source_style_id != style_id_string;
        if (!inherited_superscript && !inherited_subscript) {
            return true;
        }

        if (resolved->run_superscript.value.value_or(false)) {
            return this->set_style_run_superscript(style_id_string, true);
        }
        if (resolved->run_subscript.value.value_or(false)) {
            return this->set_style_run_subscript(style_id_string, true);
        }
        return this->set_style_run_superscript(style_id_string, false);
    };

    if (!materialize_string(resolved->run_text_color,
                            &Document::set_style_run_text_color) ||
        !materialize_bool(resolved->run_bold, &Document::set_style_run_bold) ||
        !materialize_bool(resolved->run_italic,
                          &Document::set_style_run_italic) ||
        !materialize_bool(resolved->run_strikethrough,
                          &Document::set_style_run_strikethrough) ||
        !materialize_bool(resolved->run_underline,
                          &Document::set_style_run_underline) ||
        !materialize_vertical_align() ||
        !materialize_double(resolved->run_font_size_points,
                            &Document::set_style_run_font_size_points) ||
        !materialize_string(resolved->run_font_family,
                            &Document::set_style_run_font_family) ||
        !materialize_string(resolved->run_east_asia_font_family,
                            &Document::set_style_run_east_asia_font_family) ||
        !materialize_string(resolved->run_language,
                            &Document::set_style_run_language) ||
        !materialize_string(resolved->run_east_asia_language,
                            &Document::set_style_run_east_asia_language) ||
        !materialize_string(resolved->run_bidi_language,
                            &Document::set_style_run_bidi_language) ||
        !materialize_bool(resolved->run_rtl, &Document::set_style_run_rtl)) {
        return false;
    }

    if (resolved->kind == featherdoc::style_kind::paragraph &&
        !materialize_bool(resolved->paragraph_bidi,
                          &Document::set_style_paragraph_bidi)) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}
#include "document_styles_property_metadata_write_methods.inc"
bool Document::set_style_run_text_color(std::string_view style_id,
                                        std::string_view text_color) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run color",
            std::string{styles_xml_entry});
        return false;
    }

    if (style_id.empty() || text_color.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id and run text color must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_properties = ensure_style_run_properties_node(style);
    const auto color = ensure_run_text_color_node(run_properties);
    if (color == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:color for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(color, "w:val", text_color);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_bold(std::string_view style_id, bool enabled) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run bold",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run bold",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto bold =
        ensure_run_bold_node(ensure_style_run_properties_node(style));
    if (bold == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:b for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    set_on_off_value(bold, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_italic(std::string_view style_id, bool enabled) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run italic",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run italic",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto italic =
        ensure_run_italic_node(ensure_style_run_properties_node(style));
    if (italic == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:i for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    set_on_off_value(italic, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_strikethrough(std::string_view style_id,
                                           bool enabled) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style "
                       "run strikethrough",
                       std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "strikethrough",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto strike =
        ensure_run_strikethrough_node(ensure_style_run_properties_node(style));
    if (strike == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:strike for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    set_on_off_value(strike, enabled);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_underline(std::string_view style_id,
                                       bool enabled) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run underline",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run underline",
            std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto underline =
        ensure_run_underline_node(ensure_style_run_properties_node(style));
    if (underline == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:u for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    ensure_attribute_value(underline, "w:val", enabled ? "single" : "none");
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_superscript(std::string_view style_id,
                                         bool enabled) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run "
            "superscript",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "superscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto vertical_align =
        ensure_run_vertical_align_node(ensure_style_run_properties_node(style));
    if (vertical_align == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:vertAlign for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    ensure_attribute_value(vertical_align, "w:val",
                           enabled ? "superscript" : "baseline");
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_subscript(std::string_view style_id,
                                       bool enabled) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing style "
                       "run subscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when editing style run "
                       "subscript",
                       std::string{styles_xml_entry});
        return false;
    }
    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }
    auto style = find_style_node(this->styles.child("w:styles"), style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto vertical_align =
        ensure_run_vertical_align_node(ensure_style_run_properties_node(style));
    if (vertical_align == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:vertAlign for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    ensure_attribute_value(vertical_align, "w:val",
                           enabled ? "subscript" : "baseline");
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

#include "document_styles_property_run_font_write_methods.inc"
