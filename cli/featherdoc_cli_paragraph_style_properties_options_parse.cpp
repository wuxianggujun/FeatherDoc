#include "featherdoc_cli_run_properties_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_run_properties_common_options_parse.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_inspect_paragraph_style_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_paragraph_style_properties_options &options,
    std::string &error_message) -> bool {
    return parse_json_only_options(arguments, start_index, options,
                                   error_message);
}

auto parse_set_paragraph_style_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_paragraph_style_properties_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--based-on") {
            if (options.based_on.has_value()) {
                error_message = "duplicate --based-on option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --based-on";
                return false;
            }

            options.based_on = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--next-style") {
            if (options.next_style.has_value()) {
                error_message = "duplicate --next-style option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --next-style";
                return false;
            }

            options.next_style = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--outline-level") {
            if (options.outline_level.has_value()) {
                error_message = "duplicate --outline-level option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --outline-level";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid --outline-level value: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            if (value > 8U) {
                error_message = "invalid --outline-level value: expected 0-8";
                return false;
            }

            options.outline_level = value;
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

    if (!options.based_on.has_value() && !options.next_style.has_value() &&
        !options.outline_level.has_value()) {
        error_message =
            "set-paragraph-style-properties requires at least one mutation option";
        return false;
    }

    return true;
}

auto parse_clear_paragraph_style_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_paragraph_style_properties_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--based-on") {
            if (options.clear_based_on) {
                error_message = "duplicate --based-on option";
                return false;
            }
            options.clear_based_on = true;
            continue;
        }

        if (argument == "--next-style") {
            if (options.clear_next_style) {
                error_message = "duplicate --next-style option";
                return false;
            }
            options.clear_next_style = true;
            continue;
        }

        if (argument == "--outline-level") {
            if (options.clear_outline_level) {
                error_message = "duplicate --outline-level option";
                return false;
            }
            options.clear_outline_level = true;
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

    if (!options.clear_based_on && !options.clear_next_style &&
        !options.clear_outline_level) {
        error_message =
            "clear-paragraph-style-properties requires at least one clear option";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
