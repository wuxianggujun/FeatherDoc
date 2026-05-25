param(
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$OutputJson = "",
    [string]$CtestExecutable = "ctest",
    [ValidateSet("smoke-import", "contract-static", "cjk-flow-static", "regression-basic-text", "regression-styled-document")]
    [string]$Subset = "smoke-import"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        return [System.IO.Path]::GetFullPath($InputPath)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath))
}

function Resolve-CtestExecutable {
    param([string]$Executable)

    if ([string]::IsNullOrWhiteSpace($Executable)) {
        throw "CTest executable is required."
    }

    if ([System.IO.Path]::IsPathRooted($Executable)) {
        if (Test-Path -LiteralPath $Executable) {
            return (Resolve-Path -LiteralPath $Executable).Path
        }

        throw "CTest executable was not found: $Executable"
    }

    $resolved = Get-Command $Executable -ErrorAction SilentlyContinue
    if ($resolved) {
        return $resolved.Source
    }

    throw "CTest executable was not found. Pass -CtestExecutable or ensure ctest is in PATH."
}

function Invoke-CapturedCommand {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = @(& $ExecutablePath @Arguments 2>&1)
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    return [ordered]@{
        exit_code = $exitCode
        lines = @($output | ForEach-Object { $_.ToString() })
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedCtestExecutable = Resolve-CtestExecutable -Executable $CtestExecutable
$resolvedOutputJson = if ([string]::IsNullOrWhiteSpace($OutputJson)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputJson
}

if (-not (Test-Path -LiteralPath $resolvedBuildDir)) {
    throw "Build directory was not found: $resolvedBuildDir"
}

$subsets = [ordered]@{
    "smoke-import" = [ordered]@{
        description = "PDF smoke/import executable subset"
        tests = @(
            "pdf_document_generator_probe",
            "pdf_font_resolver",
            "pdf_text_metrics",
            "pdf_text_shaper",
            "pdf_document_adapter_font",
            "pdf_cli_export",
            "pdf_cli_import",
            "pdf_import_structure",
            "pdf_import_failure",
            "pdf_import_table_heuristic"
        )
    }
    "contract-static" = [ordered]@{
        description = "PDF docs/layout/ctest static contract subset"
        tests = @(
            "pdf_import_docs_contract",
            "pdf_ctest_timeout_contract",
            "pdf_ctest_label_contract",
            "pdf_bidi_line_layout_static_contract",
            "pdf_document_style_gallery_contract",
            "pdf_document_font_matrix_contract",
            "pdf_document_table_font_matrix_contract",
            "pdf_cjk_copy_search_matrix_contract",
            "pdf_cjk_font_embed_matrix_contract",
            "pdf_cjk_anchor_font_matrix_boundary_contract"
        )
    }
    "cjk-flow-static" = [ordered]@{
        description = "PDF CJK/RTL flow static contract subset"
        tests = @(
            "pdf_cjk_font_search_density_flow_contract",
            "pdf_cjk_font_embed_wrap_mix_contract",
            "pdf_cjk_repeated_key_boundary_flow_contract",
            "pdf_cjk_style_overlay_page_flow_contract",
            "pdf_cjk_complex_layout_contract",
            "pdf_cjk_image_wrap_stress_contract",
            "pdf_cjk_extreme_page_breaks_contract",
            "pdf_cjk_vertical_merge_wrap_cant_split_contract",
            "pdf_rtl_bidi_light_contract",
            "pdf_cjk_list_page_flow_contract"
        )
    }
    "regression-basic-text" = [ordered]@{
        description = "PDF basic text regression subset"
        tests = @(
            "pdf_regression_manifest",
            "pdf_regression_single-text",
            "pdf_regression_multi-page-text",
            "pdf_regression_font-size-text",
            "pdf_regression_color-text",
            "pdf_regression_three-page-text",
            "pdf_regression_landscape-text",
            "pdf_regression_title-body-text",
            "pdf_regression_dense-text",
            "pdf_regression_four-page-text"
        )
    }
    "regression-styled-document" = [ordered]@{
        description = "PDF styled/document regression subset"
        tests = @(
            "pdf_regression_styled-text",
            "pdf_regression_mixed-style-text",
            "pdf_regression_contract-cjk-style",
            "pdf_regression_document-contract-cjk-style",
            "pdf_regression_underline-text",
            "pdf_regression_strikethrough-text",
            "pdf_regression_superscript-subscript-text",
            "pdf_regression_style-superscript-subscript-text",
            "pdf_regression_document-style-gallery-text",
            "pdf_regression_document-font-matrix-text"
        )
    }
}

$subsetConfig = $subsets[$Subset]
$selectedTests = @($subsetConfig.tests)
$regex = "^($(($selectedTests | ForEach-Object { [regex]::Escape($_) }) -join '|'))$"

$listResult = Invoke-CapturedCommand `
    -ExecutablePath $resolvedCtestExecutable `
    -Arguments @("--test-dir", $resolvedBuildDir, "-N", "-R", $regex)

if ($listResult.exit_code -ne 0) {
    throw "Failed to list bounded PDF CTest subset."
}

$listedTests = @(
    $listResult.lines | ForEach-Object {
        if ($_ -match "^\s*Test\s+#\d+:\s+(.+?)\s*$") {
            $Matches[1]
        }
    }
)

$missingTests = @($selectedTests | Where-Object { $listedTests -notcontains $_ })
if ($missingTests.Count -gt 0) {
    throw "Bounded PDF CTest subset is incomplete. Missing tests: $($missingTests -join ', ')"
}

$runResult = Invoke-CapturedCommand `
    -ExecutablePath $resolvedCtestExecutable `
    -Arguments @("--test-dir", $resolvedBuildDir, "-R", $regex, "--output-on-failure", "--timeout", "60")

foreach ($line in $runResult.lines) {
    Write-Host $line
}

$status = if ($runResult.exit_code -eq 0) { "pass" } else { "fail" }
$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    status = $status
    verdict = $status
    subset = $Subset
    subset_description = $subsetConfig.description
    repo_root = $repoRoot
    build_dir = $resolvedBuildDir
    ctest_executable = $resolvedCtestExecutable
    ctest_timeout_seconds = 60
    selected_test_count = $selectedTests.Count
    selected_tests = $selectedTests
    regex = $regex
    missing_tests = $missingTests
    listed_tests = $listedTests
    exit_code = $runResult.exit_code
}

if (-not [string]::IsNullOrWhiteSpace($resolvedOutputJson)) {
    $outputDir = Split-Path -Parent $resolvedOutputJson
    if (-not [string]::IsNullOrWhiteSpace($outputDir)) {
        New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    }

    ($summary | ConvertTo-Json -Depth 6) | Set-Content -Path $resolvedOutputJson -Encoding UTF8
    Write-Host "[pdf-ctest-bounded-subset] Summary written to $resolvedOutputJson"
}

if ($runResult.exit_code -ne 0) {
    throw "Bounded PDF CTest subset failed."
}
