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

$sampleId = "document-style-gallery-text"
$sampleKind = "document_style_gallery_text"
$builder = "build_document_style_gallery_text_sample"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId

Assert-ContainsText -Text $sampleText -ExpectedText "define_document_style_gallery_styles" `
    -Message "PDF style gallery sample should define reusable document styles."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfStyleGalleryAccent" `
    -Message "PDF style gallery sample should keep the accent style."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfStyleGalleryNote" `
    -Message "PDF style gallery sample should keep the note style."
Assert-ContainsText -Text $sampleText -ExpectedText "run_bold = true" `
    -Message "PDF style gallery accent should preserve bold styling."
Assert-ContainsText -Text $sampleText -ExpectedText "run_italic = true" `
    -Message "PDF style gallery accent should preserve italic styling."
Assert-ContainsText -Text $sampleText -ExpectedText "run_underline = true" `
    -Message "PDF style gallery note should preserve underline styling."
Assert-ContainsText -Text $sampleText -ExpectedText "set_alignment(featherdoc::paragraph_alignment::right)" `
    -Message "PDF style gallery sample should preserve right alignment coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_hanging_indent_twips(360U)" `
    -Message "PDF style gallery sample should preserve hanging indent coverage."
Assert-ContainsText -Text $sampleText -ExpectedText "set_first_line_indent_twips(360U)" `
    -Message "PDF style gallery sample should preserve first-line indent coverage."
Assert-ContainsText -Text $sampleText -ExpectedText $builder `
    -Message "PDF regression sample generator should keep builder '$builder'."
Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $sampleKind) `
    -Message "PDF regression sample runner should dispatch scenario '$sampleKind'."

Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"output_file": "featherdoc-pdf-regression-document-style-gallery-text.pdf"' `
    -Message "PDF regression manifest should keep the style gallery output file."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF regression manifest should keep the style gallery page count."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF style gallery sample should require styled text handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_visual_baseline": true' `
    -Message "PDF style gallery sample should remain in the visual baseline list."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "bold italic blue 16pt" `
    -Message "PDF style gallery manifest should include the accent stable text."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "underlined note" `
    -Message "PDF style gallery manifest should include the note stable text."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "Hanging indent paragraph wraps" `
    -Message "PDF style gallery manifest should include hanging indent stable text."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "First line indent paragraph keeps" `
    -Message "PDF style gallery manifest should include first-line indent stable text."
Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_document_style_gallery_contract" `
    -Message "CMake should register the PDF document style gallery static contract."

Write-Host "PDF document style gallery contract passed."
