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
$manifestText = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath
$manifestTestText = Get-Content -Raw -LiteralPath $manifestTestPath
$cmakeText = Get-Content -Raw -LiteralPath $cmakePath

Assert-ContainsText -Text $sampleText -ExpectedText "candidate_arabic_fonts" `
    -Message "PDF RTL bidi samples should keep Arabic font discovery."
Assert-ContainsText -Text $sampleText -ExpectedText "FEATHERDOC_TEST_ARABIC_FONT" `
    -Message "PDF RTL bidi samples should allow explicit Arabic font configuration."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfRtlParagraph" `
    -Message "PDF RTL bidi samples should keep the RTL paragraph style."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfRtlArabic" `
    -Message "PDF RTL bidi samples should keep the Arabic character style."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfRtlArabicEmphasis" `
    -Message "PDF RTL bidi samples should keep the emphasized Arabic style."
Assert-ContainsText -Text $sampleText -ExpectedText "paragraph_bidi = true" `
    -Message "PDF RTL bidi paragraph style should enable bidi paragraphs."
Assert-ContainsText -Text $sampleText -ExpectedText "run_bidi_language = std::string{`"ar-SA`"}" `
    -Message "PDF RTL bidi Arabic style should preserve language metadata."
Assert-ContainsText -Text $sampleText -ExpectedText "run_rtl = true" `
    -Message "PDF RTL bidi Arabic style should preserve RTL run metadata."

$samples = @(
    [ordered]@{
        Id = "document-rtl-bidi-text"
        Kind = "document_rtl_bidi_text"
        Builder = "build_document_rtl_bidi_text_sample"
        OutputFile = "featherdoc-pdf-regression-document-rtl-bidi-text.pdf"
        StableText = "FE-2048"
        Pages = '"expected_pages": 1'
    },
    [ordered]@{
        Id = "header-footer-rtl-text"
        Kind = "header_footer_rtl_text"
        Builder = "build_header_footer_rtl_text_sample"
        OutputFile = "featherdoc-pdf-regression-header-footer-rtl-text.pdf"
        StableText = "HF-2048"
        Pages = '"expected_pages": 1'
    },
    [ordered]@{
        Id = "header-footer-rtl-variants-text"
        Kind = "header_footer_rtl_variants_text"
        Builder = "build_header_footer_rtl_variants_text_sample"
        OutputFile = "featherdoc-pdf-regression-header-footer-rtl-variants-text.pdf"
        StableText = "HF-303"
        Pages = '"expected_pages": 3'
    }
)

foreach ($sample in $samples) {
    $manifestSampleBlock = Get-ManifestSampleBlock `
        -ManifestText $manifestText `
        -SampleId $sample.Id

    Assert-ContainsText -Text $sampleText -ExpectedText $sample.Builder `
        -Message "PDF regression sample generator should keep builder '$($sample.Builder)'."
    Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $sample.Kind) `
        -Message "PDF regression sample runner should dispatch scenario '$($sample.Kind)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sample.Kind) `
        -Message "PDF regression manifest should keep kind '$($sample.Kind)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $sample.OutputFile) `
        -Message "PDF regression manifest should keep output '$($sample.OutputFile)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText $sample.Pages `
        -Message "PDF regression manifest should keep page count for '$($sample.Id)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText $sample.StableText `
        -Message "PDF regression manifest should include stable text '$($sample.StableText)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
        -Message "PDF RTL bidi samples should require Unicode extraction."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
        -Message "PDF RTL bidi samples should require styled text handling."
    Assert-ContainsText -Text $manifestTestText -ExpectedText $sample.Id `
        -Message "PDF regression manifest parser test should assert sample '$($sample.Id)'."
    Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sample.Kind) `
        -Message "CMake PDF regression registration should classify '$($sample.Kind)' as font-gated."
}

Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_rtl_bidi_light_contract" `
    -Message "CMake should register the PDF RTL bidi light static contract."

Write-Host "PDF RTL bidi light contract passed."
