$badReleaseGovernanceHandoffPdfBoundedSplitDir = Join-Path $failDir "release-governance-handoff-pdf-bounded-split"
$badReleaseGovernanceHandoffPdfBoundedSplitPath = Join-Path $badReleaseGovernanceHandoffPdfBoundedSplitDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfBoundedSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfBoundedSplitPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``

- Detached bounded CTest evidence:
  - pdf_bounded_ctest_summary_count: ``7``
  - pdf_bounded_ctest_pass_count: ``7``
  - pdf_bounded_ctest_skipped_test_count: ``0``
  - pdf_bounded_ctest_selected_test_count: ``70``
  - pdf_bounded_ctest_subsets: ``smoke-import, regression-business-samples``
  - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-regression-business-samples-current\summary.json``
"@

$badReleaseGovernanceHandoffPdfBoundedSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfBoundedSplitPath
} catch {
    $badReleaseGovernanceHandoffPdfBoundedSplitFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfBoundedSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with bounded PDF CTest evidence split outside source_report block."
}

$passReleaseGovernanceHandoffPdfAttemptTraceDir = Join-Path $passDir "release-governance-handoff-pdf-attempt-trace"
$passReleaseGovernanceHandoffPdfAttemptTracePath = Join-Path $passReleaseGovernanceHandoffPdfAttemptTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffPdfAttemptTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffPdfAttemptTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``smoke-import, contract-static, cjk-flow-static, regression-basic-text, regression-styled-document, regression-business-samples, regression-table-layout``
    - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-subset-current\summary.json, .\build\pdf-ctest-bounded-contract-static-current\summary.json, .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json, .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json, .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json, .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json, .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json``
    - pdf_visual_gate_attempt_status: ``partial``
    - pdf_visual_gate_attempt_verdict: ``not_complete``
    - pdf_visual_gate_attempt_full_visual_gate_status: ``not_complete``
    - pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``
    - pdf_visual_gate_attempt_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\attempt-summary.json``
    - pdf_visual_gate_attempt_stage_count: ``6``
    - pdf_visual_gate_attempt_passed_stage_count: ``4``
    - pdf_visual_gate_attempt_failed_stage_count: ``0``
    - pdf_visual_gate_attempt_incomplete_stage_count: ``2``
    - pdf_visual_gate_attempt_pdf_cli_export_status: ``pass``
    - pdf_visual_gate_attempt_pdf_regression_status: ``pass``
    - pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``91``
    - pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``0``
    - pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``
    - pdf_visual_gate_attempt_unicode_font_status: ``pass``
    - pdf_visual_gate_attempt_cjk_copy_search_status: ``pass``
    - pdf_visual_gate_attempt_cjk_copy_search_count: ``43``
    - pdf_visual_gate_attempt_cjk_copy_search_missing_text_count: ``0``
    - pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``
    - pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``22``
    - pdf_visual_gate_attempt_expected_visual_render_count: ``44``
    - pdf_visual_gate_attempt_visual_baseline_missing_pdf_count: ``0``
    - pdf_visual_gate_attempt_visual_baseline_pdf_total_bytes: ``7340032``
    - pdf_visual_gate_attempt_visual_baseline_png_page_count: ``44``
    - pdf_visual_gate_attempt_visual_baseline_missing_png_page_count: ``0``
    - pdf_visual_gate_attempt_visual_baseline_png_total_bytes: ``2097152``
    - pdf_visual_gate_attempt_visual_baseline_unreadable_png_dimension_count: ``0``
    - pdf_visual_gate_attempt_aggregate_contact_sheet_status: ``stale``
    - pdf_visual_gate_attempt_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
"@

& $auditScript -Path $passReleaseGovernanceHandoffPdfAttemptTracePath

$badReleaseGovernanceHandoffPdfAttemptSplitDir = Join-Path $failDir "release-governance-handoff-pdf-attempt-split"
$badReleaseGovernanceHandoffPdfAttemptSplitPath = Join-Path $badReleaseGovernanceHandoffPdfAttemptSplitDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfAttemptSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfAttemptSplitPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``smoke-import, contract-static, cjk-flow-static, regression-basic-text, regression-styled-document, regression-business-samples, regression-table-layout``
    - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-subset-current\summary.json, .\build\pdf-ctest-bounded-contract-static-current\summary.json, .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json, .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json, .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json, .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json, .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json``

