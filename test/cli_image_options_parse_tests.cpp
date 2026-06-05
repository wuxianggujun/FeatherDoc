#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_image_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("cli image options parse accepts selectors and outputs") {
    featherdoc_cli::inspect_images_options inspect;
    std::string error;

    CHECK(featherdoc_cli::parse_inspect_images_options(
        {"inspect-images", "input.docx", "--relationship-id", "rId7",
         "--image-entry-name", "word/media/image1.png", "--json"},
        2U, inspect, error));
    CHECK(inspect.part == featherdoc_cli::validation_part_family::body);
    REQUIRE(inspect.relationship_id.has_value());
    REQUIRE(inspect.image_entry_name.has_value());
    CHECK(*inspect.relationship_id == "rId7");
    CHECK(*inspect.image_entry_name == "word/media/image1.png");
    CHECK(inspect.json_output);

    featherdoc_cli::replace_image_options replace;
    error.clear();
    CHECK(featherdoc_cli::parse_replace_image_options(
        {"replace-image", "input.docx", "--image", "2", "--output",
         "out.docx", "--json"},
        2U, replace, error));
    REQUIRE(replace.image_index.has_value());
    REQUIRE(replace.output_path.has_value());
    CHECK(*replace.image_index == 2U);
    CHECK(replace.output_path->filename().string() == "out.docx");
    CHECK(replace.json_output);

    featherdoc_cli::extract_image_options extract;
    error.clear();
    CHECK(featherdoc_cli::parse_extract_image_options(
        {"extract-image", "input.docx", "--relationship-id", "rId7",
         "--json"},
        2U, extract, error));
    REQUIRE(extract.relationship_id.has_value());
    CHECK(*extract.relationship_id == "rId7");
    CHECK(extract.json_output);

    featherdoc_cli::remove_image_options remove;
    error.clear();
    CHECK(featherdoc_cli::parse_remove_image_options(
        {"remove-image", "input.docx", "--image-entry-name",
         "word/media/image1.png", "--output", "out.docx"},
        2U, remove, error));
    REQUIRE(remove.image_entry_name.has_value());
    REQUIRE(remove.output_path.has_value());
    CHECK(*remove.image_entry_name == "word/media/image1.png");
    CHECK(remove.output_path->filename().string() == "out.docx");
}

TEST_CASE("cli image options parse accepts append floating layout and crop") {
    featherdoc_cli::append_image_options append;
    std::string error;

    CHECK(featherdoc_cli::parse_append_image_options(
        {"append-image", "input.docx", "--width", "20", "--height", "10",
         "--floating", "--horizontal-reference", "page",
         "--horizontal-offset", "24", "--vertical-reference", "margin",
         "--vertical-offset", "-8", "--behind-text", "true",
         "--allow-overlap", "false", "--z-order", "72", "--wrap-mode",
         "square", "--wrap-distance-left", "8", "--wrap-distance-right", "10",
         "--wrap-distance-top", "6", "--wrap-distance-bottom", "12",
         "--crop-left", "15", "--crop-top", "25", "--crop-right", "35",
         "--crop-bottom", "45", "--output", "out.docx", "--json"},
        2U, append, error));
    REQUIRE(append.width_px.has_value());
    REQUIRE(append.height_px.has_value());
    REQUIRE(append.output_path.has_value());
    REQUIRE(append.floating_options.crop.has_value());
    CHECK(*append.width_px == 20U);
    CHECK(*append.height_px == 10U);
    CHECK(append.floating);
    CHECK(append.floating_options.horizontal_reference ==
          featherdoc::floating_image_horizontal_reference::page);
    CHECK(append.floating_options.horizontal_offset_px == 24);
    CHECK(append.floating_options.vertical_reference ==
          featherdoc::floating_image_vertical_reference::margin);
    CHECK(append.floating_options.vertical_offset_px == -8);
    CHECK(append.floating_options.behind_text);
    CHECK_FALSE(append.floating_options.allow_overlap);
    CHECK(append.floating_options.z_order == 72U);
    CHECK(append.floating_options.wrap_mode ==
          featherdoc::floating_image_wrap_mode::square);
    CHECK(append.floating_options.wrap_distance_left_px == 8U);
    CHECK(append.floating_options.wrap_distance_right_px == 10U);
    CHECK(append.floating_options.wrap_distance_top_px == 6U);
    CHECK(append.floating_options.wrap_distance_bottom_px == 12U);
    CHECK(append.floating_options.crop->left_per_mille == 15U);
    CHECK(append.floating_options.crop->top_per_mille == 25U);
    CHECK(append.floating_options.crop->right_per_mille == 35U);
    CHECK(append.floating_options.crop->bottom_per_mille == 45U);
    CHECK(append.output_path->filename().string() == "out.docx");
    CHECK(append.json_output);
}

TEST_CASE("cli image options parse validates selector and scaling requirements") {
    featherdoc_cli::replace_image_options replace;
    std::string error;

    CHECK_FALSE(featherdoc_cli::parse_replace_image_options(
        {"replace-image", "input.docx", "--json"}, 2U, replace, error));
    CHECK(error ==
          "replace-image requires --image <index>, --relationship-id <id>, or "
          "--image-entry-name <path>");

    featherdoc_cli::extract_image_options extract;
    error.clear();
    CHECK_FALSE(featherdoc_cli::parse_extract_image_options(
        {"extract-image", "input.docx", "--json"}, 2U, extract, error));
    CHECK(error ==
          "extract-image requires --image <index>, --relationship-id <id>, or "
          "--image-entry-name <path>");

    featherdoc_cli::remove_image_options remove;
    error.clear();
    CHECK_FALSE(featherdoc_cli::parse_remove_image_options(
        {"remove-image", "input.docx", "--json"}, 2U, remove, error));
    CHECK(error ==
          "remove-image requires --image <index>, --relationship-id <id>, or "
          "--image-entry-name <path>");

    featherdoc_cli::append_image_options append;
    error.clear();
    CHECK_FALSE(featherdoc_cli::parse_append_image_options(
        {"append-image", "input.docx", "--width", "20", "--json"}, 2U,
        append, error));
    CHECK(error ==
          "append-image requires both --width <px> and --height <px> when scaling");

    featherdoc_cli::append_image_options append_crop;
    error.clear();
    CHECK_FALSE(featherdoc_cli::parse_append_image_options(
        {"append-image", "input.docx", "--width", "20", "--height", "10",
         "--crop-left", "1", "--crop-top", "2", "--crop-right", "3",
         "--json"},
        2U, append_crop, error));
    CHECK(error ==
          "append-image requires --crop-left/--crop-top/--crop-right/--crop-bottom together");
}
