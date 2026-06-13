auto is_append_field_command(std::string_view command) -> bool {
    return command == "append-field" ||
           command == "append-page-number-field" ||
           command == "append-total-pages-field" ||
           command == "append-table-of-contents-field" ||
           command == "append-reference-field" ||
           command == "append-page-reference-field" ||
           command == "append-style-reference-field" ||
           command == "append-document-property-field" ||
           command == "append-date-field" ||
           command == "append-hyperlink-field" ||
           command == "append-sequence-field" ||
           command == "append-caption" ||
           command == "append-index-field" ||
           command == "append-index-entry-field" ||
           command == "append-complex-field" ||
           command == "replace-field";
}

auto template_field_name(std::string_view command) -> const char * {
    if (command == "append-field") {
        return "field";
    }
    if (command == "append-page-number-field") {
        return "page_number";
    }
    if (command == "append-total-pages-field") {
        return "total_pages";
    }
    if (command == "append-table-of-contents-field") {
        return "table_of_contents";
    }
    if (command == "append-reference-field") {
        return "reference";
    }
    if (command == "append-page-reference-field") {
        return "page_reference";
    }
    if (command == "append-style-reference-field") {
        return "style_reference";
    }
    if (command == "append-document-property-field") {
        return "document_property";
    }
    if (command == "append-date-field") {
        return "date";
    }
    if (command == "append-hyperlink-field") {
        return "hyperlink";
    }
    if (command == "append-sequence-field") {
        return "sequence";
    }
    if (command == "append-caption") {
        return "caption";
    }
    if (command == "append-index-field") {
        return "index";
    }
    if (command == "append-index-entry-field") {
        return "index_entry";
    }
    if (command == "append-complex-field") {
        return "complex";
    }
    if (command == "replace-field") {
        return "replacement";
    }
    return "field";
}
