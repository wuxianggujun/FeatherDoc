$fixtureRoot = Join-Path $resolvedWorkingDir "fixtures"
$documentSkeletonPath = Join-Path $fixtureRoot "document-skeleton\document_skeleton_governance.summary.json"
$numberingGovernancePath = Join-Path $fixtureRoot "numbering-governance\summary.json"
$tableLayoutPath = Join-Path $fixtureRoot "table-layout\summary.json"
$contentControlPath = Join-Path $fixtureRoot "content-control\summary.json"
$projectTemplateReadinessPath = Join-Path $fixtureRoot "project-template-readiness\summary.json"
$schemaCalibrationPath = Join-Path $fixtureRoot "schema-patch-confidence-calibration\summary.json"
$releaseCandidatePath = Join-Path $fixtureRoot "release-candidate\summary.json"
$pdfPreflightGovernancePath = Join-Path $fixtureRoot "pdf-preflight-governance\summary.json"
$styleGovernancePath = Join-Path $fixtureRoot "style-governance\summary.json"
$emptyPath = Join-Path $fixtureRoot "empty\summary.json"
$malformedPath = Join-Path $fixtureRoot "malformed\summary.json"
$failedSourcePath = Join-Path $fixtureRoot "failed-source\summary.json"
$dedupePath = Join-Path $fixtureRoot "dedupe\summary.json"

Write-JsonFile -Path $documentSkeletonPath -Value ([ordered]@{
    schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "document_skeleton.style_numbering_issues"
            severity = "error"
            status = "needs_review"
            message = "Style numbering audit reported issues."
            action = "review_style_numbering_audit"
            source_report = "output/document-skeleton-governance-rollup/summary.json"
            source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
            source_json = "output/document-skeleton-governance/contract/style-numbering-audit.json"
            source_json_display = ".\output\document-skeleton-governance\contract\style-numbering-audit.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "preview_style_numbering_repair"
            title = "Preview style numbering repair"
            command = "featherdoc_cli repair-style-numbering input.docx --plan-only --json"
        }
    )
    warning_count = 1
    warnings = @(
        [ordered]@{
            id = "document_skeleton.exemplar_catalog_missing"
            action = "open_document_skeleton_rollup"
            message = "One exemplar catalog path is missing."
            source_report = "output/document-skeleton-governance-rollup/summary.json"
            source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
            source_json = "output/document-skeleton-governance-rollup/summary.json"
            source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
        }
    )
})

Write-JsonFile -Path $numberingGovernancePath -Value ([ordered]@{
    schema = "featherdoc.numbering_catalog_governance_report.v1"
    status = "needs_review"
    release_ready = $false
    real_corpus_confidence_score = 56
    real_corpus_confidence_level = "low"
    real_corpus_confidence = [ordered]@{
        score = 56
        level = "low"
        matched_document_count = 2
        unmatched_catalog_document_count = 0
        unmatched_baseline_document_count = 0
        alignment_gap_count = 0
        catalog_coverage_percent = 100
        baseline_coverage_percent = 100
        catalog_document_keys = @("contract.docx", "invoice.docx")
        baseline_document_keys = @("contract.docx", "invoice.docx")
        matched_document_keys = @("contract.docx", "invoice.docx")
        penalty_summary = @(
            [ordered]@{ factor = "style_numbering_issues"; count = 3; penalty = 15 }
        )
    }
    release_blocker_count = 0
    release_blockers = @()
    action_items = @()
})

