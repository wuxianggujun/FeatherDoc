#include "featherdoc_cli_style_ensure_table_options_parse.hpp"

#include "featherdoc_cli_style_ensure_table_region_parse.hpp"

#include <optional>
#include <string>

namespace featherdoc_cli {
namespace {

auto next_table_style_option_value(
    const std::vector<std::string_view> &arguments, std::size_t index,
    std::string_view option_name, std::string &error_message)
    -> std::optional<std::string_view> {
    if (index + 1U >= arguments.size()) {
        error_message = "missing value after " + std::string{option_name};
        return std::nullopt;
    }

    return arguments[index + 1U];
}

} // namespace

auto parse_ensure_table_style_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::table_style_definition &definition, std::string &error_message)
    -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--style-fill") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-fill", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_fill_option(*value, definition, error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-text-color") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-text-color", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_text_color_option(*value, definition,
                                                 error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-bold") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-bold", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_bold_option(*value, definition, error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-italic") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-italic", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_italic_option(*value, definition,
                                             error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-font-size") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-font-size", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_font_size_option(*value, definition,
                                                error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-font-family" ||
        argument == "--style-east-asia-font-family") {
        const auto value = next_table_style_option_value(arguments, index,
                                                        argument, error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_font_family_option(
                *value, definition,
                argument == "--style-east-asia-font-family", error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-cell-vertical-alignment") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-cell-vertical-alignment", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_cell_vertical_alignment_option(
                *value, definition, error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-cell-text-direction") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-cell-text-direction", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_cell_text_direction_option(
                *value, definition, error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-paragraph-alignment") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-paragraph-alignment", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_paragraph_alignment_option(
                *value, definition, error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-paragraph-spacing-before") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-paragraph-spacing-before",
            error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_paragraph_spacing_before_option(
                *value, definition, error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-paragraph-spacing-after") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-paragraph-spacing-after", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_paragraph_spacing_after_option(
                *value, definition, error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-paragraph-line-spacing") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-paragraph-line-spacing", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_paragraph_line_spacing_option(
                *value, definition, error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-margin") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-margin", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_margin_option(*value, definition,
                                             error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--style-border") {
        const auto value = next_table_style_option_value(
            arguments, index, "--style-border", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        if (!parse_table_style_border_option(*value, definition,
                                             error_message)) {
            return option_parse_result::error;
        }
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

} // namespace featherdoc_cli
