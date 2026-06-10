auto table_position_preset_name(table_position_preset preset) noexcept
    -> std::string_view {
    switch (preset) {
    case table_position_preset::paragraph_callout:
        return "paragraph-callout";
    case table_position_preset::page_corner:
        return "page-corner";
    case table_position_preset::margin_anchor:
        return "margin-anchor";
    }

    return "paragraph-callout";
}

auto parse_table_position_preset(std::string_view text,
                                 table_position_preset &preset) -> bool {
    if (text == "paragraph-callout") {
        preset = table_position_preset::paragraph_callout;
        return true;
    }
    if (text == "page-corner") {
        preset = table_position_preset::page_corner;
        return true;
    }
    if (text == "margin-anchor") {
        preset = table_position_preset::margin_anchor;
        return true;
    }

    return false;
}
