<#
.SYNOPSIS
Exports and normalizes a template schema baseline from a DOCX.

.DESCRIPTION
Builds or reuses `featherdoc_cli`, runs `export-template-schema`, then runs
`normalize-template-schema` so the output can be committed as a stable schema
baseline.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\freeze_template_schema_baseline.ps1 `
    -InputDocx .\template.docx `
    -SchemaOutput .\template.schema.json `
    -ResolvedSectionTargets `
    -SkipBuild `
    -BuildDir build-codex-clang-column-visual-verify
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [string]$SchemaOutput = "",
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
    Write-Host "[template-schema-freeze] $Message"
}

if ($SectionTargets -and $ResolvedSectionTargets) {
    throw "--SectionTargets conflicts with --ResolvedSectionTargets."
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedSchemaOutput = if ([string]::IsNullOrWhiteSpace($SchemaOutput)) {
    $inputDirectory = [System.IO.Path]::GetDirectoryName($resolvedInputDocx)
    $inputStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedInputDocx)
    Join-Path $inputDirectory ($inputStem + ".template-schema.json")
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $SchemaOutput -AllowMissing
}

$outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedSchemaOutput)
if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}

$temporarySchemaPath = Join-Path $outputDirectory (
    [System.IO.Path]::GetFileNameWithoutExtension($resolvedSchemaOutput) +
    ".raw." + [System.IO.Path]::GetRandomFileName() + ".json")

try {
    Write-Step "Resolving featherdoc_cli"
    $cliPath = Resolve-TemplateSchemaCliPath `
        -RepoRoot $repoRoot `
        -BuildDir $BuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild

    $exportArguments = @(
        "export-template-schema",
        $resolvedInputDocx,
        "--output",
        $temporarySchemaPath,
        "--json"
    )
    if ($SectionTargets) {
        $exportArguments += "--section-targets"
    } elseif ($ResolvedSectionTargets) {
        $exportArguments += "--resolved-section-targets"
    }

    Write-Step "Exporting schema from $resolvedInputDocx"
    $exportResult = Invoke-TemplateSchemaCli -ExecutablePath $cliPath -Arguments $exportArguments
    if ($exportResult.ExitCode -ne 0) {
        foreach ($line in $exportResult.Output) {
            Write-Host $line
        }
        throw "export-template-schema failed with exit code $($exportResult.ExitCode)."
    }

    Write-Step "Normalizing schema into $resolvedSchemaOutput"
    $normalizeResult = Invoke-TemplateSchemaCli -ExecutablePath $cliPath -Arguments @(
        "normalize-template-schema",
        $temporarySchemaPath,
        "--output",
        $resolvedSchemaOutput,
        "--json"
    )
    if ($normalizeResult.ExitCode -ne 0) {
        foreach ($line in $normalizeResult.Output) {
            Write-Host $line
        }
        throw "normalize-template-schema failed with exit code $($normalizeResult.ExitCode)."
    }

    $summary = $null
    if (-not [string]::IsNullOrWhiteSpace($normalizeResult.Text)) {
        $summary = $normalizeResult.Text | ConvertFrom-Json
    }

    Write-Step "Schema baseline written to $resolvedSchemaOutput"
    if ($summary) {
        Write-Host "target_count: $($summary.target_count)"
        Write-Host "slot_count: $($summary.slot_count)"
    }
} finally {
    if (Test-Path -LiteralPath $temporarySchemaPath) {
        Remove-Item -LiteralPath $temporarySchemaPath -Force -ErrorAction SilentlyContinue
    }
}

