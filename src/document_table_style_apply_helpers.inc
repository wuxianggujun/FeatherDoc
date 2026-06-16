auto apply_table_style_margin(
    pugi::xml_node margins, featherdoc::cell_margin_edge edge,
    const std::optional<std::uint32_t> &value) -> bool {
    if (!value.has_value()) {
        return true;
    }

    auto margin = margins.child(to_table_style_margin_name(edge));
    if (margin == pugi::xml_node{}) {
        margin = margins.append_child(to_table_style_margin_name(edge));
    }
    if (margin == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(margin, "w:w", std::to_string(*value));
    ensure_attribute_value(margin, "w:type", "dxa");
    return true;
}

auto apply_table_style_margins(
    pugi::xml_node properties, const char *container_name,
    const featherdoc::table_style_margins_definition &margins) -> bool {
    const auto margins_node =
        ensure_table_style_margins_node(properties, container_name);
    if (margins_node == pugi::xml_node{}) {
        return false;
    }

    return apply_table_style_margin(margins_node,
                                    featherdoc::cell_margin_edge::top,
                                    margins.top_twips) &&
           apply_table_style_margin(margins_node,
                                    featherdoc::cell_margin_edge::left,
                                    margins.left_twips) &&
           apply_table_style_margin(margins_node,
                                    featherdoc::cell_margin_edge::bottom,
                                    margins.bottom_twips) &&
           apply_table_style_margin(margins_node,
                                    featherdoc::cell_margin_edge::right,
                                    margins.right_twips);
}

auto apply_table_style_border(
    pugi::xml_node borders, featherdoc::table_border_edge edge,
    const std::optional<featherdoc::border_definition> &value) -> bool {
    if (!value.has_value()) {
        return true;
    }

    auto border = borders.child(to_table_style_border_name(edge));
    if (border == pugi::xml_node{}) {
        border = borders.append_child(to_table_style_border_name(edge));
    }
    if (border == pugi::xml_node{}) {
        return false;
    }

    apply_table_style_border_definition(border, *value);
    return true;
}

auto apply_table_style_borders(
    pugi::xml_node properties, const char *container_name,
    const featherdoc::table_style_borders_definition &borders) -> bool {
    const auto borders_node =
        ensure_table_style_borders_node(properties, container_name);
    if (borders_node == pugi::xml_node{}) {
        return false;
    }

    return apply_table_style_border(
               borders_node, featherdoc::table_border_edge::top, borders.top) &&
           apply_table_style_border(borders_node,
                                    featherdoc::table_border_edge::left,
                                    borders.left) &&
           apply_table_style_border(borders_node,
                                    featherdoc::table_border_edge::bottom,
                                    borders.bottom) &&
           apply_table_style_border(borders_node,
                                    featherdoc::table_border_edge::right,
                                    borders.right) &&
           apply_table_style_border(
               borders_node, featherdoc::table_border_edge::inside_horizontal,
               borders.inside_horizontal) &&
           apply_table_style_border(
               borders_node, featherdoc::table_border_edge::inside_vertical,
               borders.inside_vertical);
}

auto apply_table_style_paragraph_spacing(
    pugi::xml_node paragraph_properties,
    const featherdoc::table_style_paragraph_spacing_definition &spacing)
    -> bool {
    const auto spacing_node =
        ensure_table_style_paragraph_spacing_node(paragraph_properties);
    if (spacing_node == pugi::xml_node{}) {
        return false;
    }

    if (spacing.before_twips.has_value()) {
        ensure_attribute_value(spacing_node, "w:before",
                               std::to_string(*spacing.before_twips));
    }
    if (spacing.after_twips.has_value()) {
        ensure_attribute_value(spacing_node, "w:after",
                               std::to_string(*spacing.after_twips));
    }
    if (spacing.line_twips.has_value()) {
        ensure_attribute_value(spacing_node, "w:line",
                               std::to_string(*spacing.line_twips));
    }
    if (spacing.line_rule.has_value()) {
        ensure_attribute_value(spacing_node, "w:lineRule",
                               to_table_style_paragraph_line_spacing_rule_value(
                                   *spacing.line_rule));
    }

    return true;
}

auto apply_table_style_region(
    pugi::xml_node parent,
    const featherdoc::table_style_region_definition &region,
    bool whole_table) -> bool {
    if (region.fill_color.has_value()) {
        const auto cell_properties =
            ensure_table_style_cell_properties_node(parent);
        const auto shading = ensure_table_style_shading_node(cell_properties);
        if (shading == pugi::xml_node{}) {
            return false;
        }

        ensure_attribute_value(shading, "w:val", "clear");
        ensure_attribute_value(shading, "w:color", "auto");
        ensure_attribute_value(shading, "w:fill", *region.fill_color);
    }

    if (region.cell_margins.has_value()) {
        const auto properties =
            whole_table ? ensure_table_style_table_properties_node(parent)
                        : ensure_table_style_cell_properties_node(parent);
        if (!apply_table_style_margins(properties,
                                       whole_table ? "w:tblCellMar" : "w:tcMar",
                                       *region.cell_margins)) {
            return false;
        }
    }

    if (region.cell_vertical_alignment.has_value()) {
        const auto cell_properties =
            ensure_table_style_cell_properties_node(parent);
        const auto vertical_alignment =
            ensure_table_style_vertical_alignment_node(cell_properties);
        if (vertical_alignment == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(vertical_alignment, "w:val",
                               to_table_style_cell_vertical_alignment_value(
                                   *region.cell_vertical_alignment));
    }

    if (region.borders.has_value()) {
        const auto properties =
            whole_table ? ensure_table_style_table_properties_node(parent)
                        : ensure_table_style_cell_properties_node(parent);
        if (!apply_table_style_borders(
                properties, whole_table ? "w:tblBorders" : "w:tcBorders",
                *region.borders)) {
            return false;
        }
    }

    if (region.cell_text_direction.has_value()) {
        const auto cell_properties =
            ensure_table_style_cell_properties_node(parent);
        const auto text_direction =
            ensure_table_style_text_direction_node(cell_properties);
        if (text_direction == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(text_direction, "w:val",
                               to_table_style_cell_text_direction_value(
                                   *region.cell_text_direction));
    }

    if (region.paragraph_alignment.has_value()) {
        const auto paragraph_properties =
            ensure_table_style_paragraph_properties_node(parent);
        const auto alignment =
            ensure_table_style_paragraph_alignment_node(paragraph_properties);
        if (alignment == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(alignment, "w:val",
                               to_table_style_paragraph_alignment_value(
                                   *region.paragraph_alignment));
    }

    if (region.paragraph_spacing.has_value()) {
        const auto paragraph_properties =
            ensure_table_style_paragraph_properties_node(parent);
        if (!apply_table_style_paragraph_spacing(paragraph_properties,
                                                 *region.paragraph_spacing)) {
            return false;
        }
    }

    if (region.text_color.has_value()) {
        const auto run_properties =
            ensure_table_style_run_properties_node(parent);
        const auto color = ensure_table_style_text_color_node(run_properties);
        if (color == pugi::xml_node{}) {
            return false;
        }

        ensure_attribute_value(color, "w:val", *region.text_color);
    }

    if (region.bold.has_value()) {
        const auto run_properties =
            ensure_table_style_run_properties_node(parent);
        const auto bold = ensure_table_style_bold_node(run_properties);
        if (bold == pugi::xml_node{}) {
            return false;
        }

        set_on_off_value(bold, *region.bold);
    }

    if (region.italic.has_value()) {
        const auto run_properties =
            ensure_table_style_run_properties_node(parent);
        const auto italic = ensure_table_style_italic_node(run_properties);
        if (italic == pugi::xml_node{}) {
            return false;
        }

        set_on_off_value(italic, *region.italic);
    }

    if (region.font_size_points.has_value()) {
        const auto run_properties =
            ensure_table_style_run_properties_node(parent);
        const auto primary_size =
            ensure_table_style_font_size_node(run_properties, "w:sz");
        const auto complex_size =
            ensure_table_style_font_size_node(run_properties, "w:szCs");
        if (primary_size == pugi::xml_node{} ||
            complex_size == pugi::xml_node{}) {
            return false;
        }

        const auto half_points = std::to_string(*region.font_size_points * 2U);
        ensure_attribute_value(primary_size, "w:val", half_points);
        ensure_attribute_value(complex_size, "w:val", half_points);
    }

    if (region.font_family.has_value() ||
        region.east_asia_font_family.has_value()) {
        const auto run_properties =
            ensure_table_style_run_properties_node(parent);
        const auto run_fonts = ensure_run_fonts_node(run_properties);
        if (run_fonts == pugi::xml_node{}) {
            return false;
        }

        if (region.font_family.has_value()) {
            ensure_attribute_value(run_fonts, "w:ascii", *region.font_family);
            ensure_attribute_value(run_fonts, "w:hAnsi", *region.font_family);
            ensure_attribute_value(run_fonts, "w:cs", *region.font_family);
        }
        if (region.east_asia_font_family.has_value()) {
            ensure_attribute_value(run_fonts, "w:eastAsia",
                                   *region.east_asia_font_family);
        }
    }

    return true;
}

auto apply_table_style_conditional_region(
    pugi::xml_node style, std::string_view type_name,
    const std::optional<featherdoc::table_style_region_definition> &region)
    -> bool {
    if (!region.has_value()) {
        return true;
    }

    const auto conditional =
        ensure_table_style_conditional_node(style, type_name);
    return conditional != pugi::xml_node{} &&
           apply_table_style_region(conditional, *region, false);
}

auto apply_table_style_regions(
    pugi::xml_node style,
    const featherdoc::table_style_definition &definition) -> bool {
    if (definition.whole_table.has_value() &&
        !apply_table_style_region(style, *definition.whole_table, true)) {
        return false;
    }

    return apply_table_style_conditional_region(style, "firstRow",
                                                definition.first_row) &&
           apply_table_style_conditional_region(style, "lastRow",
                                                definition.last_row) &&
           apply_table_style_conditional_region(style, "firstCol",
                                                definition.first_column) &&
           apply_table_style_conditional_region(style, "lastCol",
                                                definition.last_column) &&
           apply_table_style_conditional_region(style, "band1Horz",
                                                definition.banded_rows) &&
           apply_table_style_conditional_region(style, "band1Vert",
                                                definition.banded_columns) &&
           apply_table_style_conditional_region(
               style, "band2Horz", definition.second_banded_rows) &&
           apply_table_style_conditional_region(
               style, "band2Vert", definition.second_banded_columns);
}
