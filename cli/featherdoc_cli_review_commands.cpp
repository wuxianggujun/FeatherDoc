#include "featherdoc_cli_review_commands.hpp"

#include "featherdoc_cli_review_comment_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_document_mutation_options_parse.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_review_mutation_plan_commands.hpp"
#include "featherdoc_cli_review_mutation_plan.hpp"
#include "featherdoc_cli_review_mutation_plan_parse.hpp"
#include "featherdoc_cli_review_output.hpp"
#include "featherdoc_cli_review_revision_commands.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto is_review_command(std::string_view command) -> bool {
    return command == "inspect-review" ||
           command == "find-text-ranges" ||
           is_review_mutation_plan_command(command) ||
           is_review_revision_command(command) ||
           command == "append-footnote" ||
           command == "replace-footnote" ||
           command == "remove-footnote" ||
           command == "append-endnote" ||
           command == "replace-endnote" ||
           command == "remove-endnote" ||
           is_review_comment_command(command);
}

auto run_review_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "inspect-review") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-review expects an input path",
                              json_output);
            return 2;
        }
        if (arguments.size() > 3U ||
            (arguments.size() == 3U && arguments[2] != "--json")) {
            print_parse_error(command, "unknown option: " +
                                           std::string(arguments[2]),
                              json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto footnotes = doc.list_footnotes();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, json_output);
            return 1;
        }
        const auto endnotes = doc.list_endnotes();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, json_output);
            return 1;
        }
        const auto comments = doc.list_comments();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, json_output);
            return 1;
        }
        const auto revisions = doc.list_revisions();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, json_output);
            return 1;
        }

        inspect_review(footnotes, endnotes, comments, revisions, json_output);
        return 0;
    }

    if (command == "find-text-ranges") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "find-text-ranges expects an input path",
                              json_output);
            return 2;
        }

        std::optional<std::string> query_text;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--text") {
                if (query_text.has_value()) {
                    print_parse_error(command, "duplicate --text option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing value after --text",
                                      json_output);
                    return 2;
                }
                query_text = std::string(arguments[index + 1U]);
                ++index;
                continue;
            }
            if (argument == "--json") {
                continue;
            }

            print_parse_error(command, "unknown option: " + std::string(argument),
                              json_output);
            return 2;
        }

        if (!query_text.has_value()) {
            print_parse_error(command, "missing --text <text>", json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto matches = doc.find_text_ranges(*query_text);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "find", error_info, json_output);
            return 1;
        }

        if (json_output) {
            write_json_text_range_matches(std::cout, command, *query_text,
                                          matches);
        } else {
            print_text_range_matches(std::cout, *query_text, matches);
        }
        return 0;
    }

    if (is_review_mutation_plan_command(command)) {
        return run_review_mutation_plan_command(command, arguments, doc);
    }

    if (is_review_revision_command(command)) {
        return run_review_revision_command(command, arguments, doc);
    }

    if (command == "append-footnote" || command == "replace-footnote" ||
        command == "remove-footnote" || command == "append-endnote" ||
        command == "replace-endnote" || command == "remove-endnote") {
        const auto json_output = has_json_flag(arguments);
        const bool footnote = command.find("footnote") != std::string::npos;
        const bool append = command.find("append") == 0U;
        const bool replace = command.find("replace") == 0U;
        if (arguments.size() < (append ? 2U : 3U)) {
            print_parse_error(command,
                              append ? std::string(command) +
                                           " expects an input path"
                                     : std::string(command) +
                                           " expects an input path and note index",
                              json_output);
            return 2;
        }

        std::size_t note_index = 0U;
        if (!append && !parse_index(arguments[2], note_index)) {
            print_parse_error(command,
                              "invalid note index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        review_note_mutation_options options;
        std::string error_message;
        if (!parse_review_note_mutation_options(
                arguments, append ? 2U : 3U, options, append, replace || append,
                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::size_t affected = 0U;
        if (append) {
            affected = footnote ? doc.append_footnote(options.reference_text,
                                                      options.note_text)
                                : doc.append_endnote(options.reference_text,
                                                     options.note_text);
        } else if (replace) {
            affected = footnote ? (doc.replace_footnote(note_index,
                                                        options.note_text) ? 1U : 0U)
                                : (doc.replace_endnote(note_index,
                                                       options.note_text) ? 1U : 0U);
        } else {
            affected = footnote ? (doc.remove_footnote(note_index) ? 1U : 0U)
                                : (doc.remove_endnote(note_index) ? 1U : 0U);
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
                [affected](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (is_review_comment_command(command)) {
        return run_review_comment_command(command, arguments, doc);
    }


    print_parse_error(command, "unknown review command", has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
