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

function Get-ReleaseGovernanceWarningDisplayValue {
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

function Get-NormalizedReleaseGovernanceBlockers {
    param([AllowNull()]$Blockers)

    $normalizedBlockers = New-Object 'System.Collections.Generic.List[object]'
    foreach ($blocker in @(Get-ReleaseBlockerArrayProperty -Object ([ordered]@{ release_blockers = $Blockers }) -Name "release_blockers")) {
        if ($null -eq $blocker) {
            continue
        }

        $entry = [ordered]@{
            composite_id = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "composite_id")
            id = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id")
            action = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")
            status = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "status")
            severity = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "severity")
            source_schema = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema")
            source_report_display = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_report_display")
            message = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message")
        }

        [void]$normalizedBlockers.Add([pscustomobject]$entry)
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
            # Fall back to the materialized release_blockers array when
            # historical fixtures omitted a stable integer value.
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
            # Fall back to the materialized warnings array when historical fixtures
            # omitted a stable integer value for warning_count.
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
            # Fall back to the materialized action_items array when historical
            # fixtures omitted a stable integer value for action_item_count.
        }
    }

    return @(Get-NormalizedReleaseGovernanceActionItems -ActionItems (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "action_items")).Count
}

function Get-ReleaseGovernanceActionItemSummaryText {
    param([AllowNull()]$ActionItem)

    $id = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "action")
    $title = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "title")
    $sourceSchema = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "source_schema")
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

    $id = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "action")
    $message = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "message")
    $sourceSchema = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "source_schema")
    $summaryText = ('id: `{0}`; action: `{1}`; message: {2}; source_schema: `{3}`' -f `
            $id, $action, $message, $sourceSchema)

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

    $id = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "id") -Fallback "(unknown)"
    $summaryText = ('id: `{0}`; action: `{1}`' -f `
            $id,
            (Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "action")))

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
    $releaseGovernanceHandoff = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff"
    $hadSection = $false
    foreach ($sectionInfo in @(
            @{
                Heading = "Release blocker rollup blockers"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup"
            },
            @{
                Heading = "Release governance handoff blockers"
                SummaryObject = $releaseGovernanceHandoff
            },
            @{
                Heading = "Release governance handoff nested rollup blockers"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $releaseGovernanceHandoff -Name "release_blocker_rollup"
            }
        )) {
        if (Add-ReleaseGovernanceBlockerMarkdownSubsection `
                -Lines $sectionLines `
                -Heading $sectionInfo.Heading `
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
    $releaseGovernanceHandoff = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff"
    $hadSection = $false
    foreach ($sectionInfo in @(
            @{
                Heading = "Release blocker rollup action items"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup"
            },
            @{
                Heading = "Release governance handoff action items"
                SummaryObject = $releaseGovernanceHandoff
            },
            @{
                Heading = "Release governance handoff nested rollup action items"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $releaseGovernanceHandoff -Name "release_blocker_rollup"
            }
        )) {
        if (Add-ReleaseGovernanceActionItemMarkdownSubsection `
                -Lines $sectionLines `
                -Heading $sectionInfo.Heading `
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

function Get-ReleaseGovernanceActionItemChecklistItems {
    param([AllowNull()]$Summary)

    $items = New-Object 'System.Collections.Generic.List[object]'
    $releaseGovernanceHandoff = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff"
    foreach ($sectionInfo in @(
            @{
                Heading = "Release blocker rollup action items"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup"
            },
            @{
                Heading = "Release governance handoff action items"
                SummaryObject = $releaseGovernanceHandoff
            },
            @{
                Heading = "Release governance handoff nested rollup action items"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $releaseGovernanceHandoff -Name "release_blocker_rollup"
            }
        )) {
        foreach ($actionItem in @(Get-NormalizedReleaseGovernanceActionItems -ActionItems (Get-ReleaseBlockerArrayProperty -Object $sectionInfo.SummaryObject -Name "action_items"))) {
            [void]$items.Add([pscustomobject]@{
                    context = [string]$sectionInfo.Heading
                    action_item = $actionItem
                })
        }
    }

    return @($items.ToArray())
}

function Get-ReleaseGovernanceActionItemChecklistText {
    param([AllowNull()]$ActionItem)

    $context = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "context")
    $item = Get-ReleaseBlockerPropertyObject -Object $ActionItem -Name "action_item"
    $id = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "action")
    $title = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "title")
    $sourceSchema = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema")
    $text = 'Review release governance action item `{0}` ({1}): action `{2}`, title: {3}, source_schema `{4}`' -f `
        $id, $context, $action, $title, $sourceSchema

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $item -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $text += ('; source_report `{0}`' -f $sourceReportDisplay)
    }

    return $text
}

function Get-ReleaseGovernanceActionItemActionGuidanceLines {
    param(
        [AllowNull()]$ActionItem,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    $guidanceLines = New-Object 'System.Collections.Generic.List[string]'
    $id = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "action"
    $sourceSchema = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "source_schema")
    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "source_report_display"
    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        [void]$guidanceLines.Add(('Open source report `{0}` while handling release governance action item `{1}`.' -f $sourceReportDisplay, $id))
    }

    $hadCommand = $false
    foreach ($commandName in @("command", "open_command", "audit_command", "review_command")) {
        $commandValue = Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name $commandName
        if (-not [string]::IsNullOrWhiteSpace($commandValue)) {
            $hadCommand = $true
            [void]$guidanceLines.Add(('Run `{0}` for release governance action item `{1}`: `{2}`' -f $commandName, $id, $commandValue))
        }
    }

    if (-not $hadCommand) {
        if ([string]::IsNullOrWhiteSpace($action)) {
            [void]$guidanceLines.Add(('Action item command is missing for source_schema `{0}`; update the source governance report, rerun release governance checks, and regenerate the release note bundle from `{1}` before publishing.' -f $sourceSchema, $releaseSummaryDisplay))
        } else {
            [void]$guidanceLines.Add(('Follow action item `{0}` for source_schema `{1}`: update the owning governance evidence, rerun release governance checks, and regenerate the release note bundle from `{2}` before publishing.' -f `
                    $action, $sourceSchema, $releaseSummaryDisplay))
        }
    }

    return $guidanceLines.ToArray()
}

