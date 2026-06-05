#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct inspect_runs_options {
    std::optional<std::size_t> run_index;
    bool json_output = false;
};

struct set_paragraph_style_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct clear_paragraph_style_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct set_run_style_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct clear_run_style_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct set_run_font_family_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct clear_run_font_family_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct set_run_language_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct clear_run_language_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

[[nodiscard]] auto parse_inspect_runs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_runs_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_set_paragraph_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_paragraph_style_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_clear_paragraph_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_paragraph_style_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_set_run_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_run_style_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_clear_run_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_run_style_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_set_run_font_family_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_run_font_family_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_clear_run_font_family_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_run_font_family_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_set_run_language_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_run_language_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_clear_run_language_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_run_language_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
