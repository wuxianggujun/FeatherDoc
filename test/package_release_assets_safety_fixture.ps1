. (Join-Path $PSScriptRoot "pdf_import_diagnostics_contract_field_helpers.ps1")

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$workingDirParent = Split-Path -Parent $resolvedWorkingDir
if (-not [string]::IsNullOrWhiteSpace($workingDirParent)) {
    New-Item -ItemType Directory -Path $workingDirParent -Force | Out-Null
}
if (Test-Path -LiteralPath $resolvedWorkingDir) {
    Remove-Item -LiteralPath $resolvedWorkingDir -Recurse -Force
}
$null = New-Item -ItemType Directory -Path $resolvedWorkingDir -Force
$relativeWorkingDir = $resolvedWorkingDir.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
$installPrefix = Join-Path $resolvedWorkingDir "build-msvc-install"
$summaryOutputDir = Join-Path $resolvedWorkingDir "output\release-candidate-checks"
$reportDir = Join-Path $summaryOutputDir "report"
$gateOutputDir = Join-Path $resolvedWorkingDir "output\word-visual-release-gate"
$gateReportDir = Join-Path $gateOutputDir "report"
$smokeEvidenceDir = Join-Path $gateOutputDir "smoke\evidence"
$fixedGridAggregateDir = Join-Path $gateOutputDir "fixed-grid\aggregate-evidence"
$sectionPageSetupAggregateDir = Join-Path $gateOutputDir "section-page-setup\aggregate-evidence"
$smokeTaskDir = Join-Path $gateOutputDir "review-tasks\document"
$fixedGridTaskDir = Join-Path $gateOutputDir "review-tasks\fixed-grid"
$sectionPageSetupTaskDir = Join-Path $gateOutputDir "review-tasks\section-page-setup"
$pageNumberFieldsTaskDir = Join-Path $gateOutputDir "review-tasks\page-number-fields"
$pdfGateOutputDir = Join-Path $resolvedWorkingDir "output\pdf-visual-release-gate"
$pdfGateReportDir = Join-Path $pdfGateOutputDir "report"
$pdfGateCopySearchDir = Join-Path $pdfGateReportDir "cjk-copy-search"
$pdfGateUnicodeReportDir = Join-Path $pdfGateOutputDir "unicode-font\report"
$outputRoot = Join-Path $resolvedWorkingDir "release-assets"

New-Item -ItemType Directory -Path (Join-Path $installPrefix "share\FeatherDoc") -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $smokeEvidenceDir -Force | Out-Null
New-Item -ItemType Directory -Path $fixedGridAggregateDir -Force | Out-Null
New-Item -ItemType Directory -Path $sectionPageSetupAggregateDir -Force | Out-Null
foreach ($reviewTaskDir in @($smokeTaskDir, $fixedGridTaskDir, $sectionPageSetupTaskDir, $pageNumberFieldsTaskDir)) {
    New-Item -ItemType Directory -Path (Join-Path $reviewTaskDir "report") -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $reviewTaskDir "report\review_result.json") -Encoding UTF8 -Value '{"review_verdict":"pass"}'
    Set-Content -LiteralPath (Join-Path $reviewTaskDir "report\final_review.md") -Encoding UTF8 -Value "# Final Review"
}
New-Item -ItemType Directory -Path $pdfGateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $pdfGateCopySearchDir -Force | Out-Null
New-Item -ItemType Directory -Path $pdfGateUnicodeReportDir -Force | Out-Null

