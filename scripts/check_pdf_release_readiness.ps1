<#
.SYNOPSIS
Checks the current PDF release-readiness evidence without running heavy tools.

.DESCRIPTION
This script is intentionally read-only. It does not run CMake, CTest, Ninja,
MSBuild, Office, LibreOffice, browsers, PDF rendering, or PDF generation. It
only validates existing preflight, visual-gate, manifest, and contact-sheet
evidence, then emits a machine-readable release-readiness verdict.
#>
param(
    [string]$PreflightSummaryJson = "output/pdf-visual-release-gate-preflight-current/summary.json",
    [string]$VisualGateSummaryJson = "output/pdf-visual-release-gate-current/report/summary.json",
    [string]$ManifestJson = "test/pdf_regression_manifest.json",
    [string]$ReadinessChecklist = "docs/pdf_release_readiness_checklist_zh.rst",
    [string]$VisualFullGateGuardedSummaryJson = "output/pdf-visual-release-gate-current/report/full-visual-gate-guarded-summary.json",
    [string]$VisualSegmentedGateSummaryJson = "output/pdf-visual-release-gate-current/report/segmented-summary.json",
    [string]$FullCtestSummaryJson = "output/pdf-ctest-current/summary.json",
    [string]$FullCtestRemainingSummaryJson = "output/pdf-ctest-current/remaining-summary.json",
    [string]$OutputJson = "",
    [switch]$Strict
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $RepoRoot $Path
    }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }

    return $resolved
}

function Get-DisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $rootWithSeparator = $RepoRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    if ($fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        return ".\" + $fullPath.Substring($rootWithSeparator.Length)
    }

    return $fullPath
}

function Read-JsonFile {
    param([string]$Path)

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Get-OptionalPropertyValue {
    param($Object, [string]$Name)

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Test-IntegerEquals {
    param($Value, [int]$Expected)

    if ($null -eq $Value) {
        return $false
    }

    try {
        return ([int]$Value -eq $Expected)
    } catch {
        return $false
    }
}

function Test-IntegerAtLeast {
    param($Value, [int]$Minimum)

    if ($null -eq $Value) {
        return $false
    }

    try {
        return ([int]$Value -ge $Minimum)
    } catch {
        return $false
    }
}

function Convert-ToIntOrZero {
    param($Value)

    if ($null -eq $Value) {
        return 0
    }

    if ([string]::IsNullOrWhiteSpace([string]$Value)) {
        return 0
    }

    try {
        return [int]$Value
    } catch {
        return 0
    }
}

function Add-Check {
    param(
        [System.Collections.Generic.List[object]]$Checks,
        [string]$Name,
        [bool]$Passed,
        [string]$Message,
        [hashtable]$Details = @{}
    )

    $entry = [ordered]@{
        name = $Name
        status = if ($Passed) { "pass" } else { "fail" }
        required = $true
        message = $Message
    }

    if ($Details.Count -gt 0) {
        $entry.details = [ordered]@{}
        foreach ($key in ($Details.Keys | Sort-Object)) {
            $entry.details[$key] = $Details[$key]
        }
    }

    $Checks.Add([pscustomobject]$entry) | Out-Null
}

function Add-Warning {
    param(
        [System.Collections.Generic.List[object]]$Warnings,
        [string]$Id,
        [string]$Message,
        [hashtable]$Details = @{}
    )

    $entry = [ordered]@{
        id = $Id
        message = $Message
    }

    if ($Details.Count -gt 0) {
        $entry.details = [ordered]@{}
        foreach ($key in ($Details.Keys | Sort-Object)) {
            $entry.details[$key] = $Details[$key]
        }
    }

    $Warnings.Add([pscustomobject]$entry) | Out-Null
}

function Get-ManifestSamples {
    param($Manifest)

    $samples = Get-OptionalPropertyValue -Object $Manifest -Name "samples"
    if ($null -ne $samples) {
        return @($samples)
    }

    return @($Manifest)
}

function Resolve-ContactSheetPath {
    param(
        [string]$RepoRoot,
        $VisualSummary,
        [string]$VisualSummaryPath
    )

    $contactSheet = [string](Get-OptionalPropertyValue -Object $VisualSummary -Name "aggregate_contact_sheet")
    if ([string]::IsNullOrWhiteSpace($contactSheet)) {
        $reportDir = Split-Path -Parent $VisualSummaryPath
        return Join-Path $reportDir "aggregate-contact-sheet.png"
    }

    return Resolve-RepoPath -RepoRoot $RepoRoot -Path $contactSheet -AllowMissing
}

$repoRoot = Resolve-RepoRoot
$resolvedPreflightSummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $PreflightSummaryJson -AllowMissing
$resolvedVisualGateSummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $VisualGateSummaryJson -AllowMissing
$resolvedManifestJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $ManifestJson -AllowMissing
$resolvedReadinessChecklist = Resolve-RepoPath -RepoRoot $repoRoot -Path $ReadinessChecklist -AllowMissing
$resolvedVisualFullGateGuardedSummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $VisualFullGateGuardedSummaryJson -AllowMissing
$resolvedVisualSegmentedGateSummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $VisualSegmentedGateSummaryJson -AllowMissing
$resolvedFullCtestSummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $FullCtestSummaryJson -AllowMissing
$resolvedFullCtestRemainingSummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $FullCtestRemainingSummaryJson -AllowMissing
$resolvedOutputJson = if ([string]::IsNullOrWhiteSpace($OutputJson)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputJson -AllowMissing
}

$checks = [System.Collections.Generic.List[object]]::new()
$warnings = [System.Collections.Generic.List[object]]::new()

$preflight = $null
$visual = $null
$manifest = $null
$checklistText = ""
$visualFullGateGuarded = $null
$visualSegmentedGate = $null
$fullCtest = $null
$fullCtestRemaining = $null

$preflightExists = Test-Path -LiteralPath $resolvedPreflightSummaryJson -PathType Leaf
Add-Check -Checks $checks -Name "preflight_summary_exists" -Passed $preflightExists `
    -Message "Current PDF visual preflight summary must exist." `
    -Details @{ path = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedPreflightSummaryJson }
if ($preflightExists) {
    $preflight = Read-JsonFile -Path $resolvedPreflightSummaryJson
}

$visualExists = Test-Path -LiteralPath $resolvedVisualGateSummaryJson -PathType Leaf
Add-Check -Checks $checks -Name "visual_gate_summary_exists" -Passed $visualExists `
    -Message "Current PDF visual gate summary must exist." `
    -Details @{ path = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedVisualGateSummaryJson }
if ($visualExists) {
    $visual = Read-JsonFile -Path $resolvedVisualGateSummaryJson
}

$manifestExists = Test-Path -LiteralPath $resolvedManifestJson -PathType Leaf
Add-Check -Checks $checks -Name "regression_manifest_exists" -Passed $manifestExists `
    -Message "PDF regression manifest must exist." `
    -Details @{ path = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedManifestJson }
if ($manifestExists) {
    $manifest = Read-JsonFile -Path $resolvedManifestJson
}

$checklistExists = Test-Path -LiteralPath $resolvedReadinessChecklist -PathType Leaf
Add-Check -Checks $checks -Name "readiness_checklist_exists" -Passed $checklistExists `
    -Message "Fixed PDF release-readiness checklist must exist." `
    -Details @{ path = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedReadinessChecklist }
if ($checklistExists) {
    $checklistText = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedReadinessChecklist
}

$visualFullGateGuardedSummaryExists = Test-Path -LiteralPath $resolvedVisualFullGateGuardedSummaryJson -PathType Leaf
if ($visualFullGateGuardedSummaryExists) {
    $visualFullGateGuarded = Read-JsonFile -Path $resolvedVisualFullGateGuardedSummaryJson
    $visualFullGateGuardedSchema = [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "schema")
    Add-Check -Checks $checks -Name "visual_full_gate_guarded_summary_schema" -Passed ($visualFullGateGuardedSchema -eq "featherdoc.pdf_visual_full_gate_guarded_summary.v1") `
        -Message "Existing fresh full PDF visual gate guarded summary must use the stable schema." `
        -Details @{
            actual = $visualFullGateGuardedSchema
            expected = "featherdoc.pdf_visual_full_gate_guarded_summary.v1"
            path = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedVisualFullGateGuardedSummaryJson
        }
}

$visualSegmentedGateSummaryExists = Test-Path -LiteralPath $resolvedVisualSegmentedGateSummaryJson -PathType Leaf
if ($visualSegmentedGateSummaryExists) {
    $visualSegmentedGate = Read-JsonFile -Path $resolvedVisualSegmentedGateSummaryJson
    $visualSegmentedGateSchema = [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "schema")
    Add-Check -Checks $checks -Name "visual_segmented_gate_summary_schema" -Passed ($visualSegmentedGateSchema -eq "featherdoc.pdf_visual_segmented_gate_summary.v1") `
        -Message "Existing PDF visual segmented gate summary must use the stable schema." `
        -Details @{
            actual = $visualSegmentedGateSchema
            expected = "featherdoc.pdf_visual_segmented_gate_summary.v1"
            path = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedVisualSegmentedGateSummaryJson
        }
}

$visualSegmentedGateStatus = if ($null -eq $visualSegmentedGate) { "missing" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "status") }
$visualSegmentedGateVerdict = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "verdict") }
$visualSegmentedGateFullStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "full_visual_gate_status") }
$visualSegmentedGateEvidenceScope = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "evidence_scope") }
$visualSegmentedGateBoundary = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "boundary") }
$visualSegmentedGateSummaryJsonDisplay = if ($null -eq $visualSegmentedGate) { "" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "summary_json_display") }
$visualSegmentedGateSliceSummaryCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "slice_summary_count" }
$visualSegmentedGateSlicePassCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "slice_pass_count" }
$visualSegmentedGateSliceFailedCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "slice_failed_count" }
$visualSegmentedGateCoveredBaselineCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "covered_baseline_count" }
$visualSegmentedGateExpectedVisualRenderCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "expected_visual_render_count" }
$visualSegmentedGateAttemptStageCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "attempt_stage_count" }
$visualSegmentedGateAttemptPassedStageCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "attempt_passed_stage_count" }
$visualSegmentedGateVisualBaselineRenderStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "visual_baseline_render_status") }
$visualSegmentedGateAggregateContactSheetStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "aggregate_contact_sheet_status") }
$visualSegmentedGateAggregateContactSheetDisplay = if ($null -eq $visualSegmentedGate) { "" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "aggregate_contact_sheet_display") }
$visualSegmentedGateAggregateContactSheetBytes = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "aggregate_contact_sheet_bytes" }
$visualSegmentedGateAggregateRebuildStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "aggregate_rebuild_status") }
$visualSegmentedGateAggregateRebuildSelectedBaselineCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "aggregate_rebuild_selected_baseline_count" }
$visualSegmentedGateResumeStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_status") }
$visualSegmentedGateResumeVerdict = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_verdict") }
$visualSegmentedGateResumeFullStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_full_visual_gate_status") }
$visualSegmentedGateResumeEvidenceScope = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_evidence_scope") }
$visualSegmentedGateResumeBoundary = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_boundary") }
$visualSegmentedGateResumeTailFullyPlanned = if ($null -eq $visualSegmentedGate) { $false } else { [bool](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_tail_fully_planned") }
$visualSegmentedGateResumeTotalSliceCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_total_slice_count" }
$visualSegmentedGateResumePlannedSliceCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_planned_slice_count" }
$visualSegmentedGateResumeExecutedSliceCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_executed_slice_count" }
$visualSegmentedGateResumePassedSliceCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_passed_slice_count" }
$visualSegmentedGateResumeFailedSliceCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_failed_slice_count" }
$visualSegmentedGateResumeTimeoutSliceCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_timeout_slice_count" }
$visualSegmentedGateResumeAggregateRebuildStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "segmented_resume_aggregate_rebuild_status") }

