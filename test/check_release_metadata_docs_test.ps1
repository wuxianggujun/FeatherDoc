param(
    [string]$RepoRoot = "",
    [string]$WorkingDir = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "check_release_metadata_docs_test_helpers.ps1")
. (Join-Path $PSScriptRoot "check_release_metadata_docs_test_summary_assertions.ps1")

$resolvedRepoRoot = Resolve-DefaultRepoRoot
$resolvedWorkingDir = Resolve-DefaultWorkingDir
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$docsCheckScript = Join-Path $resolvedRepoRoot "scripts\check_release_metadata_docs.ps1"
Assert-ScriptParses -Path $docsCheckScript

$defaultPipelineText = @(
    'Release metadata pipeline',
    '=========================',
    '',
    '- run_word_visual_release_gate.ps1',
    '- run_release_candidate_checks.ps1',
    '- sync_visual_review_verdict.ps1',
    '- sync_latest_visual_review_verdict.ps1',
    '- -GateSummaryJson',
    '- readme_gallery',
    '- assets_dir',
    '- refresh_readme_visual_assets.ps1',
    '- write_release_note_bundle.ps1',
    '- check_word_visual_release_gate_preflight.ps1',
    '- ``featherdoc.word_visual_release_gate_preflight.v1``',
    '- ``word_visual_release_gate_preflight_static_contract_only``',
    '- ``output_encoding``',
    '- UTF-8 without BOM',
    '- ``preflight_ready``',
    '- ``release_ready``',
    '- docx_functional_smoke_readiness',
    '- ``featherdoc.docx_functional_smoke_readiness.v1``',
    '- docx_functional_smoke_readiness_trace',
    '- persisted_docx_functional_smoke_evidence_only',
    '- summary_json_display',
    '- report_markdown_display',
    '- word_visual_smoke.pending_manual_review',
    '- release_blocker_count',
    '- review_task_summary',
    '- project_template_workflow_dashboard_report',
    '- Project template workflow dashboard status',
    '- Project template workflow dashboard next action',
    '- Project template workflow dashboard business document types',
    '- Project template workflow dashboard corpus roles',
    '- Project template workflow dashboard next action groups',
    '- Project template workflow dashboard action group',
    '- release_note_bundle',
    '- entrypoint_count',
    '- required_entrypoint_count',
    '- entrypoints[]',
    '- ``path_display``',
    '- ``location``',
    '- release_assets_manifest.json',
    '- package_release_assets.ps1',
    '- ``required``',
    '- assert_release_material_safety.ps1',
    '- -SkipMaterialSafetyAudit',
    '- reviewer_manifest_scoped_project_template_trace',
    '- manifest_signoff_entrypoints_release_trace',
    '- manifest_signoff_entrypoints_manifest_trace',
    '- release_entry_project_template_readiness_checklist_trace',
    '- release governance warning contract',
    '- warning_count',
    '- release_blocker_rollup.md',
    '- release_governance_handoff.md',
    '- release_governance_pipeline.md',
    '- ``featherdoc.release_governance_local_closure.v1``',
    '- local_governance_closure',
    '- local_governance_closure.status',
    '- local_governance_closure.closed',
    '- governance_detail_source',
    '- pipeline_summary_json',
    '- pipeline_summary_json_display',
    '- pipeline_report_markdown',
    '- pipeline_report_markdown_display',
    '- release_governance_handoff.release_blockers[]',
    '- release_governance_handoff.warnings[]',
    '- release_governance_handoff.action_items[]',
    '- ``source_json_display``',
    '- ``featherdoc.release_governance_pipeline_report.v1``',
    '- ``stages[]``',
    '- ``stage_id``',
    '- ``stage_title``',
    '- ``open_command``',
    '- final_governance_report_count',
    '- final_governance_reports',
    '- required_stage_count',
    '- completed_required_stage_count',
    '- required_stages',
    '- rerun_pdf_controlled_visual_smoke_check',
    '- check_pdf_controlled_visual_smoke.ps1',
    '- controlled_visual_smoke_json_display',
    '- check_pdf_release_readiness.ps1',
    '- featherdoc.pdf_release_readiness_check.v1',
    '- pdf_release_readiness_machine_gate_trace',
    '- preflight_summary_json_display',
    '- visual_gate_summary_json_display',
    '- visual_full_gate_guarded_summary_json_display',
    '- visual_segmented_gate_summary_json_display',
    '- full_ctest_summary_json_display',
    '- full_ctest_remaining_summary_json_display',
    '- pdf_visual_gate_release_owner_acceptance_trace',
    '- restore_docx_functional_smoke_evidence',
    '- Word visual standard review metadata evidence',
    '- word_visual_standard_review_metadata_source_reports',
    '- task_reviews=',
    '- release-candidate-checks',
    '- release-candidate-checks-source',
    '- release-candidate-checks\report\summary.json',
    '- release-candidate-checks-source\summary.json',
    '- report/ARTIFACT_GUIDE.md',
    '- report/REVIEWER_CHECKLIST.md',
    '- report/release_handoff.md',
    '- report/release_body.zh-CN.md',
    '- report/release_summary.zh-CN.md',
    '- ``id``',
    '- ``action``',
    '- ``message``',
    '- ``source_schema``',
    '- ``source_report_display``',
    '- blocker_source_schema_summary',
    '- action_item_source_schema_summary',
    '- informational_action_item_source_schema_summary',
    '- warning_source_schema_summary',
    '- ``output_gap_count``',
    '- ``missing_output_count``',
    '- ``output gap checks``',
    '- ``missing outputs``',
    '- ``style_merge_suggestion_count``',
    '- ``featherdoc.style_merge_restore_audit.v1``',
    '- ``review_handoff_steps``',
    '- ``next_handoff_step``',
    '- ``next_copy_command``',
    '- ``next_step_reason``',
    '- ``issue_review_commands``',
    '- ``issue_review_command_count``',
    '- ``issue_review_group_summary``',
    '- ``first_issue_review_command``',
    '- ``copy_issue_review_command``',
    '- ``handoff_status_summary``',
    '- ``rollback_plan_summary``',
    '- ``restorable_rollback_command_summary``',
    '- ``non_restorable_merge_rollback_entry_count``',
    '- ``non_restorable_merge_rollback_entry_indexes``',
    '- ``featherdoc.project_template_delivery_readiness_report.v1``',
    '- ``featherdoc.content_control_data_binding_governance_report.v1``',
    '- ``featherdoc.numbering_catalog_governance_report.v1``',
    '- ``featherdoc.table_layout_delivery_governance_report.v1``',
    '- ``sync_bound_content_control``',
    '- ``numbering_catalog_governance.real_corpus_alignment_gap``',
    '- ``delivery_quality``',
    '- ReleaseBlockerRollupFailOnBlocker',
    '- ReleaseBlockerRollupFailOnWarning',
    '- ReleaseGovernanceHandoffFailOnMissing',
    '- ReleaseGovernanceHandoffFailOnBlocker',
    '- ReleaseGovernanceHandoffFailOnWarning',
    ''
) -join "`n"

