function Get-ReleaseBlockerPropertyObject {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return $Object[$Name]
        }

        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Get-ReleaseBlockerPropertyValue {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    $value = Get-ReleaseBlockerPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return ""
    }

    return [string]$value
}

function Get-ReleaseBlockerArrayProperty {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    $value = Get-ReleaseBlockerPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }
    if ($value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($value)) {
            return @()
        }

        return @($value)
    }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Get-ReleaseBlockers {
    param([AllowNull()]$Summary)

    return @(Get-ReleaseBlockerArrayProperty -Object $Summary -Name "release_blockers")
}

function Get-ReleaseBlockerDeclaredCount {
    param(
        [AllowNull()]$Summary,
        [string]$Context = "release summary"
    )

    $countObject = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_count"
    if ($null -eq $countObject) {
        return $null
    }

    $countValue = [string]$countObject
    if ([string]::IsNullOrWhiteSpace($countValue)) {
        return $null
    }

    try {
        $declaredCount = [int]$countValue
    } catch {
        throw "release_blocker_count must be an integer when provided in ${Context}; actual value: $countValue"
    }

    if ($declaredCount -lt 0) {
        throw "release_blocker_count must not be negative in ${Context}; actual value: $declaredCount"
    }

    return $declaredCount
}

function Assert-ReleaseBlockerCountConsistency {
    param(
        [AllowNull()]$Summary,
        [string]$Context = "release summary"
    )

    $declaredCount = Get-ReleaseBlockerDeclaredCount -Summary $Summary -Context $Context
    if ($null -eq $declaredCount) {
        return
    }

    $actualCount = @(Get-ReleaseBlockers -Summary $Summary).Count
    if ($declaredCount -ne $actualCount) {
        throw "release_blocker_count mismatch in ${Context}: declared $declaredCount but release_blockers contains $actualCount item(s)."
    }
}

function Assert-ReleaseBlockerMetadataQuality {
    param(
        [AllowNull()]$Summary,
        [string]$Context = "release summary"
    )

    Assert-ReleaseBlockerCountConsistency -Summary $Summary -Context $Context

    $requiredFields = @("id", "source", "status", "severity", "action")
    $seenIds = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    $releaseBlockers = @(Get-ReleaseBlockers -Summary $Summary)
    for ($blockerIndex = 0; $blockerIndex -lt $releaseBlockers.Count; $blockerIndex++) {
        $releaseBlocker = $releaseBlockers[$blockerIndex]
        foreach ($fieldName in $requiredFields) {
            $fieldValue = Get-ReleaseBlockerPropertyValue -Object $releaseBlocker -Name $fieldName
            if ([string]::IsNullOrWhiteSpace($fieldValue)) {
                throw "release_blockers[$blockerIndex].$fieldName must not be empty in ${Context}."
            }
        }

        $blockerId = Get-ReleaseBlockerPropertyValue -Object $releaseBlocker -Name "id"
        if (-not $seenIds.Add($blockerId)) {
            throw "duplicate release blocker id '$blockerId' in ${Context}."
        }
    }
}

function Get-ReleaseBlockerCount {
    param([AllowNull()]$Summary)

    $declaredCount = Get-ReleaseBlockerDeclaredCount -Summary $Summary
    if ($null -ne $declaredCount) {
        Assert-ReleaseBlockerCountConsistency -Summary $Summary
        return $declaredCount
    }

    return @(Get-ReleaseBlockers -Summary $Summary).Count
}

function Get-ReleaseBlockerDisplayValue {
    param(
        [AllowNull()]$Value,
        [string]$Fallback = "(not available)"
    )

    if ($null -eq $Value -or [string]::IsNullOrWhiteSpace([string]$Value)) {
        return $Fallback
    }

    return [string]$Value
}

function Get-ReleaseBlockerDisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    try {
        $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
            $Path
        } elseif (-not [string]::IsNullOrWhiteSpace($RepoRoot)) {
            Join-Path $RepoRoot $Path
        } else {
            $Path
        }
        $resolvedPath = [System.IO.Path]::GetFullPath($candidate)
        if (-not [string]::IsNullOrWhiteSpace($RepoRoot)) {
            $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
            if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
                $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
                if ([string]::IsNullOrWhiteSpace($relative)) {
                    return "."
                }

                return ".\" + ($relative -replace '/', '\')
            }
        }

        return $resolvedPath
    } catch {
        return $Path
    }
}

function Join-ReleaseBlockerValues {
    param([object[]]$Values)

    $normalizedValues = @($Values |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
        ForEach-Object { [string]$_ } |
        Sort-Object -Unique)
    if ($normalizedValues.Count -eq 0) {
        return "(none)"
    }

    return ($normalizedValues -join ",")
}

function Get-ReleaseBlockerSummaryText {
    param([AllowNull()]$Blocker)

    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "id") -Fallback "(unknown)"
    $status = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "status")
    $severity = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "severity")
    $source = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source")
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "action")
    $blockedItemCount = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "blocked_item_count") -Fallback "0"
    $issueKeys = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $Blocker -Name "issue_keys")

    return "{0}: status={1} severity={2} source={3} action={4} blocked_items={5} issues={6}" -f `
        $id, $status, $severity, $source, $action, $blockedItemCount, $issueKeys
}

function Get-ReleaseBlockerItemSummaryText {
    param([AllowNull()]$Item)

    $name = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "name") -Fallback "(unknown item)"
    $status = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "status")
    $decision = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "decision")
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "action")
    $issueKeys = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "compliance_issues")

    return "{0}: status={1} decision={2} action={3} issues={4}" -f `
        $name, $status, $decision, $action, $issueKeys
}

