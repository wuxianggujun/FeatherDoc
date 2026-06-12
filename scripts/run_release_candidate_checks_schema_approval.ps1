function Convert-OptionalBoolean {
    param(
        [AllowNull()]$Value,
        [bool]$DefaultValue = $false
    )

    if ($null -eq $Value) {
        return $DefaultValue
    }

    if ($Value -is [bool]) {
        return [bool]$Value
    }

    if ($Value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($Value)) {
            return $DefaultValue
        }

        return $Value.Equals("true", [System.StringComparison]::OrdinalIgnoreCase)
    }

    return [bool]$Value
}

function Get-ProjectTemplateSmokeSchemaBaselineCounts {
    param([AllowNull()]$Summary)

    $counts = [ordered]@{
        dirty = 0
        drift = 0
    }

    foreach ($entry in @(Get-OptionalPropertyValue -Object $Summary -Name "entries")) {
        $checks = Get-OptionalPropertyValue -Object $entry -Name "checks"
        $schemaBaseline = Get-OptionalPropertyValue -Object $checks -Name "schema_baseline"
        if ($null -eq $schemaBaseline) {
            continue
        }

        if (-not (Convert-OptionalBoolean -Value (Get-OptionalPropertyValue -Object $schemaBaseline -Name "enabled"))) {
            continue
        }

        $matches = Convert-OptionalBoolean `
            -Value (Get-OptionalPropertyValue -Object $schemaBaseline -Name "matches") `
            -DefaultValue $true
        $baselineResult = Get-OptionalPropertyValue -Object $schemaBaseline -Name "result"
        if (-not $matches -and $null -ne $baselineResult) {
            $counts.drift += 1
        }

        $schemaLintClean = Convert-OptionalBoolean `
            -Value (Get-OptionalPropertyValue -Object $schemaBaseline -Name "schema_lint_clean") `
            -DefaultValue $true
        if (-not $schemaLintClean) {
            $counts.dirty += 1
        }
    }

    return [pscustomobject]$counts
}

function Get-ProjectTemplateSmokeDirtySchemaBaselineCount {
    param([AllowNull()]$Summary)

    $value = Get-OptionalPropertyValue -Object $Summary -Name "dirty_schema_baseline_count"
    if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
        return [int]$value
    }

    $counts = Get-ProjectTemplateSmokeSchemaBaselineCounts -Summary $Summary
    return [int]$counts.dirty
}

function Get-OptionalIntegerProperty {
    param(
        [AllowNull()]$Object,
        [string]$Name,
        [int]$DefaultValue = 0
    )

    $value = Get-OptionalPropertyValue -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }

    return [int]$value
}

function Get-OptionalObjectArrayProperty {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    $value = Get-OptionalPropertyValue -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }
    if ($value -is [string]) {
        return @($value)
    }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Get-SchemaPatchApprovalGateStatus {
    param(
        [int]$PendingCount,
        [int]$ApprovalItemCount,
        [int]$InvalidResultCount,
        [int]$ComplianceIssueCount
    )

    if ($ComplianceIssueCount -gt 0 -or $InvalidResultCount -gt 0) {
        return "blocked"
    }
    if ($PendingCount -gt 0) {
        return "pending"
    }
    if ($ApprovalItemCount -gt 0) {
        return "passed"
    }

    return "not_required"
}

function Get-SchemaPatchApprovalItemCount {
    param(
        [int]$PendingCount,
        [int]$ApprovedCount,
        [int]$RejectedCount,
        [object[]]$ApprovalItems
    )

    $countedItems = $PendingCount + $ApprovedCount + $RejectedCount
    if ($ApprovalItems.Count -gt $countedItems) {
        return $ApprovalItems.Count
    }

    return $countedItems
}

function Get-ProjectTemplateSmokeSchemaApprovalGateInfo {
    param([AllowNull()]$Summary)

    $approvalItems = @(Get-OptionalObjectArrayProperty -Object $Summary -Name "schema_patch_approval_items")
    $invalidResultCount = Get-OptionalIntegerProperty -Object $Summary -Name "schema_patch_approval_invalid_result_count"
    if ($invalidResultCount -eq 0 -and $approvalItems.Count -gt 0) {
        $invalidResultCount = @($approvalItems | Where-Object { [string](Get-OptionalPropertyValue -Object $_ -Name "status") -eq "invalid_result" }).Count
    }

    $pendingCount = Get-OptionalIntegerProperty -Object $Summary -Name "schema_patch_approval_pending_count"
    $approvedCount = Get-OptionalIntegerProperty -Object $Summary -Name "schema_patch_approval_approved_count"
    $rejectedCount = Get-OptionalIntegerProperty -Object $Summary -Name "schema_patch_approval_rejected_count"
    $complianceIssueCount = Get-OptionalIntegerProperty -Object $Summary -Name "schema_patch_approval_compliance_issue_count"
    $approvalItemCount = Get-SchemaPatchApprovalItemCount `
        -PendingCount $pendingCount `
        -ApprovedCount $approvedCount `
        -RejectedCount $rejectedCount `
        -ApprovalItems $approvalItems
    $gateStatus = Get-SchemaPatchApprovalGateStatus `
        -PendingCount $pendingCount `
        -ApprovalItemCount $approvalItemCount `
        -InvalidResultCount $invalidResultCount `
        -ComplianceIssueCount $complianceIssueCount

    return [pscustomobject]@{
        pending_count = $pendingCount
        approved_count = $approvedCount
        rejected_count = $rejectedCount
        compliance_issue_count = $complianceIssueCount
        invalid_result_count = $invalidResultCount
        item_count = $approvalItemCount
        gate_status = $gateStatus
        gate_blocked = ($gateStatus -eq "blocked")
    }
}

function New-ProjectTemplateSchemaApprovalReleaseBlockerItem {
    param([AllowNull()]$Item)

    $status = [string](Get-OptionalPropertyValue -Object $Item -Name "status")
    $action = [string](Get-OptionalPropertyValue -Object $Item -Name "action")
    if ([string]::IsNullOrWhiteSpace($action) -and $status -eq "invalid_result") {
        $action = "fix_schema_patch_approval_result"
    }

    return [pscustomobject]@{
        name = [string](Get-OptionalPropertyValue -Object $Item -Name "name")
        status = $status
        decision = [string](Get-OptionalPropertyValue -Object $Item -Name "decision")
        action = $action
        compliance_issue_count = Get-OptionalIntegerProperty -Object $Item -Name "compliance_issue_count"
        compliance_issues = @(Get-OptionalObjectArrayProperty -Object $Item -Name "compliance_issues")
        approval_result = [string](Get-OptionalPropertyValue -Object $Item -Name "approval_result")
        schema_update_candidate = [string](Get-OptionalPropertyValue -Object $Item -Name "schema_update_candidate")
        review_json = [string](Get-OptionalPropertyValue -Object $Item -Name "review_json")
    }
}

function Get-ProjectTemplateSchemaApprovalBlockedItems {
    param([object[]]$ApprovalItems)

    return @(
        $ApprovalItems | Where-Object {
            $status = [string](Get-OptionalPropertyValue -Object $_ -Name "status")
            $complianceIssueCount = Get-OptionalIntegerProperty -Object $_ -Name "compliance_issue_count"
            $status -eq "invalid_result" -or $complianceIssueCount -gt 0
        } | ForEach-Object {
            New-ProjectTemplateSchemaApprovalReleaseBlockerItem -Item $_
        }
    )
}

function Get-ProjectTemplateSchemaApprovalIssueKeys {
    param([object[]]$BlockedItems)

    return @(
        $BlockedItems |
            ForEach-Object { @(Get-OptionalObjectArrayProperty -Object $_ -Name "compliance_issues") } |
            Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
            Sort-Object -Unique
    )
}

