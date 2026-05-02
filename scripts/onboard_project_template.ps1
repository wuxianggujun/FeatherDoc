<#
.SYNOPSIS
Runs a one-stop onboarding workflow for a project DOCX/DOTX template.

.DESCRIPTION
Creates a reviewable onboarding bundle for one real project template: schema
baseline candidate, temporary project-template-smoke manifest, render-data
workspace, validation report, optional render output, and handoff files.

The default workflow is non-destructive for committed manifests. Pass
`-RegisterManifest` only after reviewing the generated bundle.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [string]$TemplateName = "",
    [string]$OutputDir = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [ValidateSet("default", "section-targets", "resolved-section-targets")]
    [string]$SchemaTargetMode = "default",
    [string]$ManifestPath = "samples/project_template_smoke.manifest.json",
    [string]$SummaryJson = "",
    [switch]$PlanOnly,
    [switch]$SkipBuild,
    [switch]$SkipProjectTemplateSmoke,
    [switch]$IncludeVisualSmoke,
    [switch]$RequireComplete,
    [switch]$RegisterManifest,
    [switch]$ReplaceExisting
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step { param([string]$Message) Write-Host "[onboard-project-template] $Message" }

function Resolve-RepoRoot { return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path }

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$Path, [switch]$MustExist)
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    $fullPath = [System.IO.Path]::GetFullPath($candidate)
    if ($MustExist -and -not (Test-Path -LiteralPath $fullPath)) { throw "Path does not exist: $Path" }
    return $fullPath
}

function Ensure-Directory { param([string]$Path) if (-not [string]::IsNullOrWhiteSpace($Path)) { New-Item -ItemType Directory -Path $Path -Force | Out-Null } }

function Ensure-PathParent {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return }
    Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($Path))
}

function Get-SafeName {
    param([string]$Name)
    $safe = $Name.Trim().ToLowerInvariant() -replace '[^a-z0-9._-]+', '-'
    $safe = $safe.Trim('-')
    if ([string]::IsNullOrWhiteSpace($safe)) { return "project-template" }
    return $safe
}

function Get-DisplayPath {
    param([string]$RepoRoot, [string]$Path, [switch]$Json)
    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $root = [System.IO.Path]::GetFullPath($RepoRoot)
    if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and -not $root.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) { $root += [System.IO.Path]::DirectorySeparatorChar }
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if ($fullPath.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $fullPath.Substring($root.Length)
        if ($Json) { return "./" + ($relative -replace '\\', '/') }
        return ".\" + ($relative -replace '/', '\')
    }
    return $fullPath
}

function Quote-Value { param([string]$Value) return "'" + ($Value -replace "'", "''") + "'" }

function New-CommandLine {
    param([string]$Script, [System.Collections.IDictionary]$Arguments, [string[]]$Switches = @())
    $parts = New-Object 'System.Collections.Generic.List[string]'
    $parts.Add("pwsh") | Out-Null
    $parts.Add("-ExecutionPolicy Bypass") | Out-Null
    $parts.Add("-File $Script") | Out-Null
    foreach ($entry in $Arguments.GetEnumerator()) {
        if ([string]::IsNullOrWhiteSpace([string]$entry.Value)) { continue }
        $parts.Add("-$($entry.Key)") | Out-Null
        $parts.Add((Quote-Value -Value ([string]$entry.Value))) | Out-Null
    }
    foreach ($switchName in $Switches) { if (-not [string]::IsNullOrWhiteSpace($switchName)) { $parts.Add("-$switchName") | Out-Null } }
    return ($parts -join " ")
}

function New-Step {
    param([string]$Name, [string]$Status, [string]$Command, [string]$Message = "")
    $step = [ordered]@{ name = $Name; status = $Status; command = $Command }
    if (-not [string]::IsNullOrWhiteSpace($Message)) { $step.message = $Message }
    return $step
}

