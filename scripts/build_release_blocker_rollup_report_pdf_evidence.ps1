function Get-PdfVisualGateEvidenceObject {
    param($Summary)

    $pdfVisualGate = Get-JsonProperty -Object $Summary -Name "pdf_visual_gate"
    if ($null -ne $pdfVisualGate) {
        return $pdfVisualGate
    }

    $steps = Get-JsonProperty -Object $Summary -Name "steps"
    if ($null -eq $steps) {
        return $null
    }

    return (Get-JsonProperty -Object $steps -Name "pdf_visual_gate")
}

function Add-PdfVisualGateEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary,
        [string]$RepoRoot
    )

    $pdfVisualGate = Get-PdfVisualGateEvidenceObject -Summary $Summary
    if ($null -eq $pdfVisualGate) {
        return
    }

    $summaryJson = Get-FirstJsonString -Object $pdfVisualGate -Names @("summary_json")
    if ([string]::IsNullOrWhiteSpace($summaryJson)) {
        $summaryJson = Get-FirstJsonString -Object $Summary -Names @("pdf_visual_gate_summary_json")
    }
    $aggregateContactSheet = Get-FirstJsonString -Object $pdfVisualGate -Names @("aggregate_contact_sheet")
    $verdict = Get-FirstJsonString -Object $pdfVisualGate -Names @("verdict")
    $fullVisualGateStatus = Get-FirstJsonString -Object $pdfVisualGate -Names @("full_visual_gate_status")
    if ([string]::IsNullOrWhiteSpace($fullVisualGateStatus) -and $verdict -in @("pass", "fail")) {
        $fullVisualGateStatus = $verdict
    }

    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_status" `
        -Value (Get-FirstJsonString -Object $pdfVisualGate -Names @("status"))
    Set-OptionalSourceReportField -Target $Target -Name "full_visual_gate_status" `
        -Value $fullVisualGateStatus
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_verdict" `
        -Value $verdict
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_finalizable" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("finalizable"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_summary_json" `
        -Value $summaryJson
    if (-not [string]::IsNullOrWhiteSpace($summaryJson)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_summary_json_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $summaryJson)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_aggregate_contact_sheet" `
        -Value $aggregateContactSheet
    if (-not [string]::IsNullOrWhiteSpace($aggregateContactSheet)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_aggregate_contact_sheet_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $aggregateContactSheet)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_cjk_manifest_count" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("cjk_manifest_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_cjk_copy_search_count" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("cjk_copy_search_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_cjk_missing_text_count" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("cjk_copy_search_missing_text_count", "cjk_missing_text_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_visual_baseline_manifest_count" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("visual_baseline_manifest_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_visual_baseline_count" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("visual_baseline_count", "baselines_count"))
}

function Add-DocxFunctionalSmokeReadinessEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary
    )

    $schema = Get-JsonString -Object $Summary -Name "schema"
    if (-not [string]::Equals($schema, "featherdoc.docx_functional_smoke_readiness.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
        return
    }

    foreach ($field in @(
            "verdict",
            "docx_functional_smoke_ready",
            "evidence_scope",
            "evidence_scope_note",
            "boundary",
            "marker",
            "summary_json_display",
            "report_markdown_display"
        )) {
        Set-OptionalSourceReportField -Target $Target -Name $field `
            -Value (Get-JsonProperty -Object $Summary -Name $field)
    }
}

function Get-PdfBoundedCtestEvidenceObject {
    param($Summary)

    $pdfBoundedCtest = Get-JsonProperty -Object $Summary -Name "pdf_bounded_ctest"
    if ($null -ne $pdfBoundedCtest) {
        return $pdfBoundedCtest
    }

    $steps = Get-JsonProperty -Object $Summary -Name "steps"
    if ($null -eq $steps) {
        return $null
    }

    return (Get-JsonProperty -Object $steps -Name "pdf_bounded_ctest")
}

function Add-PdfBoundedCtestEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary
    )

    $pdfBoundedCtest = Get-PdfBoundedCtestEvidenceObject -Summary $Summary
    if ($null -eq $pdfBoundedCtest) {
        return
    }

    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_summary_count" `
        -Value (Get-FirstJsonProperty -Object $pdfBoundedCtest -Names @("summary_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_pass_count" `
        -Value (Get-FirstJsonProperty -Object $pdfBoundedCtest -Names @("pass_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_skipped_test_count" `
        -Value (Get-FirstJsonProperty -Object $pdfBoundedCtest -Names @("skipped_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_selected_test_count" `
        -Value (Get-FirstJsonProperty -Object $pdfBoundedCtest -Names @("selected_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_subsets" `
        -Value @(Get-JsonArray -Object $pdfBoundedCtest -Name "subsets")
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_summary_json_display" `
        -Value @(Get-JsonArray -Object $pdfBoundedCtest -Name "summary_json_display")
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_import_diagnostics_contract_tests" `
        -Value @(Get-JsonArray -Object $pdfBoundedCtest -Name "import_diagnostics_contract_tests")
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_import_diagnostics_contract_fields" `
        -Value @(Get-JsonArray -Object $pdfBoundedCtest -Name "import_diagnostics_contract_fields")
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_import_negative_boundary_contract_cases" `
        -Value @(Get-JsonArray -Object $pdfBoundedCtest -Name "import_negative_boundary_contract_cases")
}

function Get-PdfFullCtestReadinessEvidenceObject {
    param($Summary)

    $readiness = Get-JsonProperty -Object $Summary -Name "pdf_full_ctest_readiness"
    if ($null -ne $readiness) {
        return $readiness
    }

    $steps = Get-JsonProperty -Object $Summary -Name "steps"
    if ($null -eq $steps) {
        return $null
    }

    return (Get-JsonProperty -Object $steps -Name "pdf_full_ctest_readiness")
}

function Add-PdfFullCtestReadinessEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary,
        [string]$RepoRoot
    )

    $readiness = Get-PdfFullCtestReadinessEvidenceObject -Summary $Summary
    if ($null -eq $readiness) {
        return
    }

    $summaryJson = Get-FirstJsonString -Object $readiness -Names @("summary_json")
    if ([string]::IsNullOrWhiteSpace($summaryJson)) {
        $summaryJson = Get-FirstJsonString -Object $Summary -Names @("pdf_release_readiness_summary_json")
    }
    $fullCtestSummaryJson = Get-FirstJsonString -Object $readiness -Names @("full_ctest_summary_json")

    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_requested" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("requested"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_status" `
        -Value (Get-FirstJsonString -Object $readiness -Names @("status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_verdict" `
        -Value (Get-FirstJsonString -Object $readiness -Names @("verdict"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_release_ready" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("release_ready"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_visual_gate_release_evidence_accepted" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("visual_gate_release_evidence_accepted"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_visual_gate_fresh_full_guarded_evidence" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("visual_gate_fresh_full_guarded_evidence"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_visual_gate_segmented_full_coverage_evidence" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("visual_gate_segmented_full_coverage_evidence"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_summary_json" `
        -Value $summaryJson
    if (-not [string]::IsNullOrWhiteSpace($summaryJson)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_summary_json_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $summaryJson)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_full_ctest_summary_json" `
        -Value $fullCtestSummaryJson
    if (-not [string]::IsNullOrWhiteSpace($fullCtestSummaryJson)) {
        $fullCtestSummaryJsonDisplay = Get-FirstJsonString -Object $readiness -Names @("full_ctest_summary_json_display")
        if ([string]::IsNullOrWhiteSpace($fullCtestSummaryJsonDisplay)) {
            $fullCtestSummaryJsonDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $fullCtestSummaryJson
        }
        Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_full_ctest_summary_json_display" `
            -Value $fullCtestSummaryJsonDisplay
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_full_ctest_status" `
        -Value (Get-FirstJsonString -Object $readiness -Names @("full_ctest_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_full_ctest_verdict" `
        -Value (Get-FirstJsonString -Object $readiness -Names @("full_ctest_verdict"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_outer_guard_status" `
        -Value (Get-FirstJsonString -Object $readiness -Names @("outer_guard_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_outer_guard_timed_out" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("outer_guard_timed_out"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_selected_test_count" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("selected_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_completed_test_count" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("completed_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_passed_test_count" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("passed_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_failed_test_count" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("failed_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_skipped_test_count" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("skipped_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_not_run_test_count" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("not_run_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_completion_percent" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("completion_percent"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_remaining_test_count" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("remaining_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_zero_failed_tests_observed" `
        -Value (Get-FirstJsonProperty -Object $readiness -Names @("zero_failed_tests_observed"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_boundary" `
        -Value (Get-FirstJsonString -Object $readiness -Names @("boundary"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_full_ctest_readiness_marker" `
        -Value (Get-FirstJsonString -Object $readiness -Names @("marker"))
}

function Get-PdfVisualGateAttemptEvidenceObject {
    param($Summary)

    $attempt = Get-JsonProperty -Object $Summary -Name "pdf_visual_gate_attempt"
    if ($null -ne $attempt) {
        return $attempt
    }

    $steps = Get-JsonProperty -Object $Summary -Name "steps"
    if ($null -eq $steps) {
        return $null
    }

    return (Get-JsonProperty -Object $steps -Name "pdf_visual_gate_attempt")
}

function Add-PdfVisualGateAttemptEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary,
        [string]$RepoRoot
    )

    $attempt = Get-PdfVisualGateAttemptEvidenceObject -Summary $Summary
    if ($null -eq $attempt) {
        return
    }

    $summaryJson = Get-FirstJsonString -Object $attempt -Names @("summary_json")
    if ([string]::IsNullOrWhiteSpace($summaryJson)) {
        $summaryJson = Get-FirstJsonString -Object $Summary -Names @("pdf_visual_gate_attempt_summary_json")
    }
    $status = Get-FirstJsonString -Object $attempt -Names @("status")
    if ([string]::IsNullOrWhiteSpace($summaryJson) -and $status -eq "not_requested") {
        return
    }
    $aggregateContactSheet = Get-FirstJsonString -Object $attempt -Names @("aggregate_contact_sheet")

    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_status" `
        -Value $status
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_verdict" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("verdict"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_full_visual_gate_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("full_visual_gate_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_evidence_scope" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("evidence_scope"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_summary_json" `
        -Value $summaryJson
    if (-not [string]::IsNullOrWhiteSpace($summaryJson)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_summary_json_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $summaryJson)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_stage_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_passed_stage_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("passed_stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_failed_stage_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("failed_stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_incomplete_stage_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("incomplete_stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_outer_guard_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("outer_guard_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_outer_guard_timed_out" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("outer_guard_timed_out"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_outer_guard_timeout_seconds" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("outer_guard_timeout_seconds"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_pdf_cli_export_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("pdf_cli_export_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_pdf_regression_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("pdf_regression_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_pdf_regression_selected_test_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("pdf_regression_selected_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_pdf_regression_failed_test_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("pdf_regression_failed_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_pdf_regression_skipped_test_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("pdf_regression_skipped_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_unicode_font_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("unicode_font_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_cjk_copy_search_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("cjk_copy_search_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_cjk_copy_search_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("cjk_copy_search_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_cjk_copy_search_missing_text_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("cjk_copy_search_missing_text_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_visual_baseline_render_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("visual_baseline_render_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("visual_baseline_fresh_rendered_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_expected_visual_render_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("expected_visual_render_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_visual_baseline_missing_pdf_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("visual_baseline_missing_pdf_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_visual_baseline_pdf_total_bytes" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("visual_baseline_pdf_total_bytes"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_visual_baseline_png_page_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("visual_baseline_png_page_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_visual_baseline_missing_png_page_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("visual_baseline_missing_png_page_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_visual_baseline_png_total_bytes" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("visual_baseline_png_total_bytes"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_visual_baseline_unreadable_png_dimension_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("visual_baseline_unreadable_png_dimension_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_aggregate_contact_sheet_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("aggregate_contact_sheet_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_aggregate_contact_sheet" `
        -Value $aggregateContactSheet
    if (-not [string]::IsNullOrWhiteSpace($aggregateContactSheet)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_aggregate_contact_sheet_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $aggregateContactSheet)
    }
}

function Get-PdfVisualSegmentedGateEvidenceObject {
    param($Summary)

    $segmented = Get-JsonProperty -Object $Summary -Name "pdf_visual_segmented_gate"
    if ($null -ne $segmented) {
        return $segmented
    }

    $steps = Get-JsonProperty -Object $Summary -Name "steps"
    if ($null -eq $steps) {
        return $null
    }

    return (Get-JsonProperty -Object $steps -Name "pdf_visual_segmented_gate")
}

function Add-PdfVisualSegmentedGateEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary,
        [string]$RepoRoot
    )

    $segmented = Get-PdfVisualSegmentedGateEvidenceObject -Summary $Summary
    if ($null -eq $segmented) {
        return
    }

    $summaryJson = Get-FirstJsonString -Object $segmented -Names @("summary_json")
    if ([string]::IsNullOrWhiteSpace($summaryJson)) {
        $summaryJson = Get-FirstJsonString -Object $Summary -Names @("pdf_visual_segmented_gate_summary_json")
    }
    $status = Get-FirstJsonString -Object $segmented -Names @("status")
    if ([string]::IsNullOrWhiteSpace($summaryJson) -and $status -eq "not_requested") {
        return
    }
    $aggregateContactSheet = Get-FirstJsonString -Object $segmented -Names @("aggregate_contact_sheet")

    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_status" `
        -Value $status
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_verdict" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("verdict"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_full_visual_gate_status" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("full_visual_gate_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_evidence_scope" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("evidence_scope"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_boundary" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("boundary"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_summary_json" `
        -Value $summaryJson
    if (-not [string]::IsNullOrWhiteSpace($summaryJson)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_summary_json_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $summaryJson)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_slice_summary_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("slice_summary_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_slice_pass_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("slice_pass_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_slice_failed_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("slice_failed_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_covered_baseline_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("covered_baseline_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_expected_visual_render_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("expected_visual_render_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_attempt_stage_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("attempt_stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_attempt_passed_stage_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("attempt_passed_stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_visual_baseline_render_status" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("visual_baseline_render_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_contact_sheet_status" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("aggregate_contact_sheet_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_contact_sheet" `
        -Value $aggregateContactSheet
    if (-not [string]::IsNullOrWhiteSpace($aggregateContactSheet)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_contact_sheet_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $aggregateContactSheet)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_contact_sheet_bytes" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("aggregate_contact_sheet_bytes"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_rebuild_status" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("aggregate_rebuild_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("aggregate_rebuild_selected_baseline_count"))
}
