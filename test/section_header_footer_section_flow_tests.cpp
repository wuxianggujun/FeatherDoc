#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("move header and footer parts reorders logical indices and persists after reopen") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "move_header_footer_parts.docx";
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
      <w:footerReference w:type="default" r:id="rId5"/>
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
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
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
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
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
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.header_paragraphs(1).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_EQ(doc.footer_paragraphs(1).runs().get_text(), "section 2 footer");

    CHECK(doc.move_header_part(1, 0));
    CHECK(doc.move_footer_part(1, 0));
    CHECK_EQ(doc.header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.footer_paragraphs(0).runs().get_text(), "section 2 footer");
    CHECK_EQ(doc.footer_paragraphs(1).runs().get_text(), "section 1 footer");

    CHECK_EQ(doc.section_header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_EQ(doc.section_footer_paragraphs(1).runs().get_text(), "section 2 footer");
    CHECK_FALSE(doc.save());

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK(saved_relationships.find("Target=\"header2.xml\"") <
          saved_relationships.find("Target=\"header1.xml\""));
    CHECK(saved_relationships.find("Target=\"footer2.xml\"") <
          saved_relationships.find("Target=\"footer1.xml\""));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(reopened.header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.footer_paragraphs(0).runs().get_text(), "section 2 footer");
    CHECK_EQ(reopened.footer_paragraphs(1).runs().get_text(), "section 1 footer");
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 2 header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_EQ(reopened.section_footer_paragraphs(1).runs().get_text(), "section 2 footer");

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.move_header_part(0, 0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    featherdoc::Document invalid_range(target);
    CHECK_FALSE(invalid_range.open());
    CHECK_FALSE(invalid_range.move_header_part(2, 0));
    CHECK_EQ(invalid_range.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_range.move_footer_part(0, 2));
    CHECK_EQ(invalid_range.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("copying section header and footer references replaces target layout") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "copy_section_header_footer_refs.docx";
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
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/settings.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
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
      <w:headerReference w:type="even" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
      <w:footerReference w:type="first" r:id="rId5"/>
      <w:titlePg w:val="1"/>
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
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
  <Relationship Id="rId6"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"
                Target="settings.xml"/>
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
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
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
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string settings_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:evenAndOddHeaders w:val="1"/>
</w:settings>
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
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
            {"word/settings.xml", settings_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.copy_section_header_references(0, 1));
    CHECK(doc.copy_section_footer_references(0, 1));
    CHECK_FALSE(doc.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(1).runs().get_text(), "section 1 footer");
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto section1_properties =
        saved_document.child("w:document").child("w:body").child("w:sectPr");
    REQUIRE(section1_properties != pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type",
                                                         "default")
                 .attribute("r:id")
                 .value(),
             std::string{"rId2"});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "default")
                 .attribute("r:id")
                 .value(),
             std::string{"rId3"});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "first"),
             pugi::xml_node{});
    CHECK(section1_properties.child("w:titlePg") == pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") ==
          pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(1).runs().get_text(), "section 1 footer");
    CHECK_FALSE(reopened.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.copy_section_header_references(0, 1));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("append section can inherit or reset header and footer references") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "append_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

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

    CHECK(doc.append_section());
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "default header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");

    CHECK(doc.append_section(false));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 3);

    CHECK(section_nodes[1].child("w:titlePg") != pugi::xml_node{});
    CHECK(section_nodes[2].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type",
                                                      "default"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:footerReference", "w:type",
                                                      "first"),
             pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 3);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "default header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");
    CHECK_FALSE(reopened.section_header_paragraphs(2).has_next());
    CHECK_FALSE(reopened.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.append_section());
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("insert section can split layout inheritance or reset references") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto section0_default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_default_header.has_next());
    CHECK(section0_default_header.add_run("section 0 header").has_next());

    auto section0_even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(section0_even_header.has_next());
    CHECK(section0_even_header.add_run("section 0 even header").has_next());

    CHECK(doc.append_section(false));
    CHECK_EQ(doc.section_count(), 2);

    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.add_run("section 1 header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.insert_section(0));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 0 even header");
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(2).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    CHECK(doc.insert_section(1, false));
    CHECK_EQ(doc.section_count(), 4);
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(3).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 3, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    CHECK(doc.insert_section(3));
    CHECK_EQ(doc.section_count(), 5);
    CHECK_EQ(doc.section_header_paragraphs(4).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 4, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 5);

    CHECK(section_nodes[2].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type",
                                                      "default"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:footerReference", "w:type",
                                                      "first"),
             pugi::xml_node{});
    CHECK(section_nodes[3].child("w:titlePg") != pugi::xml_node{});
    CHECK(section_nodes[4].child("w:titlePg") != pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 5);
    CHECK_EQ(reopened.header_count(), 3);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 0 even header");
    CHECK_FALSE(reopened.section_header_paragraphs(2).has_next());
    CHECK_FALSE(reopened.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(reopened.section_header_paragraphs(3).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 3, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_EQ(reopened.section_header_paragraphs(4).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 4, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.insert_section(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("remove section merges boundaries and prunes orphaned header footer parts on save") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "remove_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto section0_default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_default_header.has_next());
    CHECK(section0_default_header.add_run("section 0 header").has_next());

    CHECK(doc.append_section(false));
    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.add_run("section 1 header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.append_section(false));
    auto section2_default_header = doc.ensure_section_header_paragraphs(2);
    REQUIRE(section2_default_header.has_next());
    CHECK(section2_default_header.add_run("section 2 header").has_next());

    auto section2_even_header = doc.ensure_section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page);
    REQUIRE(section2_even_header.has_next());
    CHECK(section2_even_header.add_run("section 2 even header").has_next());

    CHECK(doc.remove_section(1));
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());

    CHECK(doc.remove_section(1));
    CHECK_EQ(doc.section_count(), 1);
    CHECK_EQ(doc.section_header_paragraphs(0).runs().get_text(), "section 0 header");
    CHECK_FALSE(doc.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_FALSE(doc.remove_section(0));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 1);
    CHECK(section_nodes[0].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[0].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[0].find_child_by_attribute("w:footerReference", "w:type", "first"),
             pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") ==
          pugi::xml_node{});

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header3.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer1.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 1);
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 0);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 0 header");
    CHECK_FALSE(reopened.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.remove_section(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("move section reorders body content and keeps header footer layouts attached") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "move_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.paragraphs().add_run("section 0 body").has_next());
    auto section0_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_header.has_next());
    CHECK(section0_header.add_run("section 0 header").has_next());

    CHECK(doc.append_section(false));

    auto append_body_paragraph = [](featherdoc::Document &document, const char *text) {
        auto paragraph = document.paragraphs();
        while (paragraph.has_next()) {
            paragraph.next();
        }

        const auto inserted = paragraph.insert_paragraph_after(text);
        REQUIRE(inserted.has_next());
    };

    append_body_paragraph(doc, "section 1 body");
    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.append_section(false));
    append_body_paragraph(doc, "section 2 body");

    auto section2_header = doc.ensure_section_header_paragraphs(2);
    REQUIRE(section2_header.has_next());
    CHECK(section2_header.add_run("section 2 header").has_next());

    auto section2_even_header = doc.ensure_section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page);
    REQUIRE(section2_even_header.has_next());
    CHECK(section2_even_header.add_run("section 2 even header").has_next());

    CHECK(doc.move_section(2, 0));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_EQ(doc.section_header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_FALSE(doc.move_section(3, 0));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto collect_non_empty_document_text = [](featherdoc::Document &document) {
        std::ostringstream stream;
        for (auto paragraph = document.paragraphs(); paragraph.has_next(); paragraph.next()) {
            std::string text;
            for (auto run = paragraph.runs(); run.has_next(); run.next()) {
                text += run.get_text();
            }

            if (!text.empty()) {
                stream << text << '\n';
            }
        }
        return stream.str();
    };

    CHECK_EQ(collect_non_empty_document_text(reopened),
             "section 2 body\nsection 0 body\nsection 1 body\n");
    CHECK_EQ(reopened.section_count(), 3);
    CHECK_EQ(reopened.header_count(), 3);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.move_section(0, 0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}
