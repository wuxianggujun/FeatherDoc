#include "featherdoc_cli_review_comment_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_document_mutation_options_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_review_output.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_append_text_comment_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    const bool paragraph_range = command == "append-paragraph-text-comment";
    if (arguments.size() < (paragraph_range ? 5U : 6U)) {
        print_parse_error(
            command,
            paragraph_range
                ? std::string(command) +
                      " expects an input path, paragraph index, text offset, and text length"
                : std::string(command) +
                      " expects an input path, start paragraph index, start offset, end paragraph index, and end offset",
            json_output);
        return 2;
    }

    std::size_t start_paragraph_index = 0U;
    if (!parse_index(arguments[2], start_paragraph_index)) {
        print_parse_error(command,
                          "invalid start paragraph index: " +
                              std::string(arguments[2]),
                          json_output);
        return 2;
    }

    std::size_t start_text_offset = 0U;
    if (!parse_index(arguments[3], start_text_offset)) {
        print_parse_error(command,
                          "invalid start text offset: " +
                              std::string(arguments[3]),
                          json_output);
        return 2;
    }

    std::size_t text_length = 0U;
    std::size_t end_paragraph_index = start_paragraph_index;
    std::size_t end_text_offset = start_text_offset;
    if (paragraph_range) {
        if (!parse_index(arguments[4], text_length)) {
            print_parse_error(command,
                              "invalid text length: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }
    } else {
        if (!parse_index(arguments[4], end_paragraph_index)) {
            print_parse_error(command,
                              "invalid end paragraph index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }
        if (!parse_index(arguments[5], end_text_offset)) {
            print_parse_error(command,
                              "invalid end text offset: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }
    }

    comment_mutation_options options;
    std::string error_message;
    if (!parse_comment_mutation_options(arguments, paragraph_range ? 5U : 6U,
                                        options, false, true,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    std::size_t affected = 0U;
    if (paragraph_range) {
        affected = doc.append_paragraph_text_comment(
            start_paragraph_index, start_text_offset, text_length,
            options.comment_text, options.author, options.initials,
            options.date);
    } else {
        affected = doc.append_text_range_comment(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset, options.comment_text, options.author,
            options.initials, options.date);
    }
    if (affected == 0U) {
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
            [affected, paragraph_range, start_paragraph_index,
             start_text_offset, text_length, end_paragraph_index,
             end_text_offset](std::ostream &stream) {
                write_json_affected_result(stream, affected);
                stream << ",\"start_paragraph_index\":"
                       << start_paragraph_index
                       << ",\"start_text_offset\":" << start_text_offset;
                if (paragraph_range) {
                    stream << ",\"text_length\":" << text_length;
                } else {
                    stream << ",\"end_paragraph_index\":"
                           << end_paragraph_index
                           << ",\"end_text_offset\":" << end_text_offset;
                }
            });
    } else {
        print_simple_document_mutation_result(command, options.output_path,
                                              affected);
    }
    return 0;
}

auto run_set_text_comment_range_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    const bool paragraph_range = command == "set-paragraph-text-comment-range";
    if (arguments.size() < (paragraph_range ? 6U : 7U)) {
        print_parse_error(
            command,
            paragraph_range
                ? std::string(command) +
                      " expects an input path, comment index, paragraph index, text offset, and text length"
                : std::string(command) +
                      " expects an input path, comment index, start paragraph index, start offset, end paragraph index, and end offset",
            json_output);
        return 2;
    }

    std::size_t comment_index = 0U;
    if (!parse_index(arguments[2], comment_index)) {
        print_parse_error(command,
                          "invalid comment index: " +
                              std::string(arguments[2]),
                          json_output);
        return 2;
    }

    std::size_t start_paragraph_index = 0U;
    if (!parse_index(arguments[3], start_paragraph_index)) {
        print_parse_error(command,
                          "invalid start paragraph index: " +
                              std::string(arguments[3]),
                          json_output);
        return 2;
    }

    std::size_t start_text_offset = 0U;
    if (!parse_index(arguments[4], start_text_offset)) {
        print_parse_error(command,
                          "invalid start text offset: " +
                              std::string(arguments[4]),
                          json_output);
        return 2;
    }

    std::size_t text_length = 0U;
    std::size_t end_paragraph_index = start_paragraph_index;
    std::size_t end_text_offset = start_text_offset;
    if (paragraph_range) {
        if (!parse_index(arguments[5], text_length)) {
            print_parse_error(command,
                              "invalid text length: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }
    } else {
        if (!parse_index(arguments[5], end_paragraph_index)) {
            print_parse_error(command,
                              "invalid end paragraph index: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }
        if (!parse_index(arguments[6], end_text_offset)) {
            print_parse_error(command,
                              "invalid end text offset: " +
                                  std::string(arguments[6]),
                              json_output);
            return 2;
        }
    }

    simple_document_mutation_options options;
    std::string error_message;
    if (!parse_simple_document_mutation_options(
            arguments, paragraph_range ? 6U : 7U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    const bool ok =
        paragraph_range
            ? doc.set_paragraph_text_comment_range(
                  comment_index, start_paragraph_index, start_text_offset,
                  text_length)
            : doc.set_text_range_comment_range(
                  comment_index, start_paragraph_index, start_text_offset,
                  end_paragraph_index, end_text_offset);
    if (!ok) {
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
            [paragraph_range, comment_index, start_paragraph_index,
             start_text_offset, text_length, end_paragraph_index,
             end_text_offset](std::ostream &stream) {
                write_json_affected_result(stream, 1U);
                stream << ",\"comment_index\":" << comment_index
                       << ",\"start_paragraph_index\":"
                       << start_paragraph_index
                       << ",\"start_text_offset\":" << start_text_offset;
                if (paragraph_range) {
                    stream << ",\"text_length\":" << text_length;
                } else {
                    stream << ",\"end_paragraph_index\":"
                           << end_paragraph_index
                           << ",\"end_text_offset\":" << end_text_offset;
                }
            });
    } else {
        print_simple_document_mutation_result(command, options.output_path, 1U);
    }
    return 0;
}

} // namespace featherdoc_cli
