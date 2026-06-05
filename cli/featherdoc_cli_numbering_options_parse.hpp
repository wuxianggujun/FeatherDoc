#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct set_paragraph_style_numbering_options {
    std::optional<std::string> definition_name;
    std::vector<featherdoc::numbering_level_definition> levels;
    std::optional<std::uint32_t> style_level;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct ensure_style_linked_numbering_options {
    std::optional<std::string> definition_name;
    std::vector<featherdoc::numbering_level_definition> levels;
    std::vector<featherdoc::paragraph_style_numbering_link> style_links;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct clear_paragraph_style_numbering_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct ensure_numbering_definition_options {
    std::optional<std::string> definition_name;
    std::vector<featherdoc::numbering_level_definition> levels;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct set_paragraph_numbering_options {
    std::optional<std::uint32_t> definition_id;
    std::optional<std::uint32_t> level;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

[[nodiscard]] auto parse_numbering_level_definition(
    std::string_view text, featherdoc::numbering_level_definition &definition,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_paragraph_style_numbering_link(
    std::string_view text, featherdoc::paragraph_style_numbering_link &style_link,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_set_paragraph_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_paragraph_style_numbering_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_ensure_style_linked_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_style_linked_numbering_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_clear_paragraph_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_paragraph_style_numbering_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_ensure_numbering_definition_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_numbering_definition_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_set_paragraph_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_paragraph_numbering_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
