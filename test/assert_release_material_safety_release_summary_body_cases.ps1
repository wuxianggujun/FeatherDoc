$badContentControlStrategyPath = Join-Path $failDir "content_control_missing_repair_strategy.json"
$badContentControlStrategy = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "blocker"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
}
($badContentControlStrategy | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badContentControlStrategyPath -Encoding UTF8

$missingContentControlRepairStrategyFailedAsExpected = $false
try {
    & $auditScript -Path $badContentControlStrategyPath
} catch {
    $missingContentControlRepairStrategyFailedAsExpected = $true
}

if (-not $missingContentControlRepairStrategyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed content-control blocker without repair_strategy."
}

$badContentControlHintPath = Join-Path $failDir "content_control_bad_repair_hint.json"
$badContentControlHint = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "blocker"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Review the content control manually before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
}
($badContentControlHint | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badContentControlHintPath -Encoding UTF8

$badContentControlHintFailedAsExpected = $false
try {
    & $auditScript -Path $badContentControlHintPath
} catch {
    $badContentControlHintFailedAsExpected = $true
}

if (-not $badContentControlHintFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed content-control blocker without Custom XML sync repair_hint."
}

$badContentControlCommandPath = Join-Path $failDir "content_control_bad_command_template.json"
$badContentControlCommand = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "blocker"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli inspect-content-controls <input.docx> --json"
        }
    )
}
($badContentControlCommand | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badContentControlCommandPath -Encoding UTF8

$badContentControlCommandFailedAsExpected = $false
try {
    & $auditScript -Path $badContentControlCommandPath
} catch {
    $badContentControlCommandFailedAsExpected = $true
}

if (-not $badContentControlCommandFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed content-control blocker without sync command_template."
}

$badReleaseSummaryTracePath = Join-Path $failDir "release_summary.zh-CN.md"
Set-Content -LiteralPath $badReleaseSummaryTracePath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json

## Detached notes

- source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseSummaryTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryTracePath
} catch {
    $badReleaseSummaryTraceFailedAsExpected = $true
}

if (-not $badReleaseSummaryTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with project-template source_json_display outside the readiness summary line."
}

$badReleaseSummaryOnboardingTraceDir = Join-Path $failDir "release-summary-onboarding-missing-project-template-source-json"
$badReleaseSummaryOnboardingTracePath = Join-Path $badReleaseSummaryOnboardingTraceDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryOnboardingTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryOnboardingTracePath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseSummaryOnboardingTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryOnboardingTracePath
} catch {
    $badReleaseSummaryOnboardingTraceFailedAsExpected = $true
}

if (-not $badReleaseSummaryOnboardingTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with onboarding source_json_display supplied only by readiness."
}

$badReleaseSummaryReadinessSourceIdentityDir = Join-Path $failDir "release-summary-readiness-source-identity-impersonated"
$badReleaseSummaryReadinessSourceIdentityPath = Join-Path $badReleaseSummaryReadinessSourceIdentityDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryReadinessSourceIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryReadinessSourceIdentityPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseSummaryReadinessSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryReadinessSourceIdentityPath
} catch {
    $badReleaseSummaryReadinessSourceIdentityFailedAsExpected = $true
}

if (-not $badReleaseSummaryReadinessSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with readiness source displays pointing at onboarding governance evidence."
}

$badReleaseSummaryInvalidReadinessStatusDir = Join-Path $failDir "release-summary-invalid-project-template-readiness-status"
$badReleaseSummaryInvalidReadinessStatusPath = Join-Path $badReleaseSummaryInvalidReadinessStatusDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryInvalidReadinessStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryInvalidReadinessStatusPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready-ish release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseSummaryInvalidReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryInvalidReadinessStatusPath
} catch {
    $badReleaseSummaryInvalidReadinessStatusFailedAsExpected = $true
}

if (-not $badReleaseSummaryInvalidReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with an invalid project-template readiness status."
}

$badReleaseSummaryInvalidReadinessReadyDir = Join-Path $failDir "release-summary-invalid-project-template-readiness-release-ready"
$badReleaseSummaryInvalidReadinessReadyPath = Join-Path $badReleaseSummaryInvalidReadinessReadyDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryInvalidReadinessReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryInvalidReadinessReadyPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=maybe latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseSummaryInvalidReadinessReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryInvalidReadinessReadyPath
} catch {
    $badReleaseSummaryInvalidReadinessReadyFailedAsExpected = $true
}

if (-not $badReleaseSummaryInvalidReadinessReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with an invalid project-template readiness release_ready."
}

$badReleaseSummaryInvalidOnboardingStatusDir = Join-Path $failDir "release-summary-invalid-project-template-onboarding-status"
$badReleaseSummaryInvalidOnboardingStatusPath = Join-Path $badReleaseSummaryInvalidOnboardingStatusDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryInvalidOnboardingStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryInvalidOnboardingStatusPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready-ish release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseSummaryInvalidOnboardingStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryInvalidOnboardingStatusPath
} catch {
    $badReleaseSummaryInvalidOnboardingStatusFailedAsExpected = $true
}

if (-not $badReleaseSummaryInvalidOnboardingStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with an invalid project-template onboarding status."
}

$badReleaseSummaryInvalidOnboardingReadyDir = Join-Path $failDir "release-summary-invalid-project-template-onboarding-release-ready"
$badReleaseSummaryInvalidOnboardingReadyPath = Join-Path $badReleaseSummaryInvalidOnboardingReadyDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryInvalidOnboardingReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryInvalidOnboardingReadyPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready release_ready=maybe schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseSummaryInvalidOnboardingReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryInvalidOnboardingReadyPath
} catch {
    $badReleaseSummaryInvalidOnboardingReadyFailedAsExpected = $true
}

if (-not $badReleaseSummaryInvalidOnboardingReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with an invalid project-template onboarding release_ready."
}

$badReleaseSummaryOnboardingSourceJsonIdentityDir = Join-Path $failDir "release-summary-onboarding-source-json-identity-impersonated"
$badReleaseSummaryOnboardingSourceJsonIdentityPath = Join-Path $badReleaseSummaryOnboardingSourceJsonIdentityDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryOnboardingSourceJsonIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryOnboardingSourceJsonIdentityPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseSummaryOnboardingSourceJsonIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryOnboardingSourceJsonIdentityPath
} catch {
    $badReleaseSummaryOnboardingSourceJsonIdentityFailedAsExpected = $true
}

if (-not $badReleaseSummaryOnboardingSourceJsonIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with onboarding source_json_display pointing at delivery readiness evidence."
}

$badReleaseSummaryPdfSplitTraceDir = Join-Path $failDir "release-summary-split-pdf-visual-contact-sheet"
$badReleaseSummaryPdfSplitTracePath = Join-Path $badReleaseSummaryPdfSplitTraceDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryPdfSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryPdfSplitTracePath -Encoding UTF8 -Value @"
# Release summary

- PDF visual gate: verdict=pass summary=.\output\pdf-visual-release-gate-current\report\summary.json cjk_manifest_count=43 cjk_copy_search_count=43 visual_baseline_manifest_count=42 visual_baseline_count=44

## Detached notes

- aggregate_contact_sheet=.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badReleaseSummaryPdfSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryPdfSplitTracePath
} catch {
    $badReleaseSummaryPdfSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseSummaryPdfSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with a PDF visual contact-sheet path outside the PDF visual gate summary line."
}

$badReleaseSummaryPdfVerdictTraceDir = Join-Path $failDir "release-summary-invalid-pdf-visual-verdict"
$badReleaseSummaryPdfVerdictTracePath = Join-Path $badReleaseSummaryPdfVerdictTraceDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryPdfVerdictTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryPdfVerdictTracePath -Encoding UTF8 -Value @"
# Release summary

- PDF visual gate: verdict=blocked summary=.\output\pdf-visual-release-gate-current\report\summary.json aggregate_contact_sheet=.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png cjk_manifest_count=43 cjk_copy_search_count=43 visual_baseline_manifest_count=42 visual_baseline_count=44
"@

$badReleaseSummaryPdfVerdictTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryPdfVerdictTracePath
} catch {
    $badReleaseSummaryPdfVerdictTraceFailedAsExpected = $true
}

if (-not $badReleaseSummaryPdfVerdictTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with a non-pass/fail PDF visual gate verdict."
}

$badStartHerePdfDetachedCountTraceDir = Join-Path $failDir "start-here-pdf-visual-count-supplied-by-detached-notes"
$badStartHerePdfDetachedCountTracePath = Join-Path $badStartHerePdfDetachedCountTraceDir "START_HERE.md"
New-Item -ItemType Directory -Path $badStartHerePdfDetachedCountTraceDir -Force | Out-Null
Set-Content -LiteralPath $badStartHerePdfDetachedCountTracePath -Encoding UTF8 -Value @"
# START_HERE

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png

## Detached notes

- PDF visual baseline manifest samples: 42
"@

$badStartHerePdfDetachedCountTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badStartHerePdfDetachedCountTracePath
} catch {
    $badStartHerePdfDetachedCountTraceFailedAsExpected = $true
}

if (-not $badStartHerePdfDetachedCountTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with PDF visual counts supplied only by detached notes."
}

$badArtifactGuidePdfDetachedCountTraceDir = Join-Path $failDir "artifact-guide-pdf-visual-count-supplied-by-detached-notes"
$badArtifactGuidePdfDetachedCountTracePath = Join-Path $badArtifactGuidePdfDetachedCountTraceDir "ARTIFACT_GUIDE.md"
New-Item -ItemType Directory -Path $badArtifactGuidePdfDetachedCountTraceDir -Force | Out-Null
Set-Content -LiteralPath $badArtifactGuidePdfDetachedCountTracePath -Encoding UTF8 -Value @"
# Artifact Guide

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44

## Detached notes

- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badArtifactGuidePdfDetachedCountTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badArtifactGuidePdfDetachedCountTracePath
} catch {
    $badArtifactGuidePdfDetachedCountTraceFailedAsExpected = $true
}

if (-not $badArtifactGuidePdfDetachedCountTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed ARTIFACT_GUIDE.md with PDF visual contact sheet supplied only by detached notes."
}

$badReviewerChecklistPdfDetachedContactTraceDir = Join-Path $failDir "reviewer-checklist-pdf-visual-contact-sheet-supplied-by-detached-notes"
$badReviewerChecklistPdfDetachedContactTracePath = Join-Path $badReviewerChecklistPdfDetachedContactTraceDir "REVIEWER_CHECKLIST.md"
New-Item -ItemType Directory -Path $badReviewerChecklistPdfDetachedContactTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReviewerChecklistPdfDetachedContactTracePath -Encoding UTF8 -Value @"
# Reviewer Checklist

- Confirm PDF visual gate summary .\output\pdf-visual-release-gate-current\report\summary.json with 43 CJK copy/search samples and 44 visual baselines before release.
- Confirm PDF visual gate aggregate contact sheet before release.

## Detached notes

- aggregate_contact_sheet=.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badReviewerChecklistPdfDetachedContactTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReviewerChecklistPdfDetachedContactTracePath
} catch {
    $badReviewerChecklistPdfDetachedContactTraceFailedAsExpected = $true
}

if (-not $badReviewerChecklistPdfDetachedContactTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed REVIEWER_CHECKLIST.md with PDF visual contact sheet supplied only by detached notes."
}

$badReviewerChecklistPdfFinalizeTraceDir = Join-Path $failDir "reviewer-checklist-pdf-visual-finalize-line-missing-contact-sheet"
$badReviewerChecklistPdfFinalizeTracePath = Join-Path $badReviewerChecklistPdfFinalizeTraceDir "REVIEWER_CHECKLIST.md"
New-Item -ItemType Directory -Path $badReviewerChecklistPdfFinalizeTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReviewerChecklistPdfFinalizeTracePath -Encoding UTF8 -Value @"
# Reviewer Checklist

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- [ ] Confirm the PDF visual gate finalize evidence is signed off: verdict ``pass``, summary .\output\pdf-visual-release-gate-current\report\summary.json, aggregate contact sheet, CJK manifest samples ``43``, CJK copy/search samples ``43``, missing text ``0``, visual baseline manifest samples ``42``, visual baselines ``44``.
"@

$badReviewerChecklistPdfFinalizeTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReviewerChecklistPdfFinalizeTracePath
} catch {
    $badReviewerChecklistPdfFinalizeTraceFailedAsExpected = $true
}

if (-not $badReviewerChecklistPdfFinalizeTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed REVIEWER_CHECKLIST.md with PDF visual finalize evidence missing the contact-sheet path on the finalize line."
}

$badReleaseBodyTraceDir = Join-Path $failDir "release-body-missing-project-template-source-json"
$badReleaseBodyTracePath = Join-Path $badReleaseBodyTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyTracePath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json

## Detached notes

- source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseBodyTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyTracePath
} catch {
    $badReleaseBodyTraceFailedAsExpected = $true
}

if (-not $badReleaseBodyTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with project-template source_json_display outside the readiness summary line."
}

$badReleaseBodyOnboardingTraceDir = Join-Path $failDir "release-body-onboarding-missing-project-template-source-json"
$badReleaseBodyOnboardingTracePath = Join-Path $badReleaseBodyOnboardingTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyOnboardingTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyOnboardingTracePath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseBodyOnboardingTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyOnboardingTracePath
} catch {
    $badReleaseBodyOnboardingTraceFailedAsExpected = $true
}

if (-not $badReleaseBodyOnboardingTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with onboarding source_json_display supplied only by readiness."
}

$badReleaseBodyReadinessSourceIdentityDir = Join-Path $failDir "release-body-readiness-source-identity-impersonated"
$badReleaseBodyReadinessSourceIdentityPath = Join-Path $badReleaseBodyReadinessSourceIdentityDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyReadinessSourceIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyReadinessSourceIdentityPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseBodyReadinessSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyReadinessSourceIdentityPath
} catch {
    $badReleaseBodyReadinessSourceIdentityFailedAsExpected = $true
}

if (-not $badReleaseBodyReadinessSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with readiness source displays pointing at onboarding governance evidence."
}

$badReleaseBodyInvalidReadinessStatusDir = Join-Path $failDir "release-body-invalid-project-template-readiness-status"
$badReleaseBodyInvalidReadinessStatusPath = Join-Path $badReleaseBodyInvalidReadinessStatusDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyInvalidReadinessStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyInvalidReadinessStatusPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready-ish release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseBodyInvalidReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyInvalidReadinessStatusPath
} catch {
    $badReleaseBodyInvalidReadinessStatusFailedAsExpected = $true
}

if (-not $badReleaseBodyInvalidReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with an invalid project-template readiness status."
}

$badReleaseBodyInvalidReadinessReadyDir = Join-Path $failDir "release-body-invalid-project-template-readiness-release-ready"
$badReleaseBodyInvalidReadinessReadyPath = Join-Path $badReleaseBodyInvalidReadinessReadyDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyInvalidReadinessReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyInvalidReadinessReadyPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=maybe latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseBodyInvalidReadinessReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyInvalidReadinessReadyPath
} catch {
    $badReleaseBodyInvalidReadinessReadyFailedAsExpected = $true
}

if (-not $badReleaseBodyInvalidReadinessReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with an invalid project-template readiness release_ready."
}