Write-JsonFile -Path $tableLayoutPath -Value ([ordered]@{
    schema = "featherdoc.table_layout_delivery_governance_report.v1"
    delivery_quality_score = 0
    delivery_quality_level = "blocked"
    delivery_quality = [ordered]@{
        score = 0
        level = "blocked"
        document_count = 2
        ready_document_count = 1
        ready_document_percent = 50
        needs_review_document_count = 1
        failed_document_count = 0
        table_style_issue_count = 3
        automatic_tblLook_fix_count = 2
        manual_table_style_fix_count = 1
        table_position_automatic_count = 2
        table_position_review_count = 1
        pdf_floating_table_supported_geometry_count = 4
        pdf_floating_table_metadata_only_count = 5
        pdf_floating_table_tracked_geometry_count = 9
        pdf_floating_table_supported_geometry_percent = 44
        pdf_floating_table_support_coverage = "4/9 supported (44 percent); metadata_only=5"
        pdf_floating_table_reviewer_focus = "review metadata-only tblpPr fields before approving PDF-layout-sensitive release."
        pdf_floating_table_metadata_only_fields = @(
            "leftFromText",
            "rightFromText",
            "topFromText outside paragraph anchoring",
            "tblOverlap"
        )
        pdf_floating_table_review_required_fields = @(
            "full Word-compatible floating table text wrapping",
            "table overlap avoidance and collision resolution"
        )
        metadata_only_fields = @(
            "leftFromText",
            "rightFromText",
            "topFromText outside paragraph anchoring",
            "tblOverlap"
        )
        review_required_fields = @(
            "full Word-compatible floating table text wrapping",
            "table overlap avoidance and collision resolution"
        )
        command_failure_count = 0
        unresolved_item_count = 10
        penalty_summary = @(
            [ordered]@{ factor = "safe_tblLook_fixes_pending"; count = 2; penalty = 8 },
            [ordered]@{ factor = "floating_table_plans_pending"; count = 3; penalty = 14 }
        )
    }
    release_blocker_count = 2
    release_blockers = @(
        [ordered]@{
            id = "table_layout.manual_table_style_quality_work"
            severity = "warning"
            message = "Manual table style work remains."
            action = "review_table_style_quality_plan"
            repair_strategy = "review_source_table_style_quality_plan"
            repair_hint = "Review the source rollup style quality findings before release."
            command_template = "featherdoc_cli inspect-table-style <input.docx> <style-id> --json"
        },
        [ordered]@{
            id = "table_layout.positioned_tables_need_review"
            severity = "warning"
            message = "Existing floating positions need review."
            action = "review_table_position_plan"
            repair_strategy = "review_source_table_position_plan"
            repair_hint = "Review source table-position.plan.json entries with existing floating positions."
            command_template = "featherdoc_cli apply-table-position-plan <table-position.plan.json> --dry-run --json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_table_position_preset"
            action = "review_table_position_preset"
            title = "Review table position preset"
            command = "featherdoc_cli apply-table-position-plan plan.json --dry-run --json"
            repair_strategy = "dry_run_table_position_plan"
            repair_hint = "Dry-run the table position plan before applying floating-position presets."
            command_template = "featherdoc_cli apply-table-position-plan <table-position.plan.json> --dry-run --json"
        }
    )
})

Write-JsonFile -Path $styleGovernancePath -Value ([ordered]@{
    schema = "featherdoc.style_catalog_governance_report.v1"
    real_corpus_confidence_score = 12
    real_corpus_confidence_level = "experimental"
    real_corpus_confidence = [ordered]@{
        score = 12
        level = "experimental"
    }
    release_blocker_count = 0
    release_blockers = @()
    action_items = @()
})

Write-JsonFile -Path $contentControlPath -Value ([ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "error"
            status = "placeholder_visible"
            message = "A data-bound content control is still showing placeholder text."
            action = "sync_or_fill_bound_content_control"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
            source_json = "output/content-control-data-binding/inspect-content-controls.json"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            input_docx = "samples/invoice.docx"
            input_docx_display = ".\samples\invoice.docx"
            template_name = "invoice-template"
            schema_target = "invoice"
            target_mode = "resolved-section-targets"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_content_control_lock_strategy"
            action = "review_content_control_lock_strategy"
            title = "Review lock state for data-bound content control"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
            repair_strategy = "review_lock_state"
            repair_hint = "Confirm whether the lock is intentional; clear it only if template data should overwrite this control."
            command_template = "featherdoc_cli set-content-control-form-state <input.docx> --tag due_date --clear-lock --output <reviewed.docx> --json"
            source_json = "output/content-control-data-binding/inspect-content-controls.json"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            input_docx = "samples/invoice.docx"
            input_docx_display = ".\samples\invoice.docx"
            template_name = "invoice-template"
            schema_target = "invoice"
            target_mode = "resolved-section-targets"
        }
    )
})

Write-JsonFile -Path $projectTemplateReadinessPath -Value ([ordered]@{
    schema = "featherdoc.project_template_delivery_readiness_report.v1"
    status = "blocked"
    release_ready = $false
    latest_schema_approval_gate_status = "pending"
    schema_approval_status_summary = @(
        [ordered]@{
            status = "approved"
            count = 1
        },
        [ordered]@{
            status = "pending_review"
            count = 1
        }
    )
    business_document_type_summary = @(
        [ordered]@{
            document_type = "invoice"
            count = 1
        },
        [ordered]@{
            document_type = "policy"
            count = 1
        }
    )
    corpus_role_summary = @(
        [ordered]@{
            corpus_role = "planned-business-template"
            count = 1
        },
        [ordered]@{
            corpus_role = "registered-business-template"
            count = 1
        }
    )
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_onboarding.schema_approval"
            severity = "error"
            status = "pending_review"
            message = "Schema approval is pending."
            action = "review_schema_update_candidate"
            source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
            source_report = "output/project-template-onboarding-governance/summary.json"
            source_report_display = ".\output\project-template-onboarding-governance\summary.json"
            source_json_display = ".\output\project-template-onboarding-governance\summary.json"
            requires_reviewer_action = $true
            reviewer_action_summary = "review_schema_update_candidate"
            reviewer_action_reason = "latest_review_state=pending; issue_keys=(none)"
            reviewer_actions = @("review_schema_update_candidate")
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_project_template_schema_approval"
            action = "review_schema_update_candidate"
            title = "Review project template schema approval"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1"
            source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
            source_report = "output/project-template-onboarding-governance/summary.json"
            source_report_display = ".\output\project-template-onboarding-governance\summary.json"
            source_json_display = ".\output\project-template-onboarding-governance\summary.json"
            requires_reviewer_action = $true
            reviewer_action_summary = "review_schema_update_candidate"
            reviewer_action_reason = "latest_review_state=pending; issue_keys=(none)"
            reviewer_actions = @("review_schema_update_candidate")
        }
    )
})

Write-JsonFile -Path $schemaCalibrationPath -Value ([ordered]@{
    schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
    status = "pending_review"
    release_ready = $false
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "schema_patch_confidence_calibration.pending_schema_approvals"
            severity = "error"
            status = "pending_review"
            action = "resolve_pending_schema_approvals"
            message = "Schema patch confidence calibration has pending approvals."
            project_id = "project-finance"
            template_name = "invoice-template"
            candidate_type = "rename"
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_report = "output/schema-patch-confidence-calibration/summary.json"
            source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        }
    )
    action_item_count = 2
    action_items = @(
        [ordered]@{
            id = "resolve_pending_schema_approvals"
            action = "resolve_pending_schema_approvals"
            title = "Resolve pending schema approvals"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
            project_id = "project-finance"
            template_name = "invoice-template"
            candidate_type = "rename"
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_report = "output/schema-patch-confidence-calibration/summary.json"
            source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        },
        [ordered]@{
            id = "align_business_template_corpus_metadata"
            action = "align_business_template_corpus_metadata"
            title = "Align schema patch candidate corpus metadata"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
            project_id = "project-office"
            template_name = "office-notice-template"
            business_document_type = "invoice"
            source_business_document_type = "notice"
            corpus_role = "experimental-business-template"
            source_corpus_role = "registered-business-template"
            candidate_type = "add"
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_report = "output/schema-patch-confidence-calibration/summary.json"
            source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        }
    )
    warning_count = 2
    warnings = @(
        [ordered]@{
            id = "schema_patch_confidence_calibration.unscored_candidates"
            action = "add_explicit_confidence_metadata"
            message = "Some schema patch candidates do not carry explicit confidence metadata."
            project_id = "project-finance"
            template_name = "invoice-template"
            candidate_type = "rename"
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_report = "output/schema-patch-confidence-calibration/summary.json"
            source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        },
        [ordered]@{
            id = "schema_patch_confidence_calibration.mismatched_business_template_corpus_metadata"
            action = "align_business_template_corpus_metadata"
            message = "Some schema patch candidates disagree with their source business template corpus metadata."
            mismatched_corpus_metadata_count = 1
            business_document_type = "invoice"
            source_business_document_type = "notice"
            corpus_role = "experimental-business-template"
            source_corpus_role = "registered-business-template"
            business_document_type_mismatch = $true
            corpus_role_mismatch = $true
            project_id = "project-office"
            template_name = "office-notice-template"
            candidate_type = "add"
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_report = "output/schema-patch-confidence-calibration/summary.json"
            source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        }
    )
})

Write-JsonFile -Path $releaseCandidatePath -Value ([ordered]@{
    schema = "featherdoc.release_candidate_summary"
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
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_smoke.schema_approval"
            severity = "error"
            status = "blocked"
            message = "Schema approval blocks release."
            action = "fix_schema_patch_approval_result"
            source_report = "output/release-candidate/summary.json"
            source_report_display = ".\output\release-candidate\summary.json"
            source_json_display = ".\output\release-candidate\summary.json"
        }
    )
    action_items = @()
    steps = [ordered]@{
        pdf_visual_gate = [ordered]@{
            status = "loaded"
            verdict = "pass"
            finalizable = $true
            summary_json = "output/pdf-visual-release-gate-current/report/summary.json"
            aggregate_contact_sheet = "output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png"
            cjk_manifest_count = 43
            cjk_copy_search_count = 43
            cjk_copy_search_missing_text_count = 0
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

Write-JsonFile -Path $pdfPreflightGovernancePath -Value ([ordered]@{
    schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
    status = "blocked"
    release_ready = $false
    preflight_ready = $false
    full_visual_gate_required = $true
    full_visual_gate_status = "not_run_by_preflight_governance"
    controlled_visual_smoke_available = $true
    controlled_visual_smoke_status = "pass"
    controlled_visual_smoke_passed = $true
    controlled_visual_smoke_case_count = 2
    controlled_visual_smoke_json = "output/pdf-controlled-visual-smoke/controlled-visual-smoke-check-latest.json"
    controlled_visual_smoke_json_display = ".\output\pdf-controlled-visual-smoke\controlled-visual-smoke-check-latest.json"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "pdf_visual_release_gate_preflight.build_outputs_missing"
            severity = "error"
            status = "blocked"
            message = "PDF visual release gate preflight is not ready."
            action = "prepare_pdf_visual_release_gate_build_outputs"
            source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
            source_json_display = ".\output\pdf-visual-release-gate-preflight\summary.json"
            blocking_summary = [ordered]@{
                missing_visual_baseline_pdf_count = 42
                missing_cjk_text_layer_pdf_count = 43
            }
            output_gap_count = 3
            missing_output_count = 87
            output_gap_summary = @(
                [ordered]@{
                    check = "visual_baseline_manifest_pdfs_exist"
                    missing_count = 42
                    missing_paths_preview = @("test\pdf_visual_baselines\cjk\font_map_text.pdf")
                }
            )
            pdf_dependency_inputs = [ordered]@{
                status = "not_ready"
                selected_pdfium_provider = "unresolved"
                pdfio_ready = $false
                pdfium_ready = $false
                missing_input_count = 3
                missing_inputs = @(
                    "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h",
                    "PDFium source header: C:\repo\tmp\pdfium-workspace\pdfium\public\fpdfview.h",
                    "PDFium input: provide prebuilt library/include inputs or a source checkout with public/fpdfview.h."
                )
            }
            readiness_action_evidence_count = 2
            readiness_action_evidence = @(
                [ordered]@{
                    id = "pdf_visual_release_gate_preflight.pdf_dependency_inputs_ready.pdfio_source_header"
                    action = "provide_pdf_dependency_input"
                    issue_key = "pdf_dependency_inputs_ready"
                    item = "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h"
                    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                    source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                },
                [ordered]@{
                    id = "pdf_visual_release_gate_preflight.pdf_build_options_enabled.FEATHERDOC_BUILD_PDF_IMPORT"
                    action = "enable_pdf_build_option"
                    issue_key = "pdf_build_options_enabled"
                    item = "FEATHERDOC_BUILD_PDF_IMPORT"
                    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                    source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                }
            )
            build_dir_auto_candidates = @(
                [ordered]@{
                    relative_path = "build"
                    exists = $true
                    cmake_cache_exists = $true
                    ctest_manifest_exists = $true
                    pdf_build_options_enabled = $false
                    pdf_build_options = @(
                        [ordered]@{
                            name = "FEATHERDOC_BUILD_PDF"
                            present = $true
                            value = "OFF"
                            enabled = $false
                        },
                        [ordered]@{
                            name = "FEATHERDOC_BUILD_PDF_IMPORT"
                            present = $true
                            value = "OFF"
                            enabled = $false
                        }
                    )
                    looks_reusable = $false
                },
                [ordered]@{
                    relative_path = "out\build"
                    exists = $false
                    cmake_cache_exists = $false
                    ctest_manifest_exists = $false
                    pdf_build_options_enabled = $false
                    looks_reusable = $false
                }
            )
        }
    )
    action_items = @(
        [ordered]@{
            id = "prepare_pdf_visual_release_gate_build_outputs"
            action = "prepare_pdf_visual_release_gate_build_outputs"
            title = "Prepare PDF visual release gate build outputs"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -PreflightOnly"
            source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
            source_json_display = ".\output\pdf-visual-release-gate-preflight\summary.json"
            blocking_summary = [ordered]@{
                missing_cjk_text_layer_pdf_count = 43
            }
            output_gap_count = 3
            missing_output_count = 87
            output_gap_summary = @(
                [ordered]@{
                    check = "cjk_text_layer_manifest_pdfs_exist"
                    missing_count = 43
                    missing_paths_preview = @("test\pdf_text_layer_cjk\copy_search\mixed_cjk_text.pdf")
                }
            )
            pdf_dependency_inputs = [ordered]@{
                status = "not_ready"
                selected_pdfium_provider = "unresolved"
                pdfio_ready = $false
                pdfium_ready = $false
                missing_input_count = 3
                missing_inputs = @(
                    "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h",
                    "PDFium source header: C:\repo\tmp\pdfium-workspace\pdfium\public\fpdfview.h",
                    "PDFium input: provide prebuilt library/include inputs or a source checkout with public/fpdfview.h."
                )
            }
            readiness_action_evidence_count = 2
            readiness_action_evidence = @(
                [ordered]@{
                    id = "pdf_visual_release_gate_preflight.pdf_dependency_inputs_ready.pdfio_source_header"
                    action = "provide_pdf_dependency_input"
                    issue_key = "pdf_dependency_inputs_ready"
                    item = "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h"
                    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                    source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                },
                [ordered]@{
                    id = "pdf_visual_release_gate_preflight.pdf_build_options_enabled.FEATHERDOC_BUILD_PDF_IMPORT"
                    action = "enable_pdf_build_option"
                    issue_key = "pdf_build_options_enabled"
                    item = "FEATHERDOC_BUILD_PDF_IMPORT"
                    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                    source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                }
            )
            build_dir_auto_candidates = @(
                [ordered]@{
                    relative_path = "build"
                    exists = $true
                    cmake_cache_exists = $true
                    ctest_manifest_exists = $true
                    pdf_build_options_enabled = $false
                    pdf_build_options = @(
                        [ordered]@{
                            name = "FEATHERDOC_BUILD_PDF"
                            present = $true
                            value = "OFF"
                            enabled = $false
                        },
                        [ordered]@{
                            name = "FEATHERDOC_BUILD_PDF_IMPORT"
                            present = $true
                            value = "OFF"
                            enabled = $false
                        }
                    )
                    looks_reusable = $false
                },
                [ordered]@{
                    relative_path = "out\build"
                    exists = $false
                    cmake_cache_exists = $false
                    ctest_manifest_exists = $false
                    pdf_build_options_enabled = $false
                    looks_reusable = $false
                }
            )
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
            source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
            source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\controlled-visual-smoke-failed.json"
        }
    )
})

