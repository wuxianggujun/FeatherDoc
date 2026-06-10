auto parse_state_field_option(std::string_view command, std::string_view argument,
                              const append_field_command_traits &traits,
                              append_field_options &options,
                              std::string &error_message)
    -> field_option_parse_result {
    if (argument == "--dirty") {
        if (!traits.supports_field_state) {
            error_message = std::string(command) + " does not support --dirty";
            return field_option_parse_result::error;
        }
        options.dirty = true;
        return field_option_parse_result::handled;
    }

    if (argument == "--locked") {
        if (!traits.supports_field_state) {
            error_message = std::string(command) + " does not support --locked";
            return field_option_parse_result::error;
        }
        options.locked = true;
        return field_option_parse_result::handled;
    }

    if (argument == "--json") {
        options.json_output = true;
        return field_option_parse_result::handled;
    }

    return field_option_parse_result::unhandled;
}
