#include "featherdoc_cli_table_style_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_style_output.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string_view>

namespace featherdoc_cli {

void write_json_table_style_look(
    std::ostream &stream, const featherdoc::table_style_look &style_look) {
    stream << "{\"first_row\":" << json_bool(style_look.first_row)
           << ",\"last_row\":" << json_bool(style_look.last_row)
           << ",\"first_column\":" << json_bool(style_look.first_column)
           << ",\"last_column\":" << json_bool(style_look.last_column)
           << ",\"banded_rows\":" << json_bool(style_look.banded_rows)
           << ",\"banded_columns\":" << json_bool(style_look.banded_columns)
           << '}';
}


void write_json_optional_table_style_cell_vertical_alignment(
    std::ostream &stream,
    const std::optional<featherdoc::cell_vertical_alignment> &alignment) {
    if (!alignment.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream,
                      table_style_cell_vertical_alignment_name(*alignment));
}

void write_json_optional_table_style_cell_text_direction(
    std::ostream &stream,
    const std::optional<featherdoc::cell_text_direction> &direction) {
    if (!direction.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream, table_style_cell_text_direction_name(*direction));
}

void write_json_optional_table_style_paragraph_alignment(
    std::ostream &stream,
    const std::optional<featherdoc::paragraph_alignment> &alignment) {
    if (!alignment.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream, table_style_paragraph_alignment_name(*alignment));
}

void write_json_optional_table_style_paragraph_line_spacing_rule(
    std::ostream &stream,
    const std::optional<featherdoc::paragraph_line_spacing_rule> &rule) {
    if (!rule.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream,
                      table_style_paragraph_line_spacing_rule_name(*rule));
}

void write_json_table_style_paragraph_spacing(
    std::ostream &stream,
    const featherdoc::table_style_paragraph_spacing_definition &spacing) {
    stream << "{\"before_twips\":";
    write_json_optional_u32(stream, spacing.before_twips);
    stream << ",\"after_twips\":";
    write_json_optional_u32(stream, spacing.after_twips);
    stream << ",\"line_twips\":";
    write_json_optional_u32(stream, spacing.line_twips);
    stream << ",\"line_rule\":";
    write_json_optional_table_style_paragraph_line_spacing_rule(
        stream, spacing.line_rule);
    stream << '}';
}

void write_json_table_style_margins(
    std::ostream &stream,
    const featherdoc::table_style_margins_definition &margins) {
    stream << "{\"top_twips\":";
    write_json_optional_u32(stream, margins.top_twips);
    stream << ",\"left_twips\":";
    write_json_optional_u32(stream, margins.left_twips);
    stream << ",\"bottom_twips\":";
    write_json_optional_u32(stream, margins.bottom_twips);
    stream << ",\"right_twips\":";
    write_json_optional_u32(stream, margins.right_twips);
    stream << '}';
}

void write_json_table_style_border(
    std::ostream &stream,
    const featherdoc::table_style_border_summary &border) {
    stream << "{\"style\":";
    write_json_optional_string(stream, border.style);
    stream << ",\"size_eighth_points\":";
    write_json_optional_u32(stream, border.size_eighth_points);
    stream << ",\"color\":";
    write_json_optional_string(stream, border.color);
    stream << ",\"space_points\":";
    write_json_optional_u32(stream, border.space_points);
    stream << '}';
}

void write_json_optional_table_style_border(
    std::ostream &stream,
    const std::optional<featherdoc::table_style_border_summary> &border) {
    if (!border.has_value()) {
        stream << "null";
        return;
    }

    write_json_table_style_border(stream, *border);
}

void write_json_table_style_borders(
    std::ostream &stream,
    const featherdoc::table_style_borders_summary &borders) {
    stream << "{\"top\":";
    write_json_optional_table_style_border(stream, borders.top);
    stream << ",\"left\":";
    write_json_optional_table_style_border(stream, borders.left);
    stream << ",\"bottom\":";
    write_json_optional_table_style_border(stream, borders.bottom);
    stream << ",\"right\":";
    write_json_optional_table_style_border(stream, borders.right);
    stream << ",\"inside_horizontal\":";
    write_json_optional_table_style_border(stream, borders.inside_horizontal);
    stream << ",\"inside_vertical\":";
    write_json_optional_table_style_border(stream, borders.inside_vertical);
    stream << '}';
}

void write_json_table_style_region(
    std::ostream &stream,
    const featherdoc::table_style_region_summary &region) {
    stream << "{\"fill_color\":";
    write_json_optional_string(stream, region.fill_color);
    stream << ",\"text_color\":";
    write_json_optional_string(stream, region.text_color);
    stream << ",\"bold\":";
    write_json_optional_bool(stream, region.bold);
    stream << ",\"italic\":";
    write_json_optional_bool(stream, region.italic);
    stream << ",\"font_size_points\":";
    write_json_optional_u32(stream, region.font_size_points);
    stream << ",\"font_family\":";
    write_json_optional_string(stream, region.font_family);
    stream << ",\"east_asia_font_family\":";
    write_json_optional_string(stream, region.east_asia_font_family);
    stream << ",\"cell_vertical_alignment\":";
    write_json_optional_table_style_cell_vertical_alignment(
        stream, region.cell_vertical_alignment);
    stream << ",\"cell_text_direction\":";
    write_json_optional_table_style_cell_text_direction(
        stream, region.cell_text_direction);
    stream << ",\"paragraph_alignment\":";
    write_json_optional_table_style_paragraph_alignment(
        stream, region.paragraph_alignment);
    stream << ",\"paragraph_spacing\":";
    if (region.paragraph_spacing.has_value()) {
        write_json_table_style_paragraph_spacing(stream,
                                                 *region.paragraph_spacing);
    } else {
        stream << "null";
    }
    stream << ",\"cell_margins\":";
    if (region.cell_margins.has_value()) {
        write_json_table_style_margins(stream, *region.cell_margins);
    } else {
        stream << "null";
    }
    stream << ",\"borders\":";
    if (region.borders.has_value()) {
        write_json_table_style_borders(stream, *region.borders);
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_optional_table_style_region(
    std::ostream &stream,
    const std::optional<featherdoc::table_style_region_summary> &region) {
    if (!region.has_value()) {
        stream << "null";
        return;
    }

    write_json_table_style_region(stream, *region);
}

void write_json_table_style_definition_summary(
    std::ostream &stream,
    const featherdoc::table_style_definition_summary &definition) {
    stream << "{\"style\":";
    write_json_style_summary(stream, definition.style);
    stream << ",\"regions\":{\"whole_table\":";
    write_json_optional_table_style_region(stream, definition.whole_table);
    stream << ",\"first_row\":";
    write_json_optional_table_style_region(stream, definition.first_row);
    stream << ",\"last_row\":";
    write_json_optional_table_style_region(stream, definition.last_row);
    stream << ",\"first_column\":";
    write_json_optional_table_style_region(stream, definition.first_column);
    stream << ",\"last_column\":";
    write_json_optional_table_style_region(stream, definition.last_column);
    stream << ",\"banded_rows\":";
    write_json_optional_table_style_region(stream, definition.banded_rows);
    stream << ",\"banded_columns\":";
    write_json_optional_table_style_region(stream, definition.banded_columns);
    stream << ",\"second_banded_rows\":";
    write_json_optional_table_style_region(stream,
                                           definition.second_banded_rows);
    stream << ",\"second_banded_columns\":";
    write_json_optional_table_style_region(stream,
                                           definition.second_banded_columns);
    stream << "}}";
}

void print_optional_u32_field(std::ostream &stream, std::string_view name,
                              const std::optional<std::uint32_t> &value) {
    stream << ' ' << name << '=';
    if (value.has_value()) {
        stream << *value;
    } else {
        stream << '-';
    }
}

void print_optional_string_field(
    std::ostream &stream, std::string_view name,
    const std::optional<std::string> &value) {
    stream << ' ' << name << '=';
    if (value.has_value()) {
        stream << *value;
    } else {
        stream << '-';
    }
}

void print_table_style_margins(
    std::ostream &stream,
    const featherdoc::table_style_margins_definition &margins) {
    print_optional_u32_field(stream, "top_twips", margins.top_twips);
    print_optional_u32_field(stream, "left_twips", margins.left_twips);
    print_optional_u32_field(stream, "bottom_twips", margins.bottom_twips);
    print_optional_u32_field(stream, "right_twips", margins.right_twips);
}

void print_optional_table_style_paragraph_line_spacing_rule(
    std::ostream &stream, std::string_view name,
    const std::optional<featherdoc::paragraph_line_spacing_rule> &rule) {
    stream << ' ' << name << '=';
    if (rule.has_value()) {
        stream << table_style_paragraph_line_spacing_rule_name(*rule);
    } else {
        stream << '-';
    }
}

void print_table_style_paragraph_spacing(
    std::ostream &stream,
    const featherdoc::table_style_paragraph_spacing_definition &spacing) {
    print_optional_u32_field(stream, "paragraph_spacing_before_twips",
                             spacing.before_twips);
    print_optional_u32_field(stream, "paragraph_spacing_after_twips",
                             spacing.after_twips);
    print_optional_u32_field(stream, "paragraph_spacing_line_twips",
                             spacing.line_twips);
    print_optional_table_style_paragraph_line_spacing_rule(
        stream, "paragraph_spacing_line_rule", spacing.line_rule);
}

void print_table_style_border(
    std::ostream &stream, std::string_view name,
    const featherdoc::table_style_border_summary &border) {
    stream << "  border_" << name << ':';
    print_optional_string_field(stream, "style", border.style);
    print_optional_u32_field(stream, "size_eighth_points",
                             border.size_eighth_points);
    print_optional_string_field(stream, "color", border.color);
    print_optional_u32_field(stream, "space_points", border.space_points);
    stream << '\n';
}

void print_optional_table_style_border(
    std::ostream &stream, std::string_view name,
    const std::optional<featherdoc::table_style_border_summary> &border) {
    if (border.has_value()) {
        print_table_style_border(stream, name, *border);
    }
}

void print_table_style_region(
    std::ostream &stream, std::string_view name,
    const featherdoc::table_style_region_summary &region) {
    stream << "region_" << name << ": fill_color=";
    if (region.fill_color.has_value()) {
        stream << *region.fill_color;
    } else {
        stream << '-';
    }
    stream << " text_color=";
    if (region.text_color.has_value()) {
        stream << *region.text_color;
    } else {
        stream << '-';
    }
    stream << " bold=";
    if (region.bold.has_value()) {
        stream << json_bool(*region.bold);
    } else {
        stream << '-';
    }
    stream << " italic=";
    if (region.italic.has_value()) {
        stream << json_bool(*region.italic);
    } else {
        stream << '-';
    }
    print_optional_u32_field(stream, "font_size_points",
                             region.font_size_points);
    print_optional_string_field(stream, "font_family", region.font_family);
    print_optional_string_field(stream, "east_asia_font_family",
                                region.east_asia_font_family);
    stream << " cell_vertical_alignment=";
    if (region.cell_vertical_alignment.has_value()) {
        stream << table_style_cell_vertical_alignment_name(
            *region.cell_vertical_alignment);
    } else {
        stream << '-';
    }
    stream << " cell_text_direction=";
    if (region.cell_text_direction.has_value()) {
        stream << table_style_cell_text_direction_name(
            *region.cell_text_direction);
    } else {
        stream << '-';
    }
    stream << " paragraph_alignment=";
    if (region.paragraph_alignment.has_value()) {
        stream << table_style_paragraph_alignment_name(
            *region.paragraph_alignment);
    } else {
        stream << '-';
    }
    if (region.paragraph_spacing.has_value()) {
        print_table_style_paragraph_spacing(stream, *region.paragraph_spacing);
    }
    if (region.cell_margins.has_value()) {
        print_table_style_margins(stream, *region.cell_margins);
    }
    stream << '\n';

    if (region.borders.has_value()) {
        print_optional_table_style_border(stream, "top", region.borders->top);
        print_optional_table_style_border(stream, "left", region.borders->left);
        print_optional_table_style_border(stream, "bottom",
                                          region.borders->bottom);
        print_optional_table_style_border(stream, "right",
                                          region.borders->right);
        print_optional_table_style_border(
            stream, "inside_horizontal", region.borders->inside_horizontal);
        print_optional_table_style_border(
            stream, "inside_vertical", region.borders->inside_vertical);
    }
}

void print_optional_table_style_region(
    std::ostream &stream, std::string_view name,
    const std::optional<featherdoc::table_style_region_summary> &region) {
    if (region.has_value()) {
        print_table_style_region(stream, name, *region);
    } else {
        stream << "region_" << name << ": none\n";
    }
}

void inspect_table_style_definition(
    const featherdoc::table_style_definition_summary &definition,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"table_style_definition\":";
        write_json_table_style_definition_summary(std::cout, definition);
        std::cout << "}\n";
        return;
    }

    inspect_style(definition.style, std::nullopt, false);
    print_optional_table_style_region(std::cout, "whole_table",
                                      definition.whole_table);
    print_optional_table_style_region(std::cout, "first_row",
                                      definition.first_row);
    print_optional_table_style_region(std::cout, "last_row",
                                      definition.last_row);
    print_optional_table_style_region(std::cout, "first_column",
                                      definition.first_column);
    print_optional_table_style_region(std::cout, "last_column",
                                      definition.last_column);
    print_optional_table_style_region(std::cout, "banded_rows",
                                      definition.banded_rows);
    print_optional_table_style_region(std::cout, "banded_columns",
                                      definition.banded_columns);
    print_optional_table_style_region(std::cout, "second_banded_rows",
                                      definition.second_banded_rows);
    print_optional_table_style_region(std::cout, "second_banded_columns",
                                      definition.second_banded_columns);
}

} // namespace featherdoc_cli
