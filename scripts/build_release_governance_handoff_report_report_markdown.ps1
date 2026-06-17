function New-ReportMarkdown {
    param($Summary)

    function Add-TraceabilityMarkdownLines {
        param(
            [System.Collections.Generic.List[string]]$Lines,
            [object]$Item
        )

        foreach ($fieldName in @("source_report", "source_json")) {
            $fieldValue = Get-JsonString -Object $Item -Name $fieldName
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                $Lines.Add("  - ${fieldName}: ``$fieldValue``") | Out-Null
            }
        }
    }

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

    function Format-OnboardingSchemaApprovalStatusSummary {
        param(
            [object[]]$Values,
            [string]$Fallback = ""
        )

        $parts = @(
            foreach ($value in @($Values)) {
                if ($null -eq $value) {
                    continue
                }
                if ($value -is [string]) {
                    if (-not [string]::IsNullOrWhiteSpace($value)) {
                        [string]$value
                    }
                    continue
                }

                $status = Get-JsonString -Object $value -Name "status"
                $count = Get-JsonString -Object $value -Name "count"
                if (-not [string]::IsNullOrWhiteSpace($status) -and -not [string]::IsNullOrWhiteSpace($count)) {
                    "${status}=${count}"
                } elseif (-not [string]::IsNullOrWhiteSpace($status)) {
                    $status
                } elseif (-not [string]::IsNullOrWhiteSpace($count)) {
                    "count=${count}"
                } elseif (-not [string]::IsNullOrWhiteSpace([string]$value)) {
                    [string]$value
                }
            }
        )

        if ($parts.Count -eq 0) {
            return $Fallback
        }

        return ($parts -join ", ")
    }

    function Format-OnboardingNextActionSummary {
        param(
            [object[]]$Values,
            [string]$Fallback = ""
        )

        $parts = @(
            foreach ($value in @($Values)) {
                if ($null -eq $value) {
                    continue
                }
                if ($value -is [string]) {
                    if (-not [string]::IsNullOrWhiteSpace($value)) {
                        [string]$value
                    }
                    continue
                }

                $itemParts = New-Object 'System.Collections.Generic.List[string]'
                foreach ($fieldName in @("action", "status", "blocker_id", "reason")) {
                    $fieldValue = Get-JsonString -Object $value -Name $fieldName
                    if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                        $itemParts.Add("${fieldName}=${fieldValue}") | Out-Null
                    }
                }
                if ($itemParts.Count -gt 0) {
                    $itemParts.ToArray() -join " "
                } elseif (-not [string]::IsNullOrWhiteSpace([string]$value)) {
                    [string]$value
                }
            }
        )

        if ($parts.Count -eq 0) {
            return $Fallback
        }

        return ($parts -join ", ")
    }

    function Add-ProjectTemplateOnboardingContractMarkdownLines {
        param(
            [System.Collections.Generic.List[string]]$Lines,
            [object]$Item
        )

        $sourceSchema = Get-JsonString -Object $Item -Name "source_schema"
        if (-not [string]::Equals($sourceSchema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
            return
        }

        $schemaApprovalValues = @(Get-JsonArray -Object $Item -Name "onboarding_governance_schema_approval_status_summary")
        if ($schemaApprovalValues.Count -eq 0) {
            $schemaApprovalValues = @(Get-JsonArray -Object $Item -Name "schema_approval_status_summary")
        }
        $schemaApprovalSummary = Format-OnboardingSchemaApprovalStatusSummary `
            -Values $schemaApprovalValues `
            -Fallback (Get-JsonString -Object $Item -Name "onboarding_governance_status")
        if ([string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
            $schemaApprovalSummary = "unknown"
        }

        $status = Get-JsonString -Object $Item -Name "onboarding_governance_status"
        $releaseReady = Get-JsonString -Object $Item -Name "onboarding_governance_release_ready"
        $nextAction = Format-OnboardingNextActionSummary -Values @(
            Get-JsonProperty -Object $Item -Name "onboarding_governance_next_action"
        )
        $nextActionSummaryValues = @(Get-JsonArray -Object $Item -Name "onboarding_governance_next_action_summary")
        if ($nextActionSummaryValues.Count -eq 0) {
            $nextActionSummaryValues = @(Get-JsonArray -Object $Item -Name "next_action_summary")
        }
        $nextActionSummary = Format-OnboardingNextActionSummary -Values $nextActionSummaryValues
        $nextActionGroupCount = Get-JsonString -Object $Item -Name "onboarding_governance_next_action_group_count"
        if ([string]::IsNullOrWhiteSpace($nextActionGroupCount)) {
            $nextActionGroupCount = Get-JsonString -Object $Item -Name "next_action_group_count"
        }

        $sourceReportDisplay = Get-JsonString -Object $Item -Name "onboarding_governance_source_report_display"
        if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
            $sourceReportDisplay = Get-JsonString -Object $Item -Name "source_report_display"
        }
        $sourceJsonDisplay = Get-JsonString -Object $Item -Name "onboarding_governance_source_json_display"
        if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            $sourceJsonDisplay = Get-JsonString -Object $Item -Name "source_json_display"
        }

        $Lines.Add("  - project_template_onboarding_governance_contract:") | Out-Null
        $Lines.Add("    - source_schema: ``featherdoc.project_template_onboarding_governance_report.v1``") | Out-Null
        if (-not [string]::IsNullOrWhiteSpace($status)) {
            $Lines.Add("    - status: ``$status``") | Out-Null
        }
        if (-not [string]::IsNullOrWhiteSpace($releaseReady)) {
            $Lines.Add("    - release_ready: ``$releaseReady``") | Out-Null
        }
        $Lines.Add("    - schema_approval_status_summary: ``$schemaApprovalSummary``") | Out-Null
        if (-not [string]::IsNullOrWhiteSpace($nextAction)) {
            $Lines.Add("    - next_action: ``$nextAction``") | Out-Null
        }
        if (-not [string]::IsNullOrWhiteSpace($nextActionSummary)) {
            $Lines.Add("    - next_action_summary: ``$nextActionSummary``") | Out-Null
        }
        if (-not [string]::IsNullOrWhiteSpace($nextActionGroupCount)) {
            $Lines.Add("    - next_action_group_count: ``$nextActionGroupCount``") | Out-Null
        }
        if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
            $Lines.Add("    - source_report_display: ``$sourceReportDisplay``") | Out-Null
        }
        if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            $Lines.Add("    - source_json_display: ``$sourceJsonDisplay``") | Out-Null
        }
    }

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Release Governance Handoff") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Reports loaded: ``$($Summary.loaded_report_count)`` / ``$($Summary.expected_report_count)``") | Out-Null
    $lines.Add("- Missing reports: ``$($Summary.missing_report_count)``") | Out-Null
    $lines.Add("- Failed reports: ``$($Summary.failed_report_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($Summary.action_item_count)``") | Out-Null
    $lines.Add("- Informational action items: ``$($Summary.informational_action_item_count)``") | Out-Null
    $lines.Add("- Warnings: ``$($Summary.warning_count)``") | Out-Null
    $lines.Add("- Governance metrics: ``$($Summary.governance_metric_count)``") | Out-Null
    $lines.Add("") | Out-Null

    if ([bool]$Summary.release_blocker_rollup.included) {
        $rollup = $Summary.release_blocker_rollup
        $lines.Add("## Release Blocker Rollup") | Out-Null
        $lines.Add("") | Out-Null
        $lines.Add("- Status: ``$($rollup.status)``") | Out-Null
        $lines.Add("- Summary: ``$($rollup.summary_json_display)``") | Out-Null
        $lines.Add("- Report: ``$($rollup.report_markdown_display)``") | Out-Null
        $lines.Add("- Source reports: ``$($rollup.source_report_count)``") | Out-Null
        $lines.Add("- Release blockers: ``$($rollup.release_blocker_count)``") | Out-Null
        $lines.Add("- Action items: ``$($rollup.action_item_count)``") | Out-Null
        $lines.Add("- Informational action items: ``$($rollup.informational_action_item_count)``") | Out-Null
        $lines.Add("- Warnings: ``$($rollup.warning_count)``") | Out-Null
        $lines.Add("- Governance metrics: ``$($rollup.governance_metric_count)``") | Out-Null
        $lines.Add("- Blocker source schemas: ``$(Get-SummaryGroupMarkdownText -Items @($rollup.blocker_source_schema_summary))``") | Out-Null
        $lines.Add("- Action item source schemas: ``$(Get-SummaryGroupMarkdownText -Items @($rollup.action_item_source_schema_summary))``") | Out-Null
        $lines.Add("- Informational action item source schemas: ``$(Get-SummaryGroupMarkdownText -Items @($rollup.informational_action_item_source_schema_summary))``") | Out-Null
        $lines.Add("- Warning source schemas: ``$(Get-SummaryGroupMarkdownText -Items @($rollup.warning_source_schema_summary))``") | Out-Null
        $lines.Add("- DOCX functional smoke readiness evidence source reports: ``$($rollup.docx_functional_smoke_readiness_evidence_source_report_count)``") | Out-Null
        foreach ($evidence in @($rollup.docx_functional_smoke_readiness_evidence_source_reports)) {
            $lines.Add("  - source_report: ``$($evidence.path_display)`` schema=``$($evidence.schema)``") | Out-Null
            $lines.Add("    - status: ``$($evidence.status)``") | Out-Null
            $lines.Add("    - verdict: ``$($evidence.verdict)``") | Out-Null
            $lines.Add("    - release_ready: ``$($evidence.release_ready)``") | Out-Null
            $lines.Add("    - docx_functional_smoke_ready: ``$($evidence.docx_functional_smoke_ready)``") | Out-Null
            $lines.Add("    - evidence_scope: ``$($evidence.evidence_scope)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.evidence_scope_note)) {
                $lines.Add("    - evidence_scope_note: $($evidence.evidence_scope_note)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.boundary)) {
                $lines.Add("    - boundary: $($evidence.boundary)") | Out-Null
            }
            $lines.Add("    - marker: ``$($evidence.marker)``") | Out-Null
            $lines.Add("    - summary_json_display: ``$($evidence.summary_json_display)``") | Out-Null
            $lines.Add("    - report_markdown_display: ``$($evidence.report_markdown_display)``") | Out-Null
        }
        $lines.Add("- PDF visual gate evidence source reports: ``$($rollup.pdf_visual_gate_evidence_source_report_count)``") | Out-Null
        foreach ($evidence in @($rollup.pdf_visual_gate_evidence_source_reports)) {
            $lines.Add("  - source_report: ``$($evidence.path_display)`` schema=``$($evidence.schema)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_status: ``$($evidence.pdf_visual_gate_status)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_verdict: ``$($evidence.pdf_visual_gate_verdict)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_finalizable: ``$($evidence.pdf_visual_gate_finalizable)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_summary_json_display: ``$($evidence.pdf_visual_gate_summary_json_display)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_aggregate_contact_sheet_display: ``$($evidence.pdf_visual_gate_aggregate_contact_sheet_display)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_cjk_manifest_count: ``$($evidence.pdf_visual_gate_cjk_manifest_count)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_cjk_copy_search_count: ``$($evidence.pdf_visual_gate_cjk_copy_search_count)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_visual_baseline_manifest_count: ``$($evidence.pdf_visual_gate_visual_baseline_manifest_count)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_visual_baseline_count: ``$($evidence.pdf_visual_gate_visual_baseline_count)``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_summary_count: ``$($evidence.pdf_bounded_ctest_summary_count)``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_pass_count: ``$($evidence.pdf_bounded_ctest_pass_count)``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_skipped_test_count: ``$($evidence.pdf_bounded_ctest_skipped_test_count)``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_selected_test_count: ``$($evidence.pdf_bounded_ctest_selected_test_count)``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_subsets: ``$(@($evidence.pdf_bounded_ctest_subsets) -join ', ')``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_summary_json_display: ``$(@($evidence.pdf_bounded_ctest_summary_json_display) -join ', ')``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_import_diagnostics_contract_tests: ``$(@($evidence.pdf_bounded_ctest_import_diagnostics_contract_tests) -join ', ')``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_import_diagnostics_contract_fields: ``$(@($evidence.pdf_bounded_ctest_import_diagnostics_contract_fields) -join ', ')``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_import_negative_boundary_contract_cases: ``$(@($evidence.pdf_bounded_ctest_import_negative_boundary_contract_cases) -join ', ')``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.pdf_full_ctest_readiness_status)) {
                $lines.Add("    - pdf_full_ctest_readiness_status: ``$($evidence.pdf_full_ctest_readiness_status)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_verdict: ``$($evidence.pdf_full_ctest_readiness_verdict)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_release_ready: ``$($evidence.pdf_full_ctest_readiness_release_ready)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_visual_gate_release_evidence_accepted: ``$($evidence.pdf_full_ctest_readiness_visual_gate_release_evidence_accepted)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_visual_gate_fresh_full_guarded_evidence: ``$($evidence.pdf_full_ctest_readiness_visual_gate_fresh_full_guarded_evidence)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_visual_gate_segmented_full_coverage_evidence: ``$($evidence.pdf_full_ctest_readiness_visual_gate_segmented_full_coverage_evidence)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_summary_json_display: ``$($evidence.pdf_full_ctest_readiness_summary_json_display)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_full_ctest_summary_json_display: ``$($evidence.pdf_full_ctest_readiness_full_ctest_summary_json_display)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_full_ctest_status: ``$($evidence.pdf_full_ctest_readiness_full_ctest_status)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_full_ctest_verdict: ``$($evidence.pdf_full_ctest_readiness_full_ctest_verdict)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_outer_guard_status: ``$($evidence.pdf_full_ctest_readiness_outer_guard_status)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_outer_guard_timed_out: ``$($evidence.pdf_full_ctest_readiness_outer_guard_timed_out)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_selected_test_count: ``$($evidence.pdf_full_ctest_readiness_selected_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_completed_test_count: ``$($evidence.pdf_full_ctest_readiness_completed_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_passed_test_count: ``$($evidence.pdf_full_ctest_readiness_passed_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_failed_test_count: ``$($evidence.pdf_full_ctest_readiness_failed_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_skipped_test_count: ``$($evidence.pdf_full_ctest_readiness_skipped_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_not_run_test_count: ``$($evidence.pdf_full_ctest_readiness_not_run_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_completion_percent: ``$($evidence.pdf_full_ctest_readiness_completion_percent)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_remaining_test_count: ``$($evidence.pdf_full_ctest_readiness_remaining_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_zero_failed_tests_observed: ``$($evidence.pdf_full_ctest_readiness_zero_failed_tests_observed)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_boundary: ``$($evidence.pdf_full_ctest_readiness_boundary)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_marker: ``$($evidence.pdf_full_ctest_readiness_marker)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.pdf_visual_gate_attempt_status)) {
                $lines.Add("    - pdf_visual_gate_attempt_status: ``$($evidence.pdf_visual_gate_attempt_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_verdict: ``$($evidence.pdf_visual_gate_attempt_verdict)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_full_visual_gate_status: ``$($evidence.pdf_visual_gate_attempt_full_visual_gate_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_evidence_scope: ``$($evidence.pdf_visual_gate_attempt_evidence_scope)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_summary_json_display: ``$($evidence.pdf_visual_gate_attempt_summary_json_display)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_stage_count: ``$($evidence.pdf_visual_gate_attempt_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_passed_stage_count: ``$($evidence.pdf_visual_gate_attempt_passed_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_failed_stage_count: ``$($evidence.pdf_visual_gate_attempt_failed_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_incomplete_stage_count: ``$($evidence.pdf_visual_gate_attempt_incomplete_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_outer_guard_status: ``$($evidence.pdf_visual_gate_attempt_outer_guard_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_outer_guard_timed_out: ``$($evidence.pdf_visual_gate_attempt_outer_guard_timed_out)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``$($evidence.pdf_visual_gate_attempt_outer_guard_timeout_seconds)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_pdf_cli_export_status: ``$($evidence.pdf_visual_gate_attempt_pdf_cli_export_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_pdf_regression_status: ``$($evidence.pdf_visual_gate_attempt_pdf_regression_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``$($evidence.pdf_visual_gate_attempt_pdf_regression_selected_test_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``$($evidence.pdf_visual_gate_attempt_pdf_regression_failed_test_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``$($evidence.pdf_visual_gate_attempt_pdf_regression_skipped_test_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_unicode_font_status: ``$($evidence.pdf_visual_gate_attempt_unicode_font_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_cjk_copy_search_status: ``$($evidence.pdf_visual_gate_attempt_cjk_copy_search_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_cjk_copy_search_count: ``$($evidence.pdf_visual_gate_attempt_cjk_copy_search_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_cjk_copy_search_missing_text_count: ``$($evidence.pdf_visual_gate_attempt_cjk_copy_search_missing_text_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_visual_baseline_render_status: ``$($evidence.pdf_visual_gate_attempt_visual_baseline_render_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``$($evidence.pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_expected_visual_render_count: ``$($evidence.pdf_visual_gate_attempt_expected_visual_render_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_aggregate_contact_sheet_status: ``$($evidence.pdf_visual_gate_attempt_aggregate_contact_sheet_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_aggregate_contact_sheet_display: ``$($evidence.pdf_visual_gate_attempt_aggregate_contact_sheet_display)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.pdf_visual_segmented_gate_status)) {
                $lines.Add("    - pdf_visual_segmented_gate_status: ``$($evidence.pdf_visual_segmented_gate_status)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_verdict: ``$($evidence.pdf_visual_segmented_gate_verdict)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_full_visual_gate_status: ``$($evidence.pdf_visual_segmented_gate_full_visual_gate_status)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_evidence_scope: ``$($evidence.pdf_visual_segmented_gate_evidence_scope)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_boundary: ``$($evidence.pdf_visual_segmented_gate_boundary)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_summary_json_display: ``$($evidence.pdf_visual_segmented_gate_summary_json_display)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_slice_summary_count: ``$($evidence.pdf_visual_segmented_gate_slice_summary_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_slice_pass_count: ``$($evidence.pdf_visual_segmented_gate_slice_pass_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_slice_failed_count: ``$($evidence.pdf_visual_segmented_gate_slice_failed_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_covered_baseline_count: ``$($evidence.pdf_visual_segmented_gate_covered_baseline_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_expected_visual_render_count: ``$($evidence.pdf_visual_segmented_gate_expected_visual_render_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_attempt_stage_count: ``$($evidence.pdf_visual_segmented_gate_attempt_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_attempt_passed_stage_count: ``$($evidence.pdf_visual_segmented_gate_attempt_passed_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_visual_baseline_render_status: ``$($evidence.pdf_visual_segmented_gate_visual_baseline_render_status)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``$($evidence.pdf_visual_segmented_gate_aggregate_contact_sheet_status)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_aggregate_contact_sheet_display: ``$($evidence.pdf_visual_segmented_gate_aggregate_contact_sheet_display)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``$($evidence.pdf_visual_segmented_gate_aggregate_contact_sheet_bytes)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_aggregate_rebuild_status: ``$($evidence.pdf_visual_segmented_gate_aggregate_rebuild_status)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count: ``$($evidence.pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.full_visual_gate_status)) {
                $lines.Add("    - full_visual_gate_status: ``$($evidence.full_visual_gate_status)``") | Out-Null
            }
        }
        $lines.Add("- Manifest signoff entrypoints evidence source reports: ``$($rollup.manifest_signoff_entrypoints_source_report_count)``") | Out-Null
        foreach ($evidence in @($rollup.manifest_signoff_entrypoints_source_reports)) {
            $lines.Add("  - source_report: ``$($evidence.path_display)`` schema=``$($evidence.schema)``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_status: ``$($evidence.manifest_signoff_entrypoints_status)``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_release_assets_manifest_display: ``$($evidence.manifest_signoff_entrypoints_release_assets_manifest_display)``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_required_entrypoint_count: ``$($evidence.manifest_signoff_entrypoints_required_entrypoint_count)``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_entrypoint_ids: ``$(@($evidence.manifest_signoff_entrypoints_entrypoint_ids) -join ', ')``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_required_contracts: ``$(@($evidence.manifest_signoff_entrypoints_required_contracts) -join ', ')``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_required_fields: ``$(@($evidence.manifest_signoff_entrypoints_required_fields) -join ', ')``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.manifest_signoff_entrypoints_checklist_marker)) {
                $lines.Add("    - manifest_signoff_entrypoints_checklist_marker: ``$($evidence.manifest_signoff_entrypoints_checklist_marker)``") | Out-Null
            }
        }
        $lines.Add("- Project-template readiness checklist entrypoints evidence source reports: ``$($rollup.project_template_readiness_checklist_entrypoints_source_report_count)``") | Out-Null
        foreach ($evidence in @($rollup.project_template_readiness_checklist_entrypoints_source_reports)) {
            $lines.Add("  - source_report: ``$($evidence.path_display)`` schema=``$($evidence.schema)``") | Out-Null
            $lines.Add("    - project_template_readiness_checklist_entrypoints_status: ``$($evidence.project_template_readiness_checklist_entrypoints_status)``") | Out-Null
            $lines.Add("    - project_template_readiness_checklist_entrypoints_checklist_label: ``$($evidence.project_template_readiness_checklist_entrypoints_checklist_label)``") | Out-Null
            $lines.Add("    - project_template_readiness_checklist_entrypoints_checklist_path: ``$($evidence.project_template_readiness_checklist_entrypoints_checklist_path)``") | Out-Null
            $lines.Add("    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``$($evidence.project_template_readiness_checklist_entrypoints_required_entrypoint_count)``") | Out-Null
            $lines.Add("    - project_template_readiness_checklist_entrypoints_entrypoint_ids: ``$(@($evidence.project_template_readiness_checklist_entrypoints_entrypoint_ids) -join ', ')``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.project_template_readiness_checklist_entrypoints_checklist_marker)) {
                $lines.Add("    - project_template_readiness_checklist_entrypoints_checklist_marker: ``$($evidence.project_template_readiness_checklist_entrypoints_checklist_marker)``") | Out-Null
            }
        }
        $lines.Add("- Release-entry project-template readiness checklist material-safety audit source reports: ``$($rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count)``") | Out-Null
        foreach ($evidence in @($rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports)) {
            $lines.Add("  - source_report: ``$($evidence.path_display)`` schema=``$($evidence.schema)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_status)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_script)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``$(@($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints) -join ', ')``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker)``") | Out-Null
        }
        $lines.Add("- Word visual standard review metadata source reports: ``$($rollup.word_visual_standard_review_metadata_source_report_count)``") | Out-Null
        foreach ($evidence in @($rollup.word_visual_standard_review_metadata_source_reports)) {
            $lines.Add("  - source_report: ``$($evidence.path_display)`` schema=``$($evidence.schema)``") | Out-Null
            $lines.Add("    - word_visual_standard_review_metadata_count: ``$($evidence.word_visual_standard_review_metadata_count)``") | Out-Null
            $lines.Add("    - word_visual_standard_review_task_keys: ``$(@($evidence.word_visual_standard_review_task_keys) -join ', ')``") | Out-Null
            $lines.Add("    - word_visual_standard_review_status_summary: ``$($evidence.word_visual_standard_review_status_summary)``") | Out-Null
            $lines.Add("    - word_visual_standard_review_verdict_summary: ``$($evidence.word_visual_standard_review_verdict_summary)``") | Out-Null
            foreach ($entry in @($evidence.word_visual_standard_review_metadata)) {
                $lines.Add("    - ``$($entry.task_key)``: review_task_key=``$($entry.review_task_key)`` verdict=``$($entry.verdict)`` review_status=``$($entry.review_status)`` review_method=``$($entry.review_method)``") | Out-Null
                foreach ($fieldName in @("label", "reviewed_at", "review_result_path", "final_review_path")) {
                    $fieldValue = Get-JsonString -Object $entry -Name $fieldName
                    if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                        $lines.Add("      - ${fieldName}: ``$fieldValue``") | Out-Null
                    }
                }
            }
        }
        $lines.Add("") | Out-Null
    }

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

        $lines.Add("- **$($focusMetric.label)** ``$($metric.id)``: metric=``$($metric.metric)`` level=``$($metric.level)`` score=``$($metric.score)`` report=``$($metric.report_id)`` schema=``$($metric.source_schema)``") | Out-Null
        Add-TraceabilityMarkdownLines -Lines $lines -Item $metric
        $lines.Add("  - source_report_display: ``$($metric.source_report_display)``") | Out-Null
        $lines.Add("  - source_json_display: ``$($metric.source_json_display)``") | Out-Null
        Add-GovernanceMetricDetailLines -Lines $lines -Metric $metric
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Report Status") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($report in @($Summary.reports)) {
        $lines.Add("- ``$($report.id)``: status=``$($report.status)`` ready=``$($report.release_ready)`` blockers=``$($report.release_blocker_count)`` actions=``$($report.action_item_count)`` informational_actions=``$($report.informational_action_item_count)`` source_failures=``$($report.source_failure_count)`` source_failure_count=``$($report.source_failure_count)`` schema=``$($report.schema)``") | Out-Null
        $lines.Add("  - summary: ``$($report.expected_summary_display)``") | Out-Null
        $lines.Add("  - source_report_display: ``$($report.source_report_display)``") | Out-Null
        $lines.Add("  - source_json_display: ``$($report.source_json_display)``") | Out-Null
        if (-not [string]::IsNullOrWhiteSpace([string]$report.latest_schema_approval_gate_status)) {
            $lines.Add("  - latest_schema_approval_gate_status: ``$($report.latest_schema_approval_gate_status)``") | Out-Null
        }
        if (@($report.schema_approval_status_summary).Count -gt 0) {
            $statusParts = @($report.schema_approval_status_summary | ForEach-Object { "$($_.status)=$($_.count)" })
            $lines.Add("  - schema_approval_status_summary: ``$($statusParts -join ', ')``") | Out-Null
        }
        if ([string]::Equals([string]$report.schema, "featherdoc.docx_functional_smoke_readiness.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
            $lines.Add("  - docx_functional_smoke_ready: ``$($report.docx_functional_smoke_ready)``") | Out-Null
            $lines.Add("  - evidence_scope: ``$($report.evidence_scope)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$report.evidence_scope_note)) {
                $lines.Add("  - evidence_scope_note: $($report.evidence_scope_note)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$report.boundary)) {
                $lines.Add("  - boundary: $($report.boundary)") | Out-Null
            }
            $lines.Add("  - marker: ``$($report.marker)``") | Out-Null
            $lines.Add("  - summary_json_display: ``$($report.summary_json_display)``") | Out-Null
            $lines.Add("  - report_markdown_display: ``$($report.report_markdown_display)``") | Out-Null
        }
        foreach ($metric in @($report.governance_metrics)) {
            $lines.Add("  - metric ``$($metric.id)``: name=``$($metric.metric)`` level=``$($metric.level)`` score=``$($metric.score)`` report=``$($metric.report_id)`` schema=``$($metric.source_schema)``") | Out-Null
        }
        if (-not [string]::IsNullOrWhiteSpace([string]$report.error)) {
            $lines.Add("  - error: ``$($report.error)``") | Out-Null
        }
        if ([string]::IsNullOrWhiteSpace([string]$report.schema)) {
            $lines.Add("  - build: ``$($report.build_command)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Governance Metrics") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.governance_metrics).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($metric in @($Summary.governance_metrics)) {
            $lines.Add("- ``$($metric.id)``: report=``$($metric.report_id)`` metric=``$($metric.metric)`` level=``$($metric.level)`` score=``$($metric.score)`` schema=``$($metric.source_schema)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $metric
            $lines.Add("  - source_report_display: ``$($metric.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($metric.source_json_display)``") | Out-Null
            Add-GovernanceMetricDetailLines -Lines $lines -Metric $metric
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.report_id)`` / ``$($blocker.id)``: project=``$($blocker.project_id)`` template=``$($blocker.template_name)`` candidate=``$($blocker.candidate_type)`` action=``$($blocker.action)`` schema=``$($blocker.source_schema)``") | Out-Null
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
            Add-RepairActionClassMarkdownLines -Lines $lines -Item $blocker
            Add-TraceabilityMarkdownLines -Lines $lines -Item $blocker
            $lines.Add("  - source_report_display: ``$($blocker.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($blocker.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.readiness_status)) {
                $lines.Add("  - readiness_status: ``$($blocker.readiness_status)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.readiness_release_ready)) {
                $lines.Add("  - readiness_release_ready: ``$($blocker.readiness_release_ready)``") | Out-Null
            }
            Add-ProjectTemplateOnboardingContractMarkdownLines -Lines $lines -Item $blocker
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.report_id)`` / ``$($item.id)``: project=``$($item.project_id)`` template=``$($item.template_name)`` candidate=``$($item.candidate_type)`` action=``$($item.action)`` category=``$($item.category)`` release_blocking=``$($item.release_blocking)`` optional=``$($item.optional)`` schema=``$($item.source_schema)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($item.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_hint)) {
                $lines.Add("  - repair_hint: $($item.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.command_template)) {
                $lines.Add("  - command_template: ``$($item.command_template)``") | Out-Null
            }
            Add-RepairActionClassMarkdownLines -Lines $lines -Item $item
            Add-TraceabilityMarkdownLines -Lines $lines -Item $item
            $lines.Add("  - source_report_display: ``$($item.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($item.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.readiness_status)) {
                $lines.Add("  - readiness_status: ``$($item.readiness_status)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.readiness_release_ready)) {
                $lines.Add("  - readiness_release_ready: ``$($item.readiness_release_ready)``") | Out-Null
            }
            Add-ProjectTemplateOnboardingContractMarkdownLines -Lines $lines -Item $item
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Informational Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.informational_action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.informational_action_items)) {
            $lines.Add("- ``$($item.report_id)`` / ``$($item.id)``: project=``$($item.project_id)`` template=``$($item.template_name)`` candidate=``$($item.candidate_type)`` action=``$($item.action)`` category=``$($item.category)`` release_blocking=``$($item.release_blocking)`` optional=``$($item.optional)`` schema=``$($item.source_schema)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($item.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_hint)) {
                $lines.Add("  - repair_hint: $($item.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.command_template)) {
                $lines.Add("  - command_template: ``$($item.command_template)``") | Out-Null
            }
            Add-RepairActionClassMarkdownLines -Lines $lines -Item $item
            Add-TraceabilityMarkdownLines -Lines $lines -Item $item
            $lines.Add("  - source_report_display: ``$($item.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($item.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.readiness_status)) {
                $lines.Add("  - readiness_status: ``$($item.readiness_status)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.readiness_release_ready)) {
                $lines.Add("  - readiness_release_ready: ``$($item.readiness_release_ready)``") | Out-Null
            }
            Add-ProjectTemplateOnboardingContractMarkdownLines -Lines $lines -Item $item
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.warnings).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($warning in @($Summary.warnings)) {
            $lines.Add("- ``$($warning.report_id)`` / ``$($warning.id)``: project=``$($warning.project_id)`` template=``$($warning.template_name)`` candidate=``$($warning.candidate_type)`` action=``$($warning.action)`` schema=``$($warning.source_schema)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$warning.message)) {
                $lines.Add("  - $($warning.message)") | Out-Null
            }
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
            Add-RepairActionClassMarkdownLines -Lines $lines -Item $warning
            Add-TraceabilityMarkdownLines -Lines $lines -Item $warning
            $lines.Add("  - source_report_display: ``$($warning.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($warning.source_json_display)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Next Commands") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($command in @($Summary.next_commands)) {
        $lines.Add("- ``$command``") | Out-Null
    }
    if (@($Summary.next_commands).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    }

    return @($lines)
}
