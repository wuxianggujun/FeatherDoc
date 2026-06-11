#include "featherdoc_cli_table_position_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_position_options_parse_support.hpp"

#include <filesystem>
#include <string>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_apply_table_position_plan_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    apply_table_position_plan_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (const auto result = parse_table_position_output_option(
                arguments, index, options.output_path, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (argument == "--dry-run") {
            options.dry_run = true;
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

auto parse_plan_table_position_presets_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    plan_table_position_presets_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (const auto result = parse_table_position_preset_option(
                arguments, index, options.preset, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (argument == "--replace-positioned") {
            options.replace_positioned = true;
            continue;
        }

        if (argument == "--fail-on-change") {
            options.fail_on_change = true;
            continue;
        }

        if (const auto result = parse_table_position_output_option(
                arguments, index, options.output_path, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (argument == "--output-plan") {
            if (options.output_plan_path.has_value()) {
                error_message = "duplicate --output-plan option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output-plan";
                return false;
            }
            options.output_plan_path = path_type(std::string(arguments[index + 1U]));
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

    if (!options.preset.has_value()) {
        error_message =
            "missing required --preset <paragraph-callout|page-corner|margin-anchor>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
