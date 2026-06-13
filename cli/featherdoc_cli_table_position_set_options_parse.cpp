#include "featherdoc_cli_table_position_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_position_options_parse_support.hpp"
#include "featherdoc_cli_table_position_set_options_parse_detail.hpp"

#include <string>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto validate_table_position_options(
    const table_position_options &options, std::string &error_message) -> bool {
    if (!options.has_horizontal_reference) {
        error_message =
            "missing required --horizontal-reference <margin|page|column> or --preset";
        return false;
    }
    if (!options.has_horizontal_offset) {
        error_message = "missing required --horizontal-offset <twips>";
        return false;
    }
    if (!options.has_vertical_reference) {
        error_message =
            "missing required --vertical-reference <margin|page|paragraph>";
        return false;
    }
    if (!options.has_vertical_offset) {
        error_message = "missing required --vertical-offset <twips>";
        return false;
    }

    return true;
}

} // namespace

auto parse_table_position_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_position_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (const auto result = parse_table_position_table_option(
                arguments, index, options.additional_table_indices,
                error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_table_position_preset_option(
                arguments, index, options.preset, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_table_position_anchor_option(
                arguments, index, options, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_table_position_spacing_option(
                arguments, index, options, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
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

        const auto argument = arguments[index];
        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    apply_table_position_preset_defaults(options);
    return validate_table_position_options(options, error_message);
}

} // namespace featherdoc_cli
