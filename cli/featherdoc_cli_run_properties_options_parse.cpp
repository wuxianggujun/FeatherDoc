#include "featherdoc_cli_run_properties_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

namespace {

using path_type = std::filesystem::path;

template <typename Options>
auto parse_json_only_options(const std::vector<std::string_view> &arguments,
                             std::size_t start_index, Options &options,
                             std::string &error_message) -> bool {
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
auto has_any_run_property_clear(const Options &options) -> bool {
    return options.clear_font_family || options.clear_east_asia_font_family ||
           options.clear_primary_language || options.clear_language ||
           options.clear_east_asia_language || options.clear_bidi_language ||
           options.clear_rtl || options.clear_paragraph_bidi;
}

template <typename Options>
auto parse_set_run_properties_options(
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

template <typename Options>
auto parse_clear_run_properties_options(
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

auto parse_inspect_default_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_default_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_json_only_options(arguments, start_index, options, error_message);
}

auto parse_set_default_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_default_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_set_run_properties_options(
        arguments, start_index, options, error_message,
        "set-default-run-properties requires at least one mutation option");
}

auto parse_clear_default_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_default_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_clear_run_properties_options(
        arguments, start_index, options, error_message,
        "clear-default-run-properties requires at least one clear option");
}

auto parse_inspect_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_style_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_json_only_options(arguments, start_index, options, error_message);
}

auto parse_inspect_paragraph_style_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_paragraph_style_properties_options &options,
    std::string &error_message) -> bool {
    return parse_json_only_options(arguments, start_index, options, error_message);
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

auto parse_materialize_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    materialize_style_run_properties_options &options,
    std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options, error_message);
}

auto parse_rebase_style_based_on_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    rebase_style_based_on_options &options, std::string &error_message) -> bool {
    return parse_output_json_options(arguments, start_index, options, error_message);
}

auto parse_set_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_style_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_set_run_properties_options(
        arguments, start_index, options, error_message,
        "set-style-run-properties requires at least one mutation option");
}

auto parse_clear_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_style_run_properties_options &options, std::string &error_message)
    -> bool {
    return parse_clear_run_properties_options(
        arguments, start_index, options, error_message,
        "clear-style-run-properties requires at least one clear option");
}

} // namespace featherdoc_cli
