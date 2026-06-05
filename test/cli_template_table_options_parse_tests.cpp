#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_template_table_json_patch_parse.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

auto temp_template_table_json_path(std::string_view suffix)
    -> std::filesystem::path {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("featherdoc_cli_template_table_json_" + std::to_string(stamp) +
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

TEST_CASE("cli template table options parse accepts selector targets") {
    std::string error;

    std::optional<std::size_t> table_index;
    std::optional<std::string> bookmark_name;
    std::size_t options_start_index = 0U;
    CHECK(featherdoc_cli::parse_template_table_target_arguments(
        {"template-table", "12", "--json"}, 1U, table_index, bookmark_name,
        options_start_index, error));
    REQUIRE(table_index.has_value());
    CHECK(*table_index == 12U);
    CHECK_FALSE(bookmark_name.has_value());
    CHECK(options_start_index == 2U);

    table_index.reset();
    bookmark_name.reset();
    error.clear();
    CHECK(featherdoc_cli::parse_template_table_target_arguments(
        {"template-table", "--bookmark", "chapter-1"}, 1U, table_index,
        bookmark_name, options_start_index, error));
    CHECK_FALSE(table_index.has_value());
    REQUIRE(bookmark_name.has_value());
    CHECK(*bookmark_name == "chapter-1");
    CHECK(options_start_index == 3U);

    featherdoc_cli::parsed_template_table_selector_target selector_target;
    error.clear();
    CHECK(featherdoc_cli::parse_template_table_selector_target_arguments(
        {"template-table", "--after-text", "Intro", "--header-cell", "Title"},
        1U, selector_target, false, error));
    REQUIRE(selector_target.selector.after_paragraph_text.has_value());
    CHECK(*selector_target.selector.after_paragraph_text == "Intro");
    REQUIRE(selector_target.selector.header_cell_texts.size() == 1U);
    CHECK(selector_target.selector.header_cell_texts.front() == "Title");
    CHECK(selector_target.options_start_index == 5U);

    featherdoc_cli::parsed_template_table_selector_row_target row_target;
    error.clear();
    CHECK(featherdoc_cli::parse_template_table_selector_row_target_arguments(
        {"template-table", "8", "4"}, 1U, row_target, error));
    REQUIRE(row_target.selector.table_index.has_value());
    CHECK(*row_target.selector.table_index == 8U);
    CHECK(row_target.row_index == 4U);
    CHECK(row_target.options_start_index == 3U);

    featherdoc_cli::parsed_template_table_selector_cell_target cell_target;
    error.clear();
    CHECK(featherdoc_cli::parse_template_table_selector_cell_target_arguments(
        {"template-table", "--bookmark", "table-a", "2", "5"}, 1U,
        cell_target, error));
    REQUIRE(cell_target.selector.bookmark_name.has_value());
    CHECK(*cell_target.selector.bookmark_name == "table-a");
    CHECK(cell_target.row_index == 2U);
    CHECK(cell_target.cell_index == 5U);
    CHECK(cell_target.options_start_index == 5U);

    featherdoc_cli::parsed_template_table_selector_optional_cell_target optional_cell_target;
    error.clear();
    CHECK(featherdoc_cli::parse_template_table_selector_optional_cell_target_arguments(
        {"template-table", "9", "3"}, 1U, optional_cell_target, error));
    REQUIRE(optional_cell_target.selector.table_index.has_value());
    CHECK(*optional_cell_target.selector.table_index == 9U);
    CHECK(optional_cell_target.row_index == 3U);
    CHECK_FALSE(optional_cell_target.cell_index.has_value());
    CHECK(optional_cell_target.options_start_index == 3U);
}

TEST_CASE("cli template table options parse accepts json patch and batch options") {
    std::string error;

    featherdoc_cli::template_table_json_patch_options patch_options;
    CHECK(featherdoc_cli::parse_template_table_json_patch_options(
        {"template-table-json-patch", "--part", "section-header",
         "--section", "0", "--patch-file", "patch.json", "--after-text",
         "Intro", "--header-cell", "Title", "--header-row", "1",
         "--occurrence", "2", "--output", "out.docx", "--json"},
        1U, patch_options, error));
    CHECK(patch_options.part == featherdoc_cli::validation_part_family::section_header);
    REQUIRE(patch_options.patch_file.has_value());
    REQUIRE(patch_options.output_path.has_value());
    CHECK(patch_options.patch_file->filename().string() == "patch.json");
    CHECK(patch_options.output_path->filename().string() == "out.docx");
    CHECK(patch_options.json_output);
    REQUIRE(patch_options.selector.after_paragraph_text.has_value());
    CHECK(*patch_options.selector.after_paragraph_text == "Intro");
    REQUIRE(patch_options.selector.header_cell_texts.size() == 1U);
    CHECK(patch_options.selector.header_cell_texts.front() == "Title");
    CHECK(patch_options.selector.header_row_index == 1U);
    CHECK(patch_options.selector.occurrence == 2U);

    featherdoc_cli::template_table_json_batch_options batch_options;
    error.clear();
    CHECK(featherdoc_cli::parse_template_table_json_batch_options(
        {"template-table-json-batch", "--part", "footer", "--patch-file",
         "batch.json", "--output", "out.docx", "--json"},
        1U, batch_options, error));
    CHECK(batch_options.part == featherdoc_cli::validation_part_family::footer);
    REQUIRE(batch_options.patch_file.has_value());
    REQUIRE(batch_options.output_path.has_value());
    CHECK(batch_options.patch_file->filename().string() == "batch.json");
    CHECK(batch_options.output_path->filename().string() == "out.docx");
    CHECK(batch_options.json_output);
}

TEST_CASE("cli template table JSON patch parse reads patch file") {
    const auto patch_path = temp_template_table_json_path("_patch.json");
    write_binary_file(patch_path,
                      "{\"mode\":\"block\",\"start_row\":1,"
                      "\"start_cell\":2,\"rows\":[[\"A\",3,true,null]]}");

    featherdoc_cli::template_table_json_patch patch;
    std::string error;
    CHECK(featherdoc_cli::read_template_table_json_patch(patch_path, patch,
                                                         error));

    CHECK(patch.mode == featherdoc_cli::template_table_json_patch_mode::block);
    CHECK(patch.start_row_index == 1U);
    CHECK(patch.start_cell_index == 2U);
    REQUIRE(patch.rows.size() == 1U);
    CHECK(patch.rows.front() ==
          std::vector<std::string>{"A", "3", "true", ""});
}

TEST_CASE("cli template table JSON batch parse reads operations") {
    const auto batch_path = temp_template_table_json_path("_batch.json");
    write_binary_file(batch_path,
                      "{\"operations\":[{\"part\":\"section-header\","
                      "\"section\":0,\"kind\":\"first\",\"after_text\":\"Intro\","
                      "\"header_cells\":[\"Name\"],\"header_row\":0,"
                      "\"occurrence\":2,\"start_row\":1,"
                      "\"rows\":[[\"Value\"]]}]}");

    std::vector<featherdoc_cli::template_table_json_batch_operation> operations;
    std::string error;
    CHECK(featherdoc_cli::read_template_table_json_batch(batch_path, operations,
                                                         error));

    REQUIRE(operations.size() == 1U);
    const auto &operation = operations.front();
    REQUIRE(operation.part.has_value());
    CHECK(*operation.part ==
          featherdoc_cli::validation_part_family::section_header);
    REQUIRE(operation.section_index.has_value());
    CHECK(*operation.section_index == 0U);
    REQUIRE(operation.reference_kind.has_value());
    CHECK(*operation.reference_kind ==
          featherdoc::section_reference_kind::first_page);
    REQUIRE(operation.selector.after_paragraph_text.has_value());
    CHECK(*operation.selector.after_paragraph_text == "Intro");
    CHECK(operation.selector.header_cell_texts ==
          std::vector<std::string>{"Name"});
    CHECK(operation.selector.header_row_index == 0U);
    CHECK(operation.selector.occurrence == 2U);
    CHECK(operation.patch.start_row_index == 1U);
    CHECK(operation.patch.rows == std::vector<std::vector<std::string>>{{"Value"}});
}

TEST_CASE("cli template table options parse reports validation errors") {
    featherdoc_cli::parsed_template_table_selector_row_target row_target;
    std::string row_error;
    CHECK_FALSE(featherdoc_cli::parse_template_table_selector_row_target_arguments(
        {"template-table", "--bookmark", "table-a"}, 1U, row_target,
        row_error));
    CHECK(row_error == "missing row index after table selector");

    featherdoc_cli::parsed_template_table_selector_cell_target cell_target;
    std::string cell_error;
    CHECK_FALSE(featherdoc_cli::parse_template_table_selector_cell_target_arguments(
        {"template-table", "8", "4", "oops"}, 1U, cell_target, cell_error));
    CHECK(cell_error == "invalid cell index: oops");

    featherdoc_cli::template_table_json_patch_options patch_options;
    std::string patch_error;
    CHECK_FALSE(featherdoc_cli::parse_template_table_json_patch_options(
        {"template-table-json-patch", "--json"}, 1U, patch_options,
        patch_error));
    CHECK(patch_error == "missing --patch-file <path>");

    featherdoc_cli::template_table_json_batch_options batch_options;
    std::string batch_error;
    CHECK_FALSE(featherdoc_cli::parse_template_table_json_batch_options(
        {"template-table-json-batch", "--bogus"}, 1U, batch_options,
        batch_error));
    CHECK(batch_error == "unknown option: --bogus");
}
