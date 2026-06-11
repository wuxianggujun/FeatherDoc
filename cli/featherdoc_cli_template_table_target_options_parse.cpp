#include "featherdoc_cli_template_table_options_parse.hpp"

#include <utility>

namespace featherdoc_cli {

auto parse_template_table_target_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    std::optional<std::size_t> &table_index,
    std::optional<std::string> &bookmark_name,
    std::size_t &options_start_index, std::string &error_message) -> bool {
    if (start_index >= arguments.size()) {
        error_message = "expected either <table-index> or --bookmark <name>";
        return false;
    }

    if (arguments[start_index] == "--bookmark") {
        if (start_index + 1U >= arguments.size()) {
            error_message = "expected --bookmark <name>";
            return false;
        }

        const auto parsed_bookmark_name =
            std::string(arguments[start_index + 1U]);
        if (parsed_bookmark_name.empty()) {
            error_message = "bookmark name must not be empty";
            return false;
        }

        bookmark_name = std::move(parsed_bookmark_name);
        options_start_index = start_index + 2U;
        return true;
    }

    if (arguments[start_index].size() >= 2U &&
        arguments[start_index].substr(0U, 2U) == "--") {
        options_start_index = start_index;
        return true;
    }

    std::size_t parsed_table_index = 0U;
    if (!parse_index(arguments[start_index], parsed_table_index)) {
        error_message =
            "invalid table index: " + std::string(arguments[start_index]);
        return false;
    }

    table_index = parsed_table_index;
    options_start_index = start_index + 1U;
    return true;
}

auto parse_template_table_selector_target_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    parsed_template_table_selector_target &target, bool allow_empty_target,
    std::string &error_message) -> bool {
    if (start_index >= arguments.size()) {
        error_message = "expected a table index, --bookmark <name>, "
                        "--after-text <text>, or --header-cell <text>";
        return false;
    }

    if (arguments[start_index].size() < 2U ||
        arguments[start_index].substr(0U, 2U) != "--") {
        std::size_t parsed_table_index = 0U;
        if (!parse_index(arguments[start_index], parsed_table_index)) {
            error_message =
                "invalid table index: " + std::string(arguments[start_index]);
            return false;
        }

        target.selector.table_index = parsed_table_index;
        target.options_start_index = start_index + 1U;
        return true;
    }

    std::size_t index = start_index;
    bool matched_selector_option = false;
    while (index < arguments.size()) {
        const auto current_index = index;
        const auto selector_parse_result = parse_template_table_selector_option(
            arguments, index, target.selector, target.has_header_row_index,
            target.has_occurrence, error_message);
        if (selector_parse_result == option_parse_result::matched) {
            matched_selector_option = true;
            ++index;
            continue;
        }
        if (selector_parse_result == option_parse_result::error) {
            return false;
        }

        index = current_index;
        break;
    }

    if (!matched_selector_option && !allow_empty_target) {
        error_message = "expected a table index, --bookmark <name>, "
                        "--after-text <text>, or --header-cell <text>";
        return false;
    }

    target.options_start_index = index;
    return true;
}

auto parse_template_table_selector_row_target_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    parsed_template_table_selector_row_target &target,
    std::string &error_message) -> bool {
    if (start_index >= arguments.size()) {
        error_message =
            "expected either <table-index> <row-index>, --bookmark <name> "
            "<row-index>, or a text-based table selector followed by "
            "<row-index>";
        return false;
    }

    if (arguments[start_index].size() < 2U ||
        arguments[start_index].substr(0U, 2U) != "--") {
        std::size_t parsed_table_index = 0U;
        if (!parse_index(arguments[start_index], parsed_table_index)) {
            error_message =
                "invalid table index: " + std::string(arguments[start_index]);
            return false;
        }
        if (start_index + 1U >= arguments.size()) {
            error_message = "expected a table index and a row index";
            return false;
        }
        if (!parse_index(arguments[start_index + 1U], target.row_index)) {
            error_message =
                "invalid row index: " + std::string(arguments[start_index + 1U]);
            return false;
        }

        target.selector.table_index = parsed_table_index;
        target.options_start_index = start_index + 2U;
        return true;
    }

    if (!parse_template_table_selector_target_arguments(arguments, start_index,
                                                        target, false,
                                                        error_message)) {
        return false;
    }
    if (target.options_start_index >= arguments.size() ||
        (arguments[target.options_start_index].size() >= 2U &&
         arguments[target.options_start_index].substr(0U, 2U) == "--")) {
        error_message = "missing row index after table selector";
        return false;
    }
    if (!parse_index(arguments[target.options_start_index], target.row_index)) {
        error_message = "invalid row index: " +
                        std::string(arguments[target.options_start_index]);
        return false;
    }

    ++target.options_start_index;
    return true;
}

