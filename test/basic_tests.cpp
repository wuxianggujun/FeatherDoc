#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"
#include "basic_image_fixture_test_support.hpp"
#include <featherdoc.hpp>
#include <zip.h>



TEST_CASE("apply_bookmark_block_visibility keeps and removes body template blocks") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_block_visibility.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="0" w:name="keep_block"/></w:p>
    <w:p><w:r><w:t>Keep me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="0"/></w:p>
    <w:p><w:r><w:t>middle</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="1" w:name="hide_block"/></w:p>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Secret Cell</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>Hide me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="1"/></w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.apply_bookmark_block_visibility({
        {"keep_block", true},
        {"hide_block", false},
    });
    CHECK_EQ(result.requested, 2);
    CHECK_EQ(result.matched, 2);
    CHECK_EQ(result.kept, 1);
    CHECK_EQ(result.removed, 1);
    CHECK(result);
    CHECK_FALSE(doc.last_error());

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("keep_block"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("hide_block"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Keep me"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("Hide me"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("Secret Cell"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nKeep me\nmiddle\nafter\n");

    fs::remove(target);
}

TEST_CASE("header template parts can change bookmark block visibility") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_block_visibility.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>body</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
</Relationships>
)";

    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:bookmarkStart w:id="0" w:name="offer_block"/></w:p>
  <w:p><w:r><w:t>Offer line 1</w:t></w:r></w:p>
  <w:p><w:r><w:t>Offer line 2</w:t></w:r></w:p>
  <w:p><w:bookmarkEnd w:id="0"/></w:p>
  <w:p><w:r><w:t>Always here</w:t></w:r></w:p>
</w:hdr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.set_bookmark_block_visibility("offer_block", false), 1);

    CHECK_FALSE(doc.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(saved_header.find("offer_block"), std::string::npos);
    CHECK_EQ(saved_header.find("Offer line 1"), std::string::npos);
    CHECK_EQ(saved_header.find("Offer line 2"), std::string::npos);
    CHECK_NE(saved_header.find("Always here"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(), "Always here");

    fs::remove(target);
}

TEST_CASE("apply_bookmark_block_visibility rejects duplicate or empty binding names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_block_visibility_validation.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document unopened(target);
    const auto unopened_result =
        unopened.apply_bookmark_block_visibility({{"promo_block", true}});
    CHECK_EQ(unopened_result.requested, 1);
    CHECK_EQ(unopened.last_error().code, featherdoc::document_errc::document_not_open);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto duplicate_result = doc.apply_bookmark_block_visibility(
        {{"promo_block", true}, {"promo_block", false}});
    CHECK_EQ(duplicate_result.requested, 2);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    const auto empty_result = doc.apply_bookmark_block_visibility({{"", true}});
    CHECK_EQ(empty_result.requested, 1);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("table traversal exposes text stored inside table cells") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_text_access.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>outside</w:t></w:r></w:p>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p><w:r><w:t>cell one</w:t></w:r></w:p>
          <w:p><w:r><w:t>cell two</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:p><w:r><w:t>cell three</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    std::ostringstream text;
    for (auto table = doc.tables(); table.has_next(); table.next()) {
        for (auto row = table.rows(); row.has_next(); row.next()) {
            for (auto cell = row.cells(); cell.has_next(); cell.next()) {
                for (auto paragraph = cell.paragraphs(); paragraph.has_next();
                     paragraph.next()) {
                    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
                        text << run.get_text();
                    }
                    text << '\n';
                }
            }
        }
    }

    CHECK_EQ(text.str(), "cell one\ncell two\ncell three\n");

    fs::remove(target);
}

TEST_CASE("fill_bookmarks replaces multiple bookmark bindings and reports misses") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_fill_batch.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>old customer</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>Invoice: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="invoice"/>
      <w:r><w:t>old invoice</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.fill_bookmarks(
        {
            {"customer", "Acme Corp"},
            {"invoice", "INV-2026-0001"},
            {"missing", "ignored"},
        });

    CHECK_FALSE(doc.last_error());
    CHECK_EQ(result.requested, 3);
    CHECK_EQ(result.matched, 2);
    CHECK_EQ(result.replaced, 2);
    REQUIRE(result.missing_bookmarks.size() == 1);
    CHECK_EQ(result.missing_bookmarks.front(), "missing");
    CHECK_FALSE(static_cast<bool>(result));
    CHECK_EQ(collect_document_text(doc), "Customer: Acme Corp\nInvoice: INV-2026-0001\n");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(xml_text.find("Acme Corp"), std::string::npos);
    CHECK_NE(xml_text.find("INV-2026-0001"), std::string::npos);
    CHECK_EQ(xml_text.find("old customer"), std::string::npos);
    CHECK_EQ(xml_text.find("old invoice"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened),
             "Customer: Acme Corp\nInvoice: INV-2026-0001\n");

    fs::remove(target);
}

