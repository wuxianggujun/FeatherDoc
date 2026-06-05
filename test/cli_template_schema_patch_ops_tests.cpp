#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_template_schema_ops.hpp"
#include "featherdoc_cli_template_schema_patch_ops.hpp"

#include <string>
#include <utility>

namespace {

auto make_slot(std::string bookmark_name,
               featherdoc::template_slot_kind kind =
                   featherdoc::template_slot_kind::text,
               featherdoc::template_slot_source_kind source =
                   featherdoc::template_slot_source_kind::bookmark)
    -> featherdoc::template_slot_requirement {
    featherdoc::template_slot_requirement requirement;
    requirement.bookmark_name = std::move(bookmark_name);
    requirement.source = source;
    requirement.kind = kind;
    requirement.required = true;
    return requirement;
}

auto make_target(featherdoc_cli::validation_part_family part)
    -> featherdoc_cli::exported_template_schema_target {
    featherdoc_cli::exported_template_schema_target target;
    target.part = part;
    return target;
}

auto make_selector(featherdoc_cli::validation_part_family part)
    -> featherdoc_cli::template_schema_patch_target_selector {
    featherdoc_cli::template_schema_patch_target_selector selector;
    selector.part = part;
    return selector;
}

} // namespace

TEST_CASE("cli template schema patch ops normalizes targets and slots") {
    featherdoc_cli::exported_template_schema_result result;

    auto footer = make_target(featherdoc_cli::validation_part_family::footer);
    footer.entry_name = "word/footer1.xml";
    footer.requirements.push_back(make_slot("zeta"));
    result.targets.push_back(footer);

    auto body = make_target(featherdoc_cli::validation_part_family::body);
    body.entry_name = "word/document.xml";
    body.requirements.push_back(make_slot("beta"));
    body.requirements.push_back(make_slot("alpha"));
    result.targets.push_back(body);

    featherdoc_cli::normalize_template_schema_result(result);

    REQUIRE(result.targets.size() == 2U);
    CHECK(result.targets[0].part == featherdoc_cli::validation_part_family::body);
    CHECK(result.targets[0].entry_name.empty());
    REQUIRE(result.targets[0].requirements.size() == 2U);
    CHECK(result.targets[0].requirements[0].bookmark_name == "alpha");
    CHECK(result.targets[0].requirements[1].bookmark_name == "beta");
    CHECK(result.targets[1].part == featherdoc_cli::validation_part_family::footer);
    CHECK(result.targets[1].entry_name.empty());
}

TEST_CASE("cli template schema patch ops merges duplicate targets") {
    featherdoc_cli::exported_template_schema_result base;
    auto body = make_target(featherdoc_cli::validation_part_family::body);
    body.requirements.push_back(make_slot("customer"));
    base.targets.push_back(body);

    featherdoc_cli::exported_template_schema_result overlay;
    auto overlay_body = make_target(featherdoc_cli::validation_part_family::body);
    auto replacement = make_slot("customer", featherdoc::template_slot_kind::image);
    replacement.required = false;
    overlay_body.requirements.push_back(replacement);
    overlay_body.requirements.push_back(make_slot("invoice_no"));
    overlay.targets.push_back(overlay_body);

    featherdoc_cli::merged_template_schema_summary summary;
    featherdoc_cli::merge_template_schema_result(base, overlay, summary);

    CHECK(summary.input_count == 1U);
    CHECK(summary.updated_target_count == 1U);
    CHECK(summary.replaced_slot_count == 1U);
    REQUIRE(base.targets.size() == 1U);
    REQUIRE(base.targets[0].requirements.size() == 2U);
    CHECK(base.targets[0].requirements[0].kind ==
          featherdoc::template_slot_kind::image);
    CHECK_FALSE(base.targets[0].requirements[0].required);
    CHECK(base.targets[0].requirements[1].bookmark_name == "invoice_no");
}

