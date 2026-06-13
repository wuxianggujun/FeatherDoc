#include "featherdoc_cli_template_table_inspect_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_table_inspect_commands_detail.hpp"
#include "featherdoc_cli_template_table_inspect_output.hpp"
#include "featherdoc_cli_template_table_resolve.hpp"

#include <featherdoc.hpp>

#include <optional>
#include <string>

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
