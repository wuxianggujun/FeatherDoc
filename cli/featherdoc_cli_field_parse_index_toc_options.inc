auto parse_index_toc_field_option(
    std::string_view command, std::string_view argument,
    const std::vector<std::string_view> &arguments, std::size_t &index,
    const append_field_command_traits &traits, append_field_options &options,
    std::string &error_message) -> field_option_parse_result {
    if (argument == "--columns") {
        if (!traits.is_index) {
            error_message = std::string(command) + " does not support --columns";
            return field_option_parse_result::error;
        }
        if (options.columns.has_value()) {
            error_message = "duplicate --columns option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --columns";
            return field_option_parse_result::error;
        }
        std::uint32_t columns = 0U;
        if (!parse_uint32(arguments[index + 1U], columns) || columns == 0U) {
            error_message = "invalid index columns: " +
                            std::string(arguments[index + 1U]);
            return field_option_parse_result::error;
        }
        options.columns = columns;
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--min-outline-level") {
        if (!traits.is_table_of_contents) {
            error_message = std::string(command) +
                            " does not support --min-outline-level";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --min-outline-level";
            return field_option_parse_result::error;
        }
        std::uint32_t level = 0U;
        if (!parse_uint32(arguments[index + 1U], level) || level == 0U ||
            level > 9U) {
            error_message = "invalid TOC minimum outline level: " +
                            std::string(arguments[index + 1U]);
            return field_option_parse_result::error;
        }
        options.min_outline_level = level;
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--max-outline-level") {
        if (!traits.is_table_of_contents) {
            error_message = std::string(command) +
                            " does not support --max-outline-level";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --max-outline-level";
            return field_option_parse_result::error;
        }
        std::uint32_t level = 0U;
        if (!parse_uint32(arguments[index + 1U], level) || level == 0U ||
            level > 9U) {
            error_message = "invalid TOC maximum outline level: " +
                            std::string(arguments[index + 1U]);
            return field_option_parse_result::error;
        }
        options.max_outline_level = level;
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--subentry") {
        if (!traits.is_index_entry) {
            error_message = std::string(command) + " does not support --subentry";
            return field_option_parse_result::error;
        }
        if (options.subentry.has_value()) {
            error_message = "duplicate --subentry option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --subentry";
            return field_option_parse_result::error;
        }
        options.subentry = std::string(arguments[index + 1U]);
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--bookmark") {
        if (!traits.is_index_entry) {
            error_message = std::string(command) + " does not support --bookmark";
            return field_option_parse_result::error;
        }
        if (options.bookmark_name.has_value()) {
            error_message = "duplicate --bookmark option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing name after --bookmark";
            return field_option_parse_result::error;
        }
        options.bookmark_name = std::string(arguments[index + 1U]);
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--cross-reference") {
        if (!traits.is_index_entry) {
            error_message = std::string(command) +
                            " does not support --cross-reference";
            return field_option_parse_result::error;
        }
        if (options.cross_reference.has_value()) {
            error_message = "duplicate --cross-reference option";
            return field_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --cross-reference";
            return field_option_parse_result::error;
        }
        options.cross_reference = std::string(arguments[index + 1U]);
        ++index;
        return field_option_parse_result::handled;
    }

    if (argument == "--bold-page-number") {
        if (!traits.is_index_entry) {
            error_message = std::string(command) +
                            " does not support --bold-page-number";
            return field_option_parse_result::error;
        }
        options.bold_page_number = true;
        return field_option_parse_result::handled;
    }

    if (argument == "--italic-page-number") {
        if (!traits.is_index_entry) {
            error_message = std::string(command) +
                            " does not support --italic-page-number";
            return field_option_parse_result::error;
        }
        options.italic_page_number = true;
        return field_option_parse_result::handled;
    }

    if (argument == "--no-hyperlinks") {
        if (!traits.is_table_of_contents) {
            error_message = std::string(command) +
                            " does not support --no-hyperlinks";
            return field_option_parse_result::error;
        }
        options.hyperlinks = false;
        return field_option_parse_result::handled;
    }

    if (argument == "--show-page-numbers-in-web-layout") {
        if (!traits.is_table_of_contents) {
            error_message = std::string(command) +
                            " does not support --show-page-numbers-in-web-layout";
            return field_option_parse_result::error;
        }
        options.hide_page_numbers_in_web_layout = false;
        return field_option_parse_result::handled;
    }

    if (argument == "--no-outline-levels") {
        if (!traits.is_table_of_contents) {
            error_message = std::string(command) +
                            " does not support --no-outline-levels";
            return field_option_parse_result::error;
        }
        options.use_outline_levels = false;
        return field_option_parse_result::handled;
    }

    return field_option_parse_result::unhandled;
}
