#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_run_recipe_parse.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

auto temp_run_recipe_json_path(std::string_view suffix)
    -> std::filesystem::path {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_run_recipe_" + std::to_string(stamp) +
            std::string(suffix));
}

void write_binary_file(const std::filesystem::path &path,
                       std::string_view content) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    REQUIRE(stream.good());
}

} // namespace

TEST_CASE("cli run recipe parse accepts required paths") {
    featherdoc_cli::run_recipe_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_run_recipe_options(
        {"run-recipe", "--recipe", "recipe.json", "--inputs", "inputs.json",
         "--output", "out", "--json"},
        options, error));

    REQUIRE(options.recipe_path.has_value());
    CHECK(options.recipe_path->filename().string() == "recipe.json");
    REQUIRE(options.inputs_path.has_value());
    CHECK(options.inputs_path->filename().string() == "inputs.json");
    REQUIRE(options.output_dir.has_value());
    CHECK(options.output_dir->filename().string() == "out");
    CHECK(options.json_output);
}

TEST_CASE("cli run recipe parse accepts input and output aliases") {
    featherdoc_cli::run_recipe_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_run_recipe_options(
        {"run-recipe", "--input", "recipe.json", "--inputs", "inputs.json",
         "--output-dir", "out"},
        options, error));

    REQUIRE(options.recipe_path.has_value());
    CHECK(options.recipe_path->filename().string() == "recipe.json");
    REQUIRE(options.output_dir.has_value());
    CHECK(options.output_dir->filename().string() == "out");
    CHECK_FALSE(options.json_output);
}

TEST_CASE("cli run recipe parse validates errors") {
    featherdoc_cli::run_recipe_options duplicate_recipe;
    std::string duplicate_recipe_error;
    CHECK_FALSE(featherdoc_cli::parse_run_recipe_options(
        {"run-recipe", "--recipe", "a.json", "--input", "b.json", "--inputs",
         "inputs.json", "--output", "out"},
        duplicate_recipe, duplicate_recipe_error));
    CHECK(duplicate_recipe_error ==
          "run-recipe recipe path was provided more than once");

    featherdoc_cli::run_recipe_options missing_value;
    std::string missing_value_error;
    CHECK_FALSE(featherdoc_cli::parse_run_recipe_options(
        {"run-recipe", "--recipe", "recipe.json", "--inputs"}, missing_value,
        missing_value_error));
    CHECK(missing_value_error == "--inputs expects a path");

    featherdoc_cli::run_recipe_options missing_recipe;
    std::string missing_recipe_error;
    CHECK_FALSE(featherdoc_cli::parse_run_recipe_options(
        {"run-recipe", "--inputs", "inputs.json", "--output", "out"},
        missing_recipe, missing_recipe_error));
    CHECK(missing_recipe_error == "run-recipe expects --recipe <recipe.json>");

    featherdoc_cli::run_recipe_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_run_recipe_options(
        {"run-recipe", "--recipe", "recipe.json", "--inputs", "inputs.json",
         "--output", "out", "--bogus"},
        unknown, unknown_error));
    CHECK(unknown_error == "unknown run-recipe option: --bogus");
}

TEST_CASE("cli run recipe parse reads recipe id") {
    const auto recipe_path = temp_run_recipe_json_path("_recipe.json");
    write_binary_file(recipe_path,
                      "{\"description\":\"ignored\",\"id\":\"batch_replace\"}");

    std::string recipe_id;
    std::string error;
    CHECK(featherdoc_cli::read_run_recipe_recipe_id(recipe_path, recipe_id,
                                                    error));
    CHECK(recipe_id == "batch_replace");
}

TEST_CASE("cli run recipe parse reads nested inputs object") {
    const auto inputs_path = temp_run_recipe_json_path("_inputs.json");
    write_binary_file(inputs_path,
                      "{\"inputs\":{\"source_dir\":\"docs\","
                      "\"find_text\":\"old\",\"replace_text\":\"new\","
                      "\"count\":3,\"enabled\":true,\"note\":null}}");

    std::unordered_map<std::string, std::string> inputs;
    std::string error;
    CHECK(featherdoc_cli::read_run_recipe_inputs_file(inputs_path, inputs,
                                                      error));
    CHECK(inputs.at("source_dir") == "docs");
    CHECK(inputs.at("find_text") == "old");
    CHECK(inputs.at("replace_text") == "new");
    CHECK(inputs.at("count") == "3");
    CHECK(inputs.at("enabled") == "true");
    CHECK(inputs.at("note").empty());
}
