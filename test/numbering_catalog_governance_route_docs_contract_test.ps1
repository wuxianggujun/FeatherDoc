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
$releaseMetadataDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_pipeline_zh.rst"
$cmakeLists = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test\cmake") -Filter "*.cmake" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

$numberingGovernanceScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_numbering_catalog_governance_report.ps1"
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
$releasePackageScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\package_release_assets.ps1"
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
        "numbering_catalog_governance.real_corpus_alignment_gap",
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
        "numbering_catalog_governance.real_corpus_confidence",
        "real_corpus_confidence",
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

foreach ($assertion in @(
        [ordered]@{
            text = $numberingGovernanceScript
            label = "numbering catalog governance report script"
            markers = @(
                "featherdoc.numbering_catalog_governance_report.v1",
                "featherdoc.document_skeleton_governance_rollup_report.v1",
                "featherdoc.numbering_catalog_manifest_summary.v1",
                "numbering_catalog_governance.real_corpus_alignment_gap",
                "numbering_catalog_governance.dirty_baseline",
                "numbering_catalog_governance.catalog_drift",
                "numbering_catalog_governance.catalog_check_issue",
                "real_corpus_confidence_score",
                "real_corpus_confidence_level",
                "real_corpus_confidence",
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
                "open_command"
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
                "open_command"
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
                "open_command"
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
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;numbering"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep numbering governance route contract wired."
}

Write-Host "Numbering catalog governance route docs contract passed."