function Invoke-StepScript {
    param([string]$Name, [string]$Command, [string]$ScriptPath, [hashtable]$Arguments, [bool]$PlanOnly)
    if ($PlanOnly) { return New-Step -Name $Name -Status "planned" -Command $Command }
    $step = [ordered]@{ name = $Name; status = "running"; command = $Command; started_at = (Get-Date).ToString("s"); completed_at = ""; exit_code = $null; error = "" }
    try {
        $global:LASTEXITCODE = 0
        & $ScriptPath @Arguments
        $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { [int]$LASTEXITCODE }
        $step.exit_code = $exitCode
        if ($exitCode -ne 0) { throw "$Name failed with exit code $exitCode." }
        $step.status = "completed"
    } catch {
        $step.status = "failed"
        $step.error = $_.Exception.Message
    } finally {
        $step.completed_at = (Get-Date).ToString("s")
    }
    return $step
}

function Read-JsonIfPresent {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) { return $null }
    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Get-JsonString {
    param($Object, [string]$Name)
    if ($null -eq $Object) { return "" }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) { return "" }
    return [string]$property.Value
}

function Get-JsonInt {
    param($Object, [string]$Name)
    $text = Get-JsonString -Object $Object -Name $Name
    $value = 0
    if ([int]::TryParse($text, [ref]$value)) { return $value }
    return 0
}

function Get-JsonArray {
    param($Object, [string]$Name)
    if ($null -eq $Object) { return @() }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) { return @() }
    return @($property.Value | Where-Object { $null -ne $_ })
}

