    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", (@($releaseCandidateSummaryPath, $pdfPreflightGovernanceSummaryPath) -join ","),
        "-OutputDir", $outputDir,
        "-IncludeReleaseBlockerRollup"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Include-rollup handoff should pass without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $rollupSummaryPath = Join-Path $outputDir "release-blocker-rollup\summary.json"
    $rollupMarkdownPath = Join-Path $outputDir "release-blocker-rollup\release_blocker_rollup.md"
    foreach ($path in @($summaryPath, $rollupSummaryPath, $rollupMarkdownPath)) {
        Assert-True -Condition (Test-Path -LiteralPath $path) `
            -Message "Include-rollup handoff should write artifact: $path"
    }

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([bool]$summary.release_blocker_rollup.included) -Expected $true `
        -Message "Handoff summary should record the included rollup."
    Assert-ContainsText -Text ([string]$summary.release_blocker_rollup.summary_json) -ExpectedText "release-blocker-rollup\summary.json" `
        -Message "Handoff summary should expose nested rollup summary display path."
    Assert-DoesNotContainText -Text ([string]$summary.release_blocker_rollup.summary_json) -UnexpectedText $resolvedRepoRoot `
        -Message "Handoff summary should not expose a local absolute nested rollup summary path."
    Assert-Equal -Actual ([string]$summary.release_blocker_rollup.status) -Expected "blocked" `
        -Message "Handoff summary should consume nested rollup status."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.source_report_count) -Expected 8 `
        -Message "Handoff summary should consume nested rollup source report count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.release_blocker_count) -Expected 5 `
        -Message "Handoff summary should consume nested rollup blocker count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.action_item_count) -Expected 6 `
        -Message "Handoff summary should consume nested rollup action item count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.warning_count) -Expected 3 `
        -Message "Handoff summary should consume nested rollup warning count."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.blocker_source_schema_summary | ForEach-Object { "$($_.source_schema):$($_.count)" }) -join "`n") `
        -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1:1" `
        -Message "Handoff summary should consume nested rollup blocker source schema summary."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.action_item_source_schema_summary | ForEach-Object { "$($_.source_schema):$($_.count)" }) -join "`n") `
        -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1:1" `
        -Message "Handoff summary should consume nested rollup action item source schema summary."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.warning_source_schema_summary | ForEach-Object { "$($_.source_schema):$($_.count)" }) -join "`n") `
        -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1:1" `
        -Message "Handoff summary should consume nested rollup warning source schema summary."
    Assert-Equal -Actual (@($summary.release_blocker_rollup.informational_action_item_source_schema_summary).Count) -Expected 0 `
        -Message "Handoff summary should keep empty nested rollup informational action source schema summary."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.governance_metric_count) -Expected 2 `
        -Message "Handoff summary should consume nested rollup governance metric count."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n") `
        -ExpectedText "real_corpus_confidence:low:56" `
        -Message "Handoff summary should consume nested numbering confidence metric."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n") `
        -ExpectedText "delivery_quality:release_ready:100" `
        -Message "Handoff summary should consume nested table delivery metric."
    $summaryNestedNumberingMetric = ($summary.release_blocker_rollup.governance_metrics |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
        Select-Object -First 1)
    Assert-ContainsText -Text (($summaryNestedNumberingMetric.details.catalog_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Handoff summary should preserve nested numbering catalog document keys."
    Assert-ContainsText -Text (($summaryNestedNumberingMetric.details.baseline_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "invoice.docx" `
        -Message "Handoff summary should preserve nested numbering baseline document keys."
    Assert-ContainsText -Text (($summaryNestedNumberingMetric.details.matched_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Handoff summary should preserve nested numbering matched document keys."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.docx_functional_smoke_readiness_evidence_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested DOCX functional smoke readiness evidence count."
    $docxRollupEvidence = $summary.release_blocker_rollup.docx_functional_smoke_readiness_evidence_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $docxRollupEvidence) `
        -Message "Handoff summary should expose at least one DOCX functional smoke readiness evidence source report."
    Assert-Equal -Actual ([string]$docxRollupEvidence.evidence_scope) -Expected "persisted_docx_functional_smoke_evidence_only" `
        -Message "Handoff summary should expose DOCX readiness evidence scope from the nested rollup."
    Assert-Equal -Actual ([string]$docxRollupEvidence.marker) -Expected "docx_functional_smoke_readiness_trace" `
        -Message "Handoff summary should expose DOCX readiness marker from the nested rollup."
    Assert-ContainsText -Text ([string]$docxRollupEvidence.summary_json_display) -ExpectedText "docx-functional-smoke-readiness\summary.json" `
        -Message "Handoff summary should expose DOCX readiness summary display from the nested rollup."
    Assert-ContainsText -Text ([string]$docxRollupEvidence.report_markdown_display) -ExpectedText "docx_functional_smoke_readiness.md" `
        -Message "Handoff summary should expose DOCX readiness report markdown display from the nested rollup."
    Assert-ContainsText -Text ([string]$docxRollupEvidence.boundary) -ExpectedText "does not claim a fresh Word COM render" `
        -Message "Handoff summary should expose DOCX readiness boundary from the nested rollup."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.pdf_visual_gate_evidence_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested PDF visual gate evidence count."
    $pdfEvidence = $summary.release_blocker_rollup.pdf_visual_gate_evidence_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $pdfEvidence) `
        -Message "Handoff summary should expose at least one PDF visual gate evidence source report."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_status) -Expected "loaded" `
        -Message "Handoff summary should expose PDF visual gate status from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.full_visual_gate_status) -Expected "pass" `
        -Message "Handoff summary should expose the normalized full PDF visual gate status from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_verdict) -Expected "pass" `
        -Message "Handoff summary should expose PDF visual gate verdict from the nested rollup."
    Assert-Equal -Actual ([bool]$pdfEvidence.pdf_visual_gate_finalizable) -Expected $true `
        -Message "Handoff summary should expose PDF visual gate finalizable state from the nested rollup."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_visual_gate_summary_json_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\summary.json" `
        -Message "Handoff summary should expose PDF visual gate summary display path from the nested rollup."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_visual_gate_aggregate_contact_sheet_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\aggregate-contact-sheet.png" `
        -Message "Handoff summary should expose PDF visual gate contact-sheet display path from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_cjk_manifest_count) -Expected 43 `
        -Message "Handoff summary should expose PDF CJK manifest count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_cjk_copy_search_count) -Expected 43 `
        -Message "Handoff summary should expose PDF CJK copy/search count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_visual_baseline_manifest_count) -Expected 42 `
        -Message "Handoff summary should expose PDF visual baseline manifest count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_visual_baseline_count) -Expected 44 `
        -Message "Handoff summary should expose PDF visual baseline count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_bounded_ctest_summary_count) -Expected 7 `
        -Message "Handoff summary should expose PDF bounded CTest summary count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_bounded_ctest_pass_count) -Expected 7 `
        -Message "Handoff summary should expose PDF bounded CTest pass count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_bounded_ctest_selected_test_count) -Expected 70 `
        -Message "Handoff summary should expose PDF bounded CTest selected test count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_bounded_ctest_skipped_test_count) -Expected 0 `
        -Message "Handoff summary should expose PDF bounded CTest skipped test count from the nested rollup."
    Assert-ContainsText -Text (@($pdfEvidence.pdf_bounded_ctest_subsets) -join ",") `
        -ExpectedText "regression-business-samples" `
        -Message "Handoff summary should expose PDF bounded CTest subset names from the nested rollup."
    Assert-ContainsText -Text (@($pdfEvidence.pdf_bounded_ctest_summary_json_display) -join ",") `
        -ExpectedText "pdf-ctest-bounded-regression-table-layout-current\summary.json" `
        -Message "Handoff summary should expose PDF bounded CTest summary display paths from the nested rollup."
    Assert-ContainsText -Text (@($pdfEvidence.pdf_bounded_ctest_import_diagnostics_contract_tests) -join ",") `
        -ExpectedText "pdf_import_table_heuristic" `
        -Message "Handoff summary should expose PDF import diagnostics contract tests from bounded CTest evidence."
    Assert-ContainsText -Text (@($pdfEvidence.pdf_bounded_ctest_import_diagnostics_contract_fields) -join ",") `
        -ExpectedText "table_continuation_diagnostics=[]" `
        -Message "Handoff summary should expose PDF import diagnostics contract fields from bounded CTest evidence."
    foreach ($expectedImportDiagnosticField in @(
        "source_row_offset=0",
        "skipped_repeating_header=false",
        "disposition=created_new_table",
        "blocker=repeated_header_mismatch",
        "blocker=column_count_mismatch",
        "blocker=column_anchors_mismatch",
        "blocker=continuation_confidence_below_threshold",
        "continuation_confidence=70",
        "continuation_confidence=55",
        "continuation_confidence=85",
        "continuation_confidence=30",
        "minimum_continuation_confidence=90",
        "column_count_matches=false",
        "column_anchors_match=false"
    )) {
        Assert-ContainsText -Text (@($pdfEvidence.pdf_bounded_ctest_import_diagnostics_contract_fields) -join ",") `
            -ExpectedText $expectedImportDiagnosticField `
            -Message "Handoff summary should expose PDF import diagnostic contract field '$expectedImportDiagnosticField'."
    }
    Assert-ContainsText -Text (@($pdfEvidence.pdf_bounded_ctest_import_negative_boundary_contract_cases) -join ",") `
        -ExpectedText "short_label_prose_remains_paragraphs" `
        -Message "Handoff summary should expose PDF import negative boundary cases from bounded CTest evidence."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_status) -Expected "pass" `
        -Message "Handoff summary should expose PDF full CTest readiness status from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_verdict) -Expected "pass_with_warnings" `
        -Message "Handoff summary should expose PDF full CTest readiness verdict from the nested rollup."
    Assert-Equal -Actual ([bool]$pdfEvidence.pdf_full_ctest_readiness_release_ready) -Expected $true `
        -Message "Handoff summary should expose PDF full CTest release readiness from the nested rollup."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_full_ctest_readiness_summary_json_display) `
        -ExpectedText "pdf-release-readiness-current\summary.json" `
        -Message "Handoff summary should expose PDF release readiness summary display path."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_full_ctest_readiness_full_ctest_summary_json_display) `
        -ExpectedText "pdf-ctest-full-current\summary.json" `
        -Message "Handoff summary should expose PDF full CTest summary display path."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_full_ctest_status) -Expected "timeout" `
        -Message "Handoff summary should expose PDF full CTest observed status."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_full_ctest_verdict) -Expected "not_complete" `
        -Message "Handoff summary should expose PDF full CTest verdict."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_outer_guard_status) -Expected "timed_out" `
        -Message "Handoff summary should expose PDF full CTest guard status."
    Assert-Equal -Actual ([bool]$pdfEvidence.pdf_full_ctest_readiness_outer_guard_timed_out) -Expected $true `
        -Message "Handoff summary should expose PDF full CTest guard timeout flag."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_full_ctest_readiness_selected_test_count) -Expected 139 `
        -Message "Handoff summary should expose PDF full CTest selected count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_full_ctest_readiness_completed_test_count) -Expected 133 `
        -Message "Handoff summary should expose PDF full CTest completed count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_full_ctest_readiness_failed_test_count) -Expected 0 `
        -Message "Handoff summary should expose PDF full CTest observed failure count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_full_ctest_readiness_not_run_test_count) -Expected 6 `
        -Message "Handoff summary should expose PDF full CTest not-run count."
    Assert-Equal -Actual ([double]$pdfEvidence.pdf_full_ctest_readiness_completion_percent) -Expected 95.7 `
        -Message "Handoff summary should expose PDF full CTest completion percent."
    Assert-Equal -Actual ([bool]$pdfEvidence.pdf_full_ctest_readiness_zero_failed_tests_observed) -Expected $true `
        -Message "Handoff summary should expose PDF full CTest zero-failure observation."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_marker) -Expected "pdf_full_ctest_readiness_trace" `
        -Message "Handoff summary should expose PDF full CTest readiness marker."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_status) -Expected "partial" `
        -Message "Handoff summary should expose PDF visual gate attempt status from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_verdict) -Expected "not_complete" `
        -Message "Handoff summary should expose PDF visual gate attempt verdict from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_full_visual_gate_status) -Expected "not_complete" `
        -Message "Handoff summary should keep partial attempts separate from the full visual gate status."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_evidence_scope) -Expected "bounded_attempt_auxiliary_only" `
        -Message "Handoff summary should expose PDF visual gate attempt evidence scope."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_visual_gate_attempt_summary_json_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\attempt-summary.json" `
        -Message "Handoff summary should expose PDF visual gate attempt summary display path."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_stage_count) -Expected 6 `
        -Message "Handoff summary should expose PDF visual gate attempt stage count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_passed_stage_count) -Expected 4 `
        -Message "Handoff summary should expose PDF visual gate attempt passed stage count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_incomplete_stage_count) -Expected 2 `
        -Message "Handoff summary should expose PDF visual gate attempt incomplete stage count."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_outer_guard_status) -Expected "timed_out" `
        -Message "Handoff summary should expose PDF visual gate attempt outer guard status."
    Assert-Equal -Actual ([bool]$pdfEvidence.pdf_visual_gate_attempt_outer_guard_timed_out) -Expected $true `
        -Message "Handoff summary should expose PDF visual gate attempt outer guard timeout flag."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_outer_guard_timeout_seconds) -Expected 60 `
        -Message "Handoff summary should expose PDF visual gate attempt outer guard timeout seconds."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_pdf_regression_selected_test_count) -Expected 91 `
        -Message "Handoff summary should expose PDF visual gate attempt pdf_regression selected count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_pdf_regression_failed_test_count) -Expected 0 `
        -Message "Handoff summary should expose PDF visual gate attempt pdf_regression failed count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_pdf_regression_skipped_test_count) -Expected 7 `
        -Message "Handoff summary should expose PDF visual gate attempt pdf_regression skipped count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_cjk_copy_search_count) -Expected 43 `
        -Message "Handoff summary should expose PDF visual gate attempt CJK count."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_visual_baseline_render_status) -Expected "partial" `
        -Message "Handoff summary should expose PDF visual gate attempt render status."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count) -Expected 22 `
        -Message "Handoff summary should expose PDF visual gate attempt fresh render count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_expected_visual_render_count) -Expected 44 `
        -Message "Handoff summary should expose PDF visual gate attempt expected render count."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_aggregate_contact_sheet_status) -Expected "stale" `
        -Message "Handoff summary should expose PDF visual gate attempt contact sheet status."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_status) -Expected "pass" `
        -Message "Handoff summary should expose segmented PDF visual gate status from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_verdict) -Expected "pass" `
        -Message "Handoff summary should expose segmented PDF visual gate verdict from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_full_visual_gate_status) -Expected "not_complete" `
        -Message "Handoff summary should keep segmented evidence separate from the full visual gate status."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_evidence_scope) -Expected "segmented_visual_gate_auxiliary_only" `
        -Message "Handoff summary should expose segmented PDF visual gate evidence scope."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_boundary) -Expected "segmented_summary_does_not_replace_full_visual_gate_verdict" `
        -Message "Handoff summary should expose segmented PDF visual gate boundary."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_visual_segmented_gate_summary_json_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\segmented-summary.json" `
        -Message "Handoff summary should expose segmented PDF visual gate summary display path."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_slice_summary_count) -Expected 4 `
        -Message "Handoff summary should expose segmented PDF visual gate slice count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_slice_pass_count) -Expected 4 `
        -Message "Handoff summary should expose segmented PDF visual gate passing slice count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_slice_failed_count) -Expected 0 `
        -Message "Handoff summary should expose segmented PDF visual gate failed slice count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_covered_baseline_count) -Expected 44 `
        -Message "Handoff summary should expose segmented PDF visual gate covered baseline count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_expected_visual_render_count) -Expected 44 `
        -Message "Handoff summary should expose segmented PDF visual gate expected baseline count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_attempt_stage_count) -Expected 6 `
        -Message "Handoff summary should expose segmented PDF visual gate attempt stage count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_attempt_passed_stage_count) -Expected 6 `
        -Message "Handoff summary should expose segmented PDF visual gate passed attempt stage count."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_visual_baseline_render_status) -Expected "pass" `
        -Message "Handoff summary should expose segmented PDF visual gate render status."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_aggregate_contact_sheet_status) -Expected "pass" `
        -Message "Handoff summary should expose segmented PDF visual gate contact-sheet status."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_visual_segmented_gate_aggregate_contact_sheet_display) `
        -ExpectedText "aggregate-contact-sheet.png" `
        -Message "Handoff summary should expose segmented PDF visual gate contact-sheet path."
    Assert-Equal -Actual ([int64]$pdfEvidence.pdf_visual_segmented_gate_aggregate_contact_sheet_bytes) -Expected 1822428 `
        -Message "Handoff summary should expose segmented PDF visual gate contact-sheet byte size."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_aggregate_rebuild_status) -Expected "pass" `
        -Message "Handoff summary should expose segmented PDF visual gate aggregate rebuild status."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count) -Expected 44 `
        -Message "Handoff summary should expose segmented PDF visual gate aggregate rebuild selected baseline count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.manifest_signoff_entrypoints_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested manifest signoff evidence count."
    $manifestSignoffEvidence = $summary.release_blocker_rollup.manifest_signoff_entrypoints_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $manifestSignoffEvidence) `
        -Message "Handoff summary should expose at least one manifest signoff evidence source report."
    Assert-Equal -Actual ([string]$manifestSignoffEvidence.schema) -Expected "featherdoc.release_candidate_summary" `
        -Message "Handoff summary should preserve manifest signoff release-candidate source schema."
    Assert-Equal -Actual ([string]$manifestSignoffEvidence.manifest_signoff_entrypoints_status) -Expected "declared" `
        -Message "Handoff summary should expose manifest signoff status from the nested rollup."
    Assert-ContainsText -Text ([string]$manifestSignoffEvidence.manifest_signoff_entrypoints_release_assets_manifest_display) `
        -ExpectedText "release_assets_manifest.json" `
        -Message "Handoff summary should expose release assets manifest display path from the nested rollup."
    Assert-Equal -Actual ([int]$manifestSignoffEvidence.manifest_signoff_entrypoints_required_entrypoint_count) -Expected 3 `
        -Message "Handoff summary should expose required manifest signoff entrypoint count."
    Assert-ContainsText -Text (@($manifestSignoffEvidence.manifest_signoff_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "reviewer_checklist" `
        -Message "Handoff summary should expose reviewer checklist manifest signoff entrypoint."
    Assert-ContainsText -Text (@($manifestSignoffEvidence.manifest_signoff_entrypoints_required_contracts) -join "`n") `
        -ExpectedText "project_template_delivery_readiness_contract" `
        -Message "Handoff summary should expose manifest signoff required project-template delivery contract."
    $manifestSignoffRequiredFieldsText = @($manifestSignoffEvidence.manifest_signoff_entrypoints_required_fields) -join "`n"
    foreach ($requiredField in @(
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
    )) {
        Assert-ContainsText -Text $manifestSignoffRequiredFieldsText `
            -ExpectedText $requiredField `
            -Message "Handoff summary should expose manifest signoff required field '$requiredField'."
    }
    Assert-Equal -Actual ([string]$manifestSignoffEvidence.manifest_signoff_entrypoints_checklist_marker) -Expected "reviewer_manifest_scoped_project_template_trace" `
        -Message "Handoff summary should expose manifest signoff reviewer checklist marker."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.project_template_readiness_checklist_entrypoints_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested project-template readiness checklist entrypoints evidence count."
    $projectTemplateChecklistEvidence = $summary.release_blocker_rollup.project_template_readiness_checklist_entrypoints_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $projectTemplateChecklistEvidence) `
        -Message "Handoff summary should expose at least one project-template readiness checklist evidence source report."
    Assert-Equal -Actual ([string]$projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_status) -Expected "declared" `
        -Message "Handoff summary should expose project-template readiness checklist status from the nested rollup."
    Assert-Equal -Actual ([string]$projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_checklist_label) -Expected "Project template release readiness checklist" `
        -Message "Handoff summary should expose project-template readiness checklist label from the nested rollup."
    Assert-Equal -Actual ([string]$projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_checklist_path) -Expected "docs/project_template_release_readiness_checklist_zh.rst" `
        -Message "Handoff summary should expose project-template readiness checklist path from the nested rollup."
    Assert-Equal -Actual ([int]$projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_required_entrypoint_count) -Expected 3 `
        -Message "Handoff summary should expose project-template readiness checklist required entrypoint count."
    Assert-ContainsText -Text (@($projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "reviewer_checklist" `
        -Message "Handoff summary should expose reviewer checklist project-template readiness entrypoint."
    Assert-Equal -Actual ([string]$projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_checklist_marker) -Expected "release_entry_project_template_readiness_checklist_trace" `
        -Message "Handoff summary should expose project-template readiness checklist marker."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested release-entry checklist material-safety audit evidence count."
    $releaseEntryChecklistAuditEvidence = $summary.release_blocker_rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $releaseEntryChecklistAuditEvidence) `
        -Message "Handoff summary should expose at least one release-entry checklist material-safety audit source report."
    Assert-Equal -Actual ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_status) -Expected "passed" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety audit status."
    Assert-ContainsText -Text ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_script) `
        -ExpectedText "assert_release_material_safety.ps1" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety audit script."
    Assert-ContainsText -Text (@($releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints) -join "`n") `
        -ExpectedText "reviewer_checklist" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety audited entrypoints."
    Assert-Equal -Actual ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field) -Expected "project_template_readiness_checklist_entrypoints_source_reports" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety compact evidence field."
    Assert-Equal -Actual ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema) -Expected "featherdoc.release_candidate_summary" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety compact evidence source schema."
    Assert-Equal -Actual ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker) -Expected "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety marker."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.word_visual_standard_review_metadata_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested Word visual standard review metadata evidence count."
    $wordVisualMetadataEvidence = $summary.release_blocker_rollup.word_visual_standard_review_metadata_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $wordVisualMetadataEvidence) `
        -Message "Handoff summary should expose at least one Word visual standard review metadata source report."
    Assert-Equal -Actual ([string]$wordVisualMetadataEvidence.schema) -Expected "featherdoc.release_candidate_summary" `
        -Message "Handoff summary should preserve Word visual metadata release-candidate source schema."
    Assert-Equal -Actual ([int]$wordVisualMetadataEvidence.word_visual_standard_review_metadata_count) -Expected 4 `
        -Message "Handoff summary should expose Word visual standard review metadata count."
    Assert-ContainsText -Text (@($wordVisualMetadataEvidence.word_visual_standard_review_task_keys) -join "`n") `
        -ExpectedText "page_number_fields" `
        -Message "Handoff summary should expose Word visual standard review task keys."
    Assert-Equal -Actual ([string]$wordVisualMetadataEvidence.word_visual_standard_review_status_summary) -Expected "reviewed=4" `
        -Message "Handoff summary should expose Word visual standard review status summary."
    Assert-Equal -Actual ([string]$wordVisualMetadataEvidence.word_visual_standard_review_verdict_summary) -Expected "pass=4" `
        -Message "Handoff summary should expose Word visual standard review verdict summary."
    $handoffWordVisualMetadata = @($wordVisualMetadataEvidence.word_visual_standard_review_metadata)
    Assert-Equal -Actual $handoffWordVisualMetadata.Count -Expected 4 `
        -Message "Handoff summary should expose four Word visual standard review metadata entries."
    $handoffWordVisualMetadataByTask = @{}
    foreach ($entry in $handoffWordVisualMetadata) {
        $handoffWordVisualMetadataByTask[[string]$entry.task_key] = $entry
        Assert-True -Condition ($null -eq $entry.PSObject.Properties["review_note"]) `
            -Message "Handoff summary should not expose Word visual standard review notes."
    }
    $handoffSmokeWordVisualMetadata = $handoffWordVisualMetadataByTask["smoke"]
    Assert-Equal -Actual ([string]$handoffSmokeWordVisualMetadata.review_task_key) -Expected "document" `
        -Message "Handoff summary should preserve the smoke Word visual review task key."
    Assert-Equal -Actual ([string]$handoffSmokeWordVisualMetadata.label) -Expected "Word visual smoke" `
        -Message "Handoff summary should preserve the smoke Word visual review label."
    Assert-Equal -Actual ([string]$handoffSmokeWordVisualMetadata.verdict) -Expected "pass" `
        -Message "Handoff summary should preserve the smoke Word visual review verdict."
    Assert-Equal -Actual ([string]$handoffSmokeWordVisualMetadata.review_status) -Expected "reviewed" `
        -Message "Handoff summary should preserve the smoke Word visual review status."
    Assert-Equal -Actual ([string]$handoffSmokeWordVisualMetadata.review_method) -Expected "operator_supplied" `
        -Message "Handoff summary should preserve the smoke Word visual review method."
    Assert-ContainsText -Text ([string]$handoffSmokeWordVisualMetadata.review_result_path) `
        -ExpectedText "review_result.json" `
        -Message "Handoff summary should preserve the smoke Word visual review result path."
    Assert-ContainsText -Text ([string]$handoffSmokeWordVisualMetadata.final_review_path) `
        -ExpectedText "final_review.md" `
        -Message "Handoff summary should preserve the smoke Word visual final review path."
    Assert-DoesNotContainText -Text ($summary | ConvertTo-Json -Depth 32) -UnexpectedText "review_note" `
        -Message "Handoff summary JSON should not expose private Word visual review notes."
