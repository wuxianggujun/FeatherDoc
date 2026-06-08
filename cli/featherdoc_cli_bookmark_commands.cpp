#include "featherdoc_cli_bookmark_commands.hpp"

#include "featherdoc_cli_bookmark_support.hpp"
#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_image_output.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

void inspect_bookmarks(const selected_template_part &selected,
                       const std::vector<featherdoc::bookmark_summary> &bookmarks,
                       bool json_output) {
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(
                std::cout,
                featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, std::string(selected.part.entry_name()));
        std::cout << ",\"count\":" << bookmarks.size() << ",\"bookmarks\":[";
        for (std::size_t index = 0; index < bookmarks.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_bookmark_support_summary(std::cout, bookmarks[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_bookmark_part(selected);
    std::cout << "bookmarks: " << bookmarks.size() << '\n';
    for (std::size_t index = 0; index < bookmarks.size(); ++index) {
        std::cout << "bookmark[" << index << "]: ";
        print_bookmark_support_summary(std::cout, bookmarks[index]);
        std::cout << '\n';
    }
}

void inspect_bookmark(const selected_template_part &selected,
                      const featherdoc::bookmark_summary &bookmark,
                      bool json_output) {
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(
                std::cout,
                featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, std::string(selected.part.entry_name()));
        std::cout << ",\"bookmark\":";
        write_json_bookmark_support_summary(std::cout, bookmark);
        std::cout << "}\n";
        return;
    }

    print_selected_bookmark_part(selected);
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "occurrence_count: " << bookmark.occurrence_count << '\n';
    std::cout << "kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "duplicate: " << yes_no(bookmark.is_duplicate()) << '\n';
}

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

auto is_same_drawing_image_identity(const featherdoc::drawing_image_info &lhs,
                                    const featherdoc::drawing_image_info &rhs)
    -> bool {
    return lhs.relationship_id == rhs.relationship_id &&
           lhs.entry_name == rhs.entry_name;
}

auto collect_new_drawing_images(
    const std::vector<featherdoc::drawing_image_info> &before_images,
    const std::vector<featherdoc::drawing_image_info> &after_images)
    -> std::vector<featherdoc::drawing_image_info> {
    std::vector<bool> matched_before(before_images.size(), false);
    std::vector<featherdoc::drawing_image_info> inserted_images;
    inserted_images.reserve(after_images.size());

    for (const auto &image : after_images) {
        auto matched_it =
            std::find_if(before_images.begin(), before_images.end(),
                         [&before_images, &matched_before,
                          &image](const featherdoc::drawing_image_info &candidate) {
                             const auto candidate_index = static_cast<std::size_t>(
                                 &candidate - before_images.data());
                             return !matched_before[candidate_index] &&
                                    is_same_drawing_image_identity(candidate, image);
                         });
        if (matched_it == before_images.end()) {
            inserted_images.push_back(image);
            continue;
        }

        matched_before[static_cast<std::size_t>(matched_it - before_images.begin())] =
            true;
    }

    return inserted_images;
}

void write_json_bookmark_image_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, const path_type &image_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"bookmark\":";
    write_json_bookmark_support_summary(stream, bookmark);
    stream << ",\"image_path\":";
    write_json_string(stream, image_path.string());
    stream << ",\"replaced\":" << inserted_images.size() << ",\"images\":[";
    for (std::size_t index = 0; index < inserted_images.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_drawing_image_summary(stream, inserted_images[index]);
    }
    stream << ']';
}

void print_bookmark_image_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, const path_type &image_path,
    const std::optional<path_type> &output_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images) {
    print_bookmark_identity(selected, bookmark);
    std::cout << "image_path: " << image_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << inserted_images.size() << '\n';
    for (std::size_t index = 0; index < inserted_images.size(); ++index) {
        std::cout << "image[" << index << "]: ";
        print_drawing_image_summary(std::cout, inserted_images[index]);
        std::cout << '\n';
    }
}

void write_json_bookmark_paragraphs_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::string> &paragraphs, std::size_t replaced) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"bookmark\":";
    write_json_bookmark_support_summary(stream, bookmark);
    stream << ",\"replaced\":" << replaced
           << ",\"paragraph_count\":" << paragraphs.size()
           << ",\"paragraphs\":[";
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_string(stream, paragraphs[index]);
    }
    stream << ']';
}

