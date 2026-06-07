#include "featherdoc_cli_table_format_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"

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

auto parse_table_cell_location(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t &table_index, std::size_t &row_index, std::size_t &cell_index,
    bool json_output) -> bool {
    return parse_index_arg(command, arguments, 2U, "table index", table_index,
                           json_output) &&
           parse_index_arg(command, arguments, 3U, "row index", row_index,
                           json_output) &&
           parse_index_arg(command, arguments, 4U, "cell index", cell_index,
                           json_output);
}

auto parse_uint32_arg(std::string_view command,
                      const std::vector<std::string_view> &arguments,
                      std::size_t argument_index, std::string_view label,
                      std::uint32_t &value, bool json_output) -> bool {
    if (parse_uint32(arguments[argument_index], value)) {
        return true;
    }

    print_parse_error(command,
                      "invalid " + std::string(label) + ": " +
                          std::string(arguments[argument_index]),
                      json_output);
    return false;
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

auto parse_border_options(std::string_view command,
                          const std::vector<std::string_view> &arguments,
                          std::size_t option_start,
                          table_cell_border_options &options,
                          bool json_output) -> bool {
    std::string error_message;
    if (!parse_table_cell_border_options(arguments, option_start, options,
                                         error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    if (!options.style.has_value()) {
        print_parse_error(command, "missing --style option", json_output);
        return false;
    }

    return true;
}

auto make_border_definition(const table_cell_border_options &options)
    -> featherdoc::border_definition {
    featherdoc::border_definition border{};
    border.style = *options.style;
    if (options.size_eighth_points.has_value()) {
        border.size_eighth_points = *options.size_eighth_points;
    }
    if (options.color.has_value()) {
        border.color = *options.color;
    }
    if (options.space_points.has_value()) {
        border.space_points = *options.space_points;
    }
    return border;
}

auto report_invalid_table_target(std::string_view command,
                                 std::string_view summary, bool json_output)
    -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = "target table handle is not valid";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto report_invalid_table_cell_target(std::string_view command,
                                      std::string_view summary,
                                      bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = "target table cell handle is not valid";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

void write_json_table_location(std::ostream &stream,
                               std::size_t table_index) {
    stream << ",\"table_index\":" << table_index;
}

void write_json_table_cell_location(std::ostream &stream,
                                    std::size_t table_index,
                                    std::size_t row_index,
                                    std::size_t cell_index) {
    write_json_table_location(stream, table_index);
    stream << ",\"row_index\":" << row_index
           << ",\"cell_index\":" << cell_index;
}

void write_json_uint_field(std::ostream &stream, std::string_view field_name,
                           std::uint32_t value) {
    stream << ",\"" << field_name << "\":" << value;
}

void write_json_border_definition(std::ostream &stream,
                                  const featherdoc::border_definition &border) {
    stream << ",\"style\":";
    write_json_string(stream, border_style_name(border.style));
    stream << ",\"size_eighth_points\":" << border.size_eighth_points
           << ",\"color\":";
    write_json_string(stream, border.color);
    stream << ",\"space_points\":" << border.space_points;
}

template <typename Options>
auto open_table_for_format(std::string_view command,
                           const std::vector<std::string_view> &arguments,
                           featherdoc::Document &doc,
                           std::size_t table_index, const Options &options,
                           featherdoc::Table &table) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    return resolve_body_table(doc, table_index, table, command,
                              options.json_output);
}

template <typename Options>
auto open_table_cell_for_format(std::string_view command,
                                const std::vector<std::string_view> &arguments,
                                featherdoc::Document &doc,
                                std::size_t table_index,
                                std::size_t row_index,
                                std::size_t cell_index, const Options &options,
                                featherdoc::TableCell &cell) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    return resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                   command, options.json_output);
}

template <typename Mutator>
auto run_set_table_uint_property_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc, std::string_view expected_message,
    std::string_view invalid_value_label, std::string_view json_field_name,
    std::string_view failure_summary, Mutator &&mutator) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command, std::string(expected_message), json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    std::uint32_t value = 0U;
    if (!parse_uint32_arg(command, arguments, 3U, invalid_value_label, value,
                          json_output)) {
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

    if (!mutator(table, value)) {
        report_invalid_table_target(command, failure_summary,
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, json_field_name, value](std::ostream &stream) {
                write_json_table_location(stream, table_index);
                write_json_uint_field(stream, json_field_name, value);
            });
    }
    return 0;
}

template <typename Mutator>
auto run_clear_table_property_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc, std::string_view expected_message,
    std::string_view failure_summary, Mutator &&mutator) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command, std::string(expected_message), json_output);
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

    featherdoc::Table table;
    if (!open_table_for_format(command, arguments, doc, table_index, options,
                               table)) {
        return 1;
    }

    if (!mutator(table)) {
        report_invalid_table_target(command, failure_summary,
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(command, doc, options.output_path,
                                   [table_index](std::ostream &stream) {
                                       write_json_table_location(stream,
                                                                 table_index);
                                   });
    }
    return 0;
}

