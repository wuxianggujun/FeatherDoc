<#
.SYNOPSIS
Creates an onboarding plan for unregistered project-template-smoke DOCX/DOTX candidates.

.DESCRIPTION
Runs the repository template-candidate discovery pass, then writes a JSON and
Markdown plan with ready-to-run commands for freezing schema baselines,
registering project-template-smoke manifest entries, running the smoke harness,
and enforcing strict release-preflight coverage.

The script is intentionally non-mutating for the manifest and schema baselines:
it only writes plan artifacts under OutputDir. Review the generated plan, then
run the listed commands when each template is ready to become a real regression
target.
#>
param(
    [string]$ManifestPath = "samples/project_template_smoke.manifest.json",
    [string]$SearchRoot = ".",
    [string]$BuildDir = "",
    [string]$OutputDir = "output/project-template-smoke-onboarding-plan",
    [string]$SchemaBaselineDir = "baselines/template-schema",
    [string]$VisualSmokeOutputRoot = "output/project-template-smoke",
    [string]$RenderDataWorkspaceRoot = "",
    [ValidateSet("default", "section-targets", "resolved-section-targets")]
    [string]$SchemaTargetMode = "default",
    [switch]$IncludeGenerated
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "project_template_smoke_manifest_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-smoke-onboard] $Message"
}

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
        return ""
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

function Quote-CommandValue {
    param([string]$Value)

    return '"' + ($Value -replace '"', '\"') + '"'
}

function Get-CommandPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    $displayPath = Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $Path
    if ([string]::IsNullOrWhiteSpace($displayPath)) {
        return ""
    }

    return $displayPath
}

function Join-Command {
    param([string[]]$Parts)

    return ($Parts | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }) -join " "
}

function Get-FreezeSchemaModeFlag {
    param([string]$Mode)

    switch ($Mode) {
        "section-targets" { return "-SectionTargets" }
        "resolved-section-targets" { return "-ResolvedSectionTargets" }
        default { return "" }
    }
}

function New-FreezeSchemaCommand {
    param(
        [string]$RepoRoot,
        [string]$InputDocx,
        [string]$SchemaOutput,
        [string]$BuildDir,
        [string]$SchemaTargetMode
    )

    $parts = New-Object 'System.Collections.Generic.List[string]'
    $parts.Add("pwsh") | Out-Null
    $parts.Add("-ExecutionPolicy Bypass") | Out-Null
    $parts.Add("-File .\scripts\freeze_template_schema_baseline.ps1") | Out-Null
    $parts.Add("-InputDocx") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $InputDocx))) | Out-Null
    $parts.Add("-SchemaOutput") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $SchemaOutput))) | Out-Null
    if (-not [string]::IsNullOrWhiteSpace($BuildDir)) {
        $parts.Add("-SkipBuild") | Out-Null
        $parts.Add("-BuildDir") | Out-Null
        $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $BuildDir))) | Out-Null
    }
    $modeFlag = Get-FreezeSchemaModeFlag -Mode $SchemaTargetMode
    if (-not [string]::IsNullOrWhiteSpace($modeFlag)) {
        $parts.Add($modeFlag) | Out-Null
    }

    return Join-Command -Parts $parts.ToArray()
}

function New-RegisterCommand {
    param(
        [string]$RepoRoot,
        [string]$Name,
        [string]$ManifestPath,
        [string]$InputDocx,
        [string]$SchemaFile,
        [string]$GeneratedSchemaOutput,
        [string]$VisualSmokeOutputDir,
        [string]$SchemaTargetMode
    )

    $parts = New-Object 'System.Collections.Generic.List[string]'
    $parts.Add("pwsh") | Out-Null
    $parts.Add("-ExecutionPolicy Bypass") | Out-Null
    $parts.Add("-File .\scripts\register_project_template_smoke_manifest_entry.ps1") | Out-Null
    $parts.Add("-Name") | Out-Null
    $parts.Add((Quote-CommandValue -Value $Name)) | Out-Null
    $parts.Add("-ManifestPath") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $ManifestPath))) | Out-Null
    $parts.Add("-InputDocx") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $InputDocx))) | Out-Null
    $parts.Add("-SchemaValidationFile") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $SchemaFile))) | Out-Null
    $parts.Add("-SchemaBaselineFile") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $SchemaFile))) | Out-Null
    $parts.Add("-SchemaBaselineTargetMode") | Out-Null
    $parts.Add((Quote-CommandValue -Value $SchemaTargetMode)) | Out-Null
    $parts.Add("-SchemaBaselineGeneratedOutput") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $GeneratedSchemaOutput))) | Out-Null
    $parts.Add("-VisualSmokeOutputDir") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $VisualSmokeOutputDir))) | Out-Null
    $parts.Add("-ReplaceExisting") | Out-Null

    return Join-Command -Parts $parts.ToArray()
}

