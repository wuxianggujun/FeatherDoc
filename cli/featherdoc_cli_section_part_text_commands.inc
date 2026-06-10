[[nodiscard]] auto run_show_section_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "show-section-header/footer expects an input path and "
                          "a section index",
                          json_output);
        return 2;
    }

    std::size_t section_index = 0;
    if (!parse_index(arguments[2], section_index)) {
        print_parse_error(command,
                          "invalid section index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    section_text_options options;
    std::string error_message;
    if (!parse_section_text_options(arguments, 3U, false, false, true, options,
                                    error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    return show_section_text(doc, part_family_for_command(command), section_index,
                             options.reference_kind, options.json_output)
               ? 0
               : 1;
}

[[nodiscard]] auto run_set_section_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "set-section-header/footer expects an input path and "
                          "a section index",
                          json_output);
        return 2;
    }

    std::size_t section_index = 0;
    if (!parse_index(arguments[2], section_index)) {
        print_parse_error(command,
                          "invalid section index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    section_text_options options;
    std::string error_message;
    if (!parse_section_text_options(arguments, 3U, true, true, true, options,
                                    error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::string replacement_text;
    if (!read_text_source(options, replacement_text, error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "input", error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    const auto family = part_family_for_command(command);
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!replace_section_text(doc, family, section_index, replacement_text,
                              options.reference_kind, command,
                              options.json_output)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [family, section_index, &options](std::ostream &stream) {
                stream << ",\"part\":";
                write_json_string(stream, section_part_name(family));
                stream << ",\"section\":" << section_index << ",\"kind\":";
                write_json_string(
                    stream, featherdoc::to_xml_reference_type(options.reference_kind));
            });
    }

    return 0;
}
