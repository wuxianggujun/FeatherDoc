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
