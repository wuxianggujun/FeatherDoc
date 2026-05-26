<#
.SYNOPSIS
Runs the full PDF CTest selector under an outer guard and writes attempt evidence.

.DESCRIPTION
This script is intentionally limited to a single CTest command:
ctest --test-dir <build> -R "pdf_" --output-on-failure --timeout 60.
It records pass/fail/timeout evidence so release readiness can explain whether
the full PDF CTest suite actually completed in the current resource window.
#>
param(
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$OutputJson = "output/pdf-ctest-current/summary.json",
    [string]$LogDir = "output/pdf-ctest-current",
    [string]$CtestExecutable = "ctest",
    [string]$Pattern = "pdf_",
    [ValidateRange(1, 2147483647)]
    [int]$OuterTimeoutSeconds = 60,
    [ValidateRange(1, 2147483647)]
    [int]$CtestTimeoutSeconds = 60
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

function Resolve-CtestExecutable {
    param([string]$Executable)

    if ([string]::IsNullOrWhiteSpace($Executable)) {
        throw "CTest executable is required."
    }

    if ([System.IO.Path]::IsPathRooted($Executable)) {
        if (Test-Path -LiteralPath $Executable) {
            return (Resolve-Path -LiteralPath $Executable).Path
        }

        throw "CTest executable was not found: $Executable"
    }

    $resolved = Get-Command $Executable -ErrorAction SilentlyContinue
    if ($resolved) {
        return $resolved.Source
    }

    throw "CTest executable was not found. Pass -CtestExecutable or ensure ctest is in PATH."
}

function Get-PowerShellExecutable {
    $current = (Get-Process -Id $PID).Path
    if (-not [string]::IsNullOrWhiteSpace($current) -and (Test-Path -LiteralPath $current)) {
        return $current
    }

    return (Get-Command powershell.exe -ErrorAction Stop).Source
}

function Get-LaunchSpec {
    param(
        [string]$Executable,
        [string[]]$CtestArguments
    )

    if ([System.IO.Path]::GetExtension($Executable) -ieq ".ps1") {
        return [pscustomobject]@{
            Executable = Get-PowerShellExecutable
            Arguments = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $Executable) + $CtestArguments
        }
    }

    return [pscustomobject]@{
        Executable = $Executable
        Arguments = $CtestArguments
    }
}

function Get-DescendantProcessIds {
    param([int]$ParentId)

    $children = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | Where-Object { $_.ParentProcessId -eq $ParentId })
    foreach ($child in $children) {
        [int]$child.ProcessId
        Get-DescendantProcessIds -ParentId ([int]$child.ProcessId)
    }
}

function Get-LogLines {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return @()
    }

    return @(Get-Content -Encoding UTF8 -LiteralPath $Path | ForEach-Object { $_.ToString() })
}

