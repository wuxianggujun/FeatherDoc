<#
.SYNOPSIS
Writes a release-governance report for the PDF visual release gate preflight.

.DESCRIPTION
Turns the lightweight PDF visual release gate preflight summary into a
release_blockers/action_items report that can be consumed by the existing
release blocker rollup. The script is read-only for source and build outputs:
it does not build, render PDFs, install dependencies, or create virtual
environments.
#>
param(
    [string]$PreflightJson = "",
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$OutputDir = "output/pdf-visual-release-gate-preflight-governance",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [string]$CTestExecutable = "ctest",
    [switch]$FailOnBlocker
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$reportSchema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"

function Write-Step {
    param([string]$Message)
    Write-Host "[pdf-visual-preflight-governance] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }
    return $resolved
}

function Ensure-Directory {
    param([string]$Path)
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Get-DisplayPath {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    try {
        $root = [System.IO.Path]::GetFullPath($RepoRoot)
        if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
            -not $root.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
            $root += [System.IO.Path]::DirectorySeparatorChar
        }

        $resolved = [System.IO.Path]::GetFullPath($Path)
        if ($resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = $resolved.Substring($root.Length).TrimStart('\', '/')
            if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
            return ".\" + ($relative -replace '/', '\')
        }
        return $resolved
    } catch {
        return $Path
    }
}

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) { return $Object[$Name] }
        return $null
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-JsonString {
    param($Object, [string]$Name, [string]$DefaultValue = "")

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    return [string]$value
}

function Get-JsonBool {
    param($Object, [string]$Name, [bool]$DefaultValue = $false)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    if ($value -is [bool]) { return [bool]$value }
    return [string]$value -in @("true", "True", "1", "yes", "Yes")
}

function Get-JsonInt {
    param($Object, [string]$Name, [int]$DefaultValue = 0)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    try {
        return [int]$value
    } catch {
        return $DefaultValue
    }
}

function Get-JsonArray {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($value)) { return @() }
        return @($value)
    }
    if ($value -is [System.Collections.IDictionary]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Get-BlockingCheckNames {
    param($PreflightSummary)

    $declaredNames = @(
        Get-JsonArray -Object $PreflightSummary -Name "blocking_checks" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
    if ($declaredNames.Count -gt 0) {
        return @($declaredNames | Sort-Object -Unique)
    }

    return @(
        Get-JsonArray -Object $PreflightSummary -Name "checks" |
            Where-Object {
                (Get-JsonBool -Object $_ -Name "required") -and
                (Get-JsonString -Object $_ -Name "status") -ne "pass"
            } |
            ForEach-Object { Get-JsonString -Object $_ -Name "name" } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Sort-Object -Unique
    )
}

function Get-PreflightCheck {
    param($Checks, [string]$Name)

    foreach ($check in @($Checks)) {
        if ((Get-JsonString -Object $check -Name "name") -eq $Name) {
            return $check
        }
    }
    return $null
}

function Get-BuildDirectorySnapshotFromChecks {
    param($Checks)

    $buildDirCheck = Get-PreflightCheck -Checks $Checks -Name "build_dir_exists"
    $details = Get-JsonProperty -Object $buildDirCheck -Name "details"
    if ($null -eq $details) {
        return [ordered]@{}
    }

    return [ordered]@{
        cmake_cache_path = Get-JsonString -Object $details -Name "cmake_cache_path"
        cmake_cache_exists = Get-JsonBool -Object $details -Name "cmake_cache_exists"
        ctest_manifest_path = Get-JsonString -Object $details -Name "ctest_manifest_path"
        ctest_manifest_exists = Get-JsonBool -Object $details -Name "ctest_manifest_exists"
        entry_count = Get-JsonInt -Object $details -Name "entry_count"
        entries_preview = @(Get-JsonArray -Object $details -Name "entries_preview")
    }
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# PDF Visual Release Gate Preflight Governance Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Preflight status: ``$($Summary.preflight_status)``") | Out-Null
    $lines.Add("- Preflight summary: ``$($Summary.preflight_summary_json_display)``") | Out-Null
    $lines.Add("- Build dir: ``$($Summary.build_dir_display)``") | Out-Null
    $lines.Add("- Build dir source: ``$($Summary.build_dir_source)``") | Out-Null
    $lines.Add("- Requested build dir: ``$($Summary.requested_build_dir_display)``") | Out-Null
    $buildDirSnapshot = Get-JsonProperty -Object $Summary -Name "build_dir_snapshot"
    $hasBuildDirSnapshot = $false
    if ($null -ne $buildDirSnapshot) {
        if ($buildDirSnapshot -is [System.Collections.IDictionary]) {
            $hasBuildDirSnapshot = $buildDirSnapshot.Count -gt 0
        } else {
            $hasBuildDirSnapshot = @($buildDirSnapshot.PSObject.Properties).Count -gt 0
        }
    }
    if ($hasBuildDirSnapshot) {
        $lines.Add("- Build CMake cache: ``$(Get-JsonBool -Object $buildDirSnapshot -Name "cmake_cache_exists")``") | Out-Null
        $lines.Add("- Build CTest manifest: ``$(Get-JsonBool -Object $buildDirSnapshot -Name "ctest_manifest_exists")``") | Out-Null
        $lines.Add("- Build entry count: ``$(Get-JsonInt -Object $buildDirSnapshot -Name "entry_count")``") | Out-Null
    }
    $lines.Add("- Blocking checks: ``$($Summary.blocking_check_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Blocking Checks") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.blocking_checks).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($name in @($Summary.blocking_checks)) {
            $lines.Add("- ``$name``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.id)``: action=``$($blocker.action)`` status=``$($blocker.status)``") | Out-Null
            $lines.Add("  - message: $($blocker.message)") | Out-Null
            $lines.Add("  - source_json_display: ``$($blocker.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.command_template)) {
                $lines.Add("  - command_template: ``$($blocker.command_template)``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.id)``: action=``$($item.action)`` open_command=``$($item.open_command)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($item.source_json_display)``") | Out-Null
        }
    }
    return @($lines)
}

$repoRoot = Resolve-RepoRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "pdf_visual_release_gate_preflight_governance.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$preflightSummaryPath = if ([string]::IsNullOrWhiteSpace($PreflightJson)) {
    Join-Path $resolvedOutputDir "preflight-summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $PreflightJson
}

