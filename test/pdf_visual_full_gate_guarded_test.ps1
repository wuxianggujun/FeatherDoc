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

function Write-TextFile {
    param([string]$Path, [string]$Text)

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $Text | Set-Content -LiteralPath $Path -Encoding UTF8
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

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_pdf_visual_full_gate_guarded.ps1"
$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count -gt 0) {
    throw "PDF visual full gate guarded script has parse errors."
}

$fakeVisualPass = Join-Path $resolvedWorkingDir "fake-visual-pass.ps1"
Write-TextFile -Path $fakeVisualPass -Text @'
param(
    [string]$BuildDir,
    [string]$OutputDir,
    [int]$Dpi,
    [int]$MinFreeMemoryMB,
    [string]$CtestExecutable
)

$reportDir = Join-Path $OutputDir "report"
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
"fake visual gate pass BuildDir=$BuildDir Dpi=$Dpi MinFreeMemoryMB=$MinFreeMemoryMB Ctest=$CtestExecutable"
exit 0
'@

$fakeVisualNativeNonZeroPass = Join-Path $resolvedWorkingDir "fake-visual-native-nonzero-pass.ps1"
Write-TextFile -Path $fakeVisualNativeNonZeroPass -Text @'
param(
    [string]$BuildDir,
    [string]$OutputDir,
    [int]$Dpi,
    [int]$MinFreeMemoryMB,
    [string]$CtestExecutable
)

$reportDir = Join-Path $OutputDir "report"
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
"fake visual gate pass with stale native exit code BuildDir=$BuildDir"
exit 1
'@

$fakeVisualTimeout = Join-Path $resolvedWorkingDir "fake-visual-timeout.ps1"
Write-TextFile -Path $fakeVisualTimeout -Text @'
param(
    [string]$BuildDir,
    [string]$OutputDir,
    [int]$Dpi,
    [int]$MinFreeMemoryMB,
    [string]$CtestExecutable
)

New-Item -ItemType Directory -Path (Join-Path $OutputDir "report") -Force | Out-Null
Start-Sleep -Seconds 10
exit 0
'@

$fakeAttemptSummary = Join-Path $resolvedWorkingDir "fake-attempt-summary.ps1"
Write-TextFile -Path $fakeAttemptSummary -Text @'
param(
    [string]$RepoRoot,
    [string]$ReportDir,
    [string]$OutputJson,
    [string]$AttemptStartedAfter,
    [switch]$OuterGuardTimedOut,
    [int]$OuterGuardTimeoutSeconds
)

$status = if ($OuterGuardTimedOut) { "partial" } else { "pass" }
$verdict = if ($OuterGuardTimedOut) { "not_complete" } else { "pass" }
$fullStatus = if ($OuterGuardTimedOut) { "not_complete" } else { "pass" }
$summary = [ordered]@{
    schema = "featherdoc.pdf_visual_gate_attempt_summary.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    verdict = $verdict
    full_visual_gate_status = $fullStatus
    evidence_scope = "bounded_attempt_auxiliary_only"
    report_dir = $ReportDir
    attempt_started_after = $AttemptStartedAfter
    outer_guard_status = if ($OuterGuardTimedOut) { "timed_out" } else { "completed" }
    outer_guard_timed_out = [bool]$OuterGuardTimedOut
    outer_guard_timeout_seconds = $OuterGuardTimeoutSeconds
    stage_count = 6
    passed_stage_count = if ($OuterGuardTimedOut) { 2 } else { 6 }
    failed_stage_count = 0
    incomplete_stage_count = if ($OuterGuardTimedOut) { 4 } else { 0 }
    visual_baseline_fresh_rendered_count = if ($OuterGuardTimedOut) { 3 } else { 44 }
    aggregate_contact_sheet_status = "pass"
    marker = "pdf_visual_gate_attempt_summary_trace"
}
New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($OutputJson)) -Force | Out-Null
($summary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $OutputJson -Encoding UTF8
exit 0
'@

$fakeAttemptSummaryPassOnTimeout = Join-Path $resolvedWorkingDir "fake-attempt-summary-pass-on-timeout.ps1"
Write-TextFile -Path $fakeAttemptSummaryPassOnTimeout -Text @'
param(
    [string]$RepoRoot,
    [string]$ReportDir,
    [string]$OutputJson,
    [string]$AttemptStartedAfter,
    [switch]$OuterGuardTimedOut,
    [int]$OuterGuardTimeoutSeconds
)

$summary = [ordered]@{
    schema = "featherdoc.pdf_visual_gate_attempt_summary.v1"
    generated_at = (Get-Date).ToString("s")
    status = "pass"
    verdict = "pass"
    full_visual_gate_status = "pass"
    evidence_scope = "bounded_attempt_auxiliary_only"
    report_dir = $ReportDir
    attempt_started_after = $AttemptStartedAfter
    outer_guard_status = if ($OuterGuardTimedOut) { "timed_out" } else { "completed" }
    outer_guard_timed_out = [bool]$OuterGuardTimedOut
    outer_guard_timeout_seconds = $OuterGuardTimeoutSeconds
    stage_count = 6
    passed_stage_count = 6
    failed_stage_count = 0
    incomplete_stage_count = 0
    visual_baseline_fresh_rendered_count = 44
    expected_visual_render_count = 44
    aggregate_contact_sheet_status = "pass"
    marker = "pdf_visual_gate_attempt_summary_trace"
}
New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($OutputJson)) -Force | Out-Null
($summary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $OutputJson -Encoding UTF8
exit 0
'@

$passBuildDir = Join-Path $resolvedWorkingDir "build-pass"
$passOutputDir = Join-Path $resolvedWorkingDir "output-pass"
$passSummaryPath = Join-Path $resolvedWorkingDir "pass-summary.json"
New-Item -ItemType Directory -Path $passBuildDir -Force | Out-Null
$passResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $passBuildDir,
    "-OutputDir", $passOutputDir,
    "-OutputJson", $passSummaryPath,
    "-VisualGateScript", $fakeVisualPass,
    "-AttemptSummaryScript", $fakeAttemptSummary,
    "-OuterTimeoutSeconds", "20",
    "-MinFreeMemoryMB", "1"
)
Assert-Equal -Actual $passResult.ExitCode -Expected 0 `
    -Message "Guarded visual full gate pass fixture should exit 0. Output: $($passResult.Text)"
$passSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $passSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$passSummary.schema) -Expected "featherdoc.pdf_visual_full_gate_guarded_summary.v1" `
    -Message "Guarded visual summary should expose a stable schema."
