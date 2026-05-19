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

$sampleText = Get-Content -Raw -LiteralPath $samplePath
$manifestText = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath
$manifestTestText = Get-Content -Raw -LiteralPath $manifestTestPath
$cmakeText = Get-Content -Raw -LiteralPath $cmakePath

$sampleId = "document-cjk-anchor-font-matrix-boundary-text"
$sampleKind = "document_cjk_anchor_font_matrix_boundary_text"
$builderName = "build_document_cjk_anchor_font_matrix_boundary_text_sample"
$styleBuilderName = "define_document_cjk_font_embed_lite_styles"
$outputFile = "featherdoc-pdf-regression-document-cjk-anchor-font-matrix-boundary-text.pdf"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId

Assert-ContainsText -Text $sampleText -ExpectedText $builderName `
    -Message "PDF regression sample generator should keep builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText $styleBuilderName `
    -Message "PDF CJK anchor font matrix boundary should keep its style contract builder."
Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $sampleKind) `
    -Message "PDF regression sample runner should dispatch scenario '$sampleKind'."
Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK Font Embed Lite" `
    -Message "PDF CJK anchor font matrix boundary should map an East Asia font family."
Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK anchor font matrix boundary sample" `
    -Message "PDF CJK anchor font matrix boundary should keep the full-id title."
Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_header_paragraphs" `
    -Message "PDF CJK anchor font matrix boundary should keep header coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_footer_paragraphs" `
    -Message "PDF CJK anchor font matrix boundary should keep footer coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "render_headers_and_footers = true" `
    -Message "PDF CJK anchor font matrix boundary should render headers and footers."
Assert-ContainsText -Text $sampleText -ExpectedText "expand_header_footer_page_placeholders = true" `
    -Message "PDF CJK anchor font matrix boundary should expand page placeholders."
Assert-ContainsText -Text $sampleText -ExpectedText "append_table(5U, 3U)" `
    -Message "PDF CJK anchor font matrix boundary should keep boundary table coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_repeats_header" `
    -Message "PDF CJK anchor font matrix boundary should keep repeated header coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_cant_split" `
    -Message "PDF CJK anchor font matrix boundary should keep cant-split tail coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkFontEmbedLiteAccent" `
    -Message "PDF CJK anchor font matrix boundary should exercise styled accent text."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkFontEmbedLiteLarge" `
    -Message "PDF CJK anchor font matrix boundary should exercise large styled text."

foreach ($stableKey in @(
    "AM-101",
    "AM-202",
    "AM-303",
    "FE-AM-901",
    "FE-AM-921",
    "AM-A-04",
    "AM-B-04",
    "FE-AM-999",
    "Boundary checkpoint A:",
    "Boundary checkpoint B:",
    "Boundary close:",
    "ABC 123"
)) {
    Assert-ContainsText -Text $sampleText -ExpectedText $stableKey `
        -Message "PDF CJK anchor font matrix boundary sample should keep stable key '$stableKey'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText $stableKey `
        -Message "PDF CJK anchor font matrix boundary manifest should include stable key '$stableKey'."
}

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should keep sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF CJK anchor font matrix boundary should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF CJK anchor font matrix boundary manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF CJK anchor font matrix boundary manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF CJK anchor font matrix boundary manifest should mark styled text."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_visual_baseline": true' `
    -Message "PDF CJK anchor font matrix boundary manifest should keep visual baseline marker."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $manifestTestText -ExpectedText "REQUIRE_EQ(samples.size(), 83U)" `
    -Message "PDF regression manifest parser test should expect the updated sample count."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cjk_anchor_font_matrix_boundary_contract" `
    -Message "CMake should register the PDF CJK anchor font matrix boundary static contract."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cjk_anchor_font_matrix_boundary_contract_test.ps1" `
    -Message "CMake should point at the PDF CJK anchor font matrix boundary static contract script."

Write-Host "PDF CJK anchor font matrix boundary contract passed."
