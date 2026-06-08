#include "featherdoc_cli_template_table_inspect_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_table_position_output.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_border_summary(
    std::ostream &stream, const featherdoc::border_inspection_summary &border) {
    stream << "{\"style\":";
    write_json_string(stream, border_style_name(border.style));
    stream << ",\"size_eighth_points\":" << border.size_eighth_points
           << ",\"color\":";
    write_json_string(stream, border.color);
    stream << ",\"space_points\":" << border.space_points << '}';
}

void write_json_optional_border_summary(
    std::ostream &stream,
    const std::optional<featherdoc::border_inspection_summary> &border) {
    if (border.has_value()) {
        write_json_border_summary(stream, *border);
    } else {
        stream << "null";
    }
}

void write_json_optional_table_borders_summary(
    std::ostream &stream,
    const std::optional<featherdoc::table_borders_inspection_summary> &borders) {
    if (!borders.has_value()) {
        stream << "null";
        return;
    }

    stream << "{\"top\":";
    write_json_optional_border_summary(stream, borders->top);
    stream << ",\"left\":";
    write_json_optional_border_summary(stream, borders->left);
    stream << ",\"bottom\":";
    write_json_optional_border_summary(stream, borders->bottom);
    stream << ",\"right\":";
    write_json_optional_border_summary(stream, borders->right);
    stream << ",\"inside_horizontal\":";
    write_json_optional_border_summary(stream, borders->inside_horizontal);
    stream << ",\"inside_vertical\":";
    write_json_optional_border_summary(stream, borders->inside_vertical);
    stream << '}';
}

void write_json_table_summary(std::ostream &stream,
                              const featherdoc::table_inspection_summary &table) {
    stream << "{\"index\":" << table.index << ",\"style_id\":";
    write_json_optional_string(stream, table.style_id);
    stream << ",\"width_twips\":";
    write_json_optional_u32(stream, table.width_twips);
    stream << ",\"alignment\":";
    if (table.alignment.has_value()) {
        write_json_string(stream, table_alignment_name(*table.alignment));
    } else {
        stream << "null";
    }
    stream << ",\"indent_twips\":";
    write_json_optional_u32(stream, table.indent_twips);
    stream << ",\"cell_spacing_twips\":";
    write_json_optional_u32(stream, table.cell_spacing_twips);
    stream << ",\"cell_margins\":{\"top\":";
    write_json_optional_u32(stream, table.cell_margin_top_twips);
    stream << ",\"left\":";
    write_json_optional_u32(stream, table.cell_margin_left_twips);
    stream << ",\"bottom\":";
    write_json_optional_u32(stream, table.cell_margin_bottom_twips);
    stream << ",\"right\":";
    write_json_optional_u32(stream, table.cell_margin_right_twips);
    stream << '}';
    stream << ",\"borders\":";
    write_json_optional_table_borders_summary(stream, table.borders);
    stream << ",\"position\":";
    write_json_optional_table_position(stream, table.position);
    stream << ",\"row_count\":" << table.row_count
           << ",\"column_count\":" << table.column_count
           << ",\"column_widths\":[";
    for (std::size_t index = 0; index < table.column_widths.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_optional_u32(stream, table.column_widths[index]);
    }
    stream << "],\"text\":";
    write_json_string(stream, table.text);
    stream << '}';
}

void write_json_table_cell_summary(
    std::ostream &stream, const featherdoc::table_cell_inspection_summary &cell) {
    stream << "{\"row_index\":" << cell.row_index
           << ",\"cell_index\":" << cell.cell_index
           << ",\"column_index\":" << cell.column_index
           << ",\"column_span\":" << cell.column_span
           << ",\"paragraph_count\":" << cell.paragraph_count
           << ",\"width_twips\":";
    write_json_optional_u32(stream, cell.width_twips);
    stream << ",\"vertical_alignment\":";
    if (cell.vertical_alignment.has_value()) {
        write_json_string(stream,
                          cell_vertical_alignment_name(*cell.vertical_alignment));
    } else {
        stream << "null";
    }
    stream << ",\"text_direction\":";
    if (cell.text_direction.has_value()) {
        write_json_string(stream, cell_text_direction_name(*cell.text_direction));
    } else {
        stream << "null";
    }
    stream << ",\"text\":";
    write_json_string(stream, cell.text);
    stream << '}';
}

