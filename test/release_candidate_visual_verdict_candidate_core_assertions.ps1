if ($Scenario -ne "candidate_reports") {
if ($candidateSummary.execution_status -ne "pass") {
    throw "Release candidate dry run should pass when all heavy flows are skipped."
}
if ($candidateSummary.steps.pdf_visual_gate.status -ne "loaded" -or
    $candidateSummary.steps.pdf_visual_gate.verdict -ne "pass" -or
    -not [bool]$candidateSummary.steps.pdf_visual_gate.finalizable) {
    throw "Release candidate summary did not consume the PDF visual gate pass verdict as finalizable evidence."
}
if ([int]$candidateSummary.steps.pdf_visual_gate.cjk_manifest_count -ne 43 -or
    [int]$candidateSummary.steps.pdf_visual_gate.cjk_copy_search_count -ne 43 -or
    [int]$candidateSummary.steps.pdf_visual_gate.cjk_copy_search_missing_text_count -ne 0 -or
    [int]$candidateSummary.steps.pdf_visual_gate.visual_baseline_manifest_count -ne 42 -or
    [int]$candidateSummary.steps.pdf_visual_gate.visual_baseline_count -ne 44) {
    throw "Release candidate summary did not preserve PDF visual gate sample counts."
}
if ([string]$candidateSummary.pdf_visual_gate_summary_json -ne $expectedPdfSummaryPath -or
    [string]$candidateSummary.steps.pdf_visual_gate.summary_json -ne $expectedPdfSummaryPath -or
    [string]$candidateSummary.steps.pdf_visual_gate.aggregate_contact_sheet -ne $expectedPdfContactSheetPath) {
    throw "Release candidate summary did not preserve PDF visual gate evidence paths."
}
if ($candidateSummary.steps.pdf_visual_segmented_gate.status -ne "pass" -or
    $candidateSummary.steps.pdf_visual_segmented_gate.verdict -ne "pass" -or
    $candidateSummary.steps.pdf_visual_segmented_gate.full_visual_gate_status -ne "not_complete" -or
    $candidateSummary.steps.pdf_visual_segmented_gate.evidence_scope -ne "segmented_visual_gate_auxiliary_only") {
    throw "Release candidate summary did not preserve segmented PDF visual gate auxiliary status."
}
if ([string]$candidateSummary.pdf_visual_segmented_gate_summary_json -ne $expectedPdfSegmentedSummaryPath -or
    [int]$candidateSummary.steps.pdf_visual_segmented_gate.slice_pass_count -ne 4 -or
    [int]$candidateSummary.steps.pdf_visual_segmented_gate.covered_baseline_count -ne 44 -or
    [int]$candidateSummary.steps.pdf_visual_segmented_gate.expected_visual_render_count -ne 44 -or
    [string]$candidateSummary.steps.pdf_visual_segmented_gate.summary_json -ne $expectedPdfSegmentedSummaryPath -or
    [string]$candidateSummary.steps.pdf_visual_segmented_gate.aggregate_contact_sheet -ne $expectedPdfContactSheetPath) {
    throw "Release candidate summary did not preserve segmented PDF visual gate counts or paths."
}
if ([int]$candidateSummary.steps.pdf_bounded_ctest.summary_count -ne 2 -or
    [int]$candidateSummary.steps.pdf_bounded_ctest.pass_count -ne 2 -or
    [int]$candidateSummary.steps.pdf_bounded_ctest.selected_test_count -ne 20 -or
    [int]$candidateSummary.steps.pdf_bounded_ctest.skipped_test_count -ne 0 -or
    @($candidateSummary.steps.pdf_bounded_ctest.subsets) -notcontains "regression-business-samples" -or
    @($candidateSummary.steps.pdf_bounded_ctest.import_diagnostics_contract_tests) -notcontains "pdf_import_table_heuristic" -or
    @($candidateSummary.steps.pdf_bounded_ctest.import_diagnostics_contract_fields) -notcontains "table_continuation_diagnostics=[]" -or
    @($candidateSummary.steps.pdf_bounded_ctest.import_negative_boundary_contract_cases) -notcontains "short_label_prose_remains_paragraphs") {
    throw "Release candidate summary did not preserve PDF bounded CTest auxiliary evidence."
}
Assert-PdfImportDiagnosticsContractFieldsPresent `
    -Actual @($candidateSummary.steps.pdf_bounded_ctest.import_diagnostics_contract_fields) `
    -MessagePrefix "Release candidate summary did not preserve import diagnostic contract fields."
if ([string]$candidateSummary.pdf_release_readiness_summary_json -ne $expectedPdfReadinessSummaryPath -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.summary_json -ne $expectedPdfReadinessSummaryPath -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.status -ne "pass" -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.verdict -ne "pass_with_warnings" -or
    -not [bool]$candidateSummary.steps.pdf_full_ctest_readiness.release_ready -or
    -not [bool]$candidateSummary.steps.pdf_full_ctest_readiness.visual_gate_release_evidence_accepted -or
    [bool]$candidateSummary.steps.pdf_full_ctest_readiness.visual_gate_fresh_full_guarded_evidence -or
    [bool]$candidateSummary.steps.pdf_full_ctest_readiness.visual_gate_pass_summary_before_outer_timeout -or
    -not [bool]$candidateSummary.steps.pdf_full_ctest_readiness.visual_gate_segmented_full_coverage_evidence -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.visual_full_gate_status -ne "timeout" -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.full_ctest_status -ne "timeout" -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.full_ctest_verdict -ne "not_complete" -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.full_ctest_summary_json -ne $expectedFullPdfCtestSummaryPath -or
    [int]$candidateSummary.steps.pdf_full_ctest_readiness.selected_test_count -ne 139 -or
    [int]$candidateSummary.steps.pdf_full_ctest_readiness.completed_test_count -ne 133 -or
    [int]$candidateSummary.steps.pdf_full_ctest_readiness.failed_test_count -ne 0 -or
    [int]$candidateSummary.steps.pdf_full_ctest_readiness.not_run_test_count -ne 6 -or
    [double]$candidateSummary.steps.pdf_full_ctest_readiness.completion_percent -ne 95.7 -or
    -not [bool]$candidateSummary.steps.pdf_full_ctest_readiness.zero_failed_tests_observed) {
    throw "Release candidate summary did not preserve PDF full CTest readiness evidence."
}
if ($candidateSummary.steps.pdf_visual_gate_attempt.status -ne "partial" -or
    $candidateSummary.steps.pdf_visual_gate_attempt.verdict -ne "not_complete" -or
    $candidateSummary.steps.pdf_visual_gate_attempt.outer_guard_status -ne "timed_out" -or
    -not [bool]$candidateSummary.steps.pdf_visual_gate_attempt.outer_guard_timed_out -or
    [int]$candidateSummary.steps.pdf_visual_gate_attempt.outer_guard_timeout_seconds -ne 60) {
    throw "Release candidate summary did not preserve PDF visual gate attempt outer-guard evidence."
}
if ([int]$candidateSummary.warning_count -ne 1 -or @($candidateSummary.warnings).Count -ne 1) {
    throw "Release candidate summary should expose one machine-readable PDF fresh-attempt warning."
}
$pdfAttemptWarning = @($candidateSummary.warnings) |
    Where-Object { [string]$_.id -eq "pdf_visual_gate_attempt.incomplete_fresh_render" } |
    Select-Object -First 1
if ($null -eq $pdfAttemptWarning -or
    [string]$pdfAttemptWarning.action -ne "review_pdf_visual_gate_attempt_and_finalize_evidence" -or
    [string]$pdfAttemptWarning.source_schema -ne "featherdoc.release_candidate_summary" -or
    [string]$candidateSummary.pdf_visual_gate_attempt_summary_json -ne $expectedPdfAttemptSummaryPath -or
    [string]$pdfAttemptWarning.source_json -ne $expectedPdfAttemptSummaryPath -or
    [string]$pdfAttemptWarning.source_json_display -notmatch [regex]::Escape("attempt-summary.json") -or
    [string]$pdfAttemptWarning.outer_guard_status -ne "timed_out" -or
    -not [bool]$pdfAttemptWarning.outer_guard_timed_out -or
    [int]$pdfAttemptWarning.outer_guard_timeout_seconds -ne 60 -or
    -not [bool]$pdfAttemptWarning.visual_gate_release_evidence_accepted -or
    [bool]$pdfAttemptWarning.visual_gate_fresh_full_guarded_evidence -or
    [bool]$pdfAttemptWarning.visual_gate_pass_summary_before_outer_timeout -or
    -not [bool]$pdfAttemptWarning.visual_gate_segmented_full_coverage_evidence -or
    [bool]$pdfAttemptWarning.visual_gate_finalize_only -or
    -not [bool]$pdfAttemptWarning.release_owner_acceptance_required -or
    [string]$pdfAttemptWarning.release_owner_acceptance_boundary -ne "acceptance_does_not_replace_fresh_single_run_full_visual_gate" -or
    [string]$pdfAttemptWarning.visual_baseline_render_status -ne "partial" -or
    [int]$pdfAttemptWarning.visual_baseline_fresh_missing_sample_count -ne 7 -or
    -not [bool]$pdfAttemptWarning.visual_baseline_resume_needed -or
    [int]$pdfAttemptWarning.visual_baseline_resume_slice_offset -ne 37 -or
    [int]$pdfAttemptWarning.visual_baseline_resume_slice_limit -ne 7 -or
    [string]$pdfAttemptWarning.visual_baseline_resume_slice_command_template -notmatch "VisualBaselineSliceOnly" -or
    [string]$pdfAttemptWarning.aggregate_contact_sheet_status -ne "stale") {
    throw "Release candidate summary did not materialize the PDF fresh-attempt warning with reviewer-facing evidence."
}
Assert-ContainsText -Text ([string]$pdfAttemptWarning.message) -ExpectedText "segmented full-coverage visual evidence" `
    -Message "PDF fresh-attempt warning should explain that release evidence is accepted through segmented coverage."
Assert-ContainsText -Text ([string]$pdfAttemptWarning.release_owner_acceptance_policy) -ExpectedText "segmented_full_coverage" `
    -Message "PDF fresh-attempt warning should expose the release-owner acceptance policy."
Assert-ContainsText -Text ([string]$pdfAttemptWarning.release_owner_acceptance_command_template) -ExpectedText "run_release_candidate_checks.ps1" `
    -Message "PDF fresh-attempt warning should expose the release-owner acceptance command template."
Assert-ContainsText -Text ([string]$pdfAttemptWarning.visual_baseline_resume_slice_command_template) -ExpectedText "run_pdf_visual_release_gate.ps1" `
    -Message "PDF fresh-attempt warning should expose the visual baseline resume command template."
if ([string]$pdfAttemptWarning.message -match "relies on FinalizeOnly") {
    throw "PDF fresh-attempt warning should not claim FinalizeOnly evidence when segmented full coverage is accepted."
}

$passAttemptWarnings = @(Get-PdfVisualGateAttemptReleaseWarnings -ReleaseSummary ([ordered]@{
            pdf_visual_gate_attempt = [ordered]@{
                status = "pass"
                verdict = "pass"
                full_visual_gate_status = "pass"
                outer_guard_status = "timed_out_after_pass_summary"
                outer_guard_timed_out = $true
                outer_guard_timeout_seconds = 60
                visual_baseline_render_status = "pass"
                aggregate_contact_sheet_status = "pass"
            }
            pdf_full_ctest_readiness = [ordered]@{
                verdict = "pass"
                visual_gate_release_evidence_accepted = $true
                visual_gate_fresh_full_guarded_evidence = $true
                visual_gate_pass_summary_before_outer_timeout = $true
                visual_gate_segmented_full_coverage_evidence = $false
            }
            pdf_visual_gate = [ordered]@{
                finalize_only = $false
                skip_preflight = $true
            }
        }) -RepoRoot $resolvedRepoRoot)
if ($passAttemptWarnings.Count -ne 0) {
    throw "Passing PDF visual attempt with pass-summary-before-timeout evidence should not emit a fresh-attempt warning."
}

$acceptedSegmentedAttemptWarnings = @(Get-PdfVisualGateAttemptReleaseWarnings -ReleaseSummary ([ordered]@{
            pdf_visual_gate_attempt = [ordered]@{
                status = "partial"
                verdict = "not_complete"
                full_visual_gate_status = "not_complete"
                evidence_scope = "bounded_attempt_auxiliary_only"
                stage_count = 6
                passed_stage_count = 6
                failed_stage_count = 0
                incomplete_stage_count = 0
                pdf_regression_status = "pass"
                pdf_regression_failed_test_count = 0
                cjk_copy_search_status = "pass"
                cjk_copy_search_missing_text_count = 0
                visual_baseline_render_status = "pass"
                visual_baseline_fresh_rendered_count = 44
                expected_visual_render_count = 44
                aggregate_contact_sheet_status = "pass"
                outer_guard_status = "timed_out"
                outer_guard_timed_out = $true
                outer_guard_timeout_seconds = 60
            }
            pdf_full_ctest_readiness = [ordered]@{
                release_ready = $true
                verdict = "pass_with_warnings"
                visual_gate_release_evidence_accepted = $true
                visual_gate_fresh_full_guarded_evidence = $false
                visual_gate_pass_summary_before_outer_timeout = $false
                visual_gate_segmented_full_coverage_evidence = $true
            }
            pdf_visual_gate = [ordered]@{
                finalize_only = $false
                skip_preflight = $false
            }
        }) -RepoRoot $resolvedRepoRoot)
if ($acceptedSegmentedAttemptWarnings.Count -ne 0) {
    throw "Completed auxiliary PDF visual attempt with accepted segmented full coverage should not occupy release warning_count."
}

if ([int]$candidateSummary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count -ne 2 -or
    [int]$candidateSummary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count -ne 2) {
    throw "Release candidate summary did not consume project-template release entry evidence from release governance handoff."
}
if ([int]$candidateSummary.release_governance_handoff.word_visual_standard_review_metadata_source_report_count -ne 1 -or
    @($candidateSummary.release_governance_handoff.word_visual_standard_review_metadata_source_reports).Count -ne 1 -or
    [int]$candidateSummary.steps.release_governance_handoff.word_visual_standard_review_metadata_source_report_count -ne 1 -or
    @($candidateSummary.steps.release_governance_handoff.word_visual_standard_review_metadata_source_reports).Count -ne 1) {
    throw "Release candidate summary did not consume Word visual standard review metadata evidence from release governance handoff."
}
$wordVisualMetadataReport = @($candidateSummary.release_governance_handoff.word_visual_standard_review_metadata_source_reports) |
    Where-Object { [string]$_.schema -eq "featherdoc.release_candidate_summary" } |
    Select-Object -First 1
if ($null -eq $wordVisualMetadataReport -or
    [int]$wordVisualMetadataReport.word_visual_standard_review_metadata_count -ne 4 -or
    [string]$wordVisualMetadataReport.word_visual_standard_review_status_summary -ne "reviewed=4" -or
    [string]$wordVisualMetadataReport.word_visual_standard_review_verdict_summary -ne "pass=4" -or
    @($wordVisualMetadataReport.word_visual_standard_review_task_keys).Count -ne 4 -or
    @($wordVisualMetadataReport.word_visual_standard_review_metadata).Count -ne 4) {
    throw "Release candidate summary lost Word visual standard review metadata details."
}
foreach ($entry in @($wordVisualMetadataReport.word_visual_standard_review_metadata)) {
    if ($entry.PSObject.Properties.Name -contains "review_note") {
        throw "Release candidate summary exposed Word visual standard review notes in handoff metadata."
    }
}
if ([int]$candidateSummary.governance_metric_count -ne 2 -or @($candidateSummary.governance_metrics).Count -ne 2) {
    throw "Release candidate summary did not expose both release governance metrics as top-level material safety evidence."
}
$numberingMetric = @($candidateSummary.governance_metrics) |
    Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
    Select-Object -First 1
if ($null -eq $numberingMetric -or [string]::IsNullOrWhiteSpace([string]$numberingMetric.level) -or $null -eq $numberingMetric.score -or
    [int]$numberingMetric.details.catalog_coverage_percent -ne 100 -or
    @($numberingMetric.details.catalog_document_keys).Count -ne 2 -or
    @($numberingMetric.details.style_numbering_issues).Count -ne 0) {
    throw "Release candidate summary lost the numbering governance metric details."
}
$tableMetric = @($candidateSummary.governance_metrics) |
    Where-Object { [string]$_.id -eq "table_layout_delivery_governance.delivery_quality" } |
    Select-Object -First 1
if ($null -eq $tableMetric -or [string]::IsNullOrWhiteSpace([string]$tableMetric.level) -or $null -eq $tableMetric.score -or
    [int]$tableMetric.details.ready_document_percent -ne 100 -or
    [int]$tableMetric.details.unresolved_item_count -ne 0 -or
    [int]$tableMetric.details.floating_table_plans_pending -ne 0) {
    throw "Release candidate summary lost the table-layout delivery governance metric details."
}
if ([string]$tableMetric.details.pdf_floating_table_support_coverage -ne "4/9 supported (44 percent); metadata_only=5" -or
    [string]$tableMetric.details.pdf_floating_table_reviewer_focus -ne "review metadata-only tblpPr fields before approving PDF-layout-sensitive release.") {
    throw "Release candidate summary lost the PDF floating table reviewer focus details."
}
Assert-ContainsText -Text (($tableMetric.details.metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "tblOverlap" `
    -Message "Release candidate summary lost generic PDF floating table metadata-only fields."
Assert-ContainsText -Text (($tableMetric.details.review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "table overlap avoidance and collision resolution" `
    -Message "Release candidate summary lost generic PDF floating table review-required fields."
if ([int]$candidateSummary.release_governance_handoff.report_count -ne 6 -or
    @($candidateSummary.release_governance_handoff.reports).Count -ne 6) {
    throw "Release candidate summary did not preserve release governance source reports for material rendering."
}
if ($null -eq $candidateSummary.release_governance_handoff.project_template_delivery_readiness_contract -or
    [string]$candidateSummary.release_governance_handoff.project_template_delivery_readiness_contract.status -ne "ready" -or
    $null -eq $candidateSummary.release_governance_handoff.project_template_onboarding_governance_contract -or
    [string]$candidateSummary.release_governance_handoff.project_template_onboarding_governance_contract.status -ne "ready") {
    throw "Release candidate summary did not preserve project-template governance contracts from handoff."
}
$manifestSignoff = $candidateSummary.manifest_signoff_entrypoints
$expectedReleaseAssetsManifest = "output\release-assets\v$($candidateSummary.release_version)\release_assets_manifest.json"
if ($manifestSignoff.status -ne "declared" -or
    [int]$manifestSignoff.required_entrypoint_count -ne 3 -or
    [string]$manifestSignoff.release_assets_manifest -notmatch [regex]::Escape($expectedReleaseAssetsManifest)) {
    throw "Release candidate summary did not declare the packaged manifest signoff entrypoints."
}
foreach ($entrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
    $entrypoint = @($manifestSignoff.entrypoints) |
        Where-Object { [string]$_.id -eq $entrypointId } |
        Select-Object -First 1
    if ($null -eq $entrypoint -or -not [bool]$entrypoint.required -or [string]::IsNullOrWhiteSpace([string]$entrypoint.path_display)) {
        throw "Release candidate summary did not declare required manifest signoff entrypoint '$entrypointId'."
    }
}
foreach ($contractName in @(
        "project_template_delivery_readiness_contract",
        "project_template_onboarding_governance_contract"
    )) {
    if (@($manifestSignoff.required_contracts) -notcontains $contractName) {
        throw "Release candidate summary did not declare required manifest contract '$contractName'."
    }
}
foreach ($fieldName in @(
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
    )) {
    if (@($manifestSignoff.required_fields) -notcontains $fieldName) {
        throw "Release candidate summary did not declare required manifest field '$fieldName'."
    }
}
if ([string]$manifestSignoff.checklist_marker -ne "reviewer_manifest_scoped_project_template_trace") {
    throw "Release candidate summary did not preserve the reviewer manifest checklist marker."
}

$projectTemplateChecklistEntrypoints = $candidateSummary.project_template_readiness_checklist_entrypoints
if ($projectTemplateChecklistEntrypoints.status -ne "declared" -or
    [int]$projectTemplateChecklistEntrypoints.required_entrypoint_count -ne 3 -or
    [string]$projectTemplateChecklistEntrypoints.checklist_path -ne "docs/project_template_release_readiness_checklist_zh.rst") {
    throw "Release candidate summary did not declare the project-template readiness checklist entrypoints."
}
if ([string]$projectTemplateChecklistEntrypoints.checklist_label -ne "Project template release readiness checklist" -or
    [string]$projectTemplateChecklistEntrypoints.checklist_marker -ne "release_entry_project_template_readiness_checklist_trace") {
    throw "Release candidate summary did not preserve the project-template readiness checklist identity."
}
foreach ($entrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
    $entrypoint = @($projectTemplateChecklistEntrypoints.entrypoints) |
        Where-Object { [string]$_.id -eq $entrypointId } |
        Select-Object -First 1
    if ($null -eq $entrypoint -or -not [bool]$entrypoint.required -or [string]::IsNullOrWhiteSpace([string]$entrypoint.path_display)) {
        throw "Release candidate summary did not declare required project-template checklist entrypoint '$entrypointId'."
    }
}
if ($Scenario -eq "candidate_core") {
    Write-Host "Release candidate visual verdict core regression passed."
    return
}
}
