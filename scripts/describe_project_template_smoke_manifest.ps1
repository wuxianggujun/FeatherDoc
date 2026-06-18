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

function New-ManifestDescriptionGroupSummary {
    param([object[]]$Items, [string]$PropertyName, [string]$OutputName)

    $values = @(
        foreach ($item in @($Items)) {
            $value = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name $PropertyName
            if ([string]::IsNullOrWhiteSpace($value)) { $value = "unknown" }
            [pscustomobject]@{ Value = $value }
        }
    )

    return @(
        foreach ($group in @($values | Group-Object Value |
            Sort-Object -Property @{ Expression = "Count"; Descending = $true }, @{ Expression = "Name"; Ascending = $true })) {
            $summary = [ordered]@{}
            $summary[$OutputName] = [string]$group.Name
            $summary["count"] = [int]$group.Count
            $summary
        }
    )
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

$businessTemplateCorpus = New-Object 'System.Collections.Generic.List[object]'
foreach ($item in @(Get-ProjectTemplateSmokeArrayProperty -Object $manifest -Name "business_template_corpus")) {
    $businessTemplateCorpus.Add([pscustomobject]@{
        id = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "id"
        project_id = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "project_id"
        template_name = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "template_name"
        document_type = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "document_type"
        status = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "status"
        source_entry = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "source_entry"
        registration_blocker = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "registration_blocker"
        next_action = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "next_action"
        smoke_contract = @(Get-ProjectTemplateSmokeArrayProperty -Object $item -Name "smoke_contract")
        coverage_goal = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "coverage_goal"
        notes = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "notes"
    }) | Out-Null
}
$plannedBusinessTemplateRegistrationActions = @(
    foreach ($item in @($businessTemplateCorpus.ToArray() | Where-Object { [string]$_.status -eq "planned" })) {
        [pscustomobject]@{
            id = [string]$item.id
            project_id = [string]$item.project_id
            template_name = [string]$item.template_name
            document_type = [string]$item.document_type
            registration_blocker = [string]$item.registration_blocker
            next_action = [string]$item.next_action
            smoke_contract = @($item.smoke_contract)
        }
    }
)

$entries = New-Object 'System.Collections.Generic.List[object]'
foreach ($entry in @(Get-ProjectTemplateSmokeArrayProperty -Object $manifest -Name "entries")) {
    $name = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "name"
    $summaryEntry = if ($summaryLookup.ContainsKey($name)) { $summaryLookup[$name] } else { $null }
    $templateValidations = @(Get-ProjectTemplateSmokeArrayProperty -Object $entry -Name "template_validations")
    $schemaValidation = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "schema_validation"
    $schemaBaseline = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "schema_baseline"
    $renderDataSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "render_data_smoke"
    $visualSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "visual_smoke"
    $latestChecks = if ($null -eq $summaryEntry) { $null } else { Get-ProjectTemplateSmokeOptionalPropertyObject -Object $summaryEntry -Name "checks" }
    $latestRenderData = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $latestChecks -Name "render_data"
    $latestVisualSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $latestChecks -Name "visual_smoke"

    $entries.Add([pscustomobject]@{
        name = $name
        project_id = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "project_id"
        template_name = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "template_name"
        business_domain = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "business_domain"
        business_document_type = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "business_document_type"
        corpus_role = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "corpus_role"
        corpus_source_note = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "corpus_source_note"
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
        render_data_enabled = ($null -ne $renderDataSmoke)
        render_data_path = if ($null -eq $renderDataSmoke) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $renderDataSmoke -Name "data_path" }
        render_data_mapping_path = if ($null -eq $renderDataSmoke) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $renderDataSmoke -Name "mapping_path" }
        render_data_output_docx = if ($null -eq $renderDataSmoke) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $renderDataSmoke -Name "output_docx" }
        visual_smoke_enabled = if ($visualSmoke -is [bool]) { [bool]$visualSmoke } elseif ($null -ne $visualSmoke) { [bool]$true } else { $false }
        visual_smoke_input = if ($visualSmoke -isnot [bool] -and $null -ne $visualSmoke) { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "input" } else { "" }
        visual_smoke_output_dir = if ($visualSmoke -isnot [bool] -and $null -ne $visualSmoke) { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "output_dir" } else { "" }
        latest_available = ($null -ne $summaryEntry)
        latest_status = if ($null -eq $summaryEntry) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summaryEntry -Name "status" }
        latest_passed = if ($null -eq $summaryEntry) { $null } else { [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $summaryEntry -Name "passed") }
        latest_manual_review_pending = if ($null -eq $summaryEntry) { $null } else { [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $summaryEntry -Name "manual_review_pending") }
        latest_artifact_dir = if ($null -eq $summaryEntry) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summaryEntry -Name "artifact_dir" }
        latest_render_data_status = if ($null -eq $latestRenderData) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $latestRenderData -Name "status" }
        latest_render_data_output_docx = if ($null -eq $latestRenderData) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $latestRenderData -Name "output_docx" }
        latest_render_data_remaining_placeholder_count = if ($null -eq $latestRenderData) { $null } else {
            $remainingPlaceholderCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $latestRenderData -Name "remaining_placeholder_count"
            if ([string]::IsNullOrWhiteSpace($remainingPlaceholderCount)) { $null } else { [int]$remainingPlaceholderCount }
        }
        latest_visual_review_status = if ($null -eq $latestVisualSmoke) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $latestVisualSmoke -Name "review_status" }
        latest_visual_review_verdict = if ($null -eq $latestVisualSmoke) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $latestVisualSmoke -Name "review_verdict" }
        latest_visual_findings_count = if ($null -eq $summaryEntry) { $null } else {
            $visualFindingsCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $latestVisualSmoke -Name "findings_count"
            if ([string]::IsNullOrWhiteSpace($visualFindingsCount)) { $null } else { [int]$visualFindingsCount }
        }
        latest_contact_sheet = if ($null -eq $latestVisualSmoke) { "" } else { Get-ProjectTemplateSmokeOptionalPropertyValue -Object $latestVisualSmoke -Name "contact_sheet" }
    }) | Out-Null
}

