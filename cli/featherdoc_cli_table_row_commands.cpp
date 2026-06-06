#include "featherdoc_cli_table_row_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

struct table_row_target {
    std::size_t table_index = 0U;
    std::size_t row_index = 0U;
};

auto report_body_table_row_failure(std::string_view command,
                                   std::string_view summary,
                                   std::string detail, bool json_output)
    -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto parse_body_table_index(std::string_view command,
                            const std::vector<std::string_view> &arguments,
                            std::size_t argument_index,
                            std::string_view label,
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

auto parse_body_table_row_target(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t minimum_argument_count, std::string_view usage_message,
    table_row_target &target, bool json_output) -> bool {
    if (arguments.size() < minimum_argument_count) {
        print_parse_error(command, std::string(usage_message), json_output);
        return false;
    }

    if (!parse_body_table_index(command, arguments, 2U, "table index",
                                target.table_index, json_output)) {
        return false;
    }

    if (!parse_body_table_index(command, arguments, 3U, "row index",
                                target.row_index, json_output)) {
        return false;
    }

    return true;
}

auto parse_body_table_row_mutation_options(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t options_start_index, table_cell_style_options &options,
    bool json_output) -> bool {
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, options_start_index, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    return true;
}

auto resolve_body_table_for_row(std::string_view command,
                                featherdoc::Document &doc,
                                std::size_t table_index,
                                featherdoc::Table &table, bool json_output)
    -> bool {
    table = doc.tables();
    for (std::size_t current_index = 0;
         current_index < table_index && table.has_next();
         ++current_index) {
        table.next();
    }

    if (table.has_next()) {
        return true;
    }

    return report_body_table_row_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto resolve_body_table_row_for_row(std::string_view command,
                                    featherdoc::Document &doc,
                                    std::size_t table_index,
                                    std::size_t row_index,
                                    featherdoc::TableRow &row,
                                    bool json_output) -> bool {
    featherdoc::Table table;
    if (!resolve_body_table_for_row(command, doc, table_index, table,
                                    json_output)) {
        return false;
    }

    row = table.rows();
    for (std::size_t current_index = 0;
         current_index < row_index && row.has_next();
         ++current_index) {
        row.next();
    }

    if (row.has_next()) {
        return true;
    }

    return report_body_table_row_failure(
        command, "row index is out of range",
        "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'",
        json_output);
}

auto load_body_table_summary(std::string_view command, featherdoc::Document &doc,
                             std::size_t table_index,
                             featherdoc::table_inspection_summary &table,
                             bool json_output) -> bool {
    const auto inspected_table = doc.inspect_table(table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, json_output);
        return false;
    }

    if (inspected_table.has_value()) {
        table = *inspected_table;
        return true;
    }

    return report_body_table_row_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto run_insert_table_row_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    bool insert_before) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command,
                          std::string(command) +
                              " expects an input path, a table index, and a row index",
                          json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_body_table_index(command, arguments, 2U, "table index",
                                table_index, json_output)) {
        return 2;
    }

    std::size_t row_index = 0U;
    if (!parse_body_table_index(command, arguments, 3U, "row index", row_index,
                                json_output)) {
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 4U, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::TableRow row;
    if (!resolve_body_table_row_for_row(command, doc, table_index, row_index,
                                        row, options.json_output)) {
        return 1;
    }

    auto inserted_row =
        insert_before ? row.insert_row_before() : row.insert_row_after();
    if (!inserted_row.has_next()) {
        report_body_table_row_failure(
            command, "failed to insert table row",
            "cannot insert a row adjacent to row index '" +
                std::to_string(row_index) + "' in table index '" +
                std::to_string(table_index) +
                "' because the row participates in a vertical merge",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, insert_before](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"row_index\":" << row_index
                       << ",\"inserted_row_index\":"
                       << (insert_before ? row_index : row_index + 1U);
            });
    }

    return 0;
}

