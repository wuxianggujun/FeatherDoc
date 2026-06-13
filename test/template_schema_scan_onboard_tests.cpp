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
