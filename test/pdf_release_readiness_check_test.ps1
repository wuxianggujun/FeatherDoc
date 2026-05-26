param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Invoke-PowerShellScript {
    param([string]$ScriptPath, [string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellPath = (Get-Command powershell.exe -ErrorAction Stop).Source
    }

    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-ManifestSamples {
    $samples = New-Object 'System.Collections.Generic.List[object]'
    for ($index = 1; $index -le 90; $index++) {
        $sample = [ordered]@{
            id = "sample-$index"
            output_file = "sample-$index.pdf"
        }
        if ($index -le 42) {
            $sample.expect_visual_baseline = $true
        }
        if ($index -le 43) {
            $sample.expect_cjk = $true
        }
        $samples.Add([pscustomobject]$sample) | Out-Null
    }

    return $samples.ToArray()
}

function New-PassingFixture {
    param([string]$Root, [string]$ScriptPath)

    New-Item -ItemType Directory -Path (Join-Path $Root "scripts") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $Root "docs") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $Root "test") -Force | Out-Null
    Copy-Item -LiteralPath $ScriptPath -Destination (Join-Path $Root "scripts\check_pdf_release_readiness.ps1")

    $preflightPath = Join-Path $Root "output\pdf-visual-release-gate-preflight-current\summary.json"
    Write-JsonFile -Path $preflightPath -Value ([ordered]@{
        generated_at = "2026-05-27T01:38:37"
        status = "ready"
        blocking_summary = [ordered]@{
            blocking_check_count = 0
            pdf_dependency_inputs_status = "ready"
            pdf_build_options_enabled = $true
            missing_visual_baseline_pdf_count = 0
            missing_cjk_text_layer_pdf_count = 0
        }
        output_gap_count = 0
        missing_output_count = 0
        evidence_kind = "real_build"
    })

    $contactSheetPath = Join-Path $Root "output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png"
    New-Item -ItemType Directory -Path (Split-Path -Parent $contactSheetPath) -Force | Out-Null
    [System.IO.File]::WriteAllBytes($contactSheetPath, [byte[]](137, 80, 78, 71, 13, 10, 26, 10, 1, 2, 3, 4))
    $visualPath = Join-Path $Root "output\pdf-visual-release-gate-current\report\summary.json"
    Write-JsonFile -Path $visualPath -Value ([ordered]@{
        generated_at = "2026-05-26T12:03:21"
        status = "pass"
        verdict = "pass"
        finalize_only = $true
        skip_preflight = $false
        visual_baseline_manifest_count = 42
        baselines_count = 44
        cjk_manifest_count = 43
        cjk_copy_search_count = 43
        aggregate_contact_sheet = $contactSheetPath
    })

    Write-JsonFile -Path (Join-Path $Root "test\pdf_regression_manifest.json") -Value ([ordered]@{
        samples = @(New-ManifestSamples)
    })

    Write-JsonFile -Path (Join-Path $Root "output\pdf-ctest-current\summary.json") -Value ([ordered]@{
        schema = "featherdoc.pdf_full_ctest_guarded_summary.v1"
        generated_at = "2026-05-27T02:33:46"
        status = "timeout"
        verdict = "not_complete"
        full_ctest_status = "not_complete"
        evidence_scope = "guarded_full_pdf_ctest_attempt"
        ctest_pattern = "pdf_"
        ctest_timeout_seconds = 60
        outer_guard_status = "timed_out"
        outer_guard_timed_out = $true
        outer_guard_timeout_seconds = 60
        selected_test_count = 139
        completed_test_count = 102
        passed_test_count = 96
        failed_test_count = 0
        skipped_test_count = 6
        not_run_test_count = 37
        boundary = "guarded_full_ctest_attempt_does_not_replace_completed_full_ctest"
        marker = "pdf_full_ctest_guarded_summary_trace"
    })

    Write-JsonFile -Path (Join-Path $Root "output\pdf-visual-release-gate-current\report\full-visual-gate-guarded-summary.json") -Value ([ordered]@{
        schema = "featherdoc.pdf_visual_full_gate_guarded_summary.v1"
        generated_at = "2026-05-27T03:14:46"
        status = "timeout"
        verdict = "not_complete"
        full_visual_gate_status = "not_complete"
        evidence_scope = "guarded_full_pdf_visual_gate_attempt"
        outer_guard_status = "timed_out"
        outer_guard_timed_out = $true
        outer_guard_timeout_seconds = 60
        attempt_summary_json_display = ".\output\pdf-visual-release-gate-current\report\attempt-summary.json"
        attempt_stage_count = 6
        attempt_passed_stage_count = 2
        attempt_failed_stage_count = 0
        attempt_incomplete_stage_count = 4
        attempt_visual_baseline_fresh_rendered_count = 3
        attempt_aggregate_contact_sheet_status = "stale"
        boundary = "guarded_full_visual_gate_attempt_does_not_replace_completed_full_visual_gate"
        marker = "pdf_visual_full_gate_guarded_summary_trace"
    })

    Write-JsonFile -Path (Join-Path $Root "output\pdf-visual-release-gate-current\report\segmented-summary.json") -Value ([ordered]@{
        schema = "featherdoc.pdf_visual_segmented_gate_summary.v1"
        generated_at = "2026-05-27T04:10:37"
        status = "partial"
        verdict = "not_complete"
        full_visual_gate_status = "not_complete"
        evidence_scope = "segmented_visual_gate_auxiliary_only"
        boundary = "segmented_summary_does_not_replace_full_visual_gate_verdict"
        summary_json_display = ".\output\pdf-visual-release-gate-current\report\segmented-summary.json"
        slice_summary_count = 5
        slice_pass_count = 5
        slice_failed_count = 0
        covered_baseline_count = 44
        expected_visual_render_count = 44
        attempt_stage_count = 6
        attempt_passed_stage_count = 4
        visual_baseline_render_status = "partial"
        aggregate_contact_sheet_status = "stale"
        aggregate_contact_sheet_display = ".\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png"
        aggregate_contact_sheet_bytes = 1822428
        aggregate_rebuild_status = "pass"
        aggregate_rebuild_selected_baseline_count = 44
        marker = "pdf_visual_segmented_gate_summary_trace"
    })

    @"
PDF release readiness checklist fixture
check_pdf_release_readiness.ps1
run_pdf_visual_full_gate_guarded.ps1
write_pdf_visual_segmented_gate_summary.ps1
run_pdf_full_ctest_guarded.ps1
featherdoc.pdf_release_readiness_check.v1
featherdoc.pdf_visual_full_gate_guarded_summary.v1
featherdoc.pdf_visual_segmented_gate_summary.v1
featherdoc.pdf_full_ctest_guarded_summary.v1
pdf_visual_gate_evidence
pdf_bounded_ctest_evidence
release_entry_pdf_readiness_checklist_trace
pdf_release_readiness_machine_gate_trace
pdf_visual_full_gate_guarded_summary_trace
pdf_visual_segmented_gate_summary_trace
pdf_full_ctest_guarded_summary_trace
"@ | Set-Content -LiteralPath (Join-Path $Root "docs\pdf_release_readiness_checklist_zh.rst") -Encoding UTF8
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    throw "WorkingDir is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_pdf_release_readiness.ps1"
$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count -gt 0) {
    throw "PDF release readiness check script has parse errors."
}

$fixtureRoot = Join-Path $resolvedWorkingDir "fixture-pass"
New-PassingFixture -Root $fixtureRoot -ScriptPath $scriptPath
$fixtureScript = Join-Path $fixtureRoot "scripts\check_pdf_release_readiness.ps1"
$fixtureSummaryPath = Join-Path $resolvedWorkingDir "fixture-readiness-summary.json"
$fixtureResult = Invoke-PowerShellScript -ScriptPath $fixtureScript -Arguments @(
    "-OutputJson", $fixtureSummaryPath
)
Assert-Equal -Actual $fixtureResult.ExitCode -Expected 0 `
    -Message "Fixture PDF release readiness check should pass. Output: $($fixtureResult.Text)"
$fixtureSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $fixtureSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$fixtureSummary.schema) -Expected "featherdoc.pdf_release_readiness_check.v1" `
    -Message "Readiness summary should expose a stable schema."
Assert-Equal -Actual ([string]$fixtureSummary.status) -Expected "pass" `
    -Message "Passing fixture should report pass status."
Assert-Equal -Actual ([string]$fixtureSummary.verdict) -Expected "pass_with_warnings" `
    -Message "Passing fixture should retain warning verdict boundary."
Assert-Equal -Actual ([bool]$fixtureSummary.release_ready) -Expected $true `
    -Message "Release-ready flag should be true when required persisted evidence is coherent."
Assert-Equal -Actual ([int]$fixtureSummary.failed_check_count) -Expected 0 `
    -Message "Passing fixture should not have failed checks."
Assert-Equal -Actual ([int]$fixtureSummary.aggregate_contact_sheet_bytes) -Expected 12 `
    -Message "Passing fixture should record aggregate contact sheet bytes."
Assert-Equal -Actual ([string]$fixtureSummary.visual_full_gate_status) -Expected "timeout" `
    -Message "Passing fixture should preserve guarded full visual gate attempt status."
Assert-Equal -Actual ([string]$fixtureSummary.visual_full_gate_verdict) -Expected "not_complete" `
    -Message "Passing fixture should preserve guarded full visual gate attempt verdict."
Assert-Equal -Actual ([string]$fixtureSummary.visual_full_gate_full_visual_gate_status) -Expected "not_complete" `
    -Message "Passing fixture should preserve guarded full visual gate status."
Assert-Equal -Actual ([string]$fixtureSummary.visual_full_gate_outer_guard_status) -Expected "timed_out" `
    -Message "Passing fixture should preserve guarded full visual gate outer guard status."
Assert-Equal -Actual ([int]$fixtureSummary.visual_full_gate_attempt_passed_stage_count) -Expected 2 `
    -Message "Passing fixture should preserve guarded full visual gate attempt stage count."
Assert-Equal -Actual ([int]$fixtureSummary.visual_full_gate_attempt_visual_baseline_fresh_rendered_count) -Expected 3 `
    -Message "Passing fixture should preserve guarded full visual gate fresh render count."
Assert-Equal -Actual ([string]$fixtureSummary.visual_full_gate_attempt_aggregate_contact_sheet_status) -Expected "stale" `
    -Message "Passing fixture should preserve guarded full visual gate contact sheet status."
Assert-Equal -Actual ([bool]$fixtureSummary.visual_segmented_gate_summary_exists) -Expected $true `
    -Message "Passing fixture should detect segmented visual gate evidence."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_status) -Expected "partial" `
    -Message "Passing fixture should preserve segmented visual gate status."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_verdict) -Expected "not_complete" `
    -Message "Passing fixture should preserve segmented visual gate verdict."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_full_visual_gate_status) -Expected "not_complete" `
    -Message "Passing fixture should preserve segmented visual gate full status boundary."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_evidence_scope) -Expected "segmented_visual_gate_auxiliary_only" `
    -Message "Passing fixture should preserve segmented visual gate auxiliary scope."
Assert-Equal -Actual ([int]$fixtureSummary.visual_segmented_gate_slice_summary_count) -Expected 5 `
    -Message "Passing fixture should preserve segmented visual gate slice count."
Assert-Equal -Actual ([int]$fixtureSummary.visual_segmented_gate_slice_pass_count) -Expected 5 `
    -Message "Passing fixture should preserve segmented visual gate slice pass count."
Assert-Equal -Actual ([int]$fixtureSummary.visual_segmented_gate_covered_baseline_count) -Expected 44 `
    -Message "Passing fixture should preserve segmented visual gate covered baseline count."
Assert-Equal -Actual ([int]$fixtureSummary.visual_segmented_gate_expected_visual_render_count) -Expected 44 `
    -Message "Passing fixture should preserve segmented visual gate expected visual render count."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_aggregate_contact_sheet_status) -Expected "stale" `
    -Message "Passing fixture should preserve segmented visual gate contact sheet status."
