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

$calibrationScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_schema_patch_confidence_calibration_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "write_schema_patch_confidence_calibration_report_*.ps1" |
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

$calibrationTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\write_schema_patch_confidence_calibration_report_test.ps1"
$releaseRollupTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_blocker_rollup_report_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "build_release_blocker_rollup_report_*.ps1" |
        Where-Object { $_.Name -ne "build_release_blocker_rollup_report_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releasePipelineTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_governance_pipeline_report_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "build_release_governance_pipeline_report_test_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseHandoffTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_governance_handoff_report_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "build_release_governance_handoff_report_*.ps1" |
        Where-Object { $_.Name -ne "build_release_governance_handoff_report_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

foreach ($marker in @(
        "write_schema_patch_confidence_calibration_report.ps1",
        "schema-patch-confidence-calibration",
        "featherdoc.schema_patch_confidence_calibration_report.v1",
        "confidence buckets",
        "approval outcomes",
        "recommended_min_confidence",
        "release blocker rollup",
        "-FailOnPending"
    )) {
    Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText $marker `
        -Message "Governance routes docs should preserve schema patch confidence calibration route marker '$marker'."
}

foreach ($marker in @(
        "scripts/write_schema_patch_confidence_calibration_report.ps1",
        "output/schema-patch-confidence-calibration",
        "schema-patch-confidence-calibration/summary.json",
        "schema_patch_confidence_calibration.md",
        "featherdoc.schema_patch_confidence_calibration_report.v1",
        "confidence buckets",
        "approval outcomes",
        "recommended_min_confidence",
        "release_blockers",
        "warnings",
        "action_items",
        "source_report_display",
        "source_json_display",
        "open_command"
    )) {
    Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText $marker `
        -Message "Governance routes docs should preserve schema calibration route marker '$marker'."
}

foreach ($marker in @(
        "schema patch confidence calibration",
        "schema-patch-confidence-calibration/summary.json",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command",
        "confidence calibration",
        "release governance pipeline"
    )) {
    Assert-ContainsText -Text $currentDirectionDoc -ExpectedText $marker `
        -Message "Current direction document should preserve schema calibration marker '$marker'."
}

foreach ($marker in @(
        "write_schema_patch_confidence_calibration_report.ps1",
        "featherdoc.schema_patch_confidence_calibration_report.v1",
        "pending approval",
        "schema-patch-confidence-calibration/summary.json",
        "release blocker auto-discovery",
        "source_report_display",
        "source_json_display",
        "open_command",
        "summary JSON"
    )) {
    Assert-ContainsText -Text $featureGapDoc -ExpectedText $marker `
        -Message "Feature gap document should preserve schema calibration marker '$marker'."
}

foreach ($marker in @(
        "featherdoc.schema_patch_confidence_calibration_report.v1",
        "schema_patch_confidence_calibration.pending_schema_approvals",
        "schema_patch_confidence_calibration.unscored_candidates",
        "schema_patch_confidence_calibration.invalid_approval_records",
        "schema-patch-confidence-calibration/summary.json",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command"
    )) {
    Assert-ContainsText -Text $releaseMetadataDoc -ExpectedText $marker `
        -Message "Release metadata pipeline should preserve schema calibration marker '$marker'."
}

foreach ($assertion in @(
        [ordered]@{
            text = $calibrationScript
            label = "schema patch confidence calibration script"
            markers = @(
                "featherdoc.schema_patch_confidence_calibration_report.v1",
                "schema_patch_confidence_calibration.pending_schema_approvals",
                "schema_patch_confidence_calibration.unscored_candidates",
                "schema_patch_confidence_calibration.invalid_approval_records",
                "recommended_min_confidence",
                "confidence_buckets",
                "release_blockers",
                "warnings",
                "action_items",
                "source_report_display",
                "source_json_display",
                "open_command",
                "FailOnPending"
            )
        },
        [ordered]@{
            text = $releasePipelineScript
            label = "release governance pipeline script"
            markers = @(
                "schema_patch_confidence_calibration",
                "schema-patch-confidence-calibration",
                "write_schema_patch_confidence_calibration_report.ps1",
                "source_report_display",
                "source_json_display",
                "open_command"
            )
        },
        [ordered]@{
            text = $releaseHandoffScript
            label = "release governance handoff script"
            markers = @(
                "featherdoc.schema_patch_confidence_calibration_report.v1",
                "schema_patch_confidence_calibration",
                "schema-patch-confidence-calibration/summary.json",
                "write_schema_patch_confidence_calibration_report.ps1",
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
            text = $calibrationTest
            label = "schema patch confidence calibration regression"
            markers = @(
                "featherdoc.schema_patch_confidence_calibration_report.v1",
                "recommended_min_confidence",
                "confidence_buckets",
                "schema_patch_confidence_calibration.pending_schema_approvals",
                "schema_patch_confidence_calibration.unscored_candidates",
                "schema_patch_confidence_calibration.invalid_approval_records",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "open_command:",
                "-FailOnPending"
            )
        },
        [ordered]@{
            text = $releaseRollupTest
            label = "release blocker rollup regression"
            markers = @(
                "featherdoc.schema_patch_confidence_calibration_report.v1",
                "schema_patch_confidence_calibration.pending_schema_approvals",
                "schema_patch_confidence_calibration.unscored_candidates",
                "resolve_pending_schema_approvals",
                "schema-patch-confidence-calibration\summary.json",
                "origin_source_report_display"
            )
        },
        [ordered]@{
            text = $releasePipelineTest
            label = "release governance pipeline regression"
            markers = @(
                "schema_patch_confidence_calibration",
                "featherdoc.schema_patch_confidence_calibration_report.v1",
                "schema-patch-confidence-calibration\summary.json",
                "write_schema_patch_confidence_calibration_report.ps1",
                "source_report_display",
                "source_json_display",
                "open_command"
            )
        },
        [ordered]@{
            text = $releaseHandoffTest
            label = "release governance handoff regression"
            markers = @(
                "schema_patch_confidence_calibration",
                "featherdoc.schema_patch_confidence_calibration_report.v1",
                "schema_patch_confidence_calibration.pending_schema_approvals",
                "schema_patch_confidence_calibration.unscored_candidates",
                "resolve_pending_schema_approvals",
                "schema-patch-confidence-calibration\summary.json",
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
        "schema_patch_confidence_calibration_route_docs_contract",
        "schema_patch_confidence_calibration_route_docs_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;schema-calibration"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep schema calibration route contract wired."
}

Write-Host "Schema patch confidence calibration route docs contract passed."
