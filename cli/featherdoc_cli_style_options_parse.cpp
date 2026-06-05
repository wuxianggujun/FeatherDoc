#include "featherdoc_cli_style_options_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_inspect_styles_options(const std::vector<std::string_view> &arguments,
                                  std::size_t start_index,
                                  inspect_styles_options &options,
                                  std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--style") {
            if (options.style_id.has_value()) {
                error_message = "duplicate --style option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style";
                return false;
            }

            options.style_id = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        if (argument == "--usage") {
            options.usage_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_inspect_style_inheritance_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_style_inheritance_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_rename_style_options(const std::vector<std::string_view> &arguments,
                                std::size_t start_index,
                                rename_style_options &options,
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

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
