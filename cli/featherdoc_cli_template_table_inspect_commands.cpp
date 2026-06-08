#include "featherdoc_cli_template_table_inspect_commands.hpp"

#include "featherdoc_cli_template_table_inspect_output.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_template_table_resolve.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto report_template_table_range_failure(
    std::string_view command, std::string_view summary, std::string detail,
    const selected_template_part &selected, bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, "inspect", summary, error_info,
                                    json_output);
}

auto select_inspect_template_part(
    std::string_view command, featherdoc::Document &doc,
    const template_inspect_tables_options &options, selected_template_part &selected,
    std::string &error_message) -> bool {
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "inspect", error_message,
                                 doc.last_error(), options.json_output);
        return false;
    }

    return true;
}

template <typename Options>
auto select_inspect_template_part(
    std::string_view command, featherdoc::Document &doc, const Options &options,
    selected_template_part &selected, std::string &error_message) -> bool {
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "inspect", error_message,
                                 doc.last_error(), options.json_output);
        return false;
    }

    return true;
}

} // namespace

auto run_inspect_template_tables_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-template-tables expects an input path",
                          json_output);
        return 2;
    }

    template_inspect_tables_options options;
    std::string error_message;
    if (!parse_template_inspect_tables_options(arguments, 2U, options,
                                               error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_inspect_template_part(command, doc, options, selected,
                                      error_message)) {
        return 1;
    }

    if (options.table_index.has_value() || options.bookmark_name.has_value()) {
        std::size_t table_index = 0U;
        if (!resolve_template_table_index(doc, selected, options.table_index,
                                          options.bookmark_name, table_index,
                                          command, options.json_output,
                                          "inspect")) {
            return 1;
        }

        const auto table = selected.part.inspect_table(table_index);
        if (!table.has_value()) {
            report_template_table_range_failure(
                command, "table index is out of range",
                "table index '" + std::to_string(table_index) +
                    "' is out of range",
                selected, options.json_output);
            return 1;
        }

        inspect_template_table(selected, *table, options.json_output);
        return 0;
    }

    const auto tables = selected.part.inspect_tables();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    inspect_template_tables(selected, tables, options.json_output);
    return 0;
}

