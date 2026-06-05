#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_section_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli section options parse accepts update fields options") {
    featherdoc_cli::set_update_fields_on_open_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_set_update_fields_on_open_options(
        {"set-update-fields-on-open", "input.docx", "--enable", "--output",
         "out.docx", "--json"},
        2U, options, error));

    REQUIRE(options.enabled.has_value());
    CHECK(*options.enabled);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli section options parse validates update fields errors") {
    featherdoc_cli::set_update_fields_on_open_options missing_mode;
    std::string missing_mode_error;
    CHECK_FALSE(featherdoc_cli::parse_set_update_fields_on_open_options(
        {"set-update-fields-on-open", "input.docx", "--json"}, 2U,
        missing_mode, missing_mode_error));
    CHECK(missing_mode_error ==
          "set-update-fields-on-open requires --enable or --disable");

    featherdoc_cli::set_update_fields_on_open_options duplicate_mode;
    std::string duplicate_mode_error;
    CHECK_FALSE(featherdoc_cli::parse_set_update_fields_on_open_options(
        {"set-update-fields-on-open", "input.docx", "--enable", "--disable"},
        2U, duplicate_mode, duplicate_mode_error));
    CHECK(duplicate_mode_error == "--enable and --disable are mutually exclusive");

    featherdoc_cli::set_update_fields_on_open_options duplicate_output;
    std::string duplicate_output_error;
    CHECK_FALSE(featherdoc_cli::parse_set_update_fields_on_open_options(
        {"set-update-fields-on-open", "input.docx", "--disable", "--output",
         "a.docx", "--output", "b.docx"},
        2U, duplicate_output, duplicate_output_error));
    CHECK(duplicate_output_error == "duplicate --output option");
}

TEST_CASE("cli section options parse accepts inspect and set section text options") {
    featherdoc_cli::section_text_options inspect_options;
    std::string inspect_error;
    CHECK(featherdoc_cli::parse_section_text_options(
        {"inspect-section-header", "input.docx", "0", "--kind", "even",
         "--output", "header.txt", "--json"},
        3U, false, true, true, inspect_options, inspect_error));
    CHECK(inspect_options.reference_kind ==
          featherdoc::section_reference_kind::even_page);
    REQUIRE(inspect_options.output_path.has_value());
    CHECK(inspect_options.output_path->filename().string() == "header.txt");
    CHECK(inspect_options.json_output);

    featherdoc_cli::section_text_options set_options;
    std::string set_error;
    CHECK(featherdoc_cli::parse_section_text_options(
        {"set-section-footer", "input.docx", "1", "--kind", "first",
         "--text", "footer", "--output", "out.docx", "--json"},
        3U, true, true, true, set_options, set_error));
    CHECK(set_options.reference_kind ==
          featherdoc::section_reference_kind::first_page);
    REQUIRE(set_options.text.has_value());
    CHECK(*set_options.text == "footer");
    REQUIRE(set_options.output_path.has_value());
    CHECK(set_options.output_path->filename().string() == "out.docx");
    CHECK(set_options.json_output);
}

TEST_CASE("cli section options parse validates section text errors") {
    featherdoc_cli::section_text_options bad_kind;
    std::string bad_kind_error;
    CHECK_FALSE(featherdoc_cli::parse_section_text_options(
        {"inspect-section-header", "input.docx", "0", "--kind", "middle"},
        3U, false, true, true, bad_kind, bad_kind_error));
    CHECK(bad_kind_error == "invalid reference kind: middle");

    featherdoc_cli::section_text_options unsupported_json;
    std::string unsupported_json_error;
    CHECK_FALSE(featherdoc_cli::parse_section_text_options(
        {"inspect-section-header", "input.docx", "0", "--json"}, 3U, false,
        true, false, unsupported_json, unsupported_json_error));
    CHECK(unsupported_json_error == "--json is not supported by this command");

    featherdoc_cli::section_text_options unsupported_text;
    std::string unsupported_text_error;
    CHECK_FALSE(featherdoc_cli::parse_section_text_options(
        {"inspect-section-header", "input.docx", "0", "--text", "x"}, 3U,
        false, true, true, unsupported_text, unsupported_text_error));
    CHECK(unsupported_text_error ==
          "--text is only supported by set-section-* commands");

    featherdoc_cli::section_text_options duplicate_text;
    std::string duplicate_text_error;
    CHECK_FALSE(featherdoc_cli::parse_section_text_options(
        {"set-section-header", "input.docx", "0", "--text", "x",
         "--text-file", "header.txt"},
        3U, true, true, true, duplicate_text, duplicate_text_error));
    CHECK(duplicate_text_error == "text source was already specified");

    featherdoc_cli::section_text_options missing_text;
    std::string missing_text_error;
    CHECK_FALSE(featherdoc_cli::parse_section_text_options(
        {"set-section-header", "input.docx", "0", "--kind", "default"}, 3U,
        true, true, true, missing_text, missing_text_error));
    CHECK(missing_text_error == "expected --text <text> or --text-file <path>");
}
