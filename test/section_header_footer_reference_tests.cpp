#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "basic_document_xml_test_support.hpp"
#include "basic_docx_archive_test_support.hpp"
#include "doctest.h"

#include <featherdoc.hpp>

TEST_CASE("ensure_header_paragraphs and ensure_footer_paragraphs work for "
          "create_empty") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "ensure_header_footer_empty.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK_EQ(doc.section_count(), 1);

    auto header = doc.ensure_header_paragraphs();
    REQUIRE(header.has_next());
    CHECK(header.add_run("generated header").has_next());

    auto footer = doc.ensure_footer_paragraphs();
    REQUIRE(footer.has_next());
    CHECK(footer.add_run("generated footer").has_next());

    CHECK_EQ(doc.header_count(), 1);
    CHECK_EQ(doc.footer_count(), 1);
    CHECK_FALSE(doc.save());

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("xmlns:r="), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:headerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:footerReference"), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""),
             std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(),
             "generated header");
    CHECK_EQ(reopened.footer_paragraphs().runs().get_text(),
             "generated footer");

    fs::remove(target);
}

TEST_CASE("multiple relationship ids for one header part survive access prune "
          "move inspection and removal") {
    namespace fs = std::filesystem;
    const auto source = fs::current_path() / "header_relationship_alias.docx";
    const auto roundtrip =
        fs::current_path() / "header_relationship_alias_roundtrip.docx";
    const auto removed =
        fs::current_path() / "header_relationship_alias_removed.docx";
    const auto content_types_xml = std::string{R"(
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/header2.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
</Types>)"};
    const auto document_xml = std::string{R"(
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>body</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rHeaderAlias"/>
    </w:sectPr>
  </w:body>
</w:document>)"};
    const auto relationships_xml = std::string{R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rHeaderPrimary" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header1.xml"/>
  <Relationship Id="rHeaderAlias" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header1.xml"/>
  <Relationship Id="rHeaderUnused" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header2.xml"/>
</Relationships>)"};
    const auto header1_xml = std::string{R"(
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>alias header</w:t></w:r></w:p>
</w:hdr>)"};
    const auto header2_xml = std::string{R"(
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>unused header</w:t></w:r></w:p>
</w:hdr>)"};
    write_test_archive_entries(
        source, {{test_content_types_xml_entry, content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"word/_rels/document.xml.rels", relationships_xml},
                 {"word/header1.xml", header1_xml},
                 {"word/header2.xml", header2_xml}});

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE_EQ(document.header_count(), 2U);
    CHECK_EQ(document.section_header_paragraphs(0U).runs().get_text(),
             "alias header");
    const auto inspected = document.inspect_header_parts();
    REQUIRE_EQ(inspected.size(), 2U);
    CHECK_EQ(inspected[0].references.size(), 1U);
    REQUIRE(
        document.section_header_paragraphs(0U).set_text("alias header edited"));
    REQUIRE(document.move_header_part(0U, 1U));
    REQUIRE_FALSE(document.save_as(roundtrip));

    const auto saved_relationships =
        read_test_docx_entry(roundtrip, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("rHeaderPrimary"), std::string::npos);
    CHECK_NE(saved_relationships.find("rHeaderAlias"), std::string::npos);
    CHECK_EQ(saved_relationships.find("rHeaderUnused"), std::string::npos);
    CHECK(test_docx_entry_exists(roundtrip, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(roundtrip, "word/header2.xml"));

    featherdoc::Document reopened(roundtrip);
    REQUIRE_FALSE(reopened.open());
    REQUIRE_EQ(reopened.header_count(), 1U);
    CHECK_EQ(reopened.section_header_paragraphs(0U).runs().get_text(),
             "alias header edited");
    REQUIRE(reopened.remove_header_part(0U));
    REQUIRE_FALSE(reopened.save_as(removed));
    const auto removed_relationships =
        read_test_docx_entry(removed, "word/_rels/document.xml.rels");
    CHECK_EQ(removed_relationships.find("rHeaderPrimary"), std::string::npos);
    CHECK_EQ(removed_relationships.find("rHeaderAlias"), std::string::npos);
    CHECK_FALSE(test_docx_entry_exists(removed, "word/header1.xml"));

    fs::remove(source);
    fs::remove(roundtrip);
    fs::remove(removed);
}

TEST_CASE(
    "related part allocation reserves ASCII-case-equivalent package names") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "header_case_equivalent_allocation.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/WORD/HEADER1.XML"
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
      <w:headerReference w:type="default" r:id="rHeaderUpper"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rHeaderUpper"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="HEADER1.XML"/>
</Relationships>
)";
    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>uppercase header</w:t></w:r></w:p>