$defaultChecklistText = @(
    'Release metadata maintenance checklist',
    '======================================',
    '',
    '- :doc:`release_metadata_pipeline_zh`',
    '- word_visual_release_gate_smoke_verdict',
    '- check_word_visual_release_gate_preflight.ps1',
    '- check_word_visual_release_gate_preflight_test.ps1',
    '- word_visual_release_gate_preflight_route_docs_contract',
    '- word_visual_release_gate_preflight_route_docs_contract_test.ps1',
    '- ``featherdoc.word_visual_release_gate_preflight.v1``',
    '- ``word_visual_release_gate_preflight_static_contract_only``',
    '- ``checked_documents[]``',
    '- checked_document_count',
    '- checked_document_labels',
    '- checked_document_relative_paths',
    '- required_marker_count',
    '- ``output_encoding``',
    '- UTF-8 without BOM',
    '- ``preflight_ready``',
    '- ``release_ready``',
    '- release_candidate_visual_verdict',
    '- sync_latest_visual_review_verdict_table_style_quality',
    '- sync_latest_visual_review_verdict_cmake_contract',
    '- sync_visual_review_verdict_(section_page_setup|page_number_fields|curated_visual_bundle)',
    '- open_latest_word_review_task_curated_source_kind_test.ps1',
    '- open_latest_word_review_task.ps1 -SourceKind table-style-quality-visual-regression-bundle',
    '- latest_table-style-quality-visual-regression-bundle_task.json',
    '- release_note_bundle_visual_verdict_metadata',
    '- Word visual standard review metadata evidence',
    '- word_visual_standard_review_metadata_source_reports',
    '- task_reviews=',
    '- source_schema=featherdoc.release_candidate_summary',
    '- release-candidate-checks',
    '- release-candidate-checks-source',
    '- release-candidate-checks\report\summary.json',
    '- release-candidate-checks-source\summary.json',
    '- public_release_wording_regression_test.ps1',
    '- git diff --check',
    '- release_note_bundle',
    '- release_assets_manifest.json',
    '- manifest_signoff_entrypoints',
    '- release_entry_project_template_readiness_checklist_material_safety_audit',
    '- audit_script',
    '- audited_entrypoint_count',
    '- audited_entrypoints',
    '- release_entry_project_template_readiness_checklist_trace',
    '- project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace',
    '- entrypoint path contract',
    '- package_release_assets.ps1',
    '- package_release_assets_safety_test.ps1',
    '- package_release_assets_allow_incomplete_test.ps1',
    '- assert_release_material_safety.ps1',
    '- release governance warning contract',
    '- warning_count',
    '- release_blocker_rollup',
    '- release_governance_handoff',
    '- release_governance_pipeline',
    '- local_governance_closure',
    '- local_governance_closure.status',
    '- local_governance_closure.closed',
    '- governance_detail_source',
    '- pipeline_summary_json',
    '- pipeline_summary_json_display',
    '- pipeline_report_markdown',
    '- pipeline_report_markdown_display',
    '- release_governance_handoff.release_blockers[]',
    '- release_governance_handoff.warnings[]',
    '- release_governance_handoff.action_items[]',
    '- stages[]',
    '- stage_id',
    '- stage_title',
    '- final_governance_report_count',
    '- final_governance_reports',
    '- required_stage_count',
    '- completed_required_stage_count',
    '- required_stages',
    '- source_schema',
    '- source_report_display',
    '- source_json_display',
    '- block_scoped_governance_handoff_project_template_status_trace',
    '- featherdoc.style_merge_restore_audit.v1',
    '- review_handoff_steps',
    '- next_handoff_step',
    '- next_copy_command',
    '- next_step_reason',
    '- issue_review_commands',
    '- issue_review_command_count',
    '- issue_review_group_summary',
    '- first_issue_review_command',
    '- copy_issue_review_command',
    '- handoff_status_summary',
    '- rollback_plan_summary',
    '- restorable_rollback_command_summary',
    '- non_restorable_merge_rollback_entry_count',
    '- non_restorable_merge_rollback_entry_indexes',
    '- Assert-ReleaseGovernanceSourceSchemaSummaryConsistency',
    '- blocker_source_schema_summary',
    '- action_item_source_schema_summary',
    '- informational_action_item_source_schema_summary',
    '- warning_source_schema_summary',
    '- check_pdf_release_readiness.ps1',
    '- featherdoc.pdf_release_readiness_check.v1',
    '- pdf_release_readiness_machine_gate_trace',
    '- preflight_summary_json_display',
    '- visual_gate_summary_json_display',
    '- visual_full_gate_guarded_summary_json_display',
    '- visual_segmented_gate_summary_json_display',
    '- full_ctest_summary_json_display',
    '- full_ctest_remaining_summary_json_display',
    '- pdf_visual_gate_release_owner_acceptance_trace',
    '- style_merge_suggestion_count',
    '- check_docx_functional_smoke_readiness.ps1',
    '- docx_functional_smoke_readiness_test.ps1',
    '- docx_functional_smoke_readiness_route_docs_contract',
    '- docx_functional_smoke_readiness_route_docs_contract_test.ps1',
    '- docx_functional_smoke_readiness',
    '- ``featherdoc.docx_functional_smoke_readiness.v1``',
    '- docx_functional_smoke_readiness_trace',
    '- persisted_docx_functional_smoke_evidence_only',
    '- summary_json_display',
    '- report_markdown_display',
    '- word_visual_smoke.pending_manual_review',
    '- release_blocker_count',
    '- ReleaseBlockerRollupFailOnBlocker',
    '- ReleaseBlockerRollupFailOnWarning',
    '- ReleaseGovernanceHandoffFailOnMissing',
    '- ReleaseGovernanceHandoffFailOnBlocker',
    '- ReleaseGovernanceHandoffFailOnWarning',
    '- :doc:`document_governance_acceptance_zh`',
    '- build_project_template_delivery_readiness_report_test.ps1',
    '- build_content_control_data_binding_governance_report_test.ps1',
    '- build_numbering_catalog_governance_report_test.ps1',
    '- build_table_layout_delivery_governance_report_test.ps1',
    '- pdf_floating_table_support_coverage',
    '- pdf_floating_table_reviewer_focus',
    '- pdf_floating_table_metadata_only_fields',
    '- pdf_floating_table_review_required_fields',
    '- metadata-only tblpPr',
    '- metadata_only_fields',
    '- review_required_fields',
    ''
) -join "`n"

