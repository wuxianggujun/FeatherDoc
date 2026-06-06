#include "featherdoc_cli_table_inspect_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_content_options_parse.hpp"
#include "featherdoc_cli_inspect_table_item_options_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_row_summary.hpp"
#include "featherdoc_cli_text.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_optional_table_overlap(
    std::ostream &stream,
    const std::optional<featherdoc::table_overlap> &overlap) {
    if (overlap.has_value()) {
        write_json_string(stream, table_overlap_name(*overlap));
    } else {
        stream << "null";
    }
}

void write_json_optional_table_position_horizontal_spec(
    std::ostream &stream,
    const std::optional<featherdoc::table_position_horizontal_spec> &spec) {
    if (spec.has_value()) {
        write_json_string(stream, table_position_horizontal_spec_name(*spec));
    } else {
        stream << "null";
    }
}

void write_json_optional_table_position_vertical_spec(
    std::ostream &stream,
    const std::optional<featherdoc::table_position_vertical_spec> &spec) {
    if (spec.has_value()) {
        write_json_string(stream, table_position_vertical_spec_name(*spec));
    } else {
        stream << "null";
    }
}

void write_json_table_position(std::ostream &stream,
                               const featherdoc::table_position &position) {
    stream << "{\"horizontal_reference\":";
    write_json_string(
        stream,
        table_position_horizontal_reference_name(position.horizontal_reference));
    stream << ",\"horizontal_offset_twips\":"
           << position.horizontal_offset_twips << ",\"horizontal_spec\":";
    write_json_optional_table_position_horizontal_spec(stream,
                                                       position.horizontal_spec);
    stream << ",\"vertical_reference\":";
    write_json_string(stream,
                      table_position_vertical_reference_name(
                          position.vertical_reference));
    stream << ",\"vertical_offset_twips\":" << position.vertical_offset_twips
           << ",\"vertical_spec\":";
    write_json_optional_table_position_vertical_spec(stream, position.vertical_spec);
    stream << ",\"left_from_text_twips\":";
    write_json_optional_u32_value(stream, position.left_from_text_twips);
    stream << ",\"right_from_text_twips\":";
    write_json_optional_u32_value(stream, position.right_from_text_twips);
    stream << ",\"top_from_text_twips\":";
    write_json_optional_u32_value(stream, position.top_from_text_twips);
    stream << ",\"bottom_from_text_twips\":";
    write_json_optional_u32_value(stream, position.bottom_from_text_twips);
    stream << ",\"overlap\":";
    write_json_optional_table_overlap(stream, position.overlap);
    stream << '}';
}

void write_json_optional_table_position(
    std::ostream &stream,
    const std::optional<featherdoc::table_position> &position) {
    if (position.has_value()) {
        write_json_table_position(stream, *position);
    } else {
        stream << "null";
    }
}

void write_table_position_text(
    std::ostream &stream,
    const std::optional<featherdoc::table_position> &position) {
    if (!position.has_value()) {
        stream << "none";
        return;
    }

    stream << table_position_horizontal_reference_name(
                  position->horizontal_reference)
           << ':' << position->horizontal_offset_twips;
    if (position->horizontal_spec.has_value()) {
        stream << ':' << table_position_horizontal_spec_name(
                              *position->horizontal_spec);
    }
    stream << ','
           << table_position_vertical_reference_name(position->vertical_reference)
           << ':' << position->vertical_offset_twips;
    if (position->vertical_spec.has_value()) {
        stream << ':' << table_position_vertical_spec_name(*position->vertical_spec);
    }
    if (position->left_from_text_twips.has_value() ||
        position->right_from_text_twips.has_value() ||
        position->top_from_text_twips.has_value() ||
        position->bottom_from_text_twips.has_value()) {
        stream << " wrap=";
        write_json_optional_u32_value(stream, position->left_from_text_twips);
        stream << '/';
        write_json_optional_u32_value(stream, position->right_from_text_twips);
        stream << '/';
        write_json_optional_u32_value(stream, position->top_from_text_twips);
        stream << '/';
        write_json_optional_u32_value(stream, position->bottom_from_text_twips);
    }
    if (position->overlap.has_value()) {
        stream << " overlap=" << table_overlap_name(*position->overlap);
    }
}

