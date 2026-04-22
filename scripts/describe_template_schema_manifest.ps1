<#
.SYNOPSIS
Prints a maintenance-friendly summary of the registered template schema manifest.

.DESCRIPTION
Reads `baselines/template-schema/manifest.json`, optionally joins it with the
latest manifest-check summary, then prints a human-readable overview of the
currently registered baselines. This is useful when you want to confirm what
the repository-level template schema gate currently covers without manually
opening both `manifest.json` and `summary.json`.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\describe_template_schema_manifest.ps1

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\describe_template_schema_manifest.ps1 `
    -ManifestPath .\baselines\template-schema\manifest.json `
    -SummaryJson .\output\template-schema-manifest-checks-registered\summary.json `
    -Json
#>
param(
    [string]$ManifestPath = "baselines/template-schema/manifest.json",
    [string]$SummaryJson = "",
    [string]$BuildDir = "",
    [string]$OutputPath = "",
    [switch]$Json
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[template-schema-manifest-describe] $Message"
}

function Resolve-OptionalManifestPropertyValue {
    param(
        $Entry,
        [string]$Name
    )

    $property = $Entry.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Get-RepoRelativeDisplayPath {
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

function Resolve-ManifestEntryInputPath {
    param(
        [string]$RepoRoot,
        [string]$ResolvedBuildDir,
        $Entry
    )

    $repoRelativePath = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "input_docx"
    $buildRelativePath = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "input_docx_build_relative"

    if (-not [string]::IsNullOrWhiteSpace($repoRelativePath) -and
        -not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        throw "Manifest entry '$([string]$Entry.name)' sets both 'input_docx' and 'input_docx_build_relative'."
    }

    if (-not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        if ([string]::IsNullOrWhiteSpace($ResolvedBuildDir)) {
            return ""
        }

        return Resolve-TemplateSchemaPath -RepoRoot $ResolvedBuildDir -InputPath $buildRelativePath -AllowMissing
    }

    if ([string]::IsNullOrWhiteSpace($repoRelativePath)) {
        return ""
    }

    return Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $repoRelativePath -AllowMissing
}

function Get-ManifestEntrySourceType {
    param($Entry)

    $repoRelativePath = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "input_docx"
    $buildRelativePath = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "input_docx_build_relative"

    if (-not [string]::IsNullOrWhiteSpace($repoRelativePath)) {
        return "repository-docx"
    }
    if (-not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        return "build-relative-fixture"
    }

    return "unspecified"
}

function Get-ManifestSummaryLookup {
    param($Summary)

    $lookup = @{}
    if ($null -eq $Summary) {
        return $lookup
    }

    foreach ($entry in @($Summary.entries)) {
        $name = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "name"
        if (-not [string]::IsNullOrWhiteSpace($name)) {
            $lookup[$name] = $entry
        }
    }

    return $lookup
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedManifestPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $ManifestPath
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    ""
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
}

$defaultSummaryPath = Resolve-TemplateSchemaPath `
    -RepoRoot $repoRoot `
    -InputPath "output/template-schema-manifest-checks-registered/summary.json" `
    -AllowMissing
$resolvedSummaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    if (Test-Path -LiteralPath $defaultSummaryPath) {
        $defaultSummaryPath
    } else {
        ""
    }
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $SummaryJson -AllowMissing
}

$manifest = Get-Content -Raw -LiteralPath $resolvedManifestPath | ConvertFrom-Json
$summary = if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryPath) -and
    (Test-Path -LiteralPath $resolvedSummaryPath)) {
    Get-Content -Raw -LiteralPath $resolvedSummaryPath | ConvertFrom-Json
} else {
    $null
}
$summaryLookup = Get-ManifestSummaryLookup -Summary $summary

