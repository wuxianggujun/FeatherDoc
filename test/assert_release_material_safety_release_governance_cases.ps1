$passReleaseGovernanceHandoffManifestSignoffTraceDir = Join-Path $passDir "release-governance-handoff-manifest-signoff-trace"
$passReleaseGovernanceHandoffManifestSignoffTracePath = Join-Path $passReleaseGovernanceHandoffManifestSignoffTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffManifestSignoffTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffManifestSignoffTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Manifest signoff entrypoints evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - manifest_signoff_entrypoints_status: ``declared``
    - manifest_signoff_entrypoints_release_assets_manifest_display: ``.\output\release-assets\v<version>\release_assets_manifest.json``
    - manifest_signoff_entrypoints_required_entrypoint_count: ``3``
    - manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
    - manifest_signoff_entrypoints_required_contracts: ``project_template_delivery_readiness_contract, project_template_onboarding_governance_contract``
    - manifest_signoff_entrypoints_required_fields: ``status, release_ready, release_blocker_count, warning_count, schema_approval_status_summary, onboarding_governance_next_action, onboarding_governance_next_action_summary, onboarding_governance_next_action_group_count, next_action, next_action_summary, next_action_group_count, source_report_display, source_json_display``
    - manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``
"@

& $auditScript -Path $passReleaseGovernanceHandoffManifestSignoffTracePath

$badReleaseGovernanceHandoffManifestSignoffSplitDir = Join-Path $failDir "release-governance-handoff-manifest-signoff-split"
$badReleaseGovernanceHandoffManifestSignoffSplitPath = Join-Path $badReleaseGovernanceHandoffManifestSignoffSplitDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffManifestSignoffSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffManifestSignoffSplitPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Manifest signoff entrypoints evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - manifest_signoff_entrypoints_status: ``declared``
    - manifest_signoff_entrypoints_release_assets_manifest_display: ``.\output\release-assets\v<version>\release_assets_manifest.json``
    - manifest_signoff_entrypoints_required_entrypoint_count: ``3``

## Detached manifest signoff notes

- manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
- manifest_signoff_entrypoints_required_contracts: ``project_template_delivery_readiness_contract, project_template_onboarding_governance_contract``
- manifest_signoff_entrypoints_required_fields: ``status, release_ready, release_blocker_count, warning_count, schema_approval_status_summary, onboarding_governance_next_action, onboarding_governance_next_action_summary, onboarding_governance_next_action_group_count, next_action, next_action_summary, next_action_group_count, source_report_display, source_json_display``
- manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``
"@

$badReleaseGovernanceHandoffManifestSignoffSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffManifestSignoffSplitPath
} catch {
    $badReleaseGovernanceHandoffManifestSignoffSplitFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffManifestSignoffSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with manifest signoff entrypoints split outside source_report block."
}

$badReleaseGovernanceHandoffManifestSignoffWrongSchemaDir = Join-Path $failDir "release-governance-handoff-manifest-signoff-wrong-schema"
$badReleaseGovernanceHandoffManifestSignoffWrongSchemaPath = Join-Path $badReleaseGovernanceHandoffManifestSignoffWrongSchemaDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffManifestSignoffWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffManifestSignoffWrongSchemaPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Manifest signoff entrypoints evidence source reports: ``1``
  - source_report: ``.\output\release-blocker-rollup\summary.json`` schema=``featherdoc.release_blocker_rollup_report.v1``
    - manifest_signoff_entrypoints_status: ``declared``
    - manifest_signoff_entrypoints_release_assets_manifest_display: ``.\output\release-assets\v<version>\release_assets_manifest.json``
    - manifest_signoff_entrypoints_required_entrypoint_count: ``3``
    - manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
    - manifest_signoff_entrypoints_required_contracts: ``project_template_delivery_readiness_contract, project_template_onboarding_governance_contract``
    - manifest_signoff_entrypoints_required_fields: ``status, release_ready, release_blocker_count, warning_count, schema_approval_status_summary, onboarding_governance_next_action, onboarding_governance_next_action_summary, onboarding_governance_next_action_group_count, next_action, next_action_summary, next_action_group_count, source_report_display, source_json_display``
    - manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``