function Add-ReleaseBlockerActionGuidanceLine {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Text
    )

    if (-not [string]::IsNullOrWhiteSpace($Text)) {
        [void]$Lines.Add($Text)
    }
}

function Get-ReleaseBlockerRegisteredActions {
    return @(
        "complete_visual_manual_review",
        "fix_schema_patch_approval_result"
    )
}

function Test-ReleaseBlockerActionRegistered {
    param([string]$Action)

    if ([string]::IsNullOrWhiteSpace($Action)) {
        return $false
    }

    foreach ($registeredAction in @(Get-ReleaseBlockerRegisteredActions)) {
        if ([string]::Equals($registeredAction, $Action, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

function Get-ReleaseBlockerActionGuidanceLines {
    param(
        [AllowNull()]$Blocker,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    $action = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "action"
    $guidanceLines = New-Object 'System.Collections.Generic.List[string]'
    if (-not (Test-ReleaseBlockerActionRegistered -Action $action)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Unregistered release blocker action `{0}`: add a fixed checklist runbook in `release_blocker_metadata_helpers.ps1`; until then, follow the blocker summary, update the owning evidence, rerun the relevant release checks, and regenerate the release note bundle before public release.' -f $action)
        return $guidanceLines.ToArray()
    }

    switch ($action) {
        "fix_schema_patch_approval_result" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Use action `fix_schema_patch_approval_result`: update blocked `schema_patch_approval_result.json` record(s), provide reviewer and reviewed_at for non-pending decisions, then refresh release metadata.'

            $historyMarkdown = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "history_markdown"
            if (-not [string]::IsNullOrWhiteSpace($historyMarkdown)) {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ("Open the schema approval history markdown before editing approval records: {0}" -f (Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $historyMarkdown))
            }

            $summaryJson = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "summary_json"
            $displaySummaryJson = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $summaryJson
            $displayReleaseSummaryJson = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Run `sync_project_template_schema_approval.ps1` after updating approval records, using project template summary `{0}` and release summary `{1}`, then regenerate the release note bundle.' -f $displaySummaryJson, $displayReleaseSummaryJson)
            break
        }
        "complete_visual_manual_review" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Use action `complete_visual_manual_review`: open the referenced visual review task, record the reviewer verdict and reviewed_at, then resync the visual verdict metadata before public release.'
            break
        }
        default {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Registered release blocker action `{0}` does not have a checklist runbook yet; add one in `release_blocker_metadata_helpers.ps1` before relying on this action for handoff.' -f $action)
        }
    }

    return $guidanceLines.ToArray()
}

function Add-ReleaseBlockerMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$RepoRoot,
        [string]$Heading = "## Release Blockers",
        [switch]$IncludeWhenEmpty
    )

    Assert-ReleaseBlockerMetadataQuality -Summary $Summary -Context $Heading
    $releaseBlockerCount = Get-ReleaseBlockerCount -Summary $Summary
    $releaseBlockers = @(Get-ReleaseBlockers -Summary $Summary)
    if ($releaseBlockerCount -le 0 -and -not $IncludeWhenEmpty.IsPresent) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    [void]$Lines.Add("- Release blockers: $releaseBlockerCount")
    if ($releaseBlockerCount -le 0) {
        return
    }

    foreach ($releaseBlocker in $releaseBlockers) {
        [void]$Lines.Add("- $(Get-ReleaseBlockerSummaryText -Blocker $releaseBlocker)")
        $message = Get-ReleaseBlockerPropertyValue -Object $releaseBlocker -Name "message"
        if (-not [string]::IsNullOrWhiteSpace($message)) {
            [void]$Lines.Add("  - Message: $message")
        }

        foreach ($pathInfo in @(
                @{ Name = "summary_json"; Label = "Summary JSON" },
                @{ Name = "history_json"; Label = "History JSON" },
                @{ Name = "history_markdown"; Label = "History Markdown" }
            )) {
            $pathValue = Get-ReleaseBlockerPropertyValue -Object $releaseBlocker -Name $pathInfo.Name
            if (-not [string]::IsNullOrWhiteSpace($pathValue)) {
                [void]$Lines.Add("  - $($pathInfo.Label): $(Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $pathValue)")
            }
        }

        foreach ($item in @(Get-ReleaseBlockerArrayProperty -Object $releaseBlocker -Name "items")) {
            [void]$Lines.Add("  - Item $(Get-ReleaseBlockerItemSummaryText -Item $item)")
        }
    }
}

function Get-ReleaseGovernanceRollup {
    param([AllowNull()]$Summary)

    return Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup"
}

function Get-ReleaseGovernanceHandoff {
    param([AllowNull()]$Summary)

    return Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff"
}

function Add-ReleaseGovernanceRollupSourceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$RepoRoot
    )

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerDisplayPath `
            -RepoRoot $RepoRoot `
            -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report")
    }

    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath `
            -RepoRoot $RepoRoot `
            -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json")
    }

    [void]$Lines.Add("  - source_report_display: $sourceReportDisplay")
    [void]$Lines.Add("  - source_json_display: $sourceJsonDisplay")
}

function Add-ReleaseGovernanceRepairLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item
    )

    foreach ($fieldName in @("repair_strategy", "repair_hint", "command_template")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$Lines.Add("  - ${fieldName}: $value")
        }
    }
}

function Get-ReleaseGovernanceChecklistSections {
    param([AllowNull()]$Summary)

    $handoff = Get-ReleaseGovernanceHandoff -Summary $Summary
    return @(
        [pscustomobject]@{
            Context = "Release blocker rollup"
            SummaryObject = Get-ReleaseGovernanceRollup -Summary $Summary
        },
        [pscustomobject]@{
            Context = "Release governance handoff"
            SummaryObject = $handoff
        },
        [pscustomobject]@{
            Context = "Release governance handoff nested rollup"
            SummaryObject = Get-ReleaseBlockerPropertyObject -Object $handoff -Name "release_blocker_rollup"
        }
    )
}

function Get-NormalizedReleaseGovernanceBlockers {
    param([AllowNull()]$Blockers)

    $normalizedBlockers = New-Object 'System.Collections.Generic.List[object]'
    foreach ($blocker in @(Get-ReleaseBlockerArrayProperty -Object ([ordered]@{ release_blockers = $Blockers }) -Name "release_blockers")) {
        if ($null -eq $blocker) {
            continue
        }

        [void]$normalizedBlockers.Add([pscustomobject][ordered]@{
                composite_id = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "composite_id")
                id = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id")
                action = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")
                status = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "status")
                severity = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "severity")
                source_schema = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema")
                source_report_display = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_report_display")
                message = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message")
            })
    }

    return @($normalizedBlockers.ToArray())
}

function Get-ReleaseGovernanceBlockerCount {
    param([AllowNull()]$SummaryObject)

    $declaredCount = Get-ReleaseBlockerPropertyObject -Object $SummaryObject -Name "release_blocker_count"
    if ($null -ne $declaredCount -and -not [string]::IsNullOrWhiteSpace([string]$declaredCount)) {
        try {
            return [int][string]$declaredCount
        } catch {
            # Fall back to the materialized array when older fixtures used
            # non-integer placeholders.
        }
    }

    return @(Get-NormalizedReleaseGovernanceBlockers -Blockers (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "release_blockers")).Count
}

function Get-NormalizedReleaseGovernanceWarnings {
    param([AllowNull()]$Warnings)

    $normalizedWarnings = New-Object 'System.Collections.Generic.List[object]'
    foreach ($warning in @(Get-ReleaseBlockerArrayProperty -Object ([ordered]@{ warnings = $Warnings }) -Name "warnings")) {
        if ($null -eq $warning) {
            continue
        }

        $entry = [ordered]@{
            id = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "id")
            action = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")
            message = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "message")
            source_schema = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema")
            source_report_display = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_report_display")
            source_json_display = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_json_display")
        }

        $styleMergeSuggestionCount = Get-ReleaseBlockerPropertyObject -Object $warning -Name "style_merge_suggestion_count"
        if ($null -ne $styleMergeSuggestionCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionCount)) {
            $entry.style_merge_suggestion_count = [int]$styleMergeSuggestionCount
        }
        $styleMergeSuggestionPendingCount = Get-ReleaseBlockerPropertyObject -Object $warning -Name "style_merge_suggestion_pending_count"
        if ($null -ne $styleMergeSuggestionPendingCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionPendingCount)) {
            $entry.style_merge_suggestion_pending_count = [int]$styleMergeSuggestionPendingCount
        }

        [void]$normalizedWarnings.Add([pscustomobject]$entry)
    }

    return @($normalizedWarnings.ToArray())
}

function Get-ReleaseGovernanceWarningCount {
    param([AllowNull()]$SummaryObject)

    $declaredCount = Get-ReleaseBlockerPropertyObject -Object $SummaryObject -Name "warning_count"
    if ($null -ne $declaredCount -and -not [string]::IsNullOrWhiteSpace([string]$declaredCount)) {
        try {
            return [int][string]$declaredCount
        } catch {
            # Fall back to the materialized warnings array for legacy fixtures.
        }
    }

    return @(Get-NormalizedReleaseGovernanceWarnings -Warnings (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "warnings")).Count
}

function Get-NormalizedReleaseGovernanceActionItems {
    param([AllowNull()]$ActionItems)

    $normalizedActions = New-Object 'System.Collections.Generic.List[object]'
    foreach ($item in @(Get-ReleaseBlockerArrayProperty -Object ([ordered]@{ action_items = $ActionItems }) -Name "action_items")) {
        if ($null -eq $item) {
            continue
        }

        $entry = [ordered]@{
            id = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "id")
            action = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "action")
            title = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "title")
            source_schema = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema")
            source_report_display = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_report_display")
            command = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "command")
        }

        foreach ($optionalCommandName in @("open_command", "audit_command", "review_command")) {
            $optionalCommand = Get-ReleaseBlockerPropertyValue -Object $item -Name $optionalCommandName
            if (-not [string]::IsNullOrWhiteSpace($optionalCommand)) {
                $entry[$optionalCommandName] = [string]$optionalCommand
            }
        }

        [void]$normalizedActions.Add([pscustomobject]$entry)
    }

    return @($normalizedActions.ToArray())
}

function Get-ReleaseGovernanceActionItemCount {
    param([AllowNull()]$SummaryObject)

    $declaredCount = Get-ReleaseBlockerPropertyObject -Object $SummaryObject -Name "action_item_count"
    if ($null -ne $declaredCount -and -not [string]::IsNullOrWhiteSpace([string]$declaredCount)) {
        try {
            return [int][string]$declaredCount
        } catch {
            # Fall back to the materialized action_items array for legacy fixtures.
        }
    }

    return @(Get-NormalizedReleaseGovernanceActionItems -ActionItems (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "action_items")).Count
}

function Get-ReleaseGovernanceActionItemSummaryText {
    param([AllowNull()]$ActionItem)

    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "action")
    $title = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "title")
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "source_schema")
    $summaryText = ('id: `{0}`; action: `{1}`; title: {2}; source_schema: `{3}`' -f `
            $id, $action, $title, $sourceSchema)

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $summaryText += ('; source_report: `{0}`' -f $sourceReportDisplay)
    }

    return $summaryText
}

function Get-ReleaseGovernanceWarningSummaryText {
    param([AllowNull()]$Warning)

    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "action")
    $message = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "message")
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "source_schema")
    $summaryText = ('id: `{0}`; action: `{1}`; message: {2}; source_schema: `{3}`' -f `
            $id, $action, $message, $sourceSchema)

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Warning -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Warning -Name "source_json_display"
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $summaryText += ('; source_report: `{0}`' -f $sourceReportDisplay)
    }

    $styleMergeSuggestionCount = Get-ReleaseBlockerPropertyObject -Object $Warning -Name "style_merge_suggestion_count"
    if ($null -ne $styleMergeSuggestionCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionCount)) {
        $summaryText += ('; style_merge_suggestion_count: `{0}`' -f [string]$styleMergeSuggestionCount)
    }
    $styleMergeSuggestionPendingCount = Get-ReleaseBlockerPropertyObject -Object $Warning -Name "style_merge_suggestion_pending_count"
    if ($null -ne $styleMergeSuggestionPendingCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionPendingCount)) {
        $summaryText += ('; style_merge_suggestion_pending_count: `{0}`' -f [string]$styleMergeSuggestionPendingCount)
    }

    return $summaryText
}

