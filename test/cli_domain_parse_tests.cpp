#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_domain_parse.hpp"

TEST_CASE("cli domain parsers accept documented enum tokens") {
    featherdoc::page_orientation orientation{};
    CHECK(featherdoc_cli::parse_page_orientation("portrait", orientation));
    CHECK(orientation == featherdoc::page_orientation::portrait);
    CHECK(featherdoc_cli::parse_page_orientation("landscape", orientation));
    CHECK(orientation == featherdoc::page_orientation::landscape);

    featherdoc::list_kind list_kind{};
    CHECK(featherdoc_cli::parse_list_kind("bullet", list_kind));
    CHECK(list_kind == featherdoc::list_kind::bullet);
    CHECK(featherdoc_cli::parse_list_kind("decimal", list_kind));
    CHECK(list_kind == featherdoc::list_kind::decimal);

    featherdoc::section_reference_kind reference_kind{};
    CHECK(featherdoc_cli::parse_reference_kind("default", reference_kind));
    CHECK(reference_kind ==
          featherdoc::section_reference_kind::default_reference);
    CHECK(featherdoc_cli::parse_reference_kind("first", reference_kind));
    CHECK(reference_kind == featherdoc::section_reference_kind::first_page);
    CHECK(featherdoc_cli::parse_reference_kind("even", reference_kind));
    CHECK(reference_kind == featherdoc::section_reference_kind::even_page);

    featherdoc::floating_image_horizontal_reference horizontal_reference{};
    CHECK(featherdoc_cli::parse_floating_image_horizontal_reference(
        "page", horizontal_reference));
    CHECK(horizontal_reference ==
          featherdoc::floating_image_horizontal_reference::page);
    CHECK(featherdoc_cli::parse_floating_image_horizontal_reference(
        "margin", horizontal_reference));
    CHECK(horizontal_reference ==
          featherdoc::floating_image_horizontal_reference::margin);
    CHECK(featherdoc_cli::parse_floating_image_horizontal_reference(
        "column", horizontal_reference));
    CHECK(horizontal_reference ==
          featherdoc::floating_image_horizontal_reference::column);
    CHECK(featherdoc_cli::parse_floating_image_horizontal_reference(
        "character", horizontal_reference));
    CHECK(horizontal_reference ==
          featherdoc::floating_image_horizontal_reference::character);

    featherdoc::floating_image_vertical_reference vertical_reference{};
    CHECK(featherdoc_cli::parse_floating_image_vertical_reference(
        "page", vertical_reference));
    CHECK(vertical_reference ==
          featherdoc::floating_image_vertical_reference::page);
    CHECK(featherdoc_cli::parse_floating_image_vertical_reference(
        "margin", vertical_reference));
    CHECK(vertical_reference ==
          featherdoc::floating_image_vertical_reference::margin);
    CHECK(featherdoc_cli::parse_floating_image_vertical_reference(
        "paragraph", vertical_reference));
    CHECK(vertical_reference ==
          featherdoc::floating_image_vertical_reference::paragraph);
    CHECK(featherdoc_cli::parse_floating_image_vertical_reference(
        "line", vertical_reference));
    CHECK(vertical_reference ==
          featherdoc::floating_image_vertical_reference::line);

    featherdoc::floating_image_wrap_mode wrap_mode{};
    CHECK(featherdoc_cli::parse_floating_image_wrap_mode("none", wrap_mode));
    CHECK(wrap_mode == featherdoc::floating_image_wrap_mode::none);
    CHECK(featherdoc_cli::parse_floating_image_wrap_mode("square", wrap_mode));
    CHECK(wrap_mode == featherdoc::floating_image_wrap_mode::square);
    CHECK(featherdoc_cli::parse_floating_image_wrap_mode("top_bottom",
                                                        wrap_mode));
    CHECK(wrap_mode == featherdoc::floating_image_wrap_mode::top_bottom);
    CHECK(featherdoc_cli::parse_floating_image_wrap_mode("top-bottom",
                                                        wrap_mode));
    CHECK(wrap_mode == featherdoc::floating_image_wrap_mode::top_bottom);
}

