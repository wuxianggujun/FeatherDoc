#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "cli_test_support.hpp"
#include "pdf_import_test_support.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace {
namespace fs = std::filesystem;

auto pdf_fixture_path(std::string_view filename) -> fs::path {
    return test_binary_directory() / "pdf-fixtures" /
           std::string(filename);
}

void open_imported_document(const fs::path &path,
                            featherdoc::Document &document) {
    document.set_path(path);
    REQUIRE_FALSE(document.open());
}

} // namespace

TEST_CASE("cli import-pdf writes a DOCX file and json summary") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        pdf_fixture_path("featherdoc-adapter-cjk-roundtrip.pdf");
    const fs::path output = work_dir / "cjk-roundtrip.docx";
    const fs::path json_output = work_dir / "cjk-roundtrip-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    const auto text = collect_non_empty_document_text(document);
    CHECK_NE(text.find("Contract ABC"), std::string::npos);
    CHECK_EQ(document.inspect_body_blocks().size(), 1U);
    CHECK_FALSE(document.inspect_table(0U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"import-pdf")"), std::string::npos);
    CHECK_NE(json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(json.find(R"("input":)"), std::string::npos);
    CHECK_NE(json.find(R"("output":)"), std::string::npos);
    CHECK_NE(json.find(R"("paragraphs_imported":1)"), std::string::npos);
    CHECK_NE(json.find(R"("tables_imported":0)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":0)"),
             std::string::npos);
}

TEST_CASE("cli import-pdf can import table candidates as tables") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::write_table_like_grid_pdf(
            "featherdoc-cli-import-table-opt-in.pdf");
    const fs::path output = work_dir / "table-grid.docx";
    const fs::path json_output = work_dir / "table-grid-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--min-table-continuation-confidence",
                      "90",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    const auto text = collect_non_empty_document_text(document);
    CHECK_EQ(text, "Grid sample header\n");

    const auto blocks = document.inspect_body_blocks();
    REQUIRE_EQ(blocks.size(), 2U);
    CHECK_EQ(blocks[0].kind, featherdoc::body_block_kind::paragraph);
    CHECK_EQ(blocks[1].kind, featherdoc::body_block_kind::table);

    const auto table = document.inspect_table(0U);
    REQUIRE(table.has_value());
    CHECK_EQ(table->row_count, 3U);
    CHECK_EQ(table->column_count, 3U);
    REQUIRE(document.inspect_table_cell(0U, 0U, 0U).has_value());
    REQUIRE(document.inspect_table_cell(0U, 1U, 1U).has_value());
    REQUIRE(document.inspect_table_cell(0U, 2U, 2U).has_value());
    CHECK_EQ(document.inspect_table_cell(0U, 0U, 0U)->text, "Cell A1");
    CHECK_EQ(document.inspect_table_cell(0U, 1U, 1U)->text, "Cell B2");
    CHECK_EQ(document.inspect_table_cell(0U, 2U, 2U)->text, "Cell C3");

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"import-pdf")"), std::string::npos);
    CHECK_NE(json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(json.find(R"("paragraphs_imported":1)"), std::string::npos);
    CHECK_NE(json.find(R"("tables_imported":1)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":1)"),
             std::string::npos);
    CHECK_NE(json.find(R"("import_table_candidates_as_tables":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("min_table_continuation_confidence":90)"),
             std::string::npos);
}

TEST_CASE("cli import-pdf rejects table candidates by default") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::write_table_like_grid_pdf(
            "featherdoc-cli-import-table-rejected.pdf");
    const fs::path output = work_dir / "table-grid-rejected.docx";
    const fs::path json_output =
        work_dir / "table-grid-rejected-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--json"},
                     json_output),
             1);

    REQUIRE_FALSE(fs::exists(output));

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"import-pdf")"), std::string::npos);
    CHECK_NE(json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(json.find(R"("stage":"import")"), std::string::npos);
    CHECK_NE(json.find(R"("failure_kind":"table_candidates_detected")"),
             std::string::npos);
    CHECK_NE(json.find(R"("input":)"), std::string::npos);
    CHECK_NE(json.find(R"("output":)"), std::string::npos);
}