function Write-TempSmokeManifest {
    param([string]$RepoRoot, [string]$Path, [string]$Name, [string]$InputDocx, [string]$SchemaFile, [string]$TargetMode, [string]$GeneratedSchema, [string]$VisualOutputDir, [bool]$VisualEnabled)
    $entry = [ordered]@{
        name = $Name
        input_docx = Get-DisplayPath -RepoRoot $RepoRoot -Path $InputDocx -Json
        schema_validation = [ordered]@{ schema_file = Get-DisplayPath -RepoRoot $RepoRoot -Path $SchemaFile -Json }
        schema_baseline = [ordered]@{
            schema_file = Get-DisplayPath -RepoRoot $RepoRoot -Path $SchemaFile -Json
            target_mode = $TargetMode
            generated_output = Get-DisplayPath -RepoRoot $RepoRoot -Path $GeneratedSchema -Json
        }
        visual_smoke = [ordered]@{ enabled = $VisualEnabled; output_dir = Get-DisplayPath -RepoRoot $RepoRoot -Path $VisualOutputDir -Json }
    }
    $manifest = [ordered]@{ '$schema' = "./project_template_smoke.manifest.schema.json"; candidate_exclusions = @(); entries = @($entry) }
    Ensure-PathParent -Path $Path
    ($manifest | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Write-Lines {
    param([string]$Path, [object[]]$Lines)
    Ensure-PathParent -Path $Path
    (($Lines | ForEach-Object { [string]$_ }) -join [Environment]::NewLine) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-StartHere {
    param([object]$Summary, [string]$StrictValidationCommand, [string]$RenderCommand, [string]$RegisterCommand)
    return @(
        "# Project Template Onboarding",
        "",
        "这份工作台用于把单个业务模板接入 schema、render-data 和 smoke 回归闭环。",
        "",
        "## 状态",
        "",
        "- template_name: ``$($Summary.template_name)``",
        "- status: ``$($Summary.status)``",
        "- summary_json: ``$($Summary.summary_json)``",
        "- manual_review: ``$($Summary.manual_review)``",
        "- validation_report: ``$($Summary.validation_report)``",
        "- validation_issue: ``$($Summary.validation_issue)``",
        "",
        "## 下一步",
        "",
        "1. 编辑 render-data：``$($Summary.data_skeleton)``，替换所有 ``TODO:`` 占位。",
        "2. 运行严格校验，确认 ``remaining_placeholder_count=0``。",
        "",
        '```powershell',
        $StrictValidationCommand,
        '```',
        "",
        "3. 严格校验通过后渲染最终 DOCX。",
        "",
        '```powershell',
        $RenderCommand,
        '```',
        "",
        "4. 审阅通过后登记 manifest。",
        "",
        '```powershell',
        $RegisterCommand,
        '```'
    )
}

function New-ManualReview {
    param([object]$Summary, $Steps, [string]$StrictValidationCommand, [string]$RenderCommand, [string]$RegisterCommand)
    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Project Template Onboarding Review") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("## Summary") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- template_name: ``$($Summary.template_name)``") | Out-Null
    $lines.Add("- status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- input_docx: ``$($Summary.input_docx)``") | Out-Null
    $lines.Add("- schema_output: ``$($Summary.schema_output)``") | Out-Null
    $lines.Add("- workspace_dir: ``$($Summary.workspace_dir)``") | Out-Null
    $lines.Add("- temporary_smoke_manifest: ``$($Summary.temporary_smoke_manifest)``") | Out-Null
    $lines.Add("- project_template_smoke_summary: ``$($Summary.project_template_smoke_summary)``") | Out-Null
    $lines.Add("- schema_patch_review_count: ``$($Summary.schema_patch_review_count)``") | Out-Null
    $lines.Add("- schema_patch_review_changed_count: ``$($Summary.schema_patch_review_changed_count)``") | Out-Null
    $lines.Add("- schema_patch_approval_pending_count: ``$($Summary.schema_patch_approval_pending_count)``") | Out-Null
    $lines.Add("- schema_patch_approval_approved_count: ``$($Summary.schema_patch_approval_approved_count)``") | Out-Null
    $lines.Add("- schema_patch_approval_rejected_count: ``$($Summary.schema_patch_approval_rejected_count)``") | Out-Null
    $lines.Add("- validation_issue: ``$($Summary.validation_issue)``") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.schema_patch_reviews).Count -gt 0) {
        $lines.Add("## Schema Patch Reviews") | Out-Null
        $lines.Add("") | Out-Null
        foreach ($review in @($Summary.schema_patch_reviews)) {
            $lines.Add("- ``$($review.name)``: changed=``$($review.changed)`` baseline_slots=``$($review.baseline_slot_count)`` generated_slots=``$($review.generated_slot_count)`` review=``$($review.review_json)``") | Out-Null
        }
        $lines.Add("") | Out-Null
    }
    if (@($Summary.schema_patch_approval_items).Count -gt 0) {
        $lines.Add("## Schema Patch Approvals") | Out-Null
        $lines.Add("") | Out-Null
        foreach ($approval in @($Summary.schema_patch_approval_items)) {
            $lines.Add("- ``$($approval.name)``: status=``$($approval.status)`` decision=``$($approval.decision)`` candidate=``$($approval.schema_update_candidate)`` approval=``$($approval.approval_result)`` review=``$($approval.review_json)`` action=``$($approval.action)``") | Out-Null
        }
        $lines.Add("") | Out-Null
    }
    $lines.Add("## Steps") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($step in $Steps) {
        $message = if ($step.PSObject.Properties["message"]) { [string]$step.message } else { "" }
        $suffix = if ([string]::IsNullOrWhiteSpace($message)) { "" } else { " — $message" }
        $lines.Add("- ``$($step.name)``: ``$($step.status)``$suffix") | Out-Null
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Review Checklist") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- 确认 schema 没有意外 slot 漂移。") | Out-Null
    $lines.Add("- 补齐 render-data 中的 ``TODO:`` 和空表格行。") | Out-Null
    $lines.Add("- 严格校验通过后再渲染最终 DOCX。") | Out-Null
    $lines.Add("- 人工检查正文、表格、页眉页脚和分页。") | Out-Null
    $lines.Add("- 最后登记 manifest，并将 baseline 纳入版本控制。") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("## Commands") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add('```powershell') | Out-Null
    $lines.Add($StrictValidationCommand) | Out-Null
    $lines.Add($RenderCommand) | Out-Null
    $lines.Add($RegisterCommand) | Out-Null
    $lines.Add('```') | Out-Null
    return @($lines)
}

$repoRoot = Resolve-RepoRoot
$resolvedInputDocx = Resolve-RepoPath -RepoRoot $repoRoot -Path $InputDocx -MustExist
$displayName = if ([string]::IsNullOrWhiteSpace($TemplateName)) { [System.IO.Path]::GetFileNameWithoutExtension($resolvedInputDocx) } else { $TemplateName }
$safeName = Get-SafeName -Name $displayName

$resolvedOutputDir = if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    Resolve-RepoPath -RepoRoot $repoRoot -Path (Join-Path "output/project-template-onboarding" $safeName)
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir
}
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) { "" } else { Resolve-RepoPath -RepoRoot $repoRoot -Path $BuildDir }
$resolvedManifestPath = Resolve-RepoPath -RepoRoot $repoRoot -Path $ManifestPath

