$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$reportDir = Join-Path $resolvedWorkingDir "report"
$installDir = Join-Path $resolvedWorkingDir "install"
$gateReportDir = Join-Path $resolvedWorkingDir "word-visual-release-gate\report"
$schemaApprovalHistoryJsonPath = Join-Path $reportDir "project_template_schema_approval_history.json"
$schemaApprovalHistoryMarkdownPath = Join-Path $reportDir "project_template_schema_approval_history.md"
$projectTemplateWorkflowDashboardSummaryPath = Join-Path $reportDir "project_template_workflow_dashboard.json"
$projectTemplateWorkflowDashboardMarkdownPath = Join-Path $reportDir "project_template_workflow_dashboard.md"
$taskOutputRoot = Join-Path $resolvedWorkingDir "tasks"
$sectionPageSetupTaskDir = Join-Path $resolvedWorkingDir "tasks\section-page-setup"
$pageNumberFieldsTaskDir = Join-Path $resolvedWorkingDir "tasks\page-number-fields"
$curatedBundleId = "template-table-cli-selector"
$curatedBundleLabel = "Template table CLI selector"
$curatedBundleTaskDir = Join-Path $resolvedWorkingDir "tasks\$curatedBundleId"
$supersededReviewTasksReportPath = Join-Path $taskOutputRoot "superseded_review_tasks.json"
$expectedSupersededReviewTasksReportDisplayPath = if ($supersededReviewTasksReportPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    ".\" + ($supersededReviewTasksReportPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/') -replace '/', '\')
} else {
    [System.IO.Path]::GetFullPath($supersededReviewTasksReportPath)
}
$contentControlCommandTemplateMarker = "command_template: featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
$contentControlDuplicateActionCommandTemplateMarker = "command_template: featherdoc_cli inspect-content-controls <input.docx> --json"

function New-ReleaseNoteSchemaCorpusMetadataFixture {
    param([ValidateSet("missing_document_type", "missing_corpus_role", "mismatched_metadata")] [string]$Case)

    switch ($Case) {
        "missing_document_type" {
            return [ordered]@{
                business_document_type = ""
                source_business_document_type = "contract"
                corpus_role = "registered-business-template"
                source_corpus_role = "registered-business-template"
                business_document_type_mismatch = $false
                corpus_role_mismatch = $false
                missing_business_document_type_count = 1
                missing_corpus_role_count = 0
                mismatched_corpus_metadata_count = 0
                mismatched_business_document_type_count = 0
                mismatched_corpus_role_count = 0
                candidate_name = "contract.customer_name"
                schema_update_candidate = "customer_name"
            }
        }
        "missing_corpus_role" {
            return [ordered]@{
                business_document_type = "policy"
                source_business_document_type = "policy"
                corpus_role = ""
                source_corpus_role = "planned-business-template"
                business_document_type_mismatch = $false
                corpus_role_mismatch = $false
                missing_business_document_type_count = 0
                missing_corpus_role_count = 1
                mismatched_corpus_metadata_count = 0
                mismatched_business_document_type_count = 0
                mismatched_corpus_role_count = 0
                candidate_name = "policy.effective_date"
                schema_update_candidate = "effective_date"
            }
        }
        default {
            return [ordered]@{
                business_document_type = "invoice"
                source_business_document_type = "notice"
                corpus_role = "registered-business-template"
                source_corpus_role = "planned-business-template"
                business_document_type_mismatch = $true
                corpus_role_mismatch = $true
                missing_business_document_type_count = 0
                missing_corpus_role_count = 0
                mismatched_corpus_metadata_count = 1
                mismatched_business_document_type_count = 1
                mismatched_corpus_role_count = 1
                candidate_name = "notice.invoice_number"
                schema_update_candidate = "invoice_number"
            }
        }
    }
}

New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $installDir -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $taskOutputRoot -Force | Out-Null
New-Item -ItemType Directory -Path $sectionPageSetupTaskDir -Force | Out-Null
New-Item -ItemType Directory -Path $pageNumberFieldsTaskDir -Force | Out-Null
New-Item -ItemType Directory -Path $curatedBundleTaskDir -Force | Out-Null

$summaryPath = Join-Path $reportDir "summary.json"
$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"

$gateSummary = [ordered]@{
    generated_at = "2026-04-11T12:00:00"
    workspace = $resolvedRepoRoot
    report_dir = $gateReportDir
    visual_verdict = "pending_manual_review"
    review_tasks = [ordered]@{
        section_page_setup = [ordered]@{
            task_dir = $sectionPageSetupTaskDir
        }
        page_number_fields = [ordered]@{
            task_dir = $pageNumberFieldsTaskDir
        }
        curated_visual_regressions = @(
            [ordered]@{
                id = $curatedBundleId
                label = $curatedBundleLabel
                task = [ordered]@{
                    task_dir = $curatedBundleTaskDir
                }
            }
        )
    }
    manual_review = [ordered]@{
        tasks = [ordered]@{
            section_page_setup = [ordered]@{
                verdict = "pass"
            }
            page_number_fields = [ordered]@{
                verdict = "pending_manual_review"
            }
            curated_visual_regressions = @(
                [ordered]@{
                    id = $curatedBundleId
                    label = "curated:$curatedBundleId"
                    display_label = $curatedBundleLabel
                    verdict = "pass"
                    task_dir = $curatedBundleTaskDir
                }
            )
        }
    }
    section_page_setup = [ordered]@{
        review_verdict = "pass"
    }
    page_number_fields = [ordered]@{
        review_verdict = "pending_manual_review"
    }
    curated_visual_regressions = @(
        [ordered]@{
            id = $curatedBundleId
            label = $curatedBundleLabel
            review_verdict = "pass"
            status = "completed"
            task = [ordered]@{
                task_dir = $curatedBundleTaskDir
            }
        }
    )
}
($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8
Set-Content -LiteralPath $gateFinalReviewPath -Encoding UTF8 -Value "# Gate Final Review"
(@{
    generated_at = "2026-04-11T12:00:00"
    task_output_root = $taskOutputRoot
    report_path = $supersededReviewTasksReportPath
    group_count = 2
    superseded_task_count = 0
    groups = @()
} | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $supersededReviewTasksReportPath -Encoding UTF8

$summary = [ordered]@{
    generated_at = "2026-04-11T12:00:00"
    workspace = $resolvedRepoRoot
    execution_status = "pass"
    release_version = "1.6.0"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_smoke.schema_approval"
            source = "project_template_smoke"
            severity = "error"
            status = "blocked"
            message = "Project template smoke schema approval gate blocked."
            gate_status = "blocked"
            compliance_issue_count = 2
            invalid_result_count = 1
            blocked_item_count = 1
            issue_keys = @("missing_reviewer", "missing_reviewed_at")
            summary_json = Join-Path $resolvedWorkingDir "project-template-smoke-summary.json"
            history_json = $schemaApprovalHistoryJsonPath
            history_markdown = $schemaApprovalHistoryMarkdownPath
            action = "fix_schema_patch_approval_result"
            items = @(
                [ordered]@{
                    name = "schema-review-invalid"
                    status = "invalid_result"
                    decision = "approved"
                    action = "fix_schema_patch_approval_result"
                    compliance_issue_count = 2
                    compliance_issues = @("missing_reviewer", "missing_reviewed_at")
                    approval_result = Join-Path $resolvedWorkingDir "schema_patch_approval_result.json"
                }
            )
        }
    )
    release_blocker_rollup = [ordered]@{
        requested = $true
        status = "blocked"
        source_report_count = 6
        source_failure_count = 1
        source_reports = @(
            [ordered]@{
                schema = "featherdoc.table_layout_delivery_governance_report.v1"
                status = "failed"
                release_ready = $false
                path = ".\output\table-layout-delivery-governance\summary.json"
                path_display = ".\output\table-layout-delivery-governance\summary.json"
                source_json = ".\output\table-layout-delivery-governance\unreadable-summary.json"
                source_json_display = ".\output\table-layout-delivery-governance\unreadable-summary.json"
                source_failure_count = 1
                error = "Unexpected token while reading table layout governance summary."
                build_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1"
            },
            [ordered]@{
                schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                status = "blocked"
                release_ready = $false
                path = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                path_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                source_failure_count = 0
                preflight_ready = $false
                full_visual_gate_required = $true
                full_visual_gate_status = "not_run_by_preflight_governance"
                controlled_visual_smoke_available = $true
                controlled_visual_smoke_status = "pass"
                controlled_visual_smoke_passed = $true
                controlled_visual_smoke_case_count = 2
                controlled_visual_smoke_json = ".\output\pdf-controlled-visual-smoke-20260520\controlled-visual-smoke-check-latest.json"
                controlled_visual_smoke_json_display = ".\output\pdf-controlled-visual-smoke-20260520\controlled-visual-smoke-check-latest.json"
            },
            [ordered]@{
                schema = "featherdoc.project_template_delivery_readiness_report.v1"
                status = "blocked"
                release_ready = $false
                path = ".\output\project-template-delivery-readiness\summary.json"
                path_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-delivery-readiness\summary.json"
                source_failure_count = 0
                latest_schema_approval_gate_status = "pending_review"
                schema_approval_status_summary = "pending_review"
                onboarding_governance_next_action = [ordered]@{
                    action = "review_schema_update_candidate"
                    status = "pending_review"
                    blocker_id = "project_template_onboarding.schema_approval"
                    reason = "Project template onboarding schema approval is pending."
                }
                onboarding_governance_next_action_summary = @(
                    [ordered]@{
                        action = "review_schema_update_candidate"
                        status = "pending_review"
                        blocker_id = "project_template_onboarding.schema_approval"
                        reason = "Project template onboarding schema approval is pending."
                    }
                )
                onboarding_governance_next_action_group_count = 1
                schema_history_blocked_run_count = 0
                schema_history_pending_run_count = 1
                schema_history_passed_run_count = 2
                template_count = 3
                ready_template_count = 2
                blocked_template_count = 1
                release_blocker_count = 1
                action_item_count = 1
                warning_count = 0
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
                build_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1"
            },
            [ordered]@{
                id = "project_template_onboarding_governance"
                schema = "featherdoc.project_template_onboarding_governance_report.v1"
                status = "pending_review"
                release_ready = $false
                path = ".\output\project-template-onboarding-governance\summary.json"
                path_display = ".\output\project-template-onboarding-governance\summary.json"
                source_json = ".\output\project-template-onboarding-governance\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                source_failure_count = 0
                schema_approval_status_summary = "pending_review"
                next_action = [ordered]@{
                    action = "review_schema_update_candidate"
                    status = "pending_review"
                    blocker_id = "project_template_onboarding.schema_approval"
                    reason = "Project template onboarding schema approval is pending."
                }
                next_action_summary = @(
                    [ordered]@{
                        action = "review_schema_update_candidate"
                        status = "pending_review"
                        blocker_id = "project_template_onboarding.schema_approval"
                        reason = "Project template onboarding schema approval is pending."
                    }
                )
                next_action_group_count = 1
                source_file_count = 3
                entry_count = 4
                blocked_entry_count = 0
                pending_review_entry_count = 1
                not_evaluated_entry_count = 0
                approved_entry_count = 2
                not_required_entry_count = 1
                release_blocker_count = 1
                action_item_count = 1
                manual_review_recommendation_count = 1
                business_document_type_summary = @(
                    [ordered]@{
                        document_type = "invoice"
                        count = 2
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
                        count = 2
                    }
                )
            }
        )
        release_blocker_count = 5
        release_blockers = @(
            [ordered]@{
                id = "document_skeleton.style_numbering_issues"
                source = "document_skeleton_governance_rollup"
                severity = "error"
                status = "needs_review"
                action = "review_style_numbering_audit"
                message = "Document skeleton rollup found style numbering issues."
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
                source_json_display = ".\output\document-skeleton-governance\contract\style-numbering-audit.json"
            },
            [ordered]@{
                id = "content_control_data_binding.bound_placeholder"
                source = "content_control_data_binding_governance"
                severity = "error"
                status = "placeholder_visible"
                action = "sync_or_fill_bound_content_control"
                message = "Bound content control still shows placeholder text."
                source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
                source_report_display = ".\output\content-control-data-binding-governance\summary.json"
                source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
                input_docx = "samples/invoice.docx"
                input_docx_display = ".\samples\invoice.docx"
                template_name = "invoice-template"
                schema_target = "invoice"
                target_mode = "resolved-section-targets"
                repair_strategy = "sync_bound_content_control"
                repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
                command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
                repair_action_classes = @("release_blocking", "auto_repair_candidate", "manual_confirmation_required")
            },
            [ordered]@{
                id = "project_template_onboarding.schema_approval"
                source = "project_template_delivery_readiness"
                severity = "error"
                status = "pending_review"
                action = "review_schema_update_candidate"
                message = "Project template onboarding schema approval is pending."
                source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                requires_reviewer_action = $true
                reviewer_action_summary = "review_schema_update_candidate"
                reviewer_action_reason = "latest_review_state=pending; issue_keys=(none)"
                reviewer_actions = @("review_schema_update_candidate")
            },
            ([ordered]@{
                id = "schema_patch_confidence_calibration.pending_schema_approvals"
                source = "schema_patch_confidence_calibration"
                severity = "error"
                status = "pending_review"
                action = "resolve_pending_schema_approvals"
                message = "Schema patch confidence calibration still contains pending approval outcome(s)."
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            } + (New-ReleaseNoteSchemaCorpusMetadataFixture -Case "missing_document_type")),
            [ordered]@{
                id = "pdf_visual_release_gate_preflight.build_outputs_missing"
                source = "pdf_visual_release_gate_preflight"
                severity = "error"
                status = "blocked"
                action = "prepare_pdf_visual_release_gate_build_outputs"
                message = "PDF visual release gate preflight is not ready; blocking checks: build_dir_exists, cmake_cache_exists, ctest_manifest_exists, pdf_build_options_enabled."
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                repair_strategy = "reuse_or_prepare_pdf_visual_release_gate_build_outputs"
                repair_hint = "Prepare a reusable PDF build directory with CMakeCache.txt and enable FEATHERDOC_BUILD_PDF / FEATHERDOC_BUILD_PDF_IMPORT before running the full PDF visual release gate."
                command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly"
                issue_keys = @("build_dir_exists", "cmake_cache_exists", "ctest_manifest_exists", "pdf_build_options_enabled")
                blocking_summary = [ordered]@{
                    required_check_count = 12
                    blocking_check_count = 7
                    missing_cli_pdf_count = 2
                    visual_baseline_sample_count = 42
                    missing_visual_baseline_pdf_count = 42
                    cjk_text_layer_sample_count = 43
                    missing_cjk_text_layer_pdf_count = 43
                    build_dir_entry_count = 1
                    ctest_required_pattern_count = 4
                    memory_guard_blocked = $false
                    memory_guard_skipped = $false
                    free_memory_mb = 1140
                    min_free_memory_mb = 2048
                    pdf_build_options_enabled = $false
                    disabled_pdf_build_options = @("FEATHERDOC_BUILD_PDF", "FEATHERDOC_BUILD_PDF_IMPORT")
                    missing_pdf_build_options = @()
                }
                readiness_action_evidence_count = 2
                readiness_action_evidence = @(
                    [ordered]@{
                        id = "pdf_visual_release_gate_preflight.pdf_dependency_inputs_ready.pdfio_source_header"
                        action = "provide_pdf_dependency_input"
                        issue_key = "pdf_dependency_inputs_ready"
                        item = "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h"
                        source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                        source_report = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_json = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                        source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                    },
                    [ordered]@{
                        id = "pdf_visual_release_gate_preflight.pdf_build_options_enabled.FEATHERDOC_BUILD_PDF_IMPORT"
                        action = "enable_pdf_build_option"
                        issue_key = "pdf_build_options_enabled"
                        item = "FEATHERDOC_BUILD_PDF_IMPORT"
                        source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                        source_report = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_json = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                        source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                    }
                )
            }
        )
        action_item_count = 5
        action_items = @(
            [ordered]@{
                id = "open_document_skeleton_rollup"
                action = "open_document_skeleton_rollup"
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
                source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1 -InputRoot .\output\document-skeleton-governance"
            },
            [ordered]@{
                id = "review_duplicate_content_control_binding"
                action = "review_duplicate_content_control_binding"
                source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
                source_report_display = ".\output\content-control-data-binding-governance\summary.json"
                source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
                input_docx = "samples/invoice.docx"
                input_docx_display = ".\samples\invoice.docx"
                template_name = "invoice-template"
                schema_target = "invoice"
                target_mode = "resolved-section-targets"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
                repair_strategy = "deduplicate_or_confirm_shared_binding"
                repair_hint = "Confirm the repeated binding is intentional, or split the controls across distinct Custom XML paths."
                command_template = "featherdoc_cli inspect-content-controls <input.docx> --json"
                repair_action_classes = @("manual_confirmation_required")
            },
            [ordered]@{
                id = "review_invoice_schema"
                action = "review_schema_update_candidate"
                source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1"
                requires_reviewer_action = $true
                reviewer_action_summary = "review_schema_update_candidate"
                reviewer_action_reason = "latest_review_state=pending; issue_keys=(none)"
                reviewer_actions = @("review_schema_update_candidate")
            },
            ([ordered]@{
                id = "resolve_pending_schema_approvals"
                action = "resolve_pending_schema_approvals"
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
            } + (New-ReleaseNoteSchemaCorpusMetadataFixture -Case "missing_corpus_role")),
            [ordered]@{
                id = "prepare_pdf_visual_release_gate_build_outputs"
                action = "prepare_pdf_visual_release_gate_build_outputs"
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                open_command = "powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly"
                command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly"
                issue_keys = @("build_dir_exists", "cmake_cache_exists", "ctest_manifest_exists", "pdf_build_options_enabled")
                blocking_summary = [ordered]@{
                    required_check_count = 12
                    blocking_check_count = 7
                    missing_cli_pdf_count = 2
                    visual_baseline_sample_count = 42
                    missing_visual_baseline_pdf_count = 42
                    cjk_text_layer_sample_count = 43
                    missing_cjk_text_layer_pdf_count = 43
                    build_dir_entry_count = 1
                    ctest_required_pattern_count = 4
                    memory_guard_blocked = $false
                    memory_guard_skipped = $false
                    free_memory_mb = 1140
                    min_free_memory_mb = 2048
                    pdf_build_options_enabled = $false
                    disabled_pdf_build_options = @("FEATHERDOC_BUILD_PDF", "FEATHERDOC_BUILD_PDF_IMPORT")
                    missing_pdf_build_options = @()
                }
                readiness_action_evidence_count = 2
                readiness_action_evidence = @(
                    [ordered]@{
                        id = "pdf_visual_release_gate_preflight.pdf_dependency_inputs_ready.pdfio_source_header"
                        action = "provide_pdf_dependency_input"
                        issue_key = "pdf_dependency_inputs_ready"
                        item = "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h"
                        source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                        source_report = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_json = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                        source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                    },
                    [ordered]@{
                        id = "pdf_visual_release_gate_preflight.pdf_build_options_enabled.FEATHERDOC_BUILD_PDF_IMPORT"
                        action = "enable_pdf_build_option"
                        issue_key = "pdf_build_options_enabled"
                        item = "FEATHERDOC_BUILD_PDF_IMPORT"
                        source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                        source_report = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_json = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                        source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                    }
                )
            }
        )
        warning_count = 3
        warnings = @(
            [ordered]@{
                id = "document_skeleton.exemplar_catalog_missing"
                action = "open_document_skeleton_rollup"
                message = "One exemplar catalog path is missing."
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
                source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
            },
            [ordered]@{
                id = "custom_xml_sync_evidence_missing"
                action = "run_content_control_custom_xml_sync"
                message = "Data-bound content controls were inspected, but no Custom XML sync result was provided."
                source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
                source_report_display = ".\output\content-control-data-binding-governance\summary.json"
                source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
                input_docx = "samples/invoice.docx"
                input_docx_display = ".\samples\invoice.docx"
                template_name = "invoice-template"
                schema_target = "invoice"
                target_mode = "resolved-section-targets"
            },
            ([ordered]@{
                id = "schema_patch_confidence_calibration.unscored_candidates"
                action = "add_explicit_confidence_metadata"
                message = "Some schema patch candidates do not carry explicit confidence metadata."
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            } + (New-ReleaseNoteSchemaCorpusMetadataFixture -Case "mismatched_metadata"))
        )
        governance_metric_count = 2
        governance_metrics = @(
            (New-NumberingGovernanceMetricFixture),
            (New-TableLayoutDeliveryMetricFixture)
        )
    }
    release_governance_handoff = [ordered]@{
        requested = $true
        status = "blocked"
        expected_report_count = 5
        loaded_report_count = 5
        missing_report_count = 0
        failed_report_count = 1
        reports = @(
            [ordered]@{
                id = "project_template_delivery_readiness"
                schema = "featherdoc.project_template_delivery_readiness_report.v1"
                status = "failed"
                release_ready = $false
                expected_summary = ".\output\project-template-delivery-readiness\summary.json"
                expected_summary_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-delivery-readiness\summary.json"
                source_failure_count = 1
                latest_schema_approval_gate_status = "pending_review"
                schema_approval_status_summary = "pending_review"
                onboarding_governance_next_action = [ordered]@{
                    action = "review_schema_update_candidate"
                    status = "pending_review"
                    blocker_id = "project_template_onboarding.schema_approval"
                    reason = "Project template onboarding schema approval is pending."
                }
                onboarding_governance_next_action_summary = @(
                    [ordered]@{
                        action = "review_schema_update_candidate"
                        status = "pending_review"
                        blocker_id = "project_template_onboarding.schema_approval"
                        reason = "Project template onboarding schema approval is pending."
                    }
                )
                onboarding_governance_next_action_group_count = 1
                schema_history_blocked_run_count = 0
                schema_history_pending_run_count = 1
                schema_history_passed_run_count = 2
                template_count = 3
                ready_template_count = 2
                blocked_template_count = 1
                release_blocker_count = 1
                action_item_count = 1
                warning_count = 0
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
                error = "Failed to parse project template delivery readiness summary."
                build_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1"
            },
            [ordered]@{
                id = "project_template_onboarding_governance"
                schema = "featherdoc.project_template_onboarding_governance_report.v1"
                status = "pending_review"
                release_ready = $false
                expected_summary = ".\output\project-template-onboarding-governance\summary.json"
                expected_summary_display = ".\output\project-template-onboarding-governance\summary.json"
                source_json = ".\output\project-template-onboarding-governance\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                source_failure_count = 0
                schema_approval_status_summary = "pending_review"
                next_action = [ordered]@{
                    action = "review_schema_update_candidate"
                    status = "pending_review"
                    blocker_id = "project_template_onboarding.schema_approval"
                    reason = "Project template onboarding schema approval is pending."
                }
                next_action_summary = @(
                    [ordered]@{
                        action = "review_schema_update_candidate"
                        status = "pending_review"
                        blocker_id = "project_template_onboarding.schema_approval"
                        reason = "Project template onboarding schema approval is pending."
                    }
                )
                next_action_group_count = 1
                source_file_count = 3
                entry_count = 4
                blocked_entry_count = 0
                pending_review_entry_count = 1
                not_evaluated_entry_count = 0
                approved_entry_count = 2
                not_required_entry_count = 1
                release_blocker_count = 1
                action_item_count = 1
                manual_review_recommendation_count = 1
                business_document_type_summary = @(
                    [ordered]@{
                        document_type = "invoice"
                        count = 2
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
                        count = 2
                    }
                )
            }
        )
        release_blocker_count = 2
        release_blockers = @(
            [ordered]@{
                report_id = "project_template_delivery_readiness"
                id = "project_template_onboarding.schema_approval"
                severity = "error"
                status = "pending_review"
                action = "review_schema_update_candidate"
                message = "Project template onboarding schema approval is pending."
                source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                readiness_status = "failed"
                readiness_release_ready = "False"
                requires_reviewer_action = $true
                reviewer_action_summary = "review_schema_update_candidate"
                reviewer_action_reason = "latest_review_state=pending; issue_keys=(none)"
                reviewer_actions = @("review_schema_update_candidate")
                onboarding_governance_status = "pending_review"
                onboarding_governance_release_ready = "False"
                onboarding_governance_schema_approval_status_summary = "pending_review"
                onboarding_governance_next_action = [ordered]@{
                    action = "review_schema_update_candidate"
                    status = "pending_review"
                    blocker_id = "project_template_onboarding.schema_approval"
                    reason = "Project template onboarding schema approval is pending."
                }
                onboarding_governance_next_action_summary = @(
                    [ordered]@{
                        action = "review_schema_update_candidate"
                        status = "pending_review"
                        blocker_id = "project_template_onboarding.schema_approval"
                        reason = "Project template onboarding schema approval is pending."
                    }
                )
                onboarding_governance_next_action_group_count = 1
                onboarding_governance_source_report_display = ".\output\project-template-onboarding-governance\summary.json"
                onboarding_governance_source_json_display = ".\output\project-template-onboarding-governance\summary.json"
            },
            ([ordered]@{
                report_id = "schema_patch_confidence_calibration"
                id = "schema_patch_confidence_calibration.pending_schema_approvals"
                severity = "error"
                status = "pending_review"
                action = "resolve_pending_schema_approvals"
                message = "Schema patch confidence calibration still contains pending approval outcome(s)."
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            } + (New-ReleaseNoteSchemaCorpusMetadataFixture -Case "missing_document_type"))
        )
        action_item_count = 2
        action_items = @(
            [ordered]@{
                report_id = "project_template_delivery_readiness"
                id = "review_invoice_schema"
                action = "review_schema_update_candidate"
                source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1"
                readiness_status = "failed"
                readiness_release_ready = "False"
                requires_reviewer_action = $true
                reviewer_action_summary = "review_schema_update_candidate"
                reviewer_action_reason = "latest_review_state=pending; issue_keys=(none)"
                reviewer_actions = @("review_schema_update_candidate")
                onboarding_governance_status = "pending_review"
                onboarding_governance_release_ready = "False"
                onboarding_governance_schema_approval_status_summary = "pending_review"
                onboarding_governance_next_action = [ordered]@{
                    action = "review_schema_update_candidate"
                    status = "pending_review"
                    blocker_id = "project_template_onboarding.schema_approval"
                    reason = "Project template onboarding schema approval is pending."
                }
                onboarding_governance_next_action_summary = @(
                    [ordered]@{
                        action = "review_schema_update_candidate"
                        status = "pending_review"
                        blocker_id = "project_template_onboarding.schema_approval"
                        reason = "Project template onboarding schema approval is pending."
                    }
                )
                onboarding_governance_next_action_group_count = 1
                onboarding_governance_source_report_display = ".\output\project-template-onboarding-governance\summary.json"
                onboarding_governance_source_json_display = ".\output\project-template-onboarding-governance\summary.json"
            },
            ([ordered]@{
                report_id = "schema_patch_confidence_calibration"
                id = "resolve_pending_schema_approvals"
                action = "resolve_pending_schema_approvals"
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
            } + (New-ReleaseNoteSchemaCorpusMetadataFixture -Case "missing_corpus_role"))
        )
        warning_count = 1
        warnings = @(
            ([ordered]@{
                report_id = "schema_patch_confidence_calibration"
                id = "schema_patch_confidence_calibration.unscored_candidates"
                action = "add_explicit_confidence_metadata"
                message = "Some schema patch candidates do not carry explicit confidence metadata."
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            } + (New-ReleaseNoteSchemaCorpusMetadataFixture -Case "mismatched_metadata"))
        )
        project_template_readiness_checklist_entrypoints_source_report_count = 1
        project_template_readiness_checklist_entrypoints_source_reports = @(
            [ordered]@{
                schema = "featherdoc.release_candidate_summary"
                path_display = ".\output\release-candidate-checks\summary.json"
                project_template_readiness_checklist_entrypoints_status = "declared"
                project_template_readiness_checklist_entrypoints_checklist_label = "Project template release readiness checklist"
                project_template_readiness_checklist_entrypoints_checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
                project_template_readiness_checklist_entrypoints_required_entrypoint_count = 3
                project_template_readiness_checklist_entrypoints_entrypoint_ids = @(
                    "start_here",
                    "artifact_guide",
                    "reviewer_checklist"
                )
                project_template_readiness_checklist_entrypoints_entrypoints = @(
                    [ordered]@{
                        id = "start_here"
                        required = $true
                        path_display = ".\output\release-candidate-checks\START_HERE.md"
                    },
                    [ordered]@{
                        id = "artifact_guide"
                        required = $true
                        path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
                    },
                    [ordered]@{
                        id = "reviewer_checklist"
                        required = $true
                        path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
                    }
                )
                project_template_readiness_checklist_entrypoints_checklist_marker = "release_entry_project_template_readiness_checklist_trace"
            }
        )
        release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = 1
        release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @(
            [ordered]@{
                schema = "featherdoc.release_candidate_summary"
                path_display = ".\output\release-blocker-rollup\summary.json"
                release_entry_project_template_readiness_checklist_material_safety_audit_status = "passed"
                release_entry_project_template_readiness_checklist_material_safety_audit_script = ".\scripts\assert_release_material_safety.ps1"
                release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count = 3
                release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints = @(
                    "start_here",
                    "artifact_guide",
                    "reviewer_checklist"
                )
                release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label = "Project-template readiness checklist handoff evidence"
                release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
                release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema = "featherdoc.release_candidate_summary"
                release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
                release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker = "release_entry_project_template_readiness_checklist_trace"
                release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
            }
        )
        word_visual_standard_review_metadata_source_report_count = 1
        word_visual_standard_review_metadata_source_reports = @(
            [ordered]@{
                schema = "featherdoc.release_candidate_summary"
                path_display = ".\output\release-candidate-checks\summary.json"
                word_visual_standard_review_metadata_count = 4
                word_visual_standard_review_task_keys = @(
                    "smoke",
                    "fixed_grid",
                    "section_page_setup",
                    "page_number_fields"
                )
                word_visual_standard_review_status_summary = "reviewed=4"
                word_visual_standard_review_verdict_summary = "pass=4"
                word_visual_standard_review_metadata = @(
                    [ordered]@{
                        task_key = "smoke"
                        review_task_key = "document"
                        label = "Word visual smoke"
                        verdict = "pass"
                        review_status = "reviewed"
                        reviewed_at = "2026-04-12T12:10:00"
                        review_method = "operator_supplied"
                        review_result_path = ".\output\word-visual-release-gate\review-tasks\document\report\review_result.json"
                        final_review_path = ".\output\word-visual-release-gate\review-tasks\document\report\final_review.md"
                        review_note = "Private operator note"
                    },
                    [ordered]@{
                        task_key = "fixed_grid"
                        review_task_key = "fixed_grid"
                        label = "Fixed-grid merge/unmerge"
                        verdict = "pass"
                        review_status = "reviewed"
                        reviewed_at = "2026-04-12T12:20:00"
                        review_method = "operator_supplied"
                        review_result_path = ".\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json"
                        final_review_path = ".\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md"
                    },
                    [ordered]@{
                        task_key = "section_page_setup"
                        review_task_key = "section_page_setup"
                        label = "Section page setup"
                        verdict = "pass"
                        review_status = "reviewed"
                        reviewed_at = "2026-04-12T12:30:00"
                        review_method = "operator_supplied"
                        review_result_path = ".\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json"
                        final_review_path = ".\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md"
                    },
                    [ordered]@{
                        task_key = "page_number_fields"
                        review_task_key = "page_number_fields"
                        label = "Page number fields"
                        verdict = "pass"
                        review_status = "reviewed"
                        reviewed_at = "2026-04-12T12:40:00"
                        review_method = "operator_supplied"
                        review_result_path = ".\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json"
                        final_review_path = ".\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md"
                    }
                )
            }
        )
        governance_metric_count = 2
        governance_metrics = @(
            (New-NumberingGovernanceMetricFixture),
            (New-TableLayoutDeliveryMetricFixture)
        )
    }
    project_template_workflow_dashboard = $projectTemplateWorkflowDashboardSummaryPath
    project_template_workflow_dashboard_report = [ordered]@{
        requested = $true
        status = "blocked"
        output_dir = $reportDir
        summary_json = $projectTemplateWorkflowDashboardSummaryPath
        report_markdown = $projectTemplateWorkflowDashboardMarkdownPath
        release_ready = $false
        release_blocker_count = 2
        warning_count = 1
        source_report_count = 2
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
        next_action = [ordered]@{
            action = "review_schema_update_candidate"
            reason = "Project template onboarding schema approval is pending."
            blocker_id = "project_template_onboarding.schema_approval"
            command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_workflow_dashboard.ps1"
            source_report_id = "project_template_onboarding_governance"
        }
        next_action_summary = @(
            [ordered]@{
                source_report_id = "project_template_onboarding_governance"
                action = "review_schema_update_candidate"
                reason = "Project template onboarding schema approval is pending."
                blocker_id = "project_template_onboarding.schema_approval"
                command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_workflow_dashboard.ps1"
                entry_names = @("invoice-template")
                source_report_display = ".\output\project-template-onboarding-governance\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
            },
            [ordered]@{
                source_report_id = "project_template_delivery_readiness"
                action = "collect_project_template_onboarding_governance_evidence"
                reason = "Delivery readiness depends on onboarding governance evidence."
                blocker_id = "project_template_delivery_readiness.onboarding_governance"
                command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_workflow_dashboard.ps1"
                entry_names = @("policy-template")
                source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-delivery-readiness\summary.json"
            }
        )
        next_action_group_count = 2
        next_action_summary_by_source = @(
            [ordered]@{
                source_report_id = "project_template_onboarding_governance"
                source_report_display = ".\output\project-template-onboarding-governance\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                action_group_count = 1
            },
            [ordered]@{
                source_report_id = "project_template_delivery_readiness"
                source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-delivery-readiness\summary.json"
                action_group_count = 1
            }
        )
    }
    task_output_root = $taskOutputRoot
    superseded_review_tasks_report = $supersededReviewTasksReportPath
    install_dir = $installDir
    steps = [ordered]@{
        configure = [ordered]@{ status = "completed" }
        build = [ordered]@{ status = "completed" }
        tests = [ordered]@{ status = "completed" }
        install_smoke = [ordered]@{
            status = "completed"
            install_prefix = $installDir
            consumer_document = (Join-Path $resolvedWorkingDir "consumer\install-smoke.docx")
        }
        visual_gate = [ordered]@{
            status = "completed"
            summary_json = $gateSummaryPath
            final_review = $gateFinalReviewPath
            superseded_review_tasks_report = $supersededReviewTasksReportPath
            smoke_verdict = "pass"
            fixed_grid_verdict = "undetermined"
            curated_visual_regressions = @(
                [ordered]@{
                    id = $curatedBundleId
                    label = $curatedBundleLabel
                    review_verdict = "pass"
                    task_dir = $curatedBundleTaskDir
                }
            )
        }
        project_template_smoke = [ordered]@{
            status = "completed"
            schema_patch_approval_history_json = $schemaApprovalHistoryJsonPath
            schema_patch_approval_history_markdown = $schemaApprovalHistoryMarkdownPath
        }
        project_template_workflow_dashboard = [ordered]@{
            status = "blocked"
            summary_json = $projectTemplateWorkflowDashboardSummaryPath
            report_markdown = $projectTemplateWorkflowDashboardMarkdownPath
            release_ready = $false
            release_blocker_count = 2
            warning_count = 1
            source_report_count = 2
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
            next_action = [ordered]@{
                action = "review_schema_update_candidate"
                reason = "Project template onboarding schema approval is pending."
                blocker_id = "project_template_onboarding.schema_approval"
                command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_workflow_dashboard.ps1"
                source_report_id = "project_template_onboarding_governance"
            }
            next_action_summary = @(
                [ordered]@{
                    source_report_id = "project_template_onboarding_governance"
                    action = "review_schema_update_candidate"
                    reason = "Project template onboarding schema approval is pending."
                    blocker_id = "project_template_onboarding.schema_approval"
                    command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_workflow_dashboard.ps1"
                    entry_names = @("invoice-template")
                    source_report_display = ".\output\project-template-onboarding-governance\summary.json"
                    source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                },
                [ordered]@{
                    source_report_id = "project_template_delivery_readiness"
                    action = "collect_project_template_onboarding_governance_evidence"
                    reason = "Delivery readiness depends on onboarding governance evidence."
                    blocker_id = "project_template_delivery_readiness.onboarding_governance"
                    command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_workflow_dashboard.ps1"
                    entry_names = @("policy-template")
                    source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                    source_json_display = ".\output\project-template-delivery-readiness\summary.json"
                }
            )
            next_action_group_count = 2
            next_action_summary_by_source = @(
                [ordered]@{
                    source_report_id = "project_template_onboarding_governance"
                    source_report_display = ".\output\project-template-onboarding-governance\summary.json"
                    source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                    action_group_count = 1
                },
                [ordered]@{
                    source_report_id = "project_template_delivery_readiness"
                    source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                    source_json_display = ".\output\project-template-delivery-readiness\summary.json"
                    action_group_count = 1
                }
            )
        }
    }
    project_template_smoke = [ordered]@{
        requested = $true
        manifest_path = Join-Path $resolvedWorkingDir "project-template-smoke.manifest.json"
        summary_json = Join-Path $resolvedWorkingDir "project-template-smoke-summary.json"
        output_dir = Join-Path $resolvedWorkingDir "project-template-smoke"
        candidate_discovery_json = Join-Path $resolvedWorkingDir "project-template-smoke-candidates.json"
        schema_patch_approval_gate_status = "passed"
        schema_patch_approval_history_json = $schemaApprovalHistoryJsonPath
        schema_patch_approval_history_markdown = $schemaApprovalHistoryMarkdownPath
    }
}
($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$bundleScript = Join-Path $resolvedRepoRoot "scripts\write_release_note_bundle.ps1"
& $bundleScript -SummaryJson $summaryPath -SkipMaterialSafetyAudit
