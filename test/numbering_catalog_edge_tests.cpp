#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE(
    "custom numbering definition APIs validate inputs and reject missing definitions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "custom_numbering_validation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto invalid_definition = featherdoc::numbering_definition{};
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.name = "EmptyLevels";
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.name = "ReservedName";
    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 0U, 0U, "%1."},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 9U, "%1."},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, ""},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 0U, "o"},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.name = "FeatherDocBulletList";
    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 0U, "o"},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    auto valid_definition = featherdoc::numbering_definition{};
    valid_definition.name = "ValidOutline";
    valid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(valid_definition);
    REQUIRE(numbering_id.has_value());

    CHECK_FALSE(doc.set_paragraph_numbering(featherdoc::Paragraph{}, *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.find_numbering_instance(9999U).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK_FALSE(doc.set_paragraph_numbering(paragraph, 9999U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.set_paragraph_numbering(paragraph, *numbering_id, 1U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.set_paragraph_numbering(paragraph, *numbering_id, 9U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE(
    "set_paragraph_style_numbering links custom numbering definitions to paragraph styles") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_numbering_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Legal Heading";
    paragraph_style.based_on = std::string{"Heading1"};
    paragraph_style.paragraph_bidi = false;
    CHECK(doc.ensure_paragraph_style("LegalHeading", paragraph_style));

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "LegalHeadingOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(numbering_definition);
    REQUIRE(numbering_id.has_value());
    CHECK(doc.set_paragraph_style_numbering("LegalHeading", *numbering_id, 1U));

    const auto style_summary = doc.find_style("LegalHeading");
    REQUIRE(style_summary.has_value());
    REQUIRE(style_summary->numbering.has_value());
    REQUIRE(style_summary->numbering->num_id.has_value());
    REQUIRE(style_summary->numbering->level.has_value());
    CHECK_EQ(*style_summary->numbering->level, 1U);
    REQUIRE(style_summary->numbering->definition_id.has_value());
    CHECK_EQ(*style_summary->numbering->definition_id, *numbering_id);
    REQUIRE(style_summary->numbering->definition_name.has_value());
    CHECK_EQ(*style_summary->numbering->definition_name, "LegalHeadingOutline");
    REQUIRE(style_summary->numbering->instance.has_value());
    CHECK_EQ(style_summary->numbering->instance->instance_id,
             *style_summary->numbering->num_id);
    CHECK(style_summary->numbering->instance->level_overrides.empty());

    const auto styles = doc.list_styles();
    const auto *listed_style = find_style_summary(styles, "LegalHeading");
    REQUIRE(listed_style != nullptr);
    REQUIRE(listed_style->numbering.has_value());
    REQUIRE(listed_style->numbering->instance.has_value());
    CHECK_EQ(listed_style->numbering->instance->instance_id,
             *style_summary->numbering->num_id);

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_style(paragraph, "LegalHeading"));
    CHECK(paragraph.add_run("styled numbered heading").has_next());

    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});

    const auto style = find_style_xml_node(styles_root, "LegalHeading");
    REQUIRE(style != pugi::xml_node{});
    const auto style_num_pr = style.child("w:pPr").child("w:numPr");
    REQUIRE(style_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{style_num_pr.child("w:ilvl").attribute("w:val").value()}, "1");
    CHECK_NE(std::string_view{style_num_pr.child("w:numId").attribute("w:val").value()}, "");
    CHECK(style.child("w:pPr").child("w:bidi") != pugi::xml_node{});

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document document_xml;
    REQUIRE(document_xml.load_string(saved_document_xml.c_str()));
    const auto body = document_xml.child("w:document").child("w:body");
    const auto first_paragraph = body.child("w:p");
    REQUIRE(first_paragraph != pugi::xml_node{});
    CHECK(first_paragraph.child("w:pPr").child("w:pStyle") != pugi::xml_node{});
    CHECK(first_paragraph.child("w:pPr").child("w:numPr") == pugi::xml_node{});

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});
    CHECK_EQ(count_substring_occurrences(numbering_xml, "<w:num "), 1U);
    CHECK(find_numbering_abstract_xml_node(numbering_root, "LegalHeadingOutline") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_style = reopened.find_style("LegalHeading");
    REQUIRE(reopened_style.has_value());
    REQUIRE(reopened_style->numbering.has_value());
    REQUIRE(reopened_style->numbering->num_id.has_value());
    CHECK_EQ(*reopened_style->numbering->num_id, *style_summary->numbering->num_id);
    REQUIRE(reopened_style->numbering->level.has_value());
    CHECK_EQ(*reopened_style->numbering->level, 1U);
    REQUIRE(reopened_style->numbering->definition_id.has_value());
    CHECK_EQ(*reopened_style->numbering->definition_id, *numbering_id);
    REQUIRE(reopened_style->numbering->definition_name.has_value());
    CHECK_EQ(*reopened_style->numbering->definition_name, "LegalHeadingOutline");
    REQUIRE(reopened_style->numbering->instance.has_value());
    CHECK_EQ(reopened_style->numbering->instance->instance_id,
             *reopened_style->numbering->num_id);
    CHECK(reopened_style->numbering->instance->level_overrides.empty());
    auto inserted = reopened.paragraphs().insert_paragraph_after("");
    REQUIRE(inserted.has_next());
    CHECK(reopened.set_paragraph_style(inserted, "LegalHeading"));
    CHECK(inserted.add_run("reopened styled heading").has_next());
    CHECK_FALSE(reopened.save());

    fs::remove(target);
}

TEST_CASE(
    "paragraph style numbering APIs validate inputs and clear numbering without removing unrelated markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_numbering_validation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Body Numbered";
    paragraph_style.paragraph_bidi = true;
    CHECK(doc.ensure_paragraph_style("BodyNumbered", paragraph_style));

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "BodyOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(numbering_definition);
    REQUIRE(numbering_id.has_value());

    CHECK_FALSE(doc.set_paragraph_style_numbering("", *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("MissingStyle", *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("Emphasis", *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("BodyNumbered", 9999U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("BodyNumbered", *numbering_id, 1U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_paragraph_style_numbering("BodyNumbered", *numbering_id));
    CHECK(doc.clear_paragraph_style_numbering("BodyNumbered"));
    CHECK_FALSE(doc.clear_paragraph_style_numbering(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.clear_paragraph_style_numbering("MissingStyle"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.clear_paragraph_style_numbering("Emphasis"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});

    const auto style = find_style_xml_node(styles_root, "BodyNumbered");
    REQUIRE(style != pugi::xml_node{});
    CHECK(style.child("w:pPr").child("w:numPr") == pugi::xml_node{});
    CHECK(style.child("w:pPr").child("w:bidi") != pugi::xml_node{});

    fs::remove(target);
}