$schemaDir = Join-Path $resolvedOutputDir "schema"
$reportDir = Join-Path $resolvedOutputDir "report"
$workspaceDir = Join-Path $resolvedOutputDir "render-data-workspace"
$renderedDir = Join-Path $resolvedOutputDir "rendered"
$smokeDir = Join-Path $resolvedOutputDir "project-template-smoke"

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) { Join-Path $resolvedOutputDir "onboarding_summary.json" } else { Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson }
$schemaPath = Join-Path $schemaDir ($safeName + ".schema.json")
$generatedSchemaPath = Join-Path $reportDir ($safeName + ".generated.schema.json")
$temporaryManifestPath = Join-Path $resolvedOutputDir "project_template_smoke.manifest.json"
$workspaceSummaryPath = Join-Path $workspaceDir "render_data_workspace.summary.json"
$validationSummaryPath = Join-Path $reportDir "render_data_validation.summary.json"
$validationReportPath = Join-Path $reportDir "render_data_validation.md"
$strictValidationSummaryPath = Join-Path $reportDir "render_data_validation.strict.summary.json"
$strictValidationReportPath = Join-Path $reportDir "render_data_validation.strict.md"
$renderedDocxPath = Join-Path $renderedDir ($safeName + ".rendered.docx")
$renderSummaryPath = Join-Path $reportDir "render_from_workspace.summary.json"
$visualSmokeOutputDir = Join-Path $smokeDir "visual"
$smokeSummaryPath = Join-Path $smokeDir "summary.json"
$startHerePath = Join-Path $resolvedOutputDir "START_HERE.zh-CN.md"
$manualReviewPath = Join-Path $reportDir "manual_review.md"

Ensure-Directory -Path $resolvedOutputDir
Ensure-Directory -Path $schemaDir
Ensure-Directory -Path $reportDir
Ensure-Directory -Path $renderedDir

$buildDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedBuildDir
$commonSwitches = @($(if ($SkipBuild) { "SkipBuild" }))
$schemaSwitches = @($commonSwitches + $(switch ($SchemaTargetMode) {
    "section-targets" { "SectionTargets" }
    "resolved-section-targets" { "ResolvedSectionTargets" }
}))

