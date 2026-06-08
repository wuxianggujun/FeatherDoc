#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_bookmark_content_test_support.hpp"

namespace {

TEST_CASE("cli replace-bookmark-image replaces a body bookmark with a scaled inline image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_image_source.docx";
    const fs::path image =
        working_directory / "cli_replace_bookmark_image_fixture.png";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_image_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_image.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(run_cli({"replace-bookmark-image",
                      source.string(),
                      "body_logo",
                      image.string(),
                      "--width",
                      "32",
                      "--height",
                      "16",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-image\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"body_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"image_path\":" + json_quote(image.string())),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"inline\""), std::string::npos);
    CHECK_NE(json.find("\"width_px\":32"), std::string::npos);
    CHECK_NE(json.find("\"height_px\":16"), std::string::npos);
    CHECK_NE(json.find("\"content_type\":\"image/png\""), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].placement, featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(images[0].width_px, 32U);
    CHECK_EQ(images[0].height_px, 16U);
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(read_docx_entry(updated, images[0].entry_name.c_str()), tiny_png_data());
    CHECK_FALSE(body_template.find_bookmark("body_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-image rejects floating layout options") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-image",
                      "missing.docx",
                      "logo",
                      "logo.png",
                      "--floating",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"replace-bookmark-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"replace-bookmark-image does not "
            "accept floating layout options; use "
            "replace-bookmark-floating-image\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-floating-image replaces a section header bookmark with an anchored image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_floating_image_source.docx";
    const fs::path image =
        working_directory / "cli_replace_bookmark_floating_image_fixture.png";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_floating_image_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_floating_image.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(
        run_cli({"replace-bookmark-floating-image",
                 source.string(),
                 "header_logo",
                 image.string(),
                 "--part",
                 "section-header",
                 "--section",
                 "0",
                 "--width",
                 "30",
                 "--height",
                 "15",
                 "--horizontal-reference",
                 "page",
                 "--horizontal-offset",
                 "40",
                 "--vertical-reference",
                 "margin",
                 "--vertical-offset",
                 "12",
                 "--behind-text",
                 "true",
                 "--allow-overlap",
                 "false",
                 "--z-order",
                 "108",
                 "--wrap-mode",
                 "square",
                 "--wrap-distance-left",
                 "5",
                 "--wrap-distance-right",
                 "7",
                 "--crop-left",
                 "10",
                 "--crop-top",
                 "20",
                 "--crop-right",
                 "30",
                 "--crop-bottom",
                 "40",
                 "--output",
                 updated.string(),
                 "--json"},
                output),
        0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-floating-image\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"horizontal_reference\":\"page\""), std::string::npos);
    CHECK_NE(json.find("\"horizontal_offset_px\":40"), std::string::npos);
    CHECK_NE(json.find("\"vertical_reference\":\"margin\""), std::string::npos);
    CHECK_NE(json.find("\"vertical_offset_px\":12"), std::string::npos);
    CHECK_NE(json.find("\"behind_text\":true"), std::string::npos);
    CHECK_NE(json.find("\"allow_overlap\":false"), std::string::npos);
    CHECK_NE(json.find("\"z_order\":108"), std::string::npos);
    CHECK_NE(json.find("\"wrap_mode\":\"square\""), std::string::npos);
    CHECK_NE(json.find("\"crop\":{\"left_per_mille\":10,\"top_per_mille\":20,"
                       "\"right_per_mille\":30,\"bottom_per_mille\":40}"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto images = header_template.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(images[0].width_px, 30U);
    CHECK_EQ(images[0].height_px, 15U);
    REQUIRE(images[0].floating_options.has_value());
    CHECK_EQ(images[0].floating_options->horizontal_reference,
             featherdoc::floating_image_horizontal_reference::page);
    CHECK_EQ(images[0].floating_options->horizontal_offset_px, 40);
    CHECK_EQ(images[0].floating_options->vertical_reference,
             featherdoc::floating_image_vertical_reference::margin);
    CHECK_EQ(images[0].floating_options->vertical_offset_px, 12);
    CHECK(images[0].floating_options->behind_text);
    CHECK_FALSE(images[0].floating_options->allow_overlap);
    CHECK_EQ(images[0].floating_options->z_order, 108U);
    CHECK_EQ(images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::square);
    CHECK_EQ(images[0].floating_options->wrap_distance_left_px, 5U);
    CHECK_EQ(images[0].floating_options->wrap_distance_right_px, 7U);
    REQUIRE(images[0].floating_options->crop.has_value());
    CHECK_EQ(images[0].floating_options->crop->left_per_mille, 10U);
    CHECK_EQ(images[0].floating_options->crop->top_per_mille, 20U);
    CHECK_EQ(images[0].floating_options->crop->right_per_mille, 30U);
    CHECK_EQ(images[0].floating_options->crop->bottom_per_mille, 40U);
    CHECK_FALSE(header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-floating-image requires width and height together") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_floating_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-floating-image",
                      "missing.docx",
                      "logo",
                      "logo.png",
                      "--width",
                      "30",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"replace-bookmark-floating-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"replace-bookmark-floating-image "
            "requires both --width <px> and --height <px> when scaling\"}\n"});

    remove_if_exists(output);
}

} // namespace
