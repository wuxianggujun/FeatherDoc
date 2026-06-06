#include "featherdoc_cli_template_table_text_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_row_summary.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_table_mutation_options_parse.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_template_table_output.hpp"
#include "featherdoc_cli_template_table_resolve.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

struct template_table_text_context {
    featherdoc::Document doc;
    selected_template_part selected;
    std::size_t table_index = 0U;
    featherdoc::Table table;
};

template <typename Target, typename Options>
auto merge_target_bookmark(Target &target, const Options &options) -> void {
    if (!target.selector.bookmark_name.has_value() &&
        options.bookmark_name.has_value()) {
        target.selector.bookmark_name = options.bookmark_name;
    }
}

template <typename Target>
auto validate_target_selector(std::string_view command, bool json_output,
                              Target &target, std::string &error_message)
    -> bool {
    if (!validate_template_table_selector(target.selector, false,
                                          target.has_header_row_index,
                                          target.has_occurrence,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    return true;
}

template <typename Options>
auto load_template_table_text_context(
    std::string_view command, const std::vector<std::string_view> &arguments,
    const Options &options, const featherdoc::template_table_selector &selector,
    template_table_text_context &context, std::string &error_message) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), context.doc,
                       command, options.json_output)) {
        return false;
    }

    if (!select_mutable_template_part(context.doc, options.part,
                                      options.part_index,
                                      options.section_index,
                                      options.reference_kind, context.selected,
                                      error_message)) {
        report_operation_failure(command, "mutate", error_message,
                                 context.doc.last_error(), options.json_output);
        return false;
    }

    if (!resolve_template_table_index(context.doc, context.selected, selector,
                                      context.table_index, command,
                                      options.json_output, "mutate")) {
        return false;
    }

    if (!resolve_template_table(context.selected, context.table_index,
                                context.table, command, options.json_output,
                                "mutate")) {
        return false;
    }

    return true;
}

auto report_template_table_text_failure(std::string_view command,
                                        std::string_view summary,
                                        std::string detail,
                                        const selected_template_part &selected,
                                        bool json_output,
                                        std::errc code =
                                            std::errc::invalid_argument)
    -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(code);
    error_info.detail = std::move(detail);
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto validate_row_text_range(std::string_view command,
                             template_table_text_context &context,
                             std::size_t start_row_index,
                             const std::vector<std::vector<std::string>> &rows,
                             bool json_output) -> bool {
    const auto row_summaries = collect_table_row_summaries(context.table);
    if (start_row_index >= row_summaries.size()) {
        report_template_table_text_failure(
            command, "row index is out of range",
            "row index '" + std::to_string(start_row_index) +
                "' is out of range for table index '" +
                std::to_string(context.table_index) + "'",
            context.selected, json_output);
        return false;
    }

    if (rows.size() > row_summaries.size() - start_row_index) {
        report_template_table_text_failure(
            command, "row range exceeds table bounds",
            "row range starting at row index '" +
                std::to_string(start_row_index) + "' with count '" +
                std::to_string(rows.size()) + "' exceeds table index '" +
                std::to_string(context.table_index) + "' row count '" +
                std::to_string(row_summaries.size()) + "'",
            context.selected, json_output);
        return false;
    }

    for (std::size_t offset = 0U; offset < rows.size(); ++offset) {
        const auto target_row_index = start_row_index + offset;
        const auto replacement_cell_count = rows[offset].size();
        const auto target_cell_count = row_summaries[target_row_index].cell_count;
        if (replacement_cell_count != target_cell_count) {
            report_template_table_text_failure(
                command, "replacement row cell count does not match target row",
                "replacement row at offset '" + std::to_string(offset) +
                    "' contains '" + std::to_string(replacement_cell_count) +
                    "' cells but target row index '" +
                    std::to_string(target_row_index) + "' contains '" +
                    std::to_string(target_cell_count) + "' cells",
                context.selected, json_output);
            return false;
        }
    }

    return true;
}

