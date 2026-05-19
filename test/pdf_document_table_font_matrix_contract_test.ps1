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

$sampleId = "document-table-font-matrix-text"
$sampleKind = "document_table_font_matrix_text"
$builder = "build_document_table_font_matrix_text_sample"
$outputFile = "featherdoc-pdf-regression-document-table-font-matrix-text.pdf"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId

Assert-ContainsText -Text $sampleText -ExpectedText $builder `
    -Message "PDF regression sample generator should keep builder '$builder'."
Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $sampleKind) `
    -Message "PDF regression sample runner should dispatch scenario '$sampleKind'."
Assert-ContainsText -Text $sampleText -ExpectedText "Document table font matrix sample" `
    -Message "PDF table font matrix sample should keep the title."
Assert-ContainsText -Text $sampleText -ExpectedText "Document Font Matrix CJK" `
    -Message "PDF table font matrix sample should reuse the CJK font mapping name."
Assert-ContainsText -Text $sampleText -ExpectedText "Document RTL Arabic" `
    -Message "PDF table font matrix sample should reuse the RTL font mapping name."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfRtlArabic" `
    -Message "PDF table font matrix sample should reuse the RTL Arabic style."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfRtlArabicEmphasis" `
    -Message "PDF table font matrix sample should keep emphasized RTL table cell coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_default_run_east_asia_font_family" `
    -Message "PDF table font matrix sample should set the document CJK default font."
Assert-ContainsText -Text $sampleText -ExpectedText "render_headers_and_footers = true" `
    -Message "PDF table font matrix sample should render headers and footers."
Assert-ContainsText -Text $sampleText -ExpectedText "append_table(4U, 2U)" `
    -Message "PDF table font matrix sample should keep table cell font coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_repeats_header" `
    -Message "PDF table font matrix sample should keep repeated header coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_cant_split" `
    -Message "PDF table font matrix sample should keep cant-split row coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-420" `
    -Message "PDF table font matrix sample should keep stable key FE-420."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-421" `
    -Message "PDF table font matrix sample should keep stable key FE-421."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-422" `
    -Message "PDF table font matrix sample should keep stable key FE-422."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-423" `
    -Message "PDF table font matrix sample should keep stable key FE-423."

Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep the table font matrix output file."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF table font matrix should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF table font matrix sample should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF table font matrix sample should require Unicode extraction."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF table font matrix sample should require styled text handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-420" `
    -Message "PDF table font matrix manifest should include FE-420."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-421" `
    -Message "PDF table font matrix manifest should include FE-421."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-422" `
    -Message "PDF table font matrix manifest should include FE-422."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-423" `
    -Message "PDF table font matrix manifest should include FE-423."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "Table matrix header" `
    -Message "PDF table font matrix manifest should include header stable text."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "Table matrix footer" `
    -Message "PDF table font matrix manifest should include footer stable text."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $manifestTestText -ExpectedText "73U" `
    -Message "PDF regression manifest parser test should expect the updated sample count."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should font-gate '$sampleKind'."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_document_table_font_matrix_contract" `
    -Message "CMake should register the PDF table font matrix static contract."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_document_table_font_matrix_contract_test.ps1" `
    -Message "CMake should point at the PDF table font matrix static contract script."

Write-Host "PDF document table font matrix contract passed."
