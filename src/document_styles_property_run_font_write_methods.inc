bool Document::set_style_run_font_size_points(std::string_view style_id,
                                              double font_size_points) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run font size",
            std::string{styles_xml_entry});
        return false;
    }
    if (style_id.empty() || font_size_points <= 0.0) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty and style run font size "
                       "points must be greater than zero",
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
    const auto run_properties = ensure_style_run_properties_node(style);
    const auto primary_size = ensure_run_font_size_node(run_properties, "w:sz");
    const auto complex_size =
        ensure_run_font_size_node(run_properties, "w:szCs");
    if (primary_size == pugi::xml_node{} || complex_size == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create font size nodes for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }
    const auto half_points =
        std::to_string(std::llround(font_size_points * 2.0));
    ensure_attribute_value(primary_size, "w:val", half_points);
    ensure_attribute_value(complex_size, "w:val", half_points);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_font_family(std::string_view style_id,
                                         std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run fonts");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run fonts",
            std::string{styles_xml_entry});
        return false;
    }

    if (font_family.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style run font family must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    const auto run_properties = ensure_style_run_properties_node(style);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rPr for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rFonts for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_fonts, "w:ascii", font_family);
    ensure_attribute_value(run_fonts, "w:hAnsi", font_family);
    ensure_attribute_value(run_fonts, "w:cs", font_family);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

bool Document::set_style_run_east_asia_font_family(
    std::string_view style_id, std::string_view font_family) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before editing style run fonts");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when editing style run fonts",
            std::string{styles_xml_entry});
        return false;
    }

    if (font_family.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style eastAsia font family must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!this->has_styles_part) {
        if (const auto error = this->ensure_styles_part_attached()) {
            return false;
        }
        styles_root = this->styles.child("w:styles");
        style = find_style_node(styles_root, style_id);
    }

    const auto run_properties = ensure_style_run_properties_node(style);
    if (run_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rPr for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:rFonts for style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    ensure_attribute_value(run_fonts, "w:eastAsia", font_family);
    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

#include "document_styles_property_language_write_methods.inc"
