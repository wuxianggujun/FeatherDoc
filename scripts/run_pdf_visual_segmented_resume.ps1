<#
.SYNOPSIS
Runs resumable PDF visual baseline slices from attempt-summary evidence.

.DESCRIPTION
This helper consumes the resume fields written by
write_pdf_visual_gate_attempt_summary.ps1, executes bounded
VisualBaselineSliceOnly chunks, optionally rebuilds the aggregate contact
sheet, and refreshes segmented-summary.json. It is auxiliary evidence only and
does not replace a fresh non-FinalizeOnly full visual gate pass.
#>
param(
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$OutputDir = "output/pdf-visual-release-gate-current",
    [string]$AttemptSummaryJson = "output/pdf-visual-release-gate-current/report/attempt-summary.json",
    [string]$OutputJson = "",
    [string]$VisualGateScript = "scripts/run_pdf_visual_release_gate.ps1",
    [string]$SegmentedSummaryScript = "scripts/write_pdf_visual_segmented_gate_summary.ps1",
    [string]$PreflightScript = "scripts/check_pdf_visual_release_gate_preflight.ps1",
    [ValidateRange(1, 2147483647)]
    [int]$ChunkSize = 6,
    [ValidateRange(1, 2147483647)]
    [int]$PerSliceTimeoutSeconds = 60,
    [ValidateRange(0, 2147483647)]
    [int]$MaxSlices = 0,
    [ValidateRange(0, 2147483647)]
    [int]$Dpi = 144,
    [ValidateRange(0, 2147483647)]
    [int]$MinFreeMemoryMB = 2048,
    [switch]$SkipPreflight,
    [switch]$SkipMemoryGuard,
    [switch]$PlanOnly,
    [switch]$NoAggregateRebuild,
    [switch]$AllowPartialSuccessExitZero
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

    $fullPath = if ([System.IO.Path]::IsPathRooted($Path)) {
        [System.IO.Path]::GetFullPath($Path)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $Path))
    }

    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $fullPath)) {
        throw "Path does not exist: $Path"
    }

    return $fullPath
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

function Get-JsonProperty {
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

function Get-JsonInt {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return 0
    }

    return [int]$value
}

function Read-JsonFile {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Get-DescendantProcessIds {
    param([int]$ParentId)

    $children = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | Where-Object { $_.ParentProcessId -eq $ParentId })
    foreach ($child in $children) {
        [int]$child.ProcessId
        Get-DescendantProcessIds -ParentId ([int]$child.ProcessId)
    }
}

