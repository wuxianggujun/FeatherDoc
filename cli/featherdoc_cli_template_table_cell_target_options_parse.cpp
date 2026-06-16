#include "featherdoc_cli_template_table_options_parse.hpp"

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