"@

$badReleaseGovernanceHandoffManifestSignoffWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffManifestSignoffWrongSchemaPath
} catch {
    $badReleaseGovernanceHandoffManifestSignoffWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffManifestSignoffWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with manifest signoff entrypoints source_report using a non-release-candidate schema."
}

$passReleaseGovernanceHandoffProjectTemplateChecklistTraceDir = Join-Path $passDir "release-governance-handoff-project-template-checklist-source-report-trace"
$passReleaseGovernanceHandoffProjectTemplateChecklistTracePath = Join-Path $passReleaseGovernanceHandoffProjectTemplateChecklistTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffProjectTemplateChecklistTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffProjectTemplateChecklistTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Project-template readiness checklist entrypoints evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - project_template_readiness_checklist_entrypoints_status: ``declared``
    - project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``
    - project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
    - project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
"@

& $auditScript -Path $passReleaseGovernanceHandoffProjectTemplateChecklistTracePath

$badReleaseGovernanceHandoffProjectTemplateChecklistSplitDir = Join-Path $failDir "release-governance-handoff-project-template-checklist-split"
$badReleaseGovernanceHandoffProjectTemplateChecklistSplitPath = Join-Path $badReleaseGovernanceHandoffProjectTemplateChecklistSplitDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffProjectTemplateChecklistSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffProjectTemplateChecklistSplitPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Project-template readiness checklist entrypoints evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - project_template_readiness_checklist_entrypoints_status: ``declared``
    - project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``
    - project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``

## Detached checklist notes

- project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
- project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
"@

$badReleaseGovernanceHandoffProjectTemplateChecklistSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffProjectTemplateChecklistSplitPath
} catch {
    $badReleaseGovernanceHandoffProjectTemplateChecklistSplitFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffProjectTemplateChecklistSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with project-template checklist entrypoints split outside source_report block."
}

$badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaDir = Join-Path $failDir "release-governance-handoff-project-template-checklist-wrong-schema"
$badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaPath = Join-Path $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Project-template readiness checklist entrypoints evidence source reports: ``1``
  - source_report: ``.\output\release-blocker-rollup\summary.json`` schema=``featherdoc.release_blocker_rollup_report.v1``
    - project_template_readiness_checklist_entrypoints_status: ``declared``
    - project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``
    - project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
    - project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
"@

$badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaPath
} catch {
    $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with project-template checklist entrypoints source_report using a non-release-candidate schema."
}

$passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTraceDir = Join-Path $passDir "release-governance-handoff-project-template-checklist-material-safety-audit-trace"
$passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTracePath = Join-Path $passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Release-entry project-template readiness checklist material-safety audit source reports: ``1``
  - source_report: ``.\output\release-blocker-rollup\summary.json`` schema=``featherdoc.release_candidate_summary``
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``
"@

& $auditScript -Path $passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTracePath

$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitDir = Join-Path $failDir "release-governance-handoff-project-template-checklist-material-safety-audit-split"
$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitPath = Join-Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Release-entry project-template readiness checklist material-safety audit source reports: ``1``
  - source_report: ``.\output\release-blocker-rollup\summary.json`` schema=``featherdoc.release_candidate_summary``
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``

## Detached release-entry checklist material-safety audit notes

- release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``
- release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``
- release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``
- release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``
- release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``
"@

$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitPath
} catch {
    $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with release-entry project-template checklist material-safety audit evidence split outside source_report block."
}

$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir = Join-Path $failDir "release-governance-handoff-project-template-checklist-material-safety-audit-wrong-schema"
$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath = Join-Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Release-entry project-template readiness checklist material-safety audit source reports: ``1``
  - source_report: ``.\output\release-blocker-rollup\summary.json`` schema=``featherdoc.release_blocker_rollup_report.v1``
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``
"@

