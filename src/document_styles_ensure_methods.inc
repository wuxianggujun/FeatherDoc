bool Document::ensure_paragraph_style(
    std::string_view style_id,
    const featherdoc::paragraph_style_definition &definition) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before defining paragraph styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when defining paragraph styles",
            std::string{styles_xml_entry});
        return false;
    }

    if (definition.name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style name must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!validate_optional_style_value(
            this->last_error_info, definition.based_on,
            "paragraph style basedOn must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.next_style,
            "paragraph style next style must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_font_family,
            "paragraph style run font family must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_font_family,
            "paragraph style eastAsia font family must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_language,
            "paragraph style run language must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_language,
            "paragraph style eastAsia language must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_bidi_language,
            "paragraph style bidi language must not be empty")) {
        return false;
    }

    if (definition.outline_level.has_value() &&
        *definition.outline_level > 8U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style outline level must be between 0 and 8",
                       std::string{styles_xml_entry});
        return false;
    }
    if (definition.run_text_color.has_value() &&
        definition.run_text_color->empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style run text color must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }
    if (definition.run_font_size_points.has_value() &&
        *definition.run_font_size_points <= 0.0) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "paragraph style run font size points must be greater "
                       "than zero",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    bool reset_inherited_run_properties = false;
    if (const auto existing = find_style_node(styles_root, style_id);
        existing != pugi::xml_node{}) {
        const auto existing_type =
            std::string_view{existing.attribute("w:type").value()};
        if (!existing_type.empty() && existing_type != "paragraph") {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{style_id} +
                               "' already exists with type '" +
                               std::string{existing_type} + "'",
                           std::string{styles_xml_entry});
            return false;
        }

        if (definition.based_on.has_value()) {
            const auto existing_based_on =
                std::string_view{
                    existing.child("w:basedOn").attribute("w:val").value()};
            reset_inherited_run_properties =
                existing_based_on !=
                std::string_view{definition.based_on->data(),
                                 definition.based_on->size()};
        }
    }

    const auto style = ensure_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create paragraph style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!apply_common_style_definition(
            style, style_id, "paragraph", definition.name, definition.based_on,
            definition.is_custom, definition.is_semi_hidden,
            definition.is_unhide_when_used, definition.is_quick_format) ||
        !set_style_string_node(style, "w:next", definition.next_style) ||
        !set_style_outline_level(style, definition.outline_level)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update paragraph style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;

    if (definition.run_text_color.has_value() &&
        (!this->clear_style_run_text_color(style_id) ||
         !this->set_style_run_text_color(style_id,
                                         *definition.run_text_color))) {
        return false;
    }
    if (definition.run_bold.has_value() &&
        (!this->clear_style_run_bold(style_id) ||
         !this->set_style_run_bold(style_id, *definition.run_bold))) {
        return false;
    }
    if (definition.run_italic.has_value() &&
        (!this->clear_style_run_italic(style_id) ||
         !this->set_style_run_italic(style_id, *definition.run_italic))) {
        return false;
    }
    if (definition.run_strikethrough.has_value() &&
        (!this->clear_style_run_strikethrough(style_id) ||
         !this->set_style_run_strikethrough(
             style_id, *definition.run_strikethrough))) {
        return false;
    }
    if (definition.run_underline.has_value() &&
        (!this->clear_style_run_underline(style_id) ||
         !this->set_style_run_underline(style_id,
                                        *definition.run_underline))) {
        return false;
    }
    if (definition.run_font_size_points.has_value() &&
        (!this->clear_style_run_font_size_points(style_id) ||
         !this->set_style_run_font_size_points(
             style_id, *definition.run_font_size_points))) {
        return false;
    }

    if (reset_inherited_run_properties ||
        definition.run_font_family.has_value() ||
        definition.run_east_asia_font_family.has_value()) {
        if (!this->clear_style_run_font_family(style_id)) {
            return false;
        }
        if (definition.run_font_family.has_value() &&
            !this->set_style_run_font_family(style_id,
                                             *definition.run_font_family)) {
            return false;
        }
        if (definition.run_east_asia_font_family.has_value() &&
            !this->set_style_run_east_asia_font_family(
                style_id, *definition.run_east_asia_font_family)) {
            return false;
        }
    }

    if (reset_inherited_run_properties || definition.run_language.has_value() ||
        definition.run_east_asia_language.has_value() ||
        definition.run_bidi_language.has_value()) {
        if (!this->clear_style_run_language(style_id)) {
            return false;
        }
        if (definition.run_language.has_value() &&
            !this->set_style_run_language(style_id, *definition.run_language)) {
            return false;
        }
        if (definition.run_east_asia_language.has_value() &&
            !this->set_style_run_east_asia_language(
                style_id, *definition.run_east_asia_language)) {
            return false;
        }
        if (definition.run_bidi_language.has_value() &&
            !this->set_style_run_bidi_language(
                style_id, *definition.run_bidi_language)) {
            return false;
        }
    }

    if (definition.run_rtl.has_value()) {
        if (!this->set_style_run_rtl(style_id, *definition.run_rtl)) {
            return false;
        }
    } else if (reset_inherited_run_properties &&
               !this->clear_style_run_rtl(style_id)) {
        return false;
    }

    if (definition.paragraph_bidi.has_value()) {
        if (!this->set_style_paragraph_bidi(style_id,
                                            *definition.paragraph_bidi)) {
            return false;
        }
    }

    this->last_error_info.clear();
    return true;
}

