Assert-NotContains -Path $stagedSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged summary.json'
Assert-NotContains -Path $stagedGateSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged gate_summary.json'
Assert-NotContains -Path $stagedPdfGateSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged PDF visual gate summary.json'
Assert-NotContains -Path $manifestPath -UnexpectedText $resolvedRepoRoot -Label 'release_assets_manifest.json'
Assert-NotContains -Path $manifestPath -UnexpectedText '<windows-absolute-path>' -Label 'release_assets_manifest.json'
Assert-NotContains -Path $manifestPath -UnexpectedText '<unix-absolute-path>' -Label 'release_assets_manifest.json'
Assert-NotContains -Path $stagedHandoffPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_handoff.md'
Assert-NotContains -Path $stagedGovernanceHandoffPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_governance_handoff.md'
Assert-NotContains -Path $stagedReleaseBodyPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_body.zh-CN.md'
Assert-NotContains -Path $stagedReleaseSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_summary.zh-CN.md'
Assert-NotContains -Path $stagedFinalReviewPath -UnexpectedText $resolvedRepoRoot -Label 'staged final_review.md'
Assert-NotContains -Path $stagedInstalledReadmePath -UnexpectedText 'C:\path\to\target.docx' -Label 'staged installed README.md'
Assert-NotContains -Path $stagedInstalledChangelogPath -UnexpectedText 'draft' -Label 'staged installed CHANGELOG.md'
Assert-NotContains -Path $stagedInstalledChangelogPath -UnexpectedText 'C:\Users\someone\workspace' -Label 'staged installed CHANGELOG.md'
Assert-Contains -Path $stagedInstalledReadmePath -ExpectedText '<windows-absolute-path>' -Label 'staged installed README.md'
Assert-Contains -Path $stagedInstalledChangelogPath -ExpectedText 'preview' -Label 'staged installed CHANGELOG.md'
Assert-Contains -Path $stagedInstalledChangelogPath -ExpectedText '<windows-absolute-path>' -Label 'staged installed CHANGELOG.md'
Assert-Contains -Path $stagedSummaryPath -ExpectedText 'governance_metrics' -Label 'staged summary.json'
Assert-Contains -Path $stagedSummaryPath -ExpectedText 'real_corpus_confidence' -Label 'staged summary.json'
Assert-Contains -Path $stagedSummaryPath -ExpectedText 'delivery_quality' -Label 'staged summary.json'
Assert-Contains -Path $stagedSummaryPath -ExpectedText 'project_template_delivery_readiness' -Label 'staged summary.json'
Assert-Contains -Path $stagedSummaryPath -ExpectedText 'project_template_onboarding_governance' -Label 'staged summary.json'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'Governance Metrics' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'real_corpus_confidence' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'featherdoc.numbering_catalog_governance_report.v1' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'delivery_quality' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'project_template_delivery_readiness' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'source_report_display' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'source_json_display' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'latest_schema_approval_gate_status' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'schema_approval_status_summary' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText '.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'status: `ready`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'release_ready: `True`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText '.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'PDF visual gate evidence source reports: `1`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'full_visual_gate_status: `pass`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_verdict: `pass`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_finalizable: `True`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_summary_json_display' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_aggregate_contact_sheet_display' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'aggregate-contact-sheet.png' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_cjk_manifest_count: `43`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_cjk_copy_search_count: `2`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_visual_baseline_manifest_count: `42`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_visual_baseline_count: `3`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_bounded_ctest_summary_count: `7`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_bounded_ctest_pass_count: `7`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_bounded_ctest_skipped_test_count: `0`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_bounded_ctest_selected_test_count: `70`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText "pdf_bounded_ctest_subsets: ``$pdfBoundedCtestSubsetsText``" -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText "pdf_bounded_ctest_summary_json_display: ``$pdfBoundedCtestSummaryJsonDisplayText``" -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'Word visual standard review metadata source reports: `1`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'word_visual_standard_review_metadata_count: `4`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'word_visual_standard_review_task_keys: `smoke, fixed_grid, section_page_setup, page_number_fields`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'word_visual_standard_review_status_summary: `reviewed=4`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'word_visual_standard_review_verdict_summary: `pass=4`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText '`smoke`: review_task_key=`document` verdict=`pass` review_status=`reviewed` review_method=`operator_supplied`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'reviewed_at: `2026-04-12T12:10:00`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'review_result_path: `.\output\word-visual-release-gate\review-tasks\document\report\review_result.json`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'final_review_path: `.\output\word-visual-release-gate\review-tasks\document\report\final_review.md`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'project_template_delivery_readiness_contract:' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_schema: featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'latest_schema_approval_gate_status: passed' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'schema_approval_status_summary: approved=4' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'project_template_onboarding_governance_contract:' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_schema: featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'status: ready' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'release_ready: True' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'schema_approval_status_summary: approved' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedReleaseBodyPath -ExpectedText 'project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_body.zh-CN.md'
Assert-Contains -Path $stagedReleaseBodyPath -ExpectedText 'project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json' -Label 'staged release_body.zh-CN.md'
Assert-Contains -Path $stagedReleaseSummaryPath -ExpectedText 'project-template readiness governance contract' -Label 'staged release_summary.zh-CN.md'
Assert-Contains -Path $stagedReleaseSummaryPath -ExpectedText 'status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_summary.zh-CN.md'
Assert-Contains -Path $stagedReleaseSummaryPath -ExpectedText 'project-template onboarding governance contract' -Label 'staged release_summary.zh-CN.md'
Assert-Contains -Path $stagedReleaseSummaryPath -ExpectedText 'status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json' -Label 'staged release_summary.zh-CN.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'Release governance handoff details' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'project_template_delivery_readiness' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'status: ready' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'release_ready: True' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'schema_approval_status_summary: approved' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'readiness_status: ready' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'readiness_release_ready: True' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate: loaded' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate verdict: pass' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate counts: 3 visual baselines, 2 CJK copy/search' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate finalizable: True' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate summary:' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate contact sheet:' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'aggregate-contact-sheet.png' -Label 'staged final_review.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'featherdoc.content_control_data_binding_governance_report.v1' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'source_report_display' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'source_json_display' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'input_docx=samples/invoice.docx' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'template_name=invoice-template' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'schema_target=invoice' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'target_mode=resolved-section-targets' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'repair_strategy' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'repair_hint' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'sync_bound_content_control' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'command_template' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'project_template_delivery_readiness' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'project_template_delivery_readiness_contract' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'status: ready release_ready: True' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'latest_schema_approval_gate_status' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'schema_approval_status_summary' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'catalog_coverage_percent=100' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'matched_document_count=2' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'alignment_gap_count=0' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'matched_document_keys=contract.docx,invoice.docx' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'style_numbering_issues(count=4, penalty=20)' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'unresolved_item_count=0' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'floating_table_plans_pending(count=0, penalty=0)' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'PDF visual gate summary:' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'PDF CJK manifest samples: 43' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'PDF CJK copy/search samples: 2' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'PDF visual baseline manifest samples: 42' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'PDF visual baselines: 3' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'aggregate-contact-sheet.png' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'featherdoc.content_control_data_binding_governance_report.v1' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'source_report_display' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'source_json_display' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'input_docx=samples/invoice.docx' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'template_name=invoice-template' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'schema_target=invoice' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'target_mode=resolved-section-targets' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'repair_strategy' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'repair_hint' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'sync_bound_content_control' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'real_corpus_confidence' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'featherdoc.numbering_catalog_governance_report.v1' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'delivery_quality' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'project_template_delivery_readiness' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'project_template_delivery_readiness_contract' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'status: ready release_ready: True' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'latest_schema_approval_gate_status' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'project_template_onboarding_governance' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'schema_approval_status_summary' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'catalog_coverage_percent=100' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'matched_document_count=2' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'alignment_gap_count=0' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'matched_document_keys=contract.docx,invoice.docx' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'style_numbering_issues(count=4, penalty=20)' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'unresolved_item_count=0' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'floating_table_plans_pending(count=0, penalty=0)' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'command_template' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'PDF visual gate summary:' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'PDF CJK manifest samples: 43' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'PDF CJK copy/search samples: 2' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'PDF visual baseline manifest samples: 42' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'PDF visual baselines: 3' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'aggregate-contact-sheet.png' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'featherdoc.content_control_data_binding_governance_report.v1' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'source_report_display' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'source_json_display' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'input_docx=samples/invoice.docx' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'template_name=invoice-template' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'schema_target=invoice' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'target_mode=resolved-section-targets' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'repair_strategy' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'repair_hint' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'command_template' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'project_template_delivery_readiness' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'project_template_delivery_readiness_contract' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'status: ready release_ready: True' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'latest_schema_approval_gate_status' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'schema_approval_status_summary' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'catalog_coverage_percent=100' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'matched_document_count=2' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'alignment_gap_count=0' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'matched_document_keys=contract.docx,invoice.docx' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'style_numbering_issues(count=4, penalty=20)' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'unresolved_item_count=0' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'floating_table_plans_pending(count=0, penalty=0)' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'Confirm PDF visual gate summary' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText '2 CJK copy/search samples' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText '3 visual baselines' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'aggregate-contact-sheet.png' -Label 'staged REVIEWER_CHECKLIST.md'
foreach ($entrypointMaterial in @(
        @{ Path = $stagedStartHerePath; Label = 'staged START_HERE.md' },
        @{ Path = $stagedArtifactGuidePath; Label = 'staged ARTIFACT_GUIDE.md' },
        @{ Path = $stagedReviewerChecklistPath; Label = 'staged REVIEWER_CHECKLIST.md' }
    )) {
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'requires_reviewer_action' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'reviewer_action_summary' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'reviewer_action_reason' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'reviewer_actions' -Label $entrypointMaterial.Label
}
foreach ($entrypointMaterial in @(
        @{ Path = $stagedStartHerePath; Label = 'staged START_HERE.md' },
        @{ Path = $stagedArtifactGuidePath; Label = 'staged ARTIFACT_GUIDE.md' },
        @{ Path = $stagedReviewerChecklistPath; Label = 'staged REVIEWER_CHECKLIST.md' }
    )) {
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'pdf_floating_table_support_coverage' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'pdf_floating_table_reviewer_focus' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'metadata-only tblpPr' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'metadata_only_fields' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'review_required_fields' -Label $entrypointMaterial.Label
}
foreach ($entrypointMaterial in @(
        @{ Path = $stagedStartHerePath; Label = 'staged START_HERE.md' },
        @{ Path = $stagedArtifactGuidePath; Label = 'staged ARTIFACT_GUIDE.md' },
        @{ Path = $stagedReviewerChecklistPath; Label = 'staged REVIEWER_CHECKLIST.md' }
    )) {
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'Project-template readiness checklist handoff evidence' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'project_template_readiness_checklist_entrypoints_source_reports=1' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'checklist_path=docs/project_template_release_readiness_checklist_zh.rst' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'required_entrypoint_count=3' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'entrypoints=start_here, artifact_guide, reviewer_checklist' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'entrypoint_paths=' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'marker=release_entry_project_template_readiness_checklist_trace' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'source_schema=featherdoc.release_candidate_summary' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'source_report=.\output\release-candidate-checks\summary.json' -Label $entrypointMaterial.Label
    Assert-LineContainsAll -Path $entrypointMaterial.Path -ExpectedFragments @(
        'Project-template readiness checklist packaged audit evidence',
        'release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1',
        'status=passed',
        'audit_script=.\scripts\assert_release_material_safety.ps1',
        'audited_entrypoint_count=3',
        'audited_entrypoints=start_here, artifact_guide, reviewer_checklist',
        'compact_evidence_label=Project-template readiness checklist handoff evidence',
        'compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports',
        'compact_evidence_source_schema=featherdoc.release_candidate_summary',
        'checklist_path=docs/project_template_release_readiness_checklist_zh.rst',
        'checklist_marker=release_entry_project_template_readiness_checklist_trace',
        'material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace',
        'source_schema=featherdoc.release_candidate_summary',
        'source_report=.\output\release-blocker-rollup\summary.json'
    ) -Label $entrypointMaterial.Label
}
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'sync_bound_content_control' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'command_template' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'samples/invoice.docx' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'invoice-template' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'resolved-section-targets' -Label 'staged content-control summary'
Assert-Contains -Path $stagedProjectTemplateDeliveryReadinessSummaryPath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged project-template readiness summary'
Assert-Contains -Path $stagedProjectTemplateDeliveryReadinessSummaryPath -ExpectedText 'latest_schema_approval_gate_status' -Label 'staged project-template readiness summary'
Assert-Contains -Path $stagedProjectTemplateOnboardingGovernanceSummaryPath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged project-template onboarding governance summary'
Assert-Contains -Path $stagedProjectTemplateOnboardingGovernanceSummaryPath -ExpectedText 'schema_approval_status_summary' -Label 'staged project-template onboarding governance summary'
if ((Convert-TestComparablePathValue -Value $stagedSummary.release_handoff) -ne
    (Convert-TestComparablePathValue -Value $expectedSanitizedAbsolutePath)) {
    throw "staged summary.json did not sanitize release_handoff absolute path to the expected placeholder."
}
if ((Convert-TestComparablePathValue -Value $stagedGateSummary.report_dir) -ne
    (Convert-TestComparablePathValue -Value $expectedSanitizedAbsolutePath)) {
    throw "staged gate_summary.json did not sanitize report_dir absolute path to the expected placeholder."
}
if ((Convert-TestComparablePathValue -Value $stagedSummary.pdf_visual_gate_summary_json) -ne
    (Convert-TestComparablePathValue -Value $expectedSanitizedAbsolutePath)) {
    throw "staged summary.json did not sanitize pdf_visual_gate_summary_json absolute path to the expected placeholder."
}
if ((Convert-TestComparablePathValue -Value $stagedPdfGateSummary.output_dir) -ne
    (Convert-TestComparablePathValue -Value $expectedSanitizedAbsolutePath)) {
    throw "staged PDF visual gate summary.json did not sanitize output_dir absolute path to the expected placeholder."
}
if ($stagedPdfGateSummary.cjk_copy_search.Count -ne 2) {
    throw "staged PDF visual gate summary.json lost CJK copy/search entries."
}
if ($stagedPdfGateSummary.baselines.Count -ne 3) {
    throw "staged PDF visual gate summary.json lost visual baseline entries."
}
foreach ($pdfEvidencePath in @($stagedPdfGateAggregateContactSheetPath, $stagedPdfGateUnicodeContactSheetPath)) {
    if (-not (Test-Path -LiteralPath $pdfEvidencePath)) {
        throw "Expected staged PDF visual gate evidence was not created: $pdfEvidencePath"
    }
}
if ([string]$manifest.pdf_visual_gate_status -ne "loaded") {
    throw "release_assets_manifest.json did not record pdf_visual_gate_status=loaded."
}
if (-not [bool]$manifest.pdf_visual_gate_evidence_included) {
    throw "release_assets_manifest.json did not record PDF visual gate evidence as included."
}
if ([string]$manifest.pdf_visual_gate_evidence.summary_json -ne $pdfGateSummaryPathDisplay) {
    throw "release_assets_manifest.json did not preserve the PDF visual gate summary path."
}
if ([string]$manifest.pdf_visual_gate_evidence.summary_json_display -ne $pdfGateSummaryPathDisplay) {
    throw "release_assets_manifest.json did not preserve the PDF visual gate summary display path."
}
if ([string]$manifest.pdf_visual_gate_evidence.verdict -ne "pass") {
    throw "release_assets_manifest.json lost the PDF visual gate verdict."
}
if ([string]$manifest.pdf_visual_gate_evidence.full_visual_gate_status -ne "pass") {
    throw "release_assets_manifest.json lost the full PDF visual gate status."
}
if ([string]$manifest.pdf_visual_gate_evidence.aggregate_contact_sheet -ne $pdfGateAggregateContactSheetPathDisplay) {
    throw "release_assets_manifest.json did not preserve the PDF visual gate aggregate contact sheet path."
}
if ([string]$manifest.pdf_visual_gate_evidence.aggregate_contact_sheet_display -ne $pdfGateAggregateContactSheetPathDisplay) {
    throw "release_assets_manifest.json lost the PDF visual gate aggregate contact sheet."
}
if ([string]$manifest.pdf_visual_gate_evidence.cjk_manifest_count -ne "43") {
    throw "release_assets_manifest.json lost the PDF CJK manifest sample count."
}
if ([string]$manifest.pdf_visual_gate_evidence.cjk_copy_search_count -ne "2") {
    throw "release_assets_manifest.json lost the PDF CJK copy/search sample count."
}
if ([string]$manifest.pdf_visual_gate_evidence.cjk_missing_text_count -ne "0") {
    throw "release_assets_manifest.json lost the PDF CJK missing text count."
}
if ([string]$manifest.pdf_visual_gate_evidence.visual_baseline_count -ne "3") {
    throw "release_assets_manifest.json lost the PDF visual baseline count."
}
if ([string]$manifest.pdf_visual_gate_evidence.visual_baseline_manifest_count -ne "42") {
    throw "release_assets_manifest.json lost the PDF visual baseline manifest sample count."
}
if (-not [bool]$manifest.pdf_bounded_ctest_evidence_included) {
    throw "release_assets_manifest.json did not record PDF bounded CTest evidence as included."
}
if ([string]$manifest.pdf_bounded_ctest_evidence.status -ne "pass") {
    throw "release_assets_manifest.json lost the bounded PDF CTest pass status."
}
if ([string]$manifest.pdf_bounded_ctest_evidence.summary_count -ne "7") {
    throw "release_assets_manifest.json lost the bounded PDF CTest summary_count."
}
if ([string]$manifest.pdf_bounded_ctest_evidence.pass_count -ne "7") {
    throw "release_assets_manifest.json lost the bounded PDF CTest pass_count."
}
if ([string]$manifest.pdf_bounded_ctest_evidence.selected_test_count -ne "70") {
    throw "release_assets_manifest.json lost the bounded PDF CTest selected_test_count."
}
if ([string]$manifest.pdf_bounded_ctest_evidence.skipped_test_count -ne "0") {
    throw "release_assets_manifest.json lost the bounded PDF CTest skipped_test_count."
}
$manifestPdfBoundedCtestSubsets = @($manifest.pdf_bounded_ctest_evidence.subsets | ForEach-Object { [string]$_ })
foreach ($expectedSubset in $pdfBoundedCtestSubsets) {
    if (-not ($manifestPdfBoundedCtestSubsets -contains $expectedSubset)) {
        throw "release_assets_manifest.json lost bounded PDF CTest subset '$expectedSubset'."
    }
}
$manifestPdfBoundedCtestSummaryJsonDisplay = @($manifest.pdf_bounded_ctest_evidence.summary_json_display | ForEach-Object { [string]$_ })
foreach ($expectedDisplay in $pdfBoundedCtestSummaryJsonDisplay) {
    if (-not ($manifestPdfBoundedCtestSummaryJsonDisplay -contains $expectedDisplay)) {
        throw "release_assets_manifest.json lost bounded PDF CTest summary display '$expectedDisplay'."
    }
}
$manifestPdfImportDiagnosticsContractTests = @($manifest.pdf_bounded_ctest_evidence.import_diagnostics_contract_tests |
    ForEach-Object { [string]$_ })
foreach ($expectedTest in $pdfImportDiagnosticsContractTests) {
    if (-not ($manifestPdfImportDiagnosticsContractTests -contains $expectedTest)) {
        throw "release_assets_manifest.json lost bounded PDF CTest import diagnostics contract test '$expectedTest'."
    }
}
Assert-PdfImportDiagnosticsContractFieldsPresent `
    -Actual @($manifest.pdf_bounded_ctest_evidence.import_diagnostics_contract_fields) `
    -MessagePrefix "release_assets_manifest.json lost bounded PDF CTest import diagnostics contract field."
$manifestPdfImportNegativeBoundaryContractCases = @($manifest.pdf_bounded_ctest_evidence.import_negative_boundary_contract_cases |
    ForEach-Object { [string]$_ })
foreach ($expectedCase in $pdfImportNegativeBoundaryContractCases) {
    if (-not ($manifestPdfImportNegativeBoundaryContractCases -contains $expectedCase)) {
        throw "release_assets_manifest.json lost bounded PDF CTest import negative boundary case '$expectedCase'."
    }
}
$wordVisualStandardReviewMetadata = @($manifest.word_visual_standard_review_metadata)
