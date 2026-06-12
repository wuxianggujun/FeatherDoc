param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Get-RepoFileText {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected contract file was not found: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

$governanceRoutesDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\governance_routes_zh.rst"
$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$currentDirectionDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\current_direction_zh.rst"
$featureGapDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\feature_gap_analysis_zh.rst"
$releaseMetadataDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_pipeline_zh.rst"
$documentApiStatusDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\document_api_mainline_status_zh.rst"
$cmakeLists = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test\cmake") -Filter "*.cmake" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

$writeReviewScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_style_merge_suggestion_review.ps1"
$applyReviewScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\apply_reviewed_style_merge_suggestions.ps1"
$auditRestoreScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\audit_style_merge_restore_plan.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "scripts") -Filter "audit_style_merge_restore_plan_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$calibrationScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_schema_patch_confidence_calibration_report.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "scripts") -Filter "write_schema_patch_confidence_calibration_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

$writeReviewTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\write_style_merge_suggestion_review_test.ps1"
$applyReviewTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\apply_reviewed_style_merge_suggestions_test.ps1"
$auditRestoreTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\audit_style_merge_restore_plan_test.ps1"
$calibrationTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\write_schema_patch_confidence_calibration_report_test.ps1"
$releaseSafetyTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\assert_release_material_safety_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "assert_release_material_safety_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

foreach ($marker in @(
        "suggest-style-merges",
        "style-merge-suggestions.json",
        "style-merge.rollback.json",
        "restore-style-merge",
        "suggestion_confidence_summary",
        "confidence-profile recommended|strict|review|exploratory",
        "--min-confidence <0-100>",
        "--fail-on-suggestion",
        "recommended_min_confidence"
    )) {
    Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText $marker `
        -Message "Governance routes docs should preserve style merge confidence route marker '$marker'."
}

foreach ($marker in @(
        "suggestion_confidence_summary",
        "--confidence-profile recommended|strict|review|exploratory",
        "--min-confidence <0-100>",
        "--fail-on-suggestion",
        "restore-style-merge --dry-run",
        "issue_count / issue_summary"
    )) {
    Assert-ContainsText -Text $currentDirectionDoc -ExpectedText $marker `
        -Message "Current direction document should preserve style merge route marker '$marker'."
}

foreach ($marker in @(
        "suggestion_confidence_summary",
        "--confidence-profile recommended|strict|review|exploratory",
        "--min-confidence <0-100>",
        "--fail-on-suggestion",
        "restore dry-run",
        "--plan-only",
        "--entry",
        "--source-style",
        "--target-style",
        "issue_count",
        "issue_summary"
    )) {
    Assert-ContainsText -Text $featureGapDoc -ExpectedText $marker `
        -Message "Feature gap document should preserve style merge route marker '$marker'."
}

foreach ($marker in @(
        "scripts/audit_style_merge_restore_plan.ps1",
        "scripts/apply_reviewed_style_merge_suggestions.ps1",
        "scripts/write_style_merge_suggestion_review.ps1",
        "test/apply_reviewed_style_merge_suggestions_test.ps1",
        "test/audit_style_merge_restore_plan_test.ps1"
    )) {
    Assert-ContainsText -Text $documentApiStatusDoc -ExpectedText $marker `
        -Message "Document API status should preserve style merge route entry '$marker'."
}

foreach ($marker in @(
        "schema_patch_confidence_calibration",
        "schema-patch-confidence-calibration/summary.json",
        "featherdoc.schema_patch_confidence_calibration_report.v1",
        "schema_patch_confidence_calibration.pending_schema_approvals",
        "schema_patch_confidence_calibration.unscored_candidates",
        "release_blockers",
        "warnings",
        "action_items",
        "source_schema",
        "source_json_display",
        "open_command"
    )) {
    Assert-ContainsText -Text $releaseMetadataDoc -ExpectedText $marker `
        -Message "Release metadata pipeline should preserve calibration route marker '$marker'."
}

foreach ($marker in @(
        "write_schema_patch_confidence_calibration_report.ps1",
        "featherdoc.schema_patch_confidence_calibration_report.v1",
        "confidence",
        "pending approval",
        "schema-patch-confidence-calibration/summary.json",
        "real_corpus_confidence"
    )) {
    Assert-ContainsText -Text $featureGapDoc -ExpectedText $marker `
        -Message "Feature gap document should preserve confidence route marker '$marker'."
}

