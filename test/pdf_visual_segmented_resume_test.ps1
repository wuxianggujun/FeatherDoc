param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $RepoRoot "build\pdf_visual_segmented_resume_test"
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw $Message
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Write-TextFile {
    param([string]$Path, [string]$Text)
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    $Text | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-Script {
    param([string]$ScriptPath, [string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellPath = (Get-Command powershell.exe -ErrorAction Stop).Source
    }

    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = @($output | ForEach-Object { $_.ToString() })
        Text = (@($output | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine)
    }
}

$resolvedRepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_pdf_visual_segmented_resume.ps1"
$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count -gt 0) {
    throw "run_pdf_visual_segmented_resume.ps1 has parse errors."
}

$buildDir = Join-Path $resolvedWorkingDir "build"
$outputDir = Join-Path $resolvedWorkingDir "output\pdf-visual-release-gate-current"
$reportDir = Join-Path $outputDir "report"
New-Item -ItemType Directory -Path $buildDir, $reportDir -Force | Out-Null

$attemptSummaryPath = Join-Path $reportDir "attempt-summary.json"
Write-JsonFile -Path $attemptSummaryPath -Value ([ordered]@{
    schema = "featherdoc.pdf_visual_gate_attempt_summary.v1"
    status = "partial"
    verdict = "not_complete"
    full_visual_gate_status = "not_complete"
    visual_baseline_resume_needed = $true
    visual_baseline_resume_slice_offset = 3
    visual_baseline_resume_slice_limit = 5
    visual_baseline_fresh_rendered_count = 3
    expected_visual_render_count = 8
})

$fakeVisualGate = Join-Path $resolvedWorkingDir "fake-visual-gate.ps1"
Write-TextFile -Path $fakeVisualGate -Text @'
param(
    [string]$BuildDir,
    [string]$OutputDir,
    [int]$Dpi,
    [int]$MinFreeMemoryMB,
    [switch]$VisualBaselineSliceOnly,
    [int]$VisualBaselineOffset,
    [int]$VisualBaselineLimit,
    [switch]$RebuildAggregateContactSheetOnly,
    [switch]$SkipPreflight,
    [switch]$SkipMemoryGuard
)

$reportDir = Join-Path $OutputDir "report"
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
if ($VisualBaselineSliceOnly) {
    $sliceId = "visual-baseline-slice-offset-$VisualBaselineOffset-limit-$VisualBaselineLimit"
    $summaryPath = Join-Path $reportDir "$sliceId-summary.json"
    $contactSheet = Join-Path $reportDir "$sliceId-contact-sheet.png"
    [System.IO.File]::WriteAllBytes($contactSheet, [byte[]](1, 2, 3))
    ([ordered]@{
        schema = "featherdoc.pdf_visual_baseline_slice.v1"
        status = "pass"
        verdict = "pass"
        full_visual_gate_status = "not_complete"
        evidence_scope = "visual_baseline_slice_only"
        visual_baseline_offset = $VisualBaselineOffset
        visual_baseline_limit = $VisualBaselineLimit
        selected_baseline_count = $VisualBaselineLimit
        total_baseline_count = 8
        expected_visual_render_count = 8
        slice_contact_sheet = $contactSheet
    } | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
    exit 0
}

if ($RebuildAggregateContactSheetOnly) {
    $contactSheet = Join-Path $reportDir "aggregate-contact-sheet.png"
    [System.IO.File]::WriteAllBytes($contactSheet, [byte[]](1, 2, 3, 4))
    ([ordered]@{
        schema = "featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1"
        status = "pass"
        verdict = "pass"
        full_visual_gate_status = "not_complete"
        evidence_scope = "aggregate_contact_sheet_rebuild_only"
        aggregate_contact_sheet = $contactSheet
        selected_baseline_count = 8
        total_baseline_count = 8
        visual_baseline_manifest_count = 6
        expected_visual_render_count = 8
    } | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath (Join-Path $reportDir "aggregate-contact-sheet-rebuild-summary.json") -Encoding UTF8
    exit 0
}

exit 1
'@

$fakeSegmentedSummary = Join-Path $resolvedWorkingDir "fake-segmented-summary.ps1"
Write-TextFile -Path $fakeSegmentedSummary -Text @'
param(
    [string]$RepoRoot,
    [string]$ReportDir
)

([ordered]@{
    schema = "featherdoc.pdf_visual_segmented_gate_summary.v1"
    status = "pass"
    verdict = "pass"
    full_visual_gate_status = "not_complete"
    evidence_scope = "segmented_visual_gate_auxiliary_only"
    covered_baseline_count = 8
    expected_visual_render_count = 8
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath (Join-Path $ReportDir "segmented-summary.json") -Encoding UTF8
exit 0
'@

$planSummaryPath = Join-Path $reportDir "segmented-resume-plan-summary.json"
$planResult = Invoke-Script -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $buildDir,
    "-OutputDir", $outputDir,
    "-AttemptSummaryJson", $attemptSummaryPath,
    "-OutputJson", $planSummaryPath,
    "-VisualGateScript", $fakeVisualGate,
    "-SegmentedSummaryScript", $fakeSegmentedSummary,
    "-ChunkSize", "2",
    "-PerSliceTimeoutSeconds", "10",
    "-PlanOnly"
)
Assert-Equal -Actual $planResult.ExitCode -Expected 0 `
    -Message "Plan-only segmented resume should exit 0. Output: $($planResult.Text)"
$planSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $planSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$planSummary.status) -Expected "planned" `
    -Message "Plan-only run should report planned status."
Assert-Equal -Actual ([int]$planSummary.planned_slice_count) -Expected 3 `
    -Message "Plan-only run should split resume tail into chunks."
Assert-Equal -Actual ([int]$planSummary.planned_slices[0].offset) -Expected 3 `
    -Message "Plan should start at the attempt resume offset."
Assert-Equal -Actual ([int]$planSummary.planned_slices[2].limit) -Expected 1 `
    -Message "Plan should keep the final tail slice size."
Assert-Equal -Actual ([string]$planSummary.full_visual_gate_status) -Expected "not_complete" `
    -Message "Plan summary must not claim a full visual gate pass."

$runSummaryPath = Join-Path $reportDir "segmented-resume-summary.json"
$runResult = Invoke-Script -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $buildDir,
    "-OutputDir", $outputDir,
    "-AttemptSummaryJson", $attemptSummaryPath,
    "-OutputJson", $runSummaryPath,
    "-VisualGateScript", $fakeVisualGate,
    "-SegmentedSummaryScript", $fakeSegmentedSummary,
    "-ChunkSize", "2",
    "-PerSliceTimeoutSeconds", "10",
    "-SkipPreflight"
)
Assert-Equal -Actual $runResult.ExitCode -Expected 0 `
    -Message "Segmented resume fixture should exit 0. Output: $($runResult.Text)"
$runSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $runSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$runSummary.schema) -Expected "featherdoc.pdf_visual_segmented_resume_summary.v1" `
    -Message "Segmented resume summary should expose a stable schema."
Assert-Equal -Actual ([string]$runSummary.status) -Expected "pass" `
    -Message "All fake slices should produce pass status."
Assert-Equal -Actual ([string]$runSummary.full_visual_gate_status) -Expected "not_complete" `
    -Message "Segmented resume must not promote full visual gate status."
Assert-Equal -Actual ([string]$runSummary.evidence_scope) -Expected "visual_segmented_resume_auxiliary_only" `
    -Message "Segmented resume should keep auxiliary scope explicit."
Assert-Equal -Actual ([string]$runSummary.boundary) -Expected "segmented_resume_does_not_replace_full_visual_gate_verdict" `
    -Message "Segmented resume should preserve the full-gate boundary."
Assert-Equal -Actual ([int]$runSummary.planned_slice_count) -Expected 3 `
    -Message "Segmented resume should preserve planned slice count."
Assert-Equal -Actual ([int]$runSummary.executed_slice_count) -Expected 3 `
    -Message "Segmented resume should execute all fake slices."
Assert-Equal -Actual ([int]$runSummary.passed_slice_count) -Expected 3 `
    -Message "Segmented resume should count passing slices."
Assert-Equal -Actual ([string]$runSummary.aggregate_rebuild_status) -Expected "pass" `
    -Message "Segmented resume should rebuild aggregate contact sheet after slices pass."
Assert-Equal -Actual ([string]$runSummary.segmented_summary_status) -Expected "pass" `
    -Message "Segmented resume should refresh segmented summary after aggregate rebuild."
Assert-True -Condition ((Test-Path -LiteralPath (Join-Path $reportDir "segmented-summary.json")) -and (Test-Path -LiteralPath (Join-Path $reportDir "aggregate-contact-sheet.png"))) `
    -Message "Segmented resume should leave aggregate and segmented evidence files."

Write-Host "PDF visual segmented resume regression passed."
