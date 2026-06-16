    if (command == "insert-text-range-revision" ||
        command == "delete-text-range-revision" ||
        command == "replace-text-range-revision") {
        const auto json_output = has_json_flag(arguments);
        const bool insert_revision = command == "insert-text-range-revision";
        if (arguments.size() < (insert_revision ? 4U : 6U)) {
            print_parse_error(
                command,
                insert_revision
                    ? std::string(command) +
                          " expects an input path, paragraph index, and text offset"
                    : std::string(command) +
                          " expects an input path, start paragraph index, start offset, end paragraph index, and end offset",
                json_output);
            return 2;
        }

        std::size_t start_paragraph_index = 0U;
        if (!parse_index(arguments[2], start_paragraph_index)) {
            print_parse_error(command,
                              "invalid start paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t start_text_offset = 0U;
        if (!parse_index(arguments[3], start_text_offset)) {
            print_parse_error(command,
                              "invalid start text offset: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t end_paragraph_index = start_paragraph_index;
        std::size_t end_text_offset = start_text_offset;
        if (!insert_revision) {
            if (!parse_index(arguments[4], end_paragraph_index)) {
                print_parse_error(command,
                                  "invalid end paragraph index: " +
                                      std::string(arguments[4]),
                                  json_output);
                return 2;
            }
            if (!parse_index(arguments[5], end_text_offset)) {
                print_parse_error(command,
                                  "invalid end text offset: " +
                                      std::string(arguments[5]),
                                  json_output);
                return 2;
            }
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-text-range-revision";
        const auto options_start = insert_revision ? 4U : 6U;
        if (!parse_revision_authoring_options(
                arguments, options_start, options, error_message, require_text,
                "delete-text-range-revision does not accept --text",
                !insert_revision,
                "insert-text-range-revision does not accept --expected-text")) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!insert_revision &&
            !validate_revision_expected_text(
                doc, command, options, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset)) {
            return 1;
        }

        bool ok = false;
        if (insert_revision) {
            ok = doc.insert_text_range_revision(
                start_paragraph_index, start_text_offset, options.text,
                options.author, options.date);
        } else if (command == "delete-text-range-revision") {
            ok = doc.delete_text_range_revision(
                start_paragraph_index, start_text_offset, end_paragraph_index,
                end_text_offset, options.author, options.date);
        } else {
            ok = doc.replace_text_range_revision(
                start_paragraph_index, start_text_offset, end_paragraph_index,
                end_text_offset, options.text, options.author, options.date);
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
                [insert_revision, start_paragraph_index, start_text_offset,
                 end_paragraph_index, end_text_offset](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"start_paragraph_index\":"
                           << start_paragraph_index
                           << ",\"start_text_offset\":" << start_text_offset;
                    if (!insert_revision) {
                        stream << ",\"end_paragraph_index\":"
                               << end_paragraph_index
                               << ",\"end_text_offset\":" << end_text_offset;
                    }
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }
