param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Invoke-ScriptAndCapture {
    param([string]$ScriptPath, [string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    $output = @(& $powerShellPath -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

function New-ApprovalResultJson {
    param(
        [string]$Path,
        [string]$EntryName,
        [string]$Decision,
        [string]$Reviewer = "Seed Reviewer",
        [string]$ReviewedAt = "seeded-review-time"
    )

    $approvalResult = [ordered]@{
        schema = "featherdoc.template_schema_patch_approval_result.v1"
        entry_name = $EntryName
        decision = $Decision
        reviewer = $Reviewer
        reviewed_at = $ReviewedAt
        note = "seeded approval result"
        schema_file = $schemaPath
        schema_update_candidate = $generatedOutput
        review_json = $reviewJsonPath
    }
    ($approvalResult | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-SyntheticEntry {
    param([string]$Name, [string]$ApprovalResultPath)

    return [ordered]@{
        name = $Name
        input_docx = $inputDocx
        artifact_dir = $resolvedWorkingDir
        status = "failed"
        passed = $false
        manual_review_pending = $false
        checks = [ordered]@{
            template_validations = @()
            schema_validation = [ordered]@{ enabled = $false }
            schema_baseline = [ordered]@{
                enabled = $true
                schema_file = $schemaPath
                generated_output = $generatedOutput
                repaired_output = ""
                schema_patch_review_output_path = $reviewJsonPath
                schema_patch_approval_result_path = $ApprovalResultPath
                target_mode = "default"
                exit_code = 1
                matches = $false
                schema_lint_clean = $true
                schema_lint_issue_count = 0
                log_path = $logPath
                schema_patch_review = $reviewSummary
            }
            visual_smoke = [ordered]@{ enabled = $false }
        }
        issues = @("Schema baseline drift detected.")
    }
}

function New-SyntheticReviewRollup {
    param([string]$Name)

    return [ordered]@{
        name = $Name
        review_json = $reviewJsonPath
        changed = $true
        baseline_slot_count = 1
        generated_slot_count = 5
        upsert_slot_count = 4
        remove_target_count = 0
        remove_slot_count = 0
        rename_slot_count = 0
        update_slot_count = 0
        inserted_slots = 4
        replaced_slots = 0
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$syncScriptPath = Join-Path $resolvedRepoRoot "scripts\sync_project_template_schema_approval.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$summaryPath = Join-Path $resolvedWorkingDir "summary.json"
$summaryMarkdownPath = Join-Path $resolvedWorkingDir "summary.md"
$releaseSummaryPath = Join-Path $resolvedWorkingDir "release-summary.json"
$releaseFinalReviewPath = Join-Path $resolvedWorkingDir "final_review.md"
$schemaPath = Join-Path $resolvedWorkingDir "drift.template-schema.json"
$generatedOutput = Join-Path $resolvedWorkingDir "generated.template-schema.json"
$approvalResultPath = Join-Path $resolvedWorkingDir "schema_patch_approval_result.json"
$rejectedApprovalResultPath = Join-Path $resolvedWorkingDir "schema_patch_approval_rejected_result.json"
$needsChangesApprovalResultPath = Join-Path $resolvedWorkingDir "schema_patch_approval_needs_changes_result.json"
$invalidApprovalResultPath = Join-Path $resolvedWorkingDir "schema_patch_approval_invalid_result.json"
$reviewJsonPath = Join-Path $resolvedWorkingDir "schema_patch_review.json"
$logPath = Join-Path $resolvedWorkingDir "schema_baseline.log"
$inputDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"

Set-Content -LiteralPath $schemaPath -Encoding UTF8 -Value `
    '{"targets":[{"part":"body","slots":[{"bookmark":"customer_name","kind":"text"}]}]}'
Set-Content -LiteralPath $generatedOutput -Encoding UTF8 -Value `
    '{"targets":[{"part":"body","slots":[{"bookmark":"customer_name","kind":"text"},{"bookmark":"invoice_no","kind":"text"}]}]}'
Set-Content -LiteralPath $reviewJsonPath -Encoding UTF8 -Value `
    '{"schema":"featherdoc.template_schema_patch_review.v1","changed":true}'
Set-Content -LiteralPath $logPath -Encoding UTF8 -Value "synthetic baseline log"

$reviewSummary = [ordered]@{
    schema = "featherdoc.template_schema_patch_review.v1"
    baseline_slot_count = 1
    generated_slot_count = 5
    changed = $true
    upsert_slot_count = 4
    remove_target_count = 0
    remove_slot_count = 0
    rename_slot_count = 0
    update_slot_count = 0
    removed_targets = 0
    removed_slots = 0
    renamed_slots = 0
    inserted_slots = 4
    replaced_slots = 0
}

New-ApprovalResultJson -Path $rejectedApprovalResultPath -EntryName "schema-review-rejected" -Decision "rejected"
New-ApprovalResultJson -Path $needsChangesApprovalResultPath -EntryName "schema-review-needs-changes" -Decision "needs_changes"
New-ApprovalResultJson `
    -Path $invalidApprovalResultPath `
    -EntryName "schema-review-invalid" `
    -Decision "approved" `
    -Reviewer "" `
    -ReviewedAt ""

$entryNames = @("schema-review-drift", "schema-review-rejected", "schema-review-needs-changes", "schema-review-invalid")
$summary = [ordered]@{
    generated_at = "2026-04-29T09:00:00"
    manifest_path = Join-Path $resolvedWorkingDir "project_template_smoke.manifest.json"
    workspace = $resolvedRepoRoot
    build_dir = if ([string]::IsNullOrWhiteSpace($BuildDir)) { "" } else { [System.IO.Path]::GetFullPath($BuildDir) }
    output_dir = $resolvedWorkingDir
    entry_count = 4
    failed_entry_count = 4
    dirty_schema_baseline_count = 0
    schema_patch_review_count = 4
    schema_patch_review_changed_count = 4
    schema_patch_reviews = @($entryNames | ForEach-Object { New-SyntheticReviewRollup -Name $_ })
    schema_patch_approval_pending_count = 4
    schema_patch_approval_approved_count = 0
    schema_patch_approval_rejected_count = 0
    schema_patch_approval_compliance_issue_count = 0
    schema_patch_approval_items = @()
    visual_entry_count = 0
    visual_verdict = "not_applicable"
    manual_review_pending_count = 0
    visual_review_undetermined_count = 0
    passed = $false
    overall_status = "failed"
    entries = @(
        New-SyntheticEntry -Name "schema-review-drift" -ApprovalResultPath $approvalResultPath
        New-SyntheticEntry -Name "schema-review-rejected" -ApprovalResultPath $rejectedApprovalResultPath
        New-SyntheticEntry -Name "schema-review-needs-changes" -ApprovalResultPath $needsChangesApprovalResultPath
        New-SyntheticEntry -Name "schema-review-invalid" -ApprovalResultPath $invalidApprovalResultPath
    )
}
($summary | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
$releaseSummary = [ordered]@{
    generated_at = "2026-04-29T09:30:00"
    execution_status = "completed"
    visual_verdict = "not_applicable"
    steps = [ordered]@{
        project_template_smoke = [ordered]@{
            status = "completed"
            summary_json = "stale-summary.json"
        }
    }
    project_template_smoke = [ordered]@{
        summary_json = "stale-summary.json"
        schema_patch_approval_pending_count = 99
    }
    release_blockers = @(
        [ordered]@{
            id = "visual.manual_review"
            source = "visual_gate"
            severity = "warning"
            status = "blocked"
            action = "complete_visual_manual_review"
        }
    )
    release_blocker_count = 1
}
($releaseSummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8
Set-Content -LiteralPath $releaseFinalReviewPath -Encoding UTF8 -Value @"
# Release Candidate Checks

Existing release review content.
"@

$result = Invoke-ScriptAndCapture -ScriptPath $syncScriptPath -Arguments @(
    "-SummaryJson", $summaryPath,
    "-ReleaseCandidateSummaryJson", $releaseSummaryPath,
    "-EntryName", "schema-review-drift",
    "-Decision", "approved",
    "-Reviewer", "CI Reviewer",
    "-ReviewedAt", "2026-04-29T10:00:00",
    "-Note", "schema approval sync regression"
)
Assert-Equal -Actual $result.ExitCode -Expected 0 `
    -Message "Schema approval sync should succeed. Output: $($result.Text)"

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$summary.schema_patch_approval_pending_count) -Expected 1 `
    -Message "Pending approval count should include invalid approval records."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_approved_count) -Expected 1 `
    -Message "Approved approval count should refresh."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_rejected_count) -Expected 3 `
    -Message "Rejected approval count should include rejected, needs_changes, and invalid results."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_compliance_issue_count) -Expected 2 `
    -Message "Compliance issue count should include missing reviewer and reviewed_at."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_invalid_result_count) -Expected 1 `
    -Message "Invalid approval-result count should include the invalid approved record."
Assert-Equal -Actual ([string]$summary.schema_patch_approval_gate_status) -Expected "blocked" `
    -Message "Schema approval gate status should block invalid approval records."
Assert-Equal -Actual ([bool]$summary.schema_patch_approval_gate_blocked) -Expected $true `
    -Message "Schema approval gate should expose the blocked flag."

$approvedItem = $summary.schema_patch_approval_items | Where-Object { $_.name -eq "schema-review-drift" } | Select-Object -First 1
$rejectedItem = $summary.schema_patch_approval_items | Where-Object { $_.name -eq "schema-review-rejected" } | Select-Object -First 1
$needsChangesItem = $summary.schema_patch_approval_items | Where-Object { $_.name -eq "schema-review-needs-changes" } | Select-Object -First 1
$invalidItem = $summary.schema_patch_approval_items | Where-Object { $_.name -eq "schema-review-invalid" } | Select-Object -First 1
Assert-Equal -Actual ([string]$approvedItem.decision) -Expected "approved" `
    -Message "Approved approval item should expose the command-line decision."
Assert-Equal -Actual ([string]$approvedItem.status) -Expected "approved" `
    -Message "Approved approval item should expose approved status."
Assert-Equal -Actual ([string]$approvedItem.reviewer) -Expected "CI Reviewer" `
    -Message "Approved approval item should expose the reviewer."
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace([string]$approvedItem.reviewed_at)) `
    -Message "Approved approval item should expose reviewed_at."
Assert-Equal -Actual ([string]$rejectedItem.decision) -Expected "rejected" `
    -Message "Rejected approval item should expose rejected decision."
Assert-Equal -Actual ([string]$rejectedItem.status) -Expected "rejected" `
    -Message "Rejected approval item should expose rejected status."
Assert-Equal -Actual ([string]$needsChangesItem.decision) -Expected "needs_changes" `
    -Message "Needs-changes approval item should expose needs_changes decision."
Assert-Equal -Actual ([string]$needsChangesItem.status) -Expected "needs_changes" `
    -Message "Needs-changes approval item should expose needs_changes status."
Assert-Equal -Actual ([string]$invalidItem.decision) -Expected "approved" `
    -Message "Invalid approval item should preserve the normalized decision."
Assert-Equal -Actual ([string]$invalidItem.status) -Expected "invalid_result" `
    -Message "Invalid approval item should require approval-result repair."
Assert-Equal -Actual ([string]$invalidItem.action) -Expected "fix_schema_patch_approval_result" `
    -Message "Invalid approval item should expose the repair action."
Assert-Equal -Actual ([int]$invalidItem.compliance_issue_count) -Expected 2 `
    -Message "Invalid approval item should expose compliance issue count."
$invalidIssues = @($invalidItem.compliance_issues)
Assert-True -Condition ($invalidIssues -contains "missing_reviewer") `
    -Message "Invalid approval item should flag a missing reviewer."
Assert-True -Condition ($invalidIssues -contains "missing_reviewed_at") `
    -Message "Invalid approval item should flag a missing reviewed_at timestamp."

$schemaBaseline = $summary.entries[0].checks.schema_baseline
Assert-Equal -Actual ([string]$schemaBaseline.schema_patch_approval.decision) -Expected "approved" `
    -Message "Entry approval should expose the refreshed decision."
Assert-Equal -Actual ([string]$schemaBaseline.schema_patch_approval_result.decision) -Expected "approved" `
    -Message "Entry approval result should be reloaded into the summary."

$approvalResult = Get-Content -Raw -Encoding UTF8 -LiteralPath $schemaBaseline.schema_patch_approval_result_path | ConvertFrom-Json
Assert-Equal -Actual ([string]$approvalResult.schema) -Expected "featherdoc.template_schema_patch_approval_result.v1" `
    -Message "Persisted approval result should keep the stable schema id."
Assert-Equal -Actual ([string]$approvalResult.decision) -Expected "approved" `
    -Message "Persisted approval result should record the decision."
Assert-Equal -Actual ([string]$approvalResult.reviewer) -Expected "CI Reviewer" `
    -Message "Persisted approval result should record the reviewer."

$summaryMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryMarkdownPath
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approval synced at:" `
    -Message "Markdown summary should expose schema approval sync timestamp."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approvals approved: 1" `
    -Message "Markdown summary should expose approved count."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approvals rejected: 3" `
    -Message "Markdown summary should expose rejected count."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approval compliance issues: 2" `
    -Message "Markdown summary should expose compliance issue count."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approval invalid results: 1" `
    -Message "Markdown summary should expose invalid approval-result count."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approval gate status: blocked" `
    -Message "Markdown summary should expose blocked approval gate status."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approval: status=approved" `
    -Message "Markdown summary should expose approved entry status."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "status=needs_changes" `
    -Message "Markdown summary should expose needs_changes status."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "status=invalid_result" `
    -Message "Markdown summary should expose invalid approval-result status."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "compliance_issues=missing_reviewer,missing_reviewed_at" `
    -Message "Markdown summary should expose approval-result compliance issues."

$releaseSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$releaseSummary.project_template_smoke.schema_patch_approval_pending_count) -Expected 1 `
    -Message "Release summary should refresh pending schema approval count."
Assert-Equal -Actual ([int]$releaseSummary.project_template_smoke.schema_patch_approval_approved_count) -Expected 1 `
    -Message "Release summary should refresh approved schema approval count."
Assert-Equal -Actual ([int]$releaseSummary.project_template_smoke.schema_patch_approval_rejected_count) -Expected 3 `
    -Message "Release summary should refresh rejected schema approval count."
Assert-Equal -Actual ([int]$releaseSummary.project_template_smoke.schema_patch_approval_compliance_issue_count) -Expected 2 `
    -Message "Release summary should refresh schema approval compliance issue count."
Assert-Equal -Actual ([int]$releaseSummary.project_template_smoke.schema_patch_approval_invalid_result_count) -Expected 1 `
    -Message "Release summary should refresh invalid schema approval-result count."
Assert-Equal -Actual ([string]$releaseSummary.project_template_smoke.schema_patch_approval_gate_status) -Expected "blocked" `
    -Message "Release summary should expose blocked schema approval gate status."
Assert-Equal -Actual ([bool]$releaseSummary.project_template_smoke.schema_patch_approval_gate_blocked) -Expected $true `
    -Message "Release summary should expose blocked schema approval gate flag."
Assert-Equal -Actual ([int]$releaseSummary.steps.project_template_smoke.schema_patch_approval_approved_count) -Expected 1 `
    -Message "Release step should refresh approved schema approval count."
Assert-Equal -Actual ([int]$releaseSummary.steps.project_template_smoke.schema_patch_approval_compliance_issue_count) -Expected 2 `
    -Message "Release step should refresh schema approval compliance issue count."
Assert-Equal -Actual ([string]$releaseSummary.steps.project_template_smoke.schema_patch_approval_gate_status) -Expected "blocked" `
    -Message "Release step should expose blocked schema approval gate status."
Assert-Equal -Actual ([string]$releaseSummary.steps.project_template_smoke.status) -Expected "failed" `
    -Message "Release step should be marked failed when schema approval gate is blocked."
Assert-Equal -Actual ([string]$releaseSummary.execution_status) -Expected "fail" `
    -Message "Release summary execution status should fail when schema approval gate is blocked."
Assert-Equal -Actual ([string]$releaseSummary.failed_step) -Expected "project_template_smoke" `
    -Message "Release summary failed step should point to project_template_smoke."
Assert-Equal -Actual ([string]$releaseSummary.project_template_smoke.summary_json) -Expected $summaryPath `
    -Message "Release project template smoke block should point at the synced summary."
Assert-Equal -Actual ([int]$releaseSummary.release_blocker_count) -Expected 2 `
    -Message "Release summary should count schema approval blockers without dropping existing blockers."
$schemaApprovalBlocker = @($releaseSummary.release_blockers | Where-Object { $_.id -eq "project_template_smoke.schema_approval" }) | Select-Object -First 1
Assert-True -Condition ($null -ne $schemaApprovalBlocker) `
    -Message "Release summary should expose a schema approval release blocker."
Assert-Equal -Actual ([string]$schemaApprovalBlocker.status) -Expected "blocked" `
    -Message "Schema approval release blocker should expose blocked status."
Assert-Equal -Actual ([int]$schemaApprovalBlocker.compliance_issue_count) -Expected 2 `
    -Message "Schema approval release blocker should carry compliance issue count."
Assert-Equal -Actual ([int]$schemaApprovalBlocker.invalid_result_count) -Expected 1 `
    -Message "Schema approval release blocker should carry invalid approval-result count."
Assert-Equal -Actual ([int]$schemaApprovalBlocker.blocked_item_count) -Expected 1 `
    -Message "Schema approval release blocker should count blocked approval items."
Assert-True -Condition (@($schemaApprovalBlocker.issue_keys) -contains "missing_reviewer") `
    -Message "Schema approval release blocker should include compliance issue keys."
Assert-True -Condition (@($schemaApprovalBlocker.items | ForEach-Object { $_.name }) -contains "schema-review-invalid") `
    -Message "Schema approval release blocker should include blocked approval item names."
Assert-True -Condition (@($releaseSummary.release_blockers | ForEach-Object { $_.id }) -contains "visual.manual_review") `
    -Message "Release summary should preserve unrelated release blockers."

$releaseFinalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseFinalReviewPath
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "Existing release review content." `
    -Message "Release final review should preserve existing content."
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "Project template smoke schema approval" `
    -Message "Release final review should include schema approval section."
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "Schema patch approvals approved: 1" `
    -Message "Release final review should include approved schema approval count."
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "Schema patch approval compliance issues: 2" `
    -Message "Release final review should include schema approval compliance issue count."
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "Schema patch approval invalid results: 1" `
    -Message "Release final review should include invalid schema approval-result count."
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "Schema patch approval gate status: blocked" `
    -Message "Release final review should include blocked schema approval gate status."
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "schema-review-needs-changes: status=needs_changes" `
    -Message "Release final review should include needs_changes approval item."
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "schema-review-invalid: status=invalid_result" `
    -Message "Release final review should include invalid approval item."

Write-Host "Project template schema approval sync regression passed."
