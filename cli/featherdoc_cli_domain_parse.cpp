#include "featherdoc_cli_domain_parse.hpp"

namespace featherdoc_cli {

auto parse_page_orientation(std::string_view text,
                            featherdoc::page_orientation &orientation) -> bool {
    if (text == "portrait") {
        orientation = featherdoc::page_orientation::portrait;
        return true;
    }
    if (text == "landscape") {
        orientation = featherdoc::page_orientation::landscape;
        return true;
    }

    return false;
}

auto parse_list_kind(std::string_view text, featherdoc::list_kind &kind)
    -> bool {
    if (text == "bullet") {
        kind = featherdoc::list_kind::bullet;
        return true;
    }
    if (text == "decimal") {
        kind = featherdoc::list_kind::decimal;
        return true;
    }

    return false;
}

auto parse_reference_kind(std::string_view text,
                          featherdoc::section_reference_kind &reference_kind)
    -> bool {
    if (text == "default") {
        reference_kind = featherdoc::section_reference_kind::default_reference;
        return true;
    }
    if (text == "first") {
        reference_kind = featherdoc::section_reference_kind::first_page;
        return true;
    }
    if (text == "even") {
        reference_kind = featherdoc::section_reference_kind::even_page;
        return true;
    }

    return false;
}

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
