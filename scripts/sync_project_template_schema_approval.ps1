<#
.SYNOPSIS
Synchronizes project template smoke summaries with schema patch approval decisions.

.DESCRIPTION
Reads a summary.json produced by run_project_template_smoke.ps1, reloads each
schema_patch_approval_result.json referenced by schema-baseline checks, refreshes
schema_patch_approval fields, recomputes aggregate approval counts, and rewrites
summary.json plus summary.md.

Reviewers can either edit schema_patch_approval_result.json manually and run this
script, or pass -EntryName and -Decision to update a specific approval result.
#>
param(
    [string]$SummaryJson = "output/project-template-smoke/summary.json",
    [string]$SummaryMarkdown = "",
    [string]$EntryName = "",
    [ValidateSet("", "pending", "approved", "rejected", "needs_changes")]
    [string]$Decision = "",
    [string]$Reviewer = "",
    [string]$ReviewedAt = "",
    [string]$Note = "",
    [string]$ReleaseCandidateSummaryJson = "",
    [switch]$RefreshReleaseBundle
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "project_template_smoke_manifest_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-schema-approval-sync] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    return Resolve-ProjectTemplateSmokePath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing:$AllowMissing
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

function Ensure-PathParent {
    param([string]$Path)

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
}

function Set-ProjectTemplateSmokePropertyValue {
    param(
        $Object,
        [string]$Name,
        $Value
    )

    if ($Object -is [System.Collections.IDictionary]) {
        $Object[$Name] = $Value
        return
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        Add-Member -InputObject $Object -NotePropertyName $Name -NotePropertyValue $Value
    } else {
        $property.Value = $Value
    }
}

function Invoke-ChildPowerShell {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments,
        [string]$FailureMessage
    )

    $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE

    foreach ($line in $commandOutput) {
        Write-Host $line
    }

    if ($exitCode -ne 0) {
        throw $FailureMessage
    }
}

function Read-JsonFileIfPresent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Normalize-SchemaPatchApprovalDecision {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "pending"
    }

    switch ($Value.Trim().ToLowerInvariant()) {
        "approve" { return "approved" }
        "approved" { return "approved" }
        "accept" { return "approved" }
        "accepted" { return "approved" }
        "reject" { return "rejected" }
        "rejected" { return "rejected" }
        "needs_changes" { return "needs_changes" }
        "needs-changes" { return "needs_changes" }
        "changes_requested" { return "needs_changes" }
        "pending" { return "pending" }
        "pending_review" { return "pending" }
        default { return "invalid" }
    }
}

