#include "featherdoc_cli_table_border_margin_commands.hpp"

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
namespace {

auto run_set_table_default_cell_margin_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "set-table-default-cell-margin expects an input path, a table index, a margin edge, and a margin in twips",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    featherdoc::cell_margin_edge edge = featherdoc::cell_margin_edge::top;
    if (!parse_cell_margin_edge_text(arguments[3], edge)) {
        print_parse_error(command,
                          "invalid margin edge: " +
                              std::string(arguments[3]),
                          json_output);
        return 2;
    }

    std::uint32_t margin_twips = 0U;
    if (!parse_uint32_arg(command, arguments, 4U, "margin twips",
                          margin_twips, json_output)) {
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 5U, options, json_output)) {
        return 2;
    }

    featherdoc::Table table;
    if (!open_table_for_format(command, arguments, doc, table_index, options,
                               table)) {
        return 1;
    }

    if (!table.set_cell_margin_twips(edge, margin_twips)) {
        report_invalid_table_target(
            command, "failed to set table default cell margin",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, edge, margin_twips](std::ostream &stream) {
                write_json_table_location(stream, table_index);
                stream << ",\"edge\":";
                write_json_string(stream, cell_margin_edge_name(edge));
                stream << ",\"margin_twips\":" << margin_twips;
            });
    }
    return 0;
}

auto run_clear_table_default_cell_margin_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "clear-table-default-cell-margin expects an input path, a table index, and a margin edge",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    featherdoc::cell_margin_edge edge = featherdoc::cell_margin_edge::top;
    if (!parse_cell_margin_edge_text(arguments[3], edge)) {
        print_parse_error(command,
                          "invalid margin edge: " +
                              std::string(arguments[3]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 4U, options, json_output)) {
        return 2;
    }

    featherdoc::Table table;
    if (!open_table_for_format(command, arguments, doc, table_index, options,
                               table)) {
        return 1;
    }

    if (!table.clear_cell_margin(edge)) {
        report_invalid_table_target(
            command, "failed to clear table default cell margin",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, edge](std::ostream &stream) {
                write_json_table_location(stream, table_index);
                stream << ",\"edge\":";
                write_json_string(stream, cell_margin_edge_name(edge));
            });
    }
    return 0;
}

auto run_set_table_border_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-table-border expects an input path, a table index, and a border edge",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    featherdoc::table_border_edge edge = featherdoc::table_border_edge::top;
    if (!parse_table_border_edge_text(arguments[3], edge)) {
        print_parse_error(command,
                          "invalid table border edge: " +
                              std::string(arguments[3]),
                          json_output);
        return 2;
    }

    table_cell_border_options options;
    if (!parse_border_options(command, arguments, 4U, options, json_output)) {
        return 2;
    }

    featherdoc::Table table;
    if (!open_table_for_format(command, arguments, doc, table_index, options,
                               table)) {
        return 1;
    }

    const auto border = make_border_definition(options);
    if (!table.set_border(edge, border)) {
        report_invalid_table_target(command, "failed to set table border",
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, edge, border](std::ostream &stream) {
                write_json_table_location(stream, table_index);
                stream << ",\"edge\":";
                write_json_string(stream, table_border_edge_name(edge));
                write_json_border_definition(stream, border);
            });
    }
    return 0;
}

auto run_clear_table_border_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "clear-table-border expects an input path, a table index, and a border edge",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    featherdoc::table_border_edge edge = featherdoc::table_border_edge::top;
    if (!parse_table_border_edge_text(arguments[3], edge)) {
        print_parse_error(command,
                          "invalid table border edge: " +
                              std::string(arguments[3]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    if (!parse_style_options(command, arguments, 4U, options, json_output)) {
        return 2;
    }

    featherdoc::Table table;
    if (!open_table_for_format(command, arguments, doc, table_index, options,
                               table)) {
        return 1;
    }

    if (!table.clear_border(edge)) {
        report_invalid_table_target(command, "failed to clear table border",
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, edge](std::ostream &stream) {
                write_json_table_location(stream, table_index);
                stream << ",\"edge\":";
                write_json_string(stream, table_border_edge_name(edge));
            });
    }
    return 0;
}

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

auto run_set_table_cell_border_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 6U) {
        print_parse_error(
            command,
            "set-table-cell-border expects an input path, a table index, a row index, a cell index, and a border edge",
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

    featherdoc::cell_border_edge edge = featherdoc::cell_border_edge::top;
    if (!parse_cell_border_edge_text(arguments[5], edge)) {
        print_parse_error(command,
                          "invalid border edge: " +
                              std::string(arguments[5]),
                          json_output);
        return 2;
    }

    table_cell_border_options options;
    if (!parse_border_options(command, arguments, 6U, options, json_output)) {
        return 2;
    }

    featherdoc::TableCell cell;
    if (!open_table_cell_for_format(command, arguments, doc, table_index,
                                    row_index, cell_index, options, cell)) {
        return 1;
    }

    const auto border = make_border_definition(options);
    if (!cell.set_border(edge, border)) {
        report_invalid_table_cell_target(command,
                                         "failed to set table cell border",
                                         options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, cell_index, edge, border](
                std::ostream &stream) {
                write_json_table_cell_location(stream, table_index, row_index,
                                               cell_index);
                stream << ",\"edge\":";
                write_json_string(stream, cell_border_edge_name(edge));
                write_json_border_definition(stream, border);
            });
    }
    return 0;
}

auto run_clear_table_cell_border_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 6U) {
        print_parse_error(
            command,
            "clear-table-cell-border expects an input path, a table index, a row index, a cell index, and a border edge",
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

    featherdoc::cell_border_edge edge = featherdoc::cell_border_edge::top;
    if (!parse_cell_border_edge_text(arguments[5], edge)) {
        print_parse_error(command,
                          "invalid border edge: " +
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

    if (!cell.clear_border(edge)) {
        report_invalid_table_cell_target(command,
                                         "failed to clear table cell border",
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
                write_json_string(stream, cell_border_edge_name(edge));
            });
    }
    return 0;
}

} // namespace

auto is_table_border_margin_command(std::string_view command) -> bool {
    return command == "set-table-default-cell-margin" ||
           command == "clear-table-default-cell-margin" ||
           command == "set-table-border" ||
           command == "clear-table-border" ||
           command == "set-table-cell-margin" ||
           command == "clear-table-cell-margin" ||
           command == "set-table-cell-border" ||
           command == "clear-table-cell-border";
}

auto run_table_border_margin_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "set-table-default-cell-margin") {
        return run_set_table_default_cell_margin_command(command, arguments, doc);
    }
    if (command == "clear-table-default-cell-margin") {
        return run_clear_table_default_cell_margin_command(command, arguments,
                                                          doc);
    }
    if (command == "set-table-border") {
        return run_set_table_border_command(command, arguments, doc);
    }
    if (command == "clear-table-border") {
        return run_clear_table_border_command(command, arguments, doc);
    }
    if (command == "set-table-cell-margin") {
        return run_set_table_cell_margin_command(command, arguments, doc);
    }
    if (command == "clear-table-cell-margin") {
        return run_clear_table_cell_margin_command(command, arguments, doc);
    }
    if (command == "set-table-cell-border") {
        return run_set_table_cell_border_command(command, arguments, doc);
    }
    if (command == "clear-table-cell-border") {
        return run_clear_table_cell_border_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
