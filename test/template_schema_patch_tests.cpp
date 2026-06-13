#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("template schema helpers normalize merge and patch in-memory schemas") {
    featherdoc::template_schema schema{{
        {{featherdoc::template_schema_part_kind::header},
         {"header_title", featherdoc::template_slot_kind::text, true}},
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
        {{}, {"customer", featherdoc::template_slot_kind::block, false}},
    }};

    const auto normalize_summary = featherdoc::normalize_template_schema(schema);
    CHECK_EQ(normalize_summary.original_slots, 3U);
    CHECK_EQ(normalize_summary.final_slots, 2U);
    CHECK_EQ(normalize_summary.duplicate_slots_removed, 1U);
    CHECK(normalize_summary.changed());
    REQUIRE(schema.entries.size() == 2U);
    CHECK_EQ(schema.entries[0].requirement.bookmark_name, "customer");
    CHECK_EQ(schema.entries[0].requirement.kind,
             featherdoc::template_slot_kind::block);
    CHECK_FALSE(schema.entries[0].requirement.required);
    CHECK_EQ(schema.entries[1].target.part,
             featherdoc::template_schema_part_kind::header);
    REQUIRE(schema.entries[1].target.part_index.has_value());
    CHECK_EQ(*schema.entries[1].target.part_index, 0U);

    featherdoc::template_schema overlay{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
        {{}, {"line_items", featherdoc::template_slot_kind::table_rows, true}},
    }};

    const auto merge_summary = featherdoc::merge_template_schema(schema, overlay);
    CHECK_EQ(merge_summary.inserted_slots, 1U);
    CHECK_EQ(merge_summary.replaced_slots, 1U);
    REQUIRE(schema.entries.size() == 3U);
    CHECK_EQ(schema.entries[0].requirement.bookmark_name, "customer");
    CHECK_EQ(schema.entries[0].requirement.kind,
             featherdoc::template_slot_kind::text);

    featherdoc::template_schema_patch patch{};
    patch.rename_slots.push_back({{{}, "customer"}, "customer_name"});
    patch.remove_slots.push_back({{}, "line_items"});
    patch.upsert_slots.push_back(
        {{}, {"invoice_total", featherdoc::template_slot_kind::text, false}});

    const auto patch_summary = featherdoc::apply_template_schema_patch(schema, patch);
    CHECK_EQ(patch_summary.renamed_slots, 1U);
    CHECK_EQ(patch_summary.removed_slots, 1U);
    CHECK_EQ(patch_summary.inserted_slots, 1U);
    CHECK(patch_summary.changed());
    REQUIRE(schema.entries.size() == 3U);
    CHECK_EQ(schema.entries[0].requirement.bookmark_name, "customer_name");
    CHECK_EQ(schema.entries[1].requirement.bookmark_name, "invoice_total");
    CHECK_EQ(schema.entries[2].requirement.bookmark_name, "header_title");
}