auto validate_cell_block_text_range(
    std::string_view command, template_table_text_context &context,
    std::size_t start_row_index, std::size_t start_cell_index,
    const std::vector<std::vector<std::string>> &rows, bool json_output)
    -> bool {
    const auto row_summaries = collect_table_row_summaries(context.table);
    if (start_row_index >= row_summaries.size()) {
        report_template_table_text_failure(
            command, "row index is out of range",
            "row index '" + std::to_string(start_row_index) +
                "' is out of range for table index '" +
                std::to_string(context.table_index) + "'",
            context.selected, json_output);
        return false;
    }

    if (rows.size() > row_summaries.size() - start_row_index) {
        report_template_table_text_failure(
            command, "row range exceeds table bounds",
            "row range starting at row index '" +
                std::to_string(start_row_index) + "' with count '" +
                std::to_string(rows.size()) + "' exceeds table index '" +
                std::to_string(context.table_index) + "' row count '" +
                std::to_string(row_summaries.size()) + "'",
            context.selected, json_output);
        return false;
    }

    for (std::size_t offset = 0U; offset < rows.size(); ++offset) {
        const auto target_row_index = start_row_index + offset;
        const auto replacement_cell_count = rows[offset].size();
        const auto target_cell_count = row_summaries[target_row_index].cell_count;
        if (start_cell_index >= target_cell_count ||
            replacement_cell_count > target_cell_count - start_cell_index) {
            report_template_table_text_failure(
                command, "cell block exceeds target row bounds",
                "replacement row at offset '" + std::to_string(offset) +
                    "' starting at cell index '" +
                    std::to_string(start_cell_index) + "' with count '" +
                    std::to_string(replacement_cell_count) +
                    "' exceeds target row index '" +
                    std::to_string(target_row_index) + "' cell count '" +
                    std::to_string(target_cell_count) + "'",
                context.selected, json_output);
            return false;
        }
    }

    return true;
}

} // namespace