$commands = [ordered]@{
    freeze_schema_baseline = New-CommandLine -Script ".\scripts\freeze_template_schema_baseline.ps1" -Arguments ([ordered]@{ InputDocx = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedInputDocx; SchemaOutput = Get-DisplayPath -RepoRoot $repoRoot -Path $schemaPath; BuildDir = $buildDisplay; Generator = $Generator }) -Switches $schemaSwitches
    run_project_template_smoke = New-CommandLine -Script ".\scripts\run_project_template_smoke.ps1" -Arguments ([ordered]@{ ManifestPath = Get-DisplayPath -RepoRoot $repoRoot -Path $temporaryManifestPath; BuildDir = $buildDisplay; Generator = $Generator; OutputDir = Get-DisplayPath -RepoRoot $repoRoot -Path $smokeDir }) -Switches @($commonSwitches + $(if (-not $IncludeVisualSmoke) { "SkipVisualSmoke" }))
    prepare_render_data_workspace = New-CommandLine -Script ".\scripts\prepare_template_render_data_workspace.ps1" -Arguments ([ordered]@{ InputDocx = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedInputDocx; WorkspaceDir = Get-DisplayPath -RepoRoot $repoRoot -Path $workspaceDir; SummaryJson = Get-DisplayPath -RepoRoot $repoRoot -Path $workspaceSummaryPath; BuildDir = $buildDisplay; Generator = $Generator }) -Switches $commonSwitches
    validate_render_data_workspace = New-CommandLine -Script ".\scripts\validate_render_data_mapping.ps1" -Arguments ([ordered]@{ WorkspaceDir = Get-DisplayPath -RepoRoot $repoRoot -Path $workspaceDir; WorkspaceSummary = Get-DisplayPath -RepoRoot $repoRoot -Path $workspaceSummaryPath; SummaryJson = Get-DisplayPath -RepoRoot $repoRoot -Path $validationSummaryPath; ReportMarkdown = Get-DisplayPath -RepoRoot $repoRoot -Path $validationReportPath; BuildDir = $buildDisplay; Generator = $Generator }) -Switches $commonSwitches
    validate_render_data_workspace_strict = New-CommandLine -Script ".\scripts\validate_render_data_mapping.ps1" -Arguments ([ordered]@{ WorkspaceDir = Get-DisplayPath -RepoRoot $repoRoot -Path $workspaceDir; WorkspaceSummary = Get-DisplayPath -RepoRoot $repoRoot -Path $workspaceSummaryPath; SummaryJson = Get-DisplayPath -RepoRoot $repoRoot -Path $strictValidationSummaryPath; ReportMarkdown = Get-DisplayPath -RepoRoot $repoRoot -Path $strictValidationReportPath; BuildDir = $buildDisplay; Generator = $Generator }) -Switches @($commonSwitches + "RequireComplete")
    render_template_document_from_workspace = New-CommandLine -Script ".\scripts\render_template_document_from_workspace.ps1" -Arguments ([ordered]@{ WorkspaceDir = Get-DisplayPath -RepoRoot $repoRoot -Path $workspaceDir; WorkspaceSummary = Get-DisplayPath -RepoRoot $repoRoot -Path $workspaceSummaryPath; OutputDocx = Get-DisplayPath -RepoRoot $repoRoot -Path $renderedDocxPath; SummaryJson = Get-DisplayPath -RepoRoot $repoRoot -Path $renderSummaryPath; BuildDir = $buildDisplay; Generator = $Generator }) -Switches $commonSwitches
    register_manifest_entry = New-CommandLine -Script ".\scripts\register_project_template_smoke_manifest_entry.ps1" -Arguments ([ordered]@{ Name = $safeName; ManifestPath = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedManifestPath; InputDocx = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedInputDocx; SchemaValidationFile = Get-DisplayPath -RepoRoot $repoRoot -Path $schemaPath; SchemaBaselineFile = Get-DisplayPath -RepoRoot $repoRoot -Path $schemaPath; SchemaBaselineTargetMode = $SchemaTargetMode; VisualSmokeOutputDir = Get-DisplayPath -RepoRoot $repoRoot -Path $visualSmokeOutputDir }) -Switches @($(if ($ReplaceExisting) { "ReplaceExisting" }))
}

$scriptPaths = [ordered]@{
    freeze_schema_baseline = Join-Path $repoRoot "scripts\freeze_template_schema_baseline.ps1"
    run_project_template_smoke = Join-Path $repoRoot "scripts\run_project_template_smoke.ps1"
    prepare_render_data_workspace = Join-Path $repoRoot "scripts\prepare_template_render_data_workspace.ps1"
    validate_render_data_workspace = Join-Path $repoRoot "scripts\validate_render_data_mapping.ps1"
    render_template_document_from_workspace = Join-Path $repoRoot "scripts\render_template_document_from_workspace.ps1"
    register_manifest_entry = Join-Path $repoRoot "scripts\register_project_template_smoke_manifest_entry.ps1"
}

$steps = New-Object 'System.Collections.Generic.List[object]'
$failed = $false
$pendingBusinessData = $false
$validationIssueMessage = ""

Write-Step "Preparing schema baseline candidate"
$freezeArgs = @{ InputDocx = $resolvedInputDocx; SchemaOutput = $schemaPath; Generator = $Generator }
if (-not [string]::IsNullOrWhiteSpace($resolvedBuildDir)) { $freezeArgs.BuildDir = $resolvedBuildDir }
if ($SkipBuild) { $freezeArgs.SkipBuild = $true }
switch ($SchemaTargetMode) {
    "section-targets" { $freezeArgs.SectionTargets = $true }
    "resolved-section-targets" { $freezeArgs.ResolvedSectionTargets = $true }
}
$step = Invoke-StepScript -Name "freeze_schema_baseline" -Command $commands.freeze_schema_baseline -ScriptPath $scriptPaths.freeze_schema_baseline -Arguments $freezeArgs -PlanOnly:$PlanOnly
$steps.Add($step) | Out-Null
if ($step.status -eq "failed") { $failed = $true }

