#include "featherdoc_cli_domain_names.hpp"

namespace featherdoc_cli {

auto style_kind_name(featherdoc::style_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::style_kind::paragraph:
        return "paragraph";
    case featherdoc::style_kind::character:
        return "character";
    case featherdoc::style_kind::table:
        return "table";
    case featherdoc::style_kind::numbering:
        return "numbering";
    case featherdoc::style_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto list_kind_name(featherdoc::list_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::list_kind::bullet:
        return "bullet";
    case featherdoc::list_kind::decimal:
        return "decimal";
    }

    return "bullet";
}

auto bookmark_kind_name(featherdoc::bookmark_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::bookmark_kind::text:
        return "text";
    case featherdoc::bookmark_kind::block:
        return "block";
    case featherdoc::bookmark_kind::table_rows:
        return "table_rows";
    case featherdoc::bookmark_kind::block_range:
        return "block_range";
    case featherdoc::bookmark_kind::malformed:
        return "malformed";
    case featherdoc::bookmark_kind::mixed:
        return "mixed";
    }

    return "mixed";
}

auto content_control_kind_name(featherdoc::content_control_kind kind)
    -> const char * {
    switch (kind) {
    case featherdoc::content_control_kind::block:
        return "block";
    case featherdoc::content_control_kind::run:
        return "run";
    case featherdoc::content_control_kind::table_row:
        return "table_row";
    case featherdoc::content_control_kind::table_cell:
        return "table_cell";
    case featherdoc::content_control_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto content_control_form_kind_name(
    featherdoc::content_control_form_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::content_control_form_kind::rich_text:
        return "rich_text";
    case featherdoc::content_control_form_kind::plain_text:
        return "plain_text";
    case featherdoc::content_control_form_kind::picture:
        return "picture";
    case featherdoc::content_control_form_kind::checkbox:
        return "checkbox";
    case featherdoc::content_control_form_kind::drop_down_list:
        return "drop_down_list";
    case featherdoc::content_control_form_kind::combo_box:
        return "combo_box";
    case featherdoc::content_control_form_kind::date:
        return "date";
    case featherdoc::content_control_form_kind::repeating_section:
        return "repeating_section";
    case featherdoc::content_control_form_kind::group:
        return "group";
    case featherdoc::content_control_form_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto review_note_kind_name(featherdoc::review_note_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::review_note_kind::footnote:
        return "footnote";
    case featherdoc::review_note_kind::endnote:
        return "endnote";
    case featherdoc::review_note_kind::comment:
        return "comment";
    }

    return "footnote";
}

auto revision_kind_name(featherdoc::revision_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::revision_kind::insertion:
        return "insertion";
    case featherdoc::revision_kind::deletion:
        return "deletion";
    case featherdoc::revision_kind::move_from:
        return "move_from";
    case featherdoc::revision_kind::move_to:
        return "move_to";
    case featherdoc::revision_kind::paragraph_property_change:
        return "paragraph_property_change";
    case featherdoc::revision_kind::run_property_change:
        return "run_property_change";
    case featherdoc::revision_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto page_orientation_name(featherdoc::page_orientation orientation)
    -> std::string_view {
    switch (orientation) {
    case featherdoc::page_orientation::portrait:
        return "portrait";
    case featherdoc::page_orientation::landscape:
        return "landscape";
    }

    return "unknown";
}

auto style_usage_part_kind_name(featherdoc::style_usage_part_kind part_kind)
    -> const char * {
    switch (part_kind) {
    case featherdoc::style_usage_part_kind::body:
        return "body";
    case featherdoc::style_usage_part_kind::header:
        return "header";
    case featherdoc::style_usage_part_kind::footer:
        return "footer";
    }

    return "unknown";
}

auto style_usage_hit_kind_name(featherdoc::style_usage_hit_kind hit_kind)
    -> const char * {
    switch (hit_kind) {
    case featherdoc::style_usage_hit_kind::paragraph:
        return "paragraph";
    case featherdoc::style_usage_hit_kind::run:
        return "run";
    case featherdoc::style_usage_hit_kind::table:
        return "table";
    }

    return "unknown";
}

} // namespace featherdoc_cli
