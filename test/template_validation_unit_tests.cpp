#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"
#include "basic_image_fixture_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("validate_template reports missing required bookmarks") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_missing.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {
            {"customer", featherdoc::template_slot_kind::text, true},
            {"invoice", featherdoc::template_slot_kind::text, true},
            {"notes", featherdoc::template_slot_kind::block, false},
        });

    CHECK_FALSE(doc.last_error());
    REQUIRE(result.missing_required.size() == 1);
    CHECK_EQ(result.missing_required.front(), "invoice");
    CHECK(result.duplicate_bookmarks.empty());
    CHECK(result.malformed_placeholders.empty());
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template reports duplicate bookmark names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_duplicates.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>customer A: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>first</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>customer B: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="customer"/>
      <w:r><w:t>second</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {{"customer", featherdoc::template_slot_kind::text, true}});

    CHECK_FALSE(doc.last_error());
    CHECK(result.missing_required.empty());
    REQUIRE(result.duplicate_bookmarks.size() == 1);
    CHECK_EQ(result.duplicate_bookmarks.front(), "customer");
    CHECK(result.malformed_placeholders.empty());
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template reports malformed block placeholders") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_malformed.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>prefix </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="items"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
      <w:r><w:t> suffix</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {{"items", featherdoc::template_slot_kind::block, true}});

    CHECK_FALSE(doc.last_error());
    CHECK(result.missing_required.empty());
    CHECK(result.duplicate_bookmarks.empty());
    REQUIRE(result.malformed_placeholders.size() == 1);
    CHECK_EQ(result.malformed_placeholders.front(), "items");
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template passes a valid mixed bookmark template") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_valid.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="1" w:name="intro_block"/>
      <w:r><w:t>intro placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="2" w:name="line_items"/>
            <w:r><w:t>row placeholder</w:t></w:r>
            <w:bookmarkEnd w:id="2"/>
          </w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
    <w:p>
      <w:bookmarkStart w:id="3" w:name="summary_table"/>
      <w:r><w:t>table placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="3"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="4" w:name="signature_image"/>
      <w:r><w:t>image placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="4"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="5" w:name="seal_image"/>
      <w:r><w:t>floating image placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="5"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {
            {"customer", featherdoc::template_slot_kind::text, true},
            {"intro_block", featherdoc::template_slot_kind::block, true},
            {"line_items", featherdoc::template_slot_kind::table_rows, true},
            {"summary_table", featherdoc::template_slot_kind::table, true},
            {"signature_image", featherdoc::template_slot_kind::image, true},
            {"seal_image", featherdoc::template_slot_kind::floating_image, true},
            {"notes", featherdoc::template_slot_kind::text, false},
        });

    CHECK_FALSE(doc.last_error());
    CHECK(result.missing_required.empty());
    CHECK(result.duplicate_bookmarks.empty());
    CHECK(result.malformed_placeholders.empty());
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template reports unexpected bookmarks kind mismatches and occurrence mismatches") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_schema_v2.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="summary_block"/>
      <w:r><w:t>standalone block placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>Approver: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="approver"/>
      <w:r><w:t>Alice</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:p>
      <w:r><w:t>Extra: </w:t></w:r>
      <w:bookmarkStart w:id="2" w:name="extra_slot"/>
      <w:r><w:t>Keep me declared or report me</w:t></w:r>
      <w:bookmarkEnd w:id="2"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {
            {"summary_block", featherdoc::template_slot_kind::text, true},
            {"approver", featherdoc::template_slot_kind::text, true, 2U, 2U},
        });

    CHECK_FALSE(doc.last_error());
    CHECK(result.missing_required.empty());
    CHECK(result.duplicate_bookmarks.empty());
    CHECK(result.malformed_placeholders.empty());
    REQUIRE(result.unexpected_bookmarks.size() == 1U);
    CHECK_EQ(result.unexpected_bookmarks.front().bookmark_name, "extra_slot");
    CHECK_EQ(result.unexpected_bookmarks.front().occurrence_count, 1U);
    CHECK_EQ(result.unexpected_bookmarks.front().kind, featherdoc::bookmark_kind::text);
    REQUIRE(result.kind_mismatches.size() == 1U);
    CHECK_EQ(result.kind_mismatches.front().bookmark_name, "summary_block");
    CHECK_EQ(result.kind_mismatches.front().expected_kind,
             featherdoc::template_slot_kind::text);
    CHECK_EQ(result.kind_mismatches.front().actual_kind,
             featherdoc::bookmark_kind::block);
    CHECK_EQ(result.kind_mismatches.front().occurrence_count, 1U);
    REQUIRE(result.occurrence_mismatches.size() == 1U);
    CHECK_EQ(result.occurrence_mismatches.front().bookmark_name, "approver");
    CHECK_EQ(result.occurrence_mismatches.front().actual_occurrences, 1U);
    CHECK_EQ(result.occurrence_mismatches.front().min_occurrences, 2U);
    REQUIRE(result.occurrence_mismatches.front().max_occurrences.has_value());
    CHECK_EQ(*result.occurrence_mismatches.front().max_occurrences, 2U);
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template allows declared multi occurrence bookmarks") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_multi_occurrence.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>first: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>first</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>second: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="customer"/>
      <w:r><w:t>second</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {{"customer", featherdoc::template_slot_kind::text, true, 2U, 2U}});

    CHECK_FALSE(doc.last_error());
    CHECK(result.missing_required.empty());
    CHECK(result.duplicate_bookmarks.empty());
    CHECK(result.malformed_placeholders.empty());
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table swaps a standalone bookmark paragraph for a table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const std::vector<std::vector<std::string>> rows{
        {"Name", "Qty"},
        {"Apple", "2"},
        {"Pear", "5"},
    };
    CHECK_EQ(doc.replace_bookmark_with_table("items", rows), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nafter\n");
    CHECK_EQ(collect_table_text(doc), "Name\nQty\nApple\n2\nPear\n5\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:tbl>"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    std::ostringstream child_order;
    const auto body = xml_document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        child_order << child.name() << '\n';
    }
    CHECK_EQ(child_order.str(), "w:p\nw:tbl\nw:p\n");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nafter\n");
    CHECK_EQ(collect_table_text(reopened), "Name\nQty\nApple\n2\nPear\n5\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_paragraphs expands a standalone bookmark paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_paragraphs.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_paragraphs("items", {"Apple", "Pear"}), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nApple\nPear\nafter\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:t>Apple</w:t>"), std::string::npos);
    CHECK_NE(saved_document_xml.find("<w:t>Pear</w:t>"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    std::ostringstream child_order;
    const auto body = xml_document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        child_order << child.name() << '\n';
    }
    CHECK_EQ(child_order.str(), "w:p\nw:p\nw:p\nw:p\n");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nApple\nPear\nafter\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_paragraphs accepts an empty replacement list") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_paragraphs_empty.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_paragraphs("items", std::vector<std::string>{}), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nafter\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("placeholder"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nafter\n");

    fs::remove(target);
}