TEST_CASE("template schema patch builder creates replayable slot-level changes") {
    featherdoc::template_schema left{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
        {{}, {"old_optional", featherdoc::template_slot_kind::text, false}},
        {{}, {"stale", featherdoc::template_slot_kind::block, true}},
    }};

    featherdoc::template_schema right{{
        {{}, {"customer", featherdoc::template_slot_kind::block, true}},
        {{}, {"new_optional", featherdoc::template_slot_kind::text, false, 2U, 2U}},
        {{}, {"line_items", featherdoc::template_slot_kind::table_rows, true}},
    }};

    const auto patch = featherdoc::build_template_schema_patch(left, right);
    CHECK_EQ(patch.rename_slots.size(), 2U);
    REQUIRE(patch.rename_slots.size() == 2U);
    CHECK_EQ(patch.rename_slots[0].slot.bookmark_name, "old_optional");
    CHECK_EQ(patch.rename_slots[0].new_bookmark_name, "new_optional");
    CHECK_EQ(patch.rename_slots[1].slot.bookmark_name, "stale");
    CHECK_EQ(patch.rename_slots[1].new_bookmark_name, "line_items");
    CHECK(patch.remove_slots.empty());
    CHECK_EQ(patch.update_slots.size(), 3U);
    REQUIRE(patch.update_slots.size() == 3U);
    CHECK_EQ(patch.update_slots[0].slot.bookmark_name, "customer");
    REQUIRE(patch.update_slots[0].update.kind.has_value());
    CHECK_EQ(*patch.update_slots[0].update.kind,
             featherdoc::template_slot_kind::block);
    CHECK_EQ(patch.update_slots[1].slot.bookmark_name, "new_optional");
    REQUIRE(patch.update_slots[1].update.min_occurrences.has_value());
    CHECK_EQ(*patch.update_slots[1].update.min_occurrences, 2U);
    REQUIRE(patch.update_slots[1].update.max_occurrences.has_value());
    CHECK_EQ(*patch.update_slots[1].update.max_occurrences, 2U);
    CHECK_EQ(patch.update_slots[2].slot.bookmark_name, "line_items");
    REQUIRE(patch.update_slots[2].update.kind.has_value());
    CHECK_EQ(*patch.update_slots[2].update.kind,
             featherdoc::template_slot_kind::table_rows);
    CHECK(patch.upsert_slots.empty());

    const auto apply_summary = featherdoc::apply_template_schema_patch(left, patch);
    CHECK(apply_summary.changed());
    (void)featherdoc::normalize_template_schema(right);

    REQUIRE(left.entries.size() == right.entries.size());
    for (std::size_t index = 0U; index < left.entries.size(); ++index) {
        CHECK_EQ(left.entries[index].target.part, right.entries[index].target.part);
        CHECK_EQ(left.entries[index].target.part_index,
                 right.entries[index].target.part_index);
        CHECK_EQ(left.entries[index].target.section_index,
                 right.entries[index].target.section_index);
        CHECK_EQ(left.entries[index].target.reference_kind,
                 right.entries[index].target.reference_kind);
        CHECK_EQ(left.entries[index].requirement.bookmark_name,
                 right.entries[index].requirement.bookmark_name);
        CHECK_EQ(left.entries[index].requirement.kind,
                 right.entries[index].requirement.kind);
        CHECK_EQ(left.entries[index].requirement.required,
                 right.entries[index].requirement.required);
        CHECK_EQ(left.entries[index].requirement.min_occurrences,
                 right.entries[index].requirement.min_occurrences);
        CHECK_EQ(left.entries[index].requirement.max_occurrences,
                 right.entries[index].requirement.max_occurrences);
    }
}

TEST_CASE("template schema patch builder preserves source-aware rename updates") {
    featherdoc::template_schema_entry left_entry{};
    left_entry.requirement.bookmark_name = "order_no";
    left_entry.requirement.source =
        featherdoc::template_slot_source_kind::content_control_tag;
    left_entry.requirement.kind = featherdoc::template_slot_kind::text;
    left_entry.requirement.required = true;

    featherdoc::template_schema_entry right_entry{};
    right_entry.requirement.bookmark_name = "order_id";
    right_entry.requirement.source =
        featherdoc::template_slot_source_kind::content_control_tag;
    right_entry.requirement.kind = featherdoc::template_slot_kind::block;
    right_entry.requirement.required = true;
    right_entry.requirement.min_occurrences = 2U;
    right_entry.requirement.max_occurrences = 2U;

    featherdoc::template_schema left{{left_entry}};
    featherdoc::template_schema right{{right_entry}};

    const auto patch = featherdoc::build_template_schema_patch(left, right);
    REQUIRE(patch.rename_slots.size() == 1U);
    CHECK_EQ(patch.rename_slots.front().slot.source,
             featherdoc::template_slot_source_kind::content_control_tag);
    CHECK_EQ(patch.rename_slots.front().slot.bookmark_name, "order_no");
    CHECK_EQ(patch.rename_slots.front().new_bookmark_name, "order_id");
    CHECK(patch.remove_slots.empty());
    REQUIRE(patch.update_slots.size() == 1U);
    CHECK_EQ(patch.update_slots.front().slot.source,
             featherdoc::template_slot_source_kind::content_control_tag);
    CHECK_EQ(patch.update_slots.front().slot.bookmark_name, "order_id");
    REQUIRE(patch.update_slots.front().update.kind.has_value());
    CHECK_EQ(*patch.update_slots.front().update.kind,
             featherdoc::template_slot_kind::block);
    REQUIRE(patch.update_slots.front().update.min_occurrences.has_value());
    CHECK_EQ(*patch.update_slots.front().update.min_occurrences, 2U);
    REQUIRE(patch.update_slots.front().update.max_occurrences.has_value());
    CHECK_EQ(*patch.update_slots.front().update.max_occurrences, 2U);
    CHECK(patch.upsert_slots.empty());

    const auto apply_summary = featherdoc::apply_template_schema_patch(left, patch);
    CHECK(apply_summary.changed());
    (void)featherdoc::normalize_template_schema(right);
    REQUIRE(left.entries.size() == right.entries.size());
    CHECK_EQ(left.entries.front().requirement.source,
             right.entries.front().requirement.source);
    CHECK_EQ(left.entries.front().requirement.bookmark_name,
             right.entries.front().requirement.bookmark_name);
    CHECK_EQ(left.entries.front().requirement.kind,
             right.entries.front().requirement.kind);
    CHECK_EQ(left.entries.front().requirement.min_occurrences,
             right.entries.front().requirement.min_occurrences);
    CHECK_EQ(left.entries.front().requirement.max_occurrences,
             right.entries.front().requirement.max_occurrences);
}