$fullCtestSummaryExists = Test-Path -LiteralPath $resolvedFullCtestSummaryJson -PathType Leaf
if ($fullCtestSummaryExists) {
    $fullCtest = Read-JsonFile -Path $resolvedFullCtestSummaryJson
    $fullCtestSchema = [string](Get-OptionalPropertyValue -Object $fullCtest -Name "schema")
    Add-Check -Checks $checks -Name "full_ctest_guarded_summary_schema" -Passed ($fullCtestSchema -eq "featherdoc.pdf_full_ctest_guarded_summary.v1") `
        -Message "Existing full PDF CTest guarded summary must use the stable schema." `
        -Details @{
            actual = $fullCtestSchema
            expected = "featherdoc.pdf_full_ctest_guarded_summary.v1"
            path = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedFullCtestSummaryJson
        }
}

$fullCtestRemainingSummaryExists = Test-Path -LiteralPath $resolvedFullCtestRemainingSummaryJson -PathType Leaf
if ($fullCtestRemainingSummaryExists) {
    $fullCtestRemaining = Read-JsonFile -Path $resolvedFullCtestRemainingSummaryJson
    $fullCtestRemainingSchema = [string](Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "schema")
    Add-Check -Checks $checks -Name "full_ctest_remaining_guarded_summary_schema" -Passed ($fullCtestRemainingSchema -eq "featherdoc.pdf_ctest_remaining_guarded_summary.v1") `
        -Message "Existing remaining-tail PDF CTest guarded summary must use the stable schema." `
        -Details @{
            actual = $fullCtestRemainingSchema
            expected = "featherdoc.pdf_ctest_remaining_guarded_summary.v1"
            path = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedFullCtestRemainingSummaryJson
        }
}

if ($null -ne $manifest) {
    $samples = @(Get-ManifestSamples -Manifest $manifest)
    $visualSampleCount = @($samples | Where-Object { (Get-OptionalPropertyValue -Object $_ -Name "expect_visual_baseline") -eq $true }).Count
    $cjkSampleCount = @($samples | Where-Object { (Get-OptionalPropertyValue -Object $_ -Name "expect_cjk") -eq $true }).Count

    Add-Check -Checks $checks -Name "regression_manifest_sample_count" -Passed ($samples.Count -eq 90) `
        -Message "PDF regression manifest must keep the release sample count." `
        -Details @{ actual = $samples.Count; expected = 90 }
    Add-Check -Checks $checks -Name "visual_baseline_manifest_sample_count" -Passed ($visualSampleCount -eq 42) `
        -Message "PDF visual baseline manifest sample count must stay aligned." `
        -Details @{ actual = $visualSampleCount; expected = 42 }
    Add-Check -Checks $checks -Name "cjk_text_layer_manifest_sample_count" -Passed ($cjkSampleCount -eq 43) `
        -Message "PDF CJK text-layer manifest sample count must stay aligned." `
        -Details @{ actual = $cjkSampleCount; expected = 43 }
}

if ($null -ne $preflight) {
    $blockingSummary = Get-OptionalPropertyValue -Object $preflight -Name "blocking_summary"
    $blockingCheckCount = if ($null -eq $blockingSummary) { $null } else { Get-OptionalPropertyValue -Object $blockingSummary -Name "blocking_check_count" }
    $preflightStatus = [string](Get-OptionalPropertyValue -Object $preflight -Name "status")
    $evidenceKind = [string](Get-OptionalPropertyValue -Object $preflight -Name "evidence_kind")
    $outputGapCount = Get-OptionalPropertyValue -Object $preflight -Name "output_gap_count"
    $missingOutputCount = Get-OptionalPropertyValue -Object $preflight -Name "missing_output_count"
    $dependencyStatus = if ($null -eq $blockingSummary) { "" } else { [string](Get-OptionalPropertyValue -Object $blockingSummary -Name "pdf_dependency_inputs_status") }
    $buildOptionsEnabled = if ($null -eq $blockingSummary) { $false } else { [bool](Get-OptionalPropertyValue -Object $blockingSummary -Name "pdf_build_options_enabled") }
    $missingVisualPdfs = if ($null -eq $blockingSummary) { $null } else { Get-OptionalPropertyValue -Object $blockingSummary -Name "missing_visual_baseline_pdf_count" }
    $missingCjkPdfs = if ($null -eq $blockingSummary) { $null } else { Get-OptionalPropertyValue -Object $blockingSummary -Name "missing_cjk_text_layer_pdf_count" }

    Add-Check -Checks $checks -Name "preflight_status_ready" -Passed ($preflightStatus -eq "ready") `
        -Message "PDF visual preflight must be ready." `
        -Details @{ actual = $preflightStatus; expected = "ready" }
    Add-Check -Checks $checks -Name "preflight_blocking_checks_empty" -Passed (Test-IntegerEquals -Value $blockingCheckCount -Expected 0) `
        -Message "PDF visual preflight must have no blocking checks." `
        -Details @{ actual = $blockingCheckCount; expected = 0 }
    Add-Check -Checks $checks -Name "preflight_real_build_evidence" -Passed ($evidenceKind -eq "real_build") `
        -Message "PDF visual preflight must be based on real build evidence." `
        -Details @{ actual = $evidenceKind; expected = "real_build" }
    Add-Check -Checks $checks -Name "preflight_outputs_complete" -Passed ((Test-IntegerEquals -Value $outputGapCount -Expected 0) -and (Test-IntegerEquals -Value $missingOutputCount -Expected 0)) `
        -Message "PDF visual preflight must not report missing outputs." `
        -Details @{ output_gap_count = $outputGapCount; missing_output_count = $missingOutputCount }
    Add-Check -Checks $checks -Name "preflight_dependency_inputs_ready" -Passed ($dependencyStatus -eq "ready") `
        -Message "PDF dependency inputs must be ready." `
        -Details @{ actual = $dependencyStatus; expected = "ready" }
    Add-Check -Checks $checks -Name "preflight_pdf_build_options_enabled" -Passed $buildOptionsEnabled `
        -Message "PDF build options must be enabled in the checked build." `
        -Details @{ actual = $buildOptionsEnabled; expected = $true }
    Add-Check -Checks $checks -Name "preflight_visual_and_cjk_outputs_complete" -Passed ((Test-IntegerEquals -Value $missingVisualPdfs -Expected 0) -and (Test-IntegerEquals -Value $missingCjkPdfs -Expected 0)) `
        -Message "Preflight must find all visual-baseline and CJK text-layer PDFs." `
        -Details @{ missing_visual_baseline_pdf_count = $missingVisualPdfs; missing_cjk_text_layer_pdf_count = $missingCjkPdfs }
}

$contactSheetPath = ""
$contactSheetExists = $false
$contactSheetBytes = 0
$visualGateFinalizeOnly = $false
$visualGateSkipPreflight = $false
$visualGateFreshFullGuardedEvidence = $false
$visualGatePassSummaryBeforeOuterTimeout = $false
$visualGateSegmentedFullCoverageEvidence = $false
$visualGateReleaseEvidenceAccepted = $false
if ($null -ne $visual) {
    $status = [string](Get-OptionalPropertyValue -Object $visual -Name "status")
    $verdict = [string](Get-OptionalPropertyValue -Object $visual -Name "verdict")
    $finalizeOnly = [bool](Get-OptionalPropertyValue -Object $visual -Name "finalize_only")
    $skipPreflight = [bool](Get-OptionalPropertyValue -Object $visual -Name "skip_preflight")
    $visualGateFinalizeOnly = $finalizeOnly
    $visualGateSkipPreflight = $skipPreflight
    $guardedStatus = if ($null -eq $visualFullGateGuarded) { "missing" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "status") }
    $guardedVerdict = if ($null -eq $visualFullGateGuarded) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "verdict") }
    $guardedFullStatus = if ($null -eq $visualFullGateGuarded) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "full_visual_gate_status") }
    $guardedOuterStatus = if ($null -eq $visualFullGateGuarded) { "not_run" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "outer_guard_status") }
    $guardedOuterTimedOut = if ($null -eq $visualFullGateGuarded) { $false } else { [bool](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "outer_guard_timed_out") }
    $guardedPassSummaryBeforeOuterTimeout = if ($null -eq $visualFullGateGuarded) { $false } else { [bool](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "pass_summary_before_outer_timeout") }
    $guardedAttemptFailedStageCount = if ($null -eq $visualFullGateGuarded) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "attempt_failed_stage_count" }
    $guardedAttemptFailedStageCountInt = Convert-ToIntOrZero -Value $guardedAttemptFailedStageCount
    $segmentedSliceSummaryCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateSliceSummaryCount
    $segmentedSlicePassCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateSlicePassCount
    $segmentedSliceFailedCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateSliceFailedCount
    $segmentedCoveredBaselineCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateCoveredBaselineCount
    $segmentedExpectedVisualRenderCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateExpectedVisualRenderCount
    $segmentedAttemptStageCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateAttemptStageCount
    $segmentedAttemptPassedStageCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateAttemptPassedStageCount
    $segmentedContactSheetBytesInt = Convert-ToIntOrZero -Value $visualSegmentedGateAggregateContactSheetBytes
    $segmentedAggregateRebuildSelectedBaselineCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateAggregateRebuildSelectedBaselineCount
    $segmentedResumeTotalSliceCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateResumeTotalSliceCount
    $segmentedResumePlannedSliceCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateResumePlannedSliceCount
    $segmentedResumeExecutedSliceCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateResumeExecutedSliceCount
    $segmentedResumePassedSliceCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateResumePassedSliceCount
    $segmentedResumeFailedSliceCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateResumeFailedSliceCount
    $segmentedResumeTimeoutSliceCountInt = Convert-ToIntOrZero -Value $visualSegmentedGateResumeTimeoutSliceCount
    $visualGatePassSummaryBeforeOuterTimeout = (
        $guardedPassSummaryBeforeOuterTimeout -and
        $guardedOuterStatus -eq "timed_out_after_pass_summary" -and
        $guardedOuterTimedOut
    )
    $visualGateFreshFullGuardedEvidence = (
        $guardedStatus -eq "pass" -and
        $guardedVerdict -eq "pass" -and
        $guardedFullStatus -eq "pass" -and
        (
            ($guardedOuterStatus -eq "completed" -and -not $guardedOuterTimedOut) -or
            $visualGatePassSummaryBeforeOuterTimeout
        )
    )
    $segmentedGateCommonEvidence = (
        $visualSegmentedGateSummaryExists -and
        $visualSegmentedGateStatus -eq "pass" -and
        $visualSegmentedGateVerdict -eq "pass" -and
        $visualSegmentedGateFullStatus -eq "not_complete" -and
        $visualSegmentedGateEvidenceScope -eq "segmented_visual_gate_auxiliary_only" -and
        $visualSegmentedGateBoundary -eq "segmented_summary_does_not_replace_full_visual_gate_verdict" -and
        $segmentedSliceSummaryCountInt -gt 0 -and
        $segmentedSlicePassCountInt -eq $segmentedSliceSummaryCountInt -and
        $segmentedSliceFailedCountInt -eq 0 -and
        $segmentedCoveredBaselineCountInt -ge 42 -and
        $segmentedCoveredBaselineCountInt -eq $segmentedExpectedVisualRenderCountInt -and
        $segmentedContactSheetBytesInt -gt 0 -and
        $visualSegmentedGateAggregateRebuildStatus -eq "pass" -and
        $segmentedAggregateRebuildSelectedBaselineCountInt -eq $segmentedExpectedVisualRenderCountInt -and
        $guardedAttemptFailedStageCountInt -eq 0
    )
    $segmentedGateOriginalAttemptCompleteEvidence = (
        $segmentedAttemptStageCountInt -gt 0 -and
        $segmentedAttemptPassedStageCountInt -ge $segmentedAttemptStageCountInt -and
        $visualSegmentedGateVisualBaselineRenderStatus -eq "pass" -and
        $visualSegmentedGateAggregateContactSheetStatus -eq "pass"
    )
    $segmentedGateResumeCompleteEvidence = (
        $visualSegmentedGateResumeStatus -eq "pass" -and
        $visualSegmentedGateResumeVerdict -eq "pass" -and
        $visualSegmentedGateResumeFullStatus -eq "not_complete" -and
        $visualSegmentedGateResumeEvidenceScope -eq "visual_segmented_resume_auxiliary_only" -and
        $visualSegmentedGateResumeBoundary -eq "segmented_resume_does_not_replace_full_visual_gate_verdict" -and
        $visualSegmentedGateResumeTailFullyPlanned -and
        $segmentedResumeTotalSliceCountInt -gt 0 -and
        $segmentedResumePlannedSliceCountInt -eq $segmentedResumeTotalSliceCountInt -and
        $segmentedResumeExecutedSliceCountInt -eq $segmentedResumeTotalSliceCountInt -and
        $segmentedResumePassedSliceCountInt -eq $segmentedResumeTotalSliceCountInt -and
        $segmentedResumeFailedSliceCountInt -eq 0 -and
        $segmentedResumeTimeoutSliceCountInt -eq 0 -and
        $visualSegmentedGateResumeAggregateRebuildStatus -eq "pass"
    )
    $visualGateSegmentedFullCoverageEvidence = $segmentedGateCommonEvidence -and (
        $segmentedGateOriginalAttemptCompleteEvidence -or
        $segmentedGateResumeCompleteEvidence
    )
    $visualGateReleaseEvidenceAccepted = ($finalizeOnly -or $visualGateFreshFullGuardedEvidence -or $visualGateSegmentedFullCoverageEvidence)
    $visualBaselineManifestCount = Get-OptionalPropertyValue -Object $visual -Name "visual_baseline_manifest_count"
    $baselinesCount = Get-OptionalPropertyValue -Object $visual -Name "baselines_count"
    $cjkManifestCount = Get-OptionalPropertyValue -Object $visual -Name "cjk_manifest_count"
    $cjkCopySearchCount = Get-OptionalPropertyValue -Object $visual -Name "cjk_copy_search_count"

    Add-Check -Checks $checks -Name "visual_gate_status_pass" -Passed ($status -eq "pass") `
        -Message "Current PDF visual gate summary status must pass." `
        -Details @{ actual = $status; expected = "pass" }
    Add-Check -Checks $checks -Name "visual_gate_verdict_pass" -Passed ($verdict -eq "pass") `
        -Message "Current PDF visual gate summary verdict must pass." `
        -Details @{ actual = $verdict; expected = "pass" }
    Add-Check -Checks $checks -Name "visual_gate_release_evidence" -Passed $visualGateReleaseEvidenceAccepted `
        -Message "Current PDF visual gate summary must be backed by explicit finalize evidence, a fresh guarded full visual gate pass, or segmented full-coverage visual evidence." `
        -Details @{
            finalize_only = $finalizeOnly
            fresh_full_guarded_evidence = $visualGateFreshFullGuardedEvidence
            pass_summary_before_outer_timeout = $visualGatePassSummaryBeforeOuterTimeout
            segmented_full_coverage_evidence = $visualGateSegmentedFullCoverageEvidence
            guarded_status = $guardedStatus
            guarded_verdict = $guardedVerdict
            guarded_full_visual_gate_status = $guardedFullStatus
            guarded_outer_guard_status = $guardedOuterStatus
            guarded_outer_guard_timed_out = $guardedOuterTimedOut
            guarded_pass_summary_before_outer_timeout = $guardedPassSummaryBeforeOuterTimeout
            guarded_attempt_failed_stage_count = $guardedAttemptFailedStageCount
            segmented_gate_status = $visualSegmentedGateStatus
            segmented_gate_verdict = $visualSegmentedGateVerdict
            segmented_gate_evidence_scope = $visualSegmentedGateEvidenceScope
            segmented_gate_boundary = $visualSegmentedGateBoundary
            segmented_gate_slice_failed_count = $visualSegmentedGateSliceFailedCount
            segmented_gate_covered_baseline_count = $visualSegmentedGateCoveredBaselineCount
            segmented_gate_expected_visual_render_count = $visualSegmentedGateExpectedVisualRenderCount
            segmented_gate_visual_baseline_render_status = $visualSegmentedGateVisualBaselineRenderStatus
            segmented_gate_aggregate_contact_sheet_status = $visualSegmentedGateAggregateContactSheetStatus
            segmented_gate_aggregate_contact_sheet_bytes = $visualSegmentedGateAggregateContactSheetBytes
            segmented_gate_aggregate_rebuild_status = $visualSegmentedGateAggregateRebuildStatus
            segmented_gate_resume_status = $visualSegmentedGateResumeStatus
            segmented_gate_resume_tail_fully_planned = $visualSegmentedGateResumeTailFullyPlanned
            segmented_gate_resume_total_slice_count = $visualSegmentedGateResumeTotalSliceCount
            segmented_gate_resume_passed_slice_count = $visualSegmentedGateResumePassedSliceCount
            segmented_gate_resume_aggregate_rebuild_status = $visualSegmentedGateResumeAggregateRebuildStatus
        }
    Add-Check -Checks $checks -Name "visual_gate_manifest_counts" -Passed ((Test-IntegerEquals -Value $visualBaselineManifestCount -Expected 42) -and (Test-IntegerAtLeast -Value $baselinesCount -Minimum 42) -and (Test-IntegerEquals -Value $cjkManifestCount -Expected 43) -and (Test-IntegerEquals -Value $cjkCopySearchCount -Expected 43)) `
        -Message "Current PDF visual gate summary must preserve release manifest counts." `
        -Details @{
            visual_baseline_manifest_count = $visualBaselineManifestCount
            baselines_count = $baselinesCount
            cjk_manifest_count = $cjkManifestCount
            cjk_copy_search_count = $cjkCopySearchCount
        }

    if ($finalizeOnly -and -not $skipPreflight) {
        Add-Warning -Warnings $warnings -Id "pdf_visual_gate_finalize_summary.did_not_skip_preflight" `
            -Message "Current visual gate summary is finalize evidence, but skip_preflight is false; rely on the separate current preflight summary for preflight freshness." `
            -Details @{ visual_gate_summary_json = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedVisualGateSummaryJson }
    }

    $contactSheetPath = Resolve-ContactSheetPath -RepoRoot $repoRoot -VisualSummary $visual -VisualSummaryPath $resolvedVisualGateSummaryJson
    $contactSheetExists = Test-Path -LiteralPath $contactSheetPath -PathType Leaf
    if ($contactSheetExists) {
        $contactSheetBytes = (Get-Item -LiteralPath $contactSheetPath).Length
    }
    Add-Check -Checks $checks -Name "aggregate_contact_sheet_non_empty" -Passed ($contactSheetExists -and $contactSheetBytes -gt 0) `
        -Message "Current PDF aggregate contact sheet must exist and be non-empty." `
        -Details @{
            path = Get-DisplayPath -RepoRoot $repoRoot -Path $contactSheetPath
            bytes = $contactSheetBytes
        }
}

if ($checklistText.Length -gt 0) {
    $requiredChecklistMarkers = @(
        "pdf_release_readiness_machine_gate_trace",
        "check_pdf_release_readiness.ps1",
        "run_pdf_visual_full_gate_guarded.ps1",
        "write_pdf_visual_segmented_gate_summary.ps1",
        "run_pdf_full_ctest_guarded.ps1",
        "run_pdf_ctest_remaining_guarded.ps1",
        "featherdoc.pdf_release_readiness_check.v1",
        "featherdoc.pdf_visual_full_gate_guarded_summary.v1",
        "pdf_visual_full_gate_guarded_summary_trace",
        "featherdoc.pdf_visual_segmented_gate_summary.v1",
        "pdf_visual_segmented_gate_summary_trace",
        "featherdoc.pdf_full_ctest_guarded_summary.v1",
        "pdf_full_ctest_guarded_summary_trace",
        "featherdoc.pdf_ctest_remaining_guarded_summary.v1",
        "pdf_ctest_remaining_guarded_summary_trace",
        "pdf_visual_gate_evidence",
        "visual_gate_segmented_full_coverage_evidence",
        "pdf_bounded_ctest_evidence",
        "release_entry_pdf_readiness_checklist_trace"
    )
    foreach ($marker in $requiredChecklistMarkers) {
        Add-Check -Checks $checks -Name ("readiness_checklist_marker.{0}" -f $marker) -Passed ($checklistText.Contains($marker)) `
            -Message "PDF release-readiness checklist must keep the machine gate marker." `
            -Details @{ marker = $marker }
    }
}

