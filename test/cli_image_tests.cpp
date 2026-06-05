#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

namespace {
auto tiny_png_data() -> std::string {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes), sizeof(tiny_png_bytes)};
}

auto tiny_gif_data() -> std::string {
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U,
        0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U,
        0xF9U, 0x04U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U,
        0x00U, 0x00U, 0x01U, 0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U,
        0x01U, 0x00U, 0x3BU,
    };

    return {reinterpret_cast<const char *>(tiny_gif_bytes), sizeof(tiny_gif_bytes)};
}

void create_cli_empty_document_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE_FALSE(document.save());
}

void create_cli_image_fixture(const fs::path &path) {
    remove_if_exists(path);

    const auto image_path =
        path.parent_path() / (path.stem().string() + "_fixture_image.png");
    remove_if_exists(image_path);
    write_binary_file(image_path, tiny_png_data());

    featherdoc::floating_image_options body_options;
    body_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    body_options.horizontal_offset_px = 24;
    body_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    body_options.vertical_offset_px = -8;
    body_options.behind_text = true;
    body_options.allow_overlap = false;
    body_options.z_order = 72U;
    body_options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    body_options.wrap_distance_left_px = 8U;
    body_options.wrap_distance_right_px = 10U;
    body_options.wrap_distance_top_px = 6U;
    body_options.wrap_distance_bottom_px = 12U;
    body_options.crop = featherdoc::floating_image_crop{15U, 25U, 35U, 45U};

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.append_image(image_path, 20U, 10U));
    CHECK(body_template.append_floating_image(image_path, 20U, 10U, body_options));

    auto &header = document.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.add_run("cli image header").has_next());
    REQUIRE_FALSE(document.save());

    featherdoc::floating_image_options header_options;
    header_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    header_options.horizontal_offset_px = 40;
    header_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    header_options.vertical_offset_px = 12;
    header_options.z_order = 84U;
    header_options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    header_options.wrap_distance_left_px = 5U;
    header_options.wrap_distance_right_px = 7U;
    header_options.crop = featherdoc::floating_image_crop{10U, 20U, 30U, 40U};

    featherdoc::Document reopened(path);
    REQUIRE_FALSE(reopened.open());

    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.append_floating_image(image_path, 30U, 15U, header_options));
    REQUIRE_FALSE(reopened.save());

    remove_if_exists(image_path);
}

