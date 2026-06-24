    $inputRoot = Join-Path $resolvedWorkingDir "include-rollup-input"
    $explicitRoot = Join-Path $resolvedWorkingDir "include-rollup-explicit"
    $outputDir = Join-Path $resolvedWorkingDir "include-rollup-report"
    $releaseCandidateSummaryPath = Join-Path $explicitRoot "release-candidate-summary.json"
    $pdfPreflightGovernanceSummaryPath = Join-Path $explicitRoot "pdf-preflight-governance-summary.json"
    Write-GovernanceFixtures -Root $inputRoot
    Write-JsonFile -Path $releaseCandidateSummaryPath -Value ([ordered]@{
        project_template_smoke = [ordered]@{
            status = "ready"
        }
        status = "ready"
        release_ready = $true
        pdf_visual_gate_summary_json = "output/pdf-visual-release-gate-current/report/summary.json"
        manifest_signoff_entrypoints = [ordered]@{
            status = "declared"
            release_assets_manifest = "output\release-assets\v<version>\release_assets_manifest.json"
            required_entrypoint_count = 3
            entrypoints = @(
                [ordered]@{
                    id = "start_here"
                    path_display = ".\output\release-candidate-checks\START_HERE.md"
                    required = $true
                },
                [ordered]@{
                    id = "artifact_guide"
                    path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
                    required = $true
                },
                [ordered]@{
                    id = "reviewer_checklist"
                    path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
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
                "requires_reviewer_action",
                "reviewer_action_summary",
                "reviewer_action_reason",
                "reviewer_actions",
                "business_document_type_summary",
                "corpus_role_summary",
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
                    path_display = ".\output\release-candidate-checks\START_HERE.md"
                    required = $true
                },
                [ordered]@{
                    id = "artifact_guide"
                    path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
                    required = $true
                },
                [ordered]@{
                    id = "reviewer_checklist"
                    path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
                    required = $true
                }
            )
            checklist_marker = "release_entry_project_template_readiness_checklist_trace"
        }
        release_entry_project_template_readiness_checklist_material_safety_audit = [ordered]@{
            status = "passed"
            audit_script = ".\scripts\assert_release_material_safety.ps1"
            audited_entrypoint_count = 3
            audited_entrypoints = @("start_here", "artifact_guide", "reviewer_checklist")
            compact_evidence_label = "Project-template readiness checklist handoff evidence"
            compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
            compact_evidence_source_schema = "featherdoc.release_candidate_summary"
            checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
            checklist_marker = "release_entry_project_template_readiness_checklist_trace"
            material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
        }
        word_visual_standard_review_metadata_count = 4
        word_visual_standard_review_metadata = $wordVisualStandardReviewMetadata
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        steps = [ordered]@{
            pdf_visual_gate = [ordered]@{
                status = "loaded"
                verdict = "pass"
                finalizable = $true
                summary_json = "output/pdf-visual-release-gate-current/report/summary.json"
                aggregate_contact_sheet = "output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png"
                cjk_manifest_count = 43
                cjk_copy_search_count = 43
                cjk_missing_text_count = 0
                visual_baseline_manifest_count = 42
                visual_baseline_count = 44
            }
            pdf_bounded_ctest = [ordered]@{
                status = "pass"
                summary_count = 7
                pass_count = 7
                selected_test_count = 70
                skipped_test_count = 0
                subsets = @(
                    "smoke-import",
                    "contract-static",
                    "cjk-flow-static",
                    "regression-basic-text",
                    "regression-styled-document",
                    "regression-business-samples",
                    "regression-table-layout"
                )
                summary_json_display = @(
                    ".\build\pdf-ctest-bounded-subset-current\summary.json",
                    ".\build\pdf-ctest-bounded-contract-static-current\summary.json",
                    ".\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json",
                    ".\build\pdf-ctest-bounded-regression-basic-text-current\summary.json",
                    ".\build\pdf-ctest-bounded-regression-styled-document-current\summary.json",
                    ".\build\pdf-ctest-bounded-regression-business-samples-current\summary.json",
                    ".\build\pdf-ctest-bounded-regression-table-layout-current\summary.json"
                )
                import_diagnostics_contract_tests = @(
                    "pdf_cli_import",
                    "pdf_import_failure",
                    "pdf_import_table_heuristic"
                )
                import_diagnostics_contract_fields = @(Get-PdfImportDiagnosticsContractFields)
                import_negative_boundary_contract_cases = @(
                    "short_label_prose_remains_paragraphs",
                    "invoice_summary_form_remains_paragraphs"
                )
            }
            pdf_full_ctest_readiness = [ordered]@{
                requested = $true
                status = "pass"
                verdict = "pass_with_warnings"
                release_ready = $true
                summary_json = "output/pdf-release-readiness-current/summary.json"
                full_ctest_summary_json = "build/pdf-ctest-full-current/summary.json"
                full_ctest_summary_json_display = ".\build\pdf-ctest-full-current\summary.json"
                full_ctest_status = "timeout"
                full_ctest_verdict = "not_complete"
                outer_guard_status = "timed_out"
                outer_guard_timed_out = $true
                selected_test_count = 139
                completed_test_count = 133
                passed_test_count = 126
                failed_test_count = 0
                skipped_test_count = 7
                not_run_test_count = 6
                completion_percent = 95.7
                remaining_test_count = 6
                zero_failed_tests_observed = $true
                boundary = "full_ctest_timeout_is_not_release_blocking_when_zero_failures_observed"
                marker = "pdf_full_ctest_readiness_trace"
            }
            pdf_visual_gate_attempt = [ordered]@{
                status = "partial"
                verdict = "not_complete"
                full_visual_gate_status = "not_complete"
                evidence_scope = "bounded_attempt_auxiliary_only"
                summary_json = "output/pdf-visual-release-gate-current/report/attempt-summary.json"
                stage_count = 6
                passed_stage_count = 4
                failed_stage_count = 0
                incomplete_stage_count = 2
                outer_guard_status = "timed_out"
                outer_guard_timed_out = $true
                outer_guard_timeout_seconds = 60
                pdf_cli_export_status = "pass"
                pdf_regression_status = "pass"
                pdf_regression_selected_test_count = 91
                pdf_regression_failed_test_count = 0
                pdf_regression_skipped_test_count = 7
                unicode_font_status = "pass"
                cjk_copy_search_status = "pass"
                cjk_copy_search_count = 43
                cjk_copy_search_missing_text_count = 0
                visual_baseline_render_status = "partial"
                visual_baseline_fresh_rendered_count = 22
                expected_visual_render_count = 44
                visual_baseline_missing_pdf_count = 0
                visual_baseline_pdf_total_bytes = 7340032
                visual_baseline_png_page_count = 44
                visual_baseline_missing_png_page_count = 0
                visual_baseline_png_total_bytes = 2097152
                visual_baseline_unreadable_png_dimension_count = 0
                aggregate_contact_sheet_status = "stale"
                aggregate_contact_sheet = "output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png"
            }
            pdf_visual_segmented_gate = [ordered]@{
                status = "pass"
                verdict = "pass"
                full_visual_gate_status = "not_complete"
                evidence_scope = "segmented_visual_gate_auxiliary_only"
                boundary = "segmented_summary_does_not_replace_full_visual_gate_verdict"
                summary_json = "output/pdf-visual-release-gate-current/report/segmented-summary.json"
                slice_summary_count = 4
                slice_pass_count = 4
                slice_failed_count = 0
                covered_baseline_count = 44
                expected_visual_render_count = 44
                attempt_stage_count = 6
                attempt_passed_stage_count = 6
                visual_baseline_render_status = "pass"
                aggregate_contact_sheet_status = "pass"
                aggregate_contact_sheet = "output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png"
                aggregate_contact_sheet_bytes = 1822428
                aggregate_rebuild_status = "pass"
                aggregate_rebuild_selected_baseline_count = 44
            }
        }
    })
    Write-JsonFile -Path $pdfPreflightGovernanceSummaryPath -Value ([ordered]@{
        schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
        status = "blocked"
        release_ready = $false
        release_blocker_count = 1
        release_blockers = @(
            [ordered]@{
                id = "pdf_visual_release_gate_preflight.build_outputs_missing"
                severity = "error"
                status = "blocked"
                action = "prepare_pdf_visual_release_gate_build_outputs"
                message = "PDF visual release gate build outputs are missing."
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
            }
        )
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "prepare_pdf_visual_release_gate_build_outputs"
                action = "prepare_pdf_visual_release_gate_build_outputs"
                title = "Prepare PDF visual release gate build outputs"
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_pdf_visual_release_gate_preflight_governance_report.ps1"
            }
        )
        warning_count = 1
        warnings = @(
            [ordered]@{
                id = "pdf_controlled_visual_smoke.unavailable_or_failed"
                action = "rerun_pdf_controlled_visual_smoke_check"
                status = "fail"
                message = "Controlled PDF visual smoke evidence was provided but is not passing."
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\controlled-visual-smoke-failed.json"
            }
        )
    })
