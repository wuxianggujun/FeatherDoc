#pragma once

#include "featherdoc_cli_table_row_summary.hpp"

#include <featherdoc.hpp>

#include <iosfwd>
#include <optional>

namespace featherdoc_cli {

void write_json_table_summary(
    std::ostream &stream, const featherdoc::table_inspection_summary &table);

void write_json_table_cell_summary(
    std::ostream &stream,
    const featherdoc::table_cell_inspection_summary &cell);

void write_json_table_row_summary(
    std::ostream &stream, const table_row_inspection_summary &row);

void write_inspect_table_position_text(
    std::ostream &stream,
    const std::optional<featherdoc::table_position> &position);

void print_table_summary(std::ostream &stream,
                         const featherdoc::table_inspection_summary &table);

void print_table_cell_summary(
    std::ostream &stream,
    const featherdoc::table_cell_inspection_summary &cell);

void print_table_row_summary(std::ostream &stream,
                             const table_row_inspection_summary &row);

} // namespace featherdoc_cli