$visualFullGateStatus = if ($null -eq $visualFullGateGuarded) { "missing" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "status") }
$visualFullGateVerdict = if ($null -eq $visualFullGateGuarded) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "verdict") }
$visualFullGateFullStatus = if ($null -eq $visualFullGateGuarded) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "full_visual_gate_status") }
$visualFullGateOuterGuardStatus = if ($null -eq $visualFullGateGuarded) { "not_run" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "outer_guard_status") }
$visualFullGateOuterGuardTimedOut = if ($null -eq $visualFullGateGuarded) { $false } else { [bool](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "outer_guard_timed_out") }
$visualFullGatePassSummaryBeforeOuterTimeout = if ($null -eq $visualFullGateGuarded) { $false } else { [bool](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "pass_summary_before_outer_timeout") }
$visualFullGateAttemptSummaryJsonRaw = if ($null -eq $visualFullGateGuarded) { "" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "attempt_summary_json") }
$visualFullGateAttemptSummaryJson = if ($null -eq $visualFullGateGuarded) { "" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "attempt_summary_json_display") }
$visualFullGateAttemptSummaryPathInput = if (-not [string]::IsNullOrWhiteSpace($visualFullGateAttemptSummaryJsonRaw)) {
    $visualFullGateAttemptSummaryJsonRaw
} else {
    $visualFullGateAttemptSummaryJson
}
$resolvedVisualFullGateAttemptSummaryJson = if ([string]::IsNullOrWhiteSpace($visualFullGateAttemptSummaryPathInput)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $visualFullGateAttemptSummaryPathInput -AllowMissing
}
$visualFullGateAttemptSummaryExists = (-not [string]::IsNullOrWhiteSpace($resolvedVisualFullGateAttemptSummaryJson) -and (Test-Path -LiteralPath $resolvedVisualFullGateAttemptSummaryJson -PathType Leaf))
$visualFullGateAttemptSummary = if ($visualFullGateAttemptSummaryExists) { Read-JsonFile -Path $resolvedVisualFullGateAttemptSummaryJson } else { $null }
if ($visualFullGateAttemptSummaryExists) {
    $visualFullGateAttemptSummarySchema = [string](Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "schema")
    Add-Check -Checks $checks -Name "visual_full_gate_attempt_summary_schema" -Passed ($visualFullGateAttemptSummarySchema -eq "featherdoc.pdf_visual_gate_attempt_summary.v1") `
        -Message "Existing fresh full PDF visual gate attempt summary must use the stable schema." `
        -Details @{
            actual = $visualFullGateAttemptSummarySchema
            expected = "featherdoc.pdf_visual_gate_attempt_summary.v1"
            path = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedVisualFullGateAttemptSummaryJson
        }
}
$visualFullGateAttemptStageCount = if ($null -eq $visualFullGateGuarded) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "attempt_stage_count" }
$visualFullGateAttemptPassedStageCount = if ($null -eq $visualFullGateGuarded) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "attempt_passed_stage_count" }
$visualFullGateAttemptFailedStageCount = if ($null -eq $visualFullGateGuarded) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "attempt_failed_stage_count" }
$visualFullGateAttemptIncompleteStageCount = if ($null -eq $visualFullGateGuarded) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "attempt_incomplete_stage_count" }
$visualFullGateAttemptFreshRenderedCount = if ($null -eq $visualFullGateGuarded) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "attempt_visual_baseline_fresh_rendered_count" }
$visualFullGateAttemptContactSheetStatus = if ($null -eq $visualFullGateGuarded) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateGuarded -Name "attempt_aggregate_contact_sheet_status") }
$visualFullGateAttemptSummaryStatus = if ($null -eq $visualFullGateAttemptSummary) { "missing" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "status") }
$visualFullGateAttemptSummaryVerdict = if ($null -eq $visualFullGateAttemptSummary) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "verdict") }
$visualFullGateAttemptSummaryFullStatus = if ($null -eq $visualFullGateAttemptSummary) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "full_visual_gate_status") }
$visualFullGateAttemptSummaryStageCount = if ($null -eq $visualFullGateAttemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "stage_count" }
$visualFullGateAttemptSummaryPassedStageCount = if ($null -eq $visualFullGateAttemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "passed_stage_count" }
$visualFullGateAttemptSummaryIncompleteStageCount = if ($null -eq $visualFullGateAttemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "incomplete_stage_count" }
$visualFullGateAttemptSummaryVisualBaselineRenderStatus = if ($null -eq $visualFullGateAttemptSummary) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "visual_baseline_render_status") }
$visualFullGateAttemptSummaryFreshRenderedCount = if ($null -eq $visualFullGateAttemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "visual_baseline_fresh_rendered_count" }
$visualFullGateAttemptSummaryExpectedVisualRenderCount = if ($null -eq $visualFullGateAttemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "expected_visual_render_count" }
$visualFullGateAttemptSummaryFreshMissingSampleCount = if ($null -eq $visualFullGateAttemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "visual_baseline_fresh_missing_sample_count" }
$visualFullGateAttemptSummaryResumeNeeded = if ($null -eq $visualFullGateAttemptSummary) { $false } else { [bool](Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "visual_baseline_resume_needed") }
$visualFullGateAttemptSummaryResumeSliceOffset = if ($null -eq $visualFullGateAttemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "visual_baseline_resume_slice_offset" }
$visualFullGateAttemptSummaryResumeSliceLimit = if ($null -eq $visualFullGateAttemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "visual_baseline_resume_slice_limit" }
$visualFullGateAttemptSummaryResumeSliceCommandTemplate = if ($null -eq $visualFullGateAttemptSummary) { "" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "visual_baseline_resume_slice_command_template") }
$visualFullGateAttemptSummaryContactSheetStatus = if ($null -eq $visualFullGateAttemptSummary) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "aggregate_contact_sheet_status") }
$visualFullGateAttemptSummaryContactSheetBytes = if ($null -eq $visualFullGateAttemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $visualFullGateAttemptSummary -Name "aggregate_contact_sheet_bytes" }
$visualSegmentedGateStatus = if ($null -eq $visualSegmentedGate) { "missing" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "status") }
$visualSegmentedGateVerdict = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "verdict") }
$visualSegmentedGateFullStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "full_visual_gate_status") }
$visualSegmentedGateEvidenceScope = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "evidence_scope") }
$visualSegmentedGateBoundary = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "boundary") }
$visualSegmentedGateSummaryJsonDisplay = if ($null -eq $visualSegmentedGate) { "" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "summary_json_display") }
$visualSegmentedGateSliceSummaryCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "slice_summary_count" }
$visualSegmentedGateSlicePassCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "slice_pass_count" }
$visualSegmentedGateSliceFailedCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "slice_failed_count" }
$visualSegmentedGateCoveredBaselineCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "covered_baseline_count" }
$visualSegmentedGateExpectedVisualRenderCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "expected_visual_render_count" }
$visualSegmentedGateAttemptStageCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "attempt_stage_count" }
$visualSegmentedGateAttemptPassedStageCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "attempt_passed_stage_count" }
$visualSegmentedGateVisualBaselineRenderStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "visual_baseline_render_status") }
$visualSegmentedGateAggregateContactSheetStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "aggregate_contact_sheet_status") }
$visualSegmentedGateAggregateContactSheetDisplay = if ($null -eq $visualSegmentedGate) { "" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "aggregate_contact_sheet_display") }
$visualSegmentedGateAggregateContactSheetBytes = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "aggregate_contact_sheet_bytes" }
$visualSegmentedGateAggregateRebuildStatus = if ($null -eq $visualSegmentedGate) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "aggregate_rebuild_status") }
$visualSegmentedGateAggregateRebuildSelectedBaselineCount = if ($null -eq $visualSegmentedGate) { 0 } else { Get-OptionalPropertyValue -Object $visualSegmentedGate -Name "aggregate_rebuild_selected_baseline_count" }
$visualFullGateCompleted = (
    $visualFullGateStatus -eq "pass" -and
    $visualFullGateVerdict -eq "pass" -and
    $visualFullGateFullStatus -eq "pass" -and
    (
        ($visualFullGateOuterGuardStatus -eq "completed" -and -not $visualFullGateOuterGuardTimedOut) -or
        ($visualFullGatePassSummaryBeforeOuterTimeout -and $visualFullGateOuterGuardStatus -eq "timed_out_after_pass_summary" -and $visualFullGateOuterGuardTimedOut)
    )
)
if (-not $visualFullGateCompleted) {
    Add-Warning -Warnings $warnings -Id "pdf_full_fresh_visual_gate.not_completed_in_current_window" `
        -Message "A fresh non-FinalizeOnly full visual gate is still required before claiming a new end-to-end render pass; current guarded-attempt evidence is retained for reviewer traceability." `
        -Details @{
            status = $visualFullGateStatus
            verdict = $visualFullGateVerdict
            full_visual_gate_status = $visualFullGateFullStatus
            outer_guard_status = $visualFullGateOuterGuardStatus
            outer_guard_timed_out = $visualFullGateOuterGuardTimedOut
            pass_summary_before_outer_timeout = $visualFullGatePassSummaryBeforeOuterTimeout
            attempt_stage_count = $visualFullGateAttemptStageCount
            attempt_passed_stage_count = $visualFullGateAttemptPassedStageCount
            attempt_failed_stage_count = $visualFullGateAttemptFailedStageCount
            attempt_incomplete_stage_count = $visualFullGateAttemptIncompleteStageCount
            attempt_visual_baseline_fresh_rendered_count = $visualFullGateAttemptFreshRenderedCount
            attempt_aggregate_contact_sheet_status = $visualFullGateAttemptContactSheetStatus
            attempt_summary_json = $visualFullGateAttemptSummaryJson
            attempt_summary_exists = $visualFullGateAttemptSummaryExists
            attempt_summary_status = $visualFullGateAttemptSummaryStatus
            attempt_summary_verdict = $visualFullGateAttemptSummaryVerdict
            attempt_summary_full_visual_gate_status = $visualFullGateAttemptSummaryFullStatus
            attempt_summary_stage_count = $visualFullGateAttemptSummaryStageCount
            attempt_summary_passed_stage_count = $visualFullGateAttemptSummaryPassedStageCount
            attempt_summary_incomplete_stage_count = $visualFullGateAttemptSummaryIncompleteStageCount
            attempt_summary_visual_baseline_render_status = $visualFullGateAttemptSummaryVisualBaselineRenderStatus
            attempt_summary_visual_baseline_fresh_rendered_count = $visualFullGateAttemptSummaryFreshRenderedCount
            attempt_summary_expected_visual_render_count = $visualFullGateAttemptSummaryExpectedVisualRenderCount
            attempt_summary_visual_baseline_fresh_missing_sample_count = $visualFullGateAttemptSummaryFreshMissingSampleCount
            attempt_summary_visual_baseline_resume_needed = $visualFullGateAttemptSummaryResumeNeeded
            attempt_summary_visual_baseline_resume_slice_offset = $visualFullGateAttemptSummaryResumeSliceOffset
            attempt_summary_visual_baseline_resume_slice_limit = $visualFullGateAttemptSummaryResumeSliceLimit
            attempt_summary_visual_baseline_resume_slice_command_template = $visualFullGateAttemptSummaryResumeSliceCommandTemplate
            attempt_summary_aggregate_contact_sheet_status = $visualFullGateAttemptSummaryContactSheetStatus
            attempt_summary_aggregate_contact_sheet_bytes = $visualFullGateAttemptSummaryContactSheetBytes
            segmented_gate_status = $visualSegmentedGateStatus
            segmented_gate_verdict = $visualSegmentedGateVerdict
            segmented_full_coverage_evidence = $visualGateSegmentedFullCoverageEvidence
            segmented_gate_full_visual_gate_status = $visualSegmentedGateFullStatus
            segmented_gate_evidence_scope = $visualSegmentedGateEvidenceScope
            segmented_gate_boundary = $visualSegmentedGateBoundary
            segmented_gate_summary_json = $visualSegmentedGateSummaryJsonDisplay
            segmented_gate_slice_summary_count = $visualSegmentedGateSliceSummaryCount
            segmented_gate_slice_pass_count = $visualSegmentedGateSlicePassCount
            segmented_gate_slice_failed_count = $visualSegmentedGateSliceFailedCount
            segmented_gate_covered_baseline_count = $visualSegmentedGateCoveredBaselineCount
            segmented_gate_expected_visual_render_count = $visualSegmentedGateExpectedVisualRenderCount
            segmented_gate_visual_baseline_render_status = $visualSegmentedGateVisualBaselineRenderStatus
            segmented_gate_aggregate_contact_sheet_status = $visualSegmentedGateAggregateContactSheetStatus
            segmented_gate_aggregate_contact_sheet_display = $visualSegmentedGateAggregateContactSheetDisplay
            segmented_gate_aggregate_contact_sheet_bytes = $visualSegmentedGateAggregateContactSheetBytes
            segmented_gate_resume_status = $visualSegmentedGateResumeStatus
            segmented_gate_resume_tail_fully_planned = $visualSegmentedGateResumeTailFullyPlanned
            segmented_gate_resume_total_slice_count = $visualSegmentedGateResumeTotalSliceCount
            segmented_gate_resume_passed_slice_count = $visualSegmentedGateResumePassedSliceCount
            segmented_gate_resume_aggregate_rebuild_status = $visualSegmentedGateResumeAggregateRebuildStatus
            release_owner_acceptance_required = $true
            release_owner_acceptance_policy = "release_owner_may_accept_segmented_full_coverage_with_explicit_single_run_debt"
            release_owner_acceptance_boundary = "acceptance_does_not_replace_fresh_single_run_full_visual_gate"
            release_owner_acceptance_required_evidence = @(
                "preflight_status_ready",
                "visual_gate_summary_pass",
                "visual_gate_segmented_full_coverage_evidence",
                "aggregate_contact_sheet_non_empty",
                "pdf_bounded_ctest_evidence"
            )
            release_owner_acceptance_command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_pdf_release_readiness.ps1 -OutputJson .\output\pdf-release-readiness-current\summary.json; document release-owner acceptance in the release handoff without changing full_visual_gate_status."
            release_owner_acceptance_trace_marker = "pdf_visual_gate_release_owner_acceptance_trace"
            summary_json = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedVisualFullGateGuardedSummaryJson
        }
}

