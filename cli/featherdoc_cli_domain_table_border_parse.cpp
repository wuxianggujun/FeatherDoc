#include "featherdoc_cli_domain_parse.hpp"

namespace featherdoc_cli {

auto parse_table_style_margin_edge_text(
    std::string_view text, featherdoc::cell_margin_edge &edge) -> bool {
    if (text == "top") {
        edge = featherdoc::cell_margin_edge::top;
        return true;
    }
    if (text == "left") {
        edge = featherdoc::cell_margin_edge::left;
        return true;
    }
    if (text == "bottom") {
        edge = featherdoc::cell_margin_edge::bottom;
        return true;
    }
    if (text == "right") {
        edge = featherdoc::cell_margin_edge::right;
        return true;
    }

    return false;
}

auto parse_cell_margin_edge_text(std::string_view text,
                                 featherdoc::cell_margin_edge &edge) -> bool {
    return parse_table_style_margin_edge_text(text, edge);
}

auto parse_cell_border_edge_text(std::string_view text,
                                 featherdoc::cell_border_edge &edge) -> bool {
    if (text == "top") {
        edge = featherdoc::cell_border_edge::top;
        return true;
    }
    if (text == "left") {
        edge = featherdoc::cell_border_edge::left;
        return true;
    }
    if (text == "bottom") {
        edge = featherdoc::cell_border_edge::bottom;
        return true;
    }
    if (text == "right") {
        edge = featherdoc::cell_border_edge::right;
        return true;
    }

    return false;
}

auto parse_table_style_border_edge_text(
    std::string_view text, featherdoc::table_border_edge &edge) -> bool {
    if (text == "top") {
        edge = featherdoc::table_border_edge::top;
        return true;
    }
    if (text == "left") {
        edge = featherdoc::table_border_edge::left;
        return true;
    }
    if (text == "bottom") {
        edge = featherdoc::table_border_edge::bottom;
        return true;
    }
    if (text == "right") {
        edge = featherdoc::table_border_edge::right;
        return true;
    }
    if (text == "inside_horizontal" || text == "inside-horizontal") {
        edge = featherdoc::table_border_edge::inside_horizontal;
        return true;
    }
    if (text == "inside_vertical" || text == "inside-vertical") {
        edge = featherdoc::table_border_edge::inside_vertical;
        return true;
    }

    return false;
}

auto parse_table_border_edge_text(
    std::string_view text, featherdoc::table_border_edge &edge) -> bool {
    return parse_table_style_border_edge_text(text, edge);
}

auto parse_table_style_border_style_text(
    std::string_view text, featherdoc::border_style &style) -> bool {
    if (text == "none") {
        style = featherdoc::border_style::none;
        return true;
    }
    if (text == "single") {
        style = featherdoc::border_style::single;
        return true;
    }
    if (text == "double_line" || text == "double-line") {
        style = featherdoc::border_style::double_line;
        return true;
    }
    if (text == "dashed") {
        style = featherdoc::border_style::dashed;
        return true;
    }
    if (text == "dotted") {
        style = featherdoc::border_style::dotted;
        return true;
    }
    if (text == "thick") {
        style = featherdoc::border_style::thick;
        return true;
    }

    return false;
}

auto parse_border_style_text(std::string_view text,
                             featherdoc::border_style &style) -> bool {
    return parse_table_style_border_style_text(text, style);
}

} // namespace featherdoc_cli
