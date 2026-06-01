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

function Write-Step {
    param([string]$Message)
    Write-Host "[style-merge-restore-audit] $Message"
}

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) { return $Object[$Name] }
        return $null
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-JsonString {
    param($Object, [string[]]$Names, [string]$DefaultValue = "")

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Object -Name $name
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            return [string]$value
        }
    }
    return $DefaultValue
}

function Get-JsonInt {
    param($Object, [string[]]$Names, [int]$DefaultValue = 0)

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Object -Name $name
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            return [int]$value
        }
    }
    return $DefaultValue
}

function Get-JsonBool {
    param($Object, [string[]]$Names, [bool]$DefaultValue = $false)

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Object -Name $name
        if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
            continue
        }
        if ($value -is [bool]) {
            return [bool]$value
        }

        $text = ([string]$value).Trim()
        if ($text -ieq "true" -or $text -eq "1") {
            return $true
        }
        if ($text -ieq "false" -or $text -eq "0") {
            return $false
        }
    }
    return $DefaultValue
}

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$InputPath, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($InputPath)) { return "" }
    return Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $InputPath -AllowMissing:$AllowMissing
}

function Resolve-ReferencePath {
    param(
        [string]$ReferenceJsonPath,
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) { return "" }
    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        if ($AllowMissing) { return [System.IO.Path]::GetFullPath($InputPath) }
        return (Resolve-Path -LiteralPath $InputPath).Path
    }

    $referenceDir = [System.IO.Path]::GetDirectoryName($ReferenceJsonPath)
    if (-not [string]::IsNullOrWhiteSpace($referenceDir)) {
        $referenceRelativePath = [System.IO.Path]::GetFullPath((Join-Path $referenceDir $InputPath))
        if ($AllowMissing -or (Test-Path -LiteralPath $referenceRelativePath)) {
            return $referenceRelativePath
        }
    }

    $repoRelativePath = Join-Path $RepoRoot $InputPath
    if ($AllowMissing) { return [System.IO.Path]::GetFullPath($repoRelativePath) }
    return (Resolve-Path -LiteralPath $repoRelativePath).Path
}

function Convert-ToPortableRelativePath {
    param([string]$BasePath, [string]$TargetPath)

    return ConvertTo-TemplateSchemaPortableRelativePath -BasePath $BasePath -TargetPath $TargetPath
}

function Ensure-ParentDirectory {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return }
    $directory = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
}

