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
$cmakePath = Join-Path $resolvedRepoRoot "test\CMakeLists.txt"

$cliExportTestText = Get-Content -Raw -LiteralPath $cliExportTestPath
$cliTestsText = Get-Content -Raw -LiteralPath $cliTestsPath
$cmakeText = Get-Content -Raw -LiteralPath $cmakePath

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

Assert-ContainsText -Text $cliTestsText -ExpectedText "cli export-pdf reports disabled pdf support" `
    -Message "CLI tests should keep the disabled PDF support diagnostic."
Assert-ContainsText -Text $cliTestsText -ExpectedText "PDF export requires configuring with -DFEATHERDOC_BUILD_PDF=ON" `
    -Message "CLI tests should keep the PDF build flag diagnostic."

Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cli_export_tests" `
    -Message "CMake should keep pdf_cli_export_tests registered."
Assert-ContainsText -Text $cmakeText -ExpectedText "add_dependencies(pdf_cli_export_tests featherdoc_cli)" `
    -Message "CMake should keep pdf_cli_export_tests dependent on featherdoc_cli."
Assert-ContainsText -Text $cmakeText -ExpectedText "featherdoc_set_test_labels(pdf_cli_export cli smoke pdf)" `
    -Message "CMake should keep cli/smoke/pdf labels for the PDF export CLI test."
Assert-ContainsText -Text $cmakeText -ExpectedText "FEATHERDOC_CLI_ENABLE_PDF=1" `
    -Message "CMake should keep PDF-enabled CLI compile coverage."

Write-Host "PDF CLI CJK export static contract passed."