$fullCtestStatus = if ($null -eq $fullCtest) { "missing" } else { [string](Get-OptionalPropertyValue -Object $fullCtest -Name "status") }
$fullCtestVerdict = if ($null -eq $fullCtest) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $fullCtest -Name "verdict") }
$fullCtestOuterGuardStatus = if ($null -eq $fullCtest) { "not_run" } else { [string](Get-OptionalPropertyValue -Object $fullCtest -Name "outer_guard_status") }
$fullCtestOuterGuardTimedOut = if ($null -eq $fullCtest) { $false } else { [bool](Get-OptionalPropertyValue -Object $fullCtest -Name "outer_guard_timed_out") }
$fullCtestSelectedCount = if ($null -eq $fullCtest) { 0 } else { Get-OptionalPropertyValue -Object $fullCtest -Name "selected_test_count" }
$fullCtestCompletedCount = if ($null -eq $fullCtest) { 0 } else { Get-OptionalPropertyValue -Object $fullCtest -Name "completed_test_count" }
$fullCtestPassedCount = if ($null -eq $fullCtest) { 0 } else { Get-OptionalPropertyValue -Object $fullCtest -Name "passed_test_count" }
$fullCtestFailedCount = if ($null -eq $fullCtest) { 0 } else { Get-OptionalPropertyValue -Object $fullCtest -Name "failed_test_count" }
$fullCtestSkippedCount = if ($null -eq $fullCtest) { 0 } else { Get-OptionalPropertyValue -Object $fullCtest -Name "skipped_test_count" }
$fullCtestNotRunCount = if ($null -eq $fullCtest) { 0 } else { Get-OptionalPropertyValue -Object $fullCtest -Name "not_run_test_count" }
$fullCtestSelectedCountInt = Convert-ToIntOrZero -Value $fullCtestSelectedCount
$fullCtestCompletedCountInt = Convert-ToIntOrZero -Value $fullCtestCompletedCount
$fullCtestFailedCountInt = Convert-ToIntOrZero -Value $fullCtestFailedCount
$fullCtestNotRunCountInt = Convert-ToIntOrZero -Value $fullCtestNotRunCount
$fullCtestCompletionPercent = if ($fullCtestSelectedCountInt -gt 0) { [Math]::Round(($fullCtestCompletedCountInt / $fullCtestSelectedCountInt) * 100, 1) } else { 0 }
$fullCtestRemainingTestCount = [Math]::Max(0, $fullCtestNotRunCountInt)
$fullCtestZeroFailedTestsObserved = ($fullCtestSummaryExists -and $fullCtestSelectedCountInt -gt 0 -and $fullCtestFailedCountInt -eq 0)
$fullCtestCompleted = ($fullCtestStatus -eq "pass" -and $fullCtestVerdict -eq "pass" -and $fullCtestOuterGuardStatus -eq "completed" -and -not $fullCtestOuterGuardTimedOut)