TEST_CASE("fill_bookmarks rejects duplicate or empty binding names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_fill_validation.docx";
    fs::remove(target);

    featherdoc::Document unopened(target);
    const auto unopened_result = unopened.fill_bookmarks({{"customer", "Acme Corp"}});
    CHECK_EQ(unopened_result.requested, 1);
    CHECK_EQ(unopened.last_error().code, featherdoc::document_errc::document_not_open);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto duplicate_result = doc.fill_bookmarks(
        {
            {"customer", "Acme Corp"},
            {"customer", "Other"},
        });
    CHECK_EQ(duplicate_result.requested, 2);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("duplicate"), std::string::npos);

    const auto empty_result = doc.fill_bookmarks({{"", "blank"}});
    CHECK_EQ(empty_result.requested, 1);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("must not be empty"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("external hyperlinks can be appended and inspected") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "external_hyperlinks_append.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_EQ(doc.append_hyperlink("OpenAI", "https://openai.com/?a=1&b=2"), 1U);
    CHECK_FALSE(doc.last_error());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_EQ(body_template.append_hyperlink("FeatherDoc docs", "https://example.com/docs"),
             1U);
    CHECK_FALSE(doc.last_error());

    const auto links = doc.list_hyperlinks();
    REQUIRE_EQ(links.size(), 2U);
    CHECK_EQ(links[0].text, "OpenAI");
    REQUIRE(links[0].target.has_value());
    CHECK_EQ(*links[0].target, "https://openai.com/?a=1&b=2");
    CHECK(links[0].external);
    REQUIRE(links[0].relationship_id.has_value());
    CHECK_EQ(links[1].text, "FeatherDoc docs");
    REQUIRE(links[1].target.has_value());
    CHECK_EQ(*links[1].target, "https://example.com/docs");
    CHECK(links[1].external);

    const auto part_links = body_template.list_hyperlinks();
    REQUIRE_EQ(part_links.size(), 2U);
    CHECK_EQ(part_links[0].text, "OpenAI");
    CHECK_EQ(part_links[1].text, "FeatherDoc docs");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:hyperlink"), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:rStyle w:val=\"Hyperlink\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:color w:val=\"0563C1\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:u w:val=\"single\""),
             2U);
    CHECK_NE(saved_document_xml.find("OpenAI"), std::string::npos);
    CHECK_NE(saved_document_xml.find("FeatherDoc docs"), std::string::npos);

    const auto relationships_xml =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(relationships_xml, "/relationships/hyperlink"),
             2U);
    CHECK_NE(relationships_xml.find("TargetMode=\"External\""), std::string::npos);
    CHECK_NE(relationships_xml.find("Target=\"https://openai.com/?a=1&amp;b=2\""),
             std::string::npos);
    CHECK_NE(relationships_xml.find("Target=\"https://example.com/docs\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_links = reopened.list_hyperlinks();
    REQUIRE_EQ(reopened_links.size(), 2U);
    CHECK_EQ(reopened_links[0].text, "OpenAI");
    REQUIRE(reopened_links[0].target.has_value());
    CHECK_EQ(*reopened_links[0].target, "https://openai.com/?a=1&b=2");
    CHECK(reopened_links[0].external);
    CHECK_EQ(reopened_links[1].text, "FeatherDoc docs");
    REQUIRE(reopened_links[1].target.has_value());
    CHECK_EQ(*reopened_links[1].target, "https://example.com/docs");
    CHECK(reopened_links[1].external);

    fs::remove(target);
}

TEST_CASE("external hyperlink API validates text and target") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "external_hyperlinks_invalid.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_EQ(doc.append_hyperlink({}, "https://example.com"), 0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("text"), std::string::npos);

    CHECK_EQ(doc.append_hyperlink("Example", {}), 0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("target"), std::string::npos);

    CHECK(doc.list_hyperlinks().empty());
    CHECK_FALSE(doc.last_error());

    fs::remove(target);
}

