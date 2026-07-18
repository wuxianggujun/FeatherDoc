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

TEST_CASE("assign section header and footer paragraphs reuse existing parts") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "assign_section_header_footer_existing_parts.docx";
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
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);

    auto shared_default_header = doc.assign_section_header_paragraphs(1, 0);
    REQUIRE(shared_default_header.has_next());
    CHECK(shared_default_header.runs().set_text("shared header"));

    auto shared_even_header = doc.assign_section_header_paragraphs(
        1, 0, featherdoc::section_reference_kind::even_page);
    REQUIRE(shared_even_header.has_next());
    CHECK_EQ(shared_even_header.runs().get_text(), "shared header");

    auto shared_default_footer = doc.assign_section_footer_paragraphs(1, 0);
    REQUIRE(shared_default_footer.has_next());
    CHECK(shared_default_footer.runs().set_text("shared footer"));

    auto shared_first_footer = doc.assign_section_footer_paragraphs(
        1, 0, featherdoc::section_reference_kind::first_page);
    REQUIRE(shared_first_footer.has_next());
    CHECK_EQ(shared_first_footer.runs().get_text(), "shared footer");

    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    const auto section0_properties = body.child("w:p").child("w:pPr").child("w:sectPr");
    const auto section1_properties = body.child("w:sectPr");
    REQUIRE(section0_properties != pugi::xml_node{});
    REQUIRE(section1_properties != pugi::xml_node{});

    const auto find_reference_id =
        [](pugi::xml_node section_properties, const char *reference_name,
           const char *reference_type) -> std::string {
        for (auto reference = section_properties.child(reference_name);
             reference != pugi::xml_node{};
             reference = reference.next_sibling(reference_name)) {
            if (std::string_view{reference.attribute("w:type").value()} == reference_type) {
                return reference.attribute("r:id").value();
            }
        }
        return {};
    };

    CHECK_EQ(find_reference_id(section0_properties, "w:headerReference", "default"), "rId2");
    CHECK_EQ(find_reference_id(section1_properties, "w:headerReference", "default"), "rId2");
    CHECK_EQ(find_reference_id(section1_properties, "w:headerReference", "even"), "rId2");
    CHECK_EQ(find_reference_id(section0_properties, "w:footerReference", "default"), "rId3");
    CHECK_EQ(find_reference_id(section1_properties, "w:footerReference", "default"), "rId3");
    CHECK_EQ(find_reference_id(section1_properties, "w:footerReference", "first"), "rId3");
    CHECK(section1_properties.child("w:titlePg") != pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/footer2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/settings.xml"), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "shared header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "shared header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "shared header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "shared footer");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "shared footer");

    fs::remove(target);
}

