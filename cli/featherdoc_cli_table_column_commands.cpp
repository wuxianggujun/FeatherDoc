#include "featherdoc_cli_table_column_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_table_column_support.hpp"
#include "featherdoc_cli_table_structure_validation.hpp"

#include <featherdoc.hpp>

#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

auto run_remove_table_column_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    table_column_target target;
    table_cell_style_options options;
    if (!parse_body_table_column_target(
            command, arguments,
            "remove-table-column expects an input path, a table index, a row "
            "index, and a cell index",
            target, options)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::table_inspection_summary inspected_table{};
    if (!load_body_table_summary_for_column(command, doc, target.table_index,
                                            inspected_table,
                                            options.json_output)) {
        return 1;
    }

    featherdoc::table_cell_inspection_summary inspected_cell{};
    if (!load_body_table_cell_summary_for_column(command, doc, target,
                                                 inspected_cell,
                                                 options.json_output)) {
        return 1;
    }

    const auto removed_column_index = inspected_cell.column_index;
    if (inspected_table.column_count <= 1U) {
        report_body_table_column_failure(
            command, "failed to remove table column",
            "cannot remove the last column from table index '" +
                std::to_string(target.table_index) + "'",
            options.json_output);
        return 1;
    }

    std::vector<featherdoc::table_cell_inspection_summary> inspected_cells;
    if (!load_body_table_cells_summary_for_column(command, doc,
                                                  target.table_index,
                                                  inspected_cells,
                                                  options.json_output)) {
        return 1;
    }

    if (column_intersects_horizontal_merge(inspected_cells,
                                           removed_column_index)) {
        report_body_table_column_failure(
            command, "failed to remove table column",
            "cannot remove column index '" +
                std::to_string(removed_column_index) + "' from table index '" +
                std::to_string(target.table_index) +
                "' because it intersects a horizontal merge span",
            options.json_output);
        return 1;
    }

    featherdoc::TableCell cell;
    if (!resolve_body_table_cell_for_column(command, doc, target, cell,
                                            options.json_output)) {
        return 1;
    }

    if (!cell.remove()) {
        report_body_table_column_failure(
            command, "failed to remove table column",
            "failed to remove the requested column from the body table",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&target, removed_column_index](std::ostream &stream) {
                stream << ",\"table_index\":" << target.table_index
                       << ",\"row_index\":" << target.row_index
                       << ",\"cell_index\":" << target.cell_index
                       << ",\"column_index\":" << removed_column_index;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