</w:hdr>
)";

    write_test_archive_entries(
        target, {{test_content_types_xml_entry, content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"word/_rels/document.xml.rels", document_relationships_xml},
                 {"word/HEADER1.XML", header_xml}});

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    REQUIRE_EQ(document.header_count(), 1U);
    REQUIRE(document.append_section(false));

    auto new_header = document.ensure_section_header_paragraphs(1U);
    REQUIRE(new_header.has_next());
    REQUIRE(new_header.add_run("new header two").has_next());
    CHECK_EQ(document.header_count(), 2U);
    REQUIRE_FALSE(document.save());

    const auto saved_entries = read_test_archive_entries(target);
    bool has_uppercase_header_one = false;
    bool has_lowercase_header_one = false;
    bool has_header_two = false;
    for (const auto &[entry_name, unused_content] : saved_entries) {
        (void)unused_content;
        has_uppercase_header_one |= entry_name == "word/HEADER1.XML";
        has_lowercase_header_one |= entry_name == "word/header1.xml";
        has_header_two |= entry_name == "word/header2.xml";
    }
    CHECK(has_uppercase_header_one);
    CHECK_FALSE(has_lowercase_header_one);
    CHECK(has_header_two);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"HEADER1.XML\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header2.xml\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 2U);
    CHECK_EQ(reopened.section_header_paragraphs(0U).runs().get_text(),
             "uppercase header");
    CHECK_EQ(reopened.section_header_paragraphs(1U).runs().get_text(),
             "new header two");

    fs::remove(target);
}

TEST_CASE("header allocation preserves orphan source parts and relationship "
          "sidecars") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "header_orphan_source_part_allocation.docx";
    fs::remove(target);

    const auto document_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body</w:t></w:r></w:p>
  </w:body>
</w:document>
)"};
    const auto orphan_header_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>orphan header one</w:t></w:r></w:p>
</w:hdr>
)"};
    const auto orphan_header_one_relationships = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <!-- preserve orphan header one sidecar -->
</Relationships>
)"};
    const auto orphan_header_two_relationships = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <!-- reserve header two through its sidecar -->
</Relationships>
)"};

    write_test_archive_entries(
        target,
        {{test_content_types_xml_entry, test_content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, document_xml},
         {"word/header1.xml", orphan_header_xml},
         {"word/_rels/header1.xml.rels", orphan_header_one_relationships},
         {"word/_rels/header2.xml.rels", orphan_header_two_relationships}});

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    REQUIRE_EQ(document.header_count(), 0U);
    auto new_header = document.ensure_header_paragraphs();
    REQUIRE(new_header.has_next());
    REQUIRE(new_header.add_run("allocated header three").has_next());
    REQUIRE_FALSE(document.save());

    CHECK_EQ(read_test_docx_entry(target, "word/header1.xml"),
             orphan_header_xml);
    CHECK_EQ(read_test_docx_entry(target, "word/_rels/header1.xml.rels"),
             orphan_header_one_relationships);
    CHECK_EQ(read_test_docx_entry(target, "word/_rels/header2.xml.rels"),
             orphan_header_two_relationships);
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/header3.xml"));
    const auto document_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(document_relationships.find("Target=\"header3.xml\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    REQUIRE_FALSE(reopened.open());
    REQUIRE_EQ(reopened.header_count(), 1U);
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(),
             "allocated header three");

    const auto late_header_three_relationships = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <!-- added after Document::open; stale save rejects, reopen preserves this payload -->
</Relationships>
)"};
    auto entries_after_open = read_test_archive_entries(target);
    entries_after_open.emplace_back("word/_rels/header3.xml.rels",
                                    late_header_three_relationships);
    write_test_archive_entries(target, entries_after_open);

    const auto stale_save_error = reopened.save();
    CHECK_EQ(stale_save_error,
             featherdoc::document_errc::source_archive_changed);
    CHECK_FALSE(reopened.last_error().entry_name.empty());
    CHECK_EQ(read_test_docx_entry(target, "word/_rels/header3.xml.rels"),
             late_header_three_relationships);

    // Reopening explicitly adopts the externally modified source snapshot.
    REQUIRE_FALSE(reopened.open());
    REQUIRE_FALSE(reopened.save());
    CHECK_NE(read_test_docx_entry(target, "word/_rels/header3.xml.rels")
                 .find("added after Document::open"),
             std::string::npos);
    CHECK_EQ(read_test_docx_entry(target, "word/header1.xml"),
             orphan_header_xml);
    CHECK_EQ(read_test_docx_entry(target, "word/_rels/header1.xml.rels"),
             orphan_header_one_relationships);
    CHECK_EQ(read_test_docx_entry(target, "word/_rels/header2.xml.rels"),
             orphan_header_two_relationships);

    featherdoc::Document verified(target);
    REQUIRE_FALSE(verified.open());
    REQUIRE_EQ(verified.header_count(), 1U);
    CHECK_EQ(verified.header_paragraphs().runs().get_text(),
             "allocated header three");

    fs::remove(target);
}

