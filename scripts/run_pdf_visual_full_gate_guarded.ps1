<#
.SYNOPSIS
Runs the fresh non-FinalizeOnly PDF visual gate under an outer guard.

.DESCRIPTION
This script starts scripts/run_pdf_visual_release_gate.ps1 without -FinalizeOnly,
captures stdout/stderr, stops only descendant processes it launched when the
outer guard expires, and writes machine-readable attempt evidence for release
readiness. Timeout evidence is explicit evidence of an attempt; it does not
replace a completed fresh full visual gate pass.
#>
param(
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$OutputDir = "output/pdf-visual-release-gate-current",
    [string]$OutputJson = "",
    [string]$VisualGateScript = "scripts/run_pdf_visual_release_gate.ps1",
    [string]$AttemptSummaryScript = "scripts/write_pdf_visual_gate_attempt_summary.ps1",
    [string]$CtestExecutable = "ctest",
    [ValidateRange(1, 2147483647)]
    [int]$OuterTimeoutSeconds = 60,
    [ValidateRange(1, 2147483647)]
    [int]$Dpi = 144,
    [ValidateRange(0, 2147483647)]
    [int]$MinFreeMemoryMB = 2048,
    [switch]$SkipUnicodeBaseline,
    [switch]$SkipPreflight,
    [switch]$SkipMemoryGuard
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

    if ([System.IO.Path]::IsPathRooted($Path)) {
        $resolved = [System.IO.Path]::GetFullPath($Path)
    } else {
        $resolved = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $Path))
    }

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

function Get-PowerShellExecutable {
    $current = (Get-Process -Id $PID).Path
    if (-not [string]::IsNullOrWhiteSpace($current) -and (Test-Path -LiteralPath $current)) {
        return $current
    }

    return (Get-Command powershell.exe -ErrorAction Stop).Source
}

function Get-DescendantProcessIds {
    param([int]$ParentId)

    $children = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | Where-Object { $_.ParentProcessId -eq $ParentId })
    foreach ($child in $children) {
        [int]$child.ProcessId
        Get-DescendantProcessIds -ParentId ([int]$child.ProcessId)
    }
}

