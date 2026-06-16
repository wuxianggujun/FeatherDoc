auto run_audit_table_style_regions_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "audit-table-style-regions expects an input path",
                          json_output);
        return 2;
    }

    audit_table_style_regions_options options;
    std::string error_message;
    if (!parse_audit_table_style_regions_options(arguments, 2U, options,
                                                 error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto report = doc.audit_table_style_regions(
        options.style_id.has_value()
            ? std::optional<std::string_view>{std::string_view{*options.style_id}}
            : std::optional<std::string_view>{});
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "audit", error_info, options.json_output);
        return 1;
    }

    audit_table_style_regions(report, options.style_id, options.fail_on_issue,
                              options.json_output);
    return report.ok() || !options.fail_on_issue ? 0 : 1;
}

auto run_audit_table_style_inheritance_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "audit-table-style-inheritance expects an input path",
                          json_output);
        return 2;
    }

    audit_table_style_inheritance_options options;
    std::string error_message;
    if (!parse_audit_table_style_inheritance_options(arguments, 2U, options,
                                                     error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto report = doc.audit_table_style_inheritance(
        options.style_id.has_value()
            ? std::optional<std::string_view>{std::string_view{*options.style_id}}
            : std::optional<std::string_view>{});
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "audit", error_info, options.json_output);
        return 1;
    }

    audit_table_style_inheritance(report, options.style_id,
                                  options.fail_on_issue, options.json_output);
    return report.ok() || !options.fail_on_issue ? 0 : 1;
}

auto run_audit_table_style_quality_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "audit-table-style-quality expects an input path",
                          json_output);
        return 2;
    }

    audit_table_style_quality_options options;
    std::string error_message;
    if (!parse_audit_table_style_quality_options(arguments, 2U, options,
                                                 error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto report = doc.audit_table_style_quality();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "audit", error_info, options.json_output);
        return 1;
    }

    audit_table_style_quality(report, options.fail_on_issue, options.json_output);
    return report.ok() || !options.fail_on_issue ? 0 : 1;
}

auto run_plan_table_style_quality_fixes_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(
            command, "plan-table-style-quality-fixes expects an input path",
            json_output);
        return 2;
    }

    plan_table_style_quality_fixes_options options;
    std::string error_message;
    if (!parse_plan_table_style_quality_fixes_options(arguments, 2U, options,
                                                      error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto plan = doc.plan_table_style_quality_fixes();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "plan", error_info, options.json_output);
        return 1;
    }

    plan_table_style_quality_fixes(plan, options.fail_on_issue,
                                   options.json_output);
    return plan.ok() || !options.fail_on_issue ? 0 : 1;
}

auto run_apply_table_style_quality_fixes_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(
            command, "apply-table-style-quality-fixes expects an input path",
            json_output);
        return 2;
    }

    apply_table_style_quality_fixes_options options;
    std::string error_message;
    if (!parse_apply_table_style_quality_fixes_options(arguments, 2U, options,
                                                       error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    auto result = table_style_quality_apply_cli_result{};
    result.output_path = options.output_path;
    result.before = doc.plan_table_style_quality_fixes();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "plan", error_info, options.json_output);
        return 1;
    }

    const auto repair_report = doc.repair_table_style_look_consistency();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "repair", error_info, options.json_output);
        return 1;
    }
    result.changed_table_count = repair_report.changed_table_count;
    result.after = doc.plan_table_style_quality_fixes();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "plan", error_info, options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    apply_table_style_quality_fixes(result, options.json_output);
    return 0;
}