void write_json_table_row_summary(std::ostream &stream,
                                  const table_row_inspection_summary &row) {
    stream << "{\"row_index\":" << row.row_index
           << ",\"cell_count\":" << row.cell_count << ",\"height_twips\":";
    write_json_optional_u32(stream, row.height_twips);
    stream << ",\"height_rule\":";
    if (row.height_rule.has_value()) {
        write_json_string(stream, row_height_rule_name(*row.height_rule));
    } else {
        stream << "null";
    }
    stream << ",\"cant_split\":" << json_bool(row.cant_split)
           << ",\"repeats_header\":" << json_bool(row.repeats_header)
           << ",\"cell_texts\":[";
    for (std::size_t index = 0; index < row.cell_texts.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_string(stream, row.cell_texts[index]);
    }
    stream << "]}";
}

void print_optional_table_border(
    std::ostream &stream,
    const std::optional<featherdoc::border_inspection_summary> &border) {
    if (!border.has_value()) {
        stream << "none";
        return;
    }

    stream << border_style_name(border->style) << '/' << border->size_eighth_points
           << '/' << border->color << '/' << border->space_points;
}

void print_table_summary(std::ostream &stream,
                         const featherdoc::table_inspection_summary &table) {
    stream << "table[" << table.index << "]: style_id=";
    if (table.style_id.has_value()) {
        stream << *table.style_id;
    } else {
        stream << "none";
    }
    stream << " width_twips=";
    if (table.width_twips.has_value()) {
        stream << *table.width_twips;
    } else {
        stream << "none";
    }
    stream << " rows=" << table.row_count
           << " columns=" << table.column_count
           << " alignment=";
    if (table.alignment.has_value()) {
        stream << table_alignment_name(*table.alignment);
    } else {
        stream << "none";
    }
    stream << " indent_twips=";
    if (table.indent_twips.has_value()) {
        stream << *table.indent_twips;
    } else {
        stream << "none";
    }
    stream << " cell_spacing_twips=";
    if (table.cell_spacing_twips.has_value()) {
        stream << *table.cell_spacing_twips;
    } else {
        stream << "none";
    }
    stream << " cell_margins={top:";
    if (table.cell_margin_top_twips.has_value()) {
        stream << *table.cell_margin_top_twips;
    } else {
        stream << "none";
    }
    stream << ",left:";
    if (table.cell_margin_left_twips.has_value()) {
        stream << *table.cell_margin_left_twips;
    } else {
        stream << "none";
    }
    stream << ",bottom:";
    if (table.cell_margin_bottom_twips.has_value()) {
        stream << *table.cell_margin_bottom_twips;
    } else {
        stream << "none";
    }
    stream << ",right:";
    if (table.cell_margin_right_twips.has_value()) {
        stream << *table.cell_margin_right_twips;
    } else {
        stream << "none";
    }
    stream << "} borders=";
    if (table.borders.has_value()) {
        stream << "{top:";
        print_optional_table_border(stream, table.borders->top);
        stream << ",left:";
        print_optional_table_border(stream, table.borders->left);
        stream << ",bottom:";
        print_optional_table_border(stream, table.borders->bottom);
        stream << ",right:";
        print_optional_table_border(stream, table.borders->right);
        stream << ",inside_horizontal:";
        print_optional_table_border(stream, table.borders->inside_horizontal);
        stream << ",inside_vertical:";
        print_optional_table_border(stream, table.borders->inside_vertical);
        stream << '}';
    } else {
        stream << "none";
    }
    stream << " column_widths=[";
    for (std::size_t index = 0; index < table.column_widths.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        if (table.column_widths[index].has_value()) {
            stream << *table.column_widths[index];
        } else {
            stream << "none";
        }
    }
    stream << "] position=";
    write_table_position_text(stream, table.position);
    stream << " text=" << format_paragraph_text(table.text);
}

