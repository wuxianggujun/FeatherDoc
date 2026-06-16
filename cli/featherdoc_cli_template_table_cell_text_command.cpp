#include "featherdoc_cli_template_table_text_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_table_mutation_options_parse.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_template_table_resolve.hpp"
#include "featherdoc_cli_template_table_text_support.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

namespace featherdoc_cli {

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

    merge_template_table_text_target_bookmark(target, options);
    if (!validate_template_table_text_target_selector(
            command, json_output, target, error_message)) {
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

} // namespace featherdoc_cli
