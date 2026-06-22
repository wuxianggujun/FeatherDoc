$passContentControlSummaryPath = Join-Path $passDir "content_control_data_binding_governance_summary.json"
$passContentControlSummary = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "blocker"
            action = "sync_or_fill_bound_content_control"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            input_docx = $contentControlInputDocx
            input_docx_display = $contentControlInputDocxDisplay
            template_name = $contentControlTemplateName
            schema_target = $contentControlSchemaTarget
            target_mode = $contentControlTargetMode
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
            repair_action_classes = @("release_blocking", "auto_repair_candidate", "manual_confirmation_required")
        }
    )
}
($passContentControlSummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $passContentControlSummaryPath -Encoding UTF8

& $auditScript -Path $passContentControlSummaryPath

$passEntryGovernanceTracePath = Join-Path $passDir "START_HERE.md"
Set-Content -LiteralPath $passEntryGovernanceTracePath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder action=sync_or_fill_bound_content_control -> sync_bound_content_control
- Content-control provenance: input_docx=samples/invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control contract: source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json repair_action_classes=release_blocking,auto_repair_candidate,manual_confirmation_required
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence level=low score=56 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 fixed_layout_table_count=1 autofit_layout_table_count=1 unspecified_layout_table_count=1 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

& $auditScript -Path $passEntryGovernanceTracePath

$badEntryMissingContentControlActionDir = Join-Path $failDir "entry-missing-content-control-action"
$badEntryMissingContentControlActionPath = Join-Path $badEntryMissingContentControlActionDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingContentControlActionDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingContentControlActionPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder -> sync_bound_content_control
- Content-control provenance: input_docx=samples/invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control contract: source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json repair_action_classes=release_blocking,auto_repair_candidate,manual_confirmation_required
"@

$missingEntryContentControlActionFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingContentControlActionPath
} catch {
    $missingEntryContentControlActionFailedAsExpected = $true
}

if (-not $missingEntryContentControlActionFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md without content-control action."
}

$badEntryMissingContentControlRepairActionClassesDir = Join-Path $failDir "entry-missing-content-control-repair-action-classes"
$badEntryMissingContentControlRepairActionClassesPath = Join-Path $badEntryMissingContentControlRepairActionClassesDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingContentControlRepairActionClassesDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingContentControlRepairActionClassesPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder action=sync_or_fill_bound_content_control -> sync_bound_content_control
- Content-control provenance: input_docx=samples/invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control contract: source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
"@

$missingEntryContentControlRepairActionClassesFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingContentControlRepairActionClassesPath
} catch {
    $missingEntryContentControlRepairActionClassesFailedAsExpected = $true
}

if (-not $missingEntryContentControlRepairActionClassesFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md without content-control repair_action_classes."
}

$passEntryWorkflowDashboardTraceDir = Join-Path $passDir "entry-project-template-workflow-dashboard-trace"
$passEntryWorkflowDashboardTracePath = Join-Path $passEntryWorkflowDashboardTraceDir "START_HERE.md"
New-Item -ItemType Directory -Path $passEntryWorkflowDashboardTraceDir -Force | Out-Null
Set-Content -LiteralPath $passEntryWorkflowDashboardTracePath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Project template workflow dashboard status: blocked
- Project template workflow dashboard release ready: False
- Project template workflow dashboard counts: 2 reports, 2 blockers, 1 warnings
- Project template workflow dashboard summary: .\output\release-candidate-checks\report\project_template_workflow_dashboard.json
- Project template workflow dashboard report: .\output\release-candidate-checks\report\project_template_workflow_dashboard.md
- Project template workflow dashboard next action: review_schema_update_candidate (Project template onboarding schema approval is pending.)
- Project template workflow dashboard next blocker: project_template_onboarding.schema_approval
- Project template workflow dashboard next action groups: 1
- Project template workflow dashboard action group: source=project_template_onboarding_governance action=review_schema_update_candidate blocker=project_template_onboarding.schema_approval entries=invoice-template
"@

& $auditScript -Path $passEntryWorkflowDashboardTracePath

$passReviewerWorkflowDashboardTraceDir = Join-Path $passDir "reviewer-project-template-workflow-dashboard-trace"
$passReviewerWorkflowDashboardTracePath = Join-Path $passReviewerWorkflowDashboardTraceDir "REVIEWER_CHECKLIST.md"
New-Item -ItemType Directory -Path $passReviewerWorkflowDashboardTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReviewerWorkflowDashboardTracePath -Encoding UTF8 -Value @"
# REVIEWER_CHECKLIST

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Project template workflow dashboard status: blocked
- Project template workflow dashboard release ready: False
- Project template workflow dashboard counts: 2 reports, 2 blockers, 1 warnings
- Project template workflow dashboard summary: .\output\release-candidate-checks\report\project_template_workflow_dashboard.json
- Project template workflow dashboard report: .\output\release-candidate-checks\report\project_template_workflow_dashboard.md
- Project template workflow dashboard next action: review_schema_update_candidate (Project template onboarding schema approval is pending.)
- Project template workflow dashboard next action groups: 1
- Project template workflow dashboard action group: source=project_template_onboarding_governance action=review_schema_update_candidate blocker=project_template_onboarding.schema_approval entries=invoice-template
- [ ] Stop here until the project template workflow dashboard is release-ready; status `blocked`, release_ready `False`, blockers `2`, warnings `1`.
"@

