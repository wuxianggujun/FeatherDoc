#include "featherdoc_cli_table_column_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"
#include "featherdoc_cli_table_structure_validation.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

struct table_column_target {
    std::size_t table_index = 0U;
    std::size_t row_index = 0U;
    std::size_t cell_index = 0U;
};

auto report_body_table_column_failure(std::string_view command,
                                      std::string_view summary,
                                      std::string detail, bool json_output)
    -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto parse_body_table_column_index(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t argument_index, std::string_view label, std::size_t &value,
    bool json_output) -> bool {
    if (parse_index(arguments[argument_index], value)) {
        return true;
    }

    print_parse_error(command,
                      "invalid " + std::string(label) + ": " +
                          std::string(arguments[argument_index]),
                      json_output);
    return false;
}

auto parse_body_table_column_target(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::string_view usage_message, table_column_target &target,
    table_cell_style_options &options) -> bool {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(command, std::string(usage_message), json_output);
        return false;
    }

    if (!parse_body_table_column_index(command, arguments, 2U, "table index",
                                       target.table_index, json_output)) {
        return false;
    }

    if (!parse_body_table_column_index(command, arguments, 3U, "row index",
                                       target.row_index, json_output)) {
        return false;
    }

    if (!parse_body_table_column_index(command, arguments, 4U, "cell index",
                                       target.cell_index, json_output)) {
        return false;
    }

    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 5U, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    return true;
}

auto resolve_body_table_for_column(std::string_view command,
                                   featherdoc::Document &doc,
                                   std::size_t table_index,
                                   featherdoc::Table &table,
                                   bool json_output) -> bool {
    table = doc.tables();
    for (std::size_t current_index = 0;
         current_index < table_index && table.has_next();
         ++current_index) {
        table.next();
    }

    if (table.has_next()) {
        return true;
    }

    return report_body_table_column_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto resolve_body_table_row_for_column(std::string_view command,
                                       featherdoc::Document &doc,
                                       std::size_t table_index,
                                       std::size_t row_index,
                                       featherdoc::TableRow &row,
                                       bool json_output) -> bool {
    featherdoc::Table table;
    if (!resolve_body_table_for_column(command, doc, table_index, table,
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

    return report_body_table_column_failure(
        command, "row index is out of range",
        "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'",
        json_output);
}

auto resolve_body_table_cell_for_column(std::string_view command,
                                        featherdoc::Document &doc,
                                        const table_column_target &target,
                                        featherdoc::TableCell &cell,
                                        bool json_output) -> bool {
    featherdoc::TableRow row;
    if (!resolve_body_table_row_for_column(command, doc, target.table_index,
                                           target.row_index, row,
                                           json_output)) {
        return false;
    }

    cell = row.cells();
    for (std::size_t current_index = 0;
         current_index < target.cell_index && cell.has_next();
         ++current_index) {
        cell.next();
    }

    if (cell.has_next()) {
        return true;
    }

    return report_body_table_column_failure(
        command, "cell index is out of range",
        "cell index '" + std::to_string(target.cell_index) +
            "' is out of range for row index '" +
            std::to_string(target.row_index) + "' in table index '" +
            std::to_string(target.table_index) + "'",
        json_output);
}

auto load_body_table_summary_for_column(
    std::string_view command, featherdoc::Document &doc,
    std::size_t table_index, featherdoc::table_inspection_summary &table,
    bool json_output) -> bool {
    const auto inspected_table = doc.inspect_table(table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, json_output);
        return false;
    }

    if (inspected_table.has_value()) {
        table = *inspected_table;
        return true;
    }

    return report_body_table_column_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto load_body_table_cell_summary_for_column(
    std::string_view command, featherdoc::Document &doc,
    const table_column_target &target,
    featherdoc::table_cell_inspection_summary &cell, bool json_output) -> bool {
    featherdoc::table_inspection_summary table{};
    if (!load_body_table_summary_for_column(command, doc, target.table_index,
                                            table, json_output)) {
        return false;
    }

    const auto inspected_cell =
        doc.inspect_table_cell(target.table_index, target.row_index,
                               target.cell_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, json_output);
        return false;
    }

    if (inspected_cell.has_value()) {
        cell = *inspected_cell;
        return true;
    }

    if (target.row_index >= table.row_count) {
        return report_body_table_column_failure(
            command, "row index is out of range",
            "row index '" + std::to_string(target.row_index) +
                "' is out of range for table index '" +
                std::to_string(target.table_index) + "'",
            json_output);
    }

    return report_body_table_column_failure(
        command, "cell index is out of range",
        "cell index '" + std::to_string(target.cell_index) +
            "' is out of range for row index '" +
            std::to_string(target.row_index) + "' in table index '" +
            std::to_string(target.table_index) + "'",
        json_output);
}

auto load_body_table_cells_summary_for_column(
    std::string_view command, featherdoc::Document &doc,
    std::size_t table_index,
    std::vector<featherdoc::table_cell_inspection_summary> &cells,
    bool json_output) -> bool {
    featherdoc::table_inspection_summary table{};
    if (!load_body_table_summary_for_column(command, doc, table_index, table,
                                            json_output)) {
        return false;
    }

    cells = doc.inspect_table_cells(table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, json_output);
        return false;
    }

    return true;
}

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
