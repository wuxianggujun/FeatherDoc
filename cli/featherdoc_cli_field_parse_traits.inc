enum class field_option_parse_result { handled, unhandled, error };

struct append_field_command_traits {
    bool is_generic = false;
    bool is_page_number = false;
    bool is_total_pages = false;
    bool is_table_of_contents = false;
    bool is_page_reference = false;
    bool is_style_reference = false;
    bool is_document_property = false;
    bool is_date = false;
    bool is_hyperlink = false;
    bool is_reference = false;
    bool is_sequence = false;
    bool is_caption = false;
    bool is_index_entry = false;
    bool is_index = false;
    bool is_complex = false;
    bool is_replace = false;
    bool requires_field_argument = false;
    bool is_typed_field = false;
    bool supports_result_text = false;
    bool supports_field_state = false;
    bool supports_preserve_formatting = false;
};

auto make_append_field_command_traits(std::string_view command)
    -> append_field_command_traits {
    append_field_command_traits traits;
    traits.is_generic = command == "append-field";
    traits.is_page_number = command == "append-page-number-field";
    traits.is_total_pages = command == "append-total-pages-field";
    traits.is_table_of_contents = command == "append-table-of-contents-field";
    traits.is_page_reference = command == "append-page-reference-field";
    traits.is_style_reference = command == "append-style-reference-field";
    traits.is_document_property = command == "append-document-property-field";
    traits.is_date = command == "append-date-field";
    traits.is_hyperlink = command == "append-hyperlink-field";
    traits.is_reference = command == "append-reference-field";
    traits.is_sequence = command == "append-sequence-field";
    traits.is_caption = command == "append-caption";
    traits.is_index_entry = command == "append-index-entry-field";
    traits.is_index = command == "append-index-field";
    traits.is_complex = command == "append-complex-field";
    traits.is_replace = command == "replace-field";
    traits.requires_field_argument =
        traits.is_generic || traits.is_reference || traits.is_sequence ||
        traits.is_page_reference || traits.is_style_reference ||
        traits.is_document_property || traits.is_hyperlink || traits.is_caption ||
        traits.is_index_entry;
    traits.is_typed_field = traits.requires_field_argument || traits.is_date ||
                            traits.is_index || traits.is_complex;
    traits.supports_result_text = traits.is_table_of_contents ||
                                  traits.is_typed_field || traits.is_replace;
    traits.supports_field_state =
        traits.is_page_number || traits.is_total_pages ||
        traits.is_table_of_contents || traits.is_typed_field;
    traits.supports_preserve_formatting =
        traits.is_reference || traits.is_page_reference ||
        traits.is_style_reference || traits.is_document_property ||
        traits.is_date || traits.is_hyperlink || traits.is_sequence ||
        traits.is_caption || traits.is_index;
    return traits;
}