$fullCtestRemainingStatus = if ($null -eq $fullCtestRemaining) { "missing" } else { [string](Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "status") }
$fullCtestRemainingVerdict = if ($null -eq $fullCtestRemaining) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "verdict") }
$fullCtestRemainingOuterGuardStatus = if ($null -eq $fullCtestRemaining) { "not_run" } else { [string](Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "outer_guard_status") }
$fullCtestRemainingOuterGuardTimedOut = if ($null -eq $fullCtestRemaining) { $false } else { [bool](Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "outer_guard_timed_out") }
$fullCtestRemainingSelectedCount = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "selected_test_count" }
$fullCtestRemainingCompletedCount = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "completed_test_count" }
$fullCtestRemainingFailedCount = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "failed_test_count" }
$fullCtestRemainingSkippedCount = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "skipped_test_count" }
$fullCtestRemainingNotRunCount = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "not_run_test_count" }
$fullCtestRemainingCompletionPercent = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "completion_percent" }
$fullCtestRemainingCombinedSelectedCount = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "combined_selected_test_count" }
$fullCtestRemainingCombinedCompletedCount = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "combined_completed_test_count" }
$fullCtestRemainingCombinedFailedCount = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "combined_failed_test_count" }
$fullCtestRemainingCombinedSkippedCount = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "combined_skipped_test_count" }
$fullCtestRemainingCombinedNotRunCount = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "combined_not_run_test_count" }
$fullCtestRemainingCombinedCompletionPercent = if ($null -eq $fullCtestRemaining) { 0 } else { Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "combined_completion_percent" }
$fullCtestRemainingCombinedZeroFailed = if ($null -eq $fullCtestRemaining) { $false } else { [bool](Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "combined_zero_failed_tests_observed") }
$fullCtestRemainingTailCoversPreviousRemaining = if ($null -eq $fullCtestRemaining) { $false } else { [bool](Get-OptionalPropertyValue -Object $fullCtestRemaining -Name "combined_tail_covers_previous_remaining") }
$fullCtestRemainingSelectedCountInt = Convert-ToIntOrZero -Value $fullCtestRemainingSelectedCount
$fullCtestRemainingCompletedCountInt = Convert-ToIntOrZero -Value $fullCtestRemainingCompletedCount
$fullCtestRemainingFailedCountInt = Convert-ToIntOrZero -Value $fullCtestRemainingFailedCount
$fullCtestRemainingNotRunCountInt = Convert-ToIntOrZero -Value $fullCtestRemainingNotRunCount
$fullCtestRemainingCombinedSelectedCountInt = Convert-ToIntOrZero -Value $fullCtestRemainingCombinedSelectedCount
$fullCtestRemainingCombinedCompletedCountInt = Convert-ToIntOrZero -Value $fullCtestRemainingCombinedCompletedCount
$fullCtestRemainingCombinedFailedCountInt = Convert-ToIntOrZero -Value $fullCtestRemainingCombinedFailedCount
$fullCtestRemainingCombinedNotRunCountInt = Convert-ToIntOrZero -Value $fullCtestRemainingCombinedNotRunCount
$fullCtestCombinedEvidenceCompleted = (
    $fullCtestCompleted -or (
        $fullCtestRemainingSummaryExists -and
        $fullCtestRemainingStatus -eq "pass" -and
        $fullCtestRemainingVerdict -eq "pass" -and
        $fullCtestRemainingOuterGuardStatus -eq "completed" -and
        -not $fullCtestRemainingOuterGuardTimedOut -and
        $fullCtestRemainingSelectedCountInt -gt 0 -and
        $fullCtestRemainingCompletedCountInt -eq $fullCtestRemainingSelectedCountInt -and
        $fullCtestRemainingFailedCountInt -eq 0 -and
        $fullCtestRemainingNotRunCountInt -eq 0 -and
        $fullCtestRemainingTailCoversPreviousRemaining -and
        $fullCtestRemainingCombinedSelectedCountInt -gt 0 -and
        $fullCtestRemainingCombinedCompletedCountInt -ge $fullCtestRemainingCombinedSelectedCountInt -and
        $fullCtestRemainingCombinedFailedCountInt -eq 0 -and
        $fullCtestRemainingCombinedNotRunCountInt -eq 0
    )
)
if (-not $fullCtestCombinedEvidenceCompleted) {
    Add-Warning -Warnings $warnings -Id "pdf_full_ctest.not_completed_in_current_window" `
        -Message "A full ctest -R pdf_ run or guarded full-attempt plus remaining-tail completion evidence is still required when resources allow; bounded/static evidence does not replace the full PDF CTest suite." `
        -Details @{
            status = $fullCtestStatus
            verdict = $fullCtestVerdict
            outer_guard_status = $fullCtestOuterGuardStatus
            outer_guard_timed_out = $fullCtestOuterGuardTimedOut
            selected_test_count = $fullCtestSelectedCount
            completed_test_count = $fullCtestCompletedCount
            passed_test_count = $fullCtestPassedCount
            failed_test_count = $fullCtestFailedCount
            skipped_test_count = $fullCtestSkippedCount
            not_run_test_count = $fullCtestNotRunCount
            completion_percent = $fullCtestCompletionPercent
            remaining_test_count = $fullCtestRemainingTestCount
            zero_failed_tests_observed = $fullCtestZeroFailedTestsObserved
            summary_json = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedFullCtestSummaryJson
            remaining_summary_exists = $fullCtestRemainingSummaryExists
            remaining_status = $fullCtestRemainingStatus
            remaining_verdict = $fullCtestRemainingVerdict
            remaining_outer_guard_status = $fullCtestRemainingOuterGuardStatus
            remaining_outer_guard_timed_out = $fullCtestRemainingOuterGuardTimedOut
            remaining_selected_test_count = $fullCtestRemainingSelectedCount
            remaining_completed_test_count = $fullCtestRemainingCompletedCount
            remaining_failed_test_count = $fullCtestRemainingFailedCount
            remaining_skipped_test_count = $fullCtestRemainingSkippedCount
            remaining_not_run_test_count = $fullCtestRemainingNotRunCount
            remaining_completion_percent = $fullCtestRemainingCompletionPercent
            combined_selected_test_count = $fullCtestRemainingCombinedSelectedCount
            combined_completed_test_count = $fullCtestRemainingCombinedCompletedCount
            combined_failed_test_count = $fullCtestRemainingCombinedFailedCount
            combined_skipped_test_count = $fullCtestRemainingCombinedSkippedCount
            combined_not_run_test_count = $fullCtestRemainingCombinedNotRunCount
            combined_completion_percent = $fullCtestRemainingCombinedCompletionPercent
            combined_zero_failed_tests_observed = $fullCtestRemainingCombinedZeroFailed
            combined_tail_covers_previous_remaining = $fullCtestRemainingTailCoversPreviousRemaining
            remaining_summary_json = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedFullCtestRemainingSummaryJson
        }
}

$failedChecks = @($checks | Where-Object { [string]$_.status -ne "pass" })
$status = if ($failedChecks.Count -eq 0) { "pass" } else { "blocked" }
$verdict = if ($failedChecks.Count -ne 0) {
    "blocked"
} elseif ($warnings.Count -gt 0) {
    "pass_with_warnings"
} else {
    "pass"
}
$releaseReady = ($failedChecks.Count -eq 0)
$boundary = if ($failedChecks.Count -ne 0) {
    "Blocked means required PDF release evidence failed one or more machine checks."
} elseif ($warnings.Count -gt 0) {
    "Pass-with-warnings means current persisted release evidence is coherent; it does not claim a fresh non-FinalizeOnly full visual gate or full pdf_ CTest run completed in this resource window. Any release-owner acceptance of segmented visual evidence must keep the single-run full visual gate debt explicit."
} else {
    "Pass means current persisted release evidence is coherent and no full visual gate or full pdf_ CTest completion warnings remain; combined CTest tail evidence is reported separately from single full-run completion."
}

