    if (command == "set-revision-metadata") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-revision-metadata expects an input path and revision index",
                json_output);
            return 2;
        }

        std::size_t revision_index = 0U;
        if (!parse_index(arguments[2], revision_index)) {
            print_parse_error(command,
                              "invalid revision index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        revision_metadata_mutation_options options;
        std::string error_message;
        if (!parse_revision_metadata_mutation_options(arguments, 3U, options,
                                                      error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.set_revision_metadata(revision_index, options.metadata)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [revision_index](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"revision_index\":" << revision_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "accept-revision" || command == "reject-revision" ||
        command == "accept-all-revisions" || command == "reject-all-revisions") {
        const auto json_output = has_json_flag(arguments);
        const bool accept_one = command == "accept-revision";
        const bool reject_one = command == "reject-revision";
        const bool one_revision = accept_one || reject_one;
        if (arguments.size() < (one_revision ? 3U : 2U)) {
            print_parse_error(command,
                              one_revision ? std::string(command) +
                                                 " expects an input path and revision index"
                                           : std::string(command) +
                                                 " expects an input path",
                              json_output);
            return 2;
        }

        std::size_t revision_index = 0U;
        if (one_revision && !parse_index(arguments[2], revision_index)) {
            print_parse_error(command,
                              "invalid revision index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        simple_document_mutation_options options;
        std::string error_message;
        if (!parse_simple_document_mutation_options(
                arguments, one_revision ? 3U : 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::size_t affected = 0U;
        if (accept_one) {
            affected = doc.accept_revision(revision_index) ? 1U : 0U;
        } else if (reject_one) {
            affected = doc.reject_revision(revision_index) ? 1U : 0U;
        } else if (command == "accept-all-revisions") {
            affected = doc.accept_all_revisions();
        } else {
            affected = doc.reject_all_revisions();
        }
        if (one_revision && affected == 0U) {
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

    print_parse_error(command, "unknown review revision command",
                      has_json_flag(arguments));
    return 2;