Assert-Equal -Actual ([int]$fixtureSummary.visual_segmented_gate_aggregate_contact_sheet_bytes) -Expected 1822428 `
    -Message "Passing fixture should preserve segmented visual gate contact sheet bytes."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_aggregate_rebuild_status) -Expected "pass" `
    -Message "Passing fixture should preserve segmented visual gate aggregate rebuild status."
Assert-Equal -Actual ([string]$fixtureSummary.full_ctest_status) -Expected "timeout" `
    -Message "Passing fixture should preserve guarded full PDF CTest attempt status."
Assert-Equal -Actual ([string]$fixtureSummary.full_ctest_verdict) -Expected "not_complete" `
    -Message "Passing fixture should preserve guarded full PDF CTest attempt verdict."
Assert-Equal -Actual ([string]$fixtureSummary.full_ctest_outer_guard_status) -Expected "timed_out" `
    -Message "Passing fixture should preserve guarded full PDF CTest outer guard status."
Assert-Equal -Actual ([int]$fixtureSummary.full_ctest_selected_test_count) -Expected 139 `
    -Message "Passing fixture should preserve guarded full PDF CTest selected test count."
Assert-Equal -Actual ([int]$fixtureSummary.full_ctest_completed_test_count) -Expected 102 `
    -Message "Passing fixture should preserve guarded full PDF CTest completed test count."
Assert-True -Condition ((@($fixtureSummary.warnings | ForEach-Object { [string]$_.id }) -contains "pdf_full_fresh_visual_gate.not_completed_in_current_window")) `
    -Message "Readiness summary should keep fresh full visual gate debt visible."
Assert-True -Condition ((@($fixtureSummary.warnings | ForEach-Object { [string]$_.id }) -contains "pdf_full_ctest.not_completed_in_current_window")) `
    -Message "Readiness summary should keep full PDF CTest debt visible."
$visualFullGateWarning = @($fixtureSummary.warnings) |
    Where-Object { [string]$_.id -eq "pdf_full_fresh_visual_gate.not_completed_in_current_window" } |
    Select-Object -First 1
Assert-Equal -Actual ([string]$visualFullGateWarning.details.status) -Expected "timeout" `
    -Message "Fresh full visual gate warning should carry the guarded attempt status."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.attempt_passed_stage_count) -Expected 2 `
    -Message "Fresh full visual gate warning should carry guarded attempt stage counts."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.attempt_visual_baseline_fresh_rendered_count) -Expected 3 `
    -Message "Fresh full visual gate warning should carry guarded attempt fresh render counts."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.attempt_aggregate_contact_sheet_status) -Expected "stale" `
    -Message "Fresh full visual gate warning should carry guarded attempt contact sheet status."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.segmented_gate_status) -Expected "partial" `
    -Message "Fresh full visual gate warning should carry segmented visual gate status."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.segmented_gate_verdict) -Expected "not_complete" `
    -Message "Fresh full visual gate warning should carry segmented visual gate verdict."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.segmented_gate_evidence_scope) -Expected "segmented_visual_gate_auxiliary_only" `
    -Message "Fresh full visual gate warning should carry segmented visual gate auxiliary scope."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.segmented_gate_covered_baseline_count) -Expected 44 `
    -Message "Fresh full visual gate warning should carry segmented covered baseline count."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.segmented_gate_expected_visual_render_count) -Expected 44 `
    -Message "Fresh full visual gate warning should carry segmented expected render count."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.segmented_gate_aggregate_contact_sheet_status) -Expected "stale" `
    -Message "Fresh full visual gate warning should carry segmented contact sheet status."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.segmented_gate_aggregate_contact_sheet_bytes) -Expected 1822428 `
    -Message "Fresh full visual gate warning should carry segmented contact sheet bytes."
$fullCtestWarning = @($fixtureSummary.warnings) |
    Where-Object { [string]$_.id -eq "pdf_full_ctest.not_completed_in_current_window" } |
    Select-Object -First 1
