#include "featherdoc_cli_table_row_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_row_support.hpp"

#include <ostream>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

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
