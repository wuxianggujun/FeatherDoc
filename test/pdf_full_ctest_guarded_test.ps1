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

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    throw "WorkingDir is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_pdf_full_ctest_guarded.ps1"
$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count -gt 0) {
    throw "PDF full CTest guarded script has parse errors."
}

$buildDir = Join-Path $resolvedWorkingDir "build"
New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
Set-Content -LiteralPath (Join-Path $buildDir "CTestTestfile.cmake") -Encoding UTF8 -Value "# fixture"

$fakePass = Join-Path $resolvedWorkingDir "fake-ctest-pass.ps1"
@'
Write-Host "Test project fixture"
Write-Host "    Start 1: pdf_alpha"
Write-Host "1/3 Test #1: pdf_alpha ................................................   Passed    0.01 sec"
Write-Host "    Start 2: pdf_beta"
Write-Host "2/3 Test #2: pdf_beta .................................................   Passed    0.02 sec"
Write-Host "    Start 3: pdf_env_skip"
Write-Host "3/3 Test #3: pdf_env_skip .............................................***Skipped   0.00 sec"
Write-Host "100% tests passed, 0 tests failed out of 3"
Write-Host "Total Test time (real) = 0.03 sec"
exit 0
'@ | Set-Content -LiteralPath $fakePass -Encoding UTF8

$passSummaryPath = Join-Path $resolvedWorkingDir "pass-summary.json"
$passResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $buildDir,
    "-OutputJson", $passSummaryPath,
    "-LogDir", (Join-Path $resolvedWorkingDir "pass-logs"),
    "-CtestExecutable", $fakePass,
    "-OuterTimeoutSeconds", "15",
    "-CtestTimeoutSeconds", "60"
)
Assert-Equal -Actual $passResult.ExitCode -Expected 0 `
    -Message "Passing fake CTest should complete successfully. Output: $($passResult.Text)"
$passSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $passSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$passSummary.schema) -Expected "featherdoc.pdf_full_ctest_guarded_summary.v1" `
    -Message "Full PDF CTest summary should expose a stable schema."
Assert-Equal -Actual ([string]$passSummary.status) -Expected "pass" `
    -Message "Passing fake CTest should report pass status."
Assert-Equal -Actual ([string]$passSummary.verdict) -Expected "pass" `
    -Message "Passing fake CTest should report pass verdict."
Assert-Equal -Actual ([string]$passSummary.full_ctest_status) -Expected "pass" `
    -Message "Passing fake CTest should report full CTest pass."
Assert-Equal -Actual ([string]$passSummary.outer_guard_status) -Expected "completed" `
    -Message "Passing fake CTest should complete before the outer guard."
Assert-Equal -Actual ([int]$passSummary.selected_test_count) -Expected 3 `
    -Message "Passing fake CTest should preserve selected test count."
Assert-Equal -Actual ([int]$passSummary.completed_test_count) -Expected 3 `
    -Message "Passing fake CTest should preserve completed test count."
Assert-Equal -Actual ([int]$passSummary.passed_test_count) -Expected 2 `
    -Message "Passing fake CTest should preserve pass count."
Assert-Equal -Actual ([int]$passSummary.skipped_test_count) -Expected 1 `
    -Message "Passing fake CTest should preserve skipped test count without failing the full run."
Assert-Equal -Actual ([int]$passSummary.not_run_test_count) -Expected 0 `
    -Message "Passing fake CTest should not report not-run tests."
Assert-Equal -Actual ([double]$passSummary.full_ctest_completion_percent) -Expected 100.0 `
    -Message "Passing fake CTest should report full completion percent."
Assert-Equal -Actual ([int]$passSummary.full_ctest_remaining_test_count) -Expected 0 `
    -Message "Passing fake CTest should report no remaining tests."
Assert-Equal -Actual ([bool]$passSummary.full_ctest_zero_failed_tests_observed) -Expected $true `
    -Message "Passing fake CTest should report zero observed failures."
