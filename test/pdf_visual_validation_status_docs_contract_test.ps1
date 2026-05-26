param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Get-RepoFileText {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected contract file was not found: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

$statusDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\pdf_visual_validation_status_zh.rst"
$buildingPdfDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "BUILDING_PDF.md"
$releaseChecklistDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\pdf_release_readiness_checklist_zh.rst"
$pdfImportScopeDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\pdf_import_scope.rst"
$dependencyInputsScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\check_pdf_dependency_inputs.ps1"
$preflightScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\check_pdf_visual_release_gate_preflight.ps1"
$releaseReadinessScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\check_pdf_release_readiness.ps1"
$visualFullGateGuardedScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\run_pdf_visual_full_gate_guarded.ps1"
$fullCtestGuardedScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\run_pdf_full_ctest_guarded.ps1"
$governanceReportScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_pdf_visual_release_gate_preflight_governance_report.ps1"
$materialSafetyScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\assert_release_material_safety.ps1"
$materialSafetyTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\assert_release_material_safety_test.ps1"
$packageAssetsScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\package_release_assets.ps1"
$packageAssetsSafetyTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\package_release_assets_safety_test.ps1"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
$doNotRunFullVisualGateMarker = [string]::Concat(@(
    [char]0x4E0D,
    [char]0x8981,
    [char]0x76F4,
    [char]0x63A5,
    [char]0x542F,
    [char]0x52A8,
    [char]0x5B8C,
    [char]0x6574,
    " visual gate"
))

$statusMarkers = @(
    "check_pdf_dependency_inputs.ps1",
    "featherdoc.pdf_dependency_inputs_check.v1",
    "blocking_checks = []",
    "blocking_summary.blocking_check_count = 0",
    "memory_guard_blocked = false",
    "memory_guard_skipped = false",
    "workstation_free_memory_available",
    "git status --short",
    "free_memory_mb",
    "min_free_memory_mb",
    "memory guard blocked=false",
    "memory guard skipped=false",
    "free memory MB",
    "minimum free memory MB",
    "2026-05-20",
    "2026-05-24",
    "2026-05-25",
    "2026-05-26",
    "2026-05-27",
    "2026-05-26T22:50:00",
    "FinalizeOnly",
    "release_blocker_count = 0",
    "action_item_count = 0",
    "build_dir_source = requested",
    "build_dir_auto_candidates",
    "pdf_build_options",
    "looks_reusable",
    "preflight_ready = true",
    "preflight governance report",
    "raw summary",
    "full_visual_gate_required = true",
    "full_visual_gate_status = not_run_by_preflight_governance",
    "preflight-governance",
    "full gate summary",
    "contact sheet",
    "evidence_kind",
    "synthetic_fixture",
    "synthetic_preflight_evidence",
    "release_ready = true",
    "not_run_by_preflight_governance",
    "evidence_kind = real_build",
    "controlled_visual_smoke_status = pass",
    "controlled_visual_smoke_passed = true",
    "status = ready",
    "status = pass",
    "verdict = pass",
    "finalize_only = true",
    "skip_preflight = true",
    "output_gap_count = 0",
    "missing_output_count = 0",
    "visual_baseline_manifest_count = 42",
    "baselines_count = 44",
    "cjk_manifest_count = 43",
    "cjk_copy_search_count = 43",
    "generated_at = 2026-05-25T07:13:30",
    "generated_at = 2026-05-25T07:15:13",
    "generated_at = 2026-05-26T23:19:01",
    "generated_at = 2026-05-27T01:38:37",
    "output/pdf-visual-release-gate-preflight-current/summary.json",
    "pdf_preflight_default_current_summary_trace",
    "blocking_check_count = 0",
    "blocking_checks = []",
    "free_memory_mb = 2691.4",
    "-FinalizeOnly -SkipPreflight",
    "selected_pdfium_provider = prebuilt",
    "pdf_dependency_inputs_status = ready",
    "pdf_dependency_missing_input_count = 0",
    "pdf_build_options_enabled = true",
    "pdfio_dependency_ready = true",
    "pdfium_dependency_ready = true",
    "visual_baseline_sample_count = 42",
    "missing_visual_baseline_pdf_count = 0",
    "cjk_text_layer_sample_count = 43",
    "missing_cjk_text_layer_pdf_count = 0",
    "ctest_list_contains_pdf_gate_tests = pass",
    "run_pdf_ctest_bounded_subset.ps1",
    "pdf_bounded_ctest",
    "pdf_bounded_ctest_summary_count",
    "pdf_bounded_ctest_pass_count",
    "pdf_bounded_ctest_skipped_test_count",
    "pdf_bounded_ctest_selected_test_count",
    "pdf_bounded_ctest_subsets",
    "pdf_bounded_ctest_summary_json_display",
    "pdf_bounded_ctest_governance_trace",
    "pdf_bounded_ctest_source_report_block_trace",
    "pdf_visual_gate_rollup_material_safety_trace",
    "write_pdf_visual_gate_attempt_summary.ps1",
    "featherdoc.pdf_visual_gate_attempt_summary.v1",
    "attempt-summary.json",
    "pdf_visual_gate_attempt_status",
    "pdf_visual_gate_attempt_verdict",
    "pdf_visual_gate_attempt_full_visual_gate_status",
    "pdf_visual_gate_attempt_evidence_scope",
    "bounded_attempt_auxiliary_only",
    "pdf_visual_gate_attempt_outer_guard_status",
    "pdf_visual_gate_attempt_outer_guard_timed_out",
    "pdf_visual_gate_attempt_outer_guard_timeout_seconds",
    "outer_guard_status = timed_out",
    "outer_guard_timed_out = true",
    "outer_guard_timeout_seconds = 60",
    "pdf_visual_gate_attempt_outer_guard_trace",
    "pdf_visual_gate_attempt_pdf_regression_skipped_test_count",
    "pdf_visual_gate_attempt_visual_baseline_render_status",
    "pdf_visual_gate_attempt_summary_trace",
    "pdf_visual_gate_attempt_governance_trace",
    "pdf_visual_gate_attempt_material_safety_trace",
    "pdf_visual_gate_attempt_rollup_material_safety_trace",
    "pdf_visual_gate_attempt_final_review_material_safety_trace",
    "run_pdf_visual_full_gate_guarded.ps1",
    "featherdoc.pdf_visual_full_gate_guarded_summary.v1",
    "visual_full_gate_attempt_passed_stage_count",
    "visual_full_gate_attempt_visual_baseline_fresh_rendered_count",
    "visual_full_gate_attempt_aggregate_contact_sheet_status",
    "pdf_visual_full_gate_guarded_summary_trace",
    "featherdoc.pdf_visual_baseline_slice.v1",
    "VisualBaselineSliceOnly",
    "VisualBaselineOffset",
    "VisualBaselineLimit",
    "visual_baseline_slice_only",
    "slice_summary_does_not_replace_full_visual_gate_verdict",
    "pdf_visual_baseline_slice_summary_trace",
    "featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1",
    "RebuildAggregateContactSheetOnly",
    "aggregate_contact_sheet_rebuild_only",
    "aggregate_rebuild_summary_does_not_replace_full_visual_gate_verdict",
    "aggregate-contact-sheet-rebuild-summary.json",
    "pdf_visual_aggregate_contact_sheet_rebuild_trace",
    "write_pdf_visual_segmented_gate_summary.ps1",
    "featherdoc.pdf_visual_segmented_gate_summary.v1",
    "segmented-summary.json",
    "status",
    "verdict",
    "pdf_visual_segmented_gate_status",
    "pdf_visual_segmented_gate_verdict",
    "pdf_visual_segmented_gate_full_visual_gate_status",
    "pdf_visual_segmented_gate_evidence_scope",
    "segmented_visual_gate_auxiliary_only",
    "pdf_visual_segmented_gate_boundary",
    "segmented_summary_does_not_replace_full_visual_gate_verdict",
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
    "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count",
    "pdf_visual_segmented_gate_summary_trace",
    "pdf_visual_segmented_gate_governance_trace",
    "pdf_visual_segmented_gate_material_safety_trace",
    "pdf_visual_segmented_gate_rollup_material_safety_trace",
    "pdf_visual_segmented_gate_final_review_material_safety_trace",
    "pdf_ctest_bounded_subset_release_trace",
    "pdf_ctest_bounded_contract_static_release_trace",
    "pdf_ctest_bounded_cjk_flow_static_release_trace",
    "pdf_ctest_bounded_regression_basic_text_release_trace",
    "pdf_ctest_bounded_regression_styled_document_release_trace",
    "pdf_ctest_bounded_regression_business_samples_release_trace",
    "pdf_ctest_bounded_regression_table_layout_release_trace",
    "Subset",
    "subset = smoke-import",
    "subset = contract-static",
    "subset = cjk-flow-static",
    "subset = regression-basic-text",
    "subset = regression-styled-document",
    "subset = regression-business-samples",
    "subset = regression-table-layout",
    "contract-static",
    "cjk-flow-static",
    "regression-basic-text",
    "regression-styled-document",
    "regression-business-samples",
    "regression-table-layout",
    "selected_test_count = 10",
    "skipped_test_count = 0",
    "ctest_timeout_seconds = 60",
    "pdf_cjk_font_search_density_flow_contract",
    "pdf_cjk_list_page_flow_contract",
    "pdf_regression_manifest",
    "pdf_regression_four-page-text",
    "pdf_regression_styled-text",
    "pdf_regression_document-font-matrix-text",
    "pdf_regression_invoice-grid-text",
    "pdf_regression_document-cjk-image-wrap-stress-text",
    "pdf_regression_document-table-merged-header-footer-variants-text",
    "skipped",
    "***Skipped",
    "912x14566",
    "1822428",
    "pdfium_ready = true",
    "pdfium_prebuilt_root_exists = true",
    "TinaToolBox\dependencies\pdfium-win-x64",
    "lib\pdfium.dll.lib",
    "include\fpdfview.h",
    "bin\pdfium.dll",
    "missing_input_count = 1",
    "tmp\pdfio-src\pdfio.h",
    "output_gap_count = 3",
    ".bpdf-roundtrip-msvc",
    "CMakeCache.txt",
    "CTestTestfile.cmake",
    "FEATHERDOC_BUILD_PDF=ON",
    "FEATHERDOC_BUILD_PDF_IMPORT=ON",
    "pdf_build_options_enabled",
    "out\build",
    "-MinFreeMemoryMB",
    "-SkipMemoryGuard",
    "missing_output_count = 87",
    "aggregate-contact-sheet.png",
    "fake-pdf-build",
    "fake ctest",
    "fake python",
    "fake / synthetic",
    "test fixture",
    "reusable release build substitute",
    "pdf_dependency_inputs",
    "pdf_dependency_inputs_status",
    "pdf_dependency_missing_input_count",
    "pdfio_dependency_ready",
    "pdfium_dependency_ready",
    "run_pdf_visual_release_gate.ps1"
)

foreach ($marker in $statusMarkers) {
    Assert-ContainsText -Text $statusDoc -ExpectedText $marker `
        -Message "PDF visual validation status doc should preserve memory-gate and blocker status marker."
}

$buildingPdfFixtureMarkers = @(
    "check_pdf_dependency_inputs.ps1",
    "-PdfioSourceDir",
    "-PdfiumProvider",
    "PdfiumPrebuiltRoot",
    "-PdfiumPrebuiltRoot",
    "PdfiumSourceDir",
    "PdfiumRuntimeDir",
    "pdfium_prebuilt_root",
    "dependency_overrides",
    "pdfium.dll.lib",
    "selected_pdfium_provider",
    "missing_inputs",
    "tmp\pdfio-src\pdfio.h",
    "fake-pdf-build",
    "fake ctest",
    "fake python",
    "test fixture",
    "reusable release build substitute",
    "CMakeCache.txt",
    "CTestTestfile.cmake",
    "FEATHERDOC_BUILD_PDF",
    "FEATHERDOC_BUILD_PDF_IMPORT",
    "pdf_build_options_enabled",
    "release gate blockers",
    "ctest -N",
    "skipped",
    "visual baseline PDF",
    "CJK text-layer PDF",
    "90",
    "expect_visual_baseline=true",
    "expect_cjk=true",
    "pdf_release_readiness_checklist_zh",
    "check_pdf_release_readiness.ps1",
    "featherdoc.pdf_release_readiness_check.v1",
    "pass_with_warnings",
    "persisted_pdf_release_evidence_only",
    "pdf_release_readiness_machine_gate_trace",
    "pdf_full_fresh_visual_gate.not_completed_in_current_window",
    "run_pdf_visual_full_gate_guarded.ps1",
    "featherdoc.pdf_visual_full_gate_guarded_summary.v1",
    "visual_full_gate_attempt_passed_stage_count",
    "visual_full_gate_attempt_visual_baseline_fresh_rendered_count",
    "visual_full_gate_attempt_aggregate_contact_sheet_status",
    "pdf_visual_full_gate_guarded_summary_trace",
    "pdf_full_ctest.not_completed_in_current_window",
    "visual baseline manifest",
    "run_pdf_ctest_bounded_subset.ps1",
    "pdf_bounded_ctest",
    "pdf_bounded_ctest_summary_count",
    "pdf_bounded_ctest_pass_count",
    "pdf_bounded_ctest_skipped_test_count",
    "pdf_bounded_ctest_selected_test_count",
    "pdf_bounded_ctest_subsets",
    "pdf_bounded_ctest_summary_json_display",
    "pdf_bounded_ctest_governance_trace",
    "pdf_bounded_ctest_source_report_block_trace",
    "pdf_visual_gate_rollup_material_safety_trace",
    "write_pdf_visual_gate_attempt_summary.ps1",
    "featherdoc.pdf_visual_gate_attempt_summary.v1",
    "attempt-summary.json",
    "pdf_visual_gate_attempt_status",
    "pdf_visual_gate_attempt_verdict",
    "pdf_visual_gate_attempt_full_visual_gate_status",
    "pdf_visual_gate_attempt_evidence_scope",
    "bounded_attempt_auxiliary_only",
    "pdf_visual_gate_attempt_outer_guard_status",
    "pdf_visual_gate_attempt_outer_guard_timed_out",
    "pdf_visual_gate_attempt_outer_guard_timeout_seconds",
    "outer_guard_status = timed_out",
    "outer_guard_timed_out = true",
    "outer_guard_timeout_seconds = 60",
    "pdf_visual_gate_attempt_outer_guard_trace",
    "pdf_visual_gate_attempt_pdf_regression_skipped_test_count",
    "pdf_visual_gate_attempt_visual_baseline_render_status",
    "pdf_visual_gate_attempt_summary_trace",
    "pdf_visual_gate_attempt_governance_trace",
    "pdf_visual_gate_attempt_material_safety_trace",
    "pdf_visual_gate_attempt_rollup_material_safety_trace",
    "pdf_visual_gate_attempt_final_review_material_safety_trace",
    "featherdoc.pdf_visual_baseline_slice.v1",
    "VisualBaselineSliceOnly",
    "VisualBaselineOffset",
    "VisualBaselineLimit",
    "visual_baseline_slice_only",
    "slice_summary_does_not_replace_full_visual_gate_verdict",
    "pdf_visual_baseline_slice_summary_trace",
    "featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1",
    "RebuildAggregateContactSheetOnly",
    "aggregate_contact_sheet_rebuild_only",
    "aggregate_rebuild_summary_does_not_replace_full_visual_gate_verdict",
    "aggregate-contact-sheet-rebuild-summary.json",
    "pdf_visual_aggregate_contact_sheet_rebuild_trace",
    "write_pdf_visual_segmented_gate_summary.ps1",
    "featherdoc.pdf_visual_segmented_gate_summary.v1",
    "segmented-summary.json",
    "pdf_visual_segmented_gate_status",
    "pdf_visual_segmented_gate_verdict",
    "pdf_visual_segmented_gate_full_visual_gate_status",
    "pdf_visual_segmented_gate_evidence_scope",
    "segmented_visual_gate_auxiliary_only",
    "pdf_visual_segmented_gate_boundary",
    "segmented_summary_does_not_replace_full_visual_gate_verdict",
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
    "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count",
    "pdf_visual_segmented_gate_summary_trace",
    "pdf_visual_segmented_gate_governance_trace",
    "pdf_visual_segmented_gate_material_safety_trace",
    "pdf_visual_segmented_gate_rollup_material_safety_trace",
    "pdf_visual_segmented_gate_final_review_material_safety_trace",
    "-Subset smoke-import",
    "-Subset contract-static",
    "-Subset cjk-flow-static",
    "-Subset regression-basic-text",
    "-Subset regression-styled-document",
    "-Subset regression-business-samples",
    "-Subset regression-table-layout",
    "contract-static",
    "cjk-flow-static",
    "regression-basic-text",
    "regression-styled-document",
    "regression-business-samples",
    "regression-table-layout",
    "pdf_ctest_bounded_cjk_flow_static_release_trace",
    "pdf_ctest_bounded_regression_basic_text_release_trace",
    "pdf_ctest_bounded_regression_styled_document_release_trace",
    "pdf_ctest_bounded_regression_business_samples_release_trace",
    "pdf_ctest_bounded_regression_table_layout_release_trace",
    "pdf_cjk_font_search_density_flow_contract",
    "pdf_cjk_list_page_flow_contract",
    "pdf_regression_manifest",
    "pdf_regression_four-page-text",
    "pdf_regression_styled-text",
    "pdf_regression_document-font-matrix-text",
    "pdf_regression_invoice-grid-text",
    "pdf_regression_document-cjk-image-wrap-stress-text",
    "pdf_regression_document-table-merged-header-footer-variants-text",
    "skipped",
    "skipped_test_count = 0",
    "***Skipped",
    "selected_test_count = 10",
    "ctest_timeout_seconds = 60"
)

foreach ($marker in $buildingPdfFixtureMarkers) {
    Assert-ContainsText -Text $buildingPdfDoc -ExpectedText $marker `
        -Message "BUILDING_PDF.md should preserve the PDF preflight fixture boundary marker."
}

$pdfExportSupportMatrixMarkers = @(
    "support matrix",
    "text-layer",
    "cant split",
    "Word",
    "HarfBuzz shaped run",
    "CJK fallback",
    "synthetic bold / italic",
    "43",
    "CJK text-layer",
    "RTL / Bidi",
    "LTR glyph-id stream",
    "manifest ID",
    "contract-cjk-style",
    "document-contract-cjk-style",
    "invoice-grid-text",
    "document-invoice-table-text",
    "image-report-text",
    "cjk-image-report-text",
    "document-cjk-image-wrap-stress-text",
    "long-report-text",
    "document-long-flow-text",
    "sectioned-report-text",
    "header-footer-text",
    "document-cjk-table-wrap-page-flow-text",
    "pdf_real_business_sample_release_entry_trace",
    "release checklist"
)

foreach ($marker in $pdfExportSupportMatrixMarkers) {
    Assert-ContainsText -Text $buildingPdfDoc -ExpectedText $marker `
        -Message "BUILDING_PDF.md should preserve PDF export support matrix marker '$marker'."
}

$pdfImportBoundaryMarkers = @(
    "The importer is text-first.",
    "extractable PDF text and character geometry",
    "It is not a general PDF-to-Word converter",
    "does not",
    "promise arbitrary visual fidelity",
    "Paragraph import from extractable PDF text.",
    "Conservative table-candidate detection",
    "Opt-in table promotion through",
    "--import-table-candidates-as-tables",
    "Table candidates are rejected by default.",
    "Ordinary two-column prose, numbered lists, short-label prose, and free-form",
    "forms should remain paragraphs rather than becoming tables",
    "does not support scanned PDFs, OCR, image-only",
    "arbitrary nested table semantics",
    "rotated or floating content recovery",
    "exact visual reconstruction of an",
    "Text extraction is the primary contract",
    "layout reconstruction is best-effort",
    "Table import is opt-in through",
    "Unsupported cases must fail or remain paragraphs",
    "OCR, scanned pages, and visual-perfect recreation remain out of scope."
)

foreach ($marker in $pdfImportBoundaryMarkers) {
    Assert-ContainsText -Text $pdfImportScopeDoc -ExpectedText $marker `
        -Message "docs/pdf_import_scope.rst should preserve PDF import boundary marker '$marker'."
}

$pdfPackageManifestMarkers = @(
    "release_assets_manifest.json",
    'pdf_visual_gate_evidence_included = $hasPdfVisualGateEvidence',
    'pdf_visual_gate_evidence = $pdfVisualGateManifestEvidence',
    "Release evidence ZIP:"
)

foreach ($marker in $pdfPackageManifestMarkers) {
    Assert-ContainsText -Text $packageAssetsScript -ExpectedText $marker `
        -Message "package_release_assets.ps1 should preserve PDF visual gate package marker '$marker'."
}

$pdfPackageSafetyMarkers = @(
    "release_assets_manifest.json did not record PDF visual gate evidence as included.",
    "release_assets_manifest.json did not preserve the PDF visual gate summary display path.",
    "release_assets_manifest.json lost the PDF visual gate verdict.",
    "release_assets_manifest.json lost the full PDF visual gate status.",
    "release_assets_manifest.json lost the PDF visual gate aggregate contact sheet.",
    "release_assets_manifest.json lost the PDF CJK manifest sample count.",
    "release_assets_manifest.json lost the PDF CJK copy/search sample count.",
    "release_assets_manifest.json lost the PDF CJK missing text count.",
    "release_assets_manifest.json lost the PDF visual baseline count.",
    "release_assets_manifest.json lost the PDF visual baseline manifest sample count.",
    "Release evidence ZIP did not include expected PDF visual gate entry"
)

foreach ($marker in $pdfPackageSafetyMarkers) {
    Assert-ContainsText -Text $packageAssetsSafetyTest -ExpectedText $marker `
        -Message "package_release_assets_safety_test.ps1 should preserve PDF package safety marker '$marker'."
}

foreach ($marker in @(
    "check_pdf_visual_release_gate_preflight.ps1",
    "-OutputJson .\output\pdf-visual-release-gate-preflight\summary.json",
    "blocking_checks = []",
    "blocking_summary.blocking_check_count = 0",
    "preflight_ready = true",
    "status = ready",
    "evidence_kind = real_build",
    "output_gap_count = 0",
    "missing_output_count = 0",
    "pdf_dependency_inputs_status = ready",
    "pdf_build_options_enabled = true",
    "pdfio_dependency_ready = true",
    "pdfium_dependency_ready = true",
    "selected_pdfium_provider = prebuilt",
    "ctest_list_contains_pdf_gate_tests = pass",
    "status = pass",
    "verdict = pass",
    "finalize_only = true",
    "skip_preflight = true",
    "visual_baseline_manifest_count = 42",
    "baselines_count = 44",
    "cjk_manifest_count = 43",
    "cjk_copy_search_count = 43",
    "run_release_candidate_checks.ps1",
    "run_pdf_visual_release_gate.ps1",
    "-OutputDir .\output\pdf-visual-release-gate-current",
    "-FinalizeOnly",
    "-SkipPreflight",
    "full_visual_gate_status",
    "full gate summary verdict",
    "line_scoped_final_review_pdf_visual_trace",
    "line_scoped_final_review_pdf_visual_verdict",
    "block_scoped_final_review_pdf_visual_step_status",
    "section_scoped_final_review_pdf_visual_key_outputs",
    "line_scoped_release_handoff_pdf_visual_trace",
    "line_scoped_release_handoff_pdf_visual_verdict",
    "block_scoped_release_handoff_pdf_visual_evidence",
    "line_scoped_release_note_pdf_visual_trace",
    "line_scoped_release_note_pdf_visual_verdict",
    "line_scoped_release_bundle_pdf_visual_summary",
    "block_scoped_release_body_pdf_visual_evidence",
    "block_scoped_entry_pdf_visual_evidence",
    "PDF visual gate aggregate contact sheet:",
    "line_scoped_reviewer_checklist_pdf_visual_evidence",
    "block_scoped_pdf_visual_gate_handoff_trace",
    "manifest_scoped_pdf_visual_gate_count_trace",
    "check_pdf_release_readiness.ps1",
    "featherdoc.pdf_release_readiness_check.v1",
    "pass_with_warnings",
    "persisted_pdf_release_evidence_only",
    "pdf_release_readiness_machine_gate_trace",
    "pdf_full_fresh_visual_gate.not_completed_in_current_window",
    "run_pdf_visual_full_gate_guarded.ps1",
    "featherdoc.pdf_visual_full_gate_guarded_summary.v1",
    "visual_full_gate_attempt_passed_stage_count",
    "visual_full_gate_attempt_visual_baseline_fresh_rendered_count",
    "visual_full_gate_attempt_aggregate_contact_sheet_status",
    "pdf_visual_full_gate_guarded_summary_trace",
    "pdf_full_ctest.not_completed_in_current_window",
    "release_entry_pdf_readiness_checklist_trace",
    "pdf_real_business_sample_release_entry_trace",
    "docs/pdf_release_readiness_checklist_zh.rst",
    "pdf_visual_gate_evidence",
    "pdf_bounded_ctest_evidence",
    "manifest_signoff_entrypoints",
    "not_run_by_preflight_governance",
    "contact sheet",
    "aggregate-contact-sheet.png",
    "912x14566",
    "1822428",
    "pdf_visual_validation_status_docs_contract_test.ps1",
    "release_candidate_visual_verdict_test.ps1",
    "release_note_bundle_visual_verdict_metadata_test.ps1",
    "pdf_real_business_sample_manifest_contract_test.ps1",
    "run_pdf_ctest_bounded_subset.ps1",
    "pdf_bounded_ctest_summary_count",
    "pdf_bounded_ctest_pass_count",
    "pdf_bounded_ctest_skipped_test_count",
    "pdf_bounded_ctest_selected_test_count",
    "pdf_bounded_ctest_subsets",
    "pdf_bounded_ctest_summary_json_display",
    "pdf_bounded_ctest_governance_trace",
    "pdf_bounded_ctest_source_report_block_trace",
    "pdf_visual_gate_rollup_material_safety_trace",
    "write_pdf_visual_gate_attempt_summary.ps1",
    "attempt-summary.json",
    "pdf_visual_gate_attempt_status",
    "pdf_visual_gate_attempt_verdict",
    "pdf_visual_gate_attempt_full_visual_gate_status",
    "pdf_visual_gate_attempt_evidence_scope",
    "bounded_attempt_auxiliary_only",
    "pdf_visual_gate_attempt_outer_guard_status",
    "pdf_visual_gate_attempt_outer_guard_timed_out",
    "pdf_visual_gate_attempt_outer_guard_timeout_seconds",
    "outer_guard_status = timed_out",
    "outer_guard_timed_out = true",
    "outer_guard_timeout_seconds = 60",
    "pdf_visual_gate_attempt_outer_guard_trace",
    "pdf_visual_gate_attempt_pdf_regression_skipped_test_count",
    "pdf_visual_gate_attempt_visual_baseline_render_status",
    "pdf_visual_gate_attempt_summary_trace",
    "pdf_visual_gate_attempt_governance_trace",
    "pdf_visual_gate_attempt_material_safety_trace",
    "pdf_visual_gate_attempt_rollup_material_safety_trace",
    "pdf_visual_gate_attempt_final_review_material_safety_trace",
    "featherdoc.pdf_visual_baseline_slice.v1",
    "VisualBaselineSliceOnly",
    "VisualBaselineOffset",
    "VisualBaselineLimit",
    "visual_baseline_slice_only",
    "slice_summary_does_not_replace_full_visual_gate_verdict",
    "pdf_visual_baseline_slice_summary_trace",
    "featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1",
    "RebuildAggregateContactSheetOnly",
    "aggregate_contact_sheet_rebuild_only",
    "aggregate_rebuild_summary_does_not_replace_full_visual_gate_verdict",
    "aggregate-contact-sheet-rebuild-summary.json",
    "pdf_visual_aggregate_contact_sheet_rebuild_trace",
    "write_pdf_visual_segmented_gate_summary.ps1",
    "featherdoc.pdf_visual_segmented_gate_summary.v1",
    "segmented-summary.json",
    "pdf_visual_segmented_gate_status",
    "pdf_visual_segmented_gate_verdict",
    "pdf_visual_segmented_gate_full_visual_gate_status",
    "pdf_visual_segmented_gate_evidence_scope",
    "segmented_visual_gate_auxiliary_only",
    "pdf_visual_segmented_gate_boundary",
    "segmented_summary_does_not_replace_full_visual_gate_verdict",
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
    "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count",
    "pdf_visual_segmented_gate_summary_trace",
    "pdf_visual_segmented_gate_governance_trace",
    "pdf_visual_segmented_gate_material_safety_trace",
    "pdf_visual_segmented_gate_rollup_material_safety_trace",
    "pdf_visual_segmented_gate_final_review_material_safety_trace",
    "pdf_ctest_bounded_subset_release_trace",
    "pdf_ctest_bounded_contract_static_release_trace",
    "pdf_ctest_bounded_cjk_flow_static_release_trace",
    "pdf_ctest_bounded_regression_basic_text_release_trace",
    "pdf_ctest_bounded_regression_styled_document_release_trace",
    "pdf_ctest_bounded_regression_business_samples_release_trace",
    "pdf_ctest_bounded_regression_table_layout_release_trace",
    "subset = smoke-import",
    "subset = contract-static",
    "subset = cjk-flow-static",
    "subset = regression-basic-text",
    "subset = regression-styled-document",
    "subset = regression-business-samples",
    "subset = regression-table-layout",
    "contract-static",
    "cjk-flow-static",
    "regression-basic-text",
    "regression-styled-document",
    "regression-business-samples",
    "regression-table-layout",
    "pdf_import_docs_contract",
    "pdf_cjk_anchor_font_matrix_boundary_contract",
    "pdf_cjk_font_search_density_flow_contract",
    "pdf_cjk_list_page_flow_contract",
    "pdf_regression_manifest",
    "pdf_regression_four-page-text",
    "pdf_regression_styled-text",
    "pdf_regression_document-font-matrix-text",
    "pdf_regression_invoice-grid-text",
    "pdf_regression_document-cjk-image-wrap-stress-text",
    "pdf_regression_document-table-merged-header-footer-variants-text",
    "skipped",
    "skipped_test_count = 0",
    "***Skipped",
    "selected_test_count = 10",
    "ctest_timeout_seconds = 60",
    "pdf_document_generator_probe",
    "pdf_import_table_heuristic",
    'ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_" --output-on-failure --timeout 60',
    "text-first",
    "opt-in",
    "PDF"
)) {
    Assert-ContainsText -Text $releaseChecklistDoc -ExpectedText $marker `
        -Message "PDF release readiness checklist should preserve marker '$marker'."
}

$finalReviewPdfAuxMaterialSafetyScriptMarkers = @(
    "Add-FinalReviewPdfVisualGateAttemptTraceViolations",
    "Add-FinalReviewPdfVisualSegmentedGateTraceViolations",
    'Test-MarkdownSectionListRunContainsAll -Text $Content -Heading "## Step status" -Anchor "PDF visual gate attempt:"',
    'Test-MarkdownSectionListRunContainsAll -Text $Content -Heading "## Step status" -Anchor "PDF visual segmented gate:"',
    'Test-MarkdownSectionContainsAll -Text $Content -Heading "## Key outputs"',
    "PDF visual gate attempt summary:",
    "attempt-summary.json",
    "PDF visual gate attempt contact sheet:",
    "PDF visual segmented gate summary:",
    "segmented-summary.json",
    "PDF visual segmented gate contact sheet:"
)

foreach ($marker in $finalReviewPdfAuxMaterialSafetyScriptMarkers) {
    Assert-ContainsText -Text $materialSafetyScript -ExpectedText $marker `
        -Message "Material-safety audit should preserve final_review.md PDF auxiliary evidence marker '$marker'."
}

$finalReviewPdfAuxMaterialSafetyFixtureMarkers = @(
    "final-review-pdf-visual-aux-trace",
    "final-review-pdf-visual-attempt-step-status-supplied-by-detached-notes",
    "final-review-pdf-visual-attempt-key-outputs-supplied-by-detached-notes",
    "final-review-pdf-visual-segmented-step-status-supplied-by-detached-notes",
    "final-review-pdf-visual-segmented-key-outputs-supplied-by-detached-notes",
    "unexpectedly passed final_review.md with PDF visual attempt step-status markers supplied only by detached notes",
    "unexpectedly passed final_review.md with PDF visual attempt Key outputs evidence supplied only by detached notes",
    "unexpectedly passed final_review.md with PDF visual segmented gate step-status markers supplied only by detached notes",
    "unexpectedly passed final_review.md with PDF visual segmented gate Key outputs evidence supplied only by detached notes"
)

foreach ($marker in $finalReviewPdfAuxMaterialSafetyFixtureMarkers) {
    Assert-ContainsText -Text $materialSafetyTest -ExpectedText $marker `
        -Message "Material-safety regression should preserve final_review.md PDF auxiliary fixture marker '$marker'."
}

$scriptMarkers = @(
    "workstation_free_memory_available",
    "pdf_build_options_enabled",
    "FEATHERDOC_BUILD_PDF",
    "FEATHERDOC_BUILD_PDF_IMPORT",
    "disabled_pdf_build_options",
    "missing_pdf_build_options",
    "free_memory_mb",
    "min_free_memory_mb",
    "memory_guard_blocked",
    "memory_guard_skipped",
    "output_gap_count",
    "missing_output_count"
)

$preflightDefaultOutputMarkers = @(
    '[string]$OutputJson = "output/pdf-visual-release-gate-preflight-current/summary.json"',
    'if ([string]::IsNullOrWhiteSpace($OutputJson))',
    "Set-Content -LiteralPath `$resolvedOutputJson"
)

foreach ($marker in $preflightDefaultOutputMarkers) {
    Assert-ContainsText -Text $preflightScript -ExpectedText $marker `
        -Message "PDF visual preflight script should preserve default current-summary marker '$marker'."
}

$releaseReadinessMachineGateMarkers = @(
    "featherdoc.pdf_release_readiness_check.v1",
    "persisted_pdf_release_evidence_only",
    "pass_with_warnings",
    "release_ready",
    "pdf_release_readiness_machine_gate_trace",
    "run_pdf_visual_full_gate_guarded.ps1",
    "featherdoc.pdf_visual_full_gate_guarded_summary.v1",
    "pdf_visual_full_gate_guarded_summary_trace",
    "visual_full_gate_status",
    "visual_full_gate_verdict",
    "visual_full_gate_outer_guard_status",
    "visual_full_gate_attempt_passed_stage_count",
    "visual_full_gate_attempt_visual_baseline_fresh_rendered_count",
    "visual_full_gate_attempt_aggregate_contact_sheet_status",
    "write_pdf_visual_segmented_gate_summary.ps1",
    "featherdoc.pdf_visual_segmented_gate_summary.v1",
    "pdf_visual_segmented_gate_summary_trace",
    "visual_segmented_gate_status",
    "visual_segmented_gate_verdict",
    "visual_segmented_gate_full_visual_gate_status",
    "visual_segmented_gate_evidence_scope",
    "visual_segmented_gate_covered_baseline_count",
    "visual_segmented_gate_expected_visual_render_count",
    "visual_segmented_gate_aggregate_contact_sheet_status",
    "visual_segmented_gate_aggregate_contact_sheet_bytes",
    "segmented_gate_covered_baseline_count",
    "segmented_gate_aggregate_contact_sheet_bytes",
    "run_pdf_full_ctest_guarded.ps1",
    "featherdoc.pdf_full_ctest_guarded_summary.v1",
    "pdf_full_ctest_guarded_summary_trace",
    "full_ctest_status",
    "full_ctest_verdict",
    "full_ctest_outer_guard_status",
    "full_ctest_completed_test_count",
    "full_ctest_not_run_test_count",
    "pdf_full_fresh_visual_gate.not_completed_in_current_window",
    "pdf_full_ctest.not_completed_in_current_window",
    "does not run CMake, CTest, rendering, Office, LibreOffice, browsers, or PDF generation",
    "preflight_summary_json",
    "visual_gate_summary_json",
    "aggregate_contact_sheet_bytes"
)

foreach ($marker in $releaseReadinessMachineGateMarkers) {
    Assert-ContainsText -Text $releaseReadinessScript -ExpectedText $marker `
        -Message "PDF release readiness script should preserve machine gate marker '$marker'."
}

foreach ($marker in @(
    "featherdoc.pdf_visual_full_gate_guarded_summary.v1",
    "guarded_full_pdf_visual_gate_attempt",
    "full_visual_gate_status",
    "outer_guard_status",
    "outer_guard_timed_out",
    "outer_guard_timeout_seconds",
    "attempt_summary_json",
    "attempt_passed_stage_count",
    "attempt_visual_baseline_fresh_rendered_count",
    "attempt_aggregate_contact_sheet_status",
    "guarded_full_visual_gate_attempt_does_not_replace_completed_full_visual_gate",
    "pdf_visual_full_gate_guarded_summary_trace"
)) {
    Assert-ContainsText -Text $visualFullGateGuardedScript -ExpectedText $marker `
        -Message "PDF visual full gate guarded script should preserve marker '$marker'."
}

foreach ($marker in @(
    "featherdoc.pdf_full_ctest_guarded_summary.v1",
    "guarded_full_pdf_ctest_attempt",
    "outer_guard_status",
    "outer_guard_timed_out",
    "outer_guard_timeout_seconds",
    "selected_test_count",
    "completed_test_count",
    "not_run_test_count",
    "guarded_full_ctest_attempt_does_not_replace_completed_full_ctest",
    "pdf_full_ctest_guarded_summary_trace"
)) {
    Assert-ContainsText -Text $fullCtestGuardedScript -ExpectedText $marker `
        -Message "PDF full CTest guarded script should preserve marker '$marker'."
}

foreach ($marker in $scriptMarkers) {
    foreach ($entry in @(
        [ordered]@{ name = "PDF visual preflight script"; text = $preflightScript },
        [ordered]@{ name = "PDF visual preflight governance report script"; text = $governanceReportScript }
    )) {
        Assert-ContainsText -Text $entry.text -ExpectedText $marker `
            -Message "$($entry.name) should preserve memory-gate marker '$marker'."
    }
}

foreach ($marker in @(
    "featherdoc.pdf_dependency_inputs_check.v1",
    "pdfio_source_header_exists",
    "pdfium_source_header_exists",
    "pdfium_prebuilt_inputs_exist",
    "PdfiumPrebuiltRoot",
    "pdfium_prebuilt_root",
    "selected_pdfium_provider",
    "missing_inputs"
)) {
    Assert-ContainsText -Text $dependencyInputsScript -ExpectedText $marker `
        -Message "PDF dependency input script should keep marker '$marker'."
}

$releaseBlockerHelpers = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\release_blocker_metadata_helpers.ps1"
foreach ($marker in @(
    "memory guard blocked=",
    "memory guard skipped=",
    "free memory MB=",
    "minimum free memory MB=",
    "PDF build options:",
    "pdf_build_options_enabled"
)) {
    Assert-ContainsText -Text $releaseBlockerHelpers -ExpectedText $marker `
        -Message "Release blocker helpers should keep memory-gate fields visible in PDF preflight runbooks."
}

foreach ($marker in @(
    "pdf_dependency_inputs_check",
    "pdf_dependency_inputs_check_test.ps1",
    "pdf_visual_validation_status_docs_contract",
    "pdf_visual_validation_status_docs_contract_test.ps1",
    "pdf_visual_segmented_gate_summary",
    "pdf_visual_segmented_gate_summary_test.ps1",
    "pdf_visual_gate_attempt_summary",
    "pdf_visual_gate_attempt_summary_test.ps1",
    "release_note_bundle_visual_verdict_metadata",
    "release_note_bundle_visual_verdict_metadata_test.ps1",
    "pdf_real_business_sample_manifest_contract",
    "pdf_real_business_sample_manifest_contract_test.ps1",
    "pdf_release_readiness_check",
    "pdf_release_readiness_check_test.ps1",
    "pdf_visual_full_gate_guarded",
    "pdf_visual_full_gate_guarded_test.ps1",
    "pdf_full_ctest_guarded",
    "pdf_full_ctest_guarded_test.ps1",
    "TIMEOUT 60"
)) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep the PDF visual status docs contract wired."
}
