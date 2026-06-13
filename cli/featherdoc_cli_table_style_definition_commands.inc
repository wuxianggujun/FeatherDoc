auto run_ensure_table_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "ensure-table-style expects an input path and a style id",
                          json_output);
        return 2;
    }

    const auto style_id = arguments[2];
    ensure_table_style_options options;
    std::string error_message;
    if (!parse_ensure_table_style_options(arguments, 3U, options,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!doc.ensure_table_style(style_id, options.definition)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    const auto style = doc.find_style(style_id);
    if (!style.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    print_style_mutation_result(command, doc, options.output_path, *style,
                                options.json_output);
    return 0;
}

auto run_inspect_table_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "inspect-table-style expects an input path and a style id",
                          json_output);
        return 2;
    }

    inspect_options options;
    std::string error_message;
    if (!parse_inspect_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto definition = doc.find_table_style_definition(arguments[2]);
    if (!definition.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }

    inspect_table_style_definition(*definition, options.json_output);
    return 0;
}