function New-ProjectTemplateSchemaApprovalReleaseBlocker {
    param(
        [AllowNull()]$ProjectTemplateSmokeSummary,
        [AllowNull()]$HistorySummary,
        [string]$SummaryJsonPath,
        [string]$HistoryJsonPath,
        [string]$HistoryMarkdownPath
    )

    $gateInfo = Get-ProjectTemplateSmokeSchemaApprovalGateInfo -Summary $ProjectTemplateSmokeSummary
    $latestBlockingSummary = Get-OptionalPropertyValue -Object $HistorySummary -Name "latest_blocking_summary"
    $latestBlockingStatus = [string](Get-OptionalPropertyValue -Object $latestBlockingSummary -Name "status")
    $historyBlocked = $latestBlockingStatus -eq "blocked"

    if (-not [bool]$gateInfo.gate_blocked) {
        return $null
    }

    $approvalItems = @(Get-OptionalObjectArrayProperty -Object $ProjectTemplateSmokeSummary -Name "schema_patch_approval_items")
    $blockedItems = @(Get-ProjectTemplateSchemaApprovalBlockedItems -ApprovalItems $approvalItems)
    if ($historyBlocked) {
        $historyBlockedItems = @(Get-OptionalObjectArrayProperty -Object $latestBlockingSummary -Name "items" |
            ForEach-Object { New-ProjectTemplateSchemaApprovalReleaseBlockerItem -Item $_ })
        if ($historyBlockedItems.Count -gt 0) {
            $blockedItems = $historyBlockedItems
        }
    }

    $issueKeys = @(Get-ProjectTemplateSchemaApprovalIssueKeys -BlockedItems $blockedItems)
    if ($historyBlocked) {
        $historyIssueKeys = @(Get-OptionalObjectArrayProperty -Object $latestBlockingSummary -Name "issue_keys" |
            Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
            Sort-Object -Unique)
        if ($historyIssueKeys.Count -gt 0) {
            $issueKeys = $historyIssueKeys
        }
    }

    $blockingSummaryJson = [string](Get-OptionalPropertyValue -Object $latestBlockingSummary -Name "summary_json")
    if ([string]::IsNullOrWhiteSpace($blockingSummaryJson)) {
        $blockingSummaryJson = $SummaryJsonPath
    }
    $blockedItemCount = $blockedItems.Count
    if ($historyBlocked) {
        $historyBlockedItemCount = Get-OptionalIntegerProperty -Object $latestBlockingSummary -Name "blocked_item_count" -DefaultValue $blockedItemCount
        if ($historyBlockedItemCount -gt 0) {
            $blockedItemCount = $historyBlockedItemCount
        }
    }

    return [pscustomobject]@{
        id = "project_template_smoke.schema_approval"
        source = "project_template_smoke"
        severity = "error"
        status = "blocked"
        message = "Project template smoke schema approval gate blocked."
        gate_status = "blocked"
        pending_count = [int]$gateInfo.pending_count
        approved_count = [int]$gateInfo.approved_count
        rejected_count = [int]$gateInfo.rejected_count
        compliance_issue_count = [int]$gateInfo.compliance_issue_count
        invalid_result_count = [int]$gateInfo.invalid_result_count
        blocked_item_count = [int]$blockedItemCount
        issue_keys = $issueKeys
        summary_json = $blockingSummaryJson
        history_json = $HistoryJsonPath
        history_markdown = $HistoryMarkdownPath
        action = "fix_schema_patch_approval_result"
        items = $blockedItems
    }
}

function Set-ProjectTemplateSchemaApprovalReleaseBlocker {
    param(
        [System.Collections.IDictionary]$ReleaseSummary,
        [AllowNull()]$Blocker
    )

    $blockers = New-Object 'System.Collections.Generic.List[object]'
    foreach ($existingBlocker in @(Get-OptionalObjectArrayProperty -Object $ReleaseSummary -Name "release_blockers")) {
        $existingId = [string](Get-OptionalPropertyValue -Object $existingBlocker -Name "id")
        if ($existingId -ne "project_template_smoke.schema_approval") {
            [void]$blockers.Add($existingBlocker)
        }
    }
    if ($null -ne $Blocker) {
        [void]$blockers.Add($Blocker)
    }

    $ReleaseSummary["release_blockers"] = $blockers.ToArray()
    $ReleaseSummary["release_blocker_count"] = $blockers.Count
}

function Read-ProjectTemplateSchemaApprovalHistorySummary {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Get-ProjectTemplateSchemaApprovalHistoryInputList {
    param(
        [string]$ReleaseSummaryPath,
        [string]$ProjectTemplateSmokeSummaryPath
    )

    $summaryPaths = New-Object 'System.Collections.Generic.List[string]'
    if (-not [string]::IsNullOrWhiteSpace($ProjectTemplateSmokeSummaryPath) -and
        (Test-Path -LiteralPath $ProjectTemplateSmokeSummaryPath)) {
        [void]$summaryPaths.Add($ProjectTemplateSmokeSummaryPath)
    }
    if (-not [string]::IsNullOrWhiteSpace($ReleaseSummaryPath) -and
        (Test-Path -LiteralPath $ReleaseSummaryPath)) {
        [void]$summaryPaths.Add($ReleaseSummaryPath)
    }

    return @($summaryPaths.ToArray() | Sort-Object -Unique)
}

function Invoke-ProjectTemplateSchemaApprovalHistory {
    param(
        [string]$ScriptPath,
        [string]$ReleaseSummaryPath,
        [string]$ProjectTemplateSmokeSummaryPath,
        [string]$OutputJson,
        [string]$OutputMarkdown
    )

    $summaryPaths = @(Get-ProjectTemplateSchemaApprovalHistoryInputList `
            -ReleaseSummaryPath $ReleaseSummaryPath `
            -ProjectTemplateSmokeSummaryPath $ProjectTemplateSmokeSummaryPath)
    if ($summaryPaths.Count -eq 0) {
        return [pscustomobject]@{
            status = "skipped"
            input_count = 0
        }
    }

    Invoke-ChildPowerShell -ScriptPath $ScriptPath `
        -Arguments @(
            "-SummaryJson"
            ($summaryPaths -join ",")
            "-OutputJson"
            $OutputJson
            "-OutputMarkdown"
            $OutputMarkdown
        ) `
        -FailureMessage "Failed to write project template schema approval history."

    return [pscustomobject]@{
        status = "completed"
        input_count = $summaryPaths.Count
    }
}
