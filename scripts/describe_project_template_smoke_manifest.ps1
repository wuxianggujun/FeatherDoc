<#
.SYNOPSIS
Prints a maintenance-friendly summary of the registered project template smoke manifest.

.DESCRIPTION
Reads a `project_template_smoke` manifest, optionally joins it with one
previously generated smoke `summary.json`, then prints either a human-readable
overview or JSON so maintainers can quickly confirm which templates and checks
are registered.
#>
param(
    [string]$ManifestPath = "samples/project_template_smoke.manifest.json",
    [string]$SummaryJson = "",
    [string]$BuildDir = "",
    [string]$OutputPath = "",
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

    $repoRelativePath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Entry -Name "input_docx"
    $buildRelativePath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Entry -Name "input_docx_build_relative"

    if (-not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        if ([string]::IsNullOrWhiteSpace($ResolvedBuildDir)) {
            return ""
        }

        return Resolve-RepoPath -RepoRoot $ResolvedBuildDir -InputPath $buildRelativePath -AllowMissing
    }

    if ([string]::IsNullOrWhiteSpace($repoRelativePath)) {
        return ""
    }

    return Resolve-RepoPath -RepoRoot $RepoRoot -InputPath $repoRelativePath -AllowMissing
}

function Get-ManifestEntrySourceType {
    param($Entry)

    $repoRelativePath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Entry -Name "input_docx"
    $buildRelativePath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Entry -Name "input_docx_build_relative"

    if (-not [string]::IsNullOrWhiteSpace($repoRelativePath)) {
        return "repository-docx"
    }
    if (-not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        return "build-relative-fixture"
    }

    return "unspecified"
}

function Get-SummaryLookup {
    param($Summary)

    $lookup = @{}
    if ($null -eq $Summary) {
        return $lookup
    }

    foreach ($entry in @(Get-ProjectTemplateSmokeArrayProperty -Object $Summary -Name "entries")) {
        $name = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "name"
        if (-not [string]::IsNullOrWhiteSpace($name)) {
            $lookup[$name] = $entry
        }
    }

    return $lookup
}

$repoRoot = Resolve-RepoRoot
$resolvedManifestPath = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $ManifestPath
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
}

$defaultSummaryPath = Resolve-RepoPath -RepoRoot $repoRoot -InputPath "output/project-template-smoke/summary.json" -AllowMissing
$resolvedSummaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    if (Test-Path -LiteralPath $defaultSummaryPath) {
        $defaultSummaryPath
    } else {
        ""
    }
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SummaryJson -AllowMissing
}

$manifest = Get-Content -Raw -LiteralPath $resolvedManifestPath | ConvertFrom-Json
$summary = if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryPath) -and
    (Test-Path -LiteralPath $resolvedSummaryPath)) {
    Get-Content -Raw -LiteralPath $resolvedSummaryPath | ConvertFrom-Json
} else {
    $null
}
$summaryLookup = Get-SummaryLookup -Summary $summary