$preflightSummary = $null
$loadFailureMessage = ""
if ([string]::IsNullOrWhiteSpace($PreflightJson)) {
    $preflightScriptPath = Join-Path $repoRoot "scripts\check_pdf_visual_release_gate_preflight.ps1"
    if (-not (Test-Path -LiteralPath $preflightScriptPath -PathType Leaf)) {
        throw "PDF visual release gate preflight script was not found: $preflightScriptPath"
    }

    Write-Step "Running lightweight preflight"
    & $preflightScriptPath `
        -BuildDir $BuildDir `
        -OutputJson $preflightSummaryPath `
        -CTestExecutable $CTestExecutable | Out-Null
}

try {
    $preflightSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $preflightSummaryPath | ConvertFrom-Json
} catch {
    $loadFailureMessage = $_.Exception.Message
    $preflightSummary = [pscustomobject]@{
        status = "unavailable"
        checks = @()
        blocking_checks = @("preflight_summary_unavailable")
    }
}

$preflightStatus = Get-JsonString -Object $preflightSummary -Name "status" -DefaultValue "unknown"
$checks = @(Get-JsonArray -Object $preflightSummary -Name "checks")
$blockingChecks = @(Get-BlockingCheckNames -PreflightSummary $preflightSummary)
if (-not [string]::IsNullOrWhiteSpace($loadFailureMessage) -and $blockingChecks.Count -eq 0) {
    $blockingChecks = @("preflight_summary_unavailable")
}

$preflightDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $preflightSummaryPath
$summaryDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
$commandTemplate = "powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir $BuildDir -PreflightOnly"
$summaryBuildDir = Get-JsonString -Object $preflightSummary -Name "build_dir" -DefaultValue (Resolve-RepoPath -RepoRoot $repoRoot -Path $BuildDir -AllowMissing)
$summaryRequestedBuildDir = Get-JsonString -Object $preflightSummary -Name "requested_build_dir" -DefaultValue (Resolve-RepoPath -RepoRoot $repoRoot -Path $BuildDir -AllowMissing)
$summaryBuildDirSource = Get-JsonString -Object $preflightSummary -Name "build_dir_source" -DefaultValue "requested"
$buildDirSnapshot = Get-BuildDirectorySnapshotFromChecks -Checks $checks

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'

if (-not [string]::IsNullOrWhiteSpace($loadFailureMessage)) {
    $message = "PDF visual release gate preflight summary could not be read: $loadFailureMessage"
    $releaseBlockers.Add([ordered]@{
        id = "pdf_visual_release_gate_preflight.summary_unavailable"
        source = "pdf_visual_release_gate_preflight"
        severity = "error"
        status = "blocked"
        action = "rerun_pdf_visual_release_gate_preflight"
        message = $message
        source_schema = $reportSchema
        source_report = $summaryPath
        source_report_display = $summaryDisplay
        source_json = $preflightSummaryPath
        source_json_display = $preflightDisplay
        repair_strategy = "rerun_preflight"
        repair_hint = "Regenerate the PDF visual release gate preflight summary, then rebuild this governance report."
        command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 -BuildDir $BuildDir -OutputJson $preflightSummaryPath"
        blocked_item_count = 1
        issue_keys = @("preflight_summary_unavailable")
    }) | Out-Null
} elseif ($preflightStatus -ne "ready" -or $blockingChecks.Count -gt 0) {
    $blockedText = if ($blockingChecks.Count -gt 0) {
        $blockingChecks -join ", "
    } else {
        "preflight_status=$preflightStatus"
    }
    $repairHint = "Prepare a reusable PDF build directory with CTest registration, CLI baseline PDFs, manifest PDF outputs, helper scripts, and render Python before running the full PDF visual release gate."
    if ($buildDirSnapshot.Count -gt 0) {
        $snapshotHints = @()
        if (-not [bool]$buildDirSnapshot["cmake_cache_exists"]) {
            $snapshotHints += "CMakeCache.txt missing"
        }
        if (-not [bool]$buildDirSnapshot["ctest_manifest_exists"]) {
            $snapshotHints += "CTestTestfile.cmake missing"
        }
        if ($snapshotHints.Count -gt 0) {
            $repairHint += " Current build directory snapshot: $($snapshotHints -join '; ')."
        }
    }
    $message = "PDF visual release gate preflight is not ready; blocking checks: $blockedText."
    $releaseBlockers.Add([ordered]@{
        id = "pdf_visual_release_gate_preflight.build_outputs_missing"
        source = "pdf_visual_release_gate_preflight"
        severity = "error"
        status = "blocked"
        action = "prepare_pdf_visual_release_gate_build_outputs"
        message = $message
        source_schema = $reportSchema
        source_report = $summaryPath
        source_report_display = $summaryDisplay
        source_json = $preflightSummaryPath
        source_json_display = $preflightDisplay
        repair_strategy = "reuse_or_prepare_pdf_visual_release_gate_build_outputs"
        repair_hint = $repairHint
        command_template = $commandTemplate
        blocked_item_count = $blockingChecks.Count
        issue_keys = @($blockingChecks)
    }) | Out-Null
    $actionItems.Add([ordered]@{
        id = "prepare_pdf_visual_release_gate_build_outputs"
        action = "prepare_pdf_visual_release_gate_build_outputs"
        title = "Prepare reusable PDF visual release gate build outputs"
        open_command = $commandTemplate
        command_template = $commandTemplate
        source_schema = $reportSchema
        source_report = $summaryPath
        source_report_display = $summaryDisplay
        source_json = $preflightSummaryPath
        source_json_display = $preflightDisplay
        repair_strategy = "reuse_or_prepare_pdf_visual_release_gate_build_outputs"
        repair_hint = $repairHint
    }) | Out-Null
}

$sourceFailureCount = if ([string]::IsNullOrWhiteSpace($loadFailureMessage)) { 0 } else { 1 }
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0) {
    "blocked"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = $reportSchema
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = $summaryDisplay
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    preflight_status = $preflightStatus
    preflight_summary_json = $preflightSummaryPath
    preflight_summary_json_display = $preflightDisplay
    build_dir = $summaryBuildDir
    build_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryBuildDir
    build_dir_source = $summaryBuildDirSource
    requested_build_dir = $summaryRequestedBuildDir
    requested_build_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryRequestedBuildDir
    build_dir_snapshot = $buildDirSnapshot
    check_count = $checks.Count
    checks = @($checks)
    blocking_check_count = $blockingChecks.Count
    blocking_checks = @($blockingChecks)
    source_failure_count = $sourceFailureCount
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
