#include "featherdoc_cli_semantic_diff.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

auto semantic_diff_change_kind_name(
    featherdoc::document_semantic_diff_change_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::document_semantic_diff_change_kind::added:
        return "added";
    case featherdoc::document_semantic_diff_change_kind::removed:
        return "removed";
    case featherdoc::document_semantic_diff_change_kind::changed:
        return "changed";
    }

    return "changed";
}

void write_json_semantic_diff_category_summary(
    std::ostream &stream,
    const featherdoc::document_semantic_diff_category_summary &summary) {
    stream << "{\"left_count\":" << summary.left_count
           << ",\"right_count\":" << summary.right_count
           << ",\"added_count\":" << summary.added_count
           << ",\"removed_count\":" << summary.removed_count
           << ",\"changed_count\":" << summary.changed_count
           << ",\"unchanged_count\":" << summary.unchanged_count
           << ",\"change_count\":" << summary.change_count()
           << ",\"different\":" << json_bool(summary.different()) << '}';
}

void write_json_semantic_diff_field_change(
    std::ostream &stream,
    const featherdoc::document_semantic_diff_field_change &change) {
    stream << "{\"field_path\":";
    write_json_string(stream, change.field_path);
    stream << ",\"left_value\":";
    write_json_string(stream, change.left_value);
    stream << ",\"right_value\":";
    write_json_string(stream, change.right_value);
    stream << '}';
}

void write_json_semantic_diff_field_changes(
    std::ostream &stream,
    const std::vector<featherdoc::document_semantic_diff_field_change> &changes) {
    stream << '[';
    for (std::size_t index = 0; index < changes.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_semantic_diff_field_change(stream, changes[index]);
    }
    stream << ']';
}

void write_json_semantic_diff_change(
    std::ostream &stream,
    const featherdoc::document_semantic_diff_change &change) {
    stream << "{\"kind\":";
    write_json_string(stream, semantic_diff_change_kind_name(change.kind));
    stream << ",\"left_index\":";
    write_json_optional_size(stream, change.left_index);
    stream << ",\"right_index\":";
    write_json_optional_size(stream, change.right_index);
    stream << ",\"field\":";
    write_json_string(stream, change.field);
    stream << ",\"left_value\":";
    write_json_string(stream, change.left_value);
    stream << ",\"right_value\":";
    write_json_string(stream, change.right_value);
    stream << ",\"field_changes\":";
    write_json_semantic_diff_field_changes(stream, change.field_changes);
    stream << '}';
}

void write_json_semantic_diff_changes(
    std::ostream &stream,
    const std::vector<featherdoc::document_semantic_diff_change> &changes) {
    stream << '[';
    for (std::size_t index = 0; index < changes.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_semantic_diff_change(stream, changes[index]);
    }
    stream << ']';
}

auto semantic_diff_part_kind_name(featherdoc::template_schema_part_kind kind)
    -> const char * {
    switch (kind) {
    case featherdoc::template_schema_part_kind::body:
        return "body";
    case featherdoc::template_schema_part_kind::header:
        return "header";
    case featherdoc::template_schema_part_kind::footer:
        return "footer";
    case featherdoc::template_schema_part_kind::section_header:
        return "section-header";
    case featherdoc::template_schema_part_kind::section_footer:
        return "section-footer";
    }

    return "body";
}