- Detached PDF visual gate attempt evidence:
  - pdf_visual_gate_attempt_status: ``partial``
  - pdf_visual_gate_attempt_verdict: ``not_complete``
  - pdf_visual_gate_attempt_full_visual_gate_status: ``not_complete``
  - pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``
  - pdf_visual_gate_attempt_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\attempt-summary.json``
  - pdf_visual_gate_attempt_stage_count: ``6``
  - pdf_visual_gate_attempt_passed_stage_count: ``4``
  - pdf_visual_gate_attempt_failed_stage_count: ``0``
  - pdf_visual_gate_attempt_incomplete_stage_count: ``2``
  - pdf_visual_gate_attempt_pdf_cli_export_status: ``pass``
  - pdf_visual_gate_attempt_pdf_regression_status: ``pass``
  - pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``91``
  - pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``0``
  - pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``
  - pdf_visual_gate_attempt_unicode_font_status: ``pass``
  - pdf_visual_gate_attempt_cjk_copy_search_status: ``pass``
  - pdf_visual_gate_attempt_cjk_copy_search_count: ``43``
  - pdf_visual_gate_attempt_cjk_copy_search_missing_text_count: ``0``
  - pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``
  - pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``22``
  - pdf_visual_gate_attempt_expected_visual_render_count: ``44``
  - pdf_visual_gate_attempt_visual_baseline_missing_pdf_count: ``0``
  - pdf_visual_gate_attempt_visual_baseline_pdf_total_bytes: ``7340032``
  - pdf_visual_gate_attempt_visual_baseline_png_page_count: ``44``
  - pdf_visual_gate_attempt_visual_baseline_missing_png_page_count: ``0``
  - pdf_visual_gate_attempt_visual_baseline_png_total_bytes: ``2097152``
  - pdf_visual_gate_attempt_visual_baseline_unreadable_png_dimension_count: ``0``
  - pdf_visual_gate_attempt_aggregate_contact_sheet_status: ``stale``
  - pdf_visual_gate_attempt_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
"@

$badReleaseGovernanceHandoffPdfAttemptSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfAttemptSplitPath
} catch {
    $badReleaseGovernanceHandoffPdfAttemptSplitFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfAttemptSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with PDF visual gate attempt evidence split outside source_report block."
}

$passReleaseGovernanceHandoffPdfSegmentedTraceDir = Join-Path $passDir "release-governance-handoff-pdf-segmented-trace"
$passReleaseGovernanceHandoffPdfSegmentedTracePath = Join-Path $passReleaseGovernanceHandoffPdfSegmentedTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffPdfSegmentedTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffPdfSegmentedTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``smoke-import, contract-static, cjk-flow-static, regression-basic-text, regression-styled-document, regression-business-samples, regression-table-layout``
    - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-subset-current\summary.json, .\build\pdf-ctest-bounded-contract-static-current\summary.json, .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json, .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json, .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json, .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json, .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json``
    - pdf_visual_segmented_gate_status: ``pass``
    - pdf_visual_segmented_gate_verdict: ``pass``
    - pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``
    - pdf_visual_segmented_gate_evidence_scope: ``segmented_visual_gate_auxiliary_only``
    - pdf_visual_segmented_gate_boundary: ``segmented_summary_does_not_replace_full_visual_gate_verdict``
    - pdf_visual_segmented_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\segmented-summary.json``
    - pdf_visual_segmented_gate_slice_summary_count: ``4``
    - pdf_visual_segmented_gate_slice_pass_count: ``4``
    - pdf_visual_segmented_gate_slice_failed_count: ``0``
    - pdf_visual_segmented_gate_covered_baseline_count: ``44``
    - pdf_visual_segmented_gate_expected_visual_render_count: ``44``
    - pdf_visual_segmented_gate_attempt_stage_count: ``6``
    - pdf_visual_segmented_gate_attempt_passed_stage_count: ``6``
    - pdf_visual_segmented_gate_visual_baseline_render_status: ``pass``
    - pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``pass``
    - pdf_visual_segmented_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``1822428``
    - pdf_visual_segmented_gate_aggregate_rebuild_status: ``pass``
    - pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count: ``44``
"@

& $auditScript -Path $passReleaseGovernanceHandoffPdfSegmentedTracePath

$badReleaseGovernanceHandoffPdfSegmentedSplitDir = Join-Path $failDir "release-governance-handoff-pdf-segmented-split"
$badReleaseGovernanceHandoffPdfSegmentedSplitPath = Join-Path $badReleaseGovernanceHandoffPdfSegmentedSplitDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfSegmentedSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfSegmentedSplitPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``smoke-import, contract-static, cjk-flow-static, regression-basic-text, regression-styled-document, regression-business-samples, regression-table-layout``
    - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-subset-current\summary.json, .\build\pdf-ctest-bounded-contract-static-current\summary.json, .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json, .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json, .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json, .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json, .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json``

