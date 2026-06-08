#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
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

TEST_CASE("template schema patch review summary writes stable json") {
    featherdoc::template_schema baseline{{
        {{}, {"customer", featherdoc::template_slot_kind::text, false}},
        {{}, {"obsolete", featherdoc::template_slot_kind::text, true}},
    }};

    featherdoc::template_schema generated{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
        {{}, {"order_no", featherdoc::template_slot_kind::text, true}},
    }};
    generated.entries.back().requirement.source =
        featherdoc::template_slot_source_kind::content_control_tag;

    const auto summary = featherdoc::make_template_schema_patch_review_summary(
        baseline, generated);
    CHECK_EQ(summary.baseline_slot_count, 2U);
    REQUIRE(summary.generated_slot_count.has_value());
    CHECK_EQ(*summary.generated_slot_count, 2U);
    CHECK_EQ(summary.patch_upsert_slot_count, 1U);
    CHECK_EQ(summary.patch_remove_target_count, 0U);
    CHECK_EQ(summary.patch_remove_slot_count, 1U);
    CHECK_EQ(summary.patch_rename_slot_count, 0U);
    CHECK_EQ(summary.patch_update_slot_count, 1U);
    CHECK_EQ(summary.preview.removed_slots, 1U);
    CHECK_EQ(summary.preview.inserted_slots, 1U);
    CHECK_EQ(summary.preview.replaced_slots, 1U);
    CHECK(summary.changed());

    CHECK_EQ(
        featherdoc::template_schema_patch_review_json(summary),
        std::string{
            R"({"schema":"featherdoc.template_schema_patch_review.v1","baseline_slot_count":2,"generated_slot_count":2,"patch":{"upsert_slot_count":1,"remove_target_count":0,"remove_slot_count":1,"rename_slot_count":0,"update_slot_count":1},"preview":{"removed_targets":0,"removed_slots":1,"renamed_slots":0,"inserted_slots":1,"replaced_slots":1,"changed":true},"changed":true})"});

    const auto patch = featherdoc::build_template_schema_patch(baseline, generated);
    const auto patch_summary = featherdoc::make_template_schema_patch_review_summary(
        baseline, patch);
    CHECK_FALSE(patch_summary.generated_slot_count.has_value());
    CHECK_NE(featherdoc::template_schema_patch_review_json(patch_summary)
                 .find(R"("generated_slot_count":null)"),
             std::string::npos);
}

TEST_CASE("rename_template_schema_target moves slots and merges conflicts") {
    const auto find_slot = [](const featherdoc::template_schema &current,
                              featherdoc::template_schema_part_kind part,
                              std::string_view bookmark_name)
        -> const featherdoc::template_schema_entry * {
        for (const auto &entry : current.entries) {
            if (entry.target.part == part &&
                entry.requirement.bookmark_name == bookmark_name) {
                return &entry;
            }
        }
        return nullptr;
    };

    featherdoc::template_schema_part_selector header_target{};
    header_target.part = featherdoc::template_schema_part_kind::header;
    featherdoc::template_schema_part_selector footer_target{};
    footer_target.part = featherdoc::template_schema_part_kind::footer;

    featherdoc::template_schema schema{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
        {{}, {"line_items", featherdoc::template_slot_kind::table_rows, true}},
        {header_target, {"customer", featherdoc::template_slot_kind::text, false}},
        {header_target, {"header_title", featherdoc::template_slot_kind::text, true}},
        {footer_target, {"footer_note", featherdoc::template_slot_kind::block, true}},
    }};

    const auto summary = featherdoc::rename_template_schema_target(
        schema, featherdoc::template_schema_part_selector{}, header_target);
    CHECK_EQ(summary.removed_targets, 1U);
    CHECK_EQ(summary.inserted_slots, 1U);
    CHECK_EQ(summary.replaced_slots, 1U);

    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::body,
                    "customer") == nullptr);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::body,
                    "line_items") == nullptr);

    const auto *header_customer = find_slot(
        schema, featherdoc::template_schema_part_kind::header, "customer");
    REQUIRE(header_customer != nullptr);
    CHECK(header_customer->requirement.required);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::header,
                    "line_items") != nullptr);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::header,
                    "header_title") != nullptr);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::footer,
                    "footer_note") != nullptr);

    const auto noop_summary = featherdoc::rename_template_schema_target(
        schema, header_target, header_target);
    CHECK_FALSE(noop_summary.changed());

    featherdoc::template_schema_part_selector missing_target{};
    missing_target.part = featherdoc::template_schema_part_kind::section_footer;
    const auto missing_summary = featherdoc::rename_template_schema_target(
        schema, missing_target, footer_target);
    CHECK_FALSE(missing_summary.changed());
}

