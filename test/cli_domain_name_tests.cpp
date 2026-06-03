#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_domain_names.hpp"

namespace {

auto view(const char *text) -> std::string_view {
    return text;
}

} // namespace

TEST_CASE("cli domain name helpers return documented style and template names") {
    CHECK(view(featherdoc_cli::style_kind_name(
              featherdoc::style_kind::paragraph)) == "paragraph");
    CHECK(view(featherdoc_cli::style_kind_name(
              featherdoc::style_kind::character)) == "character");
    CHECK(view(featherdoc_cli::style_kind_name(featherdoc::style_kind::table)) ==
          "table");
    CHECK(view(featherdoc_cli::style_kind_name(
              featherdoc::style_kind::numbering)) == "numbering");
    CHECK(view(featherdoc_cli::style_kind_name(
              featherdoc::style_kind::unknown)) == "unknown");

    CHECK(view(featherdoc_cli::list_kind_name(featherdoc::list_kind::bullet)) ==
          "bullet");
    CHECK(view(featherdoc_cli::list_kind_name(featherdoc::list_kind::decimal)) ==
          "decimal");

    CHECK(view(featherdoc_cli::bookmark_kind_name(
              featherdoc::bookmark_kind::text)) == "text");
    CHECK(view(featherdoc_cli::bookmark_kind_name(
              featherdoc::bookmark_kind::block)) == "block");
    CHECK(view(featherdoc_cli::bookmark_kind_name(
              featherdoc::bookmark_kind::table_rows)) == "table_rows");
    CHECK(view(featherdoc_cli::bookmark_kind_name(
              featherdoc::bookmark_kind::block_range)) == "block_range");
    CHECK(view(featherdoc_cli::bookmark_kind_name(
              featherdoc::bookmark_kind::malformed)) == "malformed");
    CHECK(view(featherdoc_cli::bookmark_kind_name(
              featherdoc::bookmark_kind::mixed)) == "mixed");

    CHECK(view(featherdoc_cli::template_slot_kind_name(
              featherdoc::template_slot_kind::text)) == "text");
    CHECK(view(featherdoc_cli::template_slot_kind_name(
              featherdoc::template_slot_kind::table_rows)) == "table_rows");
    CHECK(view(featherdoc_cli::template_slot_kind_name(
              featherdoc::template_slot_kind::table)) == "table");
    CHECK(view(featherdoc_cli::template_slot_kind_name(
              featherdoc::template_slot_kind::image)) == "image");
    CHECK(view(featherdoc_cli::template_slot_kind_name(
              featherdoc::template_slot_kind::floating_image)) ==
          "floating_image");
    CHECK(view(featherdoc_cli::template_slot_kind_name(
              featherdoc::template_slot_kind::block)) == "block");
}