function Test-SchemaPatchApprovalResultCompliance {
    param(
        [string]$Decision,
        $ApprovalResult
    )

    $issues = New-Object 'System.Collections.Generic.List[string]'
    if ($Decision -in @("approved", "rejected", "needs_changes")) {
        $reviewer = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $ApprovalResult -Name "reviewer"
        if ([string]::IsNullOrWhiteSpace($reviewer)) {
            [void]$issues.Add("missing_reviewer")
        }

        $reviewedAt = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $ApprovalResult -Name "reviewed_at"
        if ([string]::IsNullOrWhiteSpace($reviewedAt)) {
            [void]$issues.Add("missing_reviewed_at")
        }
    }

    return [pscustomobject]@{
        issue_count = $issues.Count
        issues = $issues.ToArray()
    }
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

function New-SchemaPatchApprovalResultTemplate {
    param(
        [string]$Name,
        $SchemaBaseline
    )

    return [ordered]@{
        schema = "featherdoc.template_schema_patch_approval_result.v1"
        entry_name = $Name
        decision = "pending"
        reviewer = ""
        reviewed_at = ""
        note = ""
        schema_file = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "schema_file"
        schema_update_candidate = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "generated_output"
        review_json = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "schema_patch_review_output_path"
    }
}

function Get-SchemaPatchApprovalResultPath {
    param(
        $Entry,
        $SchemaBaseline
    )

    $approvalResultPath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "schema_patch_approval_result_path"
    if (-not [string]::IsNullOrWhiteSpace($approvalResultPath)) {
        return $approvalResultPath
    }

    $approval = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $SchemaBaseline -Name "schema_patch_approval"
    $approvalResultPath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $approval -Name "approval_result"
    if (-not [string]::IsNullOrWhiteSpace($approvalResultPath)) {
        return $approvalResultPath
    }

    $artifactDir = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Entry -Name "artifact_dir"
    if (-not [string]::IsNullOrWhiteSpace($artifactDir)) {
        return (Join-Path $artifactDir "schema_patch_approval_result.json")
    }

    return ""
}

function Write-SchemaPatchApprovalResult {
    param(
        [string]$Path,
        [string]$EntryName,
        $SchemaBaseline,
        [string]$DecisionValue,
        [string]$ReviewerValue,
        [string]$ReviewedAtValue,
        [string]$NoteValue
    )

    $approvalResult = Read-JsonFileIfPresent -Path $Path
    if ($null -eq $approvalResult) {
        $approvalResult = New-SchemaPatchApprovalResultTemplate `
            -Name $EntryName `
            -SchemaBaseline $SchemaBaseline
    }

    Set-ProjectTemplateSmokePropertyValue -Object $approvalResult -Name "schema" -Value "featherdoc.template_schema_patch_approval_result.v1"
    Set-ProjectTemplateSmokePropertyValue -Object $approvalResult -Name "entry_name" -Value $EntryName
    Set-ProjectTemplateSmokePropertyValue -Object $approvalResult -Name "decision" -Value (Normalize-SchemaPatchApprovalDecision -Value $DecisionValue)
    if (-not [string]::IsNullOrWhiteSpace($ReviewerValue)) {
        Set-ProjectTemplateSmokePropertyValue -Object $approvalResult -Name "reviewer" -Value $ReviewerValue
    }
    if (-not [string]::IsNullOrWhiteSpace($ReviewedAtValue)) {
        Set-ProjectTemplateSmokePropertyValue -Object $approvalResult -Name "reviewed_at" -Value $ReviewedAtValue
    } elseif ($DecisionValue -ne "pending") {
        Set-ProjectTemplateSmokePropertyValue -Object $approvalResult -Name "reviewed_at" -Value (Get-Date).ToString("s")
    }
    if (-not [string]::IsNullOrWhiteSpace($NoteValue)) {
        Set-ProjectTemplateSmokePropertyValue -Object $approvalResult -Name "note" -Value $NoteValue
    }
    Set-ProjectTemplateSmokePropertyValue -Object $approvalResult -Name "schema_file" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "schema_file")
    Set-ProjectTemplateSmokePropertyValue -Object $approvalResult -Name "schema_update_candidate" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "generated_output")
    Set-ProjectTemplateSmokePropertyValue -Object $approvalResult -Name "review_json" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "schema_patch_review_output_path")

    Ensure-PathParent -Path $Path
    ($approvalResult | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $Path -Encoding UTF8
    return $approvalResult
}

function New-SchemaPatchApprovalSummary {
    param(
        [string]$Name,
        $SchemaBaseline,
        $ApprovalResult,
        [string]$ApprovalResultPath
    )

    $reviewSummary = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $SchemaBaseline -Name "schema_patch_review"
    if ($null -eq $reviewSummary) {
        return $null
    }

    $changed = [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $reviewSummary -Name "changed")
    $decision = if ($changed) {
        Normalize-SchemaPatchApprovalDecision -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $ApprovalResult -Name "decision")
    } else {
        "not_required"
    }

    $compliance = Test-SchemaPatchApprovalResultCompliance `
        -Decision $decision `
        -ApprovalResult $ApprovalResult
    $status = switch ($decision) {
        "not_required" { "not_required" }
        "approved" { "approved" }
        "rejected" { "rejected" }
        "needs_changes" { "needs_changes" }
        "invalid" { "invalid_result" }
        default { "pending_review" }
    }
    if ($changed -and [int]$compliance.issue_count -gt 0) {
        $status = "invalid_result"
    }
    $action = switch ($status) {
        "not_required" { "none" }
        "approved" { "promote_schema_update_candidate" }
        "rejected" { "update_template_or_schema_before_retry" }
        "needs_changes" { "update_template_or_schema_before_retry" }
        "invalid_result" { "fix_schema_patch_approval_result" }
        default { "review_schema_update_candidate" }
    }
    $pending = $changed -and $status -in @("pending_review", "invalid_result")

    return [pscustomobject]@{
        name = $Name
        required = $changed
        pending = $pending
        approved = ($changed -and $status -eq "approved")
        status = $status
        decision = $decision
        action = $action
        compliance_issue_count = [int]$compliance.issue_count
        compliance_issues = $compliance.issues
        schema_file = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "schema_file"
        schema_update_candidate = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "generated_output"
        review_json = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "schema_patch_review_output_path"
        approval_result = $ApprovalResultPath
        repaired_schema_candidate = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SchemaBaseline -Name "repaired_schema_output_path"
        reviewer = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $ApprovalResult -Name "reviewer"
        reviewed_at = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $ApprovalResult -Name "reviewed_at"
        note = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $ApprovalResult -Name "note"
        baseline_slot_count = [int](Get-ProjectTemplateSmokeOptionalPropertyValue -Object $reviewSummary -Name "baseline_slot_count")
        generated_slot_count = [int](Get-ProjectTemplateSmokeOptionalPropertyValue -Object $reviewSummary -Name "generated_slot_count")
        upsert_slot_count = [int](Get-ProjectTemplateSmokeOptionalPropertyValue -Object $reviewSummary -Name "upsert_slot_count")
        remove_target_count = [int](Get-ProjectTemplateSmokeOptionalPropertyValue -Object $reviewSummary -Name "remove_target_count")
        remove_slot_count = [int](Get-ProjectTemplateSmokeOptionalPropertyValue -Object $reviewSummary -Name "remove_slot_count")
        rename_slot_count = [int](Get-ProjectTemplateSmokeOptionalPropertyValue -Object $reviewSummary -Name "rename_slot_count")
        update_slot_count = [int](Get-ProjectTemplateSmokeOptionalPropertyValue -Object $reviewSummary -Name "update_slot_count")
        inserted_slots = [int](Get-ProjectTemplateSmokeOptionalPropertyValue -Object $reviewSummary -Name "inserted_slots")
        replaced_slots = [int](Get-ProjectTemplateSmokeOptionalPropertyValue -Object $reviewSummary -Name "replaced_slots")
    }
}

