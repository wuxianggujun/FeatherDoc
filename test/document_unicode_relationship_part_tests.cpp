#include "document_core_unit_test_support.hpp"
#include "../src/package_path_helpers.hpp"

#include <string>

TEST_CASE(
    "Unicode OPC relationship targets are resolved without host code pages") {
    namespace fs = std::filesystem;

    const auto source = fs::current_path() / featherdoc::detail::path_from_utf8(
                                                 "包内中文关系-🙂-源.docx");
    const auto saved = fs::current_path() / featherdoc::detail::path_from_utf8(
                                                "包内中文关系-🙂-保存.docx");
    fs::remove(source);
    fs::remove(saved);

    const auto header_entry_result =
        featherdoc::detail::canonicalize_package_iri_path(
            "word/页眉/头部-日本語-🙂.xml", true);
    const auto header_relationships_entry_result =
        featherdoc::detail::canonicalize_package_iri_path(
            "word/页眉/_rels/头部-日本語-🙂.xml.rels", true);
    const auto image_entry_result =
        featherdoc::detail::canonicalize_package_iri_path(
            "word/media/徽标 中文-日本語-🙂.png", true);
    REQUIRE(header_entry_result);
    REQUIRE(header_relationships_entry_result);
    REQUIRE(image_entry_result);
    const auto header_entry = header_entry_result.entry_name;
    const auto header_relationships_entry =
        header_relationships_entry_result.entry_name;
    const auto image_entry = image_entry_result.entry_name;
    const auto image_payload =
        std::string{"unicode-image-payload\0binary", 28U};

    const auto content_types_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="png" ContentType="image/png"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/)"} + header_entry + R"("
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
</Types>
)";
    const auto document_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>正文中文</w:t></w:r></w:p>
    <w:sectPr><w:headerReference w:type="default" r:id="rIdHeader"/></w:sectPr>
  </w:body>
</w:document>
)"};
    const auto document_relationships_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rIdHeader"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="页眉/头部-日本語-🙂.xml"/>
</Relationships>
)"};
    const auto header_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
       xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
       xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"
       xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
       xmlns:pic="http://schemas.openxmlformats.org/drawingml/2006/picture">
  <w:p>
    <w:r>
      <w:t>中文页眉-日本語-🙂</w:t>
      <w:drawing>
        <wp:inline>
          <wp:extent cx="9525" cy="19050"/>
          <wp:docPr id="1" name="徽标 中文-🙂"/>
          <a:graphic><a:graphicData><pic:pic><pic:blipFill>
            <a:blip r:embed="rIdImage"/>
          </pic:blipFill></pic:pic></a:graphicData></a:graphic>
        </wp:inline>
      </w:drawing>
    </w:r>
  </w:p>
</w:hdr>
)"};
    const auto header_relationships_xml = std::string{
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rIdImage"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image"
                Target="../media/徽标%20中文-日本語-🙂.png"/>
</Relationships>
)"};

    write_test_archive_entries(
        source, {{test_content_types_xml_entry, content_types_xml},
                 {test_relationships_xml_entry, test_relationships_xml},
                 {test_document_xml_entry, document_xml},
                 {"word/_rels/document.xml.rels", document_relationships_xml},
                 {header_entry, header_xml},
                 {header_relationships_entry, header_relationships_xml},
                 {image_entry, image_payload}});

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE_EQ(document.header_count(), 1U);

    auto header = document.header_template();
    REQUIRE(static_cast<bool>(header));
    CHECK_EQ(header.entry_name(), header_entry);
    const auto paragraphs = header.inspect_paragraphs();
    REQUIRE_FALSE(document.last_error());
    REQUIRE_EQ(paragraphs.size(), 1U);
    CHECK_EQ(paragraphs[0].text, "中文页眉-日本語-🙂");

    const auto images = header.drawing_images();
    REQUIRE_FALSE(document.last_error());
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].entry_name, image_entry);
    CHECK_EQ(images[0].content_type, "image/png");

    auto appended = header.append_paragraph("保存后仍是 UTF-8：中文-日本語-🙂");
    REQUIRE(appended.has_next());
    REQUIRE_FALSE(document.save_as(saved));
    const auto saved_header_xml =
        read_test_docx_entry(saved, header_entry.c_str());
    CHECK_NE(saved_header_xml.find("保存后仍是 UTF-8"), std::string::npos);
    CHECK_EQ(read_test_docx_entry(saved, image_entry.c_str()), image_payload);

    featherdoc::Document reopened(saved);
    REQUIRE_FALSE(reopened.open());
    REQUIRE_EQ(reopened.header_count(), 1U);
    const auto reopened_header = reopened.header_template();
    REQUIRE(static_cast<bool>(reopened_header));
    CHECK_EQ(reopened_header.entry_name(), header_entry);
    const auto reopened_images = reopened_header.drawing_images();
    REQUIRE_FALSE(reopened.last_error());
    REQUIRE_EQ(reopened_images.size(), 1U);
    CHECK_EQ(reopened_images[0].entry_name, image_entry);

    fs::remove(source);
    fs::remove(saved);
}
