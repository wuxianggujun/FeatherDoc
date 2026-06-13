#include "featherdoc_cli_table_cell_style_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_set_table_cell_fill_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 6U) {
        print_parse_error(
            command,
            "set-table-cell-fill expects an input path, a table index, a row index, a cell index, and a fill color",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    std::size_t row_index = 0U;
    std::size_t cell_index = 0U;
    if (!parse_table_cell_location(command, arguments, table_index, row_index,
                                   cell_index, json_output)) {
        return 2;
    }

    const auto fill_color = arguments[5];
    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 6U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::TableCell cell;
    if (!open_table_cell_for_style(command, arguments, doc, table_index,
                                   row_index, cell_index, options, cell)) {
        return 1;
    }

    if (!cell.set_fill_color(fill_color)) {
        report_invalid_table_cell_target(
            command, "failed to set table cell fill color",
            fill_color.empty() ? "table cell fill color must not be empty"
                               : "target table cell handle is not valid",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, cell_index, fill_color](
                std::ostream &stream) {
                write_json_table_cell_location(stream, table_index, row_index,
                                               cell_index);
                stream << ",\"fill_color\":";
                write_json_string(stream, fill_color);
            });
    }
    return 0;
}

auto run_clear_table_cell_fill_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "clear-table-cell-fill expects an input path, a table index, a row index, and a cell index",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    std::size_t row_index = 0U;
    std::size_t cell_index = 0U;
    if (!parse_table_cell_location(command, arguments, table_index, row_index,
                                   cell_index, json_output)) {
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 5U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::TableCell cell;
    if (!open_table_cell_for_style(command, arguments, doc, table_index,
                                   row_index, cell_index, options, cell)) {
        return 1;
    }

    if (!cell.clear_fill_color()) {
        report_invalid_table_cell_target(command,
                                         "failed to clear table cell fill color",
                                         "target table cell handle is not valid",
                                         options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, cell_index](std::ostream &stream) {
                write_json_table_cell_location(stream, table_index, row_index,
                                               cell_index);
            });
    }
    return 0;
}

} // namespace featherdoc_cli