$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath
} catch {
    $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with release-entry project-template checklist material-safety audit source_report using a non-release-candidate schema."
}

$passFinalReviewTraceDir = Join-Path $passDir "final-review-project-template-trace"
$passFinalReviewTracePath = Join-Path $passFinalReviewTraceDir "final_review.md"
New-Item -ItemType Directory -Path $passFinalReviewTraceDir -Force | Out-Null
Set-Content -LiteralPath $passFinalReviewTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

- Project-template readiness checklist entrypoints evidence source reports: 1
  - source_report: .\output\release-candidate-checks\summary.json schema=featherdoc.release_candidate_summary
    - project_template_readiness_checklist_entrypoints_status: declared
    - project_template_readiness_checklist_entrypoints_checklist_label: Project template release readiness checklist
    - project_template_readiness_checklist_entrypoints_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: 3
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: start_here, artifact_guide, reviewer_checklist
    - project_template_readiness_checklist_entrypoints_checklist_marker: release_entry_project_template_readiness_checklist_trace
- Release-entry project-template readiness checklist material-safety audit source reports: 1
  - source_report: .\output\release-candidate-checks\summary.json schema=featherdoc.release_candidate_summary
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: passed
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: .\scripts\assert_release_material_safety.ps1
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: 3
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: start_here, artifact_guide, reviewer_checklist
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: Project-template readiness checklist handoff evidence
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: project_template_readiness_checklist_entrypoints_source_reports
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: featherdoc.release_candidate_summary
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: release_entry_project_template_readiness_checklist_trace
    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - readiness_status: ready
  - readiness_release_ready: True
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

& $auditScript -Path $passFinalReviewTracePath

$badFinalReviewProjectTemplateChecklistWrongSchemaDir = Join-Path $failDir "final-review-project-template-checklist-wrong-schema"
$badFinalReviewProjectTemplateChecklistWrongSchemaPath = Join-Path $badFinalReviewProjectTemplateChecklistWrongSchemaDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewProjectTemplateChecklistWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewProjectTemplateChecklistWrongSchemaPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

- Project-template readiness checklist entrypoints evidence source reports: 1
  - source_report: .\output\release-blocker-rollup\summary.json schema=featherdoc.release_blocker_rollup_report.v1
    - project_template_readiness_checklist_entrypoints_status: declared
    - project_template_readiness_checklist_entrypoints_checklist_label: Project template release readiness checklist
    - project_template_readiness_checklist_entrypoints_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: 3
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: start_here, artifact_guide, reviewer_checklist
    - project_template_readiness_checklist_entrypoints_checklist_marker: release_entry_project_template_readiness_checklist_trace
"@

$badFinalReviewProjectTemplateChecklistWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewProjectTemplateChecklistWrongSchemaPath
} catch {
    $badFinalReviewProjectTemplateChecklistWrongSchemaFailedAsExpected = $true
}

if (-not $badFinalReviewProjectTemplateChecklistWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final review with project-template checklist entrypoints source_report using a non-release-candidate schema."
}

$badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir = Join-Path $failDir "final-review-project-template-checklist-material-safety-audit-wrong-schema"
$badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath = Join-Path $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

- Release-entry project-template readiness checklist material-safety audit source reports: 1
  - source_report: .\output\release-blocker-rollup\summary.json schema=featherdoc.release_blocker_rollup_report.v1
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: passed
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: .\scripts\assert_release_material_safety.ps1
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: 3
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: start_here, artifact_guide, reviewer_checklist
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: Project-template readiness checklist handoff evidence
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: project_template_readiness_checklist_entrypoints_source_reports
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: featherdoc.release_candidate_summary
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: release_entry_project_template_readiness_checklist_trace
    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace
"@

$badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath
} catch {
    $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $true
}