$entries = New-Object 'System.Collections.Generic.List[object]'
foreach ($entry in @(Get-ProjectTemplateSmokeArrayProperty -Object $manifest -Name "entries")) {
    $name = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "name"
    $summaryEntry = if ($summaryLookup.ContainsKey($name)) { $summaryLookup[$name] } else { $null }
    $templateValidations = @(Get-ProjectTemplateSmokeArrayProperty -Object $entry -Name "template_validations")
    $schemaValidation = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "schema_validation"
    $schemaBaseline = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "schema_baseline"
    $visualSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "visual_smoke"

    $entries.Add([pscustomobject]@{
        name = $name
        source_type = Get-ManifestEntrySourceType -Entry $entry
        input_docx = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "input_docx"
        input_docx_build_relative = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "input_docx_build_relative"
        resolved_input_docx = Resolve-ManifestEntryInputPath -RepoRoot $repoRoot -ResolvedBuildDir $resolvedBuildDir -Entry $entry
        prepare_sample_target = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "prepare_sample_target"
        prepare_argument = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "prepare_argument"
        template_validation_count = $templateValidations.Count
        schema_validation_enabled = ($null -ne $schemaValidation)
        schema_validation_schema_file = if ($null -eq $schemaValidation) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaValidation -Name "schema_file" }
        schema_validation_target_count = if ($null -eq $schemaValidation) { 0 } else { @(Get-ProjectTemplateSmokeArrayProperty -Object $schemaValidation -Name "targets").Count }
        schema_baseline_enabled = ($null -ne $schemaBaseline)
        schema_baseline_schema_file = if ($null -eq $schemaBaseline) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaBaseline -Name "schema_file" }
        schema_baseline_target_mode = if ($null -eq $schemaBaseline) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaBaseline -Name "target_mode" }
        schema_baseline_generated_output = if ($null -eq $schemaBaseline) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaBaseline -Name "generated_output" }
        visual_smoke_enabled = if ($visualSmoke -is [bool]) { [bool]$visualSmoke } elseif ($null -ne $visualSmoke) { [bool]$true } else { $false }
        visual_smoke_output_dir = if ($visualSmoke -isnot [bool] -and $null -ne $visualSmoke) { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "output_dir" } else { "" }
        latest_available = ($null -ne $summaryEntry)
        latest_status = if ($null -eq $summaryEntry) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summaryEntry -Name "status" }
        latest_passed = if ($null -eq $summaryEntry) { $null } else { [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $summaryEntry -Name "passed") }
        latest_manual_review_pending = if ($null -eq $summaryEntry) { $null } else { [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $summaryEntry -Name "manual_review_pending") }
        latest_artifact_dir = if ($null -eq $summaryEntry) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summaryEntry -Name "artifact_dir" }
        latest_visual_review_status = if ($null -eq $summaryEntry) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object (Get-ProjectTemplateSmokeOptionalPropertyObject -Object (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $summaryEntry -Name "checks") -Name "visual_smoke") -Name "review_status" }
        latest_visual_review_verdict = if ($null -eq $summaryEntry) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object (Get-ProjectTemplateSmokeOptionalPropertyObject -Object (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $summaryEntry -Name "checks") -Name "visual_smoke") -Name "review_verdict" }
        latest_visual_findings_count = if ($null -eq $summaryEntry) { $null } else {
            $visualSmokeSummary = Get-ProjectTemplateSmokeOptionalPropertyObject -Object (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $summaryEntry -Name "checks") -Name "visual_smoke"
            $visualFindingsCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmokeSummary -Name "findings_count"
            if ([string]::IsNullOrWhiteSpace($visualFindingsCount)) { $null } else { [int]$visualFindingsCount }
        }
        latest_contact_sheet = if ($null -eq $summaryEntry) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object (Get-ProjectTemplateSmokeOptionalPropertyObject -Object (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $summaryEntry -Name "checks") -Name "visual_smoke") -Name "contact_sheet" }
    }) | Out-Null
}

