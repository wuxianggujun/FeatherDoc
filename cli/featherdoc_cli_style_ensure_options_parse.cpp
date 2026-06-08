#include "featherdoc_cli_style_ensure_options_parse.hpp"
#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_ensure_table_region_parse.hpp"

namespace featherdoc_cli {

namespace {

using path_type = std::filesystem::path;

template <typename Definition>
auto parse_style_catalog_option(const std::vector<std::string_view> &arguments,
                                std::size_t &index, Definition &definition,
                                std::string &error_message)
    -> option_parse_result {
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
            error_message = "invalid --custom value: " +
                            std::string(arguments[index + 1U]);
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
auto parse_run_style_option(const std::vector<std::string_view> &arguments,
                            std::size_t &index, Definition &definition,
                            std::string &error_message) -> option_parse_result {
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
            error_message = "invalid --run-rtl value: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        definition.run_rtl = value;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

} // namespace

auto parse_ensure_paragraph_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_paragraph_style_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (const auto result = parse_style_catalog_option(arguments, index,
                                                           options.definition,
                                                           error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_run_style_option(arguments, index,
                                                       options.definition,
                                                       error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        const auto argument = arguments[index];
        if (argument == "--next-style") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --next-style";
                return false;
            }

            options.definition.next_style = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--paragraph-bidi") {
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

            options.definition.paragraph_bidi = value;
            ++index;
            continue;
        }

        if (argument == "--outline-level") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --outline-level";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid outline level: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.definition.outline_level = value;
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

    if (options.definition.name.empty()) {
        error_message = "missing required --name <name>";
        return false;
    }

    return true;
}

auto parse_ensure_character_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_character_style_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (const auto result = parse_style_catalog_option(arguments, index,
                                                           options.definition,
                                                           error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_run_style_option(arguments, index,
                                                       options.definition,
                                                       error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

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

    if (options.definition.name.empty()) {
        error_message = "missing required --name <name>";
        return false;
    }

    return true;
}

auto parse_ensure_table_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_table_style_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (const auto result = parse_style_catalog_option(arguments, index,
                                                           options.definition,
                                                           error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        const auto argument = arguments[index];
        if (argument == "--style-fill") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-fill";
                return false;
            }
            if (!parse_table_style_fill_option(arguments[index + 1U],
                                               options.definition,
                                               error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-text-color") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-text-color";
                return false;
            }
            if (!parse_table_style_text_color_option(arguments[index + 1U],
                                                     options.definition,
                                                     error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-bold") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-bold";
                return false;
            }
            if (!parse_table_style_bold_option(arguments[index + 1U],
                                               options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-italic") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-italic";
                return false;
            }
            if (!parse_table_style_italic_option(arguments[index + 1U],
                                                 options.definition,
                                                 error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-font-size") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-font-size";
                return false;
            }
            if (!parse_table_style_font_size_option(arguments[index + 1U],
                                                    options.definition,
                                                    error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-font-family" ||
            argument == "--style-east-asia-font-family") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after " + std::string{argument};
                return false;
            }
            if (!parse_table_style_font_family_option(
                    arguments[index + 1U], options.definition,
                    argument == "--style-east-asia-font-family", error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-cell-vertical-alignment") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-cell-vertical-alignment";
                return false;
            }
            if (!parse_table_style_cell_vertical_alignment_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-cell-text-direction") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-cell-text-direction";
                return false;
            }
            if (!parse_table_style_cell_text_direction_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-paragraph-alignment") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-paragraph-alignment";
                return false;
            }
            if (!parse_table_style_paragraph_alignment_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-paragraph-spacing-before") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-paragraph-spacing-before";
                return false;
            }
            if (!parse_table_style_paragraph_spacing_before_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-paragraph-spacing-after") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-paragraph-spacing-after";
                return false;
            }
            if (!parse_table_style_paragraph_spacing_after_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-paragraph-line-spacing") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-paragraph-line-spacing";
                return false;
            }
            if (!parse_table_style_paragraph_line_spacing_option(
                    arguments[index + 1U], options.definition, error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-margin") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-margin";
                return false;
            }
            if (!parse_table_style_margin_option(arguments[index + 1U],
                                                 options.definition,
                                                 error_message)) {
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--style-border") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-border";
                return false;
            }
            if (!parse_table_style_border_option(arguments[index + 1U],
                                                 options.definition,
                                                 error_message)) {
                return false;
            }
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

    if (options.definition.name.empty()) {
        error_message = "missing required --name <name>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
