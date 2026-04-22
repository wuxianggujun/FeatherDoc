<#
.SYNOPSIS
Checks a DOCX against a committed template schema baseline.

.DESCRIPTION
Builds or reuses `featherdoc_cli`, then runs `check-template-schema` so the
document can be gated against a normalized schema baseline. The script returns
exit code `0` on match and `1` on drift.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\check_template_schema_baseline.ps1 `
    -InputDocx .\template.docx `
    -SchemaFile .\template.schema.json `
    -ResolvedSectionTargets `
    -GeneratedSchemaOutput .\generated-template.schema.json `
    -SkipBuild `
    -BuildDir build-codex-clang-column-visual-verify
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [Parameter(Mandatory = $true)]
    [string]$SchemaFile,
    [string]$GeneratedSchemaOutput = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [switch]$SkipBuild,
    [switch]$SectionTargets,
    [switch]$ResolvedSectionTargets
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[template-schema-check] $Message"
}

if ($SectionTargets -and $ResolvedSectionTargets) {
    throw "--SectionTargets conflicts with --ResolvedSectionTargets."
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedSchemaFile = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $SchemaFile
$resolvedGeneratedSchemaOutput = if ([string]::IsNullOrWhiteSpace($GeneratedSchemaOutput)) {
    ""
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $GeneratedSchemaOutput -AllowMissing
}

if (-not [string]::IsNullOrWhiteSpace($resolvedGeneratedSchemaOutput)) {
    $outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedGeneratedSchemaOutput)
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

$checkArguments = @(
    "check-template-schema",
    $resolvedInputDocx,
    "--schema-file",
    $resolvedSchemaFile,
    "--json"
)
if ($SectionTargets) {
    $checkArguments += "--section-targets"
} elseif ($ResolvedSectionTargets) {
    $checkArguments += "--resolved-section-targets"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedGeneratedSchemaOutput)) {
    $checkArguments += @("--output", $resolvedGeneratedSchemaOutput)
}

Write-Step "Checking $resolvedInputDocx against $resolvedSchemaFile"
$checkResult = Invoke-TemplateSchemaCli -ExecutablePath $cliPath -Arguments $checkArguments
if ($checkResult.ExitCode -notin @(0, 1)) {
    foreach ($line in $checkResult.Output) {
        Write-Host $line
    }
    throw "check-template-schema failed with exit code $($checkResult.ExitCode)."
}

$summary = $null
if (-not [string]::IsNullOrWhiteSpace($checkResult.Text)) {
    foreach ($line in $checkResult.Output) {
        Write-Host $line
    }
    $summary = $checkResult.Text | ConvertFrom-Json
}

if ($summary) {
    Write-Host "matches: $($summary.matches)"
    Write-Host "added_target_count: $($summary.added_target_count)"
    Write-Host "removed_target_count: $($summary.removed_target_count)"
    Write-Host "changed_target_count: $($summary.changed_target_count)"
    if (-not [string]::IsNullOrWhiteSpace($resolvedGeneratedSchemaOutput)) {
        Write-Host "generated_output_path: $resolvedGeneratedSchemaOutput"
    }
}

exit $checkResult.ExitCode
