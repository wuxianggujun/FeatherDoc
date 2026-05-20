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

function Get-CppFunctionBlock {
    param(
        [string]$SourceText,
        [string]$FunctionName
    )

    $pattern = '(?s)\[\[nodiscard\]\]\s+ScenarioResult\s+' +
        [regex]::Escape($FunctionName) +
        '\s*\(.*?(?=\n\[\[nodiscard\]\]\s+ScenarioResult\s+)'
    $matches = [regex]::Matches($SourceText, $pattern)
    if ($matches.Count -ne 1) {
        throw "Expected exactly one C++ function block for '$FunctionName'."
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

$sampleId = "document-cjk-style-overlay-page-flow-text"
$sampleKind = "document_cjk_style_overlay_page_flow_text"
$builderName = "build_document_cjk_style_overlay_page_flow_text_sample"
$outputFile = "featherdoc-pdf-regression-document-cjk-style-overlay-page-flow-text.pdf"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId
$builderBlock = Get-CppFunctionBlock `
    -SourceText $sampleText `
    -FunctionName $builderName

Assert-ContainsText -Text $sampleText -ExpectedText $builderName `
    -Message "PDF regression sample generator should keep builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $sampleKind) `
    -Message "PDF regression sample runner should dispatch scenario '$sampleKind'."
Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK Font Embed Lite" `
    -Message "PDF CJK style overlay page flow should reuse the lite CJK font family."
Assert-ContainsText -Text $sampleText -ExpectedText "define_document_cjk_style_overlay_lite_styles" `
    -Message "PDF CJK style overlay page flow should reuse lite overlay styles."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkOverlayLiteSuperscript" `
    -Message "PDF CJK style overlay page flow should exercise superscript styled text."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkOverlayLiteSubscript" `
    -Message "PDF CJK style overlay page flow should exercise subscript styled text."
Assert-ContainsText -Text $sampleText -ExpectedText "DocumentPdfCjkOverlayLiteStrike" `
    -Message "PDF CJK style overlay page flow should exercise strikethrough styled text."
Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_header_paragraphs(" `
    -Message "PDF CJK style overlay page flow should keep header contracts."
Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_footer_paragraphs(" `
    -Message "PDF CJK style overlay page flow should keep footer contracts."
Assert-ContainsText -Text $sampleText -ExpectedText "section_reference_kind::first_page" `
    -Message "PDF CJK style overlay page flow should keep first-page header/footer references."
Assert-ContainsText -Text $sampleText -ExpectedText "section_reference_kind::even_page" `
    -Message "PDF CJK style overlay page flow should keep even-page header/footer references."
Assert-ContainsText -Text $sampleText -ExpectedText "set_repeats_header()" `
    -Message "PDF CJK style overlay page flow should keep repeated table header contract."
Assert-ContainsText -Text $sampleText -ExpectedText "merge_down(1U)" `
    -Message "PDF CJK style overlay page flow should keep vertical merge contract."
Assert-ContainsText -Text $sampleText -ExpectedText "set_cant_split()" `
    -Message "PDF CJK style overlay page flow should keep cant-split row contract."
Assert-ContainsText -Text $sampleText -ExpectedText "SO-888" `
    -Message "PDF CJK style overlay page flow should keep shared key SO-888."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-SO-901" `
    -Message "PDF CJK style overlay page flow should keep intro key FE-SO-901."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-SO-921" `
    -Message "PDF CJK style overlay page flow should keep matrix key FE-SO-921."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-SO-961" `
    -Message "PDF CJK style overlay page flow should keep table key FE-SO-961."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-SO-981" `
    -Message "PDF CJK style overlay page flow should keep settle key FE-SO-981."
Assert-ContainsText -Text $sampleText -ExpectedText "FE-SO-999" `
    -Message "PDF CJK style overlay page flow should keep closing key FE-SO-999."
Assert-ContainsText -Text $sampleText -ExpectedText "ABC 123" `
    -Message "PDF CJK style overlay page flow should keep mixed Latin/digit text."
if ($builderBlock -match [regex]::Escape("append_image")) {
    throw "PDF CJK style overlay page flow should stay image-free in this low-resource contract."
}

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should define sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF CJK style overlay page flow should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SO-888" `
    -Message "PDF CJK style overlay page flow manifest should include SO-888."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SO-101" `
    -Message "PDF CJK style overlay page flow manifest should include SO-101."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SO-202" `
    -Message "PDF CJK style overlay page flow manifest should include SO-202."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SO-303" `
    -Message "PDF CJK style overlay page flow manifest should include SO-303."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "SO-A-03" `
    -Message "PDF CJK style overlay page flow manifest should include SO-A-03."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-SO-999" `
    -Message "PDF CJK style overlay page flow manifest should include FE-SO-999."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF CJK style overlay page flow manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF CJK style overlay page flow manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF CJK style overlay page flow manifest should mark styled text."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_visual_baseline": true' `
    -Message "PDF CJK style overlay page flow manifest should mark visual baseline intent."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $manifestTestText -ExpectedText "REQUIRE_EQ(samples.size(), 89U)" `
    -Message "PDF regression manifest parser test should track 85 samples."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cjk_style_overlay_page_flow_contract" `
    -Message "CMake should register the style overlay page flow static contract."

Write-Host "PDF CJK style overlay page flow contract passed."
