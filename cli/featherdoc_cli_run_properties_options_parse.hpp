#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct inspect_default_run_properties_options {
    bool json_output = false;
};

struct set_default_run_properties_options {
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<std::string> language;
    std::optional<std::string> east_asia_language;
    std::optional<std::string> bidi_language;
    std::optional<bool> rtl;
    std::optional<bool> paragraph_bidi;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct clear_default_run_properties_options {
    bool clear_font_family = false;
    bool clear_east_asia_font_family = false;
    bool clear_primary_language = false;
    bool clear_language = false;
    bool clear_east_asia_language = false;
    bool clear_bidi_language = false;
    bool clear_rtl = false;
    bool clear_paragraph_bidi = false;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct inspect_style_run_properties_options {
    bool json_output = false;
};

struct inspect_paragraph_style_properties_options {
    bool json_output = false;
};

struct set_paragraph_style_properties_options {
    std::optional<std::string> based_on;
    std::optional<std::string> next_style;
    std::optional<std::uint32_t> outline_level;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct clear_paragraph_style_properties_options {
    bool clear_based_on = false;
    bool clear_next_style = false;
    bool clear_outline_level = false;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct materialize_style_run_properties_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct rebase_style_based_on_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct set_style_run_properties_options {
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<std::string> language;
    std::optional<std::string> east_asia_language;
    std::optional<std::string> bidi_language;
    std::optional<bool> rtl;
    std::optional<bool> paragraph_bidi;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct clear_style_run_properties_options {
    bool clear_font_family = false;
    bool clear_east_asia_font_family = false;
    bool clear_primary_language = false;
    bool clear_language = false;
    bool clear_east_asia_language = false;
    bool clear_bidi_language = false;
    bool clear_rtl = false;
    bool clear_paragraph_bidi = false;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

[[nodiscard]] auto parse_inspect_default_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_default_run_properties_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_set_default_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_default_run_properties_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_clear_default_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_default_run_properties_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_inspect_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_style_run_properties_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_inspect_paragraph_style_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_paragraph_style_properties_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_set_paragraph_style_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_paragraph_style_properties_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_clear_paragraph_style_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_paragraph_style_properties_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_materialize_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    materialize_style_run_properties_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_rebase_style_based_on_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    rebase_style_based_on_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_set_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_style_run_properties_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_clear_style_run_properties_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_style_run_properties_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
