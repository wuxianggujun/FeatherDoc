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

} // namespace featherdoc_cli