void write_json_border_inspection_summary(
    std::ostream &stream, const featherdoc::border_inspection_summary &border) {
    stream << "{\"style\":";
    write_json_string(stream, border_style_name(border.style));
    stream << ",\"size_eighth_points\":" << border.size_eighth_points
           << ",\"color\":";
    write_json_string(stream, border.color);
    stream << ",\"space_points\":" << border.space_points << '}';
}

void write_json_optional_border_inspection_summary(
    std::ostream &stream,
    const std::optional<featherdoc::border_inspection_summary> &border) {
    if (!border.has_value()) {
        stream << "null";
        return;
    }

    write_json_border_inspection_summary(stream, *border);
}

void write_json_table_borders_inspection_summary(
    std::ostream &stream,
    const featherdoc::table_borders_inspection_summary &borders) {
    stream << "{\"top\":";
    write_json_optional_border_inspection_summary(stream, borders.top);
    stream << ",\"left\":";
    write_json_optional_border_inspection_summary(stream, borders.left);
    stream << ",\"bottom\":";
    write_json_optional_border_inspection_summary(stream, borders.bottom);
    stream << ",\"right\":";
    write_json_optional_border_inspection_summary(stream, borders.right);
    stream << ",\"inside_horizontal\":";
    write_json_optional_border_inspection_summary(stream, borders.inside_horizontal);
    stream << ",\"inside_vertical\":";
    write_json_optional_border_inspection_summary(stream, borders.inside_vertical);
    stream << '}';
}

