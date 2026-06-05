#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <optional>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct run_recipe_options {
    std::optional<std::filesystem::path> recipe_path;
    std::optional<std::filesystem::path> inputs_path;
    std::optional<std::filesystem::path> output_dir;
    bool json_output = false;
};

[[nodiscard]] auto parse_run_recipe_options(
    const std::vector<std::string_view> &arguments, run_recipe_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto read_run_recipe_inputs_file(
    const std::filesystem::path &inputs_path,
    std::unordered_map<std::string, std::string> &inputs,
    std::string &error_message) -> bool;

[[nodiscard]] auto read_run_recipe_recipe_id(
    const std::filesystem::path &recipe_path, std::string &recipe_id,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
