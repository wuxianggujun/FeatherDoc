<#
.SYNOPSIS
Audits whether a style merge rollback plan can be restored.

.DESCRIPTION
Runs featherdoc_cli restore-style-merge in dry-run mode against a rollback plan
and writes a stable featherdoc.style_merge_restore_audit.v1 summary. The script
is read-only for DOCX files and can consume the summary produced by
apply_reviewed_style_merge_suggestions.ps1.
#>
param(
    [string]$InputDocx = "",
    [string]$RollbackPlan = "",
    [string]$ApplySummaryJson = "",
    [string]$SummaryJson = "",
    [int[]]$Entry = @(),
    [string[]]$SourceStyle = @(),
    [string[]]$TargetStyle = @(),
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [string]$CliPath = "",
    [switch]$SkipBuild,
    [switch]$FailOnIssue
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

. (Join-Path $PSScriptRoot "audit_style_merge_restore_plan_helpers.ps1")

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedApplySummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $ApplySummaryJson
$applySummary = $null
if (-not [string]::IsNullOrWhiteSpace($resolvedApplySummaryJson)) {
    Write-Step "Reading apply summary: $resolvedApplySummaryJson"
    $applySummary = Read-JsonObject -Path $resolvedApplySummaryJson
}

$inputReference = if (-not [string]::IsNullOrWhiteSpace($InputDocx)) {
    $InputDocx
} elseif ($null -ne $applySummary) {
    Get-JsonString -Object $applySummary -Names @("output_docx", "output_docx_relative_path")
} else {
    ""
}
$rollbackReference = if (-not [string]::IsNullOrWhiteSpace($RollbackPlan)) {
    $RollbackPlan
} elseif ($null -ne $applySummary) {
    Get-JsonString -Object $applySummary -Names @("rollback_plan", "rollback_plan_relative_path", "rollback_plan_file")
} else {
    ""
}

if ([string]::IsNullOrWhiteSpace($inputReference)) {
    throw "InputDocx is required unless ApplySummaryJson provides output_docx."
}
if ([string]::IsNullOrWhiteSpace($rollbackReference)) {
    throw "RollbackPlan is required unless ApplySummaryJson provides rollback_plan."
}

$resolvedInputDocx = if ($null -ne $applySummary -and [string]::IsNullOrWhiteSpace($InputDocx)) {
    Resolve-ReferencePath -ReferenceJsonPath $resolvedApplySummaryJson -RepoRoot $repoRoot -InputPath $inputReference
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $inputReference
}
$resolvedRollbackPlan = if ($null -ne $applySummary -and [string]::IsNullOrWhiteSpace($RollbackPlan)) {
    Resolve-ReferencePath -ReferenceJsonPath $resolvedApplySummaryJson -RepoRoot $repoRoot -InputPath $rollbackReference
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $rollbackReference
}
$rollbackPlanObject = Read-JsonObject -Path $resolvedRollbackPlan
$rollbackPlanSummary = Get-RollbackPlanSummary -RollbackPlanObject $rollbackPlanObject

$resolvedSummaryJson = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    $rollbackDirectory = [System.IO.Path]::GetDirectoryName($resolvedRollbackPlan)
    $rollbackBaseName = [System.IO.Path]::GetFileNameWithoutExtension($resolvedRollbackPlan)
    Join-Path $rollbackDirectory ($rollbackBaseName + ".restore-audit.summary.json")
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SummaryJson -AllowMissing
}

Write-Step "Resolving featherdoc_cli"
$resolvedCliPath = if ([string]::IsNullOrWhiteSpace($CliPath)) {
    Resolve-TemplateSchemaCliPath `
        -RepoRoot $repoRoot `
        -BuildDir $BuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $CliPath
}

Ensure-ParentDirectory -Path $resolvedSummaryJson

$arguments = New-Object 'System.Collections.Generic.List[string]'
$arguments.Add("restore-style-merge") | Out-Null
$arguments.Add($resolvedInputDocx) | Out-Null
$arguments.Add("--rollback-plan") | Out-Null
$arguments.Add($resolvedRollbackPlan) | Out-Null
foreach ($entryIndex in @($Entry)) {
    $arguments.Add("--entry") | Out-Null
    $arguments.Add([string]$entryIndex) | Out-Null
}
foreach ($styleId in @($SourceStyle)) {
    if (-not [string]::IsNullOrWhiteSpace($styleId)) {
        $arguments.Add("--source-style") | Out-Null
        $arguments.Add($styleId) | Out-Null
    }
}
foreach ($styleId in @($TargetStyle)) {
    if (-not [string]::IsNullOrWhiteSpace($styleId)) {
        $arguments.Add("--target-style") | Out-Null
        $arguments.Add($styleId) | Out-Null
    }
}
$arguments.Add("--dry-run") | Out-Null
$arguments.Add("--json") | Out-Null
$argumentArray = @($arguments)