$entries = New-Object 'System.Collections.Generic.List[object]'
foreach ($entry in @($manifest.entries)) {
    $name = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "name"
    $resolvedInputPath = Resolve-ManifestEntryInputPath -RepoRoot $repoRoot -ResolvedBuildDir $resolvedBuildDir -Entry $entry
    $sourceType = Get-ManifestEntrySourceType -Entry $entry
    $summaryEntry = if ($summaryLookup.ContainsKey($name)) { $summaryLookup[$name] } else { $null }

    $entries.Add([pscustomobject]@{
        name = $name
        source_type = $sourceType
        input_docx = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "input_docx"
        input_docx_build_relative = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "input_docx_build_relative"
        resolved_input_docx = $resolvedInputPath
        prepare_sample_target = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "prepare_sample_target"
        schema_file = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "schema_file"
        target_mode = if ([string]::IsNullOrWhiteSpace((Resolve-OptionalManifestPropertyValue -Entry $entry -Name "target_mode"))) {
            "default"
        } else {
            Resolve-OptionalManifestPropertyValue -Entry $entry -Name "target_mode"
        }
        generated_output = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "generated_output"
        latest_available = ($null -ne $summaryEntry)
        latest_matches = if ($null -eq $summaryEntry) { $null } else { [bool]$summaryEntry.matches }
        latest_added_target_count = if ($null -eq $summaryEntry) { $null } else { [int]$summaryEntry.added_target_count }
        latest_removed_target_count = if ($null -eq $summaryEntry) { $null } else { [int]$summaryEntry.removed_target_count }
        latest_changed_target_count = if ($null -eq $summaryEntry) { $null } else { [int]$summaryEntry.changed_target_count }
        latest_generated_output_path = if ($null -eq $summaryEntry) { "" } else { [string]$summaryEntry.generated_output_path }
    }) | Out-Null
}

$report = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    manifest_path = $resolvedManifestPath
    summary_json = if ($summary) { $resolvedSummaryPath } else { "" }
    build_dir = $resolvedBuildDir
    entry_count = $entries.Count
    latest_passed = if ($summary) { [bool]$summary.passed } else { $null }
    latest_drift_count = if ($summary) { [int]$summary.drift_count } else { $null }
    entries = $entries
}

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    ""
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $OutputPath -AllowMissing
}

if ($Json) {
    $jsonText = $report | ConvertTo-Json -Depth 6
    if (-not [string]::IsNullOrWhiteSpace($resolvedOutputPath)) {
        New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputPath) -Force | Out-Null
        $jsonText | Set-Content -LiteralPath $resolvedOutputPath -Encoding UTF8
        Write-Step "Wrote manifest description JSON to $resolvedOutputPath"
    } else {
        Write-Output $jsonText
    }
    exit 0
}

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("Template schema manifest: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedManifestPath)")
[void]$lines.Add("Registered baselines: $($entries.Count)")
if ($summary) {
    [void]$lines.Add("Latest summary: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedSummaryPath)")
    [void]$lines.Add("Latest gate result: passed=$([string]$summary.passed) drift_count=$([int]$summary.drift_count)")
} else {
    [void]$lines.Add("Latest summary: (not available)")
}
if (-not [string]::IsNullOrWhiteSpace($resolvedBuildDir)) {
    [void]$lines.Add("Build dir: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedBuildDir)")
}
[void]$lines.Add("")

foreach ($entry in $entries) {
    [void]$lines.Add("- $($entry.name)")
    [void]$lines.Add("  source_type: $($entry.source_type)")
    if (-not [string]::IsNullOrWhiteSpace($entry.input_docx)) {
        [void]$lines.Add("  manifest_input_docx: $($entry.input_docx)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.input_docx_build_relative)) {
        [void]$lines.Add("  manifest_input_docx_build_relative: $($entry.input_docx_build_relative)")
    }
    [void]$lines.Add("  resolved_input_docx: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $entry.resolved_input_docx)")
    [void]$lines.Add("  schema_file: $($entry.schema_file)")
    [void]$lines.Add("  target_mode: $($entry.target_mode)")
    if (-not [string]::IsNullOrWhiteSpace($entry.prepare_sample_target)) {
        [void]$lines.Add("  prepare_sample_target: $($entry.prepare_sample_target)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.generated_output)) {
        [void]$lines.Add("  generated_output: $($entry.generated_output)")
    }
    if ($entry.latest_available) {
        [void]$lines.Add("  latest_matches: $($entry.latest_matches)")
        [void]$lines.Add("  latest_drift_counts: $($entry.latest_added_target_count)/$($entry.latest_removed_target_count)/$($entry.latest_changed_target_count)")
        [void]$lines.Add("  latest_generated_output_path: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $entry.latest_generated_output_path)")
    } else {
        [void]$lines.Add("  latest_matches: (not available)")
    }
    [void]$lines.Add("")
}

$text = $lines -join [Environment]::NewLine
if (-not [string]::IsNullOrWhiteSpace($resolvedOutputPath)) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputPath) -Force | Out-Null
    $text | Set-Content -LiteralPath $resolvedOutputPath -Encoding UTF8
    Write-Step "Wrote manifest description text to $resolvedOutputPath"
} else {
    Write-Output $text
}