function Invoke-GuardedProcess {
    param(
        [string]$PowerShellPath,
        [string[]]$Arguments,
        [string]$WorkingDirectory,
        [string]$StdoutLog,
        [string]$StderrLog,
        [int]$TimeoutSeconds
    )

    $startedAt = Get-Date
    $process = Start-Process `
        -FilePath $PowerShellPath `
        -ArgumentList $Arguments `
        -WorkingDirectory $WorkingDirectory `
        -RedirectStandardOutput $StdoutLog `
        -RedirectStandardError $StderrLog `
        -WindowStyle Hidden `
        -PassThru

    $completed = $process.WaitForExit($TimeoutSeconds * 1000)
    $cleanedProcessIds = @()
    if (-not $completed) {
        $descendants = @(Get-DescendantProcessIds -ParentId $process.Id)
        $cleanedProcessIds = @($descendants + $process.Id | Sort-Object -Descending -Unique)
        foreach ($processId in $cleanedProcessIds) {
            Stop-Process -Id $processId -Force -ErrorAction SilentlyContinue
        }
    } else {
        $process.WaitForExit()
        $process.Refresh()
    }

    $exitCodeSource = "process_exit_code"
    $exitCode = if (-not $completed) {
        $exitCodeSource = "outer_timeout"
        124
    } elseif ($null -ne $process.ExitCode) {
        [int]$process.ExitCode
    } else {
        $exitCodeSource = "stderr_inference"
        $stderrText = if (Test-Path -LiteralPath $StderrLog) {
            Get-Content -Raw -Encoding UTF8 -LiteralPath $StderrLog
        } else {
            ""
        }
        if ([string]::IsNullOrWhiteSpace($stderrText)) { 0 } else { 1 }
    }

    $status = if (-not $completed) {
        "timeout"
    } elseif ($exitCode -eq 0) {
        "pass"
    } else {
        "fail"
    }

    return [pscustomobject][ordered]@{
        status = $status
        verdict = if ($status -eq "pass") { "pass" } elseif ($status -eq "fail") { "fail" } else { "not_complete" }
        started_at = $startedAt.ToString("s")
        finished_at = (Get-Date).ToString("s")
        timeout_seconds = $TimeoutSeconds
        timed_out = -not $completed
        exit_code = $exitCode
        exit_code_source = $exitCodeSource
        stdout_log = $StdoutLog
        stderr_log = $StderrLog
        cleaned_process_ids = @($cleanedProcessIds)
    }
}

function New-SlicePlan {
    param(
        [int]$Offset,
        [int]$Limit,
        [int]$Size,
        [int]$Max
    )

    $slices = New-Object System.Collections.Generic.List[object]
    $remaining = $Limit
    $currentOffset = $Offset
    while ($remaining -gt 0) {
        if ($Max -gt 0 -and $slices.Count -ge $Max) {
            break
        }

        $currentLimit = [Math]::Min($Size, $remaining)
        $slices.Add([ordered]@{
            offset = $currentOffset
            limit = $currentLimit
        }) | Out-Null
        $currentOffset += $currentLimit
        $remaining -= $currentLimit
    }

    return @($slices.ToArray())
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
$resolvedReportDir = Join-Path $resolvedOutputDir "report"
$resolvedAttemptSummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $AttemptSummaryJson
$resolvedVisualGateScript = Resolve-RepoPath -RepoRoot $repoRoot -Path $VisualGateScript
$resolvedSegmentedSummaryScript = Resolve-RepoPath -RepoRoot $repoRoot -Path $SegmentedSummaryScript -AllowMissing
$resolvedPreflightScript = Resolve-RepoPath -RepoRoot $repoRoot -Path $PreflightScript -AllowMissing
$resolvedOutputJson = if ([string]::IsNullOrWhiteSpace($OutputJson)) {
    Join-Path $resolvedReportDir "segmented-resume-summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputJson -AllowMissing
}

New-Item -ItemType Directory -Path $resolvedReportDir -Force | Out-Null
$attemptSummary = Read-JsonFile -Path $resolvedAttemptSummaryJson
if ($null -eq $attemptSummary) {
    throw "Attempt summary was not found: $resolvedAttemptSummaryJson"
}

$resumeNeeded = [bool](Get-JsonProperty -Object $attemptSummary -Name "visual_baseline_resume_needed")
$resumeOffset = Get-JsonInt -Object $attemptSummary -Name "visual_baseline_resume_slice_offset"
$resumeLimit = Get-JsonInt -Object $attemptSummary -Name "visual_baseline_resume_slice_limit"
$expectedVisualRenderCount = Get-JsonInt -Object $attemptSummary -Name "expected_visual_render_count"
$freshRenderedCount = Get-JsonInt -Object $attemptSummary -Name "visual_baseline_fresh_rendered_count"
$slicePlan = if ($resumeNeeded -and $resumeLimit -gt 0) {
    New-SlicePlan -Offset $resumeOffset -Limit $resumeLimit -Size $ChunkSize -Max $MaxSlices
} else {
    @()
}
$totalResumeSliceCount = if ($resumeNeeded -and $resumeLimit -gt 0) {
    [int][Math]::Ceiling([double]$resumeLimit / [double]$ChunkSize)
} else {
    0
}
$resumeTailFullyPlanned = $MaxSlices -le 0 -or @($slicePlan).Count -ge $totalResumeSliceCount

$powerShellPath = Get-PowerShellExecutable
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$preflightResult = $null
$sliceResults = New-Object System.Collections.Generic.List[object]
$aggregateResult = $null
$segmentedResult = $null
$blockedReason = ""

if (-not $PlanOnly -and -not $SkipPreflight) {
    $preflightJson = Join-Path $resolvedReportDir "segmented-resume-preflight-summary.json"
    $preflightArgs = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $resolvedPreflightScript,
        "-BuildDir",
        $resolvedBuildDir,
        "-OutputJson",
        $preflightJson,
        "-MinFreeMemoryMB",
        [string]$MinFreeMemoryMB,
        "-Strict"
    )
    if ($SkipMemoryGuard) {
        $preflightArgs += "-SkipMemoryGuard"
    }

    $preflightResult = Invoke-GuardedProcess `
        -PowerShellPath $powerShellPath `
        -Arguments $preflightArgs `
        -WorkingDirectory $repoRoot `
        -StdoutLog (Join-Path $resolvedReportDir "segmented-resume-preflight-$stamp.out.log") `
        -StderrLog (Join-Path $resolvedReportDir "segmented-resume-preflight-$stamp.err.log") `
        -TimeoutSeconds $PerSliceTimeoutSeconds

    if ($preflightResult.status -ne "pass") {
        $blockedReason = "preflight_not_ready"
    }
}

if (-not $PlanOnly -and [string]::IsNullOrWhiteSpace($blockedReason)) {
    foreach ($slice in $slicePlan) {
        $sliceId = "visual-baseline-slice-offset-$($slice.offset)-limit-$($slice.limit)"
        $sliceArgs = @(
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
            "-VisualBaselineSliceOnly",
            "-VisualBaselineOffset",
            [string]$slice.offset,
            "-VisualBaselineLimit",
            [string]$slice.limit,
            "-SkipPreflight"
        )
        if ($SkipMemoryGuard) {
            $sliceArgs += "-SkipMemoryGuard"
        }

        $result = Invoke-GuardedProcess `
            -PowerShellPath $powerShellPath `
            -Arguments $sliceArgs `
            -WorkingDirectory $repoRoot `
            -StdoutLog (Join-Path $resolvedReportDir "$sliceId-$stamp.out.log") `
            -StderrLog (Join-Path $resolvedReportDir "$sliceId-$stamp.err.log") `
            -TimeoutSeconds $PerSliceTimeoutSeconds
        $sliceResults.Add([ordered]@{
            id = $sliceId
            offset = $slice.offset
            limit = $slice.limit
            status = $result.status
            verdict = $result.verdict
            timeout_seconds = $result.timeout_seconds
            timed_out = $result.timed_out
            exit_code = $result.exit_code
            exit_code_source = $result.exit_code_source
            stdout_log = $result.stdout_log
            stdout_log_display = Get-DisplayPath -RepoRoot $repoRoot -Path $result.stdout_log
            stderr_log = $result.stderr_log
            stderr_log_display = Get-DisplayPath -RepoRoot $repoRoot -Path $result.stderr_log
            cleaned_process_ids = @($result.cleaned_process_ids)
        }) | Out-Null

        if ($result.status -ne "pass") {
            break
        }
    }
}

$sliceResultArray = @($sliceResults.ToArray())
$allPlannedSlicesExecuted = $sliceResultArray.Count -eq @($slicePlan).Count
$allExecutedSlicesPassed = @($sliceResultArray | Where-Object { $_.status -ne "pass" }).Count -eq 0
if (-not $PlanOnly -and
    [string]::IsNullOrWhiteSpace($blockedReason) -and
    @($slicePlan).Count -gt 0 -and
    $resumeTailFullyPlanned -and
    $allPlannedSlicesExecuted -and
    $allExecutedSlicesPassed -and
    -not $NoAggregateRebuild) {
    $aggregateArgs = @(
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
        "-RebuildAggregateContactSheetOnly",
        "-SkipPreflight"
    )
    if ($SkipMemoryGuard) {
        $aggregateArgs += "-SkipMemoryGuard"
    }

    $aggregateResult = Invoke-GuardedProcess `
        -PowerShellPath $powerShellPath `
        -Arguments $aggregateArgs `
        -WorkingDirectory $repoRoot `
        -StdoutLog (Join-Path $resolvedReportDir "segmented-resume-aggregate-$stamp.out.log") `
        -StderrLog (Join-Path $resolvedReportDir "segmented-resume-aggregate-$stamp.err.log") `
        -TimeoutSeconds $PerSliceTimeoutSeconds

    if ($aggregateResult.status -eq "pass" -and (Test-Path -LiteralPath $resolvedSegmentedSummaryScript)) {
        $segmentedArgs = @(
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            $resolvedSegmentedSummaryScript,
            "-RepoRoot",
            $repoRoot,
            "-ReportDir",
            $resolvedReportDir
        )
        $segmentedResult = Invoke-GuardedProcess `
            -PowerShellPath $powerShellPath `
            -Arguments $segmentedArgs `
            -WorkingDirectory $repoRoot `
            -StdoutLog (Join-Path $resolvedReportDir "segmented-resume-summary-$stamp.out.log") `
            -StderrLog (Join-Path $resolvedReportDir "segmented-resume-summary-$stamp.err.log") `
            -TimeoutSeconds $PerSliceTimeoutSeconds
    }
}

$sliceResultArray = @($sliceResults.ToArray())
$passedSliceCount = @($sliceResultArray | Where-Object { $_.status -eq "pass" }).Count
$failedSliceCount = @($sliceResultArray | Where-Object { $_.status -eq "fail" }).Count
$timeoutSliceCount = @($sliceResultArray | Where-Object { $_.status -eq "timeout" }).Count
$aggregateStatus = if ($null -eq $aggregateResult) {
    if ($NoAggregateRebuild) { "skipped" } else { "not_run" }
} else {
    [string]$aggregateResult.status
}
$segmentedStatus = if ($null -eq $segmentedResult) { "not_run" } else { [string]$segmentedResult.status }

$status = if ($PlanOnly) {
    "planned"
} elseif (-not [string]::IsNullOrWhiteSpace($blockedReason)) {
    "blocked"
} elseif ($failedSliceCount -gt 0 -or $aggregateStatus -eq "fail" -or $segmentedStatus -eq "fail") {
    "fail"
} elseif ($timeoutSliceCount -gt 0 -or $aggregateStatus -eq "timeout" -or $segmentedStatus -eq "timeout") {
    "timeout"
} elseif (@($slicePlan).Count -eq 0 -or ($resumeTailFullyPlanned -and $allPlannedSlicesExecuted -and $allExecutedSlicesPassed -and ($NoAggregateRebuild -or $aggregateStatus -eq "pass"))) {
    "pass"
} else {
    "partial"
}

$summary = [ordered]@{
    schema = "featherdoc.pdf_visual_segmented_resume_summary.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    verdict = if ($status -eq "pass") { "pass" } elseif ($status -eq "fail") { "fail" } else { "not_complete" }
    full_visual_gate_status = "not_complete"
    evidence_scope = "visual_segmented_resume_auxiliary_only"
    boundary = "segmented_resume_does_not_replace_full_visual_gate_verdict"
    repo_root = $repoRoot
    build_dir = $resolvedBuildDir
    build_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedBuildDir
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    report_dir = $resolvedReportDir
    report_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedReportDir
    attempt_summary_json = $resolvedAttemptSummaryJson
    attempt_summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedAttemptSummaryJson
    output_json = $resolvedOutputJson
    output_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputJson
    plan_only = [bool]$PlanOnly
    skip_preflight = [bool]$SkipPreflight
    skip_memory_guard = [bool]$SkipMemoryGuard
    allow_partial_success_exit_zero = [bool]$AllowPartialSuccessExitZero
    min_free_memory_mb = $MinFreeMemoryMB
    per_slice_timeout_seconds = $PerSliceTimeoutSeconds
    chunk_size = $ChunkSize
    max_slices = $MaxSlices
    resume_needed = $resumeNeeded
    resume_offset = $resumeOffset
    resume_limit = $resumeLimit
    expected_visual_render_count = $expectedVisualRenderCount
    attempt_fresh_rendered_count = $freshRenderedCount
    total_resume_slice_count = $totalResumeSliceCount
    resume_tail_fully_planned = $resumeTailFullyPlanned
    planned_slice_count = @($slicePlan).Count
    executed_slice_count = $sliceResultArray.Count
    passed_slice_count = $passedSliceCount
    failed_slice_count = $failedSliceCount
    timeout_slice_count = $timeoutSliceCount
    blocked_reason = $blockedReason
    preflight_status = if ($null -eq $preflightResult) { if ($SkipPreflight -or $PlanOnly) { "skipped" } else { "not_run" } } else { [string]$preflightResult.status }
    aggregate_rebuild_status = $aggregateStatus
    segmented_summary_status = $segmentedStatus
    slices = @($sliceResultArray)
    planned_slices = @($slicePlan)
    marker = "pdf_visual_segmented_resume_summary_trace"
}

New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($resolvedOutputJson)) -Force | Out-Null
($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8
$summary | ConvertTo-Json -Depth 12

if ($status -in @("pass", "planned")) {
    exit 0
}
if ($status -eq "partial" -and $AllowPartialSuccessExitZero) {
    exit 0
}
if ($status -eq "timeout") {
    exit 124
}
exit 1
