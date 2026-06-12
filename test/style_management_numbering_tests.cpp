#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("ensure_style_linked_numbering links multiple paragraph styles to one shared instance") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_linked_numbering_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto heading_one_style = featherdoc::paragraph_style_definition{};
    heading_one_style.name = "Legal Heading 1";
    heading_one_style.based_on = std::string{"Heading1"};
    heading_one_style.paragraph_bidi = false;
    CHECK(doc.ensure_paragraph_style("LegalHeading1", heading_one_style));

    auto heading_two_style = featherdoc::paragraph_style_definition{};
    heading_two_style.name = "Legal Heading 2";
    heading_two_style.based_on = std::string{"Heading2"};
    heading_two_style.paragraph_bidi = false;
    CHECK(doc.ensure_paragraph_style("LegalHeading2", heading_two_style));

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "LegalOutlineShared";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_style_linked_numbering(
        numbering_definition,
        {featherdoc::paragraph_style_numbering_link{"LegalHeading1", 0U},
         featherdoc::paragraph_style_numbering_link{"LegalHeading2", 1U}});
    REQUIRE(numbering_id.has_value());

    const auto heading_one = doc.find_style("LegalHeading1");
    const auto heading_two = doc.find_style("LegalHeading2");
    REQUIRE(heading_one.has_value());
    REQUIRE(heading_two.has_value());
    REQUIRE(heading_one->numbering.has_value());
    REQUIRE(heading_two->numbering.has_value());
    REQUIRE(heading_one->numbering->num_id.has_value());
    REQUIRE(heading_two->numbering->num_id.has_value());
    CHECK_EQ(*heading_one->numbering->num_id, *heading_two->numbering->num_id);
    REQUIRE(heading_one->numbering->definition_id.has_value());
    REQUIRE(heading_two->numbering->definition_id.has_value());
    CHECK_EQ(*heading_one->numbering->definition_id, *numbering_id);
    CHECK_EQ(*heading_two->numbering->definition_id, *numbering_id);
    REQUIRE(heading_one->numbering->level.has_value());
    REQUIRE(heading_two->numbering->level.has_value());
    CHECK_EQ(*heading_one->numbering->level, 0U);
    CHECK_EQ(*heading_two->numbering->level, 1U);

    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});

    const auto heading_one_xml = find_style_xml_node(styles_root, "LegalHeading1");
    const auto heading_two_xml = find_style_xml_node(styles_root, "LegalHeading2");
    REQUIRE(heading_one_xml != pugi::xml_node{});
    REQUIRE(heading_two_xml != pugi::xml_node{});

    const auto heading_one_num_pr = heading_one_xml.child("w:pPr").child("w:numPr");
    const auto heading_two_num_pr = heading_two_xml.child("w:pPr").child("w:numPr");
    REQUIRE(heading_one_num_pr != pugi::xml_node{});
    REQUIRE(heading_two_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{heading_one_num_pr.child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string_view{heading_two_num_pr.child("w:ilvl").attribute("w:val").value()},
             "1");
    CHECK_EQ(std::string{heading_one_num_pr.child("w:numId").attribute("w:val").value()},
             std::string{heading_two_num_pr.child("w:numId").attribute("w:val").value()});

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    CHECK_EQ(count_substring_occurrences(numbering_xml, "<w:num "), 1U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_heading_one = reopened.find_style("LegalHeading1");
    const auto reopened_heading_two = reopened.find_style("LegalHeading2");
    REQUIRE(reopened_heading_one.has_value());
    REQUIRE(reopened_heading_two.has_value());
    REQUIRE(reopened_heading_one->numbering.has_value());
    REQUIRE(reopened_heading_two->numbering.has_value());
    REQUIRE(reopened_heading_one->numbering->num_id.has_value());
    REQUIRE(reopened_heading_two->numbering->num_id.has_value());
    CHECK_EQ(*reopened_heading_one->numbering->num_id,
             *reopened_heading_two->numbering->num_id);

    fs::remove(target);
}

TEST_CASE("ensure_style_linked_numbering validates style links before mutating styles") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_linked_numbering_validation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Legal Heading";
    paragraph_style.based_on = std::string{"Heading1"};
    CHECK(doc.ensure_paragraph_style("LegalHeading", paragraph_style));

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "LegalOutlineInvalid";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    CHECK_FALSE(doc.ensure_style_linked_numbering(numbering_definition, {}).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.ensure_style_linked_numbering(
                    numbering_definition,
                    {featherdoc::paragraph_style_numbering_link{"LegalHeading", 0U},
                     featherdoc::paragraph_style_numbering_link{"LegalHeading", 0U}})
                    .has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.ensure_style_linked_numbering(
                    numbering_definition,
                    {featherdoc::paragraph_style_numbering_link{"MissingStyle", 0U}})
                    .has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.ensure_style_linked_numbering(
                    numbering_definition,
                    {featherdoc::paragraph_style_numbering_link{"LegalHeading", 1U}})
                    .has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}