$startHerePath = Join-Path $summaryOutputDir "START_HERE.md"
$releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
$releaseGovernanceHandoffPath = Join-Path $reportDir "release_governance_handoff.md"
$releaseBodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$releaseSummaryPath = Join-Path $reportDir "release_summary.zh-CN.md"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$contentControlSummaryPath = Join-Path $reportDir "content_control_data_binding_governance_summary.json"
$projectTemplateDeliveryReadinessSummaryPath = Join-Path $reportDir "project_template_delivery_readiness_summary.json"
$projectTemplateOnboardingGovernanceSummaryPath = Join-Path $reportDir "project_template_onboarding_governance_summary.json"
$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$pdfGateSummaryPath = Join-Path $pdfGateReportDir "summary.json"
$pdfGateAggregateContactSheetPath = Join-Path $pdfGateReportDir "aggregate-contact-sheet.png"
$pdfGateSummaryPathDisplay = ".\output\pdf-visual-release-gate\report\summary.json"
$pdfGateAggregateContactSheetPathDisplay = ".\output\pdf-visual-release-gate\report\aggregate-contact-sheet.png"
$pdfGateCliExportLogPath = Join-Path $pdfGateReportDir "pdf-cli-export-test.log"
$pdfGateRegressionLogPath = Join-Path $pdfGateReportDir "pdf-regression-test.log"
$pdfGateUnicodeLogPath = Join-Path $pdfGateReportDir "unicode-font.log"
$pdfGateUnicodeContactSheetPath = Join-Path $pdfGateUnicodeReportDir "full-contact-sheet.png"
$pdfBoundedCtestSubsets = @(
    "smoke-import",
    "contract-static",
    "cjk-flow-static",
    "regression-basic-text",
    "regression-styled-document",
    "regression-business-samples",
    "regression-table-layout"
)
$pdfBoundedCtestSummaryJsonDisplay = @(
    ".\build\pdf-ctest-bounded-subset-current\summary.json",
    ".\build\pdf-ctest-bounded-contract-static-current\summary.json",
    ".\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json",
    ".\build\pdf-ctest-bounded-regression-basic-text-current\summary.json",
    ".\build\pdf-ctest-bounded-regression-styled-document-current\summary.json",
    ".\build\pdf-ctest-bounded-regression-business-samples-current\summary.json",
    ".\build\pdf-ctest-bounded-regression-table-layout-current\summary.json"
)
$pdfBoundedCtestSubsetsText = $pdfBoundedCtestSubsets -join ', '
$pdfBoundedCtestSummaryJsonDisplayText = $pdfBoundedCtestSummaryJsonDisplay -join ', '
$pdfImportDiagnosticsContractTests = @(
    "pdf_cli_import",
    "pdf_import_table_heuristic"
)
$pdfImportDiagnosticsContractFields = @(Get-PdfImportDiagnosticsContractFields)
$pdfImportNegativeBoundaryContractCases = @(
    "short_label_prose_remains_paragraphs",
    "invoice_summary_form_remains_paragraphs"
)
$summaryPath = Join-Path $reportDir "summary.json"
$installedReadmePath = Join-Path $installPrefix "share\FeatherDoc\README.md"
$installedChangelogPath = Join-Path $installPrefix "share\FeatherDoc\CHANGELOG.md"
$projectTemplateChecklistPackagedAuditEvidenceLine = "Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1, status=passed, audit_script=.\scripts\assert_release_material_safety.ps1, audited_entrypoint_count=3, audited_entrypoints=start_here, artifact_guide, reviewer_checklist, compact_evidence_label=Project-template readiness checklist handoff evidence, compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports, compact_evidence_source_schema=featherdoc.release_candidate_summary, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, checklist_marker=release_entry_project_template_readiness_checklist_trace, material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-blocker-rollup\summary.json"

Set-Content -LiteralPath $startHerePath -Encoding UTF8 -Value @"
# START_HERE

- Evidence root: $resolvedRepoRoot\output\release-candidate-checks\report
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder action=sync_or_fill_bound_content_control source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json repair_action_classes=release_blocking,auto_repair_candidate,manual_confirmation_required
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
- $projectTemplateChecklistPackagedAuditEvidenceLine
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
- PDF visual gate summary: $pdfGateSummaryPathDisplay
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 2
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 3
- PDF visual gate aggregate contact sheet: $pdfGateAggregateContactSheetPathDisplay
- PDF release readiness checklist: docs/pdf_release_readiness_checklist_zh.rst
"@

Set-Content -LiteralPath $releaseHandoffPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4

- Handoff path: $resolvedRepoRoot\output\release-candidate-checks\report\release_handoff.md
- Governance metrics: numbering_catalog_governance.real_corpus_confidence=low, table_layout_delivery_governance.delivery_quality=release_ready
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

Set-Content -LiteralPath $releaseGovernanceHandoffPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
- ``project_template_onboarding.schema_approval``: action=``review_schema_update_candidate`` source_schema=``featherdoc.project_template_onboarding_governance_report.v1``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``
  - project_template_onboarding_governance_contract:
    - source_schema: ``featherdoc.project_template_onboarding_governance_report.v1``
    - status: ``ready``
    - release_ready: ``True``
    - schema_approval_status_summary: ``approved``
    - source_report_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``
    - source_json_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``

## Governance Metrics

- numbering_catalog_governance.real_corpus_confidence: report=numbering_catalog_governance metric=real_corpus_confidence level=low score=56 source_schema=featherdoc.numbering_catalog_governance_report.v1
- table_layout_delivery_governance.delivery_quality: report=table_layout_delivery_governance metric=delivery_quality level=release_ready score=100 source_schema=featherdoc.table_layout_delivery_governance_report.v1

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``$pdfGateSummaryPathDisplay``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``$pdfGateAggregateContactSheetPathDisplay``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``2``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``3``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``$pdfBoundedCtestSubsetsText``
    - pdf_bounded_ctest_summary_json_display: ``$pdfBoundedCtestSummaryJsonDisplayText``
