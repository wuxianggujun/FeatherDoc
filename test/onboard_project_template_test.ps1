param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected '$Expected', got '$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Expected text '$ExpectedText'."
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\onboard_project_template.ps1"
$outputDir = Join-Path $resolvedWorkingDir "invoice-onboarding"
$inputDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$invokeParams = @{
    InputDocx = $inputDocx
    TemplateName = "Invoice Template"
    OutputDir = $outputDir
    PlanOnly = $true
    SkipProjectTemplateSmoke = $true
}
if (-not [string]::IsNullOrWhiteSpace($BuildDir)) {
    $invokeParams.BuildDir = $BuildDir
}

& $scriptPath @invokeParams
$scriptSucceeded = $?
if (-not $scriptSucceeded) {
    throw "onboard_project_template.ps1 failed."
}

$summaryPath = Join-Path $outputDir "onboarding_summary.json"
$startHerePath = Join-Path $outputDir "START_HERE.zh-CN.md"
$manualReviewPath = Join-Path $outputDir "report\manual_review.md"

Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "Onboarding summary JSON was not written."
Assert-True -Condition (Test-Path -LiteralPath $startHerePath) `
    -Message "START_HERE handoff was not written."
Assert-True -Condition (Test-Path -LiteralPath $manualReviewPath) `
    -Message "Manual review report was not written."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$startHere = Get-Content -Raw -Encoding UTF8 -LiteralPath $startHerePath
$manualReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $manualReviewPath

Assert-Equal -Actual $summary.status -Expected "planned" `
    -Message "Plan-only onboarding should report planned status."
Assert-Equal -Actual $summary.plan_only -Expected $true `
    -Message "Summary should record plan_only=true."
Assert-Equal -Actual $summary.template_name -Expected "invoice-template" `
    -Message "Template name should be normalized for file and manifest usage."
Assert-ContainsText -Text ([string]$summary.validation_issue) `
    -ExpectedText "validation_not_executed_in_plan_only_mode=true" `
    -Message "Plan-only onboarding should record an explicit not-executed hint."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_pending_count) -Expected 0 `
    -Message "Plan-only onboarding without smoke should expose zero pending schema approvals."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_approved_count) -Expected 0 `
    -Message "Plan-only onboarding without smoke should expose zero approved schema approvals."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_rejected_count) -Expected 0 `
    -Message "Plan-only onboarding without smoke should expose zero rejected schema approvals."

$steps = @($summary.steps)
Assert-True -Condition ($steps.Count -ge 6) `
    -Message "Onboarding summary should include the workflow steps."
Assert-Equal -Actual $steps[0].name -Expected "freeze_schema_baseline" `
    -Message "First step should freeze the schema baseline candidate."
Assert-Equal -Actual $steps[0].status -Expected "planned" `
    -Message "Freeze step should be planned in plan-only mode."
Assert-Equal -Actual $steps[2].name -Expected "run_project_template_smoke" `
    -Message "Smoke step should be present."
Assert-Equal -Actual $steps[2].status -Expected "skipped" `
    -Message "Smoke step should honor SkipProjectTemplateSmoke."

Assert-ContainsText -Text ([string]$summary.commands.freeze_schema_baseline) `
    -ExpectedText "freeze_template_schema_baseline.ps1" `
    -Message "Summary should include the schema freeze command."
Assert-ContainsText -Text ([string]$summary.commands.prepare_render_data_workspace) `
    -ExpectedText "prepare_template_render_data_workspace.ps1" `
    -Message "Summary should include the render-data workspace command."
Assert-ContainsText -Text ([string]$summary.commands.validate_render_data_workspace_strict) `
    -ExpectedText "-RequireComplete" `
    -Message "Strict validation command should require complete business data."
Assert-ContainsText -Text ([string]$summary.commands.register_manifest_entry) `
    -ExpectedText "register_project_template_smoke_manifest_entry.ps1" `
    -Message "Summary should include the manifest registration command."

Assert-ContainsText -Text $startHere -ExpectedText "remaining_placeholder_count=0" `
    -Message "START_HERE should explain the strict validation gate."
Assert-ContainsText -Text $startHere -ExpectedText "render_template_document_from_workspace.ps1" `
    -Message "START_HERE should include the render command."
Assert-ContainsText -Text $manualReview -ExpectedText "Review Checklist" `
    -Message "Manual review should include a checklist."
Assert-ContainsText -Text $manualReview -ExpectedText "schema_output" `
    -Message "Manual review should summarize schema output."
Assert-ContainsText -Text $manualReview -ExpectedText "schema_patch_approval_pending_count" `
    -Message "Manual review should expose schema patch approval count."
Assert-ContainsText -Text $manualReview -ExpectedText "schema_patch_approval_approved_count" `
    -Message "Manual review should expose approved schema patch approval count."

Assert-True -Condition ([int]$summary.remaining_placeholder_count -ge 0) `
    -Message "Summary should expose remaining placeholder count."

Write-Host "Project template onboarding plan-only regression passed."
