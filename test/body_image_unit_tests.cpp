#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"
#include "basic_image_fixture_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("append_image writes inline media parts and preserves them across reopen save") {
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

    const fs::path target = fs::current_path() / "append_image_roundtrip.docx";
    const fs::path image_path = fs::current_path() / "tiny_image.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));
    CHECK(doc.append_image(image_path, 20U, 10U));
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
             2);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"media/image2.png\""), std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"png\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/png\""), std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 2);
    CHECK_NE(saved_document_xml.find("cx=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.paragraphs().add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());
    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(relationships_after_resave.find("Target=\"media/image2.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("append_floating_image writes anchored media parts and preserves them across reopen save") {
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

    const fs::path target = fs::current_path() / "append_floating_image_roundtrip.docx";
    const fs::path image_path = fs::current_path() / "floating_tiny_image.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
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
    options.z_order = 64U;
    options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    options.wrap_distance_left_px = 8U;
    options.wrap_distance_right_px = 10U;
    options.wrap_distance_top_px = 6U;
    options.wrap_distance_bottom_px = 12U;
    options.crop = featherdoc::floating_image_crop{15U, 25U, 35U, 45U};

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_floating_image(image_path, 20U, 10U, options));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             1);
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
    CHECK_NE(saved_document_xml.find("relativeHeight=\"64\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distT=\"57150\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distB=\"114300\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distL=\"76200\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distR=\"95250\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:wrapSquare wrapText=\"bothSides\""),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("<a:srcRect l=\"1500\" t=\"2500\" r=\"3500\" b=\"4500\""),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto drawing_images = reopened.drawing_images();
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
    CHECK_EQ(drawing_images[0].floating_options->z_order, 64U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::square);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_left_px, 8U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_right_px, 10U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_top_px, 6U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_bottom_px, 12U);
    REQUIRE(drawing_images[0].floating_options->crop.has_value());
    CHECK_EQ(drawing_images[0].floating_options->crop->left_per_mille, 15U);
    CHECK_EQ(drawing_images[0].floating_options->crop->top_per_mille, 25U);
    CHECK_EQ(drawing_images[0].floating_options->crop->right_per_mille, 35U);
    CHECK_EQ(drawing_images[0].floating_options->crop->bottom_per_mille, 45U);
    CHECK_EQ(reopened.inline_images().size(), 0U);
    CHECK(reopened.paragraphs().add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("append_floating_image rejects crop values that remove the visible image area") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "append_floating_image_invalid_crop.docx";
    const fs::path image_path = fs::current_path() / "floating_invalid_crop.png";
    fs::remove(target);
    fs::remove(image_path);

    write_binary_file(image_path, tiny_png_data());

    featherdoc::floating_image_options options;
    options.crop = featherdoc::floating_image_crop{600U, 0U, 400U, 0U};

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK_FALSE(doc.append_floating_image(image_path, 20U, 10U, options));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("crop"), std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("inline_images lists existing body images and can extract them") {
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

    const fs::path target = fs::current_path() / "inline_images_roundtrip.docx";
    const fs::path image_path = fs::current_path() / "inline_images_roundtrip.png";
    const fs::path extracted_path = fs::current_path() / "inline_images_extracted.png";
    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    const std::vector<unsigned char> expected_image_data(image_data.begin(), image_data.end());
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));
    CHECK(doc.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto images = reopened.inline_images();
    REQUIRE_EQ(images.size(), 2U);

    CHECK_EQ(images[0].index, 0U);
    CHECK_EQ(images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(images[0].display_name, image_path.filename().string());
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(images[0].width_px, 1U);
    CHECK_EQ(images[0].height_px, 1U);

    CHECK_EQ(images[1].index, 1U);
    CHECK_EQ(images[1].entry_name, "word/media/image2.png");
    CHECK_EQ(images[1].display_name, image_path.filename().string());
    CHECK_EQ(images[1].content_type, "image/png");
    CHECK_EQ(images[1].width_px, 20U);
    CHECK_EQ(images[1].height_px, 10U);

    CHECK(reopened.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_image_data);

    CHECK_FALSE(reopened.extract_inline_image(2U, extracted_path));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);
}

TEST_CASE("replace_inline_image updates only the selected body image and preserves layout") {
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

    const fs::path target = fs::current_path() / "replace_inline_image.docx";
    const fs::path source_image_path = fs::current_path() / "replace_inline_image_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "replace_inline_image_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "replace_inline_image_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

    const std::string source_image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                        sizeof(tiny_png_bytes));
    const std::string replacement_image_data(reinterpret_cast<const char *>(tiny_gif_bytes),
                                             sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(source_image_path));
    CHECK(doc.append_image(source_image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.replace_inline_image(1U, replacement_image_path));

    auto images = reopened.inline_images();
    REQUIRE_EQ(images.size(), 2U);
    CHECK_EQ(images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(images[1].width_px, 20U);
    CHECK_EQ(images[1].height_px, 10U);
    CHECK_EQ(images[1].content_type, "image/gif");
    CHECK(images[1].entry_name.ends_with(".gif"));
    CHECK_NE(images[1].entry_name, "word/media/image2.png");

    const auto replacement_entry_name = images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename()).generic_string();

    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, replacement_entry_name.c_str()));
    CHECK_EQ(read_test_docx_entry(target, replacement_entry_name.c_str()),
             replacement_image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"media/image2.png\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"gif\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/gif\""), std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    images = reopened_again.inline_images();
    REQUIRE_EQ(images.size(), 2U);
    CHECK_EQ(images[1].content_type, "image/gif");
    CHECK_EQ(images[1].width_px, 20U);
    CHECK_EQ(images[1].height_px, 10U);
    CHECK_EQ(images[1].entry_name, replacement_entry_name);
    CHECK(reopened_again.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE("drawing_images includes anchored body images and replace_drawing_image preserves them") {
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

    const fs::path target = fs::current_path() / "drawing_images_anchor.docx";
    const fs::path source_image_path = fs::current_path() / "drawing_images_anchor_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "drawing_images_anchor_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "drawing_images_anchor_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

    const std::string source_image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                        sizeof(tiny_png_bytes));
    const std::string replacement_image_data(reinterpret_cast<const char *>(tiny_gif_bytes),
                                             sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(source_image_path));
    CHECK(doc.append_image(source_image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    auto anchored_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    anchored_document_xml = convert_nth_inline_drawing_to_anchor(anchored_document_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_document_xml, "<wp:anchor"), 1U);
    rewrite_test_docx_entry(target, test_document_xml_entry, std::move(anchored_document_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 2U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[1].content_type, "image/png");
    CHECK_EQ(drawing_images[1].width_px, 20U);
    CHECK_EQ(drawing_images[1].height_px, 10U);

    const auto inline_images = reopened.inline_images();
    REQUIRE_EQ(inline_images.size(), 1U);
    CHECK_EQ(inline_images[0].entry_name, "word/media/image1.png");

    CHECK(reopened.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path),
             std::vector<unsigned char>(source_image_data.begin(), source_image_data.end()));
    CHECK_FALSE(reopened.extract_inline_image(1U, extracted_path));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    CHECK(reopened.replace_drawing_image(1U, replacement_image_path));

    auto updated_images = reopened.drawing_images();
    REQUIRE_EQ(updated_images.size(), 2U);
    CHECK_EQ(updated_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_images[1].width_px, 20U);
    CHECK_EQ(updated_images[1].height_px, 10U);
    CHECK_EQ(updated_images[1].content_type, "image/gif");
    CHECK(updated_images[1].entry_name.ends_with(".gif"));
    const auto replacement_entry_name = updated_images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename()).generic_string();

    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, replacement_entry_name.c_str()));
    CHECK_EQ(read_test_docx_entry(target, replacement_entry_name.c_str()),
             replacement_image_data);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 1U);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"media/image2.png\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"gif\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/gif\""), std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    updated_images = reopened_again.drawing_images();
    REQUIRE_EQ(updated_images.size(), 2U);
    CHECK_EQ(updated_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_images[1].content_type, "image/gif");
    CHECK_EQ(updated_images[1].entry_name, replacement_entry_name);
    CHECK(reopened_again.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE("remove_drawing_image and remove_inline_image prune body media parts") {
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

    const fs::path target = fs::current_path() / "drawing_images_remove.docx";
    const fs::path image_path = fs::current_path() / "drawing_images_remove_source.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));
    CHECK(doc.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    auto anchored_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    anchored_document_xml = convert_nth_inline_drawing_to_anchor(anchored_document_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_document_xml, "<wp:anchor"), 1U);
    rewrite_test_docx_entry(target, test_document_xml_entry, std::move(anchored_document_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 2U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);

    CHECK(reopened.remove_drawing_image(1U));
    drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(reopened.inline_images().size(), 1U);

    CHECK(reopened.remove_inline_image(0U));
    CHECK(reopened.drawing_images().empty());
    CHECK(reopened.inline_images().empty());
    CHECK_FALSE(reopened.remove_inline_image(0U));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    CHECK_FALSE(reopened.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 0U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 0U);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(saved_relationships.find("relationships/image"), std::string::npos);

    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    CHECK(reopened_again.drawing_images().empty());
    CHECK(reopened_again.inline_images().empty());

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("append_image detects raster image dimensions through stb_image") {
    namespace fs = std::filesystem;

    struct raster_case final {
        const char *extension;
        const char *content_type;
        std::string (*data_factory)();
        std::uint32_t expected_width;
        std::uint32_t expected_height;
    };

    const raster_case cases[] = {
        {"png", "image/png", tiny_png_data, 1U, 1U},
        {"jpg", "image/jpeg", tiny_jpeg_data, 3U, 2U},
        {"gif", "image/gif", tiny_gif_data, 1U, 1U},
        {"bmp", "image/bmp", tiny_bmp_data, 3U, 2U},
    };

    const fs::path target = fs::current_path() / "stb_raster_dimensions.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    for (const auto &test_case : cases) {
        const fs::path image_path =
            fs::current_path() /
            ("stb_raster_dimensions." + std::string{test_case.extension});
        fs::remove(image_path);
        write_binary_file(image_path, test_case.data_factory());

        CHECK(doc.append_image(image_path));

        fs::remove(image_path);
    }

    const auto images = doc.drawing_images();
    REQUIRE_EQ(images.size(), std::size(cases));
    for (std::size_t index = 0U; index < std::size(cases); ++index) {
        CHECK_EQ(images[index].content_type, cases[index].content_type);
        CHECK_EQ(images[index].width_px, cases[index].expected_width);
        CHECK_EQ(images[index].height_px, cases[index].expected_height);
        CHECK_NE(images[index].entry_name.find("." + std::string{cases[index].extension}),
                 std::string::npos);
    }

    CHECK_FALSE(doc.save());
    const auto content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(content_types.find("ContentType=\"image/png\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/jpeg\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/gif\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/bmp\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_images = reopened.drawing_images();
    REQUIRE_EQ(reopened_images.size(), std::size(cases));
    for (std::size_t index = 0U; index < std::size(cases); ++index) {
        CHECK_EQ(reopened_images[index].content_type, cases[index].content_type);
        CHECK_EQ(reopened_images[index].width_px, cases[index].expected_width);
        CHECK_EQ(reopened_images[index].height_px, cases[index].expected_height);
    }

    fs::remove(target);
}

TEST_CASE("append_image lets stb_image identify raster bytes while package metadata follows extension") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "stb_raster_extension_metadata.docx";
    const fs::path image_path = fs::current_path() / "jpeg_bytes_named_png.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto jpeg_bytes = tiny_jpeg_data();
    write_binary_file(image_path, jpeg_bytes);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(image_path));

    const auto images = doc.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(images[0].width_px, 3U);
    CHECK_EQ(images[0].height_px, 2U);
    CHECK_NE(images[0].entry_name.find(".png"), std::string::npos);

    CHECK_FALSE(doc.save());
    const auto content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(content_types.find("Extension=\"png\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/png\""), std::string::npos);
    CHECK_EQ(read_test_docx_entry(target, images[0].entry_name.c_str()), jpeg_bytes);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("append_image supports SVG WebP and TIFF inputs") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "extended_image_formats.docx";
    const fs::path svg_path = fs::current_path() / "extended_image_format.svg";
    const fs::path webp_path = fs::current_path() / "extended_image_format.webp";
    const fs::path tiff_path = fs::current_path() / "extended_image_format.tiff";
    fs::remove(target);
    fs::remove(svg_path);
    fs::remove(webp_path);
    fs::remove(tiff_path);

    write_binary_file(svg_path, tiny_svg_data());
    write_binary_file(webp_path, tiny_webp_data());
    write_binary_file(tiff_path, tiny_tiff_data());

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.append_image(svg_path));
    CHECK(doc.append_image(webp_path));
    CHECK(doc.append_image(tiff_path));

    const auto images = doc.drawing_images();
    REQUIRE(images.size() == 3U);
    CHECK_EQ(images[0].content_type, "image/svg+xml");
    CHECK_EQ(images[0].width_px, 3U);
    CHECK_EQ(images[0].height_px, 2U);
    CHECK_EQ(images[1].content_type, "image/webp");
    CHECK_EQ(images[1].width_px, 3U);
    CHECK_EQ(images[1].height_px, 2U);
    CHECK_EQ(images[2].content_type, "image/tiff");
    CHECK_EQ(images[2].width_px, 3U);
    CHECK_EQ(images[2].height_px, 2U);

    CHECK_FALSE(doc.save());
    const auto content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(content_types.find("Extension=\"svg\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/svg+xml\""), std::string::npos);
    CHECK_NE(content_types.find("Extension=\"webp\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/webp\""), std::string::npos);
    CHECK_NE(content_types.find("Extension=\"tiff\""), std::string::npos);
    CHECK_NE(content_types.find("ContentType=\"image/tiff\""), std::string::npos);
    CHECK_NE(images[0].entry_name.find(".svg"), std::string::npos);
    CHECK_NE(images[1].entry_name.find(".webp"), std::string::npos);
    CHECK_NE(images[2].entry_name.find(".tiff"), std::string::npos);
    CHECK(test_docx_entry_exists(target, images[0].entry_name.c_str()));
    CHECK(test_docx_entry_exists(target, images[1].entry_name.c_str()));
    CHECK(test_docx_entry_exists(target, images[2].entry_name.c_str()));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_images = reopened.drawing_images();
    REQUIRE(reopened_images.size() == 3U);
    CHECK_EQ(reopened_images[0].content_type, "image/svg+xml");
    CHECK_EQ(reopened_images[1].content_type, "image/webp");
    CHECK_EQ(reopened_images[2].content_type, "image/tiff");

    fs::remove(target);
    fs::remove(svg_path);
    fs::remove(webp_path);
    fs::remove(tiff_path);
}

TEST_CASE("append_image rejects supported extensions with unreadable image dimensions") {
    namespace fs = std::filesystem;

    const fs::path corrupt_path = fs::current_path() / "corrupt_image.png";
    fs::remove(corrupt_path);

    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    write_binary_file(corrupt_path, "not an image");
    CHECK_FALSE(doc.append_image(corrupt_path));
    CHECK_EQ(doc.last_error().code, featherdoc::document_errc::image_size_read_failed);
    CHECK(doc.drawing_images().empty());
    CHECK(doc.inline_images().empty());

    fs::remove(corrupt_path);
}

TEST_CASE("append_image reports unsupported image extensions explicitly") {
    namespace fs = std::filesystem;

    const fs::path image_path = fs::current_path() / "unsupported_image.txt";
    fs::remove(image_path);
    write_binary_file(image_path, "not an image");

    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());
    CHECK_FALSE(doc.append_image(image_path));
    CHECK_EQ(doc.last_error().code, featherdoc::document_errc::image_format_unsupported);
    CHECK_FALSE(doc.last_error().detail.empty());

    fs::remove(image_path);
}
