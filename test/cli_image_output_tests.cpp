#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_image_output.hpp"

#include <string>
#include <system_error>
#include <vector>

TEST_CASE("cli image entry selectors use canonical OPC PartName identity") {
    featherdoc::drawing_image_info image;
    image.index = 3U;
    image.relationship_id = "rId7";
    image.entry_name = "word/media/%E5%9B%BE%E5%83%8F.PNG";

    featherdoc_cli::inspect_images_options options;
    options.image_entry_name = "/WORD/MEDIA/图像.png";

    CHECK(featherdoc_cli::drawing_image_matches_filters(image, options));

    const auto filtered =
        featherdoc_cli::filter_drawing_images({image}, options);
    REQUIRE_EQ(filtered.size(), 1U);
    CHECK_EQ(filtered[0].index, 3U);

    featherdoc::drawing_image_info selected;
    featherdoc::document_error_info error;
    CHECK(featherdoc_cli::resolve_selected_drawing_image(
        {image}, options, "word/document.xml", selected, error));
    CHECK_EQ(selected.index, 3U);
    CHECK_FALSE(error.code);
}

TEST_CASE("cli image entry selectors reject invalid OPC PartNames") {
    featherdoc::drawing_image_info image;
    image.index = 1U;
    image.entry_name = "word/media/image1.png";

    featherdoc_cli::inspect_images_options options;
    options.image_entry_name = "word//media/image1.png";

    CHECK_FALSE(featherdoc_cli::drawing_image_matches_filters(image, options));
    CHECK(featherdoc_cli::filter_drawing_images({image}, options).empty());

    featherdoc::drawing_image_info selected;
    featherdoc::document_error_info error;
    CHECK_FALSE(featherdoc_cli::resolve_selected_drawing_image(
        {image}, options, "word/document.xml", selected, error));
    CHECK_EQ(error.code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(error.entry_name, "word//media/image1.png");
    CHECK_NE(error.detail.find("valid OPC package PartName"),
             std::string::npos);
}
