#include "featherdoc_cli_table_column_support.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

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

} // namespace

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

} // namespace featherdoc_cli
