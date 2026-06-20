$handoffPath = Join-Path $reportDir "release_handoff.md"
$bodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$shortPath = Join-Path $reportDir "release_summary.zh-CN.md"

Assert-Contains -Path $handoffPath -ExpectedText 'Project version: 1.6.0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'publish_github_release.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Smoke verdict: pass' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Fixed-grid verdict: undetermined' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Section page setup verdict: pass' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Page number fields verdict: pending_manual_review' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Template table CLI selector review task' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_latest_word_review_task.ps1 -SourceKind template-table-cli-selector-visual-regression-bundle -PrintPrompt' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Superseded review tasks: 0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Release blockers: 1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Project template workflow dashboard status: blocked' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Project template workflow dashboard release ready: False' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Project template workflow dashboard counts: 2 reports, 2 blockers, 1 warnings' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Project template workflow dashboard business document types: invoice=1, policy=1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Project template workflow dashboard corpus roles: planned-business-template=1, registered-business-template=1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Project template workflow dashboard next action groups: 2' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Project template workflow dashboard action group: source=project_template_onboarding_governance action=review_schema_update_candidate blocker=project_template_onboarding.schema_approval entries=invoice-template' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Project template workflow dashboard action group: source=project_template_delivery_readiness action=collect_project_template_onboarding_governance_evidence blocker=project_template_delivery_readiness.onboarding_governance entries=policy-template' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Project template workflow dashboard action source: source=project_template_onboarding_governance action_group_count=1 source_json=.\output\project-template-onboarding-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Project template workflow dashboard action source: source=project_template_delivery_readiness action_group_count=1 source_json=.\output\project-template-delivery-readiness\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'project_template_smoke.schema_approval' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'missing_reviewer' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Release Governance Rollup Details' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'document_skeleton.style_numbering_issues' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_schema=featherdoc.table_layout_delivery_governance_report.v1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_report_display: .\output\table-layout-delivery-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\numbering-catalog-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'matched_document_count=4' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'catalog_coverage_percent=100' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText "style_numbering_issues(count=4, penalty=20)" -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\table-layout-delivery-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'ready_document_percent=100' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'table_style_issue_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'automatic_tblLook_fix_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'manual_table_style_fix_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'table_position_automatic_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'table_position_review_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'pdf_floating_table_supported_geometry_percent=44' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'pdf_floating_table_tracked_geometry_count=9' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText "pdf_floating_table_support_coverage: 4/9 supported (44 percent); metadata_only=5" -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'pdf_floating_table_reviewer_focus: review metadata-only tblpPr fields before approving PDF-layout-sensitive release.' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'pdf_floating_table_metadata_only_fields: leftFromText, rightFromText, topFromText outside paragraph anchoring, tblOverlap' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'pdf_floating_table_review_required_fields: full Word-compatible floating table text wrapping, table overlap avoidance and collision resolution' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'command_failure_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'unresolved_item_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText "floating_table_plans_pending(count=0, penalty=0)" -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'action=review_style_numbering_audit' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'action=sync_or_fill_bound_content_control' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'repair_strategy: sync_bound_content_control' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText $contentControlCommandTemplateMarker -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'action=review_schema_update_candidate' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'action=resolve_pending_schema_approvals' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_schema=featherdoc.document_skeleton_governance_rollup_report.v1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_schema=featherdoc.content_control_data_binding_governance_report.v1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_schema=featherdoc.project_template_onboarding_governance_report.v1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_schema=featherdoc.schema_patch_confidence_calibration_report.v1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_report_display: .\output\document-skeleton-governance-rollup\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_report_display: .\output\project-template-delivery-readiness\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_report_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\document-skeleton-governance\contract\style-numbering-audit.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\content-control-data-binding\inspect-content-controls.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'document_skeleton.exemplar_catalog_missing' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'custom_xml_sync_evidence_missing' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'schema_patch_confidence_calibration.unscored_candidates' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_document_skeleton_rollup' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Release Governance Handoff Details' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Handoff Warnings' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'schema_patch_confidence_calibration.unscored_candidates' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Word Visual Standard Review Metadata Evidence' -Label 'release_handoff.md'
Assert-LineContainsAll -Path $handoffPath -ExpectedFragments @(
    'Word visual standard review metadata evidence',
    'word_visual_standard_review_metadata_source_reports=1',
    'metadata_count=4',
    'task_keys=smoke, fixed_grid, section_page_setup, page_number_fields',
    'status_summary=reviewed=4',
    'verdict_summary=pass=4',
    'smoke:review_task_key=document',
    'fixed_grid:review_task_key=fixed_grid',
    'section_page_setup:review_task_key=section_page_setup',
    'page_number_fields:review_task_key=page_number_fields',
    'review_result_path=',
    'final_review_path=',
    'source_schema=featherdoc.release_candidate_summary',
    'source_report=.\output\release-candidate-checks\summary.json'
) -Label 'release_handoff.md'
Assert-NotContains -Path $handoffPath -UnexpectedText 'review_note' -Label 'release_handoff.md'
Assert-MarkdownListBlockContainsAll -Path $handoffPath `
    -Anchor 'Release-entry project-template readiness checklist material-safety audit source reports: 1' `
    -ExpectedFragments @(
        'source_report: .\output\release-blocker-rollup\summary.json',
        'schema=featherdoc.release_candidate_summary',
        'release_entry_project_template_readiness_checklist_material_safety_audit_status: passed',
        'release_entry_project_template_readiness_checklist_material_safety_audit_script: .\scripts\assert_release_material_safety.ps1',
        'release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: 3',
        'release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: start_here, artifact_guide, reviewer_checklist',
        'release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: Project-template readiness checklist handoff evidence',
        'release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: project_template_readiness_checklist_entrypoints_source_reports',
        'release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: featherdoc.release_candidate_summary',
        'release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: docs/project_template_release_readiness_checklist_zh.rst',
        'release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: release_entry_project_template_readiness_checklist_trace',
        'release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace'
    ) -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_report_display: .\output\project-template-delivery-readiness\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText $expectedSupersededReviewTasksReportDisplayPath -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_latest_page_number_fields_review_task.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'find_superseded_review_tasks.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $bodyPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\visual-validation' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Release blockers: 1' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'project_template_smoke.schema_approval' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Project template governance contracts' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'project_template_delivery_readiness project_template_delivery_readiness_contract' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'source_schema=featherdoc.project_template_delivery_readiness_report.v1' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'latest_schema_approval_gate_status=pending_review' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1' -Label 'release_body.zh-CN.md'
