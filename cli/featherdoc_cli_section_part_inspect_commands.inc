void write_json_section_flags(std::ostream &stream, featherdoc::Document &doc,
                              std::size_t section_index,
                              section_part_family family) {
    const auto has_default =
        family == section_part_family::header
            ? has_section_header(
                  doc, section_index,
                  featherdoc::section_reference_kind::default_reference)
            : has_section_footer(
                  doc, section_index,
                  featherdoc::section_reference_kind::default_reference);
    const auto has_first =
        family == section_part_family::header
            ? has_section_header(doc, section_index,
                                 featherdoc::section_reference_kind::first_page)
            : has_section_footer(doc, section_index,
                                 featherdoc::section_reference_kind::first_page);
    const auto has_even =
        family == section_part_family::header
            ? has_section_header(doc, section_index,
                                 featherdoc::section_reference_kind::even_page)
            : has_section_footer(doc, section_index,
                                 featherdoc::section_reference_kind::even_page);

    stream << "{\"default\":" << (has_default ? "true" : "false")
           << ",\"first\":" << (has_first ? "true" : "false")
           << ",\"even\":" << (has_even ? "true" : "false") << '}';
}

void write_json_part_references(
    std::ostream &stream,
    const std::vector<inspected_part_reference> &references) {
    stream << '[';
    for (std::size_t index = 0; index < references.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"section\":" << references[index].section_index
               << ",\"kind\":";
        write_json_string(stream, references[index].kind);
        stream << '}';
    }
    stream << ']';
}

void inspect_sections(featherdoc::Document &doc, bool json_output) {
    if (json_output) {
        std::cout << "{\"sections\":" << doc.section_count()
                  << ",\"headers\":" << doc.header_count()
                  << ",\"footers\":" << doc.footer_count()
                  << ",\"section_layouts\":[";
        for (std::size_t section_index = 0; section_index < doc.section_count();
             ++section_index) {
            if (section_index != 0U) {
                std::cout << ',';
            }

            std::cout << "{\"index\":" << section_index << ",\"header\":";
            write_json_section_flags(std::cout, doc, section_index,
                                     section_part_family::header);
            std::cout << ",\"footer\":";
            write_json_section_flags(std::cout, doc, section_index,
                                     section_part_family::footer);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "sections: " << doc.section_count() << '\n';
    std::cout << "headers: " << doc.header_count() << '\n';
    std::cout << "footers: " << doc.footer_count() << '\n';

    for (std::size_t section_index = 0; section_index < doc.section_count();
         ++section_index) {
        std::cout
            << "section[" << section_index << "]: header(default="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::default_reference))
            << ", first="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::first_page))
            << ", even="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::even_page))
            << ") footer(default="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::default_reference))
            << ", first="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::first_page))
            << ", even="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::even_page))
            << ")\n";
    }
}

void inspect_related_parts(const std::vector<inspected_part_entry> &parts,
                           section_part_family family, bool json_output) {
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, section_part_name(family));
        std::cout << ",\"count\":" << parts.size() << ",\"parts\":[";
        for (std::size_t index = 0; index < parts.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }

            const auto &part = parts[index];
            std::cout << "{\"index\":" << index << ",\"relationship_id\":";
            write_json_string(std::cout, part.relationship_id);
            std::cout << ",\"entry\":";
            write_json_string(std::cout, part.entry_name);
            std::cout << ",\"references\":";
            write_json_part_references(std::cout, part.references);
            std::cout << ",\"paragraphs\":";
            write_json_lines(std::cout, part.paragraphs);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << section_part_name(family) << " parts: " << parts.size() << '\n';
    for (std::size_t index = 0; index < parts.size(); ++index) {
        const auto &part = parts[index];
        std::cout << "part[" << index << "]: entry=" << part.entry_name
                  << " relationship=" << part.relationship_id << " references=";
        if (part.references.empty()) {
            std::cout << "none";
        } else {
            for (std::size_t reference_index = 0;
                 reference_index < part.references.size(); ++reference_index) {
                if (reference_index != 0U) {
                    std::cout << ", ";
                }
                std::cout << "section["
                          << part.references[reference_index].section_index
                          << "]:" << part.references[reference_index].kind;
            }
        }
        std::cout << '\n';

        for (std::size_t paragraph_index = 0;
             paragraph_index < part.paragraphs.size(); ++paragraph_index) {
            std::cout << "  paragraph[" << paragraph_index
                      << "]: " << part.paragraphs[paragraph_index] << '\n';
        }
    }
}

[[nodiscard]] auto run_inspect_sections_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-sections expects an input path",
                          json_output);
        return 2;
    }

    inspect_options options;
    std::string error_message;
    if (!parse_inspect_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    inspect_sections(doc, options.json_output);
    return 0;
}

[[nodiscard]] auto run_inspect_related_parts_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "inspect-header/footer-parts expects an input path",
                          json_output);
        return 2;
    }

    inspect_options options;
    std::string error_message;
    if (!parse_inspect_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto family = part_family_for_command(command);
    const auto input_path = path_type(std::string(arguments[1]));
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    std::vector<inspected_part_entry> parts;
    if (!load_inspected_part_entries(input_path, doc, family, parts,
                                     error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "inspect", error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    inspect_related_parts(parts, family, options.json_output);
    return 0;
}