function New-PrepareRenderDataWorkspaceCommand {
    param(
        [string]$RepoRoot,
        [string]$InputDocx,
        [string]$WorkspaceDir,
        [string]$SummaryJson,
        [string]$BuildDir
    )

    $parts = New-Object 'System.Collections.Generic.List[string]'
    $parts.Add("pwsh") | Out-Null
    $parts.Add("-ExecutionPolicy Bypass") | Out-Null
    $parts.Add("-File .\scripts\prepare_template_render_data_workspace.ps1") | Out-Null
    $parts.Add("-InputDocx") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $InputDocx))) | Out-Null
    $parts.Add("-WorkspaceDir") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $WorkspaceDir))) | Out-Null
    $parts.Add("-SummaryJson") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $SummaryJson))) | Out-Null
    if (-not [string]::IsNullOrWhiteSpace($BuildDir)) {
        $parts.Add("-BuildDir") | Out-Null
        $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $BuildDir))) | Out-Null
        $parts.Add("-SkipBuild") | Out-Null
    }

    return Join-Command -Parts $parts.ToArray()
}

function New-ValidateRenderDataWorkspaceCommand {
    param(
        [string]$RepoRoot,
        [string]$WorkspaceDir,
        [string]$SummaryJson,
        [string]$ReportMarkdown,
        [string]$BuildDir
    )

    $parts = New-Object 'System.Collections.Generic.List[string]'
    $parts.Add("pwsh") | Out-Null
    $parts.Add("-ExecutionPolicy Bypass") | Out-Null
    $parts.Add("-File .\scripts\validate_render_data_mapping.ps1") | Out-Null
    $parts.Add("-WorkspaceDir") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $WorkspaceDir))) | Out-Null
    $parts.Add("-SummaryJson") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $SummaryJson))) | Out-Null
    $parts.Add("-ReportMarkdown") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $ReportMarkdown))) | Out-Null
    if (-not [string]::IsNullOrWhiteSpace($BuildDir)) {
        $parts.Add("-BuildDir") | Out-Null
        $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $BuildDir))) | Out-Null
        $parts.Add("-SkipBuild") | Out-Null
    }
    $parts.Add("-RequireComplete") | Out-Null

    return Join-Command -Parts $parts.ToArray()
}

function New-CheckManifestCommand {
    param(
        [string]$RepoRoot,
        [string]$ManifestPath,
        [string]$BuildDir
    )

    $parts = New-Object 'System.Collections.Generic.List[string]'
    $parts.Add("pwsh") | Out-Null
    $parts.Add("-ExecutionPolicy Bypass") | Out-Null
    $parts.Add("-File .\scripts\check_project_template_smoke_manifest.ps1") | Out-Null
    $parts.Add("-ManifestPath") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $ManifestPath))) | Out-Null
    if (-not [string]::IsNullOrWhiteSpace($BuildDir)) {
        $parts.Add("-BuildDir") | Out-Null
        $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $BuildDir))) | Out-Null
    }
    $parts.Add("-CheckPaths") | Out-Null

    return Join-Command -Parts $parts.ToArray()
}

function New-RunSmokeCommand {
    param(
        [string]$RepoRoot,
        [string]$ManifestPath,
        [string]$BuildDir,
        [string]$OutputDir
    )

    $smokeOutputDir = Join-Path $OutputDir "smoke"
    $parts = New-Object 'System.Collections.Generic.List[string]'
    $parts.Add("pwsh") | Out-Null
    $parts.Add("-ExecutionPolicy Bypass") | Out-Null
    $parts.Add("-File .\scripts\run_project_template_smoke.ps1") | Out-Null
    $parts.Add("-ManifestPath") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $ManifestPath))) | Out-Null
    if (-not [string]::IsNullOrWhiteSpace($BuildDir)) {
        $parts.Add("-BuildDir") | Out-Null
        $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $BuildDir))) | Out-Null
        $parts.Add("-SkipBuild") | Out-Null
    }
    $parts.Add("-OutputDir") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $smokeOutputDir))) | Out-Null

    return Join-Command -Parts $parts.ToArray()
}

