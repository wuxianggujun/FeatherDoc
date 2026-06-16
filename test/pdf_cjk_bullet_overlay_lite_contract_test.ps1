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

function Get-ManifestSampleBlock {
    param(
        [string]$ManifestText,
        [string]$SampleId
    )

    $pattern = '(?s)"id":\s*"' + [regex]::Escape($SampleId) + '".*?\n    \}'
    $matches = [regex]::Matches($ManifestText, $pattern)
    if ($matches.Count -ne 1) {
        throw "PDF regression manifest must define exactly one '$SampleId' sample."
    }
    return $matches[0].Value
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$samplePath = Join-Path $resolvedRepoRoot "samples\pdf_regression_sample.cpp"
$manifestPath = Join-Path $resolvedRepoRoot "test\pdf_regression_manifest.json"
$manifestTestPath = Join-Path $resolvedRepoRoot "test\pdf_regression_manifest_test.cpp"
$cmakePath = Join-Path $resolvedRepoRoot "test\CMakeLists.txt"
$cmakeModuleDir = Join-Path $resolvedRepoRoot "test\cmake"
$cmakeTextParts = @(Get-Content -Raw -Encoding UTF8 -LiteralPath $cmakePath)
if (Test-Path -LiteralPath $cmakeModuleDir) {
    $cmakeTextParts += Get-ChildItem -LiteralPath $cmakeModuleDir -Filter "*.cmake" |
        Sort-Object Name |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
}
$sampleTextParts = @(
    Get-Content -Raw -Encoding UTF8 -LiteralPath $samplePath
)
$sampleTextParts += Get-ChildItem -LiteralPath (Split-Path -Parent $samplePath) -Filter "pdf_regression_sample_*.inc" |
    Sort-Object Name |
    ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
$sampleText = $sampleTextParts -join "`n"
$manifestText = Get-Content -Raw -LiteralPath $manifestPath
$manifestTestText = Get-Content -Raw -LiteralPath $manifestTestPath
$cmakeText = $cmakeTextParts -join "`n"

$sampleId = "document-cjk-bullet-overlay-lite-text"
$sampleKind = "document_cjk_bullet_overlay_lite_text"
$builderName = "build_document_cjk_bullet_overlay_lite_text_sample"
$outputFile = "featherdoc-pdf-regression-document-cjk-bullet-overlay-lite-text.pdf"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId

Assert-ContainsText -Text $sampleText -ExpectedText $builderName `
    -Message "PDF regression sample generator should keep builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText $sampleKind `
    -Message "PDF regression sample runner should mention scenario '$sampleKind'."
Assert-ContainsText -Text $sampleText -ExpectedText ("{0}(cjk_font)" -f $builderName) `
    -Message "PDF regression sample runner should call builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText "define_document_cjk_style_overlay_lite_styles" `
    -Message "PDF CJK bullet overlay lite should reuse style overlay definitions."
Assert-ContainsText -Text $sampleText -ExpectedText "append_document_list_item" `
    -Message "PDF CJK bullet overlay lite should keep bullet list coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkOverlayLiteSuperscript" `
    -Message "PDF CJK bullet overlay lite should keep superscript coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkOverlayLiteSubscript" `
    -Message "PDF CJK bullet overlay lite should keep subscript coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkOverlayLiteStrike" `
    -Message "PDF CJK bullet overlay lite should keep strike coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "BO-101" `
    -Message "PDF CJK bullet overlay lite should keep stable key BO-101."
Assert-ContainsText -Text $sampleText -ExpectedText "BO-202" `
    -Message "PDF CJK bullet overlay lite should keep stable key BO-202."
Assert-ContainsText -Text $sampleText -ExpectedText "BO-777" `
    -Message "PDF CJK bullet overlay lite should keep stable key BO-777."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-BO-999" `
    -Message "PDF CJK bullet overlay lite should keep stable key FE-BO-999."

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should keep sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF CJK bullet overlay lite should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "BO-101" `
    -Message "PDF CJK bullet overlay lite manifest should include BO-101."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "BO-202" `
    -Message "PDF CJK bullet overlay lite manifest should include BO-202."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "BO-777" `
    -Message "PDF CJK bullet overlay lite manifest should include BO-777."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-BO-999" `
    -Message "PDF CJK bullet overlay lite manifest should include FE-BO-999."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF CJK bullet overlay lite manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF CJK bullet overlay lite manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF CJK bullet overlay lite manifest should mark styled text."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."

Write-Host "PDF CJK bullet overlay lite contract passed."
