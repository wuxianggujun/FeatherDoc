#include "featherdoc_cli_review_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include <algorithm>
#include <iostream>

namespace featherdoc_cli {


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

} // namespace featherdoc_cli