function Get-ReleaseGovernanceBlockerSummaryText {
    param([AllowNull()]$Blocker)

    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "id") -Fallback "(unknown)"
    $summaryText = ('id: `{0}`; action: `{1}`' -f `
            $id,
            (Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "action")))

    $compositeId = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "composite_id"
    if (-not [string]::IsNullOrWhiteSpace($compositeId)) {
        $summaryText += ('; composite_id: `{0}`' -f $compositeId)
    }

    foreach ($field in @("status", "severity", "source_schema", "source_report_display")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name $field
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; {0}: `{1}`' -f $field, $value)
        }
    }

    $message = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "message"
    if (-not [string]::IsNullOrWhiteSpace($message)) {
        $summaryText += ('; message: {0}' -f $message)
    }

    return $summaryText
}

function Add-ReleaseGovernanceBlockerMarkdownSubsection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Heading,
        [AllowNull()]$SummaryObject,
        [switch]$IncludeWhenEmpty
    )

    $releaseBlockers = @(Get-NormalizedReleaseGovernanceBlockers -Blockers (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "release_blockers"))
    $releaseBlockerCount = Get-ReleaseGovernanceBlockerCount -SummaryObject $SummaryObject
    if ($releaseBlockerCount -le 0 -and $releaseBlockers.Count -le 0 -and -not $IncludeWhenEmpty.IsPresent) {
        return $false
    }

    [void]$Lines.Add("### $Heading")
    [void]$Lines.Add("")
    [void]$Lines.Add(('- release_blocker_count: `{0}`' -f $releaseBlockerCount))
    foreach ($releaseBlocker in $releaseBlockers) {
        [void]$Lines.Add("- $(Get-ReleaseGovernanceBlockerSummaryText -Blocker $releaseBlocker)")
    }
    [void]$Lines.Add("")

    return $true
}

function Add-ReleaseGovernanceBlockersMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [switch]$IncludeWhenEmpty
    )

    $sectionLines = New-Object 'System.Collections.Generic.List[string]'
    $hadSection = $false
    foreach ($sectionInfo in @(Get-ReleaseGovernanceChecklistSections -Summary $Summary)) {
        if (Add-ReleaseGovernanceBlockerMarkdownSubsection `
                -Lines $sectionLines `
                -Heading "$($sectionInfo.Context) blockers" `
                -SummaryObject $sectionInfo.SummaryObject `
                -IncludeWhenEmpty:$IncludeWhenEmpty.IsPresent) {
            $hadSection = $true
        }
    }

    if (-not $hadSection) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add("## Release Governance Blockers")
    [void]$Lines.Add("")
    foreach ($line in $sectionLines) {
        [void]$Lines.Add($line)
    }
}

function Add-ReleaseGovernanceActionItemMarkdownSubsection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Heading,
        [AllowNull()]$SummaryObject,
        [switch]$IncludeWhenEmpty
    )

    $actionItems = @(Get-NormalizedReleaseGovernanceActionItems -ActionItems (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "action_items"))
    $actionItemCount = Get-ReleaseGovernanceActionItemCount -SummaryObject $SummaryObject
    if ($actionItemCount -le 0 -and $actionItems.Count -le 0 -and -not $IncludeWhenEmpty.IsPresent) {
        return $false
    }

    [void]$Lines.Add("### $Heading")
    [void]$Lines.Add("")
    [void]$Lines.Add(('- action_item_count: `{0}`' -f $actionItemCount))
    foreach ($item in $actionItems) {
        [void]$Lines.Add("- $(Get-ReleaseGovernanceActionItemSummaryText -ActionItem $item)")
        foreach ($commandName in @("command", "open_command", "audit_command", "review_command")) {
            $commandValue = Get-ReleaseBlockerPropertyValue -Object $item -Name $commandName
            if (-not [string]::IsNullOrWhiteSpace($commandValue)) {
                [void]$Lines.Add("  - ${commandName}: ``$commandValue``")
            }
        }
    }
    [void]$Lines.Add("")

    return $true
}

function Add-ReleaseGovernanceActionItemsMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [switch]$IncludeWhenEmpty
    )

    $sectionLines = New-Object 'System.Collections.Generic.List[string]'
    $hadSection = $false
    foreach ($sectionInfo in @(Get-ReleaseGovernanceChecklistSections -Summary $Summary)) {
        if (Add-ReleaseGovernanceActionItemMarkdownSubsection `
                -Lines $sectionLines `
                -Heading "$($sectionInfo.Context) action items" `
                -SummaryObject $sectionInfo.SummaryObject `
                -IncludeWhenEmpty:$IncludeWhenEmpty.IsPresent) {
            $hadSection = $true
        }
    }

    if (-not $hadSection) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add("## Release Governance Action Items")
    [void]$Lines.Add("")
    foreach ($line in $sectionLines) {
        [void]$Lines.Add($line)
    }
}

function Add-ReleaseGovernanceWarningMarkdownSubsection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Heading,
        [AllowNull()]$SummaryObject,
        [switch]$IncludeWhenEmpty
    )

    $warnings = @(Get-NormalizedReleaseGovernanceWarnings -Warnings (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "warnings"))
    $warningCount = Get-ReleaseGovernanceWarningCount -SummaryObject $SummaryObject
    if ($warningCount -le 0 -and $warnings.Count -le 0 -and -not $IncludeWhenEmpty.IsPresent) {
        return $false
    }

    [void]$Lines.Add("### $Heading")
    [void]$Lines.Add("")
    [void]$Lines.Add(('- warning_count: `{0}`' -f $warningCount))
    foreach ($warning in $warnings) {
        [void]$Lines.Add("- $(Get-ReleaseGovernanceWarningSummaryText -Warning $warning)")
    }
    [void]$Lines.Add("")

    return $true
}

function Add-ReleaseGovernanceWarningsMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [switch]$IncludeWhenEmpty
    )

    $sectionLines = New-Object 'System.Collections.Generic.List[string]'
    $hadSection = $false
    foreach ($sectionInfo in @(Get-ReleaseGovernanceChecklistSections -Summary $Summary)) {
        if (Add-ReleaseGovernanceWarningMarkdownSubsection `
                -Lines $sectionLines `
                -Heading "$($sectionInfo.Context) warnings" `
                -SummaryObject $sectionInfo.SummaryObject `
                -IncludeWhenEmpty:$IncludeWhenEmpty.IsPresent) {
            $hadSection = $true
        }
    }

    if (-not $hadSection) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add("## Release Governance Warnings")
    [void]$Lines.Add("")
    foreach ($line in $sectionLines) {
        [void]$Lines.Add($line)
    }
}

function Get-ReleaseGovernanceChecklistItems {
    param(
        [AllowNull()]$Summary,
        [string]$ArrayPropertyName,
        [string]$ContextSuffix
    )

    $items = New-Object 'System.Collections.Generic.List[object]'
    foreach ($sectionInfo in @(Get-ReleaseGovernanceChecklistSections -Summary $Summary)) {
        if ($null -eq $sectionInfo.SummaryObject) {
            continue
        }

        foreach ($item in @(Get-ReleaseBlockerArrayProperty -Object $sectionInfo.SummaryObject -Name $ArrayPropertyName)) {
            [void]$items.Add([pscustomobject]@{
                    context = "$($sectionInfo.Context) $ContextSuffix"
                    item = $item
                })
        }
    }

    return @($items.ToArray())
}

function Get-ReleaseGovernanceBlockerChecklistItems {
    param([AllowNull()]$Summary)

    return @(Get-ReleaseGovernanceChecklistItems `
            -Summary $Summary `
            -ArrayPropertyName "release_blockers" `
            -ContextSuffix "blockers")
}