void write_json_semantic_diff_part_result(
    std::ostream &stream,
    const featherdoc::document_semantic_diff_part_result &part_result) {
    stream << "{\"part\":\"" << semantic_diff_part_kind_name(part_result.target.part)
           << "\"";
    if (part_result.target.part_index.has_value()) {
        stream << ",\"part_index\":" << *part_result.target.part_index;
    }
    if (part_result.target.section_index.has_value()) {
        stream << ",\"section_index\":" << *part_result.target.section_index;
    }
    if (part_result.target.part ==
            featherdoc::template_schema_part_kind::section_header ||
        part_result.target.part ==
            featherdoc::template_schema_part_kind::section_footer) {
        stream << ",\"reference_kind\":";
        write_json_string(stream, featherdoc::to_xml_reference_type(
                                      part_result.target.reference_kind));
    }
    if (part_result.left_resolved_from_section_index.has_value()) {
        stream << ",\"left_resolved_from_section_index\":"
               << *part_result.left_resolved_from_section_index;
    }
    if (part_result.right_resolved_from_section_index.has_value()) {
        stream << ",\"right_resolved_from_section_index\":"
               << *part_result.right_resolved_from_section_index;
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, part_result.entry_name);
    stream << ",\"different\":" << json_bool(part_result.different())
           << ",\"change_count\":" << part_result.change_count()
           << ",\"summary\":{\"paragraphs\":";
    write_json_semantic_diff_category_summary(stream, part_result.paragraphs);
    stream << ",\"tables\":";
    write_json_semantic_diff_category_summary(stream, part_result.tables);
    stream << ",\"images\":";
    write_json_semantic_diff_category_summary(stream, part_result.images);
    stream << ",\"content_controls\":";
    write_json_semantic_diff_category_summary(stream,
                                             part_result.content_controls);
    stream << ",\"fields\":";
    write_json_semantic_diff_category_summary(stream, part_result.fields);
    stream << "},\"paragraph_changes\":";
    write_json_semantic_diff_changes(stream, part_result.paragraph_changes);
    stream << ",\"table_changes\":";
    write_json_semantic_diff_changes(stream, part_result.table_changes);
    stream << ",\"image_changes\":";
    write_json_semantic_diff_changes(stream, part_result.image_changes);
    stream << ",\"content_control_changes\":";
    write_json_semantic_diff_changes(stream,
                                     part_result.content_control_changes);
    stream << ",\"field_changes\":";
    write_json_semantic_diff_changes(stream, part_result.field_changes);
    stream << '}';
}

void write_json_semantic_diff_part_results(
    std::ostream &stream,
    const std::vector<featherdoc::document_semantic_diff_part_result>
        &part_results) {
    stream << '[';
    for (std::size_t index = 0; index < part_results.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_semantic_diff_part_result(stream, part_results[index]);
    }
    stream << ']';
}

void write_json_semantic_diff_result(
    std::ostream &stream, const featherdoc::document_semantic_diff_result &result,
    const semantic_diff_options &options) {
    stream << "{\"command\":\"semantic-diff\",\"ok\":true"
           << ",\"different\":" << json_bool(result.different())
           << ",\"change_count\":" << result.change_count()
           << ",\"fail_on_diff\":" << json_bool(options.fail_on_diff)
           << ",\"align_sequences_by_content\":"
           << json_bool(options.diff_options.align_sequences_by_content)
           << ",\"alignment_cell_limit\":"
           << options.diff_options.alignment_cell_limit << ",\"summary\":{";
    stream << "\"paragraphs\":";
    write_json_semantic_diff_category_summary(stream, result.paragraphs);
    stream << ",\"tables\":";
    write_json_semantic_diff_category_summary(stream, result.tables);
    stream << ",\"images\":";
    write_json_semantic_diff_category_summary(stream, result.images);
    stream << ",\"content_controls\":";
    write_json_semantic_diff_category_summary(stream, result.content_controls);
    stream << ",\"fields\":";
    write_json_semantic_diff_category_summary(stream, result.fields);
    stream << ",\"styles\":";
    write_json_semantic_diff_category_summary(stream, result.styles);
    stream << ",\"numbering\":";
    write_json_semantic_diff_category_summary(stream, result.numbering);
    stream << ",\"footnotes\":";
    write_json_semantic_diff_category_summary(stream, result.footnotes);
    stream << ",\"endnotes\":";
    write_json_semantic_diff_category_summary(stream, result.endnotes);
    stream << ",\"comments\":";
    write_json_semantic_diff_category_summary(stream, result.comments);
    stream << ",\"revisions\":";
    write_json_semantic_diff_category_summary(stream, result.revisions);
    stream << ",\"sections\":";
    write_json_semantic_diff_category_summary(stream, result.sections);
    stream << ",\"template_parts\":";
    write_json_semantic_diff_category_summary(stream, result.template_parts);
    stream << "},\"paragraph_changes\":";
    write_json_semantic_diff_changes(stream, result.paragraph_changes);
    stream << ",\"table_changes\":";
    write_json_semantic_diff_changes(stream, result.table_changes);
    stream << ",\"image_changes\":";
    write_json_semantic_diff_changes(stream, result.image_changes);
    stream << ",\"content_control_changes\":";
    write_json_semantic_diff_changes(stream, result.content_control_changes);
    stream << ",\"field_changes\":";
    write_json_semantic_diff_changes(stream, result.field_changes);
    stream << ",\"style_changes\":";
    write_json_semantic_diff_changes(stream, result.style_changes);
    stream << ",\"numbering_changes\":";
    write_json_semantic_diff_changes(stream, result.numbering_changes);
    stream << ",\"footnote_changes\":";
    write_json_semantic_diff_changes(stream, result.footnote_changes);
    stream << ",\"endnote_changes\":";
    write_json_semantic_diff_changes(stream, result.endnote_changes);
    stream << ",\"comment_changes\":";
    write_json_semantic_diff_changes(stream, result.comment_changes);
    stream << ",\"revision_changes\":";
    write_json_semantic_diff_changes(stream, result.revision_changes);
    stream << ",\"section_changes\":";
    write_json_semantic_diff_changes(stream, result.section_changes);
    stream << ",\"template_part_results\":";
    write_json_semantic_diff_part_results(stream, result.template_part_results);
    stream << "}\n";
}