function Write-SummaryMarkdown {
    param(
        [string]$Path,
        [string]$RepoRoot,
        $Summary
    )

    $lines = New-Object 'System.Collections.Generic.List[string]'
    [void]$lines.Add("# Project Template Smoke")
    [void]$lines.Add("")
    [void]$lines.Add("- Generated at: $($Summary.generated_at)")
    $approvalSyncedAt = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "schema_patch_approval_synced_at"
    if (-not [string]::IsNullOrWhiteSpace($approvalSyncedAt)) {
        [void]$lines.Add("- Schema patch approval synced at: $approvalSyncedAt")
    }
    [void]$lines.Add("- Manifest: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "manifest_path"))")
    [void]$lines.Add("- Output directory: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "output_dir"))")
    [void]$lines.Add("- Overall status: $($Summary.overall_status)")
    [void]$lines.Add("- Passed flag: $($Summary.passed)")
    [void]$lines.Add("- Entry count: $($Summary.entry_count)")
    [void]$lines.Add("- Failed entries: $($Summary.failed_entry_count)")
    [void]$lines.Add("- Dirty schema baselines: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "dirty_schema_baseline_count")")
    [void]$lines.Add("- Schema patch reviews: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "schema_patch_review_count")")
    [void]$lines.Add("- Changed schema patch reviews: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "schema_patch_review_changed_count")")
    [void]$lines.Add("- Schema patch approvals pending: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "schema_patch_approval_pending_count")")
    [void]$lines.Add("- Schema patch approvals approved: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "schema_patch_approval_approved_count")")
    [void]$lines.Add("- Schema patch approvals rejected: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "schema_patch_approval_rejected_count")")
    [void]$lines.Add("- Schema patch approval compliance issues: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "schema_patch_approval_compliance_issue_count")")
    [void]$lines.Add("- Schema patch approval invalid results: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "schema_patch_approval_invalid_result_count")")
    [void]$lines.Add("- Schema patch approval gate status: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "schema_patch_approval_gate_status")")
    [void]$lines.Add("- Visual smoke entries: $($Summary.visual_entry_count)")
    [void]$lines.Add("- Visual verdict: $($Summary.visual_verdict)")
    [void]$lines.Add("- Entries with pending visual review: $($Summary.manual_review_pending_count)")
    [void]$lines.Add("- Entries with undetermined visual review: $($Summary.visual_review_undetermined_count)")
    [void]$lines.Add("")

    $approvalItems = @(Get-ProjectTemplateSmokeArrayProperty -Object $Summary -Name "schema_patch_approval_items")
    if ($approvalItems.Count -gt 0) {
        [void]$lines.Add("## Schema Patch Approvals")
        [void]$lines.Add("")
        foreach ($approval in $approvalItems) {
            $complianceIssues = @(Get-ProjectTemplateSmokeArrayProperty -Object $approval -Name "compliance_issues")
            $complianceText = if ($complianceIssues.Count -gt 0) { " compliance_issues=$($complianceIssues -join ',')" } else { "" }
            [void]$lines.Add("- $($approval.name): status=$($approval.status) decision=$($approval.decision) candidate=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $approval.schema_update_candidate) approval=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $approval.approval_result) review=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $approval.review_json) action=$($approval.action)$complianceText")
        }
        [void]$lines.Add("")
    }

    foreach ($entry in @(Get-ProjectTemplateSmokeArrayProperty -Object $Summary -Name "entries")) {
        [void]$lines.Add("## $($entry.name)")
        [void]$lines.Add("")
        [void]$lines.Add("- Status: $($entry.status)")
        [void]$lines.Add("- Input DOCX: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "input_docx"))")
        [void]$lines.Add("- Entry artifact directory: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "artifact_dir"))")

        $checks = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "checks"
        $schemaBaseline = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $checks -Name "schema_baseline"
        if ($null -ne $schemaBaseline -and [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaBaseline -Name "enabled")) {
            $schemaPatchReview = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaBaseline -Name "schema_patch_review"
            $schemaPatchApproval = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaBaseline -Name "schema_patch_approval"
            [void]$lines.Add("- Schema baseline: matches=$($schemaBaseline.matches) schema_lint_clean=$($schemaBaseline.schema_lint_clean) schema_lint_issue_count=$($schemaBaseline.schema_lint_issue_count) log=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaBaseline -Name "log_path"))")
            if ($null -ne $schemaPatchReview) {
                [void]$lines.Add("- Schema patch review: changed=$($schemaPatchReview.changed) baseline_slots=$($schemaPatchReview.baseline_slot_count) generated_slots=$($schemaPatchReview.generated_slot_count) inserted_slots=$($schemaPatchReview.inserted_slots) replaced_slots=$($schemaPatchReview.replaced_slots) review=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaBaseline -Name "schema_patch_review_output_path"))")
            }
            if ($null -ne $schemaPatchApproval) {
                $complianceIssues = @(Get-ProjectTemplateSmokeArrayProperty -Object $schemaPatchApproval -Name "compliance_issues")
                $complianceText = if ($complianceIssues.Count -gt 0) { " compliance_issues=$($complianceIssues -join ',')" } else { "" }
                [void]$lines.Add("- Schema patch approval: status=$($schemaPatchApproval.status) decision=$($schemaPatchApproval.decision) required=$($schemaPatchApproval.required) pending=$($schemaPatchApproval.pending) candidate=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $schemaPatchApproval.schema_update_candidate) approval=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $schemaPatchApproval.approval_result) action=$($schemaPatchApproval.action)$complianceText")
            }
        }

        foreach ($issue in @(Get-ProjectTemplateSmokeArrayProperty -Object $entry -Name "issues")) {
            [void]$lines.Add("- Issue: $issue")
        }

        [void]$lines.Add("")
    }

    $lines | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-ProjectTemplateSchemaApprovalReleaseBlockerItem {
    param($Item)

    $status = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Item -Name "status"
    $action = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Item -Name "action"
    if ([string]::IsNullOrWhiteSpace($action) -and $status -eq "invalid_result") {
        $action = "fix_schema_patch_approval_result"
    }

    return [pscustomobject]@{
        name = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Item -Name "name"
        status = $status
        decision = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Item -Name "decision"
        action = $action
        compliance_issue_count = [int](Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Item -Name "compliance_issue_count")
        compliance_issues = @(Get-ProjectTemplateSmokeArrayProperty -Object $Item -Name "compliance_issues")
        approval_result = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Item -Name "approval_result"
        schema_update_candidate = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Item -Name "schema_update_candidate"
        review_json = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Item -Name "review_json"
    }
}

function Get-ProjectTemplateSchemaApprovalBlockedItems {
    param([object[]]$ApprovalItems)

    return @(
        $ApprovalItems | Where-Object {
            $status = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $_ -Name "status"
            $complianceIssueCountValue = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $_ -Name "compliance_issue_count"
            $complianceIssueCount = if ([string]::IsNullOrWhiteSpace($complianceIssueCountValue)) { 0 } else { [int]$complianceIssueCountValue }
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
            ForEach-Object { @(Get-ProjectTemplateSmokeArrayProperty -Object $_ -Name "compliance_issues") } |
            Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
            Sort-Object -Unique
    )
}

function New-ProjectTemplateSchemaApprovalReleaseBlocker {
    param(
        [object[]]$ApprovalItems,
        [string]$SummaryJsonPath,
        [int]$PendingCount,
        [int]$ApprovedCount,
        [int]$RejectedCount,
        [int]$ComplianceIssueCount,
        [int]$InvalidResultCount,
        [string]$GateStatus
    )

    if ($GateStatus -ne "blocked") {
        return $null
    }

    $blockedItems = @(Get-ProjectTemplateSchemaApprovalBlockedItems -ApprovalItems $ApprovalItems)
    return [pscustomobject]@{
        id = "project_template_smoke.schema_approval"
        source = "project_template_smoke"
        severity = "error"
        status = "blocked"
        message = "Project template smoke schema approval gate blocked."
        gate_status = "blocked"
        pending_count = $PendingCount
        approved_count = $ApprovedCount
        rejected_count = $RejectedCount
        compliance_issue_count = $ComplianceIssueCount
        invalid_result_count = $InvalidResultCount
        blocked_item_count = $blockedItems.Count
        issue_keys = @(Get-ProjectTemplateSchemaApprovalIssueKeys -BlockedItems $blockedItems)
        summary_json = $SummaryJsonPath
        history_json = ""
        history_markdown = ""
        action = "fix_schema_patch_approval_result"
        items = $blockedItems
    }
}

function Set-ProjectTemplateSchemaApprovalReleaseBlocker {
    param(
        $ReleaseSummary,
        $Blocker
    )

    $blockers = New-Object 'System.Collections.Generic.List[object]'
    foreach ($existingBlocker in @(Get-ProjectTemplateSmokeArrayProperty -Object $ReleaseSummary -Name "release_blockers")) {
        $existingId = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $existingBlocker -Name "id"
        if ($existingId -ne "project_template_smoke.schema_approval") {
            [void]$blockers.Add($existingBlocker)
        }
    }
    if ($null -ne $Blocker) {
        [void]$blockers.Add($Blocker)
    }

    Set-ProjectTemplateSmokePropertyValue -Object $ReleaseSummary -Name "release_blockers" -Value $blockers.ToArray()
    Set-ProjectTemplateSmokePropertyValue -Object $ReleaseSummary -Name "release_blocker_count" -Value $blockers.Count
}

function Set-ProjectTemplateSmokeReleaseFields {
    param(
        $Object,
        $Summary,
        [string]$SummaryJsonPath,
        [object[]]$ApprovalItems,
        [int]$PendingCount,
        [int]$ApprovedCount,
        [int]$RejectedCount,
        [int]$ComplianceIssueCount,
        [int]$InvalidResultCount,
        [string]$GateStatus,
        [bool]$GateBlocked
    )

    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "summary_json" -Value $SummaryJsonPath
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "manifest_path" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "manifest_path")
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "output_dir" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "output_dir")
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "passed" -Value (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Summary -Name "passed")
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "entry_count" -Value (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Summary -Name "entry_count")
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "failed_entry_count" -Value (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Summary -Name "failed_entry_count")
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "dirty_schema_baseline_count" -Value (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Summary -Name "dirty_schema_baseline_count")
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_review_count" -Value (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Summary -Name "schema_patch_review_count")
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_review_changed_count" -Value (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Summary -Name "schema_patch_review_changed_count")
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_approval_pending_count" -Value $PendingCount
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_approval_approved_count" -Value $ApprovedCount
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_approval_rejected_count" -Value $RejectedCount
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_approval_compliance_issue_count" -Value $ComplianceIssueCount
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_approval_invalid_result_count" -Value $InvalidResultCount
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_approval_gate_status" -Value $GateStatus
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_approval_gate_blocked" -Value $GateBlocked
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_approval_items" -Value $ApprovalItems
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "schema_patch_approval_synced_at" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "schema_patch_approval_synced_at")
    Set-ProjectTemplateSmokePropertyValue -Object $Object -Name "overall_status" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "overall_status")
}