auto load_mutable_body_table_row(std::string_view command,
                                 const std::vector<std::string_view> &arguments,
                                 const table_cell_style_options &options,
                                 const table_row_target &target,
                                 featherdoc::Document &doc,
                                 featherdoc::TableRow &row) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    return resolve_body_table_row_for_row(command, doc, target.table_index,
                                          target.row_index, row,
                                          options.json_output);
}

enum class table_row_toggle_action {
    set_cant_split,
    clear_cant_split,
    set_repeat_header,
    clear_repeat_header
};

auto apply_table_row_toggle(featherdoc::TableRow &row,
                            table_row_toggle_action action) -> bool {
    switch (action) {
    case table_row_toggle_action::set_cant_split:
        return row.set_cant_split();
    case table_row_toggle_action::clear_cant_split:
        return row.clear_cant_split();
    case table_row_toggle_action::set_repeat_header:
        return row.set_repeats_header();
    case table_row_toggle_action::clear_repeat_header:
        return row.clear_repeats_header();
    }

    return false;
}

auto run_table_row_toggle_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::string_view usage_message, std::string_view failure_summary,
    table_row_toggle_action action, bool write_true_field,
    std::string_view true_field_name) -> int {
    const auto json_output = has_json_flag(arguments);

    table_row_target target;
    if (!parse_body_table_row_target(command, arguments, 4U, usage_message,
                                     target, json_output)) {
        return 2;
    }

    table_cell_style_options options;
    if (!parse_body_table_row_mutation_options(command, arguments, 4U, options,
                                               json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    featherdoc::TableRow row;
    if (!load_mutable_body_table_row(command, arguments, options, target, doc,
                                     row)) {
        return 1;
    }

    if (!apply_table_row_toggle(row, action)) {
        report_body_table_row_failure(command, failure_summary,
                                      "target table row handle is not valid",
                                      options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target, write_true_field,
             true_field_name](std::ostream &stream) {
                stream << ",\"table_index\":" << target.table_index
                       << ",\"row_index\":" << target.row_index;
                if (write_true_field) {
                    stream << ",\"" << true_field_name << "\":true";
                }
            });
    }

    return 0;
}

} // namespace

auto run_append_table_row_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "append-table-row expects an input path and a table index",
                          json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_body_table_index(command, arguments, 2U, "table index",
                                table_index, json_output)) {
        return 2;
    }

    append_table_row_options options;
    std::string error_message;
    if (!parse_append_table_row_options(arguments, 3U, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::table_inspection_summary inspected_table{};
    if (!load_body_table_summary(command, doc, table_index, inspected_table,
                                 options.json_output)) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_body_table_for_row(command, doc, table_index, table,
                                    options.json_output)) {
        return 1;
    }

    const auto cell_count = options.cell_count.value_or(inspected_table.column_count);
    if (cell_count == 0U) {
        report_body_table_row_failure(command, "failed to append table row",
                                      "table row must contain at least one cell",
                                      options.json_output);
        return 1;
    }

    auto row = table.append_row(cell_count);
    if (!row.has_next()) {
        report_body_table_row_failure(command, "failed to append table row",
                                      "target table handle is not valid",
                                      options.json_output);
        return 1;
    }

    const auto row_index = inspected_table.row_count;
    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index, cell_count](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"row_index\":" << row_index
                       << ",\"cell_count\":" << cell_count;
            });
    }

    return 0;
}

auto run_insert_table_row_before_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_insert_table_row_command(command, arguments, true);
}

auto run_insert_table_row_after_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_insert_table_row_command(command, arguments, false);
}

auto run_remove_table_row_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "remove-table-row expects an input path, a table index, and a row index",
            json_output);
        return 2;
    }

    std::size_t table_index = 0U;
    if (!parse_body_table_index(command, arguments, 2U, "table index",
                                table_index, json_output)) {
        return 2;
    }

    std::size_t row_index = 0U;
    if (!parse_body_table_index(command, arguments, 3U, "row index", row_index,
                                json_output)) {
        return 2;
    }

    table_cell_style_options options;
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, 4U, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::TableRow row;
    if (!resolve_body_table_row_for_row(command, doc, table_index, row_index,
                                        row, options.json_output)) {
        return 1;
    }

    if (!row.remove()) {
        report_body_table_row_failure(
            command, "failed to remove table row",
            "cannot remove the last row from table index '" +
                std::to_string(table_index) + "'",
            options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [table_index, row_index](std::ostream &stream) {
                stream << ",\"table_index\":" << table_index
                       << ",\"row_index\":" << row_index;
            });
    }

    return 0;
}