- Word visual standard review metadata source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - word_visual_standard_review_metadata_count: ``4``
    - word_visual_standard_review_task_keys: ``smoke, fixed_grid, section_page_setup, page_number_fields``
    - word_visual_standard_review_status_summary: ``reviewed=4``
    - word_visual_standard_review_verdict_summary: ``pass=4``
    - ``smoke``: review_task_key=``document`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``
      - label: ``Word visual smoke``
      - reviewed_at: ``2026-04-12T12:10:00``
      - review_result_path: ``.\output\word-visual-release-gate\review-tasks\document\report\review_result.json``
      - final_review_path: ``.\output\word-visual-release-gate\review-tasks\document\report\final_review.md``
    - ``fixed_grid``: review_task_key=``fixed_grid`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``
    - ``section_page_setup``: review_task_key=``section_page_setup`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``
    - ``page_number_fields``: review_task_key=``page_number_fields`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``
"@

Set-Content -LiteralPath $releaseBodyPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷璇存槑

- 璇佹嵁鐩綍锛?resolvedRepoRoot\output\word-visual-release-gate\report
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷鎽樿

- 鎽樿璺緞锛?resolvedRepoRoot\output\release-candidate-checks\report\release_summary.zh-CN.md
- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

Set-Content -LiteralPath $finalReviewPath -Encoding UTF8 -Value @"
# Release Candidate Checks

- Final review root: $resolvedRepoRoot\output\release-candidate-checks\report

## Release governance handoff details

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
- PDF visual gate counts: 3 visual baselines, 2 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: $pdfGateSummaryPathDisplay
- PDF visual gate contact sheet: $pdfGateAggregateContactSheetPathDisplay
"@

Set-Content -LiteralPath $artifactGuidePath -Encoding UTF8 -Value @"
# Artifact Guide

- Report root: $resolvedRepoRoot\output\release-candidate-checks\report
- Governance metric: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Governance metric: table_layout_delivery_governance.delivery_quality release_ready 100 source_schema=featherdoc.table_layout_delivery_governance_report.v1 table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding governance: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
- $projectTemplateChecklistPackagedAuditEvidenceLine
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder action=sync_or_fill_bound_content_control source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json repair_action_classes=release_blocking,auto_repair_candidate,manual_confirmation_required
- PDF visual gate summary: $pdfGateSummaryPathDisplay
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 2
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 3
- PDF visual gate aggregate contact sheet: $pdfGateAggregateContactSheetPathDisplay
- PDF release readiness checklist: docs/pdf_release_readiness_checklist_zh.rst
"@

Set-Content -LiteralPath $reviewerChecklistPath -Encoding UTF8 -Value @"
# Reviewer Checklist

- Review root: $resolvedRepoRoot\output\word-visual-release-gate\report
- Confirm governance_metrics before publishing release assets.
- Confirm numbering_catalog_governance.real_corpus_confidence catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20) before publishing release assets.
- Confirm content-control provenance input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets before release.
- Check content_control_data_binding.bound_placeholder action=sync_or_fill_bound_content_control source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json repair_action_classes=release_blocking,auto_repair_candidate,manual_confirmation_required before release.
- Confirm project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json before release.
- Check project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json before release.
- Confirm Project template release readiness checklist docs/project_template_release_readiness_checklist_zh.rst before release.
- Confirm release governance handoff carries project-template readiness checklist entrypoint evidence: Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json.
- Confirm release governance handoff carries project-template readiness checklist packaged audit evidence: $projectTemplateChecklistPackagedAuditEvidenceLine.
- Confirm table_layout_delivery_governance.delivery_quality table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0) before release.
- Confirm PDF visual gate summary $pdfGateSummaryPathDisplay with 2 CJK copy/search samples and 3 visual baselines before release.
- Confirm PDF visual gate aggregate contact sheet $pdfGateAggregateContactSheetPathDisplay before release.
- Confirm PDF release readiness checklist docs/pdf_release_readiness_checklist_zh.rst before release.
"@

Set-Content -LiteralPath $gateFinalReviewPath -Encoding UTF8 -Value @"
# Gate Final Review

- Local gate report: $resolvedRepoRoot\output\word-visual-release-gate\report
"@

Set-Content -LiteralPath $installedReadmePath -Encoding UTF8 -Value @'
# README

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare_word_review_task.ps1 `
    -DocxPath C:\path\to\target.docx `
    -Mode review-only
