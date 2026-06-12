#include "featherdoc_cli_template_table_row_commands.hpp"

#include "featherdoc_cli_template_table_row_commands_detail.hpp"

#include <featherdoc.hpp>

#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

auto run_insert_template_table_row_before_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "insert-template-table-row-before expects an input path plus "
            "either <table-index> <row-index>, --bookmark <name> "
            "<row-index>, or a text-based table selector followed "
            "by <row-index>",
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

    template_table_row_mutation_options options;
    options.bookmark_name = target.selector.bookmark_name;
    if (!parse_template_table_row_mutation_options(
            arguments, target.options_start_index, options, error_message)) {
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

    featherdoc::TableRow row;
    if (!resolve_template_table_row(context.selected, context.table_index,
                                    target.row_index, row, command,
                                    options.json_output, "mutate")) {
        return 1;
    }

    auto inserted_row = row.insert_row_before();
    if (!inserted_row.has_next()) {
        report_template_row_failure(
            command, "failed to insert table row",
            "cannot insert a row adjacent to row index '" +
                std::to_string(target.row_index) + "' in table index '" +
                std::to_string(context.table_index) +
                "' because the row participates in a vertical merge",
            context.selected, options.json_output);
        return 1;
    }

    if (!save_document(context.doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, context.doc, options.output_path,
            [&context, &target](std::ostream &stream) {
                stream << ',';
                write_json_selected_template_part(stream, context.selected);
                stream << ",\"table_index\":" << context.table_index
                       << ",\"row_index\":" << target.row_index
                       << ",\"inserted_row_index\":" << target.row_index;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
