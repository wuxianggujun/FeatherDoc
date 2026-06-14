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
        '\s*\(.*?(?=\n\[\[nodiscard\]\]\s+ScenarioResult\s+|\z)'
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
$cmakePath = Join-Path $resolvedRepoRoot "test\cmake\WindowsScriptReleaseTests.cmake"

$sampleTextParts = @(
    Get-Content -Raw -Encoding UTF8 -LiteralPath $samplePath
)
$sampleTextParts += Get-ChildItem -LiteralPath (Split-Path -Parent $samplePath) -Filter "pdf_regression_sample_*.inc" |
    Sort-Object Name |
    ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
$sampleText = $sampleTextParts -join "`n"
$manifestText = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath
$manifestTestText = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestTestPath
$cmakeText = Get-Content -Raw -Encoding UTF8 -LiteralPath $cmakePath

$sampleId = "document-table-merged-cant-split-text"
$sampleKind = "document_table_merged_cant_split_text"
$builderName = "build_document_table_merged_cant_split_text_sample"
$outputFile = "featherdoc-pdf-regression-document-table-merged-cant-split-text.pdf"
$manifestSampleBlock = Get-ManifestSampleBlock `
    -ManifestText $manifestText `
    -SampleId $sampleId
$builderBlock = Get-CppFunctionBlock `
    -SourceText $sampleText `
    -FunctionName $builderName

Assert-ContainsText -Text $sampleText -ExpectedText $builderName `
    -Message "PDF regression sample generator should keep builder '$builderName'."
Assert-ContainsText -Text $sampleText -ExpectedText ('"{0}"' -f $sampleKind) `
    -Message "PDF regression sample runner should dispatch scenario '$sampleKind'."
Assert-ContainsText -Text $builderBlock -ExpectedText "Merged cant split table sample" `
    -Message "PDF merged cant-split sample should keep the title."
Assert-ContainsText -Text $builderBlock -ExpectedText "Repeated headers must keep the merged row intact after the page " `
    -Message "PDF merged cant-split sample should keep the summary."
Assert-ContainsText -Text $builderBlock -ExpectedText "Merged cant split header T-404 page {{page}}" `
    -Message "PDF merged cant-split sample should keep header placeholder contract."
Assert-ContainsText -Text $builderBlock -ExpectedText "Merged cant split footer {{page}} / {{total_pages}}" `
    -Message "PDF merged cant-split sample should keep footer placeholder contract."
Assert-ContainsText -Text $builderBlock -ExpectedText "render_headers_and_footers = true" `
    -Message "PDF merged cant-split sample should render headers and footers."
Assert-ContainsText -Text $builderBlock -ExpectedText "expand_header_footer_page_placeholders = true" `
    -Message "PDF merged cant-split sample should expand page placeholders."
Assert-ContainsText -Text $builderBlock -ExpectedText "append_table(row_count, 3U)" `
    -Message "PDF merged cant-split sample should keep a three-column table."
Assert-ContainsText -Text $builderBlock -ExpectedText "merge_right(2U)" `
    -Message "PDF merged cant-split sample should keep the merged banner row."
Assert-ContainsText -Text $builderBlock -ExpectedText "merge_right(1U)" `
    -Message "PDF merged cant-split sample should keep the merged cant-split row."
Assert-ContainsText -Text $builderBlock -ExpectedText "set_repeats_header()" `
    -Message "PDF merged cant-split sample should keep repeated header coverage."
Assert-ContainsText -Text $builderBlock -ExpectedText "set_cant_split()" `
    -Message "PDF merged cant-split sample should keep cant-split row coverage."
Assert-ContainsText -Text $builderBlock -ExpectedText "row_height_rule::at_least" `
    -Message "PDF merged cant-split sample should keep at-least row height coverage."
Assert-ContainsText -Text $builderBlock -ExpectedText "cell_vertical_alignment::center" `
    -Message "PDF merged cant-split sample should keep vertical alignment coverage."

foreach ($stableKey in @(
    "Release gate",
    "MCS-01",
    "MCS-02",
    "MCS-03",
    "MCS-04",
    "Queue",
    "QA",
    "Archive",
    "Merged cant-split row must",
    "restart beneath the repeated",
    "Page numbering stays stable."
)) {
    Assert-ContainsText -Text $builderBlock -ExpectedText $stableKey `
        -Message "PDF merged cant-split sample should keep stable key '$stableKey'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText $stableKey `
        -Message "PDF merged cant-split manifest should include stable key '$stableKey'."
}

if ($builderBlock -match [regex]::Escape("append_image") -or
    $builderBlock -match [regex]::Escape("append_floating_image")) {
    throw "PDF merged cant-split lightweight sample should stay image-free."
}

Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 3' `
    -Message "PDF merged cant-split sample should stay a three-page flow sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_visual_baseline": true' `
    -Message "PDF merged cant-split sample should keep visual baseline intent."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"visual_expected_pages": 3' `
    -Message "PDF merged cant-split visual baseline should expect three pages."
if ($manifestSampleBlock -match [regex]::Escape('"expected_image_count"')) {
    throw "PDF merged cant-split lightweight sample should not set expected_image_count."
}

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $manifestTestText -ExpectedText "REQUIRE_EQ(samples.size(), 90U)" `
    -Message "PDF regression manifest parser test should track 90 samples."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_document_table_merged_cant_split_contract" `
    -Message "CMake should register the PDF merged cant-split static contract."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_document_table_merged_cant_split_contract_test.ps1" `
    -Message "CMake should point at the PDF merged cant-split contract script."

Write-Host "PDF document table merged cant split contract passed."
