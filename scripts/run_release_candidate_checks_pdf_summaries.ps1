function Get-PdfVisualGateSummaryInfo {
    param(
        [string]$SummaryJson
    )

    $info = [ordered]@{
        requested = -not [string]::IsNullOrWhiteSpace($SummaryJson)
        status = if ([string]::IsNullOrWhiteSpace($SummaryJson)) { "not_requested" } elseif (Test-Path -LiteralPath $SummaryJson) { "available" } else { "missing" }
        summary_json = $SummaryJson
        full_visual_gate_status = ""
        verdict = ""
        aggregate_contact_sheet = ""
        cjk_manifest_count = 0
        cjk_copy_search_count = 0
        cjk_copy_search_missing_text_count = 0
        visual_baseline_manifest_count = 0
        visual_baseline_count = 0
        finalizable = $false
        error = ""
    }

    if (-not [bool]$info.requested -or -not (Test-Path -LiteralPath $SummaryJson)) {
        return $info
    }

    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $SummaryJson | ConvertFrom-Json
        $info.status = "loaded"
        $info.verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "verdict")
        if ($info.verdict -in @("pass", "fail")) {
            $info.full_visual_gate_status = $info.verdict
        }
        $info.aggregate_contact_sheet = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet")
        $info.cjk_manifest_count = [int](Get-OptionalPropertyValue -Object $summary -Name "cjk_manifest_count")
        $info.cjk_copy_search_count = [int](Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_count")
        $cjkMissingTextCount = Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_missing_text_count"
        if ($null -eq $cjkMissingTextCount) {
            $cjkMissingTextCount = Get-OptionalPropertyValue -Object $summary -Name "cjk_missing_text_count"
        }
        if ($null -ne $cjkMissingTextCount -and -not [string]::IsNullOrWhiteSpace([string]$cjkMissingTextCount)) {
            $info.cjk_copy_search_missing_text_count = [int]$cjkMissingTextCount
        } else {
            $computedMissingTextCount = 0
            foreach ($entry in @(Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search")) {
                if ($null -eq $entry) {
                    continue
                }

                $missingText = Get-OptionalPropertyValue -Object $entry -Name "missing_text"
                if ($null -eq $missingText) {
                    continue
                }
                if ($missingText -is [string]) {
                    if (-not [string]::IsNullOrWhiteSpace($missingText)) {
                        $computedMissingTextCount += 1
                    }
                    continue
                }
                if ($missingText -is [System.Collections.IEnumerable]) {
                    $computedMissingTextCount += @($missingText | Where-Object {
                            $null -ne $_ -and -not [string]::IsNullOrWhiteSpace([string]$_)
                        }).Count
                    continue
                }

                $computedMissingTextCount += 1
            }

            $info.cjk_copy_search_missing_text_count = $computedMissingTextCount
        }
        $info.visual_baseline_manifest_count = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_manifest_count")
        $info.visual_baseline_count = [int](Get-OptionalPropertyValue -Object $summary -Name "baselines_count")
        $info.finalizable = -not [string]::IsNullOrWhiteSpace([string]$info.verdict) -and
            [int]$info.cjk_copy_search_count -gt 0 -and
            [int]$info.visual_baseline_count -gt 0 -and
            -not [string]::IsNullOrWhiteSpace([string]$info.aggregate_contact_sheet)
    } catch {
        $info.status = "unreadable"
        $info.error = $_.Exception.Message
    }

    return $info
}

function Get-PdfVisualGateAttemptSummaryInfo {
    param(
        [string]$SummaryJson
    )

    $info = [ordered]@{
        requested = -not [string]::IsNullOrWhiteSpace($SummaryJson)
        status = if ([string]::IsNullOrWhiteSpace($SummaryJson)) { "not_requested" } elseif (Test-Path -LiteralPath $SummaryJson) { "available" } else { "missing" }
        summary_json = $SummaryJson
        verdict = ""
        full_visual_gate_status = ""
        evidence_scope = ""
        stage_count = 0
        passed_stage_count = 0
        failed_stage_count = 0
        incomplete_stage_count = 0
        pdf_cli_export_status = ""
        pdf_regression_status = ""
        pdf_regression_selected_test_count = 0
        pdf_regression_failed_test_count = 0
        pdf_regression_skipped_test_count = 0
        unicode_font_status = ""
        cjk_copy_search_status = ""
        cjk_copy_search_count = 0
        cjk_copy_search_missing_text_count = 0
        visual_baseline_render_status = ""
        visual_baseline_fresh_rendered_count = 0
        expected_visual_render_count = 0
        visual_baseline_fresh_missing_sample_count = 0
        visual_baseline_resume_needed = $false
        visual_baseline_resume_slice_offset = 0
        visual_baseline_resume_slice_limit = 0
        visual_baseline_resume_slice_command_template = ""
        outer_guard_status = ""
        outer_guard_timed_out = $false
        outer_guard_timeout_seconds = 0
        aggregate_contact_sheet_status = ""
        aggregate_contact_sheet = ""
        aggregate_contact_sheet_bytes = 0
        error = ""
    }

    if (-not [bool]$info.requested -or -not (Test-Path -LiteralPath $SummaryJson)) {
        return $info
    }

    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $SummaryJson | ConvertFrom-Json
        $info.status = [string](Get-OptionalPropertyValue -Object $summary -Name "status")
        $info.verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "verdict")
        $info.full_visual_gate_status = [string](Get-OptionalPropertyValue -Object $summary -Name "full_visual_gate_status")
        $info.evidence_scope = [string](Get-OptionalPropertyValue -Object $summary -Name "evidence_scope")
        $info.stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "stage_count")
        $info.passed_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "passed_stage_count")
        $info.failed_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "failed_stage_count")
        $info.incomplete_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "incomplete_stage_count")
        $info.pdf_cli_export_status = [string](Get-OptionalPropertyValue -Object $summary -Name "pdf_cli_export_status")
        $info.pdf_regression_status = [string](Get-OptionalPropertyValue -Object $summary -Name "pdf_regression_status")
        $info.pdf_regression_selected_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "pdf_regression_selected_test_count")
        $info.pdf_regression_failed_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "pdf_regression_failed_test_count")
        $info.pdf_regression_skipped_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "pdf_regression_skipped_test_count")
        $info.unicode_font_status = [string](Get-OptionalPropertyValue -Object $summary -Name "unicode_font_status")
        $info.cjk_copy_search_status = [string](Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_status")
        $info.cjk_copy_search_count = [int](Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_count")
        $info.cjk_copy_search_missing_text_count = [int](Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_missing_text_count")
        $info.visual_baseline_render_status = [string](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_render_status")
        $info.visual_baseline_fresh_rendered_count = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_fresh_rendered_count")
        $info.expected_visual_render_count = [int](Get-OptionalPropertyValue -Object $summary -Name "expected_visual_render_count")
        $info.visual_baseline_fresh_missing_sample_count = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_fresh_missing_sample_count")
        $info.visual_baseline_resume_needed = [bool](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_resume_needed")
        $info.visual_baseline_resume_slice_offset = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_resume_slice_offset")
        $info.visual_baseline_resume_slice_limit = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_resume_slice_limit")
        $info.visual_baseline_resume_slice_command_template = [string](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_resume_slice_command_template")
        $info.outer_guard_status = [string](Get-OptionalPropertyValue -Object $summary -Name "outer_guard_status")
        $info.outer_guard_timed_out = [bool](Get-OptionalPropertyValue -Object $summary -Name "outer_guard_timed_out")
        $info.outer_guard_timeout_seconds = [int](Get-OptionalPropertyValue -Object $summary -Name "outer_guard_timeout_seconds")
        $info.aggregate_contact_sheet_status = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet_status")
        $info.aggregate_contact_sheet = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet")
        $info.aggregate_contact_sheet_bytes = [int](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet_bytes")
    } catch {
        $info.status = "unreadable"
        $info.error = $_.Exception.Message
    }

    return $info
}

function Get-PdfVisualGateAttemptReleaseWarnings {
    param(
        [System.Collections.IDictionary]$ReleaseSummary,
        [string]$RepoRoot
    )

    $warnings = New-Object 'System.Collections.Generic.List[object]'
    $attempt = Get-OptionalPropertyValue -Object $ReleaseSummary -Name "pdf_visual_gate_attempt"
    if ($null -eq $attempt) {
        return @()
    }

    $attemptStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "status")
    $attemptVerdict = [string](Get-OptionalPropertyValue -Object $attempt -Name "verdict")
    $attemptFullStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "full_visual_gate_status")
    $outerGuardTimedOut = [bool](Get-OptionalPropertyValue -Object $attempt -Name "outer_guard_timed_out")
    $outerGuardStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "outer_guard_status")
    $renderStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_render_status")
    $contactSheetStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "aggregate_contact_sheet_status")
    $attemptPassed = ($attemptStatus -eq "pass" -and $attemptVerdict -eq "pass" -and $attemptFullStatus -eq "pass")
    $attemptStageCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "stage_count")
    $attemptPassedStageCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "passed_stage_count")
    $attemptFailedStageCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "failed_stage_count")
    $attemptIncompleteStageCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "incomplete_stage_count")
    $renderedCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_fresh_rendered_count")
    $expectedRenderCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "expected_visual_render_count")
    $freshMissingSampleCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_fresh_missing_sample_count")
    $resumeNeeded = [bool](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_resume_needed")
    $resumeSliceOffset = [int](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_resume_slice_offset")
    $resumeSliceLimit = [int](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_resume_slice_limit")
    $resumeSliceCommandTemplate = [string](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_resume_slice_command_template")
    $pdfRegressionStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "pdf_regression_status")
    $pdfRegressionSelected = [int](Get-OptionalPropertyValue -Object $attempt -Name "pdf_regression_selected_test_count")
    $pdfRegressionFailed = [int](Get-OptionalPropertyValue -Object $attempt -Name "pdf_regression_failed_test_count")
    $pdfRegressionSkipped = [int](Get-OptionalPropertyValue -Object $attempt -Name "pdf_regression_skipped_test_count")
    $cjkCopySearchStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "cjk_copy_search_status")
    $cjkCopySearchCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "cjk_copy_search_count")
    $cjkMissingTextCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "cjk_copy_search_missing_text_count")
    $readiness = Get-OptionalPropertyValue -Object $ReleaseSummary -Name "pdf_full_ctest_readiness"
    $visualGate = Get-OptionalPropertyValue -Object $ReleaseSummary -Name "pdf_visual_gate"
    $readinessReleaseReady = [bool](Get-OptionalPropertyValue -Object $readiness -Name "release_ready")
    $visualReleaseEvidenceAccepted = [bool](Get-OptionalPropertyValue -Object $readiness -Name "visual_gate_release_evidence_accepted")
    $freshFullGuardedEvidence = [bool](Get-OptionalPropertyValue -Object $readiness -Name "visual_gate_fresh_full_guarded_evidence")
    $passSummaryBeforeOuterTimeout = [bool](Get-OptionalPropertyValue -Object $readiness -Name "visual_gate_pass_summary_before_outer_timeout")
    $segmentedFullCoverageEvidence = [bool](Get-OptionalPropertyValue -Object $readiness -Name "visual_gate_segmented_full_coverage_evidence")
    $finalizeOnlyEvidence = [bool](Get-OptionalPropertyValue -Object $visualGate -Name "finalize_only")
    $skipPreflightEvidence = [bool](Get-OptionalPropertyValue -Object $visualGate -Name "skip_preflight")
    $readinessVerdict = [string](Get-OptionalPropertyValue -Object $readiness -Name "verdict")
    $needsWarning = (-not $attemptPassed) -and (
        $attemptStatus -eq "partial" -or
        $attemptVerdict -eq "not_complete" -or
        $attemptFullStatus -eq "not_complete" -or
        $outerGuardTimedOut -or
        $outerGuardStatus -eq "timed_out"
    )

    if (-not $needsWarning) {
        return @()
    }

    $attemptAuxiliaryEvidenceComplete = (
        $attemptStageCount -gt 0 -and
        $attemptPassedStageCount -eq $attemptStageCount -and
        $attemptFailedStageCount -eq 0 -and
        $attemptIncompleteStageCount -eq 0 -and
        $pdfRegressionStatus -eq "pass" -and
        $pdfRegressionFailed -eq 0 -and
        $cjkCopySearchStatus -eq "pass" -and
        $cjkMissingTextCount -eq 0 -and
        $renderStatus -eq "pass" -and
        $expectedRenderCount -gt 0 -and
        $renderedCount -ge $expectedRenderCount -and
        $contactSheetStatus -eq "pass"
    )
    $segmentedReleaseEvidenceAccepted = (
        $readinessReleaseReady -and
        $visualReleaseEvidenceAccepted -and
        $segmentedFullCoverageEvidence
    )
    if ($segmentedReleaseEvidenceAccepted -and $attemptAuxiliaryEvidenceComplete) {
        return @()
    }

    $summaryJsonPath = [string](Get-OptionalPropertyValue -Object $attempt -Name "summary_json")
    $summaryJsonDisplay = Get-RepoRelativePath -RepoRoot $RepoRoot -Path $summaryJsonPath
    $outerGuardTimeoutSeconds = [int](Get-OptionalPropertyValue -Object $attempt -Name "outer_guard_timeout_seconds")
    $message = if ($segmentedFullCoverageEvidence) {
        "Fresh non-FinalizeOnly PDF visual gate attempt did not complete within the 60-second outer guard; release evidence is accepted through strict segmented full-coverage visual evidence, but this does not replace a completed single-run full visual gate."
    } elseif ($freshFullGuardedEvidence) {
        "Fresh non-FinalizeOnly PDF visual gate attempt did not complete within the 60-second outer guard; release evidence is accepted through an earlier completed guarded full visual gate, while this attempt remains traceability debt."
    } elseif ($finalizeOnlyEvidence) {
        "Fresh non-FinalizeOnly PDF visual gate attempt did not complete within the 60-second outer guard; release still relies on explicit FinalizeOnly summary/contact-sheet evidence while baseline render/contact-sheet refresh remains incomplete."
    } elseif ($visualReleaseEvidenceAccepted) {
        "Fresh non-FinalizeOnly PDF visual gate attempt did not complete within the 60-second outer guard; release evidence is accepted by the PDF readiness gate, but the current attempt remains traceability debt."
    } else {
        "Fresh non-FinalizeOnly PDF visual gate attempt did not complete within the 60-second outer guard, and PDF visual release evidence is not accepted by the current readiness gate."
    }

    [void]$warnings.Add([ordered]@{
            id = "pdf_visual_gate_attempt.incomplete_fresh_render"
            action = "review_pdf_visual_gate_attempt_and_finalize_evidence"
            message = $message
            source_schema = "featherdoc.release_candidate_summary"
            source_report = $summaryJsonPath
            source_report_display = $summaryJsonDisplay
            source_json = $summaryJsonPath
            source_json_display = $summaryJsonDisplay
            attempt_status = $attemptStatus
            attempt_verdict = $attemptVerdict
            full_visual_gate_status = $attemptFullStatus
            evidence_scope = [string](Get-OptionalPropertyValue -Object $attempt -Name "evidence_scope")
            outer_guard_status = $outerGuardStatus
            outer_guard_timed_out = $outerGuardTimedOut
            outer_guard_timeout_seconds = $outerGuardTimeoutSeconds
            pdf_release_readiness_verdict = $readinessVerdict
            visual_gate_release_evidence_accepted = $visualReleaseEvidenceAccepted
            visual_gate_fresh_full_guarded_evidence = $freshFullGuardedEvidence
            visual_gate_pass_summary_before_outer_timeout = $passSummaryBeforeOuterTimeout
            visual_gate_segmented_full_coverage_evidence = $segmentedFullCoverageEvidence
            visual_gate_finalize_only = $finalizeOnlyEvidence
            visual_gate_skip_preflight = $skipPreflightEvidence
            pdf_cli_export_status = [string](Get-OptionalPropertyValue -Object $attempt -Name "pdf_cli_export_status")
            pdf_regression_status = [string](Get-OptionalPropertyValue -Object $attempt -Name "pdf_regression_status")
            pdf_regression_selected_test_count = $pdfRegressionSelected
            pdf_regression_failed_test_count = $pdfRegressionFailed
            pdf_regression_skipped_test_count = $pdfRegressionSkipped
            unicode_font_status = [string](Get-OptionalPropertyValue -Object $attempt -Name "unicode_font_status")
            cjk_copy_search_status = [string](Get-OptionalPropertyValue -Object $attempt -Name "cjk_copy_search_status")
            cjk_copy_search_count = $cjkCopySearchCount
            cjk_copy_search_missing_text_count = $cjkMissingTextCount
            visual_baseline_render_status = $renderStatus
            visual_baseline_fresh_rendered_count = $renderedCount
            expected_visual_render_count = $expectedRenderCount
            visual_baseline_fresh_missing_sample_count = $freshMissingSampleCount
            visual_baseline_resume_needed = $resumeNeeded
            visual_baseline_resume_slice_offset = $resumeSliceOffset
            visual_baseline_resume_slice_limit = $resumeSliceLimit
            visual_baseline_resume_slice_command_template = $resumeSliceCommandTemplate
            aggregate_contact_sheet_status = $contactSheetStatus
            aggregate_contact_sheet = [string](Get-OptionalPropertyValue -Object $attempt -Name "aggregate_contact_sheet")
            release_owner_acceptance_required = $true
            release_owner_acceptance_policy = "release_owner_may_accept_segmented_full_coverage_with_explicit_single_run_debt"
            release_owner_acceptance_boundary = "acceptance_does_not_replace_fresh_single_run_full_visual_gate"
            release_owner_acceptance_command_template = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1 -PdfReleaseReadinessSummaryJson <summary.json>; document release-owner acceptance in final_review.md without changing full_visual_gate_status."
            release_owner_acceptance_trace_marker = "pdf_visual_gate_release_owner_acceptance_trace"
        })

    return @($warnings.ToArray())
}

function Set-ReleaseSummaryWarnings {
    param(
        [System.Collections.IDictionary]$ReleaseSummary,
        [object[]]$Warnings
    )

    $normalizedWarnings = New-Object 'System.Collections.Generic.List[object]'
    foreach ($warning in @($Warnings)) {
        if ($null -ne $warning) {
            [void]$normalizedWarnings.Add($warning)
        }
    }

    $ReleaseSummary["warnings"] = @($normalizedWarnings.ToArray())
    $ReleaseSummary["warning_count"] = $normalizedWarnings.Count
}

function Get-PdfVisualSegmentedGateSummaryInfo {
    param(
        [string]$SummaryJson
    )

    $info = [ordered]@{
        requested = -not [string]::IsNullOrWhiteSpace($SummaryJson)
        status = if ([string]::IsNullOrWhiteSpace($SummaryJson)) { "not_requested" } elseif (Test-Path -LiteralPath $SummaryJson) { "available" } else { "missing" }
        summary_json = $SummaryJson
        verdict = ""
        full_visual_gate_status = ""
        evidence_scope = ""
        boundary = ""
        slice_summary_count = 0
        slice_pass_count = 0
        slice_failed_count = 0
        covered_baseline_count = 0
        expected_visual_render_count = 0
        visual_baseline_manifest_count = 0
        attempt_status = ""
        attempt_verdict = ""
        attempt_full_visual_gate_status = ""
        attempt_stage_count = 0
        attempt_passed_stage_count = 0
        attempt_incomplete_stage_count = 0
        visual_baseline_render_status = ""
        visual_baseline_fresh_rendered_count = 0
        aggregate_contact_sheet_status = ""
        aggregate_contact_sheet = ""
        aggregate_contact_sheet_bytes = 0
        aggregate_rebuild_status = ""
        aggregate_rebuild_verdict = ""
        aggregate_rebuild_selected_baseline_count = 0
        aggregate_rebuild_expected_visual_render_count = 0
        error = ""
    }

    if (-not [bool]$info.requested -or -not (Test-Path -LiteralPath $SummaryJson)) {
        return $info
    }

    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $SummaryJson | ConvertFrom-Json
        $info.status = [string](Get-OptionalPropertyValue -Object $summary -Name "status")
        $info.verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "verdict")
        $info.full_visual_gate_status = [string](Get-OptionalPropertyValue -Object $summary -Name "full_visual_gate_status")
        $info.evidence_scope = [string](Get-OptionalPropertyValue -Object $summary -Name "evidence_scope")
        $info.boundary = [string](Get-OptionalPropertyValue -Object $summary -Name "boundary")
        $info.slice_summary_count = [int](Get-OptionalPropertyValue -Object $summary -Name "slice_summary_count")
        $info.slice_pass_count = [int](Get-OptionalPropertyValue -Object $summary -Name "slice_pass_count")
        $info.slice_failed_count = [int](Get-OptionalPropertyValue -Object $summary -Name "slice_failed_count")
        $info.covered_baseline_count = [int](Get-OptionalPropertyValue -Object $summary -Name "covered_baseline_count")
        $info.expected_visual_render_count = [int](Get-OptionalPropertyValue -Object $summary -Name "expected_visual_render_count")
        $info.visual_baseline_manifest_count = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_manifest_count")
        $info.attempt_status = [string](Get-OptionalPropertyValue -Object $summary -Name "attempt_status")
        $info.attempt_verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "attempt_verdict")
        $info.attempt_full_visual_gate_status = [string](Get-OptionalPropertyValue -Object $summary -Name "attempt_full_visual_gate_status")
        $info.attempt_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "attempt_stage_count")
        $info.attempt_passed_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "attempt_passed_stage_count")
        $info.attempt_incomplete_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "attempt_incomplete_stage_count")
        $info.visual_baseline_render_status = [string](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_render_status")
        $info.visual_baseline_fresh_rendered_count = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_fresh_rendered_count")
        $info.aggregate_contact_sheet_status = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet_status")
        $info.aggregate_contact_sheet = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet")
        $info.aggregate_contact_sheet_bytes = [int64](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet_bytes")
        $info.aggregate_rebuild_status = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_rebuild_status")
        $info.aggregate_rebuild_verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_rebuild_verdict")
        $info.aggregate_rebuild_selected_baseline_count = [int](Get-OptionalPropertyValue -Object $summary -Name "aggregate_rebuild_selected_baseline_count")
        $info.aggregate_rebuild_expected_visual_render_count = [int](Get-OptionalPropertyValue -Object $summary -Name "aggregate_rebuild_expected_visual_render_count")
    } catch {
        $info.status = "unreadable"
        $info.error = $_.Exception.Message
    }

    return $info
}

function Get-PdfBoundedCtestDefaultSummaryPaths {
    param([string]$RepoRoot)

    return @(
        "build\pdf-ctest-bounded-subset-current\summary.json",
        "build\pdf-ctest-bounded-contract-static-current\summary.json",
        "build\pdf-ctest-bounded-cjk-flow-static-current\summary.json",
        "build\pdf-ctest-bounded-regression-basic-text-current\summary.json",
        "build\pdf-ctest-bounded-regression-styled-document-current\summary.json",
        "build\pdf-ctest-bounded-regression-business-samples-current\summary.json",
        "build\pdf-ctest-bounded-regression-table-layout-current\summary.json"
    ) | ForEach-Object { Join-Path $RepoRoot $_ }
}

function Resolve-PdfBoundedCtestSummaryPaths {
    param(
        [string]$RepoRoot,
        [string[]]$SummaryJson
    )

    $requested = @($SummaryJson | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    $candidates = if ($requested.Count -gt 0) {
        @($requested | ForEach-Object { Resolve-FullPath -RepoRoot $RepoRoot -InputPath ([string]$_) })
    } else {
        @(Get-PdfBoundedCtestDefaultSummaryPaths -RepoRoot $RepoRoot)
    }

    $seen = @{}
    return @(
        foreach ($candidate in $candidates) {
            if ([string]::IsNullOrWhiteSpace([string]$candidate)) { continue }
            $resolved = [System.IO.Path]::GetFullPath([string]$candidate)
            if (-not (Test-Path -LiteralPath $resolved)) { continue }
            $key = $resolved.ToLowerInvariant()
            if ($seen.ContainsKey($key)) { continue }
            $seen[$key] = $true
            $resolved
        }
    )
}

function Get-PdfBoundedCtestSummaryInfo {
    param(
        [string[]]$SummaryJson,
        [string]$RepoRoot
    )

    $paths = @($SummaryJson | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    $summaries = New-Object 'System.Collections.Generic.List[object]'
    $subsets = New-Object 'System.Collections.Generic.List[string]'
    $displayPaths = New-Object 'System.Collections.Generic.List[string]'
    $passCount = 0
    $selectedTestCount = 0
    $skippedTestCount = 0
    $errorCount = 0
    $errors = New-Object 'System.Collections.Generic.List[string]'

    foreach ($path in $paths) {
        try {
            $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
            $status = [string](Get-OptionalPropertyValue -Object $summary -Name "status")
            $verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "verdict")
            $subset = [string](Get-OptionalPropertyValue -Object $summary -Name "subset")
            $selected = [int](Get-OptionalPropertyValue -Object $summary -Name "selected_test_count")
            $skipped = [int](Get-OptionalPropertyValue -Object $summary -Name "skipped_test_count")
            if ($status -eq "pass" -and $verdict -eq "pass") {
                $passCount++
            }
            if (-not [string]::IsNullOrWhiteSpace($subset)) {
                $subsets.Add($subset) | Out-Null
            }
            $selectedTestCount += $selected
            $skippedTestCount += $skipped
            $displayPaths.Add((Get-RepoRelativePath -RepoRoot $RepoRoot -Path $path)) | Out-Null
            $summaries.Add([ordered]@{
                    summary_json = $path
                    summary_json_display = Get-RepoRelativePath -RepoRoot $RepoRoot -Path $path
                    status = $status
                    verdict = $verdict
                    subset = $subset
                    selected_test_count = $selected
                    skipped_test_count = $skipped
                    ctest_timeout_seconds = [int](Get-OptionalPropertyValue -Object $summary -Name "ctest_timeout_seconds")
                    exit_code = [int](Get-OptionalPropertyValue -Object $summary -Name "exit_code")
                }) | Out-Null
        } catch {
            $errorCount++
            $errors.Add(("{0}: {1}" -f (Get-RepoRelativePath -RepoRoot $RepoRoot -Path $path), $_.Exception.Message)) | Out-Null
        }
    }

    $summaryCount = $summaries.Count
    $statusValue = if ($summaryCount -eq 0) {
        "not_available"
    } elseif ($errorCount -gt 0) {
        "partial"
    } elseif ($passCount -eq $summaryCount -and $skippedTestCount -eq 0) {
        "pass"
    } else {
        "needs_review"
    }

    return [ordered]@{
        status = $statusValue
        summary_count = $summaryCount
        pass_count = $passCount
        skipped_test_count = $skippedTestCount
        selected_test_count = $selectedTestCount
        subsets = @($subsets.ToArray())
        summary_json = @($paths)
        summary_json_display = @($displayPaths.ToArray())
        summaries = @($summaries.ToArray())
        error_count = $errorCount
        errors = @($errors.ToArray())
    }
}

function Get-PdfFullCtestReadinessSummaryInfo {
    param(
        [string]$SummaryJson,
        [string]$RepoRoot
    )

    $info = [ordered]@{
        requested = -not [string]::IsNullOrWhiteSpace($SummaryJson)
        status = if ([string]::IsNullOrWhiteSpace($SummaryJson)) { "not_requested" } elseif (Test-Path -LiteralPath $SummaryJson) { "available" } else { "missing" }
        summary_json = $SummaryJson
        summary_json_display = Get-RepoRelativePath -RepoRoot $RepoRoot -Path $SummaryJson
        verdict = ""
        release_ready = $false
        warning_count = 0
        visual_gate_release_evidence_accepted = $false
        visual_gate_fresh_full_guarded_evidence = $false
        visual_gate_pass_summary_before_outer_timeout = $false
        visual_gate_segmented_full_coverage_evidence = $false
        visual_full_gate_status = ""
        visual_full_gate_verdict = ""
        full_ctest_status = ""
        full_ctest_verdict = ""
        full_ctest_summary_json = ""
        full_ctest_summary_json_display = ""
        outer_guard_status = ""
        outer_guard_timed_out = $false
        selected_test_count = 0
        completed_test_count = 0
        passed_test_count = 0
        failed_test_count = 0
        skipped_test_count = 0
        not_run_test_count = 0
        completion_percent = 0.0
        remaining_test_count = 0
        zero_failed_tests_observed = $false
        boundary = ""
        marker = ""
        error = ""
    }

    if (-not [bool]$info.requested -or -not (Test-Path -LiteralPath $SummaryJson)) {
        return $info
    }

    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $SummaryJson | ConvertFrom-Json
        $info.status = [string](Get-OptionalPropertyValue -Object $summary -Name "status")
        $info.verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "verdict")
        $info.release_ready = [bool](Get-OptionalPropertyValue -Object $summary -Name "release_ready")
        $info.warning_count = [int](Get-OptionalPropertyValue -Object $summary -Name "warning_count")
        $info.visual_gate_release_evidence_accepted = [bool](Get-OptionalPropertyValue -Object $summary -Name "visual_gate_release_evidence_accepted")
        $info.visual_gate_fresh_full_guarded_evidence = [bool](Get-OptionalPropertyValue -Object $summary -Name "visual_gate_fresh_full_guarded_evidence")
        $info.visual_gate_pass_summary_before_outer_timeout = [bool](Get-OptionalPropertyValue -Object $summary -Name "visual_gate_pass_summary_before_outer_timeout")
        $info.visual_gate_segmented_full_coverage_evidence = [bool](Get-OptionalPropertyValue -Object $summary -Name "visual_gate_segmented_full_coverage_evidence")
        $info.visual_full_gate_status = [string](Get-OptionalPropertyValue -Object $summary -Name "visual_full_gate_status")
        $info.visual_full_gate_verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "visual_full_gate_verdict")
        $info.full_ctest_status = [string](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_status")
        $info.full_ctest_verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_verdict")
        $info.full_ctest_summary_json = [string](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_summary_json")
        $info.full_ctest_summary_json_display = [string](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_summary_json_display")
        $info.outer_guard_status = [string](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_outer_guard_status")
        $info.outer_guard_timed_out = [bool](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_outer_guard_timed_out")
        $info.selected_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_selected_test_count")
        $info.completed_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_completed_test_count")
        $info.passed_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_passed_test_count")
        $info.failed_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_failed_test_count")
        $info.skipped_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_skipped_test_count")
        $info.not_run_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_not_run_test_count")
        $info.completion_percent = [double](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_completion_percent")
        $info.remaining_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_remaining_test_count")
        $info.zero_failed_tests_observed = [bool](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_zero_failed_tests_observed")
        $info.boundary = [string](Get-OptionalPropertyValue -Object $summary -Name "boundary")
        $info.marker = [string](Get-OptionalPropertyValue -Object $summary -Name "marker")
    } catch {
        $info.status = "unreadable"
        $info.error = $_.Exception.Message
    }

    return $info
}
