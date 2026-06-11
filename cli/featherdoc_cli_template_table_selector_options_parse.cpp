#include "featherdoc_cli_template_inspect_options_parse.hpp"

namespace featherdoc_cli {

auto parse_template_table_bookmark_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::optional<std::string> &bookmark_name, std::string &error_message)
    -> option_parse_result {
    const auto argument = arguments[index];
    if (argument != "--bookmark") {
        return option_parse_result::not_matched;
    }

    if (bookmark_name.has_value()) {
        error_message = "duplicate --bookmark option";
        return option_parse_result::error;
    }
    if (index + 1U >= arguments.size()) {
        error_message = "missing value after --bookmark";
        return option_parse_result::error;
    }

    const auto parsed_bookmark_name = std::string(arguments[index + 1U]);
    if (parsed_bookmark_name.empty()) {
        error_message = "bookmark name must not be empty";
        return option_parse_result::error;
    }

    bookmark_name = parsed_bookmark_name;
    ++index;
    return option_parse_result::matched;
}

auto template_table_selector_uses_text_matching(
    const featherdoc::template_table_selector &selector) -> bool {
    return selector.after_paragraph_text.has_value() ||
           !selector.header_cell_texts.empty();
}

auto validate_template_table_selector(
    const std::optional<std::size_t> &table_index,
    const std::optional<std::string> &bookmark_name, bool allow_none,
    std::string &error_message) -> bool {
    if (table_index.has_value() && bookmark_name.has_value()) {
        error_message = "cannot combine a table index with --bookmark";
        return false;
    }

    if (!allow_none && !table_index.has_value() && !bookmark_name.has_value()) {
        error_message = "expected a table index or --bookmark <name>";
        return false;
    }

    return true;
}

auto validate_template_table_selector(
    const featherdoc::template_table_selector &selector, bool allow_none,
    bool has_header_row_index, bool has_occurrence,
    std::string &error_message) -> bool {
    const auto uses_direct_target =
        selector.table_index.has_value() || selector.bookmark_name.has_value();
    const auto uses_text_target =
        template_table_selector_uses_text_matching(selector);

    if (selector.table_index.has_value() && selector.bookmark_name.has_value()) {
        error_message = "cannot combine a table index with --bookmark";
        return false;
    }

    if (selector.bookmark_name.has_value() && selector.bookmark_name->empty()) {
        error_message = "bookmark name must not be empty";
        return false;
    }

    if (selector.after_paragraph_text.has_value() &&
        selector.after_paragraph_text->empty()) {
        error_message = "--after-text must not be empty";
        return false;
    }

    if (uses_direct_target && uses_text_target) {
        error_message = "cannot combine a table index or --bookmark with "
                        "--after-text or --header-cell";
        return false;
    }

    if (!allow_none && !uses_direct_target && !uses_text_target) {
        error_message = "expected a table index, --bookmark <name>, "
                        "--after-text <text>, or --header-cell <text>";
        return false;
    }

    if (has_header_row_index && selector.header_cell_texts.empty()) {
        error_message = "--header-row requires at least one --header-cell";
        return false;
    }

    if (has_occurrence && !uses_text_target) {
        error_message = "--occurrence requires --after-text or --header-cell";
        return false;
    }

    return true;
}

auto parse_template_table_selector_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::template_table_selector &selector, bool &has_header_row_index,
    bool &has_occurrence, std::string &error_message) -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--bookmark") {
        return parse_template_table_bookmark_option(arguments, index,
                                                    selector.bookmark_name,
                                                    error_message);
    }

    if (argument == "--after-text") {
        if (selector.after_paragraph_text.has_value()) {
            error_message = "duplicate --after-text option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --after-text";
            return option_parse_result::error;
        }

        selector.after_paragraph_text = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--header-cell") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --header-cell";
            return option_parse_result::error;
        }

        selector.header_cell_texts.emplace_back(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--header-row") {
        if (has_header_row_index) {
            error_message = "duplicate --header-row option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --header-row";
            return option_parse_result::error;
        }

        std::size_t parsed_header_row_index = 0U;
        if (!parse_index(arguments[index + 1U], parsed_header_row_index)) {
            error_message =
                "invalid header row index: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        selector.header_row_index = parsed_header_row_index;
        has_header_row_index = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--occurrence") {
        if (has_occurrence) {
            error_message = "duplicate --occurrence option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --occurrence";
            return option_parse_result::error;
        }

        std::size_t parsed_occurrence = 0U;
        if (!parse_index(arguments[index + 1U], parsed_occurrence)) {
            error_message =
                "invalid occurrence index: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        selector.occurrence = parsed_occurrence;
        has_occurrence = true;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

} // namespace featherdoc_cli