void print_table_cell_summary(
    std::ostream &stream, const featherdoc::table_cell_inspection_summary &cell) {
    stream << "cell[row=" << cell.row_index << ", cell=" << cell.cell_index
           << "]: column=" << cell.column_index
           << " span=" << cell.column_span
           << " paragraphs=" << cell.paragraph_count
           << " width_twips=";
    if (cell.width_twips.has_value()) {
        stream << *cell.width_twips;
    } else {
        stream << "none";
    }
    stream << " vertical_alignment=";
    if (cell.vertical_alignment.has_value()) {
        stream << cell_vertical_alignment_name(*cell.vertical_alignment);
    } else {
        stream << "none";
    }
    stream << " text_direction=";
    if (cell.text_direction.has_value()) {
        stream << cell_text_direction_name(*cell.text_direction);
    } else {
        stream << "none";
    }
    stream << " text=" << format_paragraph_text(cell.text);
}

void print_table_row_summary(std::ostream &stream,
                             const table_row_inspection_summary &row) {
    stream << "row[" << row.row_index << "]: cells=" << row.cell_count
           << " height_twips=";
    if (row.height_twips.has_value()) {
        stream << *row.height_twips;
    } else {
        stream << "none";
    }
    stream << " height_rule=";
    if (row.height_rule.has_value()) {
        stream << row_height_rule_name(*row.height_rule);
    } else {
        stream << "none";
    }
    stream << " cant_split=" << yes_no(row.cant_split)
           << " repeats_header=" << yes_no(row.repeats_header)
           << " cell_texts=[";
    for (std::size_t index = 0; index < row.cell_texts.size(); ++index) {
        if (index != 0U) {
            stream << ", ";
        }
        stream << format_paragraph_text(row.cell_texts[index]);
    }
    stream << ']';
}

