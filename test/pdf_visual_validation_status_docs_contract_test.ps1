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
$dependencyInputsScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\check_pdf_dependency_inputs.ps1"
$preflightScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\check_pdf_visual_release_gate_preflight.ps1"
$governanceReportScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_pdf_visual_release_gate_preflight_governance_report.ps1"
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
    "-FinalizeOnly -SkipPreflight",
    "selected_pdfium_provider = prebuilt",
    "pdf_dependency_inputs_status = ready",
    "pdf_build_options_enabled = true",
    "pdfio_dependency_ready = true",
    "pdfium_dependency_ready = true",
    "ctest_list_contains_pdf_gate_tests = pass",
    "run_pdf_ctest_bounded_subset.ps1",
    "pdf_ctest_bounded_subset_release_trace",
    "pdf_ctest_bounded_contract_static_release_trace",
    "pdf_ctest_bounded_cjk_flow_static_release_trace",
    "pdf_ctest_bounded_regression_basic_text_release_trace",
    "pdf_ctest_bounded_regression_styled_document_release_trace",
    "pdf_ctest_bounded_regression_business_samples_release_trace",
    "Subset",
    "subset = smoke-import",
    "subset = contract-static",
    "subset = cjk-flow-static",
    "subset = regression-basic-text",
    "subset = regression-styled-document",
    "subset = regression-business-samples",
    "contract-static",
    "cjk-flow-static",
    "regression-basic-text",
    "regression-styled-document",
    "regression-business-samples",
    "selected_test_count = 10",
    "ctest_timeout_seconds = 60",
    "pdf_cjk_font_search_density_flow_contract",
    "pdf_cjk_list_page_flow_contract",
    "pdf_regression_manifest",
    "pdf_regression_four-page-text",
    "pdf_regression_styled-text",
    "pdf_regression_document-font-matrix-text",
    "pdf_regression_invoice-grid-text",
    "pdf_regression_document-cjk-image-wrap-stress-text",
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
    "visual baseline manifest",
    "run_pdf_ctest_bounded_subset.ps1",
    "-Subset smoke-import",
    "-Subset contract-static",
    "-Subset cjk-flow-static",
    "-Subset regression-basic-text",
    "-Subset regression-styled-document",
    "-Subset regression-business-samples",
    "contract-static",
    "cjk-flow-static",
    "regression-basic-text",
    "regression-styled-document",
    "regression-business-samples",
    "pdf_ctest_bounded_cjk_flow_static_release_trace",
    "pdf_ctest_bounded_regression_basic_text_release_trace",
    "pdf_ctest_bounded_regression_styled_document_release_trace",
    "pdf_ctest_bounded_regression_business_samples_release_trace",
    "pdf_cjk_font_search_density_flow_contract",
    "pdf_cjk_list_page_flow_contract",
    "pdf_regression_manifest",
    "pdf_regression_four-page-text",
    "pdf_regression_styled-text",
    "pdf_regression_document-font-matrix-text",
    "pdf_regression_invoice-grid-text",
    "pdf_regression_document-cjk-image-wrap-stress-text",
    "selected_test_count = 10",
    "ctest_timeout_seconds = 60"
)

foreach ($marker in $buildingPdfFixtureMarkers) {
    Assert-ContainsText -Text $buildingPdfDoc -ExpectedText $marker `
        -Message "BUILDING_PDF.md should preserve the PDF preflight fixture boundary marker."
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
    "pdf_ctest_bounded_subset_release_trace",
    "pdf_ctest_bounded_contract_static_release_trace",
    "pdf_ctest_bounded_cjk_flow_static_release_trace",
    "pdf_ctest_bounded_regression_basic_text_release_trace",
    "pdf_ctest_bounded_regression_styled_document_release_trace",
    "pdf_ctest_bounded_regression_business_samples_release_trace",
    "subset = smoke-import",
    "subset = contract-static",
    "subset = cjk-flow-static",
    "subset = regression-basic-text",
    "subset = regression-styled-document",
    "subset = regression-business-samples",
    "contract-static",
    "cjk-flow-static",
    "regression-basic-text",
    "regression-styled-document",
    "regression-business-samples",
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
    "release_note_bundle_visual_verdict_metadata",
    "release_note_bundle_visual_verdict_metadata_test.ps1",
    "pdf_real_business_sample_manifest_contract",
    "pdf_real_business_sample_manifest_contract_test.ps1",
    "TIMEOUT 60"
)) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep the PDF visual status docs contract wired."
}
