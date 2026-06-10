auto parse_caption_sequence_field_option(
    std::string_view command, std::string_view argument,
    const std::vector<std::string_view> &arguments, std::size_t &index,
    const append_field_command_traits &traits, append_field_options &options,
    std::string &error_message) -> field_option_parse_result {
    if (argument == "--text") {
        if (!traits.is_caption) {
            error_message = std::string(command) + " does not support --text";
            return field_option_parse_result::error;
        }
        if (options.has_caption_text) {
            error_message = "duplicate --text option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --text";
            return field_option_parse_result::error;
        }
        options.caption_text = std::string(arguments[index + 1U]);
        options.has_caption_text = true;
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--number-result") {
        if (!traits.is_caption) {
            error_message = std::string(command) +
                            " does not support --number-result";
            return field_option_parse_result::error;
        }
        if (options.has_number_result_text) {
            error_message = "duplicate --number-result option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --number-result";
            return field_option_parse_result::error;
        }
        options.number_result_text = std::string(arguments[index + 1U]);
        options.has_number_result_text = true;
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--number-format") {
        if (!traits.is_caption && !traits.is_sequence) {
            error_message = std::string(command) +
                            " does not support --number-format";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --number-format";
            return field_option_parse_result::error;
        }
        options.number_format = std::string(arguments[index + 1U]);
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--restart") {
        if (!traits.is_caption && !traits.is_sequence) {
            error_message = std::string(command) + " does not support --restart";
            return field_option_parse_result::error;
        }
        if (options.restart_value.has_value()) {
            error_message = "duplicate --restart option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --restart";
            return field_option_parse_result::error;
        }
        std::uint32_t restart_value = 0U;
        if (!parse_uint32(arguments[index + 1U], restart_value) ||
            restart_value == 0U) {
            error_message = "invalid restart value: " +
                            std::string(arguments[index + 1U]);
            return field_option_parse_result::error;
        }
        options.restart_value = restart_value;
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--separator") {
        if (!traits.is_caption) {
            error_message = std::string(command) + " does not support --separator";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --separator";
            return field_option_parse_result::error;
        }
        options.separator = std::string(arguments[index + 1U]);
        ++index;
        return field_option_parse_result::handled;
    }

    return field_option_parse_result::unhandled;
}