TEST_CASE("remove section header and footer references prunes orphaned parts on save") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "remove_section_header_footer_references.docx";
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
    CHECK(doc.remove_section_header_reference(1));
    CHECK(doc.remove_section_footer_reference(1));
    CHECK_FALSE(doc.remove_section_header_reference(
        1, featherdoc::section_reference_kind::even_page));
    CHECK_FALSE(doc.last_error());
    CHECK_FALSE(doc.section_header_paragraphs(1).has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(1).has_next());
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    const auto section1_properties = body.child("w:sectPr");
    REQUIRE(section1_properties != pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type",
                                                         "default"),
             pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "default"),
             pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/footer2.xml"), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_FALSE(reopened.section_header_paragraphs(1).has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(1).has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.remove_section_header_reference(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("removing first and even references preserves user page-mode metadata") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "remove_section_first_even_reference_cleanup.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    auto first_footer = doc.ensure_section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.add_run("first footer").has_next());

    CHECK(doc.remove_section_header_reference(
        0, featherdoc::section_reference_kind::even_page));
    CHECK(doc.remove_section_footer_reference(
        0, featherdoc::section_reference_kind::first_page));
    CHECK_FALSE(doc.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto section_properties =
        saved_document.child("w:document").child("w:body").child("w:sectPr");
    REQUIRE(section_properties != pugi::xml_node{});
    CHECK(section_properties.child("w:titlePg") != pugi::xml_node{});
    CHECK(section_properties.find_child_by_attribute("w:headerReference", "w:type", "even") ==
          pugi::xml_node{});
    CHECK(section_properties.find_child_by_attribute("w:footerReference", "w:type", "first") ==
          pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    CHECK_FALSE(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer1.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 0);
    CHECK_EQ(reopened.footer_count(), 0);
    CHECK_FALSE(reopened.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());

    fs::remove(target);
}

TEST_CASE("remove header and footer parts updates counts and prunes archive output") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "remove_header_footer_parts.docx";
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
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);
    CHECK(doc.remove_header_part(1));
    CHECK(doc.remove_footer_part(1));
    CHECK_EQ(doc.header_count(), 1);
    CHECK_EQ(doc.footer_count(), 1);
    CHECK_FALSE(doc.section_header_paragraphs(1).has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(1).has_next());
    CHECK_FALSE(doc.save());

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/footer2.xml"), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_FALSE(reopened.section_header_paragraphs(1).has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(1).has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.remove_header_part(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("removing a related part invalidates published XML handles and rebinds the document") {
    featherdoc::Document doc;
    REQUIRE_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    auto body_paragraph = doc.paragraphs();
    auto body_run = body_paragraph.add_run("body text");

    auto header_paragraph = doc.ensure_section_header_paragraphs(0U);
    auto header_run = header_paragraph.add_run("header text");
    auto header_template = doc.header_template(0U);
    auto header_table = header_template.append_table(1U, 1U);
    auto header_row = header_table.rows();
    auto header_cell = header_row.cells();
    auto header_cell_paragraph = header_cell.paragraphs();

    auto footer_paragraph = doc.ensure_section_footer_paragraphs(0U);
    auto footer_run = footer_paragraph.add_run("footer text");
    auto footer_template = doc.footer_template(0U);

    REQUIRE(body_template);
    REQUIRE(body_paragraph.valid());
    REQUIRE(body_run.valid());
    REQUIRE(header_template);
    REQUIRE(header_paragraph.valid());
    REQUIRE(header_run.valid());
    REQUIRE(header_table.valid());
    REQUIRE(header_row.valid());
    REQUIRE(header_cell.valid());
    REQUIRE(header_cell_paragraph.valid());
    REQUIRE(footer_template);
    REQUIRE(footer_paragraph.valid());
    REQUIRE(footer_run.valid());

    CHECK_FALSE(doc.remove_header_part(1U));
    CHECK(body_template);
    CHECK(body_paragraph.valid());
    CHECK(header_template);
    CHECK(header_paragraph.valid());
    CHECK(footer_template);
    CHECK(footer_paragraph.valid());

    REQUIRE(doc.remove_header_part(0U));
    CHECK_EQ(doc.header_count(), 0U);
    CHECK_EQ(doc.footer_count(), 1U);

    REQUIRE_FALSE(body_template);
    CHECK_FALSE(body_paragraph.valid());
    CHECK_FALSE(body_run.valid());
    REQUIRE_FALSE(header_template);
    CHECK_FALSE(header_paragraph.valid());
    CHECK_FALSE(header_run.valid());
    CHECK_FALSE(header_table.valid());
    CHECK_FALSE(header_row.valid());
    CHECK_FALSE(header_cell.valid());
    CHECK_FALSE(header_cell_paragraph.valid());
    REQUIRE_FALSE(footer_template);
    CHECK_FALSE(footer_paragraph.valid());
    CHECK_FALSE(footer_run.valid());

    CHECK_FALSE(body_paragraph.set_text("stale body"));
    CHECK_FALSE(header_run.set_text("stale header"));
    CHECK_FALSE(header_table.set_cell_text(0U, 0U, "stale table"));
    CHECK_FALSE(header_cell.set_text("stale cell"));
    CHECK_FALSE(footer_paragraph.set_text("stale footer"));
    CHECK_FALSE(header_template.append_paragraph("stale template").valid());

    auto rebound_body = doc.paragraphs();
    auto rebound_footer = doc.section_footer_paragraphs(0U);
    auto rebound_footer_template = doc.footer_template(0U);
    REQUIRE(rebound_body.valid());
    REQUIRE(rebound_footer.valid());
    REQUIRE(rebound_footer_template);
    CHECK_EQ(rebound_body.runs().get_text(), "body text");
    CHECK_EQ(rebound_footer.runs().get_text(), "footer text");
    CHECK(rebound_footer_template.append_paragraph("footer after rebind").valid());

    REQUIRE(doc.remove_footer_part(0U));
    CHECK_EQ(doc.footer_count(), 0U);
    CHECK_FALSE(rebound_body.valid());
    CHECK_FALSE(rebound_footer.valid());
    CHECK_FALSE(rebound_footer_template);

    auto final_body = doc.paragraphs();
    REQUIRE(final_body.valid());
    CHECK_EQ(final_body.runs().get_text(), "body text");
}