$defaultDocumentationMaintenanceText = @(
    'Documentation maintenance',
    '=========================',
    '',
    '- docs/release_metadata_pipeline_zh.rst',
    '- docs/release_metadata_maintenance_checklist_zh.rst',
    '- docs/pdf_release_readiness_checklist_zh.rst',
    ''
) -join "`n"

$defaultPdfReadinessChecklistText = @(
    'PDF release readiness checklist',
    '===============================',
    '',
    '- check_pdf_release_readiness.ps1',
    '- featherdoc.pdf_release_readiness_check.v1',
    '- pdf_release_readiness_machine_gate_trace',
    ''
) -join "`n"

$defaultDocumentGovernanceText = @(
    'Document governance acceptance',
    '==============================',
    '',
    '- document_governance_acceptance.v1',
    '- document_governance_primary_track',
    '- featherdoc.project_template_delivery_readiness_report.v1',
    '- featherdoc.content_control_data_binding_governance_report.v1',
    '- content_control_data_binding.bound_placeholder',
    '- sync_bound_content_control',
    '- featherdoc.numbering_catalog_governance_report.v1',
    '- numbering_catalog_governance.real_corpus_alignment_gap',
    '- featherdoc.table_layout_delivery_governance_report.v1',
    '- delivery_quality',
    '- pdf_floating_table_support_coverage',
    '- pdf_floating_table_reviewer_focus',
    '- metadata-only tblpPr',
    '- release_assets_manifest.json',
    '- manifest_signoff_entrypoints',
    '- release_entry_project_template_readiness_checklist_material_safety_audit',
    '- audit_script',
    '- audited_entrypoint_count',
    '- audited_entrypoints',
    '- release_entry_project_template_readiness_checklist_trace',
    '- project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace',
    '- package_release_assets_safety_test.ps1',
    '- package_release_assets_allow_incomplete_test.ps1',
    ''
) -join "`n"

