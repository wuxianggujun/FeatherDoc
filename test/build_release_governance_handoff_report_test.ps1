param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "missing", "failed_report", "fail_on_missing", "fail_on_blocker", "fail_on_warning", "explicit_input", "explicit_only", "include_rollup", "informational_actions")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\build_release_governance_handoff_report_test"
}

. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_helpers.ps1")
. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_fixtures.ps1")

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_governance_handoff_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_aggregate_scenario.ps1")
. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_basic_scenarios.ps1")
. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_include_rollup_scenario.ps1")
. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_explicit_only_scenario.ps1")
. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_informational_actions_scenario.ps1")

Write-Host "Release governance handoff report regression passed."
