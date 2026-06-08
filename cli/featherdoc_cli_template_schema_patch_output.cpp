#include "featherdoc_cli_template_schema_patch_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_template_schema_target_output.hpp"
#include "featherdoc_cli_text.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <ostream>

namespace featherdoc_cli {

namespace {

void write_json_template_schema_patch_target_members(
    std::ostream &stream, const template_schema_patch_target_selector &target) {
    stream << "\"part\":";
    write_json_string(stream, validation_part_name(target.part));
    if (target.part_index.has_value()) {
        stream << ",\"index\":" << *target.part_index;
    }
    if (target.section_index.has_value()) {
        stream << ",\"section\":" << *target.section_index;
    }
    if (target.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*target.reference_kind));
    }
    if (target.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *target.resolved_from_section_index;
    }
    if (target.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
}

void write_json_template_schema_patch_remove_slot(
    std::ostream &stream, const template_schema_patch_remove_slot &operation) {
    stream << '{';
    write_json_template_schema_patch_target_members(stream, operation.target);
    stream << ",\"" << template_slot_source_json_name(operation.source) << "\":";
    write_json_string(stream, operation.bookmark_name);
    stream << '}';
}

void write_json_template_schema_patch_rename_slot(
    std::ostream &stream, const template_schema_patch_rename_slot &operation) {
    stream << '{';
    write_json_template_schema_patch_target_members(stream, operation.target);
    stream << ",\"" << template_slot_source_json_name(operation.source) << "\":";
    write_json_string(stream, operation.bookmark_name);
    stream << ",\"" << template_slot_source_new_json_name(operation.source) << "\":";
    write_json_string(stream, operation.new_bookmark_name);
    stream << '}';
}

void write_json_template_schema_patch_update_slot(
    std::ostream &stream, const template_schema_patch_update_slot &operation) {
    stream << '{';
    write_json_template_schema_patch_target_members(stream, operation.target);
    stream << ",\"" << template_slot_source_json_name(operation.source) << "\":";
    write_json_string(stream, operation.bookmark_name);
    if (operation.update.kind.has_value()) {
        stream << ",\"slot_kind\":";
        write_json_string(stream, template_slot_kind_name(*operation.update.kind));
    }
    if (operation.update.required.has_value()) {
        stream << ",\"required\":" << json_bool(*operation.update.required);
    }
    if (operation.update.min_occurrences.has_value()) {
        stream << ",\"min_occurrences\":" << *operation.update.min_occurrences;
    }
    if (operation.update.max_occurrences.has_value()) {
        stream << ",\"max_occurrences\":" << *operation.update.max_occurrences;
    }
    if (operation.update.clear_min_occurrences) {
        stream << ",\"clear_min_occurrences\":true";
    }
    if (operation.update.clear_max_occurrences) {
        stream << ",\"clear_max_occurrences\":true";
    }
    stream << '}';
}

} // namespace

void write_json_template_schema_patch_selector(
    std::ostream &stream, const template_schema_patch_target_selector &selector) {
    stream << '{';
    write_json_template_schema_patch_target_members(stream, selector);
    stream << '}';
}

void write_json_template_schema_patch_document(
    std::ostream &stream, const template_schema_patch_document &patch) {
    bool wrote_member = false;
    stream << '{';

    const auto write_separator = [&]() {
        if (wrote_member) {
            stream << ',';
        }
        wrote_member = true;
    };

    if (!patch.remove_targets.empty()) {
        write_separator();
        stream << "\"remove_targets\":[";
        for (std::size_t index = 0U; index < patch.remove_targets.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_selector(stream,
                                                      patch.remove_targets[index]);
        }
        stream << ']';
    }

    if (!patch.remove_slots.empty()) {
        write_separator();
        stream << "\"remove_slots\":[";
        for (std::size_t index = 0U; index < patch.remove_slots.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_remove_slot(stream,
                                                         patch.remove_slots[index]);
        }
        stream << ']';
    }

    if (!patch.rename_slots.empty()) {
        write_separator();
        stream << "\"rename_slots\":[";
        for (std::size_t index = 0U; index < patch.rename_slots.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_rename_slot(stream,
                                                         patch.rename_slots[index]);
        }
        stream << ']';
    }

    if (!patch.update_slots.empty()) {
        write_separator();
        stream << "\"update_slots\":[";
        for (std::size_t index = 0U; index < patch.update_slots.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_update_slot(stream,
                                                         patch.update_slots[index]);
        }
        stream << ']';
    }

    if (!patch.upsert_targets.empty()) {
        write_separator();
        stream << "\"upsert_targets\":[";
        for (std::size_t index = 0U; index < patch.upsert_targets.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_exported_template_schema_target(stream,
                                                       patch.upsert_targets[index]);
        }
        stream << ']';
    }

    stream << "}\n";
}

} // namespace featherdoc_cli
