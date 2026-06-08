#pragma once

#include <featherdoc.hpp>

#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto parse_table_style_fill_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_text_color_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_bold_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_italic_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_font_size_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_font_family_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    bool east_asia, std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_cell_vertical_alignment_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_cell_text_direction_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_paragraph_alignment_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_paragraph_spacing_before_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_paragraph_spacing_after_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_paragraph_line_spacing_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_margin_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_table_style_border_option(
    std::string_view text, featherdoc::table_style_definition &definition,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
