    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Blocker source schemas:" `
        -Message "Handoff Markdown should expose nested rollup blocker source schema summary label."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1=1" `
        -Message "Handoff Markdown should expose nested rollup blocker source schema summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Action item source schemas:" `
        -Message "Handoff Markdown should expose nested rollup action item source schema summary label."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1=1" `
        -Message "Handoff Markdown should expose nested rollup action item source schema summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Informational action item source schemas: ``(none)``" `
        -Message "Handoff Markdown should expose empty nested rollup informational action source schema summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Warning source schemas:" `
        -Message "Handoff Markdown should expose nested rollup warning source schema summary label."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1=1" `
        -Message "Handoff Markdown should expose nested rollup warning source schema summary."
    Assert-ContainsText -Text $markdown -ExpectedText "DOCX functional smoke readiness evidence source reports: ``1``" `
        -Message "Handoff Markdown should expose the DOCX functional smoke readiness evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "evidence_scope: ``persisted_docx_functional_smoke_evidence_only``" `
        -Message "Handoff Markdown should expose the DOCX evidence scope."
    Assert-ContainsText -Text $markdown -ExpectedText "marker: ``docx_functional_smoke_readiness_trace``" `
        -Message "Handoff Markdown should expose the DOCX readiness marker."
    Assert-ContainsText -Text $markdown -ExpectedText "docx_functional_smoke_readiness.md" `
        -Message "Handoff Markdown should expose the DOCX readiness report markdown display."
    Assert-ContainsText -Text $markdown -ExpectedText "reviewer_actions: ``review_schema_update_candidate``" `
        -Message "Handoff Markdown should include reviewer action lists."
    Assert-ContainsText -Text $markdown -ExpectedText "PDF visual gate evidence source reports: ``1``" `
        -Message "Handoff Markdown should expose the PDF visual gate evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_verdict: ``pass``" `
        -Message "Handoff Markdown should expose the PDF visual gate verdict."
    Assert-ContainsText -Text $markdown -ExpectedText "full_visual_gate_status: ``pass``" `
        -Message "Handoff Markdown should expose the normalized full PDF visual gate conclusion."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_summary_json_display" `
        -Message "Handoff Markdown should expose the PDF visual gate summary display field."
    Assert-ContainsText -Text $markdown -ExpectedText "aggregate-contact-sheet.png" `
        -Message "Handoff Markdown should expose the PDF visual gate contact sheet."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_cjk_manifest_count: ``43``" `
        -Message "Handoff Markdown should expose the PDF CJK manifest count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_cjk_copy_search_count: ``43``" `
        -Message "Handoff Markdown should expose the PDF CJK copy/search count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_visual_baseline_manifest_count: ``42``" `
        -Message "Handoff Markdown should expose the PDF visual baseline manifest count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_visual_baseline_count: ``44``" `
        -Message "Handoff Markdown should expose the PDF visual baseline count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_bounded_ctest_summary_count: ``7``" `
        -Message "Handoff Markdown should expose the PDF bounded CTest summary count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_bounded_ctest_skipped_test_count: ``0``" `
        -Message "Handoff Markdown should expose the PDF bounded CTest skipped test count."
    Assert-ContainsText -Text $markdown -ExpectedText "regression-business-samples" `
        -Message "Handoff Markdown should expose the PDF bounded CTest subset names."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_bounded_ctest_import_diagnostics_contract_tests:" `
        -Message "Handoff Markdown should expose the PDF import diagnostics contract tests field."
    Assert-ContainsText -Text $markdown -ExpectedText "table_continuation_diagnostics=[]" `
        -Message "Handoff Markdown should expose the PDF import diagnostics contract field list."
    Assert-PdfImportDiagnosticsBlockerContractFieldsInText `
        -Text $markdown `
        -MessagePrefix "Handoff Markdown should expose PDF import diagnostic contract fields."
    Assert-ContainsText -Text $markdown -ExpectedText "short_label_prose_remains_paragraphs" `
        -Message "Handoff Markdown should expose the PDF import negative boundary contract cases."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_status: ``pass``" `
        -Message "Handoff Markdown should expose PDF full CTest readiness status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_verdict: ``pass_with_warnings``" `
        -Message "Handoff Markdown should expose PDF full CTest readiness verdict."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_full_ctest_status: ``timeout``" `
        -Message "Handoff Markdown should expose PDF full CTest observed status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_completed_test_count: ``133``" `
        -Message "Handoff Markdown should expose PDF full CTest completed count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_zero_failed_tests_observed: ``True``" `
        -Message "Handoff Markdown should expose PDF full CTest zero-failure observation."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_status: ``partial``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_verdict: ``not_complete``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt verdict."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt evidence scope."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_outer_guard_status: ``timed_out``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt outer guard status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_outer_guard_timed_out: ``True``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt outer guard timeout flag."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``60``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt outer guard timeout seconds."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt skipped count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt render status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_segmented_gate_status: ``pass``" `
        -Message "Handoff Markdown should expose segmented PDF visual gate status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``" `
        -Message "Handoff Markdown should expose segmented PDF visual gate full-status boundary."
    Assert-ContainsText -Text $markdown -ExpectedText "segmented_summary_does_not_replace_full_visual_gate_verdict" `
        -Message "Handoff Markdown should expose segmented PDF visual gate boundary."
    Assert-ContainsText -Text $markdown -ExpectedText "Manifest signoff entrypoints evidence source reports: ``1``" `
        -Message "Handoff Markdown should expose the manifest signoff evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "manifest_signoff_entrypoints_status: ``declared``" `
        -Message "Handoff Markdown should expose the manifest signoff status."
    Assert-ContainsText -Text $markdown -ExpectedText "manifest_signoff_entrypoints_release_assets_manifest_display" `
        -Message "Handoff Markdown should expose the release assets manifest display field."
    Assert-ContainsText -Text $markdown -ExpectedText "manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``" `
        -Message "Handoff Markdown should expose all manifest signoff entrypoint ids."
    Assert-ContainsText -Text $markdown -ExpectedText "reviewer_manifest_scoped_project_template_trace" `
        -Message "Handoff Markdown should expose the manifest signoff checklist marker."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" -ExpectedFragments @(
        "schema=``featherdoc.release_candidate_summary``",
        "manifest_signoff_entrypoints_status: ``declared``",
        "manifest_signoff_entrypoints_release_assets_manifest_display:",
        "release_assets_manifest.json",
        "manifest_signoff_entrypoints_required_entrypoint_count: ``3``",
        "manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``",
        "manifest_signoff_entrypoints_required_contracts: ``project_template_delivery_readiness_contract, project_template_onboarding_governance_contract``",
        "manifest_signoff_entrypoints_required_fields: ``status, release_ready, release_blocker_count, warning_count, schema_approval_status_summary, onboarding_governance_next_action, onboarding_governance_next_action_summary, onboarding_governance_next_action_group_count, next_action, next_action_summary, next_action_group_count, requires_reviewer_action, reviewer_action_summary, reviewer_action_reason, reviewer_actions, source_report_display, source_json_display``",
        "manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``"
    ) -Message "Handoff Markdown should keep manifest signoff evidence and release-candidate source identity in one source_report block."
    Assert-ContainsText -Text $markdown -ExpectedText "Project-template readiness checklist entrypoints evidence source reports: ``1``" `
        -Message "Handoff Markdown should expose the project-template readiness checklist evidence count."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "reviewer_action:" -ExpectedFragments @(
        "reviewer_action:",
        "reviewer_action_reason:",
        "reviewer_actions:"
    ) -Message "Handoff Markdown should keep reviewer action fields together."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_readiness_checklist_entrypoints_status: ``declared``" `
        -Message "Handoff Markdown should expose the project-template readiness checklist status."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``" `
        -Message "Handoff Markdown should expose the project-template readiness checklist label."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``" `
        -Message "Handoff Markdown should expose the project-template readiness checklist path."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``" `
        -Message "Handoff Markdown should expose all project-template readiness checklist entrypoint ids."
    Assert-ContainsText -Text $markdown -ExpectedText "release_entry_project_template_readiness_checklist_trace" `
        -Message "Handoff Markdown should expose the project-template readiness checklist marker."
    Assert-ContainsText -Text $markdown -ExpectedText "Release-entry project-template readiness checklist material-safety audit source reports: ``1``" `
        -Message "Handoff Markdown should expose release-entry checklist material-safety audit evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``" `
        -Message "Handoff Markdown should expose release-entry checklist material-safety audit status."
    Assert-ContainsText -Text $markdown -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``" `
        -Message "Handoff Markdown should expose release-entry checklist material-safety audited entrypoints."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace" `
        -Message "Handoff Markdown should expose release-entry checklist material-safety marker."
    Assert-ContainsText -Text $markdown -ExpectedText "Word visual standard review metadata source reports: ``1``" `
        -Message "Handoff Markdown should expose Word visual standard review metadata evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "word_visual_standard_review_metadata_count: ``4``" `
        -Message "Handoff Markdown should expose Word visual standard review metadata count."
    Assert-ContainsText -Text $markdown -ExpectedText "word_visual_standard_review_task_keys: ``smoke, fixed_grid, section_page_setup, page_number_fields``" `
        -Message "Handoff Markdown should expose Word visual standard review task keys."
    Assert-ContainsText -Text $markdown -ExpectedText "word_visual_standard_review_status_summary: ``reviewed=4``" `
        -Message "Handoff Markdown should expose Word visual standard review status summary."
    Assert-ContainsText -Text $markdown -ExpectedText "word_visual_standard_review_verdict_summary: ``pass=4``" `
        -Message "Handoff Markdown should expose Word visual standard review verdict summary."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "Word visual standard review metadata source reports" -ExpectedFragments @(
        "schema=``featherdoc.release_candidate_summary``",
        "``smoke``: review_task_key=``document`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``",
        "label: ``Word visual smoke``",
        "review_result.json",
        "final_review.md"
    ) -Message "Handoff Markdown should keep Word visual metadata and release-candidate source identity in one evidence block."
    Assert-DoesNotContainText -Text $markdown -UnexpectedText "review_note" `
        -Message "Handoff Markdown should not expose private Word visual review notes."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "schema=``featherdoc.docx_functional_smoke_readiness.v1``" -ExpectedFragments @(
        "status: ``loaded``",
        "verdict: ``pass``",
        "release_ready: ``True``",
        "docx_functional_smoke_ready: ``True``",
        "evidence_scope: ``persisted_docx_functional_smoke_evidence_only``",
        "does not run CMake, CTest, Word, LibreOffice, browsers, or document rendering",
        "does not claim a fresh Word COM render",
        "marker: ``docx_functional_smoke_readiness_trace``",
        "summary_json_display:",
        "docx-functional-smoke-readiness\summary.json",
        "report_markdown_display:",
        "docx_functional_smoke_readiness.md"
    ) -Message "Handoff Markdown should keep DOCX readiness evidence and source identity in one source_report block."
    $pdfSourceReportExpectedFragments = @(
        "schema=``featherdoc.release_candidate_summary``",
        "pdf_visual_gate_status: ``loaded``",
        "full_visual_gate_status: ``pass``",
        "pdf_visual_gate_verdict: ``pass``",
        "pdf_visual_gate_finalizable: ``True``",
        "pdf_visual_gate_summary_json_display:",
        "summary.json",
        "pdf_visual_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_gate_cjk_manifest_count: ``43``",
        "pdf_visual_gate_cjk_copy_search_count: ``43``",
        "pdf_visual_gate_visual_baseline_manifest_count: ``42``",
        "pdf_visual_gate_visual_baseline_count: ``44``",
        "pdf_bounded_ctest_summary_count: ``7``",
        "pdf_bounded_ctest_pass_count: ``7``",
        "pdf_bounded_ctest_skipped_test_count: ``0``",
        "pdf_bounded_ctest_selected_test_count: ``70``",
        "pdf_bounded_ctest_subsets:",
        "regression-table-layout",
        "pdf_bounded_ctest_summary_json_display:",
        "pdf-ctest-bounded-regression-business-samples-current\summary.json",
        "pdf_bounded_ctest_import_diagnostics_contract_tests:",
        "pdf_import_table_heuristic",
        "pdf_bounded_ctest_import_diagnostics_contract_fields:",
        "table_continuation_diagnostics=[]"
    ) + (Get-PdfImportDiagnosticsBlockerContractFields) + @(
        "pdf_bounded_ctest_import_negative_boundary_contract_cases:",
        "short_label_prose_remains_paragraphs",
        "pdf_full_ctest_readiness_status: ``pass``",
        "pdf_full_ctest_readiness_verdict: ``pass_with_warnings``",
        "pdf_full_ctest_readiness_release_ready: ``True``",
        "pdf_full_ctest_readiness_summary_json_display:",
        "pdf-release-readiness-current\summary.json",
        "pdf_full_ctest_readiness_full_ctest_summary_json_display:",
        "pdf-ctest-full-current\summary.json",
        "pdf_full_ctest_readiness_full_ctest_status: ``timeout``",
        "pdf_full_ctest_readiness_full_ctest_verdict: ``not_complete``",
        "pdf_full_ctest_readiness_selected_test_count: ``139``",
        "pdf_full_ctest_readiness_completed_test_count: ``133``",
        "pdf_full_ctest_readiness_failed_test_count: ``0``",
        "pdf_full_ctest_readiness_not_run_test_count: ``6``",
        "pdf_full_ctest_readiness_completion_percent: ``95.7``",
        "pdf_full_ctest_readiness_zero_failed_tests_observed: ``True``",
        "pdf_full_ctest_readiness_marker: ``pdf_full_ctest_readiness_trace``",
        "pdf_visual_gate_attempt_status: ``partial``",
        "pdf_visual_gate_attempt_verdict: ``not_complete``",
        "pdf_visual_gate_attempt_full_visual_gate_status: ``not_complete``",
        "pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``",
        "pdf_visual_gate_attempt_summary_json_display:",
        "attempt-summary.json",
        "pdf_visual_gate_attempt_outer_guard_status: ``timed_out``",
        "pdf_visual_gate_attempt_outer_guard_timed_out: ``True``",
        "pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``60``",
        "pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``91``",
        "pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``0``",
        "pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``",
        "pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``",
        "pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``22``",
        "pdf_visual_gate_attempt_expected_visual_render_count: ``44``",
        "pdf_visual_segmented_gate_status: ``pass``",
        "pdf_visual_segmented_gate_verdict: ``pass``",
        "pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``",
        "pdf_visual_segmented_gate_evidence_scope: ``segmented_visual_gate_auxiliary_only``",
        "pdf_visual_segmented_gate_boundary: ``segmented_summary_does_not_replace_full_visual_gate_verdict``",
        "pdf_visual_segmented_gate_summary_json_display:",
        "segmented-summary.json",
        "pdf_visual_segmented_gate_slice_summary_count: ``4``",
        "pdf_visual_segmented_gate_slice_pass_count: ``4``",
        "pdf_visual_segmented_gate_slice_failed_count: ``0``",
        "pdf_visual_segmented_gate_covered_baseline_count: ``44``",
        "pdf_visual_segmented_gate_expected_visual_render_count: ``44``",
        "pdf_visual_segmented_gate_attempt_stage_count: ``6``",
        "pdf_visual_segmented_gate_attempt_passed_stage_count: ``6``",
        "pdf_visual_segmented_gate_visual_baseline_render_status: ``pass``",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``pass``",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``1822428``",
        "pdf_visual_segmented_gate_aggregate_rebuild_status: ``pass``",
        "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count: ``44``"
    )
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" `
        -ExpectedFragments $pdfSourceReportExpectedFragments `
        -Message "Handoff Markdown should keep PDF visual gate source-report evidence in one source_report block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" -ExpectedFragments @(
        "schema=``featherdoc.release_candidate_summary``",
        "project_template_readiness_checklist_entrypoints_status: ``declared``",
        "project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``",
        "project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``",
        "project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``"
    ) -Message "Handoff Markdown should keep project-template checklist entrypoint evidence and release-candidate source identity in one source_report block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" -ExpectedFragments @(
        "schema=``featherdoc.release_candidate_summary``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``"
    ) -Message "Handoff Markdown should keep release-entry checklist material-safety audit evidence in one source_report block."