auto run_set_table_row_height_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);

    table_row_target target;
    if (!parse_body_table_row_target(
            command, arguments, 6U,
            "set-table-row-height expects an input path, a table index, a row "
            "index, a height in twips, and a height rule",
            target, json_output)) {
        return 2;
    }

    std::uint32_t height_twips = 0U;
    if (!parse_uint32(arguments[4], height_twips)) {
        print_parse_error(command,
                          "invalid height twips: " +
                              std::string(arguments[4]),
                          json_output);
        return 2;
    }

    featherdoc::row_height_rule height_rule =
        featherdoc::row_height_rule::automatic;
    if (!parse_row_height_rule_text(arguments[5], height_rule)) {
        print_parse_error(command,
                          "invalid row height rule: " +
                              std::string(arguments[5]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    if (!parse_body_table_row_mutation_options(command, arguments, 6U, options,
                                               json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    featherdoc::TableRow row;
    if (!load_mutable_body_table_row(command, arguments, options, target, doc,
                                     row)) {
        return 1;
    }

    if (!row.set_height_twips(height_twips, height_rule)) {
        report_body_table_row_failure(command, "failed to set table row height",
                                      "target table row handle is not valid",
                                      options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target, height_twips, height_rule](std::ostream &stream) {
                stream << ",\"table_index\":" << target.table_index
                       << ",\"row_index\":" << target.row_index
                       << ",\"height_twips\":" << height_twips
                       << ",\"height_rule\":";
                write_json_string(stream, row_height_rule_name(height_rule));
            });
    }

    return 0;
}

auto run_clear_table_row_height_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);

    table_row_target target;
    if (!parse_body_table_row_target(
            command, arguments, 4U,
            "clear-table-row-height expects an input path, a table index, and "
            "a row index",
            target, json_output)) {
        return 2;
    }

    table_cell_style_options options;
    if (!parse_body_table_row_mutation_options(command, arguments, 4U, options,
                                               json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    featherdoc::TableRow row;
    if (!load_mutable_body_table_row(command, arguments, options, target, doc,
                                     row)) {
        return 1;
    }

    if (!row.clear_height()) {
        report_body_table_row_failure(command,
                                      "failed to clear table row height",
                                      "target table row handle is not valid",
                                      options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target](std::ostream &stream) {
                stream << ",\"table_index\":" << target.table_index
                       << ",\"row_index\":" << target.row_index;
            });
    }

    return 0;
}

auto run_set_table_row_cant_split_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_table_row_toggle_command(
        command, arguments,
        "set-table-row-cant-split expects an input path, a table index, and a "
        "row index",
        "failed to set table row cant-split",
        table_row_toggle_action::set_cant_split, true, "cant_split");
}

auto run_clear_table_row_cant_split_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_table_row_toggle_command(
        command, arguments,
        "clear-table-row-cant-split expects an input path, a table index, and "
        "a row index",
        "failed to clear table row cant-split",
        table_row_toggle_action::clear_cant_split, false, {});
}

auto run_set_table_row_repeat_header_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_table_row_toggle_command(
        command, arguments,
        "set-table-row-repeat-header expects an input path, a table index, and "
        "a row index",
        "failed to set table row repeat-header",
        table_row_toggle_action::set_repeat_header, true, "repeats_header");
}

auto run_clear_table_row_repeat_header_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    return run_table_row_toggle_command(
        command, arguments,
        "clear-table-row-repeat-header expects an input path, a table index, "
        "and a row index",
        "failed to clear table row repeat-header",
        table_row_toggle_action::clear_repeat_header, false, {});
}

} // namespace featherdoc_cli
