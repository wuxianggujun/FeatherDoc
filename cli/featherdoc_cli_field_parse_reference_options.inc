auto parse_reference_field_option(
    std::string_view command, std::string_view argument,
    const std::vector<std::string_view> &arguments, std::size_t &index,
    const append_field_command_traits &traits, append_field_options &options,
    std::string &error_message) -> field_option_parse_result {
    if (argument == "--no-hyperlink") {
        if (!traits.is_reference && !traits.is_page_reference) {
            error_message = std::string(command) +
                            " does not support --no-hyperlink";
            return field_option_parse_result::error;
        }
        options.hyperlink = false;
        return field_option_parse_result::handled;
    }

    if (argument == "--relative-position") {
        if (!traits.is_page_reference && !traits.is_style_reference) {
            error_message = std::string(command) +
                            " does not support --relative-position";
            return field_option_parse_result::error;
        }
        options.relative_position = true;
        return field_option_parse_result::handled;
    }

    if (argument == "--paragraph-number") {
        if (!traits.is_style_reference) {
            error_message = std::string(command) +
                            " does not support --paragraph-number";
            return field_option_parse_result::error;
        }
        options.paragraph_number = true;
        return field_option_parse_result::handled;
    }

    if (argument == "--no-preserve-formatting") {
        if (!traits.supports_preserve_formatting) {
            error_message = std::string(command) +
                            " does not support --no-preserve-formatting";
            return field_option_parse_result::error;
        }
        options.preserve_formatting = false;
        return field_option_parse_result::handled;
    }

    if (argument == "--format") {
        if (!traits.is_date) {
            error_message = std::string(command) + " does not support --format";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --format";
            return field_option_parse_result::error;
        }
        options.date_format = std::string(arguments[index + 1U]);
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--anchor") {
        if (!traits.is_hyperlink) {
            error_message = std::string(command) + " does not support --anchor";
            return field_option_parse_result::error;
        }
        if (options.anchor.has_value()) {
            error_message = "duplicate --anchor option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --anchor";
            return field_option_parse_result::error;
        }
        options.anchor = std::string(arguments[index + 1U]);
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--tooltip") {
        if (!traits.is_hyperlink) {
            error_message = std::string(command) + " does not support --tooltip";
            return field_option_parse_result::error;
        }
        if (options.tooltip.has_value()) {
            error_message = "duplicate --tooltip option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --tooltip";
            return field_option_parse_result::error;
        }
        options.tooltip = std::string(arguments[index + 1U]);
        ++index;
        return field_option_parse_result::handled;
    }

    return field_option_parse_result::unhandled;
}