function Read-JsonFile {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

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

function Invoke-AttemptSummary {
    param(
        [string]$PowerShellPath,
        [string]$ScriptPath,
        [string]$RepoRoot,
        [string]$ReportDir,
        [string]$AttemptSummaryJson,
        [datetime]$AttemptStartedAt,
        [bool]$OuterGuardTimedOut,
        [int]$OuterGuardTimeoutSeconds
    )

    if (-not (Test-Path -LiteralPath $ScriptPath)) {
        return [pscustomobject]@{
            exit_code = 1
            error = "Attempt summary script was not found."
        }
    }

    $arguments = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $ScriptPath,
        "-RepoRoot",
        $RepoRoot,
        "-ReportDir",
        $ReportDir,
        "-OutputJson",
        $AttemptSummaryJson,
        "-AttemptStartedAfter",
        $AttemptStartedAt.ToString("s"),
        "-OuterGuardTimeoutSeconds",
        [string]$OuterGuardTimeoutSeconds
    )
    if ($OuterGuardTimedOut) {
        $arguments += "-OuterGuardTimedOut"
    }

    $output = @(& $PowerShellPath @arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    return [pscustomobject]@{
        exit_code = $exitCode
        output = @($output | ForEach-Object { $_.ToString() })
        error = ""
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
$resolvedReportDir = Join-Path $resolvedOutputDir "report"
$resolvedVisualGateScript = Resolve-RepoPath -RepoRoot $repoRoot -Path $VisualGateScript
$resolvedAttemptSummaryScript = Resolve-RepoPath -RepoRoot $repoRoot -Path $AttemptSummaryScript -AllowMissing
$resolvedOutputJson = if ([string]::IsNullOrWhiteSpace($OutputJson)) {
    Join-Path $resolvedReportDir "full-visual-gate-guarded-summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputJson -AllowMissing
}
$attemptSummaryJson = Join-Path $resolvedReportDir "attempt-summary.json"

New-Item -ItemType Directory -Path $resolvedReportDir -Force | Out-Null
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$stdoutLog = Join-Path $resolvedReportDir "full-visual-gate-guard-$stamp.out.log"
$stderrLog = Join-Path $resolvedReportDir "full-visual-gate-guard-$stamp.err.log"
$powerShellPath = Get-PowerShellExecutable

$arguments = @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    $resolvedVisualGateScript,
    "-BuildDir",
    $resolvedBuildDir,
    "-OutputDir",
    $resolvedOutputDir,
    "-Dpi",
    [string]$Dpi,
    "-MinFreeMemoryMB",
    [string]$MinFreeMemoryMB,
    "-CtestExecutable",
    $CtestExecutable
)
if ($SkipUnicodeBaseline) { $arguments += "-SkipUnicodeBaseline" }
if ($SkipPreflight) { $arguments += "-SkipPreflight" }
if ($SkipMemoryGuard) { $arguments += "-SkipMemoryGuard" }

$startedAt = Get-Date
$process = Start-Process `
    -FilePath $powerShellPath `
    -ArgumentList $arguments `
    -WorkingDirectory $repoRoot `
    -RedirectStandardOutput $stdoutLog `
    -RedirectStandardError $stderrLog `
    -WindowStyle Hidden `
    -PassThru

$completed = $process.WaitForExit($OuterTimeoutSeconds * 1000)
$cleanedProcessIds = @()
if (-not $completed) {
    $descendants = @(Get-DescendantProcessIds -ParentId $process.Id)
    $cleanedProcessIds = @($descendants + $process.Id | Sort-Object -Descending -Unique)
    foreach ($processId in $cleanedProcessIds) {
        Stop-Process -Id $processId -Force -ErrorAction SilentlyContinue
    }
} else {
    $process.Refresh()
}
$finishedAt = Get-Date

$processExitCode = if (-not $completed) {
    124
} elseif ($null -ne $process.ExitCode) {
    [int]$process.ExitCode
} else {
    1
}

$attemptSummaryRun = Invoke-AttemptSummary `
    -PowerShellPath $powerShellPath `
    -ScriptPath $resolvedAttemptSummaryScript `
    -RepoRoot $repoRoot `
    -ReportDir $resolvedReportDir `
    -AttemptSummaryJson $attemptSummaryJson `
    -AttemptStartedAt $startedAt `
    -OuterGuardTimedOut (-not $completed) `
    -OuterGuardTimeoutSeconds $OuterTimeoutSeconds
$attemptSummary = Read-JsonFile -Path $attemptSummaryJson
$attemptStatus = if ($null -eq $attemptSummary) { "missing" } else { [string](Get-OptionalPropertyValue -Object $attemptSummary -Name "status") }
$attemptVerdict = if ($null -eq $attemptSummary) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $attemptSummary -Name "verdict") }
$attemptFullVisualGateStatus = if ($null -eq $attemptSummary) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $attemptSummary -Name "full_visual_gate_status") }
$attemptEvidenceScope = if ($null -eq $attemptSummary) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $attemptSummary -Name "evidence_scope") }

$attemptPassed = ($attemptVerdict -eq "pass" -and $attemptFullVisualGateStatus -eq "pass")
$exitCode = if (-not $completed) {
    124
} elseif ($attemptPassed) {
    0
} else {
    $processExitCode
}

$status = if (-not $completed) {
    "timeout"
} elseif ($attemptPassed) {
    "pass"
} else {
    "fail"
}
$verdict = if ($status -eq "pass") { "pass" } elseif ($status -eq "timeout") { "not_complete" } else { "fail" }
$fullVisualGateStatus = if ($status -eq "pass") { "pass" } elseif ($status -eq "fail") { "fail" } else { "not_complete" }

$summary = [ordered]@{
    schema = "featherdoc.pdf_visual_full_gate_guarded_summary.v1"
    generated_at = $finishedAt.ToString("s")
    started_at = $startedAt.ToString("s")
    finished_at = $finishedAt.ToString("s")
    status = $status
    verdict = $verdict
    full_visual_gate_status = $fullVisualGateStatus
    evidence_scope = "guarded_full_pdf_visual_gate_attempt"
    repo_root = $repoRoot
    build_dir = $resolvedBuildDir
    build_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedBuildDir
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    report_dir = $resolvedReportDir
    report_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedReportDir
    visual_gate_script = $resolvedVisualGateScript
    visual_gate_script_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedVisualGateScript
    attempt_summary_script = $resolvedAttemptSummaryScript
    attempt_summary_script_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedAttemptSummaryScript
    outer_guard_status = if ($completed) { "completed" } else { "timed_out" }
    outer_guard_timed_out = -not $completed
    outer_guard_timeout_seconds = $OuterTimeoutSeconds
    exit_code = $exitCode
    process_exit_code = $processExitCode
    stdout_log = $stdoutLog
    stdout_log_display = Get-DisplayPath -RepoRoot $repoRoot -Path $stdoutLog
    stderr_log = $stderrLog
    stderr_log_display = Get-DisplayPath -RepoRoot $repoRoot -Path $stderrLog
    attempt_summary_json = $attemptSummaryJson
    attempt_summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $attemptSummaryJson
    attempt_summary_exists = (Test-Path -LiteralPath $attemptSummaryJson -PathType Leaf)
    attempt_summary_exit_code = $attemptSummaryRun.exit_code
    attempt_status = $attemptStatus
    attempt_verdict = $attemptVerdict
    attempt_full_visual_gate_status = $attemptFullVisualGateStatus
    attempt_evidence_scope = $attemptEvidenceScope
    attempt_stage_count = if ($null -eq $attemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $attemptSummary -Name "stage_count" }
    attempt_passed_stage_count = if ($null -eq $attemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $attemptSummary -Name "passed_stage_count" }
    attempt_failed_stage_count = if ($null -eq $attemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $attemptSummary -Name "failed_stage_count" }
    attempt_incomplete_stage_count = if ($null -eq $attemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $attemptSummary -Name "incomplete_stage_count" }
    attempt_visual_baseline_fresh_rendered_count = if ($null -eq $attemptSummary) { 0 } else { Get-OptionalPropertyValue -Object $attemptSummary -Name "visual_baseline_fresh_rendered_count" }
    attempt_aggregate_contact_sheet_status = if ($null -eq $attemptSummary) { "not_available" } else { [string](Get-OptionalPropertyValue -Object $attemptSummary -Name "aggregate_contact_sheet_status") }
    cleaned_process_ids = @($cleanedProcessIds)
    boundary = "guarded_full_visual_gate_attempt_does_not_replace_completed_full_visual_gate"
    marker = "pdf_visual_full_gate_guarded_summary_trace"
}

$summaryDir = Split-Path -Parent $resolvedOutputJson
if (-not [string]::IsNullOrWhiteSpace($summaryDir)) {
    New-Item -ItemType Directory -Path $summaryDir -Force | Out-Null
}
($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8
$summary | ConvertTo-Json -Depth 10

if ($status -eq "pass") {
    exit 0
}
if ($status -eq "timeout") {
    exit 124
}
exit 1