$report = [ordered]@{
    schema = "featherdoc.project_template_smoke_manifest_description.v1"
    generated_at = (Get-Date).ToString("s")
    manifest_path = $resolvedManifestPath
    manifest_path_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedManifestPath
    summary_json = if ($summary) { $resolvedSummaryPath } else { "" }
    summary_json_display = if ($summary) { Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedSummaryPath } else { "" }
    latest_summary_available = ($null -ne $summary)
    build_dir = $resolvedBuildDir
    entry_count = $entries.Count
    registered_entry_count = $entries.Count
    business_template_corpus_count = $businessTemplateCorpus.Count
    registered_business_template_corpus_count = @($businessTemplateCorpus.ToArray() | Where-Object { [string]$_.status -eq "registered" }).Count
    planned_business_template_corpus_count = @($businessTemplateCorpus.ToArray() | Where-Object { [string]$_.status -eq "planned" }).Count
    planned_business_template_registration_action_count = $plannedBusinessTemplateRegistrationActions.Count
    planned_business_template_registration_actions = @($plannedBusinessTemplateRegistrationActions)
    business_document_type_summary = @(New-ManifestDescriptionGroupSummary -Items $businessTemplateCorpus.ToArray() -PropertyName "document_type" -OutputName "document_type")
    business_template_corpus = @($businessTemplateCorpus.ToArray())
    schema_validation_entry_count = @($entries.ToArray() | Where-Object { [bool]$_.schema_validation_enabled }).Count
    schema_baseline_entry_count = @($entries.ToArray() | Where-Object { [bool]$_.schema_baseline_enabled }).Count
    render_data_entry_count = @($entries.ToArray() | Where-Object { [bool]$_.render_data_enabled }).Count
    visual_smoke_entry_count = @($entries.ToArray() | Where-Object { [bool]$_.visual_smoke_enabled }).Count
    latest_available_entry_count = @($entries.ToArray() | Where-Object { [bool]$_.latest_available }).Count
    latest_missing_entry_count = @($entries.ToArray() | Where-Object { -not [bool]$_.latest_available }).Count
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
[void]$lines.Add("Business template corpus: $($report.business_template_corpus_count)")
[void]$lines.Add("Registered business templates: $($report.registered_business_template_corpus_count)")
[void]$lines.Add("Planned business templates: $($report.planned_business_template_corpus_count)")
[void]$lines.Add("Planned registration actions: $($report.planned_business_template_registration_action_count)")
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
    if (-not [string]::IsNullOrWhiteSpace($entry.project_id)) {
        [void]$lines.Add("  project_id: $($entry.project_id)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.template_name)) {
        [void]$lines.Add("  template_name: $($entry.template_name)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.business_document_type)) {
        [void]$lines.Add("  business_document_type: $($entry.business_document_type)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.corpus_role)) {
        [void]$lines.Add("  corpus_role: $($entry.corpus_role)")
    }
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
    [void]$lines.Add("  render_data_enabled: $($entry.render_data_enabled)")
    if (-not [string]::IsNullOrWhiteSpace($entry.render_data_path)) {
        [void]$lines.Add("  render_data_path: $($entry.render_data_path)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.render_data_mapping_path)) {
        [void]$lines.Add("  render_data_mapping_path: $($entry.render_data_mapping_path)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.render_data_output_docx)) {
        [void]$lines.Add("  render_data_output_docx: $($entry.render_data_output_docx)")
    }
    [void]$lines.Add("  visual_smoke_enabled: $($entry.visual_smoke_enabled)")
    if (-not [string]::IsNullOrWhiteSpace($entry.visual_smoke_input)) {
        [void]$lines.Add("  visual_smoke_input: $($entry.visual_smoke_input)")
    }
    if (-not [string]::IsNullOrWhiteSpace($entry.visual_smoke_output_dir)) {
        [void]$lines.Add("  visual_smoke_output_dir: $($entry.visual_smoke_output_dir)")
    }
    if ($entry.latest_available) {
        [void]$lines.Add("  latest_status: $($entry.latest_status)")
        [void]$lines.Add("  latest_passed: $($entry.latest_passed)")
        [void]$lines.Add("  latest_manual_review_pending: $($entry.latest_manual_review_pending)")
        [void]$lines.Add("  latest_artifact_dir: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $entry.latest_artifact_dir)")
        if (-not [string]::IsNullOrWhiteSpace($entry.latest_render_data_status)) {
            [void]$lines.Add("  latest_render_data_status: $($entry.latest_render_data_status)")
        }
        if (-not [string]::IsNullOrWhiteSpace($entry.latest_render_data_output_docx)) {
            [void]$lines.Add("  latest_render_data_output_docx: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $entry.latest_render_data_output_docx)")
        }
        if ($null -ne $entry.latest_render_data_remaining_placeholder_count) {
            [void]$lines.Add("  latest_render_data_remaining_placeholder_count: $($entry.latest_render_data_remaining_placeholder_count)")
        }
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

if ($businessTemplateCorpus.Count -gt 0) {
    [void]$lines.Add("Business template corpus:")
    foreach ($item in $businessTemplateCorpus) {
        [void]$lines.Add("- $($item.id)")
        [void]$lines.Add("  project_id: $($item.project_id)")
        [void]$lines.Add("  template_name: $($item.template_name)")
        [void]$lines.Add("  document_type: $($item.document_type)")
        [void]$lines.Add("  status: $($item.status)")
        if (-not [string]::IsNullOrWhiteSpace($item.source_entry)) {
            [void]$lines.Add("  source_entry: $($item.source_entry)")
        }
        if (-not [string]::IsNullOrWhiteSpace($item.registration_blocker)) {
            [void]$lines.Add("  registration_blocker: $($item.registration_blocker)")
        }
        if (-not [string]::IsNullOrWhiteSpace($item.next_action)) {
            [void]$lines.Add("  next_action: $($item.next_action)")
        }
        [void]$lines.Add("  smoke_contract: $((@($item.smoke_contract) -join ', '))")
        [void]$lines.Add("  coverage_goal: $($item.coverage_goal)")
    }
    [void]$lines.Add("")
}

if ($plannedBusinessTemplateRegistrationActions.Count -gt 0) {
    [void]$lines.Add("Planned business template registration actions:")
    foreach ($action in $plannedBusinessTemplateRegistrationActions) {
        [void]$lines.Add("- $($action.id)")
        [void]$lines.Add("  project_id: $($action.project_id)")
        [void]$lines.Add("  template_name: $($action.template_name)")
        [void]$lines.Add("  document_type: $($action.document_type)")
        [void]$lines.Add("  registration_blocker: $($action.registration_blocker)")
        [void]$lines.Add("  next_action: $($action.next_action)")
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
