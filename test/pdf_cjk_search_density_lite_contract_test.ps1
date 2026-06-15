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

$sampleId = "document-cjk-search-density-lite-text"
$sampleKind = "document_cjk_search_density_lite_text"
$builderName = "build_document_cjk_search_density_lite_text_sample"
$outputFile = "featherdoc-pdf-regression-document-cjk-search-density-lite-text.pdf"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId

Assert-ContainsText -Text $sampleText -ExpectedText $builderName `
    -Message "PDF regression sample generator should keep builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $sampleKind) `
    -Message "PDF regression sample runner should dispatch scenario '$sampleKind'."
Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK Font Embed Lite" `
    -Message "PDF CJK search density lite should reuse the CJK font embed family."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkFontEmbedLiteAccent" `
    -Message "PDF CJK search density lite should exercise styled accent text."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkFontEmbedLiteNote" `
    -Message "PDF CJK search density lite should exercise styled note text."
Assert-ContainsText -Text $sampleText -ExpectedText "SD-101" `
    -Message "PDF CJK search density lite should keep stable key SD-101."
Assert-ContainsText -Text $sampleText -ExpectedText "SD-202" `
    -Message "PDF CJK search density lite should keep stable key SD-202."
Assert-ContainsText -Text $sampleText -ExpectedText "SD-303" `
    -Message "PDF CJK search density lite should keep stable key SD-303."
Assert-ContainsText -Text $sampleText -ExpectedText "SD-999" `
    -Message "PDF CJK search density lite should keep stable key SD-999."

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should keep sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF CJK search density lite should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SD-101" `
    -Message "PDF CJK search density lite manifest should include SD-101."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SD-202" `
    -Message "PDF CJK search density lite manifest should include SD-202."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SD-303" `
    -Message "PDF CJK search density lite manifest should include SD-303."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SD-999" `
    -Message "PDF CJK search density lite manifest should include SD-999."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF CJK search density lite manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF CJK search density lite manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF CJK search density lite manifest should mark styled text."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."

Write-Host "PDF CJK search density lite contract passed."
