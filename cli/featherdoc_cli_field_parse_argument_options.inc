auto parse_field_argument(std::string_view command, std::string_view argument,
                          const append_field_command_traits &traits,
                          append_field_options &options,
                          std::string &error_message)
    -> field_option_parse_result {
    if (argument.empty() || argument.front() == '-') {
        return field_option_parse_result::unhandled;
    }

    if (traits.is_replace) {
        if (!options.has_field_index) {
            std::size_t field_index = 0U;
            if (!parse_index(argument, field_index)) {
                error_message = "invalid field index: " + std::string(argument);
                return field_option_parse_result::error;
            }
            options.field_index = field_index;
            options.has_field_index = true;
            return field_option_parse_result::handled;
        }

        if (!options.has_field_argument) {
            options.field_argument = std::string(argument);
            options.has_field_argument = true;
            return field_option_parse_result::handled;
        }

        error_message =
            "duplicate replace-field instruction: " + std::string(argument);
        return field_option_parse_result::error;
    }

    if (options.has_field_argument) {
        error_message = "duplicate field argument: " + std::string(argument);
        return field_option_parse_result::error;
    }
    options.field_argument = std::string(argument);
    options.has_field_argument = true;
    return field_option_parse_result::handled;
}
