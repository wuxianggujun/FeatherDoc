#include "featherdoc_cli_bookmark_commands_detail.hpp"

#include "featherdoc_cli_bookmark_output.hpp"
#include "featherdoc_cli_bookmark_support.hpp"
#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto resolve_text_sources(const std::vector<cli_text_source_options> &sources,
                          std::vector<std::string> &texts,
                          std::string &error_message) -> bool {
    texts.clear();
    texts.reserve(sources.size());

    for (const auto &source : sources) {
        std::string text;
        if (!read_text_source(source, text, error_message)) {
            return false;
        }
        texts.push_back(std::move(text));
    }

    return true;
}

auto resolve_fill_bookmark_bindings(
    const fill_bookmarks_options &options,
    std::vector<featherdoc::bookmark_text_binding> &bindings,
    std::string &error_message) -> bool {
    bindings.clear();
    bindings.reserve(options.binding_sources.size());

    for (const auto &binding_source : options.binding_sources) {
        std::string text;
        if (!read_text_source(binding_source.source, text, error_message)) {
            return false;
        }

        bindings.push_back({binding_source.bookmark_name, std::move(text)});
    }

    return true;
}

} // namespace

auto run_replace_bookmark_paragraphs_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "replace-bookmark-paragraphs expects an input path and bookmark name",
            json_output);
        return 2;
    }

    const auto bookmark_name = std::string(arguments[2]);
    if (bookmark_name.empty()) {
        print_parse_error(command, "bookmark name must not be empty",
                          json_output);
        return 2;
    }

    replace_bookmark_paragraphs_options options;
    std::string error_message;
    if (!parse_replace_bookmark_paragraphs_options(arguments, 3U, options,
                                                   error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::vector<std::string> paragraphs;
    if (!resolve_text_sources(options.paragraph_sources, paragraphs,
                              error_message)) {
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
        selected.part.replace_bookmark_with_paragraphs(bookmark_name, paragraphs);
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
            [&selected, &bookmark_summary, replaced,
             &paragraphs](std::ostream &stream) {
                write_json_bookmark_paragraphs_result(
                    stream, selected, bookmark_summary, paragraphs, replaced);
            });
    } else {
        print_bookmark_paragraphs_result(selected, bookmark_summary, paragraphs,
                                         options.output_path, replaced);
    }

    return 0;
}

auto run_remove_bookmark_block_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command, "remove-bookmark-block expects an input path and bookmark name",
            json_output);
        return 2;
    }

    const auto bookmark_name = std::string(arguments[2]);
    if (bookmark_name.empty()) {
        print_parse_error(command, "bookmark name must not be empty",
                          json_output);
        return 2;
    }

    remove_bookmark_block_options options;
    std::string error_message;
    if (!parse_remove_bookmark_block_options(arguments, 3U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
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

    const auto removed = selected.part.remove_bookmark_block(bookmark_name);
    if (removed == 0U) {
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
            [&selected, &bookmark_summary, removed](std::ostream &stream) {
                write_json_bookmark_block_removal_result(
                    stream, selected, bookmark_summary, removed);
            });
    } else {
        print_bookmark_block_removal_result(selected, bookmark_summary,
                                            options.output_path, removed);
    }

    return 0;
}

auto run_replace_bookmark_text_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command, "replace-bookmark-text expects an input path and bookmark name",
            json_output);
        return 2;
    }

    const auto bookmark_name = std::string(arguments[2]);
    if (bookmark_name.empty()) {
        print_parse_error(command, "bookmark name must not be empty",
                          json_output);
        return 2;
    }

    replace_bookmark_text_options options;
    std::string error_message;
    if (!parse_replace_bookmark_text_options(arguments, 3U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
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
        selected.part.replace_bookmark_text(bookmark_name, options.text);
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
            [&selected, &bookmark_summary, &options,
             replaced](std::ostream &stream) {
                write_json_bookmark_text_result(
                    stream, selected, bookmark_summary, options.text, replaced);
            });
    } else {
        print_bookmark_text_result(selected, bookmark_summary, options.text,
                                   options.output_path, replaced);
    }

    return 0;
}

auto run_fill_bookmarks_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "fill-bookmarks expects an input path",
                          json_output);
        return 2;
    }

    fill_bookmarks_options options;
    std::string error_message;
    if (!parse_fill_bookmarks_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::vector<featherdoc::bookmark_text_binding> bindings;
    if (!resolve_fill_bookmark_bindings(options, bindings, error_message)) {
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

    const auto result = selected.part.fill_bookmarks(bindings);
    if (doc.last_error().code) {
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
            [&selected, &bindings, &result](std::ostream &stream) {
                write_json_bookmark_fill_result(stream, selected, bindings,
                                                result);
            });
    } else {
        print_bookmark_fill_result(selected, bindings, options.output_path,
                                   result);
    }

    return 0;
}

} // namespace featherdoc_cli
