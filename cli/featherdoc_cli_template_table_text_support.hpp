#pragma once

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_template_table_resolve.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {

struct template_table_text_context {
    featherdoc::Document doc;
    selected_template_part selected;
    std::size_t table_index = 0U;
    featherdoc::Table table;
};

template <typename Target, typename Options>
auto merge_template_table_text_target_bookmark(Target &target,
                                               const Options &options)
    -> void {
    if (!target.selector.bookmark_name.has_value() &&
        options.bookmark_name.has_value()) {
        target.selector.bookmark_name = options.bookmark_name;
    }
}

template <typename Target>
auto validate_template_table_text_target_selector(
    std::string_view command, bool json_output, Target &target,
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

auto report_template_table_text_failure(
    std::string_view command, std::string_view summary, std::string detail,
    const selected_template_part &selected, bool json_output,
    std::errc code = std::errc::invalid_argument) -> bool;

auto validate_template_table_row_text_range(
    std::string_view command, template_table_text_context &context,
    std::size_t start_row_index,
    const std::vector<std::vector<std::string>> &rows, bool json_output)
    -> bool;

auto validate_template_table_cell_block_text_range(
    std::string_view command, template_table_text_context &context,
    std::size_t start_row_index, std::size_t start_cell_index,
    const std::vector<std::vector<std::string>> &rows, bool json_output)
    -> bool;

} // namespace featherdoc_cli
