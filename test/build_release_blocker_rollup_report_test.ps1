param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "passing", "fail_on_warning", "comma_input", "empty", "malformed", "failed_source", "dedupe")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\build_release_blocker_rollup_report_test"
}

. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_helpers.ps1")

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_blocker_rollup_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_fixtures.ps1")
. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_passing_scenario.ps1")
. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_edge_scenarios.ps1")

Write-Host "Release blocker rollup report regression passed."