- Detached PDF visual segmented gate evidence:
  - pdf_visual_segmented_gate_status: ``pass``
  - pdf_visual_segmented_gate_verdict: ``pass``
  - pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``
  - pdf_visual_segmented_gate_evidence_scope: ``segmented_visual_gate_auxiliary_only``
  - pdf_visual_segmented_gate_boundary: ``segmented_summary_does_not_replace_full_visual_gate_verdict``
  - pdf_visual_segmented_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\segmented-summary.json``
  - pdf_visual_segmented_gate_slice_summary_count: ``4``
  - pdf_visual_segmented_gate_slice_pass_count: ``4``
  - pdf_visual_segmented_gate_slice_failed_count: ``0``
  - pdf_visual_segmented_gate_covered_baseline_count: ``44``
  - pdf_visual_segmented_gate_expected_visual_render_count: ``44``
  - pdf_visual_segmented_gate_attempt_stage_count: ``6``
  - pdf_visual_segmented_gate_attempt_passed_stage_count: ``6``
  - pdf_visual_segmented_gate_visual_baseline_render_status: ``pass``
  - pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``pass``
  - pdf_visual_segmented_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
  - pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``1822428``
  - pdf_visual_segmented_gate_aggregate_rebuild_status: ``pass``
  - pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count: ``44``
"@

$badReleaseGovernanceHandoffPdfSegmentedSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfSegmentedSplitPath
} catch {
    $badReleaseGovernanceHandoffPdfSegmentedSplitFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfSegmentedSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with PDF visual segmented gate evidence split outside source_report block."
}

$badReleaseGovernanceHandoffPdfSegmentedWrongSchemaDir = Join-Path $failDir "release-governance-handoff-pdf-segmented-wrong-schema"
$badReleaseGovernanceHandoffPdfSegmentedWrongSchemaPath = Join-Path $badReleaseGovernanceHandoffPdfSegmentedWrongSchemaDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfSegmentedWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfSegmentedWrongSchemaPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\pdf-visual-release-gate-current\report\segmented-summary.json`` schema=``featherdoc.pdf_visual_segmented_gate_summary.v1``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``smoke-import, contract-static, cjk-flow-static, regression-basic-text, regression-styled-document, regression-business-samples, regression-table-layout``
    - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-subset-current\summary.json, .\build\pdf-ctest-bounded-contract-static-current\summary.json, .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json, .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json, .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json, .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json, .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json``
    - pdf_visual_segmented_gate_status: ``pass``
    - pdf_visual_segmented_gate_verdict: ``pass``
    - pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``
    - pdf_visual_segmented_gate_evidence_scope: ``segmented_visual_gate_auxiliary_only``
    - pdf_visual_segmented_gate_boundary: ``segmented_summary_does_not_replace_full_visual_gate_verdict``
    - pdf_visual_segmented_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\segmented-summary.json``
    - pdf_visual_segmented_gate_slice_summary_count: ``4``
    - pdf_visual_segmented_gate_slice_pass_count: ``4``
    - pdf_visual_segmented_gate_slice_failed_count: ``0``
    - pdf_visual_segmented_gate_covered_baseline_count: ``44``
    - pdf_visual_segmented_gate_expected_visual_render_count: ``44``
    - pdf_visual_segmented_gate_attempt_stage_count: ``6``
    - pdf_visual_segmented_gate_attempt_passed_stage_count: ``6``
    - pdf_visual_segmented_gate_visual_baseline_render_status: ``pass``
    - pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``pass``
    - pdf_visual_segmented_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``1822428``
    - pdf_visual_segmented_gate_aggregate_rebuild_status: ``pass``
    - pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count: ``44``
"@

$badReleaseGovernanceHandoffPdfSegmentedWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfSegmentedWrongSchemaPath
} catch {
    $badReleaseGovernanceHandoffPdfSegmentedWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfSegmentedWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with PDF visual segmented gate evidence using a non-release-candidate source_report schema."
}

$passReleaseBlockerRollupPdfAuxTraceDir = Join-Path $passDir "release-blocker-rollup-pdf-aux-trace"
$passReleaseBlockerRollupPdfAuxTracePath = Join-Path $passReleaseBlockerRollupPdfAuxTraceDir "release_blocker_rollup.md"
New-Item -ItemType Directory -Path $passReleaseBlockerRollupPdfAuxTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseBlockerRollupPdfAuxTracePath -Encoding UTF8 -Value @"
# Release Blocker Rollup Report

## Source Report Contracts

