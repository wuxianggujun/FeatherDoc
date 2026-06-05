#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct inspect_styles_options {
    std::optional<std::string> style_id;
    bool usage_output = false;
    bool json_output = false;
};

struct inspect_style_inheritance_options {
    bool json_output = false;
};

struct rename_style_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

using merge_style_options = rename_style_options;
using prune_unused_styles_options = rename_style_options;

[[nodiscard]] auto parse_inspect_styles_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_styles_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_inspect_style_inheritance_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_style_inheritance_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_rename_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    rename_style_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
