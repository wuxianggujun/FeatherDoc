#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"
#include "basic_image_fixture_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("header and footer template parts can replace bookmark placeholders with images") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "header_footer_template_images.docx";
    const fs::path image_path = fs::current_path() / "header_footer_template_images.png";
    fs::remove(target);
    fs::remove(image_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="1" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
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

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo", image_path, 20U, 10U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", image_path, 30U, 15U),
             1);

    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image2.png"), image_data);

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header.find("<w:drawing"), std::string::npos);
    CHECK_NE(saved_header.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_header.find("cy=\"95250\""), std::string::npos);
    CHECK_EQ(saved_header.find("w:name=\"header_logo\""), std::string::npos);

    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_footer.find("<w:drawing"), std::string::npos);
    CHECK_NE(saved_footer.find("cx=\"285750\""), std::string::npos);
    CHECK_NE(saved_footer.find("cy=\"142875\""), std::string::npos);
    CHECK_EQ(saved_footer.find("w:name=\"footer_logo\""), std::string::npos);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);

    const auto saved_footer_relationships =
        read_test_docx_entry(target, "word/_rels/footer1.xml.rels");
    CHECK_NE(saved_footer_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/_rels/header1.xml.rels"));
    CHECK(test_docx_entry_exists(target, "word/_rels/footer1.xml.rels"));
    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("template parts list existing inline images and can extract them") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "template_part_inline_images.docx";
    const fs::path image_path = fs::current_path() / "template_part_inline_images.png";
    const fs::path extracted_path =
        fs::current_path() / "template_part_inline_images_extracted.png";
    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    const std::vector<unsigned char> expected_image_data(image_data.begin(), image_data.end());
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one", image_path, 20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two", image_path, 30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", image_path, 40U, 20U),
             1);

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(header_images[0].content_type, "image/png");
    CHECK_EQ(header_images[0].width_px, 20U);
    CHECK_EQ(header_images[0].height_px, 10U);
    CHECK_EQ(header_images[1].entry_name, "word/media/image2.png");
    CHECK_EQ(header_images[1].content_type, "image/png");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);
    CHECK(header_template.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_image_data);
    CHECK_FALSE(header_template.extract_inline_image(2U, extracted_path));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");
    CHECK_EQ(footer_images[0].width_px, 40U);
    CHECK_EQ(footer_images[0].height_px, 20U);

    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);
}

TEST_CASE("template part replace_inline_image updates only the selected header image") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U, 0x80U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U, 0xF9U, 0x04U,
        0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    const fs::path target = fs::current_path() / "template_part_replace_inline_image.docx";
    const fs::path source_image_path =
        fs::current_path() / "template_part_replace_inline_image_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "template_part_replace_inline_image_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "template_part_replace_inline_image_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string source_image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                        sizeof(tiny_png_bytes));
    const std::string replacement_image_data(reinterpret_cast<const char *>(tiny_gif_bytes),
                                             sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one",
                                                         source_image_path, 20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two",
                                                         source_image_path, 30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", source_image_path,
                                                         40U, 20U),
             1);
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.replace_inline_image(1U, replacement_image_path));

    auto header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(header_images[0].content_type, "image/png");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);
    CHECK_EQ(header_images[1].content_type, "image/gif");
    CHECK(header_images[1].entry_name.ends_with(".gif"));
    CHECK_NE(header_images[1].entry_name, "word/media/image2.png");

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");

    const auto replacement_entry_name = header_images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename()).generic_string();

    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image3.png"));
    CHECK(test_docx_entry_exists(target, replacement_entry_name.c_str()));
    CHECK_EQ(read_test_docx_entry(target, replacement_entry_name.c_str()),
             replacement_image_data);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_EQ(saved_header_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);
    CHECK_NE(saved_header_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    const auto saved_footer_relationships =
        read_test_docx_entry(target, "word/_rels/footer1.xml.rels");
    CHECK_NE(saved_footer_relationships.find("Target=\"media/image3.png\""),
             std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"gif\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/gif\""), std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[1].content_type, "image/gif");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);
    CHECK_EQ(header_images[1].entry_name, replacement_entry_name);
    CHECK(header_template.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    footer_template = reopened_again.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE("template part drawing_images includes anchored header images") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U, 0x80U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U, 0xF9U, 0x04U,
        0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    const fs::path target = fs::current_path() / "template_part_drawing_images_anchor.docx";
    const fs::path source_image_path =
        fs::current_path() / "template_part_drawing_images_anchor_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "template_part_drawing_images_anchor_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "template_part_drawing_images_anchor_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string source_image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                        sizeof(tiny_png_bytes));
    const std::string replacement_image_data(reinterpret_cast<const char *>(tiny_gif_bytes),
                                             sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one",
                                                         source_image_path, 20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two",
                                                         source_image_path, 30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", source_image_path,
                                                         40U, 20U),
             1);
    CHECK_FALSE(doc.save());

    auto anchored_header_xml = read_test_docx_entry(target, "word/header1.xml");
    anchored_header_xml = convert_nth_inline_drawing_to_anchor(anchored_header_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_header_xml, "<wp:anchor"), 1U);
    rewrite_test_docx_entry(target, "word/header1.xml", std::move(anchored_header_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    const auto header_drawing_images = header_template.drawing_images();
    REQUIRE_EQ(header_drawing_images.size(), 2U);
    CHECK_EQ(header_drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(header_drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(header_drawing_images[1].width_px, 30U);
    CHECK_EQ(header_drawing_images[1].height_px, 15U);

    const auto header_inline_images = header_template.inline_images();
    REQUIRE_EQ(header_inline_images.size(), 1U);
    CHECK_EQ(header_inline_images[0].entry_name, "word/media/image1.png");

    CHECK(header_template.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path),
             std::vector<unsigned char>(source_image_data.begin(), source_image_data.end()));

    CHECK(header_template.replace_drawing_image(1U, replacement_image_path));

    auto updated_header_images = header_template.drawing_images();
    REQUIRE_EQ(updated_header_images.size(), 2U);
    CHECK_EQ(updated_header_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_header_images[1].content_type, "image/gif");
    CHECK(updated_header_images[1].entry_name.ends_with(".gif"));
    const auto replacement_entry_name = updated_header_images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename()).generic_string();

    CHECK_FALSE(reopened.save());

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:anchor"), 1U);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_EQ(saved_header_relationships.find("Target=\"media/image2.png\""), std::string::npos);
    CHECK_NE(saved_header_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    updated_header_images = header_template.drawing_images();
    REQUIRE_EQ(updated_header_images.size(), 2U);
    CHECK_EQ(updated_header_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_header_images[1].content_type, "image/gif");
    CHECK_EQ(updated_header_images[1].entry_name, replacement_entry_name);
    CHECK(header_template.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    footer_template = reopened_again.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}
