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

$sampleId = "document-cjk-font-embed-wrap-mix-text"
$sampleKind = "document_cjk_font_embed_wrap_mix_text"
$builderName = "build_document_cjk_font_embed_wrap_mix_text_sample"
$outputFile = "featherdoc-pdf-regression-document-cjk-font-embed-wrap-mix-text.pdf"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId

Assert-ContainsText -Text $sampleText -ExpectedText $builderName `
    -Message "PDF regression sample generator should keep builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $sampleKind) `
    -Message "PDF regression sample runner should dispatch scenario '$sampleKind'."
Assert-ContainsText -Text $sampleText -ExpectedText ("{0}(cjk_font)" -f $builderName) `
    -Message "PDF regression sample runner should call builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK Font Embed Lite" `
    -Message "PDF CJK font embed wrap mix should reuse the CJK font embed family."
Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK font embed wrap mix sample" `
    -Message "PDF CJK font embed wrap mix should keep the full-id title."
Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_header_paragraphs" `
    -Message "PDF CJK font embed wrap mix should keep header coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_footer_paragraphs" `
    -Message "PDF CJK font embed wrap mix should keep footer coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "render_headers_and_footers = true" `
    -Message "PDF CJK font embed wrap mix should render headers and footers."
Assert-ContainsText -Text $sampleText -ExpectedText "expand_header_footer_page_placeholders = true" `
    -Message "PDF CJK font embed wrap mix should expand page placeholders."
Assert-ContainsText -Text $sampleText -ExpectedText "append_table(5U, 3U)" `
    -Message "PDF CJK font embed wrap mix should keep table key coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_repeats_header" `
    -Message "PDF CJK font embed wrap mix should keep repeated header coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_cant_split" `
    -Message "PDF CJK font embed wrap mix should keep cant-split coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkFontEmbedLiteAccent" `
    -Message "PDF CJK font embed wrap mix should exercise styled accent text."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkFontEmbedLiteNote" `
    -Message "PDF CJK font embed wrap mix should exercise styled note text."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkFontEmbedLiteLarge" `
    -Message "PDF CJK font embed wrap mix should exercise large styled text."

foreach ($stableKey in @(
    "WM-101",
    "WM-202",
    "WM-303",
    "FE-WM-901",
    "FE-WM-911",
    "FE-WM-921",
    "FE-WM-941",
    "FE-WM-951",
    "FE-WM-961",
    "FE-WM-999",
    "ABC 123"
)) {
    Assert-ContainsText -Text $sampleText -ExpectedText $stableKey `
        -Message "PDF CJK font embed wrap mix sample should keep stable key '$stableKey'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText $stableKey `
        -Message "PDF CJK font embed wrap mix manifest should include stable key '$stableKey'."
}

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should keep sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF CJK font embed wrap mix should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF CJK font embed wrap mix manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF CJK font embed wrap mix manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF CJK font embed wrap mix manifest should mark styled text."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_visual_baseline": true' `
    -Message "PDF CJK font embed wrap mix manifest should keep visual baseline marker."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $manifestTestText -ExpectedText "REQUIRE_EQ(samples.size(), 85U)" `
    -Message "PDF regression manifest parser test should expect the updated sample count."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cjk_font_embed_wrap_mix_contract" `
    -Message "CMake should register the PDF CJK font embed wrap mix static contract."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cjk_font_embed_wrap_mix_contract_test.ps1" `
    -Message "CMake should point at the PDF CJK font embed wrap mix static contract script."

Write-Host "PDF CJK font embed wrap mix contract passed."