TEST_CASE("cli table style parsers accept documented enum tokens") {
    featherdoc::cell_margin_edge margin_edge{};
    CHECK(featherdoc_cli::parse_table_style_margin_edge_text("top",
                                                             margin_edge));
    CHECK(margin_edge == featherdoc::cell_margin_edge::top);
    CHECK(featherdoc_cli::parse_table_style_margin_edge_text("left",
                                                             margin_edge));
    CHECK(margin_edge == featherdoc::cell_margin_edge::left);
    CHECK(featherdoc_cli::parse_table_style_margin_edge_text("bottom",
                                                             margin_edge));
    CHECK(margin_edge == featherdoc::cell_margin_edge::bottom);
    CHECK(featherdoc_cli::parse_table_style_margin_edge_text("right",
                                                             margin_edge));
    CHECK(margin_edge == featherdoc::cell_margin_edge::right);

    featherdoc::table_border_edge border_edge{};
    CHECK(featherdoc_cli::parse_table_style_border_edge_text("top",
                                                             border_edge));
    CHECK(border_edge == featherdoc::table_border_edge::top);
    CHECK(featherdoc_cli::parse_table_style_border_edge_text("left",
                                                             border_edge));
    CHECK(border_edge == featherdoc::table_border_edge::left);
    CHECK(featherdoc_cli::parse_table_style_border_edge_text("bottom",
                                                             border_edge));
    CHECK(border_edge == featherdoc::table_border_edge::bottom);
    CHECK(featherdoc_cli::parse_table_style_border_edge_text("right",
                                                             border_edge));
    CHECK(border_edge == featherdoc::table_border_edge::right);
    CHECK(featherdoc_cli::parse_table_style_border_edge_text(
        "inside_horizontal", border_edge));
    CHECK(border_edge == featherdoc::table_border_edge::inside_horizontal);
    CHECK(featherdoc_cli::parse_table_style_border_edge_text(
        "inside-horizontal", border_edge));
    CHECK(border_edge == featherdoc::table_border_edge::inside_horizontal);
    CHECK(featherdoc_cli::parse_table_style_border_edge_text(
        "inside_vertical", border_edge));
    CHECK(border_edge == featherdoc::table_border_edge::inside_vertical);
    CHECK(featherdoc_cli::parse_table_style_border_edge_text(
        "inside-vertical", border_edge));
    CHECK(border_edge == featherdoc::table_border_edge::inside_vertical);

    featherdoc::border_style border_style{};
    CHECK(featherdoc_cli::parse_table_style_border_style_text("none",
                                                              border_style));
    CHECK(border_style == featherdoc::border_style::none);
    CHECK(featherdoc_cli::parse_table_style_border_style_text("single",
                                                              border_style));
    CHECK(border_style == featherdoc::border_style::single);
    CHECK(featherdoc_cli::parse_table_style_border_style_text("double_line",
                                                              border_style));
    CHECK(border_style == featherdoc::border_style::double_line);
    CHECK(featherdoc_cli::parse_table_style_border_style_text("double-line",
                                                              border_style));
    CHECK(border_style == featherdoc::border_style::double_line);
    CHECK(featherdoc_cli::parse_table_style_border_style_text("dashed",
                                                              border_style));
    CHECK(border_style == featherdoc::border_style::dashed);
    CHECK(featherdoc_cli::parse_table_style_border_style_text("dotted",
                                                              border_style));
    CHECK(border_style == featherdoc::border_style::dotted);
    CHECK(featherdoc_cli::parse_table_style_border_style_text("thick",
                                                              border_style));
    CHECK(border_style == featherdoc::border_style::thick);

    featherdoc::cell_vertical_alignment vertical_alignment{};
    CHECK(featherdoc_cli::parse_table_style_cell_vertical_alignment_text(
        "top", vertical_alignment));
    CHECK(vertical_alignment == featherdoc::cell_vertical_alignment::top);
    CHECK(featherdoc_cli::parse_table_style_cell_vertical_alignment_text(
        "center", vertical_alignment));
    CHECK(vertical_alignment == featherdoc::cell_vertical_alignment::center);
    CHECK(featherdoc_cli::parse_table_style_cell_vertical_alignment_text(
        "bottom", vertical_alignment));
    CHECK(vertical_alignment == featherdoc::cell_vertical_alignment::bottom);
    CHECK(featherdoc_cli::parse_table_style_cell_vertical_alignment_text(
        "both", vertical_alignment));
    CHECK(vertical_alignment == featherdoc::cell_vertical_alignment::both);

    featherdoc::cell_text_direction text_direction{};
    CHECK(featherdoc_cli::parse_table_style_cell_text_direction_text(
        "left_to_right_top_to_bottom", text_direction));
    CHECK(text_direction ==
          featherdoc::cell_text_direction::left_to_right_top_to_bottom);
    CHECK(featherdoc_cli::parse_table_style_cell_text_direction_text(
        "top_to_bottom_right_to_left", text_direction));
    CHECK(text_direction ==
          featherdoc::cell_text_direction::top_to_bottom_right_to_left);
    CHECK(featherdoc_cli::parse_table_style_cell_text_direction_text(
        "bottom_to_top_left_to_right", text_direction));
    CHECK(text_direction ==
          featherdoc::cell_text_direction::bottom_to_top_left_to_right);
    CHECK(featherdoc_cli::parse_table_style_cell_text_direction_text(
        "left_to_right_top_to_bottom_rotated", text_direction));
    CHECK(text_direction ==
          featherdoc::cell_text_direction::
              left_to_right_top_to_bottom_rotated);
    CHECK(featherdoc_cli::parse_table_style_cell_text_direction_text(
        "top_to_bottom_right_to_left_rotated", text_direction));
    CHECK(text_direction ==
          featherdoc::cell_text_direction::
              top_to_bottom_right_to_left_rotated);
    CHECK(featherdoc_cli::parse_table_style_cell_text_direction_text(
        "top_to_bottom_left_to_right_rotated", text_direction));
    CHECK(text_direction ==
          featherdoc::cell_text_direction::
              top_to_bottom_left_to_right_rotated);

    featherdoc::paragraph_alignment paragraph_alignment{};
    CHECK(featherdoc_cli::parse_table_style_paragraph_alignment_text(
        "left", paragraph_alignment));
    CHECK(paragraph_alignment == featherdoc::paragraph_alignment::left);
    CHECK(featherdoc_cli::parse_table_style_paragraph_alignment_text(
        "center", paragraph_alignment));
    CHECK(paragraph_alignment == featherdoc::paragraph_alignment::center);
    CHECK(featherdoc_cli::parse_table_style_paragraph_alignment_text(
        "right", paragraph_alignment));
    CHECK(paragraph_alignment == featherdoc::paragraph_alignment::right);
    CHECK(featherdoc_cli::parse_table_style_paragraph_alignment_text(
        "justified", paragraph_alignment));
    CHECK(paragraph_alignment == featherdoc::paragraph_alignment::justified);
    CHECK(featherdoc_cli::parse_table_style_paragraph_alignment_text(
        "both", paragraph_alignment));
    CHECK(paragraph_alignment == featherdoc::paragraph_alignment::justified);
    CHECK(featherdoc_cli::parse_table_style_paragraph_alignment_text(
        "distribute", paragraph_alignment));
    CHECK(paragraph_alignment == featherdoc::paragraph_alignment::distribute);

    featherdoc::paragraph_line_spacing_rule line_rule{};
    CHECK(featherdoc_cli::parse_table_style_paragraph_line_spacing_rule_text(
        "auto", line_rule));
    CHECK(line_rule == featherdoc::paragraph_line_spacing_rule::automatic);
    CHECK(featherdoc_cli::parse_table_style_paragraph_line_spacing_rule_text(
        "automatic", line_rule));
    CHECK(line_rule == featherdoc::paragraph_line_spacing_rule::automatic);
    CHECK(featherdoc_cli::parse_table_style_paragraph_line_spacing_rule_text(
        "at_least", line_rule));
    CHECK(line_rule == featherdoc::paragraph_line_spacing_rule::at_least);
    CHECK(featherdoc_cli::parse_table_style_paragraph_line_spacing_rule_text(
        "at-least", line_rule));
    CHECK(line_rule == featherdoc::paragraph_line_spacing_rule::at_least);
    CHECK(featherdoc_cli::parse_table_style_paragraph_line_spacing_rule_text(
        "exact", line_rule));
    CHECK(line_rule == featherdoc::paragraph_line_spacing_rule::exact);
}