function New-StrictPreflightCommand {
    param(
        [string]$RepoRoot,
        [string]$ManifestPath,
        [string]$BuildDir
    )

    $parts = New-Object 'System.Collections.Generic.List[string]'
    $parts.Add("pwsh") | Out-Null
    $parts.Add("-ExecutionPolicy Bypass") | Out-Null
    $parts.Add("-File .\scripts\run_release_candidate_checks.ps1") | Out-Null
    $parts.Add("-ProjectTemplateSmokeManifestPath") | Out-Null
    $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $ManifestPath))) | Out-Null
    $parts.Add("-ProjectTemplateSmokeRequireFullCoverage") | Out-Null
    if (-not [string]::IsNullOrWhiteSpace($BuildDir)) {
        $parts.Add("-BuildDir") | Out-Null
        $parts.Add((Quote-CommandValue -Value (Get-CommandPath -RepoRoot $RepoRoot -Path $BuildDir))) | Out-Null
    }

    return Join-Command -Parts $parts.ToArray()
}

function New-PlannedSchemaApprovalState {
    return [ordered]@{
        schema = "featherdoc.project_template_onboarding_schema_approval_state.v1"
        status = "not_evaluated"
        gate_status = "not_evaluated"
        release_blocked = $true
        smoke_summary_available = $false
        smoke_skipped = $false
        approval_required = $null
        review_count = 0
        changed_review_count = 0
        pending_count = 0
        approved_count = 0
        rejected_count = 0
        compliance_issue_count = 0
        invalid_result_count = 0
        action = "run_project_template_smoke_then_review_schema_patch_approval"
    }
}

function New-PlannedReleaseBlockers {
    param([string]$Name)

    return @(
        [ordered]@{
            id = "project_template_onboarding.schema_approval_not_evaluated"
            source = "project_template_smoke_onboarding_plan"
            severity = "error"
            status = "not_evaluated"
            message = "Schema approval cannot be release-ready until the candidate is frozen, registered, smoked, and reviewed."
            action = "run_project_template_smoke_then_review_schema_patch_approval"
            entry_name = $Name
        }
    )
}

function New-PlannedActionItems {
    param(
        [string]$Name,
        [object]$Commands,
        [string]$RenderDataValidationReport
    )

    return @(
        [ordered]@{
            id = "freeze_schema_baseline"
            status = "open"
            action = "freeze_schema_baseline"
            title = "Freeze the first schema baseline candidate."
            command = [string]$Commands.freeze_schema_baseline
        },
        [ordered]@{
            id = "complete_render_data_workspace"
            status = "open"
            action = "complete_render_data_workspace"
            title = "Prepare and complete the render-data workspace."
            command = [string]$Commands.prepare_render_data_workspace
            artifacts = @($RenderDataValidationReport)
        },
        [ordered]@{
            id = "review_schema_update_candidate"
            status = "open"
            action = "review_schema_update_candidate"
            title = "Run smoke after manifest registration and review schema_patch_approval_items."
            command = [string]$Commands.register_manifest_entry
        }
    )
}

function New-PlannedManualReviewRecommendations {
    param(
        [string]$Name,
        [string]$SchemaBaselinePath,
        [string]$RenderDataValidationReport
    )

    return @(
        [ordered]@{
            id = "review_schema_baseline_$Name"
            priority = "high"
            title = "Review whether the first schema baseline only contains expected bookmarks and content controls."
            artifact = $SchemaBaselinePath
            action = "review_schema_baseline"
        },
        [ordered]@{
            id = "review_render_data_business_values_$Name"
            priority = "high"
            title = "Complete business data and run strict validation."
            artifact = $RenderDataValidationReport
            action = "complete_render_data_workspace"
        },
        [ordered]@{
            id = "review_schema_approval_after_smoke_$Name"
            priority = "high"
            title = "Review approval decisions after smoke generates schema_patch_approval_items."
            artifact = ""
            action = "run_project_template_smoke_then_review_schema_patch_approval"
        }
    )
}

