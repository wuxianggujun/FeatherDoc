#include "featherdoc_cli_style_ensure_common_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

#include <string>

namespace featherdoc_cli {
namespace {

template <typename Definition>
auto parse_style_catalog_option_impl(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    Definition &definition, std::string &error_message) -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--name") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --name";
            return option_parse_result::error;
        }

        definition.name = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--based-on") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --based-on";
            return option_parse_result::error;
        }

        definition.based_on = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--custom") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --custom";
            return option_parse_result::error;
        }

        bool value = false;
        if (!parse_bool(arguments[index + 1U], value)) {
            error_message =
                "invalid --custom value: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.is_custom = value;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--semi-hidden") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --semi-hidden";
            return option_parse_result::error;
        }

        bool value = false;
        if (!parse_bool(arguments[index + 1U], value)) {
            error_message = "invalid --semi-hidden value: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.is_semi_hidden = value;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--unhide-when-used") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --unhide-when-used";
            return option_parse_result::error;
        }

        bool value = false;
        if (!parse_bool(arguments[index + 1U], value)) {
            error_message = "invalid --unhide-when-used value: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.is_unhide_when_used = value;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--quick-format") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --quick-format";
            return option_parse_result::error;
        }

        bool value = false;
        if (!parse_bool(arguments[index + 1U], value)) {
            error_message = "invalid --quick-format value: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.is_quick_format = value;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

template <typename Definition>
auto parse_run_style_option_impl(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    Definition &definition, std::string &error_message) -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--run-font-family") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-font-family";
            return option_parse_result::error;
        }

        definition.run_font_family = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--run-east-asia-font-family") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-east-asia-font-family";
            return option_parse_result::error;
        }

        definition.run_east_asia_font_family =
            std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--run-language") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-language";
            return option_parse_result::error;
        }

        definition.run_language = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--run-east-asia-language") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-east-asia-language";
            return option_parse_result::error;
        }

        definition.run_east_asia_language =
            std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--run-bidi-language") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-bidi-language";
            return option_parse_result::error;
        }

        definition.run_bidi_language = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--run-rtl") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --run-rtl";
            return option_parse_result::error;
        }

        bool value = false;
        if (!parse_bool(arguments[index + 1U], value)) {
            error_message =
                "invalid --run-rtl value: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.run_rtl = value;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

} // namespace

auto parse_style_catalog_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::paragraph_style_definition &definition,
    std::string &error_message) -> option_parse_result {
    return parse_style_catalog_option_impl(arguments, index, definition,
                                           error_message);
}

auto parse_style_catalog_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::character_style_definition &definition,
    std::string &error_message) -> option_parse_result {
    return parse_style_catalog_option_impl(arguments, index, definition,
                                           error_message);
}

auto parse_style_catalog_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::table_style_definition &definition, std::string &error_message)
    -> option_parse_result {
    return parse_style_catalog_option_impl(arguments, index, definition,
                                           error_message);
}

auto parse_run_style_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::paragraph_style_definition &definition,
    std::string &error_message) -> option_parse_result {
    return parse_run_style_option_impl(arguments, index, definition,
                                       error_message);
}

auto parse_run_style_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::character_style_definition &definition,
    std::string &error_message) -> option_parse_result {
    return parse_run_style_option_impl(arguments, index, definition,
                                       error_message);
}

} // namespace featherdoc_cli
