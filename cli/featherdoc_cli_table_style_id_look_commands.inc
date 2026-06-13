auto run_set_table_style_id_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-table-style-id expects an input path, a table index, and a style id",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_index(arguments[2], table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[3]);
    if (style_id.empty()) {
        print_parse_error(command, "style id must not be empty", json_output);
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 4U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.set_style_id(style_id)) {
        report_invalid_table_style_target(command, "failed to set table style id",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, &style_id](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index << ",\"style_id\":";
                write_json_string(stream, style_id);
            });
    }

    return 0;
}

auto run_clear_table_style_id_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "clear-table-style-id expects an input path and a table index",
                          json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_index(arguments[2], table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.clear_style_id()) {
        report_invalid_table_style_target(command,
                                          "failed to clear table style id",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(command, doc, options.output_path,
                                   [table_index](std::ostream &stream) {
                                       stream << ",\"table_index\":"
                                              << table_index;
                                   });
    }

    return 0;
}

auto run_set_table_style_look_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-table-style-look expects an input path, a table index, "
            "and at least one style-look flag",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_index(arguments[2], table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    table_style_look_options options;
    std::string error_message;
    if (!parse_table_style_look_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    if (!table_style_look_options_have_flag(options)) {
        print_parse_error(command,
                          "set-table-style-look requires at least one "
                          "style-look flag",
                          json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    auto style_look =
        table.style_look().value_or(featherdoc::table_style_look{});
    if (options.first_row.has_value()) {
        style_look.first_row = *options.first_row;
    }
    if (options.last_row.has_value()) {
        style_look.last_row = *options.last_row;
    }
    if (options.first_column.has_value()) {
        style_look.first_column = *options.first_column;
    }
    if (options.last_column.has_value()) {
        style_look.last_column = *options.last_column;
    }
    if (options.banded_rows.has_value()) {
        style_look.banded_rows = *options.banded_rows;
    }
    if (options.banded_columns.has_value()) {
        style_look.banded_columns = *options.banded_columns;
    }

    if (!table.set_style_look(style_look)) {
        report_invalid_table_style_target(command,
                                          "failed to set table style look",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, &style_look](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"style_look\":";
                write_json_table_style_look(stream, style_look);
            });
    }

    return 0;
}

auto run_clear_table_style_look_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command, "clear-table-style-look expects an input path and a table index",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_index(arguments[2], table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.clear_style_look()) {
        report_invalid_table_style_target(command,
                                          "failed to clear table style look",
                                          options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(command, doc, options.output_path,
                                   [table_index](std::ostream &stream) {
                                       stream << ",\"table_index\":"
                                              << table_index;
                                   });
    }

    return 0;
}