& $auditScript -Path $passReviewerWorkflowDashboardTracePath

$badEntryWorkflowDashboardMissingGroupCountDir = Join-Path $failDir "entry-project-template-workflow-dashboard-missing-group-count"
$badEntryWorkflowDashboardMissingGroupCountPath = Join-Path $badEntryWorkflowDashboardMissingGroupCountDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryWorkflowDashboardMissingGroupCountDir -Force | Out-Null
Set-Content -LiteralPath $badEntryWorkflowDashboardMissingGroupCountPath -Encoding UTF8 -Value @"
# START_HERE

- Project template workflow dashboard status: blocked
- Project template workflow dashboard release ready: False
- Project template workflow dashboard counts: 2 reports, 2 blockers, 1 warnings
- Project template workflow dashboard summary: .\output\release-candidate-checks\report\project_template_workflow_dashboard.json
- Project template workflow dashboard report: .\output\release-candidate-checks\report\project_template_workflow_dashboard.md
- Project template workflow dashboard next action: review_schema_update_candidate (Project template onboarding schema approval is pending.)
- Project template workflow dashboard action group: source=project_template_onboarding_governance action=review_schema_update_candidate blocker=project_template_onboarding.schema_approval entries=invoice-template
"@

$badEntryWorkflowDashboardMissingGroupCountFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryWorkflowDashboardMissingGroupCountPath
} catch {
    $badEntryWorkflowDashboardMissingGroupCountFailedAsExpected = $true
}

if (-not $badEntryWorkflowDashboardMissingGroupCountFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with workflow dashboard action group count removed."
}

$badEntryWorkflowDashboardMissingActionGroupDir = Join-Path $failDir "entry-project-template-workflow-dashboard-missing-action-group"
$badEntryWorkflowDashboardMissingActionGroupPath = Join-Path $badEntryWorkflowDashboardMissingActionGroupDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryWorkflowDashboardMissingActionGroupDir -Force | Out-Null
Set-Content -LiteralPath $badEntryWorkflowDashboardMissingActionGroupPath -Encoding UTF8 -Value @"
# START_HERE

- Project template workflow dashboard status: blocked
- Project template workflow dashboard release ready: False
- Project template workflow dashboard counts: 2 reports, 2 blockers, 1 warnings
- Project template workflow dashboard summary: .\output\release-candidate-checks\report\project_template_workflow_dashboard.json
- Project template workflow dashboard report: .\output\release-candidate-checks\report\project_template_workflow_dashboard.md
- Project template workflow dashboard next action: review_schema_update_candidate (Project template onboarding schema approval is pending.)
- Project template workflow dashboard next action groups: 1
"@

$badEntryWorkflowDashboardMissingActionGroupFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryWorkflowDashboardMissingActionGroupPath
} catch {
    $badEntryWorkflowDashboardMissingActionGroupFailedAsExpected = $true
}

if (-not $badEntryWorkflowDashboardMissingActionGroupFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with non-zero workflow dashboard action group details removed."
}

$badReviewerWorkflowDashboardMissingStopDir = Join-Path $failDir "reviewer-project-template-workflow-dashboard-missing-stop"
$badReviewerWorkflowDashboardMissingStopPath = Join-Path $badReviewerWorkflowDashboardMissingStopDir "REVIEWER_CHECKLIST.md"
New-Item -ItemType Directory -Path $badReviewerWorkflowDashboardMissingStopDir -Force | Out-Null
Set-Content -LiteralPath $badReviewerWorkflowDashboardMissingStopPath -Encoding UTF8 -Value @"
# REVIEWER_CHECKLIST

- Project template workflow dashboard status: blocked
- Project template workflow dashboard release ready: False
- Project template workflow dashboard counts: 2 reports, 2 blockers, 1 warnings
- Project template workflow dashboard summary: .\output\release-candidate-checks\report\project_template_workflow_dashboard.json
- Project template workflow dashboard report: .\output\release-candidate-checks\report\project_template_workflow_dashboard.md
- Project template workflow dashboard next action: review_schema_update_candidate (Project template onboarding schema approval is pending.)
- Project template workflow dashboard next action groups: 1
- Project template workflow dashboard action group: source=project_template_onboarding_governance action=review_schema_update_candidate blocker=project_template_onboarding.schema_approval entries=invoice-template
"@

$badReviewerWorkflowDashboardMissingStopFailedAsExpected = $false
try {
    & $auditScript -Path $badReviewerWorkflowDashboardMissingStopPath
} catch {
    $badReviewerWorkflowDashboardMissingStopFailedAsExpected = $true
}

if (-not $badReviewerWorkflowDashboardMissingStopFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed REVIEWER_CHECKLIST.md with blocked workflow dashboard stop condition removed."
}

$badEntryTableLayoutPdfCoverageMissingDir = Join-Path $failDir "entry-table-layout-pdf-coverage-missing"
$badEntryTableLayoutPdfCoverageMissingPath = Join-Path $badEntryTableLayoutPdfCoverageMissingDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryTableLayoutPdfCoverageMissingDir -Force | Out-Null
Set-Content -LiteralPath $badEntryTableLayoutPdfCoverageMissingPath -Encoding UTF8 -Value @"
# START_HERE

- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

$badEntryTableLayoutPdfCoverageMissingFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryTableLayoutPdfCoverageMissingPath
} catch {
    $badEntryTableLayoutPdfCoverageMissingFailedAsExpected = $true
}

if (-not $badEntryTableLayoutPdfCoverageMissingFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document without PDF floating table support coverage guidance."
}

$badEntryTableLayoutPdfReviewerFocusMissingDir = Join-Path $failDir "entry-table-layout-pdf-reviewer-focus-missing"
$badEntryTableLayoutPdfReviewerFocusMissingPath = Join-Path $badEntryTableLayoutPdfReviewerFocusMissingDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryTableLayoutPdfReviewerFocusMissingDir -Force | Out-Null
Set-Content -LiteralPath $badEntryTableLayoutPdfReviewerFocusMissingPath -Encoding UTF8 -Value @"
# START_HERE

- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

$badEntryTableLayoutPdfReviewerFocusMissingFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryTableLayoutPdfReviewerFocusMissingPath
} catch {
    $badEntryTableLayoutPdfReviewerFocusMissingFailedAsExpected = $true
}

if (-not $badEntryTableLayoutPdfReviewerFocusMissingFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document without PDF floating table reviewer focus guidance."
}

$badEntryTableLayoutMetadataFieldsMissingDir = Join-Path $failDir "entry-table-layout-metadata-fields-missing"
$badEntryTableLayoutMetadataFieldsMissingPath = Join-Path $badEntryTableLayoutMetadataFieldsMissingDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryTableLayoutMetadataFieldsMissingDir -Force | Out-Null
Set-Content -LiteralPath $badEntryTableLayoutMetadataFieldsMissingPath -Encoding UTF8 -Value @"
# START_HERE

- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

$badEntryTableLayoutMetadataFieldsMissingFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryTableLayoutMetadataFieldsMissingPath
} catch {
    $badEntryTableLayoutMetadataFieldsMissingFailedAsExpected = $true
}

if (-not $badEntryTableLayoutMetadataFieldsMissingFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document without table layout metadata_only_fields guidance."
}

$badEntryTableLayoutReviewFieldsMissingDir = Join-Path $failDir "entry-table-layout-review-fields-missing"
$badEntryTableLayoutReviewFieldsMissingPath = Join-Path $badEntryTableLayoutReviewFieldsMissingDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryTableLayoutReviewFieldsMissingDir -Force | Out-Null
Set-Content -LiteralPath $badEntryTableLayoutReviewFieldsMissingPath -Encoding UTF8 -Value @"
# START_HERE

- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

$badEntryTableLayoutReviewFieldsMissingFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryTableLayoutReviewFieldsMissingPath
} catch {
    $badEntryTableLayoutReviewFieldsMissingFailedAsExpected = $true
}

if (-not $badEntryTableLayoutReviewFieldsMissingFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document without table layout review_required_fields guidance."
}

$passEntryProjectTemplateChecklistHandoffEvidenceDir = Join-Path $passDir "entry-project-template-checklist-handoff-evidence"
$passEntryProjectTemplateChecklistHandoffEvidencePath = Join-Path $passEntryProjectTemplateChecklistHandoffEvidenceDir "START_HERE.md"
New-Item -ItemType Directory -Path $passEntryProjectTemplateChecklistHandoffEvidenceDir -Force | Out-Null
Set-Content -LiteralPath $passEntryProjectTemplateChecklistHandoffEvidencePath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
"@

& $auditScript -Path $passEntryProjectTemplateChecklistHandoffEvidencePath

$badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaDir = Join-Path $failDir "entry-project-template-checklist-handoff-evidence-missing-source-schema"
$badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaPath = Join-Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_report=.\output\release-candidate-checks\summary.json
"@

$badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaPath
} catch {
    $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with project-template checklist handoff evidence missing source_schema."
}

$badEntryProjectTemplateChecklistHandoffEvidenceSplitDir = Join-Path $failDir "entry-project-template-checklist-handoff-evidence-split"
$badEntryProjectTemplateChecklistHandoffEvidenceSplitPath = Join-Path $badEntryProjectTemplateChecklistHandoffEvidenceSplitDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistHandoffEvidenceSplitDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistHandoffEvidenceSplitPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json

## Detached notes

- marker=release_entry_project_template_readiness_checklist_trace
"@

$badEntryProjectTemplateChecklistHandoffEvidenceSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistHandoffEvidenceSplitPath
} catch {
    $badEntryProjectTemplateChecklistHandoffEvidenceSplitFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistHandoffEvidenceSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with project-template checklist handoff marker supplied only by detached notes."
}

$badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsDir = Join-Path $failDir "entry-project-template-checklist-handoff-evidence-missing-paths"
$badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsPath = Join-Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
"@

$badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsPath
} catch {
    $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with project-template checklist handoff evidence missing entrypoint path_display values."
}

$badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceDir = Join-Path $failDir "entry-project-template-checklist-handoff-evidence-wrong-source"
$badEntryProjectTemplateChecklistHandoffEvidenceWrongSourcePath = Join-Path $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourcePath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-blocker-rollup\summary.json
"@

$badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourcePath
} catch {
    $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with project-template checklist handoff source_report pointing at the wrong evidence source."
}

$passEntryProjectTemplateChecklistPackagedAuditEvidenceDir = Join-Path $passDir "entry-project-template-checklist-packaged-audit-evidence"
$passEntryProjectTemplateChecklistPackagedAuditEvidencePath = Join-Path $passEntryProjectTemplateChecklistPackagedAuditEvidenceDir "START_HERE.md"
New-Item -ItemType Directory -Path $passEntryProjectTemplateChecklistPackagedAuditEvidenceDir -Force | Out-Null
Set-Content -LiteralPath $passEntryProjectTemplateChecklistPackagedAuditEvidencePath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1, status=passed, audit_script=.\scripts\assert_release_material_safety.ps1, audited_entrypoint_count=3, audited_entrypoints=start_here, artifact_guide, reviewer_checklist, compact_evidence_label=Project-template readiness checklist handoff evidence, compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports, compact_evidence_source_schema=featherdoc.release_candidate_summary, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, checklist_marker=release_entry_project_template_readiness_checklist_trace, material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-blocker-rollup\summary.json
"@

& $auditScript -Path $passEntryProjectTemplateChecklistPackagedAuditEvidencePath

$badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaDir = Join-Path $failDir "entry-project-template-checklist-packaged-audit-evidence-missing-source-schema"
$badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaPath = Join-Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1, status=passed, audit_script=.\scripts\assert_release_material_safety.ps1, audited_entrypoint_count=3, audited_entrypoints=start_here, artifact_guide, reviewer_checklist, compact_evidence_label=Project-template readiness checklist handoff evidence, compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports, compact_evidence_source_schema=featherdoc.release_candidate_summary, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, checklist_marker=release_entry_project_template_readiness_checklist_trace, material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace, source_report=.\output\release-blocker-rollup\summary.json
"@

$badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaPath
} catch {
    $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with packaged checklist audit evidence missing source_schema."
}

$badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitDir = Join-Path $failDir "entry-project-template-checklist-packaged-audit-evidence-split"
$badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitPath = Join-Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1, status=passed, audit_script=.\scripts\assert_release_material_safety.ps1, audited_entrypoint_count=3, compact_evidence_label=Project-template readiness checklist handoff evidence, compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports, compact_evidence_source_schema=featherdoc.release_candidate_summary, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, checklist_marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-blocker-rollup\summary.json

## Detached packaged audit notes

- audited_entrypoints=start_here, artifact_guide, reviewer_checklist
- material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace
"@

$badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitPath
} catch {
    $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with packaged checklist audit entrypoints and material-safety marker supplied only by detached notes."
}

$badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceDir = Join-Path $failDir "entry-project-template-checklist-packaged-audit-evidence-wrong-source"
$badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourcePath = Join-Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourcePath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1, status=passed, audit_script=.\scripts\assert_release_material_safety.ps1, audited_entrypoint_count=3, audited_entrypoints=start_here, artifact_guide, reviewer_checklist, compact_evidence_label=Project-template readiness checklist handoff evidence, compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports, compact_evidence_source_schema=featherdoc.release_candidate_summary, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, checklist_marker=release_entry_project_template_readiness_checklist_trace, material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
"@

$badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourcePath
} catch {
    $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with packaged checklist audit source_report pointing at the wrong evidence source."
}

$passEntryWordVisualStandardReviewMetadataEvidenceDir = Join-Path $passDir "entry-word-visual-standard-review-metadata-evidence"
$passEntryWordVisualStandardReviewMetadataEvidencePath = Join-Path $passEntryWordVisualStandardReviewMetadataEvidenceDir "START_HERE.md"
New-Item -ItemType Directory -Path $passEntryWordVisualStandardReviewMetadataEvidenceDir -Force | Out-Null
Set-Content -LiteralPath $passEntryWordVisualStandardReviewMetadataEvidencePath -Encoding UTF8 -Value @"
# START_HERE

- Word visual standard review metadata evidence: word_visual_standard_review_metadata_source_reports=1, metadata_count=4, task_keys=smoke, fixed_grid, section_page_setup, page_number_fields, status_summary=reviewed=4, verdict_summary=pass=4, task_reviews=smoke:review_task_key=document:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\document\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\document\report\final_review.md; fixed_grid:review_task_key=fixed_grid:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md; section_page_setup:review_task_key=section_page_setup:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md; page_number_fields:review_task_key=page_number_fields:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
"@

& $auditScript -Path $passEntryWordVisualStandardReviewMetadataEvidencePath

$passFinalReviewWordVisualStandardReviewMetadataEvidenceDir = Join-Path $passDir "final-review-word-visual-standard-review-metadata-evidence"
$passFinalReviewWordVisualStandardReviewMetadataEvidencePath = Join-Path $passFinalReviewWordVisualStandardReviewMetadataEvidenceDir "final_review.md"
New-Item -ItemType Directory -Path $passFinalReviewWordVisualStandardReviewMetadataEvidenceDir -Force | Out-Null
Set-Content -LiteralPath $passFinalReviewWordVisualStandardReviewMetadataEvidencePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Word visual standard review metadata evidence

