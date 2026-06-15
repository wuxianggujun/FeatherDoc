function Get-PdfVisualGateRollupEvidence {
    param($RollupSummary)

    return @(
        foreach ($sourceReport in @(Get-JsonArray -Object $RollupSummary -Name "source_reports")) {
            $verdict = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_verdict"
            $status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_status"
            $segmentedStatus = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_status"
            $fullCtestReadinessStatus = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_status"
            $boundedCtestSummaryCount = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_bounded_ctest_summary_count")
            $boundedCtestImportDiagnosticsContractTests = @(Get-JsonArray -Object $sourceReport -Name "pdf_bounded_ctest_import_diagnostics_contract_tests")
            $boundedCtestImportDiagnosticsContractFields = @(Get-JsonArray -Object $sourceReport -Name "pdf_bounded_ctest_import_diagnostics_contract_fields")
            $boundedCtestImportNegativeBoundaryContractCases = @(Get-JsonArray -Object $sourceReport -Name "pdf_bounded_ctest_import_negative_boundary_contract_cases")
            if ([string]::IsNullOrWhiteSpace($verdict) -and
                [string]::IsNullOrWhiteSpace($status) -and
                [string]::IsNullOrWhiteSpace($segmentedStatus) -and
                [string]::IsNullOrWhiteSpace($fullCtestReadinessStatus) -and
                [string]::IsNullOrWhiteSpace([string]$boundedCtestSummaryCount) -and
                $boundedCtestImportDiagnosticsContractTests.Count -eq 0 -and
                $boundedCtestImportDiagnosticsContractFields.Count -eq 0 -and
                $boundedCtestImportNegativeBoundaryContractCases.Count -eq 0) {
                continue
            }

            [ordered]@{
                schema = Get-JsonString -Object $sourceReport -Name "schema"
                path_display = Get-JsonString -Object $sourceReport -Name "path_display"
                full_visual_gate_status = Get-JsonString -Object $sourceReport -Name "full_visual_gate_status"
                pdf_visual_gate_status = $status
                pdf_visual_gate_verdict = $verdict
                pdf_visual_gate_finalizable = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_finalizable")
                pdf_visual_gate_summary_json_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_summary_json_display"
                pdf_visual_gate_aggregate_contact_sheet_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_aggregate_contact_sheet_display"
                pdf_visual_gate_cjk_manifest_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_cjk_manifest_count")
                pdf_visual_gate_cjk_copy_search_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_cjk_copy_search_count")
                pdf_visual_gate_cjk_missing_text_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_cjk_missing_text_count")
                pdf_visual_gate_visual_baseline_manifest_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_visual_baseline_manifest_count")
                pdf_visual_gate_visual_baseline_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_visual_baseline_count")
                pdf_bounded_ctest_summary_count = $boundedCtestSummaryCount
                pdf_bounded_ctest_pass_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_bounded_ctest_pass_count")
                pdf_bounded_ctest_skipped_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_bounded_ctest_skipped_test_count")
                pdf_bounded_ctest_selected_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_bounded_ctest_selected_test_count")
                pdf_bounded_ctest_subsets = @(Get-JsonArray -Object $sourceReport -Name "pdf_bounded_ctest_subsets")
                pdf_bounded_ctest_summary_json_display = @(Get-JsonArray -Object $sourceReport -Name "pdf_bounded_ctest_summary_json_display")
                pdf_bounded_ctest_import_diagnostics_contract_tests = @($boundedCtestImportDiagnosticsContractTests)
                pdf_bounded_ctest_import_diagnostics_contract_fields = @($boundedCtestImportDiagnosticsContractFields)
                pdf_bounded_ctest_import_negative_boundary_contract_cases = @($boundedCtestImportNegativeBoundaryContractCases)
                pdf_full_ctest_readiness_requested = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_requested")
                pdf_full_ctest_readiness_status = $fullCtestReadinessStatus
                pdf_full_ctest_readiness_verdict = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_verdict"
                pdf_full_ctest_readiness_release_ready = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_release_ready")
                pdf_full_ctest_readiness_visual_gate_release_evidence_accepted = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_visual_gate_release_evidence_accepted")
                pdf_full_ctest_readiness_visual_gate_fresh_full_guarded_evidence = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_visual_gate_fresh_full_guarded_evidence")
                pdf_full_ctest_readiness_visual_gate_segmented_full_coverage_evidence = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_visual_gate_segmented_full_coverage_evidence")
                pdf_full_ctest_readiness_summary_json_display = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_summary_json_display"
                pdf_full_ctest_readiness_full_ctest_summary_json_display = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_full_ctest_summary_json_display"
                pdf_full_ctest_readiness_full_ctest_status = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_full_ctest_status"
                pdf_full_ctest_readiness_full_ctest_verdict = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_full_ctest_verdict"
                pdf_full_ctest_readiness_outer_guard_status = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_outer_guard_status"
                pdf_full_ctest_readiness_outer_guard_timed_out = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_outer_guard_timed_out")
                pdf_full_ctest_readiness_selected_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_selected_test_count")
                pdf_full_ctest_readiness_completed_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_completed_test_count")
                pdf_full_ctest_readiness_passed_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_passed_test_count")
                pdf_full_ctest_readiness_failed_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_failed_test_count")
                pdf_full_ctest_readiness_skipped_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_skipped_test_count")
                pdf_full_ctest_readiness_not_run_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_not_run_test_count")
                pdf_full_ctest_readiness_completion_percent = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_completion_percent")
                pdf_full_ctest_readiness_remaining_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_remaining_test_count")
                pdf_full_ctest_readiness_zero_failed_tests_observed = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_zero_failed_tests_observed")
                pdf_full_ctest_readiness_boundary = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_boundary"
                pdf_full_ctest_readiness_marker = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_marker"
                pdf_visual_gate_attempt_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_status"
                pdf_visual_gate_attempt_verdict = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_verdict"
                pdf_visual_gate_attempt_full_visual_gate_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_full_visual_gate_status"
                pdf_visual_gate_attempt_evidence_scope = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_evidence_scope"
                pdf_visual_gate_attempt_summary_json_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_summary_json_display"
                pdf_visual_gate_attempt_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_stage_count")
                pdf_visual_gate_attempt_passed_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_passed_stage_count")
                pdf_visual_gate_attempt_failed_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_failed_stage_count")
                pdf_visual_gate_attempt_incomplete_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_incomplete_stage_count")
                pdf_visual_gate_attempt_outer_guard_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_outer_guard_status"
                pdf_visual_gate_attempt_outer_guard_timed_out = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_outer_guard_timed_out")
                pdf_visual_gate_attempt_outer_guard_timeout_seconds = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_outer_guard_timeout_seconds")
                pdf_visual_gate_attempt_pdf_cli_export_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_pdf_cli_export_status"
                pdf_visual_gate_attempt_pdf_regression_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_pdf_regression_status"
                pdf_visual_gate_attempt_pdf_regression_selected_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_pdf_regression_selected_test_count")
                pdf_visual_gate_attempt_pdf_regression_failed_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_pdf_regression_failed_test_count")
                pdf_visual_gate_attempt_pdf_regression_skipped_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_pdf_regression_skipped_test_count")
                pdf_visual_gate_attempt_unicode_font_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_unicode_font_status"
                pdf_visual_gate_attempt_cjk_copy_search_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_cjk_copy_search_status"
                pdf_visual_gate_attempt_cjk_copy_search_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_cjk_copy_search_count")
                pdf_visual_gate_attempt_cjk_copy_search_missing_text_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_cjk_copy_search_missing_text_count")
                pdf_visual_gate_attempt_visual_baseline_render_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_visual_baseline_render_status"
                pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count")
                pdf_visual_gate_attempt_expected_visual_render_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_expected_visual_render_count")
                pdf_visual_gate_attempt_aggregate_contact_sheet_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_aggregate_contact_sheet_status"
                pdf_visual_gate_attempt_aggregate_contact_sheet_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_aggregate_contact_sheet_display"
                pdf_visual_segmented_gate_status = $segmentedStatus
                pdf_visual_segmented_gate_verdict = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_verdict"
                pdf_visual_segmented_gate_full_visual_gate_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_full_visual_gate_status"
                pdf_visual_segmented_gate_evidence_scope = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_evidence_scope"
                pdf_visual_segmented_gate_boundary = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_boundary"
                pdf_visual_segmented_gate_summary_json_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_summary_json_display"
                pdf_visual_segmented_gate_slice_summary_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_slice_summary_count")
                pdf_visual_segmented_gate_slice_pass_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_slice_pass_count")
                pdf_visual_segmented_gate_slice_failed_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_slice_failed_count")
                pdf_visual_segmented_gate_covered_baseline_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_covered_baseline_count")
                pdf_visual_segmented_gate_expected_visual_render_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_expected_visual_render_count")
                pdf_visual_segmented_gate_attempt_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_attempt_stage_count")
                pdf_visual_segmented_gate_attempt_passed_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_attempt_passed_stage_count")
                pdf_visual_segmented_gate_visual_baseline_render_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_visual_baseline_render_status"
                pdf_visual_segmented_gate_aggregate_contact_sheet_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_aggregate_contact_sheet_status"
                pdf_visual_segmented_gate_aggregate_contact_sheet_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_aggregate_contact_sheet_display"
                pdf_visual_segmented_gate_aggregate_contact_sheet_bytes = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_aggregate_contact_sheet_bytes")
                pdf_visual_segmented_gate_aggregate_rebuild_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_aggregate_rebuild_status"
                pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count")
            }
        }
    )
}