auto run_inspect_template_table_rows_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-template-table-rows expects an input path plus a "
            "table selector",
            json_output);
        return 2;
    }

    std::optional<std::size_t> table_index;
    std::size_t options_start_index = 2U;
    if (arguments[2].size() < 2U || arguments[2].substr(0U, 2U) != "--") {
        std::size_t parsed_table_index = 0U;
        if (!parse_index(arguments[2], parsed_table_index)) {
            print_parse_error(command,
                              "invalid table index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }
        table_index = parsed_table_index;
        options_start_index = 3U;
    }

    template_inspect_table_rows_options options;
    std::string error_message;
    if (!parse_template_inspect_table_rows_options(arguments, options_start_index,
                                                   options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    if (table_index.has_value()) {
        options.selector.table_index = table_index;
    }
    if (!validate_template_table_selector(options.selector, false,
                                          options.has_header_row_index,
                                          options.has_occurrence,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_inspect_template_part(command, doc, options, selected,
                                      error_message)) {
        return 1;
    }

    std::size_t resolved_table_index = 0U;
    if (!resolve_template_table_index(doc, selected, options.selector,
                                      resolved_table_index, command,
                                      options.json_output, "inspect")) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_template_table(selected, resolved_table_index, table, command,
                                options.json_output, "inspect")) {
        return 1;
    }

    if (options.row_index.has_value()) {
        featherdoc::TableRow row;
        if (!resolve_template_table_row(selected, resolved_table_index,
                                        *options.row_index, row, command,
                                        options.json_output, "inspect")) {
            return 1;
        }

        inspect_template_table_row(
            selected, resolved_table_index,
            make_table_row_summary(row, *options.row_index),
            options.json_output);
        return 0;
    }

    inspect_template_table_rows(selected, resolved_table_index,
                                collect_table_row_summaries(table),
                                options.json_output);
    return 0;
}

auto run_inspect_template_table_cells_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-template-table-cells expects an input path plus a "
            "table selector",
            json_output);
        return 2;
    }

    parsed_template_table_selector_target target;
    template_inspect_table_cells_options options;
    std::string error_message;
    if (!parse_template_table_selector_target_arguments(arguments, 2U, target,
                                                        true, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    options.bookmark_name = target.selector.bookmark_name;
    if (!parse_template_inspect_table_cells_options(
            arguments, target.options_start_index, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    if (!target.selector.bookmark_name.has_value() &&
        options.bookmark_name.has_value()) {
        target.selector.bookmark_name = options.bookmark_name;
    }
    if (!validate_template_table_selector(target.selector, false,
                                          target.has_header_row_index,
                                          target.has_occurrence,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_inspect_template_part(command, doc, options, selected,
                                      error_message)) {
        return 1;
    }

    std::size_t resolved_table_index = 0U;
    if (!resolve_template_table_index(doc, selected, target.selector,
                                      resolved_table_index, command,
                                      options.json_output, "inspect")) {
        return 1;
    }

    const auto table = selected.part.inspect_table(resolved_table_index);
    if (!table.has_value()) {
        report_template_table_range_failure(
            command, "table index is out of range",
            "table index '" + std::to_string(resolved_table_index) +
                "' is out of range",
            selected, options.json_output);
        return 1;
    }

    if (options.cell_index.has_value()) {
        const auto cell = selected.part.inspect_table_cell(
            resolved_table_index, *options.row_index, *options.cell_index);
        if (!cell.has_value()) {
            auto detail =
                "cell index '" + std::to_string(*options.cell_index) +
                "' is out of range for row index '" +
                std::to_string(*options.row_index) + "' in table index '" +
                std::to_string(resolved_table_index) + "'";
            if (*options.row_index >= table->row_count) {
                detail = "row index '" + std::to_string(*options.row_index) +
                         "' is out of range for table index '" +
                         std::to_string(resolved_table_index) + "'";
            }
            report_template_table_range_failure(
                command, "table cell selector is out of range", detail,
                selected, options.json_output);
            return 1;
        }

        inspect_template_table_cell(selected, resolved_table_index, *cell,
                                    options.json_output);
        return 0;
    }

    if (options.grid_column.has_value()) {
        const auto cell = selected.part.inspect_table_cell_by_grid_column(
            resolved_table_index, *options.row_index, *options.grid_column);
        if (!cell.has_value()) {
            auto detail =
                "grid column '" + std::to_string(*options.grid_column) +
                "' is out of range for row index '" +
                std::to_string(*options.row_index) + "' in table index '" +
                std::to_string(resolved_table_index) + "'";
            if (*options.row_index >= table->row_count) {
                detail = "row index '" + std::to_string(*options.row_index) +
                         "' is out of range for table index '" +
                         std::to_string(resolved_table_index) + "'";
            }
            report_template_table_range_failure(
                command, "table cell selector is out of range", detail,
                selected, options.json_output);
            return 1;
        }

        inspect_template_table_cell(selected, resolved_table_index, *cell,
                                    options.json_output);
        return 0;
    }

    const auto cells = selected.part.inspect_table_cells(resolved_table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    if (options.row_index.has_value()) {
        std::vector<featherdoc::table_cell_inspection_summary> row_cells;
        for (const auto &cell : cells) {
            if (cell.row_index == *options.row_index) {
                row_cells.push_back(cell);
            }
        }

        if (row_cells.empty() && *options.row_index >= table->row_count) {
            report_template_table_range_failure(
                command, "row index is out of range",
                "row index '" + std::to_string(*options.row_index) +
                    "' is out of range for table index '" +
                    std::to_string(resolved_table_index) + "'",
                selected, options.json_output);
            return 1;
        }

        inspect_template_table_cells(selected, resolved_table_index,
                                     options.row_index, row_cells,
                                     options.json_output);
        return 0;
    }

    inspect_template_table_cells(selected, resolved_table_index, std::nullopt,
                                 cells, options.json_output);
    return 0;
}

} // namespace featherdoc_cli
