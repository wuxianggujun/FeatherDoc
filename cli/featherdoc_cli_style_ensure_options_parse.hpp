#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <featherdoc.hpp>

namespace featherdoc_cli {

struct ensure_paragraph_style_options {
    featherdoc::paragraph_style_definition definition;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct ensure_character_style_options {
    featherdoc::character_style_definition definition;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct ensure_table_style_options {
    featherdoc::table_style_definition definition;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

[[nodiscard]] auto parse_ensure_paragraph_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_paragraph_style_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_ensure_character_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_character_style_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_ensure_table_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_table_style_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