TEST_CASE("removing a section reference preserves public story handles") {
    featherdoc::Document doc;
    REQUIRE_FALSE(doc.create_empty());

    auto even_header = doc.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.valid());
    REQUIRE(even_header.add_run("even header").valid());

    auto footer = doc.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.valid());
    REQUIRE(footer.add_run("default footer").valid());

    auto body = doc.paragraphs();
    REQUIRE(body.valid());
    REQUIRE(body.add_run("body text").valid());
    auto body_run = body.runs();
    auto header_run = even_header.runs();
    auto footer_run = footer.runs();

    REQUIRE_FALSE(doc.remove_section_header_reference(0U));
    CHECK_FALSE(doc.last_error());
    CHECK(body.valid());
    CHECK(body_run.valid());
    CHECK(even_header.valid());
    CHECK(header_run.valid());
    CHECK(footer.valid());
    CHECK(footer_run.valid());

    REQUIRE(doc.remove_section_header_reference(
        0U, featherdoc::section_reference_kind::even_page));
    CHECK(body.valid());
    CHECK(body_run.valid());
    CHECK_EQ(body_run.get_text(), "body text");
    CHECK(even_header.valid());
    CHECK(header_run.valid());
    CHECK(footer.valid());
    CHECK(footer_run.valid());
    CHECK_EQ(header_run.get_text(), "even header");
    CHECK_EQ(footer_run.get_text(), "default footer");

    auto rebound_body = doc.paragraphs();
    REQUIRE(rebound_body.valid());
    CHECK_EQ(rebound_body.runs().get_text(), "body text");
    CHECK_FALSE(doc.section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page)
                    .valid());
}

TEST_CASE("tolerant section reference removal survives invalid document relationships root") {
    namespace fs = std::filesystem;

    const auto source =
        fs::current_path() / "tolerant_remove_section_header_bad_doc_rels.docx";
    const auto output = fs::current_path() /
                        "tolerant_remove_section_header_bad_doc_rels_saved.docx";
    fs::remove(source);
    fs::remove(output);

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
    <w:p><w:r><w:t>body survives</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rHeader"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    const std::string invalid_document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<BrokenRelationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"
                     marker="preserve-me">
  <Relationship Id="rHeader"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
</BrokenRelationships>
)";
    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>header ignored by invalid relationships root</w:t></w:r></w:p>
</w:hdr>
)";

    write_test_archive_entries(
        source,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", invalid_document_relationships_xml},
            {"word/header1.xml", header_xml},
        });

    featherdoc::Document strict_document(source);
    CHECK_EQ(strict_document.open(),
             featherdoc::document_errc::invalid_package_structure);
    CHECK_FALSE(strict_document.is_open());
    CHECK_EQ(strict_document.last_error().entry_name,
             "word/_rels/document.xml.rels");

    featherdoc::document_open_options options;
    options.validation = featherdoc::package_validation_mode::tolerant;
    featherdoc::Document tolerant_document(source);
    REQUIRE_FALSE(tolerant_document.open(options));
    CHECK(tolerant_document.is_open());

    bool reported_invalid_relationships_root = false;
    for (const auto &diagnostic : tolerant_document.package_diagnostics()) {
        if (diagnostic.code ==
            featherdoc::package_diagnostic_code::invalid_relationships_part) {
            reported_invalid_relationships_root = true;
            break;
        }
    }
    CHECK(reported_invalid_relationships_root);

    auto body = tolerant_document.paragraphs();
    REQUIRE(body.valid());
    auto body_run = body.runs();
    REQUIRE(body_run.valid());
    CHECK_EQ(body_run.get_text(), "body survives");

    REQUIRE(tolerant_document.remove_section_header_reference(0U));
    CHECK(body.valid());
    CHECK(body_run.valid());
    CHECK_EQ(body_run.get_text(), "body survives");

    CHECK_FALSE(tolerant_document.remove_section_header_reference(0U));
    CHECK_FALSE(tolerant_document.last_error());
    CHECK(body.valid());
    CHECK(body_run.valid());

    REQUIRE_FALSE(tolerant_document.save_as(output));

    const auto saved_document_xml =
        read_test_docx_entry(output, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("<w:headerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("body survives"), std::string::npos);
    CHECK_EQ(read_test_docx_entry(output, "word/_rels/document.xml.rels"),
             invalid_document_relationships_xml);

    fs::remove(source);
    fs::remove(output);
}
