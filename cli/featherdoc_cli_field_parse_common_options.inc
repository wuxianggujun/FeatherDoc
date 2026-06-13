auto parse_common_append_field_option(
    std::string_view command, std::string_view argument,
    const std::vector<std::string_view> &arguments, std::size_t &index,
    const append_field_command_traits &traits, append_field_options &options,
    std::string &error_message) -> field_option_parse_result {
    if (argument == "--part") {
        if (options.has_part) {
            error_message = "duplicate --part option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --part";
            return field_option_parse_result::error;
        }

        if (!parse_validation_part(arguments[index + 1U], options.part)) {
            error_message = "invalid template part: " +
                            std::string(arguments[index + 1U]);
            return field_option_parse_result::error;
        }

        options.has_part = true;
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--index") {
        if (options.part_index.has_value()) {
            error_message = "duplicate --index option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --index";
            return field_option_parse_result::error;
        }

        std::size_t part_index = 0U;
        if (!parse_index(arguments[index + 1U], part_index)) {
            error_message = "invalid part index: " +
                            std::string(arguments[index + 1U]);
            return field_option_parse_result::error;
        }

        options.part_index = part_index;
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--section") {
        if (options.section_index.has_value()) {
            error_message = "duplicate --section option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --section";
            return field_option_parse_result::error;
        }

        std::size_t section_index = 0U;
        if (!parse_index(arguments[index + 1U], section_index)) {
            error_message = "invalid section index: " +
                            std::string(arguments[index + 1U]);
            return field_option_parse_result::error;
        }

        options.section_index = section_index;
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--kind") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --kind";
            return field_option_parse_result::error;
        }

        if (!parse_reference_kind(arguments[index + 1U],
                                  options.reference_kind)) {
            error_message = "invalid reference kind: " +
                            std::string(arguments[index + 1U]);
            return field_option_parse_result::error;
        }

        options.has_kind = true;
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--output") {
        if (options.output_path.has_value()) {
            error_message = "duplicate --output option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing path after --output";
            return field_option_parse_result::error;
        }

        options.output_path =
            std::filesystem::path(std::string(arguments[index + 1U]));
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--result-text") {
        if (!traits.supports_result_text) {
            error_message = std::string(command) + " does not support --result-text";
            return field_option_parse_result::error;
        }
        if (options.has_result_text) {
            error_message = "duplicate --result-text option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --result-text";
            return field_option_parse_result::error;
        }

        options.result_text = std::string(arguments[index + 1U]);
        options.has_result_text = true;
        ++index;
        return field_option_parse_result::handled;
    }

    return field_option_parse_result::unhandled;
}