function Update-ReleaseCandidateFinalReviewSchemaApprovalSection {
    param(
        [string]$Path,
        [string]$RepoRoot,
        $ReleaseSummary,
        [string]$ReleaseSummaryPath
    )

    $projectTemplateSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $ReleaseSummary -Name "project_template_smoke"
    $summaryPath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "summary_json"
    $manifestPath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "manifest_path"
    $outputDir = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "output_dir"
    $reviewCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "schema_patch_review_count"
    $changedReviewCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "schema_patch_review_changed_count"
    $pendingCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "schema_patch_approval_pending_count"
    $approvedCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "schema_patch_approval_approved_count"
    $rejectedCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "schema_patch_approval_rejected_count"
    $complianceIssueCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "schema_patch_approval_compliance_issue_count"
    $invalidResultCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "schema_patch_approval_invalid_result_count"
    $gateStatus = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "schema_patch_approval_gate_status"
    $syncedAt = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "schema_patch_approval_synced_at"

    $sectionLines = New-Object 'System.Collections.Generic.List[string]'
    [void]$sectionLines.Add("<!-- featherdoc:schema-approval:start -->")
    [void]$sectionLines.Add("## Project template smoke schema approval")
    [void]$sectionLines.Add("")
    [void]$sectionLines.Add("- Release summary: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryPath)")
    [void]$sectionLines.Add("- Project template smoke manifest: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $manifestPath)")
    [void]$sectionLines.Add("- Project template smoke summary: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $summaryPath)")
    [void]$sectionLines.Add("- Project template smoke output dir: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $outputDir)")
    [void]$sectionLines.Add("- Schema patch reviews: $reviewCount")
    [void]$sectionLines.Add("- Changed schema patch reviews: $changedReviewCount")
    [void]$sectionLines.Add("- Schema patch approvals pending: $pendingCount")
    [void]$sectionLines.Add("- Schema patch approvals approved: $approvedCount")
    [void]$sectionLines.Add("- Schema patch approvals rejected: $rejectedCount")
    [void]$sectionLines.Add("- Schema patch approval compliance issues: $complianceIssueCount")
    [void]$sectionLines.Add("- Schema patch approval invalid results: $invalidResultCount")
    [void]$sectionLines.Add("- Schema patch approval gate status: $gateStatus")
    [void]$sectionLines.Add("- Schema patch approval synced at: $syncedAt")

    $approvalItems = @(Get-ProjectTemplateSmokeArrayProperty -Object $projectTemplateSmoke -Name "schema_patch_approval_items")
    if ($approvalItems.Count -gt 0) {
        [void]$sectionLines.Add("")
        [void]$sectionLines.Add("### Schema patch approval items")
        [void]$sectionLines.Add("")
        foreach ($approval in $approvalItems) {
            $complianceIssues = @(Get-ProjectTemplateSmokeArrayProperty -Object $approval -Name "compliance_issues")
            $complianceText = if ($complianceIssues.Count -gt 0) { " compliance_issues=$($complianceIssues -join ',')" } else { "" }
            [void]$sectionLines.Add("- $($approval.name): status=$($approval.status) decision=$($approval.decision) action=$($approval.action) approval=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $approval.approval_result)$complianceText")
        }
    }
    [void]$sectionLines.Add("<!-- featherdoc:schema-approval:end -->")

    $section = ($sectionLines.ToArray() -join [System.Environment]::NewLine)
    $existing = if (Test-Path -LiteralPath $Path) {
        Get-Content -Raw -Encoding UTF8 -LiteralPath $Path
    } else {
        "# Release Candidate Checks`n"
    }

    $pattern = '(?s)<!-- featherdoc:schema-approval:start -->.*?<!-- featherdoc:schema-approval:end -->'
    $updated = if ([regex]::IsMatch($existing, $pattern)) {
        [regex]::Replace($existing, $pattern, [System.Text.RegularExpressions.MatchEvaluator]{ param($match) $section }, 1)
    } else {
        $existing.TrimEnd() + [System.Environment]::NewLine + [System.Environment]::NewLine + $section + [System.Environment]::NewLine
    }

    Ensure-PathParent -Path $Path
    $updated | Set-Content -LiteralPath $Path -Encoding UTF8
}

