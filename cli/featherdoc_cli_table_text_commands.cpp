#include "featherdoc_cli_table_text_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto report_body_table_text_range_failure(std::string_view command,
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

auto resolve_body_table_for_text(std::string_view command,
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

    return report_body_table_text_range_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto resolve_body_table_row_for_text(std::string_view command,
                                     featherdoc::Document &doc,
                                     std::size_t table_index,
                                     std::size_t row_index,
                                     featherdoc::TableRow &row,
                                     bool json_output) -> bool {
    featherdoc::Table table;
    if (!resolve_body_table_for_text(command, doc, table_index, table,
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

    return report_body_table_text_range_failure(
        command, "row index is out of range",
        "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'",
        json_output);
}

auto resolve_body_table_cell_for_text(std::string_view command,
                                      featherdoc::Document &doc,
                                      std::size_t table_index,
                                      std::size_t row_index,
                                      std::size_t cell_index,
                                      featherdoc::TableCell &cell,
                                      bool json_output) -> bool {
    featherdoc::TableRow row;
    if (!resolve_body_table_row_for_text(command, doc, table_index, row_index,
                                         row, json_output)) {
        return false;
    }

    cell = row.cells();
    for (std::size_t current_index = 0;
         current_index < cell_index && cell.has_next();
         ++current_index) {
        cell.next();
    }

    if (cell.has_next()) {
        return true;
    }

    return report_body_table_text_range_failure(
        command, "cell index is out of range",
        "cell index '" + std::to_string(cell_index) +
            "' is out of range for row index '" + std::to_string(row_index) +
            "' in table index '" + std::to_string(table_index) + "'",
        json_output);
}

auto resolve_body_table_cell_by_grid_column_for_text(
    std::string_view command, featherdoc::Document &doc,
    std::size_t table_index, std::size_t row_index, std::size_t grid_column,
    featherdoc::TableCell &cell, bool json_output) -> bool {
    featherdoc::TableRow row;
    if (!resolve_body_table_row_for_text(command, doc, table_index, row_index,
                                         row, json_output)) {
        return false;
    }

    auto resolved_cell = row.find_cell_by_grid_column(grid_column);
    if (resolved_cell.has_value()) {
        cell = *resolved_cell;
        return true;
    }

    return report_body_table_text_range_failure(
        command, "grid column is out of range",
        "grid column '" + std::to_string(grid_column) +
            "' is out of range for row index '" + std::to_string(row_index) +
            "' in table index '" + std::to_string(table_index) + "'",
        json_output);
}

auto parse_required_table_index(std::string_view command,
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

} // namespace

auto run_set_table_cell_text_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-table-cell-text expects an input path, a table index, "
            "a row index, and either a cell index or --grid-column",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_required_table_index(command, arguments, 2U, "table index",
                                    table_index, json_output)) {
        return 2;
    }

    std::size_t row_index = 0U;
    if (!parse_required_table_index(command, arguments, 3U, "row index",
                                    row_index, json_output)) {
        return 2;
    }

    auto cell_index = std::optional<std::size_t>{};
    auto options_start_index = std::size_t{4U};
    if (arguments.size() > 4U &&
        !(arguments[4].size() >= 2U && arguments[4].substr(0U, 2U) == "--")) {
        std::size_t parsed_cell_index = 0U;
        if (!parse_required_table_index(command, arguments, 4U, "cell index",
                                        parsed_cell_index, json_output)) {
            return 2;
        }
        cell_index = parsed_cell_index;
        options_start_index = 5U;
    }

    table_cell_text_options options;
    std::string error_message;
    if (!parse_table_cell_text_options(arguments, options_start_index, options,
                                       error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    if (!cell_index.has_value() && !options.grid_column.has_value()) {
        print_parse_error(command,
                          "expected a cell index or --grid-column <index>",
                          json_output);
        return 2;
    }
    if (cell_index.has_value() && options.grid_column.has_value()) {
        print_parse_error(command,
                          "cell index and --grid-column are mutually exclusive",
                          json_output);
        return 2;
    }

    std::string replacement_text;
    if (!read_text_source(options, replacement_text, error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "input",
                                     error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    std::optional<featherdoc::table_cell_inspection_summary>
        resolved_cell_summary;
    featherdoc::TableCell cell;
    if (options.grid_column.has_value()) {
        resolved_cell_summary =
            doc.inspect_table_cell_by_grid_column(table_index, row_index,
                                                  *options.grid_column);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }
        if (!resolve_body_table_cell_by_grid_column_for_text(
                command, doc, table_index, row_index, *options.grid_column,
                cell, options.json_output)) {
            return 1;
        }
    } else if (!resolve_body_table_cell_for_text(command, doc, table_index,
                                                row_index, *cell_index, cell,
                                                options.json_output)) {
        return 1;
    }

    if (!cell.set_text(replacement_text)) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::io_error);
        error_info.detail = "failed to set text for the requested table cell";
        error_info.entry_name = "word/document.xml";
        report_operation_failure(command, "mutate",
                                 "failed to set table cell text", error_info,
                                 options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, cell_index, &options,
             &resolved_cell_summary](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"row_index\":" << row_index;
                if (options.grid_column.has_value()) {
                    stream << ",\"grid_column\":" << *options.grid_column;
                    if (resolved_cell_summary.has_value()) {
                        stream << ",\"cell_index\":"
                               << resolved_cell_summary->cell_index
                               << ",\"column_index\":"
                               << resolved_cell_summary->column_index
                               << ",\"column_span\":"
                               << resolved_cell_summary->column_span;
                    }
                } else {
                    stream << ",\"cell_index\":" << *cell_index;
                }
            });
    }

    return 0;
}

auto run_insert_paragraph_after_table_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "insert-paragraph-after-table expects an input path and a table index",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_required_table_index(command, arguments, 2U, "table index",
                                    table_index, json_output)) {
        return 2;
    }

    table_cell_text_options options;
    std::string error_message;
    if (!parse_table_cell_text_options(arguments, 3U, options,
                                       error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    if (options.grid_column.has_value()) {
        print_parse_error(
            command,
            "insert-paragraph-after-table does not accept --grid-column",
            json_output);
        return 2;
    }

    std::string paragraph_text;
    if (!read_text_source(options, paragraph_text, error_message)) {
        print_parse_error(command, error_message, options.json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table_for_text(command, doc, table_index, table,
                                     options.json_output)) {
        return 1;
    }

    auto inserted_paragraph = table.insert_paragraph_after(paragraph_text);
    if (!inserted_paragraph.has_next()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = "target table handle is not valid";
        error_info.entry_name = "word/document.xml";
        report_operation_failure(command, "mutate",
                                 "failed to insert paragraph", error_info,
                                 options.json_output);
        return 1;
    }

    std::optional<std::size_t> inserted_paragraph_index;
    const auto body_blocks = doc.inspect_body_blocks();
    for (std::size_t index = 0U; index + 1U < body_blocks.size(); ++index) {
        if (body_blocks[index].kind == featherdoc::body_block_kind::table &&
            body_blocks[index].item_index == table_index &&
            body_blocks[index + 1U].kind ==
                featherdoc::body_block_kind::paragraph) {
            inserted_paragraph_index = body_blocks[index + 1U].item_index;
            break;
        }
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, inserted_paragraph_index,
             text_length = paragraph_text.size()](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"paragraph_index\":";
                if (inserted_paragraph_index.has_value()) {
                    stream << *inserted_paragraph_index;
                } else {
                    stream << "null";
                }
                stream << ",\"text_length\":" << text_length;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
