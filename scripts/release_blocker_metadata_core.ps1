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

function Test-ReleaseBlockerPropertyExists {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $false
    }

    if ($Object -is [System.Collections.IDictionary]) {
        return $Object.Contains($Name)
    }

    return ($null -ne $Object.PSObject.Properties[$Name])
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

    if ($value -is [datetime]) {
        return $value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    return [string]$value
}

function Get-ReleaseBlockerIntPropertyValue {
    param(
        [AllowNull()]$Object,
        [string]$Name,
        [int]$DefaultValue = 0
    )

    $value = Get-ReleaseBlockerPropertyObject -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }

    try {
        return [int]$value
    } catch {
        return $DefaultValue
    }
}

function Get-ReleaseBlockerBoolPropertyDisplayValue {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    $value = Get-ReleaseBlockerPropertyObject -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return ""
    }

    if ($value -is [bool]) {
        return ([string]$value).ToLowerInvariant()
    }

    $text = [string]$value
    if ([string]::Equals($text, "true", [System.StringComparison]::OrdinalIgnoreCase)) {
        return "true"
    }
    if ([string]::Equals($text, "false", [System.StringComparison]::OrdinalIgnoreCase)) {
        return "false"
    }

    return $text
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

function Assert-ReleaseGovernanceDeclaredCountConsistency {
    param(
        [AllowNull()]$Container,
        [string]$ContainerName,
        [string]$CountProperty,
        [string]$ItemsProperty,
        [string]$Context
    )

    $countObject = Get-ReleaseBlockerPropertyObject -Object $Container -Name $CountProperty
    if ($null -eq $countObject -or [string]::IsNullOrWhiteSpace([string]$countObject)) {
        return
    }

    try {
        $declaredCount = [int]([string]$countObject)
    } catch {
        throw "$ContainerName.$CountProperty must be an integer when provided in ${Context}; actual value: $countObject"
    }

    if ($declaredCount -lt 0) {
        throw "$ContainerName.$CountProperty must not be negative in ${Context}; actual value: $declaredCount"
    }

    $actualCount = @(Get-ReleaseBlockerArrayProperty -Object $Container -Name $ItemsProperty).Count
    if ($declaredCount -ne $actualCount) {
        throw "$ContainerName.$CountProperty mismatch in ${Context}: declared $declaredCount but $ContainerName.$ItemsProperty contains $actualCount item(s)."
    }
}

function Assert-ReleaseGovernanceReviewerMetadataItemQuality {
    param(
        [AllowNull()]$Item,
        [string]$ItemPath,
        [string[]]$RequiredFields,
        [string]$Context
    )

    foreach ($fieldName in @($RequiredFields)) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if ([string]::IsNullOrWhiteSpace($fieldValue)) {
            throw "$ItemPath.$fieldName must not be empty in ${Context}."
        }
    }
}