if (-not $failed) {
    $steps.Add((New-Step -Name "write_project_template_smoke_manifest" -Status $(if ($PlanOnly) { "planned" } else { "completed" }) -Command "write temporary project-template-smoke manifest")) | Out-Null
    if (-not $PlanOnly) {
        Write-TempSmokeManifest -RepoRoot $repoRoot -Path $temporaryManifestPath -Name $safeName -InputDocx $resolvedInputDocx -SchemaFile $schemaPath -TargetMode $SchemaTargetMode -GeneratedSchema $generatedSchemaPath -VisualOutputDir $visualSmokeOutputDir -VisualEnabled:$IncludeVisualSmoke.IsPresent
    }
}

if ($SkipProjectTemplateSmoke) {
    $steps.Add((New-Step -Name "run_project_template_smoke" -Status "skipped" -Command $commands.run_project_template_smoke -Message "SkipProjectTemplateSmoke was provided.")) | Out-Null
} elseif (-not $failed) {
    Write-Step "Running project template smoke"
    $smokeArgs = @{ ManifestPath = $temporaryManifestPath; Generator = $Generator; OutputDir = $smokeDir }
    if (-not [string]::IsNullOrWhiteSpace($resolvedBuildDir)) { $smokeArgs.BuildDir = $resolvedBuildDir }
    if ($SkipBuild) { $smokeArgs.SkipBuild = $true }
    if (-not $IncludeVisualSmoke) { $smokeArgs.SkipVisualSmoke = $true }
    $step = Invoke-StepScript -Name "run_project_template_smoke" -Command $commands.run_project_template_smoke -ScriptPath $scriptPaths.run_project_template_smoke -Arguments $smokeArgs -PlanOnly:$PlanOnly
    $steps.Add($step) | Out-Null
    if ($step.status -eq "failed") { $failed = $true }
}

if (-not $failed) {
    Write-Step "Preparing render-data workspace"
    $prepareArgs = @{ InputDocx = $resolvedInputDocx; WorkspaceDir = $workspaceDir; SummaryJson = $workspaceSummaryPath; Generator = $Generator }
    if (-not [string]::IsNullOrWhiteSpace($resolvedBuildDir)) { $prepareArgs.BuildDir = $resolvedBuildDir }
    if ($SkipBuild) { $prepareArgs.SkipBuild = $true }
    $step = Invoke-StepScript -Name "prepare_render_data_workspace" -Command $commands.prepare_render_data_workspace -ScriptPath $scriptPaths.prepare_render_data_workspace -Arguments $prepareArgs -PlanOnly:$PlanOnly
    $steps.Add($step) | Out-Null
    if ($step.status -eq "failed") { $failed = $true }
}

$remainingPlaceholderCount = 0
if (-not $failed) {
    Write-Step "Validating render-data workspace"
    $validateArgs = @{ WorkspaceDir = $workspaceDir; WorkspaceSummary = $workspaceSummaryPath; SummaryJson = $validationSummaryPath; ReportMarkdown = $validationReportPath; Generator = $Generator }
    if (-not [string]::IsNullOrWhiteSpace($resolvedBuildDir)) { $validateArgs.BuildDir = $resolvedBuildDir }
    if ($SkipBuild) { $validateArgs.SkipBuild = $true }
    $step = Invoke-StepScript -Name "validate_render_data_workspace" -Command $commands.validate_render_data_workspace -ScriptPath $scriptPaths.validate_render_data_workspace -Arguments $validateArgs -PlanOnly:$PlanOnly
    $steps.Add($step) | Out-Null
    if ($step.status -eq "failed") {
        $validationSummary = Read-JsonIfPresent -Path $validationSummaryPath
        $validationIssueMessage = if ($null -ne $validationSummary) { [string]$validationSummary.error } else { "" }
        $expectedPendingIssue = ($validationIssueMessage -match 'bookmark_table_rows rows must be arrays of one or more cell texts' -or
                                 $validationIssueMessage -match 'render plan still contains' -or
                                 $validationIssueMessage -match 'TODO:' -or
                                 $validationIssueMessage -match 'empty table rows')
        if ($RequireComplete -or -not $expectedPendingIssue) {
            $failed = $true
        } else {
            $pendingBusinessData = $true
            $step.status = "completed_with_pending_business_data"
            if ([string]::IsNullOrWhiteSpace($step.error)) {
                $step.error = $validationIssueMessage
            }
        }
    } elseif (-not $PlanOnly) {
        $validationSummary = Read-JsonIfPresent -Path $validationSummaryPath
        $remainingPlaceholderCount = Get-JsonInt -Object $validationSummary -Name "remaining_placeholder_count"
        if ($RequireComplete -and $remainingPlaceholderCount -gt 0) {
            $pendingBusinessData = $true
            $failed = $true
            $validationIssueMessage = "remaining_placeholder_count=$remainingPlaceholderCount"
        } elseif ($remainingPlaceholderCount -gt 0) {
            $pendingBusinessData = $true
            $step.status = "completed_with_pending_business_data"
        }
    }
}

