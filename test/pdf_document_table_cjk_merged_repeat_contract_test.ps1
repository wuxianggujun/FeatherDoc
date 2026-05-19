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

$sampleId = "document-table-cjk-merged-repeat-text"
$sampleKind = "document_table_cjk_merged_repeat_text"
$builder = "build_document_table_cjk_merged_repeat_text_sample"
$outputFile = "featherdoc-pdf-regression-document-table-cjk-merged-repeat-text.pdf"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId

Assert-ContainsText -Text $sampleText -ExpectedText $builder `
    -Message "PDF regression sample generator should keep builder '$builder'."
Assert-ContainsText -Text $sampleText -ExpectedText "config.scenario ==" `
    -Message "PDF regression sample runner should compare config.scenario."
Assert-ContainsText -Text $sampleText -ExpectedText ('"{0}"' -f $sampleKind) `
    -Message "PDF regression sample runner should dispatch scenario '$sampleKind'."
Assert-ContainsText -Text $sampleText -ExpectedText "Document table CJK merged repeat sample" `
    -Message "PDF table CJK merged repeat sample should keep the title."
Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK Font Embed Lite" `
    -Message "PDF table CJK merged repeat sample should reuse the CJK font embed family."
Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_header_paragraphs" `
    -Message "PDF table CJK merged repeat sample should keep header coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_footer_paragraphs" `
    -Message "PDF table CJK merged repeat sample should keep footer coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "render_headers_and_footers = true" `
    -Message "PDF table CJK merged repeat sample should render headers and footers."
Assert-ContainsText -Text $sampleText -ExpectedText "expand_header_footer_page_placeholders = true" `
    -Message "PDF table CJK merged repeat sample should expand page placeholders."
Assert-ContainsText -Text $sampleText -ExpectedText "append_table(5U, 3U)" `
    -Message "PDF table CJK merged repeat sample should keep its table shape."
Assert-ContainsText -Text $sampleText -ExpectedText "merge_right(2U)" `
    -Message "PDF table CJK merged repeat sample should keep merged board header coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "merge_down(1U)" `
    -Message "PDF table CJK merged repeat sample should keep vertical merge coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "merge_right(1U)" `
    -Message "PDF table CJK merged repeat sample should keep tail merge coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_repeats_header" `
    -Message "PDF table CJK merged repeat sample should keep repeated header coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_cant_split" `
    -Message "PDF table CJK merged repeat sample should keep cant-split tail coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "cell_vertical_alignment::center" `
    -Message "PDF table CJK merged repeat sample should keep vertical alignment coverage."

foreach ($stableKey in @("MR-CJK-101", "MR-CJK-201", "MR-CJK-303", "FE-MR-CJK-303", "MR-CJK-999", "ABC 123")) {
    Assert-ContainsText -Text $sampleText -ExpectedText $stableKey `
        -Message "PDF table CJK merged repeat sample should keep stable key '$stableKey'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText $stableKey `
        -Message "PDF table CJK merged repeat manifest should include stable key '$stableKey'."
}

Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF table CJK merged repeat should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF table CJK merged repeat manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF table CJK merged repeat manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF table CJK merged repeat manifest should mark styled text."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $manifestTestText -ExpectedText "REQUIRE_EQ(samples.size(), 85U)" `
    -Message "PDF regression manifest parser test should expect the updated sample count."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_document_table_cjk_merged_repeat_contract" `
    -Message "CMake should register the PDF table CJK merged repeat static contract."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_document_table_cjk_merged_repeat_contract_test.ps1" `
    -Message "CMake should point at the PDF table CJK merged repeat static contract script."

Write-Host "PDF document table CJK merged repeat contract passed."
