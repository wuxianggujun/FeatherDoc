#include "featherdoc_cli_run_properties_mutation_options_parse.hpp"

#include <filesystem>
#include <string>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

template <typename Options>
auto has_any_run_property_clear(const Options &options) -> bool {
    return options.clear_font_family || options.clear_east_asia_font_family ||
           options.clear_primary_language || options.clear_language ||
           options.clear_east_asia_language || options.clear_bidi_language ||
           options.clear_rtl || options.clear_paragraph_bidi;
}

template <typename Options>
auto parse_clear_run_properties_options_impl(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    Options &options, std::string &error_message,
    std::string_view required_option_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--font-family") {
            if (options.clear_font_family) {
                error_message = "duplicate --font-family option";
                return false;
            }
            options.clear_font_family = true;
            continue;
        }

        if (argument == "--east-asia-font-family") {
            if (options.clear_east_asia_font_family) {
                error_message = "duplicate --east-asia-font-family option";
                return false;
            }
            options.clear_east_asia_font_family = true;
            continue;
        }

        if (argument == "--primary-language") {
            if (options.clear_primary_language) {
                error_message = "duplicate --primary-language option";
                return false;
            }
            options.clear_primary_language = true;
            continue;
        }

        if (argument == "--language") {
            if (options.clear_language) {
                error_message = "duplicate --language option";
                return false;
            }
            options.clear_language = true;
            continue;
        }

        if (argument == "--east-asia-language") {
            if (options.clear_east_asia_language) {
                error_message = "duplicate --east-asia-language option";
                return false;
            }
            options.clear_east_asia_language = true;
            continue;
        }

        if (argument == "--bidi-language") {
            if (options.clear_bidi_language) {
                error_message = "duplicate --bidi-language option";
                return false;
            }
            options.clear_bidi_language = true;
            continue;
        }

        if (argument == "--rtl") {
            if (options.clear_rtl) {
                error_message = "duplicate --rtl option";
                return false;
            }
            options.clear_rtl = true;
            continue;
        }

        if (argument == "--paragraph-bidi") {
            if (options.clear_paragraph_bidi) {
                error_message = "duplicate --paragraph-bidi option";
                return false;
            }
            options.clear_paragraph_bidi = true;
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

    if (!has_any_run_property_clear(options)) {
        error_message = std::string(required_option_message);
        return false;
    }

    return true;
}

} // namespace

auto parse_clear_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_default_run_properties_options &options, std::string &error_message,
    std::string_view required_option_message) -> bool {
    return parse_clear_run_properties_options_impl(
        arguments, start_index, options, error_message, required_option_message);
}

auto parse_clear_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_style_run_properties_options &options, std::string &error_message,
    std::string_view required_option_message) -> bool {
    return parse_clear_run_properties_options_impl(
        arguments, start_index, options, error_message, required_option_message);
}

} // namespace featherdoc_cli
