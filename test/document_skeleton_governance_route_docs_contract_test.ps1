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
$currentDirectionDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\current_direction_zh.rst"
$featureGapDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\feature_gap_analysis_zh.rst"
$releaseMetadataDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_pipeline_zh.rst"
$cmakeLists = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test\cmake") -Filter "*.cmake" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

$singleReportScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_document_skeleton_governance_report.ps1"
$rollupReportScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_document_skeleton_governance_rollup_report.ps1"
$releaseRollupScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_blocker_rollup_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_release_blocker_rollup_report_*.ps1" |
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

$singleReportTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_document_skeleton_governance_report_test.ps1"
$rollupReportTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_document_skeleton_governance_rollup_report_test.ps1"
$releaseRollupTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_blocker_rollup_report_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "build_release_blocker_rollup_report_*.ps1" |
        Where-Object { $_.Name -ne "build_release_blocker_rollup_report_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$warningHelperTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_governance_warning_helper_contract_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "release_governance_warning_helper_contract_test_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

foreach ($marker in @(
        "scripts/build_document_skeleton_governance_report.ps1",
        "scripts/build_document_skeleton_governance_rollup_report.ps1",
        "output/document-skeleton-governance",
        "output/document-skeleton-governance-rollup",
        "featherdoc.document_skeleton_governance_report.v1",
        "featherdoc.document_skeleton_governance_rollup_report.v1",
        "document_skeleton_governance.summary.json",
        "summary.json",
        "exemplar.numbering-catalog.json",
        "style-merge-suggestions.json",
        "style_numbering_issue_count",
        "style_merge_suggestion_count",
        "style_merge_manual_review_reason_count",
        "manual_review_reasons",
        "manual_review_before_apply",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command"
    )) {
    Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText $marker `
        -Message "Governance routes docs should preserve document skeleton governance route marker '$marker'."
}

foreach ($marker in @(
        "build_document_skeleton_governance_report.ps1",
        "build_document_skeleton_governance_rollup_report.ps1",
        "featherdoc.document_skeleton_governance_rollup_report.v1",
        "document skeleton governance",
        "exemplar catalog",
        "release blockers",
        "action items",
        "build_release_blocker_rollup_report.ps1"
    )) {
    Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText $marker `
        -Message "Governance routes docs should preserve document skeleton governance route marker '$marker'."
}

foreach ($marker in @(
        "build_document_skeleton_governance_report.ps1",
        "build_document_skeleton_governance_rollup_report.ps1",
        "featherdoc.document_skeleton_governance_rollup_report.v1",
        "release_blocker_rollup.release_blockers",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command",
        "numbering_catalog_governance"
    )) {
    Assert-ContainsText -Text $currentDirectionDoc -ExpectedText $marker `
        -Message "Current direction document should preserve skeleton governance marker '$marker'."
}

foreach ($marker in @(
        "build_document_skeleton_governance_report.ps1",
        "build_document_skeleton_governance_rollup_report.ps1",
        "featherdoc.document_skeleton_governance_rollup_report.v1",
        "build_numbering_catalog_governance_report.ps1",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command"
    )) {
    Assert-ContainsText -Text $featureGapDoc -ExpectedText $marker `
        -Message "Feature gap document should preserve skeleton governance marker '$marker'."
}

foreach ($marker in @(
        "featherdoc.document_skeleton_governance_rollup_report.v1",
        "document_skeleton.style_merge_suggestions_pending",
        "style_merge_suggestion_count",
        "style_merge_manual_review_reason_count",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command",
        "release_blocker_rollup"
    )) {
    Assert-ContainsText -Text $releaseMetadataDoc -ExpectedText $marker `
        -Message "Release metadata document should preserve skeleton governance marker '$marker'."
}

foreach ($assertion in @(
        [ordered]@{
            text = $singleReportScript
            label = "document skeleton governance report script"
            markers = @(
                "featherdoc.document_skeleton_governance_report.v1",
                "document_skeleton_governance.summary.json",
                "document_skeleton_governance.md",
                "exemplar.numbering-catalog.json",
                "style-merge-suggestions.json",
                "StyleMergeReviewJson",
                "export-numbering-catalog",
                "audit-style-numbering",
                "inspect-styles",
                "suggest-style-merges",
                "style_merge_review_json_path",
                "style_merge_manual_review_reason_count",
                "manual_review_reasons",
                "document_skeleton.style_numbering_issues",
                "review_style_numbering_audit",
                "preview_style_numbering_repair",
                "promote_numbering_catalog_exemplar",
                "register_numbering_catalog_baseline"
            )
        },
        [ordered]@{
            text = $rollupReportScript
            label = "document skeleton governance rollup script"
            markers = @(
                "featherdoc.document_skeleton_governance_rollup_report.v1",
                "document_skeleton_governance_rollup.md",
                "source_report_count",
                "document_entries",
                "catalog_exemplars",
                "total_style_numbering_issue_count",
                "total_style_merge_suggestion_count",
                "total_style_merge_manual_review_reason_count",
                "style_merge_manual_review_reason_count",
                "manual_review_reasons",
                "release_blockers",
                "action_items",
                "warnings",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "origin_source_report_display",
                "open_command",
                "source_report_read_failed",
                "source_report_schema_skipped",
                "release_blocker_count_mismatch"
            )
        },
        [ordered]@{
            text = $releaseRollupScript
            label = "release blocker rollup"
            markers = @(
                "source_report_display",
                "source_json_display",
                "origin_source_report_display",
                "open_command"
            )
        },
        [ordered]@{
            text = $releasePipelineScript
            label = "release governance pipeline"
            markers = @(
                "document-skeleton-governance-rollup",
                "numbering_catalog_governance",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "open_command"
            )
        },
        [ordered]@{
            text = $releaseHandoffScript
            label = "release governance handoff"
            markers = @(
                "document-skeleton-governance-rollup",
                "source_report_display",
                "source_json_display",
                "open_command"
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
            text = $singleReportTest
            label = "document skeleton governance report regression"
            markers = @(
                "exemplar.numbering-catalog.json",
                "style-merge-suggestions.json",
                "style_merge_suggestion_count",
                "style_merge_manual_review_reason_count",
                "manual_review_before_apply",
                "repair-style-numbering",
                "suggest-style-merges"
            )
        },
        [ordered]@{
            text = $rollupReportTest
            label = "document skeleton governance rollup regression"
            markers = @(
                "featherdoc.document_skeleton_governance_rollup_report.v1",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "origin_source_report_display",
                "open_command",
                "style_merge_suggestions",
                "style_merge_manual_review_reason_count",
                "manual_review_reasons",
                "manual_review_before_apply"
            )
        },
        [ordered]@{
            text = $releaseRollupTest
            label = "release blocker rollup regression"
            markers = @(
                "featherdoc.document_skeleton_governance_rollup_report.v1",
                "document_skeleton.style_numbering_issues",
                "style-numbering-audit.json",
                "origin_source_report_display",
                "repair-style-numbering"
            )
        },
        [ordered]@{
            text = $warningHelperTest
            label = "release governance warning helper contract"
            markers = @(
                "document_skeleton.style_merge_suggestions_pending",
                "review_style_merge_suggestions",
                "featherdoc.document_skeleton_governance_rollup_report.v1",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "open_command"
            )
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($marker in @(
        "document_skeleton_governance_route_docs_contract",
        "document_skeleton_governance_route_docs_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;document-skeleton"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep document skeleton governance route contract wired."
}

Write-Host "Document skeleton governance route docs contract passed."
