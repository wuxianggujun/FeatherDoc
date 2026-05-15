#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "cli_test_support.hpp"
#include "pdf_import_test_support.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

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

TEST_CASE("cli import-pdf reports table continuation diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_table_paragraph_pdf(
                "featherdoc-cli-import-pagebreak-table.pdf");
    const fs::path output = work_dir / "pagebreak-table.docx";
    const fs::path json_output = work_dir / "pagebreak-table-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    const auto table = document.inspect_table(0U);
    REQUIRE(table.has_value());
    CHECK_EQ(table->row_count, 6U);
    CHECK_EQ(table->column_count, 3U);

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics":[{)"),
             std::string::npos);
    CHECK_NE(json.find(R"("disposition":"created_new_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("disposition":"merged_with_previous_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"no_previous_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"none")"), std::string::npos);
    CHECK_NE(json.find(R"("continuation_confidence":85)"),
             std::string::npos);
    CHECK_NE(json.find(R"("minimum_continuation_confidence":0)"),
             std::string::npos);
    CHECK_NE(json.find(R"("is_first_block_on_page":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("is_near_page_top":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_count_matches":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_anchors_match":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_match_kind":"not_required")"),
             std::string::npos);
    CHECK_NE(json.find(R"("skipped_repeating_header":false)"),
             std::string::npos);
}

TEST_CASE(
    "cli import-pdf reports repeated header mismatch continuation diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_invoice_grid_pagebreak_subtotal_semantic_header_variant_pdf(
                "featherdoc-cli-import-pagebreak-semantic-header-mismatch.pdf");
    const fs::path output =
        work_dir / "pagebreak-semantic-header-mismatch.docx";
    const fs::path json_output =
        work_dir / "pagebreak-semantic-header-mismatch-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    REQUIRE(document.inspect_table(0U).has_value());
    REQUIRE(document.inspect_table(1U).has_value());
    CHECK_FALSE(document.inspect_table(2U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":2)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("previous_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_matches_previous":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_match_kind":"none")"),
             std::string::npos);
    CHECK_NE(json.find(R"("skipped_repeating_header":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("disposition":"created_new_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"repeated_header_mismatch")"),
             std::string::npos);
}

TEST_CASE(
    "cli import-pdf reports column anchor mismatch continuation diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_invoice_grid_pagebreak_subtotal_anchor_mismatch_pdf(
                "featherdoc-cli-import-pagebreak-anchor-mismatch.pdf");
    const fs::path output = work_dir / "pagebreak-anchor-mismatch.docx";
    const fs::path json_output =
        work_dir / "pagebreak-anchor-mismatch-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    REQUIRE(document.inspect_table(0U).has_value());
    REQUIRE(document.inspect_table(1U).has_value());
    CHECK_FALSE(document.inspect_table(2U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":2)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("previous_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_matches_previous":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_match_kind":"exact")"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_count_matches":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_anchors_match":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("skipped_repeating_header":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("disposition":"created_new_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"column_anchors_mismatch")"),
             std::string::npos);
}

TEST_CASE(
    "cli import-pdf reports continuation confidence threshold diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_table_paragraph_pdf(
                "featherdoc-cli-import-pagebreak-confidence-threshold.pdf");
    const fs::path output = work_dir / "pagebreak-confidence-threshold.docx";
    const fs::path json_output =
        work_dir / "pagebreak-confidence-threshold-import.json";
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
    REQUIRE(document.inspect_table(0U).has_value());
    REQUIRE(document.inspect_table(1U).has_value());
    CHECK_FALSE(document.inspect_table(2U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":2)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("min_table_continuation_confidence":90)"),
             std::string::npos);
    CHECK_NE(json.find(R"("continuation_confidence":85)"),
             std::string::npos);
    CHECK_NE(json.find(R"("minimum_continuation_confidence":90)"),
             std::string::npos);
    CHECK_NE(json.find(R"("disposition":"created_new_table")"),
             std::string::npos);
    CHECK_NE(
        json.find(R"("blocker":"continuation_confidence_below_threshold")"),
        std::string::npos);
}