TEST_CASE("remove_bookmark_block deletes a standalone bookmark paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_remove_block.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.remove_bookmark_block("missing"), 0);
    CHECK_FALSE(doc.last_error());

    CHECK_EQ(doc.remove_bookmark_block("items"), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nafter\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("placeholder"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nafter\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table_rows expands a template row inside an existing table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table_rows.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:trPr><w:cantSplit/></w:trPr>
        <w:tc>
          <w:tcPr><w:shd w:fill="AAAAAA"/></w:tcPr>
          <w:p>
            <w:pPr><w:jc w:val="center"/></w:pPr>
            <w:bookmarkStart w:id="0" w:name="item_row"/>
            <w:r><w:rPr><w:b/></w:rPr><w:t>template name</w:t></w:r>
            <w:bookmarkEnd w:id="0"/>
          </w:p>
        </w:tc>
        <w:tc>
          <w:tcPr><w:shd w:fill="BBBBBB"/></w:tcPr>
          <w:p>
            <w:pPr><w:jc w:val="right"/></w:pPr>
            <w:r><w:rPr><w:i/></w:rPr><w:t>template qty</w:t></w:r>
          </w:p>
        </w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>7</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_table_rows(
                 "item_row", {{"Apple", "2"}, {"Pear", "5"}}),
             1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_table_text(doc), "Name\nQty\nApple\n2\nPear\n5\nTotal\n7\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("template name"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("template qty"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"item_row\""), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:fill=\"AAAAAA\""), 2);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:fill=\"BBBBBB\""), 2);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto table = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table != pugi::xml_node{});
    CHECK_EQ(count_named_children(table, "w:tr"), 4);
    CHECK_EQ(count_named_descendants(table, "w:cantSplit"), 2);
    CHECK_EQ(count_named_descendants(table, "w:b"), 2);
    CHECK_EQ(count_named_descendants(table, "w:i"), 2);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "Name\nQty\nApple\n2\nPear\n5\nTotal\n7\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table_rows accepts an empty replacement list") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table_rows_empty.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="0" w:name="item_row"/>
            <w:r><w:t>template name</w:t></w:r>
            <w:bookmarkEnd w:id="0"/>
          </w:p>
        </w:tc>
        <w:tc><w:p><w:r><w:t>template qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>7</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_table_rows("item_row", {}), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_table_text(doc), "Name\nQty\nTotal\n7\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("template name"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("template qty"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"item_row\""), std::string::npos);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto table = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table != pugi::xml_node{});
    CHECK_EQ(count_named_children(table, "w:tr"), 2);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "Name\nQty\nTotal\n7\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table_rows rejects mismatched row widths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table_rows_validation.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="0" w:name="item_row"/>
            <w:r><w:t>template name</w:t></w:r>
            <w:bookmarkEnd w:id="0"/>
          </w:p>
        </w:tc>
        <w:tc><w:p><w:r><w:t>template qty</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_table_rows("item_row", {{"Apple"}}), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("cell count"), std::string::npos);
    CHECK_EQ(collect_table_text(doc), "template name\ntemplate qty\n");

    fs::remove(target);
}
