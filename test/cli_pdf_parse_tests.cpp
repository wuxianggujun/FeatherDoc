#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_pdf_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace {

auto parse_export(const std::vector<std::string_view> &arguments,
                  featherdoc_cli::export_pdf_options &options,
                  std::string &error_message) -> bool {
    return featherdoc_cli::parse_export_pdf_options(arguments, 2U, options,
                                                    error_message);
}

auto parse_import(const std::vector<std::string_view> &arguments,
                  featherdoc_cli::import_pdf_options &options,
                  std::string &error_message) -> bool {
    return featherdoc_cli::parse_import_pdf_options(arguments, 2U, options,
                                                    error_message);
}

} // namespace

TEST_CASE("cli pdf parse accepts export options") {
    featherdoc_cli::export_pdf_options options;
    std::string error;

    CHECK(parse_export({"export-pdf", "input.docx", "--output", "out.pdf",
                        "--font-file", "base.ttf", "--cjk-font-file",
                        "cjk.ttf", "--font-map", "Body=body.ttf", "--title",
                        "Report", "--creator", "FeatherDoc",
                        "--render-headers-and-footers",
                        "--render-inline-images",
                        "--expand-header-footer-page-placeholders",
                        "--no-font-subset", "--no-system-font-fallbacks",
                        "--summary-json", "summary.json", "--json"},
                       options, error));

    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.pdf");
    REQUIRE(options.font_file_path.has_value());
    CHECK(options.font_file_path->filename().string() == "base.ttf");
    REQUIRE(options.cjk_font_file_path.has_value());
    CHECK(options.cjk_font_file_path->filename().string() == "cjk.ttf");
    REQUIRE(options.font_mappings.size() == 1U);
    CHECK(options.font_mappings[0].first == "Body");
    CHECK(options.font_mappings[0].second.filename().string() == "body.ttf");
    REQUIRE(options.title.has_value());
    CHECK(*options.title == "Report");
    REQUIRE(options.creator.has_value());
    CHECK(*options.creator == "FeatherDoc");
    CHECK(options.render_headers_and_footers);
    CHECK(options.render_inline_images);
    CHECK(options.expand_header_footer_page_placeholders);
    CHECK_FALSE(options.subset_unicode_fonts);
    CHECK_FALSE(options.use_system_font_fallbacks);
    REQUIRE(options.summary_json_path.has_value());
    CHECK(options.summary_json_path->filename().string() == "summary.json");
    CHECK(options.json_output);
}

TEST_CASE("cli pdf parse validates export errors") {
    featherdoc_cli::export_pdf_options missing_output;
    std::string missing_output_error;
    CHECK_FALSE(parse_export({"export-pdf", "input.docx"}, missing_output,
                             missing_output_error));
    CHECK(missing_output_error == "export-pdf requires --output <path>");

    featherdoc_cli::export_pdf_options duplicate_output;
    std::string duplicate_output_error;
    CHECK_FALSE(parse_export({"export-pdf", "input.docx", "--output", "a.pdf",
                              "--output", "b.pdf"},
                             duplicate_output, duplicate_output_error));
    CHECK(duplicate_output_error == "duplicate --output option");

    featherdoc_cli::export_pdf_options invalid_mapping;
    std::string invalid_mapping_error;
    CHECK_FALSE(parse_export({"export-pdf", "input.docx", "--output", "a.pdf",
                              "--font-map", "Body"},
                             invalid_mapping, invalid_mapping_error));
    CHECK(invalid_mapping_error ==
          "--font-map expects <font-family>=<font-file>");

    featherdoc_cli::export_pdf_options unknown_option;
    std::string unknown_option_error;
    CHECK_FALSE(parse_export({"export-pdf", "input.docx", "--output", "a.pdf",
                              "--wat"},
                             unknown_option, unknown_option_error));
    CHECK(unknown_option_error == "unknown option: --wat");
}

TEST_CASE("cli pdf parse accepts import options") {
    featherdoc_cli::import_pdf_options options;
    std::string error;

    CHECK(parse_import({"import-pdf", "input.pdf", "--output", "out.docx",
                        "--import-table-candidates-as-tables",
                        "--min-table-continuation-confidence", "75", "--json"},
                       options, error));

    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.import_table_candidates_as_tables);
    REQUIRE(options.min_table_continuation_confidence.has_value());
    CHECK(*options.min_table_continuation_confidence == 75U);
    CHECK(options.json_output);
}

TEST_CASE("cli pdf parse validates import errors") {
    featherdoc_cli::import_pdf_options missing_output;
    std::string missing_output_error;
    CHECK_FALSE(parse_import({"import-pdf", "input.pdf"}, missing_output,
                             missing_output_error));
    CHECK(missing_output_error == "import-pdf requires --output <path>");

    featherdoc_cli::import_pdf_options duplicate_confidence;
    std::string duplicate_confidence_error;
    CHECK_FALSE(parse_import({"import-pdf", "input.pdf", "--output", "out.docx",
                              "--min-table-continuation-confidence", "80",
                              "--min-table-continuation-confidence", "90"},
                             duplicate_confidence, duplicate_confidence_error));
    CHECK(duplicate_confidence_error ==
          "duplicate --min-table-continuation-confidence option");

    featherdoc_cli::import_pdf_options invalid_confidence;
    std::string invalid_confidence_error;
    CHECK_FALSE(parse_import({"import-pdf", "input.pdf", "--output", "out.docx",
                              "--min-table-continuation-confidence", "bad"},
                             invalid_confidence, invalid_confidence_error));
    CHECK(invalid_confidence_error ==
          "invalid value after --min-table-continuation-confidence");
}