$summary = [ordered]@{
    schema = "featherdoc.pdf_release_readiness_check.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    verdict = $verdict
    release_ready = $releaseReady
    evidence_scope = "persisted_pdf_release_evidence_only"
    evidence_scope_note = "This read-only gate does not run CMake, CTest, rendering, Office, LibreOffice, browsers, or PDF generation."
    preflight_summary_json = $resolvedPreflightSummaryJson
    preflight_summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedPreflightSummaryJson
    visual_gate_summary_json = $resolvedVisualGateSummaryJson
    visual_gate_summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedVisualGateSummaryJson
    visual_gate_finalize_only = $visualGateFinalizeOnly
    visual_gate_skip_preflight = $visualGateSkipPreflight
    visual_gate_fresh_full_guarded_evidence = $visualGateFreshFullGuardedEvidence
    visual_gate_pass_summary_before_outer_timeout = $visualGatePassSummaryBeforeOuterTimeout
    visual_gate_segmented_full_coverage_evidence = $visualGateSegmentedFullCoverageEvidence
    visual_gate_release_evidence_accepted = $visualGateReleaseEvidenceAccepted
    aggregate_contact_sheet = $contactSheetPath
    aggregate_contact_sheet_display = if ([string]::IsNullOrWhiteSpace($contactSheetPath)) { "" } else { Get-DisplayPath -RepoRoot $repoRoot -Path $contactSheetPath }
    aggregate_contact_sheet_bytes = $contactSheetBytes
    manifest_json = $resolvedManifestJson
    manifest_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedManifestJson
    readiness_checklist = $resolvedReadinessChecklist
    readiness_checklist_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedReadinessChecklist
    visual_full_gate_guarded_summary_json = $resolvedVisualFullGateGuardedSummaryJson
    visual_full_gate_guarded_summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedVisualFullGateGuardedSummaryJson
    visual_full_gate_guarded_summary_exists = $visualFullGateGuardedSummaryExists
    visual_full_gate_status = $visualFullGateStatus
    visual_full_gate_verdict = $visualFullGateVerdict
    visual_full_gate_full_visual_gate_status = $visualFullGateFullStatus
    visual_full_gate_outer_guard_status = $visualFullGateOuterGuardStatus
    visual_full_gate_outer_guard_timed_out = $visualFullGateOuterGuardTimedOut
    visual_full_gate_pass_summary_before_outer_timeout = $visualFullGatePassSummaryBeforeOuterTimeout
    visual_full_gate_attempt_summary_json = $visualFullGateAttemptSummaryJson
    visual_full_gate_attempt_summary_exists = $visualFullGateAttemptSummaryExists
    visual_full_gate_attempt_summary_status = $visualFullGateAttemptSummaryStatus
    visual_full_gate_attempt_summary_verdict = $visualFullGateAttemptSummaryVerdict
    visual_full_gate_attempt_summary_full_visual_gate_status = $visualFullGateAttemptSummaryFullStatus
    visual_full_gate_attempt_summary_stage_count = $visualFullGateAttemptSummaryStageCount
    visual_full_gate_attempt_summary_passed_stage_count = $visualFullGateAttemptSummaryPassedStageCount
    visual_full_gate_attempt_summary_incomplete_stage_count = $visualFullGateAttemptSummaryIncompleteStageCount
    visual_full_gate_attempt_summary_visual_baseline_render_status = $visualFullGateAttemptSummaryVisualBaselineRenderStatus
    visual_full_gate_attempt_summary_visual_baseline_fresh_rendered_count = $visualFullGateAttemptSummaryFreshRenderedCount
    visual_full_gate_attempt_summary_expected_visual_render_count = $visualFullGateAttemptSummaryExpectedVisualRenderCount
    visual_full_gate_attempt_summary_visual_baseline_fresh_missing_sample_count = $visualFullGateAttemptSummaryFreshMissingSampleCount
    visual_full_gate_attempt_summary_visual_baseline_resume_needed = $visualFullGateAttemptSummaryResumeNeeded
    visual_full_gate_attempt_summary_visual_baseline_resume_slice_offset = $visualFullGateAttemptSummaryResumeSliceOffset
    visual_full_gate_attempt_summary_visual_baseline_resume_slice_limit = $visualFullGateAttemptSummaryResumeSliceLimit
    visual_full_gate_attempt_summary_visual_baseline_resume_slice_command_template = $visualFullGateAttemptSummaryResumeSliceCommandTemplate
    visual_full_gate_attempt_summary_aggregate_contact_sheet_status = $visualFullGateAttemptSummaryContactSheetStatus
    visual_full_gate_attempt_summary_aggregate_contact_sheet_bytes = $visualFullGateAttemptSummaryContactSheetBytes
    visual_full_gate_attempt_stage_count = $visualFullGateAttemptStageCount
    visual_full_gate_attempt_passed_stage_count = $visualFullGateAttemptPassedStageCount
    visual_full_gate_attempt_failed_stage_count = $visualFullGateAttemptFailedStageCount
    visual_full_gate_attempt_incomplete_stage_count = $visualFullGateAttemptIncompleteStageCount
    visual_full_gate_attempt_visual_baseline_fresh_rendered_count = $visualFullGateAttemptFreshRenderedCount
    visual_full_gate_attempt_aggregate_contact_sheet_status = $visualFullGateAttemptContactSheetStatus
    visual_segmented_gate_summary_json = $resolvedVisualSegmentedGateSummaryJson
    visual_segmented_gate_summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedVisualSegmentedGateSummaryJson
    visual_segmented_gate_summary_exists = $visualSegmentedGateSummaryExists
    visual_segmented_gate_status = $visualSegmentedGateStatus
    visual_segmented_gate_verdict = $visualSegmentedGateVerdict
    visual_segmented_gate_full_visual_gate_status = $visualSegmentedGateFullStatus
    visual_segmented_gate_evidence_scope = $visualSegmentedGateEvidenceScope
    visual_segmented_gate_boundary = $visualSegmentedGateBoundary
    visual_segmented_gate_slice_summary_count = $visualSegmentedGateSliceSummaryCount
    visual_segmented_gate_slice_pass_count = $visualSegmentedGateSlicePassCount
    visual_segmented_gate_slice_failed_count = $visualSegmentedGateSliceFailedCount
    visual_segmented_gate_covered_baseline_count = $visualSegmentedGateCoveredBaselineCount
    visual_segmented_gate_expected_visual_render_count = $visualSegmentedGateExpectedVisualRenderCount
    visual_segmented_gate_attempt_stage_count = $visualSegmentedGateAttemptStageCount
    visual_segmented_gate_attempt_passed_stage_count = $visualSegmentedGateAttemptPassedStageCount
    visual_segmented_gate_visual_baseline_render_status = $visualSegmentedGateVisualBaselineRenderStatus
    visual_segmented_gate_aggregate_contact_sheet_status = $visualSegmentedGateAggregateContactSheetStatus
    visual_segmented_gate_aggregate_contact_sheet_display = $visualSegmentedGateAggregateContactSheetDisplay
    visual_segmented_gate_aggregate_contact_sheet_bytes = $visualSegmentedGateAggregateContactSheetBytes
    visual_segmented_gate_aggregate_rebuild_status = $visualSegmentedGateAggregateRebuildStatus
    visual_segmented_gate_aggregate_rebuild_selected_baseline_count = $visualSegmentedGateAggregateRebuildSelectedBaselineCount
    visual_segmented_gate_resume_status = $visualSegmentedGateResumeStatus
    visual_segmented_gate_resume_tail_fully_planned = $visualSegmentedGateResumeTailFullyPlanned
    visual_segmented_gate_resume_total_slice_count = $visualSegmentedGateResumeTotalSliceCount
    visual_segmented_gate_resume_passed_slice_count = $visualSegmentedGateResumePassedSliceCount
    visual_segmented_gate_resume_aggregate_rebuild_status = $visualSegmentedGateResumeAggregateRebuildStatus
    full_ctest_summary_json = $resolvedFullCtestSummaryJson
    full_ctest_summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedFullCtestSummaryJson
    full_ctest_summary_exists = $fullCtestSummaryExists
    full_ctest_status = $fullCtestStatus
    full_ctest_verdict = $fullCtestVerdict
    full_ctest_outer_guard_status = $fullCtestOuterGuardStatus
    full_ctest_outer_guard_timed_out = $fullCtestOuterGuardTimedOut
    full_ctest_selected_test_count = $fullCtestSelectedCount
    full_ctest_completed_test_count = $fullCtestCompletedCount
    full_ctest_passed_test_count = $fullCtestPassedCount
    full_ctest_failed_test_count = $fullCtestFailedCount
    full_ctest_skipped_test_count = $fullCtestSkippedCount
    full_ctest_not_run_test_count = $fullCtestNotRunCount
    full_ctest_completion_percent = $fullCtestCompletionPercent
    full_ctest_remaining_test_count = $fullCtestRemainingTestCount
    full_ctest_zero_failed_tests_observed = $fullCtestZeroFailedTestsObserved
    full_ctest_single_run_completed = $fullCtestCompleted
    full_ctest_combined_evidence_completed = $fullCtestCombinedEvidenceCompleted
    full_ctest_remaining_summary_json = $resolvedFullCtestRemainingSummaryJson
    full_ctest_remaining_summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedFullCtestRemainingSummaryJson
    full_ctest_remaining_summary_exists = $fullCtestRemainingSummaryExists
    full_ctest_remaining_status = $fullCtestRemainingStatus
    full_ctest_remaining_verdict = $fullCtestRemainingVerdict
    full_ctest_remaining_outer_guard_status = $fullCtestRemainingOuterGuardStatus
    full_ctest_remaining_outer_guard_timed_out = $fullCtestRemainingOuterGuardTimedOut
    full_ctest_remaining_selected_test_count = $fullCtestRemainingSelectedCount
    full_ctest_remaining_completed_test_count = $fullCtestRemainingCompletedCount
    full_ctest_remaining_failed_test_count = $fullCtestRemainingFailedCount
    full_ctest_remaining_skipped_test_count = $fullCtestRemainingSkippedCount
    full_ctest_remaining_not_run_test_count = $fullCtestRemainingNotRunCount
    full_ctest_remaining_completion_percent = $fullCtestRemainingCompletionPercent
    full_ctest_combined_selected_test_count = $fullCtestRemainingCombinedSelectedCount
    full_ctest_combined_completed_test_count = $fullCtestRemainingCombinedCompletedCount
    full_ctest_combined_failed_test_count = $fullCtestRemainingCombinedFailedCount
    full_ctest_combined_skipped_test_count = $fullCtestRemainingCombinedSkippedCount
    full_ctest_combined_not_run_test_count = $fullCtestRemainingCombinedNotRunCount
    full_ctest_combined_completion_percent = $fullCtestRemainingCombinedCompletionPercent
    full_ctest_combined_zero_failed_tests_observed = $fullCtestRemainingCombinedZeroFailed
    full_ctest_remaining_tail_covers_previous_remaining = $fullCtestRemainingTailCoversPreviousRemaining
    check_count = $checks.Count
    failed_check_count = $failedChecks.Count
    warning_count = $warnings.Count
    checks = @($checks)
    failed_checks = @($failedChecks | ForEach-Object { $_.name })
    warnings = @($warnings)
    boundary = $boundary
    marker = "pdf_release_readiness_machine_gate_trace"
}

if (-not [string]::IsNullOrWhiteSpace($resolvedOutputJson)) {
    $outputDir = Split-Path -Parent $resolvedOutputJson
    if (-not [string]::IsNullOrWhiteSpace($outputDir)) {
        New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    }
    ($summary | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8
}

$summary | ConvertTo-Json -Depth 16
if ($Strict -and -not $releaseReady) {
    exit 1
}
