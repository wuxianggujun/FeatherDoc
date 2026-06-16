#include "featherdoc_cli_template_table_column_commands.hpp"

#include "featherdoc_cli_template_table_column_commands_detail.hpp"
#include "featherdoc_cli_table_structure_validation.hpp"

#include <featherdoc.hpp>

#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

auto run_insert_template_table_column_after_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    parsed_template_table_selector_cell_target target;
    template_table_cell_mutation_options options;
    if (!parse_template_table_column_command(
            command, arguments,
            "insert-template-table-column-after expects an input path plus "
            "either <table-index> <row-index> <cell-index>, "
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

    featherdoc::table_cell_inspection_summary inspected_cell{};
    if (!load_template_table_cell_summary(
            context.doc, context.selected, context.table_index, target.row_index,
            target.cell_index, inspected_cell, command, options.json_output)) {
        return 1;
    }

    std::vector<featherdoc::table_cell_inspection_summary> inspected_cells;
    if (!load_template_table_cells_summary(context.doc, context.selected,
                                           context.table_index, inspected_cells,
                                           command, options.json_output)) {
        return 1;
    }

    const auto inserted_column_index =
        inspected_cell.column_index + inspected_cell.column_span;
    if (insertion_boundary_intersects_horizontal_merge(inspected_cells,
                                                       inserted_column_index)) {
        report_template_column_failure(
            command, "failed to insert table column",
            "cannot insert a column after cell index '" +
                std::to_string(target.cell_index) + "' at row index '" +
                std::to_string(target.row_index) + "' in table index '" +
                std::to_string(context.table_index) +
                "' because the insertion boundary intersects a horizontal "
                "merge span",
            context.selected, options.json_output);
        return 1;
    }

    featherdoc::TableCell cell;
    if (!resolve_template_table_cell(context.selected, context.table_index,
                                     target.row_index, target.cell_index, cell,
                                     command, options.json_output)) {
        return 1;
    }

    auto inserted_cell = cell.insert_cell_after();
    if (!inserted_cell.has_next()) {
        report_template_column_failure(
            command, "failed to insert table column",
            "failed to create a cloned column after the requested cell",
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
            [&context, &target, inserted_column_index](std::ostream &stream) {
                stream << ',';
                write_json_selected_template_part(stream, context.selected);
                stream << ",\"table_index\":" << context.table_index
                       << ",\"row_index\":" << target.row_index
                       << ",\"cell_index\":" << target.cell_index
                       << ",\"inserted_cell_index\":"
                       << (target.cell_index + 1U)
                       << ",\"inserted_column_index\":"
                       << inserted_column_index;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