TEST_CASE("template schema patch builder emits rename occurrence clear updates") {
    featherdoc::template_schema left{{
        {{}, {"old_customer", featherdoc::template_slot_kind::text, true, 2U, 2U}},
    }};
    featherdoc::template_schema right{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
    }};

    const auto patch = featherdoc::build_template_schema_patch(left, right);
    REQUIRE(patch.rename_slots.size() == 1U);
    CHECK_EQ(patch.rename_slots.front().slot.bookmark_name, "old_customer");
    CHECK_EQ(patch.rename_slots.front().new_bookmark_name, "customer");
    CHECK(patch.remove_slots.empty());
    REQUIRE(patch.update_slots.size() == 1U);
    CHECK_EQ(patch.update_slots.front().slot.bookmark_name, "customer");
    CHECK_FALSE(patch.update_slots.front().update.min_occurrences.has_value());
    CHECK_FALSE(patch.update_slots.front().update.max_occurrences.has_value());
    CHECK(patch.update_slots.front().update.clear_min_occurrences);
    CHECK(patch.update_slots.front().update.clear_max_occurrences);
    CHECK(patch.upsert_slots.empty());

    const auto apply_summary = featherdoc::apply_template_schema_patch(left, patch);
    CHECK(apply_summary.changed());
    (void)featherdoc::normalize_template_schema(right);
    REQUIRE(left.entries.size() == right.entries.size());
    CHECK_EQ(left.entries.front().requirement.bookmark_name,
             right.entries.front().requirement.bookmark_name);
    CHECK_EQ(left.entries.front().requirement.min_occurrences,
             right.entries.front().requirement.min_occurrences);
    CHECK_EQ(left.entries.front().requirement.max_occurrences,
             right.entries.front().requirement.max_occurrences);
}

TEST_CASE("template schema patch builder keeps ambiguous renames explicit") {
    featherdoc::template_schema left{{
        {{}, {"old_primary", featherdoc::template_slot_kind::text, true}},
        {{}, {"old_secondary", featherdoc::template_slot_kind::text, true}},
    }};

    featherdoc::template_schema right{{
        {{}, {"new_primary", featherdoc::template_slot_kind::text, true}},
        {{}, {"new_secondary", featherdoc::template_slot_kind::text, true}},
    }};

    const auto patch = featherdoc::build_template_schema_patch(left, right);
    CHECK(patch.rename_slots.empty());
    CHECK(patch.update_slots.empty());
    CHECK_EQ(patch.remove_slots.size(), 2U);
    CHECK_EQ(patch.upsert_slots.size(), 2U);

    const auto has_remove_slot = [&](std::string_view bookmark_name) {
        return std::any_of(
            patch.remove_slots.begin(), patch.remove_slots.end(),
            [&](const featherdoc::template_schema_slot_selector &selector) {
                return selector.bookmark_name == bookmark_name;
            });
    };
    const auto has_upsert_slot = [&](std::string_view bookmark_name) {
        return std::any_of(
            patch.upsert_slots.begin(), patch.upsert_slots.end(),
            [&](const featherdoc::template_schema_entry &entry) {
                return entry.requirement.bookmark_name == bookmark_name;
            });
    };
    CHECK(has_remove_slot("old_primary"));
    CHECK(has_remove_slot("old_secondary"));
    CHECK(has_upsert_slot("new_primary"));
    CHECK(has_upsert_slot("new_secondary"));

    const auto apply_summary = featherdoc::apply_template_schema_patch(left, patch);
    CHECK(apply_summary.changed());
    (void)featherdoc::normalize_template_schema(right);
    REQUIRE(left.entries.size() == right.entries.size());
    for (std::size_t index = 0U; index < left.entries.size(); ++index) {
        CHECK_EQ(left.entries[index].requirement.bookmark_name,
                 right.entries[index].requirement.bookmark_name);
        CHECK_EQ(left.entries[index].requirement.kind,
                 right.entries[index].requirement.kind);
        CHECK_EQ(left.entries[index].requirement.required,
                 right.entries[index].requirement.required);
    }
}

