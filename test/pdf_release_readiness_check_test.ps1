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

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Assert-FileHasNoBom {
    param([string]$Path, [string]$Message)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        throw $Message
    }
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
    Copy-Item -LiteralPath (Join-Path (Split-Path -Parent $ScriptPath) "check_pdf_release_readiness_helpers.ps1") -Destination (Join-Path $Root "scripts\check_pdf_release_readiness_helpers.ps1")

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
        finalize_only = $false
        skip_preflight = $true
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
        pass_summary_before_outer_timeout = $false
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

    Write-JsonFile -Path (Join-Path $Root "output\pdf-visual-release-gate-current\report\attempt-summary.json") -Value ([ordered]@{
        schema = "featherdoc.pdf_visual_gate_attempt_summary.v1"
        generated_at = "2026-05-27T04:56:32"
        status = "partial"
        verdict = "not_complete"
        full_visual_gate_status = "not_complete"
        evidence_scope = "bounded_attempt_auxiliary_only"
        stage_count = 6
        passed_stage_count = 4
        incomplete_stage_count = 2
        visual_baseline_render_status = "partial"
        visual_baseline_fresh_rendered_count = 37
        expected_visual_render_count = 44
        visual_baseline_fresh_missing_sample_count = 7
        visual_baseline_resume_needed = $true
        visual_baseline_resume_slice_offset = 37
        visual_baseline_resume_slice_limit = 7
        visual_baseline_resume_slice_command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir <build-dir> -OutputDir .\output\pdf-visual-release-gate-current -VisualBaselineSliceOnly -VisualBaselineOffset 37 -VisualBaselineLimit 7 -SkipPreflight"
        aggregate_contact_sheet_status = "stale"
        aggregate_contact_sheet_bytes = 1822428
        marker = "pdf_visual_gate_attempt_summary_trace"
    })

    Write-JsonFile -Path (Join-Path $Root "output\pdf-visual-release-gate-current\report\segmented-summary.json") -Value ([ordered]@{
        schema = "featherdoc.pdf_visual_segmented_gate_summary.v1"
        generated_at = "2026-05-27T04:36:21"
        status = "pass"
        verdict = "pass"
        full_visual_gate_status = "not_complete"
        evidence_scope = "segmented_visual_gate_auxiliary_only"
        boundary = "segmented_summary_does_not_replace_full_visual_gate_verdict"
        summary_json_display = ".\output\pdf-visual-release-gate-current\report\segmented-summary.json"
        slice_summary_count = 6
        slice_pass_count = 6
        slice_failed_count = 0
        covered_baseline_count = 44
        expected_visual_render_count = 44
        attempt_stage_count = 6
        attempt_passed_stage_count = 6
        visual_baseline_render_status = "pass"
        aggregate_contact_sheet_status = "pass"
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
run_pdf_ctest_remaining_guarded.ps1
featherdoc.pdf_release_readiness_check.v1
featherdoc.pdf_visual_full_gate_guarded_summary.v1
featherdoc.pdf_visual_segmented_gate_summary.v1
featherdoc.pdf_full_ctest_guarded_summary.v1
featherdoc.pdf_ctest_remaining_guarded_summary.v1
pdf_visual_gate_evidence
visual_gate_pass_summary_before_outer_timeout
visual_gate_segmented_full_coverage_evidence
pdf_bounded_ctest_evidence
pdf_visual_gate_release_owner_acceptance_trace
release_entry_pdf_readiness_checklist_trace
pdf_release_readiness_machine_gate_trace
pdf_visual_full_gate_guarded_summary_trace
pdf_visual_segmented_gate_summary_trace
pdf_full_ctest_guarded_summary_trace
pdf_ctest_remaining_guarded_summary_trace
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
$scriptHelperPath = Join-Path $resolvedRepoRoot "scripts\check_pdf_release_readiness_helpers.ps1"
foreach ($pathToParse in @($scriptPath, $scriptHelperPath)) {
    $tokens = $null
    $errors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($pathToParse, [ref]$tokens, [ref]$errors) | Out-Null
    if ($errors.Count -gt 0) {
        throw "PDF release readiness check script has parse errors."
    }
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
Assert-FileHasNoBom -Path $fixtureSummaryPath `
    -Message "Fixture readiness summary JSON should be UTF-8 without BOM."
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
Assert-Equal -Actual ([bool]$fixtureSummary.visual_gate_finalize_only) -Expected $false `
    -Message "Passing fixture should exercise non-finalize visual release evidence."
Assert-Equal -Actual ([bool]$fixtureSummary.visual_gate_segmented_full_coverage_evidence) -Expected $true `
    -Message "Passing fixture should accept strict segmented full-coverage visual evidence."
Assert-Equal -Actual ([bool]$fixtureSummary.visual_gate_pass_summary_before_outer_timeout) -Expected $false `
    -Message "Passing fixture should not claim pass-summary-before-timeout evidence for a timed-out guarded attempt."
Assert-Equal -Actual ([bool]$fixtureSummary.visual_gate_release_evidence_accepted) -Expected $true `
    -Message "Passing fixture should accept segmented full-coverage visual evidence for release readiness."
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
Assert-Equal -Actual ([bool]$fixtureSummary.visual_full_gate_pass_summary_before_outer_timeout) -Expected $false `
    -Message "Passing fixture should preserve missing pass-summary-before-timeout state."
Assert-Equal -Actual ([int]$fixtureSummary.visual_full_gate_attempt_passed_stage_count) -Expected 2 `
    -Message "Passing fixture should preserve guarded full visual gate attempt stage count."
Assert-Equal -Actual ([int]$fixtureSummary.visual_full_gate_attempt_visual_baseline_fresh_rendered_count) -Expected 3 `
    -Message "Passing fixture should preserve guarded full visual gate fresh render count."
Assert-Equal -Actual ([string]$fixtureSummary.visual_full_gate_attempt_aggregate_contact_sheet_status) -Expected "stale" `
    -Message "Passing fixture should preserve guarded full visual gate contact sheet status."
Assert-Equal -Actual ([bool]$fixtureSummary.visual_full_gate_attempt_summary_exists) -Expected $true `
    -Message "Passing fixture should detect post-timeout visual gate attempt summary evidence."
Assert-Equal -Actual ([string]$fixtureSummary.visual_full_gate_attempt_summary_status) -Expected "partial" `
    -Message "Passing fixture should preserve post-timeout visual gate attempt summary status."
Assert-Equal -Actual ([string]$fixtureSummary.visual_full_gate_attempt_summary_verdict) -Expected "not_complete" `
    -Message "Passing fixture should preserve post-timeout visual gate attempt summary verdict."
Assert-Equal -Actual ([int]$fixtureSummary.visual_full_gate_attempt_summary_passed_stage_count) -Expected 4 `
    -Message "Passing fixture should preserve post-timeout visual gate attempt passed stage count."
Assert-Equal -Actual ([int]$fixtureSummary.visual_full_gate_attempt_summary_visual_baseline_fresh_rendered_count) -Expected 37 `
    -Message "Passing fixture should preserve post-timeout visual gate fresh render count."
Assert-Equal -Actual ([int]$fixtureSummary.visual_full_gate_attempt_summary_visual_baseline_fresh_missing_sample_count) -Expected 7 `
    -Message "Passing fixture should preserve post-timeout visual gate fresh missing sample count."
Assert-Equal -Actual ([bool]$fixtureSummary.visual_full_gate_attempt_summary_visual_baseline_resume_needed) -Expected $true `
    -Message "Passing fixture should preserve post-timeout visual gate resume-needed state."
Assert-Equal -Actual ([int]$fixtureSummary.visual_full_gate_attempt_summary_visual_baseline_resume_slice_offset) -Expected 37 `
    -Message "Passing fixture should preserve post-timeout visual gate resume offset."
Assert-Equal -Actual ([int]$fixtureSummary.visual_full_gate_attempt_summary_visual_baseline_resume_slice_limit) -Expected 7 `
    -Message "Passing fixture should preserve post-timeout visual gate resume limit."
Assert-ContainsText -Text ([string]$fixtureSummary.visual_full_gate_attempt_summary_visual_baseline_resume_slice_command_template) -ExpectedText "VisualBaselineSliceOnly" `
    -Message "Passing fixture should preserve post-timeout visual gate resume command template."
Assert-Equal -Actual ([string]$fixtureSummary.visual_full_gate_attempt_summary_aggregate_contact_sheet_status) -Expected "stale" `
    -Message "Passing fixture should preserve post-timeout visual gate contact sheet status."
Assert-Equal -Actual ([bool]$fixtureSummary.visual_segmented_gate_summary_exists) -Expected $true `
    -Message "Passing fixture should detect segmented visual gate evidence."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_status) -Expected "pass" `
    -Message "Passing fixture should preserve segmented visual gate status."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_verdict) -Expected "pass" `
    -Message "Passing fixture should preserve segmented visual gate verdict."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_full_visual_gate_status) -Expected "not_complete" `
    -Message "Passing fixture should preserve segmented visual gate full status boundary."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_evidence_scope) -Expected "segmented_visual_gate_auxiliary_only" `
    -Message "Passing fixture should preserve segmented visual gate auxiliary scope."
Assert-Equal -Actual ([int]$fixtureSummary.visual_segmented_gate_slice_summary_count) -Expected 6 `
    -Message "Passing fixture should preserve segmented visual gate slice count."
Assert-Equal -Actual ([int]$fixtureSummary.visual_segmented_gate_slice_pass_count) -Expected 6 `
    -Message "Passing fixture should preserve segmented visual gate slice pass count."
Assert-Equal -Actual ([int]$fixtureSummary.visual_segmented_gate_covered_baseline_count) -Expected 44 `
    -Message "Passing fixture should preserve segmented visual gate covered baseline count."
Assert-Equal -Actual ([int]$fixtureSummary.visual_segmented_gate_expected_visual_render_count) -Expected 44 `
    -Message "Passing fixture should preserve segmented visual gate expected visual render count."
Assert-Equal -Actual ([string]$fixtureSummary.visual_segmented_gate_aggregate_contact_sheet_status) -Expected "pass" `
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
Assert-Equal -Actual ([double]$fixtureSummary.full_ctest_completion_percent) -Expected 73.4 `
    -Message "Passing fixture should expose guarded full PDF CTest completion percent."
Assert-Equal -Actual ([int]$fixtureSummary.full_ctest_remaining_test_count) -Expected 37 `
    -Message "Passing fixture should expose guarded full PDF CTest remaining test count."
Assert-Equal -Actual ([bool]$fixtureSummary.full_ctest_zero_failed_tests_observed) -Expected $true `
    -Message "Passing fixture should expose guarded full PDF CTest zero-failure observation."
Assert-Equal -Actual ([bool]$fixtureSummary.full_ctest_single_run_completed) -Expected $false `
    -Message "Passing fixture should not claim a single full PDF CTest run completed."
Assert-Equal -Actual ([bool]$fixtureSummary.full_ctest_combined_evidence_completed) -Expected $false `
    -Message "Passing fixture should not claim combined PDF CTest evidence without remaining-tail evidence."
Assert-Equal -Actual ([bool]$fixtureSummary.full_ctest_remaining_summary_exists) -Expected $false `
    -Message "Passing fixture should report missing remaining-tail PDF CTest evidence."
Assert-True -Condition ((@($fixtureSummary.warnings | ForEach-Object { [string]$_.id }) -contains "pdf_full_fresh_visual_gate.not_completed_in_current_window")) `
    -Message "Readiness summary should keep fresh full visual gate debt visible."
Assert-True -Condition ((@($fixtureSummary.warnings | ForEach-Object { [string]$_.id }) -contains "pdf_full_ctest.not_completed_in_current_window")) `
    -Message "Readiness summary should keep full PDF CTest debt visible."
$visualFullGateWarning = @($fixtureSummary.warnings) |
    Where-Object { [string]$_.id -eq "pdf_full_fresh_visual_gate.not_completed_in_current_window" } |
    Select-Object -First 1
Assert-Equal -Actual ([string]$visualFullGateWarning.details.status) -Expected "timeout" `
    -Message "Fresh full visual gate warning should carry the guarded attempt status."
Assert-Equal -Actual ([bool]$visualFullGateWarning.details.pass_summary_before_outer_timeout) -Expected $false `
    -Message "Fresh full visual gate warning should carry pass-summary-before-timeout state."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.attempt_passed_stage_count) -Expected 2 `
    -Message "Fresh full visual gate warning should carry guarded attempt stage counts."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.attempt_visual_baseline_fresh_rendered_count) -Expected 3 `
    -Message "Fresh full visual gate warning should carry guarded attempt fresh render counts."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.attempt_aggregate_contact_sheet_status) -Expected "stale" `
    -Message "Fresh full visual gate warning should carry guarded attempt contact sheet status."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.attempt_summary_aggregate_contact_sheet_status) -Expected "stale" `
    -Message "Fresh full visual gate warning should carry post-timeout attempt summary contact sheet status."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.attempt_summary_visual_baseline_fresh_rendered_count) -Expected 37 `
    -Message "Fresh full visual gate warning should carry post-timeout attempt summary fresh render count."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.attempt_summary_visual_baseline_fresh_missing_sample_count) -Expected 7 `
    -Message "Fresh full visual gate warning should carry post-timeout attempt summary missing sample count."
Assert-Equal -Actual ([bool]$visualFullGateWarning.details.attempt_summary_visual_baseline_resume_needed) -Expected $true `
    -Message "Fresh full visual gate warning should expose whether a visual baseline resume is still needed."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.attempt_summary_visual_baseline_resume_slice_offset) -Expected 37 `
    -Message "Fresh full visual gate warning should expose the next resume offset."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.attempt_summary_visual_baseline_resume_slice_limit) -Expected 7 `
    -Message "Fresh full visual gate warning should expose the remaining resume limit."
Assert-ContainsText -Text ([string]$visualFullGateWarning.details.attempt_summary_visual_baseline_resume_slice_command_template) -ExpectedText "run_pdf_visual_release_gate.ps1" `
    -Message "Fresh full visual gate warning should expose a visual baseline resume command template."
Assert-ContainsText -Text ([string]$visualFullGateWarning.details.attempt_summary_visual_baseline_resume_slice_command_template) -ExpectedText "-VisualBaselineSliceOnly" `
    -Message "Fresh full visual gate warning should point reviewers at the slice-only resume mode."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.segmented_gate_status) -Expected "pass" `
    -Message "Fresh full visual gate warning should carry segmented visual gate status."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.segmented_gate_verdict) -Expected "pass" `
    -Message "Fresh full visual gate warning should carry segmented visual gate verdict."
Assert-Equal -Actual ([bool]$visualFullGateWarning.details.segmented_full_coverage_evidence) -Expected $true `
    -Message "Fresh full visual gate warning should carry segmented full-coverage acceptance state."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.segmented_gate_evidence_scope) -Expected "segmented_visual_gate_auxiliary_only" `
    -Message "Fresh full visual gate warning should carry segmented visual gate auxiliary scope."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.segmented_gate_covered_baseline_count) -Expected 44 `
    -Message "Fresh full visual gate warning should carry segmented covered baseline count."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.segmented_gate_expected_visual_render_count) -Expected 44 `
    -Message "Fresh full visual gate warning should carry segmented expected render count."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.segmented_gate_aggregate_contact_sheet_status) -Expected "pass" `
    -Message "Fresh full visual gate warning should carry segmented contact sheet status."
Assert-Equal -Actual ([int]$visualFullGateWarning.details.segmented_gate_aggregate_contact_sheet_bytes) -Expected 1822428 `
    -Message "Fresh full visual gate warning should carry segmented contact sheet bytes."
Assert-Equal -Actual ([bool]$visualFullGateWarning.details.release_owner_acceptance_required) -Expected $true `
    -Message "Fresh full visual gate warning should require explicit release-owner acceptance."
Assert-Equal -Actual ([string]$visualFullGateWarning.details.release_owner_acceptance_boundary) -Expected "acceptance_does_not_replace_fresh_single_run_full_visual_gate" `
    -Message "Fresh full visual gate warning should preserve the acceptance boundary."
Assert-ContainsText -Text ([string]$visualFullGateWarning.details.release_owner_acceptance_policy) -ExpectedText "segmented_full_coverage" `
    -Message "Fresh full visual gate warning should describe the segmented-evidence acceptance policy."
Assert-ContainsText -Text ([string]$visualFullGateWarning.details.release_owner_acceptance_command_template) -ExpectedText "check_pdf_release_readiness.ps1" `
    -Message "Fresh full visual gate warning should include a release-owner acceptance command template."
Assert-ContainsText -Text ((@($visualFullGateWarning.details.release_owner_acceptance_required_evidence) | ForEach-Object { [string]$_ }) -join "`n") -ExpectedText "visual_gate_segmented_full_coverage_evidence" `
    -Message "Fresh full visual gate warning should enumerate required acceptance evidence."
$fullCtestWarning = @($fixtureSummary.warnings) |
    Where-Object { [string]$_.id -eq "pdf_full_ctest.not_completed_in_current_window" } |
    Select-Object -First 1
Assert-Equal -Actual ([string]$fullCtestWarning.details.status) -Expected "timeout" `
    -Message "Full PDF CTest warning should carry the guarded attempt status."
Assert-Equal -Actual ([int]$fullCtestWarning.details.completed_test_count) -Expected 102 `
    -Message "Full PDF CTest warning should carry the guarded attempt completed count."
Assert-Equal -Actual ([double]$fullCtestWarning.details.completion_percent) -Expected 73.4 `
    -Message "Full PDF CTest warning should carry the guarded attempt completion percent."
Assert-Equal -Actual ([int]$fullCtestWarning.details.remaining_test_count) -Expected 37 `
    -Message "Full PDF CTest warning should carry the guarded attempt remaining count."
Assert-Equal -Actual ([bool]$fullCtestWarning.details.zero_failed_tests_observed) -Expected $true `
    -Message "Full PDF CTest warning should carry the zero-failure observation."
Assert-Equal -Actual ([bool]$fullCtestWarning.details.remaining_summary_exists) -Expected $false `
    -Message "Full PDF CTest warning should expose missing remaining-tail evidence."

$tailRoot = Join-Path $resolvedWorkingDir "fixture-tail-combined"
New-PassingFixture -Root $tailRoot -ScriptPath $scriptPath
Write-JsonFile -Path (Join-Path $tailRoot "output\pdf-ctest-current\remaining-summary.json") -Value ([ordered]@{
    schema = "featherdoc.pdf_ctest_remaining_guarded_summary.v1"
    generated_at = "2026-05-27T06:10:00"
    status = "pass"
    verdict = "pass"
    remaining_ctest_status = "pass"
    evidence_scope = "remaining_pdf_ctest_tail_evidence"
    outer_guard_status = "completed"
    outer_guard_timed_out = $false
    selected_test_count = 37
    completed_test_count = 37
    failed_test_count = 0
    skipped_test_count = 0
    not_run_test_count = 0
    completion_percent = 100.0
    combined_selected_test_count = 139
    combined_completed_test_count = 139
    combined_failed_test_count = 0
    combined_skipped_test_count = 6
    combined_not_run_test_count = 0
    combined_completion_percent = 100.0
    combined_zero_failed_tests_observed = $true
    combined_tail_covers_previous_remaining = $true
    boundary = "remaining_ctest_tail_evidence_does_not_replace_single_full_ctest"
    marker = "pdf_ctest_remaining_guarded_summary_trace"
})
$tailScript = Join-Path $tailRoot "scripts\check_pdf_release_readiness.ps1"
$tailSummaryPath = Join-Path $resolvedWorkingDir "tail-readiness-summary.json"
$tailResult = Invoke-PowerShellScript -ScriptPath $tailScript -Arguments @(
    "-OutputJson", $tailSummaryPath
)
Assert-Equal -Actual $tailResult.ExitCode -Expected 0 `
    -Message "Tail-combined PDF release readiness check should pass. Output: $($tailResult.Text)"
Assert-FileHasNoBom -Path $tailSummaryPath `
    -Message "Tail-combined readiness summary JSON should be UTF-8 without BOM."
$tailSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tailSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$tailSummary.status) -Expected "pass" `
    -Message "Tail-combined fixture should report pass status."
Assert-Equal -Actual ([string]$tailSummary.verdict) -Expected "pass_with_warnings" `
    -Message "Tail-combined fixture should retain visual warning verdict boundary."
Assert-Equal -Actual ([bool]$tailSummary.full_ctest_single_run_completed) -Expected $false `
    -Message "Tail-combined fixture should not claim single full PDF CTest completion."
Assert-Equal -Actual ([bool]$tailSummary.full_ctest_combined_evidence_completed) -Expected $true `
    -Message "Tail-combined fixture should report combined PDF CTest evidence completion."
Assert-Equal -Actual ([bool]$tailSummary.full_ctest_remaining_summary_exists) -Expected $true `
    -Message "Tail-combined fixture should detect remaining-tail PDF CTest evidence."
Assert-Equal -Actual ([string]$tailSummary.full_ctest_remaining_status) -Expected "pass" `
    -Message "Tail-combined fixture should preserve remaining-tail status."
Assert-Equal -Actual ([int]$tailSummary.full_ctest_remaining_selected_test_count) -Expected 37 `
    -Message "Tail-combined fixture should preserve remaining-tail selected count."
Assert-Equal -Actual ([int]$tailSummary.full_ctest_combined_completed_test_count) -Expected 139 `
    -Message "Tail-combined fixture should preserve combined completed count."
Assert-Equal -Actual ([int]$tailSummary.full_ctest_combined_not_run_test_count) -Expected 0 `
    -Message "Tail-combined fixture should preserve zero combined not-run count."
Assert-Equal -Actual ([bool]$tailSummary.full_ctest_remaining_tail_covers_previous_remaining) -Expected $true `
    -Message "Tail-combined fixture should mark tail coverage."
Assert-True -Condition ((@($tailSummary.warnings | ForEach-Object { [string]$_.id }) -contains "pdf_full_fresh_visual_gate.not_completed_in_current_window")) `
    -Message "Tail-combined readiness summary should keep visual full-gate warning visible."
Assert-True -Condition (-not ((@($tailSummary.warnings | ForEach-Object { [string]$_.id }) -contains "pdf_full_ctest.not_completed_in_current_window"))) `
    -Message "Tail-combined readiness summary should clear full PDF CTest warning."

$completedRoot = Join-Path $resolvedWorkingDir "fixture-completed"
New-PassingFixture -Root $completedRoot -ScriptPath $scriptPath
$completedVisualFullPath = Join-Path $completedRoot "output\pdf-visual-release-gate-current\report\full-visual-gate-guarded-summary.json"
$completedVisualFull = Get-Content -Raw -Encoding UTF8 -LiteralPath $completedVisualFullPath | ConvertFrom-Json
$completedVisualFull.status = "pass"
$completedVisualFull.verdict = "pass"
$completedVisualFull.full_visual_gate_status = "pass"
$completedVisualFull.outer_guard_status = "completed"
$completedVisualFull.outer_guard_timed_out = $false
Write-JsonFile -Path $completedVisualFullPath -Value $completedVisualFull
$completedVisualSummaryPath = Join-Path $completedRoot "output\pdf-visual-release-gate-current\report\summary.json"
$completedVisualSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $completedVisualSummaryPath | ConvertFrom-Json
$completedVisualSummary.finalize_only = $false
$completedVisualSummary.skip_preflight = $true
Write-JsonFile -Path $completedVisualSummaryPath -Value $completedVisualSummary
$completedCtestPath = Join-Path $completedRoot "output\pdf-ctest-current\summary.json"
$completedCtest = Get-Content -Raw -Encoding UTF8 -LiteralPath $completedCtestPath | ConvertFrom-Json
$completedCtest.status = "pass"
$completedCtest.verdict = "pass"
$completedCtest.full_ctest_status = "pass"
$completedCtest.outer_guard_status = "completed"
$completedCtest.outer_guard_timed_out = $false
$completedCtest.completed_test_count = 139
$completedCtest.passed_test_count = 132
$completedCtest.skipped_test_count = 7
$completedCtest.not_run_test_count = 0
Write-JsonFile -Path $completedCtestPath -Value $completedCtest
$completedScript = Join-Path $completedRoot "scripts\check_pdf_release_readiness.ps1"
$completedSummaryPath = Join-Path $resolvedWorkingDir "completed-readiness-summary.json"
$completedResult = Invoke-PowerShellScript -ScriptPath $completedScript -Arguments @(
    "-OutputJson", $completedSummaryPath
)
Assert-Equal -Actual $completedResult.ExitCode -Expected 0 `
    -Message "Completed fixture PDF release readiness check should pass. Output: $($completedResult.Text)"
Assert-FileHasNoBom -Path $completedSummaryPath `
    -Message "Completed readiness summary JSON should be UTF-8 without BOM."
$completedSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $completedSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$completedSummary.status) -Expected "pass" `
    -Message "Completed fixture should report pass status."
Assert-Equal -Actual ([string]$completedSummary.verdict) -Expected "pass" `
    -Message "Completed fixture with no warnings should report pass verdict."
Assert-Equal -Actual ([bool]$completedSummary.release_ready) -Expected $true `
    -Message "Completed fixture should stay release-ready."
Assert-Equal -Actual ([int]$completedSummary.warning_count) -Expected 0 `
    -Message "Completed fixture should not carry warning debt."
Assert-Equal -Actual ([int]$completedSummary.failed_check_count) -Expected 0 `
    -Message "Completed fixture should not carry failed checks."
Assert-Equal -Actual ([string]$completedSummary.visual_full_gate_status) -Expected "pass" `
    -Message "Completed fixture should preserve completed visual full-gate status."
Assert-Equal -Actual ([bool]$completedSummary.visual_gate_finalize_only) -Expected $false `
    -Message "Completed fixture should not require finalize-only evidence when a fresh full guarded visual gate passed."
Assert-Equal -Actual ([bool]$completedSummary.visual_gate_fresh_full_guarded_evidence) -Expected $true `
    -Message "Completed fixture should expose fresh full guarded visual evidence."
Assert-Equal -Actual ([bool]$completedSummary.visual_gate_pass_summary_before_outer_timeout) -Expected $false `
    -Message "Completed fixture should not require pass-summary-before-timeout evidence."
Assert-Equal -Actual ([bool]$completedSummary.visual_gate_release_evidence_accepted) -Expected $true `
    -Message "Completed fixture should accept fresh full guarded visual evidence for release readiness."
Assert-Equal -Actual ([string]$completedSummary.full_ctest_status) -Expected "pass" `
    -Message "Completed fixture should preserve completed full CTest status."
Assert-Equal -Actual ([bool]$completedSummary.full_ctest_single_run_completed) -Expected $true `
    -Message "Completed fixture should report single full PDF CTest completion."
Assert-Equal -Actual ([bool]$completedSummary.full_ctest_combined_evidence_completed) -Expected $true `
    -Message "Completed fixture should report CTest completion without tail evidence."
Assert-Equal -Actual ([double]$completedSummary.full_ctest_completion_percent) -Expected 100.0 `
    -Message "Completed fixture should expose full CTest completion percent."
Assert-Equal -Actual ([int]$completedSummary.full_ctest_remaining_test_count) -Expected 0 `
    -Message "Completed fixture should expose zero remaining full CTest tests."

$passSummaryTimeoutRoot = Join-Path $resolvedWorkingDir "fixture-pass-summary-timeout"
New-PassingFixture -Root $passSummaryTimeoutRoot -ScriptPath $scriptPath
$passSummaryTimeoutVisualFullPath = Join-Path $passSummaryTimeoutRoot "output\pdf-visual-release-gate-current\report\full-visual-gate-guarded-summary.json"
$passSummaryTimeoutVisualFull = Get-Content -Raw -Encoding UTF8 -LiteralPath $passSummaryTimeoutVisualFullPath | ConvertFrom-Json
$passSummaryTimeoutVisualFull.status = "pass"
$passSummaryTimeoutVisualFull.verdict = "pass"
$passSummaryTimeoutVisualFull.full_visual_gate_status = "pass"
$passSummaryTimeoutVisualFull.outer_guard_status = "timed_out_after_pass_summary"
$passSummaryTimeoutVisualFull.outer_guard_timed_out = $true
$passSummaryTimeoutVisualFull.pass_summary_before_outer_timeout = $true
Write-JsonFile -Path $passSummaryTimeoutVisualFullPath -Value $passSummaryTimeoutVisualFull
$passSummaryTimeoutCtestPath = Join-Path $passSummaryTimeoutRoot "output\pdf-ctest-current\summary.json"
$passSummaryTimeoutCtest = Get-Content -Raw -Encoding UTF8 -LiteralPath $passSummaryTimeoutCtestPath | ConvertFrom-Json
$passSummaryTimeoutCtest.status = "pass"
$passSummaryTimeoutCtest.verdict = "pass"
$passSummaryTimeoutCtest.full_ctest_status = "pass"
$passSummaryTimeoutCtest.outer_guard_status = "completed"
$passSummaryTimeoutCtest.outer_guard_timed_out = $false
$passSummaryTimeoutCtest.completed_test_count = 139
$passSummaryTimeoutCtest.passed_test_count = 132
$passSummaryTimeoutCtest.skipped_test_count = 7
$passSummaryTimeoutCtest.not_run_test_count = 0
Write-JsonFile -Path $passSummaryTimeoutCtestPath -Value $passSummaryTimeoutCtest
$passSummaryTimeoutScript = Join-Path $passSummaryTimeoutRoot "scripts\check_pdf_release_readiness.ps1"
$passSummaryTimeoutSummaryPath = Join-Path $resolvedWorkingDir "pass-summary-timeout-readiness-summary.json"
$passSummaryTimeoutResult = Invoke-PowerShellScript -ScriptPath $passSummaryTimeoutScript -Arguments @(
    "-OutputJson", $passSummaryTimeoutSummaryPath
)
Assert-Equal -Actual $passSummaryTimeoutResult.ExitCode -Expected 0 `
    -Message "Pass-summary-before-timeout readiness check should pass. Output: $($passSummaryTimeoutResult.Text)"
Assert-FileHasNoBom -Path $passSummaryTimeoutSummaryPath `
    -Message "Pass-summary-before-timeout readiness summary JSON should be UTF-8 without BOM."
$passSummaryTimeoutSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $passSummaryTimeoutSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$passSummaryTimeoutSummary.verdict) -Expected "pass" `
    -Message "Pass-summary-before-timeout fixture with full CTest pass should not retain visual warning debt."
