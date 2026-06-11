#include "featherdoc_cli_template_schema_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_template_schema_ops.hpp"
#include "featherdoc_cli_template_schema_output_detail.hpp"
#include "featherdoc_cli_template_schema_patch_output.hpp"
#include "featherdoc_cli_template_schema_target_output.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <ostream>

namespace featherdoc_cli {

void write_json_exported_template_schema(
    std::ostream &stream, const exported_template_schema_result &result) {
    stream << "{\"targets\":[";
    for (std::size_t target_index = 0U; target_index < result.targets.size();
         ++target_index) {
        if (target_index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_target(
            stream, result.targets[target_index]);
    }
    stream << "]}\n";
}

void write_json_template_schema_lint_issue(
    std::ostream &stream, const template_schema_lint_issue &issue) {
    stream << "{\"issue\":";
    write_json_string(stream, template_schema_lint_issue_name(issue.kind));
    stream << ",\"target_index\":" << issue.target_index << ",\"target\":";
    write_json_template_schema_patch_selector(stream, issue.target);
    if (issue.previous_target_index.has_value()) {
        stream << ",\"previous_target_index\":" << *issue.previous_target_index;
    }
    if (issue.slot_index.has_value()) {
        stream << ",\"slot_index\":" << *issue.slot_index;
    }
    if (issue.previous_slot_index.has_value()) {
        stream << ",\"previous_slot_index\":" << *issue.previous_slot_index;
    }
    if (!issue.bookmark_name.empty()) {
        stream << ",\"bookmark\":";
        write_json_string(stream, issue.bookmark_name);
    }
    if (!issue.previous_bookmark_name.empty()) {
        stream << ",\"previous_bookmark\":";
        write_json_string(stream, issue.previous_bookmark_name);
    }
    if (!issue.entry_name.empty()) {
        stream << ",\"entry_name\":";
        write_json_string(stream, issue.entry_name);
    }
    stream << '}';
}

void write_json_exported_template_schema_skipped_bookmark(
    std::ostream &stream,
    const exported_template_schema_skipped_bookmark &bookmark) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(bookmark.part));
    if (bookmark.part_index.has_value()) {
        stream << ",\"part_index\":" << *bookmark.part_index;
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, bookmark.entry_name);
    if (bookmark.section_index.has_value()) {
        stream << ",\"section\":" << *bookmark.section_index;
    }
    if (bookmark.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*bookmark.reference_kind));
    }
    if (bookmark.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *bookmark.resolved_from_section_index;
    }
    if (bookmark.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"bookmark_name\":";
    write_json_string(stream, bookmark.bookmark.bookmark_name);
    stream << ",\"kind\":";
    write_json_string(stream, bookmark_kind_name(bookmark.bookmark.kind));
    stream << ",\"occurrence_count\":" << bookmark.bookmark.occurrence_count
           << ",\"reason\":";
    write_json_string(stream, bookmark.reason);
    stream << '}';
}

} // namespace featherdoc_cli
