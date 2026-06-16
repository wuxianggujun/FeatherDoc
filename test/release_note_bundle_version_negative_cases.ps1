function Copy-ReleaseSummaryForNegativeCase {
    return ($summary | ConvertTo-Json -Depth 10 | ConvertFrom-Json)
}

function Set-ProjectTemplateReadinessWarningOnlyFixture {
    param([AllowNull()]$Report)

    if ($null -eq $Report) {
        return
    }

    $Report.status = "needs_review"
    $Report.release_ready = $false
    $Report.source_failure_count = 0
    $Report.latest_schema_approval_gate_status = "passed"
    $Report.schema_approval_status_summary = "approved=4"
    $Report.onboarding_governance_next_action = [pscustomobject]@{
        action = "review_project_template_delivery_readiness_evidence"
        status = "needs_review"
        blocker_id = "project_template_delivery_readiness.warning_only"
        reason = "Review warning-only project-template readiness evidence."
    }
    $Report.onboarding_governance_next_action_summary = @(
        [pscustomobject]@{
            action = "review_project_template_delivery_readiness_evidence"
            status = "needs_review"
            blocker_id = "project_template_delivery_readiness.warning_only"
            reason = "Review warning-only project-template readiness evidence."
        }
    )
    $Report.onboarding_governance_next_action_group_count = 1
    $Report.schema_history_blocked_run_count = 0
    $Report.schema_history_pending_run_count = 0
    $Report.schema_history_passed_run_count = 4
    $Report.template_count = 3
    $Report.ready_template_count = 3
    $Report.blocked_template_count = 0
    $Report.release_blocker_count = 0
    $Report.action_item_count = 0
    $Report.warning_count = 1

    if ($null -ne $Report.PSObject.Properties["error"]) {
        $Report.error = ""
    }
}

$warningOnlyReadinessSummary = Copy-ReleaseSummaryForNegativeCase
$warningOnlyRollupReadinessReport = $warningOnlyReadinessSummary.release_blocker_rollup.source_reports |
    Where-Object { [string]$_.schema -eq "featherdoc.project_template_delivery_readiness_report.v1" } |
    Select-Object -First 1
$warningOnlyHandoffReadinessReport = $warningOnlyReadinessSummary.release_governance_handoff.reports |
    Where-Object { [string]$_.id -eq "project_template_delivery_readiness" } |
    Select-Object -First 1
if ($null -eq $warningOnlyRollupReadinessReport) {
    throw "Missing project template delivery readiness source report fixture."
}
if ($null -eq $warningOnlyHandoffReadinessReport) {
    throw "Missing project template delivery readiness handoff report fixture."
}
Set-ProjectTemplateReadinessWarningOnlyFixture -Report $warningOnlyRollupReadinessReport
Set-ProjectTemplateReadinessWarningOnlyFixture -Report $warningOnlyHandoffReadinessReport
$warningOnlyReadinessSummary.release_governance_handoff.failed_report_count = 0