TEST_CASE("cli domain parsers reject unknown enum tokens") {
    featherdoc::page_orientation orientation =
        featherdoc::page_orientation::portrait;
    CHECK_FALSE(featherdoc_cli::parse_page_orientation("wide", orientation));
    CHECK(orientation == featherdoc::page_orientation::portrait);

    featherdoc::list_kind list_kind = featherdoc::list_kind::bullet;
    CHECK_FALSE(featherdoc_cli::parse_list_kind("numbered", list_kind));
    CHECK(list_kind == featherdoc::list_kind::bullet);

    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    CHECK_FALSE(featherdoc_cli::parse_reference_kind("odd", reference_kind));
    CHECK(reference_kind ==
          featherdoc::section_reference_kind::default_reference);

    featherdoc::floating_image_horizontal_reference horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    CHECK_FALSE(featherdoc_cli::parse_floating_image_horizontal_reference(
        "paragraph", horizontal_reference));
    CHECK(horizontal_reference ==
          featherdoc::floating_image_horizontal_reference::page);

    featherdoc::floating_image_vertical_reference vertical_reference =
        featherdoc::floating_image_vertical_reference::page;
    CHECK_FALSE(featherdoc_cli::parse_floating_image_vertical_reference(
        "column", vertical_reference));
    CHECK(vertical_reference ==
          featherdoc::floating_image_vertical_reference::page);

    featherdoc::floating_image_wrap_mode wrap_mode =
        featherdoc::floating_image_wrap_mode::none;
    CHECK_FALSE(
        featherdoc_cli::parse_floating_image_wrap_mode("tight", wrap_mode));
    CHECK(wrap_mode == featherdoc::floating_image_wrap_mode::none);
}

