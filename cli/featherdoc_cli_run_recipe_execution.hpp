#pragma once

#include "featherdoc_cli_run_recipe_parse.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace featherdoc_cli {

struct run_recipe_output_record {
    std::string id;
    std::string type;
    std::string label;
    std::filesystem::path path;
};

struct run_recipe_batch_replace_result {
    std::size_t documents_count = 0U;
    std::size_t changed_count = 0U;
    std::size_t copied_count = 0U;
    std::size_t replacements_count = 0U;
    std::vector<run_recipe_output_record> outputs;
};

[[nodiscard]] auto execute_run_recipe_batch_replace(
    const std::unordered_map<std::string, std::string> &inputs,
    const std::filesystem::path &output_dir,
    run_recipe_batch_replace_result &result, std::string &error_message)
    -> bool;

void write_json_run_recipe_batch_replace_success(
    std::string_view recipe_id, const run_recipe_batch_replace_result &result);

void print_run_recipe_batch_replace_success(
    const run_recipe_batch_replace_result &result);

[[nodiscard]] auto report_run_recipe_execution_error(
    std::string_view stage, const std::string &message, bool json_output)
    -> int;

[[nodiscard]] auto execute_run_recipe(const run_recipe_options &options) -> int;

} // namespace featherdoc_cli
