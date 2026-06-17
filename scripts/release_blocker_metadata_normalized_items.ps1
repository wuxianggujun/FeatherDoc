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
                project_id = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "project_id")
                template_name = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "template_name")
                candidate_type = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "candidate_type")
                action = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")
                status = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "status")
                severity = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "severity")
                source_schema = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema")
                source_report = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_report")
                source_report_display = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_report_display")
                source_json = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_json")
                source_json_display = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_json_display")
                message = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message")
                repair_action_classes = @(Get-ReleaseBlockerArrayProperty -Object $blocker -Name "repair_action_classes")
                matched_document_count = Get-ReleaseBlockerPropertyObject -Object $blocker -Name "matched_document_count"
                unmatched_catalog_document_count = Get-ReleaseBlockerPropertyObject -Object $blocker -Name "unmatched_catalog_document_count"
                unmatched_baseline_document_count = Get-ReleaseBlockerPropertyObject -Object $blocker -Name "unmatched_baseline_document_count"
                alignment_gap_count = Get-ReleaseBlockerPropertyObject -Object $blocker -Name "alignment_gap_count"
                catalog_coverage_percent = Get-ReleaseBlockerPropertyObject -Object $blocker -Name "catalog_coverage_percent"
                baseline_coverage_percent = Get-ReleaseBlockerPropertyObject -Object $blocker -Name "baseline_coverage_percent"
                coverage_score = Get-ReleaseBlockerPropertyObject -Object $blocker -Name "coverage_score"
                catalog_document_keys = @(Get-ReleaseBlockerArrayProperty -Object $blocker -Name "catalog_document_keys")
                baseline_document_keys = @(Get-ReleaseBlockerArrayProperty -Object $blocker -Name "baseline_document_keys")
                matched_document_keys = @(Get-ReleaseBlockerArrayProperty -Object $blocker -Name "matched_document_keys")
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
            project_id = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "project_id")
            template_name = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "template_name")
            candidate_type = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "candidate_type")
            action = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")
            message = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "message")
            source_schema = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema")
            source_report = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_report")
            source_report_display = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_report_display")
            source_json = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_json")
            source_json_display = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_json_display")
            repair_action_classes = @(Get-ReleaseBlockerArrayProperty -Object $warning -Name "repair_action_classes")
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

        $itemId = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "id")
        if ([string]::IsNullOrWhiteSpace($itemId)) {
            $itemId = "action_item"
        }
        $itemAction = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "action")
        if ([string]::IsNullOrWhiteSpace($itemAction)) {
            $itemAction = $itemId
        }

        $entry = [ordered]@{
            id = $itemId
            project_id = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "project_id")
            template_name = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "template_name")
            candidate_type = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "candidate_type")
            action = $itemAction
            title = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "title")
            source_schema = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema")
            source_report = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_report")
            source_report_display = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_report_display")
            source_json = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_json")
            source_json_display = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_json_display")
            command = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "command")
            repair_action_classes = @(Get-ReleaseBlockerArrayProperty -Object $item -Name "repair_action_classes")
        }

        foreach ($optionalCommandName in @("open_command", "audit_command", "review_command")) {
            $optionalCommand = Get-ReleaseBlockerPropertyValue -Object $item -Name $optionalCommandName
            if (-not [string]::IsNullOrWhiteSpace($optionalCommand)) {
                $entry[$optionalCommandName] = [string]$optionalCommand
            }
        }

        foreach ($repairFieldName in @("repair_strategy", "repair_hint", "command_template")) {
            $repairValue = Get-ReleaseBlockerPropertyValue -Object $item -Name $repairFieldName
            if (-not [string]::IsNullOrWhiteSpace($repairValue)) {
                $entry[$repairFieldName] = [string]$repairValue
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

    foreach ($fieldName in @("project_id", "template_name", "candidate_type")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; {0}: `{1}`' -f $fieldName, $value)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $summaryText += ('; source_report: `{0}`' -f $sourceReportDisplay)
    }

    $repairActionClasses = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $ActionItem -Name "repair_action_classes")
    if ($repairActionClasses -ne "(none)") {
        $summaryText += ('; repair_action_classes: `{0}`' -f $repairActionClasses)
    }

    foreach ($fieldName in @("source_report", "source_json")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; raw_{0}: `{1}`' -f $fieldName, $value)
        }
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

    foreach ($fieldName in @("project_id", "template_name", "candidate_type")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Warning -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; {0}: `{1}`' -f $fieldName, $value)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Warning -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Warning -Name "source_json_display"
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $summaryText += ('; source_report: `{0}`' -f $sourceReportDisplay)
    }

    foreach ($fieldName in @("source_report", "source_json")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Warning -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; raw_{0}: `{1}`' -f $fieldName, $value)
        }
    }

    $repairActionClasses = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $Warning -Name "repair_action_classes")
    if ($repairActionClasses -ne "(none)") {
        $summaryText += ('; repair_action_classes: `{0}`' -f $repairActionClasses)
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

    foreach ($field in @("project_id", "template_name", "candidate_type", "status", "severity", "source_schema", "source_report_display", "source_json_display", "source_report", "source_json")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name $field
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; {0}: `{1}`' -f $field, $value)
        }
    }

    $message = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "message"
    if (-not [string]::IsNullOrWhiteSpace($message)) {
        $summaryText += ('; message: {0}' -f $message)
    }

    $repairActionClasses = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $Blocker -Name "repair_action_classes")
    if ($repairActionClasses -ne "(none)") {
        $summaryText += ('; repair_action_classes: `{0}`' -f $repairActionClasses)
    }

    foreach ($field in @(
            "matched_document_count",
            "unmatched_catalog_document_count",
            "unmatched_baseline_document_count",
            "alignment_gap_count",
            "catalog_coverage_percent",
            "baseline_coverage_percent",
            "coverage_score"
        )) {
        $value = Get-ReleaseBlockerPropertyObject -Object $Blocker -Name $field
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            $summaryText += ('; {0}: `{1}`' -f $field, $value)
        }
    }

    foreach ($field in @("catalog_document_keys", "baseline_document_keys", "matched_document_keys")) {
        $values = @(
            Get-ReleaseBlockerArrayProperty -Object $Blocker -Name $field |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        if ($values.Count -gt 0) {
            $summaryText += ('; {0}: `{1}`' -f $field, ($values -join ","))
        }
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
        Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $item
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

    foreach ($fieldName in @("project_id", "template_name", "candidate_type")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $blocker -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $text += ('; {0} `{1}`' -f $fieldName, $value)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $text += ('; source_report `{0}`' -f $sourceReportDisplay)
    }

    $repairActionClasses = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $blocker -Name "repair_action_classes")
    if ($repairActionClasses -ne "(none)") {
        $text += ('; repair_action_classes `{0}`' -f $repairActionClasses)
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

    $issueKeys = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $item -Name "issue_keys")
    if ($issueKeys -ne "(none)") {
        $text += ('; issue_keys `{0}`' -f $issueKeys)
    }

    foreach ($fieldName in @("project_id", "template_name", "candidate_type")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $text += ('; {0} `{1}`' -f $fieldName, $value)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $item -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $text += ('; source_report `{0}`' -f $sourceReportDisplay)
    }

    $repairActionClasses = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $item -Name "repair_action_classes")
    if ($repairActionClasses -ne "(none)") {
        $text += ('; repair_action_classes `{0}`' -f $repairActionClasses)
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

    foreach ($fieldName in @("project_id", "template_name", "candidate_type")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $warning -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $text += ('; {0} `{1}`' -f $fieldName, $value)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $text += ('; source_report `{0}`' -f $sourceReportDisplay)
    }

    $repairActionClasses = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $warning -Name "repair_action_classes")
    if ($repairActionClasses -ne "(none)") {
        $text += ('; repair_action_classes `{0}`' -f $repairActionClasses)
    }

    return $text
}