TEST_CASE("section header and footer access resolves references per section") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "section_header_footer_access.docx";
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
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/header3.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:headerReference w:type="even" r:id="rId5"/>
      <w:footerReference w:type="first" r:id="rId6"/>
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
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header3.xml"/>
  <Relationship Id="rId6"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 default header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header3_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 even header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 first footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/header3.xml", header3_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.header_count(), 3);
    CHECK_EQ(doc.footer_count(), 2);

    auto section0_header = doc.section_header_paragraphs(0);
    REQUIRE(section0_header.has_next());
    CHECK_EQ(section0_header.runs().get_text(), "section 1 header");

    auto section0_footer = doc.section_footer_paragraphs(0);
    REQUIRE(section0_footer.has_next());
    CHECK_EQ(section0_footer.runs().get_text(), "section 1 footer");

    auto section1_default_header = doc.section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK_EQ(section1_default_header.runs().get_text(),
             "section 2 default header");

    auto section1_even_header = doc.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page);
    REQUIRE(section1_even_header.has_next());
    CHECK_EQ(section1_even_header.runs().get_text(), "section 2 even header");

    auto section1_first_footer = doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK_EQ(section1_first_footer.runs().get_text(), "section 2 first footer");

    CHECK_FALSE(doc.section_header_paragraphs(
                       0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(1).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());

    CHECK(section1_default_header.runs().set_text("updated section 2 header"));
    CHECK(section1_even_header.runs().set_text("updated section 2 even"));
    CHECK(section1_first_footer.runs().set_text(
        "updated section 2 first footer"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 2);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(),
             "updated section 2 header");
    CHECK_EQ(reopened
                 .section_header_paragraphs(
                     1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "updated section 2 even");
    CHECK_EQ(reopened
                 .section_footer_paragraphs(
                     1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "updated section 2 first footer");

    fs::remove(target);
}

TEST_CASE("ensure section header and footer paragraphs create references for a "
          "single section") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "ensure_section_header_footer_single.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body text</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 1);
    CHECK_EQ(doc.header_count(), 0);
    CHECK_EQ(doc.footer_count(), 0);

    auto default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(default_header.has_next());
    CHECK(default_header.add_run("default header").has_next());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    auto first_footer = doc.ensure_section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.add_run("first footer").has_next());

    auto same_even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(same_even_header.has_next());
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 1);
    CHECK_EQ(same_even_header.runs().get_text(), "even header");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(),
                                       saved_document_xml.size()));
    const auto saved_section_properties =
        saved_document.child("w:document").child("w:body").child("w:sectPr");
    REQUIRE(saved_section_properties != pugi::xml_node{});
    CHECK_NE(saved_document_xml.find("<w:sectPr"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:headerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"default\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"even\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:footerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"first\""), std::string::npos);
    CHECK(saved_section_properties.child("w:titlePg") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header2.xml\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("relationships/settings"),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""),
             std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/settings.xml"), std::string::npos);

    const auto saved_settings_xml =
        read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(),
                                       saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 1);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(),
             "default header");
    CHECK_EQ(reopened
                 .section_header_paragraphs(
                     0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(reopened
                 .section_footer_paragraphs(
                     0, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");

    fs::remove(target);
}

TEST_CASE("replace section header and footer text rewrites parts cleanly") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "replace_section_header_footer_text.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.replace_section_header_text(0, "old header\nold second line"));
    CHECK(doc.replace_section_header_text(0, "new header"));
    CHECK(doc.replace_section_header_text(
        0, "even header", featherdoc::section_reference_kind::even_page));
    CHECK(doc.replace_section_footer_text(
        0, " first footer ", featherdoc::section_reference_kind::first_page));
    CHECK_FALSE(doc.save());

    auto collect_section_part_lines =
        [](featherdoc::Paragraph paragraph) -> std::vector<std::string> {
        std::vector<std::string> lines;
        for (; paragraph.has_next(); paragraph.next()) {
            std::string text;
            for (auto run = paragraph.runs(); run.has_next(); run.next()) {
                text += run.get_text();
            }
            lines.push_back(std::move(text));
        }
        return lines;
    };

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 1);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(collect_section_part_lines(reopened.section_header_paragraphs(0)),
             std::vector<std::string>{"new header"});
    CHECK_EQ(collect_section_part_lines(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>{"even header"});
    CHECK_EQ(collect_section_part_lines(reopened.section_footer_paragraphs(
                 0, featherdoc::section_reference_kind::first_page)),
             std::vector<std::string>{" first footer "});

    const auto saved_settings_xml =
        read_test_docx_entry(target, "word/settings.xml");
    CHECK_NE(saved_settings_xml.find("w:evenAndOddHeaders"), std::string::npos);

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.replace_section_header_text(0, "missing"));
    CHECK_EQ(invalid.last_error().code,
             featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("successful section text replacement invalidates only the replaced "
          "part handles") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto body = document.paragraphs();
    auto body_run = body.add_run("body remains live");
    REQUIRE(body.valid());
    REQUIRE(body_run.valid());

    auto header = document.ensure_section_header_paragraphs(0U);
    auto header_run = header.add_run("old header");
    REQUIRE(header.valid());
    REQUIRE(header_run.valid());

    auto footer = document.ensure_section_footer_paragraphs(0U);
    auto footer_run = footer.add_run("old footer");
    REQUIRE(footer.valid());
    REQUIRE(footer_run.valid());

    REQUIRE(document.replace_section_header_text(0U, "new header"));
    CHECK_FALSE(header.valid());
    CHECK_FALSE(header_run.valid());
    CHECK_FALSE(header_run.set_text("stale header"));
    CHECK(body.valid());
    CHECK(body_run.valid());
    CHECK_EQ(body_run.get_text(), "body remains live");
    CHECK(footer.valid());
    CHECK(footer_run.valid());
    CHECK_EQ(footer_run.get_text(), "old footer");

    auto new_header = document.section_header_paragraphs(0U);
    auto new_header_run = new_header.runs();
    REQUIRE(new_header.valid());
    REQUIRE(new_header_run.valid());
    CHECK_EQ(new_header_run.get_text(), "new header");

    REQUIRE(document.replace_section_footer_text(0U, "new footer"));
    CHECK_FALSE(footer.valid());
    CHECK_FALSE(footer_run.valid());
    CHECK(body.valid());
    CHECK(body_run.valid());
    CHECK(new_header.valid());
    CHECK(new_header_run.valid());
    CHECK_EQ(new_header_run.get_text(), "new header");
    CHECK_EQ(document.section_footer_paragraphs(0U).runs().get_text(),
             "new footer");
}

TEST_CASE("ensure section header and footer paragraphs create references per "
          "section") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "ensure_section_header_footer_multi.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr/>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 2);

    auto section0_footer = doc.ensure_section_footer_paragraphs(0);
    REQUIRE(section0_footer.has_next());
    CHECK(section0_footer.add_run("section 1 footer").has_next());

    auto section0_even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(section0_even_header.has_next());
    CHECK(section0_even_header.add_run("section 1 even header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 2 first footer").has_next());

    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(
        section1_default_header.add_run("section 2 default header").has_next());

    auto same_section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(same_section1_default_header.has_next());
    CHECK_EQ(same_section1_default_header.runs().get_text(),
             "section 2 default header");
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);

    CHECK_FALSE(doc.save());

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(),
                                       saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    const auto section0_properties =
        body.child("w:p").child("w:pPr").child("w:sectPr");
    const auto section1_properties = body.child("w:sectPr");
    REQUIRE(section0_properties != pugi::xml_node{});
    REQUIRE(section1_properties != pugi::xml_node{});
    CHECK_NE(saved_document_xml.find("w:type=\"even\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"first\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"default\""), std::string::npos);
    CHECK(section0_properties.child("w:titlePg") == pugi::xml_node{});
    CHECK(section1_properties.child("w:titlePg") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header2.xml\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer2.xml\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""),
             std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/settings.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 2);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 2);
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(),
             "section 1 footer");
    CHECK_EQ(reopened
                 .section_header_paragraphs(
                     0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 1 even header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(),
             "section 2 default header");
    CHECK_EQ(reopened
                 .section_footer_paragraphs(
                     1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 2 first footer");

    fs::remove(target);
}

TEST_CASE(
    "ensure even-page section headers preserve existing settings.xml content") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "ensure_section_even_settings_reuse.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/settings.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body text</w:t></w:r></w:p>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"
                Target="settings.xml"/>
</Relationships>
)";

    const std::string settings_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:zoom w:percent="125"/>
</w:settings>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/settings.xml", settings_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    CHECK_FALSE(doc.save());

    const auto saved_settings_xml =
        read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(),
                                       saved_settings_xml.size()));
    const auto settings_root = saved_settings.child("w:settings");
    REQUIRE(settings_root != pugi::xml_node{});
    CHECK(settings_root.child("w:zoom") != pugi::xml_node{});
    CHECK(settings_root.child("w:evenAndOddHeaders") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened
                 .section_header_paragraphs(
                     0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");

    fs::remove(target);
}

TEST_CASE("document can toggle update fields on open setting") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "update_fields_on_open_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto initially_enabled = doc.update_fields_on_open_enabled();
    REQUIRE(initially_enabled.has_value());
    CHECK_FALSE(*initially_enabled);

    CHECK(doc.enable_update_fields_on_open());
    const auto enabled = doc.update_fields_on_open_enabled();
    REQUIRE(enabled.has_value());
    CHECK(*enabled);
    CHECK_FALSE(doc.save());

    auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    CHECK_NE(saved_settings_xml.find("<w:updateFields"), std::string::npos);
    CHECK_NE(saved_settings_xml.find("w:val=\"1\""), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""),
             std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("PartName=\"/word/settings.xml\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_enabled = reopened.update_fields_on_open_enabled();
    REQUIRE(reopened_enabled.has_value());
    CHECK(*reopened_enabled);

    CHECK(reopened.clear_update_fields_on_open());
    const auto cleared = reopened.update_fields_on_open_enabled();
    REQUIRE(cleared.has_value());
    CHECK_FALSE(*cleared);
    CHECK_FALSE(reopened.save());

    saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    CHECK_EQ(saved_settings_xml.find("<w:updateFields"), std::string::npos);

    featherdoc::Document cleared_doc(target);
    CHECK_FALSE(cleared_doc.open());
    const auto cleared_reopened = cleared_doc.update_fields_on_open_enabled();
    REQUIRE(cleared_reopened.has_value());
    CHECK_FALSE(*cleared_reopened);

    fs::remove(target);
}

TEST_CASE("settings mutations save back to a non-default relationship target") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "settings_custom_target_roundtrip.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/custom/settings.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
</Types>
)";
    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>settings target</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rSettings"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"
                Target="custom/settings.xml"/>