$repoRoot = Resolve-RepoRoot
$resolvedManifestPath = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $ManifestPath
$resolvedSearchRoot = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SearchRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir -AllowMissing
$resolvedSchemaBaselineDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SchemaBaselineDir -AllowMissing
$resolvedVisualSmokeOutputRoot = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $VisualSmokeOutputRoot -AllowMissing
$resolvedRenderDataWorkspaceRoot = if ([string]::IsNullOrWhiteSpace($RenderDataWorkspaceRoot)) {
    Join-Path $resolvedOutputDir "render-data-workspaces"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $RenderDataWorkspaceRoot -AllowMissing
}
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
}

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$candidateDiscoveryPath = Join-Path $resolvedOutputDir "candidate_discovery.json"
$planJsonPath = Join-Path $resolvedOutputDir "plan.json"
$planMarkdownPath = Join-Path $resolvedOutputDir "plan.md"
$discoverScript = Join-Path $repoRoot "scripts\discover_project_template_smoke_candidates.ps1"

$discoverArgs = @(
    "-ManifestPath"
    $resolvedManifestPath
    "-SearchRoot"
    $resolvedSearchRoot
    "-OutputPath"
    $candidateDiscoveryPath
    "-Json"
    "-IncludeRegistered"
    "-IncludeExcluded"
)
if (-not [string]::IsNullOrWhiteSpace($resolvedBuildDir)) {
    $discoverArgs += @("-BuildDir", $resolvedBuildDir)
}
if ($IncludeGenerated) {
    $discoverArgs += "-IncludeGenerated"
}

Write-Step "Discovering template candidates"
$discoverOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $discoverScript @discoverArgs 2>&1)
$discoverExitCode = $LASTEXITCODE
foreach ($line in $discoverOutput) {
    Write-Host $line
}
if ($discoverExitCode -ne 0) {
    throw "Template candidate discovery failed."
}

$candidateDiscovery = Get-Content -Raw -LiteralPath $candidateDiscoveryPath | ConvertFrom-Json
$unregisteredCandidates = @(
    $candidateDiscovery.candidates |
        Where-Object { -not $_.registered -and -not $_.excluded }
)

