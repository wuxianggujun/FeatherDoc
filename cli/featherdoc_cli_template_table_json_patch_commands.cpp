#include "featherdoc_cli_template_table_json_patch_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_table_json_patch_apply.hpp"
#include "featherdoc_cli_template_table_json_patch_parse.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_template_table_output.hpp"
#include "featherdoc_cli_template_table_resolve.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace featherdoc_cli {

auto run_set_template_table_from_json_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-template-table-from-json expects an input path plus "
            "a table selector, followed by "
            "--patch-file <path>",
            json_output);
        return 2;
    }

    std::optional<std::size_t> table_index;
    std::optional<std::string> bookmark_name;
    std::size_t options_start_index = 0U;
    std::string error_message;
    if (!parse_template_table_target_arguments(arguments, 2U, table_index,
                                               bookmark_name, options_start_index,
                                               error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    template_table_json_patch_options options;
    if (!parse_template_table_json_patch_options(arguments, options_start_index,
                                                 options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }
    if (bookmark_name.has_value()) {
        if (options.selector.bookmark_name.has_value()) {
            print_parse_error(command, "duplicate --bookmark option",
                              options.json_output);
            return 2;
        }
        options.selector.bookmark_name = bookmark_name;
    }
    if (table_index.has_value()) {
        options.selector.table_index = table_index;
    }
    if (!validate_template_table_selector(options.selector, false,
                                          options.has_header_row_index,
                                          options.has_occurrence,
                                          error_message)) {
        print_parse_error(command, error_message, options.json_output);
        return 2;
    }

    template_table_json_patch patch;
    if (!read_template_table_json_patch(*options.patch_file, patch,
                                        error_message)) {
        print_parse_error(command, error_message, options.json_output);
        return 2;
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
    if (!resolve_template_table_index(doc, selected, options.selector,
                                      resolved_table_index, command,
                                      options.json_output, "mutate")) {
        return 1;
    }

    featherdoc::Table table;
    if (!resolve_template_table(selected, resolved_table_index, table, command,
                                options.json_output, "mutate")) {
        return 1;
    }

    if (!apply_template_table_json_patch(
            command, table, selected.part.entry_name(), resolved_table_index,
            patch, options.json_output)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, resolved_table_index, &patch, &options](
                std::ostream &stream) {
                write_json_template_table_from_json_result(
                    stream, selected, resolved_table_index, patch,
                    options.selector);
            });
    } else {
        print_template_table_from_json_result(
            selected, resolved_table_index, patch, options.selector,
            options.output_path);
    }

    return 0;
}

auto run_set_template_tables_from_json_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "set-template-tables-from-json expects an input path plus "
            "--patch-file <path>",
            json_output);
        return 2;
    }

    template_table_json_batch_options options;
    std::string error_message;
    if (!parse_template_table_json_batch_options(arguments, 2U, options,
                                                 error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::vector<template_table_json_batch_operation> operations;
    if (!read_template_table_json_batch(*options.patch_file, operations,
                                        error_message)) {
        print_parse_error(command, error_message, options.json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    std::vector<applied_template_table_json_batch_operation> applied_operations;
    applied_operations.reserve(operations.size());
    for (std::size_t operation_index = 0U; operation_index < operations.size();
         ++operation_index) {
        const auto &operation = operations[operation_index];

        validation_part_family part = validation_part_family::body;
        std::optional<std::size_t> part_index;
        std::optional<std::size_t> section_index;
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference;
        bool has_kind = false;
        featherdoc::template_table_selector selector;
        if (!resolve_template_table_json_batch_operation_selection(
                options, operation, part, part_index, section_index,
                reference_kind, has_kind, selector, error_message)) {
            print_parse_error(
                command,
                prefix_template_table_json_batch_message(operation_index,
                                                         error_message),
                options.json_output);
            return 2;
        }

        selected_template_part selected;
        if (!select_mutable_template_part(doc, part, part_index, section_index,
                                          reference_kind, selected,
                                          error_message)) {
            auto error_info = doc.last_error();
            annotate_template_table_json_batch_error(operation_index, error_info);
            report_operation_failure(
                command, "mutate",
                prefix_template_table_json_batch_message(operation_index,
                                                         error_message),
                error_info, options.json_output);
            return 1;
        }

        std::size_t resolved_table_index = 0U;
        if (!resolve_template_table_index_for_batch(
                doc, selected, selector, resolved_table_index, operation_index,
                command, options.json_output, "mutate")) {
            return 1;
        }

        featherdoc::Table table;
        if (!resolve_template_table_for_batch(
                selected, resolved_table_index, table, operation_index, command,
                options.json_output, "mutate")) {
            return 1;
        }

        if (!apply_template_table_json_patch(
                command, table, selected.part.entry_name(), resolved_table_index,
                operation.patch, options.json_output, operation_index)) {
            return 1;
        }

        applied_template_table_json_batch_operation applied{};
        applied.operation_index = operation_index;
        applied.selected = selected;
        applied.table_index = resolved_table_index;
        applied.selector = selector;
        applied.patch = operation.patch;
        applied_operations.push_back(std::move(applied));
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&applied_operations](std::ostream &stream) {
                write_json_template_tables_from_json_result(stream,
                                                            applied_operations);
            });
    } else {
        print_template_tables_from_json_result(applied_operations,
                                               options.output_path);
    }

    return 0;
}

} // namespace featherdoc_cli