$defaultPolicyText = @(
    'Release policy',
    '==============',
    '',
    'See :doc:`release_metadata_pipeline_zh`.',
    '- ReleaseBlockerRollupFailOnBlocker',
    '- ReleaseBlockerRollupFailOnWarning',
    '- ReleaseGovernanceHandoffFailOnMissing',
    '- ReleaseGovernanceHandoffFailOnBlocker',
    '- ReleaseGovernanceHandoffFailOnWarning',
    '- Word visual standard review metadata evidence',
    '- word_visual_standard_review_metadata_source_reports',
    '- task_reviews=',
    '- source_schema=featherdoc.release_candidate_summary',
    '- release-candidate-checks',
    '- release-candidate-checks-source',
    '- release-candidate-checks\report\summary.json',
    '- release-candidate-checks-source\summary.json',
    '- report/ARTIFACT_GUIDE.md',
    '- report/REVIEWER_CHECKLIST.md',
    '- report/release_handoff.md',
    '- release_note_bundle',
    '- release_assets_manifest.json',
    '- package_release_assets.ps1',
    '- assert_release_material_safety.ps1',
    '- ``required``',
    '- ``location``',
    '- ``path_display``',
    '1. Prepare ``CHANGELOG.md`` release decision.',
    '2. Align ``CMakeLists.txt`` version with release notes.',
    '3. Complete MSVC build, tests, samples and install smoke.',
    '4. Run local release candidate checks with Word visual release gate.',
    '5. Create final tag from ``CHANGELOG.md`` release notes.',
    ''
) -join "`n"

$defaultIndexText = @(
    'FeatherDoc',
    '==========',
    '',
    '.. toctree::',
    '',
    '   release_metadata_pipeline_zh',
    '   release_metadata_maintenance_checklist_zh',
    '   pdf_release_readiness_checklist_zh',
    ''
) -join "`n"

$defaultReadmeText = @(
    '# FeatherDoc',
    '',
    '- Release metadata pipeline guide: `docs/release_metadata_pipeline_zh.rst`',
    '- Release metadata maintenance checklist: `docs/release_metadata_maintenance_checklist_zh.rst`',
    '- PDF release readiness checklist: `docs/pdf_release_readiness_checklist_zh.rst`',
    ''
) -join "`n"

$defaultReadmeZhText = @(
    '# FeatherDoc',
    '',
    '- Release metadata 流水线：`docs/release_metadata_pipeline_zh.rst`',
    '- Release metadata 维护清单：`docs/release_metadata_maintenance_checklist_zh.rst`',
    '- PDF 发布准入清单：`docs/pdf_release_readiness_checklist_zh.rst`',
    ''
) -join "`n"

$passingCaseRoot = New-DocsCase -Name "passing"
$summaryJsonPath = Join-Path $passingCaseRoot "docs-check-summary.json"
Invoke-DocsCheck -CaseRoot $passingCaseRoot -SummaryJson $summaryJsonPath

if (-not (Test-Path -LiteralPath $summaryJsonPath)) {
    throw "check_release_metadata_docs.ps1 did not write the requested JSON summary."
}
Assert-FileHasNoBom -Path $summaryJsonPath
$summary = Get-Content -Raw -LiteralPath $summaryJsonPath | ConvertFrom-Json
if ($summary.status -ne "passed") {
    throw "Expected JSON summary status to be passed, got: $($summary.status)"
}
Assert-PassingReleaseMetadataDocsSummary -Summary $summary -SummaryJsonPath $summaryJsonPath


$quietCaseRoot = New-DocsCase -Name "quiet-passing"
$quietSummaryJsonPath = Join-Path $quietCaseRoot "docs-check-summary.json"
Invoke-DocsCheck -CaseRoot $quietCaseRoot -SummaryJson $quietSummaryJsonPath -Quiet
if (-not (Test-Path -LiteralPath $quietSummaryJsonPath)) {
    throw "check_release_metadata_docs.ps1 did not write JSON summary in quiet mode."
}


$missingPolicyCaseRoot = Join-Path $resolvedWorkingDir ("missing-policy-{0}" -f [System.Guid]::NewGuid().ToString("N"))
$missingPolicyDocsDir = Join-Path $missingPolicyCaseRoot "docs"
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyDocsDir "release_metadata_pipeline_zh.rst") `
    -Text $defaultPipelineText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyDocsDir "release_metadata_maintenance_checklist_zh.rst") `
    -Text $defaultChecklistText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyDocsDir "pdf_release_readiness_checklist_zh.rst") `
    -Text $defaultPdfReadinessChecklistText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyDocsDir "documentation_maintenance_zh.rst") `
    -Text $defaultDocumentationMaintenanceText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyDocsDir "document_governance_acceptance_zh.rst") `
    -Text $defaultDocumentGovernanceText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyDocsDir "index.rst") `
    -Text $defaultIndexText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyCaseRoot "README.md") `
    -Text $defaultReadmeText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyCaseRoot "README.zh-CN.md") `
    -Text $defaultReadmeZhText
