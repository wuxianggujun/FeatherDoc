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

$sampleId = "document-cjk-style-overlay-lite-text"
$sampleKind = "document_cjk_style_overlay_lite_text"
$builderName = "build_document_cjk_style_overlay_lite_text_sample"
$styleBuilderName = "define_document_cjk_style_overlay_lite_styles"
$outputFile = "featherdoc-pdf-regression-document-cjk-style-overlay-lite-text.pdf"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId

Assert-ContainsText -Text $sampleText -ExpectedText $builderName `
    -Message "PDF regression sample generator should keep builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText $styleBuilderName `
    -Message "PDF CJK style overlay lite should keep its style contract builder."
Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $sampleKind) `
    -Message "PDF regression sample runner should dispatch scenario '$sampleKind'."
Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK Font Embed Lite" `
    -Message "PDF CJK style overlay lite should reuse the CJK font embed family."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkOverlayLiteSuperscript" `
    -Message "PDF CJK style overlay lite should exercise superscript styled text."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkOverlayLiteSubscript" `
    -Message "PDF CJK style overlay lite should exercise subscript styled text."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkOverlayLiteStrike" `
    -Message "PDF CJK style overlay lite should exercise strikethrough styled text."
Assert-ContainsText -Text $sampleText -ExpectedText "run_superscript = true" `
    -Message "PDF CJK style overlay lite should keep superscript metadata."
Assert-ContainsText -Text $sampleText -ExpectedText "run_subscript = true" `
    -Message "PDF CJK style overlay lite should keep subscript metadata."
Assert-ContainsText -Text $sampleText -ExpectedText "run_strikethrough = true" `
    -Message "PDF CJK style overlay lite should keep strikethrough metadata."
Assert-ContainsText -Text $sampleText -ExpectedText "SO-101" `
    -Message "PDF CJK style overlay lite should keep stable key SO-101."
Assert-ContainsText -Text $sampleText -ExpectedText "SO-202" `
    -Message "PDF CJK style overlay lite should keep stable key SO-202."
Assert-ContainsText -Text $sampleText -ExpectedText "SO-303" `
    -Message "PDF CJK style overlay lite should keep stable key SO-303."
Assert-ContainsText -Text $sampleText -ExpectedText "SO-999" `
    -Message "PDF CJK style overlay lite should keep stable key SO-999."

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should keep sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF CJK style overlay lite should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SO-101" `
    -Message "PDF CJK style overlay lite manifest should include SO-101."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SO-202" `
    -Message "PDF CJK style overlay lite manifest should include SO-202."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SO-303" `
    -Message "PDF CJK style overlay lite manifest should include SO-303."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SO-999" `
    -Message "PDF CJK style overlay lite manifest should include SO-999."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF CJK style overlay lite manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF CJK style overlay lite manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF CJK style overlay lite manifest should mark styled text."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."

Write-Host "PDF CJK style overlay lite contract passed."
