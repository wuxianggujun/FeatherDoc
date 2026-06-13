#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("merge_style rewrites references removes source style and round-trips") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_merge_round_trip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition source_style;
    source_style.name = "Source Style";
    source_style.based_on = "Normal";
    source_style.next_style = "SourceStyle";
    source_style.is_quick_format = true;
    CHECK(doc.ensure_paragraph_style("SourceStyle", source_style));

    featherdoc::paragraph_style_definition target_style;
    target_style.name = "Target Style";
    target_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("TargetStyle", target_style));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("merged style paragraph").has_next());
    CHECK(doc.set_paragraph_style(paragraph, "SourceStyle"));

    CHECK(doc.merge_style("SourceStyle", "TargetStyle"));
    CHECK_FALSE(doc.find_style("SourceStyle").has_value());
    const auto merged_style = doc.find_style("TargetStyle");
    REQUIRE(merged_style.has_value());
    CHECK_EQ(merged_style->style_id, "TargetStyle");
    CHECK_EQ(merged_style->name, "Target Style");

    const auto usage = doc.find_style_usage("TargetStyle");
    REQUIRE(usage.has_value());
    CHECK_EQ(usage->paragraph_count, 1U);
    CHECK_EQ(usage->total_count(), 1U);

    CHECK_FALSE(doc.merge_style("TargetStyle", "TargetStyle"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_FALSE(reopened.find_style("SourceStyle").has_value());
    const auto reopened_usage = reopened.find_style_usage("TargetStyle");
    REQUIRE(reopened_usage.has_value());
    CHECK_EQ(reopened_usage->paragraph_count, 1U);
    CHECK_EQ(reopened_usage->total_count(), 1U);

    const auto styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(styles_xml.find(R"(w:styleId="SourceStyle")"), std::string::npos);
    CHECK_NE(styles_xml.find(R"(w:styleId="TargetStyle")"), std::string::npos);
    const auto document_xml = read_test_docx_entry(target, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w:pStyle w:val="TargetStyle")"), std::string::npos);
    CHECK_EQ(document_xml.find(R"(w:pStyle w:val="SourceStyle")"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("plan_style_refactor validates rename and merge operations without mutating") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_refactor_plan.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition source_style;
    source_style.name = "Source Style";
    source_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("SourceStyle", source_style));

    featherdoc::paragraph_style_definition target_style;
    target_style.name = "Target Style";
    target_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("TargetStyle", target_style));

    featherdoc::character_style_definition character_target;
    character_target.name = "Character Target";
    CHECK(doc.ensure_character_style("CharacterTarget", character_target));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("refactor plan paragraph").has_next());
    CHECK(doc.set_paragraph_style(paragraph, "SourceStyle"));

    const auto plan = doc.plan_style_refactor(
        {
            {featherdoc::style_refactor_action::rename, "SourceStyle", "RenamedStyle"},
            {featherdoc::style_refactor_action::merge, "SourceStyle", "TargetStyle"},
            {featherdoc::style_refactor_action::merge, "SourceStyle", "CharacterTarget"},
            {featherdoc::style_refactor_action::rename, "MissingStyle", "NewStyle"},
            {featherdoc::style_refactor_action::rename, "SourceStyle", "TargetStyle"},
        });
    REQUIRE(plan.has_value());
    CHECK_FALSE(plan->clean());
    CHECK_EQ(plan->operation_count, 5U);
    CHECK_EQ(plan->applyable_count, 2U);
    CHECK_EQ(plan->issue_count, 3U);
    REQUIRE_EQ(plan->operations.size(), 5U);

    CHECK_EQ(plan->operations[0].action, featherdoc::style_refactor_action::rename);
    CHECK(plan->operations[0].applyable);
    REQUIRE(plan->operations[0].source_usage.has_value());
    CHECK_EQ(plan->operations[0].source_usage->total_count(), 1U);
    CHECK_FALSE(plan->operations[0].target_style.has_value());

    CHECK_EQ(plan->operations[1].action, featherdoc::style_refactor_action::merge);
    CHECK(plan->operations[1].applyable);
    REQUIRE(plan->operations[1].target_style.has_value());
    CHECK_EQ(plan->operations[1].target_style->kind, featherdoc::style_kind::paragraph);

    REQUIRE_EQ(plan->operations[2].issues.size(), 1U);
    CHECK_EQ(plan->operations[2].issues[0].code, "style_type_mismatch");
    CHECK_FALSE(plan->operations[2].applyable);

    REQUIRE_EQ(plan->operations[3].issues.size(), 1U);
    CHECK_EQ(plan->operations[3].issues[0].code, "missing_source_style");
    CHECK_FALSE(plan->operations[3].applyable);

    REQUIRE_EQ(plan->operations[4].issues.size(), 1U);
    CHECK_EQ(plan->operations[4].issues[0].code, "target_style_exists");
    CHECK_FALSE(plan->operations[4].applyable);

    CHECK(doc.find_style("SourceStyle").has_value());
    CHECK(doc.find_style("TargetStyle").has_value());
    CHECK_FALSE(doc.find_style("RenamedStyle").has_value());

    fs::remove(target);
}

