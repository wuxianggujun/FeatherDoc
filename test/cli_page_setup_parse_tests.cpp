#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_page_setup_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli page setup parse accepts inspect options") {
    featherdoc_cli::inspect_page_setup_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_page_setup_options(
        {"inspect-page-setup", "input.docx", "--section", "2", "--json"},
        2U, options, error));

    REQUIRE(options.section_index.has_value());
    CHECK(*options.section_index == 2U);
    CHECK(options.json_output);
}

TEST_CASE("cli page setup parse validates inspect options") {
    featherdoc_cli::inspect_page_setup_options duplicate;
    std::string duplicate_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_page_setup_options(
        {"inspect-page-setup", "input.docx", "--section", "1", "--section",
         "2"},
        2U, duplicate, duplicate_error));
    CHECK(duplicate_error == "duplicate --section option");

    featherdoc_cli::inspect_page_setup_options invalid;
    std::string invalid_error;
    CHECK_FALSE(featherdoc_cli::parse_inspect_page_setup_options(
        {"inspect-page-setup", "input.docx", "--section", "bad"}, 2U,
        invalid, invalid_error));
    CHECK(invalid_error == "invalid section index: bad");
}

TEST_CASE("cli page setup parse accepts section page setup options") {
    featherdoc_cli::set_section_page_setup_options options;
    std::string error;

    CHECK(featherdoc_cli::parse_set_section_page_setup_options(
        {"set-section-page-setup", "input.docx", "0", "--orientation",
         "landscape", "--width", "15840", "--height", "12240",
         "--margin-top", "720", "--margin-bottom", "1080",
         "--margin-left", "1440", "--margin-right", "1440",
         "--margin-header", "360", "--margin-footer", "540",
         "--page-number-start", "5", "--output", "out.docx", "--json"},
        3U, options, error));

    REQUIRE(options.orientation.has_value());
    CHECK(*options.orientation == featherdoc::page_orientation::landscape);
    REQUIRE(options.width_twips.has_value());
    CHECK(*options.width_twips == 15840U);
    REQUIRE(options.height_twips.has_value());
    CHECK(*options.height_twips == 12240U);
    REQUIRE(options.margin_top_twips.has_value());
    CHECK(*options.margin_top_twips == 720U);
    REQUIRE(options.margin_bottom_twips.has_value());
    CHECK(*options.margin_bottom_twips == 1080U);
    REQUIRE(options.margin_left_twips.has_value());
    CHECK(*options.margin_left_twips == 1440U);
    REQUIRE(options.margin_right_twips.has_value());
    CHECK(*options.margin_right_twips == 1440U);
    REQUIRE(options.margin_header_twips.has_value());
    CHECK(*options.margin_header_twips == 360U);
    REQUIRE(options.margin_footer_twips.has_value());
    CHECK(*options.margin_footer_twips == 540U);
    REQUIRE(options.page_number_start.has_value());
    CHECK(*options.page_number_start == 5U);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
    CHECK(options.json_output);
}

TEST_CASE("cli page setup parse validates section page setup errors") {
    featherdoc_cli::set_section_page_setup_options bad_orientation;
    std::string bad_orientation_error;
    CHECK_FALSE(featherdoc_cli::parse_set_section_page_setup_options(
        {"set-section-page-setup", "input.docx", "0", "--orientation",
         "diagonal"},
        3U, bad_orientation, bad_orientation_error));
    CHECK(bad_orientation_error == "invalid orientation: diagonal");

    featherdoc_cli::set_section_page_setup_options bad_width;
    std::string bad_width_error;
    CHECK_FALSE(featherdoc_cli::parse_set_section_page_setup_options(
        {"set-section-page-setup", "input.docx", "0", "--width", "wide"},
        3U, bad_width, bad_width_error));
    CHECK(bad_width_error == "invalid value for --width: wide");

    featherdoc_cli::set_section_page_setup_options duplicate_page_number;
    std::string duplicate_page_number_error;
    CHECK_FALSE(featherdoc_cli::parse_set_section_page_setup_options(
        {"set-section-page-setup", "input.docx", "0", "--page-number-start",
         "1", "--clear-page-number-start"},
        3U, duplicate_page_number, duplicate_page_number_error));
    CHECK(duplicate_page_number_error == "duplicate page number start option");

    featherdoc_cli::set_section_page_setup_options duplicate_output;
    std::string duplicate_output_error;
    CHECK_FALSE(featherdoc_cli::parse_set_section_page_setup_options(
        {"set-section-page-setup", "input.docx", "0", "--output", "a.docx",
         "--output", "b.docx"},
        3U, duplicate_output, duplicate_output_error));
    CHECK(duplicate_output_error == "duplicate --output option");
}