- Word visual standard review metadata evidence: word_visual_standard_review_metadata_source_reports=1, metadata_count=4, task_keys=smoke, fixed_grid, section_page_setup, page_number_fields, status_summary=reviewed=4, verdict_summary=pass=4, task_reviews=smoke:review_task_key=document:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\document\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\document\report\final_review.md; fixed_grid:review_task_key=fixed_grid:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md; section_page_setup:review_task_key=section_page_setup:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md; page_number_fields:review_task_key=page_number_fields:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
"@

& $auditScript -Path $passFinalReviewWordVisualStandardReviewMetadataEvidencePath

$passReleaseHandoffWordVisualStandardReviewMetadataEvidenceDir = Join-Path $passDir "release-handoff-word-visual-standard-review-metadata-evidence"
$passReleaseHandoffWordVisualStandardReviewMetadataEvidencePath = Join-Path $passReleaseHandoffWordVisualStandardReviewMetadataEvidenceDir "release_handoff.md"
New-Item -ItemType Directory -Path $passReleaseHandoffWordVisualStandardReviewMetadataEvidenceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseHandoffWordVisualStandardReviewMetadataEvidencePath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.0

## Word Visual Standard Review Metadata Evidence

- Word visual standard review metadata evidence: word_visual_standard_review_metadata_source_reports=1, metadata_count=4, task_keys=smoke, fixed_grid, section_page_setup, page_number_fields, status_summary=reviewed=4, verdict_summary=pass=4, task_reviews=smoke:review_task_key=document:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\document\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\document\report\final_review.md; fixed_grid:review_task_key=fixed_grid:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md; section_page_setup:review_task_key=section_page_setup:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md; page_number_fields:review_task_key=page_number_fields:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
"@

& $auditScript -Path $passReleaseHandoffWordVisualStandardReviewMetadataEvidencePath

$badEntryWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaDir = Join-Path $failDir "entry-word-visual-standard-review-metadata-evidence-missing-source-schema"
$badEntryWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaPath = Join-Path $badEntryWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badEntryWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaPath -Encoding UTF8 -Value @"
# START_HERE

- Word visual standard review metadata evidence: word_visual_standard_review_metadata_source_reports=1, metadata_count=4, task_keys=smoke, fixed_grid, section_page_setup, page_number_fields, status_summary=reviewed=4, verdict_summary=pass=4, task_reviews=smoke:review_task_key=document:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\document\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\document\report\final_review.md; fixed_grid:review_task_key=fixed_grid:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md; section_page_setup:review_task_key=section_page_setup:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md; page_number_fields:review_task_key=page_number_fields:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md, source_report=.\output\release-candidate-checks\summary.json
"@

$badEntryWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaPath
} catch {
    $badEntryWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaFailedAsExpected = $true
}

if (-not $badEntryWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with Word visual metadata evidence missing source_schema."
}

$badEntryWordVisualStandardReviewMetadataEvidenceWrongSourceDir = Join-Path $failDir "entry-word-visual-standard-review-metadata-evidence-wrong-source"
$badEntryWordVisualStandardReviewMetadataEvidenceWrongSourcePath = Join-Path $badEntryWordVisualStandardReviewMetadataEvidenceWrongSourceDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryWordVisualStandardReviewMetadataEvidenceWrongSourceDir -Force | Out-Null
Set-Content -LiteralPath $badEntryWordVisualStandardReviewMetadataEvidenceWrongSourcePath -Encoding UTF8 -Value @"
# START_HERE

- Word visual standard review metadata evidence: word_visual_standard_review_metadata_source_reports=1, metadata_count=4, task_keys=smoke, fixed_grid, section_page_setup, page_number_fields, status_summary=reviewed=4, verdict_summary=pass=4, task_reviews=smoke:review_task_key=document:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\document\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\document\report\final_review.md; fixed_grid:review_task_key=fixed_grid:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md; section_page_setup:review_task_key=section_page_setup:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md; page_number_fields:review_task_key=page_number_fields:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-blocker-rollup\summary.json
"@

$badEntryWordVisualStandardReviewMetadataEvidenceWrongSourceFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryWordVisualStandardReviewMetadataEvidenceWrongSourcePath
} catch {
    $badEntryWordVisualStandardReviewMetadataEvidenceWrongSourceFailedAsExpected = $true
}

if (-not $badEntryWordVisualStandardReviewMetadataEvidenceWrongSourceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with Word visual metadata source_report pointing at the wrong evidence source."
}

$badFinalReviewWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaDir = Join-Path $failDir "final-review-word-visual-standard-review-metadata-evidence-missing-source-schema"
$badFinalReviewWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaPath = Join-Path $badFinalReviewWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Word visual standard review metadata evidence

- Word visual standard review metadata evidence: word_visual_standard_review_metadata_source_reports=1, metadata_count=4, task_keys=smoke, fixed_grid, section_page_setup, page_number_fields, status_summary=reviewed=4, verdict_summary=pass=4, task_reviews=smoke:review_task_key=document:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\document\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\document\report\final_review.md; fixed_grid:review_task_key=fixed_grid:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md; section_page_setup:review_task_key=section_page_setup:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md; page_number_fields:review_task_key=page_number_fields:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md, source_report=.\output\release-candidate-checks\summary.json
"@

$badFinalReviewWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaPath
} catch {
    $badFinalReviewWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaFailedAsExpected = $true
}

if (-not $badFinalReviewWordVisualStandardReviewMetadataEvidenceMissingSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with Word visual metadata evidence missing source_schema."
}

$badReleaseHandoffWordVisualStandardReviewMetadataEvidenceWrongSourceDir = Join-Path $failDir "release-handoff-word-visual-standard-review-metadata-evidence-wrong-source"
$badReleaseHandoffWordVisualStandardReviewMetadataEvidenceWrongSourcePath = Join-Path $badReleaseHandoffWordVisualStandardReviewMetadataEvidenceWrongSourceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffWordVisualStandardReviewMetadataEvidenceWrongSourceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffWordVisualStandardReviewMetadataEvidenceWrongSourcePath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.0

## Word Visual Standard Review Metadata Evidence

- Word visual standard review metadata evidence: word_visual_standard_review_metadata_source_reports=1, metadata_count=4, task_keys=smoke, fixed_grid, section_page_setup, page_number_fields, status_summary=reviewed=4, verdict_summary=pass=4, task_reviews=smoke:review_task_key=document:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\document\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\document\report\final_review.md; fixed_grid:review_task_key=fixed_grid:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md; section_page_setup:review_task_key=section_page_setup:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md; page_number_fields:review_task_key=page_number_fields:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-blocker-rollup\summary.json
"@

$badReleaseHandoffWordVisualStandardReviewMetadataEvidenceWrongSourceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffWordVisualStandardReviewMetadataEvidenceWrongSourcePath
} catch {
    $badReleaseHandoffWordVisualStandardReviewMetadataEvidenceWrongSourceFailedAsExpected = $true
}

if (-not $badReleaseHandoffWordVisualStandardReviewMetadataEvidenceWrongSourceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with Word visual metadata source_report pointing at the wrong evidence source."
}

$badEntryWordVisualStandardReviewMetadataEvidenceMissingTaskDir = Join-Path $failDir "entry-word-visual-standard-review-metadata-evidence-missing-task"
$badEntryWordVisualStandardReviewMetadataEvidenceMissingTaskPath = Join-Path $badEntryWordVisualStandardReviewMetadataEvidenceMissingTaskDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryWordVisualStandardReviewMetadataEvidenceMissingTaskDir -Force | Out-Null
Set-Content -LiteralPath $badEntryWordVisualStandardReviewMetadataEvidenceMissingTaskPath -Encoding UTF8 -Value @"
# START_HERE

- Word visual standard review metadata evidence: word_visual_standard_review_metadata_source_reports=1, metadata_count=4, task_keys=smoke, fixed_grid, section_page_setup, page_number_fields, status_summary=reviewed=4, verdict_summary=pass=4, task_reviews=smoke:review_task_key=document:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\document\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\document\report\final_review.md; fixed_grid:review_task_key=fixed_grid:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md; section_page_setup:review_task_key=section_page_setup:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
"@

$badEntryWordVisualStandardReviewMetadataEvidenceMissingTaskFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryWordVisualStandardReviewMetadataEvidenceMissingTaskPath
} catch {
    $badEntryWordVisualStandardReviewMetadataEvidenceMissingTaskFailedAsExpected = $true
}

if (-not $badEntryWordVisualStandardReviewMetadataEvidenceMissingTaskFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with Word visual metadata evidence missing page_number_fields task review."
}

$badEntryWordVisualStandardReviewMetadataEvidenceReviewNoteDir = Join-Path $failDir "entry-word-visual-standard-review-metadata-evidence-review-note"
$badEntryWordVisualStandardReviewMetadataEvidenceReviewNotePath = Join-Path $badEntryWordVisualStandardReviewMetadataEvidenceReviewNoteDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryWordVisualStandardReviewMetadataEvidenceReviewNoteDir -Force | Out-Null
Set-Content -LiteralPath $badEntryWordVisualStandardReviewMetadataEvidenceReviewNotePath -Encoding UTF8 -Value @"
# START_HERE

- Word visual standard review metadata evidence: word_visual_standard_review_metadata_source_reports=1, metadata_count=4, task_keys=smoke, fixed_grid, section_page_setup, page_number_fields, status_summary=reviewed=4, verdict_summary=pass=4, task_reviews=smoke:review_task_key=document:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\document\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\document\report\final_review.md; fixed_grid:review_task_key=fixed_grid:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md; section_page_setup:review_task_key=section_page_setup:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md; page_number_fields:review_task_key=page_number_fields:verdict=pass:review_status=reviewed:review_method=operator_supplied:review_result_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json:final_review_path=.\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json

## Detached private notes

- review_note: do not publish this operator note
"@

$badEntryWordVisualStandardReviewMetadataEvidenceReviewNoteFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryWordVisualStandardReviewMetadataEvidenceReviewNotePath
} catch {
    $badEntryWordVisualStandardReviewMetadataEvidenceReviewNoteFailedAsExpected = $true
}

if (-not $badEntryWordVisualStandardReviewMetadataEvidenceReviewNoteFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with Word visual metadata review_note."
}

