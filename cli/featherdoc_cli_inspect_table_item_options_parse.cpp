#include "featherdoc_cli_inspect_table_item_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

auto parse_inspect_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_table_cells_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--row") {
            if (options.row_index.has_value()) {
                error_message = "duplicate --row option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --row";
                return false;
            }

            std::size_t row_index = 0U;
            if (!parse_index(arguments[index + 1U], row_index)) {
                error_message =
                    "invalid row index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.row_index = row_index;
            ++index;
            continue;
        }

        if (argument == "--cell") {
            if (options.cell_index.has_value()) {
                error_message = "duplicate --cell option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --cell";
                return false;
            }

            std::size_t cell_index = 0U;
            if (!parse_index(arguments[index + 1U], cell_index)) {
                error_message =
                    "invalid cell index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.cell_index = cell_index;
            ++index;
            continue;
        }

        if (argument == "--grid-column") {
            if (options.grid_column.has_value()) {
                error_message = "duplicate --grid-column option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --grid-column";
                return false;
            }

            std::size_t grid_column = 0U;
            if (!parse_index(arguments[index + 1U], grid_column)) {
                error_message = "invalid grid column: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.grid_column = grid_column;
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.cell_index.has_value() && !options.row_index.has_value()) {
        error_message = "--cell requires --row";
        return false;
    }
    if (options.grid_column.has_value() && !options.row_index.has_value()) {
        error_message = "--grid-column requires --row";
        return false;
    }
    if (options.cell_index.has_value() && options.grid_column.has_value()) {
        error_message = "--cell and --grid-column are mutually exclusive";
        return false;
    }

    return true;
}

auto parse_inspect_table_rows_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_table_rows_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--row") {
            if (options.row_index.has_value()) {
                error_message = "duplicate --row option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --row";
                return false;
            }

            std::size_t row_index = 0U;
            if (!parse_index(arguments[index + 1U], row_index)) {
                error_message =
                    "invalid row index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.row_index = row_index;
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