function Get-ReleaseGovernanceActionItemChecklistItems {
    param([AllowNull()]$Summary)

    return @(Get-ReleaseGovernanceChecklistItems `
            -Summary $Summary `
            -ArrayPropertyName "action_items" `
            -ContextSuffix "action items")
}

function Get-ReleaseGovernanceWarningChecklistItems {
    param([AllowNull()]$Summary)

    return @(Get-ReleaseGovernanceChecklistItems `
            -Summary $Summary `
            -ArrayPropertyName "warnings" `
            -ContextSuffix "warnings")
}

function Get-ReleaseGovernanceChecklistSourceDisplay {
    param(
        [AllowNull()]$Item,
        [string]$RepoRoot,
        [string]$DisplayPropertyName,
        [string]$PathPropertyName
    )

    $displayValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $DisplayPropertyName
    if (-not [string]::IsNullOrWhiteSpace($displayValue)) {
        return $displayValue
    }

    $pathValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $PathPropertyName
    if ([string]::IsNullOrWhiteSpace($pathValue)) {
        return ""
    }

    return Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $pathValue
}

function Get-ReleaseGovernanceBlockerChecklistText {
    param([AllowNull()]$BlockerItem)

    $context = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $BlockerItem -Name "context")
    $blocker = Get-ReleaseBlockerPropertyObject -Object $BlockerItem -Name "item"
    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")
    $status = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "status")
    $severity = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "severity")
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema")
    $message = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message")
    $text = 'Resolve release governance blocker `{0}` ({1}): action `{2}`, status `{3}`, severity `{4}`, source_schema `{5}`, message: {6}' -f `
        $id, $context, $action, $status, $severity, $sourceSchema, $message

    $reportId = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "report_id"
    if (-not [string]::IsNullOrWhiteSpace($reportId)) {
        $text += ('; report_id `{0}`' -f $reportId)
    }

    $compositeId = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "composite_id"
    if (-not [string]::IsNullOrWhiteSpace($compositeId)) {
        $text += ('; composite_id `{0}`' -f $compositeId)
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $text += ('; source_report `{0}`' -f $sourceReportDisplay)
    }

    return $text
}

function Get-ReleaseGovernanceActionItemChecklistText {
    param([AllowNull()]$ActionItem)

    $context = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "context")
    $item = Get-ReleaseBlockerPropertyObject -Object $ActionItem -Name "item"
    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "action")
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema")
    $title = Get-ReleaseBlockerPropertyValue -Object $item -Name "title"
    $text = 'Review release governance action item `{0}` ({1}): action `{2}`, source_schema `{3}`' -f `
        $id, $context, $action, $sourceSchema

    if (-not [string]::IsNullOrWhiteSpace($title)) {
        $text += ('; title: {0}' -f $title)
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $item -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $text += ('; source_report `{0}`' -f $sourceReportDisplay)
    }

    return $text
}

function Get-ReleaseGovernanceWarningChecklistText {
    param([AllowNull()]$WarningItem)

    $context = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $WarningItem -Name "context")
    $warning = Get-ReleaseBlockerPropertyObject -Object $WarningItem -Name "item"
    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema")
    $message = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "message")
    $text = 'Review release governance warning `{0}` ({1}): action `{2}`, source_schema `{3}`, message: {4}' -f `
        $id, $context, $action, $sourceSchema, $message

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $text += ('; source_report `{0}`' -f $sourceReportDisplay)
    }

    return $text
}

function Get-ReleaseGovernanceChecklistGuidanceLines {
    param(
        [AllowNull()]$Item,
        [string]$ItemKind,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    $guidanceLines = New-Object 'System.Collections.Generic.List[string]'
    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerPropertyValue -Object $Item -Name "action"
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_schema")
    $sourceReportDisplay = Get-ReleaseGovernanceChecklistSourceDisplay `
        -Item $Item `
        -RepoRoot $RepoRoot `
        -DisplayPropertyName "source_report_display" `
        -PathPropertyName "source_report"
    $sourceJsonDisplay = Get-ReleaseGovernanceChecklistSourceDisplay `
        -Item $Item `
        -RepoRoot $RepoRoot `
        -DisplayPropertyName "source_json_display" `
        -PathPropertyName "source_json"
    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson

    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Open source report `{0}` while handling release governance {1} `{2}`.' -f $sourceReportDisplay, $ItemKind, $id)
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay) -and
        -not [string]::Equals($sourceJsonDisplay, $sourceReportDisplay, [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Open source JSON `{0}` while handling release governance {1} `{2}`.' -f $sourceJsonDisplay, $ItemKind, $id)
    }

    $hadCommand = $false
    foreach ($commandName in @("command", "open_command", "audit_command", "review_command")) {
        $commandValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $commandName
        if (-not [string]::IsNullOrWhiteSpace($commandValue)) {
            $hadCommand = $true
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Run `{0}` for release governance {1} `{2}`: `{3}`' -f $commandName, $ItemKind, $id, $commandValue)
        }
    }

    foreach ($repairField in @("repair_strategy", "repair_hint", "command_template")) {
        $repairValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $repairField
        if (-not [string]::IsNullOrWhiteSpace($repairValue)) {
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Review `{0}` for release governance {1} `{2}`: {3}' -f $repairField, $ItemKind, $id, $repairValue)
        }
    }

    if (-not $hadCommand) {
        if ([string]::IsNullOrWhiteSpace($action)) {
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Action is missing for release governance {0} `{1}` from source_schema `{2}`; update the owning governance report, rerun release governance checks, and regenerate the release note bundle from `{3}` before publishing.' -f $ItemKind, $id, $sourceSchema, $releaseSummaryDisplay)
        } else {
            if ([string]::Equals($ItemKind, "warning", [System.StringComparison]::OrdinalIgnoreCase) -and
                [string]::Equals($action, "review_style_merge_suggestions", [System.StringComparison]::OrdinalIgnoreCase)) {
                Add-ReleaseBlockerActionGuidanceLine `
                    -Lines $guidanceLines `
                    -Text ('Use action `{0}` for release governance warning `{1}` before publishing.' -f $action, $id)
                $pendingCount = Get-ReleaseBlockerPropertyObject -Object $Item -Name "style_merge_suggestion_pending_count"
                if ($null -ne $pendingCount -and -not [string]::IsNullOrWhiteSpace([string]$pendingCount)) {
                    Add-ReleaseBlockerActionGuidanceLine `
                        -Lines $guidanceLines `
                        -Text ('Current pending style merge suggestion count is `{0}`.' -f [string]$pendingCount)
                }
            }

            $actionLabel = if ([string]::Equals($ItemKind, "warning", [System.StringComparison]::OrdinalIgnoreCase)) {
                "warning action"
            } elseif ([string]::Equals($ItemKind, "blocker", [System.StringComparison]::OrdinalIgnoreCase)) {
                "blocker action"
            } else {
                "action"
            }
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Follow {0} `{1}` for release governance {2} `{3}` from source_schema `{4}`: update the owning governance evidence, rerun release governance checks, and regenerate the release note bundle from `{5}` before publishing.' -f $actionLabel, $action, $ItemKind, $id, $sourceSchema, $releaseSummaryDisplay)
        }
    }

    return $guidanceLines.ToArray()
}

function Get-ReleaseGovernanceBlockerActionGuidanceLines {
    param(
        [AllowNull()]$Blocker,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    return @(Get-ReleaseGovernanceChecklistGuidanceLines `
            -Item $Blocker `
            -ItemKind "blocker" `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson)
}

