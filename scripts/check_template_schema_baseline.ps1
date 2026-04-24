<#
.SYNOPSIS
Checks a DOCX against a committed template schema baseline.

.DESCRIPTION
Builds or reuses `featherdoc_cli`, then runs `lint-template-schema` followed by
`check-template-schema` so the document can be gated against a normalized
schema baseline. The script returns exit code `0` only when the committed
schema is lint-clean and the generated schema matches; it returns `1` on
baseline lint failures or schema drift.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\check_template_schema_baseline.ps1 `
    -InputDocx .\template.docx `
    -SchemaFile .\template.schema.json `
    -ResolvedSectionTargets `
    -GeneratedSchemaOutput .\generated-template.schema.json `
    -RepairedSchemaOutput .\repaired-template.schema.json `
    -SkipBuild `
    -BuildDir build-codex-clang-column-visual-verify
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [Parameter(Mandatory = $true)]
    [string]$SchemaFile,
    [string]$GeneratedSchemaOutput = "",
    [string]$RepairedSchemaOutput = "",
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
$resolvedRepairedSchemaOutput = if ([string]::IsNullOrWhiteSpace($RepairedSchemaOutput)) {
    ""
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $RepairedSchemaOutput -AllowMissing
}

if (-not [string]::IsNullOrWhiteSpace($resolvedGeneratedSchemaOutput)) {
    $outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedGeneratedSchemaOutput)
    if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
        New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
    }
}
if (-not [string]::IsNullOrWhiteSpace($resolvedRepairedSchemaOutput)) {
    $repairOutputDirectory = [System.IO.Path]::GetDirectoryName($resolvedRepairedSchemaOutput)
    if (-not [string]::IsNullOrWhiteSpace($repairOutputDirectory)) {
        New-Item -ItemType Directory -Path $repairOutputDirectory -Force | Out-Null
    }
}

Write-Step "Resolving featherdoc_cli"
$cliPath = Resolve-TemplateSchemaCliPath `
    -RepoRoot $repoRoot `
    -BuildDir $BuildDir `
    -Generator $Generator `
    -SkipBuild:$SkipBuild

$lintArguments = @(
    "lint-template-schema",
    $resolvedSchemaFile,
    "--json"
)

Write-Step "Linting committed schema $resolvedSchemaFile"
$lintResult = Invoke-TemplateSchemaCli -ExecutablePath $cliPath -Arguments $lintArguments
if ($lintResult.ExitCode -notin @(0, 1)) {
    foreach ($line in $lintResult.Output) {
        Write-Host $line
    }
    throw "lint-template-schema failed with exit code $($lintResult.ExitCode)."
}

foreach ($line in $lintResult.Output) {
    Write-Host $line
}
$lintSummary = Get-TemplateSchemaCommandJsonObject `
    -Lines $lintResult.Output `
    -Command "lint-template-schema"
Write-Host "schema_lint_clean: $($lintSummary.clean)"
Write-Host "schema_lint_issue_count: $($lintSummary.issue_count)"

if (-not [bool]$lintSummary.clean -and
    -not [string]::IsNullOrWhiteSpace($resolvedRepairedSchemaOutput)) {
    $repairArguments = @(
        "repair-template-schema",
        $resolvedSchemaFile,
        "--output",
        $resolvedRepairedSchemaOutput,
        "--json"
    )

    Write-Step "Writing repaired schema candidate to $resolvedRepairedSchemaOutput"
    $repairResult = Invoke-TemplateSchemaCli -ExecutablePath $cliPath -Arguments $repairArguments
    if ($repairResult.ExitCode -ne 0) {
        foreach ($line in $repairResult.Output) {
            Write-Host $line
        }
        throw "repair-template-schema failed with exit code $($repairResult.ExitCode)."
    }

    foreach ($line in $repairResult.Output) {
        Write-Host $line
    }

    $repairSummary = Get-TemplateSchemaCommandJsonObject `
        -Lines $repairResult.Output `
        -Command "repair-template-schema"
    if (-not [string]::IsNullOrWhiteSpace([string]$repairSummary.output_path)) {
        Write-Host "repaired_schema_output_path: $($repairSummary.output_path)"
    }
}

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

foreach ($line in $checkResult.Output) {
    Write-Host $line
}

$summary = Get-TemplateSchemaCommandJsonObject `
    -Lines $checkResult.Output `
    -Command "check-template-schema"

if ($summary) {
    Write-Host "matches: $($summary.matches)"
    Write-Host "added_target_count: $($summary.added_target_count)"
    Write-Host "removed_target_count: $($summary.removed_target_count)"
    Write-Host "changed_target_count: $($summary.changed_target_count)"
    if (-not [string]::IsNullOrWhiteSpace($resolvedGeneratedSchemaOutput)) {
        Write-Host "generated_output_path: $resolvedGeneratedSchemaOutput"
    }
}

$hasLintIssues = -not [bool]$lintSummary.clean
$hasDrift = -not [bool]$summary.matches
exit $(if ($hasLintIssues -or $hasDrift) { 1 } else { 0 })