void print_semantic_diff_category(
    std::ostream &stream, std::string_view name,
    const featherdoc::document_semantic_diff_category_summary &summary) {
    stream << name << ": left=" << summary.left_count
           << " right=" << summary.right_count
           << " added=" << summary.added_count
           << " removed=" << summary.removed_count
           << " changed=" << summary.changed_count
           << " unchanged=" << summary.unchanged_count << '\n';
}

void print_semantic_diff_changes(
    std::ostream &stream, std::string_view name,
    const std::vector<featherdoc::document_semantic_diff_change> &changes) {
    for (std::size_t index = 0; index < changes.size(); ++index) {
        const auto &change = changes[index];
        stream << name << '[' << index << "]: "
               << semantic_diff_change_kind_name(change.kind) << ' ';
        if (change.left_index.has_value()) {
            stream << "left=" << *change.left_index << ' ';
        }
        if (change.right_index.has_value()) {
            stream << "right=" << *change.right_index << ' ';
        }
        stream << "field=" << change.field
               << " field_changes=" << change.field_changes.size() << '\n';
        for (std::size_t field_index = 0U;
             field_index < change.field_changes.size(); ++field_index) {
            const auto &field_change = change.field_changes[field_index];
            stream << "  " << name << '[' << index << "].field["
                   << field_index << "]: " << field_change.field_path << '\n';
        }
    }
}

void print_semantic_diff_part_results(
    std::ostream &stream,
    const std::vector<featherdoc::document_semantic_diff_part_result>
        &part_results) {
    for (std::size_t index = 0U; index < part_results.size(); ++index) {
        const auto &part_result = part_results[index];
        stream << "template_part[" << index << "]: part="
               << semantic_diff_part_kind_name(part_result.target.part);
        if (part_result.target.part_index.has_value()) {
            stream << " index=" << *part_result.target.part_index;
        }
        if (part_result.target.section_index.has_value()) {
            stream << " section=" << *part_result.target.section_index;
        }
        if (part_result.target.part ==
                featherdoc::template_schema_part_kind::section_header ||
            part_result.target.part ==
                featherdoc::template_schema_part_kind::section_footer) {
            stream << " kind="
                   << featherdoc::to_xml_reference_type(
                          part_result.target.reference_kind);
        }
        if (part_result.left_resolved_from_section_index.has_value()) {
            stream << " left_resolved_from="
                   << *part_result.left_resolved_from_section_index;
        }
        if (part_result.right_resolved_from_section_index.has_value()) {
            stream << " right_resolved_from="
                   << *part_result.right_resolved_from_section_index;
        }
        stream << " entry=" << part_result.entry_name
               << " change_count=" << part_result.change_count() << '\n';
        print_semantic_diff_category(stream, "  paragraphs",
                                     part_result.paragraphs);
        print_semantic_diff_category(stream, "  tables", part_result.tables);
        print_semantic_diff_category(stream, "  images", part_result.images);
        print_semantic_diff_category(stream, "  content_controls",
                                     part_result.content_controls);
        print_semantic_diff_category(stream, "  fields", part_result.fields);
        print_semantic_diff_changes(stream, "  template_part_paragraph_change",
                                    part_result.paragraph_changes);
        print_semantic_diff_changes(stream, "  template_part_table_change",
                                    part_result.table_changes);
        print_semantic_diff_changes(stream, "  template_part_image_change",
                                    part_result.image_changes);
        print_semantic_diff_changes(
            stream, "  template_part_content_control_change",
            part_result.content_control_changes);
        print_semantic_diff_changes(stream, "  template_part_field_change",
                                    part_result.field_changes);
    }
}