auto run_set_table_layout_mode_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-table-layout-mode expects an input path, a table index, and a layout mode",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    featherdoc::table_layout_mode layout_mode =
        featherdoc::table_layout_mode::autofit;
    if (!parse_table_layout_mode_text(arguments[3], layout_mode)) {
        print_parse_error(command,
                          "invalid table layout mode: " +
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

    if (!table.set_layout_mode(layout_mode)) {
        report_invalid_table_target(command, "failed to set table layout mode",
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, layout_mode](std::ostream &stream) {
                write_json_table_location(stream, table_index);
                stream << ",\"layout_mode\":";
                write_json_string(stream, table_layout_mode_name(layout_mode));
            });
    }
    return 0;
}

auto run_set_table_alignment_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-table-alignment expects an input path, a table index, and an alignment",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_table_index(command, arguments, table_index, json_output)) {
        return 2;
    }

    featherdoc::table_alignment alignment = featherdoc::table_alignment::left;
    if (!parse_table_alignment_text(arguments[3], alignment)) {
        print_parse_error(command,
                          "invalid table alignment: " +
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

    if (!table.set_alignment(alignment)) {
        report_invalid_table_target(command, "failed to set table alignment",
                                    options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, alignment](std::ostream &stream) {
                write_json_table_location(stream, table_index);
                stream << ",\"alignment\":";
                write_json_string(stream, table_alignment_name(alignment));
            });
    }
    return 0;
}

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

auto is_table_format_command(std::string_view command) -> bool {
    return command == "set-table-width" ||
           command == "clear-table-width" ||
           command == "set-table-layout-mode" ||
           command == "clear-table-layout-mode" ||
           command == "set-table-alignment" ||
           command == "clear-table-alignment" ||
           command == "set-table-indent" ||
           command == "clear-table-indent" ||
           command == "set-table-cell-spacing" ||
           command == "clear-table-cell-spacing" ||
           command == "set-table-default-cell-margin" ||
           command == "clear-table-default-cell-margin" ||
           command == "set-table-border" ||
           command == "clear-table-border" ||
           command == "set-table-cell-margin" ||
           command == "clear-table-cell-margin" ||
           command == "set-table-cell-border" ||
           command == "clear-table-cell-border";
}

auto run_table_format_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "set-table-width") {
        return run_set_table_uint_property_command(
            command, arguments, doc,
            "set-table-width expects an input path, a table index, and a width in twips",
            "width twips", "width_twips", "failed to set table width",
            [](featherdoc::Table &table, std::uint32_t value) {
                return table.set_width_twips(value);
            });
    }
    if (command == "clear-table-width") {
        return run_clear_table_property_command(
            command, arguments, doc,
            "clear-table-width expects an input path and a table index",
            "failed to clear table width",
            [](featherdoc::Table &table) { return table.clear_width(); });
    }
    if (command == "set-table-layout-mode") {
        return run_set_table_layout_mode_command(command, arguments, doc);
    }
    if (command == "clear-table-layout-mode") {
        return run_clear_table_property_command(
            command, arguments, doc,
            "clear-table-layout-mode expects an input path and a table index",
            "failed to clear table layout mode",
            [](featherdoc::Table &table) { return table.clear_layout_mode(); });
    }
    if (command == "set-table-alignment") {
        return run_set_table_alignment_command(command, arguments, doc);
    }
    if (command == "clear-table-alignment") {
        return run_clear_table_property_command(
            command, arguments, doc,
            "clear-table-alignment expects an input path and a table index",
            "failed to clear table alignment",
            [](featherdoc::Table &table) { return table.clear_alignment(); });
    }
    if (command == "set-table-indent") {
        return run_set_table_uint_property_command(
            command, arguments, doc,
            "set-table-indent expects an input path, a table index, and an indent in twips",
            "indent twips", "indent_twips", "failed to set table indent",
            [](featherdoc::Table &table, std::uint32_t value) {
                return table.set_indent_twips(value);
            });
    }
    if (command == "clear-table-indent") {
        return run_clear_table_property_command(
            command, arguments, doc,
            "clear-table-indent expects an input path and a table index",
            "failed to clear table indent",
            [](featherdoc::Table &table) { return table.clear_indent(); });
    }
    if (command == "set-table-cell-spacing") {
        return run_set_table_uint_property_command(
            command, arguments, doc,
            "set-table-cell-spacing expects an input path, a table index, and spacing in twips",
            "cell spacing twips", "cell_spacing_twips",
            "failed to set table cell spacing",
            [](featherdoc::Table &table, std::uint32_t value) {
                return table.set_cell_spacing_twips(value);
            });
    }
    if (command == "clear-table-cell-spacing") {
        return run_clear_table_property_command(
            command, arguments, doc,
            "clear-table-cell-spacing expects an input path and a table index",
            "failed to clear table cell spacing",
            [](featherdoc::Table &table) { return table.clear_cell_spacing(); });
    }
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