- ``featherdoc.release_candidate_summary``: status=``ready`` ready=``True`` path=``.\output\release-candidate-checks\summary.json``
  - pdf_visual_gate_status: ``loaded``
  - full_visual_gate_status: ``pass``
  - pdf_visual_gate_verdict: ``pass``
  - pdf_visual_gate_finalizable: ``True``
  - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
  - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
  - pdf_visual_gate_cjk_manifest_count: ``43``
  - pdf_visual_gate_cjk_copy_search_count: ``43``
  - pdf_visual_gate_cjk_missing_text_count: ``0``
  - pdf_visual_gate_visual_baseline_manifest_count: ``42``
  - pdf_visual_gate_visual_baseline_count: ``44``
  - pdf_bounded_ctest_summary_count: ``7``
  - pdf_bounded_ctest_pass_count: ``7``
  - pdf_bounded_ctest_skipped_test_count: ``0``
  - pdf_bounded_ctest_selected_test_count: ``70``
  - pdf_bounded_ctest_subsets: ``smoke-import, contract-static, cjk-flow-static, regression-basic-text, regression-styled-document, regression-business-samples, regression-table-layout``
  - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-regression-business-samples-current\summary.json``
  - pdf_visual_gate_attempt_status: ``partial``
  - pdf_visual_gate_attempt_verdict: ``not_complete``
  - pdf_visual_gate_attempt_full_visual_gate_status: ``not_complete``
  - pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``
  - pdf_visual_gate_attempt_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\attempt-summary.json``
  - pdf_visual_gate_attempt_stage_count: ``6``
  - pdf_visual_gate_attempt_passed_stage_count: ``4``
  - pdf_visual_gate_attempt_failed_stage_count: ``0``
  - pdf_visual_gate_attempt_incomplete_stage_count: ``2``
  - pdf_visual_gate_attempt_outer_guard_status: ``timed_out``
  - pdf_visual_gate_attempt_outer_guard_timed_out: ``True``
  - pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``60``
  - pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``91``
  - pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``0``
  - pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``
  - pdf_visual_gate_attempt_cjk_copy_search_count: ``43``
  - pdf_visual_gate_attempt_cjk_copy_search_missing_text_count: ``0``
  - pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``
  - pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``22``
  - pdf_visual_gate_attempt_expected_visual_render_count: ``44``
  - pdf_visual_gate_attempt_visual_baseline_missing_pdf_count: ``0``
  - pdf_visual_gate_attempt_visual_baseline_pdf_total_bytes: ``7340032``
  - pdf_visual_gate_attempt_visual_baseline_png_page_count: ``44``
  - pdf_visual_gate_attempt_visual_baseline_missing_png_page_count: ``0``
  - pdf_visual_gate_attempt_visual_baseline_png_total_bytes: ``2097152``
  - pdf_visual_gate_attempt_visual_baseline_unreadable_png_dimension_count: ``0``
  - pdf_visual_gate_attempt_aggregate_contact_sheet_status: ``stale``
  - pdf_visual_gate_attempt_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
  - pdf_visual_segmented_gate_status: ``pass``
  - pdf_visual_segmented_gate_verdict: ``pass``
  - pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``
  - pdf_visual_segmented_gate_evidence_scope: ``segmented_visual_gate_auxiliary_only``
  - pdf_visual_segmented_gate_boundary: ``segmented_summary_does_not_replace_full_visual_gate_verdict``
  - pdf_visual_segmented_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\segmented-summary.json``
  - pdf_visual_segmented_gate_slice_summary_count: ``4``
  - pdf_visual_segmented_gate_slice_pass_count: ``4``
  - pdf_visual_segmented_gate_slice_failed_count: ``0``
  - pdf_visual_segmented_gate_covered_baseline_count: ``44``
  - pdf_visual_segmented_gate_expected_visual_render_count: ``44``
  - pdf_visual_segmented_gate_attempt_stage_count: ``6``
  - pdf_visual_segmented_gate_attempt_passed_stage_count: ``6``
  - pdf_visual_segmented_gate_visual_baseline_render_status: ``pass``
  - pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``pass``
  - pdf_visual_segmented_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
  - pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``1822428``
  - pdf_visual_segmented_gate_aggregate_rebuild_status: ``pass``
  - pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count: ``44``
"@

& $auditScript -Path $passReleaseBlockerRollupPdfAuxTracePath

$badReleaseBlockerRollupPdfAuxSplitDir = Join-Path $failDir "release-blocker-rollup-pdf-aux-split"
$badReleaseBlockerRollupPdfAuxSplitPath = Join-Path $badReleaseBlockerRollupPdfAuxSplitDir "release_blocker_rollup.md"
New-Item -ItemType Directory -Path $badReleaseBlockerRollupPdfAuxSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBlockerRollupPdfAuxSplitPath -Encoding UTF8 -Value @"
# Release Blocker Rollup Report

## Source Report Contracts

- ``featherdoc.release_candidate_summary``: status=``ready`` ready=``True`` path=``.\output\release-candidate-checks\summary.json``
  - pdf_visual_gate_status: ``loaded``
  - full_visual_gate_status: ``pass``
  - pdf_visual_gate_verdict: ``pass``
  - pdf_visual_gate_finalizable: ``True``
  - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
  - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
  - pdf_visual_gate_cjk_manifest_count: ``43``
  - pdf_visual_gate_cjk_copy_search_count: ``43``
  - pdf_visual_gate_cjk_missing_text_count: ``0``
  - pdf_visual_gate_visual_baseline_manifest_count: ``42``
  - pdf_visual_gate_visual_baseline_count: ``44``

