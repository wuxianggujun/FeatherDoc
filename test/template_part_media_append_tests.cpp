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

TEST_CASE("template part remove_drawing_image and remove_inline_image prune header media") {
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

    const fs::path target = fs::current_path() / "template_part_remove_images.docx";
    const fs::path image_path = fs::current_path() / "template_part_remove_images_source.png";
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
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one", image_path,
                                                         20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two", image_path,
                                                         30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", image_path, 40U, 20U),
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

    auto header_drawing_images = header_template.drawing_images();
    REQUIRE_EQ(header_drawing_images.size(), 2U);
    CHECK_EQ(header_drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);

    CHECK(header_template.remove_drawing_image(1U));
    header_drawing_images = header_template.drawing_images();
    REQUIRE_EQ(header_drawing_images.size(), 1U);
    CHECK_EQ(header_drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(header_template.inline_images().size(), 1U);

    CHECK(header_template.remove_inline_image(0U));
    CHECK(header_template.drawing_images().empty());
    CHECK(header_template.inline_images().empty());
    CHECK_FALSE(header_template.remove_drawing_image(0U));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");

    CHECK_FALSE(reopened.save());

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 0U);
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:anchor"), 0U);
    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_EQ(saved_header_relationships.find("relationships/image"), std::string::npos);

    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image3.png"));

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.drawing_images().empty());
    CHECK(header_template.inline_images().empty());

    footer_template = reopened_again.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.inline_images().size(), 1U);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("template part replace_bookmark_with_floating_image writes anchored header drawings") {
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

    const fs::path target =
        fs::current_path() / "template_part_replace_bookmark_floating_image.docx";
    const fs::path image_path =
        fs::current_path() / "template_part_replace_bookmark_floating_image.png";
    const fs::path extracted_path =
        fs::current_path() /
        "template_part_replace_bookmark_floating_image_extracted.png";
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
  <w:p><w:r><w:t>before</w:t></w:r></w:p>
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p><w:r><w:t>after</w:t></w:r></w:p>
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

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    const std::vector<unsigned char> expected_image_data(image_data.begin(), image_data.end());
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 40;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = 12;

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_floating_image("header_logo",
                                                                  image_path, 30U, 15U,
                                                                  options),
             1);
    CHECK_FALSE(doc.save());

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header_xml.find("<wp:anchor"), std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>381000</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>114300</wp:posOffset>"),
             std::string::npos);
    CHECK_EQ(saved_header_xml.find("w:name=\"header_logo\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto drawing_images = header_template.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].width_px, 30U);
    CHECK_EQ(drawing_images[0].height_px, 15U);
    CHECK(header_template.extract_drawing_image(0U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_image_data);

    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);
}

TEST_CASE("body template part can append inline images and preserve them across reopen save") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "body_template_append_image_roundtrip.docx";
    const fs::path image_path =
        fs::current_path() / "body_template_append_image_roundtrip.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.append_image(image_path));
    CHECK(body_template.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image2.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             2U);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"media/image2.png\""), std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 2U);
    CHECK_NE(saved_document_xml.find("cx=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto body_images = body_template.inline_images();
    REQUIRE_EQ(body_images.size(), 2U);
    CHECK_EQ(body_images[0].content_type, "image/png");
    CHECK_EQ(body_images[0].width_px, 1U);
    CHECK_EQ(body_images[0].height_px, 1U);
    CHECK_EQ(body_images[1].content_type, "image/png");
    CHECK_EQ(body_images[1].width_px, 20U);
    CHECK_EQ(body_images[1].height_px, 10U);

    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(relationships_after_resave.find("Target=\"media/image2.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("body template part can append floating images and preserve them across reopen save") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "body_template_append_floating_image_roundtrip.docx";
    const fs::path image_path =
        fs::current_path() / "body_template_append_floating_image_roundtrip.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 24;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = -8;
    options.behind_text = true;
    options.allow_overlap = false;
    options.z_order = 32U;
    options.wrap_mode = featherdoc::floating_image_wrap_mode::top_bottom;
    options.wrap_distance_top_px = 4U;
    options.wrap_distance_bottom_px = 9U;
    options.crop = featherdoc::floating_image_crop{25U, 50U, 75U, 100U};

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.append_floating_image(image_path, 20U, 10U, options));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             1U);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 0U);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>228600</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>-76200</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("behindDoc=\"1\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("allowOverlap=\"0\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeHeight=\"32\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distT=\"38100\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distB=\"85725\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:wrapTopAndBottom"), std::string::npos);
    CHECK_NE(saved_document_xml.find("<a:srcRect l=\"2500\" t=\"5000\" r=\"7500\" b=\"10000\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto drawing_images = body_template.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[0].width_px, 20U);
    CHECK_EQ(drawing_images[0].height_px, 10U);
    REQUIRE(drawing_images[0].floating_options.has_value());
    CHECK_EQ(drawing_images[0].floating_options->horizontal_reference,
             featherdoc::floating_image_horizontal_reference::page);
    CHECK_EQ(drawing_images[0].floating_options->horizontal_offset_px, 24);
    CHECK_EQ(drawing_images[0].floating_options->vertical_reference,
             featherdoc::floating_image_vertical_reference::margin);
    CHECK_EQ(drawing_images[0].floating_options->vertical_offset_px, -8);
    CHECK(drawing_images[0].floating_options->behind_text);
    CHECK_FALSE(drawing_images[0].floating_options->allow_overlap);
    CHECK_EQ(drawing_images[0].floating_options->z_order, 32U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::top_bottom);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_top_px, 4U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_bottom_px, 9U);
    REQUIRE(drawing_images[0].floating_options->crop.has_value());
    CHECK_EQ(drawing_images[0].floating_options->crop->left_per_mille, 25U);
    CHECK_EQ(drawing_images[0].floating_options->crop->top_per_mille, 50U);
    CHECK_EQ(drawing_images[0].floating_options->crop->right_per_mille, 75U);
    CHECK_EQ(drawing_images[0].floating_options->crop->bottom_per_mille, 100U);
    CHECK_EQ(body_template.inline_images().size(), 0U);

    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("header template part can append inline images") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_append_images.docx";
    const fs::path image_path =
        fs::current_path() / "header_template_append_images.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.add_run("header intro").has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.append_image(image_path));
    CHECK(header_template.append_image(image_path, 30U, 15U));
    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 2U);
    CHECK_NE(saved_header_xml.find("cx=\"9525\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("cy=\"9525\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("cx=\"285750\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("cy=\"142875\""), std::string::npos);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_header_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             2U);
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(saved_header_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"png\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/png\""), std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());

    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[0].content_type, "image/png");
    CHECK_EQ(header_images[0].width_px, 1U);
    CHECK_EQ(header_images[0].height_px, 1U);
    CHECK_EQ(header_images[1].content_type, "image/png");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);

    CHECK(reopened_again.header_paragraphs().add_run(" reopened edit").has_next());
    CHECK_FALSE(reopened_again.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(relationships_after_resave.find("Target=\"media/image2.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("header template part can append floating images") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "header_template_append_floating_images.docx";
    const fs::path image_path =
        fs::current_path() / "header_template_append_floating_images.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 40;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = 12;
    options.z_order = 48U;
    options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    options.wrap_distance_left_px = 5U;
    options.wrap_distance_right_px = 7U;
    options.crop = featherdoc::floating_image_crop{10U, 20U, 30U, 40U};

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.add_run("header intro").has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.append_floating_image(image_path, 30U, 15U, options));
    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:anchor"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 0U);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>381000</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>114300</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeHeight=\"48\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("distL=\"47625\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("distR=\"66675\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:wrapSquare wrapText=\"bothSides\""),
             std::string::npos);
    CHECK_NE(saved_header_xml.find("<a:srcRect l=\"1000\" t=\"2000\" r=\"3000\" b=\"4000\""),
             std::string::npos);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_header_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             1U);
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());

    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto drawing_images = header_template.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[0].width_px, 30U);
    CHECK_EQ(drawing_images[0].height_px, 15U);
    REQUIRE(drawing_images[0].floating_options.has_value());
    CHECK_EQ(drawing_images[0].floating_options->z_order, 48U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::square);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_left_px, 5U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_right_px, 7U);
    REQUIRE(drawing_images[0].floating_options->crop.has_value());
    CHECK_EQ(drawing_images[0].floating_options->crop->left_per_mille, 10U);
    CHECK_EQ(drawing_images[0].floating_options->crop->top_per_mille, 20U);
    CHECK_EQ(drawing_images[0].floating_options->crop->right_per_mille, 30U);
    CHECK_EQ(drawing_images[0].floating_options->crop->bottom_per_mille, 40U);
    CHECK_EQ(header_template.inline_images().size(), 0U);

    CHECK(reopened_again.header_paragraphs().add_run(" reopened edit").has_next());
    CHECK_FALSE(reopened_again.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}