$missingPolicySummaryJsonPath = Join-Path $missingPolicyCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPolicyCaseRoot `
    -ShouldFail `
    -ExpectedMessage "Missing release policy doc" `
    -SummaryJson $missingPolicySummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPolicySummaryJsonPath `
    -ExpectedMessage "Missing release policy doc" `
    -ExpectedFailureKind "missing_file" `
    -ExpectedFailureRelativePath 'docs/release_policy_zh.rst'

$missingIndexEntrypointText = $defaultIndexText.Replace(
    "release_metadata_maintenance_checklist_zh",
    "release_metadata_maintenance_checklist_removed"
)
$missingIndexEntrypointCaseRoot = New-DocsCase `
    -Name "missing-index-entrypoint" `
    -IndexText $missingIndexEntrypointText
$missingIndexEntrypointSummaryJsonPath = Join-Path $missingIndexEntrypointCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingIndexEntrypointCaseRoot `
    -ShouldFail `
    -ExpectedMessage "Sphinx index doc is missing expected text: release_metadata_maintenance_checklist_zh" `
    -SummaryJson $missingIndexEntrypointSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingIndexEntrypointSummaryJsonPath `
    -ExpectedMessage "Sphinx index doc is missing expected text: release_metadata_maintenance_checklist_zh" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/index.rst' `
    -ExpectedFailureExpectedText "release_metadata_maintenance_checklist_zh"

$missingDocumentationMaintenanceEntrypointText = $defaultDocumentationMaintenanceText.Replace(
    "release_metadata_pipeline_zh",
    "release_metadata_pipeline_removed"
)
$missingDocumentationMaintenanceEntrypointCaseRoot = New-DocsCase `
    -Name "missing-documentation-maintenance-entrypoint" `
    -DocumentationMaintenanceText $missingDocumentationMaintenanceEntrypointText
$missingDocumentationMaintenanceEntrypointSummaryJsonPath = Join-Path $missingDocumentationMaintenanceEntrypointCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingDocumentationMaintenanceEntrypointCaseRoot `
    -ShouldFail `
    -ExpectedMessage "documentation maintenance overview doc is missing expected text: release_metadata_pipeline_zh" `
    -SummaryJson $missingDocumentationMaintenanceEntrypointSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingDocumentationMaintenanceEntrypointSummaryJsonPath `
    -ExpectedMessage "documentation maintenance overview doc is missing expected text: release_metadata_pipeline_zh" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/documentation_maintenance_zh.rst' `
    -ExpectedFailureExpectedText "release_metadata_pipeline_zh"

$missingDocumentationMaintenanceChecklistEntrypointText = $defaultDocumentationMaintenanceText.Replace(
    "release_metadata_maintenance_checklist_zh",
    "release_metadata_maintenance_checklist_removed"
)
$missingDocumentationMaintenanceChecklistEntrypointCaseRoot = New-DocsCase `
    -Name "missing-documentation-maintenance-checklist-entrypoint" `
    -DocumentationMaintenanceText $missingDocumentationMaintenanceChecklistEntrypointText
$missingDocumentationMaintenanceChecklistEntrypointSummaryJsonPath = Join-Path $missingDocumentationMaintenanceChecklistEntrypointCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingDocumentationMaintenanceChecklistEntrypointCaseRoot `
    -ShouldFail `
    -ExpectedMessage "documentation maintenance overview doc is missing expected text: release_metadata_maintenance_checklist_zh" `
    -SummaryJson $missingDocumentationMaintenanceChecklistEntrypointSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingDocumentationMaintenanceChecklistEntrypointSummaryJsonPath `
    -ExpectedMessage "documentation maintenance overview doc is missing expected text: release_metadata_maintenance_checklist_zh" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/documentation_maintenance_zh.rst' `
    -ExpectedFailureExpectedText "release_metadata_maintenance_checklist_zh"

$missingPolicyWordVisualMetadataText = $defaultPolicyText.Replace(
    "Word visual standard review metadata evidence",
    "Word visual metadata evidence removed"
)
$missingPolicyWordVisualMetadataCaseRoot = New-DocsCase `
    -Name "missing-policy-word-visual-metadata" `
    -PolicyText $missingPolicyWordVisualMetadataText
$missingPolicyWordVisualMetadataSummaryJsonPath = Join-Path $missingPolicyWordVisualMetadataCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPolicyWordVisualMetadataCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release policy doc is missing expected text: Word visual standard review metadata evidence" `
    -SummaryJson $missingPolicyWordVisualMetadataSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPolicyWordVisualMetadataSummaryJsonPath `
    -ExpectedMessage "release policy doc is missing expected text: Word visual standard review metadata evidence" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_policy_zh.rst' `
    -ExpectedFailureExpectedText "Word visual standard review metadata evidence"

$missingPipelineWordVisualMetadataText = $defaultPipelineText.Replace(
    "Word visual standard review metadata evidence",
    "Word visual metadata evidence removed"
)
$missingPipelineWordVisualMetadataCaseRoot = New-DocsCase `
    -Name "missing-pipeline-word-visual-metadata" `
    -PipelineText $missingPipelineWordVisualMetadataText