function Read-JsonObject {
    param([string]$Path)
    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Read-JsonObjectFromLines {
    param([string[]]$Lines, [string]$Command)

    foreach ($line in @($Lines)) {
        $text = [string]$line
        if (-not $text.TrimStart().StartsWith("{")) {
            continue
        }

        try {
            $object = $text | ConvertFrom-Json
            $commandValue = Get-JsonString -Object $object -Names @("command")
            if ($commandValue -eq $Command) {
                return $object
            }
        } catch {
            continue
        }
    }

    throw "Command '$Command' did not emit a JSON object."
}

function Get-IssueSummaryGroups {
    param($IssueSummary)

    if ($null -eq $IssueSummary) {
        return @()
    }

    $groups = [ordered]@{}
    foreach ($issue in @($IssueSummary)) {
        $code = Get-JsonString -Object $issue -Names @("code", "issue") -DefaultValue "unknown"
        $count = Get-JsonInt -Object $issue -Names @("count") -DefaultValue 1
        if ($groups.Contains($code)) {
            $groups[$code] = [int]$groups[$code] + $count
        } else {
            $groups[$code] = $count
        }
    }

    $result = New-Object 'System.Collections.Generic.List[object]'
    foreach ($entry in $groups.GetEnumerator()) {
        $result.Add([ordered]@{
            code = [string]$entry.Key
            count = [int]$entry.Value
        }) | Out-Null
    }
    return @($result.ToArray())
}

function Get-IssueReviewGroups {
    param(
        $Operations,
        [string]$InputDocx,
        [string]$RollbackPlan
    )

    if ($null -eq $Operations) {
        return @()
    }

    $groups = [ordered]@{}
    foreach ($operation in @($Operations)) {
        $issues = Get-JsonProperty -Object $operation -Name "issues"
        if ($null -eq $issues) {
            continue
        }

        $sourceStyleId = Get-JsonString -Object $operation -Names @("source_style_id")
        $targetStyleId = Get-JsonString -Object $operation -Names @("target_style_id")
        $operationAction = Get-JsonString -Object $operation -Names @("action") -DefaultValue "merge"

        foreach ($issue in @($issues)) {
            $code = Get-JsonString -Object $issue -Names @("code", "issue") -DefaultValue "unknown"
            if (-not $groups.Contains($code)) {
                $groups[$code] = [ordered]@{
                    count = 0
                    affected_operations = New-Object 'System.Collections.Generic.List[object]'
                    affected_operation_keys = @{}
                }
            }

            $group = $groups[$code]
            $group.count = [int]$group.count + 1

            $operationKey = "$operationAction`0$sourceStyleId`0$targetStyleId"
            if ($group.affected_operation_keys.ContainsKey($operationKey)) {
                continue
            }

            $reviewArguments = New-Object 'System.Collections.Generic.List[string]'
            $reviewArguments.Add("restore-style-merge") | Out-Null
            $reviewArguments.Add($InputDocx) | Out-Null
            $reviewArguments.Add("--rollback-plan") | Out-Null
            $reviewArguments.Add($RollbackPlan) | Out-Null
            if (-not [string]::IsNullOrWhiteSpace($sourceStyleId)) {
                $reviewArguments.Add("--source-style") | Out-Null
                $reviewArguments.Add($sourceStyleId) | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($targetStyleId)) {
                $reviewArguments.Add("--target-style") | Out-Null
                $reviewArguments.Add($targetStyleId) | Out-Null
            }
            $reviewArguments.Add("--dry-run") | Out-Null
            $reviewArguments.Add("--json") | Out-Null

            $group.affected_operation_keys[$operationKey] = $true
            $group.affected_operations.Add([ordered]@{
                action = $operationAction
                source_style_id = $sourceStyleId
                target_style_id = $targetStyleId
                review_command = "featherdoc_cli " + (ConvertTo-CommandLine -Arguments @($reviewArguments))
            }) | Out-Null
        }
    }

    $result = New-Object 'System.Collections.Generic.List[object]'
    foreach ($entry in $groups.GetEnumerator()) {
        $operations = @($entry.Value.affected_operations.ToArray())
        $reviewCommands = @($operations | ForEach-Object { [string]$_.review_command })
        $firstReviewCommand = if ($reviewCommands.Count -gt 0) { [string]$reviewCommands[0] } else { "" }
        $result.Add([ordered]@{
            code = [string]$entry.Key
            count = [int]$entry.Value.count
            affected_operation_count = $operations.Count
            affected_operations = $operations
            review_commands = $reviewCommands
            first_review_command = $firstReviewCommand
            copy_review_command = $firstReviewCommand
        }) | Out-Null
    }
    return @($result.ToArray())
}

function ConvertTo-CommandLine {
    param([string[]]$Arguments)

    return ConvertTo-TemplateSchemaCommandLine -Arguments $Arguments
}

function Get-DisplayPath {
    param([string]$RepoRoot, [string]$Path)

    return Convert-ToPortableRelativePath -BasePath $RepoRoot -TargetPath $Path
}

function New-StyleMergeRestoreCommand {
    param(
        [string]$InputDocx,
        [string]$RollbackPlan,
        [int[]]$EntryIndexes = @(),
        [string[]]$SourceStyles = @(),
        [string[]]$TargetStyles = @(),
        [switch]$DryRun,
        [switch]$OutputTemplate
    )

    $commandArguments = New-Object 'System.Collections.Generic.List[string]'
    $commandArguments.Add("restore-style-merge") | Out-Null
    $commandArguments.Add($InputDocx) | Out-Null
    $commandArguments.Add("--rollback-plan") | Out-Null
    $commandArguments.Add($RollbackPlan) | Out-Null

    foreach ($entryIndex in @($EntryIndexes)) {
        $commandArguments.Add("--entry") | Out-Null
        $commandArguments.Add([string]$entryIndex) | Out-Null
    }
    foreach ($styleId in @($SourceStyles)) {
        if (-not [string]::IsNullOrWhiteSpace($styleId)) {
            $commandArguments.Add("--source-style") | Out-Null
            $commandArguments.Add($styleId) | Out-Null
        }
    }
    foreach ($styleId in @($TargetStyles)) {
        if (-not [string]::IsNullOrWhiteSpace($styleId)) {
            $commandArguments.Add("--target-style") | Out-Null
            $commandArguments.Add($styleId) | Out-Null
        }
    }

    if ($DryRun) {
        $commandArguments.Add("--dry-run") | Out-Null
    } elseif ($OutputTemplate) {
        $commandArguments.Add("--output") | Out-Null
        $commandArguments.Add("<output.docx>") | Out-Null
    }
    $commandArguments.Add("--json") | Out-Null

    $commandLine = "featherdoc_cli " + (ConvertTo-CommandLine -Arguments @($commandArguments))
    return $commandLine.Replace('"<output.docx>"', '<output.docx>')
}

function Get-RestorableRollbackEntryCommands {
    param(
        $RollbackPlanObject,
        [string]$InputDocx,
        [string]$RollbackPlan
    )

    $operations = Get-JsonProperty -Object $RollbackPlanObject -Name "rollback_operations"
    if ($null -eq $operations) {
        return [ordered]@{
            restorable_rollback_entry_count = 0
            restorable_rollback_entry_indexes = @()
            per_entry_dry_run_commands = @()
            per_entry_restore_command_templates = @()
            batch_restorable_dry_run_command = ""
            batch_restorable_restore_command_template = ""
        }
    }

    $restorableIndexes = New-Object 'System.Collections.Generic.List[int]'
    $dryRunCommands = New-Object 'System.Collections.Generic.List[object]'
    $restoreCommandTemplates = New-Object 'System.Collections.Generic.List[object]'
    $operationIndex = 0
    foreach ($operation in @($operations)) {
        $action = Get-JsonString -Object $operation -Names @("action")
        $restorable = Get-JsonBool -Object $operation -Names @("restorable") -DefaultValue $false
        if ($action -ne "merge" -or -not $restorable) {
            $operationIndex += 1
            continue
        }

        $sourceStyleId = Get-JsonString -Object $operation -Names @("source_style_id")
        $targetStyleId = Get-JsonString -Object $operation -Names @("target_style_id")
        $entryIndexes = @([int]$operationIndex)
        $dryRunCommand = New-StyleMergeRestoreCommand `
            -InputDocx $InputDocx `
            -RollbackPlan $RollbackPlan `
            -EntryIndexes $entryIndexes `
            -DryRun
        $restoreCommandTemplate = New-StyleMergeRestoreCommand `
            -InputDocx $InputDocx `
            -RollbackPlan $RollbackPlan `
            -EntryIndexes $entryIndexes `
            -OutputTemplate

        $restorableIndexes.Add([int]$operationIndex) | Out-Null
        $dryRunCommands.Add([ordered]@{
            entry_index = [int]$operationIndex
            source_style_id = $sourceStyleId
            target_style_id = $targetStyleId
            command = $dryRunCommand
        }) | Out-Null
        $restoreCommandTemplates.Add([ordered]@{
            entry_index = [int]$operationIndex
            source_style_id = $sourceStyleId
            target_style_id = $targetStyleId
            command_template = $restoreCommandTemplate
        }) | Out-Null
        $operationIndex += 1
    }

    $restorableEntryIndexes = @($restorableIndexes.ToArray())
    $batchDryRunCommand = if ($restorableEntryIndexes.Count -eq 0) {
        ""
    } else {
        New-StyleMergeRestoreCommand `
            -InputDocx $InputDocx `
            -RollbackPlan $RollbackPlan `
            -EntryIndexes $restorableEntryIndexes `
            -DryRun
    }
    $batchRestoreCommandTemplate = if ($restorableEntryIndexes.Count -eq 0) {
        ""
    } else {
        New-StyleMergeRestoreCommand `
            -InputDocx $InputDocx `
            -RollbackPlan $RollbackPlan `
            -EntryIndexes $restorableEntryIndexes `
            -OutputTemplate
    }

    return [ordered]@{
        restorable_rollback_entry_count = $restorableEntryIndexes.Count
        restorable_rollback_entry_indexes = $restorableEntryIndexes
        per_entry_dry_run_commands = @($dryRunCommands.ToArray())
        per_entry_restore_command_templates = @($restoreCommandTemplates.ToArray())
        batch_restorable_dry_run_command = $batchDryRunCommand
        batch_restorable_restore_command_template = $batchRestoreCommandTemplate
    }
}

function Get-RollbackPlanSummary {
    param($RollbackPlanObject)

    $operations = Get-JsonProperty -Object $RollbackPlanObject -Name "rollback_operations"
    if ($null -eq $operations) {
        return [ordered]@{
            rollback_operation_count = 0
            merge_rollback_entry_count = 0
            restorable_merge_rollback_entry_count = 0
            non_restorable_merge_rollback_entry_count = 0
            has_non_restorable_merge_rollback_entries = $false
            non_restorable_merge_rollback_entry_indexes = @()
        }
    }

    $mergeEntryCount = 0
    $restorableMergeEntryCount = 0
    $nonRestorableMergeEntryIndexes = New-Object 'System.Collections.Generic.List[int]'
    $operationIndex = 0
    foreach ($operation in @($operations)) {
        $action = Get-JsonString -Object $operation -Names @("action")
        if ($action -ne "merge") {
            $operationIndex += 1
            continue
        }

        $mergeEntryCount += 1
        $restorable = Get-JsonBool -Object $operation -Names @("restorable") -DefaultValue $false
        if ($restorable) {
            $restorableMergeEntryCount += 1
        } else {
            $nonRestorableMergeEntryIndexes.Add([int]$operationIndex) | Out-Null
        }
        $operationIndex += 1
    }

    $nonRestorableIndexes = @($nonRestorableMergeEntryIndexes.ToArray())
    return [ordered]@{
        rollback_operation_count = @($operations).Count
        merge_rollback_entry_count = $mergeEntryCount
        restorable_merge_rollback_entry_count = $restorableMergeEntryCount
        non_restorable_merge_rollback_entry_count = $nonRestorableIndexes.Count
        has_non_restorable_merge_rollback_entries = ($nonRestorableIndexes.Count -gt 0)
        non_restorable_merge_rollback_entry_indexes = $nonRestorableIndexes
    }
}

function Get-StyleMergeRestoreReviewHandoffSteps {
    param(
        [int]$IssueCount,
        $IssueReviewGroups,
        $RestorableRollbackCommandSummary,
        [string]$RestoreAuditCommand,
        [string]$SelectedRestoreCommandTemplate,
        [string]$VisualReviewCommand,
        [string]$OpenVisualReviewCommand,
        [string]$SourceSchema,
        [string]$SourceReportDisplay,
        [string]$SourceJsonDisplay,
        [string]$RollbackPlanDisplay
    )

    $sourceFields = [ordered]@{
        source_schema = $SourceSchema
        source_report_display = $SourceReportDisplay
        source_json_display = $SourceJsonDisplay
        rollback_plan_display = $RollbackPlanDisplay
    }

    $steps = New-Object 'System.Collections.Generic.List[object]'
    $step = [ordered]@{
        order = 1
        id = "review_restore_audit_dry_run"
        action = "review_style_merge_restore_audit"
        status = "completed"
        reason = "style_merge_restore_audit_dry_run_completed"
        command = $RestoreAuditCommand
    }
    Add-StyleMergeRestoreHandoffStep -Steps $steps -Step $step -SourceFields $sourceFields

    if ($IssueCount -gt 0) {
        $issueReviewCommands = New-Object 'System.Collections.Generic.List[string]'
        foreach ($group in @($IssueReviewGroups)) {
            foreach ($command in @(Get-JsonProperty -Object $group -Name "review_commands")) {
                if (-not [string]::IsNullOrWhiteSpace([string]$command)) {
                    $issueReviewCommands.Add([string]$command) | Out-Null
                }
            }
        }
        $firstIssueReviewCommand = if ($issueReviewCommands.Count -gt 0) {
            [string]$issueReviewCommands[0]
        } else {
            $RestoreAuditCommand
        }

        $step = [ordered]@{
            order = 2
            id = "review_issue_groups"
            action = "review_style_merge_restore_audit_issues"
            status = "next"
            reason = "style_merge_restore_audit_issues_require_issue_group_review"
            issue_count = $IssueCount
            issue_group_count = @($IssueReviewGroups).Count
            command = $firstIssueReviewCommand
            audit_command = $RestoreAuditCommand
            first_review_command = $firstIssueReviewCommand
            copy_review_command = $firstIssueReviewCommand
            issue_review_commands = @($issueReviewCommands.ToArray())
        }
        Add-StyleMergeRestoreHandoffStep -Steps $steps -Step $step -SourceFields $sourceFields
        $step = [ordered]@{
            order = 3
            id = "prepare_word_visual_review_after_clean_audit"
            action = "prepare_word_visual_review"
            status = "blocked"
            reason = "style_merge_restore_audit_issues_block_word_visual_review"
            blocked_by = @("style_merge.restore_audit_issues")
            command = $VisualReviewCommand
            open_command = $OpenVisualReviewCommand
        }
        Add-StyleMergeRestoreHandoffStep -Steps $steps -Step $step -SourceFields $sourceFields
    } else {
        $step = [ordered]@{
            order = 2
            id = "prepare_word_visual_review"
            action = "prepare_word_visual_review"
            status = "next"
            reason = "style_merge_restore_audit_clean_requires_word_visual_review"
            command = $VisualReviewCommand
            open_command = $OpenVisualReviewCommand
        }
        Add-StyleMergeRestoreHandoffStep -Steps $steps -Step $step -SourceFields $sourceFields
        $step = [ordered]@{
            order = 3
            id = "restore_selected_style_merges"
            action = "restore_style_merge"
            status = "ready"
            reason = "style_merge_restore_ready_after_word_visual_review"
            command_template = $SelectedRestoreCommandTemplate
            restorable_rollback_entry_count = $RestorableRollbackCommandSummary.restorable_rollback_entry_count
            batch_restore_command_template = $RestorableRollbackCommandSummary.batch_restorable_restore_command_template
        }
        Add-StyleMergeRestoreHandoffStep -Steps $steps -Step $step -SourceFields $sourceFields
    }

    return @($steps.ToArray())
}

function Add-StyleMergeRestoreHandoffStep {
    param(
        [System.Collections.Generic.List[object]]$Steps,
        [System.Collections.Specialized.OrderedDictionary]$Step,
        [System.Collections.Specialized.OrderedDictionary]$SourceFields
    )

    foreach ($entry in $SourceFields.GetEnumerator()) {
        $Step[$entry.Key] = $entry.Value
    }

    $copyCommand = Get-JsonString -Object $Step -Names @("command", "command_template", "batch_restore_command_template")
    $Step["copy_command"] = $copyCommand
    $Steps.Add($Step) | Out-Null
}

function Get-NextStyleMergeRestoreHandoffStep {
    param($Steps)

    foreach ($step in @($Steps)) {
        $status = Get-JsonString -Object $step -Names @("status")
        if ($status -eq "next") {
            return $step
        }
    }
    return $null
}

function Get-StyleMergeRestoreIssueReviewCommands {
    param($IssueReviewGroups)

    $commands = New-Object 'System.Collections.Generic.List[string]'
    foreach ($group in @($IssueReviewGroups)) {
        foreach ($command in @(Get-JsonProperty -Object $group -Name "review_commands")) {
            if (-not [string]::IsNullOrWhiteSpace([string]$command)) {
                $commands.Add([string]$command) | Out-Null
            }
        }
    }

    return @($commands.ToArray())
}

function Get-StyleMergeRestoreIssueReviewGroupSummary {
    param(
        $IssueReviewGroups,
        [string[]]$IssueReviewCommands
    )

    $groups = New-Object 'System.Collections.Generic.List[object]'
    foreach ($group in @($IssueReviewGroups)) {
        if ($null -ne $group) {
            $groups.Add($group) | Out-Null
        }
    }

    $commands = New-Object 'System.Collections.Generic.List[string]'
    foreach ($command in @($IssueReviewCommands)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$command)) {
            $commands.Add([string]$command) | Out-Null
        }
    }

    $firstGroup = if ($groups.Count -gt 0) { $groups[0] } else { $null }
    $firstAffectedOperation = $null
    if ($null -ne $firstGroup) {
        $affectedOperations = @(Get-JsonProperty -Object $firstGroup -Name "affected_operations")
        if ($affectedOperations.Count -gt 0) {
            $firstAffectedOperation = $affectedOperations[0]
        }
    }

    $firstReviewCommand = if ($commands.Count -gt 0) { [string]$commands[0] } else { "" }
    return [ordered]@{
        issue_review_group_count = $groups.Count
        issue_review_command_count = $commands.Count
        has_issue_review_commands = ($commands.Count -gt 0)
        first_issue_code = Get-JsonString -Object $firstGroup -Names @("code")
        first_issue_count = Get-JsonInt -Object $firstGroup -Names @("count")
        first_affected_operation_count = Get-JsonInt -Object $firstGroup -Names @("affected_operation_count")
        first_source_style_id = Get-JsonString -Object $firstAffectedOperation -Names @("source_style_id")
        first_target_style_id = Get-JsonString -Object $firstAffectedOperation -Names @("target_style_id")
        first_review_command = $firstReviewCommand
        copy_review_command = $firstReviewCommand
    }
}

