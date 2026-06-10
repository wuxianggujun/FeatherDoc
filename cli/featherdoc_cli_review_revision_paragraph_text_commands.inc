    if (command == "insert-paragraph-text-revision" ||
        command == "delete-paragraph-text-revision" ||
        command == "replace-paragraph-text-revision") {
        const auto json_output = has_json_flag(arguments);
        const bool insert_revision = command == "insert-paragraph-text-revision";
        if (arguments.size() < (insert_revision ? 4U : 5U)) {
            print_parse_error(
                command,
                insert_revision
                    ? std::string(command) +
                          " expects an input path, paragraph index, and text offset"
                    : std::string(command) +
                          " expects an input path, paragraph index, text offset, and text length",
                json_output);
            return 2;
        }

        std::size_t paragraph_index = 0U;
        if (!parse_index(arguments[2], paragraph_index)) {
            print_parse_error(command,
                              "invalid paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t text_offset = 0U;
        if (!parse_index(arguments[3], text_offset)) {
            print_parse_error(command,
                              "invalid text offset: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t text_length = 0U;
        if (!insert_revision && !parse_index(arguments[4], text_length)) {
            print_parse_error(command,
                              "invalid text length: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-paragraph-text-revision";
        const auto options_start = insert_revision ? 4U : 5U;
        if (!parse_revision_authoring_options(
                arguments, options_start, options, error_message, require_text,
                "delete-paragraph-text-revision does not accept --text",
                !insert_revision,
                "insert-paragraph-text-revision does not accept --expected-text")) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!insert_revision) {
            if (text_length >
                std::numeric_limits<std::size_t>::max() - text_offset) {
                featherdoc::document_error_info error_info{};
                error_info.code =
                    std::make_error_code(std::errc::invalid_argument);
                error_info.entry_name = "word/document.xml";
                error_info.detail = "text range is out of range";
                report_operation_failure(command, "validate",
                                         "text range is out of range",
                                         error_info, options.json_output);
                return 1;
            }

            if (!validate_revision_expected_text(
                    doc, command, options, paragraph_index, text_offset,
                    paragraph_index, text_offset + text_length)) {
                return 1;
            }
        }

        bool ok = false;
        if (insert_revision) {
            ok = doc.insert_paragraph_text_revision(
                paragraph_index, text_offset, options.text, options.author,
                options.date);
        } else if (command == "delete-paragraph-text-revision") {
            ok = doc.delete_paragraph_text_revision(
                paragraph_index, text_offset, text_length, options.author,
                options.date);
        } else {
            ok = doc.replace_paragraph_text_revision(
                paragraph_index, text_offset, text_length, options.text,
                options.author, options.date);
        }
        if (!ok) {
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
                [insert_revision, paragraph_index, text_offset,
                 text_length](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"paragraph_index\":" << paragraph_index
                           << ",\"text_offset\":" << text_offset;
                    if (!insert_revision) {
                        stream << ",\"text_length\":" << text_length;
                    }
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }
