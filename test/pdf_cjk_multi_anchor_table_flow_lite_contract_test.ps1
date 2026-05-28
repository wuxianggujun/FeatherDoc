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
$manifestText = Get-Content -Raw -LiteralPath $manifestPath
$manifestTestText = Get-Content -Raw -LiteralPath $manifestTestPath
$cmakeText = Get-Content -Raw -LiteralPath $cmakePath

$sampleId = "document-cjk-multi-anchor-table-flow-lite-text"
$sampleKind = "document_cjk_multi_anchor_table_flow_lite_text"
$builderName = "build_document_cjk_multi_anchor_table_flow_lite_text_sample"
$outputFile = "featherdoc-pdf-regression-document-cjk-multi-anchor-table-flow-lite-text.pdf"
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
    -Message "PDF CJK multi anchor table flow lite should reuse the CJK font embed family."
Assert-ContainsText -Text $sampleText -ExpectedText "append_table(5U, 3U)" `
    -Message "PDF CJK multi anchor table flow lite should keep its table contract."
Assert-ContainsText -Text $sampleText -ExpectedText "set_column_width_twips" `
    -Message "PDF CJK multi anchor table flow lite should keep explicit column widths."
Assert-ContainsText -Text $sampleText -ExpectedText "set_repeats_header" `
    -Message "PDF CJK multi anchor table flow lite should keep repeated header coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_cant_split" `
    -Message "PDF CJK multi anchor table flow lite should keep cant-split row coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkFontEmbedLiteLarge" `
    -Message "PDF CJK multi anchor table flow lite should keep large styled text."
Assert-ContainsText -Text $sampleText -ExpectedText "MA-101" `
    -Message "PDF CJK multi anchor table flow lite should keep stable key MA-101."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-MA-202" `
    -Message "PDF CJK multi anchor table flow lite should keep stable key FE-MA-202."
Assert-ContainsText -Text $sampleText -ExpectedText "MA-A-04" `
    -Message "PDF CJK multi anchor table flow lite should keep stable key MA-A-04."
Assert-ContainsText -Text $sampleText -ExpectedText "MA-B-04" `
    -Message "PDF CJK multi anchor table flow lite should keep stable key MA-B-04."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-MA-999" `
    -Message "PDF CJK multi anchor table flow lite should keep stable key FE-MA-999."

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should keep sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF CJK multi anchor table flow lite should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "MA-101" `
    -Message "PDF CJK multi anchor table flow lite manifest should include MA-101."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-MA-202" `
    -Message "PDF CJK multi anchor table flow lite manifest should include FE-MA-202."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "MA-A-04" `
    -Message "PDF CJK multi anchor table flow lite manifest should include MA-A-04."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "MA-B-04" `
    -Message "PDF CJK multi anchor table flow lite manifest should include MA-B-04."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-MA-999" `
    -Message "PDF CJK multi anchor table flow lite manifest should include FE-MA-999."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF CJK multi anchor table flow lite manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF CJK multi anchor table flow lite manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF CJK multi anchor table flow lite manifest should mark styled text."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."

Write-Host "PDF CJK multi anchor table flow lite contract passed."