void print_bookmark_paragraphs_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::string> &paragraphs,
    const std::optional<path_type> &output_path, std::size_t replaced) {
    print_bookmark_identity(selected, bookmark);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
    std::cout << "paragraph_count: " << paragraphs.size() << '\n';
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        std::cout << "paragraph[" << index << "]: " << paragraphs[index] << '\n';
    }
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

void write_json_bookmark_block_removal_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, std::size_t removed) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"bookmark\":";
    write_json_bookmark_support_summary(stream, bookmark);
    stream << ",\"removed\":" << removed;
}

void print_bookmark_block_removal_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::optional<path_type> &output_path, std::size_t removed) {
    print_bookmark_identity(selected, bookmark);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "removed: " << removed << '\n';
}

void write_json_bookmark_text_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, std::string_view text,
    std::size_t replaced) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"bookmark\":";
    write_json_bookmark_support_summary(stream, bookmark);
    stream << ",\"replaced\":" << replaced << ",\"text\":";
    write_json_string(stream, text);
}

void print_bookmark_text_result(const selected_template_part &selected,
                                const featherdoc::bookmark_summary &bookmark,
                                std::string_view text,
                                const std::optional<path_type> &output_path,
                                std::size_t replaced) {
    print_bookmark_identity(selected, bookmark);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
    std::cout << "text: " << text << '\n';
}

void write_json_bookmark_text_bindings(
    std::ostream &stream,
    const std::vector<featherdoc::bookmark_text_binding> &bindings) {
    stream << '[';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"bookmark_name\":";
        write_json_string(stream, bindings[index].bookmark_name);
        stream << ",\"text\":";
        write_json_string(stream, bindings[index].text);
        stream << '}';
    }
    stream << ']';
}

void write_json_bookmark_fill_result(
    std::ostream &stream, const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_text_binding> &bindings,
    const featherdoc::bookmark_fill_result &result) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"complete\":" << json_bool(static_cast<bool>(result))
           << ",\"requested\":" << result.requested
           << ",\"matched\":" << result.matched
           << ",\"replaced\":" << result.replaced
           << ",\"bindings\":";
    write_json_bookmark_text_bindings(stream, bindings);
    stream << ",\"missing_bookmarks\":";
    write_json_strings(stream, result.missing_bookmarks);
}

void print_bookmark_fill_result(
    const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_text_binding> &bindings,
    const std::optional<path_type> &output_path,
    const featherdoc::bookmark_fill_result &result) {
    print_selected_bookmark_part(selected);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "complete: " << yes_no(static_cast<bool>(result)) << '\n';
    std::cout << "requested: " << result.requested << '\n';
    std::cout << "matched: " << result.matched << '\n';
    std::cout << "replaced: " << result.replaced << '\n';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        std::cout << "binding[" << index << "]: "
                  << bindings[index].bookmark_name << " => "
                  << bindings[index].text << '\n';
    }
    std::cout << "missing_bookmarks: ";
    if (result.missing_bookmarks.empty()) {
        std::cout << "none\n";
    } else {
        for (std::size_t index = 0; index < result.missing_bookmarks.size();
             ++index) {
            if (index != 0U) {
                std::cout << ", ";
            }
            std::cout << result.missing_bookmarks[index];
        }
        std::cout << '\n';
    }
}

void write_json_bookmark_block_visibility_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, bool visible,
    std::size_t changed) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"bookmark\":";
    write_json_bookmark_support_summary(stream, bookmark);
    stream << ",\"visible\":" << json_bool(visible)
           << ",\"changed\":" << changed;
}

void print_bookmark_block_visibility_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::optional<path_type> &output_path, bool visible,
    std::size_t changed) {
    print_bookmark_identity(selected, bookmark);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "visible: " << yes_no(visible) << '\n';
    std::cout << "changed: " << changed << '\n';
}

void write_json_bookmark_block_visibility_bindings(
    std::ostream &stream,
    const std::vector<featherdoc::bookmark_block_visibility_binding> &bindings) {
    stream << '[';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"bookmark_name\":";
        write_json_string(stream, bindings[index].bookmark_name);
        stream << ",\"visible\":" << json_bool(bindings[index].visible) << '}';
    }
    stream << ']';
}