bool Document::ensure_character_style(
    std::string_view style_id,
    const featherdoc::character_style_definition &definition) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before defining character styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when defining character styles",
            std::string{styles_xml_entry});
        return false;
    }

    if (definition.name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "character style name must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!validate_optional_style_value(
            this->last_error_info, definition.based_on,
            "character style basedOn must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_font_family,
            "character style run font family must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_font_family,
            "character style eastAsia font family must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_language,
            "character style run language must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_east_asia_language,
            "character style eastAsia language must not be empty") ||
        !validate_optional_style_value(
            this->last_error_info, definition.run_bidi_language,
            "character style bidi language must not be empty")) {
        return false;
    }
    if (definition.run_text_color.has_value() &&
        definition.run_text_color->empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "character style run text color must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }
    if (definition.run_font_size_points.has_value() &&
        *definition.run_font_size_points <= 0.0) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "character style run font size points must be greater "
                       "than zero",
                       std::string{styles_xml_entry});
        return false;
    }
    if (definition.run_superscript.value_or(false) &&
        definition.run_subscript.value_or(false)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "character style run superscript and subscript cannot "
                       "both be true",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    bool reset_inherited_run_properties = false;
    if (const auto existing = find_style_node(styles_root, style_id);
        existing != pugi::xml_node{}) {
        const auto existing_type =
            std::string_view{existing.attribute("w:type").value()};
        if (!existing_type.empty() && existing_type != "character") {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{style_id} +
                               "' already exists with type '" +
                               std::string{existing_type} + "'",
                           std::string{styles_xml_entry});
            return false;
        }

        if (definition.based_on.has_value()) {
            const auto existing_based_on =
                std::string_view{
                    existing.child("w:basedOn").attribute("w:val").value()};
            reset_inherited_run_properties =
                existing_based_on !=
                std::string_view{definition.based_on->data(),
                                 definition.based_on->size()};
        }
    }

    const auto style = ensure_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create character style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!apply_common_style_definition(
            style, style_id, "character", definition.name, definition.based_on,
            definition.is_custom, definition.is_semi_hidden,
            definition.is_unhide_when_used, definition.is_quick_format)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update character style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;

    if (definition.run_text_color.has_value() &&
        (!this->clear_style_run_text_color(style_id) ||
         !this->set_style_run_text_color(style_id,
                                         *definition.run_text_color))) {
        return false;
    }
    if (definition.run_bold.has_value() &&
        (!this->clear_style_run_bold(style_id) ||
         !this->set_style_run_bold(style_id, *definition.run_bold))) {
        return false;
    }
    if (definition.run_italic.has_value() &&
        (!this->clear_style_run_italic(style_id) ||
         !this->set_style_run_italic(style_id, *definition.run_italic))) {
        return false;
    }
    if (definition.run_strikethrough.has_value() &&
        (!this->clear_style_run_strikethrough(style_id) ||
         !this->set_style_run_strikethrough(
             style_id, *definition.run_strikethrough))) {
        return false;
    }
    if (definition.run_underline.has_value() &&
        (!this->clear_style_run_underline(style_id) ||
         !this->set_style_run_underline(style_id,
                                        *definition.run_underline))) {
        return false;
    }
    if (definition.run_superscript.has_value() ||
        definition.run_subscript.has_value()) {
        if (!this->clear_style_run_superscript(style_id)) {
            return false;
        }
        if (definition.run_superscript.value_or(false)) {
            if (!this->set_style_run_superscript(style_id, true)) {
                return false;
            }
        } else if (definition.run_subscript.value_or(false)) {
            if (!this->set_style_run_subscript(style_id, true)) {
                return false;
            }
        } else if (!this->set_style_run_superscript(style_id, false)) {
            return false;
        }
    }
    if (definition.run_font_size_points.has_value() &&
        (!this->clear_style_run_font_size_points(style_id) ||
         !this->set_style_run_font_size_points(
             style_id, *definition.run_font_size_points))) {
        return false;
    }

    if (reset_inherited_run_properties ||
        definition.run_font_family.has_value() ||
        definition.run_east_asia_font_family.has_value()) {
        if (!this->clear_style_run_font_family(style_id)) {
            return false;
        }
        if (definition.run_font_family.has_value() &&
            !this->set_style_run_font_family(style_id,
                                             *definition.run_font_family)) {
            return false;
        }
        if (definition.run_east_asia_font_family.has_value() &&
            !this->set_style_run_east_asia_font_family(
                style_id, *definition.run_east_asia_font_family)) {
            return false;
        }
    }

    if (reset_inherited_run_properties || definition.run_language.has_value() ||
        definition.run_east_asia_language.has_value() ||
        definition.run_bidi_language.has_value()) {
        if (!this->clear_style_run_language(style_id)) {
            return false;
        }
        if (definition.run_language.has_value() &&
            !this->set_style_run_language(style_id, *definition.run_language)) {
            return false;
        }
        if (definition.run_east_asia_language.has_value() &&
            !this->set_style_run_east_asia_language(
                style_id, *definition.run_east_asia_language)) {
            return false;
        }
        if (definition.run_bidi_language.has_value() &&
            !this->set_style_run_bidi_language(
                style_id, *definition.run_bidi_language)) {
            return false;
        }
    }

    if (definition.run_rtl.has_value()) {
        if (!this->set_style_run_rtl(style_id, *definition.run_rtl)) {
            return false;
        }
    } else if (reset_inherited_run_properties &&
               !this->clear_style_run_rtl(style_id)) {
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::ensure_table_style(
    std::string_view style_id,
    const featherdoc::table_style_definition &definition) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before defining table styles");
        return false;
    }

    if (style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id must not be empty when defining table styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (definition.name.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "table style name must not be empty",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!validate_optional_style_value(
            this->last_error_info, definition.based_on,
            "table style basedOn must not be empty")) {
        return false;
    }

    if (!detail::validate_table_style_regions(this->last_error_info, definition)) {
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto existing = find_style_node(styles_root, style_id);
        existing != pugi::xml_node{}) {
        const auto existing_type =
            std::string_view{existing.attribute("w:type").value()};
        if (!existing_type.empty() && existing_type != "table") {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{style_id} +
                               "' already exists with type '" +
                               std::string{existing_type} + "'",
                           std::string{styles_xml_entry});
            return false;
        }
    }

    const auto style = ensure_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create table style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!apply_common_style_definition(
            style, style_id, "table", definition.name, definition.based_on,
            definition.is_custom, definition.is_semi_hidden,
            definition.is_unhide_when_used, definition.is_quick_format)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update table style '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    if (!detail::apply_table_style_regions(style, definition)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to update table style properties for '" +
                           std::string{style_id} + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}
