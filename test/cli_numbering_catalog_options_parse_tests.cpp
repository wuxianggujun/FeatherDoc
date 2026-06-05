#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_numbering_catalog_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli numbering catalog options parse accepts export options") {
    featherdoc_cli::export_numbering_catalog_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_export_numbering_catalog_options(
        {"export-numbering-catalog", "input.docx", "--output", "catalog.json",
         "--json"},
        2U, options, error));

    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "catalog.json");
    CHECK(options.json_output);
}

TEST_CASE("cli numbering catalog options parse accepts catalog file commands") {
    featherdoc_cli::import_numbering_catalog_options import_options;
    std::string import_error;
    CHECK(featherdoc_cli::parse_import_numbering_catalog_options(
        {"import-numbering-catalog", "input.docx", "--catalog-file",
         "catalog.json", "--output", "out.docx", "--json"},
        2U, import_options, import_error));
    REQUIRE(import_options.catalog_path.has_value());
    CHECK(import_options.catalog_path->filename().string() == "catalog.json");
    REQUIRE(import_options.output_path.has_value());
    CHECK(import_options.output_path->filename().string() == "out.docx");
    CHECK(import_options.json_output);

    featherdoc_cli::check_numbering_catalog_options check_options;
    std::string check_error;
    CHECK(featherdoc_cli::parse_check_numbering_catalog_options(
        {"check-numbering-catalog", "input.docx", "--catalog-file",
         "catalog.json", "--output", "report.json", "--json"},
        2U, check_options, check_error));
    REQUIRE(check_options.catalog_path.has_value());
    CHECK(check_options.catalog_path->filename().string() == "catalog.json");
    REQUIRE(check_options.output_path.has_value());
    CHECK(check_options.output_path->filename().string() == "report.json");
    CHECK(check_options.json_output);
}

TEST_CASE("cli numbering catalog options parse accepts patch command") {
    featherdoc_cli::patch_numbering_catalog_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_patch_numbering_catalog_options(
        {"patch-numbering-catalog", "input.docx", "--patch-file",
         "patch.json", "--output", "out.docx", "--json"},
        2U, options, error));

    REQUIRE(options.patch_path.has_value());
    CHECK(options.patch_path->filename().string() == "patch.json");
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli numbering catalog options parse validates errors") {
    featherdoc_cli::import_numbering_catalog_options missing_catalog;
    std::string missing_catalog_error;
    CHECK_FALSE(featherdoc_cli::parse_import_numbering_catalog_options(
        {"import-numbering-catalog", "input.docx", "--json"}, 2U,
        missing_catalog, missing_catalog_error));
    CHECK(missing_catalog_error == "missing --catalog-file <catalog.json>");

    featherdoc_cli::import_numbering_catalog_options duplicate_catalog;
    std::string duplicate_catalog_error;
    CHECK_FALSE(featherdoc_cli::parse_import_numbering_catalog_options(
        {"import-numbering-catalog", "input.docx", "--catalog-file", "a.json",
         "--catalog-file", "b.json"},
        2U, duplicate_catalog, duplicate_catalog_error));
    CHECK(duplicate_catalog_error == "duplicate --catalog-file option");

    featherdoc_cli::patch_numbering_catalog_options missing_patch;
    std::string missing_patch_error;
    CHECK_FALSE(featherdoc_cli::parse_patch_numbering_catalog_options(
        {"patch-numbering-catalog", "input.docx", "--json"}, 2U,
        missing_patch, missing_patch_error));
    CHECK(missing_patch_error == "missing --patch-file <patch.json>");

    featherdoc_cli::export_numbering_catalog_options unknown;
    std::string unknown_error;
    CHECK_FALSE(featherdoc_cli::parse_export_numbering_catalog_options(
        {"export-numbering-catalog", "input.docx", "--bogus"}, 2U, unknown,
        unknown_error));
    CHECK(unknown_error == "unknown option: --bogus");
}