Write-Step "Auditing style merge restore plan"
$result = Invoke-TemplateSchemaCli -ExecutablePath $resolvedCliPath -Arguments $argumentArray
foreach ($line in @($result.Output)) {
    Write-Host $line
}

$cliJson = $null
try {
    $cliJson = Read-JsonObjectFromLines -Lines $result.Output -Command "restore-style-merge"
} catch {
    if ($result.ExitCode -eq 0) {
        throw
    }
}
if ($result.ExitCode -ne 0) {
    $message = if ([string]::IsNullOrWhiteSpace($result.Text)) {
        "featherdoc_cli restore-style-merge --dry-run failed with exit code $($result.ExitCode)."
    } else {
        $result.Text
    }
    throw $message
}

$issueCount = Get-JsonInt -Object $cliJson -Names @("issue_count") -DefaultValue 0
$issueSummary = Get-JsonProperty -Object $cliJson -Name "issue_summary"
$issueSummaryGroups = Get-IssueSummaryGroups -IssueSummary $issueSummary
$issueReviewGroups = Get-IssueReviewGroups `
    -Operations (Get-JsonProperty -Object $cliJson -Name "operations") `
    -InputDocx $resolvedInputDocx `
    -RollbackPlan $resolvedRollbackPlan
$requestedCount = Get-JsonInt -Object $cliJson -Names @("requested_count") -DefaultValue 0
$status = if ($issueCount -eq 0) { "clean" } else { "needs_review" }
$statusReason = if ($issueCount -eq 0) {
    "style_merge_restore_audit_clean"
} else {
    "style_merge_restore_audit_issues"
}
$summaryBasePath = [System.IO.Path]::GetDirectoryName($resolvedSummaryJson)
if ([string]::IsNullOrWhiteSpace($summaryBasePath)) {
    $summaryBasePath = (Get-Location).Path
}
$summaryDisplayPath = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedSummaryJson
$rollbackPlanDisplayPath = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedRollbackPlan

$restoreAuditCommand = "featherdoc_cli " + (ConvertTo-CommandLine -Arguments $argumentArray)
$selectedRestoreCommandTemplate = New-StyleMergeRestoreCommand `
    -InputDocx $resolvedInputDocx `
    -RollbackPlan $resolvedRollbackPlan `
    -EntryIndexes @($Entry) `
    -SourceStyles @($SourceStyle) `
    -TargetStyles @($TargetStyle) `
    -OutputTemplate
$restorableRollbackCommandSummary = Get-RestorableRollbackEntryCommands `
    -RollbackPlanObject $rollbackPlanObject `
    -InputDocx $resolvedInputDocx `
    -RollbackPlan $resolvedRollbackPlan
$restorableCommandSummary = Get-StyleMergeRestoreRestorableCommandSummary `
    -RestorableRollbackCommandSummary $restorableRollbackCommandSummary
$visualReviewDocx = Convert-ToPortableRelativePath -BasePath $repoRoot -TargetPath $resolvedInputDocx
if ([string]::IsNullOrWhiteSpace($visualReviewDocx)) {
    $visualReviewDocx = $resolvedInputDocx
}
$visualReviewSourceKind = "style-merge-restore-audit"
$visualReviewCommand = ConvertTo-CommandLine -Arguments @(
    "pwsh",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    ".\scripts\prepare_word_review_task.ps1",
    "-DocxPath",
    $visualReviewDocx,
    "-DocumentSourceKind",
    $visualReviewSourceKind,
    "-DocumentSourceLabel",
    "Style merge restore audit",
    "-Mode",
    "review-only"
)
$openVisualReviewCommand = ConvertTo-CommandLine -Arguments @(
    "pwsh",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    ".\scripts\open_latest_word_review_task.ps1",
    "-SourceKind",
    $visualReviewSourceKind,
    "-PrintPrompt"
)
$minimumRiskNextAction = if ($issueCount -eq 0) {
    "prepare_word_visual_review"
} else {
    "review_style_merge_restore_audit_issues"
}
$minimumRiskNextActionCommand = if ($issueCount -eq 0) {
    $visualReviewCommand
} else {
    $restoreAuditCommand
}
$reviewHandoffSteps = Get-StyleMergeRestoreReviewHandoffSteps `
    -IssueCount $issueCount `
    -IssueReviewGroups $issueReviewGroups `
    -RestorableRollbackCommandSummary $restorableRollbackCommandSummary `
    -RestoreAuditCommand $restoreAuditCommand `
    -SelectedRestoreCommandTemplate $selectedRestoreCommandTemplate `
    -VisualReviewCommand $visualReviewCommand `
    -OpenVisualReviewCommand $openVisualReviewCommand `
    -SourceSchema "featherdoc.style_merge_restore_audit.v1" `
    -SourceReportDisplay $summaryDisplayPath `
    -SourceJsonDisplay $summaryDisplayPath `
    -RollbackPlanDisplay $rollbackPlanDisplayPath
