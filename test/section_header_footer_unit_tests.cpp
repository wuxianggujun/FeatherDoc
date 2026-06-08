#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("ensure_header_paragraphs and ensure_footer_paragraphs work for create_empty") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_header_footer_empty.docx";
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

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("xmlns:r="), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:headerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:footerReference"), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(), "generated header");
    CHECK_EQ(reopened.footer_paragraphs().runs().get_text(), "generated footer");

    fs::remove(target);
}

TEST_CASE("section header and footer access resolves references per section") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "section_header_footer_access.docx";
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
    CHECK_EQ(section1_default_header.runs().get_text(), "section 2 default header");

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
    CHECK(section1_first_footer.runs().set_text("updated section 2 first footer"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 2);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(),
             "updated section 2 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "updated section 2 even");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "updated section 2 first footer");

    fs::remove(target);
}

TEST_CASE("ensure section header and footer paragraphs create references for a single section") {
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

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
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
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("relationships/settings"), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/settings.xml"), std::string::npos);

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 1);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "default header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 0, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");

    fs::remove(target);
}

TEST_CASE("replace section header and footer text rewrites parts cleanly") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "replace_section_header_footer_text.docx";
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

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    CHECK_NE(saved_settings_xml.find("w:evenAndOddHeaders"), std::string::npos);

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.replace_section_header_text(0, "missing"));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("ensure section header and footer paragraphs create references per section") {
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
    CHECK(section1_default_header.add_run("section 2 default header").has_next());

    auto same_section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(same_section1_default_header.has_next());
    CHECK_EQ(same_section1_default_header.runs().get_text(), "section 2 default header");
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
    CHECK_NE(saved_document_xml.find("w:type=\"even\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"first\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"default\""), std::string::npos);
    CHECK(section0_properties.child("w:titlePg") == pugi::xml_node{});
    CHECK(section1_properties.child("w:titlePg") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);

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
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 1 even header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(),
             "section 2 default header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 2 first footer");

    fs::remove(target);
}

TEST_CASE("ensure even-page section headers preserve existing settings.xml content") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_section_even_settings_reuse.docx";
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

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    const auto settings_root = saved_settings.child("w:settings");
    REQUIRE(settings_root != pugi::xml_node{});
    CHECK(settings_root.child("w:zoom") != pugi::xml_node{});
    CHECK(settings_root.child("w:evenAndOddHeaders") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");

    fs::remove(target);
}

TEST_CASE("document can toggle update fields on open setting") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "update_fields_on_open_roundtrip.docx";
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

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
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

TEST_CASE("removing first and even references cleans title page and settings flags") {
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
    CHECK(section_properties.child("w:titlePg") == pugi::xml_node{});
    CHECK(section_properties.find_child_by_attribute("w:headerReference", "w:type", "even") ==
          pugi::xml_node{});
    CHECK(section_properties.find_child_by_attribute("w:footerReference", "w:type", "first") ==
          pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") ==
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
