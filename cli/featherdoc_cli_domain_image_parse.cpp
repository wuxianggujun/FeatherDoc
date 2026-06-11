#include "featherdoc_cli_domain_parse.hpp"

namespace featherdoc_cli {

auto parse_floating_image_horizontal_reference(
    std::string_view text,
    featherdoc::floating_image_horizontal_reference &reference) -> bool {
    if (text == "page") {
        reference = featherdoc::floating_image_horizontal_reference::page;
        return true;
    }
    if (text == "margin") {
        reference = featherdoc::floating_image_horizontal_reference::margin;
        return true;
    }
    if (text == "column") {
        reference = featherdoc::floating_image_horizontal_reference::column;
        return true;
    }
    if (text == "character") {
        reference = featherdoc::floating_image_horizontal_reference::character;
        return true;
    }

    return false;
}

auto parse_floating_image_vertical_reference(
    std::string_view text,
    featherdoc::floating_image_vertical_reference &reference) -> bool {
    if (text == "page") {
        reference = featherdoc::floating_image_vertical_reference::page;
        return true;
    }
    if (text == "margin") {
        reference = featherdoc::floating_image_vertical_reference::margin;
        return true;
    }
    if (text == "paragraph") {
        reference = featherdoc::floating_image_vertical_reference::paragraph;
        return true;
    }
    if (text == "line") {
        reference = featherdoc::floating_image_vertical_reference::line;
        return true;
    }

    return false;
}

auto parse_floating_image_wrap_mode(
    std::string_view text, featherdoc::floating_image_wrap_mode &mode) -> bool {
    if (text == "none") {
        mode = featherdoc::floating_image_wrap_mode::none;
        return true;
    }
    if (text == "square") {
        mode = featherdoc::floating_image_wrap_mode::square;
        return true;
    }
    if (text == "top_bottom" || text == "top-bottom") {
        mode = featherdoc::floating_image_wrap_mode::top_bottom;
        return true;
    }

    return false;
}

} // namespace featherdoc_cli
