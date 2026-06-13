#include "featherdoc_cli_content_control_table_commands.hpp"

#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_content_control_output.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_content_control_table_result(
    std::ostream &stream, const selected_template_part &selected,
    const content_control_table_replacement_options &options,
    const std::vector<std::vector<std::string>> &rows, std::size_t replaced) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << ",\"replaced\":" << replaced << ",\"row_count\":"
           << rows.size() << ",\"rows\":[";
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        if (row_index != 0U) {
            stream << ',';
        }
        stream << '[';
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            if (cell_index != 0U) {
                stream << ',';
            }
            write_json_string(stream, rows[row_index][cell_index]);
        }
        stream << ']';
    }
    stream << ']';
}

void print_content_control_table_result(
    const selected_template_part &selected,
    const content_control_table_replacement_options &options,
    const std::vector<std::vector<std::string>> &rows, std::size_t replaced) {
    print_content_control_common_result(selected, options.tag, options.alias,
                                        options.output_path, replaced);
    std::cout << "row_count: " << rows.size() << '\n';
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        std::cout << "row[" << row_index << "]:";
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            std::cout << (cell_index == 0U ? " " : " | ")
                      << rows[row_index][cell_index];
        }
        std::cout << '\n';
    }
}

auto resolve_content_control_table_row_sources(
    const content_control_table_replacement_options &options,
    std::vector<std::vector<std::string>> &rows, std::string &error_message)
    -> bool {
    rows.clear();
    rows.reserve(options.row_sources.size());

    for (const auto &row_sources : options.row_sources) {
        std::vector<std::string> row;
        row.reserve(row_sources.size());

        for (const auto &source : row_sources) {
            std::string text;
            if (!read_text_source(source, text, error_message)) {
                return false;
            }
            row.push_back(std::move(text));
        }

        rows.push_back(std::move(row));
    }

    return true;
}

} // namespace

auto run_replace_content_control_table_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, std::string(command) + " expects an input path",
                          json_output);
        return 2;
    }

    content_control_table_replacement_options options;
    std::string error_message;
    const bool allow_empty_rows =
        command == "replace-content-control-table-rows";
    if (!parse_content_control_table_replacement_options(
            arguments, 2U, options, allow_empty_rows, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::vector<std::vector<std::string>> rows;
    if (!resolve_content_control_table_row_sources(options, rows,
                                                   error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "input",
                                     error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "mutate", error_message,
                                 doc.last_error(), options.json_output);
        return 1;
    }

    const auto replaced =
        command == "replace-content-control-table"
            ? (options.tag.has_value()
                   ? selected.part.replace_content_control_with_table_by_tag(
                         *options.tag, rows)
                   : selected.part.replace_content_control_with_table_by_alias(
                         *options.alias, rows))
            : (options.tag.has_value()
                   ? selected.part.replace_content_control_with_table_rows_by_tag(
                         *options.tag, rows)
                   : selected.part
                         .replace_content_control_with_table_rows_by_alias(
                             *options.alias, rows));
    if (replaced == 0U) {
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
        } else {
            report_operation_failure(command, "mutate",
                                     "matching content control not found",
                                     doc.last_error(), options.json_output);
        }
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &options, &rows, replaced](std::ostream &stream) {
                write_json_content_control_table_result(
                    stream, selected, options, rows, replaced);
            });
    } else {
        print_content_control_table_result(selected, options, rows, replaced);
    }

    return 0;
}

} // namespace featherdoc_cli