void write_json_applied_bookmark_block_visibility_result(
    std::ostream &stream, const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_block_visibility_binding> &bindings,
    const featherdoc::bookmark_block_visibility_result &result) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"complete\":" << json_bool(static_cast<bool>(result))
           << ",\"requested\":" << result.requested
           << ",\"matched\":" << result.matched
           << ",\"kept\":" << result.kept
           << ",\"removed\":" << result.removed
           << ",\"bindings\":";
    write_json_bookmark_block_visibility_bindings(stream, bindings);
    stream << ",\"missing_bookmarks\":";
    write_json_strings(stream, result.missing_bookmarks);
}

void print_applied_bookmark_block_visibility_result(
    const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_block_visibility_binding> &bindings,
    const std::optional<path_type> &output_path,
    const featherdoc::bookmark_block_visibility_result &result) {
    print_selected_bookmark_part(selected);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "complete: " << yes_no(static_cast<bool>(result)) << '\n';
    std::cout << "requested: " << result.requested << '\n';
    std::cout << "matched: " << result.matched << '\n';
    std::cout << "kept: " << result.kept << '\n';
    std::cout << "removed: " << result.removed << '\n';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        std::cout << "binding[" << index << "]: "
                  << bindings[index].bookmark_name << " => "
                  << (bindings[index].visible ? "visible" : "hidden") << '\n';
    }
    std::cout << "missing_bookmarks: ";
    if (result.missing_bookmarks.empty()) {
        std::cout << "none\n";
    } else {
        for (std::size_t index = 0; index < result.missing_bookmarks.size();
             ++index) {
            if (index != 0U) {
                std::cout << ", ";
            }
            std::cout << result.missing_bookmarks[index];
        }
        std::cout << '\n';
    }
}

auto run_inspect_bookmarks_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-bookmarks expects an input path",
                          json_output);
        return 2;
    }

    inspect_bookmarks_options options;
    std::string error_message;
    if (!parse_inspect_bookmarks_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "inspect", error_message,
                                 doc.last_error(), options.json_output);
        return 1;
    }

    if (options.bookmark_name.has_value()) {
        const auto bookmark = selected.part.find_bookmark(*options.bookmark_name);
        if (!bookmark.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        inspect_bookmark(selected, *bookmark, options.json_output);
        return 0;
    }

    const auto bookmarks = selected.part.list_bookmarks();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    inspect_bookmarks(selected, bookmarks, options.json_output);
    return 0;
}

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

auto run_set_bookmark_block_visibility_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "set-bookmark-block-visibility expects an input path and bookmark name",
            json_output);
        return 2;
    }

    const auto bookmark_name = std::string(arguments[2]);
    if (bookmark_name.empty()) {
        print_parse_error(command, "bookmark name must not be empty",
                          json_output);
        return 2;
    }

    set_bookmark_block_visibility_options options;
    std::string error_message;
    if (!parse_set_bookmark_block_visibility_options(arguments, 3U, options,
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

    const auto changed =
        selected.part.set_bookmark_block_visibility(bookmark_name, options.visible);
    if (changed == 0U) {
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
             changed](std::ostream &stream) {
                write_json_bookmark_block_visibility_result(
                    stream, selected, bookmark_summary, options.visible, changed);
            });
    } else {
        print_bookmark_block_visibility_result(
            selected, bookmark_summary, options.output_path, options.visible,
            changed);
    }

    return 0;
}

auto run_apply_bookmark_block_visibility_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            "apply-bookmark-block-visibility expects an input path and at least one visibility binding",
            json_output);
        return 2;
    }

    apply_bookmark_block_visibility_options options;
    std::string error_message;
    if (!parse_apply_bookmark_block_visibility_options(arguments, 2U, options,
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

    const auto result =
        selected.part.apply_bookmark_block_visibility(options.bindings);
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
            [&selected, &options, &result](std::ostream &stream) {
                write_json_applied_bookmark_block_visibility_result(
                    stream, selected, options.bindings, result);
            });
    } else {
        print_applied_bookmark_block_visibility_result(
            selected, options.bindings, options.output_path, result);
    }

    return 0;
}

