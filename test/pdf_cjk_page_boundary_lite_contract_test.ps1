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

$sampleTextParts = @(
    Get-Content -Raw -Encoding UTF8 -LiteralPath $samplePath
)
$sampleTextParts += Get-ChildItem -LiteralPath (Split-Path -Parent $samplePath) -Filter "pdf_regression_sample_*.inc" |
    Sort-Object Name |
    ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
$sampleText = $sampleTextParts -join "`n"
$manifestText = Get-Content -Raw -LiteralPath $manifestPath
$manifestTestText = Get-Content -Raw -LiteralPath $manifestTestPath
$cmakeText = Get-Content -Raw -LiteralPath $cmakePath

$sampleId = "document-cjk-page-boundary-lite-text"
$sampleKind = "document_cjk_page_boundary_lite_text"
$builderName = "build_document_cjk_page_boundary_lite_text_sample"
$outputFile = "featherdoc-pdf-regression-document-cjk-page-boundary-lite-text.pdf"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId

Assert-ContainsText -Text $sampleText -ExpectedText $builderName `
    -Message "PDF regression sample generator should keep builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText $sampleKind `
    -Message "PDF regression sample runner should mention scenario '$sampleKind'."
Assert-ContainsText -Text $sampleText -ExpectedText ("{0}(" -f $builderName) `
    -Message "PDF regression sample runner should call builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK Font Embed Lite" `
    -Message "PDF CJK page boundary lite should reuse the CJK font embed family."
Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_header_paragraphs" `
    -Message "PDF CJK page boundary lite should keep header coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_footer_paragraphs" `
    -Message "PDF CJK page boundary lite should keep footer coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "document.append_image" `
    -Message "PDF CJK page boundary lite should keep inline image coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "append_floating_image" `
    -Message "PDF CJK page boundary lite should keep floating image wrap coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "floating_image_wrap_mode::square" `
    -Message "PDF CJK page boundary lite should keep square wrap coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "render_headers_and_footers = true" `
    -Message "PDF CJK page boundary lite should render headers and footers."
Assert-ContainsText -Text $sampleText -ExpectedText "render_inline_images = true" `
    -Message "PDF CJK page boundary lite should render inline images."
Assert-ContainsText -Text $sampleText -ExpectedText "PB-101" `
    -Message "PDF CJK page boundary lite should keep stable key PB-101."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-PB-202" `
    -Message "PDF CJK page boundary lite should keep stable key FE-PB-202."
Assert-ContainsText -Text $sampleText -ExpectedText "PB-404" `
    -Message "PDF CJK page boundary lite should keep stable key PB-404."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-PB-505" `
    -Message "PDF CJK page boundary lite should keep stable key FE-PB-505."
Assert-ContainsText -Text $sampleText -ExpectedText "PB-604" `
    -Message "PDF CJK page boundary lite should keep stable key PB-604."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-PB-999" `
    -Message "PDF CJK page boundary lite should keep stable key FE-PB-999."

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should keep sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF CJK page boundary lite should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_image_count": 2' `
    -Message "PDF CJK page boundary lite manifest should keep inline and floating image count."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "PB-101" `
    -Message "PDF CJK page boundary lite manifest should include PB-101."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-PB-202" `
    -Message "PDF CJK page boundary lite manifest should include FE-PB-202."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-PB-999" `
    -Message "PDF CJK page boundary lite manifest should include FE-PB-999."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF CJK page boundary lite manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF CJK page boundary lite manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF CJK page boundary lite manifest should mark styled text."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."

Write-Host "PDF CJK page boundary lite contract passed."