TEST_CASE("cli table style parsers reject unknown enum tokens") {
    featherdoc::cell_margin_edge margin_edge =
        featherdoc::cell_margin_edge::left;
    CHECK_FALSE(featherdoc_cli::parse_table_style_margin_edge_text(
        "inside", margin_edge));
    CHECK(margin_edge == featherdoc::cell_margin_edge::left);

    featherdoc::table_border_edge border_edge =
        featherdoc::table_border_edge::top;
    CHECK_FALSE(featherdoc_cli::parse_table_style_border_edge_text(
        "diagonal", border_edge));
    CHECK(border_edge == featherdoc::table_border_edge::top);

    featherdoc::border_style border_style = featherdoc::border_style::single;
    CHECK_FALSE(featherdoc_cli::parse_table_style_border_style_text(
        "dash_dot", border_style));
    CHECK(border_style == featherdoc::border_style::single);

    featherdoc::cell_vertical_alignment vertical_alignment =
        featherdoc::cell_vertical_alignment::center;
    CHECK_FALSE(
        featherdoc_cli::parse_table_style_cell_vertical_alignment_text(
            "middle", vertical_alignment));
    CHECK(vertical_alignment == featherdoc::cell_vertical_alignment::center);

    featherdoc::cell_text_direction text_direction =
        featherdoc::cell_text_direction::left_to_right_top_to_bottom;
    CHECK_FALSE(featherdoc_cli::parse_table_style_cell_text_direction_text(
        "sideways", text_direction));
    CHECK(text_direction ==
          featherdoc::cell_text_direction::left_to_right_top_to_bottom);

    featherdoc::paragraph_alignment paragraph_alignment =
        featherdoc::paragraph_alignment::center;
    CHECK_FALSE(featherdoc_cli::parse_table_style_paragraph_alignment_text(
        "middle", paragraph_alignment));
    CHECK(paragraph_alignment == featherdoc::paragraph_alignment::center);

    featherdoc::paragraph_line_spacing_rule line_rule =
        featherdoc::paragraph_line_spacing_rule::exact;
    CHECK_FALSE(
        featherdoc_cli::parse_table_style_paragraph_line_spacing_rule_text(
            "minimum", line_rule));
    CHECK(line_rule == featherdoc::paragraph_line_spacing_rule::exact);
}