TEST_CASE("cli template schema patch ops applies patch operations") {
    featherdoc_cli::exported_template_schema_result result;
    auto body = make_target(featherdoc_cli::validation_part_family::body);
    body.requirements.push_back(make_slot("customer"));
    body.requirements.push_back(make_slot("obsolete"));
    result.targets.push_back(body);

    auto footer = make_target(featherdoc_cli::validation_part_family::footer);
    footer.requirements.push_back(make_slot("footer_only"));
    result.targets.push_back(footer);

    featherdoc_cli::template_schema_patch_document patch;
    featherdoc_cli::template_schema_patch_remove_slot remove_slot;
    remove_slot.target = make_selector(featherdoc_cli::validation_part_family::footer);
    remove_slot.bookmark_name = "footer_only";
    patch.remove_slots.push_back(remove_slot);

    featherdoc_cli::template_schema_patch_rename_slot rename_slot;
    rename_slot.target = make_selector(featherdoc_cli::validation_part_family::body);
    rename_slot.bookmark_name = "customer";
    rename_slot.new_bookmark_name = "customer_name";
    patch.rename_slots.push_back(rename_slot);

    featherdoc_cli::template_schema_patch_update_slot update_slot;
    update_slot.target = make_selector(featherdoc_cli::validation_part_family::body);
    update_slot.bookmark_name = "customer_name";
    update_slot.update.required = false;
    patch.update_slots.push_back(update_slot);

    auto header = make_target(featherdoc_cli::validation_part_family::header);
    header.requirements.push_back(make_slot("header_slot"));
    patch.upsert_targets.push_back(header);

    featherdoc_cli::patched_template_schema_summary summary;
    featherdoc_cli::apply_template_schema_patch(result, patch, summary);

    CHECK(summary.upsert_target_count == 1U);
    CHECK(summary.remove_slot_count == 1U);
    CHECK(summary.rename_slot_count == 1U);
    CHECK(summary.update_slot_count == 1U);
    CHECK(summary.applied_remove_slot_count == 1U);
    CHECK(summary.applied_rename_slot_count == 1U);
    CHECK(summary.applied_update_slot_count == 1U);
    CHECK(summary.pruned_empty_target_count == 1U);
    REQUIRE(result.targets.size() == 2U);
    CHECK(result.targets[0].requirements[0].bookmark_name == "customer_name");
    CHECK_FALSE(result.targets[0].requirements[0].required);
    CHECK(result.targets[1].part == featherdoc_cli::validation_part_family::header);
}

TEST_CASE("cli template schema patch ops repairs duplicate schemas") {
    featherdoc_cli::exported_template_schema_result input;
    auto body = make_target(featherdoc_cli::validation_part_family::body);
    body.entry_name = "word/document.xml";
    body.requirements.push_back(make_slot("zeta"));
    body.requirements.push_back(make_slot("alpha"));
    body.requirements.push_back(make_slot("alpha"));
    input.targets.push_back(body);

    auto duplicate_body = make_target(featherdoc_cli::validation_part_family::body);
    duplicate_body.requirements.push_back(make_slot("beta"));
    input.targets.push_back(duplicate_body);

    featherdoc_cli::exported_template_schema_result repaired;
    featherdoc_cli::repaired_template_schema_summary summary;
    featherdoc_cli::repair_template_schema_result(input, repaired, summary);

    CHECK(summary.input_target_count == 2U);
    CHECK(summary.input_slot_count == 4U);
    CHECK(summary.stripped_entry_name_count == 1U);
    CHECK(summary.merged_duplicate_target_count == 1U);
    CHECK(summary.deduplicated_target_count == 1U);
    CHECK(summary.deduplicated_slot_count == 1U);
    CHECK(summary.changed);
    REQUIRE(repaired.targets.size() == 1U);
    REQUIRE(repaired.targets[0].requirements.size() == 3U);
    CHECK(repaired.targets[0].requirements[0].bookmark_name == "alpha");
    CHECK(repaired.targets[0].requirements[1].bookmark_name == "beta");
    CHECK(repaired.targets[0].requirements[2].bookmark_name == "zeta");
}