## Detached PDF auxiliary notes

- pdf_bounded_ctest_summary_count: ``7``
- pdf_bounded_ctest_pass_count: ``7``
- pdf_bounded_ctest_skipped_test_count: ``0``
- pdf_bounded_ctest_selected_test_count: ``70``
- pdf_bounded_ctest_subsets: ``smoke-import, regression-business-samples``
- pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-regression-business-samples-current\summary.json``
- pdf_visual_gate_attempt_status: ``partial``
- pdf_visual_gate_attempt_verdict: ``not_complete``
- pdf_visual_gate_attempt_full_visual_gate_status: ``not_complete``
- pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``
- pdf_visual_gate_attempt_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\attempt-summary.json``
- pdf_visual_gate_attempt_stage_count: ``6``
- pdf_visual_gate_attempt_passed_stage_count: ``4``
- pdf_visual_gate_attempt_failed_stage_count: ``0``
- pdf_visual_gate_attempt_incomplete_stage_count: ``2``
- pdf_visual_gate_attempt_outer_guard_status: ``timed_out``
- pdf_visual_gate_attempt_outer_guard_timed_out: ``True``
- pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``60``
- pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``91``
- pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``0``
- pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``
- pdf_visual_gate_attempt_cjk_copy_search_count: ``43``
- pdf_visual_gate_attempt_cjk_copy_search_missing_text_count: ``0``
- pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``
- pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``22``
- pdf_visual_gate_attempt_expected_visual_render_count: ``44``
- pdf_visual_gate_attempt_visual_baseline_missing_pdf_count: ``0``
- pdf_visual_gate_attempt_visual_baseline_pdf_total_bytes: ``7340032``
- pdf_visual_gate_attempt_visual_baseline_png_page_count: ``44``
- pdf_visual_gate_attempt_visual_baseline_missing_png_page_count: ``0``
- pdf_visual_gate_attempt_visual_baseline_png_total_bytes: ``2097152``
- pdf_visual_gate_attempt_visual_baseline_unreadable_png_dimension_count: ``0``
- pdf_visual_gate_attempt_aggregate_contact_sheet_status: ``stale``
- pdf_visual_gate_attempt_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
- pdf_visual_segmented_gate_status: ``pass``
- pdf_visual_segmented_gate_verdict: ``pass``
- pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``
- pdf_visual_segmented_gate_evidence_scope: ``segmented_visual_gate_auxiliary_only``
- pdf_visual_segmented_gate_boundary: ``segmented_summary_does_not_replace_full_visual_gate_verdict``
- pdf_visual_segmented_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\segmented-summary.json``
- pdf_visual_segmented_gate_slice_summary_count: ``4``
- pdf_visual_segmented_gate_slice_pass_count: ``4``
- pdf_visual_segmented_gate_slice_failed_count: ``0``
- pdf_visual_segmented_gate_covered_baseline_count: ``44``
- pdf_visual_segmented_gate_expected_visual_render_count: ``44``
- pdf_visual_segmented_gate_attempt_stage_count: ``6``
- pdf_visual_segmented_gate_attempt_passed_stage_count: ``6``
- pdf_visual_segmented_gate_visual_baseline_render_status: ``pass``
- pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``pass``
- pdf_visual_segmented_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
- pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``1822428``
- pdf_visual_segmented_gate_aggregate_rebuild_status: ``pass``
- pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count: ``44``
"@

$badReleaseBlockerRollupPdfAuxSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBlockerRollupPdfAuxSplitPath
} catch {
    $badReleaseBlockerRollupPdfAuxSplitFailedAsExpected = $true
}

if (-not $badReleaseBlockerRollupPdfAuxSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release blocker rollup with PDF visual auxiliary evidence split outside the release-candidate Source Report Contracts block."
}

$badReleaseBlockerRollupPdfAuxWrongSchemaDir = Join-Path $failDir "release-blocker-rollup-pdf-aux-wrong-schema"
$badReleaseBlockerRollupPdfAuxWrongSchemaPath = Join-Path $badReleaseBlockerRollupPdfAuxWrongSchemaDir "release_blocker_rollup.md"
New-Item -ItemType Directory -Path $badReleaseBlockerRollupPdfAuxWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBlockerRollupPdfAuxWrongSchemaPath -Encoding UTF8 -Value @"
# Release Blocker Rollup Report

## Source Report Contracts