TEST_CASE("template schema patch builder keeps cross-source changes explicit") {
    featherdoc::template_schema_entry left_entry{};
    left_entry.requirement.bookmark_name = "shared_id";
    left_entry.requirement.source =
        featherdoc::template_slot_source_kind::bookmark;
    left_entry.requirement.kind = featherdoc::template_slot_kind::text;
    left_entry.requirement.required = true;

    featherdoc::template_schema_entry right_entry{};
    right_entry.requirement.bookmark_name = "shared_id";
    right_entry.requirement.source =
        featherdoc::template_slot_source_kind::content_control_tag;
    right_entry.requirement.kind = featherdoc::template_slot_kind::text;
    right_entry.requirement.required = true;

    featherdoc::template_schema left{{left_entry}};
    featherdoc::template_schema right{{right_entry}};

    const auto patch = featherdoc::build_template_schema_patch(left, right);
    CHECK(patch.rename_slots.empty());
    CHECK(patch.update_slots.empty());
    REQUIRE(patch.remove_slots.size() == 1U);
    CHECK_EQ(patch.remove_slots.front().source,
             featherdoc::template_slot_source_kind::bookmark);
    CHECK_EQ(patch.remove_slots.front().bookmark_name, "shared_id");
    REQUIRE(patch.upsert_slots.size() == 1U);
    CHECK_EQ(patch.upsert_slots.front().requirement.source,
             featherdoc::template_slot_source_kind::content_control_tag);
    CHECK_EQ(patch.upsert_slots.front().requirement.bookmark_name, "shared_id");

    const auto apply_summary = featherdoc::apply_template_schema_patch(left, patch);
    CHECK(apply_summary.changed());
    (void)featherdoc::normalize_template_schema(right);
    REQUIRE(left.entries.size() == right.entries.size());
    CHECK_EQ(left.entries.front().requirement.source,
             right.entries.front().requirement.source);
    CHECK_EQ(left.entries.front().requirement.bookmark_name,
             right.entries.front().requirement.bookmark_name);
    CHECK_EQ(left.entries.front().requirement.kind,
             right.entries.front().requirement.kind);
}

TEST_CASE("template schema patch builder keeps cross-target changes explicit") {
    featherdoc::template_schema_part_selector header_target{};
    header_target.part = featherdoc::template_schema_part_kind::header;
    header_target.part_index = 1U;

    featherdoc::template_schema left{{
        {{}, {"body_keep", featherdoc::template_slot_kind::text, true}},
        {{}, {"old_title", featherdoc::template_slot_kind::text, true}},
        {header_target, {"header_keep", featherdoc::template_slot_kind::text, true}},
    }};

    featherdoc::template_schema right{{
        {{}, {"body_keep", featherdoc::template_slot_kind::text, true}},
        {header_target, {"header_keep", featherdoc::template_slot_kind::text, true}},
        {header_target, {"new_title", featherdoc::template_slot_kind::text, true}},
    }};

    const auto patch = featherdoc::build_template_schema_patch(left, right);
    CHECK(patch.rename_slots.empty());
    CHECK(patch.update_slots.empty());
    REQUIRE(patch.remove_slots.size() == 1U);
    CHECK_EQ(patch.remove_slots.front().target.part,
             featherdoc::template_schema_part_kind::body);
    CHECK_EQ(patch.remove_slots.front().bookmark_name, "old_title");
    REQUIRE(patch.upsert_slots.size() == 1U);
    CHECK_EQ(patch.upsert_slots.front().target.part,
             featherdoc::template_schema_part_kind::header);
    CHECK_EQ(patch.upsert_slots.front().target.part_index, 1U);
    CHECK_EQ(patch.upsert_slots.front().requirement.bookmark_name, "new_title");

    const auto apply_summary = featherdoc::apply_template_schema_patch(left, patch);
    CHECK(apply_summary.changed());
    (void)featherdoc::normalize_template_schema(right);
    REQUIRE(left.entries.size() == right.entries.size());
    for (std::size_t index = 0U; index < left.entries.size(); ++index) {
        CHECK_EQ(left.entries[index].target.part, right.entries[index].target.part);
        CHECK_EQ(left.entries[index].target.part_index,
                 right.entries[index].target.part_index);
        CHECK_EQ(left.entries[index].requirement.bookmark_name,
                 right.entries[index].requirement.bookmark_name);
        CHECK_EQ(left.entries[index].requirement.kind,
                 right.entries[index].requirement.kind);
    }
}

