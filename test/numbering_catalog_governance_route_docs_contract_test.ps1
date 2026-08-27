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
$scriptRoot = Join-Path $resolvedRepoRoot "scripts"

$governanceRoutesDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\governance_routes_zh.rst"
$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$featureGapDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\feature_gap_analysis_zh.rst"
$nextTasksDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\next_tasks_zh.rst"
$currentDirectionDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\current_direction_zh.rst"
$longTaskBoardDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\long_task_board_zh.rst"
$releaseMetadataDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_pipeline_zh.rst"
$cmakeLists = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test\cmake") -Filter "*.cmake" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

$numberingGovernanceScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_numbering_catalog_governance_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_numbering_catalog_governance_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releasePipelineScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_pipeline_report.ps1"
$releaseHandoffScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_handoff_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_release_governance_handoff_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseRollupScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_blocker_rollup_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_release_blocker_rollup_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releasePackageScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\package_release_assets.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "package_release_assets_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseSafetyScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\assert_release_material_safety.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "assert_release_material_safety_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

$numberingGovernanceTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_numbering_catalog_governance_report_test.ps1"
$metricsContractTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_governance_metrics_contract_test.ps1"
$releaseSafetyTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\assert_release_material_safety_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "assert_release_material_safety_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

foreach ($marker in @(
        "scripts/build_numbering_catalog_governance_report.ps1",
        "output/numbering-catalog-governance/summary.json",
        "featherdoc.numbering_catalog_governance_report.v1",
        "featherdoc.document_skeleton_governance_rollup_report.v1",
        "numbering_catalog_governance.real_corpus_confidence",
        "real_corpus_confidence",
        "real_corpus_alignment",
        "missing_baseline",
        "missing_exemplar",
        "numbering_catalog_governance.missing_baseline",
        "numbering_catalog_governance.missing_exemplar",
        "numbering_catalog_governance.real_corpus_alignment_gap",
        "exemplar_conflict_count",
        "exemplar_conflicts",
        "numbering_catalog_governance.exemplar_catalog_conflict",
        "review_numbering_catalog_exemplar_conflict",
        "featherdoc.numbering_catalog_governance_patch_plan.v1",
        "catalog_patch_plan_count",
        "catalog_patch_plans",
        "catalog_patch_plan_id",
        "catalog_patch_plan",
        "awaiting_authoritative_catalog",
        "safe_to_apply",
        "automatic_patch_available",
        "patch_apply_supported",
        "manual_review_required",
        "requires_authoritative_catalog_selection",
        "candidate_catalog_count",
        "candidate_catalog_paths",
        "candidate_catalog_displays",
        "reviewer_inputs",
        "supported_patch_operations",
        "upsert_levels",
        "upsert_overrides",
        "remove_overrides",
        "unsupported_automatic_changes",
        "definition_topology_changes",
        "instance_topology_changes",
        "unsupported_change_count",
        "patch_counts",
        "diff_commands",
        "review_command",
        "patch_command_template",
        "lint_command_template",
        "verification_command_template",
        "required_steps",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command"
    )) {
    Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText $marker `
        -Message "Governance routes docs should preserve numbering governance route marker '$marker'."
}

foreach ($marker in @(
        "numbering catalog governance",
        "build_numbering_catalog_governance_report.ps1",
        "output/numbering-catalog-governance/summary.json",
        "featherdoc.numbering_catalog_governance_report.v1",
        "ReleaseBlockerRollupAutoDiscover"
    )) {
    Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText $marker `
        -Message "Governance routes docs should preserve numbering governance release route marker '$marker'."
}

