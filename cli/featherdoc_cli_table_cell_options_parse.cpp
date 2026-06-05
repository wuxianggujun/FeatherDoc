#include "featherdoc_cli_table_cell_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_table_cell_text_options(const std::vector<std::string_view> &arguments,
                                   std::size_t start_index,
                                   table_cell_text_options &options,
                                   std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
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

        if (argument == "--text") {
            if (options.text.has_value() || options.text_file.has_value()) {
                error_message = "text source was already specified";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --text";
                return false;
            }

            options.text = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--text-file") {
            if (options.text.has_value() || options.text_file.has_value()) {
                error_message = "text source was already specified";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --text-file";
                return false;
            }

            options.text_file = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.text.has_value() && !options.text_file.has_value()) {
        error_message = "expected --text <text> or --text-file <path>";
        return false;
    }

    return true;
}

auto parse_table_merge_direction(std::string_view text,
                                 table_merge_direction &direction) -> bool {
    if (text == "right") {
        direction = table_merge_direction::right;
        return true;
    }
    if (text == "down") {
        direction = table_merge_direction::down;
        return true;
    }

    return false;
}

auto table_merge_direction_name(table_merge_direction direction) -> const char * {
    switch (direction) {
    case table_merge_direction::right:
        return "right";
    case table_merge_direction::down:
        return "down";
    }

    return "unknown";
}

auto parse_merge_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    merge_table_cells_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--direction") {
            if (options.has_direction) {
                error_message = "duplicate --direction option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --direction";
                return false;
            }

            if (!parse_table_merge_direction(arguments[index + 1U], options.direction)) {
                error_message = "invalid merge direction: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_direction = true;
            ++index;
            continue;
        }

        if (argument == "--count") {
            if (options.has_count) {
                error_message = "duplicate --count option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --count";
                return false;
            }

            std::size_t count = 0U;
            if (!parse_index(arguments[index + 1U], count)) {
                error_message =
                    "invalid merge count: " + std::string(arguments[index + 1U]);
                return false;
            }
            if (count == 0U) {
                error_message = "merge count must be greater than zero";
                return false;
            }

            options.count = count;
            options.has_count = true;
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
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

    if (!options.has_direction) {
        error_message = "missing required --direction <right|down>";
        return false;
    }

    return true;
}

auto parse_unmerge_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    unmerge_table_cells_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--direction") {
            if (options.has_direction) {
                error_message = "duplicate --direction option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --direction";
                return false;
            }

            if (!parse_table_merge_direction(arguments[index + 1U], options.direction)) {
                error_message = "invalid merge direction: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_direction = true;
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
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

    if (!options.has_direction) {
        error_message = "missing required --direction <right|down>";
        return false;
    }

    return true;
}

auto parse_table_cell_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_cell_style_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
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

auto parse_append_table_row_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    append_table_row_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--cell-count") {
            if (options.cell_count.has_value()) {
                error_message = "duplicate --cell-count option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --cell-count";
                return false;
            }

            std::size_t cell_count = 0U;
            if (!parse_index(arguments[index + 1U], cell_count)) {
                error_message =
                    "invalid cell count: " + std::string(arguments[index + 1U]);
                return false;
            }
            if (cell_count == 0U) {
                error_message = "cell count must be greater than 0";
                return false;
            }

            options.cell_count = cell_count;
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
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

auto parse_table_cell_border_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_cell_border_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--style") {
            if (options.style.has_value()) {
                error_message = "duplicate --style option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style";
                return false;
            }

            featherdoc::border_style style = featherdoc::border_style::single;
            if (!parse_border_style_text(arguments[index + 1U], style)) {
                error_message =
                    "invalid border style: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.style = style;
            ++index;
            continue;
        }

        if (argument == "--size") {
            if (options.size_eighth_points.has_value()) {
                error_message = "duplicate --size option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --size";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message =
                    "invalid border size: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.size_eighth_points = value;
            ++index;
            continue;
        }

        if (argument == "--color") {
            if (options.color.has_value()) {
                error_message = "duplicate --color option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --color";
                return false;
            }

            options.color = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--space") {
            if (options.space_points.has_value()) {
                error_message = "duplicate --space option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --space";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message =
                    "invalid border space: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.space_points = value;
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
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