$badStartHereProjectTemplateMissingChecklistDir = Join-Path $failDir "start-here-project-template-readiness-checklist-missing"
$badStartHereProjectTemplateMissingChecklistPath = Join-Path $badStartHereProjectTemplateMissingChecklistDir "START_HERE.md"
New-Item -ItemType Directory -Path $badStartHereProjectTemplateMissingChecklistDir -Force | Out-Null
Set-Content -LiteralPath $badStartHereProjectTemplateMissingChecklistPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badStartHereProjectTemplateMissingChecklistFailedAsExpected = $false
try {
    & $auditScript -Path $badStartHereProjectTemplateMissingChecklistPath
} catch {
    $badStartHereProjectTemplateMissingChecklistFailedAsExpected = $true
}

if (-not $badStartHereProjectTemplateMissingChecklistFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md without the fixed project-template release readiness checklist."
}

$passReleaseSummaryTracePath = Join-Path $passDir "release_summary.zh-CN.md"
Set-Content -LiteralPath $passReleaseSummaryTracePath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

& $auditScript -Path $passReleaseSummaryTracePath

$passReleaseSummaryPdfTraceDir = Join-Path $passDir "release-summary-pdf-visual-gate-trace"
$passReleaseSummaryPdfTracePath = Join-Path $passReleaseSummaryPdfTraceDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $passReleaseSummaryPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseSummaryPdfTracePath -Encoding UTF8 -Value @"
# Release summary

- PDF visual gate: verdict=pass summary=.\output\pdf-visual-release-gate-current\report\summary.json aggregate_contact_sheet=.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png cjk_manifest_count=43 cjk_copy_search_count=43 visual_baseline_manifest_count=42 visual_baseline_count=44
"@

& $auditScript -Path $passReleaseSummaryPdfTracePath

$passStartHerePdfTraceDir = Join-Path $passDir "start-here-pdf-visual-gate-trace"
$passStartHerePdfTracePath = Join-Path $passStartHerePdfTraceDir "START_HERE.md"
New-Item -ItemType Directory -Path $passStartHerePdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passStartHerePdfTracePath -Encoding UTF8 -Value @"
# START_HERE

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF release readiness checklist: docs/pdf_release_readiness_checklist_zh.rst
"@

& $auditScript -Path $passStartHerePdfTracePath

$passReviewerChecklistPdfTraceDir = Join-Path $passDir "reviewer-checklist-pdf-visual-gate-trace"
$passReviewerChecklistPdfTracePath = Join-Path $passReviewerChecklistPdfTraceDir "REVIEWER_CHECKLIST.md"
New-Item -ItemType Directory -Path $passReviewerChecklistPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReviewerChecklistPdfTracePath -Encoding UTF8 -Value @"
# Reviewer Checklist

- Confirm PDF visual gate summary .\output\pdf-visual-release-gate-current\report\summary.json with 43 CJK copy/search samples and 44 visual baselines before release.
- Confirm PDF visual gate aggregate contact sheet .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png before release.
- Confirm the fixed PDF release readiness checklist has been reviewed before publishing: docs/pdf_release_readiness_checklist_zh.rst.
"@

& $auditScript -Path $passReviewerChecklistPdfTracePath

$passReviewerChecklistFinalizePdfTraceDir = Join-Path $passDir "reviewer-checklist-pdf-visual-finalize-trace"
$passReviewerChecklistFinalizePdfTracePath = Join-Path $passReviewerChecklistFinalizePdfTraceDir "REVIEWER_CHECKLIST.md"
New-Item -ItemType Directory -Path $passReviewerChecklistFinalizePdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReviewerChecklistFinalizePdfTracePath -Encoding UTF8 -Value @"
# Reviewer Checklist

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- [ ] Confirm the PDF visual gate finalize evidence is signed off: verdict ``pass``, summary .\output\pdf-visual-release-gate-current\report\summary.json, aggregate contact sheet .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png, CJK manifest samples ``43``, CJK copy/search samples ``43``, missing text ``0``, visual baseline manifest samples ``42``, visual baselines ``44``.
- [ ] Confirm the fixed PDF release readiness checklist has been reviewed before publishing: docs/pdf_release_readiness_checklist_zh.rst.
"@

& $auditScript -Path $passReviewerChecklistFinalizePdfTracePath

$badStartHerePdfMissingChecklistDir = Join-Path $failDir "start-here-pdf-readiness-checklist-missing"
$badStartHerePdfMissingChecklistPath = Join-Path $badStartHerePdfMissingChecklistDir "START_HERE.md"
New-Item -ItemType Directory -Path $badStartHerePdfMissingChecklistDir -Force | Out-Null
Set-Content -LiteralPath $badStartHerePdfMissingChecklistPath -Encoding UTF8 -Value @"
# START_HERE

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badStartHerePdfMissingChecklistFailedAsExpected = $false
try {
    & $auditScript -Path $badStartHerePdfMissingChecklistPath
} catch {
    $badStartHerePdfMissingChecklistFailedAsExpected = $true
}

if (-not $badStartHerePdfMissingChecklistFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md without the fixed PDF release readiness checklist."
}

$passReleaseBodyTraceDir = Join-Path $passDir "release-body-project-template-trace"
$passReleaseBodyTracePath = Join-Path $passReleaseBodyTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $passReleaseBodyTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseBodyTracePath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

