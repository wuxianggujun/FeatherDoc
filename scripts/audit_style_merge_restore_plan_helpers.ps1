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

function Get-StyleMergeRestoreRestorableCommandSummary {
    param($RestorableRollbackCommandSummary)

    $dryRunCommands = @(Get-JsonProperty -Object $RestorableRollbackCommandSummary -Name "per_entry_dry_run_commands")
    $restoreCommandTemplates = @(Get-JsonProperty -Object $RestorableRollbackCommandSummary -Name "per_entry_restore_command_templates")
    $batchDryRunCommand = Get-JsonString -Object $RestorableRollbackCommandSummary -Names @("batch_restorable_dry_run_command")
    $batchRestoreCommandTemplate = Get-JsonString -Object $RestorableRollbackCommandSummary -Names @("batch_restorable_restore_command_template")
    $firstDryRunCommand = if ($dryRunCommands.Count -gt 0) {
        Get-JsonString -Object $dryRunCommands[0] -Names @("command")
    } else {
        ""
    }
    $firstRestoreCommandTemplate = if ($restoreCommandTemplates.Count -gt 0) {
        Get-JsonString -Object $restoreCommandTemplates[0] -Names @("command_template")
    } else {
        ""
    }

    return [ordered]@{
        restorable_rollback_entry_count = Get-JsonInt -Object $RestorableRollbackCommandSummary -Names @("restorable_rollback_entry_count")
        restorable_rollback_entry_indexes = @(Get-JsonProperty -Object $RestorableRollbackCommandSummary -Name "restorable_rollback_entry_indexes")
        per_entry_dry_run_command_count = $dryRunCommands.Count
        per_entry_restore_command_template_count = $restoreCommandTemplates.Count
        has_batch_restorable_dry_run_command = (-not [string]::IsNullOrWhiteSpace($batchDryRunCommand))
        has_batch_restorable_restore_command_template = (-not [string]::IsNullOrWhiteSpace($batchRestoreCommandTemplate))
        first_per_entry_dry_run_command = $firstDryRunCommand
        first_per_entry_restore_command_template = $firstRestoreCommandTemplate
        batch_restorable_dry_run_command = $batchDryRunCommand
        batch_restorable_restore_command_template = $batchRestoreCommandTemplate
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
