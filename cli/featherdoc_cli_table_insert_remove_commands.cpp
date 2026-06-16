#include "featherdoc_cli_table_insert_remove_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_structure_support.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

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
    if (!parse_table_structure_table_index(command, arguments, table_index,
                                           json_output)) {
        return 2;
    }

    std::size_t row_count = 0U;
    if (!parse_table_structure_index_arg(command, arguments, 3U, "row count",
                                         row_count, json_output)) {
        return 2;
    }
    if (row_count == 0U) {
        print_parse_error(command, "row count must be greater than 0",
                          json_output);
        return 2;
    }

    std::size_t column_count = 0U;
    if (!parse_table_structure_index_arg(command, arguments, 4U,
                                         "column count", column_count,
                                         json_output)) {
        return 2;
    }
    if (column_count == 0U) {
        print_parse_error(command, "column count must be greater than 0",
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    if (!parse_table_structure_style_options(command, arguments, 5U, options,
                                             json_output)) {
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
        report_invalid_table_structure_target(command, "failed to insert table",
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
    if (!parse_table_structure_table_index(command, arguments, table_index,
                                           json_output)) {
        return 2;
    }

    table_cell_style_options options;
    if (!parse_table_structure_style_options(command, arguments, 3U, options,
                                             json_output)) {
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
        report_invalid_table_structure_target(command, "failed to insert table",
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
    if (!parse_table_structure_table_index(command, arguments, table_index,
                                           json_output)) {
        return 2;
    }

    table_cell_style_options options;
    if (!parse_table_structure_style_options(command, arguments, 3U, options,
                                             json_output)) {
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

} // namespace featherdoc_cli
