#include "featherdoc_cli_template_table_text_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_table_mutation_options_parse.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_template_table_output.hpp"
#include "featherdoc_cli_template_table_text_support.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

namespace featherdoc_cli {

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

    merge_template_table_text_target_bookmark(target, options);
    if (!validate_template_table_text_target_selector(
            command, json_output, target, error_message)) {
        return 2;
    }

    template_table_text_context context;
    if (!load_template_table_text_context(command, arguments, options,
                                          target.selector, context,
                                          error_message)) {
        return 1;
    }

    if (!validate_template_table_row_text_range(
            command, context, target.row_index, options.rows,
            options.json_output)) {
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

    merge_template_table_text_target_bookmark(target, options);
    if (!validate_template_table_text_target_selector(
            command, json_output, target, error_message)) {
        return 2;
    }

    template_table_text_context context;
    if (!load_template_table_text_context(command, arguments, options,
                                          target.selector, context,
                                          error_message)) {
        return 1;
    }

    if (!validate_template_table_cell_block_text_range(
            command, context, target.row_index, target.cell_index,
            options.rows, options.json_output)) {
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