void print_semantic_diff_result(
    std::ostream &stream, const featherdoc::document_semantic_diff_result &result,
    const semantic_diff_options &options) {
    stream << "different: " << yes_no(result.different()) << '\n'
           << "change_count: " << result.change_count() << '\n'
           << "fail_on_diff: " << yes_no(options.fail_on_diff) << '\n'
           << "align_sequences_by_content: "
           << yes_no(options.diff_options.align_sequences_by_content) << '\n'
           << "alignment_cell_limit: "
           << options.diff_options.alignment_cell_limit << '\n';
    print_semantic_diff_category(stream, "paragraphs", result.paragraphs);
    print_semantic_diff_category(stream, "tables", result.tables);
    print_semantic_diff_category(stream, "images", result.images);
    print_semantic_diff_category(stream, "content_controls",
                                 result.content_controls);
    print_semantic_diff_category(stream, "fields", result.fields);
    print_semantic_diff_category(stream, "styles", result.styles);
    print_semantic_diff_category(stream, "numbering", result.numbering);
    print_semantic_diff_category(stream, "footnotes", result.footnotes);
    print_semantic_diff_category(stream, "endnotes", result.endnotes);
    print_semantic_diff_category(stream, "comments", result.comments);
    print_semantic_diff_category(stream, "revisions", result.revisions);
    print_semantic_diff_category(stream, "sections", result.sections);
    print_semantic_diff_category(stream, "template_parts",
                                 result.template_parts);
    print_semantic_diff_changes(stream, "paragraph_change",
                                result.paragraph_changes);
    print_semantic_diff_changes(stream, "table_change", result.table_changes);
    print_semantic_diff_changes(stream, "image_change", result.image_changes);
    print_semantic_diff_changes(stream, "content_control_change",
                                result.content_control_changes);
    print_semantic_diff_changes(stream, "field_change", result.field_changes);
    print_semantic_diff_changes(stream, "style_change", result.style_changes);
    print_semantic_diff_changes(stream, "numbering_change",
                                result.numbering_changes);
    print_semantic_diff_changes(stream, "footnote_change",
                                result.footnote_changes);
    print_semantic_diff_changes(stream, "endnote_change",
                                result.endnote_changes);
    print_semantic_diff_changes(stream, "comment_change",
                                result.comment_changes);
    print_semantic_diff_changes(stream, "revision_change",
                                result.revision_changes);
    print_semantic_diff_changes(stream, "section_change",
                                result.section_changes);
    print_semantic_diff_part_results(stream, result.template_part_results);
}

} // namespace

void output_semantic_diff_result(
    const featherdoc::document_semantic_diff_result &result,
    const semantic_diff_options &options) {
    if (options.json_output) {
        write_json_semantic_diff_result(std::cout, result, options);
        return;
    }

    print_semantic_diff_result(std::cout, result, options);
}

auto run_semantic_diff_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command, "semantic-diff expects left and right input paths",
                          json_output);
        return 2;
    }

    semantic_diff_options options;
    std::string error_message;
    if (!parse_semantic_diff_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Document right_doc;
    if (!open_document(path_type(std::string(arguments[2])), right_doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto result = doc.compare_semantic(right_doc, options.diff_options);
    if (!result.has_value()) {
        report_document_error(command, "compare", doc.last_error(),
                              options.json_output);
        return 1;
    }

    output_semantic_diff_result(*result, options);
    return options.fail_on_diff && result->different() ? 1 : 0;
}

} // namespace featherdoc_cli