$repoRoot = Resolve-RepoRoot
$resolvedSummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedSummaryMarkdown = if ([string]::IsNullOrWhiteSpace($SummaryMarkdown)) {
    Join-Path (Split-Path -Parent $resolvedSummaryJson) "summary.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SummaryMarkdown -AllowMissing
}
$resolvedReleaseSummaryPath = if ([string]::IsNullOrWhiteSpace($ReleaseCandidateSummaryJson)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $ReleaseCandidateSummaryJson
}

if (-not (Test-Path -LiteralPath $resolvedSummaryJson)) {
    throw "Project template smoke summary was not found: $resolvedSummaryJson"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath) -and -not (Test-Path -LiteralPath $resolvedReleaseSummaryPath)) {
    throw "Release-candidate summary JSON was not found: $resolvedReleaseSummaryPath"
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedSummaryJson | ConvertFrom-Json
$entries = @(Get-ProjectTemplateSmokeArrayProperty -Object $summary -Name "entries")

if (-not [string]::IsNullOrWhiteSpace($Decision)) {
    if ([string]::IsNullOrWhiteSpace($EntryName)) {
        throw "-EntryName is required when -Decision is provided."
    }

    $targetEntries = @($entries | Where-Object { [string]$_.name -eq $EntryName })
    if ($targetEntries.Count -ne 1) {
        throw "Expected exactly one entry named '$EntryName', found $($targetEntries.Count)."
    }

    $targetChecks = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $targetEntries[0] -Name "checks"
    $targetSchemaBaseline = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $targetChecks -Name "schema_baseline"
    if ($null -eq $targetSchemaBaseline -or -not [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $targetSchemaBaseline -Name "enabled")) {
        throw "Entry '$EntryName' does not have an enabled schema_baseline check."
    }

    $targetApprovalResultPath = Get-SchemaPatchApprovalResultPath -Entry $targetEntries[0] -SchemaBaseline $targetSchemaBaseline
    if ([string]::IsNullOrWhiteSpace($targetApprovalResultPath)) {
        throw "Entry '$EntryName' does not expose a schema patch approval result path."
    }

    [void](Write-SchemaPatchApprovalResult `
        -Path $targetApprovalResultPath `
        -EntryName $EntryName `
        -SchemaBaseline $targetSchemaBaseline `
        -DecisionValue $Decision `
        -ReviewerValue $Reviewer `
        -ReviewedAtValue $ReviewedAt `
        -NoteValue $Note)
    Write-Step "Recorded '$Decision' decision for '$EntryName'"
}

$schemaPatchApprovalItems = New-Object 'System.Collections.Generic.List[object]'
foreach ($entry in $entries) {
    $name = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "name"
    $checks = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "checks"
    $schemaBaseline = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $checks -Name "schema_baseline"
    if ($null -eq $schemaBaseline -or -not [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaBaseline -Name "enabled")) {
        continue
    }

    $reviewSummary = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaBaseline -Name "schema_patch_review"
    if ($null -eq $reviewSummary) {
        continue
    }

    $changed = [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $reviewSummary -Name "changed")
    $approvalResultPath = Get-SchemaPatchApprovalResultPath -Entry $entry -SchemaBaseline $schemaBaseline
    $approvalResult = $null
    if ($changed) {
        if ([string]::IsNullOrWhiteSpace($approvalResultPath)) {
            throw "Entry '$name' needs schema approval but has no approval result path."
        }
        if (-not (Test-Path -LiteralPath $approvalResultPath)) {
            $approvalResult = Write-SchemaPatchApprovalResult `
                -Path $approvalResultPath `
                -EntryName $name `
                -SchemaBaseline $schemaBaseline `
                -DecisionValue "pending" `
                -ReviewerValue "" `
                -ReviewedAtValue "" `
                -NoteValue ""
        } else {
            $approvalResult = Read-JsonFileIfPresent -Path $approvalResultPath
        }
        Set-ProjectTemplateSmokePropertyValue -Object $schemaBaseline -Name "schema_patch_approval_result_path" -Value $approvalResultPath
        Set-ProjectTemplateSmokePropertyValue -Object $schemaBaseline -Name "schema_patch_approval_result" -Value $approvalResult
    }

    $approval = New-SchemaPatchApprovalSummary `
        -Name $name `
        -SchemaBaseline $schemaBaseline `
        -ApprovalResult $approvalResult `
        -ApprovalResultPath $approvalResultPath
    if ($null -ne $approval) {
        Set-ProjectTemplateSmokePropertyValue -Object $schemaBaseline -Name "schema_patch_approval" -Value $approval
        if ([bool]$approval.required) {
            [void]$schemaPatchApprovalItems.Add($approval)
        }
    }
}

