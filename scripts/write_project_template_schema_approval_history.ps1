<#
.SYNOPSIS
Writes a schema approval history report from project template smoke or release summaries.

.DESCRIPTION
Reads one or more project template smoke summary.json files, or release-candidate
summary.json files containing a project_template_smoke block, and writes a stable
JSON plus Markdown trend report for schema approval gate status, approval counts,
and blocked approval items.
#>
param(
    [string[]]$SummaryJson = @(),
    [string]$SummaryJsonDir = "",
    [switch]$Recurse,
    [string]$OutputJson = "output/project-template-schema-approval-history/history.json",
    [string]$OutputMarkdown = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-schema-approval-history] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-InputPath {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $RepoRoot $Path
    }

    if ($AllowMissing) {
        return [System.IO.Path]::GetFullPath($candidate)
    }

    return (Resolve-Path -LiteralPath $candidate).Path
}

function Get-RepoRelativeDisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Get-LatestNonEmptyString {
    param(
        [object[]]$Values,
        [string]$DefaultValue = ""
    )

    $normalizedValues = @($Values)
    for ($index = $normalizedValues.Count - 1; $index -ge 0; $index -= 1) {
        $value = [string]($normalizedValues[$index])
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }

    return $DefaultValue
}

function Get-TemplateScope {
    param(
        [string]$ProjectId,
        [string]$TemplateName,
        [string]$Fallback = ""
    )

    if (-not [string]::IsNullOrWhiteSpace($ProjectId) -and
        -not [string]::IsNullOrWhiteSpace($TemplateName)) {
        return "$ProjectId/$TemplateName"
    }
    if (-not [string]::IsNullOrWhiteSpace($TemplateName)) {
        return $TemplateName
    }
    if (-not [string]::IsNullOrWhiteSpace($ProjectId)) {
        return $ProjectId
    }

    return $Fallback
}

function Ensure-PathParent {
    param([string]$Path)

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
}