& $auditScript -Path $passReleaseBodyTracePath

$passReleaseBodyPdfTraceDir = Join-Path $passDir "release-body-pdf-visual-gate-trace"
$passReleaseBodyPdfTracePath = Join-Path $passReleaseBodyPdfTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $passReleaseBodyPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseBodyPdfTracePath -Encoding UTF8 -Value @"
# Release body

## Validation

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: loaded
- PDF visual gate verdict: pass
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
"@

& $auditScript -Path $passReleaseBodyPdfTracePath

$passReleaseHandoffTraceDir = Join-Path $passDir "release-handoff-project-template-trace"
$passReleaseHandoffTracePath = Join-Path $passReleaseHandoffTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $passReleaseHandoffTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseHandoffTracePath -Encoding UTF8 -Value @"
# Release handoff

- Project-template readiness checklist entrypoints evidence source reports: 1
  - source_report: .\output\release-candidate-checks\summary.json schema=featherdoc.release_candidate_summary
    - project_template_readiness_checklist_entrypoints_status: declared
    - project_template_readiness_checklist_entrypoints_checklist_label: Project template release readiness checklist
    - project_template_readiness_checklist_entrypoints_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: 3
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: start_here, artifact_guide, reviewer_checklist
    - project_template_readiness_checklist_entrypoints_checklist_marker: release_entry_project_template_readiness_checklist_trace
- Release-entry project-template readiness checklist material-safety audit source reports: 1
  - source_report: .\output\release-blocker-rollup\summary.json schema=featherdoc.release_candidate_summary
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
- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

& $auditScript -Path $passReleaseHandoffTracePath

$badReleaseMetadataProjectTemplateChecklistWrongSchemaDir = Join-Path $failDir "release-metadata-project-template-checklist-wrong-schema"
$badReleaseMetadataProjectTemplateChecklistWrongSchemaPath = Join-Path $badReleaseMetadataProjectTemplateChecklistWrongSchemaDir "START_HERE.md"
New-Item -ItemType Directory -Path $badReleaseMetadataProjectTemplateChecklistWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseMetadataProjectTemplateChecklistWrongSchemaPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist entrypoints evidence source reports: 1
  - source_report: .\output\release-blocker-rollup\summary.json schema=featherdoc.release_blocker_rollup_report.v1
    - project_template_readiness_checklist_entrypoints_status: declared
    - project_template_readiness_checklist_entrypoints_checklist_label: Project template release readiness checklist
    - project_template_readiness_checklist_entrypoints_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: 3
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: start_here, artifact_guide, reviewer_checklist
    - project_template_readiness_checklist_entrypoints_checklist_marker: release_entry_project_template_readiness_checklist_trace
"@

$badReleaseMetadataProjectTemplateChecklistWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseMetadataProjectTemplateChecklistWrongSchemaPath
} catch {
    $badReleaseMetadataProjectTemplateChecklistWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseMetadataProjectTemplateChecklistWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release metadata with project-template checklist entrypoints source_report using a non-release-candidate schema."
}

$badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir = Join-Path $failDir "release-metadata-project-template-checklist-material-safety-audit-wrong-schema"
$badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath = Join-Path $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath -Encoding UTF8 -Value @"
# Release handoff

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

$badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath
} catch {
    $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release metadata with release-entry project-template checklist material-safety audit source_report using a non-release-candidate schema."
}

$passReleaseHandoffPdfTraceDir = Join-Path $passDir "release-handoff-pdf-visual-gate-trace"
$passReleaseHandoffPdfTracePath = Join-Path $passReleaseHandoffPdfTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $passReleaseHandoffPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseHandoffPdfTracePath -Encoding UTF8 -Value @"
# Release handoff

## PDF visual gate evidence

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: ready
- PDF visual gate verdict: pass
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

& $auditScript -Path $passReleaseHandoffPdfTracePath

$passReleaseGovernanceHandoffTraceDir = Join-Path $passDir "release-governance-handoff-project-template-trace"
$passReleaseGovernanceHandoffTracePath = Join-Path $passReleaseGovernanceHandoffTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
"@

& $auditScript -Path $passReleaseGovernanceHandoffTracePath

$passReleaseGovernanceHandoffOnboardingTraceDir = Join-Path $passDir "release-governance-handoff-project-template-onboarding-trace"
$passReleaseGovernanceHandoffOnboardingTracePath = Join-Path $passReleaseGovernanceHandoffOnboardingTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffOnboardingTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffOnboardingTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blockers

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

& $auditScript -Path $passReleaseGovernanceHandoffOnboardingTracePath

$passReleaseGovernanceHandoffPdfTraceDir = Join-Path $passDir "release-governance-handoff-pdf-visual-gate-trace"
$passReleaseGovernanceHandoffPdfTracePath = Join-Path $passReleaseGovernanceHandoffPdfTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffPdfTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``smoke-import, contract-static, cjk-flow-static, regression-basic-text, regression-styled-document, regression-business-samples, regression-table-layout``
    - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-subset-current\summary.json, .\build\pdf-ctest-bounded-contract-static-current\summary.json, .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json, .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json, .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json, .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json, .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json``
"@

& $auditScript -Path $passReleaseGovernanceHandoffPdfTracePath
