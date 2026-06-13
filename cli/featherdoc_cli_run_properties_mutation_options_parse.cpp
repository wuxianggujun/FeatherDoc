#include "featherdoc_cli_run_properties_mutation_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

#include <filesystem>
#include <string>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

template <typename Options>
auto has_any_run_property_mutation(const Options &options) -> bool {
    return options.font_family.has_value() ||
           options.east_asia_font_family.has_value() ||
           options.language.has_value() ||
           options.east_asia_language.has_value() ||
           options.bidi_language.has_value() || options.rtl.has_value() ||
           options.paragraph_bidi.has_value();
}

template <typename Options>
auto parse_set_run_properties_options_impl(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    Options &options, std::string &error_message,
    std::string_view required_option_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--font-family") {
            if (options.font_family.has_value()) {
                error_message = "duplicate --font-family option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --font-family";
                return false;
            }

            options.font_family = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--east-asia-font-family") {
            if (options.east_asia_font_family.has_value()) {
                error_message = "duplicate --east-asia-font-family option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --east-asia-font-family";
                return false;
            }

            options.east_asia_font_family = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--language") {
            if (options.language.has_value()) {
                error_message = "duplicate --language option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --language";
                return false;
            }

            options.language = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--east-asia-language") {
            if (options.east_asia_language.has_value()) {
                error_message = "duplicate --east-asia-language option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --east-asia-language";
                return false;
            }

            options.east_asia_language = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--bidi-language") {
            if (options.bidi_language.has_value()) {
                error_message = "duplicate --bidi-language option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --bidi-language";
                return false;
            }

            options.bidi_language = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--rtl") {
            if (options.rtl.has_value()) {
                error_message = "duplicate --rtl option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --rtl";
                return false;
            }

            bool value = false;
            if (!parse_bool(arguments[index + 1U], value)) {
                error_message = "invalid --rtl value: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.rtl = value;
            ++index;
            continue;
        }

        if (argument == "--paragraph-bidi") {
            if (options.paragraph_bidi.has_value()) {
                error_message = "duplicate --paragraph-bidi option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --paragraph-bidi";
                return false;
            }

            bool value = false;
            if (!parse_bool(arguments[index + 1U], value)) {
                error_message = "invalid --paragraph-bidi value: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.paragraph_bidi = value;
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

    if (!has_any_run_property_mutation(options)) {
        error_message = std::string(required_option_message);
        return false;
    }

    return true;
}

} // namespace

auto parse_set_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_default_run_properties_options &options, std::string &error_message,
    std::string_view required_option_message) -> bool {
    return parse_set_run_properties_options_impl(
        arguments, start_index, options, error_message, required_option_message);
}

auto parse_set_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_style_run_properties_options &options, std::string &error_message,
    std::string_view required_option_message) -> bool {
    return parse_set_run_properties_options_impl(
        arguments, start_index, options, error_message, required_option_message);
}

} // namespace featherdoc_cli