function Assert-ReleaseGovernanceSourceSchemaSummaryConsistency {
    param(
        [AllowNull()]$Container,
        [string]$ContainerName,
        [string]$ItemsProperty,
        [string]$SummaryProperty,
        [string]$Context
    )

    if (-not (Test-ReleaseBlockerPropertyExists -Object $Container -Name $SummaryProperty)) {
        return
    }

    $expectedCounts = @{}
    foreach ($item in @(Get-ReleaseBlockerArrayProperty -Object $Container -Name $ItemsProperty)) {
        $sourceSchema = Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema"
        if ([string]::IsNullOrWhiteSpace($sourceSchema)) {
            throw "$ContainerName.$ItemsProperty source_schema must not be empty when $ContainerName.$SummaryProperty is provided in ${Context}."
        }

        if (-not $expectedCounts.ContainsKey($sourceSchema)) {
            $expectedCounts[$sourceSchema] = 0
        }
        $expectedCounts[$sourceSchema] += 1
    }

    $actualCounts = @{}
    $summaryGroups = @(Get-ReleaseBlockerArrayProperty -Object $Container -Name $SummaryProperty)
    for ($groupIndex = 0; $groupIndex -lt $summaryGroups.Count; $groupIndex++) {
        $group = $summaryGroups[$groupIndex]
        $sourceSchema = Get-ReleaseBlockerPropertyValue -Object $group -Name "source_schema"
        if ([string]::IsNullOrWhiteSpace($sourceSchema)) {
            throw "$ContainerName.$SummaryProperty[$groupIndex].source_schema must not be empty in ${Context}."
        }
        if ($actualCounts.ContainsKey($sourceSchema)) {
            throw "$ContainerName.$SummaryProperty contains duplicate source_schema '$sourceSchema' in ${Context}."
        }

        $countText = Get-ReleaseBlockerPropertyValue -Object $group -Name "count"
        if ([string]::IsNullOrWhiteSpace($countText)) {
            throw "$ContainerName.$SummaryProperty[$groupIndex].count must not be empty in ${Context}."
        }

        $count = 0
        if (-not [int]::TryParse($countText, [ref]$count)) {
            throw "$ContainerName.$SummaryProperty[$groupIndex].count must be an integer in ${Context}; actual value: $countText"
        }
        if ($count -lt 0) {
            throw "$ContainerName.$SummaryProperty[$groupIndex].count must not be negative in ${Context}; actual value: $count"
        }

        $actualCounts[$sourceSchema] = $count
    }

    foreach ($sourceSchema in @($expectedCounts.Keys)) {
        if (-not $actualCounts.ContainsKey($sourceSchema)) {
            throw "$ContainerName.$SummaryProperty missing source_schema '$sourceSchema' in ${Context}."
        }
        if ([int]$actualCounts[$sourceSchema] -ne [int]$expectedCounts[$sourceSchema]) {
            throw "$ContainerName.$SummaryProperty count mismatch for '$sourceSchema' in ${Context}: declared $($actualCounts[$sourceSchema]) but $ContainerName.$ItemsProperty contains $($expectedCounts[$sourceSchema]) item(s)."
        }
    }

    foreach ($sourceSchema in @($actualCounts.Keys)) {
        if (-not $expectedCounts.ContainsKey($sourceSchema)) {
            throw "$ContainerName.$SummaryProperty has source_schema '$sourceSchema' but $ContainerName.$ItemsProperty contains no matching item in ${Context}."
        }
    }
}

