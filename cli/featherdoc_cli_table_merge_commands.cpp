#include "featherdoc_cli_table_merge_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
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

struct body_table_cell_target {
    std::size_t table_index = 0U;
    std::size_t row_index = 0U;
    std::size_t cell_index = 0U;
};

auto report_body_table_merge_range_failure(std::string_view command,
                                           std::string_view summary,
                                           std::string detail,
                                           bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto parse_body_table_cell_target(
    std::string_view command, const std::vector<std::string_view> &arguments,
    body_table_cell_target &target, bool json_output) -> bool {
    if (!parse_index(arguments[2], target.table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return false;
    }

    if (!parse_index(arguments[3], target.row_index)) {
        print_parse_error(command,
                          "invalid row index: " + std::string(arguments[3]),
                          json_output);
        return false;
    }

    if (!parse_index(arguments[4], target.cell_index)) {
        print_parse_error(command,
                          "invalid cell index: " + std::string(arguments[4]),
                          json_output);
        return false;
    }

    return true;
}

auto resolve_body_table_for_merge(std::string_view command,
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

    return report_body_table_merge_range_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto resolve_body_table_row_for_merge(std::string_view command,
                                      featherdoc::Document &doc,
                                      std::size_t table_index,
                                      std::size_t row_index,
                                      featherdoc::TableRow &row,
                                      bool json_output) -> bool {
    featherdoc::Table table;
    if (!resolve_body_table_for_merge(command, doc, table_index, table,
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

    return report_body_table_merge_range_failure(
        command, "row index is out of range",
        "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'",
        json_output);
}

auto resolve_body_table_cell_for_merge(std::string_view command,
                                       featherdoc::Document &doc,
                                       const body_table_cell_target &target,
                                       featherdoc::TableCell &cell,
                                       bool json_output) -> bool {
    featherdoc::TableRow row;
    if (!resolve_body_table_row_for_merge(command, doc, target.table_index,
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

    return report_body_table_merge_range_failure(
        command, "cell index is out of range",
        "cell index '" + std::to_string(target.cell_index) +
            "' is out of range for row index '" +
            std::to_string(target.row_index) + "' in table index '" +
            std::to_string(target.table_index) + "'",
        json_output);
}

auto report_body_table_merge_failure(std::string_view command,
                                     std::string_view summary,
                                     std::string detail,
                                     bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto write_json_body_table_merge_target(
    std::ostream &stream, const body_table_cell_target &target,
    table_merge_direction direction) -> void {
    stream << ",\"table_index\":" << target.table_index
           << ",\"row_index\":" << target.row_index
           << ",\"cell_index\":" << target.cell_index << ",\"direction\":";
    write_json_string(stream, table_merge_direction_name(direction));
}

} // namespace

auto run_merge_table_cells_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(command,
                          "merge-table-cells expects an input path, a table "
                          "index, a row index, and a cell index",
                          json_output);
        return 2;
    }

    body_table_cell_target target;
    if (!parse_body_table_cell_target(command, arguments, target,
                                      json_output)) {
        return 2;
    }

    merge_table_cells_options options;
    std::string error_message;
    if (!parse_merge_table_cells_options(arguments, 5U, options,
                                         error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::TableCell cell;
    if (!resolve_body_table_cell_for_merge(command, doc, target, cell,
                                           options.json_output)) {
        return 1;
    }

    const auto success =
        options.direction == table_merge_direction::right
            ? cell.merge_right(options.count)
            : cell.merge_down(options.count);
    if (!success) {
        report_body_table_merge_failure(
            command, "failed to merge table cells",
            "cell at table index '" + std::to_string(target.table_index) +
                "', row index '" + std::to_string(target.row_index) +
                "', and cell index '" + std::to_string(target.cell_index) +
                "' could not be merged towards '" +
                std::string(table_merge_direction_name(options.direction)) +
                "' with count '" + std::to_string(options.count) + "'",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&target, &options](std::ostream &stream) {
                write_json_body_table_merge_target(stream, target,
                                                   options.direction);
                stream << ",\"count\":" << options.count;
            });
    }

    return 0;
}

auto run_unmerge_table_cells_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(command,
                          "unmerge-table-cells expects an input path, a table "
                          "index, a row index, and a cell index",
                          json_output);
        return 2;
    }

    body_table_cell_target target;
    if (!parse_body_table_cell_target(command, arguments, target,
                                      json_output)) {
        return 2;
    }

    unmerge_table_cells_options options;
    std::string error_message;
    if (!parse_unmerge_table_cells_options(arguments, 5U, options,
                                           error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::TableCell cell;
    if (!resolve_body_table_cell_for_merge(command, doc, target, cell,
                                           options.json_output)) {
        return 1;
    }

    const auto success =
        options.direction == table_merge_direction::right
            ? cell.unmerge_right()
            : cell.unmerge_down();
    if (!success) {
        report_body_table_merge_failure(
            command, "failed to unmerge table cells",
            "cell at table index '" + std::to_string(target.table_index) +
                "', row index '" + std::to_string(target.row_index) +
                "', and cell index '" + std::to_string(target.cell_index) +
                "' could not be unmerged towards '" +
                std::string(table_merge_direction_name(options.direction)) + "'",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&target, &options](std::ostream &stream) {
                write_json_body_table_merge_target(stream, target,
                                                   options.direction);
            });
    }

    return 0;
}

} // namespace featherdoc_cli