auto run_replace_bookmark_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            std::string(command) +
                " expects an input path, bookmark name, and image path",
            json_output);
        return 2;
    }

    const auto bookmark_name = std::string(arguments[2]);
    if (bookmark_name.empty()) {
        print_parse_error(command, "bookmark name must not be empty",
                          json_output);
        return 2;
    }

    append_image_options options;
    std::string error_message;
    const bool floating = command == "replace-bookmark-floating-image";
    if (!parse_bookmark_image_command_options(arguments, 4U, command, floating,
                                              options, error_message)) {
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

    const auto existing_images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    const auto image_path = path_type(std::string(arguments[3]));
    std::size_t replaced = 0U;
    if (floating) {
        if (options.width_px.has_value()) {
            replaced = selected.part.replace_bookmark_with_floating_image(
                bookmark_name, image_path, *options.width_px, *options.height_px,
                options.floating_options);
        } else {
            replaced = selected.part.replace_bookmark_with_floating_image(
                bookmark_name, image_path, options.floating_options);
        }
    } else if (options.width_px.has_value()) {
        replaced = selected.part.replace_bookmark_with_image(
            bookmark_name, image_path, *options.width_px, *options.height_px);
    } else {
        replaced =
            selected.part.replace_bookmark_with_image(bookmark_name, image_path);
    }

    if (replaced == 0U) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto updated_images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, options.json_output);
        return 1;
    }

    const auto inserted_images =
        collect_new_drawing_images(existing_images, updated_images);
    if (inserted_images.size() != replaced) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::result_out_of_range);
        error_info.detail = "expected " + std::to_string(replaced) +
                            " replaced drawing image(s) in " +
                            std::string(selected.part.entry_name()) +
                            ", found " +
                            std::to_string(inserted_images.size());
        error_info.entry_name = std::string(selected.part.entry_name());
        report_operation_failure(command, "mutate",
                                 "replaced drawing images not found", error_info,
                                 options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &bookmark_summary, &image_path,
             &inserted_images](std::ostream &stream) {
                write_json_bookmark_image_result(
                    stream, selected, bookmark_summary, image_path,
                    inserted_images);
            });
    } else {
        print_bookmark_image_result(selected, bookmark_summary, image_path,
                                    options.output_path, inserted_images);
    }

    return 0;
}

} // namespace

auto is_bookmark_command(std::string_view command) -> bool {
    return command == "inspect-bookmarks" ||
           command == "replace-bookmark-paragraphs" ||
           command == "replace-bookmark-table" ||
           command == "replace-bookmark-table-rows" ||
           command == "remove-bookmark-block" ||
           command == "replace-bookmark-text" ||
           command == "fill-bookmarks" ||
           command == "set-bookmark-block-visibility" ||
           command == "apply-bookmark-block-visibility" ||
           command == "replace-bookmark-image" ||
           command == "replace-bookmark-floating-image";
}

auto run_bookmark_command(std::string_view command,
                          const std::vector<std::string_view> &arguments,
                          featherdoc::Document &doc) -> int {
    if (command == "inspect-bookmarks") {
        return run_inspect_bookmarks_command(command, arguments, doc);
    }
    if (command == "replace-bookmark-paragraphs") {
        return run_replace_bookmark_paragraphs_command(command, arguments, doc);
    }
    if (command == "replace-bookmark-table" ||
        command == "replace-bookmark-table-rows") {
        return run_replace_bookmark_table_command(command, arguments, doc);
    }
    if (command == "remove-bookmark-block") {
        return run_remove_bookmark_block_command(command, arguments, doc);
    }
    if (command == "replace-bookmark-text") {
        return run_replace_bookmark_text_command(command, arguments, doc);
    }
    if (command == "fill-bookmarks") {
        return run_fill_bookmarks_command(command, arguments, doc);
    }
    if (command == "set-bookmark-block-visibility") {
        return run_set_bookmark_block_visibility_command(command, arguments, doc);
    }
    if (command == "apply-bookmark-block-visibility") {
        return run_apply_bookmark_block_visibility_command(command, arguments,
                                                           doc);
    }
    if (command == "replace-bookmark-image" ||
        command == "replace-bookmark-floating-image") {
        return run_replace_bookmark_image_command(command, arguments, doc);
    }

    print_parse_error(command, "unsupported bookmark command", has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
