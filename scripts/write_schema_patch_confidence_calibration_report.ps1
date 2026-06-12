<#
.SYNOPSIS
Writes a schema patch confidence calibration report from existing summaries.

.DESCRIPTION
Reads project-template smoke summaries, schema approval history summaries, or
onboarding summaries and aggregates schema patch review size, approval outcome,
and optional confidence metadata into calibration buckets. The script is
read-only for input evidence.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/schema-patch-confidence-calibration",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnPending
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$calibrationSchema = "featherdoc.schema_patch_confidence_calibration_report.v1"
$calibrationOpenCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"

. (Join-Path $PSScriptRoot "write_schema_patch_confidence_calibration_report_helpers.ps1")

$repoRoot = Resolve-RepoRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "schema_patch_confidence_calibration.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) schema approval summary file(s)"

$entries = New-Object 'System.Collections.Generic.List[object]'
$inputSummaries = New-Object 'System.Collections.Generic.List[object]'

foreach ($path in @($inputPaths)) {
    $status = "loaded"
    $errorMessage = ""
    $beforeCount = $entries.Count
    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        if ((Get-JsonString -Object $summary -Name "schema") -eq "featherdoc.project_template_schema_approval_history.v1" -or
            $null -ne (Get-JsonProperty -Object $summary -Name "entry_histories")) {
            Add-EntriesFromHistory -Path $path -Summary $summary -Entries $entries
        } else {
            Add-EntriesFromSmokeSummary -Path $path -Summary $summary -Entries $entries
        }
    } catch {
        $status = "failed"
        $errorMessage = $_.Exception.Message
    }

    $inputSummaries.Add([ordered]@{
        path = $path
        path_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
        status = $status
        extracted_entry_count = ($entries.Count - $beforeCount)
        error = $errorMessage
    }) | Out-Null
}

$entryArray = @($entries.ToArray())
$recommendedMinConfidence = Get-RecommendedMinConfidence -Entries $entryArray
$pendingCount = @($entryArray | Where-Object { $_.calibration_outcome -eq "pending" }).Count
$invalidCount = @($entryArray | Where-Object { $_.calibration_outcome -eq "invalid_result" }).Count
$sourceFailureCount = @($inputSummaries.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$summaryDisplayPath = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
$recommendations = @(New-Recommendations -Entries $entryArray -RecommendedMinConfidence $recommendedMinConfidence)
$releaseBlockers = @(New-ReleaseBlockers -Entries $entryArray -SourceReport $summaryPath -SourceReportDisplay $summaryDisplayPath -SourceJson $summaryPath -SourceJsonDisplay $summaryDisplayPath)
$warnings = @(New-Warnings -Entries $entryArray -InputSummaries $inputSummaries.ToArray() -SourceReport $summaryPath -SourceReportDisplay $summaryDisplayPath -SourceJson $summaryPath -SourceJsonDisplay $summaryDisplayPath)
$actionItems = @(New-ActionItems -Recommendations $recommendations -SourceReport $summaryPath -SourceReportDisplay $summaryDisplayPath -SourceJson $summaryPath -SourceJsonDisplay $summaryDisplayPath)
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($invalidCount -gt 0) {
    "blocked"
} elseif ($pendingCount -gt 0) {
    "pending_review"
} else {
    "ready"
}

$summaryObject = [ordered]@{
    schema = $calibrationSchema
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    output_dir = $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = $summaryDisplayPath
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    input_summaries = @($inputSummaries.ToArray())
    input_summary_count = $inputSummaries.Count
    source_failure_count = $sourceFailureCount
    run_count = $inputSummaries.Count
    entry_count = $entryArray.Count
    confidence_source = Get-ConfidenceSource -Entries $entryArray
    recommended_min_confidence = $recommendedMinConfidence
    gate_status_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "status" -OutputName "status")
    approval_outcome_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "calibration_outcome" -OutputName "outcome")
    confidence_buckets = @(New-BucketSummary -Entries $entryArray)
    reason_code_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "reason_code" -OutputName "reason_code")
    project_count = @($entryArray | ForEach-Object { Get-JsonString -Object $_ -Name "project_id" } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique).Count
    template_count = @($entryArray | ForEach-Object { Get-JsonString -Object $_ -Name "template_scope" } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique).Count
    project_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "project_id" -OutputName "project_id")
    template_scope_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "template_scope" -OutputName "template_scope")
    candidate_type_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "candidate_type" -OutputName "candidate_type")
    entries = $entryArray
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers)
    warning_count = $warnings.Count
    warnings = @($warnings)
    action_item_count = $actionItems.Count
    action_items = @($actionItems)
    recommendations = @($recommendations)
}

($summaryObject | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summaryObject) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnPending -and (($pendingCount + $invalidCount) -gt 0)) { exit 1 }
