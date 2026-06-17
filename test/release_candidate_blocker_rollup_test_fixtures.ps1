Write-JsonFile -Path $documentSkeletonRollupPath -Value ([ordered]@{
    schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
    status = "needs_review"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "document_skeleton.style_numbering_issues"
            severity = "error"
            status = "needs_review"
            message = "Document skeleton rollup found style numbering issues."
            action = "review_style_numbering_audit"
            source_json = "output/document-skeleton-governance/contract/style-numbering-audit.json"
            source_json_display = ".\output\document-skeleton-governance\contract\style-numbering-audit.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "open_document_skeleton_rollup"
            action = "open_document_skeleton_rollup"
            title = "Open document skeleton rollup"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1 -InputRoot .\output\document-skeleton-governance"
        }
    )
    warning_count = 1
    warnings = @(
        [ordered]@{
            id = "document_skeleton.exemplar_catalog_missing"
            action = "open_document_skeleton_rollup"
            message = "One exemplar catalog path is missing."
            source_json = "output/document-skeleton-governance-rollup/summary.json"
            source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
        }
    )
})

Write-JsonFile -Path $numberingSummaryPath -Value ([ordered]@{
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
        coverage_score = 100
        catalog_document_keys = @("contract.docx", "invoice.docx")
        baseline_document_keys = @("contract.docx", "invoice.docx")
        matched_document_keys = @("contract.docx", "invoice.docx")
    }
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "numbering_catalog_governance.style_numbering_issues"
            severity = "error"
            status = "blocked"
            message = "Style numbering audit reported issues."
            action = "review_style_numbering_audit"
        }
    )
    action_items = @(
        [ordered]@{
            id = "preview_style_numbering_repair"
            action = "preview_style_numbering_repair"
            title = "Preview style numbering repair"
        }
    )
})

Write-JsonFile -Path $tableSummaryPath -Value ([ordered]@{
    schema = "featherdoc.table_layout_delivery_governance_report.v1"
    status = "needs_review"
    release_ready = $false
    delivery_quality_score = 42
    delivery_quality_level = "blocked"
    delivery_quality = [ordered]@{
        score = 42
        level = "blocked"
        unresolved_item_count = 2
        table_position_automatic_count = 1
        table_position_review_count = 1
        pdf_floating_table_support_coverage = "4/9 supported (44 percent); metadata_only=5"
        pdf_floating_table_reviewer_focus = "review metadata-only tblpPr fields before approving PDF-layout-sensitive release."
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
    }
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "table_layout_delivery.floating_table_review_pending"
            severity = "warning"
            status = "needs_review"
            message = "Floating table plans still need review."
            action = "review_table_position_plan"
        }
    )
    action_items = @(
        [ordered]@{
            id = "run_table_style_quality_visual_regression"
            action = "run_table_style_quality_visual_regression"
            title = "Generate Word-rendered table layout evidence"
        }
    )
})