function Get-OptionalPropertyValue {
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

function Get-OptionalStringProperty {
    param(
        [AllowNull()]$Object,
        [string]$Name,
        [string]$DefaultValue = ""
    )

    $value = Get-OptionalPropertyValue -Object $Object -Name $Name
    if ($null -eq $value) {
        return $DefaultValue
    }
    if ($value -is [System.DateTime]) {
        return $value.ToString("s", [System.Globalization.CultureInfo]::InvariantCulture)
    }

    return [string]$value
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

function Get-OptionalBooleanProperty {
    param(
        [AllowNull()]$Object,
        [string]$Name,
        [bool]$DefaultValue = $false
    )

    $value = Get-OptionalPropertyValue -Object $Object -Name $Name
    if ($null -eq $value) {
        return $DefaultValue
    }
    if ($value -is [bool]) {
        return [bool]$value
    }
    if ($value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($value)) {
            return $DefaultValue
        }

        return $value.Equals("true", [System.StringComparison]::OrdinalIgnoreCase)
    }

    return [bool]$value
}

function Get-OptionalArrayProperty {
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

function Test-ProjectTemplateSchemaApprovalSummary {
    param([string]$Path)

    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
    } catch {
        return $true
    }

    if ($null -ne (Get-OptionalPropertyValue -Object $summary -Name "project_template_smoke")) {
        return $true
    }

    $projectTemplateSummaryFields = @(
        "schema_patch_review_count",
        "schema_patch_review_changed_count",
        "schema_patch_approval_pending_count",
        "schema_patch_approval_approved_count",
        "schema_patch_approval_rejected_count",
        "schema_patch_approval_compliance_issue_count",
        "schema_patch_approval_invalid_result_count",
        "schema_patch_approval_gate_status",
        "schema_patch_approval_items"
    )
    foreach ($field in $projectTemplateSummaryFields) {
        if ($null -ne (Get-OptionalPropertyValue -Object $summary -Name $field)) {
            return $true
        }
    }

    return $false
}

function Get-SummaryInputPaths {
    param(
        [string]$RepoRoot,
        [string[]]$InputPaths,
        [string]$InputDir,
        [bool]$Recursive
    )

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @($InputPaths)) {
        foreach ($expandedPath in @([string]$path -split ',')) {
            $trimmedPath = $expandedPath.Trim()
            if (-not [string]::IsNullOrWhiteSpace($trimmedPath)) {
                [void]$paths.Add((Resolve-InputPath -RepoRoot $RepoRoot -Path $trimmedPath))
            }
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($InputDir)) {
        $resolvedDir = Resolve-InputPath -RepoRoot $RepoRoot -Path $InputDir
        $childArgs = @{
            LiteralPath = $resolvedDir
            File = $true
            Filter = "summary.json"
        }
        if ($Recursive) {
            $childArgs.Recurse = $true
        }
        foreach ($file in @(Get-ChildItem @childArgs | Sort-Object FullName)) {
            if (Test-ProjectTemplateSchemaApprovalSummary -Path $file.FullName) {
                [void]$paths.Add($file.FullName)
            }
        }
    }

    return @($paths.ToArray() | Sort-Object -Unique)
}

function New-SchemaApprovalHistoryApprovalItem {
    param(
        [string]$RepoRoot,
        $Item
    )

    $status = Get-OptionalStringProperty -Object $Item -Name "status"
    $complianceIssueCount = Get-OptionalIntegerProperty -Object $Item -Name "compliance_issue_count"
    $approvalResult = Get-OptionalStringProperty -Object $Item -Name "approval_result"
    $schemaUpdateCandidate = Get-OptionalStringProperty -Object $Item -Name "schema_update_candidate"
    $reviewJson = Get-OptionalStringProperty -Object $Item -Name "review_json"
    $name = Get-OptionalStringProperty -Object $Item -Name "name"
    $projectId = Get-OptionalStringProperty -Object $Item -Name "project_id"
    $templateName = Get-OptionalStringProperty -Object $Item -Name "template_name"

    return [pscustomobject]@{
        name = $name
        project_id = $projectId
        template_name = $templateName
        template_scope = Get-TemplateScope -ProjectId $projectId -TemplateName $templateName -Fallback $name
        candidate_type = Get-OptionalStringProperty -Object $Item -Name "candidate_type"
        status = $status
        decision = Get-OptionalStringProperty -Object $Item -Name "decision"
        action = Get-OptionalStringProperty -Object $Item -Name "action"
        compliance_issue_count = $complianceIssueCount
        compliance_issues = @(Get-OptionalArrayProperty -Object $Item -Name "compliance_issues")
        blocked = ($status -eq "invalid_result" -or $complianceIssueCount -gt 0)
        approval_result = $approvalResult
        approval_result_display = Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $approvalResult
        schema_update_candidate = $schemaUpdateCandidate
        schema_update_candidate_display = Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $schemaUpdateCandidate
        review_json = $reviewJson
        review_json_display = Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $reviewJson
    }
}

function New-SchemaApprovalHistoryRun {
    param(
        [string]$RepoRoot,
        [string]$SummaryPath,
        $Summary
    )

    $projectTemplateSmoke = Get-OptionalPropertyValue -Object $Summary -Name "project_template_smoke"
    $sourceKind = if ($null -ne $projectTemplateSmoke) {
        "release_candidate_summary"
    } else {
        $projectTemplateSmoke = $Summary
        "project_template_smoke_summary"
    }

    $approvalItems = @(Get-OptionalArrayProperty -Object $projectTemplateSmoke -Name "schema_patch_approval_items")
    $pendingCount = Get-OptionalIntegerProperty -Object $projectTemplateSmoke -Name "schema_patch_approval_pending_count"
    $approvedCount = Get-OptionalIntegerProperty -Object $projectTemplateSmoke -Name "schema_patch_approval_approved_count"
    $rejectedCount = Get-OptionalIntegerProperty -Object $projectTemplateSmoke -Name "schema_patch_approval_rejected_count"
    $complianceIssueCount = Get-OptionalIntegerProperty -Object $projectTemplateSmoke -Name "schema_patch_approval_compliance_issue_count"
    $invalidResultCount = Get-OptionalIntegerProperty -Object $projectTemplateSmoke -Name "schema_patch_approval_invalid_result_count"
    if ($invalidResultCount -eq 0 -and $approvalItems.Count -gt 0) {
        $invalidResultCount = @($approvalItems | Where-Object { [string](Get-OptionalPropertyValue -Object $_ -Name "status") -eq "invalid_result" }).Count
    }
    $approvalItemCount = Get-SchemaPatchApprovalItemCount `
        -PendingCount $pendingCount `
        -ApprovedCount $approvedCount `
        -RejectedCount $rejectedCount `
        -ApprovalItems $approvalItems

    $gateStatus = Get-OptionalStringProperty -Object $projectTemplateSmoke -Name "schema_patch_approval_gate_status"
    if ([string]::IsNullOrWhiteSpace($gateStatus)) {
        $gateStatus = Get-SchemaPatchApprovalGateStatus `
            -PendingCount $pendingCount `
            -ApprovalItemCount $approvalItemCount `
            -InvalidResultCount $invalidResultCount `
            -ComplianceIssueCount $complianceIssueCount
    }
    $gateBlocked = Get-OptionalBooleanProperty -Object $projectTemplateSmoke -Name "schema_patch_approval_gate_blocked" `
        -DefaultValue ($gateStatus -eq "blocked")

    $approvalItemSnapshots = New-Object 'System.Collections.Generic.List[object]'
    $blockedItems = New-Object 'System.Collections.Generic.List[object]'
    foreach ($item in $approvalItems) {
        $approvalItem = New-SchemaApprovalHistoryApprovalItem -RepoRoot $RepoRoot -Item $item
        [void]$approvalItemSnapshots.Add($approvalItem)
        if ([bool]$approvalItem.blocked) {
            [void]$blockedItems.Add($approvalItem)
        }
    }

    $generatedAt = Get-OptionalStringProperty -Object $Summary -Name "generated_at"
    if ([string]::IsNullOrWhiteSpace($generatedAt)) {
        $generatedAt = Get-OptionalStringProperty -Object $projectTemplateSmoke -Name "generated_at"
    }

    return [pscustomobject]@{
        generated_at = $generatedAt
        source_kind = $sourceKind
        summary_json = $SummaryPath
        summary_json_display = Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $SummaryPath
        execution_status = Get-OptionalStringProperty -Object $Summary -Name "execution_status"
        project_template_smoke_status = Get-OptionalStringProperty -Object (Get-OptionalPropertyValue -Object (Get-OptionalPropertyValue -Object $Summary -Name "steps") -Name "project_template_smoke") -Name "status"
        schema_patch_review_count = Get-OptionalIntegerProperty -Object $projectTemplateSmoke -Name "schema_patch_review_count"
        schema_patch_review_changed_count = Get-OptionalIntegerProperty -Object $projectTemplateSmoke -Name "schema_patch_review_changed_count"
        schema_patch_approval_pending_count = $pendingCount
        schema_patch_approval_approved_count = $approvedCount
        schema_patch_approval_rejected_count = $rejectedCount
        schema_patch_approval_compliance_issue_count = $complianceIssueCount
        schema_patch_approval_invalid_result_count = $invalidResultCount
        schema_patch_approval_item_count = $approvalItemCount
        schema_patch_approval_gate_status = $gateStatus
        schema_patch_approval_gate_blocked = $gateBlocked
        approval_items = $approvalItemSnapshots.ToArray()
        blocked_items = $blockedItems.ToArray()
    }
}

function New-LatestSchemaApprovalBlockingSummary {
    param([object[]]$Runs)

    $blockedRuns = @($Runs | Where-Object {
            [bool]$_.schema_patch_approval_gate_blocked -or @($_.blocked_items).Count -gt 0
        })
    if ($blockedRuns.Count -eq 0) {
        return [pscustomobject]@{
            status = "none"
            generated_at = ""
            gate_status = "not_required"
            summary_json = ""
            summary_json_display = "(not available)"
            blocked_item_count = 0
            compliance_issue_count = 0
            invalid_result_count = 0
            issue_keys = @()
            items = @()
        }
    }

    $latestBlockedRun = $blockedRuns[-1]
    $blockedItems = @($latestBlockedRun.blocked_items)
    $issueKeys = @($blockedItems |
        ForEach-Object { @($_.compliance_issues) } |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
        Sort-Object -Unique)

    return [pscustomobject]@{
        status = "blocked"
        generated_at = [string]$latestBlockedRun.generated_at
        gate_status = [string]$latestBlockedRun.schema_patch_approval_gate_status
        summary_json = [string]$latestBlockedRun.summary_json
        summary_json_display = [string]$latestBlockedRun.summary_json_display
        blocked_item_count = $blockedItems.Count
        compliance_issue_count = [int]$latestBlockedRun.schema_patch_approval_compliance_issue_count
        invalid_result_count = [int]$latestBlockedRun.schema_patch_approval_invalid_result_count
        issue_keys = $issueKeys
        items = $blockedItems
    }
}

function New-SchemaApprovalEntryHistories {
    param([object[]]$Runs)

    $entryNames = @($Runs |
        ForEach-Object { @($_.approval_items) } |
        ForEach-Object { [string]$_.name } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Sort-Object -Unique)

    $histories = New-Object 'System.Collections.Generic.List[object]'
    foreach ($entryName in $entryNames) {
        $snapshots = New-Object 'System.Collections.Generic.List[object]'
        foreach ($run in $Runs) {
            foreach ($item in @($run.approval_items)) {
                if ([string]$item.name -eq $entryName) {
                    [void]$snapshots.Add([pscustomobject]@{
                        generated_at = [string]$run.generated_at
                        summary_json = [string]$run.summary_json
                        summary_json_display = [string]$run.summary_json_display
                        project_id = [string]$item.project_id
                        template_name = [string]$item.template_name
                        template_scope = [string]$item.template_scope
                        candidate_type = [string]$item.candidate_type
                        status = [string]$item.status
                        decision = [string]$item.decision
                        action = [string]$item.action
                        compliance_issue_count = [int]$item.compliance_issue_count
                        compliance_issues = @($item.compliance_issues)
                        blocked = [bool]$item.blocked
                        approval_result = [string]$item.approval_result
                        approval_result_display = [string]$item.approval_result_display
                        schema_update_candidate = [string]$item.schema_update_candidate
                        schema_update_candidate_display = [string]$item.schema_update_candidate_display
                        review_json = [string]$item.review_json
                        review_json_display = [string]$item.review_json_display
                    })
                }
            }
        }

        $orderedSnapshots = @($snapshots.ToArray() | Sort-Object generated_at, summary_json)
        if ($orderedSnapshots.Count -eq 0) {
            continue
        }
        $latestSnapshot = $orderedSnapshots[-1]
        $projectId = Get-LatestNonEmptyString -Values @($orderedSnapshots | ForEach-Object { [string]$_.project_id })
        $templateName = Get-LatestNonEmptyString -Values @($orderedSnapshots | ForEach-Object { [string]$_.template_name }) -DefaultValue $entryName
        $templateScope = Get-TemplateScope -ProjectId $projectId -TemplateName $templateName -Fallback $entryName
        $candidateType = Get-LatestNonEmptyString -Values @($orderedSnapshots | ForEach-Object { [string]$_.candidate_type })
        $issueKeys = @($orderedSnapshots |
            ForEach-Object { @($_.compliance_issues) } |
            Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
            Sort-Object -Unique)

        [void]$histories.Add([pscustomobject]@{
            name = $entryName
            project_id = $projectId
            template_name = $templateName
            template_scope = $templateScope
            candidate_type = $candidateType
            run_count = $orderedSnapshots.Count
            blocked_run_count = @($orderedSnapshots | Where-Object { [bool]$_.blocked }).Count
            pending_run_count = @($orderedSnapshots | Where-Object { $_.decision -eq "pending" -or $_.status -eq "pending_review" }).Count
            approved_run_count = @($orderedSnapshots | Where-Object { $_.decision -eq "approved" }).Count
            rejected_run_count = @($orderedSnapshots | Where-Object { $_.decision -eq "rejected" }).Count
            needs_changes_run_count = @($orderedSnapshots | Where-Object { $_.decision -eq "needs_changes" }).Count
            invalid_result_run_count = @($orderedSnapshots | Where-Object { $_.status -eq "invalid_result" }).Count
            issue_keys = $issueKeys
            latest_generated_at = [string]$latestSnapshot.generated_at
            latest_status = [string]$latestSnapshot.status
            latest_decision = [string]$latestSnapshot.decision
            latest_action = [string]$latestSnapshot.action
            latest_blocked = [bool]$latestSnapshot.blocked
            latest_compliance_issue_count = [int]$latestSnapshot.compliance_issue_count
            latest_compliance_issues = @($latestSnapshot.compliance_issues)
            latest_summary_json = [string]$latestSnapshot.summary_json
            latest_summary_json_display = [string]$latestSnapshot.summary_json_display
            latest_approval_result = [string]$latestSnapshot.approval_result
            latest_approval_result_display = [string]$latestSnapshot.approval_result_display
            runs = $orderedSnapshots
        })
    }

    return @($histories.ToArray() | Sort-Object name)
}

function Get-SchemaApprovalEntryLatestReviewState {
    param($Entry)

    $status = [string]$Entry.latest_status
    $decision = [string]$Entry.latest_decision
    if ([bool]$Entry.latest_blocked -or $status -eq "invalid_result" -or [int]$Entry.latest_compliance_issue_count -gt 0) {
        return "blocked"
    }
    if ($decision -eq "pending" -or $status -eq "pending_review") {
        return "pending"
    }
    if ($decision -in @("rejected", "needs_changes") -or $status -in @("rejected", "needs_changes")) {
        return "rejected"
    }
    if ($decision -eq "approved" -or $status -eq "approved") {
        return "approved"
    }
    if ($decision -eq "not_required" -or $status -eq "not_required") {
        return "not_required"
    }

    return "unknown"
}

function Get-SchemaApprovalMatrixReviewState {
    param([string[]]$States)

    if (@($States | Where-Object { $_ -eq "blocked" }).Count -gt 0) {
        return "blocked"
    }
    if (@($States | Where-Object { $_ -eq "pending" }).Count -gt 0) {
        return "pending"
    }
    if (@($States | Where-Object { $_ -eq "rejected" }).Count -gt 0) {
        return "rejected"
    }
    if (@($States | Where-Object { $_ -eq "approved" }).Count -gt 0) {
        return "approved"
    }
    if (@($States | Where-Object { $_ -eq "not_required" }).Count -gt 0) {
        return "not_required"
    }

    return "unknown"
}

function Get-SchemaApprovalMatrixReviewerActionSummary {
    param(
        [string]$ReviewState,
        [string[]]$LatestActions
    )

    $actions = @($LatestActions |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
        Sort-Object -Unique)
    if ($ReviewState -in @("approved", "not_required")) {
        return "none"
    }
    if ($actions.Count -gt 0) {
        return ($actions -join ",")
    }

    switch ($ReviewState) {
        "blocked" { return "fix_blocking_schema_approval" }
        "pending" { return "review_pending_schema_approval" }
        "rejected" { return "revise_rejected_schema_approval" }
        default { return "inspect_schema_approval_history" }
    }
}

function New-SchemaApprovalReviewMatrix {
    param([object[]]$EntryHistories)

    $scopes = @($EntryHistories |
        ForEach-Object { [string]$_.template_scope } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Sort-Object -Unique)

    $matrix = New-Object 'System.Collections.Generic.List[object]'
    foreach ($scope in $scopes) {
        $scopeEntries = @($EntryHistories | Where-Object { [string]$_.template_scope -eq $scope } | Sort-Object name)
        if ($scopeEntries.Count -eq 0) {
            continue
        }

        $latestOrderedEntries = @($scopeEntries | Sort-Object latest_generated_at, latest_summary_json_display, name)
        $latestEntry = $latestOrderedEntries[-1]
        $latestStates = @($scopeEntries | ForEach-Object { Get-SchemaApprovalEntryLatestReviewState -Entry $_ })
        $reviewState = Get-SchemaApprovalMatrixReviewState -States $latestStates
        $issueKeys = @($scopeEntries |
            ForEach-Object { @($_.issue_keys) } |
            Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
            Sort-Object -Unique)
        $latestActions = @($scopeEntries |
            ForEach-Object { [string]$_.latest_action } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Sort-Object -Unique)
        $requiresReviewerAction = ($reviewState -notin @("approved", "not_required"))
        $reviewerActionSummary = Get-SchemaApprovalMatrixReviewerActionSummary `
            -ReviewState $reviewState `
            -LatestActions $latestActions
        $reviewerActions = if ($requiresReviewerAction -and $latestActions.Count -gt 0) {
            @($latestActions)
        } elseif ($requiresReviewerAction -and $reviewerActionSummary -ne "none") {
            @($reviewerActionSummary)
        } else {
            @()
        }
        $issueSummary = if ($issueKeys.Count -gt 0) { $issueKeys -join "," } else { "(none)" }
        $reviewerActionReason = if ($requiresReviewerAction) {
            "latest_review_state=$reviewState; issue_keys=$issueSummary"
        } else {
            "latest_review_state=$reviewState; no reviewer action required"
        }

        $entryDetails = @(
            foreach ($entry in $scopeEntries) {
                [pscustomobject]@{
                    name = [string]$entry.name
                    latest_review_state = Get-SchemaApprovalEntryLatestReviewState -Entry $entry
                    latest_status = [string]$entry.latest_status
                    latest_decision = [string]$entry.latest_decision
                    latest_action = [string]$entry.latest_action
                    run_count = [int]$entry.run_count
                    blocked_run_count = [int]$entry.blocked_run_count
                    latest_summary_json_display = [string]$entry.latest_summary_json_display
                }
            }
        )

        [void]$matrix.Add([pscustomobject]@{
            template_scope = $scope
            project_id = Get-LatestNonEmptyString -Values @($scopeEntries | ForEach-Object { [string]$_.project_id })
            template_name = Get-LatestNonEmptyString -Values @($scopeEntries | ForEach-Object { [string]$_.template_name }) -DefaultValue $scope
            latest_review_state = $reviewState
            requires_reviewer_action = $requiresReviewerAction
            reviewer_action_summary = $reviewerActionSummary
            reviewer_action_reason = $reviewerActionReason
            reviewer_actions = @($reviewerActions)
            entry_count = $scopeEntries.Count
            run_count = [int](@($scopeEntries | Measure-Object -Property run_count -Sum).Sum)
            historical_blocked_run_count = [int](@($scopeEntries | Measure-Object -Property blocked_run_count -Sum).Sum)
            latest_blocked_entry_count = @($latestStates | Where-Object { $_ -eq "blocked" }).Count
            latest_pending_entry_count = @($latestStates | Where-Object { $_ -eq "pending" }).Count
            latest_rejected_entry_count = @($latestStates | Where-Object { $_ -eq "rejected" }).Count
            latest_approved_entry_count = @($latestStates | Where-Object { $_ -eq "approved" }).Count
            latest_not_required_entry_count = @($latestStates | Where-Object { $_ -eq "not_required" }).Count
            issue_keys = $issueKeys
            latest_actions = $latestActions
            latest_generated_at = [string]$latestEntry.latest_generated_at
            latest_summary_json_display = [string]$latestEntry.latest_summary_json_display
            entries = $entryDetails
        })
    }

    return @($matrix.ToArray() | Sort-Object template_scope)
}

function Write-HistoryMarkdown {
    param(
        [string]$Path,
        [string]$RepoRoot,
        $History
    )

    $lines = New-Object 'System.Collections.Generic.List[string]'
    [void]$lines.Add("# Project Template Schema Approval History")
    [void]$lines.Add("")
    [void]$lines.Add("- Generated at: $($History.generated_at)")
    [void]$lines.Add("- Summary count: $($History.summary_count)")
    [void]$lines.Add("- Latest gate status: $($History.latest_gate_status)")
    [void]$lines.Add("- Blocked runs: $($History.blocked_run_count)")
    [void]$lines.Add("- Pending runs: $($History.pending_run_count)")
    [void]$lines.Add("- Passed runs: $($History.passed_run_count)")
    [void]$lines.Add("- Total compliance issues: $($History.total_schema_patch_approval_compliance_issue_count)")
    [void]$lines.Add("- Total invalid approval results: $($History.total_schema_patch_approval_invalid_result_count)")
    [void]$lines.Add("- Project-template approval matrix templates: $($History.project_template_approval_matrix_template_count)")
    [void]$lines.Add("- Project-template approval matrix projects: $($History.project_template_approval_matrix_project_count)")
    [void]$lines.Add("- Project-template approval matrix latest pending templates: $($History.project_template_approval_matrix_latest_pending_template_count)")
    [void]$lines.Add("- Project-template approval matrix latest blocked templates: $($History.project_template_approval_matrix_latest_blocked_template_count)")
    if ($History.latest_blocking_summary.status -eq "blocked") {
        $latestIssues = @($History.latest_blocking_summary.issue_keys) -join ","
        if ([string]::IsNullOrWhiteSpace($latestIssues)) {
            $latestIssues = "(none)"
        }
        [void]$lines.Add("- Latest blocking summary: $($History.latest_blocking_summary.generated_at) blocked_items=$($History.latest_blocking_summary.blocked_item_count) compliance_issues=$($History.latest_blocking_summary.compliance_issue_count) invalid_results=$($History.latest_blocking_summary.invalid_result_count) issues=$latestIssues")
    } else {
        [void]$lines.Add("- Latest blocking summary: none")
    }
    [void]$lines.Add("")
    [void]$lines.Add("## Latest Blocking Summary")
    [void]$lines.Add("")
    if ($History.latest_blocking_summary.status -eq "blocked") {
        [void]$lines.Add("- Run: $($History.latest_blocking_summary.generated_at) gate=$($History.latest_blocking_summary.gate_status) summary=$($History.latest_blocking_summary.summary_json_display)")
        [void]$lines.Add("- Counts: blocked_items=$($History.latest_blocking_summary.blocked_item_count) compliance_issues=$($History.latest_blocking_summary.compliance_issue_count) invalid_results=$($History.latest_blocking_summary.invalid_result_count)")
        foreach ($item in @($History.latest_blocking_summary.items)) {
            $issues = @($item.compliance_issues) -join ","
            if ([string]::IsNullOrWhiteSpace($issues)) {
                $issues = "(none)"
            }
            [void]$lines.Add("- $($item.name): status=$($item.status) decision=$($item.decision) issues=$issues action=$($item.action) approval=$($item.approval_result_display)")
        }
    } else {
        [void]$lines.Add("- No blocked schema approval run found in the selected summaries.")
    }
    [void]$lines.Add("")
    [void]$lines.Add("## Project Template Approval Matrix")
    [void]$lines.Add("")
    if (@($History.project_template_approval_matrix).Count -gt 0) {
        foreach ($item in @($History.project_template_approval_matrix)) {
            $actions = @($item.latest_actions) -join ","
            if ([string]::IsNullOrWhiteSpace($actions)) {
                $actions = "(none)"
            }
            $issues = @($item.issue_keys) -join ","
            if ([string]::IsNullOrWhiteSpace($issues)) {
                $issues = "(none)"
            }
            [void]$lines.Add("- $($item.template_scope): state=$($item.latest_review_state) project=$($item.project_id) template=$($item.template_name) entries=$($item.entry_count) runs=$($item.run_count) historical_blocked_runs=$($item.historical_blocked_run_count) latest_blocked=$($item.latest_blocked_entry_count) latest_pending=$($item.latest_pending_entry_count) latest_rejected=$($item.latest_rejected_entry_count) latest_approved=$($item.latest_approved_entry_count) actions=$actions reviewer_action=$($item.reviewer_action_summary) reviewer_reason=$($item.reviewer_action_reason) issues=$issues summary=$($item.latest_summary_json_display)")
        }
    } else {
        [void]$lines.Add("- No project-template approval matrix entries found in the selected summaries.")
    }
    [void]$lines.Add("")
    [void]$lines.Add("## Entry History")
    [void]$lines.Add("")
    if (@($History.entry_histories).Count -gt 0) {
        foreach ($entry in @($History.entry_histories)) {
            $issues = @($entry.issue_keys) -join ","
            if ([string]::IsNullOrWhiteSpace($issues)) {
                $issues = "(none)"
            }
            [void]$lines.Add("- $($entry.name): latest=$($entry.latest_status)/$($entry.latest_decision) runs=$($entry.run_count) blocked_runs=$($entry.blocked_run_count) pending=$($entry.pending_run_count) approved=$($entry.approved_run_count) rejected=$($entry.rejected_run_count) needs_changes=$($entry.needs_changes_run_count) invalid_results=$($entry.invalid_result_run_count) issues=$issues summary=$($entry.latest_summary_json_display)")
        }
    } else {
        [void]$lines.Add("- No schema approval entries found in the selected summaries.")
    }
    [void]$lines.Add("")
    [void]$lines.Add("## Runs")
    [void]$lines.Add("")
    foreach ($run in @($History.runs)) {
        [void]$lines.Add("- $($run.generated_at): gate=$($run.schema_patch_approval_gate_status) pending=$($run.schema_patch_approval_pending_count) approved=$($run.schema_patch_approval_approved_count) rejected=$($run.schema_patch_approval_rejected_count) compliance_issues=$($run.schema_patch_approval_compliance_issue_count) invalid_results=$($run.schema_patch_approval_invalid_result_count) summary=$($run.summary_json_display)")
    }

    $blockedRuns = @($History.runs | Where-Object { @($_.blocked_items).Count -gt 0 })
    if ($blockedRuns.Count -gt 0) {
        [void]$lines.Add("")
        [void]$lines.Add("## Blocked Approval Items")
        [void]$lines.Add("")
        foreach ($run in $blockedRuns) {
            foreach ($item in @($run.blocked_items)) {
                $issues = @($item.compliance_issues) -join ","
                if ([string]::IsNullOrWhiteSpace($issues)) {
                    $issues = "(none)"
                }
                [void]$lines.Add("- $($run.generated_at) / $($item.name): status=$($item.status) decision=$($item.decision) issues=$issues action=$($item.action) approval=$($item.approval_result_display)")
            }
        }
    }

    Ensure-PathParent -Path $Path
    $lines | Set-Content -LiteralPath $Path -Encoding UTF8
}

$repoRoot = Resolve-RepoRoot
$resolvedOutputJson = Resolve-InputPath -RepoRoot $repoRoot -Path $OutputJson -AllowMissing
$resolvedOutputMarkdown = if ([string]::IsNullOrWhiteSpace($OutputMarkdown)) {
    [System.IO.Path]::ChangeExtension($resolvedOutputJson, ".md")
} else {
    Resolve-InputPath -RepoRoot $repoRoot -Path $OutputMarkdown -AllowMissing
}

$summaryPaths = @(Get-SummaryInputPaths `
    -RepoRoot $repoRoot `
    -InputPaths $SummaryJson `
    -InputDir $SummaryJsonDir `
    -Recursive ([bool]$Recurse))
if ($summaryPaths.Count -eq 0) {
    throw "At least one -SummaryJson or -SummaryJsonDir input is required."
}

$runs = New-Object 'System.Collections.Generic.List[object]'
foreach ($summaryPath in $summaryPaths) {
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    [void]$runs.Add((New-SchemaApprovalHistoryRun `
        -RepoRoot $repoRoot `
        -SummaryPath $summaryPath `
        -Summary $summary))
}