foreach ($marker in @(
        "build_numbering_catalog_governance_report.ps1",
        "numbering_catalog_governance",
        "numbering_catalog_governance.real_corpus_confidence",
        "real_corpus_confidence",
        "real_corpus_alignment",
        "exemplar_conflict_count",
        "exemplar_conflicts",
        "numbering_catalog_governance.exemplar_catalog_conflict",
        "review_numbering_catalog_exemplar_conflict",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command"
    )) {
    Assert-ContainsText -Text $featureGapDoc -ExpectedText $marker `
        -Message "Feature gap document should preserve numbering governance marker '$marker'."
}

foreach ($marker in @(
        "featherdoc.numbering_catalog_governance_report.v1",
        "output/numbering-catalog-governance/summary.json",
        "numbering_catalog_governance.real_corpus_alignment_gap",
        "exemplar_conflict_count",
        "exemplar_conflicts",
        "numbering_catalog_governance.exemplar_catalog_conflict",
        "review_numbering_catalog_exemplar_conflict",
        "numbering_catalog_governance.real_corpus_confidence",
        "real_corpus_confidence",
        "real_corpus_alignment",
        "numbering_catalog_governance.missing_baseline",
        "numbering_catalog_governance.missing_exemplar",
        "numbering_catalog_real_corpus_confidence",
        "catalog_coverage_percent",
        "baseline_coverage_percent",
        "matched_document_count",
        "penalty_summary",
        "numbering_catalog_governance",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command",
        "review_numbering_catalog_real_corpus_alignment",
        "fix_numbering_catalog_baseline_lint",
        "refresh_numbering_catalog_baseline_or_repair_docx",
        "review_numbering_catalog_check_issues",
        "rebuild_document_skeleton_governance_rollup",
        "rebuild_numbering_catalog_manifest_summary",
        "review_numbering_catalog_governance_sources",
        "review_style_numbering_audit",
        "preview_style_numbering_repair",
        "promote_numbering_catalog_exemplar",
        "register_numbering_catalog_baseline",
        "rerun_document_skeleton_governance_report",
        "reviewer runbook",
        "build_document_skeleton_governance_report.ps1",
        "build_document_skeleton_governance_rollup_report.ps1",
        "check_numbering_catalog_baseline.ps1",
        "check_numbering_catalog_manifest.ps1"
    )) {
    Assert-ContainsText -Text $releaseMetadataDoc -ExpectedText $marker `
        -Message "Release metadata document should preserve numbering governance marker '$marker'."
}

$catalogPatchPlanDocMarkers = @(
    "featherdoc.numbering_catalog_governance_patch_plan.v1",
    "catalog_patch_plan_count",
    "catalog_patch_plans",
    "catalog_patch_plan_id",
    "catalog_patch_plan",
    "awaiting_authoritative_catalog",
    "safe_to_apply",
    "automatic_patch_available",
    "manual_review_required",
    "requires_authoritative_catalog_selection",
    "upsert_levels",
    "upsert_overrides",
    "remove_overrides",
    "required_steps"
)

foreach ($assertion in @(
        [ordered]@{ text = $governanceRoutesDoc; label = "governance routes patch plan docs" }
        [ordered]@{ text = $featureGapDoc; label = "feature gap patch plan docs" }
        [ordered]@{ text = $nextTasksDoc; label = "next tasks patch plan docs" }
        [ordered]@{ text = $currentDirectionDoc; label = "current direction patch plan docs" }
        [ordered]@{ text = $longTaskBoardDoc; label = "long task board patch plan docs" }
        [ordered]@{ text = $releaseMetadataDoc; label = "release metadata patch plan docs" }
    )) {
    foreach ($marker in $catalogPatchPlanDocMarkers) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve structured catalog patch plan marker '$marker'."
    }
}

