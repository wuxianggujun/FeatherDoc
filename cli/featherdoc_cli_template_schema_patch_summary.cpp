#include "featherdoc_cli_template_schema_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <iostream>
#include <optional>

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