- ``featherdoc.pdf_visual_segmented_gate_summary.v1``: status=``pass`` ready=``True`` path=``.\output\pdf-visual-release-gate-current\report\segmented-summary.json``
  - pdf_visual_gate_status: ``loaded``
  - full_visual_gate_status: ``pass``
  - pdf_visual_gate_verdict: ``pass``
  - pdf_visual_gate_finalizable: ``True``
  - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
  - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
  - pdf_visual_gate_cjk_manifest_count: ``43``
  - pdf_visual_gate_cjk_copy_search_count: ``43``
  - pdf_visual_gate_cjk_missing_text_count: ``0``
  - pdf_visual_gate_visual_baseline_manifest_count: ``42``
  - pdf_visual_gate_visual_baseline_count: ``44``
  - pdf_bounded_ctest_summary_count: ``7``
  - pdf_bounded_ctest_pass_count: ``7``
  - pdf_bounded_ctest_skipped_test_count: ``0``
  - pdf_bounded_ctest_selected_test_count: ``70``
  - pdf_bounded_ctest_subsets: ``smoke-import, regression-business-samples``
  - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-regression-business-samples-current\summary.json``
  - pdf_visual_gate_attempt_status: ``partial``
  - pdf_visual_gate_attempt_verdict: ``not_complete``
  - pdf_visual_gate_attempt_full_visual_gate_status: ``not_complete``
  - pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``
  - pdf_visual_gate_attempt_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\attempt-summary.json``
  - pdf_visual_gate_attempt_stage_count: ``6``
  - pdf_visual_gate_attempt_passed_stage_count: ``4``
  - pdf_visual_gate_attempt_failed_stage_count: ``0``
  - pdf_visual_gate_attempt_incomplete_stage_count: ``2``
  - pdf_visual_gate_attempt_outer_guard_status: ``timed_out``
  - pdf_visual_gate_attempt_outer_guard_timed_out: ``True``
  - pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``60``
  - pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``91``
  - pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``0``
  - pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``
  - pdf_visual_gate_attempt_cjk_copy_search_count: ``43``
  - pdf_visual_gate_attempt_cjk_copy_search_missing_text_count: ``0``
  - pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``
  - pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``22``
  - pdf_visual_gate_attempt_expected_visual_render_count: ``44``
  - pdf_visual_gate_attempt_visual_baseline_missing_pdf_count: ``0``
  - pdf_visual_gate_attempt_visual_baseline_pdf_total_bytes: ``7340032``
  - pdf_visual_gate_attempt_visual_baseline_png_page_count: ``44``
  - pdf_visual_gate_attempt_visual_baseline_missing_png_page_count: ``0``
  - pdf_visual_gate_attempt_visual_baseline_png_total_bytes: ``2097152``
  - pdf_visual_gate_attempt_visual_baseline_unreadable_png_dimension_count: ``0``
  - pdf_visual_gate_attempt_aggregate_contact_sheet_status: ``stale``
  - pdf_visual_gate_attempt_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
  - pdf_visual_segmented_gate_status: ``pass``
  - pdf_visual_segmented_gate_verdict: ``pass``
  - pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``
  - pdf_visual_segmented_gate_evidence_scope: ``segmented_visual_gate_auxiliary_only``
  - pdf_visual_segmented_gate_boundary: ``segmented_summary_does_not_replace_full_visual_gate_verdict``
  - pdf_visual_segmented_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\segmented-summary.json``
  - pdf_visual_segmented_gate_slice_summary_count: ``4``
  - pdf_visual_segmented_gate_slice_pass_count: ``4``
  - pdf_visual_segmented_gate_slice_failed_count: ``0``
  - pdf_visual_segmented_gate_covered_baseline_count: ``44``
  - pdf_visual_segmented_gate_expected_visual_render_count: ``44``
  - pdf_visual_segmented_gate_attempt_stage_count: ``6``
  - pdf_visual_segmented_gate_attempt_passed_stage_count: ``6``
  - pdf_visual_segmented_gate_visual_baseline_render_status: ``pass``
  - pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``pass``
  - pdf_visual_segmented_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
  - pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``1822428``
  - pdf_visual_segmented_gate_aggregate_rebuild_status: ``pass``
  - pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count: ``44``
"@

$badReleaseBlockerRollupPdfAuxWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBlockerRollupPdfAuxWrongSchemaPath
} catch {
    $badReleaseBlockerRollupPdfAuxWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseBlockerRollupPdfAuxWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release blocker rollup with PDF visual auxiliary evidence under a non-release-candidate Source Report Contracts schema."
}

$passReleaseBlockerRollupManifestSignoffTraceDir = Join-Path $passDir "release-blocker-rollup-manifest-signoff-trace"
$passReleaseBlockerRollupManifestSignoffTracePath = Join-Path $passReleaseBlockerRollupManifestSignoffTraceDir "release_blocker_rollup.md"
New-Item -ItemType Directory -Path $passReleaseBlockerRollupManifestSignoffTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseBlockerRollupManifestSignoffTracePath -Encoding UTF8 -Value @"
# Release Blocker Rollup Report

