#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_bookmark_text_options_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("bookmark text options parse accepts inspection and simple mutations") {
    std::string error;

    featherdoc_cli::inspect_bookmarks_options inspect_bookmarks;
    CHECK(featherdoc_cli::parse_inspect_bookmarks_options(
        {"inspect-bookmarks", "input.docx", "--bookmark", "bookmark1",
         "--json"},
        2U, inspect_bookmarks, error));
    REQUIRE(inspect_bookmarks.bookmark_name.has_value());
    CHECK(*inspect_bookmarks.bookmark_name == "bookmark1");
    CHECK(inspect_bookmarks.json_output);

    featherdoc_cli::inspect_content_controls_options inspect_content_controls;
    error.clear();
    CHECK(featherdoc_cli::parse_inspect_content_controls_options(
        {"inspect-content-controls", "input.docx", "--tag", "tag1",
         "--json"},
        2U, inspect_content_controls, error));
    REQUIRE(inspect_content_controls.tag.has_value());
    CHECK(*inspect_content_controls.tag == "tag1");
    CHECK(inspect_content_controls.json_output);

    featherdoc_cli::replace_content_control_text_options replace_text;
    error.clear();
    CHECK(featherdoc_cli::parse_replace_content_control_text_options(
        {"replace-content-control-text", "input.docx", "--tag", "tag1",
         "--text", "hello", "--output", "out.docx", "--json"},
        2U, replace_text, error));
    CHECK(replace_text.text == "hello");
    REQUIRE(replace_text.output_path.has_value());
    CHECK(replace_text.output_path->filename().string() == "out.docx");
    CHECK(replace_text.json_output);

    featherdoc_cli::set_content_control_form_state_options form_state;
    error.clear();
    CHECK(featherdoc_cli::parse_set_content_control_form_state_options(
        {"set-content-control-form-state", "input.docx", "--tag", "tag1",
         "--lock", "locked", "--checked", "true", "--json"},
        2U, form_state, error));
    REQUIRE(form_state.tag.has_value());
    REQUIRE(form_state.state.lock.has_value());
    REQUIRE(form_state.state.checked.has_value());
    CHECK(*form_state.state.lock == "locked");
    CHECK(*form_state.state.checked);

    featherdoc_cli::sync_content_controls_from_custom_xml_options sync;
    error.clear();
    CHECK(featherdoc_cli::parse_sync_content_controls_from_custom_xml_options(
        {"sync-content-controls-from-custom-xml", "input.docx", "--output",
         "out.docx", "--json"},
        2U, sync, error));
    REQUIRE(sync.output_path.has_value());
    CHECK(sync.output_path->filename().string() == "out.docx");
    CHECK(sync.json_output);
}

TEST_CASE("bookmark text options parse accepts paragraph, table, image, and visibility mutations") {
    std::string error;

    featherdoc_cli::replace_bookmark_paragraphs_options paragraphs;
    CHECK(featherdoc_cli::parse_replace_bookmark_paragraphs_options(
        {"replace-bookmark-paragraphs", "input.docx", "--paragraph", "One",
         "--paragraph", "Two", "--output", "out.docx", "--json"},
        2U, paragraphs, error));
    REQUIRE(paragraphs.paragraph_sources.size() == 2U);
    REQUIRE(paragraphs.output_path.has_value());
    CHECK(paragraphs.output_path->filename().string() == "out.docx");

    featherdoc_cli::bookmark_table_replacement_options table;
    error.clear();
    CHECK(featherdoc_cli::parse_bookmark_table_replacement_options(
        {"replace-bookmark-table", "input.docx", "--row", "A", "--cell",
         "B", "--output", "out.docx", "--json"},
        2U, table, false, error));
    REQUIRE(table.row_sources.size() == 1U);
    REQUIRE(table.row_sources.front().size() == 2U);
    REQUIRE(table.output_path.has_value());
    CHECK(table.output_path->filename().string() == "out.docx");

    featherdoc_cli::replace_content_control_image_options image;
    error.clear();
    CHECK(featherdoc_cli::parse_replace_content_control_image_options(
        {"replace-content-control-image", "input.docx", "image.png", "--tag",
         "tag1", "--width", "20", "--height", "10", "--output", "out.docx",
         "--json"},
        3U, image, error));
    REQUIRE(image.output_path.has_value());
    REQUIRE(image.width_px.has_value());
    REQUIRE(image.height_px.has_value());
    CHECK(*image.width_px == 20U);
    CHECK(*image.height_px == 10U);
    CHECK(image.output_path->filename().string() == "out.docx");

    featherdoc_cli::fill_bookmarks_options fill;
    error.clear();
    CHECK(featherdoc_cli::parse_fill_bookmarks_options(
        {"fill-bookmarks", "input.docx", "--set", "Bookmark1", "Hello",
         "--output", "out.docx", "--json"},
        2U, fill, error));
    REQUIRE(fill.binding_sources.size() == 1U);
    CHECK(fill.binding_sources.front().bookmark_name == "Bookmark1");
    REQUIRE(fill.binding_sources.front().source.text.has_value());
    CHECK(*fill.binding_sources.front().source.text == "Hello");

    featherdoc_cli::set_bookmark_block_visibility_options visibility;
    error.clear();
    CHECK(featherdoc_cli::parse_set_bookmark_block_visibility_options(
        {"set-bookmark-block-visibility", "input.docx", "Bookmark1", "--visible",
         "true", "--output", "out.docx", "--json"},
        3U, visibility, error));
    CHECK(visibility.has_visible);
    CHECK(visibility.visible);

    featherdoc_cli::apply_bookmark_block_visibility_options apply;
    error.clear();
    CHECK(featherdoc_cli::parse_apply_bookmark_block_visibility_options(
        {"apply-bookmark-block-visibility", "input.docx", "--show", "Bookmark1",
         "--hide", "Bookmark2", "--output", "out.docx", "--json"},
        2U, apply, error));
    REQUIRE(apply.bindings.size() == 2U);
    CHECK(apply.bindings.front().visible);
    CHECK_FALSE(apply.bindings.back().visible);
}

TEST_CASE("bookmark text options parse validates required values and floating guard") {
    std::string error;

    featherdoc_cli::replace_content_control_text_options replace_text;
    CHECK_FALSE(featherdoc_cli::parse_replace_content_control_text_options(
        {"replace-content-control-text", "input.docx", "--tag", "tag1",
         "--json"},
        2U, replace_text, error));
    CHECK(error == "expected --text <text>");

    featherdoc_cli::fill_bookmarks_options fill;
    error.clear();
    CHECK_FALSE(featherdoc_cli::parse_fill_bookmarks_options(
        {"fill-bookmarks", "input.docx", "--json"}, 2U, fill, error));
    CHECK(error ==
          "expected at least one --set <bookmark-name> <text> or --set-file <bookmark-name> <path>");

    featherdoc_cli::append_image_options image;
    error.clear();
    CHECK_FALSE(featherdoc_cli::parse_bookmark_image_command_options(
        {"--floating"}, 0U, "replace-content-control-image", false, image,
        error));
    CHECK(error ==
          "replace-content-control-image does not accept floating layout options; use replace-bookmark-floating-image");
}
