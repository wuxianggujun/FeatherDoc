#include "featherdoc_cli_template_table_column_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_structure_validation.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_table_mutation_options_parse.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_template_table_resolve.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

struct template_table_column_context {
    featherdoc::Document doc;
    selected_template_part selected;
    std::size_t table_index = 0U;
};

auto merge_target_bookmark(parsed_template_table_selector_cell_target &target,
                           const template_table_cell_mutation_options &options)
    -> void {
    if (!target.selector.bookmark_name.has_value() &&
        options.bookmark_name.has_value()) {
        target.selector.bookmark_name = options.bookmark_name;
    }
}

auto validate_target_selector(
    std::string_view command, bool json_output,
    parsed_template_table_selector_cell_target &target,
    std::string &error_message) -> bool {
    if (!validate_template_table_selector(target.selector, false,
                                          target.has_header_row_index,
                                          target.has_occurrence,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    return true;
}

auto load_template_table_column_context(
    std::string_view command, const std::vector<std::string_view> &arguments,
    const template_table_cell_mutation_options &options,
    const featherdoc::template_table_selector &selector,
    template_table_column_context &context, std::string &error_message) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), context.doc,
                       command, options.json_output)) {
        return false;
    }

    if (!select_mutable_template_part(context.doc, options.part,
                                      options.part_index,
                                      options.section_index,
                                      options.reference_kind, context.selected,
                                      error_message)) {
        report_operation_failure(command, "mutate", error_message,
                                 context.doc.last_error(), options.json_output);
        return false;
    }

    if (!resolve_template_table_index(context.doc, context.selected, selector,
                                      context.table_index, command,
                                      options.json_output, "mutate")) {
        return false;
    }

    return true;
}

auto report_template_column_failure(std::string_view command,
                                    std::string_view summary,
                                    std::string detail,
                                    const selected_template_part &selected,
                                    bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto parse_template_table_column_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::string_view usage_message,
    parsed_template_table_selector_cell_target &target,
    template_table_cell_mutation_options &options) -> bool {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(command, std::string(usage_message), json_output);
        return false;
    }

    std::string error_message;
    if (!parse_template_table_selector_cell_target_arguments(
            arguments, 2U, target, error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    options.bookmark_name = target.selector.bookmark_name;
    if (!parse_template_table_cell_mutation_options(
            arguments, target.options_start_index, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    merge_target_bookmark(target, options);
    return validate_target_selector(command, json_output, target, error_message);
}

} // namespace

auto run_insert_template_table_column_before_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    parsed_template_table_selector_cell_target target;
    template_table_cell_mutation_options options;
    if (!parse_template_table_column_command(
            command, arguments,
            "insert-template-table-column-before expects an input path plus "
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

    const auto inserted_column_index = inspected_cell.column_index;
    if (insertion_boundary_intersects_horizontal_merge(inspected_cells,
                                                       inserted_column_index)) {
        report_template_column_failure(
            command, "failed to insert table column",
            "cannot insert a column before cell index '" +
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

    auto inserted_cell = cell.insert_cell_before();
    if (!inserted_cell.has_next()) {
        report_template_column_failure(
            command, "failed to insert table column",
            "failed to create a cloned column before the requested cell",
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
                       << ",\"inserted_cell_index\":" << target.cell_index
                       << ",\"inserted_column_index\":"
                       << inserted_column_index;
            });
    }

    return 0;
}

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