$warningOnlyReadinessSummaryPath = Join-Path $reportDir "summary.project-template-warning-only-readiness.json"
($warningOnlyReadinessSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $warningOnlyReadinessSummaryPath -Encoding UTF8
& $bundleScript -SummaryJson $warningOnlyReadinessSummaryPath -SkipMaterialSafetyAudit

Assert-LineContainsAll -Path $bodyPath -ExpectedFragments @(
    'Project template readiness:',
    'project_template_delivery_readiness_contract',
    'status=needs_review',
    'release_ready=False',
    'latest_schema_approval_gate_status=passed',
    'schema_approval_status_summary=approved=4',
    'onboarding_governance_next_action_group_count=1',
    'onboarding_governance_next_action=action=review_project_template_delivery_readiness_evidence',
    'schema_history_blocked_run_count=0',
    'schema_history_pending_run_count=0',
    'schema_history_passed_run_count=4',
    'template_count=3',
    'ready_template_count=3',
    'blocked_template_count=0',
    'release_blocker_count=0',
    'action_item_count=0',
    'warning_count=1',
    'source_failure_count=0',
    'source_report_display=.\output\project-template-delivery-readiness\summary.json',
    'source_json_display=.\output\project-template-delivery-readiness\summary.json'
) -Label 'release_body.zh-CN.md warning-only project-template readiness'
Assert-LineContainsAll -Path $shortPath -ExpectedFragments @(
    'project-template readiness governance contract',
    'status=needs_review',
    'release_ready=False',
    'latest_schema_approval_gate_status=passed',
    'schema_approval_status_summary=approved=4',
    'onboarding_governance_next_action=action=review_project_template_delivery_readiness_evidence',
    'onboarding_governance_next_action_group_count=1',
    'release_blocker_count=0',
    'warning_count=1',
    'source_report_display=.\output\project-template-delivery-readiness\summary.json',
    'source_json_display=.\output\project-template-delivery-readiness\summary.json'
) -Label 'release_summary.zh-CN.md warning-only project-template readiness'

function Assert-BundleRejectsSummary {
    param(
        [object]$CandidateSummary,
        [string]$FileName,
        [string[]]$ExpectedFragments
    )

    $candidateSummaryPath = Join-Path $reportDir $FileName
    ($CandidateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $candidateSummaryPath -Encoding UTF8

    $failed = $false
    $message = ""
    try {
        & $bundleScript -SummaryJson $candidateSummaryPath -SkipMaterialSafetyAudit
    } catch {
        $failed = $true
        $message = $_.Exception.Message
    }
    if (-not $failed) {
        throw "write_release_note_bundle.ps1 unexpectedly accepted invalid release blocker metadata: $FileName"
    }

    foreach ($expectedFragment in $ExpectedFragments) {
        if ($message -notmatch [regex]::Escape($expectedFragment)) {
            throw "write_release_note_bundle.ps1 failed with unexpected release blocker validation message: $message"
        }
    }
}

$mismatchedSummary = Copy-ReleaseSummaryForNegativeCase
$mismatchedSummary.release_blocker_count = 2
Assert-BundleRejectsSummary `
    -CandidateSummary $mismatchedSummary `
    -FileName "summary.release-blocker-count-mismatch.json" `
    -ExpectedFragments @(
        "release_blocker_count mismatch",
        "declared 2 but release_blockers contains 1 item(s)"
    )

$missingFieldSummary = Copy-ReleaseSummaryForNegativeCase
$missingFieldBlocker = @($missingFieldSummary.release_blockers)[0]
$missingFieldBlocker.action = ""
Assert-BundleRejectsSummary `
    -CandidateSummary $missingFieldSummary `
    -FileName "summary.release-blocker-missing-action.json" `
    -ExpectedFragments @("release_blockers[0].action must not be empty")

$missingRollupEvidenceSummary = Copy-ReleaseSummaryForNegativeCase
$missingRollupEvidenceBlocker = @($missingRollupEvidenceSummary.release_blocker_rollup.release_blockers)[0]
$missingRollupEvidenceBlocker.source_json_display = ""
Assert-BundleRejectsSummary `
    -CandidateSummary $missingRollupEvidenceSummary `
    -FileName "summary.release-rollup-missing-source-json-display.json" `
    -ExpectedFragments @("release_blocker_rollup.release_blockers[0].source_json_display must not be empty")

$mismatchedRollupSourceSchemaSummary = Copy-ReleaseSummaryForNegativeCase
$mismatchedRollup = $mismatchedRollupSourceSchemaSummary.release_blocker_rollup
$blockerSourceSchemaSummary = @(
    @($mismatchedRollup.release_blockers) |
        Group-Object -Property source_schema |
        ForEach-Object {
            [pscustomobject]@{
                source_schema = [string]$_.Name
                count = [int]$_.Count
            }
        }
)
$mismatchedRollup | Add-Member `
    -NotePropertyName "blocker_source_schema_summary" `
    -NotePropertyValue $blockerSourceSchemaSummary `
    -Force
$mismatchedSourceSchemaGroup = @($mismatchedRollup.blocker_source_schema_summary) |
    Where-Object { [string]$_.source_schema -eq "featherdoc.document_skeleton_governance_rollup_report.v1" } |
    Select-Object -First 1
if ($null -eq $mismatchedSourceSchemaGroup) {
    throw "Missing release blocker rollup source schema summary fixture."
}
$mismatchedSourceSchemaGroup.count = 2
Assert-BundleRejectsSummary `
    -CandidateSummary $mismatchedRollupSourceSchemaSummary `
    -FileName "summary.release-rollup-blocker-source-schema-summary-count-mismatch.json" `
    -ExpectedFragments @(
        "release_blocker_rollup.blocker_source_schema_summary count mismatch for 'featherdoc.document_skeleton_governance_rollup_report.v1'",
        "declared 2 but release_blocker_rollup.release_blockers contains 1 item(s)"
    )

$missingHandoffCommandSummary = Copy-ReleaseSummaryForNegativeCase
$missingHandoffActionItem = @($missingHandoffCommandSummary.release_governance_handoff.action_items)[0]
$missingHandoffActionItem.open_command = ""
Assert-BundleRejectsSummary `
    -CandidateSummary $missingHandoffCommandSummary `
    -FileName "summary.release-handoff-missing-open-command.json" `
    -ExpectedFragments @("release_governance_handoff.action_items[0].open_command must not be empty")

$duplicateIdSummary = Copy-ReleaseSummaryForNegativeCase
$firstBlocker = @($duplicateIdSummary.release_blockers)[0]
$duplicateBlocker = $firstBlocker | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$duplicateIdSummary.release_blockers = @($firstBlocker, $duplicateBlocker)
$duplicateIdSummary.release_blocker_count = 2
Assert-BundleRejectsSummary `
    -CandidateSummary $duplicateIdSummary `
    -FileName "summary.release-blocker-duplicate-id.json" `
    -ExpectedFragments @("duplicate release blocker id 'project_template_smoke.schema_approval'")

$unknownActionSummaryPath = Join-Path $reportDir "summary.release-blocker-unregistered-action.json"
$unknownActionSummary = Copy-ReleaseSummaryForNegativeCase
$unknownActionBlocker = @($unknownActionSummary.release_blockers)[0]
$unknownActionBlocker.id = "custom_gate.unregistered_action"
$unknownActionBlocker.source = "custom_gate"
$unknownActionBlocker.action = "investigate_custom_gate"
($unknownActionSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $unknownActionSummaryPath -Encoding UTF8
& $bundleScript -SummaryJson $unknownActionSummaryPath -SkipMaterialSafetyAudit
$checklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath -ExpectedText 'Unregistered release blocker action `investigate_custom_gate`: add a fixed checklist runbook in `release_blocker_metadata_helpers.ps1`' -Label 'REVIEWER_CHECKLIST.md'

$bodyContent = Get-Utf8FileText -Path $bodyPath
if ($bodyContent -match 'v1\.6\.1') {
    throw "release_body.zh-CN.md unexpectedly referenced the current development version: $bodyPath"
}

Write-Host "Release note bundle version pinning passed."
