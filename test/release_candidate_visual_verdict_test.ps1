param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "contract", "candidate", "candidate_core", "candidate_reports")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "release_candidate_visual_verdict_helpers.ps1")

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptRoot = Join-Path $resolvedRepoRoot "scripts"
$scriptPath = Join-Path $scriptRoot "run_release_candidate_checks.ps1"
$releaseCandidateScriptPaths = @($scriptPath) + @(
    Get-ChildItem -LiteralPath $scriptRoot -Filter "run_release_candidate_checks_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { $_.FullName }
)
$scriptText = @(
    $releaseCandidateScriptPaths | ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_ }
) -join "`n"
$metadataHelpersPath = Join-Path $scriptRoot "release_blocker_metadata_helpers.ps1"
$metadataHelpersText = @(
    Get-Content -Raw -Encoding UTF8 -LiteralPath $metadataHelpersPath
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "scripts") -Filter "release_blocker_metadata_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
. $metadataHelpersPath
$templateSchemaCommonPath = Join-Path $resolvedRepoRoot "scripts\template_schema_cli_common.ps1"
. $templateSchemaCommonPath

. (Join-Path $PSScriptRoot "release_candidate_visual_verdict_contract_checks.ps1")
if ($Scenario -eq "contract") {
    return
}

. (Join-Path $PSScriptRoot "release_candidate_visual_verdict_candidate_setup.ps1")
. (Join-Path $PSScriptRoot "release_candidate_visual_verdict_candidate_core_assertions.ps1")
if ($Scenario -eq "candidate_core") {
    return
}

. (Join-Path $PSScriptRoot "release_candidate_visual_verdict_candidate_report_assertions.ps1")
