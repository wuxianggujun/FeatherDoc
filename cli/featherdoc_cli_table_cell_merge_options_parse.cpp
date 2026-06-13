#include "featherdoc_cli_table_cell_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

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

} // namespace featherdoc_cli
