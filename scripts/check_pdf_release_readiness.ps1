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
if ($null -ne $visual) {
    $status = [string](Get-OptionalPropertyValue -Object $visual -Name "status")
    $verdict = [string](Get-OptionalPropertyValue -Object $visual -Name "verdict")
    $finalizeOnly = [bool](Get-OptionalPropertyValue -Object $visual -Name "finalize_only")
    $skipPreflight = [bool](Get-OptionalPropertyValue -Object $visual -Name "skip_preflight")
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
    Add-Check -Checks $checks -Name "visual_gate_finalize_evidence" -Passed $finalizeOnly `
        -Message "Current PDF visual gate summary must be an explicit finalize evidence summary." `
        -Details @{ actual = $finalizeOnly; expected = $true }
    Add-Check -Checks $checks -Name "visual_gate_manifest_counts" -Passed ((Test-IntegerEquals -Value $visualBaselineManifestCount -Expected 42) -and (Test-IntegerAtLeast -Value $baselinesCount -Minimum 42) -and (Test-IntegerEquals -Value $cjkManifestCount -Expected 43) -and (Test-IntegerEquals -Value $cjkCopySearchCount -Expected 43)) `
        -Message "Current PDF visual gate summary must preserve release manifest counts." `
        -Details @{
            visual_baseline_manifest_count = $visualBaselineManifestCount
            baselines_count = $baselinesCount
            cjk_manifest_count = $cjkManifestCount
            cjk_copy_search_count = $cjkCopySearchCount
        }

    if (-not $skipPreflight) {
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
        "featherdoc.pdf_release_readiness_check.v1",
        "pdf_visual_gate_evidence",
        "pdf_bounded_ctest_evidence",
        "release_entry_pdf_readiness_checklist_trace"
    )
    foreach ($marker in $requiredChecklistMarkers) {
        Add-Check -Checks $checks -Name ("readiness_checklist_marker.{0}" -f $marker) -Passed ($checklistText.Contains($marker)) `
            -Message "PDF release-readiness checklist must keep the machine gate marker." `
            -Details @{ marker = $marker }
    }
}

Add-Warning -Warnings $warnings -Id "pdf_full_fresh_visual_gate.not_completed_in_current_window" `
    -Message "A fresh non-FinalizeOnly full visual gate is still required before claiming a new end-to-end render pass; this check only validates current persisted release evidence."
Add-Warning -Warnings $warnings -Id "pdf_full_ctest.not_completed_in_current_window" `
    -Message "A full ctest -R pdf_ run is still required when resources allow; bounded/static evidence does not replace the full PDF CTest suite."

$failedChecks = @($checks | Where-Object { [string]$_.status -ne "pass" })
$status = if ($failedChecks.Count -eq 0) { "pass" } else { "blocked" }
$verdict = if ($failedChecks.Count -eq 0) { "pass_with_warnings" } else { "blocked" }
$releaseReady = ($failedChecks.Count -eq 0)

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
    aggregate_contact_sheet = $contactSheetPath
    aggregate_contact_sheet_display = if ([string]::IsNullOrWhiteSpace($contactSheetPath)) { "" } else { Get-DisplayPath -RepoRoot $repoRoot -Path $contactSheetPath }
    aggregate_contact_sheet_bytes = $contactSheetBytes
    manifest_json = $resolvedManifestJson
    manifest_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedManifestJson
    readiness_checklist = $resolvedReadinessChecklist
    readiness_checklist_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedReadinessChecklist
    check_count = $checks.Count
    failed_check_count = $failedChecks.Count
    warning_count = $warnings.Count
    checks = @($checks)
    failed_checks = @($failedChecks | ForEach-Object { $_.name })
    warnings = @($warnings)
    boundary = "Pass-with-warnings means current persisted release evidence is coherent; it does not claim a fresh non-FinalizeOnly full visual gate or full pdf_ CTest run completed in this resource window."
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
