#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto read_run_recipe_utf8_text_file(
    const std::filesystem::path &path, std::string_view label,
    std::string &content, std::string &error_message) -> bool;

[[nodiscard]] auto consume_run_recipe_json_object_separator(
    std::string_view content, std::size_t &index, std::string &error_message,
    bool &done) -> bool;

} // namespace featherdoc_cli