## Source Report Contracts

- ``featherdoc.release_candidate_summary``: status=``ready`` ready=``True`` path=``.\output\release-candidate-checks\summary.json``
  - manifest_signoff_entrypoints_status: ``declared``
  - manifest_signoff_entrypoints_release_assets_manifest_display: ``.\output\release-assets\v<version>\release_assets_manifest.json``
  - manifest_signoff_entrypoints_required_entrypoint_count: ``3``
  - manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
  - manifest_signoff_entrypoints_required_contracts: ``project_template_delivery_readiness_contract, project_template_onboarding_governance_contract``
  - manifest_signoff_entrypoints_required_fields: ``status, release_ready, release_blocker_count, warning_count, schema_approval_status_summary, onboarding_governance_next_action, onboarding_governance_next_action_summary, onboarding_governance_next_action_group_count, next_action, next_action_summary, next_action_group_count, requires_reviewer_action, reviewer_action_summary, reviewer_action_reason, reviewer_actions, business_document_type_summary, corpus_role_summary, source_report_display, source_json_display``
  - manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``
"@

& $auditScript -Path $passReleaseBlockerRollupManifestSignoffTracePath

$badReleaseBlockerRollupManifestSignoffSplitDir = Join-Path $failDir "release-blocker-rollup-manifest-signoff-split"
$badReleaseBlockerRollupManifestSignoffSplitPath = Join-Path $badReleaseBlockerRollupManifestSignoffSplitDir "release_blocker_rollup.md"
New-Item -ItemType Directory -Path $badReleaseBlockerRollupManifestSignoffSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBlockerRollupManifestSignoffSplitPath -Encoding UTF8 -Value @"
# Release Blocker Rollup Report

## Source Report Contracts

- ``featherdoc.release_candidate_summary``: status=``ready`` ready=``True`` path=``.\output\release-candidate-checks\summary.json``
  - manifest_signoff_entrypoints_status: ``declared``
  - manifest_signoff_entrypoints_release_assets_manifest_display: ``.\output\release-assets\v<version>\release_assets_manifest.json``
  - manifest_signoff_entrypoints_required_entrypoint_count: ``3``

## Detached manifest signoff notes

- manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
- manifest_signoff_entrypoints_required_contracts: ``project_template_delivery_readiness_contract, project_template_onboarding_governance_contract``
- manifest_signoff_entrypoints_required_fields: ``status, release_ready, release_blocker_count, warning_count, schema_approval_status_summary, onboarding_governance_next_action, onboarding_governance_next_action_summary, onboarding_governance_next_action_group_count, next_action, next_action_summary, next_action_group_count, requires_reviewer_action, reviewer_action_summary, reviewer_action_reason, reviewer_actions, business_document_type_summary, corpus_role_summary, source_report_display, source_json_display``
- manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``
"@

$badReleaseBlockerRollupManifestSignoffSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBlockerRollupManifestSignoffSplitPath
} catch {
    $badReleaseBlockerRollupManifestSignoffSplitFailedAsExpected = $true
}

if (-not $badReleaseBlockerRollupManifestSignoffSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release blocker rollup with manifest signoff entrypoints split outside the release-candidate Source Report Contracts block."
}

$badReleaseBlockerRollupManifestSignoffWrongSchemaDir = Join-Path $failDir "release-blocker-rollup-manifest-signoff-wrong-schema"
$badReleaseBlockerRollupManifestSignoffWrongSchemaPath = Join-Path $badReleaseBlockerRollupManifestSignoffWrongSchemaDir "release_blocker_rollup.md"
New-Item -ItemType Directory -Path $badReleaseBlockerRollupManifestSignoffWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBlockerRollupManifestSignoffWrongSchemaPath -Encoding UTF8 -Value @"
# Release Blocker Rollup Report

## Source Report Contracts

- ``featherdoc.release_blocker_rollup_report.v1``: status=``ready`` ready=``True`` path=``.\output\release-blocker-rollup\summary.json``
  - manifest_signoff_entrypoints_status: ``declared``
  - manifest_signoff_entrypoints_release_assets_manifest_display: ``.\output\release-assets\v<version>\release_assets_manifest.json``
  - manifest_signoff_entrypoints_required_entrypoint_count: ``3``
  - manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
  - manifest_signoff_entrypoints_required_contracts: ``project_template_delivery_readiness_contract, project_template_onboarding_governance_contract``
  - manifest_signoff_entrypoints_required_fields: ``status, release_ready, release_blocker_count, warning_count, schema_approval_status_summary, onboarding_governance_next_action, onboarding_governance_next_action_summary, onboarding_governance_next_action_group_count, next_action, next_action_summary, next_action_group_count, requires_reviewer_action, reviewer_action_summary, reviewer_action_reason, reviewer_actions, business_document_type_summary, corpus_role_summary, source_report_display, source_json_display``
  - manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``
