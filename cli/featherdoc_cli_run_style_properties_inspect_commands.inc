auto run_inspect_style_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-style-run-properties expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    inspect_style_run_properties_options options;
    std::string error_message;
    if (!parse_inspect_style_run_properties_options(arguments, 3U, options,
                                                    error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    run_properties_summary summary{};
    summary.font_family = doc.style_run_font_family(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.east_asia_font_family = doc.style_run_east_asia_font_family(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.language = doc.style_run_language(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.east_asia_language = doc.style_run_east_asia_language(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.bidi_language = doc.style_run_bidi_language(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.rtl = doc.style_run_rtl(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.paragraph_bidi = doc.style_paragraph_bidi(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    if (options.json_output) {
        std::cout << "{\"style_id\":";
        write_json_string(std::cout, style_id);
        std::cout << ",\"style_run_properties\":";
        write_json_run_properties_summary(std::cout, summary);
        std::cout << "}\n";
    } else {
        std::cout << "style_id: " << style_id << '\n';
        print_run_properties_summary(std::cout, summary);
    }

    return 0;
}

auto run_inspect_paragraph_style_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-paragraph-style-properties expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    inspect_paragraph_style_properties_options options;
    std::string error_message;
    if (!parse_inspect_paragraph_style_properties_options(arguments, 3U, options,
                                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    paragraph_style_properties_summary summary{};
    const auto style = doc.find_style(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }
    if (!style.has_value()) {
        report_operation_failure(
            command, "inspect", "operation failed",
            featherdoc::document_error_info{
                std::make_error_code(std::errc::invalid_argument),
                "style id '" + style_id + "' was not found in word/styles.xml",
                "word/styles.xml", std::nullopt},
            options.json_output);
        return 1;
    }

    if (style->kind != featherdoc::style_kind::paragraph) {
        report_operation_failure(
            command, "inspect", "operation failed",
            featherdoc::document_error_info{
                std::make_error_code(std::errc::invalid_argument),
                "style id '" + style_id + "' is not a paragraph style",
                "word/styles.xml", std::nullopt},
            options.json_output);
        return 1;
    }
    summary.based_on = style->based_on;

    summary.next_style = doc.paragraph_style_next_style(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    summary.outline_level = doc.paragraph_style_outline_level(style_id);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    if (options.json_output) {
        std::cout << "{\"style_id\":";
        write_json_string(std::cout, style_id);
        std::cout << ",\"paragraph_style_properties\":";
        write_json_paragraph_style_properties_summary(std::cout, summary);
        std::cout << "}\n";
    } else {
        std::cout << "style_id: " << style_id << '\n';
        print_paragraph_style_properties_summary(std::cout, summary);
    }

    return 0;
}

auto run_materialize_style_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "materialize-style-run-properties expects an input path and a style id",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    materialize_style_run_properties_options options;
    std::string error_message;
    if (!parse_materialize_style_run_properties_options(arguments, 3U, options,
                                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    const auto resolved =
        resolve_style_properties_for_command(command, doc, style_id,
                                             options.json_output);
    if (!resolved.has_value()) {
        return 1;
    }

    const auto materialized_properties =
        collect_materialized_style_run_properties(style_id, *resolved);

    if (!doc.materialize_style_run_properties(style_id)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&style_id, &materialized_properties](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"materialized\":";
                write_json_materialized_style_run_properties(stream,
                                                             materialized_properties);
            });
    }

    return 0;
}