TEST_CASE("validate_template_schema aggregates section header and footer validation") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_schema_validate_parts.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml"
           ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>Schema Validation Fixture</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
</Relationships>
)";

    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Header title: </w:t></w:r>
    <w:bookmarkStart w:id="0" w:name="header_title"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Footer company A: </w:t></w:r>
    <w:bookmarkStart w:id="1" w:name="footer_company"/>
    <w:r><w:t>placeholder A</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
  <w:p>
    <w:r><w:t>Footer company B: </w:t></w:r>
    <w:bookmarkStart w:id="2" w:name="footer_company"/>
    <w:r><w:t>placeholder B</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
  <w:p>
    <w:r><w:t>Summary: prefix </w:t></w:r>
    <w:bookmarkStart w:id="3" w:name="footer_summary"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="3"/>
    <w:r><w:t> suffix</w:t></w:r>
  </w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
            {"word/footer1.xml", footer_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    featherdoc::template_schema schema{
        {
            {
                {featherdoc::template_schema_part_kind::section_header, std::nullopt, 0U,
                 featherdoc::section_reference_kind::default_reference},
                {"header_title", featherdoc::template_slot_kind::text, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 0U,
                 featherdoc::section_reference_kind::default_reference},
                {"footer_company", featherdoc::template_slot_kind::text, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 0U,
                 featherdoc::section_reference_kind::default_reference},
                {"footer_summary", featherdoc::template_slot_kind::block, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 0U,
                 featherdoc::section_reference_kind::default_reference},
                {"footer_signature", featherdoc::template_slot_kind::text, true},
            },
        }};

    const auto result = doc.validate_template_schema(schema);

    CHECK_FALSE(doc.last_error());
    REQUIRE(result.part_results.size() == 2U);
    CHECK(result.part_results[0].available);
    CHECK_EQ(result.part_results[0].entry_name, "word/header1.xml");
    CHECK(static_cast<bool>(result.part_results[0]));
    CHECK_EQ(result.part_results[0].target.part,
             featherdoc::template_schema_part_kind::section_header);
    REQUIRE(result.part_results[0].target.section_index.has_value());
    CHECK_EQ(*result.part_results[0].target.section_index, 0U);

    CHECK(result.part_results[1].available);
    CHECK_EQ(result.part_results[1].entry_name, "word/footer1.xml");
    CHECK_FALSE(static_cast<bool>(result.part_results[1]));
    CHECK_EQ(result.part_results[1].target.part,
             featherdoc::template_schema_part_kind::section_footer);
    REQUIRE(result.part_results[1].target.section_index.has_value());
    CHECK_EQ(*result.part_results[1].target.section_index, 0U);
    REQUIRE(result.part_results[1].validation.missing_required.size() == 1U);
    CHECK_EQ(result.part_results[1].validation.missing_required.front(),
             "footer_signature");
    REQUIRE(result.part_results[1].validation.duplicate_bookmarks.size() == 1U);
    CHECK_EQ(result.part_results[1].validation.duplicate_bookmarks.front(),
             "footer_company");
    REQUIRE(result.part_results[1].validation.malformed_placeholders.size() == 1U);
    CHECK_EQ(result.part_results[1].validation.malformed_placeholders.front(),
             "footer_summary");
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template_schema resolves linked-to-previous section targets") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_schema_validate_resolved.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml"
           ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>Section 0 body</w:t></w:r></w:p>
    <w:p>
      <w:r><w:t>Section break</w:t></w:r>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
    </w:p>
    <w:p><w:r><w:t>Section 1 body</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
</Relationships>
)";

    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Header title: </w:t></w:r>
    <w:bookmarkStart w:id="0" w:name="header_title"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Footer company: </w:t></w:r>
    <w:bookmarkStart w:id="1" w:name="footer_company"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
            {"word/footer1.xml", footer_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template_schema(
        {
            {
                {featherdoc::template_schema_part_kind::section_header, std::nullopt, 1U,
                 featherdoc::section_reference_kind::default_reference},
                {"header_title", featherdoc::template_slot_kind::text, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 1U,
                 featherdoc::section_reference_kind::default_reference},
                {"footer_company", featherdoc::template_slot_kind::text, true},
            },
        });

    CHECK_FALSE(doc.last_error());
    REQUIRE(result.part_results.size() == 2U);
    CHECK(result.part_results[0].available);
    CHECK_EQ(result.part_results[0].entry_name, "word/header1.xml");
    CHECK(static_cast<bool>(result.part_results[0]));
    REQUIRE(result.part_results[0].target.section_index.has_value());
    CHECK_EQ(*result.part_results[0].target.section_index, 1U);

    CHECK(result.part_results[1].available);
    CHECK_EQ(result.part_results[1].entry_name, "word/footer1.xml");
    CHECK(static_cast<bool>(result.part_results[1]));
    REQUIRE(result.part_results[1].target.section_index.has_value());
    CHECK_EQ(*result.part_results[1].target.section_index, 1U);
    CHECK(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template_schema reports unavailable parts without failing optional-only groups") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_schema_validate_unavailable.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto result = doc.validate_template_schema(
        {
            {
                {featherdoc::template_schema_part_kind::section_header, std::nullopt, 1U,
                 featherdoc::section_reference_kind::default_reference},
                {"optional_header", featherdoc::template_slot_kind::text, false},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 1U,
                 featherdoc::section_reference_kind::default_reference},
                {"required_footer", featherdoc::template_slot_kind::text, true},
            },
        });

    CHECK_FALSE(doc.last_error());
    REQUIRE(result.part_results.size() == 2U);
    CHECK_FALSE(result.part_results[0].available);
    CHECK(result.part_results[0].entry_name.empty());
    CHECK(result.part_results[0].validation.missing_required.empty());
    CHECK(static_cast<bool>(result.part_results[0]));

    CHECK_FALSE(result.part_results[1].available);
    CHECK(result.part_results[1].entry_name.empty());
    REQUIRE(result.part_results[1].validation.missing_required.size() == 1U);
    CHECK_EQ(result.part_results[1].validation.missing_required.front(),
             "required_footer");
    CHECK_FALSE(static_cast<bool>(result.part_results[1]));
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template_schema rejects invalid selectors") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_schema_validate_invalid.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto result = doc.validate_template_schema(
        {{
            {featherdoc::template_schema_part_kind::section_header, std::nullopt,
             std::nullopt, featherdoc::section_reference_kind::default_reference},
            {"header_title", featherdoc::template_slot_kind::text, true},
        }});

    CHECK(result.part_results.empty());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail,
             "section-header/section-footer schema target requires section_index");
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    fs::remove(target);
}


