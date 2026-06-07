#include "featherdoc_cli_review_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_document_mutation_options_parse.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_review_mutation_plan.hpp"
#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_review_note_summary(
    std::ostream &stream, const featherdoc::review_note_summary &note) {
    stream << "{\"index\":" << note.index << ",\"kind\":";
    write_json_string(stream, review_note_kind_name(note.kind));
    stream << ",\"id\":";
    write_json_string(stream, note.id);
    stream << ",\"author\":";
    write_json_optional_string(stream, note.author);
    stream << ",\"initials\":";
    write_json_optional_string(stream, note.initials);
    stream << ",\"date\":";
    write_json_optional_string(stream, note.date);
    stream << ",\"anchor_text\":";
    write_json_optional_string(stream, note.anchor_text);
    stream << ",\"resolved\":" << json_bool(note.resolved);
    stream << ",\"parent_index\":";
    write_json_optional_size(stream, note.parent_index);
    stream << ",\"parent_id\":";
    write_json_optional_string(stream, note.parent_id);
    stream << ",\"text\":";
    write_json_string(stream, note.text);
    stream << '}';
}

void write_json_revision_summary(
    std::ostream &stream, const featherdoc::revision_summary &revision) {
    stream << "{\"index\":" << revision.index << ",\"kind\":";
    write_json_string(stream, revision_kind_name(revision.kind));
    stream << ",\"id\":";
    write_json_string(stream, revision.id);
    stream << ",\"author\":";
    write_json_optional_string(stream, revision.author);
    stream << ",\"date\":";
    write_json_optional_string(stream, revision.date);
    stream << ",\"part_entry_name\":";
    write_json_string(stream, revision.part_entry_name);
    stream << ",\"text\":";
    write_json_string(stream, revision.text);
    stream << '}';
}

void write_json_text_range_preview_segment(
    std::ostream &stream,
    const featherdoc::text_range_preview_segment &segment) {
    stream << "{\"paragraph_index\":" << segment.paragraph_index
           << ",\"text_offset\":" << segment.text_offset
           << ",\"text_length\":" << segment.text_length << ",\"text\":";
    write_json_string(stream, segment.text);
    stream << '}';
}

void write_json_text_range_preview(
    std::ostream &stream, const featherdoc::text_range_preview &preview) {
    stream << "{\"start_paragraph_index\":" << preview.start_paragraph_index
           << ",\"start_text_offset\":" << preview.start_text_offset
           << ",\"end_paragraph_index\":" << preview.end_paragraph_index
           << ",\"end_text_offset\":" << preview.end_text_offset
           << ",\"text_length\":" << preview.text_length
           << ",\"plain_text_runs_supported\":"
           << json_bool(preview.plain_text_runs_supported) << ",\"text\":";
    write_json_string(stream, preview.text);
    stream << ",\"segments\":[";
    for (std::size_t index = 0U; index < preview.segments.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_text_range_preview_segment(stream, preview.segments[index]);
    }
    stream << "]}";
}

void write_json_text_range_matches(
    std::ostream &stream, std::string_view command, std::string_view query,
    const std::vector<featherdoc::text_range_preview> &matches) {
    stream << "{\"command\":";
    write_json_string(stream, command);
    stream << ",\"ok\":true,\"query\":";
    write_json_string(stream, query);
    stream << ",\"matches_count\":" << matches.size() << ",\"matches\":[";
    for (std::size_t index = 0U; index < matches.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"index\":" << index << ",\"preview\":";
        write_json_text_range_preview(stream, matches[index]);
        stream << '}';
    }
    stream << "]}\n";
}

void print_review_note_summary(
    std::ostream &stream, const featherdoc::review_note_summary &note) {
    stream << "index=" << note.index
           << " kind=" << review_note_kind_name(note.kind)
           << " id=" << note.id
           << " author=" << optional_display_value(note.author)
           << " initials=" << optional_display_value(note.initials)
           << " date=" << optional_display_value(note.date)
           << " anchor_text=" << optional_display_value(note.anchor_text)
           << " resolved=" << yes_no(note.resolved)
           << " parent_index=" << optional_size_display_value(note.parent_index)
           << " parent_id=" << optional_display_value(note.parent_id)
           << " text=";
    write_json_string(stream, note.text);
}

void print_revision_summary(
    std::ostream &stream, const featherdoc::revision_summary &revision) {
    stream << "index=" << revision.index
           << " kind=" << revision_kind_name(revision.kind)
           << " id=" << revision.id
           << " author=" << optional_display_value(revision.author)
           << " date=" << optional_display_value(revision.date)
           << " part_entry_name=" << revision.part_entry_name << " text=";
    write_json_string(stream, revision.text);
}

