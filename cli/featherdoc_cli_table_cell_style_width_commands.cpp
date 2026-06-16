#include "featherdoc_cli_table_cell_style_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_set_table_cell_width_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 6U) {
        print_parse_error(
            command,
            "set-table-cell-width expects an input path, a table index, a row index, a cell index, and a width in twips",
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

    std::uint32_t width_twips = 0U;
    if (!parse_uint32(arguments[5], width_twips)) {
        print_parse_error(command,
                          "invalid width twips: " + std::string(arguments[5]),
                          json_output);
        return 2;
    }

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

    if (!cell.set_width_twips(width_twips)) {
        report_invalid_table_cell_target(command,
                                         "failed to set table cell width",
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
            [table_index, row_index, cell_index, width_twips](
                std::ostream &stream) {
                write_json_table_cell_location(stream, table_index, row_index,
                                               cell_index);
                stream << ",\"width_twips\":" << width_twips;
            });
    }
    return 0;
}

auto run_clear_table_cell_width_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "clear-table-cell-width expects an input path, a table index, a row index, and a cell index",
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

    if (!cell.clear_width()) {
        report_invalid_table_cell_target(command,
                                         "failed to clear table cell width",
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