if (-not $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final review with release-entry project-template checklist material-safety audit source_report using a non-release-candidate schema."
}

$badEntryMissingGovernanceMetricDetailsDir = Join-Path $failDir "entry-missing-governance-metric-details"
$badEntryMissingGovernanceMetricDetailsPath = Join-Path $badEntryMissingGovernanceMetricDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingGovernanceMetricDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingGovernanceMetricDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0
"@

$missingEntryGovernanceMetricDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingGovernanceMetricDetailsPath
} catch {
    $missingEntryGovernanceMetricDetailsFailedAsExpected = $true
}

if (-not $missingEntryGovernanceMetricDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document without governance metric detail markers."
}

$badEntryDetachedNumberingDetailsDir = Join-Path $failDir "entry-numbering-details-supplied-by-detached-notes"
$badEntryDetachedNumberingDetailsPath = Join-Path $badEntryDetachedNumberingDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryDetachedNumberingDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryDetachedNumberingDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence level=low score=56
- Detached governance details are intentionally outside this metric.
- Detached details: catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
"@

$detachedEntryNumberingDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryDetachedNumberingDetailsPath
} catch {
    $detachedEntryNumberingDetailsFailedAsExpected = $true
}

if (-not $detachedEntryNumberingDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with numbering details supplied only by detached notes."
}

$badEntryDetachedTableLayoutDetailsDir = Join-Path $failDir "entry-table-layout-details-supplied-by-detached-notes"
$badEntryDetachedTableLayoutDetailsPath = Join-Path $badEntryDetachedTableLayoutDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryDetachedTableLayoutDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryDetachedTableLayoutDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready
- Detached governance details are intentionally outside this metric.
- Detached details: table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

$detachedEntryTableLayoutDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryDetachedTableLayoutDetailsPath
} catch {
    $detachedEntryTableLayoutDetailsFailedAsExpected = $true
}

if (-not $detachedEntryTableLayoutDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with table-layout details supplied only by detached notes."
}

$badEntryMissingRepairDetailsDir = Join-Path $failDir "entry-missing-content-control-repair-details"
$badEntryMissingRepairDetailsPath = Join-Path $badEntryMissingRepairDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingRepairDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingRepairDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder -> sync_bound_content_control
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$missingEntryContentControlRepairDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingRepairDetailsPath
} catch {
    $missingEntryContentControlRepairDetailsFailedAsExpected = $true
}

if (-not $missingEntryContentControlRepairDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document without full content-control repair workflow details."
}

$badEntryDetachedContentControlDetailsDir = Join-Path $failDir "entry-content-control-details-supplied-by-detached-notes"
$badEntryDetachedContentControlDetailsPath = Join-Path $badEntryDetachedContentControlDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryDetachedContentControlDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryDetachedContentControlDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder -> sync_bound_content_control
- Detached governance details are intentionally outside this entry.
- Detached details: source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json input_docx=samples/invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
"@

$detachedEntryContentControlDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryDetachedContentControlDetailsPath
} catch {
    $detachedEntryContentControlDetailsFailedAsExpected = $true
}

if (-not $detachedEntryContentControlDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with content-control repair details supplied only by detached notes."
}

$badEntryMissingProjectTemplateContractsDir = Join-Path $failDir "entry-missing-project-template-contracts"
$badEntryMissingProjectTemplateContractsPath = Join-Path $badEntryMissingProjectTemplateContractsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingProjectTemplateContractsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingProjectTemplateContractsPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness release_ready
- Project template onboarding: project_template_onboarding.schema_approval approved
"@

$missingEntryProjectTemplateContractsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingProjectTemplateContractsPath
} catch {
    $missingEntryProjectTemplateContractsFailedAsExpected = $true
}

if (-not $missingEntryProjectTemplateContractsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document without full project template contract details."
}

