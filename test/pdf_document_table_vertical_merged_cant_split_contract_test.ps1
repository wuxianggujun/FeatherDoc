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

$sampleId = "document-table-vertical-merged-cant-split-text"
$sampleKind = "document_table_vertical_merged_cant_split_text"
$builderName = "build_document_table_vertical_merged_cant_split_text_sample"
$outputFile = "featherdoc-pdf-regression-document-table-vertical-merged-cant-split-text.pdf"
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
Assert-ContainsText -Text $builderBlock -ExpectedText "Vertical merged cant split sample" `
    -Message "PDF vertical merged cant-split sample should keep the old title."
Assert-ContainsText -Text $builderBlock -ExpectedText "Repeated headers must keep a vertical merged owner block intact" `
    -Message "PDF vertical merged cant-split sample should keep the old summary."
Assert-ContainsText -Text $builderBlock -ExpectedText "Vertical merged header V-505 page {{page}}" `
    -Message "PDF vertical merged cant-split sample should keep header placeholder contract."
Assert-ContainsText -Text $builderBlock -ExpectedText "Vertical merged footer {{page}} / {{total_pages}}" `
    -Message "PDF vertical merged cant-split sample should keep footer placeholder contract."
Assert-ContainsText -Text $builderBlock -ExpectedText "render_headers_and_footers = true" `
    -Message "PDF vertical merged cant-split sample should render headers and footers."
Assert-ContainsText -Text $builderBlock -ExpectedText "expand_header_footer_page_placeholders = true" `
    -Message "PDF vertical merged cant-split sample should expand page placeholders."
Assert-ContainsText -Text $builderBlock -ExpectedText "append_table(row_count, 3U)" `
    -Message "PDF vertical merged cant-split sample should keep a three-column table."
Assert-ContainsText -Text $builderBlock -ExpectedText "merge_right(2U)" `
    -Message "PDF vertical merged cant-split sample should keep the merged board header."
Assert-ContainsText -Text $builderBlock -ExpectedText "merge_down(1U)" `
    -Message "PDF vertical merged cant-split sample should keep vertical merge coverage."
Assert-ContainsText -Text $builderBlock -ExpectedText "set_repeats_header()" `
    -Message "PDF vertical merged cant-split sample should keep repeated header coverage."
Assert-ContainsText -Text $builderBlock -ExpectedText "set_cant_split()" `
    -Message "PDF vertical merged cant-split sample should keep cant-split row coverage."
Assert-ContainsText -Text $builderBlock -ExpectedText "cell_vertical_alignment::center" `
    -Message "PDF vertical merged cant-split sample should keep vertical alignment coverage."

foreach ($stableKey in @(
    "Delivery board",
    "VMC-01",
    "VMC-02",
    "VMC-03",
    "VMC-04",
    "VMC-05",
    "Cluster",
    "Owner block restarts below header.",
    "Second note stays with owner.",
    "Tail row confirms clean flow resumes.",
    "Page numbering stays stable."
)) {
    Assert-ContainsText -Text $builderBlock -ExpectedText $stableKey `
        -Message "PDF vertical merged cant-split sample should keep stable key '$stableKey'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText $stableKey `
        -Message "PDF vertical merged cant-split manifest should include stable key '$stableKey'."
}

if ($builderBlock -match [regex]::Escape("append_image") -or
    $builderBlock -match [regex]::Escape("append_floating_image")) {
    throw "PDF vertical merged cant-split lightweight sample should stay image-free."
}

Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sampleId) `
    -Message "PDF regression manifest should define sample '$sampleId'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sampleKind) `
    -Message "PDF regression manifest should keep kind '$sampleKind'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $outputFile) `
    -Message "PDF regression manifest should keep output file '$outputFile'."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
    -Message "PDF vertical merged cant-split should stay a one-page lightweight sample."
Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_visual_baseline": true' `
    -Message "PDF vertical merged cant-split should keep visual baseline intent."
if ($manifestSampleBlock -match [regex]::Escape('"visual_expected_pages"')) {
    throw "PDF vertical merged cant-split lightweight sample should not claim old visual_expected_pages."
}
if ($manifestSampleBlock -match [regex]::Escape('"expected_image_count"')) {
    throw "PDF vertical merged cant-split lightweight sample should not set expected_image_count."
}

Assert-ContainsText -Text $manifestTestText -ExpectedText $sampleId `
    -Message "PDF regression manifest parser test should assert sample '$sampleId'."
Assert-ContainsText -Text $manifestTestText -ExpectedText "REQUIRE_EQ(samples.size(), 90U)" `
    -Message "PDF regression manifest parser test should track 90 samples."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_document_table_vertical_merged_cant_split_contract" `
    -Message "CMake should register the PDF vertical merged cant-split static contract."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_document_table_vertical_merged_cant_split_contract_test.ps1" `
    -Message "CMake should point at the PDF vertical merged cant-split contract script."

Write-Host "PDF document table vertical merged cant split contract passed."