TEST_CASE("template schema patch builder emits occurrence clear updates") {
    featherdoc::template_schema left{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true, 2U, 2U}},
    }};
    featherdoc::template_schema right{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
    }};

    const auto patch = featherdoc::build_template_schema_patch(left, right);
    CHECK(patch.rename_slots.empty());
    CHECK(patch.remove_slots.empty());
    CHECK(patch.upsert_slots.empty());
    REQUIRE(patch.update_slots.size() == 1U);
    CHECK_EQ(patch.update_slots.front().slot.bookmark_name, "customer");
    CHECK_FALSE(patch.update_slots.front().update.min_occurrences.has_value());
    CHECK_FALSE(patch.update_slots.front().update.max_occurrences.has_value());
    CHECK(patch.update_slots.front().update.clear_min_occurrences);
    CHECK(patch.update_slots.front().update.clear_max_occurrences);

    const auto apply_summary = featherdoc::apply_template_schema_patch(left, patch);
    CHECK(apply_summary.changed());
    (void)featherdoc::normalize_template_schema(right);
    REQUIRE(left.entries.size() == right.entries.size());
    CHECK_EQ(left.entries.front().requirement.bookmark_name,
             right.entries.front().requirement.bookmark_name);
    CHECK_EQ(left.entries.front().requirement.min_occurrences,
             right.entries.front().requirement.min_occurrences);
    CHECK_EQ(left.entries.front().requirement.max_occurrences,
             right.entries.front().requirement.max_occurrences);
}

