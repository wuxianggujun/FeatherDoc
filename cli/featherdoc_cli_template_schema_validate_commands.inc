    if (command == "validate-template") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "validate-template expects an input path and validation options",
                json_output);
            return 2;
        }

        validate_template_options options;
        std::string error_message;
        if (!parse_validate_template_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "inspect", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto result = selected.part.validate_template(options.requirements);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, options.json_output);
            return 1;
        }

        print_template_validation_result(selected, result, options.json_output);
        return 0;
    }

    if (command == "validate-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "validate-template-schema expects an input path and schema validation options",
                json_output);
            return 2;
        }

        validate_template_schema_options options;
        std::string error_message;
        if (!parse_validate_template_schema_options(arguments, 2U, options,
                                                    error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        for (const auto &schema_file : options.schema_files) {
            if (!append_validate_template_schema_file_targets(
                    schema_file, options.targets, error_message)) {
                print_parse_error(command, error_message, json_output);
                return 2;
            }
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::template_schema schema;
        std::size_t entry_count = 0U;
        for (const auto &target : options.targets) {
            entry_count += target.requirements.size();
        }
        schema.entries.reserve(entry_count);

        for (const auto &target : options.targets) {
            featherdoc::template_schema_part_selector selector{};
            selector.part = to_template_schema_part_kind(target.part);
            selector.part_index = target.part_index;
            selector.section_index = target.section_index;
            selector.reference_kind = target.reference_kind;

            for (const auto &requirement : target.requirements) {
                schema.entries.push_back({selector, requirement});
            }
        }

        const auto result = doc.validate_template_schema(schema);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, options.json_output);
            return 1;
        }

        print_template_schema_validation_result(result, options.json_output);
        return 0;
    }

    if (command == "export-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "export-template-schema expects an input path",
                              json_output);
            return 2;
        }

        export_template_schema_options options;
        std::string error_message;
        if (!parse_export_template_schema_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        exported_template_schema_result result;
        if (!build_exported_template_schema(
                doc, options.section_targets, options.resolved_section_targets, result,
                command, options.json_output)) {
            return 1;
        }

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
                                     "failed to write template schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_exported_template_schema_summary(result, options.output_path,
                                               options.json_output);
        return 0;
    }
