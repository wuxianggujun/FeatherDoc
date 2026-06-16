#include "table_xml_helpers.hpp"

namespace featherdoc::detail {

auto ensure_cell_properties_node(pugi::xml_node cell) -> pugi::xml_node {
    if (cell == pugi::xml_node{}) {
        return {};
    }

    auto cell_properties = cell.child("w:tcPr");
    if (cell_properties != pugi::xml_node{}) {
        return cell_properties;
    }

    if (const auto first_paragraph = cell.child("w:p"); first_paragraph != pugi::xml_node{}) {
        return cell.insert_child_before("w:tcPr", first_paragraph);
    }

    if (const auto first_child = cell.first_child(); first_child != pugi::xml_node{}) {
        return cell.insert_child_before("w:tcPr", first_child);
    }

    return cell.append_child("w:tcPr");
}

void ensure_default_cell_properties(pugi::xml_node cell) {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return;
    }

    auto cell_width = cell_properties.child("w:tcW");
    if (cell_width == pugi::xml_node{}) {
        cell_width = cell_properties.append_child("w:tcW");
    }
    ensure_attribute_value(cell_width, "w:w", "0");
    ensure_attribute_value(cell_width, "w:type", "auto");
}

auto ensure_cell_width_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto cell_width = cell_properties.child("w:tcW");
    if (cell_width != pugi::xml_node{}) {
        return cell_width;
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:tcW", first_child);
    }

    return cell_properties.append_child("w:tcW");
}

bool clear_cell_width_node(pugi::xml_node cell) {
    if (cell == pugi::xml_node{}) {
        return false;
    }

    auto cell_properties = cell.child("w:tcPr");
    if (cell_properties == pugi::xml_node{}) {
        return true;
    }

    const auto width_node = cell_properties.child("w:tcW");
    return width_node == pugi::xml_node{} || cell_properties.remove_child(width_node);
}

auto ensure_cell_vertical_merge_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto vertical_merge = cell_properties.child("w:vMerge");
    if (vertical_merge != pugi::xml_node{}) {
        return vertical_merge;
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vMerge", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vMerge", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:vMerge", first_child);
    }

    return cell_properties.append_child("w:vMerge");
}

auto ensure_cell_borders_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto cell_borders = cell_properties.child("w:tcBorders");
    if (cell_borders != pugi::xml_node{}) {
        return cell_borders;
    }

    if (const auto vertical_merge = cell_properties.child("w:vMerge");
        vertical_merge != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcBorders", vertical_merge);
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcBorders", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcBorders", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:tcBorders", first_child);
    }

    return cell_properties.append_child("w:tcBorders");
}

auto ensure_cell_vertical_alignment_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto vertical_alignment = cell_properties.child("w:vAlign");
    if (vertical_alignment != pugi::xml_node{}) {
        return vertical_alignment;
    }

    if (const auto margins = cell_properties.child("w:tcMar"); margins != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", margins);
    }

    if (const auto shading = cell_properties.child("w:shd"); shading != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", shading);
    }

    if (const auto borders = cell_properties.child("w:tcBorders"); borders != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", borders);
    }

    if (const auto vertical_merge = cell_properties.child("w:vMerge");
        vertical_merge != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", vertical_merge);
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:vAlign", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:vAlign", first_child);
    }

    return cell_properties.append_child("w:vAlign");
}

auto ensure_cell_text_direction_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto text_direction = cell_properties.child("w:textDirection");
    if (text_direction != pugi::xml_node{}) {
        return text_direction;
    }

    if (const auto vertical_alignment = cell_properties.child("w:vAlign");
        vertical_alignment != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:textDirection", vertical_alignment);
    }

    if (const auto margins = cell_properties.child("w:tcMar"); margins != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", margins);
    }

    if (const auto shading = cell_properties.child("w:shd"); shading != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", shading);
    }

    if (const auto borders = cell_properties.child("w:tcBorders"); borders != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", borders);
    }

    if (const auto vertical_merge = cell_properties.child("w:vMerge");
        vertical_merge != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", vertical_merge);
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:textDirection", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:textDirection", first_child);
    }

    return cell_properties.append_child("w:textDirection");
}

auto ensure_cell_shading_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto shading = cell_properties.child("w:shd");
    if (shading != pugi::xml_node{}) {
        return shading;
    }

    if (const auto borders = cell_properties.child("w:tcBorders"); borders != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:shd", borders);
    }

    if (const auto vertical_merge = cell_properties.child("w:vMerge");
        vertical_merge != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:shd", vertical_merge);
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:shd", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:shd", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:shd", first_child);
    }

    return cell_properties.append_child("w:shd");
}

auto ensure_cell_margins_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto margins = cell_properties.child("w:tcMar");
    if (margins != pugi::xml_node{}) {
        return margins;
    }

    if (const auto shading = cell_properties.child("w:shd"); shading != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcMar", shading);
    }

    if (const auto borders = cell_properties.child("w:tcBorders"); borders != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcMar", borders);
    }

    if (const auto vertical_merge = cell_properties.child("w:vMerge");
        vertical_merge != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcMar", vertical_merge);
    }

    if (const auto grid_span = cell_properties.child("w:gridSpan"); grid_span != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcMar", grid_span);
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:tcMar", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:tcMar", first_child);
    }

    return cell_properties.append_child("w:tcMar");
}

auto ensure_cell_margin_node(pugi::xml_node cell, const char *margin_name) -> pugi::xml_node {
    auto margins = ensure_cell_margins_node(cell);
    if (margins == pugi::xml_node{}) {
        return {};
    }

    auto margin = margins.child(margin_name);
    if (margin != pugi::xml_node{}) {
        return margin;
    }

    return margins.append_child(margin_name);
}

auto ensure_cell_grid_span_node(pugi::xml_node cell) -> pugi::xml_node {
    auto cell_properties = ensure_cell_properties_node(cell);
    if (cell_properties == pugi::xml_node{}) {
        return {};
    }

    auto grid_span = cell_properties.child("w:gridSpan");
    if (grid_span != pugi::xml_node{}) {
        return grid_span;
    }

    if (const auto cell_width = cell_properties.child("w:tcW"); cell_width != pugi::xml_node{}) {
        return cell_properties.insert_child_after("w:gridSpan", cell_width);
    }

    if (const auto first_child = cell_properties.first_child(); first_child != pugi::xml_node{}) {
        return cell_properties.insert_child_before("w:gridSpan", first_child);
    }

    return cell_properties.append_child("w:gridSpan");
}

} // namespace featherdoc::detail
