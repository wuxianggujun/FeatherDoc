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
