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

$requiredSamples = @(
    [ordered]@{
        Id = "document-cjk-numbered-list-page-flow-text"
        Kind = "document_cjk_numbered_list_page_flow_text"
        Builder = "build_document_cjk_numbered_list_page_flow_text_sample"
        OutputFile = "featherdoc-pdf-regression-document-cjk-numbered-list-page-flow-text.pdf"
        StableKeys = @("NL-101", "FE-NL-202", "NL-888", "FE-NL-921", "FE-NL-999")
        RequiredCode = @("list_kind::decimal", "append_document_list_item_at_level", "set_repeats_header")
    },
    [ordered]@{
        Id = "document-cjk-bullet-page-flow-text"
        Kind = "document_cjk_bullet_page_flow_text"
        Builder = "build_document_cjk_bullet_page_flow_text_sample"
        OutputFile = "featherdoc-pdf-regression-document-cjk-bullet-page-flow-text.pdf"
        StableKeys = @("BL-101", "FE-BL-202", "BL-777", "FE-BL-921", "FE-BL-999")
        RequiredCode = @("list_kind::bullet", "append_document_list_item_at_level", "DocumentPdfCjkFontEmbedLiteLarge")
    },
    [ordered]@{
        Id = "document-cjk-bullet-overlay-page-flow-text"
        Kind = "document_cjk_bullet_overlay_page_flow_text"
        Builder = "build_document_cjk_bullet_overlay_page_flow_text_sample"
        OutputFile = "featherdoc-pdf-regression-document-cjk-bullet-overlay-page-flow-text.pdf"
        StableKeys = @("BO-101", "BO-202", "BO-777", "FE-BO-921", "FE-BO-999")
        RequiredCode = @("DocumentPdfCjkOverlayLiteSuperscript", "DocumentPdfCjkOverlayLiteSubscript", "DocumentPdfCjkOverlayLiteStrike")
    }
)

foreach ($required in $requiredSamples) {
    $manifestSampleBlock = Get-ManifestSampleBlock `
        -ManifestText $manifestText `
        -SampleId $required.Id

    Assert-ContainsText -Text $sampleText -ExpectedText $required.Builder `
        -Message "PDF regression sample generator should keep builder '$($required.Builder)'."
    Assert-ContainsText -Text $sampleText -ExpectedText $required.Kind `
        -Message "PDF regression sample runner should mention scenario '$($required.Kind)'."
    Assert-ContainsText -Text $sampleText -ExpectedText ("{0}(" -f $required.Builder) `
        -Message "PDF regression sample runner should call builder '$($required.Builder)'."
    Assert-ContainsText -Text $sampleText -ExpectedText "cjk_font" `
        -Message "PDF CJK list page flow samples should pass the resolved CJK font."
    Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK Font Embed Lite" `
        -Message "PDF CJK list page flow samples should reuse the CJK font embed family."
    Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_header_paragraphs" `
        -Message "PDF CJK list page flow samples should keep header coverage."
    Assert-ContainsText -Text $sampleText -ExpectedText "ensure_section_footer_paragraphs" `
        -Message "PDF CJK list page flow samples should keep footer coverage."
    Assert-ContainsText -Text $sampleText -ExpectedText "render_headers_and_footers = true" `
        -Message "PDF CJK list page flow samples should render headers and footers."

    foreach ($requiredCode in $required.RequiredCode) {
        Assert-ContainsText -Text $sampleText -ExpectedText $requiredCode `
            -Message "PDF CJK list page flow samples should keep code marker '$requiredCode'."
    }

    Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $required.Id) `
        -Message "PDF regression manifest should keep sample '$($required.Id)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $required.Kind) `
        -Message "PDF regression manifest should keep kind '$($required.Kind)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $required.OutputFile) `
        -Message "PDF regression manifest should keep output file '$($required.OutputFile)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
        -Message "PDF CJK list page flow samples should stay one-page lightweight samples."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
        -Message "PDF CJK list page flow manifests should require CJK handling."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
        -Message "PDF CJK list page flow manifests should require Unicode handling."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
        -Message "PDF CJK list page flow manifests should mark styled text."

    foreach ($stableKey in $required.StableKeys) {
        Assert-ContainsText -Text $sampleText -ExpectedText $stableKey `
            -Message "PDF CJK list page flow sample should keep stable key '$stableKey'."
        Assert-ContainsText -Text $manifestSampleBlock -ExpectedText $stableKey `
            -Message "PDF CJK list page flow manifest should include stable key '$stableKey'."
    }

    Assert-ContainsText -Text $manifestTestText -ExpectedText $required.Id `
        -Message "PDF regression manifest parser test should assert sample '$($required.Id)'."
    Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $required.Kind) `
        -Message "CMake PDF regression registration should mark '$($required.Kind)' as a CJK test."
}

Assert-ContainsText -Text $manifestTestText -ExpectedText "REQUIRE_EQ(samples.size(), 85U)" `
    -Message "PDF regression manifest parser test should expect the updated sample count."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cjk_list_page_flow_contract" `
    -Message "CMake should register the PDF CJK list page flow static contract."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_cjk_list_page_flow_contract_test.ps1" `
    -Message "CMake should point to the PDF CJK list page flow contract script."

Write-Host "PDF CJK list page flow contract passed."