foreach ($marker in @(
        "release_blocker_rollup Markdown",
        "release_governance_handoff Markdown",
        "release_governance_pipeline Markdown",
        "patch_command_template",
        "lint_command_template",
        "verification_command_template",
        "required_steps"
    )) {
    Assert-ContainsText -Text $releaseMetadataDoc -ExpectedText $marker `
        -Message "Release metadata docs should preserve downstream catalog patch plan Markdown marker '$marker'."
}

foreach ($assertion in @(
        [ordered]@{
            text = $featureGapDoc
            label = "feature gap patch plan review contract"
        }
        [ordered]@{
            text = $nextTasksDoc
            label = "next tasks patch plan review contract"
        }
        [ordered]@{
            text = $currentDirectionDoc
            label = "current direction patch plan review contract"
        }
        [ordered]@{
            text = $longTaskBoardDoc
            label = "long task board patch plan review contract"
        }
        [ordered]@{
            text = $releaseMetadataDoc
            label = "release metadata patch plan review contract"
        }
    )) {
    foreach ($marker in @(
            "patch_apply_supported",
            "candidate_catalog_count",
            "candidate_catalog_paths",
            "candidate_catalog_displays",
            "reviewer_inputs",
            "supported_patch_operations",
            "unsupported_automatic_changes",
            "definition_topology_changes",
            "instance_topology_changes",
            "patch_counts",
            "diff_commands",
            "review_command",
            "patch_command_template",
            "lint_command_template",
            "verification_command_template",
            "authoritative catalog",
            "reviewed patch",
            "--fail-on-diff"
        )) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve patch review marker '$marker'."
    }
}

foreach ($assertion in @(
        [ordered]@{
            text = $numberingGovernanceScript
            label = "numbering catalog governance report script"
            markers = @(
                "featherdoc.numbering_catalog_governance_report.v1",
                "featherdoc.document_skeleton_governance_rollup_report.v1",
                "featherdoc.numbering_catalog_manifest_summary.v1",
                "numbering_catalog_governance.real_corpus_alignment_gap",
                "numbering_catalog_governance.exemplar_catalog_conflict",
                "featherdoc.numbering_catalog_governance_patch_plan.v1",
                "catalog_patch_plan_count",
                "catalog_patch_plans",
                "catalog_patch_plan_id",
                "catalog_patch_plan",
                "awaiting_authoritative_catalog",
                "safe_to_apply",
                "automatic_patch_available",
                "patch_apply_supported",
                "manual_review_required",
                "requires_authoritative_catalog_selection",
                "candidate_catalog_count",
                "candidate_catalog_paths",
                "candidate_catalog_displays",
                "reviewer_inputs",
                "supported_patch_operations",
                "upsert_levels",
                "upsert_overrides",
                "remove_overrides",
                "unsupported_automatic_changes",
                "definition_topology_changes",
                "instance_topology_changes",
                "unsupported_change_count",
                "patch_counts",
                "diff_commands",
                "review_command",
                "patch_command_template",
                "lint_command_template",
                "verification_command_template",
                "required_steps",
                "numbering_catalog_governance.dirty_baseline",
                "numbering_catalog_governance.catalog_drift",
                "numbering_catalog_governance.catalog_check_issue",
                "real_corpus_confidence_score",
                "real_corpus_confidence_level",
                "real_corpus_confidence",
                "real_corpus_alignment_count",
                "real_corpus_alignment_gap_count",
                "real_corpus_alignment",
                "exemplar_conflict_count",
                "exemplar_conflicts",
                "numbering_catalog_governance.missing_baseline",
                "numbering_catalog_governance.missing_exemplar",
                "matched_document_count",
                "unmatched_catalog_document_count",
                "unmatched_baseline_document_count",
                "alignment_gap_count",
                "catalog_document_keys",
                "baseline_document_keys",
                "matched_document_keys",
                "penalty_summary",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "open_command",
                "review_numbering_catalog_real_corpus_alignment",
                "review_numbering_catalog_exemplar_conflict",
                "fix_numbering_catalog_baseline_lint",
                "refresh_numbering_catalog_baseline_or_repair_docx",
                "review_numbering_catalog_check_issues",
                "rebuild_document_skeleton_governance_rollup",
                "rebuild_numbering_catalog_manifest_summary",
                "review_numbering_catalog_governance_sources"
            )
        },
        [ordered]@{
            text = $releasePipelineScript
            label = "release governance pipeline"
            markers = @(
                "numbering_catalog_governance",
                "numbering-catalog-governance",
                "build_numbering_catalog_governance_report.ps1",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "open_command",
                "catalog_patch_plan_id",
                "catalog_patch_plan"
            )
        },
        [ordered]@{
            text = $releaseHandoffScript
            label = "release governance handoff"
            markers = @(
                "numbering_catalog_governance",
                "numbering-catalog-governance",
                "featherdoc.numbering_catalog_governance_report.v1",
                "numbering_catalog_governance.real_corpus_confidence",
                "real_corpus_confidence",
                "source_report_display",
                "source_json_display",
                "open_command",
                "catalog_patch_plan_id",
                "catalog_patch_plan"
            )
        },
        [ordered]@{
            text = $releaseRollupScript
            label = "release blocker rollup"
            markers = @(
                "featherdoc.numbering_catalog_governance_report.v1",
                "numbering_catalog_governance",
                "numbering_catalog_governance.real_corpus_confidence",
                "real_corpus_confidence",
                "matched_document_count",
                "source_report_display",
                "source_json_display",
                "open_command",
                "catalog_patch_plan_id",
                "catalog_patch_plan"
            )
        },
        [ordered]@{
            text = $releasePackageScript
            label = "release asset package script"
            markers = @(
                "numbering_catalog_governance.real_corpus_confidence",
                "numbering_catalog_real_corpus_confidence",
                "featherdoc.numbering_catalog_governance_report.v1",
                "real_corpus_confidence",
                "source_schema"
            )
        },
        [ordered]@{
            text = $releaseSafetyScript
            label = "release material safety audit"
            markers = @(
                "numbering_catalog_governance.real_corpus_confidence",
                "numbering_catalog_real_corpus_confidence",
                "featherdoc.numbering_catalog_governance_report.v1",
                "matched_document_count",
                "unmatched_catalog_document_count",
                "unmatched_baseline_document_count",
                "alignment_gap_count",
                "catalog_document_keys",
                "baseline_document_keys",
                "matched_document_keys",
                "source_schema"
            )
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($assertion in @(
        [ordered]@{
            text = $numberingGovernanceTest
            label = "numbering catalog governance regression"
            markers = @(
                "featherdoc.numbering_catalog_governance_report.v1",
                "real_corpus_confidence_score",
                "real_corpus_confidence_level",
                "real_corpus_confidence",
                "matched_document_count",
                "exemplar_conflict_count",
                "exemplar_conflicts",
                "numbering_catalog_governance.exemplar_catalog_conflict",
                "review_numbering_catalog_exemplar_conflict",
                "featherdoc.numbering_catalog_governance_patch_plan.v1",
                "catalog_patch_plan_count",
                "catalog_patch_plans",
                "catalog_patch_plan_id",
                "catalog_patch_plan",
                "awaiting_authoritative_catalog",
                "safe_to_apply",
                "automatic_patch_available",
                "patch_apply_supported",
                "manual_review_required",
                "requires_authoritative_catalog_selection",
                "candidate_catalog_count",
                "candidate_catalog_paths",
                "candidate_catalog_displays",
                "supported_patch_operations",
                "upsert_levels",
                "upsert_overrides",
                "remove_overrides",
                "unsupported_automatic_changes",
                "definition_topology_changes",
                "instance_topology_changes",
                "diff_commands",
                "review_command",
                "patch_command_template",
                "lint_command_template",
                "verification_command_template",
                "required_steps",
                "numbering_catalog_governance.real_corpus_alignment_gap",
                "source_schema",
                "source_report_display",
                "source_json_display"
            )
        },
        [ordered]@{
            text = $metricsContractTest
            label = "release governance metrics contract"
            markers = @(
                "real_corpus_confidence",
                "numbering_catalog_governance.real_corpus_alignment_gap",
                "matched_document_count",
                "catalog_document_keys",
                "baseline_document_keys"
            )
        },
        [ordered]@{
            text = $releaseSafetyTest
            label = "release material safety regression"
            markers = @(
                "numbering_catalog_governance.real_corpus_confidence",
                "numbering_catalog_real_corpus_confidence",
                "featherdoc.numbering_catalog_governance_report.v1",
                "matched_document_count",
                "catalog_document_keys",
                "baseline_document_keys",
                "penalty_summary"
            )
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($marker in @(
        "numbering_catalog_governance_route_docs_contract",
        "numbering_catalog_governance_route_docs_contract_test.ps1",
        "build_numbering_catalog_governance_report_exemplar_conflict",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;numbering"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep numbering governance route contract wired."
}

Write-Host "Numbering catalog governance route docs contract passed."