$pendingCount = @($schemaPatchApprovalItems | Where-Object { [bool]$_.pending }).Count
$approvedCount = @($schemaPatchApprovalItems | Where-Object { [bool]$_.approved }).Count
$rejectedCount = @($schemaPatchApprovalItems | Where-Object { $_.status -in @("rejected", "needs_changes", "invalid_result") }).Count
$invalidResultCount = @($schemaPatchApprovalItems.ToArray() | Where-Object { $_.status -eq "invalid_result" }).Count
$complianceIssueCount = 0
foreach ($item in $schemaPatchApprovalItems.ToArray()) {
    $itemComplianceIssueCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "compliance_issue_count"
    if (-not [string]::IsNullOrWhiteSpace($itemComplianceIssueCount)) {
        $complianceIssueCount += [int]$itemComplianceIssueCount
    }
}

Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "schema_patch_approval_synced_at" -Value (Get-Date).ToString("s")
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "schema_patch_approval_pending_count" -Value $pendingCount
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "schema_patch_approval_approved_count" -Value $approvedCount
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "schema_patch_approval_rejected_count" -Value $rejectedCount
$gateStatus = Get-SchemaPatchApprovalGateStatus `
    -PendingCount $pendingCount `
    -ApprovalItemCount $schemaPatchApprovalItems.Count `
    -InvalidResultCount $invalidResultCount `
    -ComplianceIssueCount $complianceIssueCount