$badEntryMissingReadinessSchemaSummaryDir = Join-Path $failDir "entry-missing-readiness-schema-summary"
$badEntryMissingReadinessSchemaSummaryPath = Join-Path $badEntryMissingReadinessSchemaSummaryDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingReadinessSchemaSummaryDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingReadinessSchemaSummaryPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Content-control provenance: input_docx=samples/invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$missingEntryReadinessSchemaSummaryFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingReadinessSchemaSummaryPath
} catch {
    $missingEntryReadinessSchemaSummaryFailedAsExpected = $true
}

if (-not $missingEntryReadinessSchemaSummaryFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document where only onboarding carried schema_approval_status_summary."
}

$badEntryProjectTemplateReadinessSplitDir = Join-Path $failDir "entry-project-template-readiness-split-block"
$badEntryProjectTemplateReadinessSplitPath = Join-Path $badEntryProjectTemplateReadinessSplitDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateReadinessSplitDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateReadinessSplitPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Detached readiness note: project_template_delivery_readiness source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$projectTemplateReadinessSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateReadinessSplitPath
} catch {
    $projectTemplateReadinessSplitFailedAsExpected = $true
}

if (-not $projectTemplateReadinessSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with project-template readiness details split across detached notes."
}

$badEntryProjectTemplateOnboardingSplitDir = Join-Path $failDir "entry-project-template-onboarding-split-block"
$badEntryProjectTemplateOnboardingSplitPath = Join-Path $badEntryProjectTemplateOnboardingSplitDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateOnboardingSplitDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateOnboardingSplitPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Detached onboarding note: project_template_onboarding.schema_approval source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$projectTemplateOnboardingSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateOnboardingSplitPath
} catch {
    $projectTemplateOnboardingSplitFailedAsExpected = $true
}

if (-not $projectTemplateOnboardingSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with project-template onboarding details split across detached notes."
}

$badEntryMissingReadinessForOnboardingDir = Join-Path $failDir "entry-missing-readiness-for-onboarding"
$badEntryMissingReadinessForOnboardingPath = Join-Path $badEntryMissingReadinessForOnboardingDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingReadinessForOnboardingDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingReadinessForOnboardingPath -Encoding UTF8 -Value @"
# START_HERE

- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$missingEntryReadinessForOnboardingFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingReadinessForOnboardingPath
} catch {
    $missingEntryReadinessForOnboardingFailedAsExpected = $true
}

if (-not $missingEntryReadinessForOnboardingFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with onboarding governance but without delivery readiness contract."
}

$badEntryProjectTemplateOnboardingSourceJsonDir = Join-Path $failDir "entry-onboarding-source-json-supplied-by-readiness"
$badEntryProjectTemplateOnboardingSourceJsonPath = Join-Path $badEntryProjectTemplateOnboardingSourceJsonDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateOnboardingSourceJsonDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateOnboardingSourceJsonPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$missingEntryProjectTemplateOnboardingSourceJsonFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateOnboardingSourceJsonPath
} catch {
    $missingEntryProjectTemplateOnboardingSourceJsonFailedAsExpected = $true
}

if (-not $missingEntryProjectTemplateOnboardingSourceJsonFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document where readiness source_json_display satisfied onboarding."
}

$badDraftFile = Join-Path $failDir "bad_draft.md"
Set-Content -LiteralPath $badDraftFile -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷璇存槑鑽夌

- This file is still drafting and should not be public.
"@

$badPathFile = Join-Path $failDir "bad_path.md"
Set-Content -LiteralPath $badPathFile -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷璇存槑

- Local path leak: C:\Users\wuxianggujun\CodeSpace\CMakeProjects\FeatherDoc\output\release-candidate-checks\report\release_body.zh-CN.md
"@

$failedAsExpected = $false
try {
    & $auditScript -Path @($badDraftFile, $badPathFile)
} catch {
    $failedAsExpected = $true
}

if (-not $failedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed forbidden release materials."
}
