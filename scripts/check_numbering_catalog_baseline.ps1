<#
.SYNOPSIS
Checks a DOCX against a committed numbering catalog baseline.

.DESCRIPTION
Builds or reuses `featherdoc_cli`, then runs `lint-numbering-catalog` followed
by `check-numbering-catalog` so the document can be gated against a stable
numbering catalog baseline. The script returns exit code `0` only when the
committed catalog is lint-clean and the generated catalog matches; it returns
`1` on baseline lint failures or catalog drift.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 `
    -InputDocx .\template.docx `
    -CatalogFile .\template.numbering-catalog.json `
    -GeneratedCatalogOutput .\generated-template.numbering-catalog.json `
    -SkipBuild `
    -BuildDir build-codex-clang-compat
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [Parameter(Mandatory = $true)]
    [string]$CatalogFile,
    [string]$GeneratedCatalogOutput = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[numbering-catalog-check] $Message"
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedCatalogFile = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $CatalogFile
$resolvedGeneratedCatalogOutput = if ([string]::IsNullOrWhiteSpace($GeneratedCatalogOutput)) {
    ""
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $GeneratedCatalogOutput -AllowMissing
}

if (-not [string]::IsNullOrWhiteSpace($resolvedGeneratedCatalogOutput)) {
    $outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedGeneratedCatalogOutput)
    if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
        New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
    }
}

Write-Step "Resolving featherdoc_cli"
$cliPath = Resolve-TemplateSchemaCliPath `
    -RepoRoot $repoRoot `
    -BuildDir $BuildDir `
    -Generator $Generator `
    -SkipBuild:$SkipBuild

$lintArguments = @(
    "lint-numbering-catalog",
    $resolvedCatalogFile,
    "--json"
)

Write-Step "Linting committed catalog $resolvedCatalogFile"
$lintResult = Invoke-TemplateSchemaCli -ExecutablePath $cliPath -Arguments $lintArguments
if ($lintResult.ExitCode -notin @(0, 1)) {
    foreach ($line in $lintResult.Output) {
        Write-Host $line
    }
    throw "lint-numbering-catalog failed with exit code $($lintResult.ExitCode)."
}

foreach ($line in $lintResult.Output) {
    Write-Host $line
}
$lintSummary = Get-TemplateSchemaCommandJsonObject `
    -Lines $lintResult.Output `
    -Command "lint-numbering-catalog"
Write-Host "catalog_lint_clean: $($lintSummary.clean)"
Write-Host "catalog_lint_issue_count: $($lintSummary.issue_count)"

$checkArguments = @(
    "check-numbering-catalog",
    $resolvedInputDocx,
    "--catalog-file",
    $resolvedCatalogFile,
    "--json"
)
if (-not [string]::IsNullOrWhiteSpace($resolvedGeneratedCatalogOutput)) {
    $checkArguments += @("--output", $resolvedGeneratedCatalogOutput)
}

Write-Step "Checking $resolvedInputDocx against $resolvedCatalogFile"
$checkResult = Invoke-TemplateSchemaCli -ExecutablePath $cliPath -Arguments $checkArguments
if ($checkResult.ExitCode -notin @(0, 1)) {
    foreach ($line in $checkResult.Output) {
        Write-Host $line
    }
    throw "check-numbering-catalog failed with exit code $($checkResult.ExitCode)."
}

foreach ($line in $checkResult.Output) {
    Write-Host $line
}

$summary = Get-TemplateSchemaCommandJsonObject `
    -Lines $checkResult.Output `
    -Command "check-numbering-catalog"

if ($summary) {
    Write-Host "matches: $($summary.matches)"
    Write-Host "clean: $($summary.clean)"
    Write-Host "baseline_issue_count: $($summary.baseline_issue_count)"
    Write-Host "generated_issue_count: $($summary.generated_issue_count)"
    Write-Host "added_definition_count: $($summary.added_definition_count)"
    Write-Host "removed_definition_count: $($summary.removed_definition_count)"
    Write-Host "changed_definition_count: $($summary.changed_definition_count)"
    if (-not [string]::IsNullOrWhiteSpace($resolvedGeneratedCatalogOutput)) {
        Write-Host "generated_output_path: $resolvedGeneratedCatalogOutput"
    }
}

$hasLintIssues = -not [bool]$lintSummary.clean
$hasCheckIssues = -not [bool]$summary.clean
$hasDrift = -not [bool]$summary.matches
exit $(if ($hasLintIssues -or $hasCheckIssues -or $hasDrift) { 1 } else { 0 })