$gateBlocked = $gateStatus -eq "blocked"
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "schema_patch_approval_compliance_issue_count" -Value $complianceIssueCount
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "schema_patch_approval_invalid_result_count" -Value $invalidResultCount
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "schema_patch_approval_gate_status" -Value $gateStatus
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "schema_patch_approval_gate_blocked" -Value $gateBlocked
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "schema_patch_approval_items" -Value $schemaPatchApprovalItems.ToArray()

($summary | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
Write-SummaryMarkdown -Path $resolvedSummaryMarkdown -RepoRoot $repoRoot -Summary $summary

if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    $releaseSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedReleaseSummaryPath | ConvertFrom-Json
    $releaseSteps = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $releaseSummary -Name "steps"
    if ($null -eq $releaseSteps) {
        $releaseSteps = [ordered]@{}
        Set-ProjectTemplateSmokePropertyValue -Object $releaseSummary -Name "steps" -Value $releaseSteps
    }
    $projectTemplateSmokeStep = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $releaseSteps -Name "project_template_smoke"
    if ($null -eq $projectTemplateSmokeStep) {
        $projectTemplateSmokeStep = [ordered]@{}
        Set-ProjectTemplateSmokePropertyValue -Object $releaseSteps -Name "project_template_smoke" -Value $projectTemplateSmokeStep
    }
    $projectTemplateSmokeSummary = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $releaseSummary -Name "project_template_smoke"
    if ($null -eq $projectTemplateSmokeSummary) {
        $projectTemplateSmokeSummary = [ordered]@{}
        Set-ProjectTemplateSmokePropertyValue -Object $releaseSummary -Name "project_template_smoke" -Value $projectTemplateSmokeSummary
    }

    $statusValue = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmokeStep -Name "status"
    if ([string]::IsNullOrWhiteSpace($statusValue)) {
        $statusValue = "completed"
    }
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "status" -Value $statusValue
    if ($gateBlocked) {
        Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "status" -Value "failed"
        Set-ProjectTemplateSmokePropertyValue -Object $releaseSummary -Name "execution_status" -Value "fail"
        Set-ProjectTemplateSmokePropertyValue -Object $releaseSummary -Name "failed_step" -Value "project_template_smoke"
        Set-ProjectTemplateSmokePropertyValue -Object $releaseSummary -Name "error" -Value "Project template smoke schema approval gate blocked."
    }
    $schemaApprovalReleaseBlocker = New-ProjectTemplateSchemaApprovalReleaseBlocker `
        -ApprovalItems $schemaPatchApprovalItems.ToArray() `
        -SummaryJsonPath $resolvedSummaryJson `
        -PendingCount $pendingCount `
        -ApprovedCount $approvedCount `
        -RejectedCount $rejectedCount `
        -ComplianceIssueCount $complianceIssueCount `
        -InvalidResultCount $invalidResultCount `
        -GateStatus $gateStatus
    Set-ProjectTemplateSchemaApprovalReleaseBlocker `
        -ReleaseSummary $releaseSummary `
        -Blocker $schemaApprovalReleaseBlocker
    Set-ProjectTemplateSmokeReleaseFields `
        -Object $projectTemplateSmokeStep `
        -Summary $summary `
        -SummaryJsonPath $resolvedSummaryJson `
        -ApprovalItems $schemaPatchApprovalItems.ToArray() `
        -PendingCount $pendingCount `
        -ApprovedCount $approvedCount `
        -RejectedCount $rejectedCount `
        -ComplianceIssueCount $complianceIssueCount `
        -InvalidResultCount $invalidResultCount `
        -GateStatus $gateStatus `
        -GateBlocked $gateBlocked
    Set-ProjectTemplateSmokeReleaseFields `
        -Object $projectTemplateSmokeSummary `
        -Summary $summary `
        -SummaryJsonPath $resolvedSummaryJson `
        -ApprovalItems $schemaPatchApprovalItems.ToArray() `
        -PendingCount $pendingCount `
        -ApprovedCount $approvedCount `
        -RejectedCount $rejectedCount `
        -ComplianceIssueCount $complianceIssueCount `
        -InvalidResultCount $invalidResultCount `
        -GateStatus $gateStatus `
        -GateBlocked $gateBlocked

    ($releaseSummary | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedReleaseSummaryPath -Encoding UTF8
    $releaseFinalReviewPath = Join-Path (Split-Path -Parent $resolvedReleaseSummaryPath) "final_review.md"
    Update-ReleaseCandidateFinalReviewSchemaApprovalSection `
        -Path $releaseFinalReviewPath `
        -RepoRoot $repoRoot `
        -ReleaseSummary $releaseSummary `
        -ReleaseSummaryPath $resolvedReleaseSummaryPath
    Write-Step "Updated release-candidate summary and final review"

    if ($RefreshReleaseBundle) {
        $releaseNoteBundleScript = Join-Path $repoRoot "scripts\write_release_note_bundle.ps1"
        Invoke-ChildPowerShell -ScriptPath $releaseNoteBundleScript `
            -Arguments @(
                "-SummaryJson"
                $resolvedReleaseSummaryPath
            ) `
            -FailureMessage "Failed to refresh the release note bundle."
        Write-Step "Refreshed release note bundle"
    }
}

Write-Step "Summary JSON: $resolvedSummaryJson"
Write-Step "Summary Markdown: $resolvedSummaryMarkdown"
Write-Step "Schema approvals pending: $pendingCount"
Write-Step "Schema approvals approved: $approvedCount"
Write-Step "Schema approvals rejected: $rejectedCount"
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    Write-Host "Release summary: $resolvedReleaseSummaryPath"
    Write-Host "Release final review: $(Join-Path (Split-Path -Parent $resolvedReleaseSummaryPath) 'final_review.md')"
}

exit 0
