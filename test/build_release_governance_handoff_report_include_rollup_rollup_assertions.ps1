    $rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$rollupSummary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Nested rollup should expose release blocker rollup schema."
    Assert-Equal -Actual ([int]$rollupSummary.source_report_count) -Expected 8 `
        -Message "Nested rollup should consume all loaded governance reports and explicit release-candidate evidence."
    Assert-Equal -Actual ([int]$rollupSummary.release_blocker_count) -Expected 5 `
        -Message "Nested rollup should preserve blocker count."
    Assert-Equal -Actual ([int]$rollupSummary.action_item_count) -Expected 6 `
        -Message "Nested rollup should preserve action item count."
    Assert-Equal -Actual ([int]$rollupSummary.governance_metric_count) -Expected 2 `
        -Message "Nested rollup should preserve governance metric count."
    Assert-Equal -Actual (
        ($summary.release_blocker_rollup.blocker_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Expected (
        ($rollupSummary.blocker_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Message "Handoff summary should mirror nested blocker source schema summary."
    Assert-Equal -Actual (
        ($summary.release_blocker_rollup.action_item_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Expected (
        ($rollupSummary.action_item_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Message "Handoff summary should mirror nested action item source schema summary."
    Assert-Equal -Actual (
        ($summary.release_blocker_rollup.warning_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Expected (
        ($rollupSummary.warning_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Message "Handoff summary should mirror nested warning source schema summary."
    Assert-ContainsText -Text (($rollupSummary.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n") `
        -ExpectedText "real_corpus_confidence:low:56" `
        -Message "Nested rollup should preserve numbering confidence metric."
    Assert-ContainsText -Text (($rollupSummary.governance_metrics | ForEach-Object { "$($_.id):$($_.report_id):$($_.source_schema)" }) -join "`n") `
        -ExpectedText "numbering_catalog_governance.real_corpus_confidence:numbering_catalog_governance:featherdoc.numbering_catalog_governance_report.v1" `
        -Message "Nested rollup should preserve numbering confidence metric contract."
    Assert-ContainsText -Text (($rollupSummary.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n") `
        -ExpectedText "delivery_quality:release_ready:100" `
        -Message "Nested rollup should preserve table delivery quality metric."
    $rollupNumberingMetric = ($rollupSummary.governance_metrics |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
        Select-Object -First 1)
    Assert-ContainsText -Text (($rollupNumberingMetric.details.catalog_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Nested rollup should preserve numbering catalog document keys."
    Assert-ContainsText -Text (($rollupNumberingMetric.details.baseline_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "invoice.docx" `
        -Message "Nested rollup should preserve numbering baseline document keys."
    Assert-ContainsText -Text (($rollupNumberingMetric.details.matched_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Nested rollup should preserve numbering matched document keys."
    Assert-ContainsText -Text (($rollupSummary.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Nested rollup should preserve calibration source schema."
    Assert-ContainsText -Text (($rollupSummary.release_blockers | ForEach-Object { [string]$_.project_id }) -join "`n") `
        -ExpectedText "project-finance" `
        -Message "Nested rollup should preserve calibration project id."
    Assert-ContainsText -Text (($rollupSummary.action_items | ForEach-Object { [string]$_.template_name }) -join "`n") `
        -ExpectedText "invoice-template" `
        -Message "Nested rollup should preserve calibration template name."
    Assert-ContainsText -Text (($rollupSummary.warnings | ForEach-Object { [string]$_.candidate_type }) -join "`n") `
        -ExpectedText "rename" `
        -Message "Nested rollup should preserve calibration candidate type."
    Assert-ContainsText -Text (($rollupSummary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "pdf_controlled_visual_smoke.unavailable_or_failed" `
        -Message "Nested rollup should preserve PDF preflight warnings."
    Assert-ContainsText -Text (($rollupSummary.warnings | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" `
        -Message "Nested rollup should preserve PDF preflight warning source schema."
    Assert-ContainsText -Text (($rollupSummary.warnings | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
        -ExpectedText "controlled-visual-smoke-failed.json" `
        -Message "Nested rollup should preserve PDF preflight warning source JSON display."
    $rollupReleaseCandidateSourceReport = ($rollupSummary.source_reports |
        Where-Object { [string]$_.schema -eq "featherdoc.release_candidate_summary" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.manifest_signoff_entrypoints_status) -Expected "declared" `
        -Message "Nested rollup should preserve manifest signoff status from release candidate summaries."
    Assert-ContainsText -Text (@($rollupReleaseCandidateSourceReport.manifest_signoff_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "start_here" `
        -Message "Nested rollup should preserve manifest signoff START_HERE entrypoint."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_status) -Expected "declared" `
        -Message "Nested rollup should preserve project-template readiness checklist entrypoint status."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_checklist_path) -Expected "docs/project_template_release_readiness_checklist_zh.rst" `
        -Message "Nested rollup should preserve project-template readiness checklist path."
    Assert-ContainsText -Text (@($rollupReleaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "start_here" `
        -Message "Nested rollup should preserve project-template readiness START_HERE entrypoint."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_status) -Expected "passed" `
        -Message "Nested rollup should preserve packaged release-entry checklist material-safety audit status."
    Assert-ContainsText -Text (@($rollupReleaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints) -join "`n") `
        -ExpectedText "artifact_guide" `
        -Message "Nested rollup should preserve packaged release-entry checklist material-safety audited entrypoints."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema) -Expected "featherdoc.release_candidate_summary" `
        -Message "Nested rollup should preserve packaged release-entry checklist material-safety compact evidence source schema."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_full_ctest_readiness_status) -Expected "pass" `
        -Message "Nested rollup should preserve PDF full CTest readiness status."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_full_ctest_readiness_full_ctest_status) -Expected "timeout" `
        -Message "Nested rollup should preserve PDF full CTest observed status."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_full_ctest_readiness_completed_test_count) -Expected 133 `
        -Message "Nested rollup should preserve PDF full CTest completed count."
    Assert-Equal -Actual ([bool]$rollupReleaseCandidateSourceReport.pdf_full_ctest_readiness_zero_failed_tests_observed) -Expected $true `
        -Message "Nested rollup should preserve PDF full CTest zero-failure observation."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_status) -Expected "partial" `
        -Message "Nested rollup should preserve PDF visual gate attempt status."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_verdict) -Expected "not_complete" `
        -Message "Nested rollup should preserve PDF visual gate attempt verdict."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_outer_guard_status) -Expected "timed_out" `
        -Message "Nested rollup should preserve PDF visual gate attempt outer guard status."
    Assert-Equal -Actual ([bool]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_outer_guard_timed_out) -Expected $true `
        -Message "Nested rollup should preserve PDF visual gate attempt outer guard timeout flag."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_outer_guard_timeout_seconds) -Expected 60 `
        -Message "Nested rollup should preserve PDF visual gate attempt outer guard timeout seconds."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_pdf_regression_skipped_test_count) -Expected 7 `
        -Message "Nested rollup should preserve PDF visual gate attempt skipped count."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count) -Expected 22 `
        -Message "Nested rollup should preserve PDF visual gate attempt fresh rendered count."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_segmented_gate_status) -Expected "pass" `
        -Message "Nested rollup should preserve segmented PDF visual gate status."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_segmented_gate_full_visual_gate_status) -Expected "not_complete" `
        -Message "Nested rollup should keep segmented evidence separate from full visual gate pass."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_visual_segmented_gate_covered_baseline_count) -Expected 44 `
        -Message "Nested rollup should preserve segmented PDF visual gate coverage."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.word_visual_standard_review_metadata_count) -Expected 4 `
        -Message "Nested rollup should preserve Word visual standard review metadata count."
    Assert-ContainsText -Text (@($rollupReleaseCandidateSourceReport.word_visual_standard_review_task_keys) -join "`n") `
        -ExpectedText "section_page_setup" `
        -Message "Nested rollup should preserve Word visual standard review task keys."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.word_visual_standard_review_status_summary) -Expected "reviewed=4" `
        -Message "Nested rollup should preserve Word visual standard review status summary."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.word_visual_standard_review_verdict_summary) -Expected "pass=4" `
        -Message "Nested rollup should preserve Word visual standard review verdict summary."
    $nestedWordVisualMetadata = @($rollupReleaseCandidateSourceReport.word_visual_standard_review_metadata)
    Assert-Equal -Actual $nestedWordVisualMetadata.Count -Expected 4 `
        -Message "Nested rollup should preserve four Word visual standard review metadata entries."
    $nestedSmokeWordVisualMetadata = $nestedWordVisualMetadata |
        Where-Object { [string]$_.task_key -eq "smoke" } |
        Select-Object -First 1
    Assert-Equal -Actual ([string]$nestedSmokeWordVisualMetadata.review_task_key) -Expected "document" `
        -Message "Nested rollup should preserve the smoke Word visual review task key."
    Assert-Equal -Actual ([string]$nestedSmokeWordVisualMetadata.review_status) -Expected "reviewed" `
        -Message "Nested rollup should preserve the smoke Word visual review status."
    Assert-ContainsText -Text ([string]$nestedSmokeWordVisualMetadata.review_result_path) `
        -ExpectedText "review_result.json" `
        -Message "Nested rollup should preserve the smoke Word visual review result path."
    Assert-DoesNotContainText -Text ($rollupSummary | ConvertTo-Json -Depth 32) -UnexpectedText "review_note" `
        -Message "Nested rollup summary should not expose private Word visual review notes."
