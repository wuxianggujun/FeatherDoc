auto validate_append_field_options(std::string_view command,
                                   const append_field_command_traits &traits,
                                   const append_field_options &options,
                                   std::string &error_message) -> bool {
    if (traits.requires_field_argument && !options.has_field_argument) {
        if (traits.is_generic) {
            error_message = "append-field expects <instruction>";
        } else if (traits.is_reference) {
            error_message = "append-reference-field expects <bookmark-name>";
        } else if (traits.is_sequence) {
            error_message = "append-sequence-field expects <identifier>";
        } else if (traits.is_page_reference) {
            error_message = "append-page-reference-field expects <bookmark-name>";
        } else if (traits.is_style_reference) {
            error_message = "append-style-reference-field expects <style-name>";
        } else if (traits.is_document_property) {
            error_message = "append-document-property-field expects <property-name>";
        } else if (traits.is_hyperlink) {
            error_message = "append-hyperlink-field expects <target>";
        } else if (traits.is_caption) {
            error_message = "append-caption expects <label>";
        } else {
            error_message = "append-index-entry-field expects <entry-text>";
        }
        return false;
    }

    if (traits.is_caption && !options.has_caption_text) {
        error_message = "append-caption requires --text <caption-text>";
        return false;
    }

    if (traits.is_complex && !validate_complex_field_options(options, error_message)) {
        return false;
    }

    if (traits.is_table_of_contents &&
        options.min_outline_level > options.max_outline_level) {
        error_message = "TOC minimum outline level must be less than or equal to maximum outline level";
        return false;
    }

    if (traits.is_replace) {
        if (!options.has_field_index) {
            error_message = "replace-field expects <field-index>";
            return false;
        }
        if (!options.has_field_argument) {
            error_message = "replace-field expects <instruction>";
            return false;
        }
    }

    if (!traits.requires_field_argument && !traits.is_replace &&
        options.has_field_argument) {
        error_message = std::string(command) +
                        " does not accept a field argument";
        return false;
    }

    if (!options.has_part) {
        error_message = "missing --part <body|header|footer|section-header|section-footer>";
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "mutation", error_message);
}
