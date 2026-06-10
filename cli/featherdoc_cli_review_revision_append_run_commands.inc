    if (command == "append-insertion-revision" ||
        command == "append-deletion-revision") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              std::string(command) + " expects an input path",
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        if (!parse_revision_authoring_options(arguments, 2U, options,
                                              error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const bool insertion = command == "append-insertion-revision";
        const auto affected = insertion
                                  ? doc.append_insertion_revision(
                                        options.text, options.author, options.date)
                                  : doc.append_deletion_revision(
                                        options.text, options.author, options.date);
        if (affected == 0U) {
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
                [affected](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "insert-run-revision-after" ||
        command == "delete-run-revision" ||
        command == "replace-run-revision") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                std::string(command) +
                    " expects an input path, paragraph index, and run index",
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

        std::size_t run_index = 0U;
        if (!parse_index(arguments[3], run_index)) {
            print_parse_error(command,
                              "invalid run index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-run-revision";
        if (!parse_revision_authoring_options(arguments, 4U, options,
                                              error_message, require_text)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        bool ok = false;
        if (command == "insert-run-revision-after") {
            ok = doc.insert_run_revision_after(
                paragraph_index, run_index, options.text, options.author,
                options.date);
        } else if (command == "delete-run-revision") {
            ok = doc.delete_run_revision(paragraph_index, run_index,
                                         options.author, options.date);
        } else {
            ok = doc.replace_run_revision(paragraph_index, run_index,
                                          options.text, options.author,
                                          options.date);
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
                [paragraph_index, run_index](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"paragraph_index\":" << paragraph_index
                           << ",\"run_index\":" << run_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }
