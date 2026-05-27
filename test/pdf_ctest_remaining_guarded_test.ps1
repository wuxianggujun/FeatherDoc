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

    ($Value | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $Path -Encoding UTF8
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

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_pdf_ctest_remaining_guarded.ps1"
$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count -gt 0) {
    throw "PDF remaining CTest guarded script has parse errors."
}

$buildDir = Join-Path $resolvedWorkingDir "build"
New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
Set-Content -LiteralPath (Join-Path $buildDir "CTestTestfile.cmake") -Encoding UTF8 -Value "# fixture"

$previousLog = Join-Path $resolvedWorkingDir "previous-full.out.log"
@'
Test project fixture
    Start 1: pdf_alpha
1/4 Test #1: pdf_alpha ................................................   Passed    0.01 sec
    Start 2: pdf_beta
2/4 Test #2: pdf_beta .................................................***Skipped   0.00 sec
    Start 3: pdf_gamma
'@ | Set-Content -LiteralPath $previousLog -Encoding UTF8

$fullSummaryPath = Join-Path $resolvedWorkingDir "full-summary.json"
Write-JsonFile -Path $fullSummaryPath -Value ([ordered]@{
    schema = "featherdoc.pdf_full_ctest_guarded_summary.v1"
    generated_at = "2026-05-27T06:01:56"
    status = "timeout"
    verdict = "not_complete"
    full_ctest_status = "not_complete"
    evidence_scope = "guarded_full_pdf_ctest_attempt"
    ctest_pattern = "pdf_"
    outer_guard_status = "timed_out"
    outer_guard_timed_out = $true
    selected_test_count = 4
    completed_test_count = 2
    passed_test_count = 1
    failed_test_count = 0
    skipped_test_count = 1
    not_run_test_count = 2
    stdout_log = $previousLog
    boundary = "guarded_full_ctest_attempt_does_not_replace_completed_full_ctest"
    marker = "pdf_full_ctest_guarded_summary_trace"
})

$fakePass = Join-Path $resolvedWorkingDir "fake-ctest-remaining-pass.ps1"
@'
if ($args -contains "-N") {
    Write-Host "Test project fixture"
    Write-Host "  Test #1: pdf_alpha"
    Write-Host "  Test #2: pdf_beta"
    Write-Host "  Test #3: pdf_gamma"
    Write-Host "  Test #4: pdf_delta"
    exit 0
}

Write-Host "Test project fixture"
Write-Host "    Start 3: pdf_gamma"
Write-Host "1/2 Test #3: pdf_gamma ................................................   Passed    0.01 sec"
Write-Host "    Start 4: pdf_delta"
Write-Host "2/2 Test #4: pdf_delta ................................................   Passed    0.02 sec"
Write-Host "100% tests passed, 0 tests failed out of 2"
Write-Host "Total Test time (real) = 0.03 sec"
exit 0
'@ | Set-Content -LiteralPath $fakePass -Encoding UTF8

$remainingSummaryPath = Join-Path $resolvedWorkingDir "remaining-summary.json"
$passResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $buildDir,
    "-FullCtestSummaryJson", $fullSummaryPath,
    "-OutputJson", $remainingSummaryPath,
    "-LogDir", (Join-Path $resolvedWorkingDir "remaining-logs"),
    "-CtestExecutable", $fakePass,
    "-OuterTimeoutSeconds", "5",
    "-CtestTimeoutSeconds", "60"
)
Assert-Equal -Actual $passResult.ExitCode -Expected 0 `
    -Message "Passing remaining-tail fake CTest should complete successfully. Output: $($passResult.Text)"
$passSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $remainingSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$passSummary.schema) -Expected "featherdoc.pdf_ctest_remaining_guarded_summary.v1" `
    -Message "Remaining-tail summary should expose a stable schema."
Assert-Equal -Actual ([string]$passSummary.status) -Expected "pass" `
    -Message "Passing remaining-tail run should report pass status."
Assert-Equal -Actual ([string]$passSummary.verdict) -Expected "pass" `
    -Message "Passing remaining-tail run should report pass verdict."
Assert-Equal -Actual ([string]$passSummary.remaining_ctest_status) -Expected "pass" `
    -Message "Passing remaining-tail run should report remaining CTest pass."
Assert-Equal -Actual ([string]$passSummary.evidence_scope) -Expected "remaining_pdf_ctest_tail_evidence" `
    -Message "Remaining-tail summary should preserve evidence scope."
Assert-Equal -Actual ([string]$passSummary.outer_guard_status) -Expected "completed" `
    -Message "Passing remaining-tail run should complete before outer guard."
Assert-Equal -Actual ([int]$passSummary.previous_selected_test_count) -Expected 4 `
    -Message "Remaining-tail summary should preserve previous selected count."
Assert-Equal -Actual ([int]$passSummary.previous_completed_test_count) -Expected 2 `
    -Message "Remaining-tail summary should preserve previous completed count."
Assert-Equal -Actual ([int]$passSummary.previous_not_run_test_count) -Expected 2 `
    -Message "Remaining-tail summary should preserve previous not-run count."
Assert-Equal -Actual ([int]$passSummary.selected_test_count) -Expected 2 `
    -Message "Remaining-tail summary should select only previously unfinished tests."
Assert-Equal -Actual ([int]$passSummary.completed_test_count) -Expected 2 `
    -Message "Remaining-tail summary should preserve completed tail count."
Assert-Equal -Actual ([int]$passSummary.failed_test_count) -Expected 0 `
    -Message "Remaining-tail summary should preserve failed tail count."
Assert-Equal -Actual ([int]$passSummary.not_run_test_count) -Expected 0 `
    -Message "Remaining-tail summary should preserve not-run tail count."
Assert-Equal -Actual ([int]$passSummary.combined_selected_test_count) -Expected 4 `
    -Message "Remaining-tail summary should preserve combined selected count."
Assert-Equal -Actual ([int]$passSummary.combined_completed_test_count) -Expected 4 `
    -Message "Remaining-tail summary should preserve combined completed count."
Assert-Equal -Actual ([int]$passSummary.combined_failed_test_count) -Expected 0 `
    -Message "Remaining-tail summary should preserve combined failed count."
Assert-Equal -Actual ([int]$passSummary.combined_not_run_test_count) -Expected 0 `
    -Message "Remaining-tail summary should preserve combined not-run count."
Assert-Equal -Actual ([double]$passSummary.combined_completion_percent) -Expected 100.0 `
    -Message "Remaining-tail summary should preserve combined completion percent."
Assert-Equal -Actual ([bool]$passSummary.combined_zero_failed_tests_observed) -Expected $true `
    -Message "Remaining-tail summary should preserve combined zero-failure observation."
Assert-Equal -Actual ([bool]$passSummary.combined_tail_covers_previous_remaining) -Expected $true `
    -Message "Remaining-tail summary should mark that tail covered the previous remainder."
Assert-True -Condition ((@($passSummary.selected_tests | ForEach-Object { [string]$_ }) -contains "pdf_gamma")) `
    -Message "Remaining-tail summary should include pdf_gamma."
Assert-True -Condition ((@($passSummary.selected_tests | ForEach-Object { [string]$_ }) -contains "pdf_delta")) `
    -Message "Remaining-tail summary should include pdf_delta."
Assert-Equal -Actual ([string]$passSummary.boundary) -Expected "remaining_ctest_tail_evidence_does_not_replace_single_full_ctest" `
    -Message "Remaining-tail summary should preserve single-run boundary."
Assert-Equal -Actual ([string]$passSummary.marker) -Expected "pdf_ctest_remaining_guarded_summary_trace" `
    -Message "Remaining-tail summary should keep a fixed trace marker."

$completePreviousLog = Join-Path $resolvedWorkingDir "previous-complete.out.log"
@'
Test project fixture
    Start 1: pdf_alpha
1/4 Test #1: pdf_alpha ................................................   Passed    0.01 sec
    Start 2: pdf_beta
2/4 Test #2: pdf_beta .................................................***Skipped   0.00 sec
    Start 3: pdf_gamma
3/4 Test #3: pdf_gamma ................................................   Passed    0.01 sec
    Start 4: pdf_delta
4/4 Test #4: pdf_delta ................................................   Passed    0.02 sec
'@ | Set-Content -LiteralPath $completePreviousLog -Encoding UTF8

$completeFullSummaryPath = Join-Path $resolvedWorkingDir "full-complete-summary.json"
Write-JsonFile -Path $completeFullSummaryPath -Value ([ordered]@{
    schema = "featherdoc.pdf_full_ctest_guarded_summary.v1"
    generated_at = "2026-05-27T06:05:00"
    status = "pass"
    verdict = "pass"
    full_ctest_status = "pass"
    evidence_scope = "guarded_full_pdf_ctest_attempt"
    ctest_pattern = "pdf_"
    outer_guard_status = "completed"
    outer_guard_timed_out = $false
    selected_test_count = 4
    completed_test_count = 4
    passed_test_count = 3
    failed_test_count = 0
    skipped_test_count = 1
    not_run_test_count = 0
    stdout_log = $completePreviousLog
    boundary = "guarded_full_ctest_attempt_does_not_replace_completed_full_ctest"
    marker = "pdf_full_ctest_guarded_summary_trace"
})

$noopSummaryPath = Join-Path $resolvedWorkingDir "remaining-noop-summary.json"
$noopResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $buildDir,
    "-FullCtestSummaryJson", $completeFullSummaryPath,
    "-OutputJson", $noopSummaryPath,
    "-LogDir", (Join-Path $resolvedWorkingDir "remaining-noop-logs"),
    "-CtestExecutable", $fakePass,
    "-OuterTimeoutSeconds", "5",
    "-CtestTimeoutSeconds", "60"
)
Assert-Equal -Actual $noopResult.ExitCode -Expected 0 `
    -Message "No-op remaining-tail run should pass when the previous attempt has no remainder. Output: $($noopResult.Text)"
$noopSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $noopSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$noopSummary.status) -Expected "pass" `
    -Message "No-op remaining-tail run should report pass status."
Assert-Equal -Actual ([int]$noopSummary.selected_test_count) -Expected 0 `
    -Message "No-op remaining-tail run should select no tests."
Assert-Equal -Actual ([int]$noopSummary.previous_not_run_test_count) -Expected 0 `
    -Message "No-op remaining-tail run should preserve zero previous not-run tests."
Assert-Equal -Actual ([bool]$noopSummary.combined_tail_covers_previous_remaining) -Expected $true `
    -Message "No-op remaining-tail run should preserve combined coverage."

$inconsistentFullSummaryPath = Join-Path $resolvedWorkingDir "full-inconsistent-summary.json"
Write-JsonFile -Path $inconsistentFullSummaryPath -Value ([ordered]@{
    schema = "featherdoc.pdf_full_ctest_guarded_summary.v1"
    generated_at = "2026-05-27T06:06:00"
    status = "timeout"
    verdict = "not_complete"
    full_ctest_status = "not_complete"
    evidence_scope = "guarded_full_pdf_ctest_attempt"
    ctest_pattern = "pdf_"
    outer_guard_status = "timed_out"
    outer_guard_timed_out = $true
    selected_test_count = 4
    completed_test_count = 3
    passed_test_count = 3
    failed_test_count = 0
    skipped_test_count = 0
    not_run_test_count = 1
    stdout_log = $completePreviousLog
    boundary = "guarded_full_ctest_attempt_does_not_replace_completed_full_ctest"
    marker = "pdf_full_ctest_guarded_summary_trace"
})

$inconsistentSummaryPath = Join-Path $resolvedWorkingDir "remaining-inconsistent-summary.json"
$inconsistentResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $buildDir,
    "-FullCtestSummaryJson", $inconsistentFullSummaryPath,
    "-OutputJson", $inconsistentSummaryPath,
    "-LogDir", (Join-Path $resolvedWorkingDir "remaining-inconsistent-logs"),
    "-CtestExecutable", $fakePass,
    "-OuterTimeoutSeconds", "5",
    "-CtestTimeoutSeconds", "60"
)
Assert-Equal -Actual $inconsistentResult.ExitCode -Expected 1 `
    -Message "Inconsistent no-selection tail evidence should fail when previous summary still reports not-run tests. Output: $($inconsistentResult.Text)"
$inconsistentSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $inconsistentSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$inconsistentSummary.status) -Expected "fail" `
    -Message "Inconsistent no-selection tail evidence should report fail status."
Assert-Equal -Actual ([int]$inconsistentSummary.selected_test_count) -Expected 0 `
    -Message "Inconsistent no-selection tail evidence should select no tests."
Assert-Equal -Actual ([int]$inconsistentSummary.combined_not_run_test_count) -Expected 1 `
    -Message "Inconsistent no-selection tail evidence should preserve combined not-run count."
Assert-Equal -Actual ([bool]$inconsistentSummary.combined_tail_covers_previous_remaining) -Expected $false `
    -Message "Inconsistent no-selection tail evidence should not claim tail coverage."

$fakeTimeout = Join-Path $resolvedWorkingDir "fake-ctest-remaining-timeout.ps1"
@'
if ($args -contains "-N") {
    Write-Host "Test project fixture"
    Write-Host "  Test #1: pdf_alpha"
    Write-Host "  Test #2: pdf_beta"
    Write-Host "  Test #3: pdf_gamma"
    Write-Host "  Test #4: pdf_delta"
    exit 0
}

Write-Host "Test project fixture"
Write-Host "    Start 3: pdf_gamma"
Write-Host "1/2 Test #3: pdf_gamma ................................................   Passed    0.01 sec"
Start-Sleep -Seconds 10
Write-Host "    Start 4: pdf_delta"
Write-Host "2/2 Test #4: pdf_delta ................................................   Passed    9.99 sec"
exit 0
'@ | Set-Content -LiteralPath $fakeTimeout -Encoding UTF8

$timeoutSummaryPath = Join-Path $resolvedWorkingDir "remaining-timeout-summary.json"
$timeoutResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $buildDir,
    "-FullCtestSummaryJson", $fullSummaryPath,
    "-OutputJson", $timeoutSummaryPath,
    "-LogDir", (Join-Path $resolvedWorkingDir "remaining-timeout-logs"),
    "-CtestExecutable", $fakeTimeout,
    "-OuterTimeoutSeconds", "1",
    "-CtestTimeoutSeconds", "60"
)
Assert-Equal -Actual $timeoutResult.ExitCode -Expected 124 `
    -Message "Timed-out remaining-tail fake CTest should return 124. Output: $($timeoutResult.Text)"
$timeoutSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $timeoutSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$timeoutSummary.status) -Expected "timeout" `
    -Message "Timed-out remaining-tail run should report timeout status."
Assert-Equal -Actual ([string]$timeoutSummary.verdict) -Expected "not_complete" `
    -Message "Timed-out remaining-tail run should report not_complete verdict."
Assert-Equal -Actual ([string]$timeoutSummary.outer_guard_status) -Expected "timed_out" `
    -Message "Timed-out remaining-tail run should preserve outer guard status."
Assert-Equal -Actual ([bool]$timeoutSummary.outer_guard_timed_out) -Expected $true `
    -Message "Timed-out remaining-tail run should preserve outer guard flag."
Assert-Equal -Actual ([bool]$timeoutSummary.combined_tail_covers_previous_remaining) -Expected $false `
    -Message "Timed-out remaining-tail run should not claim tail coverage."

$scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
foreach ($expectedText in @(
        "featherdoc.pdf_ctest_remaining_guarded_summary.v1",
        "remaining_pdf_ctest_tail_evidence",
        "combined_tail_covers_previous_remaining",
        "combined_zero_failed_tests_observed",
        "remaining_ctest_tail_evidence_does_not_replace_single_full_ctest",
        "pdf_ctest_remaining_guarded_summary_trace"
    )) {
    Assert-True -Condition ($scriptText -match [regex]::Escape($expectedText)) `
        -Message "Remaining-tail CTest script should keep marker '$expectedText'."
}

Write-Host "PDF remaining CTest guarded script contract passed."
