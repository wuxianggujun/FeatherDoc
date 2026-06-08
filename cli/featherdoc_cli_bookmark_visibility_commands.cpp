#include "featherdoc_cli_bookmark_visibility_commands.hpp"

#include "featherdoc_cli_bookmark_support.hpp"
#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

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

} // namespace

auto is_bookmark_visibility_command(std::string_view command) -> bool {
    return command == "set-bookmark-block-visibility" ||
           command == "apply-bookmark-block-visibility";
}

auto run_bookmark_visibility_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "set-bookmark-block-visibility") {
        return run_set_bookmark_block_visibility_command(command, arguments, doc);
    }
    if (command == "apply-bookmark-block-visibility") {
        return run_apply_bookmark_block_visibility_command(command, arguments,
                                                            doc);
    }

    print_parse_error(command, "unsupported bookmark visibility command",
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