foreach ($assertion in @(
        [ordered]@{
            text = $writeReviewScript
            markers = @(
                "featherdoc.style_merge_suggestion_review.v1",
                "suggestion_confidence_summary",
                "reviewed_suggestion_count",
                "style_merge_suggestion_count",
                "rollback_plan_file",
                "cannot exceed suggestion count"
            )
            label = "style merge review writer"
        },
        [ordered]@{
            text = $applyReviewScript
            markers = @(
                "featherdoc.style_merge_suggestion_review.v1",
                "reviewed_suggestion_count",
                "style_merge_suggestion_count",
                "suggestion_confidence_summary",
                "rollback_plan_exists",
                "rollback_plan_relative_path"
            )
            label = "reviewed style merge apply script"
        },
        [ordered]@{
            text = $auditRestoreScript
            markers = @(
                "restore-style-merge",
                "--dry-run",
                "--entry",
                "--source-style",
                "--target-style",
                "issue_summary",
                "issue_summary_group_count",
                "issue_summary_groups",
                "selected_entries",
                "selection_summary",
                "requested_count",
                "status_reason",
                "minimum_risk_next_action",
                "minimum_risk_next_action_command",
                "review_handoff_step_count",
                "review_handoff_steps",
                "next_handoff_step",
                "next_copy_command",
                "next_step_reason",
                "issue_review_commands",
                "issue_review_command_count",
                "issue_review_group_summary",
                "first_issue_review_command",
                "copy_issue_review_command",
                "handoff_status_summary",
                "rollback_plan_summary",
                "copy_command",
                "source_report_display",
                "rollback_plan_display",
                "non_restorable_merge_rollback_entry_count",
                "non_restorable_merge_rollback_entry_indexes",
                "restorable_rollback_command_summary",
                "selected_restore_command_template",
                "per_entry_dry_run_commands",
                "per_entry_restore_command_templates",
                "batch_restorable_dry_run_command",
                "batch_restorable_restore_command_template",
                "issue_review_groups",
                "first_review_command",
                "copy_review_command",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "repair_strategy"
            )
            label = "style merge restore audit script"
        },
        [ordered]@{
            text = $calibrationScript
            markers = @(
                "featherdoc.schema_patch_confidence_calibration_report.v1",
                "schema_patch_confidence_calibration.pending_schema_approvals",
                "schema_patch_confidence_calibration.unscored_candidates",
                "recommended_min_confidence",
                "confidence_buckets",
                "release_blockers",
                "warnings",
                "open_command",
                "FailOnPending"
            )
            label = "schema patch confidence calibration script"
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($assertion in @(
        [ordered]@{
            text = $writeReviewTest
            markers = @(
                "style-merge-suggestions.json",
                "style-merge.rollback.json",
                "featherdoc.style_merge_suggestion_review.v1",
                "suggestion_confidence_summary",
                "reviewed_suggestion_count",
                "cannot exceed suggestion count"
            )
            label = "style merge review writer regression"
        },
        [ordered]@{
            text = $applyReviewTest
            markers = @(
                "suggest-style-merges",
                "style-merge-suggestions.json",
                "suggestion_confidence_summary",
                "featherdoc.style_merge_suggestion_review.v1",
                "rollback_plan_exists"
            )
            label = "reviewed style merge apply regression"
        },
        [ordered]@{
            text = $auditRestoreTest
            markers = @(
                "restore-style-merge",
                "dry-run",
                "issue_count",
                "issue_summary",
                "issue_summary_group_count",
                "issue_summary_groups",
                "selection_summary",
                "requested_count",
                "status_reason",
                "minimum_risk_next_action",
                "minimum_risk_next_action_command",
                "review_handoff_step_count",
                "review_handoff_steps",
                "actionItem.review_handoff_steps",
                "blocker.review_handoff_steps",
                "next_handoff_step",
                "next_copy_command",
                "next_step_reason",
                "issue_review_commands",
                "issue_review_command_count",
                "issue_review_group_summary",
                "first_issue_review_command",
                "copy_issue_review_command",
                "handoff_status_summary",
                "rollback_plan_summary",
                "copy_command",
                "source_report_display",
                "rollback_plan_display",
                "non_restorable_merge_rollback_entry_count",
                "non_restorable_merge_rollback_entry_indexes",
                "restorable_rollback_command_summary",
                "selected_restore_command_template",
                "per_entry_dry_run_commands",
                "per_entry_restore_command_templates",
                "batch_restorable_dry_run_command",
                "batch_restorable_restore_command_template",
                "issue_review_groups",
                "first_review_command",
                "copy_review_command",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "repair_strategy",
                "Entry",
                "SourceStyle",
                "TargetStyle"
            )
            label = "style merge restore audit regression"
        },
        [ordered]@{
            text = $calibrationTest
            markers = @(
                "featherdoc.schema_patch_confidence_calibration_report.v1",
                "recommended_min_confidence",
                "confidence_buckets",
                "schema_patch_confidence_calibration.pending_schema_approvals",
                "schema_patch_confidence_calibration.unscored_candidates",
                "source_schema",
                "open_command:",
                "-FailOnPending"
            )
            label = "schema patch confidence calibration regression"
        },
        [ordered]@{
            text = $releaseSafetyTest
            markers = @(
                "real_corpus_confidence",
                "numbering_catalog_governance.real_corpus_confidence",
                "numbering_catalog_real_corpus_confidence"
            )
            label = "release material safety confidence regression"
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($marker in @(
        "style_merge_confidence_route_docs_contract",
        "style_merge_confidence_route_docs_contract_test.ps1",
        "write_style_merge_suggestion_review_valid",
        "write_style_merge_suggestion_review_invalid_count",
        "write_style_merge_suggestion_review_test.ps1",
        "apply_reviewed_style_merge_suggestions_valid",
        "apply_reviewed_style_merge_suggestions_pending_review",
        "apply_reviewed_style_merge_suggestions_partial_review",
        "apply_reviewed_style_merge_suggestions_test.ps1",
        "audit_style_merge_restore_plan_clean",
        "audit_style_merge_restore_plan_issue",
        "audit_style_merge_restore_plan_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;style"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep style merge confidence route contract wired."
}

Write-Host "Style merge confidence route docs contract passed."