TEST_CASE("suggest_style_merges recommends duplicate custom styles by usage") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_merge_suggestions.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition duplicate_a;
    duplicate_a.name = "Duplicate Body A";
    duplicate_a.based_on = "Normal";
    duplicate_a.run_font_family = "Aptos";
    CHECK(doc.ensure_paragraph_style("DuplicateBodyA", duplicate_a));

    featherdoc::paragraph_style_definition duplicate_b;
    duplicate_b.name = "Duplicate Body B";
    duplicate_b.based_on = "Normal";
    duplicate_b.run_font_family = "Aptos";
    CHECK(doc.ensure_paragraph_style("DuplicateBodyB", duplicate_b));

    featherdoc::paragraph_style_definition duplicate_c;
    duplicate_c.name = "Duplicate Body C";
    duplicate_c.based_on = "Normal";
    duplicate_c.next_style = "Normal";
    duplicate_c.run_font_family = "Aptos";
    CHECK(doc.ensure_paragraph_style("DuplicateBodyC", duplicate_c));

    auto first_paragraph = doc.paragraphs();
    REQUIRE(first_paragraph.has_next());
    REQUIRE(first_paragraph.add_run("first duplicate style reference").has_next());
    CHECK(doc.set_paragraph_style(first_paragraph, "DuplicateBodyA"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("second");
    CHECK(doc.set_paragraph_style(second_paragraph, "DuplicateBodyA"));
    auto third_paragraph = second_paragraph.insert_paragraph_after("third");
    CHECK(doc.set_paragraph_style(third_paragraph, "DuplicateBodyB"));
    auto fourth_paragraph = third_paragraph.insert_paragraph_after("fourth");
    CHECK(doc.set_paragraph_style(fourth_paragraph, "DuplicateBodyC"));

    const auto plan = doc.suggest_style_merges();
    REQUIRE(plan.has_value());
    CHECK(plan->clean());
    CHECK_EQ(plan->operation_count, 2U);
    CHECK_EQ(plan->applyable_count, 2U);
    CHECK_EQ(plan->issue_count, 0U);
    REQUIRE_EQ(plan->operations.size(), 2U);

    const auto confidence_summary = plan->suggestion_confidence_summary();
    CHECK_EQ(confidence_summary.suggestion_count, 2U);
    CHECK_EQ(confidence_summary.exact_xml_match_count, 1U);
    CHECK_EQ(confidence_summary.xml_difference_count, 1U);
    REQUIRE(confidence_summary.min_confidence.has_value());
    CHECK_EQ(*confidence_summary.min_confidence, 80U);
    REQUIRE(confidence_summary.max_confidence.has_value());
    CHECK_EQ(*confidence_summary.max_confidence, 95U);
    REQUIRE(confidence_summary.recommended_min_confidence.has_value());
    CHECK_EQ(*confidence_summary.recommended_min_confidence, 95U);
    CHECK_NE(confidence_summary.recommendation.find("review lower-confidence"),
             std::string::npos);

    const auto &exact_operation = plan->operations[0];
    CHECK_EQ(exact_operation.action, featherdoc::style_refactor_action::merge);
    CHECK_EQ(exact_operation.source_style_id, "DuplicateBodyB");
    CHECK_EQ(exact_operation.target_style_id, "DuplicateBodyA");
    REQUIRE(exact_operation.source_usage.has_value());
    CHECK_EQ(exact_operation.source_usage->paragraph_count, 1U);
    REQUIRE(exact_operation.suggestion.has_value());
    CHECK_EQ(exact_operation.suggestion->reason_code,
             "matching_style_signature_and_xml");
    CHECK_EQ(exact_operation.suggestion->confidence, 95U);
    CHECK(exact_operation.suggestion->differences.empty());
    CHECK_NE(std::find(exact_operation.suggestion->evidence.begin(),
                       exact_operation.suggestion->evidence.end(),
                       "style_definition_xml_matches"),
             exact_operation.suggestion->evidence.end());
    REQUIRE(exact_operation.target_style.has_value());
    CHECK_EQ(exact_operation.target_style->kind, featherdoc::style_kind::paragraph);

    const auto &diff_operation = plan->operations[1];
    CHECK_EQ(diff_operation.action, featherdoc::style_refactor_action::merge);
    CHECK_EQ(diff_operation.source_style_id, "DuplicateBodyC");
    CHECK_EQ(diff_operation.target_style_id, "DuplicateBodyA");
    REQUIRE(diff_operation.suggestion.has_value());
    CHECK_EQ(diff_operation.suggestion->reason_code,
             "matching_resolved_style_signature");
    CHECK_EQ(diff_operation.suggestion->confidence, 80U);
    CHECK_NE(std::find(diff_operation.suggestion->evidence.begin(),
                       diff_operation.suggestion->evidence.end(),
                       "style_definition_xml_differs"),
             diff_operation.suggestion->evidence.end());
    CHECK_NE(std::find(diff_operation.suggestion->differences.begin(),
                       diff_operation.suggestion->differences.end(), "w:next"),
             diff_operation.suggestion->differences.end());

    CHECK(doc.find_style("DuplicateBodyA").has_value());
    CHECK(doc.find_style("DuplicateBodyB").has_value());
    CHECK(doc.find_style("DuplicateBodyC").has_value());

    fs::remove(target);
}

TEST_CASE("apply_style_refactor applies clean batches and rejects conflicts") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_refactor_apply.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::paragraph_style_definition source_a;
    source_a.name = "Source A";
    source_a.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("SourceA", source_a));

    featherdoc::paragraph_style_definition source_b;
    source_b.name = "Source B";
    source_b.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("SourceB", source_b));

    featherdoc::paragraph_style_definition target_style;
    target_style.name = "Target Style";
    target_style.based_on = "Normal";
    CHECK(doc.ensure_paragraph_style("TargetStyle", target_style));

    auto first_paragraph = doc.paragraphs();
    REQUIRE(first_paragraph.has_next());
    REQUIRE(first_paragraph.add_run("first").has_next());
    CHECK(doc.set_paragraph_style(first_paragraph, "SourceA"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("second");
    CHECK(doc.set_paragraph_style(second_paragraph, "SourceB"));
    auto target_paragraph = second_paragraph.insert_paragraph_after("target");
    CHECK(doc.set_paragraph_style(target_paragraph, "TargetStyle"));

    const auto result = doc.apply_style_refactor(
        {
            {featherdoc::style_refactor_action::rename, "SourceA", "RenamedA"},
            {featherdoc::style_refactor_action::merge, "SourceB", "TargetStyle"},
        });
    REQUIRE(result.has_value());
    CHECK(result->applied());
    CHECK(result->changed);
    CHECK_EQ(result->requested_count, 2U);
    CHECK_EQ(result->applied_count, 2U);
    CHECK_EQ(result->skipped_count(), 0U);
    REQUIRE_EQ(result->rollback_entries.size(), 2U);
    CHECK(result->rollback_entries[0].automatic);
    CHECK_EQ(result->rollback_entries[0].action,
             featherdoc::style_refactor_action::rename);
    CHECK_EQ(result->rollback_entries[0].source_style_id, "RenamedA");
    CHECK_EQ(result->rollback_entries[0].target_style_id, "SourceA");
    CHECK_FALSE(result->rollback_entries[1].automatic);
    CHECK(result->rollback_entries[1].restorable);
    CHECK_EQ(result->rollback_entries[1].action,
             featherdoc::style_refactor_action::merge);
    CHECK_EQ(result->rollback_entries[1].source_style_id, "SourceB");
    CHECK_EQ(result->rollback_entries[1].target_style_id, "TargetStyle");
    CHECK_NE(result->rollback_entries[1].source_style_xml.find(
                 R"(w:styleId="SourceB")"),
             std::string::npos);
    REQUIRE(result->rollback_entries[1].source_usage.has_value());
    CHECK_EQ(result->rollback_entries[1].source_usage->paragraph_count, 1U);
    REQUIRE_EQ(result->rollback_entries[1].source_usage->hits.size(), 1U);
    CHECK_EQ(result->rollback_entries[1].source_usage->hits[0].node_ordinal, 2U);

    CHECK_FALSE(doc.find_style("SourceA").has_value());
    CHECK_FALSE(doc.find_style("SourceB").has_value());
    CHECK(doc.find_style("RenamedA").has_value());
    CHECK(doc.find_style("TargetStyle").has_value());

    const auto renamed_usage = doc.find_style_usage("RenamedA");
    REQUIRE(renamed_usage.has_value());
    CHECK_EQ(renamed_usage->paragraph_count, 1U);
    const auto target_usage = doc.find_style_usage("TargetStyle");
    REQUIRE(target_usage.has_value());
    CHECK_EQ(target_usage->paragraph_count, 2U);

    const auto restore_plan =
        doc.plan_style_refactor_restore({result->rollback_entries[1]});
    REQUIRE(restore_plan.has_value());
    CHECK(restore_plan->restored());
    CHECK_FALSE(restore_plan->changed);
    CHECK(restore_plan->dry_run);
    CHECK_EQ(restore_plan->requested_count, 1U);
    CHECK_EQ(restore_plan->restored_count, 1U);
    CHECK_EQ(restore_plan->issue_count(), 0U);
    CHECK(restore_plan->issue_summary().empty());
    CHECK_EQ(restore_plan->restored_style_count, 1U);
    CHECK_EQ(restore_plan->restored_reference_count, 1U);
    REQUIRE_EQ(restore_plan->operations.size(), 1U);
    CHECK(restore_plan->operations[0].restored);
    CHECK(restore_plan->operations[0].style_restored);
    CHECK_EQ(restore_plan->operations[0].restored_reference_count, 1U);
    CHECK_FALSE(doc.find_style("SourceB").has_value());
    const auto dry_run_target_usage = doc.find_style_usage("TargetStyle");
    REQUIRE(dry_run_target_usage.has_value());
    CHECK_EQ(dry_run_target_usage->paragraph_count, 2U);

    const auto restore = doc.restore_style_refactor({result->rollback_entries[1]});
    REQUIRE(restore.has_value());
    CHECK(restore->restored());
    CHECK(restore->changed);
    CHECK_EQ(restore->requested_count, 1U);
    CHECK_EQ(restore->restored_count, 1U);
    CHECK_EQ(restore->restored_style_count, 1U);
    CHECK_EQ(restore->restored_reference_count, 1U);
    REQUIRE_EQ(restore->operations.size(), 1U);
    CHECK(restore->operations[0].restored);
    CHECK(restore->operations[0].style_restored);
    CHECK_EQ(restore->operations[0].restored_reference_count, 1U);
    CHECK(doc.find_style("SourceB").has_value());
    const auto restored_source_usage = doc.find_style_usage("SourceB");
    REQUIRE(restored_source_usage.has_value());
    CHECK_EQ(restored_source_usage->paragraph_count, 1U);
    const auto restored_target_usage = doc.find_style_usage("TargetStyle");
    REQUIRE(restored_target_usage.has_value());
    CHECK_EQ(restored_target_usage->paragraph_count, 1U);

    const auto restore_conflict_plan =
        doc.plan_style_refactor_restore({result->rollback_entries[1]});
    REQUIRE(restore_conflict_plan.has_value());
    CHECK_FALSE(restore_conflict_plan->restored());
    CHECK(restore_conflict_plan->dry_run);
    CHECK_EQ(restore_conflict_plan->issue_count(), 1U);
    const auto restore_conflict_summary = restore_conflict_plan->issue_summary();
    REQUIRE_EQ(restore_conflict_summary.size(), 1U);
    CHECK_EQ(restore_conflict_summary[0].code, "source_style_exists");
    CHECK_EQ(restore_conflict_summary[0].count, 1U);
    CHECK_NE(restore_conflict_summary[0].suggestion.find(
                 "skip this rollback entry"),
             std::string::npos);
    REQUIRE_EQ(restore_conflict_plan->operations.size(), 1U);
    REQUIRE_EQ(restore_conflict_plan->operations[0].issues.size(), 1U);
    CHECK_EQ(restore_conflict_plan->operations[0].issues[0].code,
             "source_style_exists");
    CHECK_NE(restore_conflict_plan->operations[0].issues[0].suggestion.find(
                 "skip this rollback entry"),
             std::string::npos);

    const auto conflict = doc.apply_style_refactor(
        {
            {featherdoc::style_refactor_action::rename, "TargetStyle", "FinalTarget"},
            {featherdoc::style_refactor_action::merge, "TargetStyle", "RenamedA"},
        });
    REQUIRE(conflict.has_value());
    CHECK_FALSE(conflict->applied());
    CHECK_FALSE(conflict->changed);
    CHECK_EQ(conflict->applied_count, 0U);
    CHECK_EQ(conflict->skipped_count(), 2U);
    CHECK_EQ(conflict->plan.issue_count, 2U);
    CHECK_EQ(conflict->plan.operations[0].issues[0].code,
             "duplicate_source_operation");
    CHECK_EQ(conflict->plan.operations[1].issues[0].code,
             "duplicate_source_operation");
    CHECK(doc.find_style("TargetStyle").has_value());
    CHECK_FALSE(doc.find_style("FinalTarget").has_value());

    fs::remove(target);
}
