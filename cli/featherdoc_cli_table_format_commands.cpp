#include "featherdoc_cli_table_format_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_border_margin_commands.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"
#include "featherdoc_cli_table_format_support.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace featherdoc_cli {
namespace {

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
           is_table_border_margin_command(command);
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
    if (is_table_border_margin_command(command)) {
        return run_table_border_margin_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