TEST_CASE("scan_template_schema builds a patch from document slots") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_schema_scan_patch.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>Ada Lovelace</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>Order: </w:t></w:r>
      <w:sdt>
        <w:sdtPr><w:tag w:val="order_no"/></w:sdtPr>
        <w:sdtContent><w:r><w:t>INV-001</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto find_slot = [](const featherdoc::template_schema &schema,
                              std::string_view bookmark_name,
                              featherdoc::template_slot_source_kind source =
                                  featherdoc::template_slot_source_kind::bookmark)
        -> const featherdoc::template_schema_entry * {
        for (const auto &entry : schema.entries) {
            if (entry.target.part == featherdoc::template_schema_part_kind::body &&
                entry.requirement.bookmark_name == bookmark_name &&
                entry.requirement.source == source) {
                return &entry;
            }
        }
        return nullptr;
    };

    const auto scan = doc.scan_template_schema();
    REQUIRE(scan.has_value());
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(scan->slot_count(), 2U);
    CHECK(scan->skipped_bookmarks.empty());

    const auto *customer = find_slot(scan->schema, "customer");
    REQUIRE(customer != nullptr);
    CHECK_EQ(customer->requirement.kind, featherdoc::template_slot_kind::text);
    CHECK(customer->requirement.required);

    const auto *order = find_slot(
        scan->schema, "order_no",
        featherdoc::template_slot_source_kind::content_control_tag);
    REQUIRE(order != nullptr);
    CHECK_EQ(order->requirement.kind, featherdoc::template_slot_kind::text);

    featherdoc::template_schema baseline{{
        {{}, {"customer", featherdoc::template_slot_kind::text, false}},
        {{}, {"obsolete", featherdoc::template_slot_kind::text, true}},
    }};

    const auto patch = doc.build_template_schema_patch_from_scan(baseline);
    REQUIRE(patch.has_value());
    CHECK_FALSE(doc.last_error());
    REQUIRE(patch->remove_slots.size() == 1U);
    CHECK_EQ(patch->remove_slots.front().bookmark_name, "obsolete");
    REQUIRE(patch->upsert_slots.size() == 1U);
    CHECK_EQ(patch->upsert_slots.front().requirement.bookmark_name, "order_no");
    CHECK_EQ(patch->upsert_slots.front().requirement.source,
             featherdoc::template_slot_source_kind::content_control_tag);
    REQUIRE(patch->update_slots.size() == 1U);
    CHECK_EQ(patch->update_slots.front().slot.bookmark_name, "customer");
    REQUIRE(patch->update_slots.front().update.required.has_value());
    CHECK(*patch->update_slots.front().update.required);

    auto patched = baseline;
    const auto summary = featherdoc::apply_template_schema_patch(patched, *patch);
    CHECK_EQ(summary.removed_slots, 1U);
    CHECK_EQ(summary.inserted_slots, 1U);
    CHECK_EQ(summary.replaced_slots, 1U);
    CHECK(find_slot(patched, "obsolete") == nullptr);
    CHECK(find_slot(patched, "customer") != nullptr);
    CHECK(find_slot(patched, "order_no",
                    featherdoc::template_slot_source_kind::content_control_tag) !=
          nullptr);

    fs::remove(target);
}