function Get-CtestLineStats {
    param([string[]]$Lines)

    $selectedCount = 0
    $completedCount = 0
    $passedCount = 0
    $failedCount = 0
    $skippedCount = 0
    $totalTimeSeconds = ""

    foreach ($line in $Lines) {
        if ($line -match '^\s*(\d+)\s*/\s*(\d+)\s+Test\s+#\d+:') {
            $completedCount = [Math]::Max($completedCount, [int]$Matches[1])
            $selectedCount = [Math]::Max($selectedCount, [int]$Matches[2])
        }
        if ($line -match '^\s*\d+\s*/\s*\d+\s+Test\s+#\d+:.+\bPassed\b') {
            $passedCount++
        }
        if ($line -match '^\s*\d+\s*/\s*\d+\s+Test\s+#\d+:.+(\*\*\*Failed\b|\bFailed\b)') {
            $failedCount++
        }
        if ($line -match '^\s*\d+\s*/\s*\d+\s+Test\s+#\d+:.+\*\*\*Skipped\b') {
            $skippedCount++
        }
        if ($line -match '(\d+)% tests passed,\s*(\d+) tests failed out of\s*(\d+)') {
            $failedCount = [int]$Matches[2]
            $selectedCount = [int]$Matches[3]
        }
        if ($line -match 'Total Test time \(real\)\s*=\s*([0-9.]+)\s*sec') {
            $totalTimeSeconds = $Matches[1]
        }
    }

    if ($selectedCount -gt 0 -and $completedCount -eq 0) {
        $completedCount = [Math]::Min($selectedCount, $passedCount + $failedCount + $skippedCount)
    }

    return [pscustomobject]@{
        selected_test_count = $selectedCount
        completed_test_count = $completedCount
        passed_test_count = $passedCount
        failed_test_count = $failedCount
        skipped_test_count = $skippedCount
        not_run_test_count = [Math]::Max(0, $selectedCount - $completedCount)
        total_time_seconds = $totalTimeSeconds
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $BuildDir
$resolvedOutputJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputJson -AllowMissing
$resolvedLogDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $LogDir -AllowMissing
$resolvedCtestExecutable = Resolve-CtestExecutable -Executable $CtestExecutable

New-Item -ItemType Directory -Path $resolvedLogDir -Force | Out-Null
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$stdoutLog = Join-Path $resolvedLogDir "full-ctest-guard-$stamp.out.log"
$stderrLog = Join-Path $resolvedLogDir "full-ctest-guard-$stamp.err.log"

$ctestArguments = @(
    "--test-dir", $resolvedBuildDir,
    "-R", $Pattern,
    "--output-on-failure",
    "--timeout", [string]$CtestTimeoutSeconds
)
$launch = Get-LaunchSpec -Executable $resolvedCtestExecutable -CtestArguments $ctestArguments

$startedAt = Get-Date
$process = Start-Process `
    -FilePath $launch.Executable `
    -ArgumentList $launch.Arguments `
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

$stdoutLines = Get-LogLines -Path $stdoutLog
$stderrLines = Get-LogLines -Path $stderrLog
$stats = Get-CtestLineStats -Lines ($stdoutLines + $stderrLines)
$exitCode = if (-not $completed) {
    124
} elseif ($null -ne $process.ExitCode) {
    [int]$process.ExitCode
} elseif ([int]$stats.selected_test_count -gt 0 -and [int]$stats.completed_test_count -ge [int]$stats.selected_test_count -and [int]$stats.failed_test_count -eq 0) {
    0
} else {
    1
}

$status = if (-not $completed) {
    "timeout"
} elseif ($exitCode -eq 0 -and [int]$stats.failed_test_count -eq 0 -and [int]$stats.skipped_test_count -eq 0) {
    "pass"
} else {
    "fail"
}
$verdict = if ($status -eq "pass") { "pass" } elseif ($status -eq "timeout") { "not_complete" } else { "fail" }
$fullCtestStatus = if ($status -eq "pass") { "pass" } elseif ($status -eq "timeout") { "not_complete" } else { "fail" }

$summary = [ordered]@{
    schema = "featherdoc.pdf_full_ctest_guarded_summary.v1"
    generated_at = $finishedAt.ToString("s")
    started_at = $startedAt.ToString("s")
    finished_at = $finishedAt.ToString("s")
    status = $status
    verdict = $verdict
    full_ctest_status = $fullCtestStatus
    evidence_scope = "guarded_full_pdf_ctest_attempt"
    repo_root = $repoRoot
    build_dir = $resolvedBuildDir
    ctest_executable = $resolvedCtestExecutable
    ctest_pattern = $Pattern
    ctest_timeout_seconds = $CtestTimeoutSeconds
    outer_guard_status = if ($completed) { "completed" } else { "timed_out" }
    outer_guard_timed_out = -not $completed
    outer_guard_timeout_seconds = $OuterTimeoutSeconds
    exit_code = $exitCode
    selected_test_count = [int]$stats.selected_test_count
    completed_test_count = [int]$stats.completed_test_count
    passed_test_count = [int]$stats.passed_test_count
    failed_test_count = [int]$stats.failed_test_count
    skipped_test_count = [int]$stats.skipped_test_count
    not_run_test_count = [int]$stats.not_run_test_count
    total_time_seconds = [string]$stats.total_time_seconds
    stdout_log = $stdoutLog
    stdout_log_display = Get-DisplayPath -RepoRoot $repoRoot -Path $stdoutLog
    stderr_log = $stderrLog
    stderr_log_display = Get-DisplayPath -RepoRoot $repoRoot -Path $stderrLog
    cleaned_process_ids = @($cleanedProcessIds)
    boundary = "guarded_full_ctest_attempt_does_not_replace_completed_full_ctest"
    marker = "pdf_full_ctest_guarded_summary_trace"
}

$outputDir = Split-Path -Parent $resolvedOutputJson
if (-not [string]::IsNullOrWhiteSpace($outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}
($summary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8
$summary | ConvertTo-Json -Depth 8

if ($status -eq "pass") {
    exit 0
}
if ($status -eq "timeout") {
    exit 124
}
exit 1