void write_json_optional_table_borders_inspection_summary(
    std::ostream &stream,
    const std::optional<featherdoc::table_borders_inspection_summary> &borders) {
    if (!borders.has_value()) {
        stream << "null";
        return;
    }

    write_json_table_borders_inspection_summary(stream, *borders);
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
    write_json_optional_table_borders_inspection_summary(stream, table.borders);
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

void inspect_tables(const std::vector<featherdoc::table_inspection_summary> &tables,
                    bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << tables.size() << ",\"tables\":[";
        for (std::size_t index = 0; index < tables.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_summary(std::cout, tables[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "tables: " << tables.size() << '\n';
    for (const auto &table : tables) {
        print_table_summary(std::cout, table);
        std::cout << '\n';
    }
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

auto report_body_table_range_failure(
    std::string_view command, std::string_view summary, std::string detail,
    bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "inspect", summary, error_info,
                                    json_output);
}

auto resolve_body_table_for_inspect(
    std::string_view command, featherdoc::Document &doc, std::size_t table_index,
    featherdoc::Table &table, bool json_output) -> bool {
    table = doc.tables();
    for (std::size_t current_index = 0;
         current_index < table_index && table.has_next();
         ++current_index) {
        table.next();
    }

    if (table.has_next()) {
        return true;
    }

    return report_body_table_range_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto resolve_body_table_row_for_inspect(
    std::string_view command, featherdoc::Document &doc, std::size_t table_index,
    std::size_t row_index, featherdoc::TableRow &row, bool json_output) -> bool {
    featherdoc::Table table;
    if (!resolve_body_table_for_inspect(command, doc, table_index, table,
                                        json_output)) {
        return false;
    }

    row = table.rows();
    for (std::size_t current_index = 0;
         current_index < row_index && row.has_next();
         ++current_index) {
        row.next();
    }

    if (row.has_next()) {
        return true;
    }

    return report_body_table_range_failure(
        command, "row index is out of range",
        "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'",
        json_output);
}

auto parse_body_table_index(std::string_view command,
                            const std::vector<std::string_view> &arguments,
                            std::size_t &table_index, bool json_output) -> bool {
    if (!parse_index(arguments[2], table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return false;
    }

    return true;
}

} // namespace

auto run_inspect_tables_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-tables expects an input path",
                          json_output);
        return 2;
    }

    inspect_tables_options options;
    std::string error_message;
    if (!parse_inspect_tables_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    const auto input_path = path_type(std::string(arguments[1]));
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    const auto tables = doc.inspect_tables();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    if (options.table_index.has_value()) {
        if (*options.table_index >= tables.size()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "table index '" + std::to_string(*options.table_index) +
                "' is out of range";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "inspect",
                                     "table index is out of range", error_info,
                                     options.json_output);
            return 1;
        }

        inspect_table(tables[*options.table_index], options.json_output);
        return 0;
    }

    inspect_tables(tables, options.json_output);
    return 0;
}

auto run_inspect_table_rows_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-table-rows expects an input path and a table index",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_body_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    inspect_table_rows_options options;
    std::string error_message;
    if (!parse_inspect_table_rows_options(arguments, 3U, options,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table_for_inspect(command, doc, table_index, table,
                                        options.json_output)) {
        return 1;
    }

    if (options.row_index.has_value()) {
        featherdoc::TableRow row;
        if (!resolve_body_table_row_for_inspect(
                command, doc, table_index, *options.row_index, row,
                options.json_output)) {
            return 1;
        }

        inspect_table_row(table_index,
                          make_table_row_summary(row, *options.row_index),
                          options.json_output);
        return 0;
    }

    inspect_table_rows(table_index, collect_table_row_summaries(table),
                       options.json_output);
    return 0;
}

auto run_inspect_table_cells_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-table-cells expects an input path and a table index",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_body_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    inspect_table_cells_options options;
    std::string error_message;
    if (!parse_inspect_table_cells_options(arguments, 3U, options,
                                           error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    const auto input_path = path_type(std::string(arguments[1]));
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    const auto table = doc.inspect_table(table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }
    if (!table.has_value()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail =
            "table index '" + std::to_string(table_index) + "' is out of range";
        error_info.entry_name = "word/document.xml";
        report_operation_failure(command, "inspect",
                                 "table index is out of range", error_info,
                                 options.json_output);
        return 1;
    }

    if (options.row_index.has_value() && options.cell_index.has_value()) {
        const auto cell =
            doc.inspect_table_cell(table_index, *options.row_index,
                                   *options.cell_index);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }
        if (!cell.has_value()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            if (*options.row_index >= table->row_count) {
                error_info.detail =
                    "row index '" + std::to_string(*options.row_index) +
                    "' is out of range for table index '" +
                    std::to_string(table_index) + "'";
            } else {
                error_info.detail =
                    "cell index '" + std::to_string(*options.cell_index) +
                    "' is out of range for row index '" +
                    std::to_string(*options.row_index) +
                    "' in table index '" + std::to_string(table_index) + "'";
            }
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "inspect",
                                     "table cell selector is out of range",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_table_cell(table_index, *cell, options.json_output);
        return 0;
    }

    if (options.row_index.has_value() && options.grid_column.has_value()) {
        const auto cell = doc.inspect_table_cell_by_grid_column(
            table_index, *options.row_index, *options.grid_column);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }
        if (!cell.has_value()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            if (*options.row_index >= table->row_count) {
                error_info.detail =
                    "row index '" + std::to_string(*options.row_index) +
                    "' is out of range for table index '" +
                    std::to_string(table_index) + "'";
            } else {
                error_info.detail =
                    "grid column '" + std::to_string(*options.grid_column) +
                    "' is out of range for row index '" +
                    std::to_string(*options.row_index) +
                    "' in table index '" + std::to_string(table_index) + "'";
            }
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "inspect",
                                     "table cell selector is out of range",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_table_cell(table_index, *cell, options.json_output);
        return 0;
    }

    auto cells = doc.inspect_table_cells(table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    if (options.row_index.has_value()) {
        std::vector<featherdoc::table_cell_inspection_summary> row_cells;
        for (const auto &cell : cells) {
            if (cell.row_index == *options.row_index) {
                row_cells.push_back(cell);
            }
        }

        if (row_cells.empty() && *options.row_index >= table->row_count) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "row index '" + std::to_string(*options.row_index) +
                "' is out of range for table index '" +
                std::to_string(table_index) + "'";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "inspect",
                                     "row index is out of range",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_table_cells(table_index, options.row_index, row_cells,
                            options.json_output);
        return 0;
    }

    inspect_table_cells(table_index, std::nullopt, cells, options.json_output);
    return 0;
}

} // namespace featherdoc_cli