if (-not $failed -and -not $pendingBusinessData) {
    if ($PlanOnly) {
        $steps.Add((New-Step -Name "render_template_document_from_workspace" -Status "planned" -Command $commands.render_template_document_from_workspace)) | Out-Null
    } elseif ($remainingPlaceholderCount -gt 0) {
        $steps.Add((New-Step -Name "render_template_document_from_workspace" -Status "skipped" -Command $commands.render_template_document_from_workspace -Message "Render-data still contains $remainingPlaceholderCount generated placeholders.")) | Out-Null
    } else {
        Write-Step "Rendering DOCX from workspace"
        $renderArgs = @{ WorkspaceDir = $workspaceDir; WorkspaceSummary = $workspaceSummaryPath; OutputDocx = $renderedDocxPath; SummaryJson = $renderSummaryPath; Generator = $Generator }
        if (-not [string]::IsNullOrWhiteSpace($resolvedBuildDir)) { $renderArgs.BuildDir = $resolvedBuildDir }
        if ($SkipBuild) { $renderArgs.SkipBuild = $true }
        $step = Invoke-StepScript -Name "render_template_document_from_workspace" -Command $commands.render_template_document_from_workspace -ScriptPath $scriptPaths.render_template_document_from_workspace -Arguments $renderArgs -PlanOnly:$false
        $steps.Add($step) | Out-Null
        if ($step.status -eq "failed") { $failed = $true }
    }
}

if ($RegisterManifest -and -not $failed) {
    Write-Step "Registering committed manifest entry"
    $registerArgs = @{ Name = $safeName; ManifestPath = $resolvedManifestPath; InputDocx = $resolvedInputDocx; SchemaValidationFile = $schemaPath; SchemaBaselineFile = $schemaPath; SchemaBaselineTargetMode = $SchemaTargetMode; VisualSmokeOutputDir = $visualSmokeOutputDir }
    if ($ReplaceExisting) { $registerArgs.ReplaceExisting = $true }
    $step = Invoke-StepScript -Name "register_manifest_entry" -Command $commands.register_manifest_entry -ScriptPath $scriptPaths.register_manifest_entry -Arguments $registerArgs -PlanOnly:$PlanOnly
    $steps.Add($step) | Out-Null
    if ($step.status -eq "failed") { $failed = $true }
} else {
    $steps.Add((New-Step -Name "register_manifest_entry" -Status "skipped" -Command $commands.register_manifest_entry -Message "Pass -RegisterManifest after review to update the committed manifest.")) | Out-Null
}

$workspaceSummary = Read-JsonIfPresent -Path $workspaceSummaryPath
$smokeSummary = Read-JsonIfPresent -Path $smokeSummaryPath
$schemaPatchReviews = Get-JsonArray -Object $smokeSummary -Name "schema_patch_reviews"
$schemaPatchApprovalItems = Get-JsonArray -Object $smokeSummary -Name "schema_patch_approval_items"
$dataSkeleton = Get-JsonString -Object $workspaceSummary -Name "data_skeleton"
$mappingPath = Get-JsonString -Object $workspaceSummary -Name "mapping"
$renderPlanPath = Get-JsonString -Object $workspaceSummary -Name "render_plan"

