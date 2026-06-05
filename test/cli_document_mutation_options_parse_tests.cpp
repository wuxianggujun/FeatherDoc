#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_document_mutation_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("document mutation options parse accepts base and revision options") {
    std::string error;

    featherdoc_cli::simple_document_mutation_options simple;
    CHECK(featherdoc_cli::parse_simple_document_mutation_options(
        {"command", "input.docx", "--output", "out.docx", "--json"}, 2U,
        simple, error));
    REQUIRE(simple.output_path.has_value());
    CHECK(simple.output_path->filename().string() == "out.docx");
    CHECK(simple.json_output);

    featherdoc_cli::revision_authoring_options revision;
    error.clear();
    CHECK(featherdoc_cli::parse_revision_authoring_options(
        {"command", "input.docx", "--text", "Edited", "--author", "Ada",
         "--date", "2026-06-05", "--output", "rev.docx"},
        2U, revision, error));
    CHECK(revision.has_text);
    CHECK(revision.text == "Edited");
    CHECK(revision.author == "Ada");
    CHECK(revision.date == "2026-06-05");
    REQUIRE(revision.output_path.has_value());
    CHECK(revision.output_path->filename().string() == "rev.docx");
}

TEST_CASE("document mutation options parse validates revision metadata") {
    std::string error;

    featherdoc_cli::revision_metadata_mutation_options metadata;
    CHECK(featherdoc_cli::parse_revision_metadata_mutation_options(
        {"command", "input.docx", "--author", "Ada", "--clear-date"}, 2U,
        metadata, error));
    CHECK(metadata.metadata.author.has_value());
    CHECK(metadata.metadata.clear_date);

    error.clear();
    featherdoc_cli::revision_metadata_mutation_options conflict;
    CHECK_FALSE(featherdoc_cli::parse_revision_metadata_mutation_options(
        {"command", "input.docx", "--author", "Ada", "--clear-author"}, 2U,
        conflict, error));
    CHECK(error ==
          "--clear-author conflicts with or duplicates --author");
}
