#include "featherdoc_cli_inspect_style_options_parse.hpp"

#include <filesystem>
#include <string>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_plan_table_style_quality_fixes_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    plan_table_style_quality_fixes_options &options,
    std::string &error_message) -> bool {
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

auto parse_apply_table_style_quality_fixes_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    apply_table_style_quality_fixes_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--look-only") {
            options.look_only = true;
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

    if (!options.look_only) {
        error_message = "apply-table-style-quality-fixes requires --look-only";
        return false;
    }
    if (!options.output_path.has_value()) {
        error_message = "apply-table-style-quality-fixes requires --output <path>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