```
'@

Set-Content -LiteralPath $installedChangelogPath -Encoding UTF8 -Value @'
# Changelog

- Removed remaining public-facing "draft" wording from release docs.
- The release-note drafting helper still keeps the old draft boilerplate.
- `-Publish` flips `draft=false` after final signoff.
- Normalize `C:\Users\someone\workspace` before public release.
'@

Set-Content -LiteralPath (Join-Path $smokeEvidenceDir "README.md") -Encoding UTF8 -Value @"
# Smoke Evidence

- Local smoke evidence: $resolvedRepoRoot\output\word-visual-release-gate\smoke\evidence
"@

Set-Content -LiteralPath (Join-Path $fixedGridAggregateDir "README.md") -Encoding UTF8 -Value @"
# Fixed Grid Evidence

- Local aggregate evidence: $resolvedRepoRoot\output\word-visual-release-gate\fixed-grid\aggregate-evidence
"@

Set-Content -LiteralPath (Join-Path $sectionPageSetupAggregateDir "README.md") -Encoding UTF8 -Value @"
# Section Page Setup Evidence

- Local aggregate evidence: $resolvedRepoRoot\output\word-visual-release-gate\section-page-setup\aggregate-evidence
"@

$gateSummary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    gate_output_dir = $gateOutputDir
    report_dir = $gateReportDir
    execution_status = "pass"
    visual_verdict = "pass"
    smoke = [ordered]@{
        evidence_dir = $smokeEvidenceDir
        review_verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:10:00"
        review_method = "operator_supplied"
        task = [ordered]@{
            task_dir = $smokeTaskDir
        }
    }
    fixed_grid = [ordered]@{
        aggregate_evidence_dir = $fixedGridAggregateDir
        review_verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:20:00"
        review_method = "operator_supplied"
        task = [ordered]@{
            task_dir = $fixedGridTaskDir
        }
    }
    section_page_setup = [ordered]@{
        review_verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:30:00"
        review_method = "operator_supplied"
        task = [ordered]@{
            task_dir = $sectionPageSetupTaskDir
        }
    }
    page_number_fields = [ordered]@{
        review_verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:40:00"
        review_method = "operator_supplied"
        task = [ordered]@{
            task_dir = $pageNumberFieldsTaskDir
        }
    }
}
($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

[System.IO.File]::WriteAllBytes($pdfGateAggregateContactSheetPath, [byte[]](0x89, 0x50, 0x4E, 0x47))
Set-Content -LiteralPath $pdfGateCliExportLogPath -Encoding UTF8 -Value "PDF CLI export passed from $resolvedRepoRoot"
Set-Content -LiteralPath $pdfGateRegressionLogPath -Encoding UTF8 -Value "PDF regression passed from $resolvedRepoRoot"
Set-Content -LiteralPath $pdfGateUnicodeLogPath -Encoding UTF8 -Value "Unicode font smoke passed from $resolvedRepoRoot"
[System.IO.File]::WriteAllBytes($pdfGateUnicodeContactSheetPath, [byte[]](0x89, 0x50, 0x4E, 0x47))

$pdfVisualGateSummary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    verdict = "pass"
    repo_root = $resolvedRepoRoot
    output_dir = $pdfGateOutputDir
    aggregate_contact_sheet = $pdfGateAggregateContactSheetPath
    cjk_manifest_count = 43
    visual_baseline_manifest_count = 42
    logs = [ordered]@{
        pdf_cli_export = $pdfGateCliExportLogPath
        pdf_regression = $pdfGateRegressionLogPath
        cjk_copy_search = $pdfGateCopySearchDir
        unicode_font = $pdfGateUnicodeLogPath
    }
    cjk_copy_search = @(
        [ordered]@{
            sample_id = "cjk-text"
            pdf_path = Join-Path $resolvedWorkingDir "build\cjk-text.pdf"
            summary_path = Join-Path $pdfGateCopySearchDir "cjk-text-summary.json"
            text_output_path = Join-Path $pdfGateCopySearchDir "cjk-text.txt"
            expected_text = @("中文文本路径回归样本")
            matched_text = @("中文文本路径回归样本")
            missing_text = @()
            page_count = 1
        },
        [ordered]@{
            sample_id = "contract-cjk-style"
            pdf_path = Join-Path $resolvedWorkingDir "build\contract-cjk-style.pdf"
            summary_path = Join-Path $pdfGateCopySearchDir "contract-cjk-style-summary.json"
            text_output_path = Join-Path $pdfGateCopySearchDir "contract-cjk-style.txt"
            expected_text = @("SERVICE AGREEMENT", "中文合同样本")
            matched_text = @("SERVICE AGREEMENT", "中文合同样本")
            missing_text = @()
            page_count = 1
        }
    )
    baselines = @(
        [ordered]@{ sample_id = "pdf-basic"; baseline_pdf = Join-Path $resolvedWorkingDir "baseline\pdf-basic.pdf" },
        [ordered]@{ sample_id = "pdf-cjk"; baseline_pdf = Join-Path $resolvedWorkingDir "baseline\pdf-cjk.pdf" },
        [ordered]@{ sample_id = "pdf-table"; baseline_pdf = Join-Path $resolvedWorkingDir "baseline\pdf-table.pdf" }
    )
}
($pdfVisualGateSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $pdfGateSummaryPath -Encoding UTF8

$contentControlSummary = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    status = "blocked"
    release_ready = $false
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "error"
            status = "placeholder_visible"
            action = "sync_or_fill_bound_content_control"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
            repair_action_classes = @("release_blocking", "auto_repair_candidate", "manual_confirmation_required")
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            input_docx = "samples/invoice.docx"
            input_docx_display = ".\samples\invoice.docx"
            template_name = "invoice-template"
            schema_target = "invoice"
            target_mode = "resolved-section-targets"
        }
    )
}
($contentControlSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $contentControlSummaryPath -Encoding UTF8

$projectTemplateDeliveryReadinessSummary = [ordered]@{
    schema = "featherdoc.project_template_delivery_readiness_report.v1"
    status = "ready"
    release_ready = $true
    latest_schema_approval_gate_status = "passed"
    schema_approval_status_summary = @(
        [ordered]@{
            status = "approved"
            count = 4
        }
    )
    schema_history_blocked_run_count = 0
    schema_history_pending_run_count = 0
    schema_history_passed_run_count = 3
    template_count = 4
    ready_template_count = 4
    blocked_template_count = 0
    release_blocker_count = 0
    action_item_count = 0
    warning_count = 0
    onboarding_governance_next_action = [ordered]@{
        action = "publish_project_template"
        status = "ready"
        blocker_id = ""
        reason = "Project template delivery readiness is release-ready."
    }
    onboarding_governance_next_action_summary = @(
        [ordered]@{
            action = "publish_project_template"
            status = "ready"
            blocker_id = ""
            reason = "Project template delivery readiness is release-ready."
        }
    )
    onboarding_governance_next_action_group_count = 1
}
($projectTemplateDeliveryReadinessSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $projectTemplateDeliveryReadinessSummaryPath -Encoding UTF8

$projectTemplateOnboardingGovernanceSummary = [ordered]@{
    schema = "featherdoc.project_template_onboarding_governance_report.v1"
    status = "ready"
    release_ready = $true
    source_file_count = 3
    source_failure_count = 0
    entry_count = 4
    schema_approval_status_summary = @(
        [ordered]@{
            status = "approved"
            count = 3
        },
        [ordered]@{
            status = "not_required"
            count = 1
        }
    )
    blocked_entry_count = 0
    pending_review_entry_count = 0
    not_evaluated_entry_count = 0
    approved_entry_count = 3
    not_required_entry_count = 1
    release_blocker_count = 0
    action_item_count = 0
    manual_review_recommendation_count = 1
    next_action = [ordered]@{
        action = "publish_project_template"
        status = "ready"
        blocker_id = ""
        reason = "Project template onboarding governance is release-ready."
    }
    next_action_summary = @(
        [ordered]@{
            action = "publish_project_template"
            status = "ready"
            blocker_id = ""
            reason = "Project template onboarding governance is release-ready."
        }
    )
    next_action_group_count = 1
}
($projectTemplateOnboardingGovernanceSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $projectTemplateOnboardingGovernanceSummaryPath -Encoding UTF8

$summary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    build_dir = (Join-Path $resolvedWorkingDir "build-msvc-nmake")
    install_dir = $installPrefix
    consumer_build_dir = (Join-Path $resolvedWorkingDir "build-msvc-install-consumer")
    gate_output_dir = $gateOutputDir
    task_output_root = (Join-Path $resolvedWorkingDir "output\word-visual-smoke\tasks-release-gate")
    release_version = "1.6.4"
    execution_status = "pass"
    visual_verdict = "pass"
    governance_metric_count = 3
    governance_metrics = @(
        [ordered]@{
            id = "style_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = "style_catalog_governance"
            source_schema = "featherdoc.style_catalog_governance_report.v1"
            score = 12
            level = "experimental"
            details = [ordered]@{
                score = 12
                level = "experimental"
                document_count = 99
                catalog_exemplar_count = 1
                baseline_entry_count = 1
                penalty_summary = @(
                    [ordered]@{ factor = "style_catalog_only"; count = 99; penalty = 88 }
                )
            }
        },
        [ordered]@{
            id = "numbering_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            source_report = ".\output\numbering-catalog-governance\summary.json"
            source_report_display = ".\output\numbering-catalog-governance\summary.json"
            source_json = ".\output\numbering-catalog-governance\summary.json"
            source_json_display = ".\output\numbering-catalog-governance\summary.json"
            score = 56
            level = "low"
            details = [ordered]@{
                score = 56
                level = "low"
                document_count = 2
                catalog_exemplar_count = 2
                baseline_entry_count = 2
                catalog_coverage_percent = 100
                baseline_coverage_percent = 100
                coverage_score = 100
                matched_document_count = 2
                unmatched_catalog_document_count = 0
                unmatched_baseline_document_count = 0
                alignment_gap_count = 0
                catalog_document_keys = @("contract.docx", "invoice.docx")
                baseline_document_keys = @("contract.docx", "invoice.docx")
                matched_document_keys = @("contract.docx", "invoice.docx")
                penalty_summary = @(
                    [ordered]@{ factor = "style_numbering_issues"; count = 4; penalty = 20 },
                    [ordered]@{ factor = "catalog_drift_or_dirty_baseline"; count = 2; penalty = 20 },
                    [ordered]@{ factor = "command_failures"; count = 1; penalty = 20 }
                )
            }
        },
        [ordered]@{
            id = "table_layout_delivery_governance.delivery_quality"
            metric = "delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
            source_report = ".\output\table-layout-delivery-governance\summary.json"
            source_report_display = ".\output\table-layout-delivery-governance\summary.json"
            source_json = ".\output\table-layout-delivery-governance\summary.json"
            source_json_display = ".\output\table-layout-delivery-governance\summary.json"
            score = 100
            level = "release_ready"
            details = [ordered]@{
                score = 100
                level = "release_ready"
                document_count = 3
                ready_document_count = 3
                ready_document_percent = 100
                needs_review_document_count = 0
                failed_document_count = 0
                table_style_issue_count = 0
                automatic_tblLook_fix_count = 0
                manual_table_style_fix_count = 0
                table_position_automatic_count = 0
                table_position_review_count = 0
                command_failure_count = 0
                unresolved_item_count = 0
                pdf_floating_table_supported_geometry_count = 4
                pdf_floating_table_metadata_only_count = 5
                pdf_floating_table_tracked_geometry_count = 9
                pdf_floating_table_supported_geometry_percent = 44
                metadata_only_fields = @("leftFromText", "rightFromText", "topFromText outside paragraph anchoring", "tblOverlap")
                review_required_fields = @("full Word-compatible floating table text wrapping", "table overlap avoidance and collision resolution")
                penalty_summary = @(
                    [ordered]@{ factor = "needs_review_documents"; count = 0; penalty = 0 },
                    [ordered]@{ factor = "floating_table_plans_pending"; count = 0; penalty = 0 }
                )
            }
        }
    )
    content_control_data_binding_governance = $contentControlSummaryPath
    project_template_delivery_readiness = $projectTemplateDeliveryReadinessSummaryPath
    project_template_onboarding_governance = $projectTemplateOnboardingGovernanceSummaryPath
    release_handoff = $releaseHandoffPath
    release_body_zh_cn = $releaseBodyPath
    release_summary_zh_cn = $releaseSummaryPath
    artifact_guide = $artifactGuidePath
    reviewer_checklist = $reviewerChecklistPath
    start_here = $startHerePath
    release_note_bundle = New-TestReleaseNoteBundleContract `
        -SummaryOutputDir $summaryOutputDir `
        -ReportDir $reportDir `
        -StartHerePath $startHerePath `
        -ArtifactGuidePath $artifactGuidePath `
        -ReviewerChecklistPath $reviewerChecklistPath `
        -ReleaseHandoffPath $releaseHandoffPath `
        -ReleaseBodyPath $releaseBodyPath `
        -ReleaseSummaryPath $releaseSummaryPath `
        -RepoRoot $resolvedRepoRoot
    manifest_signoff_entrypoints = [ordered]@{
        status = "declared"
        release_assets_manifest = (Join-Path $outputRoot "v1.6.4\release_assets_manifest.json")
        required_entrypoint_count = 3
        entrypoints = @(
            [ordered]@{
                id = "start_here"
                path = $startHerePath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $startHerePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "artifact_guide"
                path = $artifactGuidePath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $artifactGuidePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "reviewer_checklist"
                path = $reviewerChecklistPath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $reviewerChecklistPath -RepoRoot $resolvedRepoRoot
                required = $true
            }
        )
        required_contracts = @(
            "project_template_delivery_readiness_contract",
            "project_template_onboarding_governance_contract"
        )
        required_fields = @(
            "status",
            "release_ready",
            "release_blocker_count",
            "warning_count",
            "schema_approval_status_summary",
            "onboarding_governance_next_action",
            "onboarding_governance_next_action_summary",
            "onboarding_governance_next_action_group_count",
            "next_action",
            "next_action_summary",
            "next_action_group_count",
            "source_report_display",
            "source_json_display"
        )
        checklist_marker = "reviewer_manifest_scoped_project_template_trace"
    }
    project_template_readiness_checklist_entrypoints = [ordered]@{
        status = "declared"
        checklist_label = "Project template release readiness checklist"
        checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
        required_entrypoint_count = 3
        entrypoints = @(
            [ordered]@{
                id = "start_here"
                path = $startHerePath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $startHerePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "artifact_guide"
                path = $artifactGuidePath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $artifactGuidePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "reviewer_checklist"
                path = $reviewerChecklistPath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $reviewerChecklistPath -RepoRoot $resolvedRepoRoot
                required = $true
            }
        )
        checklist_marker = "release_entry_project_template_readiness_checklist_trace"
    }
    pdf_visual_gate_summary_json = $pdfGateSummaryPath
    pdf_visual_gate = [ordered]@{
        requested = $true
        status = "available"
        summary_json = $pdfGateSummaryPath
    }
    readme_gallery = [ordered]@{
        status = "completed"
        assets_dir = (Join-Path $resolvedRepoRoot "docs\assets\readme")
    }
    steps = [ordered]@{
        install_smoke = [ordered]@{
            status = "completed"
            install_prefix = $installPrefix
            consumer_document = (Join-Path $resolvedWorkingDir "build-msvc-install-consumer\featherdoc-install-smoke.docx")
        }
        visual_gate = [ordered]@{
            status = "completed"
            summary_json = $gateSummaryPath
            final_review = $gateFinalReviewPath
            visual_verdict = "pass"
            smoke_review_result_path = (Join-Path $smokeTaskDir "report\review_result.json")
            smoke_final_review_path = (Join-Path $smokeTaskDir "report\final_review.md")
            fixed_grid_review_result_path = (Join-Path $fixedGridTaskDir "report\review_result.json")
            fixed_grid_final_review_path = (Join-Path $fixedGridTaskDir "report\final_review.md")
            section_page_setup_review_result_path = (Join-Path $sectionPageSetupTaskDir "report\review_result.json")
            section_page_setup_final_review_path = (Join-Path $sectionPageSetupTaskDir "report\final_review.md")
            page_number_fields_review_result_path = (Join-Path $pageNumberFieldsTaskDir "report\review_result.json")
            page_number_fields_final_review_path = (Join-Path $pageNumberFieldsTaskDir "report\final_review.md")
        }
        pdf_visual_gate = [ordered]@{
            requested = $true
            status = "available"
            summary_json = $pdfGateSummaryPath
        }
        pdf_bounded_ctest = [ordered]@{
            status = "pass"
            summary_count = 7
            pass_count = 7
            skipped_test_count = 0
            selected_test_count = 70
            subsets = $pdfBoundedCtestSubsets
            summary_json_display = $pdfBoundedCtestSummaryJsonDisplay
            import_diagnostics_contract_tests = $pdfImportDiagnosticsContractTests
            import_diagnostics_contract_fields = $pdfImportDiagnosticsContractFields
            import_negative_boundary_contract_cases = $pdfImportNegativeBoundaryContractCases
        }
    }
}
($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$packageScript = Join-Path $resolvedRepoRoot "scripts\package_release_assets.ps1"
& $packageScript `
    -SummaryJson $summaryPath `
    -OutputRoot $outputRoot `
    -KeepStaging

$fontPolicyOutputRoot = Join-Path $resolvedWorkingDir "release-assets-font-policy"
$dummyBundledFontPath = Join-Path $installPrefix "share\FeatherDoc\fonts\NotoSansSC-Regular.ttf"
New-Item -ItemType Directory -Path (Split-Path -Parent $dummyBundledFontPath) -Force | Out-Null
Set-Content -LiteralPath $dummyBundledFontPath -Encoding ASCII -Value "not a real font"
try {
    & $packageScript `
        -SummaryJson $summaryPath `
        -OutputRoot $fontPolicyOutputRoot `
        -KeepStaging
    throw "package_release_assets.ps1 unexpectedly packaged a bundled font file without license manifest evidence."
} catch {
    $message = $_.Exception.Message
    if ($message -notmatch [regex]::Escape("current CJK font distribution policy forbids bundled TTF/OTF/TTC files")) {
        throw "package_release_assets.ps1 rejected the bundled font with an unexpected message: $message"
    }
    if ($message -notmatch [regex]::Escape("NotoSansSC-Regular.ttf")) {
        throw "package_release_assets.ps1 bundled-font rejection did not name the staged font file: $message"
    }
} finally {
    Remove-Item -LiteralPath $dummyBundledFontPath -Force -ErrorAction SilentlyContinue
}

