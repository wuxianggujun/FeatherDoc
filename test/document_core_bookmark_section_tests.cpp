#include "document_core_unit_test_support.hpp"

TEST_CASE("replace_bookmark_text rewrites bookmarked content and preserves markers") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>prefix</w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="bookmark"/>
      <w:r><w:t>old</w:t></w:r>
      <w:proofErr w:type="spellStart"/>
      <w:r><w:t>content</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
      <w:r><w:t>suffix</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_text("bookmark", " updated value "), 1);
    CHECK_EQ(collect_document_text(doc), "prefix updated value suffix\n");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(xml_text.find("w:bookmarkStart"), std::string::npos);
    CHECK_NE(xml_text.find("w:bookmarkEnd"), std::string::npos);
    CHECK_NE(xml_text.find("updated value"), std::string::npos);
    CHECK_NE(xml_text.find("xml:space=\"preserve\""), std::string::npos);
    CHECK_EQ(xml_text.find("old"), std::string::npos);
    CHECK_EQ(xml_text.find("content"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "prefix updated value suffix\n");

    fs::remove(target);
}

TEST_CASE("existing header and footer paragraphs can be edited and saved") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_footer_access.docx";
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
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
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
      <w:footerReference w:type="default" r:id="rId3"/>
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
</Relationships>
)";

    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>old header</w:t></w:r></w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>old footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
            {"word/footer1.xml", footer_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.header_count(), 1);
    CHECK_EQ(doc.footer_count(), 1);

    auto header = doc.header_paragraphs();
    REQUIRE(header.has_next());
    auto header_run = header.runs();
    REQUIRE(header_run.has_next());
    CHECK(header_run.set_text("updated header"));

    auto footer = doc.footer_paragraphs();
    REQUIRE(footer.has_next());
    auto footer_run = footer.runs();
    REQUIRE(footer_run.has_next());
    CHECK(footer_run.set_text(" updated footer "));

    CHECK_FALSE(doc.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header.find("updated header"), std::string::npos);

    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_footer.find("updated footer"), std::string::npos);
    CHECK_NE(saved_footer.find("xml:space=\"preserve\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);

    auto reopened_header = reopened.header_paragraphs();
    REQUIRE(reopened_header.has_next());
    CHECK_EQ(reopened_header.runs().get_text(), "updated header");

    auto reopened_footer = reopened.footer_paragraphs();
    REQUIRE(reopened_footer.has_next());
    CHECK_EQ(reopened_footer.runs().get_text(), " updated footer ");

    CHECK_FALSE(reopened.header_paragraphs(1).has_next());
    CHECK_FALSE(reopened.footer_paragraphs(1).has_next());

    fs::remove(target);
}

TEST_CASE("ensure_header_paragraphs creates a default header for existing documents") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_header_existing.docx";
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

    auto header = doc.ensure_header_paragraphs();
    REQUIRE(header.has_next());
    CHECK(header.add_run("generated header").has_next());
    CHECK_EQ(doc.header_count(), 1);

    auto same_header = doc.ensure_header_paragraphs();
    REQUIRE(same_header.has_next());
    CHECK_EQ(doc.header_count(), 1);

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("xmlns:r="), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:headerReference"), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("relationships/header"), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body text\n");
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(), "generated header");

    fs::remove(target);
}
