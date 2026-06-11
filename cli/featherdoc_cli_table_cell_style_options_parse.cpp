#include "featherdoc_cli_table_cell_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

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
