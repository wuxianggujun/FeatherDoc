#include "featherdoc_cli_template_table_row_commands.hpp"

#include "featherdoc_cli_template_table_row_commands_detail.hpp"

#include <featherdoc.hpp>

#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

auto run_append_template_table_row_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "append-template-table-row expects an input path plus either a "
            "table index, --bookmark <name>, or a text-based table selector",
            json_output);
        return 2;
    }

    parsed_template_table_selector_target target;
    template_append_table_row_options options;
    std::string error_message;
    if (!parse_template_table_selector_target_arguments(arguments, 2U, target,
                                                        true, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    options.bookmark_name = target.selector.bookmark_name;
    if (!parse_template_append_table_row_options(arguments,
                                                 target.options_start_index,
                                                 options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    merge_target_bookmark(target, options);
    if (!validate_target_selector(command, json_output, target, error_message)) {
        return 2;
    }

    template_table_row_context context;
    if (!load_template_table_row_context(command, arguments, options,
                                         target.selector, context,
                                         error_message)) {
        return 1;
    }

    const auto inspected_table =
        context.selected.part.inspect_table(context.table_index);
    if (!inspected_table.has_value()) {
        report_template_row_failure(
            command, "table index is out of range",
            "table index '" + std::to_string(context.table_index) +
                "' is out of range",
            context.selected, options.json_output);
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_template_table(context.selected, context.table_index, table,
                                command, options.json_output, "mutate")) {
        return 1;
    }

    const auto cell_count =
        options.cell_count.value_or(inspected_table->column_count);
    if (cell_count == 0U) {
        report_template_row_failure(command, "failed to append table row",
                                    "table row must contain at least one cell",
                                    context.selected, options.json_output);
        return 1;
    }

    auto row = table.append_row(cell_count);
    if (!row.has_next()) {
        report_template_row_failure(command, "failed to append table row",
                                    "target table handle is not valid",
                                    context.selected, options.json_output);
        return 1;
    }

    const auto row_index = inspected_table->row_count;
    if (!save_document(context.doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, context.doc, options.output_path,
            [&context, row_index, cell_count](std::ostream &stream) {
                stream << ',';
                write_json_selected_template_part(stream, context.selected);
                stream << ",\"table_index\":" << context.table_index
                       << ",\"row_index\":" << row_index
                       << ",\"cell_count\":" << cell_count;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