auto parse_template_table_selector_cell_target_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    parsed_template_table_selector_cell_target &target,
    std::string &error_message) -> bool {
    if (start_index >= arguments.size()) {
        error_message =
            "expected either <table-index> <row-index> <cell-index>, "
            "--bookmark <name> <row-index> <cell-index>, or a text-based "
            "table selector followed by <row-index> <cell-index>";
        return false;
    }

    if (arguments[start_index].size() < 2U ||
        arguments[start_index].substr(0U, 2U) != "--") {
        std::size_t parsed_table_index = 0U;
        if (!parse_index(arguments[start_index], parsed_table_index)) {
            error_message =
                "invalid table index: " + std::string(arguments[start_index]);
            return false;
        }
        if (start_index + 2U >= arguments.size()) {
            error_message =
                "expected a table index, a row index, and a cell index";
            return false;
        }
        if (!parse_index(arguments[start_index + 1U], target.row_index)) {
            error_message =
                "invalid row index: " + std::string(arguments[start_index + 1U]);
            return false;
        }
        if (!parse_index(arguments[start_index + 2U], target.cell_index)) {
            error_message =
                "invalid cell index: " + std::string(arguments[start_index + 2U]);
            return false;
        }

        target.selector.table_index = parsed_table_index;
        target.options_start_index = start_index + 3U;
        return true;
    }

    if (!parse_template_table_selector_target_arguments(arguments, start_index,
                                                        target, false,
                                                        error_message)) {
        return false;
    }
    if (target.options_start_index >= arguments.size() ||
        (arguments[target.options_start_index].size() >= 2U &&
         arguments[target.options_start_index].substr(0U, 2U) == "--")) {
        error_message = "missing row index after table selector";
        return false;
    }
    if (!parse_index(arguments[target.options_start_index], target.row_index)) {
        error_message = "invalid row index: " +
                        std::string(arguments[target.options_start_index]);
        return false;
    }

    ++target.options_start_index;
    if (target.options_start_index >= arguments.size() ||
        (arguments[target.options_start_index].size() >= 2U &&
         arguments[target.options_start_index].substr(0U, 2U) == "--")) {
        error_message = "missing cell index after table selector";
        return false;
    }
    if (!parse_index(arguments[target.options_start_index], target.cell_index)) {
        error_message = "invalid cell index: " +
                        std::string(arguments[target.options_start_index]);
        return false;
    }

    ++target.options_start_index;
    return true;
}

auto parse_template_table_selector_optional_cell_target_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    parsed_template_table_selector_optional_cell_target &target,
    std::string &error_message) -> bool {
    if (start_index >= arguments.size()) {
        error_message =
            "expected either <table-index> <row-index> [<cell-index>], "
            "--bookmark <name> <row-index> [<cell-index>], or a text-based "
            "table selector followed by <row-index> [<cell-index>]";
        return false;
    }

    if (arguments[start_index].size() < 2U ||
        arguments[start_index].substr(0U, 2U) != "--") {
        std::size_t parsed_table_index = 0U;
        if (!parse_index(arguments[start_index], parsed_table_index)) {
            error_message =
                "invalid table index: " + std::string(arguments[start_index]);
            return false;
        }
        if (start_index + 1U >= arguments.size()) {
            error_message = "expected a table index and a row index";
            return false;
        }
        if (!parse_index(arguments[start_index + 1U], target.row_index)) {
            error_message =
                "invalid row index: " + std::string(arguments[start_index + 1U]);
            return false;
        }

        target.selector.table_index = parsed_table_index;
        target.options_start_index = start_index + 2U;
        if (target.options_start_index < arguments.size() &&
            !(arguments[target.options_start_index].size() >= 2U &&
              arguments[target.options_start_index].substr(0U, 2U) == "--")) {
            std::size_t parsed_cell_index = 0U;
            if (!parse_index(arguments[target.options_start_index],
                             parsed_cell_index)) {
                error_message = "invalid cell index: " +
                                std::string(arguments[target.options_start_index]);
                return false;
            }
            target.cell_index = parsed_cell_index;
            ++target.options_start_index;
        }
        return true;
    }

    if (!parse_template_table_selector_target_arguments(arguments, start_index,
                                                        target, false,
                                                        error_message)) {
        return false;
    }
    if (target.options_start_index >= arguments.size() ||
        (arguments[target.options_start_index].size() >= 2U &&
         arguments[target.options_start_index].substr(0U, 2U) == "--")) {
        error_message = "missing row index after table selector";
        return false;
    }
    if (!parse_index(arguments[target.options_start_index], target.row_index)) {
        error_message = "invalid row index: " +
                        std::string(arguments[target.options_start_index]);
        return false;
    }

    ++target.options_start_index;
    if (target.options_start_index < arguments.size() &&
        !(arguments[target.options_start_index].size() >= 2U &&
          arguments[target.options_start_index].substr(0U, 2U) == "--")) {
        std::size_t parsed_cell_index = 0U;
        if (!parse_index(arguments[target.options_start_index],
                         parsed_cell_index)) {
            error_message = "invalid cell index: " +
                            std::string(arguments[target.options_start_index]);
            return false;
        }
        target.cell_index = parsed_cell_index;
        ++target.options_start_index;
    }
    return true;
}

} // namespace featherdoc_cli
