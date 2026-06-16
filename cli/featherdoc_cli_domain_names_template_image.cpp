#include "featherdoc_cli_domain_names.hpp"

#include <string>

namespace featherdoc_cli {

auto template_slot_kind_name(featherdoc::template_slot_kind kind)
    -> const char * {
    switch (kind) {
    case featherdoc::template_slot_kind::text:
        return "text";
    case featherdoc::template_slot_kind::table_rows:
        return "table_rows";
    case featherdoc::template_slot_kind::table:
        return "table";
    case featherdoc::template_slot_kind::image:
        return "image";
    case featherdoc::template_slot_kind::floating_image:
        return "floating_image";
    case featherdoc::template_slot_kind::block:
        return "block";
    }

    return "text";
}

auto template_slot_source_json_name(
    featherdoc::template_slot_source_kind source) -> const char * {
    switch (source) {
    case featherdoc::template_slot_source_kind::bookmark:
        return "bookmark";
    case featherdoc::template_slot_source_kind::content_control_tag:
        return "content_control_tag";
    case featherdoc::template_slot_source_kind::content_control_alias:
        return "content_control_alias";
    }

    return "bookmark";
}

auto template_slot_source_text_name(
    featherdoc::template_slot_source_kind source) -> const char * {
    switch (source) {
    case featherdoc::template_slot_source_kind::bookmark:
        return "bookmark";
    case featherdoc::template_slot_source_kind::content_control_tag:
        return "content-control-tag";
    case featherdoc::template_slot_source_kind::content_control_alias:
        return "content-control-alias";
    }

    return "bookmark";
}

auto template_slot_source_new_json_name(
    featherdoc::template_slot_source_kind source) -> std::string {
    if (source == featherdoc::template_slot_source_kind::bookmark) {
        return "new_bookmark";
    }

    auto name = std::string{"new_"};
    name += template_slot_source_json_name(source);
    return name;
}

auto drawing_image_placement_name(
    featherdoc::drawing_image_placement placement) -> const char * {
    switch (placement) {
    case featherdoc::drawing_image_placement::inline_object:
        return "inline";
    case featherdoc::drawing_image_placement::anchored_object:
        return "anchored";
    }

    return "inline";
}

auto floating_image_horizontal_reference_name(
    featherdoc::floating_image_horizontal_reference reference) -> const char * {
    switch (reference) {
    case featherdoc::floating_image_horizontal_reference::page:
        return "page";
    case featherdoc::floating_image_horizontal_reference::margin:
        return "margin";
    case featherdoc::floating_image_horizontal_reference::column:
        return "column";
    case featherdoc::floating_image_horizontal_reference::character:
        return "character";
    }

    return "column";
}

auto floating_image_vertical_reference_name(
    featherdoc::floating_image_vertical_reference reference) -> const char * {
    switch (reference) {
    case featherdoc::floating_image_vertical_reference::page:
        return "page";
    case featherdoc::floating_image_vertical_reference::margin:
        return "margin";
    case featherdoc::floating_image_vertical_reference::paragraph:
        return "paragraph";
    case featherdoc::floating_image_vertical_reference::line:
        return "line";
    }

    return "paragraph";
}

auto floating_image_wrap_mode_name(featherdoc::floating_image_wrap_mode mode)
    -> const char * {
    switch (mode) {
    case featherdoc::floating_image_wrap_mode::none:
        return "none";
    case featherdoc::floating_image_wrap_mode::square:
        return "square";
    case featherdoc::floating_image_wrap_mode::top_bottom:
        return "top_bottom";
    }

    return "none";
}

} // namespace featherdoc_cli
