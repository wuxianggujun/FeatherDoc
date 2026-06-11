#include "featherdoc_cli_table_column_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_table_column_support.hpp"
#include "featherdoc_cli_table_structure_validation.hpp"

#include <featherdoc.hpp>

#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {
namespace {

auto run_insert_table_column_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    bool insert_before) -> int {
    table_column_target target;
    table_cell_style_options options;
    if (!parse_body_table_column_target(
            command, arguments,
            insert_before
                ? "insert-table-column-before expects an input path, a table "
                  "index, a row index, and a cell index"
                : "insert-table-column-after expects an input path, a table "
                  "index, a row index, and a cell index",
            target, options)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::table_cell_inspection_summary inspected_cell{};
    if (!load_body_table_cell_summary_for_column(command, doc, target,
                                                 inspected_cell,
                                                 options.json_output)) {
        return 1;
    }

    std::vector<featherdoc::table_cell_inspection_summary> inspected_cells;
    if (!load_body_table_cells_summary_for_column(command, doc,
                                                  target.table_index,
                                                  inspected_cells,
                                                  options.json_output)) {
        return 1;
    }

    const auto inserted_column_index =
        insert_before ? inspected_cell.column_index
                      : inspected_cell.column_index + inspected_cell.column_span;
    if (insertion_boundary_intersects_horizontal_merge(inspected_cells,
                                                       inserted_column_index)) {
        report_body_table_column_failure(
            command, "failed to insert table column",
            "cannot insert a column " +
                std::string(insert_before ? "before" : "after") +
                " cell index '" + std::to_string(target.cell_index) +
                "' at row index '" + std::to_string(target.row_index) +
                "' in table index '" + std::to_string(target.table_index) +
                "' because the insertion boundary intersects a horizontal "
                "merge span",
            options.json_output);
        return 1;
    }

    featherdoc::TableCell cell;
    if (!resolve_body_table_cell_for_column(command, doc, target, cell,
                                            options.json_output)) {
        return 1;
    }

    auto inserted_cell =
        insert_before ? cell.insert_cell_before() : cell.insert_cell_after();
    if (!inserted_cell.has_next()) {
        report_body_table_column_failure(
            command, "failed to insert table column",
            "failed to create a cloned column " +
                std::string(insert_before ? "before" : "after") +
                " the requested cell",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&target, insert_before,
             inserted_column_index](std::ostream &stream) {
                stream << ",\"table_index\":" << target.table_index
                       << ",\"row_index\":" << target.row_index
                       << ",\"cell_index\":" << target.cell_index
                       << ",\"inserted_cell_index\":"
                       << (insert_before ? target.cell_index
                                         : target.cell_index + 1U)
                       << ",\"inserted_column_index\":"
                       << inserted_column_index;
            });
    }

    return 0;
}

} // namespace

auto run_insert_table_column_before_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_insert_table_column_command(command, arguments, true);
}

auto run_insert_table_column_after_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_insert_table_column_command(command, arguments, false);
}

} // namespace featherdoc_cli
