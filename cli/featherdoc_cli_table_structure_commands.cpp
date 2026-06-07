#include "featherdoc_cli_table_structure_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace featherdoc_cli {
namespace {

auto parse_index_arg(std::string_view command,
                     const std::vector<std::string_view> &arguments,
                     std::size_t argument_index, std::string_view label,
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

auto parse_table_index(std::string_view command,
                       const std::vector<std::string_view> &arguments,
                       std::size_t &table_index, bool json_output) -> bool {
    return parse_index_arg(command, arguments, 2U, "table index", table_index,
                           json_output);
}

auto parse_style_options(std::string_view command,
                         const std::vector<std::string_view> &arguments,
                         std::size_t option_start,
                         table_cell_style_options &options,
                         bool json_output) -> bool {
    std::string error_message;
    if (parse_table_cell_style_options(arguments, option_start, options,
                                       error_message)) {
        return true;
    }

    print_parse_error(command, error_message, json_output);
    return false;
}

auto report_table_structure_failure(std::string_view command,
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

auto report_invalid_table_target(std::string_view command,
                                 std::string_view summary, bool json_output)
    -> bool {
    return report_table_structure_failure(command, summary,
                                          "target table handle is not valid",
                                          json_output);
}

auto load_body_table_summary_for_structure(
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

    return report_table_structure_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto validate_column_index(std::string_view command, std::size_t table_index,
                           std::size_t column_index,
                           const featherdoc::table_inspection_summary &table,
                           bool json_output) -> bool {
    if (column_index < table.column_count) {
        return true;
    }

    return report_table_structure_failure(
        command, "column index is out of range",
        "column index '" + std::to_string(column_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'",
        json_output);
}

auto parse_table_column_width_target(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t minimum_argument_count, std::string_view usage_message,
    std::size_t &table_index, std::size_t &column_index, bool json_output)
    -> bool {
    if (arguments.size() < minimum_argument_count) {
        print_parse_error(command, std::string(usage_message), json_output);
        return false;
    }

    return parse_table_index(command, arguments, table_index, json_output) &&
           parse_index_arg(command, arguments, 3U, "column index",
                           column_index, json_output);
}

auto run_set_table_column_width_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);

    std::size_t table_index = 0U;
    std::size_t column_index = 0U;
    if (!parse_table_column_width_target(
            command, arguments, 5U,
            "set-table-column-width expects an input path, a table index, a column index, and a width in twips",
            table_index, column_index, json_output)) {
        return 2;
    }

    std::uint32_t width_twips = 0U;
    if (!parse_uint32(arguments[4], width_twips)) {
        print_parse_error(command,
                          "invalid width twips: " + std::string(arguments[4]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 5U, options, json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::table_inspection_summary inspected_table{};
    if (!load_body_table_summary_for_structure(command, doc, table_index,
                                               inspected_table,
                                               options.json_output) ||
        !validate_column_index(command, table_index, column_index,
                               inspected_table, options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.set_column_width_twips(column_index, width_twips)) {
        report_invalid_table_target(command, "failed to set table column width",
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, column_index, width_twips](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"column_index\":" << column_index
                       << ",\"width_twips\":" << width_twips;
            });
    }
    return 0;
}

auto run_clear_table_column_width_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);

    std::size_t table_index = 0U;
    std::size_t column_index = 0U;
    if (!parse_table_column_width_target(
            command, arguments, 4U,
            "clear-table-column-width expects an input path, a table index, and a column index",
            table_index, column_index, json_output)) {
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 4U, options, json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::table_inspection_summary inspected_table{};
    if (!load_body_table_summary_for_structure(command, doc, table_index,
                                               inspected_table,
                                               options.json_output) ||
        !validate_column_index(command, table_index, column_index,
                               inspected_table, options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.clear_column_width(column_index)) {
        report_invalid_table_target(command,
                                    "failed to clear table column width",
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, column_index](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"column_index\":" << column_index;
            });
    }
    return 0;
}

auto run_insert_table_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    bool insert_before) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            std::string(command) +
                " expects an input path, a table index, a row count, and a column count",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    std::size_t row_count = 0U;
    if (!parse_index_arg(command, arguments, 3U, "row count", row_count,
                         json_output)) {
        return 2;
    }
    if (row_count == 0U) {
        print_parse_error(command, "row count must be greater than 0",
                          json_output);
        return 2;
    }

    std::size_t column_count = 0U;
    if (!parse_index_arg(command, arguments, 4U, "column count", column_count,
                         json_output)) {
        return 2;
    }
    if (column_count == 0U) {
        print_parse_error(command, "column count must be greater than 0",
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 5U, options, json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    auto inserted_table =
        insert_before ? table.insert_table_before(row_count, column_count)
                      : table.insert_table_after(row_count, column_count);
    if (!inserted_table.has_next()) {
        report_invalid_table_target(command, "failed to insert table",
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_count, column_count,
             insert_before](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"inserted_table_index\":"
                       << (insert_before ? table_index : table_index + 1U)
                       << ",\"row_count\":" << row_count
                       << ",\"column_count\":" << column_count;
            });
    }
    return 0;
}

auto run_insert_table_like_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    bool insert_before) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          std::string(command) +
                              " expects an input path and a table index",
                          json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 3U, options, json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    auto inserted_table = insert_before ? table.insert_table_like_before()
                                        : table.insert_table_like_after();
    if (!inserted_table.has_next()) {
        report_invalid_table_target(command, "failed to insert table",
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, insert_before](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"inserted_table_index\":"
                       << (insert_before ? table_index : table_index + 1U);
            });
    }
    return 0;
}

auto run_remove_table_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "remove-table expects an input path and a table index",
                          json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 3U, options, json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.remove()) {
        report_table_structure_failure(
            command, "failed to remove table",
            "cannot remove table index '" + std::to_string(table_index) +
                "' because its parent would be left without block content",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index;
            });
    }
    return 0;
}

} // namespace

auto is_table_structure_command(std::string_view command) -> bool {
    return command == "set-table-column-width" ||
           command == "clear-table-column-width" ||
           command == "insert-table-before" ||
           command == "insert-table-after" ||
           command == "insert-table-like-before" ||
           command == "insert-table-like-after" ||
           command == "remove-table";
}

auto run_table_structure_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    if (command == "set-table-column-width") {
        return run_set_table_column_width_command(command, arguments);
    }
    if (command == "clear-table-column-width") {
        return run_clear_table_column_width_command(command, arguments);
    }
    if (command == "insert-table-before") {
        return run_insert_table_command(command, arguments, true);
    }
    if (command == "insert-table-after") {
        return run_insert_table_command(command, arguments, false);
    }
    if (command == "insert-table-like-before") {
        return run_insert_table_like_command(command, arguments, true);
    }
    if (command == "insert-table-like-after") {
        return run_insert_table_like_command(command, arguments, false);
    }
    if (command == "remove-table") {
        return run_remove_table_command(command, arguments);
    }

    return 2;
}

} // namespace featherdoc_cli