$missingPipelineWordVisualMetadataSummaryJsonPath = Join-Path $missingPipelineWordVisualMetadataCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPipelineWordVisualMetadataCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: Word visual standard review metadata evidence" `
    -SummaryJson $missingPipelineWordVisualMetadataSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPipelineWordVisualMetadataSummaryJsonPath `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: Word visual standard review metadata evidence" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst' `
    -ExpectedFailureExpectedText "Word visual standard review metadata evidence"

$missingPipelineWorkflowDashboardText = $defaultPipelineText.Replace(
    "project_template_workflow_dashboard_report",
    "project_template_workflow_dashboard_removed"
)
$missingPipelineWorkflowDashboardCaseRoot = New-DocsCase `
    -Name "missing-pipeline-workflow-dashboard" `
    -PipelineText $missingPipelineWorkflowDashboardText
$missingPipelineWorkflowDashboardSummaryJsonPath = Join-Path $missingPipelineWorkflowDashboardCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPipelineWorkflowDashboardCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: project_template_workflow_dashboard_report" `
    -SummaryJson $missingPipelineWorkflowDashboardSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPipelineWorkflowDashboardSummaryJsonPath `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: project_template_workflow_dashboard_report" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst' `
    -ExpectedFailureExpectedText "project_template_workflow_dashboard_report"

$missingPipelineReleaseManifestText = $defaultPipelineText.Replace(
    "release_assets_manifest.json",
    "release_assets_manifest_removed.json"
)
$missingPipelineReleaseManifestCaseRoot = New-DocsCase `
    -Name "missing-pipeline-release-manifest" `
    -PipelineText $missingPipelineReleaseManifestText
$missingPipelineReleaseManifestSummaryJsonPath = Join-Path $missingPipelineReleaseManifestCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPipelineReleaseManifestCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: release_assets_manifest.json" `
    -SummaryJson $missingPipelineReleaseManifestSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPipelineReleaseManifestSummaryJsonPath `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: release_assets_manifest.json" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst' `
    -ExpectedFailureExpectedText "release_assets_manifest.json"

$missingPipelineHandoffActionItemsText = $defaultPipelineText.Replace(
    "release_governance_handoff.action_items[]",
    "release_governance_handoff.action_items_removed[]"
)
$missingPipelineHandoffActionItemsCaseRoot = New-DocsCase `
    -Name "missing-pipeline-handoff-action-items" `
    -PipelineText $missingPipelineHandoffActionItemsText
$missingPipelineHandoffActionItemsSummaryJsonPath = Join-Path $missingPipelineHandoffActionItemsCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPipelineHandoffActionItemsCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: release_governance_handoff.action_items[]" `
    -SummaryJson $missingPipelineHandoffActionItemsSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPipelineHandoffActionItemsSummaryJsonPath `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: release_governance_handoff.action_items[]" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst' `
    -ExpectedFailureExpectedText "release_governance_handoff.action_items[]"

$missingChecklistPipelineStagesText = $defaultChecklistText.Replace(
    "stages[]",
    "stages_removed[]"
)
$missingChecklistPipelineStagesCaseRoot = New-DocsCase `
    -Name "missing-checklist-pipeline-stages" `
    -ChecklistText $missingChecklistPipelineStagesText