$report = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    manifest_path = $resolvedManifestPath
    summary_json = if ($summary) { $resolvedSummaryPath } else { "" }
    build_dir = $resolvedBuildDir
    entry_count = $entries.Count
    latest_overall_status = if ($summary) { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summary -Name "overall_status" } else { "" }
    latest_visual_verdict = if ($summary) { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summary -Name "visual_verdict" } else { "" }
    latest_passed = if ($summary) { [bool]$summary.passed } else { $null }
    latest_failed_entry_count = if ($summary) { [int]$summary.failed_entry_count } else { $null }
    latest_manual_review_pending_count = if ($summary) { [int]$summary.manual_review_pending_count } else { $null }
    latest_visual_review_undetermined_count = if ($summary) {
        $visualUndeterminedCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summary -Name "visual_review_undetermined_count"
        if ([string]::IsNullOrWhiteSpace($visualUndeterminedCount)) { $null } else { [int]$visualUndeterminedCount }
    } else { $null }
    entries = $entries.ToArray()
}

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
        Write-Host "[project-template-smoke-describe] Wrote JSON report to $resolvedOutputPath"
    }
    exit 0
}

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("Project template smoke manifest: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedManifestPath)")
[void]$lines.Add("Registered entries: $($entries.Count)")
if ($summary) {
    [void]$lines.Add("Latest summary: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedSummaryPath)")
    [void]$lines.Add("Latest status: $($report.latest_overall_status)")
    [void]$lines.Add("Latest visual verdict: $($report.latest_visual_verdict)")
    [void]$lines.Add("Latest passed: $($report.latest_passed)")
    [void]$lines.Add("Latest failed entries: $($report.latest_failed_entry_count)")
    [void]$lines.Add("Latest pending visual reviews: $($report.latest_manual_review_pending_count)")
    [void]$lines.Add("Latest undetermined visual reviews: $($report.latest_visual_review_undetermined_count)")
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
    if (-not [string]::IsNullOrWhiteSpace($entry.prepare_sample_target)) {
        [void]$lines.Add("  prepare_sample_target: $($entry.prepare_sample_target)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.prepare_argument)) {
        [void]$lines.Add("  prepare_argument: $($entry.prepare_argument)")
    }
    [void]$lines.Add("  template_validation_count: $($entry.template_validation_count)")
    [void]$lines.Add("  schema_validation_enabled: $($entry.schema_validation_enabled)")
    if (-not [string]::IsNullOrWhiteSpace($entry.schema_validation_schema_file)) {
        [void]$lines.Add("  schema_validation_schema_file: $($entry.schema_validation_schema_file)")
    }
    if ($entry.schema_validation_enabled) {
        [void]$lines.Add("  schema_validation_target_count: $($entry.schema_validation_target_count)")
    }
    [void]$lines.Add("  schema_baseline_enabled: $($entry.schema_baseline_enabled)")
    if (-not [string]::IsNullOrWhiteSpace($entry.schema_baseline_schema_file)) {
        [void]$lines.Add("  schema_baseline_schema_file: $($entry.schema_baseline_schema_file)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.schema_baseline_target_mode)) {
        [void]$lines.Add("  schema_baseline_target_mode: $($entry.schema_baseline_target_mode)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.schema_baseline_generated_output)) {
        [void]$lines.Add("  schema_baseline_generated_output: $($entry.schema_baseline_generated_output)")
    }
    [void]$lines.Add("  visual_smoke_enabled: $($entry.visual_smoke_enabled)")
    if (-not [string]::IsNullOrWhiteSpace($entry.visual_smoke_output_dir)) {
        [void]$lines.Add("  visual_smoke_output_dir: $($entry.visual_smoke_output_dir)")
    }
    if ($entry.latest_available) {
        [void]$lines.Add("  latest_status: $($entry.latest_status)")
        [void]$lines.Add("  latest_passed: $($entry.latest_passed)")
        [void]$lines.Add("  latest_manual_review_pending: $($entry.latest_manual_review_pending)")
        [void]$lines.Add("  latest_artifact_dir: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $entry.latest_artifact_dir)")
        if (-not [string]::IsNullOrWhiteSpace($entry.latest_visual_review_status)) {
            [void]$lines.Add("  latest_visual_review_status: $($entry.latest_visual_review_status)")
        }
        if (-not [string]::IsNullOrWhiteSpace($entry.latest_visual_review_verdict)) {
            [void]$lines.Add("  latest_visual_review_verdict: $($entry.latest_visual_review_verdict)")
        }
        if ($null -ne $entry.latest_visual_findings_count) {
            [void]$lines.Add("  latest_visual_findings_count: $($entry.latest_visual_findings_count)")
        }
        if (-not [string]::IsNullOrWhiteSpace($entry.latest_contact_sheet)) {
            [void]$lines.Add("  latest_contact_sheet: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $entry.latest_contact_sheet)")
        }
    } else {
        [void]$lines.Add("  latest_status: (not available)")
    }
    [void]$lines.Add("")
}

$text = $lines -join [Environment]::NewLine
if ([string]::IsNullOrWhiteSpace($resolvedOutputPath)) {
    Write-Output $text
} else {
    $directory = Split-Path -Parent $resolvedOutputPath
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
    $text | Set-Content -LiteralPath $resolvedOutputPath -Encoding UTF8
    Write-Host "[project-template-smoke-describe] Wrote text report to $resolvedOutputPath"
}
