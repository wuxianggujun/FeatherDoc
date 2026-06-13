#include "featherdoc_cli_template_table_column_commands.hpp"

#include "featherdoc_cli_template_table_column_commands_detail.hpp"
#include "featherdoc_cli_table_structure_validation.hpp"

#include <featherdoc.hpp>

#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

auto run_remove_template_table_column_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    parsed_template_table_selector_cell_target target;
    template_table_cell_mutation_options options;
    if (!parse_template_table_column_command(
            command, arguments,
            "remove-template-table-column expects an input path plus either "
            "<table-index> <row-index> <cell-index>, "
            "--bookmark <name> <row-index> <cell-index>, or a "
            "text-based table selector followed by <row-index> <cell-index>",
            target, options)) {
        return 2;
    }

    std::string error_message;
    template_table_column_context context;
    if (!load_template_table_column_context(command, arguments, options,
                                            target.selector, context,
                                            error_message)) {
        return 1;
    }

    featherdoc::table_inspection_summary inspected_table{};
    if (!load_template_table_summary(context.doc, context.selected,
                                     context.table_index, inspected_table,
                                     command, options.json_output)) {
        return 1;
    }

    featherdoc::table_cell_inspection_summary inspected_cell{};
    if (!load_template_table_cell_summary(
            context.doc, context.selected, context.table_index, target.row_index,
            target.cell_index, inspected_cell, command, options.json_output)) {
        return 1;
    }

    const auto removed_column_index = inspected_cell.column_index;
    if (inspected_table.column_count <= 1U) {
        report_template_column_failure(
            command, "failed to remove table column",
            "cannot remove the last column from table index '" +
                std::to_string(context.table_index) + "'",
            context.selected, options.json_output);
        return 1;
    }

    std::vector<featherdoc::table_cell_inspection_summary> inspected_cells;
    if (!load_template_table_cells_summary(context.doc, context.selected,
                                           context.table_index, inspected_cells,
                                           command, options.json_output)) {
        return 1;
    }

    if (column_intersects_horizontal_merge(inspected_cells,
                                           removed_column_index)) {
        report_template_column_failure(
            command, "failed to remove table column",
            "cannot remove column index '" +
                std::to_string(removed_column_index) + "' from table index '" +
                std::to_string(context.table_index) +
                "' because it intersects a horizontal merge span",
            context.selected, options.json_output);
        return 1;
    }

    featherdoc::TableCell cell;
    if (!resolve_template_table_cell(context.selected, context.table_index,
                                     target.row_index, target.cell_index, cell,
                                     command, options.json_output)) {
        return 1;
    }

    if (!cell.remove()) {
        report_template_column_failure(
            command, "failed to remove table column",
            "failed to remove the requested column from the template table",
            context.selected, options.json_output);
        return 1;
    }

    if (!save_document(context.doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, context.doc, options.output_path,
            [&context, &target, removed_column_index](std::ostream &stream) {
                stream << ',';
                write_json_selected_template_part(stream, context.selected);
                stream << ",\"table_index\":" << context.table_index
                       << ",\"row_index\":" << target.row_index
                       << ",\"cell_index\":" << target.cell_index
                       << ",\"column_index\":" << removed_column_index;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
