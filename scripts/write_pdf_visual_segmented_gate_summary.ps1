param(
    [string]$RepoRoot = "",
    [string]$ReportDir = "",
    [string]$OutputJson = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

$resolvedRepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($ReportDir)) {
    $ReportDir = Join-Path $resolvedRepoRoot "output\pdf-visual-release-gate-current\report"
}
$resolvedReportDir = [System.IO.Path]::GetFullPath($ReportDir)

if ([string]::IsNullOrWhiteSpace($OutputJson)) {
    $OutputJson = Join-Path $resolvedReportDir "segmented-summary.json"
}
$resolvedOutputJson = [System.IO.Path]::GetFullPath($OutputJson)

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) {
        return $null
    }
    if ($Object -is [System.Collections.IDictionary] -and $Object.Contains($Name)) {
        return $Object[$Name]
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Get-JsonString {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) {
        return ""
    }

    return [string]$value
}

function Get-JsonInt {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return 0
    }

    return [int]$value
}

function Get-RepoRelativePath {
    param(
        [string]$Root,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $rootPath = [System.IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    if ($fullPath.StartsWith($rootPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relativePath = $fullPath.Substring($rootPath.Length).TrimStart('\', '/')
        return ".\" + $relativePath
    }

    return $fullPath
}

function Read-JsonFile {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Get-SummaryStatus {
    param($Summary)

    if ($null -eq $Summary) {
        return "missing"
    }

    $status = Get-JsonString -Object $Summary -Name "status"
    $verdict = Get-JsonString -Object $Summary -Name "verdict"
    if ($status -eq "pass" -and $verdict -eq "pass") {
        return "pass"
    }
    if ($status -eq "fail" -or $verdict -eq "fail") {
        return "fail"
    }

    return "not_complete"
}

$attemptSummaryPath = Join-Path $resolvedReportDir "attempt-summary.json"
$aggregateSummaryPath = Join-Path $resolvedReportDir "aggregate-contact-sheet-rebuild-summary.json"
$sliceFiles = @()
if (Test-Path -LiteralPath $resolvedReportDir) {
    $sliceFiles = @(Get-ChildItem -LiteralPath $resolvedReportDir -Filter "visual-baseline-slice-*-summary.json" -File |
        Sort-Object Name)
}

$attemptSummary = Read-JsonFile -Path $attemptSummaryPath
$aggregateSummary = Read-JsonFile -Path $aggregateSummaryPath

$sliceSummaries = @(
    foreach ($sliceFile in $sliceFiles) {
        $sliceSummary = Read-JsonFile -Path $sliceFile.FullName
        $sliceContactSheet = Get-JsonString -Object $sliceSummary -Name "slice_contact_sheet"
        [ordered]@{
            summary_json = $sliceFile.FullName
            summary_json_display = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $sliceFile.FullName
            status = Get-JsonString -Object $sliceSummary -Name "status"
            verdict = Get-JsonString -Object $sliceSummary -Name "verdict"
            full_visual_gate_status = Get-JsonString -Object $sliceSummary -Name "full_visual_gate_status"
            evidence_scope = Get-JsonString -Object $sliceSummary -Name "evidence_scope"
            visual_baseline_offset = Get-JsonInt -Object $sliceSummary -Name "visual_baseline_offset"
            visual_baseline_limit = Get-JsonInt -Object $sliceSummary -Name "visual_baseline_limit"
            selected_baseline_count = Get-JsonInt -Object $sliceSummary -Name "selected_baseline_count"
            total_baseline_count = Get-JsonInt -Object $sliceSummary -Name "total_baseline_count"
            expected_visual_render_count = Get-JsonInt -Object $sliceSummary -Name "expected_visual_render_count"
            slice_contact_sheet = $sliceContactSheet
            slice_contact_sheet_display = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $sliceContactSheet
        }
    }
)

$slicePassCount = @($sliceSummaries | Where-Object { $_.status -eq "pass" -and $_.verdict -eq "pass" }).Count
$sliceFailedCount = @($sliceSummaries | Where-Object { $_.status -eq "fail" -or $_.verdict -eq "fail" }).Count
$sliceSelectedBaselineCount = 0
$sliceExpectedVisualRenderCount = 0
foreach ($slice in $sliceSummaries) {
    $sliceSelectedBaselineCount += [int]$slice.selected_baseline_count
    if ([int]$slice.expected_visual_render_count -gt $sliceExpectedVisualRenderCount) {
        $sliceExpectedVisualRenderCount = [int]$slice.expected_visual_render_count
    }
}

$attemptStatus = Get-SummaryStatus -Summary $attemptSummary
$aggregateStatus = Get-SummaryStatus -Summary $aggregateSummary
$attemptStageCount = Get-JsonInt -Object $attemptSummary -Name "stage_count"
$attemptPassedStageCount = Get-JsonInt -Object $attemptSummary -Name "passed_stage_count"
$attemptFailedStageCount = Get-JsonInt -Object $attemptSummary -Name "failed_stage_count"
$attemptIncompleteStageCount = Get-JsonInt -Object $attemptSummary -Name "incomplete_stage_count"
$attemptFreshRenderedCount = Get-JsonInt -Object $attemptSummary -Name "visual_baseline_fresh_rendered_count"
$attemptExpectedVisualRenderCount = Get-JsonInt -Object $attemptSummary -Name "expected_visual_render_count"
$attemptAggregateContactSheetStatus = Get-JsonString -Object $attemptSummary -Name "aggregate_contact_sheet_status"
$aggregateSelectedBaselineCount = Get-JsonInt -Object $aggregateSummary -Name "selected_baseline_count"
$aggregateExpectedVisualRenderCount = Get-JsonInt -Object $aggregateSummary -Name "expected_visual_render_count"
$aggregateContactSheet = Get-JsonString -Object $aggregateSummary -Name "aggregate_contact_sheet"

$expectedVisualRenderCount = @(
    $attemptExpectedVisualRenderCount,
    $aggregateExpectedVisualRenderCount,
    $sliceExpectedVisualRenderCount
) | Measure-Object -Maximum | ForEach-Object { [int]$_.Maximum }
$coveredBaselineCount = @(
    $attemptFreshRenderedCount,
    $aggregateSelectedBaselineCount,
    $sliceSelectedBaselineCount
) | Measure-Object -Maximum | ForEach-Object { [int]$_.Maximum }

$contactSheetBytes = 0
if (-not [string]::IsNullOrWhiteSpace($aggregateContactSheet) -and (Test-Path -LiteralPath $aggregateContactSheet)) {
    $contactSheetBytes = [int64](Get-Item -LiteralPath $aggregateContactSheet).Length
}

$hasMissingEvidence = $null -eq $attemptSummary -or $null -eq $aggregateSummary -or @($sliceSummaries).Count -eq 0
$hasFailure = $attemptFailedStageCount -gt 0 -or $sliceFailedCount -gt 0 -or $aggregateStatus -eq "fail" -or $attemptStatus -eq "fail"
$hasCompleteSegmentedEvidence = -not $hasMissingEvidence -and
    -not $hasFailure -and
    $attemptStageCount -gt 0 -and
    $attemptPassedStageCount -eq $attemptStageCount -and
    $attemptIncompleteStageCount -eq 0 -and
    (Get-JsonString -Object $attemptSummary -Name "visual_baseline_render_status") -eq "pass" -and
    $attemptExpectedVisualRenderCount -gt 0 -and
    $attemptFreshRenderedCount -ge $attemptExpectedVisualRenderCount -and
    $attemptAggregateContactSheetStatus -eq "pass" -and
    $aggregateStatus -eq "pass" -and
    $aggregateExpectedVisualRenderCount -gt 0 -and
    $aggregateSelectedBaselineCount -ge $aggregateExpectedVisualRenderCount -and
    $slicePassCount -eq @($sliceSummaries).Count

$status = if ($hasCompleteSegmentedEvidence) {
    "pass"
} elseif ($hasFailure) {
    "fail"
} elseif ($hasMissingEvidence -or $coveredBaselineCount -gt 0) {
    "partial"
} else {
    "missing"
}

$summary = [ordered]@{
    schema = "featherdoc.pdf_visual_segmented_gate_summary.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    verdict = if ($status -eq "pass") { "pass" } elseif ($status -eq "fail") { "fail" } else { "not_complete" }
    full_visual_gate_status = "not_complete"
    evidence_scope = "segmented_visual_gate_auxiliary_only"
    boundary = "segmented_summary_does_not_replace_full_visual_gate_verdict"
    repo_root = $resolvedRepoRoot
    report_dir = $resolvedReportDir
    report_dir_display = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $resolvedReportDir
    summary_json = $resolvedOutputJson
    summary_json_display = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $resolvedOutputJson
    attempt_summary_json = $attemptSummaryPath
    attempt_summary_json_display = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $attemptSummaryPath
    aggregate_rebuild_summary_json = $aggregateSummaryPath
    aggregate_rebuild_summary_json_display = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $aggregateSummaryPath
    slice_summary_count = @($sliceSummaries).Count
    slice_pass_count = $slicePassCount
    slice_failed_count = $sliceFailedCount
    slice_selected_baseline_count = $sliceSelectedBaselineCount
    covered_baseline_count = $coveredBaselineCount
    expected_visual_render_count = $expectedVisualRenderCount
    visual_baseline_manifest_count = Get-JsonInt -Object $aggregateSummary -Name "visual_baseline_manifest_count"
    attempt_status = Get-JsonString -Object $attemptSummary -Name "status"
    attempt_verdict = Get-JsonString -Object $attemptSummary -Name "verdict"
    attempt_full_visual_gate_status = Get-JsonString -Object $attemptSummary -Name "full_visual_gate_status"
    attempt_stage_count = $attemptStageCount
    attempt_passed_stage_count = $attemptPassedStageCount
    attempt_failed_stage_count = $attemptFailedStageCount
    attempt_incomplete_stage_count = $attemptIncompleteStageCount
    visual_baseline_render_status = Get-JsonString -Object $attemptSummary -Name "visual_baseline_render_status"
    visual_baseline_fresh_rendered_count = $attemptFreshRenderedCount
    aggregate_contact_sheet_status = $attemptAggregateContactSheetStatus
    aggregate_contact_sheet = $aggregateContactSheet
    aggregate_contact_sheet_display = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $aggregateContactSheet
    aggregate_contact_sheet_bytes = $contactSheetBytes
    aggregate_rebuild_status = Get-JsonString -Object $aggregateSummary -Name "status"
    aggregate_rebuild_verdict = Get-JsonString -Object $aggregateSummary -Name "verdict"
    aggregate_rebuild_selected_baseline_count = $aggregateSelectedBaselineCount
    aggregate_rebuild_expected_visual_render_count = $aggregateExpectedVisualRenderCount
    slice_summaries = @($sliceSummaries)
}

New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($resolvedOutputJson)) -Force | Out-Null
($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8
Write-Host "PDF visual segmented gate summary: $resolvedOutputJson"
