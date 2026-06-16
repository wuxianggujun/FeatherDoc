#include "featherdoc_cli_template_schema_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <iostream>
#include <optional>

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