TEST_CASE("cli domain name helpers return documented content-control and review names") {
    CHECK(view(featherdoc_cli::content_control_kind_name(
              featherdoc::content_control_kind::block)) == "block");
    CHECK(view(featherdoc_cli::content_control_kind_name(
              featherdoc::content_control_kind::run)) == "run");
    CHECK(view(featherdoc_cli::content_control_kind_name(
              featherdoc::content_control_kind::table_row)) == "table_row");
    CHECK(view(featherdoc_cli::content_control_kind_name(
              featherdoc::content_control_kind::table_cell)) == "table_cell");
    CHECK(view(featherdoc_cli::content_control_kind_name(
              featherdoc::content_control_kind::unknown)) == "unknown");

    CHECK(view(featherdoc_cli::content_control_form_kind_name(
              featherdoc::content_control_form_kind::rich_text)) == "rich_text");
    CHECK(view(featherdoc_cli::content_control_form_kind_name(
              featherdoc::content_control_form_kind::plain_text)) == "plain_text");
    CHECK(view(featherdoc_cli::content_control_form_kind_name(
              featherdoc::content_control_form_kind::picture)) == "picture");
    CHECK(view(featherdoc_cli::content_control_form_kind_name(
              featherdoc::content_control_form_kind::checkbox)) == "checkbox");
    CHECK(view(featherdoc_cli::content_control_form_kind_name(
              featherdoc::content_control_form_kind::drop_down_list)) ==
          "drop_down_list");
    CHECK(view(featherdoc_cli::content_control_form_kind_name(
              featherdoc::content_control_form_kind::combo_box)) == "combo_box");
    CHECK(view(featherdoc_cli::content_control_form_kind_name(
              featherdoc::content_control_form_kind::date)) == "date");
    CHECK(view(featherdoc_cli::content_control_form_kind_name(
              featherdoc::content_control_form_kind::repeating_section)) ==
          "repeating_section");
    CHECK(view(featherdoc_cli::content_control_form_kind_name(
              featherdoc::content_control_form_kind::group)) == "group");
    CHECK(view(featherdoc_cli::content_control_form_kind_name(
              featherdoc::content_control_form_kind::unknown)) == "unknown");

    CHECK(view(featherdoc_cli::review_note_kind_name(
              featherdoc::review_note_kind::footnote)) == "footnote");
    CHECK(view(featherdoc_cli::review_note_kind_name(
              featherdoc::review_note_kind::endnote)) == "endnote");
    CHECK(view(featherdoc_cli::review_note_kind_name(
              featherdoc::review_note_kind::comment)) == "comment");

    CHECK(view(featherdoc_cli::revision_kind_name(
              featherdoc::revision_kind::insertion)) == "insertion");
    CHECK(view(featherdoc_cli::revision_kind_name(
              featherdoc::revision_kind::deletion)) == "deletion");
    CHECK(view(featherdoc_cli::revision_kind_name(
              featherdoc::revision_kind::move_from)) == "move_from");
    CHECK(view(featherdoc_cli::revision_kind_name(
              featherdoc::revision_kind::move_to)) == "move_to");
    CHECK(view(featherdoc_cli::revision_kind_name(
              featherdoc::revision_kind::paragraph_property_change)) ==
          "paragraph_property_change");
    CHECK(view(featherdoc_cli::revision_kind_name(
              featherdoc::revision_kind::run_property_change)) ==
          "run_property_change");
    CHECK(view(featherdoc_cli::revision_kind_name(
              featherdoc::revision_kind::unknown)) == "unknown");
}

TEST_CASE("cli domain name helpers return documented image and page names") {
    CHECK(view(featherdoc_cli::template_slot_source_json_name(
              featherdoc::template_slot_source_kind::bookmark)) == "bookmark");
    CHECK(view(featherdoc_cli::template_slot_source_json_name(
              featherdoc::template_slot_source_kind::content_control_tag)) ==
          "content_control_tag");
    CHECK(view(featherdoc_cli::template_slot_source_json_name(
              featherdoc::template_slot_source_kind::content_control_alias)) ==
          "content_control_alias");
    CHECK(view(featherdoc_cli::template_slot_source_text_name(
              featherdoc::template_slot_source_kind::bookmark)) == "bookmark");
    CHECK(view(featherdoc_cli::template_slot_source_text_name(
              featherdoc::template_slot_source_kind::content_control_tag)) ==
          "content-control-tag");
    CHECK(view(featherdoc_cli::template_slot_source_text_name(
              featherdoc::template_slot_source_kind::content_control_alias)) ==
          "content-control-alias");
    CHECK(featherdoc_cli::template_slot_source_new_json_name(
              featherdoc::template_slot_source_kind::bookmark) ==
          std::string{"new_bookmark"});
    CHECK(featherdoc_cli::template_slot_source_new_json_name(
              featherdoc::template_slot_source_kind::content_control_tag) ==
          std::string{"new_content_control_tag"});

    CHECK(view(featherdoc_cli::drawing_image_placement_name(
              featherdoc::drawing_image_placement::inline_object)) == "inline");
    CHECK(view(featherdoc_cli::drawing_image_placement_name(
              featherdoc::drawing_image_placement::anchored_object)) ==
          "anchored");

    CHECK(view(featherdoc_cli::floating_image_horizontal_reference_name(
              featherdoc::floating_image_horizontal_reference::page)) == "page");
    CHECK(view(featherdoc_cli::floating_image_horizontal_reference_name(
              featherdoc::floating_image_horizontal_reference::margin)) ==
          "margin");
    CHECK(view(featherdoc_cli::floating_image_horizontal_reference_name(
              featherdoc::floating_image_horizontal_reference::column)) ==
          "column");
    CHECK(view(featherdoc_cli::floating_image_horizontal_reference_name(
              featherdoc::floating_image_horizontal_reference::character)) ==
          "character");
    CHECK(view(featherdoc_cli::floating_image_vertical_reference_name(
              featherdoc::floating_image_vertical_reference::page)) == "page");
    CHECK(view(featherdoc_cli::floating_image_vertical_reference_name(
              featherdoc::floating_image_vertical_reference::margin)) ==
          "margin");
    CHECK(view(featherdoc_cli::floating_image_vertical_reference_name(
              featherdoc::floating_image_vertical_reference::paragraph)) ==
          "paragraph");
    CHECK(view(featherdoc_cli::floating_image_vertical_reference_name(
              featherdoc::floating_image_vertical_reference::line)) == "line");
    CHECK(view(featherdoc_cli::floating_image_wrap_mode_name(
              featherdoc::floating_image_wrap_mode::none)) == "none");
    CHECK(view(featherdoc_cli::floating_image_wrap_mode_name(
              featherdoc::floating_image_wrap_mode::square)) == "square");
    CHECK(view(featherdoc_cli::floating_image_wrap_mode_name(
              featherdoc::floating_image_wrap_mode::top_bottom)) == "top_bottom");

    CHECK(featherdoc_cli::page_orientation_name(
              featherdoc::page_orientation::portrait) == "portrait");
    CHECK(featherdoc_cli::page_orientation_name(
              featherdoc::page_orientation::landscape) == "landscape");
}

