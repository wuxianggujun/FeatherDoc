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

$sampleId = "document-cjk-extreme-page-breaks-text"
$sampleKind = "document_cjk_extreme_page_breaks_text"
$builderName = "build_document_cjk_extreme_page_breaks_text_sample"
$outputFile = "featherdoc-pdf-regression-document-cjk-extreme-page-breaks-text.pdf"
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
Assert-ContainsText -Text $builderBlock -ExpectedText "Document CJK Font Embed Lite" `
    -Message "PDF CJK extreme page breaks should reuse the lite CJK font family."
Assert-ContainsText -Text $builderBlock -ExpectedText "ensure_section_header_paragraphs(" `
    -Message "PDF CJK extreme page breaks should keep header contracts."
Assert-ContainsText -Text $builderBlock -ExpectedText "ensure_section_footer_paragraphs(" `
    -Message "PDF CJK extreme page breaks should keep footer contracts."
Assert-ContainsText -Text $builderBlock -ExpectedText "section_reference_kind::first_page" `
    -Message "PDF CJK extreme page breaks should keep first-page header/footer references."
Assert-ContainsText -Text $builderBlock -ExpectedText "section_reference_kind::even_page" `
    -Message "PDF CJK extreme page breaks should keep even-page header/footer references."
Assert-ContainsText -Text $builderBlock -ExpectedText "set_repeats_header()" `
    -Message "PDF CJK extreme page breaks should keep repeated table header contract."
Assert-ContainsText -Text $builderBlock -ExpectedText "merge_down(1U)" `
    -Message "PDF CJK extreme page breaks should keep vertical merge contract."
Assert-ContainsText -Text $builderBlock -ExpectedText "set_cant_split()" `
    -Message "PDF CJK extreme page breaks should keep cant-split row contract."
Assert-ContainsText -Text $builderBlock -ExpectedText "Break first header PB-101 page {{page}}" `
    -Message "PDF CJK extreme page breaks should keep first-page header key PB-101."
Assert-ContainsText -Text $builderBlock -ExpectedText "Break even header PB-202 page {{page}}" `
    -Message "PDF CJK extreme page breaks should keep even-page header key PB-202."
Assert-ContainsText -Text $builderBlock -ExpectedText "Break header PB-303 page {{page}}" `
    -Message "PDF CJK extreme page breaks should keep default header key PB-303."
Assert-ContainsText -Text $builderBlock -ExpectedText "Break first footer {{page}} / {{total_pages}}" `
    -Message "PDF CJK extreme page breaks should keep first-page footer contract."
Assert-ContainsText -Text $builderBlock -ExpectedText "Break even footer {{page}} / {{total_pages}}" `
    -Message "PDF CJK extreme page breaks should keep even-page footer contract."
Assert-ContainsText -Text $builderBlock -ExpectedText "Break footer {{page}} / {{total_pages}}" `
    -Message "PDF CJK extreme page breaks should keep default footer contract."
Assert-ContainsText -Text $builderBlock -ExpectedText "FE-PB-901" `
    -Message "PDF CJK extreme page breaks should keep overview key FE-PB-901."
Assert-ContainsText -Text $builderBlock -ExpectedText "FE-PB-921" `
    -Message "PDF CJK extreme page breaks should keep boundary key FE-PB-921."
Assert-ContainsText -Text $builderBlock -ExpectedText "FE-PB-932" `
    -Message "PDF CJK extreme page breaks should keep group 12 key FE-PB-932."
Assert-ContainsText -Text $builderBlock -ExpectedText "FE-PB-938" `
    -Message "PDF CJK extreme page breaks should keep group 18 key FE-PB-938."
Assert-ContainsText -Text $builderBlock -ExpectedText "FE-PB-999" `
    -Message "PDF CJK extreme page breaks should keep close key FE-PB-999."
Assert-ContainsText -Text $builderBlock -ExpectedText "ABC 123" `
    -Message "PDF CJK extreme page breaks should keep mixed Latin/digit text."
if ($builderBlock -match [regex]::Escape("append_image") -or
    $builderBlock -match [regex]::Escape("append_floating_image")) {
    throw "PDF CJK extreme page breaks low-resource contract should stay image-free."
}

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should define sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF CJK extreme page breaks should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "PB-101" `
    -Message "PDF CJK extreme page breaks manifest should include PB-101."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "PB-202" `
    -Message "PDF CJK extreme page breaks manifest should include PB-202."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "PB-303" `
    -Message "PDF CJK extreme page breaks manifest should include PB-303."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText "FE-PB-999" `
    -Message "PDF CJK extreme page breaks manifest should include FE-PB-999."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
    -Message "PDF CJK extreme page breaks manifest should require CJK handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
    -Message "PDF CJK extreme page breaks manifest should require Unicode handling."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
    -Message "PDF CJK extreme page breaks manifest should mark styled text."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_visual_baseline": true' `
    -Message "PDF CJK extreme page breaks manifest should mark visual baseline intent."

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $manifestTestText -ExpectedText "REQUIRE_EQ(samples.size(), 89U)" `
    -Message "PDF regression manifest parser test should track 89 samples."
Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $sampleKind) `
    -Message "CMake PDF regression registration should mark '$sampleKind' as a CJK test."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cjk_extreme_page_breaks_contract" `
    -Message "CMake should register the CJK extreme page breaks static contract."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cjk_extreme_page_breaks_contract_test.ps1" `
    -Message "CMake should register the CJK extreme page breaks contract script."

Write-Host "PDF CJK extreme page breaks contract passed."