Assert-Equal -Actual ([string]$fullCtestWarning.details.status) -Expected "timeout" `
    -Message "Full PDF CTest warning should carry the guarded attempt status."
Assert-Equal -Actual ([int]$fullCtestWarning.details.completed_test_count) -Expected 102 `
    -Message "Full PDF CTest warning should carry the guarded attempt completed count."

$blockedRoot = Join-Path $resolvedWorkingDir "fixture-blocked"
New-PassingFixture -Root $blockedRoot -ScriptPath $scriptPath
$blockedVisualPath = Join-Path $blockedRoot "output\pdf-visual-release-gate-current\report\summary.json"
$blockedVisual = Get-Content -Raw -Encoding UTF8 -LiteralPath $blockedVisualPath | ConvertFrom-Json
$blockedVisual.verdict = "fail"
$blockedVisual.cjk_copy_search_count = 42
Write-JsonFile -Path $blockedVisualPath -Value $blockedVisual
$blockedScript = Join-Path $blockedRoot "scripts\check_pdf_release_readiness.ps1"
$blockedSummaryPath = Join-Path $resolvedWorkingDir "blocked-readiness-summary.json"
$blockedResult = Invoke-PowerShellScript -ScriptPath $blockedScript -Arguments @(
    "-OutputJson", $blockedSummaryPath,
    "-Strict"
)
Assert-True -Condition ($blockedResult.ExitCode -ne 0) `
    -Message "Strict readiness check should fail blocked fixture."
$blockedSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $blockedSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$blockedSummary.status) -Expected "blocked" `
    -Message "Blocked fixture should report blocked status."
Assert-Equal -Actual ([string]$blockedSummary.verdict) -Expected "blocked" `
    -Message "Blocked fixture should report blocked verdict."
Assert-True -Condition ((@($blockedSummary.failed_checks | ForEach-Object { [string]$_ }) -contains "visual_gate_verdict_pass")) `
    -Message "Blocked fixture should identify the failed visual verdict check."
Assert-True -Condition ((@($blockedSummary.failed_checks | ForEach-Object { [string]$_ }) -contains "visual_gate_manifest_counts")) `
    -Message "Blocked fixture should identify the failed count check."

$scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
foreach ($expectedText in @(
        "featherdoc.pdf_release_readiness_check.v1",
        "persisted_pdf_release_evidence_only",
        "pdf_release_readiness_machine_gate_trace",
        "featherdoc.pdf_visual_full_gate_guarded_summary.v1",
        "pdf_visual_full_gate_guarded_summary_trace",
        "visual_full_gate_attempt_passed_stage_count",
        "visual_full_gate_attempt_visual_baseline_fresh_rendered_count",
        "visual_full_gate_attempt_aggregate_contact_sheet_status",
        "attempt_visual_baseline_fresh_rendered_count",
        "attempt_aggregate_contact_sheet_status",
        "featherdoc.pdf_visual_segmented_gate_summary.v1",
        "pdf_visual_segmented_gate_summary_trace",
        "visual_segmented_gate_status",
        "visual_segmented_gate_covered_baseline_count",
        "visual_segmented_gate_aggregate_contact_sheet_status",
        "segmented_gate_covered_baseline_count",
        "segmented_gate_aggregate_contact_sheet_bytes",
        "featherdoc.pdf_full_ctest_guarded_summary.v1",
        "pdf_full_ctest_guarded_summary_trace",
        "full_ctest_completed_test_count",
        "pdf_full_fresh_visual_gate.not_completed_in_current_window",
        "pdf_full_ctest.not_completed_in_current_window",
        "pass_with_warnings",
        "does not run CMake, CTest, rendering, Office, LibreOffice, browsers, or PDF generation"
    )) {
    Assert-True -Condition ($scriptText -match [regex]::Escape($expectedText)) `
        -Message "Readiness script should keep marker '$expectedText'."
}

Write-Host "PDF release readiness check contract passed."
