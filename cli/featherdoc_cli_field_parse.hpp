#pragma once

#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct append_field_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::filesystem::path> output_path;
    std::string field_argument;
    std::string result_text;
    std::string caption_text;
    std::string number_result_text{"1"};
    std::string date_format{"yyyy-MM-dd"};
    std::string number_format{"ARABIC"};
    std::string separator{": "};
    std::optional<std::uint32_t> restart_value;
    std::optional<std::uint32_t> columns;
    std::optional<std::string> anchor;
    std::optional<std::string> tooltip;
    std::optional<std::string> subentry;
    std::optional<std::string> bookmark_name;
    std::optional<std::string> cross_reference;
    std::optional<std::string> instruction;
    std::optional<std::string> instruction_before;
    std::optional<std::string> instruction_after;
    std::optional<std::string> nested_instruction;
    std::optional<std::size_t> field_index;
    std::string nested_result_text;
    std::uint32_t min_outline_level = 1U;
    std::uint32_t max_outline_level = 3U;
    bool has_part = false;
    bool has_kind = false;
    bool has_field_argument = false;
    bool has_result_text = false;
    bool has_caption_text = false;
    bool has_number_result_text = false;
    bool has_field_index = false;
    bool hyperlink = true;
    bool relative_position = false;
    bool paragraph_number = false;
    bool preserve_formatting = true;
    bool hyperlinks = true;
    bool hide_page_numbers_in_web_layout = true;
    bool use_outline_levels = true;
    bool dirty = false;
    bool locked = false;
    bool bold_page_number = false;
    bool italic_page_number = false;
    bool json_output = false;
};

[[nodiscard]] auto parse_append_field_options(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t start_index, append_field_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto is_append_field_command(std::string_view command) -> bool;

[[nodiscard]] auto template_field_name(std::string_view command) -> const char *;

} // namespace featherdoc_cli