TEST_CASE("onboard_template summarizes first-time template onboarding") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_onboarding_first_run.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>Ada Lovelace</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:sdt>
        <w:sdtPr><w:tag w:val="order_no"/></w:sdtPr>
        <w:sdtContent><w:r><w:t>INV-001</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.onboard_template();
    REQUIRE(result.has_value());
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(result->slot_count(), 2U);
    CHECK_FALSE(result->baseline_schema_available);
    CHECK_FALSE(result->schema_patch.has_value());
    CHECK_FALSE(result->patch_review.has_value());
    CHECK_FALSE(result->has_errors());
    CHECK(result->ready_for_render_data());
    CHECK_FALSE(result->ready_for_project_smoke());

    const auto has_action = [&](featherdoc::template_onboarding_next_action_kind kind) {
        return std::any_of(result->next_actions.begin(), result->next_actions.end(),
                           [&](const featherdoc::template_onboarding_next_action &action) {
                               return action.kind == kind;
                           });
    };
    CHECK(has_action(featherdoc::template_onboarding_next_action_kind::create_schema_baseline));
    CHECK(has_action(featherdoc::template_onboarding_next_action_kind::prepare_render_data));

    fs::remove(target);
}

TEST_CASE("onboard_template reports baseline schema drift for review") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_onboarding_schema_drift.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>Ada Lovelace</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:sdt>
        <w:sdtPr><w:tag w:val="order_no"/></w:sdtPr>
        <w:sdtContent><w:r><w:t>INV-001</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    featherdoc::template_schema baseline{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
        {{}, {"obsolete", featherdoc::template_slot_kind::text, true}},
    }};
    featherdoc::template_onboarding_options options{};
    options.baseline_schema = baseline;

    const auto result = doc.onboard_template(options);
    REQUIRE(result.has_value());
    CHECK_FALSE(doc.last_error());
    CHECK(result->baseline_schema_available);
    REQUIRE(result->schema_patch.has_value());
    REQUIRE(result->patch_review.has_value());
    CHECK(result->requires_schema_review());
    CHECK_FALSE(result->ready_for_render_data());
    CHECK_FALSE(result->has_errors());
    CHECK(result->has_warnings());

    const auto review_action = std::find_if(
        result->next_actions.begin(), result->next_actions.end(),
        [](const featherdoc::template_onboarding_next_action &action) {
            return action.kind ==
                   featherdoc::template_onboarding_next_action_kind::review_schema_patch;
        });
    REQUIRE(review_action != result->next_actions.end());
    CHECK(review_action->blocking);

    fs::remove(target);
}