TEST_CASE("template schema high-level mutation helpers update targets and slots") {
    const auto find_slot = [](const featherdoc::template_schema &current,
                              featherdoc::template_schema_part_kind part,
                              std::string_view bookmark_name,
                              featherdoc::template_slot_source_kind source =
                                  featherdoc::template_slot_source_kind::bookmark)
        -> const featherdoc::template_schema_entry * {
        for (const auto &entry : current.entries) {
            if (entry.target.part == part && entry.requirement.source == source &&
                entry.requirement.bookmark_name == bookmark_name) {
                return &entry;
            }
        }
        return nullptr;
    };

    featherdoc::template_schema_part_selector header_target{};
    header_target.part = featherdoc::template_schema_part_kind::header;

    featherdoc::template_schema schema{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
        {{}, {"line_items", featherdoc::template_slot_kind::table_rows, true}},
        {header_target, {"header_title", featherdoc::template_slot_kind::text, true}},
    }};
    schema.entries[0].requirement.min_occurrences = 1U;
    schema.entries[0].requirement.max_occurrences = 1U;

    featherdoc::template_schema_patch preview_patch{};
    preview_patch.remove_slots.push_back({{}, "line_items"});
    featherdoc::template_schema_slot_patch_update preview_update{};
    preview_update.slot.bookmark_name = "customer";
    preview_update.update.required = false;
    preview_patch.update_slots.push_back(preview_update);
    preview_patch.upsert_slots.push_back(
        {{}, {"invoice_total", featherdoc::template_slot_kind::text, false}});
    const auto preview_summary =
        featherdoc::preview_template_schema_patch(schema, preview_patch);
    CHECK_EQ(preview_summary.removed_slots, 1U);
    CHECK_EQ(preview_summary.inserted_slots, 1U);
    CHECK_EQ(preview_summary.replaced_slots, 1U);
    REQUIRE(schema.entries.size() == 3U);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::body,
                    "line_items") != nullptr);
    const auto *preview_customer = find_slot(
        schema, featherdoc::template_schema_part_kind::body, "customer");
    REQUIRE(preview_customer != nullptr);
    CHECK(preview_customer->requirement.required);

    const auto replace_summary = featherdoc::replace_template_schema_target(
        schema,
        featherdoc::template_schema_part_selector{},
        {{{}, {"invoice_total", featherdoc::template_slot_kind::text, false}},
         {{}, {"customer", featherdoc::template_slot_kind::text, true}}});
    CHECK_EQ(replace_summary.removed_targets, 1U);
    CHECK_EQ(replace_summary.inserted_slots, 2U);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::body,
                    "line_items") == nullptr);
    REQUIRE(find_slot(schema, featherdoc::template_schema_part_kind::header,
                      "header_title") != nullptr);

    featherdoc::template_schema_slot_update update{};
    update.kind = featherdoc::template_slot_kind::block;
    update.required = false;
    update.min_occurrences = 2U;
    update.max_occurrences = 5U;
    const auto update_summary = featherdoc::update_template_schema_slot(
        schema, {{}, "customer"}, update);
    CHECK_EQ(update_summary.replaced_slots, 1U);
    const auto *customer = find_slot(
        schema, featherdoc::template_schema_part_kind::body, "customer");
    REQUIRE(customer != nullptr);
    CHECK_EQ(customer->requirement.kind, featherdoc::template_slot_kind::block);
    CHECK_FALSE(customer->requirement.required);
    REQUIRE(customer->requirement.min_occurrences.has_value());
    CHECK_EQ(*customer->requirement.min_occurrences, 2U);
    REQUIRE(customer->requirement.max_occurrences.has_value());
    CHECK_EQ(*customer->requirement.max_occurrences, 5U);

    featherdoc::template_schema_slot_update clear_occurrences{};
    clear_occurrences.clear_min_occurrences = true;
    clear_occurrences.clear_max_occurrences = true;
    const auto clear_summary = featherdoc::update_template_schema_slot(
        schema, {{}, "customer"}, clear_occurrences);
    CHECK_EQ(clear_summary.replaced_slots, 1U);
    customer = find_slot(schema, featherdoc::template_schema_part_kind::body,
                         "customer");
    REQUIRE(customer != nullptr);
    CHECK_FALSE(customer->requirement.min_occurrences.has_value());
    CHECK_FALSE(customer->requirement.max_occurrences.has_value());

    featherdoc::template_schema_entry status_slot{};
    status_slot.requirement.bookmark_name = "invoice_status";
    status_slot.requirement.kind = featherdoc::template_slot_kind::text;
    status_slot.requirement.source =
        featherdoc::template_slot_source_kind::content_control_tag;
    const auto upsert_summary =
        featherdoc::upsert_template_schema_slot(schema, status_slot);
    CHECK_EQ(upsert_summary.inserted_slots, 1U);

    featherdoc::template_schema_slot_selector status_selector{};
    status_selector.bookmark_name = "invoice_status";
    status_selector.source =
        featherdoc::template_slot_source_kind::content_control_tag;
    const auto rename_summary = featherdoc::rename_template_schema_slot(
        schema, status_selector, "invoice_state");
    CHECK_EQ(rename_summary.renamed_slots, 1U);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::body,
                    "invoice_status",
                    featherdoc::template_slot_source_kind::content_control_tag) ==
          nullptr);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::body,
                    "invoice_state",
                    featherdoc::template_slot_source_kind::content_control_tag) !=
          nullptr);

    const auto remove_slot_summary = featherdoc::remove_template_schema_slot(
        schema, {{}, "invoice_total"});
    CHECK_EQ(remove_slot_summary.removed_slots, 1U);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::body,
                    "invoice_total") == nullptr);

    const auto missing_update_summary = featherdoc::update_template_schema_slot(
        schema, {{}, "missing"}, update);
    CHECK_FALSE(missing_update_summary.changed());

    const auto remove_target_summary =
        featherdoc::remove_template_schema_target(schema, header_target);
    CHECK_EQ(remove_target_summary.removed_targets, 1U);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::header,
                    "header_title") == nullptr);
}