Assert-Equal -Actual ([int]$passSummaryTimeoutSummary.warning_count) -Expected 0 `
    -Message "Pass-summary-before-timeout fixture should not carry warning debt."
Assert-Equal -Actual ([bool]$passSummaryTimeoutSummary.visual_gate_fresh_full_guarded_evidence) -Expected $true `
    -Message "Pass-summary-before-timeout fixture should count as fresh guarded full visual evidence."
Assert-Equal -Actual ([bool]$passSummaryTimeoutSummary.visual_gate_pass_summary_before_outer_timeout) -Expected $true `
    -Message "Pass-summary-before-timeout fixture should expose the readiness-level marker."
Assert-Equal -Actual ([string]$passSummaryTimeoutSummary.visual_full_gate_outer_guard_status) -Expected "timed_out_after_pass_summary" `
    -Message "Pass-summary-before-timeout fixture should preserve the guarded outer status."
Assert-Equal -Actual ([bool]$passSummaryTimeoutSummary.visual_full_gate_pass_summary_before_outer_timeout) -Expected $true `
    -Message "Pass-summary-before-timeout fixture should expose the guarded summary marker."

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
Assert-FileHasNoBom -Path $blockedSummaryPath `
    -Message "Blocked readiness summary JSON should be UTF-8 without BOM."
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
        "visual_full_gate_attempt_summary_aggregate_contact_sheet_status",
        "visual_full_gate_attempt_summary_visual_baseline_fresh_missing_sample_count",
        "visual_full_gate_attempt_summary_visual_baseline_resume_slice_command_template",
        "visual_gate_pass_summary_before_outer_timeout",
        "visual_full_gate_pass_summary_before_outer_timeout",
        "pass_summary_before_outer_timeout",
        "timed_out_after_pass_summary",
        "attempt_summary_aggregate_contact_sheet_status",
        "attempt_visual_baseline_fresh_rendered_count",
        "attempt_aggregate_contact_sheet_status",
        "attempt_summary_visual_baseline_fresh_rendered_count",
        "attempt_summary_visual_baseline_fresh_missing_sample_count",
        "attempt_summary_visual_baseline_resume_slice_offset",
        "attempt_summary_visual_baseline_resume_slice_limit",
        "attempt_summary_visual_baseline_resume_slice_command_template",
        "visual_gate_release_evidence",
        "visual_gate_fresh_full_guarded_evidence",
        "visual_gate_segmented_full_coverage_evidence",
        "release_owner_acceptance_required",
        "release_owner_acceptance_policy",
        "release_owner_acceptance_boundary",
        "release_owner_acceptance_command_template",
        "visual_gate_release_evidence_accepted",
        "segmented_full_coverage_evidence",
        "featherdoc.pdf_visual_segmented_gate_summary.v1",
        "pdf_visual_segmented_gate_summary_trace",
        "visual_segmented_gate_status",
        "visual_segmented_gate_covered_baseline_count",
        "visual_segmented_gate_aggregate_contact_sheet_status",
        "segmented_gate_covered_baseline_count",
        "segmented_gate_aggregate_contact_sheet_bytes",
        "featherdoc.pdf_full_ctest_guarded_summary.v1",
        "pdf_full_ctest_guarded_summary_trace",
        "run_pdf_ctest_remaining_guarded.ps1",
        "featherdoc.pdf_ctest_remaining_guarded_summary.v1",
        "pdf_ctest_remaining_guarded_summary_trace",
        "full_ctest_completed_test_count",
        "full_ctest_completion_percent",
        "full_ctest_remaining_test_count",
        "full_ctest_zero_failed_tests_observed",
        "full_ctest_combined_evidence_completed",
        "full_ctest_remaining_tail_covers_previous_remaining",
        "remaining_summary_exists",
        "zero_failed_tests_observed",
        "pdf_full_fresh_visual_gate.not_completed_in_current_window",
        "pdf_full_ctest.not_completed_in_current_window",
        "pdf_visual_gate_release_owner_acceptance_trace",
        "pass_with_warnings",
        "does not run CMake, CTest, rendering, Office, LibreOffice, browsers, or PDF generation"
    )) {
    Assert-True -Condition ($scriptText -match [regex]::Escape($expectedText)) `
        -Message "Readiness script should keep marker '$expectedText'."
}

Write-Host "PDF release readiness check contract passed."
