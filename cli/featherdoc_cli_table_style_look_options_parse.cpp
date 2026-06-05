#include "featherdoc_cli_table_style_look_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_check_table_style_look_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    check_table_style_look_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--fail-on-issue") {
            options.fail_on_issue = true;
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

auto parse_repair_table_style_look_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    repair_table_style_look_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--plan-only") {
            options.plan_only = true;
            continue;
        }

        if (argument == "--apply") {
            options.apply = true;
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

    if (options.plan_only && options.apply) {
        error_message = "--plan-only and --apply are mutually exclusive";
        return false;
    }
    if (!options.apply) {
        options.plan_only = true;
    }
    if (options.plan_only && options.output_path.has_value()) {
        error_message = "--output requires --apply";
        return false;
    }
    if (options.apply && !options.output_path.has_value()) {
        error_message = "repair-table-style-look --apply requires --output <path>";
        return false;
    }

    return true;
}

auto table_style_look_options_have_flag(
    const table_style_look_options &options) -> bool {
    return options.first_row.has_value() || options.last_row.has_value() ||
           options.first_column.has_value() || options.last_column.has_value() ||
           options.banded_rows.has_value() || options.banded_columns.has_value();
}

auto parse_table_style_look_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_style_look_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];

        auto parse_bool_option = [&](std::optional<bool> &target,
                                     std::string_view option_name) -> bool {
            if (target.has_value()) {
                error_message = "duplicate " + std::string(option_name) + " option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after " + std::string(option_name);
                return false;
            }

            bool value = false;
            if (!parse_bool(arguments[index + 1U], value)) {
                error_message = "invalid " + std::string(option_name) +
                                " value: " + std::string(arguments[index + 1U]);
                return false;
            }

            target = value;
            ++index;
            return true;
        };

        if (argument == "--first-row") {
            if (!parse_bool_option(options.first_row, argument)) {
                return false;
            }
            continue;
        }

        if (argument == "--last-row") {
            if (!parse_bool_option(options.last_row, argument)) {
                return false;
            }
            continue;
        }

        if (argument == "--first-column") {
            if (!parse_bool_option(options.first_column, argument)) {
                return false;
            }
            continue;
        }

        if (argument == "--last-column") {
            if (!parse_bool_option(options.last_column, argument)) {
                return false;
            }
            continue;
        }

        if (argument == "--banded-rows") {
            if (!parse_bool_option(options.banded_rows, argument)) {
                return false;
            }
            continue;
        }

        if (argument == "--banded-columns") {
            if (!parse_bool_option(options.banded_columns, argument)) {
                return false;
            }
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