Assert-Equal -Actual ([string]$passSummary.status) -Expected "pass" `
    -Message "Pass fixture should report pass status."
Assert-Equal -Actual ([string]$passSummary.full_visual_gate_status) -Expected "pass" `
    -Message "Pass fixture should preserve full visual gate status."
Assert-Equal -Actual ([string]$passSummary.outer_guard_status) -Expected "completed" `
    -Message "Pass fixture should record completed outer guard."
Assert-True -Condition ([bool]$passSummary.attempt_summary_exists) `
    -Message "Pass fixture should write attempt summary evidence."

$nativeNonZeroBuildDir = Join-Path $resolvedWorkingDir "build-native-nonzero"
$nativeNonZeroOutputDir = Join-Path $resolvedWorkingDir "output-native-nonzero"
$nativeNonZeroSummaryPath = Join-Path $resolvedWorkingDir "native-nonzero-summary.json"
New-Item -ItemType Directory -Path $nativeNonZeroBuildDir -Force | Out-Null
$nativeNonZeroResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $nativeNonZeroBuildDir,
    "-OutputDir", $nativeNonZeroOutputDir,
    "-OutputJson", $nativeNonZeroSummaryPath,
    "-VisualGateScript", $fakeVisualNativeNonZeroPass,
    "-AttemptSummaryScript", $fakeAttemptSummary,
    "-OuterTimeoutSeconds", "20",
    "-MinFreeMemoryMB", "1"
)
Assert-Equal -Actual $nativeNonZeroResult.ExitCode -Expected 0 `
    -Message "Guarded visual full gate should trust pass attempt evidence even if the child process inherited a stale native exit code. Output: $($nativeNonZeroResult.Text)"
$nativeNonZeroSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $nativeNonZeroSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$nativeNonZeroSummary.status) -Expected "pass" `
    -Message "Native non-zero fixture should still report pass status from attempt evidence."
Assert-Equal -Actual ([int]$nativeNonZeroSummary.exit_code) -Expected 0 `
    -Message "Native non-zero fixture should expose effective exit_code 0."
Assert-Equal -Actual ([int]$nativeNonZeroSummary.process_exit_code) -Expected 0 `
    -Message "Native non-zero fixture should expose effective process_exit_code 0."
Assert-Equal -Actual ([int]$nativeNonZeroSummary.raw_process_exit_code) -Expected 1 `
    -Message "Native non-zero fixture should preserve the raw child process exit code."

$timeoutAfterPassBuildDir = Join-Path $resolvedWorkingDir "build-timeout-after-pass"
$timeoutAfterPassOutputDir = Join-Path $resolvedWorkingDir "output-timeout-after-pass"
$timeoutAfterPassSummaryPath = Join-Path $resolvedWorkingDir "timeout-after-pass-summary.json"
New-Item -ItemType Directory -Path $timeoutAfterPassBuildDir -Force | Out-Null
$timeoutAfterPassResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $timeoutAfterPassBuildDir,
    "-OutputDir", $timeoutAfterPassOutputDir,
    "-OutputJson", $timeoutAfterPassSummaryPath,
    "-VisualGateScript", $fakeVisualTimeout,
    "-AttemptSummaryScript", $fakeAttemptSummaryPassOnTimeout,
    "-OuterTimeoutSeconds", "1",
    "-MinFreeMemoryMB", "1"
)
Assert-Equal -Actual $timeoutAfterPassResult.ExitCode -Expected 0 `
    -Message "Guarded visual full gate should exit 0 when a pass summary is available before the outer timeout is observed. Output: $($timeoutAfterPassResult.Text)"
$timeoutAfterPassSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $timeoutAfterPassSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$timeoutAfterPassSummary.status) -Expected "pass" `
    -Message "Pass-before-timeout fixture should report pass status."
Assert-Equal -Actual ([string]$timeoutAfterPassSummary.verdict) -Expected "pass" `
    -Message "Pass-before-timeout fixture should report pass verdict."
Assert-Equal -Actual ([string]$timeoutAfterPassSummary.full_visual_gate_status) -Expected "pass" `
    -Message "Pass-before-timeout fixture should preserve pass full visual gate status."
Assert-Equal -Actual ([string]$timeoutAfterPassSummary.outer_guard_status) -Expected "timed_out_after_pass_summary" `
    -Message "Pass-before-timeout fixture should distinguish process-exit timeout from render incompletion."
Assert-Equal -Actual ([bool]$timeoutAfterPassSummary.outer_guard_timed_out) -Expected $true `
    -Message "Pass-before-timeout fixture should keep the outer timeout trace."
Assert-Equal -Actual ([bool]$timeoutAfterPassSummary.pass_summary_before_outer_timeout) -Expected $true `
    -Message "Pass-before-timeout fixture should expose the explicit pass-summary marker."
Assert-Equal -Actual ([int]$timeoutAfterPassSummary.exit_code) -Expected 0 `
    -Message "Pass-before-timeout fixture should expose effective exit_code 0."

$timeoutBuildDir = Join-Path $resolvedWorkingDir "build-timeout"
$timeoutOutputDir = Join-Path $resolvedWorkingDir "output-timeout"
$timeoutSummaryPath = Join-Path $resolvedWorkingDir "timeout-summary.json"
New-Item -ItemType Directory -Path $timeoutBuildDir -Force | Out-Null
$timeoutResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-BuildDir", $timeoutBuildDir,
    "-OutputDir", $timeoutOutputDir,
    "-OutputJson", $timeoutSummaryPath,
    "-VisualGateScript", $fakeVisualTimeout,
    "-AttemptSummaryScript", $fakeAttemptSummary,
    "-OuterTimeoutSeconds", "1",
    "-MinFreeMemoryMB", "1"
)
Assert-Equal -Actual $timeoutResult.ExitCode -Expected 124 `
    -Message "Guarded visual full gate timeout fixture should exit 124. Output: $($timeoutResult.Text)"
$timeoutSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $timeoutSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$timeoutSummary.status) -Expected "timeout" `
    -Message "Timeout fixture should report timeout status."
Assert-Equal -Actual ([string]$timeoutSummary.verdict) -Expected "not_complete" `
    -Message "Timeout fixture should report not_complete verdict."
Assert-Equal -Actual ([string]$timeoutSummary.full_visual_gate_status) -Expected "not_complete" `
    -Message "Timeout fixture should not claim a full visual gate pass."
Assert-Equal -Actual ([string]$timeoutSummary.outer_guard_status) -Expected "timed_out" `
    -Message "Timeout fixture should record timed_out outer guard."
Assert-Equal -Actual ([bool]$timeoutSummary.outer_guard_timed_out) -Expected $true `
    -Message "Timeout fixture should record outer guard timeout boolean."
Assert-Equal -Actual ([int]$timeoutSummary.attempt_passed_stage_count) -Expected 2 `
    -Message "Timeout fixture should preserve attempt stage evidence."
Assert-Equal -Actual ([string]$timeoutSummary.boundary) -Expected "guarded_full_visual_gate_attempt_does_not_replace_completed_full_visual_gate" `
    -Message "Timeout fixture should preserve the guarded attempt boundary."
Assert-Equal -Actual ([string]$timeoutSummary.marker) -Expected "pdf_visual_full_gate_guarded_summary_trace" `
    -Message "Timeout fixture should preserve the stable trace marker."

$scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
foreach ($expectedText in @(
        "featherdoc.pdf_visual_full_gate_guarded_summary.v1",
        "guarded_full_pdf_visual_gate_attempt",
        "full_visual_gate_status",
        "outer_guard_status",
        "outer_guard_timed_out",
        "outer_guard_timeout_seconds",
        "timed_out_after_pass_summary",
        "pass_summary_before_outer_timeout",
        "attempt_summary_json",
        "attempt_passed_stage_count",
        "guarded_full_visual_gate_attempt_does_not_replace_completed_full_visual_gate",
        "pdf_visual_full_gate_guarded_summary_trace"
    )) {
    Assert-True -Condition ($scriptText -match [regex]::Escape($expectedText)) `
        -Message "Guarded visual full gate script should keep marker '$expectedText'."
}

Write-Host "PDF visual full gate guarded contract passed."
