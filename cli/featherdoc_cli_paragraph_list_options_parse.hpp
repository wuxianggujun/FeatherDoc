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

struct paragraph_list_options {
    featherdoc::list_kind kind = featherdoc::list_kind::bullet;
    bool has_kind = false;
    std::optional<std::uint32_t> level;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct clear_paragraph_list_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

[[nodiscard]] auto parse_paragraph_list_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    paragraph_list_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_clear_paragraph_list_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_paragraph_list_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
