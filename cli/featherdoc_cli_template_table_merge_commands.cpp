#include "featherdoc_cli_template_table_merge_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_table_mutation_options_parse.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_template_table_resolve.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

struct template_table_merge_context {
    featherdoc::Document doc;
    selected_template_part selected;
    parsed_template_table_selector_cell_target target;
    std::size_t table_index = 0U;
    featherdoc::TableCell cell;
};

template <typename Options>
auto merge_target_bookmark(parsed_template_table_selector_cell_target &target,
                           const Options &options) -> void {
    if (!target.selector.bookmark_name.has_value() &&
        options.bookmark_name.has_value()) {
        target.selector.bookmark_name = options.bookmark_name;
    }
}

auto validate_target_selector(
    std::string_view command, bool json_output,
    parsed_template_table_selector_cell_target &target,
    std::string &error_message) -> bool {
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
auto load_template_table_merge_context(
    std::string_view command, const std::vector<std::string_view> &arguments,
    const Options &options, template_table_merge_context &context,
    std::string &error_message) -> bool {
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

    if (!resolve_template_table_index(
            context.doc, context.selected, context.target.selector,
            context.table_index, command, options.json_output, "mutate")) {
        return false;
    }

    if (!resolve_template_table_cell(
            context.selected, context.table_index, context.target.row_index,
            context.target.cell_index, context.cell, command,
            options.json_output)) {
        return false;
    }

    return true;
}

auto report_template_merge_failure(std::string_view command,
                                   std::string_view summary, std::string detail,
                                   const selected_template_part &selected,
                                   bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto write_json_template_table_merge_target(
    std::ostream &stream, const template_table_merge_context &context) -> void {
    stream << ',';
    write_json_selected_template_part(stream, context.selected);
    stream << ",\"table_index\":" << context.table_index
           << ",\"row_index\":" << context.target.row_index
           << ",\"cell_index\":" << context.target.cell_index
           << ",\"direction\":";
}

} // namespace

auto run_merge_template_table_cells_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "merge-template-table-cells expects an input path plus either "
            "<table-index> <row-index> <cell-index> or "
            "--bookmark <name> <row-index> <cell-index>, or a "
            "text-based table selector followed by <row-index> <cell-index>",
            json_output);
        return 2;
    }

    template_table_merge_context context;
    std::string error_message;
    if (!parse_template_table_selector_cell_target_arguments(
            arguments, 2U, context.target, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    template_merge_table_cells_options options;
    options.bookmark_name = context.target.selector.bookmark_name;
    if (!parse_template_merge_table_cells_options(
            arguments, context.target.options_start_index, options,
            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    merge_target_bookmark(context.target, options);
    if (!validate_target_selector(command, json_output, context.target,
                                  error_message)) {
        return 2;
    }

    if (!load_template_table_merge_context(command, arguments, options, context,
                                           error_message)) {
        return 1;
    }

    const auto success =
        options.direction == table_merge_direction::right
            ? context.cell.merge_right(options.count)
            : context.cell.merge_down(options.count);
    if (!success) {
        report_template_merge_failure(
            command, "failed to merge table cells",
            "cell at table index '" + std::to_string(context.table_index) +
                "', row index '" + std::to_string(context.target.row_index) +
                "', and cell index '" +
                std::to_string(context.target.cell_index) +
                "' could not be merged towards '" +
                std::string(table_merge_direction_name(options.direction)) +
                "' with count '" + std::to_string(options.count) + "'",
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
            [&context, &options](std::ostream &stream) {
                write_json_template_table_merge_target(stream, context);
                write_json_string(stream,
                                  table_merge_direction_name(options.direction));
                stream << ",\"count\":" << options.count;
            });
    }

    return 0;
}

auto run_unmerge_template_table_cells_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(
            command,
            "unmerge-template-table-cells expects an input path plus either "
            "<table-index> <row-index> <cell-index> or "
            "--bookmark <name> <row-index> <cell-index>, or a "
            "text-based table selector followed by <row-index> <cell-index>",
            json_output);
        return 2;
    }

    template_table_merge_context context;
    std::string error_message;
    if (!parse_template_table_selector_cell_target_arguments(
            arguments, 2U, context.target, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    template_unmerge_table_cells_options options;
    options.bookmark_name = context.target.selector.bookmark_name;
    if (!parse_template_unmerge_table_cells_options(
            arguments, context.target.options_start_index, options,
            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    merge_target_bookmark(context.target, options);
    if (!validate_target_selector(command, json_output, context.target,
                                  error_message)) {
        return 2;
    }

    if (!load_template_table_merge_context(command, arguments, options, context,
                                           error_message)) {
        return 1;
    }

    const auto success =
        options.direction == table_merge_direction::right
            ? context.cell.unmerge_right()
            : context.cell.unmerge_down();
    if (!success) {
        report_template_merge_failure(
            command, "failed to unmerge table cells",
            "cell at table index '" + std::to_string(context.table_index) +
                "', row index '" + std::to_string(context.target.row_index) +
                "', and cell index '" +
                std::to_string(context.target.cell_index) +
                "' could not be unmerged towards '" +
                std::string(table_merge_direction_name(options.direction)) + "'",
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
            [&context, &options](std::ostream &stream) {
                write_json_template_table_merge_target(stream, context);
                write_json_string(stream,
                                  table_merge_direction_name(options.direction));
            });
    }

    return 0;
}

} // namespace featherdoc_cli