$nextHandoffStep = Get-NextStyleMergeRestoreHandoffStep -Steps $reviewHandoffSteps
$issueReviewCommands = Get-StyleMergeRestoreIssueReviewCommands -IssueReviewGroups $issueReviewGroups
$firstIssueReviewCommand = if (@($issueReviewCommands).Count -gt 0) { [string]@($issueReviewCommands)[0] } else { "" }
$issueReviewGroupSummary = Get-StyleMergeRestoreIssueReviewGroupSummary `
    -IssueReviewGroups $issueReviewGroups `
    -IssueReviewCommands @($issueReviewCommands)
$nextCopyCommand = Get-JsonString -Object $nextHandoffStep -Names @("copy_command", "command", "command_template")
$nextStepReason = Get-JsonString -Object $nextHandoffStep -Names @("reason")
$handoffStatusSummary = Get-StyleMergeRestoreHandoffStatusSummary `
    -Steps $reviewHandoffSteps `
    -NextHandoffStep $nextHandoffStep `
    -Status $status `
    -StatusReason $statusReason `
    -IssueCount $issueCount

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
if ($issueCount -gt 0) {
    $releaseBlockers.Add([ordered]@{
        id = "style_merge.restore_audit_issues"
        severity = "error"
        status = "blocked"
        action = "review_style_merge_restore_audit"
        message = "Style merge restore dry-run reported $issueCount issue(s)."
        issue_keys = @($statusReason)
        repair_strategy = "review_style_merge_restore_audit"
        repair_hint = "Review issue_summary_groups, rerun the restore dry-run for the selected rollback entries, then prepare Word visual review after the audit is clean."
        command = $restoreAuditCommand
        open_command = $openVisualReviewCommand
        source_schema = "featherdoc.style_merge_restore_audit.v1"
        source_report = $resolvedSummaryJson
        source_report_display = $summaryDisplayPath
        source_json = $resolvedSummaryJson
        source_json_display = $summaryDisplayPath
        rollback_plan_display = $rollbackPlanDisplayPath
        review_handoff_step_count = @($reviewHandoffSteps).Count
        review_handoff_steps = @($reviewHandoffSteps)
        next_handoff_step = $nextHandoffStep
        next_copy_command = $nextCopyCommand
        next_step_reason = $nextStepReason
        issue_review_commands = @($issueReviewCommands)
        issue_review_command_count = @($issueReviewCommands).Count
        first_issue_review_command = $firstIssueReviewCommand
        copy_issue_review_command = $firstIssueReviewCommand
        issue_review_group_summary = $issueReviewGroupSummary
        handoff_status_summary = $handoffStatusSummary
        rollback_plan_summary = $rollbackPlanSummary
    }) | Out-Null
}

$actionItems = New-Object 'System.Collections.Generic.List[object]'
$actionItems.Add([ordered]@{
    id = "review_style_merge_restore_audit"
    action = "review_style_merge_restore_audit"
    title = "Review style merge restore audit and Word render"
    command = $visualReviewCommand
    open_command = $openVisualReviewCommand
    audit_command = $restoreAuditCommand
    minimum_risk_next_action = $minimumRiskNextAction
    minimum_risk_next_action_command = $minimumRiskNextActionCommand
    source_schema = "featherdoc.style_merge_restore_audit.v1"
    source_report = $resolvedSummaryJson
    source_report_display = $summaryDisplayPath
    source_json = $resolvedSummaryJson
    source_json_display = $summaryDisplayPath
    rollback_plan_display = $rollbackPlanDisplayPath
    review_handoff_step_count = @($reviewHandoffSteps).Count
    review_handoff_steps = @($reviewHandoffSteps)
    next_handoff_step = $nextHandoffStep
    next_copy_command = $nextCopyCommand
    next_step_reason = $nextStepReason
    issue_review_commands = @($issueReviewCommands)
    issue_review_command_count = @($issueReviewCommands).Count
    first_issue_review_command = $firstIssueReviewCommand
    copy_issue_review_command = $firstIssueReviewCommand
    issue_review_group_summary = $issueReviewGroupSummary
    handoff_status_summary = $handoffStatusSummary
    rollback_plan_summary = $rollbackPlanSummary
}) | Out-Null

$selectionSummary = [ordered]@{
    entry_filter_count = @($Entry).Count
    source_style_filter_count = @($SourceStyle | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }).Count
    target_style_filter_count = @($TargetStyle | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }).Count
    has_entry_filter = (@($Entry).Count -gt 0)
    has_source_style_filter = (@($SourceStyle | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }).Count -gt 0)
    has_target_style_filter = (@($TargetStyle | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }).Count -gt 0)
    requested_count = $requestedCount
    restored_count = Get-JsonInt -Object $cliJson -Names @("restored_count") -DefaultValue 0
}

$summary = [ordered]@{
    schema = "featherdoc.style_merge_restore_audit.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    status_reason = $statusReason
    dry_run = $true
    input_docx = $resolvedInputDocx
    input_docx_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedInputDocx
    rollback_plan = $resolvedRollbackPlan
    rollback_plan_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedRollbackPlan
    rollback_plan_exists = Test-Path -LiteralPath $resolvedRollbackPlan
    apply_summary_json = $resolvedApplySummaryJson
    apply_summary_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedApplySummaryJson
    selected_entries = @($Entry)
    selected_source_styles = @($SourceStyle)
    selected_target_styles = @($TargetStyle)
    selection_summary = $selectionSummary
    review_handoff_step_count = @($reviewHandoffSteps).Count
    review_handoff_steps = @($reviewHandoffSteps)
    next_handoff_step = $nextHandoffStep
    next_copy_command = $nextCopyCommand
    next_step_reason = $nextStepReason
    issue_review_commands = @($issueReviewCommands)
    issue_review_command_count = @($issueReviewCommands).Count
    first_issue_review_command = $firstIssueReviewCommand
    copy_issue_review_command = $firstIssueReviewCommand
    issue_review_group_summary = $issueReviewGroupSummary
    handoff_status_summary = $handoffStatusSummary
    rollback_plan_summary = $rollbackPlanSummary
    restorable_rollback_command_summary = $restorableCommandSummary
    selected_restore_command_template = $selectedRestoreCommandTemplate
    restorable_rollback_entry_count = $restorableRollbackCommandSummary.restorable_rollback_entry_count
    restorable_rollback_entry_indexes = @($restorableRollbackCommandSummary.restorable_rollback_entry_indexes)
    per_entry_dry_run_commands = @($restorableRollbackCommandSummary.per_entry_dry_run_commands)
    per_entry_restore_command_templates = @($restorableRollbackCommandSummary.per_entry_restore_command_templates)
    batch_restorable_dry_run_command = $restorableRollbackCommandSummary.batch_restorable_dry_run_command
    batch_restorable_restore_command_template = $restorableRollbackCommandSummary.batch_restorable_restore_command_template
    issue_count = $issueCount
    issue_summary = $issueSummary
    issue_summary_group_count = @($issueSummaryGroups).Count
    issue_summary_groups = @($issueSummaryGroups)
    issue_review_group_count = @($issueReviewGroups).Count
    issue_review_groups = @($issueReviewGroups)
    requested_count = $requestedCount
    restored_count = Get-JsonInt -Object $cliJson -Names @("restored_count") -DefaultValue 0
    restored_style_count = Get-JsonInt -Object $cliJson -Names @("restored_style_count") -DefaultValue 0
    restored_reference_count = Get-JsonInt -Object $cliJson -Names @("restored_reference_count") -DefaultValue 0
    command = $restoreAuditCommand
    minimum_risk_next_action = $minimumRiskNextAction
    minimum_risk_next_action_command = $minimumRiskNextActionCommand
    visual_review_command = $visualReviewCommand
    open_visual_review_command = $openVisualReviewCommand
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    cli_exit_code = $result.ExitCode
    cli_result = $cliJson
}

($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
Write-Step "Restore audit summary: $resolvedSummaryJson"
Write-Step "Status: $status"
Write-Step "Issue count: $issueCount"

if ($FailOnIssue -and $issueCount -gt 0) {
    throw "Style merge restore audit found $issueCount issue(s)."
}
