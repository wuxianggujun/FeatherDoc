#include "featherdoc_cli_template_schema_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_template_schema_ops.hpp"
#include "featherdoc_cli_template_schema_patch_output.hpp"
#include "featherdoc_cli_template_schema_target_output.hpp"
#include "featherdoc_cli_text.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <fstream>
#include <iostream>
#include <ostream>

namespace featherdoc_cli {

void write_json_exported_template_schema(std::ostream &stream,
                                         const exported_template_schema_result &result) {
    stream << "{\"targets\":[";
    for (std::size_t target_index = 0U; target_index < result.targets.size();
         ++target_index) {
        if (target_index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_target(stream, result.targets[target_index]);
    }
    stream << "]}\n";
}

void write_json_template_schema_lint_issue(std::ostream &stream,
                                           const template_schema_lint_issue &issue) {
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

void print_template_schema_patch_selector_summary(
    std::ostream &stream, const template_schema_patch_target_selector &selector) {
    stream << "part=" << validation_part_name(selector.part);
    if (selector.part_index.has_value()) {
        stream << " index=" << *selector.part_index;
    }
    if (selector.section_index.has_value()) {
        stream << " section=" << *selector.section_index;
    }
    if (selector.reference_kind.has_value()) {
        stream << " kind="
               << featherdoc::to_xml_reference_type(*selector.reference_kind);
    }
    if (selector.resolved_from_section_index.has_value()) {
        stream << " resolved_from_section="
               << *selector.resolved_from_section_index;
    }
    if (selector.linked_to_previous) {
        stream << " linked_to_previous=yes";
    }
}

void print_linted_template_schema_result(const template_schema_lint_result &result,
                                         bool json_output) {
    const auto duplicate_target_count =
        result.issue_count(template_schema_lint_issue_kind::duplicate_target_identity);
    const auto duplicate_slot_count =
        result.issue_count(template_schema_lint_issue_kind::duplicate_slot_name);
    const auto target_order_issue_count =
        result.issue_count(template_schema_lint_issue_kind::target_order);
    const auto slot_order_issue_count =
        result.issue_count(template_schema_lint_issue_kind::slot_order);
    const auto entry_name_issue_count =
        result.issue_count(template_schema_lint_issue_kind::entry_name_present);

    if (json_output) {
        std::cout << "{\"command\":\"lint-template-schema\",\"ok\":true,"
                  << "\"clean\":" << json_bool(result.clean())
                  << ",\"target_count\":" << result.target_count
                  << ",\"slot_count\":" << result.slot_count
                  << ",\"issue_count\":" << result.issues.size()
                  << ",\"duplicate_target_count\":" << duplicate_target_count
                  << ",\"duplicate_slot_count\":" << duplicate_slot_count
                  << ",\"target_order_issue_count\":" << target_order_issue_count
                  << ",\"slot_order_issue_count\":" << slot_order_issue_count
                  << ",\"entry_name_issue_count\":" << entry_name_issue_count
                  << ",\"issues\":[";
        for (std::size_t index = 0U; index < result.issues.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_template_schema_lint_issue(std::cout, result.issues[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "clean: " << yes_no(result.clean()) << '\n'
              << "target_count: " << result.target_count << '\n'
              << "slot_count: " << result.slot_count << '\n'
              << "issue_count: " << result.issues.size() << '\n'
              << "duplicate_target_count: " << duplicate_target_count << '\n'
              << "duplicate_slot_count: " << duplicate_slot_count << '\n'
              << "target_order_issue_count: " << target_order_issue_count << '\n'
              << "slot_order_issue_count: " << slot_order_issue_count << '\n'
              << "entry_name_issue_count: " << entry_name_issue_count << '\n';
    if (result.issues.empty()) {
        std::cout << "issues: none\n";
        return;
    }

    for (std::size_t index = 0U; index < result.issues.size(); ++index) {
        const auto &issue = result.issues[index];
        std::cout << "issue[" << index << "]: issue="
                  << template_schema_lint_issue_name(issue.kind)
                  << " target_index=" << issue.target_index << ' ';
        print_template_schema_patch_selector_summary(std::cout, issue.target);
        if (issue.previous_target_index.has_value()) {
            std::cout << " previous_target_index=" << *issue.previous_target_index;
        }
        if (issue.slot_index.has_value()) {
            std::cout << " slot_index=" << *issue.slot_index;
        }
        if (issue.previous_slot_index.has_value()) {
            std::cout << " previous_slot_index=" << *issue.previous_slot_index;
        }
        if (!issue.bookmark_name.empty()) {
            std::cout << " bookmark=" << issue.bookmark_name;
        }
        if (!issue.previous_bookmark_name.empty()) {
            std::cout << " previous_bookmark=" << issue.previous_bookmark_name;
        }
        if (!issue.entry_name.empty()) {
            std::cout << " entry_name=" << issue.entry_name;
        }
        std::cout << '\n';
    }
}

void print_repaired_template_schema_summary(
    const exported_template_schema_result &result,
    const repaired_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"repair-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"input_target_count\":" << summary.input_target_count
                  << ",\"input_slot_count\":" << summary.input_slot_count
                  << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"merged_duplicate_target_count\":"
                  << summary.merged_duplicate_target_count
                  << ",\"deduplicated_target_count\":"
                  << summary.deduplicated_target_count
                  << ",\"deduplicated_slot_count\":"
                  << summary.deduplicated_slot_count
                  << ",\"stripped_entry_name_count\":"
                  << summary.stripped_entry_name_count
                  << ",\"replaced_slot_count\":"
                  << summary.replaced_slot_count
                  << ",\"changed\":" << json_bool(summary.changed) << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "input_target_count: " << summary.input_target_count << '\n'
              << "input_slot_count: " << summary.input_slot_count << '\n'
              << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "merged_duplicate_target_count: "
              << summary.merged_duplicate_target_count << '\n'
              << "deduplicated_target_count: "
              << summary.deduplicated_target_count << '\n'
              << "deduplicated_slot_count: "
              << summary.deduplicated_slot_count << '\n'
              << "stripped_entry_name_count: "
              << summary.stripped_entry_name_count << '\n'
              << "replaced_slot_count: " << summary.replaced_slot_count << '\n'
              << "changed: " << yes_no(summary.changed) << '\n';
}

void print_merged_template_schema_summary(
    const exported_template_schema_result &result,
    const merged_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"merge-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"input_count\":" << summary.input_count
                  << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"updated_target_count\":" << summary.updated_target_count
                  << ",\"replaced_slot_count\":" << summary.replaced_slot_count
                  << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "input_count: " << summary.input_count << '\n'
              << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "updated_target_count: " << summary.updated_target_count << '\n'
              << "replaced_slot_count: " << summary.replaced_slot_count << '\n';
}

void print_patched_template_schema_summary(
    const exported_template_schema_result &result,
    const patched_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"patch-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"upsert_target_count\":" << summary.upsert_target_count
                  << ",\"remove_target_count\":" << summary.remove_target_count
                  << ",\"remove_slot_count\":" << summary.remove_slot_count
                  << ",\"rename_slot_count\":" << summary.rename_slot_count
                  << ",\"update_slot_count\":" << summary.update_slot_count
                  << ",\"updated_target_count\":" << summary.updated_target_count
                  << ",\"replaced_slot_count\":" << summary.replaced_slot_count
                  << ",\"applied_remove_target_count\":"
                  << summary.applied_remove_target_count
                  << ",\"applied_remove_slot_count\":"
                  << summary.applied_remove_slot_count
                  << ",\"applied_rename_slot_count\":"
                  << summary.applied_rename_slot_count
                  << ",\"applied_update_slot_count\":"
                  << summary.applied_update_slot_count
                  << ",\"pruned_empty_target_count\":"
                  << summary.pruned_empty_target_count << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "upsert_target_count: " << summary.upsert_target_count << '\n'
              << "remove_target_count: " << summary.remove_target_count << '\n'
              << "remove_slot_count: " << summary.remove_slot_count << '\n'
              << "rename_slot_count: " << summary.rename_slot_count << '\n'
              << "update_slot_count: " << summary.update_slot_count << '\n'
              << "updated_target_count: " << summary.updated_target_count << '\n'
              << "replaced_slot_count: " << summary.replaced_slot_count << '\n'
              << "applied_remove_target_count: "
              << summary.applied_remove_target_count << '\n'
              << "applied_remove_slot_count: "
              << summary.applied_remove_slot_count << '\n'
              << "applied_rename_slot_count: "
              << summary.applied_rename_slot_count << '\n'
              << "applied_update_slot_count: "
              << summary.applied_update_slot_count << '\n'
              << "pruned_empty_target_count: "
              << summary.pruned_empty_target_count << '\n';
}

void print_previewed_template_schema_patch_summary(
    const previewed_template_schema_patch_summary &summary, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"preview-template-schema-patch\",\"ok\":true";
        if (summary.output_patch_path.has_value()) {
            std::cout << ",\"output_patch_path\":";
            write_json_string(std::cout, summary.output_patch_path->string());
        }
        if (summary.review_json_path.has_value()) {
            std::cout << ",\"review_json_path\":";
            write_json_string(std::cout, summary.review_json_path->string());
        }
        std::cout << ",\"left_slot_count\":" << summary.left_slot_count;
        if (summary.right_slot_count.has_value()) {
            std::cout << ",\"right_slot_count\":" << *summary.right_slot_count;
        }
        if (summary.upsert_slot_count.has_value()) {
            std::cout << ",\"upsert_slot_count\":" << *summary.upsert_slot_count;
        }
        if (summary.remove_target_count.has_value()) {
            std::cout << ",\"remove_target_count\":"
                      << *summary.remove_target_count;
        }
        if (summary.remove_slot_count.has_value()) {
            std::cout << ",\"remove_slot_count\":" << *summary.remove_slot_count;
        }
        if (summary.rename_slot_count.has_value()) {
            std::cout << ",\"rename_slot_count\":" << *summary.rename_slot_count;
        }
        if (summary.update_slot_count.has_value()) {
            std::cout << ",\"update_slot_count\":" << *summary.update_slot_count;
        }
        std::cout << ",\"removed_targets\":" << summary.applied.removed_targets
                  << ",\"removed_slots\":" << summary.applied.removed_slots
                  << ",\"renamed_slots\":" << summary.applied.renamed_slots
                  << ",\"inserted_slots\":" << summary.applied.inserted_slots
                  << ",\"replaced_slots\":" << summary.applied.replaced_slots
                  << ",\"changed\":" << json_bool(summary.applied.changed())
                  << "}\n";
        return;
    }

    if (summary.output_patch_path.has_value()) {
        std::cout << "output_patch_path: " << summary.output_patch_path->string()
                  << '\n';
    }
    if (summary.review_json_path.has_value()) {
        std::cout << "review_json_path: " << summary.review_json_path->string()
                  << '\n';
    }
    std::cout << "left_slot_count: " << summary.left_slot_count << '\n';
    if (summary.right_slot_count.has_value()) {
        std::cout << "right_slot_count: " << *summary.right_slot_count << '\n';
    }
    if (summary.upsert_slot_count.has_value()) {
        std::cout << "upsert_slot_count: " << *summary.upsert_slot_count << '\n';
    }
    if (summary.remove_target_count.has_value()) {
        std::cout << "remove_target_count: " << *summary.remove_target_count << '\n';
    }
    if (summary.remove_slot_count.has_value()) {
        std::cout << "remove_slot_count: " << *summary.remove_slot_count << '\n';
    }
    if (summary.rename_slot_count.has_value()) {
        std::cout << "rename_slot_count: " << *summary.rename_slot_count << '\n';
    }
    if (summary.update_slot_count.has_value()) {
        std::cout << "update_slot_count: " << *summary.update_slot_count << '\n';
    }
    std::cout << "removed_targets: " << summary.applied.removed_targets << '\n'
              << "removed_slots: " << summary.applied.removed_slots << '\n'
              << "renamed_slots: " << summary.applied.renamed_slots << '\n'
              << "inserted_slots: " << summary.applied.inserted_slots << '\n'
              << "replaced_slots: " << summary.applied.replaced_slots << '\n'
              << "changed: " << yes_no(summary.applied.changed()) << '\n';
}

void print_built_template_schema_patch_summary(
    const built_template_schema_patch_summary &summary,
    const std::optional<path_type> &output_path,
    const std::optional<path_type> &review_json_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"build-template-schema-patch\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        if (review_json_path.has_value()) {
            std::cout << ",\"review_json_path\":";
            write_json_string(std::cout, review_json_path->string());
        }
        std::cout << ",\"added_target_count\":" << summary.added_target_count
                  << ",\"removed_target_count\":" << summary.removed_target_count
                  << ",\"changed_target_count\":" << summary.changed_target_count
                  << ",\"generated_remove_target_count\":"
                  << summary.generated_remove_target_count
                  << ",\"generated_remove_slot_count\":"
                  << summary.generated_remove_slot_count
                  << ",\"generated_rename_slot_count\":"
                  << summary.generated_rename_slot_count
                  << ",\"generated_update_slot_count\":"
                  << summary.generated_update_slot_count
                  << ",\"generated_upsert_target_count\":"
                  << summary.generated_upsert_target_count
                  << ",\"empty_patch\":" << json_bool(summary.empty_patch()) << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    if (review_json_path.has_value()) {
        std::cout << "review_json_path: " << review_json_path->string() << '\n';
    }
    std::cout << "added_target_count: " << summary.added_target_count << '\n'
              << "removed_target_count: " << summary.removed_target_count << '\n'
              << "changed_target_count: " << summary.changed_target_count << '\n'
              << "generated_remove_target_count: "
              << summary.generated_remove_target_count << '\n'
              << "generated_remove_slot_count: "
              << summary.generated_remove_slot_count << '\n'
              << "generated_rename_slot_count: "
              << summary.generated_rename_slot_count << '\n'
              << "generated_update_slot_count: "
              << summary.generated_update_slot_count << '\n'
              << "generated_upsert_target_count: "
              << summary.generated_upsert_target_count << '\n'
              << "empty_patch: " << yes_no(summary.empty_patch()) << '\n';
}

auto write_exported_template_schema_file(
    const path_type &output_path, const exported_template_schema_result &result,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open schema output path: " + output_path.string();
        return false;
    }

    write_json_exported_template_schema(stream, result);
    if (!stream.good()) {
        error_message = "failed to write schema output path: " + output_path.string();
        return false;
    }

    return true;
}

auto write_template_schema_patch_file(const path_type &output_path,
                                      const template_schema_patch_document &patch,
                                      std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open patch output path: " + output_path.string();
        return false;
    }

    write_json_template_schema_patch_document(stream, patch);
    if (!stream.good()) {
        error_message = "failed to write patch output path: " + output_path.string();
        return false;
    }

    return true;
}

auto write_template_schema_patch_review_file(
    const path_type &output_path,
    const featherdoc::template_schema_patch_review_summary &summary,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message =
            "failed to open schema patch review output path: " + output_path.string();
        return false;
    }

    featherdoc::write_template_schema_patch_review_json(stream, summary);
    stream << '\n';
    if (!stream.good()) {
        error_message =
            "failed to write schema patch review output path: " + output_path.string();
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