function Get-DocxFunctionalSmokeReadinessRollupEvidence {
    param($RollupSummary)

    return @(
        foreach ($sourceReport in @(Get-JsonArray -Object $RollupSummary -Name "source_reports")) {
            $schema = Get-JsonString -Object $sourceReport -Name "schema"
            if (-not [string]::Equals($schema, "featherdoc.docx_functional_smoke_readiness.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
                continue
            }

            [ordered]@{
                schema = $schema
                path_display = Get-JsonString -Object $sourceReport -Name "path_display"
                status = Get-JsonString -Object $sourceReport -Name "status"
                verdict = Get-JsonString -Object $sourceReport -Name "verdict"
                release_ready = Get-FirstJsonProperty -Object $sourceReport -Names @("release_ready")
                docx_functional_smoke_ready = Get-FirstJsonProperty -Object $sourceReport -Names @("docx_functional_smoke_ready")
                evidence_scope = Get-JsonString -Object $sourceReport -Name "evidence_scope"
                evidence_scope_note = Get-JsonString -Object $sourceReport -Name "evidence_scope_note"
                boundary = Get-JsonString -Object $sourceReport -Name "boundary"
                marker = Get-JsonString -Object $sourceReport -Name "marker"
                summary_json_display = Get-JsonString -Object $sourceReport -Name "summary_json_display"
                report_markdown_display = Get-JsonString -Object $sourceReport -Name "report_markdown_display"
            }
        }
    )
}

function Get-ManifestSignoffEntrypointsRollupEvidence {
    param($RollupSummary)

    return @(
        foreach ($sourceReport in @(Get-JsonArray -Object $RollupSummary -Name "source_reports")) {
            $status = Get-JsonString -Object $sourceReport -Name "manifest_signoff_entrypoints_status"
            $releaseAssetsManifest = Get-JsonString -Object $sourceReport -Name "manifest_signoff_entrypoints_release_assets_manifest"
            if ([string]::IsNullOrWhiteSpace($status) -and [string]::IsNullOrWhiteSpace($releaseAssetsManifest)) {
                continue
            }

            [ordered]@{
                schema = Get-JsonString -Object $sourceReport -Name "schema"
                path_display = Get-JsonString -Object $sourceReport -Name "path_display"
                manifest_signoff_entrypoints_status = $status
                manifest_signoff_entrypoints_release_assets_manifest = $releaseAssetsManifest
                manifest_signoff_entrypoints_release_assets_manifest_display = Get-JsonString -Object $sourceReport -Name "manifest_signoff_entrypoints_release_assets_manifest_display"
                manifest_signoff_entrypoints_required_entrypoint_count = Get-FirstJsonProperty -Object $sourceReport -Names @("manifest_signoff_entrypoints_required_entrypoint_count")
                manifest_signoff_entrypoints_entrypoint_ids = @(Get-JsonArray -Object $sourceReport -Name "manifest_signoff_entrypoints_entrypoint_ids")
                manifest_signoff_entrypoints_entrypoints = @(Get-JsonArray -Object $sourceReport -Name "manifest_signoff_entrypoints_entrypoints")
                manifest_signoff_entrypoints_required_contracts = @(Get-JsonArray -Object $sourceReport -Name "manifest_signoff_entrypoints_required_contracts")
                manifest_signoff_entrypoints_required_fields = @(Get-JsonArray -Object $sourceReport -Name "manifest_signoff_entrypoints_required_fields")
                manifest_signoff_entrypoints_checklist_marker = Get-JsonString -Object $sourceReport -Name "manifest_signoff_entrypoints_checklist_marker"
            }
        }
    )
}

function Get-ProjectTemplateReadinessChecklistEntrypointsRollupEvidence {
    param($RollupSummary)

    return @(
        foreach ($sourceReport in @(Get-JsonArray -Object $RollupSummary -Name "source_reports")) {
            $status = Get-JsonString -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_status"
            $checklistPath = Get-JsonString -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_checklist_path"
            if ([string]::IsNullOrWhiteSpace($status) -and [string]::IsNullOrWhiteSpace($checklistPath)) {
                continue
            }

            [ordered]@{
                schema = Get-JsonString -Object $sourceReport -Name "schema"
                path_display = Get-JsonString -Object $sourceReport -Name "path_display"
                project_template_readiness_checklist_entrypoints_status = $status
                project_template_readiness_checklist_entrypoints_checklist_label = Get-JsonString -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_checklist_label"
                project_template_readiness_checklist_entrypoints_checklist_path = $checklistPath
                project_template_readiness_checklist_entrypoints_required_entrypoint_count = Get-FirstJsonProperty -Object $sourceReport -Names @("project_template_readiness_checklist_entrypoints_required_entrypoint_count")
                project_template_readiness_checklist_entrypoints_entrypoint_ids = @(Get-JsonArray -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_entrypoint_ids")
                project_template_readiness_checklist_entrypoints_entrypoints = @(Get-JsonArray -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_entrypoints")
                project_template_readiness_checklist_entrypoints_checklist_marker = Get-JsonString -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_checklist_marker"
            }
        }
    )
}

function Get-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditRollupEvidence {
    param($RollupSummary)

    return @(
        foreach ($sourceReport in @(Get-JsonArray -Object $RollupSummary -Name "source_reports")) {
            $status = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_status"
            $marker = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
            if ([string]::IsNullOrWhiteSpace($status) -and [string]::IsNullOrWhiteSpace($marker)) {
                continue
            }

            [ordered]@{
                schema = Get-JsonString -Object $sourceReport -Name "schema"
                path_display = Get-JsonString -Object $sourceReport -Name "path_display"
                release_entry_project_template_readiness_checklist_material_safety_audit_status = $status
                release_entry_project_template_readiness_checklist_material_safety_audit_script = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_script"
                release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count = Get-FirstJsonProperty -Object $sourceReport -Names @("release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count")
                release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints = @(Get-JsonArray -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints")
                release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label"
                release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field"
                release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema"
                release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path"
                release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker"
                release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker = $marker
            }
        }
    )
}

function Get-WordVisualStandardReviewMetadataRollupEvidence {
    param($RollupSummary)

    return @(
        foreach ($sourceReport in @(Get-JsonArray -Object $RollupSummary -Name "source_reports")) {
            $metadataCount = Get-FirstJsonProperty -Object $sourceReport -Names @("word_visual_standard_review_metadata_count")
            $metadata = @(Get-JsonArray -Object $sourceReport -Name "word_visual_standard_review_metadata")
            if ($null -eq $metadataCount -and $metadata.Count -eq 0) {
                continue
            }

            [ordered]@{
                schema = Get-JsonString -Object $sourceReport -Name "schema"
                path_display = Get-JsonString -Object $sourceReport -Name "path_display"
                word_visual_standard_review_metadata_count = $metadataCount
                word_visual_standard_review_task_keys = @(Get-JsonArray -Object $sourceReport -Name "word_visual_standard_review_task_keys")
                word_visual_standard_review_status_summary = Get-JsonString -Object $sourceReport -Name "word_visual_standard_review_status_summary"
                word_visual_standard_review_verdict_summary = Get-JsonString -Object $sourceReport -Name "word_visual_standard_review_verdict_summary"
                word_visual_standard_review_metadata = @(
                    foreach ($entry in $metadata) {
                        $taskKey = Get-JsonString -Object $entry -Name "task_key"
                        if ([string]::IsNullOrWhiteSpace($taskKey)) {
                            continue
                        }

                        [ordered]@{
                            task_key = $taskKey
                            review_task_key = Get-JsonString -Object $entry -Name "review_task_key"
                            label = Get-JsonString -Object $entry -Name "label"
                            verdict = Get-JsonString -Object $entry -Name "verdict"
                            review_status = Get-JsonString -Object $entry -Name "review_status"
                            reviewed_at = Get-JsonString -Object $entry -Name "reviewed_at"
                            review_method = Get-JsonString -Object $entry -Name "review_method"
                            review_result_path = Get-JsonString -Object $entry -Name "review_result_path"
                            final_review_path = Get-JsonString -Object $entry -Name "final_review_path"
                        }
                    }
                )
            }
        }
    )
}
