function New-SchemaPatchReviewSummary {
    param($ReviewJson)

    if ($null -eq $ReviewJson) {
        return $null
    }

    return [pscustomobject]@{
        schema = [string]$ReviewJson.schema
        baseline_slot_count = [int]$ReviewJson.baseline_slot_count
        generated_slot_count = [int]$ReviewJson.generated_slot_count
        changed = [bool]$ReviewJson.changed
        upsert_slot_count = [int]$ReviewJson.patch.upsert_slot_count
        remove_target_count = [int]$ReviewJson.patch.remove_target_count
        remove_slot_count = [int]$ReviewJson.patch.remove_slot_count
        rename_slot_count = [int]$ReviewJson.patch.rename_slot_count
        update_slot_count = [int]$ReviewJson.patch.update_slot_count
        removed_targets = [int]$ReviewJson.preview.removed_targets
        removed_slots = [int]$ReviewJson.preview.removed_slots
        renamed_slots = [int]$ReviewJson.preview.renamed_slots
        inserted_slots = [int]$ReviewJson.preview.inserted_slots
        replaced_slots = [int]$ReviewJson.preview.replaced_slots
    }
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
        $reviewer = Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "reviewer"
        if ([string]::IsNullOrWhiteSpace($reviewer)) {
            [void]$issues.Add("missing_reviewer")
        }

        $reviewedAt = Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "reviewed_at"
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
        [int]$ApprovedCount = 0,
        [int]$RejectedCount = 0,
        [int]$InvalidResultCount,
        [int]$ComplianceIssueCount
    )

    if ($ComplianceIssueCount -gt 0 -or $InvalidResultCount -gt 0 -or $RejectedCount -gt 0) {
        return "blocked"
    }
    if ($PendingCount -gt 0) {
        return "pending"
    }
    if ($ApprovedCount -gt 0) {
        return "passed"
    }
    if ($ApprovalItemCount -gt 0) {
        return "not_required"
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
        schema_file = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "schema_file"
        schema_update_candidate = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "generated_output"
        review_json = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "schema_patch_review_output_path"
    }
}

function New-SchemaPatchApprovalSummary {
    param(
        [string]$Name,
        $SchemaBaseline,
        $ApprovalResult,
        [string]$ApprovalResultPath
    )

    $reviewSummary = Get-OptionalObjectPropertyObject -Object $SchemaBaseline -Name "schema_patch_review"
    if ($null -eq $reviewSummary) {
        return $null
    }

    $changed = [bool]$reviewSummary.changed
    $decision = if ($changed) {
        Normalize-SchemaPatchApprovalDecision -Value (Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "decision")
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
        schema_file = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "schema_file"
        schema_update_candidate = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "generated_output"
        review_json = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "schema_patch_review_output_path"
        approval_result = $ApprovalResultPath
        repaired_schema_candidate = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "repaired_schema_output_path"
        reviewer = Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "reviewer"
        reviewed_at = Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "reviewed_at"
        note = Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "note"
        baseline_slot_count = [int]$reviewSummary.baseline_slot_count
        generated_slot_count = [int]$reviewSummary.generated_slot_count
        upsert_slot_count = [int]$reviewSummary.upsert_slot_count
        remove_target_count = [int]$reviewSummary.remove_target_count
        remove_slot_count = [int]$reviewSummary.remove_slot_count
        rename_slot_count = [int]$reviewSummary.rename_slot_count
        update_slot_count = [int]$reviewSummary.update_slot_count
        inserted_slots = [int]$reviewSummary.inserted_slots
        replaced_slots = [int]$reviewSummary.replaced_slots
    }
}
