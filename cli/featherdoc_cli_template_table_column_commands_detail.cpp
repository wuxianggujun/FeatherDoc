#include "featherdoc_cli_template_table_column_commands_detail.hpp"

#include <system_error>
#include <utility>

namespace featherdoc_cli {

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

auto load_template_table_column_context(
    std::string_view command, const std::vector<std::string_view> &arguments,
    const template_table_cell_mutation_options &options,
    const featherdoc::template_table_selector &selector,
    template_table_column_context &context, std::string &error_message) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), context.doc,
                       command, options.json_output)) {
        return false;
    }

    if (!select_mutable_template_part(context.doc, options.part,
                                      options.part_index, options.section_index,
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

    return true;
}

auto report_template_column_failure(std::string_view command,
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

auto parse_template_table_column_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::string_view usage_message,
    parsed_template_table_selector_cell_target &target,
    template_table_cell_mutation_options &options) -> bool {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 5U) {
        print_parse_error(command, std::string(usage_message), json_output);
        return false;
    }

    std::string error_message;
    if (!parse_template_table_selector_cell_target_arguments(
            arguments, 2U, target, error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    options.bookmark_name = target.selector.bookmark_name;
    if (!parse_template_table_cell_mutation_options(
            arguments, target.options_start_index, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    merge_target_bookmark(target, options);
    return validate_target_selector(command, json_output, target, error_message);
}

} // namespace featherdoc_cli
