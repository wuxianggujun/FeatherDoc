auto is_section_part_command(std::string_view command) -> bool {
    return command == "inspect-sections" ||
           command == "inspect-header-parts" ||
           command == "inspect-footer-parts" ||
           is_section_management_command(command) ||
           is_section_part_reference_command(command) ||
           is_section_part_management_command(command) ||
           command == "show-section-header" ||
           command == "show-section-footer" ||
           command == "set-section-header" ||
           command == "set-section-footer";
}

auto run_section_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "inspect-sections") {
        return run_inspect_sections_command(command, arguments, doc);
    }
    if (command == "inspect-header-parts" || command == "inspect-footer-parts") {
        return run_inspect_related_parts_command(command, arguments, doc);
    }
    if (is_section_management_command(command)) {
        return run_section_management_command(command, arguments, doc);
    }
    if (is_section_part_reference_command(command)) {
        return run_section_part_reference_command(command, arguments, doc);
    }
    if (is_section_part_management_command(command)) {
        return run_section_part_management_command(command, arguments, doc);
    }
    if (command == "show-section-header" || command == "show-section-footer") {
        return run_show_section_part_command(command, arguments, doc);
    }
    if (command == "set-section-header" || command == "set-section-footer") {
        return run_set_section_part_command(command, arguments, doc);
    }

    print_parse_error(command, "unsupported section part command",
                      has_json_flag(arguments));
    return 2;
}
