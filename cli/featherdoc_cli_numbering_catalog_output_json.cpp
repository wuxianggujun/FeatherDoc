#include "featherdoc_cli_numbering_catalog_output_json.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_json.hpp"

#include <cstddef>
#include <ostream>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_numbering_catalog_lint_issue(
    std::ostream &stream, const numbering_catalog_lint_issue &issue) {
    stream << "{\"issue\":";
    write_json_string(stream, numbering_catalog_lint_issue_name(issue.kind));
    stream << ",\"definition_index\":" << issue.definition_index
           << ",\"definition_name\":";
    write_json_string(stream, issue.definition_name);
    if (issue.instance_index.has_value()) {
        stream << ",\"instance_index\":" << *issue.instance_index;
    }
    if (issue.instance_id.has_value()) {
        stream << ",\"instance_id\":" << *issue.instance_id;
    }
    if (issue.level_index.has_value()) {
        stream << ",\"level_index\":" << *issue.level_index;
    }
    if (issue.override_index.has_value()) {
        stream << ",\"override_index\":" << *issue.override_index;
    }
    if (issue.level.has_value()) {
        stream << ",\"level\":" << *issue.level;
    }
    stream << ",\"detail\":";
    write_json_string(stream, issue.detail);
    stream << '}';
}

} // namespace

auto numbering_catalog_instance_count(
    const featherdoc::numbering_catalog &catalog) -> std::size_t {
    std::size_t count = 0U;
    for (const auto &definition : catalog.definitions) {
        count += definition.instances.size();
    }
    return count;
}

void write_json_numbering_catalog_definition(
    std::ostream &stream,
    const featherdoc::numbering_catalog_definition &definition) {
    stream << "{\"name\":";
    write_json_string(stream, definition.definition.name);
    stream << ",\"levels\":[";
    for (std::size_t index = 0; index < definition.definition.levels.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream,
                                              definition.definition.levels[index]);
    }
    stream << "],\"instances\":[";
    for (std::size_t index = 0; index < definition.instances.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_instance_summary(stream, definition.instances[index]);
    }
    stream << "]}";
}

void write_json_numbering_catalog_patch_summary(
    std::ostream &stream, const numbering_catalog_patch_summary &summary) {
    stream << "\"upserted_level_count\":" << summary.upserted_level_count
           << ",\"upserted_override_count\":" << summary.upserted_override_count
           << ",\"removed_override_count\":" << summary.removed_override_count
           << ",\"missing_override_count\":" << summary.missing_override_count;
}

void write_json_numbering_catalog_lint_issues(
    std::ostream &stream,
    const std::vector<numbering_catalog_lint_issue> &issues) {
    for (std::size_t index = 0U; index < issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_lint_issue(stream, issues[index]);
    }
}

} // namespace featherdoc_cli