TEST_CASE("cli inspect-images lists selected body drawing images as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_images_body_source.docx";
    const fs::path output = working_directory / "cli_images_body.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    CHECK_EQ(run_cli({"inspect-images", source.string(), "--json"}, output), 0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"entry_name\":\"word/document.xml\""), std::string::npos);
    CHECK_NE(json.find("\"count\":2"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"inline\""), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"floating_options\":null"), std::string::npos);
    CHECK_NE(json.find("\"z_order\":72"), std::string::npos);
    CHECK_NE(json.find("\"wrap_mode\":\"square\""), std::string::npos);
    CHECK_NE(json.find("\"crop\":{\"left_per_mille\":15,\"top_per_mille\":25,"
                       "\"right_per_mille\":35,\"bottom_per_mille\":45}"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-images supports single image text output for header parts") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_images_header_source.docx";
    const fs::path output = working_directory / "cli_images_header.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    CHECK_EQ(run_cli({"inspect-images", source.string(), "--part", "header",
                      "--index", "0", "--image", "0"},
                     output),
             0);

    const auto text = read_text_file(output);
    CHECK_NE(text.find("part: header\n"), std::string::npos);
    CHECK_NE(text.find("part_index: 0\n"), std::string::npos);
    CHECK_NE(text.find("entry_name: word/header1.xml\n"), std::string::npos);
    CHECK_NE(text.find("image_index: 0\n"), std::string::npos);
    CHECK_NE(text.find("placement: anchored\n"), std::string::npos);
    CHECK_NE(text.find("z_order: 84\n"), std::string::npos);
    CHECK_NE(text.find("wrap_mode: square\n"), std::string::npos);
    CHECK_NE(text.find("crop_left_per_mille: 10\n"), std::string::npos);
    CHECK_NE(text.find("crop_bottom_per_mille: 40\n"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-images filters images by relationship id and image entry name") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_images_filter_source.docx";
    const fs::path output = working_directory / "cli_images_filter.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    featherdoc::Document doc(source);
    REQUIRE_FALSE(doc.open());
    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    const auto &anchored_image = images[1];

    CHECK_EQ(run_cli({"inspect-images",
                      source.string(),
                      "--relationship-id",
                      anchored_image.relationship_id,
                      "--image-entry-name",
                      anchored_image.entry_name,
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"filters\":{\"relationship_id\":\"" +
                           anchored_image.relationship_id +
                           "\",\"image_entry_name\":\"" + anchored_image.entry_name +
                           "\"}"),
             std::string::npos);
    CHECK_NE(json.find("\"count\":1"), std::string::npos);
    CHECK_NE(json.find("\"index\":" + std::to_string(anchored_image.index)),
             std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-images reports filtered single image misses") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_images_filter_miss_source.docx";
    const fs::path output = working_directory / "cli_images_filter_miss.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    featherdoc::Document doc(source);
    REQUIRE_FALSE(doc.open());
    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto header_images = header_template.drawing_images();
    REQUIRE_EQ(header_images.size(), 1U);
    const auto entry_name = std::string(header_template.entry_name());

    CHECK_EQ(run_cli({"inspect-images",
                      source.string(),
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--image",
                      std::to_string(header_images[0].index),
                      "--relationship-id",
                      "missing-rid",
                      "--json"},
                     output),
             1);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"inspect-images\""), std::string::npos);
    CHECK_NE(json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(json.find("\"message\":\"result out of range\""), std::string::npos);
    CHECK_NE(json.find("\"detail\":\"drawing image index 0 was not found in " +
                           entry_name + " for relationship_id 'missing-rid'\""),
             std::string::npos);
    CHECK_NE(json.find("\"entry\":\"" + entry_name + "\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-images reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_images_parse_source.docx";
    const fs::path output = working_directory / "cli_images_parse.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    CHECK_EQ(run_cli({"inspect-images", source.string(), "--part",
                      "section-header", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"inspect-images\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "inspection requires --section <index>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli extract-image exports a filtered anchored body image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_extract_image_source.docx";
    const fs::path extracted = working_directory / "cli_extract_image.png";
    const fs::path output = working_directory / "cli_extract_image.json";

    remove_if_exists(source);
    remove_if_exists(extracted);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    featherdoc::Document doc(source);
    REQUIRE_FALSE(doc.open());
    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    const auto &anchored_image = images[1];

    CHECK_EQ(run_cli({"extract-image",
                      source.string(),
                      extracted.string(),
                      "--relationship-id",
                      anchored_image.relationship_id,
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"extract-image\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"output_path\":" + json_quote(extracted.string())),
             std::string::npos);
    CHECK_NE(json.find("\"filters\":{\"relationship_id\":\"" +
                           anchored_image.relationship_id + "\"}"),
             std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"content_type\":\"image/png\""), std::string::npos);
    CHECK_EQ(read_binary_file(extracted), tiny_png_data());

    remove_if_exists(source);
    remove_if_exists(extracted);
    remove_if_exists(output);
}

TEST_CASE("cli extract-image requires an explicit image selector") {
    const fs::path working_directory = fs::current_path();
    const fs::path output = working_directory / "cli_extract_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"extract-image", "missing.docx", "exported.png", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"extract-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"extract-image requires --image "
            "<index>, --relationship-id <id>, or --image-entry-name <path>\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli replace-image replaces a filtered anchored body image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_replace_image_source.docx";
    const fs::path replacement = working_directory / "cli_replace_image_replacement.gif";
    const fs::path updated = working_directory / "cli_replace_image_updated.docx";
    const fs::path output = working_directory / "cli_replace_image.json";

    remove_if_exists(source);
    remove_if_exists(replacement);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_image_fixture(source);
    write_binary_file(replacement, tiny_gif_data());

    featherdoc::Document doc(source);
    REQUIRE_FALSE(doc.open());
    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    const auto &anchored_image = images[1];

    CHECK_EQ(run_cli({"replace-image",
                      source.string(),
                      replacement.string(),
                      "--relationship-id",
                      anchored_image.relationship_id,
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-image\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"replacement_path\":" + json_quote(replacement.string())),
             std::string::npos);
    CHECK_NE(json.find("\"filters\":{\"relationship_id\":\"" +
                           anchored_image.relationship_id + "\"}"),
             std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"content_type\":\"image/gif\""), std::string::npos);

    featherdoc::Document reopened(updated);
    CHECK_FALSE(reopened.open());
    const auto updated_images = reopened.drawing_images();
    REQUIRE_EQ(updated_images.size(), 2U);
    CHECK_EQ(updated_images[1].index, anchored_image.index);
    CHECK_EQ(updated_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_images[1].width_px, anchored_image.width_px);
    CHECK_EQ(updated_images[1].height_px, anchored_image.height_px);
    CHECK_EQ(updated_images[1].content_type, "image/gif");
    CHECK(updated_images[1].entry_name.ends_with(".gif"));
    CHECK_EQ(read_docx_entry(updated, updated_images[1].entry_name.c_str()),
             tiny_gif_data());

    remove_if_exists(source);
    remove_if_exists(replacement);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-image requires an explicit image selector") {
    const fs::path working_directory = fs::current_path();
    const fs::path output = working_directory / "cli_replace_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-image", "missing.docx", "replacement.gif", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"replace-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"replace-image requires --image "
            "<index>, --relationship-id <id>, or --image-entry-name <path>\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli remove-image removes a filtered anchored body image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_remove_image_source.docx";
    const fs::path updated = working_directory / "cli_remove_image_updated.docx";
    const fs::path output = working_directory / "cli_remove_image.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    featherdoc::Document doc(source);
    REQUIRE_FALSE(doc.open());
    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    const auto &anchored_image = images[1];

    CHECK_EQ(run_cli({"remove-image",
                      source.string(),
                      "--relationship-id",
                      anchored_image.relationship_id,
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"remove-image\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"filters\":{\"relationship_id\":\"" +
                           anchored_image.relationship_id + "\"}"),
             std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"content_type\":\"image/png\""), std::string::npos);

    featherdoc::Document reopened(updated);
    CHECK_FALSE(reopened.open());
    auto updated_body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(updated_body_template));
    const auto updated_images = updated_body_template.drawing_images();
    REQUIRE_EQ(updated_images.size(), 1U);
    CHECK_EQ(updated_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(updated_images[0].content_type, "image/png");

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli remove-image requires an explicit image selector") {
    const fs::path working_directory = fs::current_path();
    const fs::path output = working_directory / "cli_remove_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"remove-image", "missing.docx", "--json"}, output), 2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"remove-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"remove-image requires --image "
            "<index>, --relationship-id <id>, or --image-entry-name <path>\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli append-image appends a scaled inline body image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_append_image_source.docx";
    const fs::path image = working_directory / "cli_append_image_fixture_image.png";
    const fs::path updated = working_directory / "cli_append_image_updated.docx";
    const fs::path output = working_directory / "cli_append_image.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_empty_document_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(run_cli({"append-image",
                      source.string(),
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
    CHECK_NE(json.find("\"command\":\"append-image\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"image_path\":" + json_quote(image.string())),
             std::string::npos);
    CHECK_NE(json.find("\"floating\":false"), std::string::npos);
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
    CHECK_FALSE(images[0].floating_options.has_value());

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli append-image materializes a section header floating image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_section_header_source.docx";
    const fs::path image =
        working_directory / "cli_append_section_header_fixture_image.png";
    const fs::path updated =
        working_directory / "cli_append_section_header_updated.docx";
    const fs::path output =
        working_directory / "cli_append_section_header.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_empty_document_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(
        run_cli({"append-image",
                 source.string(),
                 image.string(),
                 "--part",
                 "section-header",
                 "--section",
                 "0",
                 "--floating",
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
                 "96",
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
    CHECK_NE(json.find("\"command\":\"append-image\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"floating\":true"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"horizontal_reference\":\"page\""), std::string::npos);
    CHECK_NE(json.find("\"horizontal_offset_px\":40"), std::string::npos);
    CHECK_NE(json.find("\"vertical_reference\":\"margin\""), std::string::npos);
    CHECK_NE(json.find("\"vertical_offset_px\":12"), std::string::npos);
    CHECK_NE(json.find("\"behind_text\":true"), std::string::npos);
    CHECK_NE(json.find("\"allow_overlap\":false"), std::string::npos);
    CHECK_NE(json.find("\"z_order\":96"), std::string::npos);
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
    CHECK_EQ(images[0].floating_options->z_order, 96U);
    CHECK_EQ(images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::square);
    CHECK_EQ(images[0].floating_options->wrap_distance_left_px, 5U);
    CHECK_EQ(images[0].floating_options->wrap_distance_right_px, 7U);
    REQUIRE(images[0].floating_options->crop.has_value());
    CHECK_EQ(images[0].floating_options->crop->left_per_mille, 10U);
    CHECK_EQ(images[0].floating_options->crop->top_per_mille, 20U);
    CHECK_EQ(images[0].floating_options->crop->right_per_mille, 30U);
    CHECK_EQ(images[0].floating_options->crop->bottom_per_mille, 40U);

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli append-image requires width and height together") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_image_parse_source.docx";
    const fs::path image =
        working_directory / "cli_append_image_parse_fixture_image.png";
    const fs::path output =
        working_directory / "cli_append_image_parse.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(output);

    create_cli_empty_document_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(run_cli({"append-image",
                      source.string(),
                      image.string(),
                      "--width",
                      "32",
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{"{\"command\":\"append-image\",\"ok\":false,"
                         "\"stage\":\"parse\",\"message\":\"append-image "
                         "requires both --width <px> and --height <px> when "
                         "scaling\"}\n"});

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(output);
}

} // namespace
