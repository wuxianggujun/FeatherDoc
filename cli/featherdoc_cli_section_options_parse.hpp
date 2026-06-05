#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct set_update_fields_on_open_options {
    std::optional<bool> enabled;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct section_text_options {
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::filesystem::path> output_path;
    std::optional<std::string> text;
    std::optional<std::filesystem::path> text_file;
    bool json_output = false;
};

[[nodiscard]] auto parse_set_update_fields_on_open_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_update_fields_on_open_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_section_text_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    bool require_text_source, bool allow_output, bool allow_json,
    section_text_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