TEST_CASE("external hyperlinks can be replaced and removed") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "external_hyperlinks_replace_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_EQ(doc.append_hyperlink("Old docs", "https://old.example/docs"), 1U);
    CHECK_EQ(doc.append_hyperlink("Temporary docs", "https://temp.example/docs"), 1U);
    CHECK(doc.replace_hyperlink(0U, "New docs", "https://new.example/docs?a=1&b=2"));
    CHECK(doc.remove_hyperlink(1U));
    CHECK_FALSE(doc.last_error());

    const auto links = doc.list_hyperlinks();
    REQUIRE_EQ(links.size(), 1U);
    CHECK_EQ(links.front().text, "New docs");
    REQUIRE(links.front().target.has_value());
    CHECK_EQ(*links.front().target, "https://new.example/docs?a=1&b=2");

    CHECK_FALSE(doc.replace_hyperlink(9U, "Missing", "https://missing.example"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "hyperlink index is out of range");

    CHECK_FALSE(doc.save());
    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:hyperlink"), 1U);
    CHECK_NE(saved_document_xml.find("New docs"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Temporary docs"), std::string::npos);

    const auto relationships_xml =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(relationships_xml, "/relationships/hyperlink"),
             1U);
    CHECK_NE(relationships_xml.find("Target=\"https://new.example/docs?a=1&amp;b=2\""),
             std::string::npos);
    CHECK_EQ(relationships_xml.find("temp.example"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_links = reopened.list_hyperlinks();
    REQUIRE_EQ(reopened_links.size(), 1U);
    CHECK_EQ(reopened_links.front().text, "New docs");

    fs::remove(target);
}

TEST_CASE("OMML builder helpers create appendable formulas") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "omml_builder_helpers.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto fraction = featherdoc::make_omml_fraction("a+b", "c", true);
    const auto superscript = featherdoc::make_omml_superscript("x", "2");
    const auto subscript = featherdoc::make_omml_subscript("a", "i");
    const auto radical = featherdoc::make_omml_radical("x+1");
    const auto delimited = featherdoc::make_omml_delimiter("x+1", "[", "]");
    const auto nary = featherdoc::make_omml_nary("∑", "i", "i=1", "n");
    const auto inline_text = featherdoc::make_omml_text("k=7");

    CHECK(doc.append_omml(fraction));
    CHECK(doc.append_omml(superscript));
    CHECK(doc.append_omml(subscript));
    CHECK(doc.replace_omml(0U, radical));
    CHECK(doc.append_omml(delimited));
    CHECK(doc.append_omml(nary));
    CHECK(doc.append_omml(inline_text));

    const auto summaries = doc.list_omml();
    REQUIRE_EQ(summaries.size(), 6U);
    CHECK(summaries[0].display);
    CHECK_NE(summaries[0].xml.find("m:rad"), std::string::npos);
    CHECK_FALSE(summaries[1].display);
    CHECK_NE(summaries[1].xml.find("m:sSup"), std::string::npos);
    CHECK_FALSE(summaries[2].display);
    CHECK_NE(summaries[2].xml.find("m:sSub"), std::string::npos);
    CHECK_FALSE(summaries[3].display);
    CHECK_NE(summaries[3].xml.find("m:d"), std::string::npos);
    CHECK(summaries[4].display);
    CHECK_NE(summaries[4].xml.find("m:nary"), std::string::npos);
    CHECK_EQ(summaries[5].text, "k=7");

    CHECK_FALSE(doc.save());
    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find("m:rad"), std::string::npos);
    CHECK_NE(saved_xml.find("m:sSup"), std::string::npos);
    CHECK_NE(saved_xml.find("m:sSub"), std::string::npos);
    CHECK_NE(saved_xml.find("m:d"), std::string::npos);
    CHECK_NE(saved_xml.find("m:nary"), std::string::npos);
    CHECK_NE(saved_xml.find("m:chr m:val=\"∑\""), std::string::npos);
    CHECK_NE(saved_xml.find("k=7"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("replace_bookmark_text preserves explicit line breaks inside bookmark ranges") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "bookmark_replace_multiline.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>prefix</w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="bookmark"/>
      <w:r><w:t>old value</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
      <w:r><w:t>suffix</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_text("bookmark", "first line\nsecond line"), 1);
    CHECK_EQ(collect_document_text(doc), "prefixfirst line\nsecond linesuffix\n");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto paragraph =
        xml_document.child("w:document").child("w:body").child("w:p");
    const auto bookmark_start = paragraph.child("w:bookmarkStart");
    REQUIRE(bookmark_start != pugi::xml_node{});
    const auto replacement_run = bookmark_start.next_sibling("w:r");
    REQUIRE(replacement_run != pugi::xml_node{});
    CHECK_EQ(std::string{replacement_run.child("w:t").text().get()}, "first line");
    REQUIRE(replacement_run.child("w:br") != pugi::xml_node{});
    CHECK_EQ(std::string{replacement_run.last_child().text().get()}, "second line");
    CHECK_NE(xml_text.find("w:bookmarkStart"), std::string::npos);
    CHECK_NE(xml_text.find("w:bookmarkEnd"), std::string::npos);
    CHECK_EQ(xml_text.find("old value"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened),
             "prefixfirst line\nsecond linesuffix\n");

    fs::remove(target);
}

TEST_CASE("validate_template rejects empty slot bookmark names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_empty_slot_name.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto result = doc.validate_template(
        {{"", featherdoc::template_slot_kind::text, true}});

    CHECK(result.missing_required.empty());
    CHECK(result.duplicate_bookmarks.empty());
    CHECK(result.malformed_placeholders.empty());
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "template slot bookmark name must not be empty");
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    fs::remove(target);
}
