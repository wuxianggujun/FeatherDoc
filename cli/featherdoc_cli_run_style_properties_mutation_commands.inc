auto run_set_paragraph_style_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-paragraph-style-properties expects an input path, a style id, and mutation options",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    set_paragraph_style_properties_options options;
    std::string error_message;
    if (!parse_set_paragraph_style_properties_options(arguments, 3U, options,
                                                      error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    if (options.based_on.has_value() &&
        !doc.set_paragraph_style_based_on(style_id, *options.based_on)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.next_style.has_value() &&
        !doc.set_paragraph_style_next_style(style_id, *options.next_style)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.outline_level.has_value() &&
        !doc.set_paragraph_style_outline_level(style_id, *options.outline_level)) {
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
            [&style_id, &options](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"based_on\":";
                write_json_optional_string(stream, options.based_on);
                stream << ",\"next_style\":";
                write_json_optional_string(stream, options.next_style);
                stream << ",\"outline_level\":";
                if (options.outline_level.has_value()) {
                    stream << *options.outline_level;
                } else {
                    stream << "null";
                }
            });
    }

    return 0;
}

auto run_clear_paragraph_style_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "clear-paragraph-style-properties expects an input path, a style id, and clear options",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    clear_paragraph_style_properties_options options;
    std::string error_message;
    if (!parse_clear_paragraph_style_properties_options(arguments, 3U, options,
                                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    std::vector<std::string> cleared_fields;
    if (options.clear_based_on) {
        if (!doc.clear_paragraph_style_based_on(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("based_on");
    }

    if (options.clear_next_style) {
        if (!doc.clear_paragraph_style_next_style(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("next_style");
    }

    if (options.clear_outline_level) {
        if (!doc.clear_paragraph_style_outline_level(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("outline_level");
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&style_id, &cleared_fields](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"cleared\":";
                write_json_cleared_fields(stream, cleared_fields);
            });
    }

    return 0;
}

auto run_set_style_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-style-run-properties expects an input path, a style id, and mutation options",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    set_style_run_properties_options options;
    std::string error_message;
    if (!parse_set_style_run_properties_options(arguments, 3U, options,
                                                error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    if (options.font_family.has_value() &&
        !doc.set_style_run_font_family(style_id, *options.font_family)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.east_asia_font_family.has_value() &&
        !doc.set_style_run_east_asia_font_family(style_id,
                                                 *options.east_asia_font_family)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.language.has_value() &&
        !doc.set_style_run_language(style_id, *options.language)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.east_asia_language.has_value() &&
        !doc.set_style_run_east_asia_language(style_id,
                                              *options.east_asia_language)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.bidi_language.has_value() &&
        !doc.set_style_run_bidi_language(style_id, *options.bidi_language)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.rtl.has_value() && !doc.set_style_run_rtl(style_id, *options.rtl)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.paragraph_bidi.has_value() &&
        !doc.set_style_paragraph_bidi(style_id, *options.paragraph_bidi)) {
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
            [&options, &style_id](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"font_family\":";
                write_json_optional_string(stream, options.font_family);
                stream << ",\"east_asia_font_family\":";
                write_json_optional_string(stream, options.east_asia_font_family);
                stream << ",\"language\":";
                write_json_optional_string(stream, options.language);
                stream << ",\"east_asia_language\":";
                write_json_optional_string(stream, options.east_asia_language);
                stream << ",\"bidi_language\":";
                write_json_optional_string(stream, options.bidi_language);
                stream << ",\"rtl\":";
                write_json_optional_bool(stream, options.rtl);
                stream << ",\"paragraph_bidi\":";
                write_json_optional_bool(stream, options.paragraph_bidi);
            });
    }

    return 0;
}

auto run_clear_style_run_properties_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "clear-style-run-properties expects an input path, a style id, and clear options",
            json_output);
        return 2;
    }

    const auto style_id = std::string(arguments[2]);
    clear_style_run_properties_options options;
    std::string error_message;
    if (!parse_clear_style_run_properties_options(arguments, 3U, options,
                                                  error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document_for_command(command, arguments, doc, options.json_output)) {
        return 1;
    }

    std::vector<std::string> cleared_fields;
    if (options.clear_font_family) {
        if (!doc.clear_style_run_font_family(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("font_family");
    }

    if (options.clear_east_asia_font_family) {
        if (!doc.clear_style_run_east_asia_font_family(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("east_asia_font_family");
    }

    if (options.clear_primary_language) {
        if (!doc.clear_style_run_primary_language(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("primary_language");
    }

    if (options.clear_language) {
        if (!doc.clear_style_run_language(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("language");
    }

    if (options.clear_east_asia_language) {
        if (!doc.clear_style_run_east_asia_language(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("east_asia_language");
    }

    if (options.clear_bidi_language) {
        if (!doc.clear_style_run_bidi_language(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("bidi_language");
    }

    if (options.clear_rtl) {
        if (!doc.clear_style_run_rtl(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("rtl");
    }

    if (options.clear_paragraph_bidi) {
        if (!doc.clear_style_paragraph_bidi(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        cleared_fields.emplace_back("paragraph_bidi");
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&cleared_fields, &style_id](std::ostream &stream) {
                stream << ",\"style_id\":";
                write_json_string(stream, style_id);
                stream << ",\"cleared\":";
                write_json_cleared_fields(stream, cleared_fields);
            });
    }

    return 0;
}

