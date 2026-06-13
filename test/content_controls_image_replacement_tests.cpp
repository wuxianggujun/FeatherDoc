#include <filesystem>
#include <string>
#include <system_error>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"
#include "basic_image_fixture_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("content control image replacement supports block run and table-cell controls") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "content_controls_replace_images.docx";
    const fs::path image_path = fs::current_path() / "content_controls_replace_images.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:sdt>
      <w:sdtPr>
        <w:tag w:val="hero_image"/>
        <w:showingPlcHdr/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>hero placeholder</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:p>
      <w:r><w:t>Inline badge: </w:t></w:r>
      <w:sdt>
        <w:sdtPr><w:alias w:val="Inline Badge"/></w:sdtPr>
        <w:sdtContent><w:r><w:t>inline placeholder</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:tbl>
      <w:tr>
        <w:sdt>
          <w:sdtPr><w:tag w:val="cell_image"/></w:sdtPr>
          <w:sdtContent>
            <w:tc><w:p><w:r><w:t>cell placeholder</w:t></w:r></w:p></w:tc>
          </w:sdtContent>
        </w:sdt>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_content_control_with_image_by_tag("hero_image", image_path,
                                                           48U, 24U),
             1U);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(doc.replace_content_control_with_image_by_alias("Inline Badge", image_path),
             1U);
    CHECK_FALSE(doc.last_error());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_EQ(body_template.replace_content_control_with_image_by_tag(
                 "cell_image", image_path, 30U, 20U),
             1U);
    CHECK_FALSE(doc.last_error());
    CHECK_FALSE(doc.save());

    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image2.png"), image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image3.png"), image_data);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("hero placeholder"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("inline placeholder"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("cell placeholder"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:showingPlcHdr"), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 3U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:drawing"), 3U);
    CHECK_NE(saved_document_xml.find("cx=\"457200\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"228600\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"285750\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"190500\""), std::string::npos);

    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_document_xml.c_str()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    const auto hero_content = body.child("w:sdt").child("w:sdtContent");
    REQUIRE(hero_content != pugi::xml_node{});
    CHECK_EQ(count_named_children(hero_content, "w:p"), 1U);
    CHECK_EQ(count_named_descendants(hero_content, "w:drawing"), 1U);

    const auto inline_content = body.child("w:p").child("w:sdt").child("w:sdtContent");
    REQUIRE(inline_content != pugi::xml_node{});
    CHECK_EQ(count_named_children(inline_content, "w:r"), 1U);
    CHECK_EQ(count_named_descendants(inline_content, "w:p"), 0U);
    CHECK_EQ(count_named_descendants(inline_content, "w:drawing"), 1U);

    const auto cell_content = body.child("w:tbl")
                                  .child("w:tr")
                                  .child("w:sdt")
                                  .child("w:sdtContent");
    REQUIRE(cell_content != pugi::xml_node{});
    CHECK_EQ(count_named_children(cell_content, "w:tc"), 1U);
    CHECK_EQ(count_named_descendants(cell_content, "w:p"), 1U);
    CHECK_EQ(count_named_descendants(cell_content, "w:drawing"), 1U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 3U);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[0].width_px, 48U);
    CHECK_EQ(drawing_images[0].height_px, 24U);
    CHECK_EQ(drawing_images[1].content_type, "image/png");
    CHECK_EQ(drawing_images[1].width_px, 1U);
    CHECK_EQ(drawing_images[1].height_px, 1U);
    CHECK_EQ(drawing_images[2].content_type, "image/png");
    CHECK_EQ(drawing_images[2].width_px, 30U);
    CHECK_EQ(drawing_images[2].height_px, 20U);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("content control image replacement validates selector dimensions and row kind") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "content_controls_replace_images_invalid.docx";
    const fs::path image_path = fs::current_path() / "content_controls_replace_images_invalid.png";
    const fs::path missing_image_path = fs::current_path() / "content_controls_replace_images_missing.png";
    fs::remove(target);
    fs::remove(image_path);
    fs::remove(missing_image_path);

    write_binary_file(image_path, tiny_png_data());

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:sdt>
        <w:sdtPr>
          <w:tag w:val="row_image"/>
          <w:alias w:val="Row Image"/>
        </w:sdtPr>
        <w:sdtContent>
          <w:tr><w:tc><w:p><w:r><w:t>row placeholder</w:t></w:r></w:p></w:tc></w:tr>
        </w:sdtContent>
      </w:sdt>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_content_control_with_image_by_tag({}, image_path), 0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("tag"), std::string::npos);

    CHECK_EQ(doc.replace_content_control_with_image_by_alias({}, image_path), 0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("alias"), std::string::npos);

    CHECK_EQ(doc.replace_content_control_with_image_by_tag("row_image", image_path, 0U,
                                                           20U),
             0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("width and height"), std::string::npos);

    CHECK_EQ(doc.replace_content_control_with_image_by_tag("missing", missing_image_path),
             0U);
    CHECK_FALSE(doc.last_error());

    CHECK_EQ(doc.replace_content_control_with_image_by_tag("row_image", image_path), 0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("image replacement"), std::string::npos);

    const auto controls = doc.list_content_controls();
    REQUIRE_EQ(controls.size(), 1U);
    CHECK_EQ(controls.front().kind, featherdoc::content_control_kind::table_row);
    CHECK_EQ(controls.front().text, "row placeholder");

    fs::remove(target);
    fs::remove(image_path);
}