void print_text_range_preview(std::ostream &stream,
                              const featherdoc::text_range_preview &preview) {
    stream << "start_paragraph_index=" << preview.start_paragraph_index
           << " start_text_offset=" << preview.start_text_offset
           << " end_paragraph_index=" << preview.end_paragraph_index
           << " end_text_offset=" << preview.end_text_offset
           << " text_length=" << preview.text_length
           << " plain_text_runs_supported="
           << yes_no(preview.plain_text_runs_supported) << " text=";
    write_json_string(stream, preview.text);
    for (const auto &segment : preview.segments) {
        stream << '\n'
               << "segment paragraph_index=" << segment.paragraph_index
               << " text_offset=" << segment.text_offset
               << " text_length=" << segment.text_length << " text=";
        write_json_string(stream, segment.text);
    }
}

void print_text_range_matches(
    std::ostream &stream, std::string_view query,
    const std::vector<featherdoc::text_range_preview> &matches) {
    stream << "query=";
    write_json_string(stream, query);
    stream << " matches_count=" << matches.size();
    for (std::size_t index = 0U; index < matches.size(); ++index) {
        stream << '\n' << "match index=" << index << ' ';
        print_text_range_preview(stream, matches[index]);
    }
    stream << '\n';
}

void inspect_review(
    const std::vector<featherdoc::review_note_summary> &footnotes,
    const std::vector<featherdoc::review_note_summary> &endnotes,
    const std::vector<featherdoc::review_note_summary> &comments,
    const std::vector<featherdoc::revision_summary> &revisions, bool json_output) {
    if (json_output) {
        std::cout << "{\"footnotes_count\":" << footnotes.size()
                  << ",\"endnotes_count\":" << endnotes.size()
                  << ",\"comments_count\":" << comments.size()
                  << ",\"revisions_count\":" << revisions.size()
                  << ",\"footnotes\":[";
        for (std::size_t index = 0; index < footnotes.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_review_note_summary(std::cout, footnotes[index]);
        }
        std::cout << "],\"endnotes\":[";
        for (std::size_t index = 0; index < endnotes.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_review_note_summary(std::cout, endnotes[index]);
        }
        std::cout << "],\"comments\":[";
        for (std::size_t index = 0; index < comments.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_review_note_summary(std::cout, comments[index]);
        }
        std::cout << "],\"revisions\":[";
        for (std::size_t index = 0; index < revisions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_revision_summary(std::cout, revisions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "footnotes: " << footnotes.size() << '\n';
    for (std::size_t index = 0; index < footnotes.size(); ++index) {
        std::cout << "footnote[" << index << "]: ";
        print_review_note_summary(std::cout, footnotes[index]);
        std::cout << '\n';
    }
    std::cout << "endnotes: " << endnotes.size() << '\n';
    for (std::size_t index = 0; index < endnotes.size(); ++index) {
        std::cout << "endnote[" << index << "]: ";
        print_review_note_summary(std::cout, endnotes[index]);
        std::cout << '\n';
    }
    std::cout << "comments: " << comments.size() << '\n';
    for (std::size_t index = 0; index < comments.size(); ++index) {
        std::cout << "comment[" << index << "]: ";
        print_review_note_summary(std::cout, comments[index]);
        std::cout << '\n';
    }
    std::cout << "revisions: " << revisions.size() << '\n';
    for (std::size_t index = 0; index < revisions.size(); ++index) {
        std::cout << "revision[" << index << "]: ";
        print_revision_summary(std::cout, revisions[index]);
        std::cout << '\n';
    }
}

void print_simple_document_mutation_result(
    std::string_view command, const std::optional<path_type> &output_path,
    std::size_t affected) {
    std::cout << "command: " << command << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "affected: " << affected << '\n';
}

void write_json_affected_result(std::ostream &stream, std::size_t affected) {
    stream << ",\"affected\":" << affected;
}


auto report_expected_revision_text_mismatch(std::string_view command,
                                            std::string_view expected_text,
                                            const featherdoc::text_range_preview &preview,
                                            bool json_output) -> bool {
    constexpr std::string_view message =
        "expected text did not match selected text";
    if (json_output) {
        std::cerr << "{\"command\":";
        write_json_string(std::cerr, command);
        std::cerr << ",\"ok\":false,\"stage\":\"validate\",\"message\":";
        write_json_string(std::cerr, message);
        std::cerr << ",\"expected_text\":";
        write_json_string(std::cerr, expected_text);
        std::cerr << ",\"actual_text\":";
        write_json_string(std::cerr, preview.text);
        std::cerr << ",\"preview\":";
        write_json_text_range_preview(std::cerr, preview);
        std::cerr << "}\n";
        return false;
    }

    std::cerr << message << '\n' << "expected_text=";
    write_json_string(std::cerr, expected_text);
    std::cerr << '\n' << "actual_text=";
    write_json_string(std::cerr, preview.text);
    std::cerr << '\n' << "selected_range=";
    print_text_range_preview(std::cerr, preview);
    std::cerr << '\n';
    return false;
}

auto validate_revision_expected_text(featherdoc::Document &doc,
                                     std::string_view command,
                                     const featherdoc_cli::revision_authoring_options
                                         &options,
                                     std::size_t start_paragraph_index,
                                     std::size_t start_text_offset,
                                     std::size_t end_paragraph_index,
                                     std::size_t end_text_offset) -> bool {
    if (!options.has_expected_text) {
        return true;
    }

    const auto preview = doc.preview_text_range(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset);
    if (!preview.has_value()) {
        return report_document_error(command, "preview", doc.last_error(),
                                     options.json_output);
    }

    if (preview->text != options.expected_text) {
        return report_expected_revision_text_mismatch(
            command, options.expected_text, *preview, options.json_output);
    }

    return true;
}

void write_json_review_mutation_plan_preview_result(
    std::ostream &stream,
    const review_mutation_plan_preview_result &result) {
    stream << "{\"index\":" << result.index << ",\"kind\":";
    write_json_string(stream, review_mutation_plan_operation_kind_name(result.kind));
    stream << ",\"ok\":" << json_bool(result.ok) << ",\"message\":";
    write_json_string(stream, result.message);
    if (result.comment_index.has_value()) {
        stream << ",\"comment_index\":" << *result.comment_index;
    }
    stream << ",\"expected_text\":";
    if (result.expected_text.has_value()) {
        write_json_string(stream, *result.expected_text);
    } else {
        stream << "null";
    }
    if (result.expected_resolved.has_value()) {
        stream << ",\"expected_resolved\":"
               << json_bool(*result.expected_resolved);
    }
    if (result.expected_comment_text.has_value()) {
        stream << ",\"expected_comment_text\":";
        write_json_string(stream, *result.expected_comment_text);
    }
    if (result.expected_parent_index.has_value()) {
        stream << ",\"expected_parent_index\":"
               << *result.expected_parent_index;
    }
    stream << ",\"actual_text\":";
    if (result.actual_text.has_value()) {
        write_json_string(stream, *result.actual_text);
    } else if (result.preview.has_value()) {
        write_json_string(stream, result.preview->text);
    } else {
        stream << "null";
    }
    if (result.actual_resolved.has_value()) {
        stream << ",\"actual_resolved\":" << json_bool(*result.actual_resolved);
    }
    if (result.actual_comment_text.has_value()) {
        stream << ",\"actual_comment_text\":";
        write_json_string(stream, *result.actual_comment_text);
    }
    if (result.comment_index.has_value() ||
        result.expected_parent_index.has_value() ||
        result.actual_parent_index.has_value()) {
        stream << ",\"actual_parent_index\":";
        if (result.actual_parent_index.has_value()) {
            stream << *result.actual_parent_index;
        } else {
            stream << "null";
        }
    }
    stream << ",\"preview\":";
    if (result.preview.has_value()) {
        write_json_text_range_preview(stream, *result.preview);
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_review_mutation_plan_preview(
    std::ostream &stream,
    const std::vector<review_mutation_plan_preview_result> &results) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "{\"command\":\"preview-review-mutation-plan\",\"ok\":"
           << json_bool(failed_count == 0U)
           << ",\"operations_count\":" << results.size()
           << ",\"failed_count\":" << failed_count << ",\"operations\":[";
    for (std::size_t index = 0U; index < results.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_preview_result(stream, results[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_apply(
    std::ostream &stream, featherdoc::Document &doc,
    const std::optional<path_type> &output_path,
    const std::vector<review_mutation_plan_preview_result> &results,
    std::size_t applied_count) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "{\"command\":\"apply-review-mutation-plan\",\"ok\":true"
           << ",\"in_place\":" << json_bool(!output_path.has_value())
           << ",\"output_path\":";
    if (output_path.has_value()) {
        write_json_string(stream, output_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"sections\":" << doc.section_count()
           << ",\"headers\":" << doc.header_count()
           << ",\"footers\":" << doc.footer_count()
           << ",\"operations_count\":" << results.size()
           << ",\"applied_count\":" << applied_count
           << ",\"failed_count\":" << failed_count << ",\"operations\":[";
    for (std::size_t index = 0U; index < results.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_preview_result(stream, results[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_apply_failure(
    std::ostream &stream, std::string_view stage, std::string_view message,
    const std::vector<review_mutation_plan_preview_result> &results) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "{\"command\":\"apply-review-mutation-plan\",\"ok\":false"
           << ",\"stage\":";
    write_json_string(stream, stage);
    stream << ",\"message\":";
    write_json_string(stream, message);
    stream << ",\"operations_count\":" << results.size()
           << ",\"failed_count\":" << failed_count << ",\"operations\":[";
    for (std::size_t index = 0U; index < results.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_preview_result(stream, results[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_build_resolution(
    std::ostream &stream,
    const review_mutation_plan_build_resolution &resolution) {
    stream << "{\"index\":" << resolution.index << ",\"kind\":";
    write_json_string(stream,
                      review_mutation_plan_operation_kind_name(resolution.kind));
    stream << ",\"find_text\":";
    write_json_string(stream, resolution.find_text);
    stream << ",\"before_text\":";
    if (resolution.before_text.has_value()) {
        write_json_string(stream, *resolution.before_text);
    } else {
        stream << "null";
    }
    stream << ",\"after_text\":";
    if (resolution.after_text.has_value()) {
        write_json_string(stream, *resolution.after_text);
    } else {
        stream << "null";
    }
    stream << ",\"require_unique\":" << json_bool(resolution.require_unique)
           << ",\"insert_after_match\":"
           << json_bool(resolution.insert_after_match)
           << ",\"occurrence\":" << resolution.occurrence
           << ",\"raw_matches_count\":" << resolution.raw_matches_count
           << ",\"matches_count\":" << resolution.matches_count
           << ",\"selected_match_index\":";
    if (resolution.selected_match_index.has_value()) {
        stream << *resolution.selected_match_index;
    } else {
        stream << "null";
    }
    stream << ",\"preview\":";
    write_json_text_range_preview(stream, resolution.preview);
    stream << '}';
}

void write_json_review_mutation_plan_build_result(
    std::ostream &stream,
    const std::vector<review_mutation_plan_operation> &operations,
    const std::vector<review_mutation_plan_build_resolution> &resolutions,
    const std::optional<path_type> &output_plan_path) {
    stream << "{\"command\":\"build-review-mutation-plan\",\"ok\":true"
           << ",\"operations_count\":" << operations.size()
           << ",\"output_plan_path\":";
    if (output_plan_path.has_value()) {
        write_json_string(stream, output_plan_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"plan\":";
    write_json_review_mutation_plan_document(stream, operations);
    stream << ",\"resolutions\":[";
    for (std::size_t index = 0U; index < resolutions.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_build_resolution(stream,
                                                         resolutions[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_build_failure(
    std::ostream &stream, std::string_view stage, std::string_view message,
    std::size_t operation_index, std::size_t matches_count,
    std::size_t raw_matches_count) {
    stream << "{\"command\":\"build-review-mutation-plan\",\"ok\":false"
           << ",\"stage\":";
    write_json_string(stream, stage);
    stream << ",\"message\":";
    write_json_string(stream, message);
    stream << ",\"operation_index\":" << operation_index
           << ",\"matches_count\":" << matches_count
           << ",\"raw_matches_count\":" << raw_matches_count << "}\n";
}

void print_review_mutation_plan_preview(
    std::ostream &stream,
    const std::vector<review_mutation_plan_preview_result> &results) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "ok=" << yes_no(failed_count == 0U)
           << " operations_count=" << results.size()
           << " failed_count=" << failed_count;

    for (const auto &result : results) {
        stream << '\n'
               << "operation index=" << result.index
               << " kind="
               << review_mutation_plan_operation_kind_name(result.kind)
               << " ok=" << yes_no(result.ok) << " message=";
        write_json_string(stream, result.message);
        if (result.expected_text.has_value()) {
            stream << " expected_text=";
            write_json_string(stream, *result.expected_text);
        }
        if (result.expected_resolved.has_value()) {
            stream << " expected_resolved="
                   << json_bool(*result.expected_resolved);
        }
        if (result.expected_comment_text.has_value()) {
            stream << " expected_comment_text=";
            write_json_string(stream, *result.expected_comment_text);
        }
        if (result.expected_parent_index.has_value()) {
            stream << " expected_parent_index="
                   << *result.expected_parent_index;
        }
        if (result.actual_text.has_value()) {
            stream << " actual_text=";
            write_json_string(stream, *result.actual_text);
        }
        if (result.actual_resolved.has_value()) {
            stream << " actual_resolved="
                   << json_bool(*result.actual_resolved);
        }
        if (result.actual_comment_text.has_value()) {
            stream << " actual_comment_text=";
            write_json_string(stream, *result.actual_comment_text);
        }
        if (result.actual_parent_index.has_value()) {
            stream << " actual_parent_index="
                   << *result.actual_parent_index;
        }
        if (result.preview.has_value()) {
            stream << " actual_text=";
            write_json_string(stream, result.preview->text);
            stream << '\n';
            print_text_range_preview(stream, *result.preview);
        }
    }
    stream << '\n';
}

} // namespace

auto is_review_command(std::string_view command) -> bool {
    return command == "inspect-review" ||
           command == "find-text-ranges" ||
           command == "build-review-mutation-plan" ||
           command == "preview-review-mutation-plan" ||
           command == "apply-review-mutation-plan" ||
           command == "preview-text-range" ||
           command == "append-insertion-revision" ||
           command == "append-deletion-revision" ||
           command == "insert-run-revision-after" ||
           command == "delete-run-revision" ||
           command == "replace-run-revision" ||
           command == "insert-paragraph-text-revision" ||
           command == "delete-paragraph-text-revision" ||
           command == "replace-paragraph-text-revision" ||
           command == "insert-text-range-revision" ||
           command == "delete-text-range-revision" ||
           command == "replace-text-range-revision" ||
           command == "set-revision-metadata" ||
           command == "accept-revision" ||
           command == "reject-revision" ||
           command == "accept-all-revisions" ||
           command == "reject-all-revisions" ||
           command == "append-footnote" ||
           command == "replace-footnote" ||
           command == "remove-footnote" ||
           command == "append-endnote" ||
           command == "replace-endnote" ||
           command == "remove-endnote" ||
           command == "append-paragraph-text-comment" ||
           command == "append-text-range-comment" ||
           command == "set-paragraph-text-comment-range" ||
           command == "set-text-range-comment-range" ||
           command == "append-comment-reply" ||
           command == "set-comment-metadata" ||
           command == "set-comment-resolved" ||
           command == "append-comment" ||
           command == "replace-comment" ||
           command == "remove-comment";
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

    if (command == "build-review-mutation-plan") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "build-review-mutation-plan expects an input path",
                              json_output);
            return 2;
        }

        std::optional<path_type> request_file_path;
        std::optional<path_type> output_plan_path;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--request-file") {
                if (request_file_path.has_value()) {
                    print_parse_error(command, "duplicate --request-file option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command,
                                      "missing path after --request-file",
                                      json_output);
                    return 2;
                }
                request_file_path =
                    path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--output-plan") {
                if (output_plan_path.has_value()) {
                    print_parse_error(command, "duplicate --output-plan option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command,
                                      "missing path after --output-plan",
                                      json_output);
                    return 2;
                }
                output_plan_path = path_type(std::string(arguments[index + 1U]));
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

        if (!request_file_path.has_value()) {
            print_parse_error(command, "missing --request-file <request.json>",
                              json_output);
            return 2;
        }

        std::vector<review_mutation_plan_build_request_operation> requests;
        std::string error_message;
        if (!read_review_mutation_plan_build_request_file(
                *request_file_path, requests, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        std::vector<review_mutation_plan_operation> operations;
        std::vector<review_mutation_plan_build_resolution> resolutions;
        std::size_t failed_operation_index = 0U;
        std::size_t failed_matches_count = 0U;
        std::size_t failed_raw_matches_count = 0U;
        if (!build_review_mutation_plan_operations(
                doc, requests, operations, resolutions, error_message,
                failed_operation_index, failed_matches_count,
                failed_raw_matches_count)) {
            if (json_output) {
                write_json_review_mutation_plan_build_failure(
                    std::cout, "resolve", error_message, failed_operation_index,
                    failed_matches_count, failed_raw_matches_count);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (output_plan_path.has_value() &&
            !write_review_mutation_plan_file(*output_plan_path, operations,
                                             error_message)) {
            if (json_output) {
                write_json_review_mutation_plan_build_failure(
                    std::cout, "write", error_message, 0U, 0U, 0U);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (json_output) {
            write_json_review_mutation_plan_build_result(
                std::cout, operations, resolutions, output_plan_path);
        } else if (output_plan_path.has_value()) {
            std::cout << "command: " << command << '\n'
                      << "output_plan_path: " << output_plan_path->string()
                      << '\n'
                      << "operations_count: " << operations.size() << '\n';
        } else {
            write_json_review_mutation_plan_document(std::cout, operations);
            std::cout << '\n';
        }
        return 0;
    }

    if (command == "preview-review-mutation-plan") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "preview-review-mutation-plan expects an input path",
                              json_output);
            return 2;
        }

        std::optional<path_type> plan_file_path;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--plan-file") {
                if (plan_file_path.has_value()) {
                    print_parse_error(command, "duplicate --plan-file option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing path after --plan-file",
                                      json_output);
                    return 2;
                }
                plan_file_path = path_type(std::string(arguments[index + 1U]));
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

        if (!plan_file_path.has_value()) {
            print_parse_error(command, "missing --plan-file <plan.json>",
                              json_output);
            return 2;
        }

        std::vector<review_mutation_plan_operation> operations;
        std::string error_message;
        if (!read_review_mutation_plan_file(*plan_file_path, operations,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto results =
            preview_review_mutation_plan_operations(doc, operations);
        const auto failed_count =
            static_cast<std::size_t>(std::count_if(
                results.begin(), results.end(),
                [](const auto &result) { return !result.ok; }));

        if (json_output) {
            write_json_review_mutation_plan_preview(std::cout, results);
        } else {
            print_review_mutation_plan_preview(std::cout, results);
        }
        return failed_count == 0U ? 0 : 1;
    }

    if (command == "apply-review-mutation-plan") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "apply-review-mutation-plan expects an input path",
                              json_output);
            return 2;
        }

        std::optional<path_type> plan_file_path;
        std::optional<path_type> output_path;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--plan-file") {
                if (plan_file_path.has_value()) {
                    print_parse_error(command, "duplicate --plan-file option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing path after --plan-file",
                                      json_output);
                    return 2;
                }
                plan_file_path = path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--output") {
                if (output_path.has_value()) {
                    print_parse_error(command, "duplicate --output option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing path after --output",
                                      json_output);
                    return 2;
                }
                output_path = path_type(std::string(arguments[index + 1U]));
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

        if (!plan_file_path.has_value()) {
            print_parse_error(command, "missing --plan-file <plan.json>",
                              json_output);
            return 2;
        }

        std::vector<review_mutation_plan_operation> operations;
        std::string error_message;
        if (!read_review_mutation_plan_file(*plan_file_path, operations,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto preview_results =
            preview_review_mutation_plan_operations(doc, operations);
        const auto failed_count =
            static_cast<std::size_t>(std::count_if(
                preview_results.begin(), preview_results.end(),
                [](const auto &result) { return !result.ok; }));
        if (failed_count != 0U) {
            if (json_output) {
                write_json_review_mutation_plan_apply_failure(
                    std::cout, "preflight",
                    "review mutation plan preflight failed", preview_results);
            } else {
                std::cerr << "review mutation plan preflight failed\n";
                print_review_mutation_plan_preview(std::cerr, preview_results);
            }
            return 1;
        }

        if (find_review_mutation_plan_overlap(operations, error_message)) {
            if (json_output) {
                write_json_review_mutation_plan_apply_failure(
                    std::cout, "validate", error_message, preview_results);
            } else {
                std::cerr << error_message << '\n';
                print_review_mutation_plan_preview(std::cerr, preview_results);
            }
            return 1;
        }

        std::size_t applied_count = 0U;
        if (!apply_review_mutation_plan_operations(
                doc, operations, applied_count, error_message)) {
            if (json_output) {
                write_json_review_mutation_plan_apply_failure(
                    std::cout, "mutate", error_message, preview_results);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!save_document(doc, output_path, command, json_output)) {
            return 1;
        }

        if (json_output) {
            write_json_review_mutation_plan_apply(
                std::cout, doc, output_path, preview_results, applied_count);
        } else {
            print_simple_document_mutation_result(command, output_path,
                                                  applied_count);
        }
        return 0;
    }

    if (command == "preview-text-range") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 6U) {
            print_parse_error(
                command,
                "preview-text-range expects an input path, start paragraph index, start offset, end paragraph index, and end offset",
                json_output);
            return 2;
        }
        if (arguments.size() > 7U ||
            (arguments.size() == 7U && arguments[6] != "--json")) {
            print_parse_error(command, "unknown option: " +
                                           std::string(arguments[6]),
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

        std::size_t end_paragraph_index = 0U;
        if (!parse_index(arguments[4], end_paragraph_index)) {
            print_parse_error(command,
                              "invalid end paragraph index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        std::size_t end_text_offset = 0U;
        if (!parse_index(arguments[5], end_text_offset)) {
            print_parse_error(command,
                              "invalid end text offset: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto preview = doc.preview_text_range(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset);
        if (!preview.has_value()) {
            report_document_error(command, "preview", doc.last_error(),
                                  json_output);
            return 1;
        }

        if (json_output) {
            std::cout << "{\"command\":\"preview-text-range\",\"ok\":true,"
                      << "\"preview\":";
            write_json_text_range_preview(std::cout, *preview);
            std::cout << "}\n";
        } else {
            print_text_range_preview(std::cout, *preview);
            std::cout << '\n';
        }
        return 0;
    }

    if (command == "append-insertion-revision" ||
        command == "append-deletion-revision") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              std::string(command) + " expects an input path",
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        if (!parse_revision_authoring_options(arguments, 2U, options,
                                              error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const bool insertion = command == "append-insertion-revision";
        const auto affected = insertion
                                  ? doc.append_insertion_revision(
                                        options.text, options.author, options.date)
                                  : doc.append_deletion_revision(
                                        options.text, options.author, options.date);
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

    if (command == "insert-run-revision-after" ||
        command == "delete-run-revision" ||
        command == "replace-run-revision") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                std::string(command) +
                    " expects an input path, paragraph index, and run index",
                json_output);
            return 2;
        }

        std::size_t paragraph_index = 0U;
        if (!parse_index(arguments[2], paragraph_index)) {
            print_parse_error(command,
                              "invalid paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t run_index = 0U;
        if (!parse_index(arguments[3], run_index)) {
            print_parse_error(command,
                              "invalid run index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-run-revision";
        if (!parse_revision_authoring_options(arguments, 4U, options,
                                              error_message, require_text)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        bool ok = false;
        if (command == "insert-run-revision-after") {
            ok = doc.insert_run_revision_after(
                paragraph_index, run_index, options.text, options.author,
                options.date);
        } else if (command == "delete-run-revision") {
            ok = doc.delete_run_revision(paragraph_index, run_index,
                                         options.author, options.date);
        } else {
            ok = doc.replace_run_revision(paragraph_index, run_index,
                                          options.text, options.author,
                                          options.date);
        }
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
                [paragraph_index, run_index](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"paragraph_index\":" << paragraph_index
                           << ",\"run_index\":" << run_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "insert-paragraph-text-revision" ||
        command == "delete-paragraph-text-revision" ||
        command == "replace-paragraph-text-revision") {
        const auto json_output = has_json_flag(arguments);
        const bool insert_revision = command == "insert-paragraph-text-revision";
        if (arguments.size() < (insert_revision ? 4U : 5U)) {
            print_parse_error(
                command,
                insert_revision
                    ? std::string(command) +
                          " expects an input path, paragraph index, and text offset"
                    : std::string(command) +
                          " expects an input path, paragraph index, text offset, and text length",
                json_output);
            return 2;
        }

        std::size_t paragraph_index = 0U;
        if (!parse_index(arguments[2], paragraph_index)) {
            print_parse_error(command,
                              "invalid paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t text_offset = 0U;
        if (!parse_index(arguments[3], text_offset)) {
            print_parse_error(command,
                              "invalid text offset: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t text_length = 0U;
        if (!insert_revision && !parse_index(arguments[4], text_length)) {
            print_parse_error(command,
                              "invalid text length: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-paragraph-text-revision";
        const auto options_start = insert_revision ? 4U : 5U;
        if (!parse_revision_authoring_options(
                arguments, options_start, options, error_message, require_text,
                "delete-paragraph-text-revision does not accept --text",
                !insert_revision,
                "insert-paragraph-text-revision does not accept --expected-text")) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!insert_revision) {
            if (text_length >
                std::numeric_limits<std::size_t>::max() - text_offset) {
                featherdoc::document_error_info error_info{};
                error_info.code =
                    std::make_error_code(std::errc::invalid_argument);
                error_info.entry_name = "word/document.xml";
                error_info.detail = "text range is out of range";
                report_operation_failure(command, "validate",
                                         "text range is out of range",
                                         error_info, options.json_output);
                return 1;
            }

            if (!validate_revision_expected_text(
                    doc, command, options, paragraph_index, text_offset,
                    paragraph_index, text_offset + text_length)) {
                return 1;
            }
        }

        bool ok = false;
        if (insert_revision) {
            ok = doc.insert_paragraph_text_revision(
                paragraph_index, text_offset, options.text, options.author,
                options.date);
        } else if (command == "delete-paragraph-text-revision") {
            ok = doc.delete_paragraph_text_revision(
                paragraph_index, text_offset, text_length, options.author,
                options.date);
        } else {
            ok = doc.replace_paragraph_text_revision(
                paragraph_index, text_offset, text_length, options.text,
                options.author, options.date);
        }
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
                [insert_revision, paragraph_index, text_offset,
                 text_length](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"paragraph_index\":" << paragraph_index
                           << ",\"text_offset\":" << text_offset;
                    if (!insert_revision) {
                        stream << ",\"text_length\":" << text_length;
                    }
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "insert-text-range-revision" ||
        command == "delete-text-range-revision" ||
        command == "replace-text-range-revision") {
        const auto json_output = has_json_flag(arguments);
        const bool insert_revision = command == "insert-text-range-revision";
        if (arguments.size() < (insert_revision ? 4U : 6U)) {
            print_parse_error(
                command,
                insert_revision
                    ? std::string(command) +
                          " expects an input path, paragraph index, and text offset"
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

        std::size_t end_paragraph_index = start_paragraph_index;
        std::size_t end_text_offset = start_text_offset;
        if (!insert_revision) {
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

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-text-range-revision";
        const auto options_start = insert_revision ? 4U : 6U;
        if (!parse_revision_authoring_options(
                arguments, options_start, options, error_message, require_text,
                "delete-text-range-revision does not accept --text",
                !insert_revision,
                "insert-text-range-revision does not accept --expected-text")) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!insert_revision &&
            !validate_revision_expected_text(
                doc, command, options, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset)) {
            return 1;
        }

        bool ok = false;
        if (insert_revision) {
            ok = doc.insert_text_range_revision(
                start_paragraph_index, start_text_offset, options.text,
                options.author, options.date);
        } else if (command == "delete-text-range-revision") {
            ok = doc.delete_text_range_revision(
                start_paragraph_index, start_text_offset, end_paragraph_index,
                end_text_offset, options.author, options.date);
        } else {
            ok = doc.replace_text_range_revision(
                start_paragraph_index, start_text_offset, end_paragraph_index,
                end_text_offset, options.text, options.author, options.date);
        }
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
                [insert_revision, start_paragraph_index, start_text_offset,
                 end_paragraph_index, end_text_offset](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"start_paragraph_index\":"
                           << start_paragraph_index
                           << ",\"start_text_offset\":" << start_text_offset;
                    if (!insert_revision) {
                        stream << ",\"end_paragraph_index\":"
                               << end_paragraph_index
                               << ",\"end_text_offset\":" << end_text_offset;
                    }
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "set-revision-metadata") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-revision-metadata expects an input path and revision index",
                json_output);
            return 2;
        }

        std::size_t revision_index = 0U;
        if (!parse_index(arguments[2], revision_index)) {
            print_parse_error(command,
                              "invalid revision index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        revision_metadata_mutation_options options;
        std::string error_message;
        if (!parse_revision_metadata_mutation_options(arguments, 3U, options,
                                                      error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.set_revision_metadata(revision_index, options.metadata)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [revision_index](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"revision_index\":" << revision_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "accept-revision" || command == "reject-revision" ||
        command == "accept-all-revisions" || command == "reject-all-revisions") {
        const auto json_output = has_json_flag(arguments);
        const bool accept_one = command == "accept-revision";
        const bool reject_one = command == "reject-revision";
        const bool one_revision = accept_one || reject_one;
        if (arguments.size() < (one_revision ? 3U : 2U)) {
            print_parse_error(command,
                              one_revision ? std::string(command) +
                                                 " expects an input path and revision index"
                                           : std::string(command) +
                                                 " expects an input path",
                              json_output);
            return 2;
        }

        std::size_t revision_index = 0U;
        if (one_revision && !parse_index(arguments[2], revision_index)) {
            print_parse_error(command,
                              "invalid revision index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        simple_document_mutation_options options;
        std::string error_message;
        if (!parse_simple_document_mutation_options(
                arguments, one_revision ? 3U : 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::size_t affected = 0U;
        if (accept_one) {
            affected = doc.accept_revision(revision_index) ? 1U : 0U;
        } else if (reject_one) {
            affected = doc.reject_revision(revision_index) ? 1U : 0U;
        } else if (command == "accept-all-revisions") {
            affected = doc.accept_all_revisions();
        } else {
            affected = doc.reject_all_revisions();
        }
        if (one_revision && affected == 0U) {
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

    if (command == "append-paragraph-text-comment" ||
        command == "append-text-range-comment") {
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
        if (!parse_comment_mutation_options(
                arguments, paragraph_range ? 5U : 6U, options, false, true,
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

    if (command == "set-paragraph-text-comment-range" ||
        command == "set-text-range-comment-range") {
        const auto json_output = has_json_flag(arguments);
        const bool paragraph_range =
            command == "set-paragraph-text-comment-range";
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
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "append-comment-reply") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "append-comment-reply expects an input path and parent comment index",
                json_output);
            return 2;
        }

        std::size_t parent_comment_index = 0U;
        if (!parse_index(arguments[2], parent_comment_index)) {
            print_parse_error(command,
                              "invalid parent comment index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        comment_mutation_options options;
        std::string error_message;
        if (!parse_comment_mutation_options(arguments, 3U, options, false, true,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto affected = doc.append_comment_reply(
            parent_comment_index, options.comment_text, options.author,
            options.initials, options.date);
        if (affected == 0U) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [affected, parent_comment_index](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                    stream << ",\"parent_comment_index\":"
                           << parent_comment_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "set-comment-metadata") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-comment-metadata expects an input path and comment index",
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

        comment_metadata_mutation_options options;
        std::string error_message;
        if (!parse_comment_metadata_mutation_options(arguments, 3U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.set_comment_metadata(comment_index, options.metadata)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [comment_index](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"comment_index\":" << comment_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "set-comment-resolved") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                std::string(command) +
                    " expects an input path, comment index, and resolved value",
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

        bool resolved = false;
        if (!parse_bool(arguments[3], resolved)) {
            print_parse_error(command,
                              "invalid resolved value: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        simple_document_mutation_options options;
        std::string error_message;
        if (!parse_simple_document_mutation_options(arguments, 4U, options,
                                                    error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.set_comment_resolved(comment_index, resolved)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [comment_index, resolved](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"comment_index\":" << comment_index
                           << ",\"resolved\":" << json_bool(resolved);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "append-comment" || command == "replace-comment" ||
        command == "remove-comment") {
        const auto json_output = has_json_flag(arguments);
        const bool append = command == "append-comment";
        const bool replace = command == "replace-comment";
        if (arguments.size() < (append ? 2U : 3U)) {
            print_parse_error(command,
                              append ? "append-comment expects an input path"
                                     : std::string(command) +
                                           " expects an input path and comment index",
                              json_output);
            return 2;
        }

        std::size_t comment_index = 0U;
        if (!append && !parse_index(arguments[2], comment_index)) {
            print_parse_error(command,
                              "invalid comment index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        comment_mutation_options options;
        std::string error_message;
        if (!parse_comment_mutation_options(
                arguments, append ? 2U : 3U, options, append, append || replace,
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
            affected = doc.append_comment(options.selected_text,
                                          options.comment_text, options.author,
                                          options.initials, options.date);
        } else if (replace) {
            affected = doc.replace_comment(comment_index, options.comment_text) ? 1U
                                                                                : 0U;
        } else {
            affected = doc.remove_comment(comment_index) ? 1U : 0U;
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


    print_parse_error(command, "unknown review command", has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
