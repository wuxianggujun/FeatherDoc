function Add-RepairActionClassMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Item
    )

    $repairActionClasses = @(
        Get-JsonArray -Object $Item -Name "repair_action_classes" |
            Where-Object { $null -ne $_ -and -not [string]::IsNullOrWhiteSpace([string]$_) } |
            ForEach-Object { [string]$_ }
    )
    if ($repairActionClasses.Count -gt 0) {
        $Lines.Add("  - repair_action_classes: ``$($repairActionClasses -join ', ')``") | Out-Null
    }
}

function Add-SchemaCorpusMetadataMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Item
    )

    foreach ($fieldName in @(
            "business_document_type",
            "source_business_document_type",
            "corpus_role",
            "source_corpus_role",
            "business_document_type_mismatch",
            "corpus_role_mismatch",
            "mismatched_corpus_metadata_count",
            "mismatched_business_document_type_count",
            "mismatched_corpus_role_count",
            "candidate_name",
            "schema_update_candidate"
        )) {
        $fieldValue = Get-JsonProperty -Object $Item -Name $fieldName
        if ($null -eq $fieldValue) { continue }
        $fieldDisplay = if ($fieldValue -is [System.Collections.IEnumerable] -and $fieldValue -isnot [string]) {
            @($fieldValue | ForEach-Object { [string]$_ }) -join ", "
        } else {
            [string]$fieldValue
        }
        if (-not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
            $Lines.Add("  - ${fieldName}: ``$fieldDisplay``") | Out-Null
        }
    }
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Release Blocker Rollup Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Source reports: ``$($Summary.source_report_count)``") | Out-Null
    $lines.Add("- Source failures: ``$($Summary.source_failure_count)``") | Out-Null
    $lines.Add("- source_failure_count: ``$($Summary.source_failure_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($Summary.action_item_count)``") | Out-Null
    $lines.Add("- Informational action items: ``$($Summary.informational_action_item_count)``") | Out-Null
    $lines.Add("- Warnings: ``$($Summary.warning_count)``") | Out-Null
    $lines.Add("- Blocker source schemas: ``$(Get-SummaryGroupMarkdownText -Items $Summary.blocker_source_schema_summary -PropertyName "source_schema")``") | Out-Null
    $lines.Add("- Action item source schemas: ``$(Get-SummaryGroupMarkdownText -Items $Summary.action_item_source_schema_summary -PropertyName "source_schema")``") | Out-Null
    $lines.Add("- Informational action item source schemas: ``$(Get-SummaryGroupMarkdownText -Items $Summary.informational_action_item_source_schema_summary -PropertyName "source_schema")``") | Out-Null
    $lines.Add("- Warning source schemas: ``$(Get-SummaryGroupMarkdownText -Items $Summary.warning_source_schema_summary -PropertyName "source_schema")``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Governance Metric Review Focus") | Out-Null
    $lines.Add("") | Out-Null
    $focusMetrics = @(
        [ordered]@{
            metric = "real_corpus_confidence"
            id = "numbering_catalog_governance.real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            label = "Numbering real-corpus confidence"
        },
        [ordered]@{
            metric = "delivery_quality"
            id = "table_layout_delivery_governance.delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
            label = "Table/layout delivery quality"
        }
    )
    foreach ($focusMetric in $focusMetrics) {
        $metric = Get-GovernanceMetricByContract `
            -Metrics $Summary.governance_metrics `
            -Metric ([string]$focusMetric.metric) `
            -Id ([string]$focusMetric.id) `
            -ReportId ([string]$focusMetric.report_id) `
            -SourceSchema ([string]$focusMetric.source_schema)

        if ($null -eq $metric) {
            $lines.Add("- **$($focusMetric.label)** (``$($focusMetric.metric)``): missing") | Out-Null
            continue
        }

        $lines.Add("- **$($focusMetric.label)** ``$($metric.id)``: metric=``$($metric.metric)`` level=``$($metric.level)`` score=``$($metric.score)`` report=``$($metric.report_id)`` source_schema=``$($metric.source_schema)``") | Out-Null
        Add-TraceabilityMarkdownLines -Lines $lines -Item $metric
        $lines.Add("  - source_report_display: ``$($metric.source_report_display)``") | Out-Null
        $lines.Add("  - source_json_display: ``$($metric.source_json_display)``") | Out-Null
        Add-GovernanceMetricDetailLines -Lines $lines -Metric $metric
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Governance Metrics") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.governance_metrics).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($metric in @($Summary.governance_metrics)) {
            $lines.Add("- ``$($metric.id)``: report=``$($metric.report_id)`` metric=``$($metric.metric)`` level=``$($metric.level)`` score=``$($metric.score)`` schema=``$($metric.source_schema)`` source_report_display=``$($metric.source_report_display)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $metric
            $lines.Add("  - source_json_display: ``$($metric.source_json_display)``") | Out-Null
            Add-GovernanceMetricDetailLines -Lines $lines -Metric $metric
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Source Report Contracts") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.source_reports).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($report in @($Summary.source_reports)) {
            $lines.Add("- ``$($report.schema)``: status=``$($report.status)`` ready=``$($report.release_ready)`` path=``$($report.path_display)``") | Out-Null
            $preflightReady = Get-JsonProperty -Object $report -Name "preflight_ready"
            if ($null -ne $preflightReady) {
                $lines.Add("  - preflight_ready: ``$preflightReady``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$report.error)) {
                $lines.Add("  - error: ``$($report.error)``") | Out-Null
            }
            $payloadPreview = Get-JsonString -Object $report -Name "payload_preview"
            if (-not [string]::IsNullOrWhiteSpace($payloadPreview)) {
                $lines.Add("  - payload_preview: ``$payloadPreview``") | Out-Null
            }
            $fullVisualGateRequired = Get-JsonProperty -Object $report -Name "full_visual_gate_required"
            if ($null -ne $fullVisualGateRequired) {
                $lines.Add("  - full_visual_gate_required: ``$fullVisualGateRequired``") | Out-Null
            }
            $fullVisualGateStatus = Get-JsonString -Object $report -Name "full_visual_gate_status"
            if (-not [string]::IsNullOrWhiteSpace($fullVisualGateStatus)) {
                $lines.Add("  - full_visual_gate_status: ``$fullVisualGateStatus``") | Out-Null
            }
            foreach ($fieldName in @(
                    "verdict",
                    "docx_functional_smoke_ready",
                    "evidence_scope",
                    "evidence_scope_note",
                    "boundary",
                    "marker",
                    "summary_json_display",
                    "report_markdown_display",
                    "pdf_visual_gate_status",
                    "pdf_visual_gate_verdict",
                    "pdf_visual_gate_finalizable",
                    "pdf_visual_gate_summary_json_display",
                    "pdf_visual_gate_aggregate_contact_sheet_display",
                    "pdf_visual_gate_cjk_manifest_count",
                    "pdf_visual_gate_cjk_copy_search_count",
                    "pdf_visual_gate_cjk_missing_text_count",
                    "pdf_visual_gate_visual_baseline_manifest_count",
                    "pdf_visual_gate_visual_baseline_count",
                    "pdf_bounded_ctest_summary_count",
                    "pdf_bounded_ctest_pass_count",
                    "pdf_bounded_ctest_skipped_test_count",
                    "pdf_bounded_ctest_selected_test_count",
                    "pdf_bounded_ctest_subsets",
                    "pdf_bounded_ctest_summary_json_display",
                    "pdf_full_ctest_readiness_requested",
                    "pdf_full_ctest_readiness_status",
                    "pdf_full_ctest_readiness_verdict",
                    "pdf_full_ctest_readiness_release_ready",
                    "pdf_full_ctest_readiness_visual_gate_release_evidence_accepted",
                    "pdf_full_ctest_readiness_visual_gate_fresh_full_guarded_evidence",
                    "pdf_full_ctest_readiness_visual_gate_segmented_full_coverage_evidence",
                    "pdf_full_ctest_readiness_summary_json_display",
                    "pdf_full_ctest_readiness_full_ctest_summary_json_display",
                    "pdf_full_ctest_readiness_full_ctest_status",
                    "pdf_full_ctest_readiness_full_ctest_verdict",
                    "pdf_full_ctest_readiness_outer_guard_status",
                    "pdf_full_ctest_readiness_outer_guard_timed_out",
                    "pdf_full_ctest_readiness_selected_test_count",
                    "pdf_full_ctest_readiness_completed_test_count",
                    "pdf_full_ctest_readiness_passed_test_count",
                    "pdf_full_ctest_readiness_failed_test_count",
                    "pdf_full_ctest_readiness_skipped_test_count",
                    "pdf_full_ctest_readiness_not_run_test_count",
                    "pdf_full_ctest_readiness_completion_percent",
                    "pdf_full_ctest_readiness_remaining_test_count",
                    "pdf_full_ctest_readiness_zero_failed_tests_observed",
                    "pdf_full_ctest_readiness_boundary",
                    "pdf_full_ctest_readiness_marker",
                    "pdf_visual_gate_attempt_status",
                    "pdf_visual_gate_attempt_verdict",
                    "pdf_visual_gate_attempt_full_visual_gate_status",
                    "pdf_visual_gate_attempt_evidence_scope",
                    "pdf_visual_gate_attempt_summary_json_display",
                    "pdf_visual_gate_attempt_stage_count",
                    "pdf_visual_gate_attempt_passed_stage_count",
                    "pdf_visual_gate_attempt_failed_stage_count",
                    "pdf_visual_gate_attempt_incomplete_stage_count",
                    "pdf_visual_gate_attempt_outer_guard_status",
                    "pdf_visual_gate_attempt_outer_guard_timed_out",
                    "pdf_visual_gate_attempt_outer_guard_timeout_seconds",
                    "pdf_visual_gate_attempt_pdf_cli_export_status",
                    "pdf_visual_gate_attempt_pdf_regression_status",
                    "pdf_visual_gate_attempt_pdf_regression_selected_test_count",
                    "pdf_visual_gate_attempt_pdf_regression_failed_test_count",
                    "pdf_visual_gate_attempt_pdf_regression_skipped_test_count",
                    "pdf_visual_gate_attempt_unicode_font_status",
                    "pdf_visual_gate_attempt_cjk_copy_search_status",
                    "pdf_visual_gate_attempt_cjk_copy_search_count",
                    "pdf_visual_gate_attempt_cjk_copy_search_missing_text_count",
                    "pdf_visual_gate_attempt_visual_baseline_render_status",
                    "pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count",
                    "pdf_visual_gate_attempt_expected_visual_render_count",
                    "pdf_visual_gate_attempt_visual_baseline_missing_pdf_count",
                    "pdf_visual_gate_attempt_visual_baseline_pdf_total_bytes",
                    "pdf_visual_gate_attempt_visual_baseline_png_page_count",
                    "pdf_visual_gate_attempt_visual_baseline_missing_png_page_count",
                    "pdf_visual_gate_attempt_visual_baseline_png_total_bytes",
                    "pdf_visual_gate_attempt_visual_baseline_unreadable_png_dimension_count",
                    "pdf_visual_gate_attempt_aggregate_contact_sheet_status",
                    "pdf_visual_gate_attempt_aggregate_contact_sheet_display",
                    "pdf_visual_segmented_gate_status",
                    "pdf_visual_segmented_gate_verdict",
                    "pdf_visual_segmented_gate_full_visual_gate_status",
                    "pdf_visual_segmented_gate_evidence_scope",
                    "pdf_visual_segmented_gate_boundary",
                    "pdf_visual_segmented_gate_summary_json_display",
                    "pdf_visual_segmented_gate_slice_summary_count",
                    "pdf_visual_segmented_gate_slice_pass_count",
                    "pdf_visual_segmented_gate_slice_failed_count",
                    "pdf_visual_segmented_gate_covered_baseline_count",
                    "pdf_visual_segmented_gate_expected_visual_render_count",
                    "pdf_visual_segmented_gate_attempt_stage_count",
                    "pdf_visual_segmented_gate_attempt_passed_stage_count",
                    "pdf_visual_segmented_gate_visual_baseline_render_status",
                    "pdf_visual_segmented_gate_aggregate_contact_sheet_status",
                    "pdf_visual_segmented_gate_aggregate_contact_sheet_display",
                    "pdf_visual_segmented_gate_aggregate_contact_sheet_bytes",
                    "pdf_visual_segmented_gate_aggregate_rebuild_status",
                    "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count"
                )) {
                $fieldValue = Get-JsonProperty -Object $report -Name $fieldName
                $fieldDisplay = if ($fieldValue -is [System.Collections.IEnumerable] -and $fieldValue -isnot [string]) {
                    @($fieldValue | ForEach-Object { [string]$_ }) -join ", "
                } else {
                    [string]$fieldValue
                }
                if ($null -ne $fieldValue -and -not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
                    $lines.Add("  - ${fieldName}: ``$fieldDisplay``") | Out-Null
                }
            }
            Add-ManifestSignoffEntrypointsMarkdownLines -Lines $lines -Report $report
            Add-ProjectTemplateReadinessChecklistEntrypointsMarkdownLines -Lines $lines -Report $report
            Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditMarkdownLines -Lines $lines -Report $report
            Add-WordVisualStandardReviewMetadataMarkdownLines -Lines $lines -Report $report
            $controlledVisualSmokeAvailable = Get-JsonProperty -Object $report -Name "controlled_visual_smoke_available"
            if ($null -ne $controlledVisualSmokeAvailable) {
                $lines.Add("  - controlled_visual_smoke_available: ``$controlledVisualSmokeAvailable``") | Out-Null
            }
            $controlledVisualSmokeStatus = Get-JsonString -Object $report -Name "controlled_visual_smoke_status"
            if (-not [string]::IsNullOrWhiteSpace($controlledVisualSmokeStatus)) {
                $lines.Add("  - controlled_visual_smoke_status: ``$controlledVisualSmokeStatus``") | Out-Null
            }
            $controlledVisualSmokePassed = Get-JsonProperty -Object $report -Name "controlled_visual_smoke_passed"
            if ($null -ne $controlledVisualSmokePassed) {
                $lines.Add("  - controlled_visual_smoke_passed: ``$controlledVisualSmokePassed``") | Out-Null
            }
            $controlledVisualSmokeCaseCount = Get-JsonProperty -Object $report -Name "controlled_visual_smoke_case_count"
            if ($null -ne $controlledVisualSmokeCaseCount) {
                $lines.Add("  - controlled_visual_smoke_case_count: ``$controlledVisualSmokeCaseCount``") | Out-Null
            }
            $controlledVisualSmokeJsonDisplay = Get-JsonString -Object $report -Name "controlled_visual_smoke_json_display"
            if (-not [string]::IsNullOrWhiteSpace($controlledVisualSmokeJsonDisplay)) {
                $lines.Add("  - controlled_visual_smoke_json_display: ``$controlledVisualSmokeJsonDisplay``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$report.latest_schema_approval_gate_status)) {
                $lines.Add("  - latest_schema_approval_gate_status: ``$($report.latest_schema_approval_gate_status)``") | Out-Null
            }
            if (@($report.schema_approval_status_summary).Count -gt 0) {
                $statusParts = @($report.schema_approval_status_summary | ForEach-Object { "$($_.status)=$($_.count)" })
                $lines.Add("  - schema_approval_status_summary: ``$($statusParts -join ', ')``") | Out-Null
            }
            if (@($report.business_document_type_summary).Count -gt 0) {
                $businessDocumentTypeSummary = Get-SummaryGroupMarkdownText `
                    -Items @($report.business_document_type_summary) `
                    -PropertyName "document_type"
                $lines.Add("  - business_document_type_summary: ``$businessDocumentTypeSummary``") | Out-Null
            }
            if (@($report.corpus_role_summary).Count -gt 0) {
                $corpusRoleSummary = Get-SummaryGroupMarkdownText `
                    -Items @($report.corpus_role_summary) `
                    -PropertyName "corpus_role"
                $lines.Add("  - corpus_role_summary: ``$corpusRoleSummary``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Blocker Summary") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.blocker_id_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.blocker_id_summary)) {
            $lines.Add("- ``$($item.id)``: $($item.count)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.composite_id)``: project=``$($blocker.project_id)`` template=``$($blocker.template_name)`` candidate=``$($blocker.candidate_type)`` action=``$($blocker.action)`` schema=``$($blocker.source_schema)`` source_report_display=``$($blocker.source_report_display)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $blocker
            Add-SchemaCorpusMetadataMarkdownLines -Lines $lines -Item $blocker
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.source_json_display)) {
                $lines.Add("  - source_json_display: ``$($blocker.source_json_display)``") | Out-Null
            }
            Add-RepairActionClassMarkdownLines -Lines $lines -Item $blocker
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.message)) {
                $lines.Add("  - $($blocker.message)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($blocker.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.repair_hint)) {
                $lines.Add("  - repair_hint: $($blocker.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.command_template)) {
                $lines.Add("  - command_template: ``$($blocker.command_template)``") | Out-Null
            }
            $reviewerActionSummary = Get-JsonString -Object $blocker -Name "reviewer_action_summary"
            $reviewerActionReason = Get-JsonString -Object $blocker -Name "reviewer_action_reason"
            if (-not [string]::IsNullOrWhiteSpace($reviewerActionSummary)) {
                $lines.Add("  - reviewer_action: ``$reviewerActionSummary``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($reviewerActionReason)) {
                $lines.Add("  - reviewer_action_reason: $reviewerActionReason") | Out-Null
            }
            Add-ReviewerActionsMarkdownLines -Lines $lines -Item $blocker
            Add-ReadinessActionEvidenceMarkdownLines -Lines $lines -Item $blocker
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.composite_id)``: project=``$($item.project_id)`` template=``$($item.template_name)`` candidate=``$($item.candidate_type)`` action=``$($item.action)`` category=``$($item.category)`` release_blocking=``$($item.release_blocking)`` optional=``$($item.optional)`` schema=``$($item.source_schema)`` source_report_display=``$($item.source_report_display)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $item
            Add-SchemaCorpusMetadataMarkdownLines -Lines $lines -Item $item
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.source_json_display)) {
                $lines.Add("  - source_json_display: ``$($item.source_json_display)``") | Out-Null
            }
            Add-RepairActionClassMarkdownLines -Lines $lines -Item $item
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($item.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_hint)) {
                $lines.Add("  - repair_hint: $($item.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.command_template)) {
                $lines.Add("  - command_template: ``$($item.command_template)``") | Out-Null
            }
            $reviewerActionSummary = Get-JsonString -Object $item -Name "reviewer_action_summary"
            $reviewerActionReason = Get-JsonString -Object $item -Name "reviewer_action_reason"
            if (-not [string]::IsNullOrWhiteSpace($reviewerActionSummary)) {
                $lines.Add("  - reviewer_action: ``$reviewerActionSummary``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($reviewerActionReason)) {
                $lines.Add("  - reviewer_action_reason: $reviewerActionReason") | Out-Null
            }
            Add-ReviewerActionsMarkdownLines -Lines $lines -Item $item
            Add-ReadinessActionEvidenceMarkdownLines -Lines $lines -Item $item
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Informational Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.informational_action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.informational_action_items)) {
            $lines.Add("- ``$($item.composite_id)``: project=``$($item.project_id)`` template=``$($item.template_name)`` candidate=``$($item.candidate_type)`` action=``$($item.action)`` category=``$($item.category)`` release_blocking=``$($item.release_blocking)`` optional=``$($item.optional)`` schema=``$($item.source_schema)`` source_report_display=``$($item.source_report_display)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $item
            Add-SchemaCorpusMetadataMarkdownLines -Lines $lines -Item $item
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.source_json_display)) {
                $lines.Add("  - source_json_display: ``$($item.source_json_display)``") | Out-Null
            }
            Add-RepairActionClassMarkdownLines -Lines $lines -Item $item
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($item.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_hint)) {
                $lines.Add("  - repair_hint: $($item.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.command_template)) {
                $lines.Add("  - command_template: ``$($item.command_template)``") | Out-Null
            }
            $reviewerActionSummary = Get-JsonString -Object $item -Name "reviewer_action_summary"
            $reviewerActionReason = Get-JsonString -Object $item -Name "reviewer_action_reason"
            if (-not [string]::IsNullOrWhiteSpace($reviewerActionSummary)) {
                $lines.Add("  - reviewer_action: ``$reviewerActionSummary``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($reviewerActionReason)) {
                $lines.Add("  - reviewer_action_reason: $reviewerActionReason") | Out-Null
            }
            Add-ReviewerActionsMarkdownLines -Lines $lines -Item $item
            Add-ReadinessActionEvidenceMarkdownLines -Lines $lines -Item $item
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.warnings).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($warning in @($Summary.warnings)) {
            $lines.Add("- ``$($warning.id)``: project=``$($warning.project_id)`` template=``$($warning.template_name)`` candidate=``$($warning.candidate_type)`` action=``$($warning.action)`` schema=``$($warning.source_schema)`` source_report_display=``$($warning.source_report_display)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $warning
            Add-SchemaCorpusMetadataMarkdownLines -Lines $lines -Item $warning
            if (-not [string]::IsNullOrWhiteSpace([string]$warning.source_json_display)) {
                $lines.Add("  - source_json_display: ``$($warning.source_json_display)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$warning.message)) {
                $lines.Add("  - $($warning.message)") | Out-Null
            }
            Add-RepairActionClassMarkdownLines -Lines $lines -Item $warning
            $repairStrategy = Get-JsonString -Object $warning -Name "repair_strategy"
            $repairHint = Get-JsonString -Object $warning -Name "repair_hint"
            $commandTemplate = Get-JsonString -Object $warning -Name "command_template"
            if (-not [string]::IsNullOrWhiteSpace($repairStrategy)) {
                $lines.Add("  - repair_strategy: ``$repairStrategy``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($repairHint)) {
                $lines.Add("  - repair_hint: $repairHint") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($commandTemplate)) {
                $lines.Add("  - command_template: ``$commandTemplate``") | Out-Null
            }
        }
    }
    return @($lines)
}