TEST_CASE("onboard_template blocks malformed template slots") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_onboarding_malformed.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>Ada Lovelace</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="1" w:name="broken_slot"/>
      <w:r><w:t>Missing end marker</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.onboard_template();
    REQUIRE(result.has_value());
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(result->slot_count(), 1U);
    REQUIRE_EQ(result->scan.skipped_bookmarks.size(), 1U);
    CHECK_EQ(result->scan.skipped_bookmarks.front().bookmark.bookmark_name,
             "broken_slot");
    CHECK(result->has_errors());
    CHECK_FALSE(result->ready_for_render_data());

    const auto fix_action = std::find_if(
        result->next_actions.begin(), result->next_actions.end(),
        [](const featherdoc::template_onboarding_next_action &action) {
            return action.kind ==
                   featherdoc::template_onboarding_next_action_kind::fix_template_slots;
        });
    REQUIRE(fix_action != result->next_actions.end());
    CHECK(fix_action->blocking);

    fs::remove(target);
}

TEST_CASE("validate_template_schema supports content control tag and alias slots") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_schema_content_controls.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Customer Name"/>
        <w:tag w:val="customer_name"/>
        <w:id w:val="42"/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Ada Lovelace</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:p>
      <w:r><w:t>Order: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Order Number"/>
          <w:tag w:val="order_no"/>
          <w:id w:val="43"/>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>INV-001</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:tbl>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Line Items"/>
          <w:tag w:val="line_items"/>
          <w:id w:val="44"/>
        </w:sdtPr>
        <w:sdtContent>
          <w:tr>
            <w:tc><w:p><w:r><w:t>SKU-1</w:t></w:r></w:p></w:tc>
          </w:tr>
        </w:sdtContent>
      </w:sdt>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto make_requirement =
        [](std::string name, featherdoc::template_slot_kind kind,
           featherdoc::template_slot_source_kind source) {
            featherdoc::template_slot_requirement requirement{};
            requirement.bookmark_name = std::move(name);
            requirement.kind = kind;
            requirement.source = source;
            return requirement;
        };

    const featherdoc::template_schema schema{{
        {{}, make_requirement("customer_name", featherdoc::template_slot_kind::block,
                              featherdoc::template_slot_source_kind::content_control_tag)},
        {{}, make_requirement("order_no", featherdoc::template_slot_kind::text,
                              featherdoc::template_slot_source_kind::content_control_tag)},
        {{}, make_requirement("Line Items", featherdoc::template_slot_kind::table_rows,
                              featherdoc::template_slot_source_kind::content_control_alias)},
    }};
    const auto result = doc.validate_template_schema(schema);
    CHECK_FALSE(doc.last_error());
    REQUIRE(result.part_results.size() == 1U);
    CHECK(static_cast<bool>(result.part_results.front()));
    CHECK(static_cast<bool>(result));

    const featherdoc::template_schema missing_schema{{
        {{}, make_requirement("missing", featherdoc::template_slot_kind::text,
                              featherdoc::template_slot_source_kind::content_control_tag)},
    }};
    const auto missing_result = doc.validate_template_schema(missing_schema);
    CHECK_FALSE(doc.last_error());
    REQUIRE(missing_result.part_results.size() == 1U);
    REQUIRE(missing_result.part_results.front().validation.missing_required.size() == 1U);
    CHECK_EQ(missing_result.part_results.front().validation.missing_required.front(),
             "content_control_tag:missing");

    const featherdoc::template_schema mismatch_schema{{
        {{}, make_requirement("order_no", featherdoc::template_slot_kind::table_rows,
                              featherdoc::template_slot_source_kind::content_control_tag)},
    }};
    const auto mismatch_result = doc.validate_template_schema(mismatch_schema);
    CHECK_FALSE(doc.last_error());
    REQUIRE(mismatch_result.part_results.size() == 1U);
    REQUIRE(mismatch_result.part_results.front().validation.kind_mismatches.size() == 1U);
    const auto &mismatch =
        mismatch_result.part_results.front().validation.kind_mismatches.front();
    CHECK_EQ(mismatch.bookmark_name, "content_control_tag:order_no");
    CHECK_EQ(mismatch.expected_kind, featherdoc::template_slot_kind::table_rows);
    CHECK_EQ(mismatch.actual_kind, featherdoc::bookmark_kind::text);
    CHECK_EQ(mismatch.occurrence_count, 1U);

    fs::remove(target);
}
