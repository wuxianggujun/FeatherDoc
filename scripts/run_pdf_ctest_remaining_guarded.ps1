<#
.SYNOPSIS
Runs the unfinished tail of a guarded PDF CTest attempt and writes evidence.

.DESCRIPTION
This script reads the previous guarded full PDF CTest summary, derives completed
tests from its stdout log, lists the current ctest -R "pdf_" selector, and runs
only the tests not completed by that guarded attempt. The resulting summary is
explicitly tail evidence and does not claim to replace a single completed full
ctest -R "pdf_" run.
#>
param(
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$FullCtestSummaryJson = "output/pdf-ctest-current/summary.json",
    [string]$OutputJson = "output/pdf-ctest-current/remaining-summary.json",
    [string]$LogDir = "output/pdf-ctest-current",
    [string]$CtestExecutable = "ctest",
    [string]$Pattern = "pdf_",
    [string[]]$TestName = @(),
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

function Convert-ToIntOrZero {
    param($Value)

    if ($null -eq $Value -or [string]::IsNullOrWhiteSpace([string]$Value)) {
        return 0
    }

    try {
        return [int]$Value
    } catch {
        return 0
    }
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
        [string[]]$Arguments
    )

    if ([System.IO.Path]::GetExtension($Executable) -ieq ".ps1") {
        return [pscustomobject]@{
            Executable = Get-PowerShellExecutable
            Arguments = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $Executable) + $Arguments
        }
    }

    return [pscustomobject]@{
        Executable = $Executable
        Arguments = $Arguments
    }
}

