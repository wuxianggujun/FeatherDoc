#include "pdf_cli_import_test_support.hpp"

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
            remove_if_exists(output);
            remove_if_exists(json_output);

            CHECK_EQ(run_cli(arguments, json_output), 2);
            CHECK_FALSE(fs::exists(output));

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