function Get-StyleMergeRestoreHandoffStatusSummary {
    param(
        $Steps,
        $NextHandoffStep,
        [string]$Status,
        [string]$StatusReason,
        [int]$IssueCount
    )

    $statusCounts = [ordered]@{
        completed = 0
        next = 0
        ready = 0
        blocked = 0
    }

    foreach ($step in @($Steps)) {
        $stepStatus = Get-JsonString -Object $step -Names @("status") -DefaultValue "unknown"
        if ($statusCounts.Contains($stepStatus)) {
            $statusCounts[$stepStatus] = [int]$statusCounts[$stepStatus] + 1
        }
    }

    return [ordered]@{
        status = $Status
        status_reason = $StatusReason
        issue_count = $IssueCount
        total_step_count = @($Steps).Count
        completed_step_count = [int]$statusCounts.completed
        next_step_count = [int]$statusCounts.next
        ready_step_count = [int]$statusCounts.ready
        blocked_step_count = [int]$statusCounts.blocked
        next_step_id = Get-JsonString -Object $NextHandoffStep -Names @("id")
        next_step_action = Get-JsonString -Object $NextHandoffStep -Names @("action")
        next_step_reason = Get-JsonString -Object $NextHandoffStep -Names @("reason")
        next_copy_command = Get-JsonString -Object $NextHandoffStep -Names @("copy_command", "command", "command_template")
    }
}

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