TEST_CASE("cli import-pdf reports missing cell continuation merge diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_invoice_grid_pagebreak_subtotal_missing_unit_pdf(
                "featherdoc-cli-import-pagebreak-missing-unit.pdf");
    const fs::path output = work_dir / "pagebreak-missing-unit.docx";
    const fs::path json_output =
        work_dir / "pagebreak-missing-unit-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    const auto table = document.inspect_table(0U);
    REQUIRE(table.has_value());
    CHECK_EQ(table->row_count, 7U);
    CHECK_EQ(table->column_count, 4U);
    CHECK_FALSE(document.inspect_table(1U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":1)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("previous_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_matches_previous":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_match_kind":"exact")"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_count_matches":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_anchors_match":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("skipped_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_row_offset":1)"), std::string::npos);
    CHECK_NE(json.find(R"("disposition":"merged_with_previous_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"none")"), std::string::npos);
}

TEST_CASE("cli import-pdf reports sparse row continuation merge diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_invoice_grid_pagebreak_subtotal_sparse_body_pdf(
                "featherdoc-cli-import-pagebreak-sparse-body.pdf");
    const fs::path output = work_dir / "pagebreak-sparse-body.docx";
    const fs::path json_output =
        work_dir / "pagebreak-sparse-body-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    const auto table = document.inspect_table(0U);
    REQUIRE(table.has_value());
    CHECK_EQ(table->row_count, 7U);
    CHECK_EQ(table->column_count, 4U);
    CHECK_FALSE(document.inspect_table(1U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":1)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("previous_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_matches_previous":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_match_kind":"exact")"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_count_matches":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_anchors_match":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("skipped_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_row_offset":1)"), std::string::npos);
    CHECK_NE(json.find(R"("disposition":"merged_with_previous_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"none")"), std::string::npos);
}

TEST_CASE(
    "cli import-pdf reports column count mismatch continuation diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_invoice_grid_pagebreak_subtotal_column_count_mismatch_pdf(
                "featherdoc-cli-import-pagebreak-column-count-mismatch.pdf");
    const fs::path output =
        work_dir / "pagebreak-column-count-mismatch.docx";
    const fs::path json_output =
        work_dir / "pagebreak-column-count-mismatch-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->column_count, 4U);
    CHECK_EQ(second_table->column_count, 3U);
    CHECK_FALSE(document.inspect_table(2U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":2)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("previous_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_count_matches":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_anchors_match":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_matches_previous":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_match_kind":"none")"),
             std::string::npos);
    CHECK_NE(json.find(R"("skipped_repeating_header":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("disposition":"created_new_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"column_count_mismatch")"),
             std::string::npos);
}

TEST_CASE("cli import-pdf reports amount-only continuation merge diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_invoice_grid_pagebreak_subtotal_amount_only_body_pdf(
                "featherdoc-cli-import-pagebreak-amount-only-body.pdf");
    const fs::path output = work_dir / "pagebreak-amount-only-body.docx";
    const fs::path json_output =
        work_dir / "pagebreak-amount-only-body-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    const auto table = document.inspect_table(0U);
    REQUIRE(table.has_value());
    CHECK_EQ(table->row_count, 7U);
    CHECK_EQ(table->column_count, 4U);
    CHECK_FALSE(document.inspect_table(1U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":1)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("previous_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_matches_previous":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_match_kind":"exact")"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_count_matches":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_anchors_match":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("skipped_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_row_offset":1)"), std::string::npos);
    CHECK_NE(json.find(R"("disposition":"merged_with_previous_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"none")"), std::string::npos);
}