"@

$badReleaseBlockerRollupManifestSignoffWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBlockerRollupManifestSignoffWrongSchemaPath
} catch {
    $badReleaseBlockerRollupManifestSignoffWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseBlockerRollupManifestSignoffWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release blocker rollup with manifest signoff entrypoints under a non-release-candidate Source Report Contracts schema."
}

$passReleaseBlockerRollupProjectTemplateChecklistTraceDir = Join-Path $passDir "release-blocker-rollup-project-template-checklist-trace"
$passReleaseBlockerRollupProjectTemplateChecklistTracePath = Join-Path $passReleaseBlockerRollupProjectTemplateChecklistTraceDir "release_blocker_rollup.md"
New-Item -ItemType Directory -Path $passReleaseBlockerRollupProjectTemplateChecklistTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseBlockerRollupProjectTemplateChecklistTracePath -Encoding UTF8 -Value @"
# Release Blocker Rollup Report

## Source Report Contracts

- ``featherdoc.release_candidate_summary``: status=``ready`` ready=``True`` path=``.\output\release-candidate-checks\summary.json``
  - project_template_readiness_checklist_entrypoints_status: ``declared``
  - project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``
  - project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
  - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``
  - project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
  - project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
  - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``
  - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``
  - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``
  - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``
  - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``
  - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``
  - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``
  - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
  - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
  - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``
"@

& $auditScript -Path $passReleaseBlockerRollupProjectTemplateChecklistTracePath

$badReleaseBlockerRollupProjectTemplateChecklistSplitDir = Join-Path $failDir "release-blocker-rollup-project-template-checklist-split"
$badReleaseBlockerRollupProjectTemplateChecklistSplitPath = Join-Path $badReleaseBlockerRollupProjectTemplateChecklistSplitDir "release_blocker_rollup.md"
New-Item -ItemType Directory -Path $badReleaseBlockerRollupProjectTemplateChecklistSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBlockerRollupProjectTemplateChecklistSplitPath -Encoding UTF8 -Value @"
# Release Blocker Rollup Report

## Source Report Contracts

- ``featherdoc.release_candidate_summary``: status=``ready`` ready=``True`` path=``.\output\release-candidate-checks\summary.json``
  - project_template_readiness_checklist_entrypoints_status: ``declared``
  - project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``
  - project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
  - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``
  - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``
  - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``
  - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``
  - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
  - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``

## Detached project-template checklist notes

- project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
- project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
- release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``
- release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``
- release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``
- release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``
- release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``
"@

$badReleaseBlockerRollupProjectTemplateChecklistSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBlockerRollupProjectTemplateChecklistSplitPath
} catch {
    $badReleaseBlockerRollupProjectTemplateChecklistSplitFailedAsExpected = $true
}

if (-not $badReleaseBlockerRollupProjectTemplateChecklistSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release blocker rollup with project-template checklist and packaged audit evidence split outside the release-candidate Source Report Contracts block."
}

$badReleaseBlockerRollupProjectTemplateChecklistWrongSchemaDir = Join-Path $failDir "release-blocker-rollup-project-template-checklist-wrong-schema"
$badReleaseBlockerRollupProjectTemplateChecklistWrongSchemaPath = Join-Path $badReleaseBlockerRollupProjectTemplateChecklistWrongSchemaDir "release_blocker_rollup.md"
New-Item -ItemType Directory -Path $badReleaseBlockerRollupProjectTemplateChecklistWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBlockerRollupProjectTemplateChecklistWrongSchemaPath -Encoding UTF8 -Value @"
# Release Blocker Rollup Report

## Source Report Contracts

- ``featherdoc.release_blocker_rollup_report.v1``: status=``ready`` ready=``True`` path=``.\output\release-blocker-rollup\summary.json``
  - project_template_readiness_checklist_entrypoints_status: ``declared``
  - project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``
  - project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
  - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``
  - project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
  - project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
  - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``
  - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``
  - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``
  - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``
  - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``
  - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``
  - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``
  - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
  - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
  - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``
"@

$badReleaseBlockerRollupProjectTemplateChecklistWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBlockerRollupProjectTemplateChecklistWrongSchemaPath
} catch {
    $badReleaseBlockerRollupProjectTemplateChecklistWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseBlockerRollupProjectTemplateChecklistWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release blocker rollup with project-template checklist and packaged audit evidence under a non-release-candidate Source Report Contracts schema."
}
