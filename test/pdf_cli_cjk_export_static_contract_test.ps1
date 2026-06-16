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
        throw $Message
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$cliExportTestPath = Join-Path $resolvedRepoRoot "test\pdf_cli_export_tests.cpp"
$cliTestsPath = Join-Path $resolvedRepoRoot "test\cli_tests.cpp"
$cliPdfDisabledTestsPath = Join-Path $resolvedRepoRoot "test\cli_pdf_disabled_tests.cpp"
$cmakePath = Join-Path $resolvedRepoRoot "test\CMakeLists.txt"
$cmakeModuleDir = Join-Path $resolvedRepoRoot "test\cmake"
$cmakeTextParts = @(Get-Content -Raw -Encoding UTF8 -LiteralPath $cmakePath)
if (Test-Path -LiteralPath $cmakeModuleDir) {
    $cmakeTextParts += Get-ChildItem -LiteralPath $cmakeModuleDir -Filter "*.cmake" |
        Sort-Object Name |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
}
$visualGatePath = Join-Path $resolvedRepoRoot "scripts\run_pdf_visual_release_gate.ps1"

$cliExportTestText = Get-Content -Raw -LiteralPath $cliExportTestPath
$cliTestsText = Get-Content -Raw -LiteralPath $cliTestsPath
$cliPdfDisabledTestsText = Get-Content -Raw -LiteralPath $cliPdfDisabledTestsPath
$cmakeText = $cmakeTextParts -join "`n"
$visualGateText = Get-Content -Raw -LiteralPath $visualGatePath

Assert-ContainsText -Text $cliExportTestText -ExpectedText "auto find_cjk_font() -> fs::path" `
    -Message "CLI PDF export tests should keep CJK font discovery coverage."
Assert-ContainsText -Text $cliExportTestText -ExpectedText "cli export-pdf honors font subset toggles for CJK fonts" `
    -Message "CLI PDF export tests should keep CJK font subset toggle coverage."
Assert-ContainsText -Text $cliExportTestText -ExpectedText "--cjk-font-file" `
    -Message "CLI PDF export tests should keep the --cjk-font-file path."
Assert-ContainsText -Text $cliExportTestText -ExpectedText "--no-font-subset" `
    -Message "CLI PDF export tests should keep the full-font embedding switch."
Assert-ContainsText -Text $cliExportTestText -ExpectedText "cli export-pdf accepts explicit CJK font without system fallback" `
    -Message "CLI PDF export tests should keep explicit CJK font coverage without system fallback."
Assert-ContainsText -Text $cliExportTestText -ExpectedText "--no-system-font-fallbacks" `
    -Message "CLI PDF export tests should keep the no-system-font-fallbacks option."
Assert-ContainsText -Text $cliExportTestText -ExpectedText "assert_pdfium_can_read(subset_output" `
    -Message "CLI PDF export tests should keep PDFium readback for subset output."
Assert-ContainsText -Text $cliExportTestText -ExpectedText "assert_pdfium_can_read(full_output" `
    -Message "CLI PDF export tests should keep PDFium readback for full output."
Assert-ContainsText -Text $cliExportTestText -ExpectedText "assert_pdfium_can_read(output" `
    -Message "CLI PDF export tests should keep PDFium readback for explicit CJK output."

Assert-ContainsText -Text $cliPdfDisabledTestsText -ExpectedText "cli export-pdf reports disabled pdf support" `
    -Message "CLI tests should keep the disabled PDF support diagnostic."
Assert-ContainsText -Text $cliPdfDisabledTestsText -ExpectedText "PDF export requires configuring with -DFEATHERDOC_BUILD_PDF=ON" `
    -Message "CLI tests should keep the PDF build flag diagnostic."

Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cli_export_tests" `
    -Message "CMake should keep pdf_cli_export_tests registered."
Assert-ContainsText -Text $cmakeText -ExpectedText "add_dependencies(pdf_cli_export_tests featherdoc_cli)" `
    -Message "CMake should keep pdf_cli_export_tests dependent on featherdoc_cli."
Assert-ContainsText -Text $cmakeText -ExpectedText "featherdoc_set_test_labels(pdf_cli_export cli smoke pdf)" `
    -Message "CMake should keep cli/smoke/pdf labels for the PDF export CLI test."
Assert-ContainsText -Text $cmakeText -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT=1" `
    -Message "CMake should keep PDF-enabled CLI compile coverage."
Assert-ContainsText -Text $cmakeText -ExpectedText 'CMAKE_BUILD_TYPE STREQUAL "Debug"' `
    -Message "CMake should keep Debug PDF test runtime DLL resolution."
Assert-ContainsText -Text $cmakeText -ExpectedText "PNG_LIBRARY_DEBUG PNG_LIBRARY_RELEASE PNG_LIBRARY" `
    -Message "CMake should prefer Debug PNG runtime libraries for Debug PDF tests."
Assert-ContainsText -Text $cmakeText -ExpectedText "FREETYPE_LIBRARY_DEBUG" `
    -Message "CMake should prefer Debug FreeType runtime libraries for Debug PDF tests."
Assert-ContainsText -Text $cmakeText -ExpectedText "HARFBUZZ_LIBRARY_DEBUG" `
    -Message "CMake should prefer Debug HarfBuzz runtime libraries for Debug PDF tests."

Assert-ContainsText -Text $visualGateText -ExpectedText 'name = "cli-cjk-font-source"' `
    -Message "PDF visual release gate should keep the CLI CJK font source baseline entry."
Assert-ContainsText -Text $visualGateText -ExpectedText 'test\pdf_cli_export\cjk-font-source.pdf' `
    -Message "PDF visual release gate should render the CLI CJK PDF export output."
Assert-ContainsText -Text $visualGateText -ExpectedText 'output = Join-Path $baselineDir "cli-cjk-font-source"' `
    -Message "PDF visual release gate should keep a stable CLI CJK baseline output directory."
Assert-ContainsText -Text $visualGateText -ExpectedText "expected_pages = 3" `
    -Message "PDF visual release gate should keep the expected CLI CJK PDF page count."

Write-Host "PDF CLI CJK export static contract passed."
