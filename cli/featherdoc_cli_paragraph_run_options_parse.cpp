#include "featherdoc_cli_paragraph_run_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

namespace {

using path_type = std::filesystem::path;

template <typename Options>
auto parse_output_json_options(const std::vector<std::string_view> &arguments,
                               std::size_t start_index, Options &options,
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

} // namespace

auto parse_inspect_runs_options(const std::vector<std::string_view> &arguments,
                                std::size_t start_index,
                                inspect_runs_options &options,
                                std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--run") {
            if (options.run_index.has_value()) {
                error_message = "duplicate --run option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --run";
                return false;
            }

            std::size_t run_index = 0U;
            if (!parse_index(arguments[index + 1U], run_index)) {
                error_message =
                    "invalid run index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.run_index = run_index;
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

auto parse_set_paragraph_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_paragraph_style_options &options, std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options, error_message);
}

auto parse_clear_paragraph_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_paragraph_style_options &options, std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options, error_message);
}

auto parse_set_run_style_options(const std::vector<std::string_view> &arguments,
                                 std::size_t start_index,
                                 set_run_style_options &options,
                                 std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options, error_message);
}

auto parse_clear_run_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_run_style_options &options, std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options, error_message);
}

auto parse_set_run_font_family_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_run_font_family_options &options, std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options, error_message);
}

auto parse_clear_run_font_family_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_run_font_family_options &options, std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options, error_message);
}

auto parse_set_run_language_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_run_language_options &options, std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options, error_message);
}

auto parse_clear_run_language_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_run_language_options &options, std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options, error_message);
}

} // namespace featherdoc_cli
