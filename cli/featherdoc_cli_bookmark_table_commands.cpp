#include "featherdoc_cli_bookmark_table_commands.hpp"

#include "featherdoc_cli_bookmark_support.hpp"
#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto resolve_bookmark_table_row_sources(
    const bookmark_table_replacement_options &options,
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

void write_json_bookmark_table_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::vector<std::string>> &rows, std::size_t replaced) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"bookmark\":";
    write_json_bookmark_support_summary(stream, bookmark);
    stream << ",\"replaced\":" << replaced << ",\"row_count\":" << rows.size()
           << ",\"rows\":[";
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

void print_bookmark_table_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::vector<std::string>> &rows,
    const std::optional<path_type> &output_path, std::size_t replaced) {
    print_bookmark_identity(selected, bookmark);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
    std::cout << "row_count: " << rows.size() << '\n';
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        std::cout << "row[" << row_index << "]: ";
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            if (cell_index != 0U) {
                std::cout << '\t';
            }
            std::cout << rows[row_index][cell_index];
        }
        std::cout << '\n';
    }
}

} // namespace

auto run_replace_bookmark_table_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          std::string(command) +
                              " expects an input path and bookmark name",
                          json_output);
        return 2;
    }

    const auto bookmark_name = std::string(arguments[2]);
    if (bookmark_name.empty()) {
        print_parse_error(command, "bookmark name must not be empty",
                          json_output);
        return 2;
    }

    bookmark_table_replacement_options options;
    std::string error_message;
    const bool allow_empty_rows = command == "replace-bookmark-table-rows";
    if (!parse_bookmark_table_replacement_options(arguments, 3U, options,
                                                  allow_empty_rows,
                                                  error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::vector<std::vector<std::string>> rows;
    if (!resolve_bookmark_table_row_sources(options, rows, error_message)) {
        return report_bookmark_input_error(command, options.json_output,
                                           error_message);
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_bookmark_part(command, doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              options.json_output, selected, error_message)) {
        return 1;
    }

    const auto bookmark = selected.part.find_bookmark(bookmark_name);
    if (!bookmark.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }
    const auto bookmark_summary = *bookmark;

    const auto replaced =
        command == "replace-bookmark-table"
            ? selected.part.replace_bookmark_with_table(bookmark_name, rows)
            : selected.part.replace_bookmark_with_table_rows(bookmark_name, rows);
    if (replaced == 0U) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &bookmark_summary, &rows,
             replaced](std::ostream &stream) {
                write_json_bookmark_table_result(
                    stream, selected, bookmark_summary, rows, replaced);
            });
    } else {
        print_bookmark_table_result(selected, bookmark_summary, rows,
                                    options.output_path, replaced);
    }

    return 0;
}

} // namespace featherdoc_cli
