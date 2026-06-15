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
$manifestText = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath
$manifestTestText = Get-Content -Raw -LiteralPath $manifestTestPath
$cmakeText = $cmakeTextParts -join "`n"

$sampleId = "document-font-matrix-text"
$sampleKind = "document_font_matrix_text"
$builder = "build_document_font_matrix_text_sample"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId

Assert-ContainsText -Text $sampleText -ExpectedText $builder `
    -Message "PDF regression sample generator should keep builder '$builder'."
Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $sampleKind) `
    -Message "PDF regression sample runner should dispatch scenario '$sampleKind'."
Assert-ContainsText -Text $sampleText -ExpectedText "Document Font Matrix CJK" `
    -Message "PDF font matrix sample should keep the CJK font mapping name."
Assert-ContainsText -Text $sampleText -ExpectedText "Document RTL Arabic" `
    -Message "PDF font matrix sample should keep the RTL font mapping name."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfRtlArabic" `
    -Message "PDF font matrix sample should reuse the RTL Arabic style."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfRtlArabicEmphasis" `
    -Message "PDF font matrix sample should reuse the emphasized RTL Arabic style."
Assert-ContainsText -Text $sampleText -ExpectedText "set_default_run_east_asia_font_family" `
    -Message "PDF font matrix sample should set the document CJK default font."
Assert-ContainsText -Text $sampleText -ExpectedText "render_headers_and_footers = true" `
    -Message "PDF font matrix sample should keep header/footer rendering active."
Assert-ContainsText -Text $sampleText -ExpectedText "append_table(3U, 2U)" `
    -Message "PDF font matrix sample should keep table cell font coverage."

Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"output_file": "featherdoc-pdf-regression-document-font-matrix-text.pdf"' `
    -Message "PDF regression manifest should keep the font matrix output file."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF regression manifest should keep the font matrix page count."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF font matrix sample should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF font matrix sample should require Unicode extraction."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF font matrix sample should require styled text handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-300" `
    -Message "PDF font matrix manifest should include the stable identifier."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "Header matrix" `
    -Message "PDF font matrix manifest should include header stable text."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "Footer matrix" `
    -Message "PDF font matrix manifest should include footer stable text."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "Cell" `
    -Message "PDF font matrix manifest should include table cell stable text."
Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should font-gate '$sampleKind'."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_document_font_matrix_contract" `
    -Message "CMake should register the PDF document font matrix static contract."

Write-Host "PDF document font matrix contract passed."