function Get-ReleaseGovernanceBlockerChecklistItems {
    param([AllowNull()]$Summary)

    $items = New-Object 'System.Collections.Generic.List[object]'
    $releaseGovernanceHandoff = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff"
    foreach ($sectionInfo in @(
            @{
                Heading = "Release blocker rollup blockers"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup"
            },
            @{
                Heading = "Release governance handoff blockers"
                SummaryObject = $releaseGovernanceHandoff
            },
            @{
                Heading = "Release governance handoff nested rollup blockers"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $releaseGovernanceHandoff -Name "release_blocker_rollup"
            }
        )) {
        foreach ($blocker in @(Get-NormalizedReleaseGovernanceBlockers -Blockers (Get-ReleaseBlockerArrayProperty -Object $sectionInfo.SummaryObject -Name "release_blockers"))) {
            [void]$items.Add([pscustomobject]@{
                    context = [string]$sectionInfo.Heading
                    blocker = $blocker
                })
        }
    }

    return @($items.ToArray())
}

function Get-ReleaseGovernanceBlockerChecklistText {
    param([AllowNull()]$BlockerItem)

    $context = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $BlockerItem -Name "context")
    $blocker = Get-ReleaseBlockerPropertyObject -Object $BlockerItem -Name "blocker"
    $id = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")
    $status = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "status")
    $severity = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "severity")
    $sourceSchema = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema")
    $message = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message")
    $text = 'Resolve release governance blocker `{0}` ({1}): action `{2}`, status `{3}`, severity `{4}`, source_schema `{5}`, message: {6}' -f `
        $id, $context, $action, $status, $severity, $sourceSchema, $message

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

function Get-ReleaseGovernanceBlockerActionGuidanceLines {
    param(
        [AllowNull()]$Blocker,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    $guidanceLines = New-Object 'System.Collections.Generic.List[string]'
    $action = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "action"
    $sourceSchema = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_schema")
    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_report_display"
    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
    if ([string]::IsNullOrWhiteSpace($action)) {
        [void]$guidanceLines.Add(('Blocker action is missing for source_schema `{0}`; update the source governance report, rerun release governance checks, and regenerate the release note bundle before publishing.' -f $sourceSchema))
        return $guidanceLines.ToArray()
    }

    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        [void]$guidanceLines.Add(('Open source report `{0}` for blocker action `{1}`, resolve the owning evidence, rerun release governance checks, and regenerate the release note bundle from `{2}` before publishing.' -f `
                $sourceReportDisplay, $action, $releaseSummaryDisplay))
    } else {
        [void]$guidanceLines.Add(('Follow blocker action `{0}` for source_schema `{1}`: resolve the owning governance evidence, rerun release governance checks, and regenerate the release note bundle from `{2}` before publishing.' -f `
                $action, $sourceSchema, $releaseSummaryDisplay))
    }

    return $guidanceLines.ToArray()
}

function Get-ReleaseGovernanceWarningChecklistItems {
    param([AllowNull()]$Summary)

    $items = New-Object 'System.Collections.Generic.List[object]'
    $releaseGovernanceHandoff = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff"
    foreach ($sectionInfo in @(
            @{
                Heading = "Release blocker rollup warnings"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup"
            },
            @{
                Heading = "Release governance handoff warnings"
                SummaryObject = $releaseGovernanceHandoff
            },
            @{
                Heading = "Release governance handoff nested rollup warnings"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $releaseGovernanceHandoff -Name "release_blocker_rollup"
            }
        )) {
        foreach ($warning in @(Get-NormalizedReleaseGovernanceWarnings -Warnings (Get-ReleaseBlockerArrayProperty -Object $sectionInfo.SummaryObject -Name "warnings"))) {
            [void]$items.Add([pscustomobject]@{
                    context = [string]$sectionInfo.Heading
                    warning = $warning
                })
        }
    }

    return @($items.ToArray())
}

function Get-ReleaseGovernanceWarningChecklistText {
    param([AllowNull()]$WarningItem)

    $context = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $WarningItem -Name "context")
    $warning = Get-ReleaseBlockerPropertyObject -Object $WarningItem -Name "warning"
    $id = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")
    $sourceSchema = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema")
    $message = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "message")

    return 'Review release governance warning `{0}` ({1}): action `{2}`, source_schema `{3}`, message: {4}' -f `
        $id, $context, $action, $sourceSchema, $message
}

function Get-ReleaseGovernanceWarningActionGuidanceLines {
    param(
        [AllowNull()]$Warning,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    $guidanceLines = New-Object 'System.Collections.Generic.List[string]'
    $action = Get-ReleaseBlockerPropertyValue -Object $Warning -Name "action"
    $sourceSchema = Get-ReleaseGovernanceWarningDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "source_schema")
    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
    if ([string]::IsNullOrWhiteSpace($action)) {
        [void]$guidanceLines.Add(('Warning action is missing for source_schema `{0}`; update the source governance report, rerun release governance checks, and regenerate the release note bundle before publishing.' -f $sourceSchema))
        return $guidanceLines.ToArray()
    }

    switch ($action) {
        { $_ -in @("review_style_merge_suggestions", "review_style_merge_plan") } {
            $styleMergeSuggestionCount = Get-ReleaseBlockerPropertyObject -Object $Warning -Name "style_merge_suggestion_count"
            $styleMergeSuggestionPendingCount = Get-ReleaseBlockerPropertyObject -Object $Warning -Name "style_merge_suggestion_pending_count"
            $countText = ""
            if ($null -ne $styleMergeSuggestionPendingCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionPendingCount)) {
                $countText = (' Current pending style merge suggestion count is `{0}`.' -f [string]$styleMergeSuggestionPendingCount)
            } elseif ($null -ne $styleMergeSuggestionCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionCount)) {
                $countText = (' Current style merge suggestion count is `{0}`.' -f [string]$styleMergeSuggestionCount)
            }

            [void]$guidanceLines.Add(('Use action `{0}`: review the referenced style merge suggestion plan before tightening release gates.{1} Update the owning governance evidence, rerun the relevant governance rollup, then regenerate the release note bundle from `{2}`.' -f `
                    $action, $countText, $releaseSummaryDisplay))
            break
        }
        default {
            [void]$guidanceLines.Add(('Follow warning action `{0}` for source_schema `{1}`: update the owning governance report, rerun release governance checks, and regenerate the release note bundle from `{2}` before publishing.' -f `
                    $action, $sourceSchema, $releaseSummaryDisplay))
        }
    }

    return $guidanceLines.ToArray()
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
    $releaseGovernanceHandoff = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff"
    $hadSection = $false
    foreach ($sectionInfo in @(
            @{
                Heading = "Release blocker rollup warnings"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup"
            },
            @{
                Heading = "Release governance handoff warnings"
                SummaryObject = $releaseGovernanceHandoff
            },
            @{
                Heading = "Release governance handoff nested rollup warnings"
                SummaryObject = Get-ReleaseBlockerPropertyObject -Object $releaseGovernanceHandoff -Name "release_blocker_rollup"
            }
        )) {
        if (Add-ReleaseGovernanceWarningMarkdownSubsection `
                -Lines $sectionLines `
                -Heading $sectionInfo.Heading `
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
