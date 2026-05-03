param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Expected text '$ExpectedText'."
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\new_project_template_smoke_onboarding_plan.ps1"
$manifestPath = Join-Path $resolvedWorkingDir "empty_project_template_smoke.manifest.json"
$outputDir = Join-Path $resolvedWorkingDir "onboarding-plan"
$workspaceRoot = Join-Path $resolvedWorkingDir "render-data-workspaces"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null
@{
    entries = @()
    candidate_exclusions = @()
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $manifestPath -Encoding UTF8

$invokeParams = @{
    ManifestPath = $manifestPath
    SearchRoot = Join-Path $resolvedRepoRoot "samples"
    OutputDir = $outputDir
    RenderDataWorkspaceRoot = $workspaceRoot
}
if (-not [string]::IsNullOrWhiteSpace($BuildDir)) {
    $invokeParams.BuildDir = $BuildDir
}

& $scriptPath @invokeParams
if ($LASTEXITCODE -ne 0) {
    throw "new_project_template_smoke_onboarding_plan.ps1 failed."
}

$planJsonPath = Join-Path $outputDir "plan.json"
$planMarkdownPath = Join-Path $outputDir "plan.md"

Assert-True -Condition (Test-Path -LiteralPath $planJsonPath) `
    -Message "Onboarding plan JSON was not written."
Assert-True -Condition (Test-Path -LiteralPath $planMarkdownPath) `
    -Message "Onboarding plan Markdown was not written."

$plan = Get-Content -Raw -Encoding UTF8 -LiteralPath $planJsonPath | ConvertFrom-Json
$markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $planMarkdownPath

Assert-True -Condition ([int]$plan.onboarding_entry_count -gt 0) `
    -Message "Expected at least one unregistered sample template candidate."
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace([string]$plan.render_data_workspace_root)) `
    -Message "Plan should include render_data_workspace_root."

$entry = @($plan.entries)[0]
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace([string]$entry.render_data_workspace_dir)) `
    -Message "Entry should include render_data_workspace_dir."
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace([string]$entry.render_data_workspace_summary)) `
    -Message "Entry should include render_data_workspace_summary."
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace([string]$entry.render_data_validation_summary)) `
    -Message "Entry should include render_data_validation_summary."
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace([string]$entry.render_data_validation_report)) `
    -Message "Entry should include render_data_validation_report."
Assert-True -Condition (@($entry.review_checklist).Count -ge 4) `
    -Message "Entry should include a useful review checklist."
Assert-True -Condition ($null -ne $entry.schema_approval_state) `
    -Message "Entry should include schema_approval_state."
Assert-ContainsText -Text ([string]$entry.schema_approval_state.status) `
    -ExpectedText "not_evaluated" `
    -Message "Plan entry should mark schema approval as not evaluated."
Assert-ContainsText -Text ([string]$entry.schema_approval_state.action) `
    -ExpectedText "run_project_template_smoke_then_review_schema_patch_approval" `
    -Message "Plan entry should expose the schema approval action."
Assert-True -Condition ([int]$entry.release_blocker_count -eq 1) `
    -Message "Plan entry should expose one planned release blocker."
Assert-ContainsText -Text ([string]$entry.release_blockers[0].id) `
    -ExpectedText "project_template_onboarding.schema_approval_not_evaluated" `
    -Message "Plan entry release blocker should identify not-evaluated schema approval."
Assert-True -Condition (@($entry.action_items).Count -ge 3) `
    -Message "Plan entry should include action items."
Assert-True -Condition (@($entry.manual_review_recommendations).Count -ge 3) `
    -Message "Plan entry should include manual review recommendations."

Assert-ContainsText -Text ([string]$entry.commands.prepare_render_data_workspace) `
    -ExpectedText "prepare_template_render_data_workspace.ps1" `
    -Message "Prepare command should call the render-data workspace helper."
Assert-ContainsText -Text ([string]$entry.commands.prepare_render_data_workspace) `
    -ExpectedText "-WorkspaceDir" `
    -Message "Prepare command should choose a workspace directory."
Assert-ContainsText -Text ([string]$entry.commands.validate_render_data_workspace) `
    -ExpectedText "validate_render_data_mapping.ps1" `
    -Message "Validate command should call the render-data validation helper."
Assert-ContainsText -Text ([string]$entry.commands.validate_render_data_workspace) `
    -ExpectedText "-RequireComplete" `
    -Message "Validate command should require complete business data."
Assert-ContainsText -Text ([string]$entry.commands.validate_render_data_workspace) `
    -ExpectedText "-ReportMarkdown" `
    -Message "Validate command should emit a readable report."

Assert-ContainsText -Text $markdown `
    -ExpectedText "Render-data workspace root" `
    -Message "Markdown plan should summarize render-data workspace root."
Assert-ContainsText -Text $markdown `
    -ExpectedText "Review checklist" `
    -Message "Markdown plan should include the per-entry checklist."
Assert-ContainsText -Text $markdown `
    -ExpectedText "Schema approval status" `
    -Message "Markdown plan should expose schema approval status."
Assert-ContainsText -Text $markdown `
    -ExpectedText "Release blockers" `
    -Message "Markdown plan should expose release blockers."
Assert-ContainsText -Text $markdown `
    -ExpectedText "Manual review recommendations" `
    -Message "Markdown plan should expose manual review recommendations."
Assert-ContainsText -Text $markdown `
    -ExpectedText "prepare_template_render_data_workspace.ps1" `
    -Message "Markdown plan should include the prepare workspace command."
Assert-ContainsText -Text $markdown `
    -ExpectedText "validate_render_data_mapping.ps1" `
    -Message "Markdown plan should include the validation command."

Write-Host "Project template smoke onboarding plan regression passed."
