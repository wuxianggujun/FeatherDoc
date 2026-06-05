#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_field_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace {

auto parse_field(std::string_view command,
                 const std::vector<std::string_view> &arguments,
                 featherdoc_cli::append_field_options &options,
                 std::string &error_message) -> bool {
    return featherdoc_cli::parse_append_field_options(command, arguments, 0U,
                                                      options, error_message);
}

} // namespace

TEST_CASE("cli field parse recognizes field commands and names") {
    CHECK(featherdoc_cli::is_append_field_command("append-field"));
    CHECK(featherdoc_cli::is_append_field_command("append-page-number-field"));
    CHECK(featherdoc_cli::is_append_field_command("append-table-of-contents-field"));
    CHECK(featherdoc_cli::is_append_field_command("append-complex-field"));
    CHECK(featherdoc_cli::is_append_field_command("replace-field"));
    CHECK_FALSE(featherdoc_cli::is_append_field_command("append-bookmark"));

    CHECK(std::string_view{featherdoc_cli::template_field_name("append-field")} ==
          "field");
    CHECK(std::string_view{featherdoc_cli::template_field_name(
              "append-page-reference-field")} == "page_reference");
    CHECK(std::string_view{featherdoc_cli::template_field_name(
              "append-index-entry-field")} == "index_entry");
    CHECK(std::string_view{featherdoc_cli::template_field_name("replace-field")} ==
          "replacement");
}

TEST_CASE("cli field parse accepts page number field options") {
    featherdoc_cli::append_field_options options;
    std::string error;

    CHECK(parse_field("append-page-number-field",
                      {"--part", "body", "--dirty", "--locked", "--json",
                       "--output", "out.docx"},
                      options, error));

    CHECK(options.has_part);
    CHECK(options.part == featherdoc_cli::validation_part_family::body);
    CHECK(options.dirty);
    CHECK(options.locked);
    CHECK(options.json_output);
    REQUIRE(options.output_path.has_value());
    CHECK(options.output_path->filename().string() == "out.docx");
}

TEST_CASE("cli field parse accepts generic and replace arguments") {
    featherdoc_cli::append_field_options append_options;
    std::string append_error;
    CHECK(parse_field("append-field",
                      {"AUTHOR", "--part", "header", "--index", "2",
                       "--result-text", "Ada"},
                      append_options, append_error));
    CHECK(append_options.field_argument == "AUTHOR");
    CHECK(append_options.result_text == "Ada");
    REQUIRE(append_options.part_index.has_value());
    CHECK(*append_options.part_index == 2U);

    featherdoc_cli::append_field_options replace_options;
    std::string replace_error;
    CHECK(parse_field("replace-field",
                      {"3", "DATE", "--part", "body", "--result-text", "today"},
                      replace_options, replace_error));
    REQUIRE(replace_options.field_index.has_value());
    CHECK(*replace_options.field_index == 3U);
    CHECK(replace_options.field_argument == "DATE");
    CHECK(replace_options.result_text == "today");
}

TEST_CASE("cli field parse requires valid template part selection") {
    featherdoc_cli::append_field_options missing_part;
    std::string missing_part_error;
    CHECK_FALSE(parse_field("append-page-number-field", {}, missing_part,
                            missing_part_error));
    CHECK(missing_part_error ==
          "missing --part <body|header|footer|section-header|section-footer>");

    featherdoc_cli::append_field_options duplicate_part;
    std::string duplicate_part_error;
    CHECK_FALSE(parse_field("append-page-number-field",
                            {"--part", "body", "--part", "footer"},
                            duplicate_part, duplicate_part_error));
    CHECK(duplicate_part_error == "duplicate --part option");

    featherdoc_cli::append_field_options missing_section;
    std::string missing_section_error;
    CHECK_FALSE(parse_field("append-page-number-field",
                            {"--part", "section-header"}, missing_section,
                            missing_section_error));
    CHECK(missing_section_error ==
          "section-header/section-footer mutation requires --section <index>");
}

TEST_CASE("cli field parse rejects command-specific option misuse") {
    featherdoc_cli::append_field_options page_number;
    std::string page_number_error;
    CHECK_FALSE(parse_field("append-page-number-field",
                            {"--part", "body", "--result-text", "1"},
                            page_number, page_number_error));
    CHECK(page_number_error ==
          "append-page-number-field does not support --result-text");

    featherdoc_cli::append_field_options date;
    std::string date_error;
    CHECK_FALSE(parse_field("append-date-field", {"--part", "body", "--text", "x"},
                            date, date_error));
    CHECK(date_error == "append-date-field does not support --text");

    featherdoc_cli::append_field_options caption;
    std::string caption_error;
    CHECK_FALSE(parse_field("append-caption", {"Figure", "--part", "body"},
                            caption, caption_error));
    CHECK(caption_error == "append-caption requires --text <caption-text>");
}

TEST_CASE("cli field parse validates numeric boundaries") {
    featherdoc_cli::append_field_options toc_level;
    std::string toc_level_error;
    CHECK_FALSE(parse_field("append-table-of-contents-field",
                            {"--part", "body", "--min-outline-level", "4",
                             "--max-outline-level", "2"},
                            toc_level, toc_level_error));
    CHECK(toc_level_error ==
          "TOC minimum outline level must be less than or equal to maximum outline level");

    featherdoc_cli::append_field_options restart;
    std::string restart_error;
    CHECK_FALSE(parse_field("append-sequence-field",
                            {"SEQ", "--part", "body", "--restart", "0"},
                            restart, restart_error));
    CHECK(restart_error == "invalid restart value: 0");

    featherdoc_cli::append_field_options columns;
    std::string columns_error;
    CHECK_FALSE(parse_field("append-index-field",
                            {"--part", "body", "--columns", "0"}, columns,
                            columns_error));
    CHECK(columns_error == "invalid index columns: 0");
}

TEST_CASE("cli field parse accepts complex nested fields") {
    featherdoc_cli::append_field_options options;
    std::string error;

    CHECK(parse_field("append-complex-field",
                      {"--part", "body", "--instruction-before", "IF ",
                       "--nested-instruction", "PAGE", "--nested-result-text", "1",
                       "--instruction-after", " = 1 \"A\" \"B\"",
                       "--result-text", "A"},
                      options, error));

    REQUIRE(options.instruction_before.has_value());
    CHECK(*options.instruction_before == "IF ");
    REQUIRE(options.nested_instruction.has_value());
    CHECK(*options.nested_instruction == "PAGE");
    CHECK(options.nested_result_text == "1");
    REQUIRE(options.instruction_after.has_value());
    CHECK(*options.instruction_after == " = 1 \"A\" \"B\"");
    CHECK(options.result_text == "A");
}
