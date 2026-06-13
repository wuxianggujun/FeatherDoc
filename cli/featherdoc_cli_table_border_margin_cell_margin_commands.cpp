#include "featherdoc_cli_table_border_margin_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_format_support.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_set_table_cell_margin_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 7U) {
        print_parse_error(
            command,
            "set-table-cell-margin expects an input path, a table index, a row index, a cell index, a margin edge, and a margin in twips",
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

    featherdoc::cell_margin_edge edge = featherdoc::cell_margin_edge::top;
    if (!parse_cell_margin_edge_text(arguments[5], edge)) {
        print_parse_error(command,
                          "invalid margin edge: " +
                              std::string(arguments[5]),
                          json_output);
        return 2;
    }

    std::uint32_t margin_twips = 0U;
    if (!parse_uint32_arg(command, arguments, 6U, "margin twips",
                          margin_twips, json_output)) {
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 7U, options, json_output)) {
        return 2;
    }

    featherdoc::TableCell cell;
    if (!open_table_cell_for_format(command, arguments, doc, table_index,
                                    row_index, cell_index, options, cell)) {
        return 1;
    }

    if (!cell.set_margin_twips(edge, margin_twips)) {
        report_invalid_table_cell_target(command,
                                         "failed to set table cell margin",
                                         options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, cell_index, edge, margin_twips](
                std::ostream &stream) {
                write_json_table_cell_location(stream, table_index, row_index,
                                               cell_index);
                stream << ",\"edge\":";
                write_json_string(stream, cell_margin_edge_name(edge));
                stream << ",\"margin_twips\":" << margin_twips;
            });
    }
    return 0;
}

auto run_clear_table_cell_margin_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 6U) {
        print_parse_error(
            command,
            "clear-table-cell-margin expects an input path, a table index, a row index, a cell index, and a margin edge",
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

    featherdoc::cell_margin_edge edge = featherdoc::cell_margin_edge::top;
    if (!parse_cell_margin_edge_text(arguments[5], edge)) {
        print_parse_error(command,
                          "invalid margin edge: " +
                              std::string(arguments[5]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 6U, options, json_output)) {
        return 2;
    }

    featherdoc::TableCell cell;
    if (!open_table_cell_for_format(command, arguments, doc, table_index,
                                    row_index, cell_index, options, cell)) {
        return 1;
    }

    if (!cell.clear_margin(edge)) {
        report_invalid_table_cell_target(command,
                                         "failed to clear table cell margin",
                                         options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, cell_index, edge](std::ostream &stream) {
                write_json_table_cell_location(stream, table_index, row_index,
                                               cell_index);
                stream << ",\"edge\":";
                write_json_string(stream, cell_margin_edge_name(edge));
            });
    }
    return 0;
}

} // namespace featherdoc_cli