TEST_CASE("cli domain name helpers return documented style usage and table names") {
    CHECK(view(featherdoc_cli::style_usage_part_kind_name(
              featherdoc::style_usage_part_kind::body)) == "body");
    CHECK(view(featherdoc_cli::style_usage_part_kind_name(
              featherdoc::style_usage_part_kind::header)) == "header");
    CHECK(view(featherdoc_cli::style_usage_part_kind_name(
              featherdoc::style_usage_part_kind::footer)) == "footer");
    CHECK(view(featherdoc_cli::style_usage_hit_kind_name(
              featherdoc::style_usage_hit_kind::paragraph)) == "paragraph");
    CHECK(view(featherdoc_cli::style_usage_hit_kind_name(
              featherdoc::style_usage_hit_kind::run)) == "run");
    CHECK(view(featherdoc_cli::style_usage_hit_kind_name(
              featherdoc::style_usage_hit_kind::table)) == "table");

    CHECK(featherdoc_cli::table_layout_mode_name(
              featherdoc::table_layout_mode::autofit) == "autofit");
    CHECK(featherdoc_cli::table_layout_mode_name(
              featherdoc::table_layout_mode::fixed) == "fixed");
    CHECK(featherdoc_cli::table_alignment_name(
              featherdoc::table_alignment::left) == "left");
    CHECK(featherdoc_cli::table_alignment_name(
              featherdoc::table_alignment::center) == "center");
    CHECK(featherdoc_cli::table_alignment_name(
              featherdoc::table_alignment::right) == "right");
    CHECK(featherdoc_cli::cell_margin_edge_name(
              featherdoc::cell_margin_edge::top) == "top");
    CHECK(featherdoc_cli::cell_margin_edge_name(
              featherdoc::cell_margin_edge::left) == "left");
    CHECK(featherdoc_cli::cell_margin_edge_name(
              featherdoc::cell_margin_edge::bottom) == "bottom");
    CHECK(featherdoc_cli::cell_margin_edge_name(
              featherdoc::cell_margin_edge::right) == "right");
    CHECK(featherdoc_cli::cell_border_edge_name(
              featherdoc::cell_border_edge::top) == "top");
    CHECK(featherdoc_cli::cell_border_edge_name(
              featherdoc::cell_border_edge::left) == "left");
    CHECK(featherdoc_cli::cell_border_edge_name(
              featherdoc::cell_border_edge::bottom) == "bottom");
    CHECK(featherdoc_cli::cell_border_edge_name(
              featherdoc::cell_border_edge::right) == "right");
    CHECK(featherdoc_cli::table_border_edge_name(
              featherdoc::table_border_edge::inside_horizontal) ==
          "inside_horizontal");
    CHECK(featherdoc_cli::table_border_edge_name(
              featherdoc::table_border_edge::inside_vertical) ==
          "inside_vertical");
    CHECK(featherdoc_cli::border_style_name(featherdoc::border_style::none) ==
          "none");
    CHECK(featherdoc_cli::border_style_name(featherdoc::border_style::single) ==
          "single");
    CHECK(featherdoc_cli::border_style_name(
              featherdoc::border_style::double_line) == "double_line");
    CHECK(featherdoc_cli::border_style_name(featherdoc::border_style::dashed) ==
          "dashed");
    CHECK(featherdoc_cli::border_style_name(featherdoc::border_style::dotted) ==
          "dotted");
    CHECK(featherdoc_cli::border_style_name(featherdoc::border_style::thick) ==
          "thick");
    CHECK(featherdoc_cli::row_height_rule_name(
              featherdoc::row_height_rule::automatic) == "automatic");
    CHECK(featherdoc_cli::row_height_rule_name(
              featherdoc::row_height_rule::at_least) == "at_least");
    CHECK(featherdoc_cli::row_height_rule_name(
              featherdoc::row_height_rule::exact) == "exact");
    CHECK(featherdoc_cli::cell_text_direction_name(
              featherdoc::cell_text_direction::left_to_right_top_to_bottom) ==
          "left_to_right_top_to_bottom");
    CHECK(featherdoc_cli::cell_text_direction_name(
              featherdoc::cell_text_direction::top_to_bottom_left_to_right_rotated) ==
          "top_to_bottom_left_to_right_rotated");
    CHECK(featherdoc_cli::table_position_horizontal_reference_name(
              featherdoc::table_position_horizontal_reference::margin) ==
          "margin");
    CHECK(featherdoc_cli::table_position_horizontal_reference_name(
              featherdoc::table_position_horizontal_reference::page) == "page");
    CHECK(featherdoc_cli::table_position_horizontal_reference_name(
              featherdoc::table_position_horizontal_reference::column) ==
          "column");
    CHECK(featherdoc_cli::table_position_vertical_reference_name(
              featherdoc::table_position_vertical_reference::margin) ==
          "margin");
    CHECK(featherdoc_cli::table_position_vertical_reference_name(
              featherdoc::table_position_vertical_reference::page) == "page");
    CHECK(featherdoc_cli::table_position_vertical_reference_name(
              featherdoc::table_position_vertical_reference::paragraph) ==
          "paragraph");
    CHECK(featherdoc_cli::table_position_horizontal_spec_name(
              featherdoc::table_position_horizontal_spec::left) == "left");
    CHECK(featherdoc_cli::table_position_horizontal_spec_name(
              featherdoc::table_position_horizontal_spec::center) == "center");
    CHECK(featherdoc_cli::table_position_horizontal_spec_name(
              featherdoc::table_position_horizontal_spec::right) == "right");
    CHECK(featherdoc_cli::table_position_horizontal_spec_name(
              featherdoc::table_position_horizontal_spec::inside) == "inside");
    CHECK(featherdoc_cli::table_position_horizontal_spec_name(
              featherdoc::table_position_horizontal_spec::outside) == "outside");
    CHECK(featherdoc_cli::table_position_vertical_spec_name(
              featherdoc::table_position_vertical_spec::top) == "top");
    CHECK(featherdoc_cli::table_position_vertical_spec_name(
              featherdoc::table_position_vertical_spec::center) == "center");
    CHECK(featherdoc_cli::table_position_vertical_spec_name(
              featherdoc::table_position_vertical_spec::bottom) == "bottom");
    CHECK(featherdoc_cli::table_position_vertical_spec_name(
              featherdoc::table_position_vertical_spec::inside) == "inside");
    CHECK(featherdoc_cli::table_position_vertical_spec_name(
              featherdoc::table_position_vertical_spec::outside) == "outside");
    CHECK(featherdoc_cli::table_overlap_name(
              featherdoc::table_overlap::allow) == "allow");
    CHECK(featherdoc_cli::table_overlap_name(
              featherdoc::table_overlap::never) == "never");
}