Assert-LineContainsAll -Path $bodyPath -ExpectedFragments @(
    'Project template readiness:',
    'latest_schema_approval_gate_status=pending_review',
    'schema_approval_status_summary=pending_review',
    'onboarding_governance_next_action_group_count=1',
    'onboarding_governance_next_action=action=review_schema_update_candidate',
    'onboarding_governance_next_action_summary=action=review_schema_update_candidate',
    'requires_reviewer_action=True',
    'reviewer_action_summary=review_schema_update_candidate',
    'reviewer_action_reason=latest_review_state=pending; issue_keys=(none)',
    'reviewer_actions=review_schema_update_candidate',
    'source_report_display=.\output\project-template-delivery-readiness\summary.json',
    'source_json_display=.\output\project-template-delivery-readiness\summary.json'
) -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'source_json_display=.\output\project-template-delivery-readiness\summary.json' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'source_schema=featherdoc.project_template_onboarding_governance_report.v1' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'schema_approval_status_summary=pending_review' -Label 'release_body.zh-CN.md'
Assert-LineContainsAll -Path $bodyPath -ExpectedFragments @(
    'Project template onboarding:',
    'project_template_onboarding.schema_approval',
    'schema_approval_status_summary=pending_review',
    'next_action=action=review_schema_update_candidate',
    'next_action_summary=action=review_schema_update_candidate',
    'next_action_group_count=1',
    'requires_reviewer_action=True',
    'reviewer_action_summary=review_schema_update_candidate',
    'reviewer_action_reason=latest_review_state=pending; issue_keys=(none)',
    'reviewer_actions=review_schema_update_candidate',
    'source_report_display=.\output\project-template-onboarding-governance\summary.json',
    'source_json_display=.\output\project-template-onboarding-governance\summary.json'
) -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'source_json_display=.\output\project-template-onboarding-governance\summary.json' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Project template release checklist evidence' -Label 'release_body.zh-CN.md'
Assert-LineContainsAll -Path $bodyPath -ExpectedFragments @(
    'Project-template readiness checklist handoff evidence',
    'project_template_readiness_checklist_entrypoints_source_reports=1',
    'docs/project_template_release_readiness_checklist_zh.rst',
    'required_entrypoint_count=3',
    'entrypoint_paths=',
    'source_schema=featherdoc.release_candidate_summary',
    'source_report=.\output\release-candidate-checks\summary.json'
) -Label 'release_body.zh-CN.md'
Assert-LineContainsAll -Path $bodyPath -ExpectedFragments @(
    'Project-template readiness checklist packaged audit evidence',
    'release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1',
    'audit_script=.\scripts\assert_release_material_safety.ps1',
    'audited_entrypoints=start_here, artifact_guide, reviewer_checklist',
    'compact_evidence_source_schema=featherdoc.release_candidate_summary',
    'project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace',
    'source_schema=featherdoc.release_candidate_summary',
    'source_report=.\output\release-blocker-rollup\summary.json'
) -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Readiness action evidence:' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'provide_pdf_dependency_input/pdf_dependency_inputs_ready -> PDFio source header' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'enable_pdf_build_option/pdf_build_options_enabled -> FEATHERDOC_BUILD_PDF_IMPORT' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'source JSON: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Smoke verdict' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Fixed-grid verdict' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Section page setup verdict' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Page number fields verdict' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Template table CLI selector verdict' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'smoke=`pass`' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText ('fixed-grid={0}undetermined{0}' -f [char]96) -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'section page setup=`pass`' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'page number fields=`pending_manual_review`' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'Template table CLI selector=`pass`' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'project-template readiness governance contract' -Label 'release_summary.zh-CN.md'
Assert-LineContainsAll -Path $shortPath -ExpectedFragments @(
    'project-template readiness governance contract',
    'status=blocked',
    'release_ready=False',
    'latest_schema_approval_gate_status=pending_review',
    'schema_approval_status_summary=pending_review',
    'onboarding_governance_next_action=action=review_schema_update_candidate',
    'onboarding_governance_next_action_group_count=1',
    'release_blocker_count=1',
    'warning_count=0',
    'requires_reviewer_action=True',
    'reviewer_action_summary=review_schema_update_candidate',
    'reviewer_action_reason=latest_review_state=pending; issue_keys=(none)',
    'reviewer_actions=review_schema_update_candidate',
    'source_report_display=.\output\project-template-delivery-readiness\summary.json',
    'source_json_display=.\output\project-template-delivery-readiness\summary.json'
) -Label 'release_summary.zh-CN.md'