Write-JsonFile -Path $contentControlSummaryPath -Value ([ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "error"
            status = "placeholder_visible"
            message = "Bound content control still shows placeholder text."
            action = "sync_or_fill_bound_content_control"
            repair_action_classes = @("release_blocking", "auto_repair_candidate", "manual_confirmation_required")
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
            id = "review_duplicate_content_control_binding"
            action = "review_duplicate_content_control_binding"
            title = "Review repeated content controls that share one Custom XML binding"
            repair_action_classes = @("manual_confirmation_required")
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
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

Write-JsonFile -Path $autoDiscoverDocumentSkeletonRollupPath -Value `
    (Get-Content -Raw -Encoding UTF8 -LiteralPath $documentSkeletonRollupPath | ConvertFrom-Json)

Write-JsonFile -Path $autoDiscoverNumberingSummaryPath -Value ([ordered]@{
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
        coverage_score = 100
        catalog_document_keys = @("contract.docx", "invoice.docx")
        baseline_document_keys = @("contract.docx", "invoice.docx")
        matched_document_keys = @("contract.docx", "invoice.docx")
    }
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "numbering_catalog_governance.style_numbering_issues"
            severity = "error"
            status = "blocked"
            message = "Autodiscovered numbering governance issue."
            action = "review_style_numbering_audit"
        }
    )
    action_items = @(
        [ordered]@{
            id = "preview_style_numbering_repair"
            action = "preview_style_numbering_repair"
            title = "Preview style numbering repair"
        }
    )
})

Write-JsonFile -Path $autoDiscoverTableSummaryPath -Value ([ordered]@{
    schema = "featherdoc.table_layout_delivery_governance_report.v1"
    status = "needs_review"
    release_ready = $false
    delivery_quality_score = 42
    delivery_quality_level = "blocked"
    delivery_quality = [ordered]@{
        score = 42
        level = "blocked"
        unresolved_item_count = 2
        table_position_automatic_count = 1
        table_position_review_count = 1
        pdf_floating_table_support_coverage = "4/9 supported (44 percent); metadata_only=5"
        pdf_floating_table_reviewer_focus = "review metadata-only tblpPr fields before approving PDF-layout-sensitive release."
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
    }
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "table_layout_delivery.floating_table_review_pending"
            severity = "warning"
            status = "needs_review"
            message = "Autodiscovered floating table plan still needs review."
            action = "review_table_position_plan"
        }
    )
    action_items = @(
        [ordered]@{
            id = "run_table_style_quality_visual_regression"
            action = "run_table_style_quality_visual_regression"
            title = "Generate Word-rendered table layout evidence"
        }
    )
})

Write-JsonFile -Path $autoDiscoverContentControlSummaryPath -Value ([ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "error"
            status = "placeholder_visible"
            message = "Autodiscovered bound content control still shows placeholder text."
            action = "sync_or_fill_bound_content_control"
            repair_action_classes = @("release_blocking", "auto_repair_candidate", "manual_confirmation_required")
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
            id = "review_duplicate_content_control_binding"
            action = "review_duplicate_content_control_binding"
            title = "Review repeated content controls that share one Custom XML binding"
            repair_action_classes = @("manual_confirmation_required")
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
            source_json = "output/content-control-data-binding/inspect-content-controls.json"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            input_docx = "samples/invoice.docx"
            input_docx_display = ".\samples\invoice.docx"
            template_name = "invoice-template"
            schema_target = "invoice"
            target_mode = "resolved-section-targets"
        }
    )
    warning_count = 1
    warnings = @(
        [ordered]@{
            id = "custom_xml_sync_evidence_missing"
            action = "run_content_control_custom_xml_sync"
            repair_strategy = "run_content_control_custom_xml_sync"
            repair_hint = "Run Custom XML sync evidence collection before treating the content-control binding governance summary as release evidence."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
            message = "Data-bound content controls were inspected, but no Custom XML sync result was provided."
            source_json = "output/content-control-data-binding-governance/summary.json"
            source_json_display = ".\output\content-control-data-binding-governance\summary.json"
            input_docx = "samples/invoice.docx"
            input_docx_display = ".\samples\invoice.docx"
            template_name = "invoice-template"
            schema_target = "invoice"
            target_mode = "resolved-section-targets"
        }
    )
})

Write-JsonFile -Path $autoDiscoverProjectSummaryPath -Value ([ordered]@{
    schema = "featherdoc.project_template_delivery_readiness_report.v1"
    status = "blocked"
    release_ready = $false
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_onboarding.schema_approval"
            severity = "error"
            status = "blocked"
            message = "Autodiscovered project template schema approval is pending."
            action = "review_schema_update_candidate"
            source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
            source_json = "output/project-template-onboarding-governance/summary.json"
            source_json_display = ".\output\project-template-onboarding-governance\summary.json"
            onboarding_governance_status = "pending_review"
            onboarding_governance_release_ready = "False"
            onboarding_governance_schema_approval_status_summary = "pending_review"
            onboarding_governance_source_report_display = ".\output\project-template-onboarding-governance\summary.json"
            onboarding_governance_source_json_display = ".\output\project-template-onboarding-governance\summary.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_invoice_schema"
            action = "review_schema_update_candidate"
            title = "Review invoice schema before release"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1"
            source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
            source_json = "output/project-template-onboarding-governance/summary.json"
            source_json_display = ".\output\project-template-onboarding-governance\summary.json"
            onboarding_governance_status = "pending_review"
            onboarding_governance_release_ready = "False"
            onboarding_governance_schema_approval_status_summary = "pending_review"
            onboarding_governance_source_report_display = ".\output\project-template-onboarding-governance\summary.json"
            onboarding_governance_source_json_display = ".\output\project-template-onboarding-governance\summary.json"
        }
    )
})

Write-JsonFile -Path $autoDiscoverCalibrationSummaryPath -Value ([ordered]@{
    schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
    status = "pending_review"
    release_ready = $false
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "schema_patch_confidence_calibration.pending_schema_approvals"
            severity = "error"
            status = "pending_review"
            message = "Autodiscovered schema patch confidence calibration has pending approvals."
            action = "resolve_pending_schema_approvals"
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_json = "output/schema-patch-confidence-calibration/summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        }
    )
    action_item_count = 1
    action_items = @(
        [ordered]@{
            id = "resolve_pending_schema_approvals"
            action = "resolve_pending_schema_approvals"
            title = "Resolve pending schema approvals before tightening confidence thresholds"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_json = "output/schema-patch-confidence-calibration/summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        }
    )
    warning_count = 1
    warnings = @(
        [ordered]@{
            id = "schema_patch_confidence_calibration.unscored_candidates"
            action = "add_explicit_confidence_metadata"
            message = "Some schema patch candidates do not carry explicit confidence metadata."
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_json = "output/schema-patch-confidence-calibration/summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        }
    )
})

Write-JsonFile -Path $autoDiscoverDocxReadinessSummaryPath -Value ([ordered]@{
    schema = "featherdoc.docx_functional_smoke_readiness.v1"
    status = "pass"
    verdict = "pass"
    release_ready = $true
    docx_functional_smoke_ready = $true
    release_blocker_count = 0
    release_blockers = @()
    action_item_count = 0
    action_items = @()
    warning_count = 0
    warnings = @()
})
