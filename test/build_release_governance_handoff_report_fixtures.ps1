$wordVisualStandardReviewMetadata = @(
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

function Write-GovernanceFixtures {
    param(
        [string]$Root,
        [bool]$IncludeContentControl = $true,
        [bool]$IncludeProjectTemplate = $true,
        [bool]$IncludeCalibration = $true
    )

    Write-JsonFile -Path (Join-Path $Root "numbering-catalog-governance\summary.json") -Value ([ordered]@{
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
        release_blocker_count = 1
        warning_count = 1
        release_blockers = @(
            [ordered]@{
                id = "numbering_catalog_governance.style_numbering_issues"
                severity = "error"
                status = "blocked"
                action = "review_style_numbering_audit"
                message = "Style numbering audit reported issues."
            }
        )
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "preview_style_numbering_repair"
                title = "Preview style numbering repair"
            }
        )
        warnings = @(
            [ordered]@{
                id = "numbering_catalog_manifest_summary_missing"
                repair_strategy = "rebuild_numbering_catalog_manifest_summary"
                repair_hint = "Restore the numbering catalog manifest and generate a real manifest check summary; do not synthesize a pass summary when the manifest or catalog outputs are absent."
                command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_manifest.ps1 -ManifestPath .\baselines\numbering-catalog\manifest.json -BuildDir <build-dir> -OutputDir .\output\numbering-catalog-manifest-checks -SkipBuild"
                message = "No numbering catalog manifest summary was loaded."
            }
        )
    })

    Write-JsonFile -Path (Join-Path $Root "table-layout-delivery-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.table_layout_delivery_governance_report.v1"
        status = "ready"
        release_ready = $true
        delivery_quality_score = 100
        delivery_quality_level = "release_ready"
        delivery_quality = [ordered]@{
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
            unresolved_item_count = 0
            penalty_summary = @(
                [ordered]@{ factor = "safe_tblLook_fixes_pending"; count = 0; penalty = 0 }
            )
        }
        release_blocker_count = 0
        warning_count = 0
        release_blockers = @()
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "run_table_style_quality_visual_regression"
                action = "run_table_style_quality_visual_regression"
                title = "Generate Word-rendered table layout evidence"
            }
        )
    })

    if ($IncludeContentControl) {
        Write-JsonFile -Path (Join-Path $Root "content-control-data-binding-governance\summary.json") -Value ([ordered]@{
            schema = "featherdoc.content_control_data_binding_governance_report.v1"
            status = "blocked"
            release_ready = $false
            release_blocker_count = 1
            warning_count = 0
            release_blockers = @(
                [ordered]@{
                    id = "content_control_data_binding.bound_placeholder"
                    severity = "error"
                    status = "placeholder_visible"
                    action = "sync_or_fill_bound_content_control"
                    message = "A data-bound content control is still showing placeholder text."
                    repair_strategy = "sync_bound_content_control"
                    repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
                    command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
                    source_report = "output/content-control-data-binding-governance/summary.json"
                    source_report_display = ".\output\content-control-data-binding-governance\summary.json"
                    source_json = "output/content-control-data-binding/inspect-content-controls.json"
                    source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
                    input_docx = "samples/invoice.docx"
                    input_docx_display = ".\samples\invoice.docx"
                    template_name = "invoice-template"
                    schema_target = "invoice"
                    target_mode = "resolved-section-targets"
                }
            )
            action_item_count = 1
            action_items = @(
                [ordered]@{
                    id = "review_duplicate_content_control_binding"
                    action = "review_duplicate_content_control_binding"
                    title = "Review repeated content controls that share one Custom XML binding"
                    open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
                    repair_strategy = "deduplicate_or_confirm_shared_binding"
                    repair_hint = "Confirm the repeated binding is intentional, or split the controls across distinct Custom XML paths."
                    command_template = "featherdoc_cli inspect-content-controls <input.docx> --tag due_date --json"
                    source_report = "output/content-control-data-binding-governance/summary.json"
                    source_report_display = ".\output\content-control-data-binding-governance\summary.json"
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
    }

    if ($IncludeProjectTemplate) {
        Write-JsonFile -Path (Join-Path $Root "project-template-onboarding-governance\summary.json") -Value ([ordered]@{
            schema = "featherdoc.project_template_onboarding_governance_report.v1"
            status = "pending_review"
            release_ready = $false
            schema_approval_status_summary = @(
                [ordered]@{
                    status = "pending_review"
                    count = 1
                }
            )
            next_action = [ordered]@{
                action = "approve_project_template_schema"
                status = "pending_review"
                blocker_id = "project_template_delivery.pending_schema_approval"
                reason = "Schema approval is still pending."
            }
            next_action_summary = @(
                [ordered]@{
                    action = "approve_project_template_schema"
                    status = "pending_review"
                    blocker_id = "project_template_delivery.pending_schema_approval"
                    reason = "Schema approval is still pending."
                }
            )
            next_action_group_count = 1
            release_blocker_count = 1
            action_item_count = 1
        })

        Write-JsonFile -Path (Join-Path $Root "project-template-delivery-readiness\summary.json") -Value ([ordered]@{
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
            onboarding_governance_next_action = [ordered]@{
                action = "approve_project_template_schema"
                status = "pending_review"
                blocker_id = "project_template_delivery.pending_schema_approval"
                reason = "Schema approval is still pending."
            }
            onboarding_governance_next_action_summary = @(
                [ordered]@{
                    action = "approve_project_template_schema"
                    status = "pending_review"
                    blocker_id = "project_template_delivery.pending_schema_approval"
                    reason = "Schema approval is still pending."
                }
            )
            onboarding_governance_next_action_group_count = 1
            release_blocker_count = 1
            warning_count = 0
            release_blockers = @(
                [ordered]@{
                    id = "project_template_delivery.pending_schema_approval"
                    severity = "error"
                    status = "blocked"
                    action = "approve_project_template_schema"
                    message = "Schema approval is still pending."
                    source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                    source_report = "output/project-template-delivery-readiness/summary.json"
                    source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                    source_json = "output/project-template-onboarding-governance/summary.json"
                    source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                    requires_reviewer_action = $true
                    reviewer_action_summary = "review_schema_update_candidate"
                    reviewer_action_reason = "latest_review_state=pending; issue_keys=(none)"
                    reviewer_actions = @("review_schema_update_candidate")
                    schema_approval_status_summary = @(
                        [ordered]@{
                            status = "pending_review"
                            count = 1
                        }
                    )
                }
            )
            action_item_count = 1
            action_items = @(
                [ordered]@{
                    id = "approve_project_template_schema"
                    action = "approve_project_template_schema"
                    title = "Approve project template schema before release"
                    source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                    source_report = "output/project-template-delivery-readiness/summary.json"
                    source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                    source_json = "output/project-template-onboarding-governance/summary.json"
                    source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                    requires_reviewer_action = $true
                    reviewer_action_summary = "review_schema_update_candidate"
                    reviewer_action_reason = "latest_review_state=pending; issue_keys=(none)"
                    reviewer_actions = @("review_schema_update_candidate")
                    schema_approval_status_summary = @(
                        [ordered]@{
                            status = "pending_review"
                            count = 1
                        }
                    )
                }
            )
        })
    }

    if ($IncludeCalibration) {
        Write-JsonFile -Path (Join-Path $Root "schema-patch-confidence-calibration\summary.json") -Value ([ordered]@{
            schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            status = "pending_review"
            release_ready = $false
            release_blocker_count = 1
            warning_count = 1
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
                    source_json = "output/schema-patch-confidence-calibration/summary.json"
                    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                }
            )
            action_item_count = 1
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
                    source_json = "output/schema-patch-confidence-calibration/summary.json"
                    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                }
            )
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
                    source_json = "output/schema-patch-confidence-calibration/summary.json"
                    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                }
            )
        })
    }

    Write-JsonFile -Path (Join-Path $Root "docx-functional-smoke-readiness\summary.json") -Value ([ordered]@{
        schema = "featherdoc.docx_functional_smoke_readiness.v1"
        status = "pass"
        verdict = "pass"
        release_ready = $true
        docx_functional_smoke_ready = $true
        summary_json_display = ".\output\docx-functional-smoke-readiness\summary.json"
        report_markdown_display = ".\output\docx-functional-smoke-readiness\docx_functional_smoke_readiness.md"
        evidence_scope = "persisted_docx_functional_smoke_evidence_only"
        evidence_scope_note = "This read-only gate does not run CMake, CTest, Word, LibreOffice, browsers, or document rendering."
        boundary = "Pass means persisted DOCX functional evidence is coherent, reused visual PNGs are non-empty, and screenshot-backed review verdicts are pass; it does not claim a fresh Word COM render."
        marker = "docx_functional_smoke_readiness_trace"
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
    })
}