TEST_CASE(
    "cli import-pdf reports isolated amount-only continuation merge diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_invoice_grid_pagebreak_subtotal_isolated_amount_only_body_pdf(
                "featherdoc-cli-import-pagebreak-isolated-amount-only-body.pdf");
    const fs::path output =
        work_dir / "pagebreak-isolated-amount-only-body.docx";
    const fs::path json_output =
        work_dir / "pagebreak-isolated-amount-only-body-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    const auto table = document.inspect_table(0U);
    REQUIRE(table.has_value());
    CHECK_EQ(table->row_count, 6U);
    CHECK_EQ(table->column_count, 4U);
    CHECK_FALSE(document.inspect_table(1U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":1)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("previous_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_matches_previous":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("header_match_kind":"exact")"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_count_matches":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_anchors_match":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("skipped_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_row_offset":1)"), std::string::npos);
    CHECK_NE(json.find(R"("disposition":"merged_with_previous_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"none")"), std::string::npos);
}

TEST_CASE(
    "cli import-pdf reports local anchor drift continuation diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_invoice_grid_pagebreak_subtotal_local_anchor_drift_pdf(
                "featherdoc-cli-import-pagebreak-local-anchor-drift.pdf");
    const fs::path output = work_dir / "pagebreak-local-anchor-drift.docx";
    const fs::path json_output =
        work_dir / "pagebreak-local-anchor-drift-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    const auto first_table = document.inspect_table(0U);
    const auto second_table = document.inspect_table(1U);
    REQUIRE(first_table.has_value());
    REQUIRE(second_table.has_value());
    CHECK_EQ(first_table->column_count, second_table->column_count);
    CHECK_FALSE(document.inspect_table(2U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":2)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("previous_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_has_repeating_header":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_count_matches":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("column_anchors_match":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("skipped_repeating_header":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("source_row_offset":0)"), std::string::npos);
    CHECK_NE(json.find(R"("disposition":"created_new_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"column_anchors_mismatch")"),
             std::string::npos);
}

TEST_CASE(
    "cli import-pdf reports low page table continuation diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::
            write_paragraph_table_pagebreak_low_table_paragraph_pdf(
                "featherdoc-cli-import-pagebreak-low-table.pdf");
    const fs::path output = work_dir / "pagebreak-low-table.docx";
    const fs::path json_output =
        work_dir / "pagebreak-low-table-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    REQUIRE(document.inspect_table(0U).has_value());
    REQUIRE(document.inspect_table(1U).has_value());
    CHECK_FALSE(document.inspect_table(2U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":2)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("is_first_block_on_page":true)"),
             std::string::npos);
    CHECK_NE(json.find(R"("is_near_page_top":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("disposition":"created_new_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"not_near_page_top")"),
             std::string::npos);
}

TEST_CASE(
    "cli import-pdf reports non-first block continuation diagnostics") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source =
        featherdoc::test_support::write_paragraph_table_table_paragraph_pdf(
            "featherdoc-cli-import-paragraph-table-table.pdf");
    const fs::path output = work_dir / "paragraph-table-table.docx";
    const fs::path json_output =
        work_dir / "paragraph-table-table-import.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    REQUIRE(fs::exists(source));

    CHECK_EQ(run_cli({"import-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--import-table-candidates-as-tables",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));

    featherdoc::Document document;
    open_imported_document(output, document);
    REQUIRE(document.inspect_table(0U).has_value());
    REQUIRE(document.inspect_table(1U).has_value());
    CHECK_FALSE(document.inspect_table(2U).has_value());

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("tables_imported":2)"), std::string::npos);
    CHECK_NE(json.find(R"("table_continuation_diagnostics_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("is_first_block_on_page":false)"),
             std::string::npos);
    CHECK_NE(json.find(R"("disposition":"created_new_table")"),
             std::string::npos);
    CHECK_NE(json.find(R"("blocker":"not_first_block_on_page")"),
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

TEST_CASE("cli import-pdf reports confidence threshold parse errors as json") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_import";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path output = work_dir / "parse-error-output.docx";
    const auto assert_parse_error =
        [&](std::vector<std::string> arguments, const char *filename,
            const char *expected_message) {
            const fs::path json_output = work_dir / filename;
            remove_if_exists(json_output);

            CHECK_EQ(run_cli(arguments, json_output), 2);

            const auto json = read_text_file(json_output);
            CHECK_NE(json.find(R"("command":"import-pdf")"),
                     std::string::npos);
            CHECK_NE(json.find(R"("ok":false)"), std::string::npos);
            CHECK_NE(json.find(R"("stage":"parse")"), std::string::npos);
            CHECK_NE(json.find(expected_message), std::string::npos);
        };

    assert_parse_error({"import-pdf",
                        "input.pdf",
                        "--json",
                        "--output",
                        output.string(),
                        "--min-table-continuation-confidence"},
                       "missing-confidence-threshold.json",
                       "missing value after --min-table-continuation-confidence");
    assert_parse_error({"import-pdf",
                        "input.pdf",
                        "--output",
                        output.string(),
                        "--min-table-continuation-confidence",
                        "not-a-score",
                        "--json"},
                       "invalid-confidence-threshold.json",
                       "invalid value after --min-table-continuation-confidence");
    assert_parse_error({"import-pdf",
                        "input.pdf",
                        "--output",
                        output.string(),
                        "--min-table-continuation-confidence",
                        "80",
                        "--min-table-continuation-confidence",
                        "90",
                        "--json"},
                       "duplicate-confidence-threshold.json",
                       "duplicate --min-table-continuation-confidence option");
}
