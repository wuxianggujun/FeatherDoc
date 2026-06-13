    if (command == "normalize-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "normalize-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        normalize_template_schema_options options;
        std::string error_message;
        if (!parse_normalize_template_schema_options(arguments, 2U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write normalized schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_normalized_template_schema_summary(result, options.output_path,
                                                 options.json_output);
        return 0;
    }

    if (command == "lint-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "lint-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        lint_template_schema_options options;
        std::string error_message;
        if (!parse_lint_template_schema_options(arguments, 2U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        const auto lint = lint_template_schema(result);
        print_linted_template_schema_result(lint, options.json_output);
        if (!lint.clean()) {
            return 1;
        }
        return 0;
    }

    if (command == "repair-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "repair-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        repair_template_schema_options options;
        std::string error_message;
        if (!parse_repair_template_schema_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result input;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), input,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result result;
        repaired_template_schema_summary summary{};
        repair_template_schema_result(input, result, summary);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write repaired schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_repaired_template_schema_summary(result, summary, options.output_path,
                                               options.json_output);
        return 0;
    }

    if (command == "merge-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "merge-template-schema expects at least two schema paths",
                              json_output);
            return 2;
        }

        merge_template_schema_options options;
        std::string error_message;
        if (!parse_merge_template_schema_options(arguments, 1U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        merged_template_schema_summary summary;
        for (const auto &schema_path : options.schema_paths) {
            exported_template_schema_result input;
            if (!read_template_schema_file(schema_path, input, error_message)) {
                print_parse_error(command, error_message, options.json_output);
                return 2;
            }
            merge_template_schema_result(result, input, summary);
        }
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write merged schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_merged_template_schema_summary(result, summary, options.output_path,
                                             options.json_output);
        return 0;
    }