function Assert-ReleaseGovernanceReviewerMetadataCollectionQuality {
    param(
        [AllowNull()]$Container,
        [string]$ContainerName,
        [string]$Context
    )

    if ($null -eq $Container) {
        return
    }

    Assert-ReleaseGovernanceDeclaredCountConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -CountProperty "release_blocker_count" `
        -ItemsProperty "release_blockers" `
        -Context $Context
    Assert-ReleaseGovernanceDeclaredCountConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -CountProperty "warning_count" `
        -ItemsProperty "warnings" `
        -Context $Context
    Assert-ReleaseGovernanceDeclaredCountConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -CountProperty "action_item_count" `
        -ItemsProperty "action_items" `
        -Context $Context
    Assert-ReleaseGovernanceDeclaredCountConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -CountProperty "informational_action_item_count" `
        -ItemsProperty "informational_action_items" `
        -Context $Context

    $blockerRequiredFields = @("id", "action", "message", "source_schema", "source_report_display", "source_json_display")
    $warningRequiredFields = @("id", "action", "message", "source_schema", "source_report_display", "source_json_display")
    $actionItemRequiredFields = @("id", "action", "open_command", "source_schema", "source_report_display", "source_json_display")

    $releaseBlockers = @(Get-ReleaseBlockerArrayProperty -Object $Container -Name "release_blockers")
    for ($itemIndex = 0; $itemIndex -lt $releaseBlockers.Count; $itemIndex++) {
        Assert-ReleaseGovernanceReviewerMetadataItemQuality `
            -Item $releaseBlockers[$itemIndex] `
            -ItemPath "$ContainerName.release_blockers[$itemIndex]" `
            -RequiredFields $blockerRequiredFields `
            -Context $Context
    }

    $warnings = @(Get-ReleaseBlockerArrayProperty -Object $Container -Name "warnings")
    for ($itemIndex = 0; $itemIndex -lt $warnings.Count; $itemIndex++) {
        Assert-ReleaseGovernanceReviewerMetadataItemQuality `
            -Item $warnings[$itemIndex] `
            -ItemPath "$ContainerName.warnings[$itemIndex]" `
            -RequiredFields $warningRequiredFields `
            -Context $Context
    }

    $actionItems = @(Get-ReleaseBlockerArrayProperty -Object $Container -Name "action_items")
    for ($itemIndex = 0; $itemIndex -lt $actionItems.Count; $itemIndex++) {
        Assert-ReleaseGovernanceReviewerMetadataItemQuality `
            -Item $actionItems[$itemIndex] `
            -ItemPath "$ContainerName.action_items[$itemIndex]" `
            -RequiredFields $actionItemRequiredFields `
            -Context $Context
    }

    Assert-ReleaseGovernanceSourceSchemaSummaryConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -ItemsProperty "release_blockers" `
        -SummaryProperty "blocker_source_schema_summary" `
        -Context $Context
    Assert-ReleaseGovernanceSourceSchemaSummaryConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -ItemsProperty "action_items" `
        -SummaryProperty "action_item_source_schema_summary" `
        -Context $Context
    Assert-ReleaseGovernanceSourceSchemaSummaryConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -ItemsProperty "informational_action_items" `
        -SummaryProperty "informational_action_item_source_schema_summary" `
        -Context $Context
    Assert-ReleaseGovernanceSourceSchemaSummaryConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -ItemsProperty "warnings" `
        -SummaryProperty "warning_source_schema_summary" `
        -Context $Context
}

