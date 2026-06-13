    if (command == "diff-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "diff-template-schema expects left and right schema paths",
                              json_output);
            return 2;
        }

        diff_template_schema_options options;
        std::string error_message;
        if (!parse_diff_template_schema_options(arguments, 3U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result left;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), left,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result right;
        if (!read_template_schema_file(path_type(std::string(arguments[2])), right,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        normalize_template_schema_result(left);
        normalize_template_schema_result(right);
        const auto result = diff_template_schema_results(left, right);
        print_template_schema_diff_result(result, options.json_output);
        if (options.fail_on_diff && !result.equal()) {
            return 1;
        }
        return 0;
    }

    if (command == "check-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "check-template-schema expects an input path and --schema-file <path>",
                json_output);
            return 2;
        }

        check_template_schema_options options;
        std::string error_message;
        if (!parse_check_template_schema_options(arguments, 2U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result baseline;
        if (!read_template_schema_file(*options.schema_path, baseline, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(baseline);

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        exported_template_schema_result generated;
        if (!build_exported_template_schema(
                doc, options.section_targets, options.resolved_section_targets,
                generated, command, options.json_output)) {
            return 1;
        }
        normalize_template_schema_result(generated);

        if (options.output_path.has_value() &&
            !write_exported_template_schema_file(*options.output_path, generated,
                                                error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write generated schema output",
                                     error_info, options.json_output);
            return 1;
        }

        const auto result = diff_template_schema_results(baseline, generated);
        print_checked_template_schema_result(*options.schema_path, result,
                                             options.output_path,
                                             options.json_output);
        if (!result.equal()) {
            return 1;
        }
        return 0;
    }
