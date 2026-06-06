#include "featherdoc_cli_table_row_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto report_body_table_row_failure(std::string_view command,
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

auto parse_body_table_index(std::string_view command,
                            const std::vector<std::string_view> &arguments,
                            std::size_t argument_index,
                            std::string_view label,
                            std::size_t &value, bool json_output) -> bool {
    if (parse_index(arguments[argument_index], value)) {
        return true;
    }

    print_parse_error(command,
                      "invalid " + std::string(label) + ": " +
                          std::string(arguments[argument_index]),
                      json_output);
    return false;
}

auto resolve_body_table_for_row(std::string_view command,
                                featherdoc::Document &doc,
                                std::size_t table_index,
                                featherdoc::Table &table, bool json_output)
    -> bool {
    table = doc.tables();
    for (std::size_t current_index = 0;
         current_index < table_index && table.has_next();
         ++current_index) {
        table.next();
    }

    if (table.has_next()) {
        return true;
    }

    return report_body_table_row_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto resolve_body_table_row_for_row(std::string_view command,
                                    featherdoc::Document &doc,
                                    std::size_t table_index,
                                    std::size_t row_index,
                                    featherdoc::TableRow &row,
                                    bool json_output) -> bool {
    featherdoc::Table table;
    if (!resolve_body_table_for_row(command, doc, table_index, table,
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

    return report_body_table_row_failure(
        command, "row index is out of range",
        "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'",
        json_output);
}

auto load_body_table_summary(std::string_view command, featherdoc::Document &doc,
                             std::size_t table_index,
                             featherdoc::table_inspection_summary &table,
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

    return report_body_table_row_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto run_insert_table_row_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    bool insert_before) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command,
                          std::string(command) +
                              " expects an input path, a table index, and a row index",
                          json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_body_table_index(command, arguments, 2U, "table index",
                                table_index, json_output)) {
        return 2;
    }

    std::size_t row_index = 0U;
    if (!parse_body_table_index(command, arguments, 3U, "row index", row_index,
                                json_output)) {
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 4U, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::TableRow row;
    if (!resolve_body_table_row_for_row(command, doc, table_index, row_index,
                                        row, options.json_output)) {
        return 1;
    }

    auto inserted_row =
        insert_before ? row.insert_row_before() : row.insert_row_after();
    if (!inserted_row.has_next()) {
        report_body_table_row_failure(
            command, "failed to insert table row",
            "cannot insert a row adjacent to row index '" +
                std::to_string(row_index) + "' in table index '" +
                std::to_string(table_index) +
                "' because the row participates in a vertical merge",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, insert_before](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"row_index\":" << row_index
                       << ",\"inserted_row_index\":"
                       << (insert_before ? row_index : row_index + 1U);
            });
    }

    return 0;
}

} // namespace

auto run_append_table_row_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "append-table-row expects an input path and a table index",
                          json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_body_table_index(command, arguments, 2U, "table index",
                                table_index, json_output)) {
        return 2;
    }

    append_table_row_options options;
    std::string error_message;
    if (!parse_append_table_row_options(arguments, 3U, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::table_inspection_summary inspected_table{};
    if (!load_body_table_summary(command, doc, table_index, inspected_table,
                                 options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table_for_row(command, doc, table_index, table,
                                    options.json_output)) {
        return 1;
    }

    const auto cell_count = options.cell_count.value_or(inspected_table.column_count);
    if (cell_count == 0U) {
        report_body_table_row_failure(command, "failed to append table row",
                                      "table row must contain at least one cell",
                                      options.json_output);
        return 1;
    }

    auto row = table.append_row(cell_count);
    if (!row.has_next()) {
        report_body_table_row_failure(command, "failed to append table row",
                                      "target table handle is not valid",
                                      options.json_output);
        return 1;
    }

    const auto row_index = inspected_table.row_count;
    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, cell_count](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"row_index\":" << row_index
                       << ",\"cell_count\":" << cell_count;
            });
    }

    return 0;
}

auto run_insert_table_row_before_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_insert_table_row_command(command, arguments, true);
}

auto run_insert_table_row_after_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_insert_table_row_command(command, arguments, false);
}

auto run_remove_table_row_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "remove-table-row expects an input path, a table index, and a row index",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_body_table_index(command, arguments, 2U, "table index",
                                table_index, json_output)) {
        return 2;
    }

    std::size_t row_index = 0U;
    if (!parse_body_table_index(command, arguments, 3U, "row index", row_index,
                                json_output)) {
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 4U, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::TableRow row;
    if (!resolve_body_table_row_for_row(command, doc, table_index, row_index,
                                        row, options.json_output)) {
        return 1;
    }

    if (!row.remove()) {
        report_body_table_row_failure(
            command, "failed to remove table row",
            "cannot remove the last row from table index '" +
                std::to_string(table_index) + "'",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"row_index\":" << row_index;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