$stagingRoot = Join-Path $outputRoot "v1.6.4\staging"
$stagedSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\summary.json"
$stagedGateSummaryPath = Join-Path $stagingRoot "word-visual-release-gate\report\gate_summary.json"
$stagedPdfGateSummaryPath = Join-Path $stagingRoot "pdf-visual-release-gate\report\summary.json"
$stagedPdfGateAggregateContactSheetPath = Join-Path $stagingRoot "pdf-visual-release-gate\report\aggregate-contact-sheet.png"
$stagedPdfGateUnicodeContactSheetPath = Join-Path $stagingRoot "pdf-visual-release-gate\unicode-font\report\full-contact-sheet.png"
$stagedHandoffPath = Join-Path $stagingRoot "release-candidate-checks\report\release_handoff.md"
$stagedGovernanceHandoffPath = Join-Path $stagingRoot "release-candidate-checks\report\release_governance_handoff.md"
$stagedReleaseBodyPath = Join-Path $stagingRoot "release-candidate-checks\report\release_body.zh-CN.md"
$stagedReleaseSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\release_summary.zh-CN.md"
$stagedFinalReviewPath = Join-Path $stagingRoot "release-candidate-checks\report\final_review.md"
$stagedStartHerePath = Join-Path $stagingRoot "release-candidate-checks\START_HERE.md"
$stagedArtifactGuidePath = Join-Path $stagingRoot "release-candidate-checks\report\ARTIFACT_GUIDE.md"
$stagedReviewerChecklistPath = Join-Path $stagingRoot "release-candidate-checks\report\REVIEWER_CHECKLIST.md"
$stagedContentControlSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\content_control_data_binding_governance_summary.json"
$stagedProjectTemplateDeliveryReadinessSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\project_template_delivery_readiness_summary.json"
$stagedProjectTemplateOnboardingGovernanceSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\project_template_onboarding_governance_summary.json"
$stagedInstalledReadmePath = Join-Path $stagingRoot "build-msvc-install\share\FeatherDoc\README.md"
$stagedInstalledChangelogPath = Join-Path $stagingRoot "build-msvc-install\share\FeatherDoc\CHANGELOG.md"
$manifestPath = Join-Path $outputRoot "v1.6.4\release_assets_manifest.json"
$installZipPath = Join-Path $outputRoot "v1.6.4\FeatherDoc-v1.6.4-msvc-install.zip"
$galleryZipPath = Join-Path $outputRoot "v1.6.4\FeatherDoc-v1.6.4-visual-validation-gallery.zip"
$evidenceZipPath = Join-Path $outputRoot "v1.6.4\FeatherDoc-v1.6.4-release-evidence.zip"
$expectedSanitizedAbsolutePath = "<windows-absolute-path>"
$expectedManifestSummaryPath = Convert-TestEvidencePathToPublicDisplay `
    -Path $summaryPath `
    -RepoRoot $resolvedRepoRoot
$expectedManifestPdfGateOutputDir = Convert-TestEvidencePathToPublicDisplay `
    -Path $pdfGateOutputDir `
    -RepoRoot $resolvedRepoRoot
$expectedSmokeReviewResultPath = Convert-TestEvidencePathToPublicDisplay `
    -Path (Join-Path $smokeTaskDir "report\review_result.json") `
    -RepoRoot $resolvedRepoRoot
$expectedSmokeFinalReviewPath = Convert-TestEvidencePathToPublicDisplay `
    -Path (Join-Path $smokeTaskDir "report\final_review.md") `
    -RepoRoot $resolvedRepoRoot
$stagedSummary = Get-Content -Raw -LiteralPath $stagedSummaryPath | ConvertFrom-Json
$stagedGateSummary = Get-Content -Raw -LiteralPath $stagedGateSummaryPath | ConvertFrom-Json
$stagedPdfGateSummary = Get-Content -Raw -LiteralPath $stagedPdfGateSummaryPath | ConvertFrom-Json
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