Assert-Equal -Actual ([string]$passSummary.marker) -Expected "pdf_full_ctest_guarded_summary_trace" `
    -Message "Full PDF CTest summary should keep a fixed trace marker."

$fakeTimeout = Join-Path $resolvedWorkingDir "fake-ctest-timeout.ps1"
@'
Write-Host "Test project fixture"
Write-Host "    Start 1: pdf_alpha"
Write-Host "1/3 Test #1: pdf_alpha ................................................   Passed    0.01 sec"
Write-Host "    Start 2: pdf_slow"
Start-Sleep -Seconds 10
Write-Host "2/3 Test #2: pdf_slow .................................................   Passed    9.99 sec"
exit 0
'@ | Set-Content -LiteralPath $fakeTimeout -Encoding UTF8

$timeoutSummaryPath = Join-Path $resolvedWorkingDir "timeout-summary.json"
$timeoutResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $buildDir,
    "-OutputJson", $timeoutSummaryPath,
    "-LogDir", (Join-Path $resolvedWorkingDir "timeout-logs"),
    "-CtestExecutable", $fakeTimeout,
    "-OuterTimeoutSeconds", "1",
    "-CtestTimeoutSeconds", "60"
)
Assert-Equal -Actual $timeoutResult.ExitCode -Expected 124 `
    -Message "Timed-out fake CTest should return 124. Output: $($timeoutResult.Text)"
$timeoutSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $timeoutSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$timeoutSummary.status) -Expected "timeout" `
    -Message "Timed-out fake CTest should report timeout status."
Assert-Equal -Actual ([string]$timeoutSummary.verdict) -Expected "not_complete" `
    -Message "Timed-out fake CTest should report not_complete verdict."
Assert-Equal -Actual ([string]$timeoutSummary.full_ctest_status) -Expected "not_complete" `
    -Message "Timed-out fake CTest should not claim full CTest pass."
Assert-Equal -Actual ([string]$timeoutSummary.outer_guard_status) -Expected "timed_out" `
    -Message "Timed-out fake CTest should preserve outer guard status."
Assert-True -Condition ([bool]$timeoutSummary.outer_guard_timed_out) `
    -Message "Timed-out fake CTest should preserve outer guard flag."
Assert-Equal -Actual ([int]$timeoutSummary.outer_guard_timeout_seconds) -Expected 1 `
    -Message "Timed-out fake CTest should preserve outer guard timeout seconds."
Assert-Equal -Actual ([int]$timeoutSummary.selected_test_count) -Expected 3 `
    -Message "Timed-out fake CTest should preserve selected test count from progress lines."
Assert-Equal -Actual ([int]$timeoutSummary.completed_test_count) -Expected 1 `
    -Message "Timed-out fake CTest should preserve completed test count."
Assert-Equal -Actual ([int]$timeoutSummary.not_run_test_count) -Expected 2 `
    -Message "Timed-out fake CTest should preserve not-run test count."
Assert-Equal -Actual ([double]$timeoutSummary.full_ctest_completion_percent) -Expected 33.3 `
    -Message "Timed-out fake CTest should report partial completion percent."
Assert-Equal -Actual ([int]$timeoutSummary.full_ctest_remaining_test_count) -Expected 2 `
    -Message "Timed-out fake CTest should report remaining tests."
Assert-Equal -Actual ([bool]$timeoutSummary.full_ctest_zero_failed_tests_observed) -Expected $true `
    -Message "Timed-out fake CTest should report no observed failures when none were logged."

$scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
foreach ($expectedText in @(
        "ctest --test-dir <build> -R `"pdf_`" --output-on-failure --timeout 60",
        "featherdoc.pdf_full_ctest_guarded_summary.v1",
        "guarded_full_pdf_ctest_attempt",
        "outer_guard_status",
        "outer_guard_timed_out",
        "outer_guard_timeout_seconds",
        "not_run_test_count",
        "full_ctest_completion_percent",
        "full_ctest_remaining_test_count",
        "full_ctest_zero_failed_tests_observed",
        "guarded_full_ctest_attempt_does_not_replace_completed_full_ctest",
        "pdf_full_ctest_guarded_summary_trace"
    )) {
    Assert-True -Condition ($scriptText -match [regex]::Escape($expectedText)) `
        -Message "Full PDF CTest guarded script should keep marker '$expectedText'."
}

Write-Host "PDF full CTest guarded script contract passed."