$sortedRuns = @($runs.ToArray() | Sort-Object generated_at, summary_json)
$latestRun = if ($sortedRuns.Count -gt 0) { $sortedRuns[-1] } else { $null }
$latestBlockingSummary = New-LatestSchemaApprovalBlockingSummary -Runs $sortedRuns
$entryHistories = @(New-SchemaApprovalEntryHistories -Runs $sortedRuns)
$projectTemplateApprovalMatrix = @(New-SchemaApprovalReviewMatrix -EntryHistories $entryHistories)
$history = [ordered]@{
    schema = "featherdoc.project_template_schema_approval_history.v1"
    generated_at = (Get-Date).ToString("s")
    summary_count = $sortedRuns.Count
    blocked_run_count = @($sortedRuns | Where-Object { [bool]$_.schema_patch_approval_gate_blocked }).Count
    pending_run_count = @($sortedRuns | Where-Object { $_.schema_patch_approval_gate_status -eq "pending" }).Count
    passed_run_count = @($sortedRuns | Where-Object { $_.schema_patch_approval_gate_status -eq "passed" }).Count
    not_required_run_count = @($sortedRuns | Where-Object { $_.schema_patch_approval_gate_status -eq "not_required" }).Count
    total_schema_patch_review_changed_count = @($sortedRuns | Measure-Object -Property schema_patch_review_changed_count -Sum).Sum
    total_schema_patch_approval_item_count = @($sortedRuns | Measure-Object -Property schema_patch_approval_item_count -Sum).Sum
    total_schema_patch_approval_compliance_issue_count = @($sortedRuns | Measure-Object -Property schema_patch_approval_compliance_issue_count -Sum).Sum
    total_schema_patch_approval_invalid_result_count = @($sortedRuns | Measure-Object -Property schema_patch_approval_invalid_result_count -Sum).Sum
    latest_gate_status = if ($null -ne $latestRun) { [string]$latestRun.schema_patch_approval_gate_status } else { "not_required" }
    latest_summary_json = if ($null -ne $latestRun) { [string]$latestRun.summary_json } else { "" }
    latest_blocking_summary = $latestBlockingSummary
    project_template_approval_matrix_template_count = $projectTemplateApprovalMatrix.Count
    project_template_approval_matrix_project_count = @($projectTemplateApprovalMatrix | ForEach-Object { [string]$_.project_id } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique).Count
    project_template_approval_matrix_latest_pending_template_count = @($projectTemplateApprovalMatrix | Where-Object { $_.latest_review_state -eq "pending" }).Count
    project_template_approval_matrix_latest_blocked_template_count = @($projectTemplateApprovalMatrix | Where-Object { $_.latest_review_state -eq "blocked" }).Count
    project_template_approval_matrix_latest_rejected_template_count = @($projectTemplateApprovalMatrix | Where-Object { $_.latest_review_state -eq "rejected" }).Count
    project_template_approval_matrix_latest_approved_template_count = @($projectTemplateApprovalMatrix | Where-Object { $_.latest_review_state -eq "approved" }).Count
    project_template_approval_matrix_missing_project_metadata_count = @($projectTemplateApprovalMatrix | Where-Object { [string]::IsNullOrWhiteSpace([string]$_.project_id) }).Count
    project_template_approval_matrix = $projectTemplateApprovalMatrix
    entry_histories = $entryHistories
    runs = $sortedRuns
}

Ensure-PathParent -Path $resolvedOutputJson
($history | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8
Write-HistoryMarkdown -Path $resolvedOutputMarkdown -RepoRoot $repoRoot -History ([pscustomobject]$history)

Write-Step "History JSON: $resolvedOutputJson"
Write-Step "History Markdown: $resolvedOutputMarkdown"