function Assert-ReleaseGovernanceReviewerMetadataQuality {
    param(
        [AllowNull()]$Summary,
        [string]$Context = "release summary"
    )

    Assert-ReleaseGovernanceReviewerMetadataCollectionQuality `
        -Container (Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup") `
        -ContainerName "release_blocker_rollup" `
        -Context $Context
    Assert-ReleaseGovernanceReviewerMetadataCollectionQuality `
        -Container (Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff") `
        -ContainerName "release_governance_handoff" `
        -Context $Context

    $stages = @(Get-ReleaseBlockerArrayProperty -Object $Summary -Name "stages")
    for ($stageIndex = 0; $stageIndex -lt $stages.Count; $stageIndex++) {
        $stageId = Get-ReleaseBlockerDisplayValue `
            -Value (Get-ReleaseBlockerPropertyValue -Object $stages[$stageIndex] -Name "id") `
            -Fallback $stageIndex
        Assert-ReleaseGovernanceReviewerMetadataCollectionQuality `
            -Container $stages[$stageIndex] `
            -ContainerName "stages[$($stageIndex):$stageId]" `
            -Context $Context
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

    if ($null -eq $Value) {
        return $Fallback
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    $text = [string]$Value
    if ([string]::IsNullOrWhiteSpace($text)) {
        return $Fallback
    }

    return $text
}

function Get-ReleaseBlockerSummaryGroupDisplay {
    param(
        [object[]]$Items,
        [string]$NameProperty = "source_schema",
        [string]$CountProperty = "count"
    )

    $parts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($item in @($Items)) {
        $name = Get-ReleaseBlockerPropertyValue -Object $item -Name $NameProperty
        if ([string]::IsNullOrWhiteSpace($name)) {
            continue
        }

        $count = Get-ReleaseBlockerPropertyValue -Object $item -Name $CountProperty
        if ([string]::IsNullOrWhiteSpace($count)) {
            $count = "0"
        }

        [void]$parts.Add("${name}=${count}")
    }

    if ($parts.Count -eq 0) {
        return "(none)"
    }

    return ($parts.ToArray() -join ", ")
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

function Get-ReleaseBlockerPublicDisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    $displayPath = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $Path
    if ([string]::IsNullOrWhiteSpace($displayPath) -or
        [string]::Equals($displayPath, "(not available)", [System.StringComparison]::OrdinalIgnoreCase)) {
        return $displayPath
    }

    if ($displayPath.StartsWith(".\") -or
        $displayPath.StartsWith("./") -or
        -not [System.IO.Path]::IsPathRooted($displayPath)) {
        return $displayPath
    }

    return Split-Path -Leaf $displayPath
}

function Convert-ReleaseBlockerLocalPathsForPublicText {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ""
    }

    return [regex]::Replace(
        $Text,
        '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|;,)]+|(?<!\w)/(?:Users|home)/[^\s"''`<>|;,)]+',
        '<local-path>'
    )
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

function Format-ProjectTemplateSchemaApprovalStatusSummary {
    param(
        [AllowNull()]$Value,
        [string]$Fallback = ""
    )

    if ($null -eq $Value) {
        return $Fallback
    }

    if ($Value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($Value)) {
            return $Fallback
        }

        return $Value
    }

    $items = if ($Value -is [System.Collections.IEnumerable] -and -not ($Value -is [System.Collections.IDictionary])) {
        @($Value | Where-Object { $null -ne $_ })
    } else {
        @($Value)
    }

    $parts = @(
        foreach ($item in $items) {
            if ($null -eq $item) {
                continue
            }

            if ($item -is [string]) {
                if (-not [string]::IsNullOrWhiteSpace($item)) {
                    [string]$item
                }
                continue
            }

            $status = Get-ReleaseBlockerPropertyValue -Object $item -Name "status"
            $count = Get-ReleaseBlockerPropertyValue -Object $item -Name "count"
            if (-not [string]::IsNullOrWhiteSpace($status) -and -not [string]::IsNullOrWhiteSpace($count)) {
                "${status}=${count}"
            } elseif (-not [string]::IsNullOrWhiteSpace($status)) {
                $status
            } elseif (-not [string]::IsNullOrWhiteSpace($count)) {
                "count=${count}"
            } elseif (-not [string]::IsNullOrWhiteSpace([string]$item)) {
                [string]$item
            }
        }
    )

    if ($parts.Count -eq 0) {
        return $Fallback
    }

    return ($parts -join ", ")
}

function Format-ProjectTemplateNextActionSummary {
    param(
        [AllowNull()]$Value,
        [string]$Fallback = ""
    )

    if ($null -eq $Value) {
        return $Fallback
    }

    if ($Value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($Value)) {
            return $Fallback
        }

        return $Value
    }

    $items = if ($Value -is [System.Collections.IEnumerable] -and -not ($Value -is [System.Collections.IDictionary])) {
        @($Value | Where-Object { $null -ne $_ })
    } else {
        @($Value)
    }

    $itemsText = @(
        foreach ($item in $items) {
            if ($null -eq $item) {
                continue
            }

            if ($item -is [string]) {
                if (-not [string]::IsNullOrWhiteSpace($item)) {
                    [string]$item
                }
                continue
            }

            $parts = New-Object 'System.Collections.Generic.List[string]'
            foreach ($fieldName in @("action", "status", "blocker_id", "reason")) {
                $fieldValue = Get-ReleaseBlockerPropertyValue -Object $item -Name $fieldName
                if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                    [void]$parts.Add("${fieldName}=${fieldValue}")
                }
            }

            if ($parts.Count -gt 0) {
                $parts -join " "
            } elseif (-not [string]::IsNullOrWhiteSpace([string]$item)) {
                [string]$item
            }
        }
    )

    if ($itemsText.Count -eq 0) {
        return $Fallback
    }

    return ($itemsText -join ", ")
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