</Relationships>
)";
    const std::string settings_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:zoom w:percent="110"/>
</w:settings>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/custom/settings.xml", settings_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    const auto initially_enabled = doc.update_fields_on_open_enabled();
    REQUIRE(initially_enabled.has_value());
    CHECK_FALSE(*initially_enabled);

    CHECK(doc.enable_update_fields_on_open());
    CHECK_FALSE(doc.save());

    CHECK_FALSE(test_docx_entry_exists(target, "word/settings.xml"));
    const auto saved_settings_xml =
        read_test_docx_entry(target, "word/custom/settings.xml");
    CHECK_NE(saved_settings_xml.find("<w:updateFields"), std::string::npos);
    CHECK_NE(saved_settings_xml.find("w:val=\"1\""), std::string::npos);
    CHECK_NE(saved_settings_xml.find("<w:zoom w:percent=\"110\""),
             std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"custom/settings.xml\""),
             std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"settings.xml\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_enabled = reopened.update_fields_on_open_enabled();
    REQUIRE(reopened_enabled.has_value());
    CHECK(*reopened_enabled);

    rewrite_test_docx_entry(
        target, "word/custom/settings.xml",
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:unexpected xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"/>
)");
    featherdoc::Document invalid_settings(target);
    CHECK_FALSE(invalid_settings.open());
    CHECK_FALSE(invalid_settings.update_fields_on_open_enabled().has_value());
    CHECK_EQ(invalid_settings.last_error().code,
             featherdoc::make_error_code(
                 featherdoc::document_errc::invalid_package_structure));
    CHECK_EQ(invalid_settings.last_error().entry_name,
             "word/custom/settings.xml");
    CHECK_NE(invalid_settings.last_error().detail.find("}unexpected'"),
             std::string::npos);
    CHECK_NE(invalid_settings.last_error().detail.find("}settings'"),
             std::string::npos);

    fs::remove(target);
}
