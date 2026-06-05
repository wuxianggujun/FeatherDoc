#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_semantic_diff_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("semantic diff options parse accepts flags") {
    std::string error;
    featherdoc_cli::semantic_diff_options options;

    CHECK(featherdoc_cli::parse_semantic_diff_options(
        {"semantic-diff", "left.docx", "right.docx", "--json",
         "--fail-on-diff", "--include-image-relationship-ids",
         "--include-content-control-ids", "--index-alignment",
         "--alignment-cell-limit", "12", "--no-paragraphs"},
        3U, options, error));

    CHECK(options.json_output);
    CHECK(options.fail_on_diff);
    CHECK(options.diff_options.compare_image_relationship_ids);
    CHECK(options.diff_options.compare_content_control_ids);
    CHECK_FALSE(options.diff_options.align_sequences_by_content);
    CHECK(options.diff_options.alignment_cell_limit == 12U);
    CHECK_FALSE(options.diff_options.compare_paragraphs);
}

TEST_CASE("semantic diff options parse rejects invalid alignment limit") {
    std::string error;
    featherdoc_cli::semantic_diff_options options;

    CHECK_FALSE(featherdoc_cli::parse_semantic_diff_options(
        {"semantic-diff", "left.docx", "right.docx", "--alignment-cell-limit",
         "oops"},
        3U, options, error));
    CHECK(error == "--alignment-cell-limit expects a non-negative integer");
}
