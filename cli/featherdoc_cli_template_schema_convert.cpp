#include "featherdoc_cli_template_schema_convert.hpp"

#include <utility>

namespace featherdoc_cli {
namespace {

auto to_template_schema_part_selector(
    const template_schema_patch_target_selector &selector)
    -> featherdoc::template_schema_part_selector {
    featherdoc::template_schema_part_selector target{};
    target.part = to_template_schema_part_kind(selector.part);
    target.part_index = selector.part_index;
    target.section_index = selector.section_index;
    target.reference_kind = selector.reference_kind.value_or(
        featherdoc::section_reference_kind::default_reference);
    return target;
}

auto to_template_schema_part_selector(const exported_template_schema_target &target)
    -> featherdoc::template_schema_part_selector {
    featherdoc::template_schema_part_selector selector{};
    selector.part = to_template_schema_part_kind(target.part);
    selector.part_index = target.part_index;
    selector.section_index = target.section_index;
    selector.reference_kind = target.reference_kind.value_or(
        featherdoc::section_reference_kind::default_reference);
    return selector;
}

} // namespace

auto to_template_schema_part_kind(validation_part_family family)
    -> featherdoc::template_schema_part_kind {
    switch (family) {
    case validation_part_family::body:
        return featherdoc::template_schema_part_kind::body;
    case validation_part_family::header:
        return featherdoc::template_schema_part_kind::header;
    case validation_part_family::footer:
        return featherdoc::template_schema_part_kind::footer;
    case validation_part_family::section_header:
        return featherdoc::template_schema_part_kind::section_header;
    case validation_part_family::section_footer:
        return featherdoc::template_schema_part_kind::section_footer;
    }

    return featherdoc::template_schema_part_kind::body;
}

auto to_template_schema(const exported_template_schema_result &result)
    -> featherdoc::template_schema {
    featherdoc::template_schema schema{};
    schema.entries.reserve(result.slot_count());
    for (const auto &target : result.targets) {
        const auto selector = to_template_schema_part_selector(target);
        for (const auto &requirement : target.requirements) {
            schema.entries.push_back({selector, requirement});
        }
    }
    return schema;
}

auto to_template_schema_patch(const template_schema_patch_document &document)
    -> featherdoc::template_schema_patch {
    featherdoc::template_schema_patch patch{};

    for (const auto &target : document.upsert_targets) {
        const auto selector = to_template_schema_part_selector(target);
        for (const auto &requirement : target.requirements) {
            patch.upsert_slots.push_back({selector, requirement});
        }
    }

    patch.remove_targets.reserve(document.remove_targets.size());
    for (const auto &target : document.remove_targets) {
        patch.remove_targets.push_back(to_template_schema_part_selector(target));
    }

    patch.remove_slots.reserve(document.remove_slots.size());
    for (const auto &remove_slot : document.remove_slots) {
        featherdoc::template_schema_slot_selector slot{};
        slot.target = to_template_schema_part_selector(remove_slot.target);
        slot.source = remove_slot.source;
        slot.bookmark_name = remove_slot.bookmark_name;
        patch.remove_slots.push_back(std::move(slot));
    }

    patch.rename_slots.reserve(document.rename_slots.size());
    for (const auto &rename_slot : document.rename_slots) {
        featherdoc::template_schema_slot_rename operation{};
        operation.slot.target = to_template_schema_part_selector(rename_slot.target);
        operation.slot.source = rename_slot.source;
        operation.slot.bookmark_name = rename_slot.bookmark_name;
        operation.new_bookmark_name = rename_slot.new_bookmark_name;
        patch.rename_slots.push_back(std::move(operation));
    }

    patch.update_slots.reserve(document.update_slots.size());
    for (const auto &update_slot : document.update_slots) {
        featherdoc::template_schema_slot_patch_update operation{};
        operation.slot.target = to_template_schema_part_selector(update_slot.target);
        operation.slot.source = update_slot.source;
        operation.slot.bookmark_name = update_slot.bookmark_name;
        operation.update = update_slot.update;
        patch.update_slots.push_back(std::move(operation));
    }

    return patch;
}

} // namespace featherdoc_cli
