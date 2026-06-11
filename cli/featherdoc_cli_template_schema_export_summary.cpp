#include "featherdoc_cli_template_schema_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_template_schema_output_detail.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <iostream>
#include <optional>

namespace featherdoc_cli {

void print_exported_template_schema_summary(
    const exported_template_schema_result &result,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"export-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"skipped_count\":" << result.skipped_bookmarks.size()
                  << ",\"skipped_bookmarks\":[";
        for (std::size_t index = 0U; index < result.skipped_bookmarks.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_exported_template_schema_skipped_bookmark(
                std::cout, result.skipped_bookmarks[index]);
        }
        std::cout << "]}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "skipped_count: " << result.skipped_bookmarks.size() << '\n';
    if (result.skipped_bookmarks.empty()) {
        std::cout << "skipped_bookmarks: none\n";
        return;
    }

    for (std::size_t index = 0U; index < result.skipped_bookmarks.size(); ++index) {
        const auto &bookmark = result.skipped_bookmarks[index];
        std::cout << "skipped_bookmarks[" << index << "]: part="
                  << validation_part_name(bookmark.part);
        if (bookmark.part_index.has_value()) {
            std::cout << " part_index=" << *bookmark.part_index;
        }
        if (bookmark.section_index.has_value()) {
            std::cout << " section=" << *bookmark.section_index;
        }
        if (bookmark.reference_kind.has_value()) {
            std::cout << " kind="
                      << featherdoc::to_xml_reference_type(*bookmark.reference_kind);
        }
        if (bookmark.resolved_from_section_index.has_value()) {
            std::cout << " resolved_from_section="
                      << *bookmark.resolved_from_section_index;
        }
        if (bookmark.linked_to_previous) {
            std::cout << " linked_to_previous=yes";
        }
        std::cout << " entry=" << bookmark.entry_name
                  << " bookmark=" << bookmark.bookmark.bookmark_name
                  << " kind=" << bookmark_kind_name(bookmark.bookmark.kind)
                  << " occurrences=" << bookmark.bookmark.occurrence_count
                  << " reason=" << bookmark.reason << '\n';
    }
}

void print_normalized_template_schema_summary(
    const exported_template_schema_result &result,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"normalize-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count() << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n';
}

} // namespace featherdoc_cli