$missingChecklistPipelineStagesSummaryJsonPath = Join-Path $missingChecklistPipelineStagesCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingChecklistPipelineStagesCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: stages[]" `
    -SummaryJson $missingChecklistPipelineStagesSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingChecklistPipelineStagesSummaryJsonPath `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: stages[]" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "stages[]"

$missingChecklistManifestSafetyTraceText = $defaultChecklistText.Replace(
    "release_entry_project_template_readiness_checklist_material_safety_audit",
    "release_entry_project_template_readiness_checklist_material_safety_removed"
)
$missingChecklistManifestSafetyTraceCaseRoot = New-DocsCase `
    -Name "missing-checklist-manifest-safety-trace" `
    -ChecklistText $missingChecklistManifestSafetyTraceText
$missingChecklistManifestSafetyTraceSummaryJsonPath = Join-Path $missingChecklistManifestSafetyTraceCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingChecklistManifestSafetyTraceCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: release_entry_project_template_readiness_checklist_material_safety_audit" `
    -SummaryJson $missingChecklistManifestSafetyTraceSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingChecklistManifestSafetyTraceSummaryJsonPath `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: release_entry_project_template_readiness_checklist_material_safety_audit" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit"

$missingChecklistReleaseEntryTraceText = $defaultChecklistText.Replace(
    "release_entry_project_template_readiness_checklist_trace",
    "release_entry_project_template_readiness_checklist_removed"
)
$missingChecklistReleaseEntryTraceCaseRoot = New-DocsCase `
    -Name "missing-checklist-release-entry-trace" `
    -ChecklistText $missingChecklistReleaseEntryTraceText
$missingChecklistReleaseEntryTraceSummaryJsonPath = Join-Path $missingChecklistReleaseEntryTraceCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingChecklistReleaseEntryTraceCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: release_entry_project_template_readiness_checklist_trace" `
    -SummaryJson $missingChecklistReleaseEntryTraceSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingChecklistReleaseEntryTraceSummaryJsonPath `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: release_entry_project_template_readiness_checklist_trace" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "release_entry_project_template_readiness_checklist_trace"

$missingChecklistMaterialSafetyMarkerText = $defaultChecklistText.Replace(
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_removed"
)
$missingChecklistMaterialSafetyMarkerCaseRoot = New-DocsCase `
    -Name "missing-checklist-material-safety-marker" `
    -ChecklistText $missingChecklistMaterialSafetyMarkerText
$missingChecklistMaterialSafetyMarkerSummaryJsonPath = Join-Path $missingChecklistMaterialSafetyMarkerCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingChecklistMaterialSafetyMarkerCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace" `
    -SummaryJson $missingChecklistMaterialSafetyMarkerSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingChecklistMaterialSafetyMarkerSummaryJsonPath `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"

$missingChecklistProjectTemplateHandoffTraceText = $defaultChecklistText.Replace(
    "block_scoped_governance_handoff_project_template_status_trace",
    "block_scoped_governance_handoff_project_template_status_removed"
)
$missingChecklistProjectTemplateHandoffTraceCaseRoot = New-DocsCase `
    -Name "missing-checklist-project-template-handoff-trace" `
    -ChecklistText $missingChecklistProjectTemplateHandoffTraceText
$missingChecklistProjectTemplateHandoffTraceSummaryJsonPath = Join-Path $missingChecklistProjectTemplateHandoffTraceCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingChecklistProjectTemplateHandoffTraceCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: block_scoped_governance_handoff_project_template_status_trace" `
    -SummaryJson $missingChecklistProjectTemplateHandoffTraceSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingChecklistProjectTemplateHandoffTraceSummaryJsonPath `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: block_scoped_governance_handoff_project_template_status_trace" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "block_scoped_governance_handoff_project_template_status_trace"

$missingPolicyMaterialSafetyText = $defaultPolicyText.Replace(
    "assert_release_material_safety.ps1",
    "assert_release_material_removed.ps1"
)
$missingPolicyMaterialSafetyCaseRoot = New-DocsCase `
    -Name "missing-policy-material-safety" `
    -PolicyText $missingPolicyMaterialSafetyText
$missingPolicyMaterialSafetySummaryJsonPath = Join-Path $missingPolicyMaterialSafetyCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPolicyMaterialSafetyCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release policy doc is missing expected text: assert_release_material_safety.ps1" `
    -SummaryJson $missingPolicyMaterialSafetySummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPolicyMaterialSafetySummaryJsonPath `
    -ExpectedMessage "release policy doc is missing expected text: assert_release_material_safety.ps1" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_policy_zh.rst' `
    -ExpectedFailureExpectedText "assert_release_material_safety.ps1"

$releasePolicyStep4 = '4. Run local release candidate checks with Word visual release gate.'
$releasePolicyStep5 = '5. Create final tag from ``CHANGELOG.md`` release notes.'
$releasePolicyOutOfOrderText = $defaultPolicyText.Replace(
    ($releasePolicyStep4 + "`n" + $releasePolicyStep5),
    ($releasePolicyStep5 + "`n" + $releasePolicyStep4)
)
$releasePolicyOutOfOrderCaseRoot = New-DocsCase `
    -Name "release-policy-execution-order" `
    -PolicyText $releasePolicyOutOfOrderText
$releasePolicyOutOfOrderSummaryJsonPath = Join-Path $releasePolicyOutOfOrderCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $releasePolicyOutOfOrderCaseRoot `
    -ShouldFail `
    -ExpectedMessage 'release policy doc has missing or out-of-order line sequence: 1. + ``CHANGELOG.md``' `
    -SummaryJson $releasePolicyOutOfOrderSummaryJsonPath
Assert-SummaryFailure `
    -Path $releasePolicyOutOfOrderSummaryJsonPath `
    -ExpectedMessage 'release policy doc has missing or out-of-order line sequence: 1. + ``CHANGELOG.md``' `
    -ExpectedFailureKind "text_order" `
    -ExpectedFailureRuleId "release_metadata_docs.release_policy_execution_order" `
    -ExpectedFailureRelativePath 'docs/release_policy_zh.rst' `
    -ExpectedFailureExpectedText '1. + ``CHANGELOG.md``'

$missingChecklistWordVisualMetadataText = $defaultChecklistText.Replace(
    "Word visual standard review metadata evidence",
    "Word visual metadata evidence removed"
)
$missingChecklistWordVisualMetadataCaseRoot = New-DocsCase `
    -Name "missing-checklist-word-visual-metadata" `
    -ChecklistText $missingChecklistWordVisualMetadataText
$missingChecklistWordVisualMetadataSummaryJsonPath = Join-Path $missingChecklistWordVisualMetadataCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingChecklistWordVisualMetadataCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: Word visual standard review metadata evidence" `
    -SummaryJson $missingChecklistWordVisualMetadataSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingChecklistWordVisualMetadataSummaryJsonPath `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: Word visual standard review metadata evidence" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "Word visual standard review metadata evidence"

$trailingWhitespacePipelineText = $defaultPipelineText.Replace(
    "- review_task_summary",
    "- review_task_summary "
)
$trailingWhitespaceExpectedExcerpt = "- review_task_summary "
$trailingWhitespaceExpectedLines = @($trailingWhitespacePipelineText -split "`r?`n")
$trailingWhitespaceExpectedLineNumber = [array]::IndexOf(
    $trailingWhitespaceExpectedLines,
    $trailingWhitespaceExpectedExcerpt
) + 1
if ($trailingWhitespaceExpectedLineNumber -le 0) {
    throw "Expected trailing whitespace fixture line was not found."
}
$trailingWhitespaceExpectedColumnNumber = $trailingWhitespaceExpectedExcerpt.Length
$trailingWhitespaceCaseRoot = New-DocsCase `
    -Name "trailing-whitespace-pipeline" `
    -PipelineText $trailingWhitespacePipelineText
$trailingWhitespaceSummaryJsonPath = Join-Path $trailingWhitespaceCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $trailingWhitespaceCaseRoot `
    -ShouldFail `
    -ExpectedMessage "Trailing whitespace" `
    -SummaryJson $trailingWhitespaceSummaryJsonPath
