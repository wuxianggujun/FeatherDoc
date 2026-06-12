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

$sampleId = "document-cjk-numbered-list-page-flow-lite-text"
$sampleKind = "document_cjk_numbered_list_page_flow_lite_text"
$builderName = "build_document_cjk_numbered_list_page_flow_lite_text_sample"
$outputFile = "featherdoc-pdf-regression-document-cjk-numbered-list-page-flow-lite-text.pdf"
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
    -Message "PDF CJK numbered list page flow lite should reuse the CJK font embed family."
Assert-ContainsText -Text $sampleText -ExpectedText "append_document_list_item" `
    -Message "PDF CJK numbered list page flow lite should keep list coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "list_kind::decimal" `
    -Message "PDF CJK numbered list page flow lite should keep decimal numbering coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "list_kind::bullet" `
    -Message "PDF CJK numbered list page flow lite should keep bullet switch coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkFontEmbedLiteLarge" `
    -Message "PDF CJK numbered list page flow lite should keep large styled text."
Assert-ContainsText -Text $sampleText -ExpectedText "NL-101" `
    -Message "PDF CJK numbered list page flow lite should keep stable key NL-101."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-NL-202" `
    -Message "PDF CJK numbered list page flow lite should keep stable key FE-NL-202."
Assert-ContainsText -Text $sampleText -ExpectedText "NL-888" `
    -Message "PDF CJK numbered list page flow lite should keep stable key NL-888."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-NL-921" `
    -Message "PDF CJK numbered list page flow lite should keep stable key FE-NL-921."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-NL-999" `
    -Message "PDF CJK numbered list page flow lite should keep stable key FE-NL-999."

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should keep sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF CJK numbered list page flow lite should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "NL-101" `
    -Message "PDF CJK numbered list page flow lite manifest should include NL-101."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-NL-202" `
    -Message "PDF CJK numbered list page flow lite manifest should include FE-NL-202."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "NL-888" `
    -Message "PDF CJK numbered list page flow lite manifest should include NL-888."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-NL-921" `
    -Message "PDF CJK numbered list page flow lite manifest should include FE-NL-921."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-NL-999" `
    -Message "PDF CJK numbered list page flow lite manifest should include FE-NL-999."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF CJK numbered list page flow lite manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF CJK numbered list page flow lite manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF CJK numbered list page flow lite manifest should mark styled text."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."

Write-Host "PDF CJK numbered list page flow lite contract passed."
