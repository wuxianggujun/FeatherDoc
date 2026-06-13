#include "featherdoc_cli_table_column_width_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_structure_support.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

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
    if (!parse_table_structure_style_options(command, arguments, 5U, options,
                                             json_output)) {
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
        !validate_table_structure_column_index(command, table_index,
                                               column_index, inspected_table,
                                               options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.set_column_width_twips(column_index, width_twips)) {
        report_invalid_table_structure_target(
            command, "failed to set table column width", options.json_output);
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
    if (!parse_table_structure_style_options(command, arguments, 4U, options,
                                             json_output)) {
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
        !validate_table_structure_column_index(command, table_index,
                                               column_index, inspected_table,
                                               options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command,
                            options.json_output)) {
        return 1;
    }

    if (!table.clear_column_width(column_index)) {
        report_invalid_table_structure_target(
            command, "failed to clear table column width", options.json_output);
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

} // namespace featherdoc_cli