Assert-SummaryFailure `
    -Path $trailingWhitespaceSummaryJsonPath `
    -ExpectedMessage "Trailing whitespace" `
    -ExpectedFailureKind "trailing_whitespace" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst' `
    -ExpectedFailureLineNumber $trailingWhitespaceExpectedLineNumber `
    -ExpectedFailureColumnNumber $trailingWhitespaceExpectedColumnNumber `
    -ExpectedFailureExcerpt $trailingWhitespaceExpectedExcerpt

$tabChecklistText = $defaultChecklistText.Replace(
    "- git diff --check",
    "- git`t diff --check"
)
$tabExpectedExcerpt = "- git`t diff --check"
$tabExpectedLines = @($tabChecklistText -split "`r?`n")
$tabExpectedLineNumber = [array]::IndexOf($tabExpectedLines, $tabExpectedExcerpt) + 1
if ($tabExpectedLineNumber -le 0) {
    throw "Expected tab fixture line was not found."
}
$tabExpectedColumnNumber = $tabExpectedExcerpt.IndexOf("`t") + 1
$tabCaseRoot = New-DocsCase -Name "tab-checklist" -ChecklistText $tabChecklistText
$tabSummaryJsonPath = Join-Path $tabCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $tabCaseRoot `
    -ShouldFail `
    -ExpectedMessage "Tab character found" `
    -SummaryJson $tabSummaryJsonPath
Assert-SummaryFailure `
    -Path $tabSummaryJsonPath `
    -ExpectedMessage "Tab character found" `
    -ExpectedFailureKind "tab_character" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureLineNumber $tabExpectedLineNumber `
    -ExpectedFailureColumnNumber $tabExpectedColumnNumber `
    -ExpectedFailureExcerpt $tabExpectedExcerpt

$missingChecklistEntry = $defaultChecklistText.Replace(
    "release_note_bundle_visual_verdict_metadata",
    "release_note_bundle_visual_verdict_removed"
)
$missingChecklistCaseRoot = New-DocsCase -Name "missing-checklist-entry" -ChecklistText $missingChecklistEntry
$missingChecklistSummaryJsonPath = Join-Path $missingChecklistCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingChecklistCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: release_note_bundle_visual_verdict_metadata" `
    -SummaryJson $missingChecklistSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingChecklistSummaryJsonPath `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: release_note_bundle_visual_verdict_metadata" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "release_note_bundle_visual_verdict_metadata"

$missingDocumentGovernanceEntry = $defaultDocumentGovernanceText.Replace(
    "- sync_bound_content_control",
    "- sync_removed_content_control"
)
$missingDocumentGovernanceCaseRoot = New-DocsCase `
    -Name "missing-document-governance-entry" `
    -DocumentGovernanceText $missingDocumentGovernanceEntry
$missingDocumentGovernanceSummaryJsonPath = Join-Path $missingDocumentGovernanceCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingDocumentGovernanceCaseRoot `
    -ShouldFail `
    -ExpectedMessage "document governance acceptance doc is missing expected text: sync_bound_content_control" `
    -SummaryJson $missingDocumentGovernanceSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingDocumentGovernanceSummaryJsonPath `
    -ExpectedMessage "document governance acceptance doc is missing expected text: sync_bound_content_control" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/document_governance_acceptance_zh.rst' `
    -ExpectedFailureExpectedText "sync_bound_content_control"

$bomCaseRoot = New-DocsCase -Name "bom-pipeline"
Write-Utf8BomFile `
    -Path (Join-Path $bomCaseRoot "docs/release_metadata_pipeline_zh.rst") `
    -Text $defaultPipelineText
$bomSummaryJsonPath = Join-Path $bomCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $bomCaseRoot `
    -ShouldFail `
    -ExpectedMessage "File must be UTF-8 without BOM" `
    -SummaryJson $bomSummaryJsonPath
Assert-SummaryFailure `
    -Path $bomSummaryJsonPath `
    -ExpectedMessage "File must be UTF-8 without BOM" `
    -ExpectedFailureKind "utf8_bom" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst'

Write-Host "Release metadata docs checker regression passed."
