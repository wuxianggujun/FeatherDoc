<#
.SYNOPSIS
Validates a project template smoke manifest before the full harness runs.

.DESCRIPTION
Reads one `project_template_smoke` manifest, validates the entry structure,
selection shapes, slot declarations, and optional path references, then emits
either a human-readable report or JSON. Exit code `0` means the manifest is
valid; exit code `1` means the manifest contains one or more errors.
#>
param(
    [string]$ManifestPath = "samples/project_template_smoke.manifest.json",
    [string]$BuildDir = "",
    [string]$OutputPath = "",
    [switch]$CheckPaths,
    [switch]$Json
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "project_template_smoke_manifest_common.ps1")

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    return Resolve-ProjectTemplateSmokePath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing:$AllowMissing
}

function Get-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

$repoRoot = Resolve-RepoRoot
$resolvedManifestPath = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $ManifestPath
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
}

$report = Test-ProjectTemplateSmokeManifest `
    -RepoRoot $repoRoot `
    -ManifestPath $resolvedManifestPath `
    -BuildDir $resolvedBuildDir `
    -CheckPaths:$CheckPaths

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputPath -AllowMissing
}

if ($Json) {
    $jsonText = $report | ConvertTo-Json -Depth 8
    if ([string]::IsNullOrWhiteSpace($resolvedOutputPath)) {
        Write-Output $jsonText
    } else {
        $directory = Split-Path -Parent $resolvedOutputPath
        if (-not [string]::IsNullOrWhiteSpace($directory)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
        }
        $jsonText | Set-Content -LiteralPath $resolvedOutputPath -Encoding UTF8
        Write-Host "[project-template-smoke-manifest] Wrote JSON report to $resolvedOutputPath"
    }

    if (-not $report.passed) {
        exit 1
    }

    exit 0
}

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("Project template smoke manifest: $(Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedManifestPath)")
[void]$lines.Add("Build dir: $(Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedBuildDir)")
[void]$lines.Add("Check paths: $([string]$CheckPaths.IsPresent)")
[void]$lines.Add("Entries: $($report.entry_count)")
[void]$lines.Add("Passed: $($report.passed)")
[void]$lines.Add("Errors: $($report.error_count)")
[void]$lines.Add("")

foreach ($entry in $report.entries) {
    $displayName = if ([string]::IsNullOrWhiteSpace($entry.name)) { "(unnamed entry)" } else { $entry.name }
    [void]$lines.Add("- $displayName")
    [void]$lines.Add("  checks: $((@($entry.configured_checks) -join ', '))")
    [void]$lines.Add("  errors: $($entry.error_count)")
    foreach ($issue in @($entry.errors)) {
        [void]$lines.Add("  - $($issue.path): $($issue.message)")
    }
}

if ($report.error_count -gt 0) {
    [void]$lines.Add("")
    [void]$lines.Add("All issues:")
    foreach ($issue in @($report.errors)) {
        [void]$lines.Add("- $($issue.path): $($issue.message)")
    }
}

if ([string]::IsNullOrWhiteSpace($resolvedOutputPath)) {
    $lines | ForEach-Object { Write-Host $_ }
} else {
    $directory = Split-Path -Parent $resolvedOutputPath
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
    $lines | Set-Content -LiteralPath $resolvedOutputPath -Encoding UTF8
    Write-Host "[project-template-smoke-manifest] Wrote text report to $resolvedOutputPath"
}

if (-not $report.passed) {
    exit 1
}

exit 0
