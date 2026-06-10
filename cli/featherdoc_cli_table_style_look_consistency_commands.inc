auto run_check_table_style_look_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "check-table-style-look expects an input path",
                          json_output);
        return 2;
    }

    check_table_style_look_options options;
    std::string error_message;
    if (!parse_check_table_style_look_options(arguments, 2U, options,
                                              error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto input_path = path_type(std::string(arguments[1]));
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    const auto report = doc.check_table_style_look_consistency();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "check", error_info, options.json_output);
        return 1;
    }

    check_table_style_look(report, options.fail_on_issue, options.json_output);
    return report.ok() || !options.fail_on_issue ? 0 : 1;
}

auto run_repair_table_style_look_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "repair-table-style-look expects an input path",
                          json_output);
        return 2;
    }

    repair_table_style_look_options options;
    std::string error_message;
    if (!parse_repair_table_style_look_options(arguments, 2U, options,
                                               error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto input_path = path_type(std::string(arguments[1]));
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    auto result = table_style_look_repair_cli_result{};
    result.apply = options.apply;
    result.output_path = options.output_path;
    if (options.apply) {
        const auto repair_report = doc.repair_table_style_look_consistency();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "repair", error_info,
                                  options.json_output);
            return 1;
        }
        result.before = repair_report.before;
        result.after = repair_report.after;
        result.changed_table_count = repair_report.changed_table_count;
        result.applicable_issue_count =
            count_applicable_table_style_look_issues(result.before);
        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }
    } else {
        result.before = doc.check_table_style_look_consistency();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "check", error_info,
                                  options.json_output);
            return 1;
        }
        result.applicable_issue_count =
            count_applicable_table_style_look_issues(result.before);
    }

    repair_table_style_look(result, options.json_output);
    return 0;
}