$status = if ($failed) { "failed" } elseif ($PlanOnly) { "planned" } elseif ($remainingPlaceholderCount -gt 0) { "completed_with_pending_business_data" } else { "completed" }
if (-not $failed -and ($pendingBusinessData -or $remainingPlaceholderCount -gt 0)) {
    $status = "completed_with_pending_business_data"
}

$summary = [ordered]@{}
if (-not $failed -and [string]::IsNullOrWhiteSpace($validationIssueMessage)) {
    if ($PlanOnly) {
        $validationIssueMessage = "validation_not_executed_in_plan_only_mode=true"
    } elseif ($pendingBusinessData -and $remainingPlaceholderCount -gt 0) {
        $validationIssueMessage = "remaining_placeholder_count=$remainingPlaceholderCount"
    } elseif ($pendingBusinessData) {
        $validationIssueMessage = "pending_business_data=true"
    }
}

$summary["generated_at"] = (Get-Date).ToString("s")
$summary["status"] = $status
$summary["plan_only"] = $PlanOnly.IsPresent
$summary["template_name"] = $safeName
$summary["input_docx"] = $resolvedInputDocx
$summary["output_dir"] = $resolvedOutputDir
$summary["summary_json"] = $summaryPath
$summary["start_here"] = $startHerePath
$summary["manual_review"] = $manualReviewPath
$summary["schema_output"] = $schemaPath
$summary["temporary_smoke_manifest"] = $temporaryManifestPath
$summary["project_template_smoke_summary"] = $smokeSummaryPath
$summary["schema_patch_review_count"] = Get-JsonInt -Object $smokeSummary -Name "schema_patch_review_count"
$summary["schema_patch_review_changed_count"] = Get-JsonInt -Object $smokeSummary -Name "schema_patch_review_changed_count"
$summary["schema_patch_reviews"] = @($schemaPatchReviews)
$summary["schema_patch_approval_pending_count"] = Get-JsonInt -Object $smokeSummary -Name "schema_patch_approval_pending_count"
$summary["schema_patch_approval_approved_count"] = Get-JsonInt -Object $smokeSummary -Name "schema_patch_approval_approved_count"
$summary["schema_patch_approval_rejected_count"] = Get-JsonInt -Object $smokeSummary -Name "schema_patch_approval_rejected_count"
$summary["schema_patch_approval_items"] = @($schemaPatchApprovalItems)
$summary["workspace_dir"] = $workspaceDir
$summary["workspace_summary"] = $workspaceSummaryPath
$summary["render_plan"] = $renderPlanPath
$summary["mapping"] = $mappingPath
$summary["data_skeleton"] = $dataSkeleton
$summary["validation_summary"] = $validationSummaryPath
$summary["validation_report"] = $validationReportPath
$summary["remaining_placeholder_count"] = $remainingPlaceholderCount
$summary["pending_business_data"] = $pendingBusinessData
$summary["validation_issue"] = $validationIssueMessage
$summary["rendered_docx"] = $renderedDocxPath
$summary["render_summary"] = $renderSummaryPath
$summary["committed_manifest"] = $resolvedManifestPath
$summary["commands"] = $commands
$summary["steps"] = @($steps.ToArray())
$summary["next_actions"] = @(
    "Edit render-data JSON and replace TODO placeholders.",
    "Run validate_render_data_workspace_strict and require remaining_placeholder_count=0.",
    "Render the DOCX and review layout.",
    "Rerun with -RegisterManifest when the bundle is ready to commit."
)

Ensure-PathParent -Path $summaryPath
($summary | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

Write-Lines -Path $startHerePath -Lines (New-StartHere -Summary $summary -StrictValidationCommand $commands.validate_render_data_workspace_strict -RenderCommand $commands.render_template_document_from_workspace -RegisterCommand $commands.register_manifest_entry)
Write-Lines -Path $manualReviewPath -Lines (New-ManualReview -Summary $summary -Steps $steps.ToArray() -StrictValidationCommand $commands.validate_render_data_workspace_strict -RenderCommand $commands.render_template_document_from_workspace -RegisterCommand $commands.register_manifest_entry)

Write-Step "Summary JSON: $summaryPath"
Write-Step "START_HERE: $startHerePath"
Write-Step "Manual review: $manualReviewPath"
Write-Step "Overall status: $status"

if ($failed) { exit 1 }