$warningOnlyReviewerActionDir = Join-Path $resolvedWorkingDir "warning-only-project-template-reviewer-action"
New-Item -ItemType Directory -Path $warningOnlyReviewerActionDir -Force | Out-Null
$warningOnlySummaryPath = Join-Path $warningOnlyReviewerActionDir "summary.json"
$warningOnlyBodyPath = Join-Path $warningOnlyReviewerActionDir "release_body.zh-CN.md"
$warningOnlyShortPath = Join-Path $warningOnlyReviewerActionDir "release_summary.zh-CN.md"
$warningOnlySummary = $summary | ConvertTo-Json -Depth 30 | ConvertFrom-Json
$warningOnlySummary.release_governance_handoff.release_blockers = @(
    $warningOnlySummary.release_governance_handoff.release_blockers |
        Where-Object { [string]$_.report_id -ne "project_template_delivery_readiness" }
)
$warningOnlySummary.release_governance_handoff.action_items = @(
    $warningOnlySummary.release_governance_handoff.action_items |
        Where-Object { [string]$_.report_id -ne "project_template_delivery_readiness" }
)
$warningOnlySummary.release_blocker_rollup.warnings = @($warningOnlySummary.release_blocker_rollup.warnings) + [pscustomobject]@{
    report_id = "project_template_delivery_readiness"
    id = "project_template_delivery_readiness.warning_only_reviewer_action"
    action = "review_warning_only_schema_metadata"
    message = "Project template readiness reviewer action is available only from a warning item."
    source_schema = "featherdoc.project_template_delivery_readiness_report.v1"
    source_report_display = ".\output\project-template-delivery-readiness\summary.json"
    source_json_display = ".\output\project-template-delivery-readiness\summary.json"
    requires_reviewer_action = $true
    reviewer_action_summary = "review_warning_only_schema_metadata"
    reviewer_action_reason = "latest_review_state=warning_only; issue_keys=(none)"
    reviewer_actions = @("review_warning_only_schema_metadata")
}
($warningOnlySummary | ConvertTo-Json -Depth 30) | Set-Content -LiteralPath $warningOnlySummaryPath -Encoding UTF8
& (Join-Path $resolvedRepoRoot "scripts\write_release_body_zh.ps1") `
    -SummaryJson $warningOnlySummaryPath `
    -OutputPath $warningOnlyBodyPath `
    -ShortOutputPath $warningOnlyShortPath | Out-Null
Assert-LineContainsAll -Path $warningOnlyBodyPath -ExpectedFragments @(
    'Project template readiness:',
    'requires_reviewer_action=True',
    'reviewer_action_summary=review_warning_only_schema_metadata',
    'reviewer_action_reason=latest_review_state=warning_only; issue_keys=(none)',
    'reviewer_actions=review_warning_only_schema_metadata',
    'source_report_display=.\output\project-template-delivery-readiness\summary.json',
    'source_json_display=.\output\project-template-delivery-readiness\summary.json'
) -Label 'warning-only release_body.zh-CN.md'
Assert-LineContainsAll -Path $warningOnlyShortPath -ExpectedFragments @(
    'project-template readiness governance contract',
    'requires_reviewer_action=True',
    'reviewer_action_summary=review_warning_only_schema_metadata',
    'reviewer_action_reason=latest_review_state=warning_only; issue_keys=(none)',
    'reviewer_actions=review_warning_only_schema_metadata',
    'source_report_display=.\output\project-template-delivery-readiness\summary.json',
    'source_json_display=.\output\project-template-delivery-readiness\summary.json'
) -Label 'warning-only release_summary.zh-CN.md'

Assert-Contains -Path $shortPath -ExpectedText 'project-template onboarding governance contract' -Label 'release_summary.zh-CN.md'
Assert-LineContainsAll -Path $shortPath -ExpectedFragments @(
    'project-template onboarding governance contract',
    'status=pending_review',
    'release_ready=False',
    'schema_approval_status_summary=pending_review',
    'next_action=action=review_schema_update_candidate',
    'next_action_group_count=1',
    'requires_reviewer_action=True',
    'reviewer_action_summary=review_schema_update_candidate',
    'reviewer_action_reason=latest_review_state=pending; issue_keys=(none)',
    'reviewer_actions=review_schema_update_candidate',
    'source_report_display=.\output\project-template-onboarding-governance\summary.json',
    'source_json_display=.\output\project-template-onboarding-governance\summary.json'
) -Label 'release_summary.zh-CN.md'
Assert-LineContainsAll -Path $shortPath -ExpectedFragments @(
    'project-template readiness checklist handoff evidence',
    'Project-template readiness checklist handoff evidence',
    'project_template_readiness_checklist_entrypoints_source_reports=1',
    'required_entrypoint_count=3',
    'entrypoint_paths=',
    'source_schema=featherdoc.release_candidate_summary',
    'source_report=.\output\release-candidate-checks\summary.json'
) -Label 'release_summary.zh-CN.md'
Assert-LineContainsAll -Path $shortPath -ExpectedFragments @(
    'project-template readiness checklist packaged audit evidence',
    'Project-template readiness checklist packaged audit evidence',
    'release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1',
    'audited_entrypoints=start_here, artifact_guide, reviewer_checklist',
    'compact_evidence_source_schema=featherdoc.release_candidate_summary',
    'source_schema=featherdoc.release_candidate_summary',
    'source_report=.\output\release-blocker-rollup\summary.json'
) -Label 'release_summary.zh-CN.md'
Assert-LineContainsAll -Path $shortPath -ExpectedFragments @(
    'project-template readiness governance contract',
    'status=blocked',
    'release_ready=False',
    'latest_schema_approval_gate_status=pending_review',
    'schema_approval_status_summary=pending_review',
    'requires_reviewer_action=True',
    'reviewer_action_summary=review_schema_update_candidate',
    'reviewer_action_reason=latest_review_state=pending; issue_keys=(none)',
    'reviewer_actions=review_schema_update_candidate',
    'source_report_display=.\output\project-template-delivery-readiness\summary.json',
    'source_json_display=.\output\project-template-delivery-readiness\summary.json'
) -Label 'release_summary.zh-CN.md'
Assert-LineContainsAll -Path $shortPath -ExpectedFragments @(
    'project-template onboarding governance contract',
    'status=pending_review',
    'release_ready=False',
    'schema_approval_status_summary=pending_review',
    'requires_reviewer_action=True',
    'reviewer_action_summary=review_schema_update_candidate',
    'reviewer_action_reason=latest_review_state=pending; issue_keys=(none)',
    'reviewer_actions=review_schema_update_candidate',
    'source_report_display=.\output\project-template-onboarding-governance\summary.json',
    'source_json_display=.\output\project-template-onboarding-governance\summary.json'
) -Label 'release_summary.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText $installDir -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText $resolvedWorkingDir -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText 'draft' -Label 'release_body.zh-CN.md'
$draftWord = -join (0x8349, 0x7a3f | ForEach-Object { [char]$_ })
$releaseDraft = -join (0x53d1, 0x5e03, 0x8bf4, 0x660e, 0x8349, 0x7a3f | ForEach-Object { [char]$_ })
$fillBeforeRelease = -join (0x8bf7, 0x5728, 0x53d1, 0x5e03, 0x524d, 0x8865, 0x9f50 | ForEach-Object { [char]$_ })
$generatedByScript = (-join (0x8fd9, 0x4efd, 0x6587, 0x4ef6, 0x7531 | ForEach-Object { [char]$_ })) +
    ([char]96) + 'write_release_body_zh.ps1' + ([char]96) +
    (-join (0x81ea, 0x52a8, 0x751f, 0x6210 | ForEach-Object { [char]$_ }))
Assert-NotContains -Path $bodyPath -UnexpectedText $draftWord -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText $releaseDraft -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText $fillBeforeRelease -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText $generatedByScript -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $shortPath -UnexpectedText 'draft' -Label 'release_summary.zh-CN.md'
Assert-NotContains -Path $shortPath -UnexpectedText $draftWord -Label 'release_summary.zh-CN.md'
Assert-NotContains -Path $shortPath -UnexpectedText $generatedByScript -Label 'release_summary.zh-CN.md'
$guidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$checklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$startHerePath = Join-Path (Split-Path -Parent $reportDir) "START_HERE.md"
foreach ($document in @(
        [pscustomobject]@{ Path = $handoffPath; Label = "release_handoff.md" },
        [pscustomobject]@{ Path = $bodyPath; Label = "release_body.zh-CN.md" },
        [pscustomobject]@{ Path = $shortPath; Label = "release_summary.zh-CN.md" },
        [pscustomobject]@{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        [pscustomobject]@{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        [pscustomobject]@{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-FileHasNoBom -Path $document.Path -Label $document.Label
}
$releaseGovernanceReportIssueDocuments = @(
    [pscustomobject]@{ Path = $handoffPath; Label = "release_handoff.md" },
    [pscustomobject]@{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
    [pscustomobject]@{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
    [pscustomobject]@{ Path = $startHerePath; Label = "START_HERE.md" }
)
foreach ($document in $releaseGovernanceReportIssueDocuments) {
    Assert-Contains -Path $document.Path -ExpectedText 'Rollup Source Report Contracts' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'featherdoc.pdf_visual_release_gate_preflight_governance_report.v1: status=blocked ready=False' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'preflight_ready: False' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'full_visual_gate_status: not_run_by_preflight_governance' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'controlled_visual_smoke_status: pass' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'controlled_visual_smoke_json_display: .\output\pdf-controlled-visual-smoke-20260520\controlled-visual-smoke-check-latest.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'Source failures: 1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_failure_count: 1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'readiness_action_evidence_count: 2' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'readiness_action_evidence:' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'provide_pdf_dependency_input' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'pdf_dependency_inputs_ready' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'PDFio source header' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'enable_pdf_build_option' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'FEATHERDOC_BUILD_PDF_IMPORT' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'project_template_onboarding_governance_contract:' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'schema_approval_status_summary: pending_review' -Label $document.Label
    Assert-MarkdownListBlockContainsAll -Path $document.Path -Anchor 'featherdoc.project_template_delivery_readiness_report.v1: status=blocked' -ExpectedFragments @(
        'source_report_display: .\output\project-template-delivery-readiness\summary.json',
        'source_json_display: .\output\project-template-delivery-readiness\summary.json',
        'project_template_delivery_readiness_contract:',
        'source_schema: featherdoc.project_template_delivery_readiness_report.v1',
        'status: blocked',
        'release_ready: False',
        'schema_approval_status_summary: pending_review',
        'onboarding_governance_next_action_group_count: 1'
    ) -Label $document.Label
    Assert-MarkdownListBlockContainsAll -Path $document.Path -Anchor 'project_template_onboarding.schema_approval: action=review_schema_update_candidate' -ExpectedFragments @(
        'source_schema=featherdoc.project_template_onboarding_governance_report.v1',
        'source_report_display: .\output\project-template-delivery-readiness\summary.json',
        'source_json_display: .\output\project-template-onboarding-governance\summary.json',
        'readiness_status: failed',
        'readiness_release_ready: False',
        'requires_reviewer_action: true',
        'reviewer_action: review_schema_update_candidate',
        'reviewer_action_reason: latest_review_state=pending; issue_keys=(none)',
        'reviewer_actions: review_schema_update_candidate',
        'project_template_onboarding_governance_contract:',
        'status: pending_review',
        'release_ready: False',
        'schema_approval_status_summary: pending_review',
        'next_action:',
        'next_action_summary:',
        'next_action_group_count: 1'
    ) -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'catalog_document_keys: contract-template,invoice-template,report-template,long-doc-template' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'baseline_document_keys: contract-template,invoice-template,report-template,long-doc-template' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'matched_document_keys: contract-template,invoice-template,report-template,long-doc-template' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report: .\output\pdf-visual-release-gate-preflight-governance\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json_display: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'Rollup Source Report Issues' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report: .\output\table-layout-delivery-governance\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json: .\output\table-layout-delivery-governance\unreadable-summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'error: Unexpected token while reading table layout governance summary.' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'build: pwsh -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'Handoff Report Issues' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report: .\output\project-template-delivery-readiness\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json: .\output\project-template-delivery-readiness\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'project_template_delivery_readiness_contract:' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_schema: featherdoc.project_template_delivery_readiness_report.v1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'latest_schema_approval_gate_status: pending_review' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'schema_approval_status_summary: pending_review' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'onboarding_governance_next_action_group_count: 1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'template_count: 3' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'release_blocker_count: 1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report_display: .\output\project-template-delivery-readiness\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json_display: .\output\project-template-delivery-readiness\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'error: Failed to parse project template delivery readiness summary.' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'build: pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText '<local-path>' -Label $document.Label
    Assert-NotContains -Path $document.Path -UnexpectedText 'C:\repo\tmp\pdfio-src\pdfio.h' -Label $document.Label
}
Assert-Contains -Path $bodyPath -ExpectedText '<local-path>' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText 'C:\repo\tmp\pdfio-src\pdfio.h' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $guidePath -ExpectedText 'publish_github_release.ps1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'publish_github_release.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'publish_github_release.ps1' -Label 'START_HERE.md'
Assert-Contains -Path $guidePath -ExpectedText 'release-refresh.yml' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-refresh.yml' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'release-refresh.yml' -Label 'START_HERE.md'
Assert-Contains -Path $guidePath -ExpectedText 'release-publish.yml' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-publish.yml' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'release-publish.yml' -Label 'START_HERE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Release Refresh' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release Publish' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $guidePath -ExpectedText 'Run workflow' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Actions' -Label 'START_HERE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-refresh-output' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-publish-output' -Label 'REVIEWER_CHECKLIST.md'
$manifestSignoffRequiredFields = @(
    'status',
    'release_ready',
    'release_blocker_count',
    'warning_count',
    'schema_approval_status_summary',
    'onboarding_governance_next_action',
    'onboarding_governance_next_action_summary',
    'onboarding_governance_next_action_group_count',
    'next_action',
    'next_action_summary',
    'next_action_group_count',
    'requires_reviewer_action',
    'reviewer_action_summary',
    'reviewer_action_reason',
    'reviewer_actions',
    'source_report_display',
    'source_json_display'
)
$manifestEntryDocuments = @(
    [pscustomobject]@{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
    [pscustomobject]@{ Path = $startHerePath; Label = "START_HERE.md" }
)
foreach ($document in $manifestEntryDocuments) {
    Assert-Contains -Path $document.Path -ExpectedText 'release_assets_manifest.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'project_template_delivery_readiness_contract' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'project_template_onboarding_governance_contract' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'status' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'release_ready' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'release_blocker_count' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'warning_count' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'schema_approval_status_summary' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'onboarding_governance_next_action' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'onboarding_governance_next_action_summary' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'onboarding_governance_next_action_group_count' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'next_action' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'next_action_summary' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'next_action_group_count' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'requires_reviewer_action' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'reviewer_action_summary' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'reviewer_action_reason' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'reviewer_actions' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report_display' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json_display' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'before refreshing or publishing GitHub Release assets' -Label $document.Label
}
$manifestChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'Confirm the packaged release manifest preserves project-template governance contracts before publishing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'release_assets_manifest.json must include' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'project_template_delivery_readiness_contract' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'status' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'release_ready' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'release_blocker_count' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'warning_count' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'schema_approval_status_summary' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'onboarding_governance_next_action' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'onboarding_governance_next_action_summary' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'onboarding_governance_next_action_group_count' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'next_action' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'next_action_summary' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'next_action_group_count' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'requires_reviewer_action' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'reviewer_action_summary' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'reviewer_action_reason' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'reviewer_actions' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'source_report_display' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $manifestChecklistPath -ExpectedText 'source_json_display' -Label 'REVIEWER_CHECKLIST.md'
$manifestSignoffEntryDocuments = @(
    [pscustomobject]@{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
    [pscustomobject]@{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
    [pscustomobject]@{ Path = $startHerePath; Label = "START_HERE.md" }
)
foreach ($document in $manifestSignoffEntryDocuments) {
    $manifestSignoffExpectedFragments = @(
        'release_assets_manifest.json',
        'project_template_delivery_readiness_contract',
        'project_template_onboarding_governance_contract'
    ) + $manifestSignoffRequiredFields
    Assert-LineContainsAll -Path $document.Path -ExpectedFragments $manifestSignoffExpectedFragments -Label $document.Label
}
$checklistHandoffEntryDocuments = @(
    [pscustomobject]@{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
    [pscustomobject]@{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
    [pscustomobject]@{ Path = $startHerePath; Label = "START_HERE.md" }
)
foreach ($document in $checklistHandoffEntryDocuments) {
    Assert-Contains -Path $document.Path -ExpectedText 'Project-template readiness checklist handoff evidence' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'project_template_readiness_checklist_entrypoints_source_reports=1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'docs/project_template_release_readiness_checklist_zh.rst' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'required_entrypoint_count=3' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'start_here, artifact_guide, reviewer_checklist' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'entrypoint_paths=' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'release_entry_project_template_readiness_checklist_trace' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_schema=featherdoc.release_candidate_summary' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report=.\output\release-candidate-checks\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'Project-template readiness checklist packaged audit evidence' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'assert_release_material_safety.ps1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'audited_entrypoints=start_here, artifact_guide, reviewer_checklist' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'compact_evidence_source_schema=featherdoc.release_candidate_summary' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_schema=featherdoc.release_candidate_summary' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report=.\output\release-blocker-rollup\summary.json' -Label $document.Label
    Assert-LineContainsAll -Path $document.Path -ExpectedFragments @(
        'Project-template readiness checklist handoff evidence',
        'project_template_readiness_checklist_entrypoints_source_reports=1',
        'docs/project_template_release_readiness_checklist_zh.rst',
        'required_entrypoint_count=3',
        'entrypoint_paths=',
        'start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md',
        'artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md',
        'reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md',
        'release_entry_project_template_readiness_checklist_trace',
        'source_schema=featherdoc.release_candidate_summary',
        'source_report=.\output\release-candidate-checks\summary.json'
    ) -Label $document.Label
    Assert-LineContainsAll -Path $document.Path -ExpectedFragments @(
        'Project-template readiness checklist packaged audit evidence',
        'release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1',
        'audit_script=.\scripts\assert_release_material_safety.ps1',
        'audited_entrypoints=start_here, artifact_guide, reviewer_checklist',
        'compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports',
        'compact_evidence_source_schema=featherdoc.release_candidate_summary',
        'checklist_path=docs/project_template_release_readiness_checklist_zh.rst',
        'checklist_marker=release_entry_project_template_readiness_checklist_trace',
        'material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace',
        'source_schema=featherdoc.release_candidate_summary',
        'source_report=.\output\release-blocker-rollup\summary.json'
    ) -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'Word visual standard review metadata evidence' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'word_visual_standard_review_metadata_source_reports=1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'metadata_count=4' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'task_keys=smoke, fixed_grid, section_page_setup, page_number_fields' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'status_summary=reviewed=4' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'verdict_summary=pass=4' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'smoke:review_task_key=document' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'fixed_grid:review_task_key=fixed_grid' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'section_page_setup:review_task_key=section_page_setup' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'page_number_fields:review_task_key=page_number_fields' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'review_result.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'final_review.md' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_schema=featherdoc.release_candidate_summary' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report=.\output\release-candidate-checks\summary.json' -Label $document.Label
    Assert-NotContains -Path $document.Path -UnexpectedText 'review_note' -Label $document.Label
}
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm release governance handoff carries project-template readiness checklist entrypoint evidence' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm release governance handoff carries packaged project-template readiness checklist material-safety audit evidence' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm release governance handoff carries Word visual standard review metadata evidence' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm the Word visual standard review metadata evidence remains ready for publishing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Confirm release governance handoff carries packaged project-template readiness checklist material-safety audit evidence' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Confirm release governance handoff carries Word visual standard review metadata evidence' -Label 'START_HERE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project-template readiness checklist packaged audit signoff' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Word visual standard review metadata signoff' -Label 'ARTIFACT_GUIDE.md'
foreach ($document in @(
        [pscustomobject]@{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        [pscustomobject]@{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        [pscustomobject]@{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-Contains -Path $document.Path -ExpectedText 'docs/pdf_release_readiness_checklist_zh.rst' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'docs/project_template_release_readiness_checklist_zh.rst' -Label $document.Label
}
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm the fixed PDF release readiness checklist has been reviewed before publishing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm the fixed Project template release readiness checklist has been reviewed before publishing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $guidePath -ExpectedText 'Smoke verdict: pass' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Fixed-grid verdict: undetermined' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Section page setup verdict: pass' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Page number fields review task' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Template table CLI selector review task' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template schema approval history JSON' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'project_template_schema_approval_history.md' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard status: blocked' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard release ready: False' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard counts: 2 reports, 2 blockers, 1 warnings' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard business document types: invoice=1, policy=1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard corpus roles: planned-business-template=1, registered-business-template=1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard next action: review_schema_update_candidate (Project template onboarding schema approval is pending.)' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard next blocker: project_template_onboarding.schema_approval' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard next action groups: 2' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard action group: source=project_template_onboarding_governance action=review_schema_update_candidate blocker=project_template_onboarding.schema_approval entries=invoice-template' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard action group: source=project_template_delivery_readiness action=collect_project_template_onboarding_governance_evidence blocker=project_template_delivery_readiness.onboarding_governance entries=policy-template' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard action source: source=project_template_onboarding_governance action_group_count=1 source_json=.\output\project-template-onboarding-governance\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard action source: source=project_template_delivery_readiness action_group_count=1 source_json=.\output\project-template-delivery-readiness\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template workflow dashboard summary' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'project_template_workflow_dashboard.md' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Release blockers: 1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'document_skeleton.style_numbering_issues' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.document_skeleton_governance_rollup_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.content_control_data_binding_governance_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'repair_strategy: sync_bound_content_control' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText $contentControlCommandTemplateMarker -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.project_template_onboarding_governance_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.table_layout_delivery_governance_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_json_display: .\output\table-layout-delivery-governance\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_json_display: .\output\numbering-catalog-governance\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'matched_document_count=4' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'catalog_coverage_percent=100' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText "style_numbering_issues(count=4, penalty=20)" -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'ready_document_percent=100' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'table_position_automatic_count=0' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'table_position_review_count=0' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'pdf_floating_table_supported_geometry_percent=44' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'pdf_floating_table_tracked_geometry_count=9' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText "pdf_floating_table_support_coverage: 4/9 supported (44 percent); metadata_only=5" -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'pdf_floating_table_reviewer_focus: review metadata-only tblpPr fields before approving PDF-layout-sensitive release.' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'pdf_floating_table_metadata_only_fields: leftFromText, rightFromText, topFromText outside paragraph anchoring, tblOverlap' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'pdf_floating_table_review_required_fields: full Word-compatible floating table text wrapping, table overlap avoidance and collision resolution' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'unresolved_item_count=0' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText "floating_table_plans_pending(count=0, penalty=0)" -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.schema_patch_confidence_calibration_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_json_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Item schema-review-invalid' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open the project template schema approval history trend report' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard status: blocked' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard release ready: False' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard counts: 2 reports, 2 blockers, 1 warnings' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard business document types: invoice=1, policy=1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard corpus roles: planned-business-template=1, registered-business-template=1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard next action groups: 2' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard action group: source=project_template_onboarding_governance action=review_schema_update_candidate blocker=project_template_onboarding.schema_approval entries=invoice-template' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard action group: source=project_template_delivery_readiness action=collect_project_template_onboarding_governance_evidence blocker=project_template_delivery_readiness.onboarding_governance entries=policy-template' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard action source: source=project_template_onboarding_governance action_group_count=1 source_json=.\output\project-template-onboarding-governance\summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard action source: source=project_template_delivery_readiness action_group_count=1 source_json=.\output\project-template-delivery-readiness\summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Project template workflow dashboard report' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'project_template_workflow_dashboard.md' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Stop here until the project template workflow dashboard is release-ready; status `blocked`, release_ready `False`, blockers `2`, warnings `1`.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Resolve project template workflow dashboard next action before public release: review_schema_update_candidate (Project template onboarding schema approval is pending.)' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Review project template workflow dashboard action group: source=project_template_onboarding_governance action=review_schema_update_candidate blocker=project_template_onboarding.schema_approval entries=invoice-template' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Review project template workflow dashboard action group: source=project_template_delivery_readiness action=collect_project_template_onboarding_governance_evidence blocker=project_template_delivery_readiness.onboarding_governance entries=policy-template' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Use the project template workflow dashboard handoff command when the dashboard evidence must be rebuilt: pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_workflow_dashboard.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release blockers: 1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'document_skeleton.style_numbering_issues' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'document_skeleton.exemplar_catalog_missing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'repair_strategy: sync_bound_content_control' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText $contentControlCommandTemplateMarker -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'custom_xml_sync_evidence_missing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_schema=featherdoc.project_template_onboarding_governance_report.v1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_schema=featherdoc.table_layout_delivery_governance_report.v1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_json_display: .\output\numbering-catalog-governance\summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'matched_document_count=4' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'catalog_coverage_percent=100' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText "style_numbering_issues(count=4, penalty=20)" -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_json_display: .\output\table-layout-delivery-governance\summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'ready_document_percent=100' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'table_position_automatic_count=0' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'table_position_review_count=0' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'pdf_floating_table_supported_geometry_percent=44' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'pdf_floating_table_tracked_geometry_count=9' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText "pdf_floating_table_support_coverage: 4/9 supported (44 percent); metadata_only=5" -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'pdf_floating_table_reviewer_focus: review metadata-only tblpPr fields before approving PDF-layout-sensitive release.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'pdf_floating_table_metadata_only_fields: leftFromText, rightFromText, topFromText outside paragraph anchoring, tblOverlap' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'pdf_floating_table_review_required_fields: full Word-compatible floating table text wrapping, table overlap avoidance and collision resolution' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'unresolved_item_count=0' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText "floating_table_plans_pending(count=0, penalty=0)" -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'schema_patch_confidence_calibration.unscored_candidates' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_schema=featherdoc.schema_patch_confidence_calibration_report.v1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release Governance Handoff Details' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Handoff Action Items' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_json_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'pdf_visual_release_gate_preflight.build_outputs_missing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_schema=featherdoc.pdf_visual_release_gate_preflight_governance_report.v1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_report_display: .\output\pdf-visual-release-gate-preflight-governance\summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_json_display: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'prepare_pdf_visual_release_gate_build_outputs' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Readiness action evidence' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'provide_pdf_dependency_input/pdf_dependency_inputs_ready -> PDFio source header' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'enable_pdf_build_option/pdf_build_options_enabled -> FEATHERDOC_BUILD_PDF_IMPORT' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source JSON: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'cmake_cache_exists' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'CMakeCache.txt' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'pdf_build_options_enabled' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText '-DFEATHERDOC_BUILD_PDF=ON' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText '-DFEATHERDOC_BUILD_PDF_IMPORT=ON' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'missing CLI PDFs=2' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'missing visual baseline PDFs=42' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'missing CJK text-layer PDFs=43' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'memory guard blocked=false' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'memory guard skipped=false' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'free memory MB=1140' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'minimum free memory MB=2048' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'PDF build options: enabled=false' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'disabled=FEATHERDOC_BUILD_PDF,FEATHERDOC_BUILD_PDF_IMPORT' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'not release-ready evidence' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Only after preflight is ready and workstation resources allow it' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release note bundle' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Stop here until' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'fix_schema_patch_approval_result' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'sync_project_template_schema_approval.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Resolve release governance blocker' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release blocker rollup blockers' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open source report' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'while handling release governance blocker' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open source JSON' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Use reviewer action `review_schema_update_candidate` for release governance blocker `project_template_onboarding.schema_approval`.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Reviewer action reason for release governance blocker `project_template_onboarding.schema_approval`: latest_review_state=pending; issue_keys=(none)' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Reviewer action candidates for release governance blocker `project_template_onboarding.schema_approval`: review_schema_update_candidate' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Review ' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'repair_hint' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Rerun Custom XML sync or explicitly fill the bound content control before release.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open source report `.\output\content-control-data-binding-governance\summary.json` while handling release governance blocker `content_control_data_binding.bound_placeholder`.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open source JSON `.\output\content-control-data-binding\inspect-content-controls.json` while handling release governance blocker `content_control_data_binding.bound_placeholder`.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Use action `sync_or_fill_bound_content_control` for release governance blocker `content_control_data_binding.bound_placeholder`' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Review `command_template` for release governance blocker `content_control_data_binding.bound_placeholder`: featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Follow blocker action `sync_or_fill_bound_content_control` for release governance blocker `content_control_data_binding.bound_placeholder` from source_schema `featherdoc.content_control_data_binding_governance_report.v1`' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release governance handoff blockers' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Review release governance action item' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'build_dir_exists,cmake_cache_exists,ctest_manifest_exists,pdf_build_options_enabled' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release blocker rollup action items' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Run ' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'for release governance action item' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Use reviewer action `review_schema_update_candidate` for release governance action item `review_invoice_schema`.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Reviewer action reason for release governance action item `review_invoice_schema`: latest_review_state=pending; issue_keys=(none)' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'build_document_skeleton_governance_rollup_report.ps1 -InputRoot .\output\document-skeleton-governance' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open source report `.\output\content-control-data-binding-governance\summary.json` while handling release governance action item `review_duplicate_content_control_binding`.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open source JSON `.\output\content-control-data-binding\inspect-content-controls.json` while handling release governance action item `review_duplicate_content_control_binding`.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Use action `review_duplicate_content_control_binding` for release governance action item `review_duplicate_content_control_binding`' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Run `open_command` for release governance action item `review_duplicate_content_control_binding`: `pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1`' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Review `command_template` for release governance action item `review_duplicate_content_control_binding`: featherdoc_cli inspect-content-controls <input.docx> --json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release governance handoff action items' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'write_schema_patch_confidence_calibration_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Review release governance warning' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release blocker rollup warnings' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release governance handoff warnings' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Do not approve for public release when release governance blocker counts are non-zero in the final rollup, governance handoff, or nested handoff rollup.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Do not approve for public release when' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'project_template_schema_approval_history.md' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Superseded review tasks: 0' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm `superseded_review_tasks.json` reports zero stale task directories' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Smoke verdict: pass' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Fixed-grid verdict: undetermined' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm the Word visual smoke verdict is signed off as `pass` in the gate summary.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText ('Confirm the fixed-grid visual verdict is signed off as {0}undetermined{0} in the gate summary.' -f [char]96) -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Page number fields verdict: pending_manual_review' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open the Template table CLI selector review task if the release touches this curated visual bundle' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open the page number fields review task if the release touches page numbers' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Release blockers: 1' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'project_template_smoke.schema_approval' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'document_skeleton.style_numbering_issues' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'repair_strategy: sync_bound_content_control' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText $contentControlCommandTemplateMarker -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\numbering-catalog-governance\summary.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'matched_document_count=4' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'catalog_coverage_percent=100' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText "style_numbering_issues(count=4, penalty=20)" -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\table-layout-delivery-governance\summary.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'ready_document_percent=100' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'table_position_automatic_count=0' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'table_position_review_count=0' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'unresolved_item_count=0' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText "floating_table_plans_pending(count=0, penalty=0)" -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\document-skeleton-governance\contract\style-numbering-audit.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\content-control-data-binding\inspect-content-controls.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard status: blocked' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard release ready: False' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard counts: 2 reports, 2 blockers, 1 warnings' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard business document types: invoice=1, policy=1' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard corpus roles: planned-business-template=1, registered-business-template=1' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard next action: review_schema_update_candidate (Project template onboarding schema approval is pending.)' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard next blocker: project_template_onboarding.schema_approval' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard next action groups: 2' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard action group: source=project_template_onboarding_governance action=review_schema_update_candidate blocker=project_template_onboarding.schema_approval entries=invoice-template' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard action group: source=project_template_delivery_readiness action=collect_project_template_onboarding_governance_evidence blocker=project_template_delivery_readiness.onboarding_governance entries=policy-template' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard action source: source=project_template_onboarding_governance action_group_count=1 source_json=.\output\project-template-onboarding-governance\summary.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard action source: source=project_template_delivery_readiness action_group_count=1 source_json=.\output\project-template-delivery-readiness\summary.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Project template workflow dashboard report' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'project_template_workflow_dashboard.md' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Confirm the Project template workflow dashboard reports `release_ready = True` and zero blockers before publishing.' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Visual verdict: pending_manual_review' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Smoke verdict: pass' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Fixed-grid verdict: undetermined' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Section page setup review task' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Template table CLI selector review task' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'open_latest_word_review_task.ps1 -SourceKind template-table-cli-selector-visual-regression-bundle -PrintPrompt' -Label 'START_HERE.md'

Assert-ContentControlGovernanceTrace -Path $handoffPath -Label 'release_handoff.md'
Assert-ContentControlGovernanceTrace -Path $guidePath -Label 'ARTIFACT_GUIDE.md'
Assert-ContentControlGovernanceTrace -Path $checklistPath -Label 'REVIEWER_CHECKLIST.md'
Assert-ContentControlGovernanceTrace -Path $startHerePath -Label 'START_HERE.md'