Write-JsonFile -Path $emptyPath -Value ([ordered]@{
    schema = "featherdoc.empty_report.v1"
    release_blocker_count = 0
    release_blockers = @()
    action_items = @()
})

Write-JsonFile -Path $malformedPath -Value ([ordered]@{
    schema = "featherdoc.malformed_report.v1"
    release_blocker_count = 3
    release_blockers = @(
        [ordered]@{
            id = "malformed.actual_blocker"
            severity = "error"
            message = "Only one blocker is present."
            action = "fix_malformed_fixture"
        }
    )
    action_items = @()
})

Write-JsonFile -Path $dedupePath -Value ([ordered]@{
    schema = "featherdoc.dedupe_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_smoke.schema_approval"
            severity = "error"
            status = "blocked"
            message = "A second report has the same id."
            action = "fix_schema_patch_approval_result"
        }
    )
    action_items = @(
        [ordered]@{
            id = "same_action"
            action = "fix_schema_patch_approval_result"
            title = "Fix schema approval"
        }
    )
})

function Write-PassingInputRoot {
    param([string]$Root)

    Write-JsonFile -Path (Join-Path $Root "document-skeleton\document_skeleton_governance.summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $documentSkeletonPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $Root "numbering-governance\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $numberingGovernancePath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $Root "table-layout\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $tableLayoutPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $Root "style-governance\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $styleGovernancePath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $Root "content-control\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $contentControlPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $Root "project-template-readiness\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $projectTemplateReadinessPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $Root "schema-patch-confidence-calibration\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $schemaCalibrationPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $Root "release-candidate\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseCandidatePath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $Root "pdf-preflight-governance\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfPreflightGovernancePath | ConvertFrom-Json)
}