$badReleaseBodyInvalidOnboardingStatusDir = Join-Path $failDir "release-body-invalid-project-template-onboarding-status"
$badReleaseBodyInvalidOnboardingStatusPath = Join-Path $badReleaseBodyInvalidOnboardingStatusDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyInvalidOnboardingStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyInvalidOnboardingStatusPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready-ish release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseBodyInvalidOnboardingStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyInvalidOnboardingStatusPath
} catch {
    $badReleaseBodyInvalidOnboardingStatusFailedAsExpected = $true
}

if (-not $badReleaseBodyInvalidOnboardingStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with an invalid project-template onboarding status."
}

$badReleaseBodyInvalidOnboardingReadyDir = Join-Path $failDir "release-body-invalid-project-template-onboarding-release-ready"
$badReleaseBodyInvalidOnboardingReadyPath = Join-Path $badReleaseBodyInvalidOnboardingReadyDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyInvalidOnboardingReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyInvalidOnboardingReadyPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=maybe schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseBodyInvalidOnboardingReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyInvalidOnboardingReadyPath
} catch {
    $badReleaseBodyInvalidOnboardingReadyFailedAsExpected = $true
}

if (-not $badReleaseBodyInvalidOnboardingReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with an invalid project-template onboarding release_ready."
}

$badReleaseBodyOnboardingSourceIdentityDir = Join-Path $failDir "release-body-onboarding-source-identity-impersonated"
$badReleaseBodyOnboardingSourceIdentityPath = Join-Path $badReleaseBodyOnboardingSourceIdentityDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyOnboardingSourceIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyOnboardingSourceIdentityPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseBodyOnboardingSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyOnboardingSourceIdentityPath
} catch {
    $badReleaseBodyOnboardingSourceIdentityFailedAsExpected = $true
}

if (-not $badReleaseBodyOnboardingSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with onboarding source_json_display pointing at delivery readiness evidence."
}

$badReleaseBodyPdfSplitTraceDir = Join-Path $failDir "release-body-split-pdf-visual-contact-sheet"
$badReleaseBodyPdfSplitTracePath = Join-Path $badReleaseBodyPdfSplitTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyPdfSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyPdfSplitTracePath -Encoding UTF8 -Value @"
# Release body

## Validation

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: loaded
- PDF visual gate verdict: pass
- PDF visual gate aggregate contact sheet:
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44

## Detached notes

- aggregate_contact_sheet=.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badReleaseBodyPdfSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyPdfSplitTracePath
} catch {
    $badReleaseBodyPdfSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseBodyPdfSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with a PDF visual contact-sheet path outside the PDF visual gate aggregate contact sheet line."
}

$badReleaseBodyPdfDetachedCountTraceDir = Join-Path $failDir "release-body-pdf-visual-count-supplied-by-detached-notes"
$badReleaseBodyPdfDetachedCountTracePath = Join-Path $badReleaseBodyPdfDetachedCountTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyPdfDetachedCountTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyPdfDetachedCountTracePath -Encoding UTF8 -Value @"
# Release body

## Validation

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: loaded
- PDF visual gate verdict: pass
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baselines: 44

## Detached notes

- PDF visual baseline manifest samples: 42
"@

$badReleaseBodyPdfDetachedCountTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyPdfDetachedCountTracePath
} catch {
    $badReleaseBodyPdfDetachedCountTraceFailedAsExpected = $true
}

if (-not $badReleaseBodyPdfDetachedCountTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with PDF visual counts supplied only by detached notes."
}

$badReleaseBodyPdfVerdictTraceDir = Join-Path $failDir "release-body-invalid-pdf-visual-verdict"
$badReleaseBodyPdfVerdictTracePath = Join-Path $badReleaseBodyPdfVerdictTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyPdfVerdictTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyPdfVerdictTracePath -Encoding UTF8 -Value @"
# Release body

## Validation

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: loaded
- PDF visual gate verdict: blocked
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
"@

$badReleaseBodyPdfVerdictTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyPdfVerdictTracePath
} catch {
    $badReleaseBodyPdfVerdictTraceFailedAsExpected = $true
}

if (-not $badReleaseBodyPdfVerdictTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with a non-pass/fail PDF visual gate verdict."
}
