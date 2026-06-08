#include "featherdoc_cli_table_inspect_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_content_options_parse.hpp"
#include "featherdoc_cli_inspect_table_item_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_inspect_output.hpp"
#include "featherdoc_cli_table_row_summary.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace featherdoc_cli {
namespace {

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
