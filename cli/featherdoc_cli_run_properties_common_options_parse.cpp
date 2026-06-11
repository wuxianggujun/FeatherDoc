#include "featherdoc_cli_run_properties_common_options_parse.hpp"

#include <filesystem>
#include <string>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

template <typename Options>
auto parse_json_only_options_impl(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    Options &options, std::string &error_message) -> bool {
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

template <typename Options>
auto parse_output_json_options_impl(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    Options &options, std::string &error_message) -> bool {
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

} // namespace

auto parse_json_only_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_default_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_json_only_options_impl(arguments, start_index, options,
                                        error_message);
}

auto parse_json_only_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_style_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_json_only_options_impl(arguments, start_index, options,
                                        error_message);
}

auto parse_json_only_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_paragraph_style_properties_options &options,
    std::string &error_message) -> bool {
    return parse_json_only_options_impl(arguments, start_index, options,
                                        error_message);
}

auto parse_output_json_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    materialize_style_run_properties_options &options,
    std::string &error_message) -> bool {
    return parse_output_json_options_impl(arguments, start_index, options,
                                          error_message);
}

auto parse_output_json_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    rebase_style_based_on_options &options, std::string &error_message) -> bool {
    return parse_output_json_options_impl(arguments, start_index, options,
                                          error_message);
}

} // namespace featherdoc_cli
