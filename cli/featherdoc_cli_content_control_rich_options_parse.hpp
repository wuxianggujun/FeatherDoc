#pragma once

#include "featherdoc_cli_bookmark_text_options_parse.hpp"

namespace featherdoc_cli {

[[nodiscard]] auto parse_replace_content_control_paragraphs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_paragraphs_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_content_control_table_replacement_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    content_control_table_replacement_options &options, bool allow_empty_rows,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_replace_content_control_image_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_image_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