function Invoke-CapturedCommand {
    param(
        [string]$Executable,
        [string[]]$Arguments
    )

    $launch = Get-LaunchSpec -Executable $Executable -Arguments $Arguments
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = @(& $launch.Executable @($launch.Arguments) 2>&1)
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    return [pscustomobject]@{
        exit_code = if ($null -eq $exitCode) { 0 } else { [int]$exitCode }
        lines = @($output | ForEach-Object { $_.ToString() })
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

function Get-CtestCompletedNames {
    param([string[]]$Lines)

    $names = New-Object 'System.Collections.Generic.List[string]'
    foreach ($line in $Lines) {
        if ($line -match '^\s*\d+\s*/\s*\d+\s+Test\s+#\d+:\s+(.+?)\s+\.{3,}') {
            $names.Add($Matches[1].Trim()) | Out-Null
        }
    }

    return @($names | Select-Object -Unique)
}

function Get-CtestLineStats {
    param([string[]]$Lines)

    $selectedCount = 0
    $completedCount = 0
    $passedCount = 0
    $failedCount = 0
    $skippedCount = 0
    $totalTimeSeconds = ""
    $completedNames = New-Object 'System.Collections.Generic.List[string]'

    foreach ($line in $Lines) {
        if ($line -match '^\s*(\d+)\s*/\s*(\d+)\s+Test\s+#\d+:\s+(.+?)\s+\.{3,}') {
            $completedCount = [Math]::Max($completedCount, [int]$Matches[1])
            $selectedCount = [Math]::Max($selectedCount, [int]$Matches[2])
            $completedNames.Add($Matches[3].Trim()) | Out-Null
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
        completed_tests = @($completedNames | Select-Object -Unique)
    }
}

function Get-ExactRegex {
    param([string[]]$Names)

    $nameList = @($Names | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($nameList.Count -eq 0) {
        return "^$"
    }

    return "^($(($nameList | ForEach-Object { [regex]::Escape($_) }) -join '|'))$"
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $BuildDir
$resolvedFullCtestSummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $FullCtestSummaryJson
$resolvedOutputJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputJson -AllowMissing
$resolvedLogDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $LogDir -AllowMissing
$resolvedCtestExecutable = Resolve-CtestExecutable -Executable $CtestExecutable

New-Item -ItemType Directory -Path $resolvedLogDir -Force | Out-Null
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$stdoutLog = Join-Path $resolvedLogDir "remaining-ctest-guard-$stamp.out.log"
$stderrLog = Join-Path $resolvedLogDir "remaining-ctest-guard-$stamp.err.log"

$fullSummary = Read-JsonFile -Path $resolvedFullCtestSummaryJson
$previousStdoutLog = [string](Get-OptionalPropertyValue -Object $fullSummary -Name "stdout_log")
if ([string]::IsNullOrWhiteSpace($previousStdoutLog)) {
    throw "Previous full CTest summary does not contain stdout_log."
}
$resolvedPreviousStdoutLog = Resolve-RepoPath -RepoRoot $repoRoot -Path $previousStdoutLog
$previousCompletedNames = @(Get-CtestCompletedNames -Lines (Get-LogLines -Path $resolvedPreviousStdoutLog))

$listResult = Invoke-CapturedCommand -Executable $resolvedCtestExecutable -Arguments @("--test-dir", $resolvedBuildDir, "-N", "-R", $Pattern)
if ($listResult.exit_code -ne 0) {
    throw "Failed to list PDF CTest tests for remaining-tail evidence."
}

$listedTests = @(
    $listResult.lines | ForEach-Object {
        if ($_ -match '^\s*Test\s+#\d+:\s+(.+?)\s*$') {
            $Matches[1].Trim()
        }
    }
)
if ($listedTests.Count -eq 0) {
    throw "No tests matched ctest pattern '$Pattern'."
}

$explicitTests = @($TestName | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
$selectedTests = if ($explicitTests.Count -gt 0) {
    @($explicitTests)
} else {
    @($listedTests | Where-Object { $previousCompletedNames -notcontains $_ })
}
$selectedTests = @($selectedTests | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })

$missingSelectedTests = @($selectedTests | Where-Object { $listedTests -notcontains $_ })
if ($missingSelectedTests.Count -gt 0) {
    throw "Remaining PDF CTest selection contains tests missing from ctest -N: $($missingSelectedTests -join ', ')"
}

$regex = Get-ExactRegex -Names $selectedTests
$startedAt = Get-Date
$completed = $true
$cleanedProcessIds = @()
$exitCode = 0
if ($selectedTests.Count -gt 0) {
    $ctestArguments = @(
        "--test-dir", $resolvedBuildDir,
        "-R", $regex,
        "--output-on-failure",
        "--timeout", [string]$CtestTimeoutSeconds
    )
    $launch = Get-LaunchSpec -Executable $resolvedCtestExecutable -Arguments $ctestArguments
    $process = Start-Process `
        -FilePath $launch.Executable `
        -ArgumentList $launch.Arguments `
        -WorkingDirectory $repoRoot `
        -RedirectStandardOutput $stdoutLog `
        -RedirectStandardError $stderrLog `
        -WindowStyle Hidden `
        -PassThru

    $completed = $process.WaitForExit($OuterTimeoutSeconds * 1000)
    if (-not $completed) {
        $descendants = @(Get-DescendantProcessIds -ParentId $process.Id)
        $cleanedProcessIds = @($descendants + $process.Id | Sort-Object -Descending -Unique)
        foreach ($processId in $cleanedProcessIds) {
            Stop-Process -Id $processId -Force -ErrorAction SilentlyContinue
        }
        $exitCode = 124
    } else {
        $process.Refresh()
        $exitCode = if ($null -ne $process.ExitCode) { [int]$process.ExitCode } else { 0 }
    }
} else {
    "" | Set-Content -LiteralPath $stdoutLog -Encoding UTF8
    "" | Set-Content -LiteralPath $stderrLog -Encoding UTF8
}
$finishedAt = Get-Date

$stdoutLines = Get-LogLines -Path $stdoutLog
$stderrLines = Get-LogLines -Path $stderrLog
$stats = Get-CtestLineStats -Lines ($stdoutLines + $stderrLines)
if ($selectedTests.Count -eq 0) {
    $stats = [pscustomobject]@{
        selected_test_count = 0
        completed_test_count = 0
        passed_test_count = 0
        failed_test_count = 0
        skipped_test_count = 0
        not_run_test_count = 0
        total_time_seconds = ""
        completed_tests = @()
    }
}

$selectedTestCount = [int]$selectedTests.Count
$completedTestCount = [int]$stats.completed_test_count
$passedTestCount = [int]$stats.passed_test_count
$failedTestCount = [int]$stats.failed_test_count
$skippedTestCount = [int]$stats.skipped_test_count
$notRunTestCount = [Math]::Max(0, $selectedTestCount - $completedTestCount)
$previousSelectedCount = Convert-ToIntOrZero -Value (Get-OptionalPropertyValue -Object $fullSummary -Name "selected_test_count")
$previousCompletedCount = Convert-ToIntOrZero -Value (Get-OptionalPropertyValue -Object $fullSummary -Name "completed_test_count")
$previousFailedCount = Convert-ToIntOrZero -Value (Get-OptionalPropertyValue -Object $fullSummary -Name "failed_test_count")
$previousSkippedCount = Convert-ToIntOrZero -Value (Get-OptionalPropertyValue -Object $fullSummary -Name "skipped_test_count")
$previousNotRunCount = Convert-ToIntOrZero -Value (Get-OptionalPropertyValue -Object $fullSummary -Name "not_run_test_count")
$combinedSelectedCount = if ($previousSelectedCount -gt 0) { $previousSelectedCount } else { $listedTests.Count }
$combinedCompletedCount = [Math]::Min($combinedSelectedCount, $previousCompletedCount + $completedTestCount)
$combinedFailedCount = $previousFailedCount + $failedTestCount
$combinedSkippedCount = $previousSkippedCount + $skippedTestCount
$combinedNotRunCount = [Math]::Max(0, $combinedSelectedCount - $combinedCompletedCount)
$completionPercent = if ($selectedTestCount -gt 0) {
    [Math]::Round(($completedTestCount / $selectedTestCount) * 100, 1)
} else {
    100.0
}
$combinedCompletionPercent = if ($combinedSelectedCount -gt 0) {
    [Math]::Round(($combinedCompletedCount / $combinedSelectedCount) * 100, 1)
} else {
    0.0
}
$coversPreviousRemaining = (
    $selectedTestCount -eq $previousNotRunCount -and
    $completedTestCount -eq $selectedTestCount -and
    $failedTestCount -eq 0 -and
    $combinedFailedCount -eq 0 -and
    $combinedNotRunCount -eq 0
)
$status = if (-not $completed) {
    "timeout"
} elseif ($selectedTestCount -eq 0 -and $previousNotRunCount -eq 0 -and $combinedFailedCount -eq 0 -and $combinedNotRunCount -eq 0) {
    "pass"
} elseif ($coversPreviousRemaining) {
    "pass"
} elseif ($selectedTestCount -gt 0 -and $exitCode -eq 0 -and $completedTestCount -eq $selectedTestCount -and $failedTestCount -eq 0) {
    "pass"
} else {
    "fail"
}
$verdict = if ($status -eq "pass") { "pass" } elseif ($status -eq "timeout") { "not_complete" } else { "fail" }

$summary = [ordered]@{
    schema = "featherdoc.pdf_ctest_remaining_guarded_summary.v1"
    generated_at = $finishedAt.ToString("s")
    started_at = $startedAt.ToString("s")
    finished_at = $finishedAt.ToString("s")
    status = $status
    verdict = $verdict
    remaining_ctest_status = if ($status -eq "pass") { "pass" } elseif ($status -eq "timeout") { "not_complete" } else { "fail" }
    evidence_scope = "remaining_pdf_ctest_tail_evidence"
    repo_root = $repoRoot
    build_dir = $resolvedBuildDir
    ctest_executable = $resolvedCtestExecutable
    ctest_pattern = $Pattern
    ctest_regex = $regex
    ctest_timeout_seconds = $CtestTimeoutSeconds
    outer_guard_status = if ($completed) { "completed" } else { "timed_out" }
    outer_guard_timed_out = -not $completed
    outer_guard_timeout_seconds = $OuterTimeoutSeconds
    exit_code = $exitCode
    full_ctest_summary_json = $resolvedFullCtestSummaryJson
    full_ctest_summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedFullCtestSummaryJson
    previous_stdout_log = $resolvedPreviousStdoutLog
    previous_stdout_log_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedPreviousStdoutLog
    previous_selected_test_count = $previousSelectedCount
    previous_completed_test_count = $previousCompletedCount
    previous_failed_test_count = $previousFailedCount
    previous_skipped_test_count = $previousSkippedCount
    previous_not_run_test_count = $previousNotRunCount
    listed_test_count = $listedTests.Count
    selected_test_count = $selectedTestCount
    completed_test_count = $completedTestCount
    passed_test_count = $passedTestCount
    failed_test_count = $failedTestCount
    skipped_test_count = $skippedTestCount
    not_run_test_count = $notRunTestCount
    completion_percent = $completionPercent
    selected_tests = @($selectedTests)
    completed_tests = @($stats.completed_tests)
    missing_selected_tests = @($missingSelectedTests)
    combined_selected_test_count = $combinedSelectedCount
    combined_completed_test_count = $combinedCompletedCount
    combined_failed_test_count = $combinedFailedCount
    combined_skipped_test_count = $combinedSkippedCount
    combined_not_run_test_count = $combinedNotRunCount
    combined_completion_percent = $combinedCompletionPercent
    combined_zero_failed_tests_observed = ($combinedSelectedCount -gt 0 -and $combinedFailedCount -eq 0)
    combined_tail_covers_previous_remaining = $coversPreviousRemaining
    stdout_log = $stdoutLog
    stdout_log_display = Get-DisplayPath -RepoRoot $repoRoot -Path $stdoutLog
    stderr_log = $stderrLog
    stderr_log_display = Get-DisplayPath -RepoRoot $repoRoot -Path $stderrLog
    cleaned_process_ids = @($cleanedProcessIds)
    boundary = "remaining_ctest_tail_evidence_does_not_replace_single_full_ctest"
    marker = "pdf_ctest_remaining_guarded_summary_trace"
}

$outputDir = Split-Path -Parent $resolvedOutputJson
if (-not [string]::IsNullOrWhiteSpace($outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
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