void inspect_table(const featherdoc::table_inspection_summary &table,
                   bool json_output) {
    if (json_output) {
        std::cout << "{\"table\":";
        write_json_table_summary(std::cout, table);
        std::cout << "}\n";
        return;
    }

    std::cout << "index: " << table.index << '\n'
              << "style_id: ";
    if (table.style_id.has_value()) {
        std::cout << *table.style_id;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "width_twips: ";
    if (table.width_twips.has_value()) {
        std::cout << *table.width_twips;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "position: ";
    write_table_position_text(std::cout, table.position);
    std::cout << '\n' << "row_count: " << table.row_count << '\n'
              << "column_count: " << table.column_count << '\n'
              << "column_widths: [";
    for (std::size_t index = 0; index < table.column_widths.size(); ++index) {
        if (index != 0U) {
            std::cout << ',';
        }
        if (table.column_widths[index].has_value()) {
            std::cout << *table.column_widths[index];
        } else {
            std::cout << "none";
        }
    }
    std::cout << "]\n"
              << "text: " << format_paragraph_text(table.text) << '\n';
}

void inspect_table_cells(
    std::size_t table_index, std::optional<std::size_t> row_index,
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"table_index\":" << table_index;
        if (row_index.has_value()) {
            std::cout << ",\"row_index\":" << *row_index;
        }
        std::cout << ",\"count\":" << cells.size() << ",\"cells\":[";
        for (std::size_t index = 0; index < cells.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_cell_summary(std::cout, cells[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "table_index: " << table_index << '\n';
    if (row_index.has_value()) {
        std::cout << "row_index: " << *row_index << '\n';
    }
    std::cout << "cells: " << cells.size() << '\n';
    for (const auto &cell : cells) {
        print_table_cell_summary(std::cout, cell);
        std::cout << '\n';
    }
}

void inspect_table_cell(std::size_t table_index,
                        const featherdoc::table_cell_inspection_summary &cell,
                        bool json_output) {
    if (json_output) {
        std::cout << "{\"table_index\":" << table_index << ",\"cell\":";
        write_json_table_cell_summary(std::cout, cell);
        std::cout << "}\n";
        return;
    }

    std::cout << "table_index: " << table_index << '\n'
              << "row_index: " << cell.row_index << '\n'
              << "cell_index: " << cell.cell_index << '\n'
              << "column_index: " << cell.column_index << '\n'
              << "column_span: " << cell.column_span << '\n'
              << "paragraph_count: " << cell.paragraph_count << '\n'
              << "width_twips: ";
    if (cell.width_twips.has_value()) {
        std::cout << *cell.width_twips;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "vertical_alignment: ";
    if (cell.vertical_alignment.has_value()) {
        std::cout << cell_vertical_alignment_name(*cell.vertical_alignment);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text_direction: ";
    if (cell.text_direction.has_value()) {
        std::cout << cell_text_direction_name(*cell.text_direction);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text: " << format_paragraph_text(cell.text) << '\n';
}

void inspect_table_rows(std::size_t table_index,
                        const std::vector<table_row_inspection_summary> &rows,
                        bool json_output) {
    if (json_output) {
        std::cout << "{\"table_index\":" << table_index
                  << ",\"count\":" << rows.size() << ",\"rows\":[";
        for (std::size_t index = 0; index < rows.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_row_summary(std::cout, rows[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "table_index: " << table_index << '\n'
              << "rows: " << rows.size() << '\n';
    for (const auto &row : rows) {
        print_table_row_summary(std::cout, row);
        std::cout << '\n';
    }
}

void inspect_table_row(std::size_t table_index,
                       const table_row_inspection_summary &row,
                       bool json_output) {
    if (json_output) {
        std::cout << "{\"table_index\":" << table_index << ",\"row\":";
        write_json_table_row_summary(std::cout, row);
        std::cout << "}\n";
        return;
    }

    std::cout << "table_index: " << table_index << '\n'
              << "row_index: " << row.row_index << '\n'
              << "cell_count: " << row.cell_count << '\n'
              << "height_twips: ";
    if (row.height_twips.has_value()) {
        std::cout << *row.height_twips;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "height_rule: ";
    if (row.height_rule.has_value()) {
        std::cout << row_height_rule_name(*row.height_rule);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "cant_split: " << yes_no(row.cant_split) << '\n'
              << "repeats_header: " << yes_no(row.repeats_header) << '\n'
              << "cell_texts: [";
    for (std::size_t index = 0; index < row.cell_texts.size(); ++index) {
        if (index != 0U) {
            std::cout << ", ";
        }
        std::cout << format_paragraph_text(row.cell_texts[index]);
    }
    std::cout << "]\n";
}

} // namespace

void inspect_template_tables(
    const selected_template_part &selected,
    const std::vector<featherdoc::table_inspection_summary> &tables,
    bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"count\":" << tables.size() << ",\"tables\":[";
        for (std::size_t index = 0; index < tables.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_summary(std::cout, tables[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    std::cout << "tables: " << tables.size() << '\n';
    for (const auto &table : tables) {
        print_table_summary(std::cout, table);
        std::cout << '\n';
    }
}

void inspect_template_table(const selected_template_part &selected,
                            const featherdoc::table_inspection_summary &table,
                            bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"table\":";
        write_json_table_summary(std::cout, table);
        std::cout << "}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    inspect_table(table, false);
}

void inspect_template_table_cells(
    const selected_template_part &selected, std::size_t table_index,
    std::optional<std::size_t> row_index,
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"table_index\":" << table_index;
        if (row_index.has_value()) {
            std::cout << ",\"row_index\":" << *row_index;
        }
        std::cout << ",\"count\":" << cells.size() << ",\"cells\":[";
        for (std::size_t index = 0; index < cells.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_cell_summary(std::cout, cells[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    inspect_table_cells(table_index, row_index, cells, false);
}

void inspect_template_table_cell(
    const selected_template_part &selected, std::size_t table_index,
    const featherdoc::table_cell_inspection_summary &cell, bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"table_index\":" << table_index << ",\"cell\":";
        write_json_table_cell_summary(std::cout, cell);
        std::cout << "}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    inspect_table_cell(table_index, cell, false);
}

void inspect_template_table_rows(
    const selected_template_part &selected, std::size_t table_index,
    const std::vector<table_row_inspection_summary> &rows, bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"table_index\":" << table_index
                  << ",\"count\":" << rows.size() << ",\"rows\":[";
        for (std::size_t index = 0; index < rows.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_row_summary(std::cout, rows[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    inspect_table_rows(table_index, rows, false);
}

void inspect_template_table_row(const selected_template_part &selected,
                                std::size_t table_index,
                                const table_row_inspection_summary &row,
                                bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"table_index\":" << table_index << ",\"row\":";
        write_json_table_row_summary(std::cout, row);
        std::cout << "}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    inspect_table_row(table_index, row, false);
}
} // namespace featherdoc_cli