TEST_CASE("cli template schema patch ops builds patch from diffs") {
    featherdoc_cli::exported_template_schema_result left;
    auto left_body = make_target(featherdoc_cli::validation_part_family::body);
    left_body.requirements.push_back(make_slot("changed"));
    left_body.requirements.push_back(make_slot("keep"));
    left_body.requirements.push_back(make_slot("old_name"));
    left_body.requirements.push_back(
        make_slot("removed", featherdoc::template_slot_kind::image));
    left.targets.push_back(left_body);

    featherdoc_cli::exported_template_schema_result right;
    auto right_body = make_target(featherdoc_cli::validation_part_family::body);
    right_body.requirements.push_back(
        make_slot("added_tag", featherdoc::template_slot_kind::text,
                  featherdoc::template_slot_source_kind::content_control_tag));
    right_body.requirements.push_back(
        make_slot("changed", featherdoc::template_slot_kind::image));
    right_body.requirements.push_back(make_slot("keep"));
    right_body.requirements.push_back(make_slot("new_name"));
    right.targets.push_back(right_body);

    featherdoc_cli::normalize_template_schema_result(left);
    featherdoc_cli::normalize_template_schema_result(right);

    const auto diff = featherdoc_cli::diff_template_schema_results(left, right);
    REQUIRE(diff.changed_targets.size() == 1U);

    featherdoc_cli::built_template_schema_patch_summary summary;
    const auto patch =
        featherdoc_cli::build_template_schema_patch_document(diff, summary);

    CHECK(summary.changed_target_count == 1U);
    CHECK(summary.generated_remove_slot_count == 1U);
    CHECK(summary.generated_rename_slot_count == 1U);
    CHECK(summary.generated_update_slot_count == 1U);
    CHECK(summary.generated_upsert_target_count == 1U);
    CHECK_FALSE(summary.empty_patch());
    REQUIRE(patch.remove_slots.size() == 1U);
    CHECK(patch.remove_slots[0].bookmark_name == "removed");
    REQUIRE(patch.rename_slots.size() == 1U);
    CHECK(patch.rename_slots[0].bookmark_name == "old_name");
    CHECK(patch.rename_slots[0].new_bookmark_name == "new_name");
    REQUIRE(patch.update_slots.size() == 1U);
    CHECK(patch.update_slots[0].bookmark_name == "changed");
    REQUIRE(patch.upsert_targets.size() == 1U);
    REQUIRE(patch.upsert_targets[0].requirements.size() == 1U);
    CHECK(patch.upsert_targets[0].requirements[0].bookmark_name == "added_tag");

    auto patched = left;
    featherdoc_cli::patched_template_schema_summary apply_summary;
    featherdoc_cli::apply_template_schema_patch(patched, patch, apply_summary);
    featherdoc_cli::normalize_template_schema_result(patched);
    const auto remaining_diff =
        featherdoc_cli::diff_template_schema_results(patched, right);
    CHECK(remaining_diff.equal());
}

TEST_CASE("cli template schema patch ops detects resolved metadata") {
    featherdoc_cli::exported_template_schema_result result;
    auto body = make_target(featherdoc_cli::validation_part_family::body);
    body.linked_to_previous = true;
    result.targets.push_back(body);
    CHECK(featherdoc_cli::has_template_schema_resolved_target_metadata(result));

    featherdoc_cli::template_schema_patch_document patch;
    featherdoc_cli::template_schema_patch_update_slot update_slot;
    update_slot.target = make_selector(featherdoc_cli::validation_part_family::body);
    update_slot.target.resolved_from_section_index = 2U;
    patch.update_slots.push_back(update_slot);
    CHECK(featherdoc_cli::has_template_schema_resolved_target_metadata(patch));
}