function Get-ReleaseGovernanceActionItemActionGuidanceLines {
    param(
        [AllowNull()]$ActionItem,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    return @(Get-ReleaseGovernanceChecklistGuidanceLines `
            -Item $ActionItem `
            -ItemKind "action item" `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson)
}

function Get-ReleaseGovernanceWarningActionGuidanceLines {
    param(
        [AllowNull()]$Warning,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    return @(Get-ReleaseGovernanceChecklistGuidanceLines `
            -Item $Warning `
            -ItemKind "warning" `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson)
}

function Add-ReleaseGovernanceMetricsMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$Heading = "### Governance Metrics"
    )

    $metrics = @(Get-ReleaseBlockerArrayProperty -Object $Summary -Name "governance_metrics")
    if ($metrics.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    foreach ($metric in $metrics) {
        $reportId = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "report_id")
        $metricName = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "metric")
        $metricId = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "id")
        $level = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "level")
        $score = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "score")
        $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_schema")
        $sourceReportDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_report_display")
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_json_display")

        [void]$Lines.Add("- ${metricId}: report=$reportId metric=$metricName level=$level score=$score source_schema=$sourceSchema")
        [void]$Lines.Add("  - source_report_display: $sourceReportDisplay")
        [void]$Lines.Add("  - source_json_display: $sourceJsonDisplay")
        Add-ReleaseGovernanceMetricDetailLines -Lines $Lines -Metric $metric
    }
}

function Add-ReleaseGovernanceMetricDetailLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Metric
    )

    $details = Get-ReleaseBlockerPropertyObject -Object $Metric -Name "details"
    if ($null -eq $details) {
        return
    }

    $detailFields = @(
        "document_count",
        "catalog_exemplar_count",
        "baseline_entry_count",
        "matched_document_count",
        "unmatched_catalog_document_count",
        "unmatched_baseline_document_count",
        "alignment_gap_count",
        "catalog_coverage_percent",
        "baseline_coverage_percent",
        "coverage_score",
        "ready_document_count",
        "ready_document_percent",
        "needs_review_document_count",
        "failed_document_count",
        "table_style_issue_count",
        "automatic_tblLook_fix_count",
        "manual_table_style_fix_count",
        "table_position_automatic_count",
        "table_position_review_count",
        "command_failure_count",
        "unresolved_item_count"
    )
    $detailParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in $detailFields) {
        $value = Get-ReleaseBlockerPropertyObject -Object $details -Name $fieldName
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            [void]$detailParts.Add("$fieldName=$value")
        }
    }
    if ($detailParts.Count -gt 0) {
        [void]$Lines.Add("  - details: $($detailParts -join ', ')")
    }

    $penaltyParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($penalty in @(Get-ReleaseBlockerArrayProperty -Object $details -Name "penalty_summary")) {
        $factor = Get-ReleaseBlockerPropertyValue -Object $penalty -Name "factor"
        if ([string]::IsNullOrWhiteSpace($factor)) {
            continue
        }

        $count = Get-ReleaseBlockerPropertyObject -Object $penalty -Name "count"
        $penaltyValue = Get-ReleaseBlockerPropertyObject -Object $penalty -Name "penalty"
        [void]$penaltyParts.Add("$factor(count=$count, penalty=$penaltyValue)")
    }
    if ($penaltyParts.Count -gt 0) {
        [void]$Lines.Add("  - penalty_summary: $($penaltyParts -join '; ')")
    }
}

function Add-ReleaseGovernanceRollupMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$RepoRoot,
        [string]$Heading = "## Release Governance Rollup Details"
    )

    $rollup = Get-ReleaseGovernanceRollup -Summary $Summary
    if ($null -eq $rollup) {
        return
    }

    $requested = Get-ReleaseBlockerPropertyValue -Object $rollup -Name "requested"
    $status = Get-ReleaseBlockerPropertyValue -Object $rollup -Name "status"
    $releaseBlockers = @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "release_blockers")
    $warnings = @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "warnings")
    $actionItems = @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "action_items")
    if ($requested -eq "False" -and
        $status -eq "not_requested" -and
        $releaseBlockers.Count -eq 0 -and
        $warnings.Count -eq 0 -and
        $actionItems.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    [void]$Lines.Add("- Status: $(Get-ReleaseBlockerDisplayValue -Value $status)")
    [void]$Lines.Add("- Source reports: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $rollup -Name "source_report_count") -Fallback "0")")
    [void]$Lines.Add("- Blockers: $($releaseBlockers.Count)")
    [void]$Lines.Add("- Warnings: $($warnings.Count)")
    [void]$Lines.Add("- Action items: $($actionItems.Count)")

    Add-ReleaseGovernanceMetricsMarkdownSection -Lines $Lines -Summary $rollup

    if ($releaseBlockers.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Rollup Blockers")
        [void]$Lines.Add("")
        foreach ($blocker in $releaseBlockers) {
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id") -Fallback "(unknown blocker)"): action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $blocker -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $blocker
        }
    }

    if ($warnings.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Rollup Warnings")
        [void]$Lines.Add("")
        foreach ($warning in $warnings) {
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "id") -Fallback "(unknown warning)"): action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $warning -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $warning -RepoRoot $RepoRoot
        }
    }

    if ($actionItems.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Rollup Action Items")
        [void]$Lines.Add("")
        foreach ($item in $actionItems) {
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "id") -Fallback "(unknown action item)"): action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema"))")
            $openCommand = Get-ReleaseBlockerPropertyValue -Object $item -Name "open_command"
            if (-not [string]::IsNullOrWhiteSpace($openCommand)) {
                [void]$Lines.Add("  - open_command: $openCommand")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $item -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $item
        }
    }
}

function Add-ReleaseGovernanceHandoffMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$RepoRoot,
        [string]$Heading = "## Release Governance Handoff Details"
    )

    $handoff = Get-ReleaseGovernanceHandoff -Summary $Summary
    if ($null -eq $handoff) {
        return
    }

    $requested = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "requested"
    $status = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "status"
    $releaseBlockers = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "release_blockers")
    $warnings = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "warnings")
    $actionItems = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "action_items")
    if ($requested -eq "False" -and
        $status -eq "not_requested" -and
        $releaseBlockers.Count -eq 0 -and
        $warnings.Count -eq 0 -and
        $actionItems.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    [void]$Lines.Add("- Status: $(Get-ReleaseBlockerDisplayValue -Value $status)")
    [void]$Lines.Add("- Reports loaded: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "loaded_report_count") -Fallback "0") / $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "expected_report_count") -Fallback "0")")
    [void]$Lines.Add("- Missing reports: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "missing_report_count") -Fallback "0")")
    [void]$Lines.Add("- Blockers: $($releaseBlockers.Count)")
    [void]$Lines.Add("- Warnings: $($warnings.Count)")
    [void]$Lines.Add("- Action items: $($actionItems.Count)")

    Add-ReleaseGovernanceMetricsMarkdownSection -Lines $Lines -Summary $handoff

    if ($releaseBlockers.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Handoff Blockers")
        [void]$Lines.Add("")
        foreach ($blocker in $releaseBlockers) {
            $reportId = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "report_id"
            $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id") -Fallback "(unknown blocker)"
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value $reportId -Fallback "(unknown report)") / ${id}: action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $blocker -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $blocker
        }
    }

    if ($warnings.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Handoff Warnings")
        [void]$Lines.Add("")
        foreach ($warning in $warnings) {
            $reportId = Get-ReleaseBlockerPropertyValue -Object $warning -Name "report_id"
            $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "id") -Fallback "(unknown warning)"
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value $reportId -Fallback "(unknown report)") / ${id}: action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $warning -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $warning -RepoRoot $RepoRoot
        }
    }

    if ($actionItems.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Handoff Action Items")
        [void]$Lines.Add("")
        foreach ($item in $actionItems) {
            $reportId = Get-ReleaseBlockerPropertyValue -Object $item -Name "report_id"
            $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "id") -Fallback "(unknown action item)"
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value $reportId -Fallback "(unknown report)") / ${id}: action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema"))")
            $openCommand = Get-ReleaseBlockerPropertyValue -Object $item -Name "open_command"
            if (-not [string]::IsNullOrWhiteSpace($openCommand)) {
                [void]$Lines.Add("  - open_command: $openCommand")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $item -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $item
        }
    }
}