auto run_set_template_table_cell_text_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-template-table-cell-text expects an input path plus either "
            "<table-index> <row-index> (<cell-index>|--grid-column <index>), "
            "--bookmark <name> <row-index> (<cell-index>|--grid-column <index>), "
            "or a text-based table selector followed by <row-index> "
            "(<cell-index>|--grid-column <index>)",
            json_output);
        return 2;
    }

    parsed_template_table_selector_optional_cell_target target;
    std::string error_message;
    if (!parse_template_table_selector_optional_cell_target_arguments(
            arguments, 2U, target, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    template_table_cell_text_options options;
    options.bookmark_name = target.selector.bookmark_name;
    if (!parse_template_table_cell_text_options(arguments,
                                                target.options_start_index,
                                                options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    if (!target.cell_index.has_value() && !options.grid_column.has_value()) {
        print_parse_error(command,
                          "expected a cell index or --grid-column <index>",
                          json_output);
        return 2;
    }
    if (target.cell_index.has_value() && options.grid_column.has_value()) {
        print_parse_error(command,
                          "cell index and --grid-column are mutually exclusive",
                          json_output);
        return 2;
    }

    merge_target_bookmark(target, options);
    if (!validate_target_selector(command, json_output, target, error_message)) {
        return 2;
    }

    std::string replacement_text;
    if (!read_text_source(options, replacement_text, error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "input",
                                     error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_mutable_template_part(doc, options.part, options.part_index,
                                      options.section_index,
                                      options.reference_kind, selected,
                                      error_message)) {
        report_operation_failure(command, "mutate", error_message,
                                 doc.last_error(), options.json_output);
        return 1;
    }

    std::size_t resolved_table_index = 0U;
    if (!resolve_template_table_index(doc, selected, target.selector,
                                      resolved_table_index, command,
                                      options.json_output, "mutate")) {
        return 1;
    }

    std::optional<featherdoc::table_cell_inspection_summary>
        resolved_cell_summary;
    featherdoc::TableCell cell;
    if (options.grid_column.has_value()) {
        resolved_cell_summary = selected.part.inspect_table_cell_by_grid_column(
            resolved_table_index, target.row_index, *options.grid_column);
        if (!resolve_template_table_cell_by_grid_column(
                selected, resolved_table_index, target.row_index,
                *options.grid_column, cell, command, options.json_output)) {
            return 1;
        }
    } else if (!resolve_template_table_cell(
                   selected, resolved_table_index, target.row_index,
                   *target.cell_index, cell, command, options.json_output)) {
        return 1;
    }

    if (!cell.set_text(replacement_text)) {
        report_template_table_text_failure(
            command, "failed to set table cell text",
            "failed to set text for the requested table cell", selected,
            options.json_output, std::errc::io_error);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, resolved_table_index, &target, &options,
             &resolved_cell_summary](std::ostream &stream) {
                stream << ',';
                write_json_selected_template_part(stream, selected);
                stream << ",\"table_index\":" << resolved_table_index
                       << ",\"row_index\":" << target.row_index;
                if (options.grid_column.has_value()) {
                    stream << ",\"grid_column\":" << *options.grid_column;
                    if (resolved_cell_summary.has_value()) {
                        stream << ",\"cell_index\":"
                               << resolved_cell_summary->cell_index
                               << ",\"column_index\":"
                               << resolved_cell_summary->column_index
                               << ",\"column_span\":"
                               << resolved_cell_summary->column_span;
                    }
                } else {
                    stream << ",\"cell_index\":" << *target.cell_index;
                }
            });
    }

    return 0;
}

auto run_set_template_table_row_texts_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-template-table-row-texts expects an input path plus either "
            "<table-index> <start-row-index>, --bookmark <name> "
            "<start-row-index>, or a text-based table selector followed by "
            "<start-row-index>",
            json_output);
        return 2;
    }

    parsed_template_table_selector_row_target target;
    std::string error_message;
    if (!parse_template_table_selector_row_target_arguments(
            arguments, 2U, target, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    template_table_row_texts_options options;
    options.bookmark_name = target.selector.bookmark_name;
    if (!parse_template_table_row_texts_options(
            arguments, target.options_start_index, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    merge_target_bookmark(target, options);
    if (!validate_target_selector(command, json_output, target, error_message)) {
        return 2;
    }

    template_table_text_context context;
    if (!load_template_table_text_context(command, arguments, options,
                                          target.selector, context,
                                          error_message)) {
        return 1;
    }

    if (!validate_row_text_range(command, context, target.row_index,
                                 options.rows, options.json_output)) {
        return 1;
    }

    if (!context.table.set_rows_texts(target.row_index, options.rows)) {
        report_template_table_text_failure(
            command, "failed to set template table row texts",
            "failed to set text for the requested table row range",
            context.selected, options.json_output, std::errc::io_error);
        return 1;
    }

    if (!save_document(context.doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, context.doc, options.output_path,
            [&context, &target, &options](std::ostream &stream) {
                write_json_template_table_row_texts_result(
                    stream, context.selected, context.table_index,
                    target.row_index, options.rows, options.bookmark_name);
            });
    } else {
        print_template_table_row_texts_result(
            context.selected, context.table_index, target.row_index,
            options.rows, options.bookmark_name, options.output_path);
    }

    return 0;
}

auto run_set_template_table_cell_block_texts_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "set-template-table-cell-block-texts expects an input path plus "
            "either <table-index> <start-row-index> <start-cell-index>, "
            "--bookmark <name> <start-row-index> <start-cell-index>, or "
            "a text-based table selector followed by <start-row-index> "
            "<start-cell-index>",
            json_output);
        return 2;
    }

    parsed_template_table_selector_cell_target target;
    std::string error_message;
    if (!parse_template_table_selector_cell_target_arguments(
            arguments, 2U, target, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    template_table_row_texts_options options;
    options.bookmark_name = target.selector.bookmark_name;
    if (!parse_template_table_row_texts_options(
            arguments, target.options_start_index, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    merge_target_bookmark(target, options);
    if (!validate_target_selector(command, json_output, target, error_message)) {
        return 2;
    }

    template_table_text_context context;
    if (!load_template_table_text_context(command, arguments, options,
                                          target.selector, context,
                                          error_message)) {
        return 1;
    }

    if (!validate_cell_block_text_range(command, context, target.row_index,
                                        target.cell_index, options.rows,
                                        options.json_output)) {
        return 1;
    }

    if (!context.table.set_cell_block_texts(target.row_index, target.cell_index,
                                            options.rows)) {
        report_template_table_text_failure(
            command, "failed to set template table cell block texts",
            "failed to set text for the requested table cell block",
            context.selected, options.json_output, std::errc::io_error);
        return 1;
    }

    if (!save_document(context.doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, context.doc, options.output_path,
            [&context, &target, &options](std::ostream &stream) {
                write_json_template_table_cell_block_texts_result(
                    stream, context.selected, context.table_index,
                    target.row_index, target.cell_index, options.rows,
                    options.bookmark_name);
            });
    } else {
        print_template_table_cell_block_texts_result(
            context.selected, context.table_index, target.row_index,
            target.cell_index, options.rows, options.bookmark_name,
            options.output_path);
    }

    return 0;
}

} // namespace featherdoc_cli
