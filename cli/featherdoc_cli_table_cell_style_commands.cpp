#include "featherdoc_cli_table_cell_style_commands.hpp"

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

namespace featherdoc_cli {
namespace {

auto parse_table_cell_location(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t &table_index, std::size_t &row_index, std::size_t &cell_index,
    bool json_output) -> bool {
    if (!parse_index(arguments[2], table_index)) {
        print_parse_error(command,
                          "invalid table index: " + std::string(arguments[2]),
                          json_output);
        return false;
    }
    if (!parse_index(arguments[3], row_index)) {
        print_parse_error(command,
                          "invalid row index: " + std::string(arguments[3]),
                          json_output);
        return false;
    }
    if (!parse_index(arguments[4], cell_index)) {
        print_parse_error(command,
                          "invalid cell index: " + std::string(arguments[4]),
                          json_output);
        return false;
    }
    return true;
}

auto open_table_cell_for_style(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc, std::size_t table_index, std::size_t row_index,
    std::size_t cell_index, const table_cell_style_options &options,
    featherdoc::TableCell &cell) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    return resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                   command, options.json_output);
}

auto report_invalid_table_cell_target(std::string_view command,
                                      std::string_view summary,
                                      std::string detail,
                                      bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

void write_json_table_cell_location(std::ostream &stream,
                                    std::size_t table_index,
                                    std::size_t row_index,
                                    std::size_t cell_index) {
    stream << ",\"table_index\":" << table_index
           << ",\"row_index\":" << row_index
           << ",\"cell_index\":" << cell_index;
}

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

auto run_set_table_cell_vertical_alignment_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 6U) {
        print_parse_error(
            command,
            "set-table-cell-vertical-alignment expects an input path, a table index, a row index, a cell index, and an alignment",
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

    featherdoc::cell_vertical_alignment alignment =
        featherdoc::cell_vertical_alignment::top;
    if (!parse_cell_vertical_alignment_text(arguments[5], alignment)) {
        print_parse_error(command,
                          "invalid vertical alignment: " +
                              std::string(arguments[5]),
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

    if (!cell.set_vertical_alignment(alignment)) {
        report_invalid_table_cell_target(
            command, "failed to set table cell vertical alignment",
            "target table cell handle is not valid", options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, cell_index, alignment](
                std::ostream &stream) {
                write_json_table_cell_location(stream, table_index, row_index,
                                               cell_index);
                stream << ",\"vertical_alignment\":";
                write_json_string(stream,
                                  cell_vertical_alignment_name(alignment));
            });
    }
    return 0;
}

auto run_clear_table_cell_vertical_alignment_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "clear-table-cell-vertical-alignment expects an input path, a table index, a row index, and a cell index",
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

    if (!cell.clear_vertical_alignment()) {
        report_invalid_table_cell_target(
            command, "failed to clear table cell vertical alignment",
            "target table cell handle is not valid", options.json_output);
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

auto run_set_table_cell_text_direction_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 6U) {
        print_parse_error(
            command,
            "set-table-cell-text-direction expects an input path, a table index, a row index, a cell index, and a direction",
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

    featherdoc::cell_text_direction direction =
        featherdoc::cell_text_direction::left_to_right_top_to_bottom;
    if (!parse_cell_text_direction_text(arguments[5], direction)) {
        print_parse_error(command,
                          "invalid text direction: " +
                              std::string(arguments[5]),
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

    if (!cell.set_text_direction(direction)) {
        report_invalid_table_cell_target(
            command, "failed to set table cell text direction",
            "target table cell handle is not valid", options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, cell_index, direction](
                std::ostream &stream) {
                write_json_table_cell_location(stream, table_index, row_index,
                                               cell_index);
                stream << ",\"text_direction\":";
                write_json_string(stream, cell_text_direction_name(direction));
            });
    }
    return 0;
}

auto run_clear_table_cell_text_direction_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "clear-table-cell-text-direction expects an input path, a table index, a row index, and a cell index",
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

    if (!cell.clear_text_direction()) {
        report_invalid_table_cell_target(
            command, "failed to clear table cell text direction",
            "target table cell handle is not valid", options.json_output);
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

} // namespace

auto is_table_cell_style_command(std::string_view command) -> bool {
    return command == "set-table-cell-fill" ||
           command == "clear-table-cell-fill" ||
           command == "set-table-cell-vertical-alignment" ||
           command == "clear-table-cell-vertical-alignment" ||
           command == "set-table-cell-text-direction" ||
           command == "clear-table-cell-text-direction" ||
           command == "set-table-cell-width" ||
           command == "clear-table-cell-width";
}

auto run_table_cell_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "set-table-cell-fill") {
        return run_set_table_cell_fill_command(command, arguments, doc);
    }
    if (command == "clear-table-cell-fill") {
        return run_clear_table_cell_fill_command(command, arguments, doc);
    }
    if (command == "set-table-cell-vertical-alignment") {
        return run_set_table_cell_vertical_alignment_command(command, arguments,
                                                            doc);
    }
    if (command == "clear-table-cell-vertical-alignment") {
        return run_clear_table_cell_vertical_alignment_command(command, arguments,
                                                              doc);
    }
    if (command == "set-table-cell-text-direction") {
        return run_set_table_cell_text_direction_command(command, arguments, doc);
    }
    if (command == "clear-table-cell-text-direction") {
        return run_clear_table_cell_text_direction_command(command, arguments,
                                                          doc);
    }
    if (command == "set-table-cell-width") {
        return run_set_table_cell_width_command(command, arguments, doc);
    }
    if (command == "clear-table-cell-width") {
        return run_clear_table_cell_width_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