$entries = New-Object 'System.Collections.Generic.List[object]'
foreach ($candidate in $unregisteredCandidates) {
    $entryName = [string]$candidate.suggested_name
    if ([string]::IsNullOrWhiteSpace($entryName)) {
        $entryName = [System.IO.Path]::GetFileNameWithoutExtension([string]$candidate.file_name)
    }

    $inputDocx = [string]$candidate.path
    $schemaBaselinePath = Join-Path $resolvedSchemaBaselineDir ("{0}.schema.json" -f $entryName)
    $generatedSchemaOutput = Join-Path $resolvedOutputDir ("{0}.generated.schema.json" -f $entryName)
    $visualSmokeOutputDir = Join-Path $resolvedVisualSmokeOutputRoot ("{0}-visual" -f $entryName)
    $renderDataWorkspaceDir = Join-Path $resolvedRenderDataWorkspaceRoot $entryName
    $renderDataWorkspaceSummary = Join-Path $renderDataWorkspaceDir "summary.json"
    $renderDataValidationSummary = Join-Path $renderDataWorkspaceDir "validation.summary.json"
    $renderDataValidationReport = Join-Path $renderDataWorkspaceDir "validation.report.md"
    $entryCommands = [ordered]@{
        freeze_schema_baseline = New-FreezeSchemaCommand `
            -RepoRoot $repoRoot `
            -InputDocx $inputDocx `
            -SchemaOutput $schemaBaselinePath `
            -BuildDir $resolvedBuildDir `
            -SchemaTargetMode $SchemaTargetMode
        register_manifest_entry = New-RegisterCommand `
            -RepoRoot $repoRoot `
            -Name $entryName `
            -ManifestPath $resolvedManifestPath `
            -InputDocx $inputDocx `
            -SchemaFile $schemaBaselinePath `
            -GeneratedSchemaOutput $generatedSchemaOutput `
            -VisualSmokeOutputDir $visualSmokeOutputDir `
            -SchemaTargetMode $SchemaTargetMode
        prepare_render_data_workspace = New-PrepareRenderDataWorkspaceCommand `
            -RepoRoot $repoRoot `
            -InputDocx $inputDocx `
            -WorkspaceDir $renderDataWorkspaceDir `
            -SummaryJson $renderDataWorkspaceSummary `
            -BuildDir $resolvedBuildDir
        validate_render_data_workspace = New-ValidateRenderDataWorkspaceCommand `
            -RepoRoot $repoRoot `
            -WorkspaceDir $renderDataWorkspaceDir `
            -SummaryJson $renderDataValidationSummary `
            -ReportMarkdown $renderDataValidationReport `
            -BuildDir $resolvedBuildDir
    }
    $schemaApprovalState = New-PlannedSchemaApprovalState
    $releaseBlockers = New-PlannedReleaseBlockers -Name $entryName
    $actionItems = New-PlannedActionItems `
        -Name $entryName `
        -Commands $entryCommands `
        -RenderDataValidationReport $renderDataValidationReport
    $manualReviewRecommendations = New-PlannedManualReviewRecommendations `
        -Name $entryName `
        -SchemaBaselinePath $schemaBaselinePath `
        -RenderDataValidationReport $renderDataValidationReport

    $entries.Add([pscustomobject]@{
        name = $entryName
        input_docx = $inputDocx
        input_docx_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $inputDocx
        schema_baseline = $schemaBaselinePath
        schema_baseline_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $schemaBaselinePath
        generated_schema_output = $generatedSchemaOutput
        generated_schema_output_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $generatedSchemaOutput
        visual_smoke_output_dir = $visualSmokeOutputDir
        visual_smoke_output_dir_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $visualSmokeOutputDir
        render_data_workspace_dir = $renderDataWorkspaceDir
        render_data_workspace_dir_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $renderDataWorkspaceDir
        render_data_workspace_summary = $renderDataWorkspaceSummary
        render_data_workspace_summary_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $renderDataWorkspaceSummary
        render_data_validation_summary = $renderDataValidationSummary
        render_data_validation_summary_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $renderDataValidationSummary
        render_data_validation_report = $renderDataValidationReport
        render_data_validation_report_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $renderDataValidationReport
        schema_approval_state = $schemaApprovalState
        release_blockers = @($releaseBlockers)
        release_blocker_count = @($releaseBlockers).Count
        action_items = @($actionItems)
        manual_review_recommendations = @($manualReviewRecommendations)
        review_checklist = @(
            "Run freeze_schema_baseline and review the generated schema for unexpected, duplicate, or malformed bookmarks.",
            "Run prepare_render_data_workspace and fill the generated data skeleton with real business values.",
            "Run validate_render_data_workspace until remaining_placeholder_count is 0.",
            "Run project-template-smoke after registration and resolve every schema_patch_approval_item before release.",
            "Register the manifest entry, then run project-template-smoke and strict release preflight."
        )
        commands = $entryCommands
    }) | Out-Null
}

$plan = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    manifest_path = $resolvedManifestPath
    manifest_path_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedManifestPath
    search_root = $resolvedSearchRoot
    search_root_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedSearchRoot
    build_dir = $resolvedBuildDir
    build_dir_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedBuildDir
    output_dir = $resolvedOutputDir
    output_dir_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    schema_baseline_dir = $resolvedSchemaBaselineDir
    schema_baseline_dir_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedSchemaBaselineDir
    visual_smoke_output_root = $resolvedVisualSmokeOutputRoot
    visual_smoke_output_root_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedVisualSmokeOutputRoot
    render_data_workspace_root = $resolvedRenderDataWorkspaceRoot
    render_data_workspace_root_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedRenderDataWorkspaceRoot
    schema_target_mode = $SchemaTargetMode
    candidate_discovery_json = $candidateDiscoveryPath
    candidate_discovery_json_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $candidateDiscoveryPath
    registered_candidate_count = [int]$candidateDiscovery.registered_candidate_count
    unregistered_candidate_count = [int]$candidateDiscovery.unregistered_candidate_count
    excluded_candidate_count = [int]$candidateDiscovery.excluded_candidate_count
    onboarding_entry_count = $entries.Count
    entries = $entries.ToArray()
    commands = [ordered]@{
        check_manifest = New-CheckManifestCommand -RepoRoot $repoRoot -ManifestPath $resolvedManifestPath -BuildDir $resolvedBuildDir
        run_project_template_smoke = New-RunSmokeCommand -RepoRoot $repoRoot -ManifestPath $resolvedManifestPath -BuildDir $resolvedBuildDir -OutputDir $resolvedOutputDir
        strict_release_preflight = New-StrictPreflightCommand -RepoRoot $repoRoot -ManifestPath $resolvedManifestPath -BuildDir $resolvedBuildDir
    }
}

($plan | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $planJsonPath -Encoding UTF8

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("# Project Template Smoke Onboarding Plan")
[void]$lines.Add("")
[void]$lines.Add("- Generated at: $($plan.generated_at)")
[void]$lines.Add("- Manifest: $($plan.manifest_path_display)")
[void]$lines.Add("- Search root: $($plan.search_root_display)")
[void]$lines.Add("- Candidate discovery JSON: $($plan.candidate_discovery_json_display)")
[void]$lines.Add("- Registered / unregistered / excluded candidates: $($plan.registered_candidate_count)/$($plan.unregistered_candidate_count)/$($plan.excluded_candidate_count)")
[void]$lines.Add("- Onboarding entries to add: $($plan.onboarding_entry_count)")
[void]$lines.Add("- Schema target mode: $($plan.schema_target_mode)")
[void]$lines.Add("- Render-data workspace root: $($plan.render_data_workspace_root_display)")
[void]$lines.Add("")
[void]$lines.Add("## Entry Commands")
[void]$lines.Add("")
if ($entries.Count -eq 0) {
    [void]$lines.Add("- No unregistered DOCX/DOTX candidates require onboarding.")
    [void]$lines.Add("")
} else {
    foreach ($entry in $entries) {
        [void]$lines.Add("### $($entry.name)")
        [void]$lines.Add("")
        [void]$lines.Add("- Input DOCX: $($entry.input_docx_display)")
        [void]$lines.Add("- Schema baseline: $($entry.schema_baseline_display)")
        [void]$lines.Add("- Generated schema output: $($entry.generated_schema_output_display)")
        [void]$lines.Add("- Render-data workspace: $($entry.render_data_workspace_dir_display)")
        [void]$lines.Add("- Render-data validation report: $($entry.render_data_validation_report_display)")
        [void]$lines.Add("- Visual smoke output dir: $($entry.visual_smoke_output_dir_display)")
        [void]$lines.Add("- Schema approval status: $($entry.schema_approval_state.status)")
        [void]$lines.Add("- Schema approval action: $($entry.schema_approval_state.action)")
        [void]$lines.Add("- Release blockers: $($entry.release_blocker_count)")
        [void]$lines.Add("")
        [void]$lines.Add("Release blockers:")
        foreach ($blocker in @($entry.release_blockers)) {
            [void]$lines.Add("- $($blocker.id): status=$($blocker.status) action=$($blocker.action)")
        }
        [void]$lines.Add("")
        [void]$lines.Add("Action items:")
        foreach ($item in @($entry.action_items)) {
            [void]$lines.Add("- $($item.id): action=$($item.action)")
        }
        [void]$lines.Add("")
        [void]$lines.Add("Manual review recommendations:")
        foreach ($recommendation in @($entry.manual_review_recommendations)) {
            [void]$lines.Add("- $($recommendation.id): priority=$($recommendation.priority) action=$($recommendation.action)")
        }
        [void]$lines.Add("")
        [void]$lines.Add("Review checklist:")
        foreach ($item in @($entry.review_checklist)) {
            [void]$lines.Add("- $item")
        }
        [void]$lines.Add("")
        [void]$lines.Add('```powershell')
        [void]$lines.Add($entry.commands.freeze_schema_baseline)
        [void]$lines.Add($entry.commands.prepare_render_data_workspace)
        [void]$lines.Add($entry.commands.validate_render_data_workspace)
        [void]$lines.Add($entry.commands.register_manifest_entry)
        [void]$lines.Add('```')
        [void]$lines.Add("")
    }
}

[void]$lines.Add("## Final Checks")
[void]$lines.Add("")
[void]$lines.Add("Run these after reviewing and executing the entry commands above:")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($plan.commands.check_manifest)
[void]$lines.Add($plan.commands.run_project_template_smoke)
[void]$lines.Add($plan.commands.strict_release_preflight)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add('If a tracked DOCX/DOTX file is intentionally not a project template, add it to `candidate_exclusions` in the manifest instead of registering it.')

($lines -join [Environment]::NewLine) | Set-Content -LiteralPath $planMarkdownPath -Encoding UTF8

Write-Step "Plan JSON: $planJsonPath"
Write-Step "Plan Markdown: $planMarkdownPath"
Write-Step "Onboarding entries: $($entries.Count)"
