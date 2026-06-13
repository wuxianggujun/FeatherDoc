param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Get-OptionalPropertyValue {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function New-TextFromCodePoints {
    param([object[]]$CodePoints)

    $builder = [System.Text.StringBuilder]::new()
    foreach ($codePoint in $CodePoints) {
        if ($codePoint -is [string]) {
            [void]$builder.Append($codePoint)
        } else {
            [void]$builder.Append([char][int]$codePoint)
        }
    }

    return $builder.ToString()
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$manifestPath = Join-Path $resolvedRepoRoot "test\pdf_regression_manifest.json"
$checklistPath = Join-Path $resolvedRepoRoot "docs\pdf_release_readiness_checklist_zh.rst"
$buildingPdfPath = Join-Path $resolvedRepoRoot "BUILDING_PDF.md"
$boundedSubsetScriptPath = Join-Path $resolvedRepoRoot "scripts\run_pdf_ctest_bounded_subset.ps1"

$manifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath | ConvertFrom-Json
$sampleIds = @($manifest.samples | ForEach-Object { [string]$_.id })
$sampleIdSet = @{}
foreach ($sampleId in $sampleIds) {
    $sampleIdSet[$sampleId] = $true
}

Assert-True -Condition ($sampleIds.Count -eq 90) `
    -Message "PDF regression manifest should contain 90 samples."
Assert-True -Condition (@($manifest.samples | Where-Object { (Get-OptionalPropertyValue -Object $_ -Name "expect_visual_baseline") -eq $true }).Count -eq 42) `
    -Message "PDF regression manifest should contain 42 visual baseline samples."
Assert-True -Condition (@($manifest.samples | Where-Object { (Get-OptionalPropertyValue -Object $_ -Name "expect_cjk") -eq $true }).Count -eq 43) `
    -Message "PDF regression manifest should contain 43 CJK text-layer samples."

$businessSampleGroups = [ordered]@{
    contract = @(
        "contract-cjk-style",
        "document-contract-cjk-style"
    )
    invoice_or_quote = @(
        "invoice-grid-text",
        "document-invoice-table-text"
    )
    image_report = @(
        "image-report-text",
        "cjk-image-report-text",
        "document-cjk-image-wrap-stress-text"
    )
    long_document = @(
        "long-report-text",
        "document-long-flow-text"
    )
    multi_section = @(
        "sectioned-report-text",
        "header-footer-text",
        "document-cjk-table-wrap-page-flow-text"
    )
}

$boundedBusinessSampleIds = @(
    "contract-cjk-style",
    "document-contract-cjk-style",
    "invoice-grid-text",
    "document-invoice-table-text",
    "image-report-text",
    "cjk-image-report-text",
    "document-cjk-image-wrap-stress-text",
    "long-report-text",
    "document-long-flow-text",
    "sectioned-report-text"
)

$allBusinessSampleIds = New-Object 'System.Collections.Generic.List[string]'
foreach ($group in $businessSampleGroups.GetEnumerator()) {
    foreach ($sampleId in @($group.Value)) {
        $allBusinessSampleIds.Add($sampleId) | Out-Null
        Assert-True -Condition $sampleIdSet.ContainsKey($sampleId) `
            -Message "PDF business sample group '$($group.Key)' should reference existing manifest sample '$sampleId'."
    }
}

$checklistText = Get-Content -Raw -Encoding UTF8 -LiteralPath $checklistPath
$buildingPdfText = @(
    Get-Content -Raw -Encoding UTF8 -LiteralPath $buildingPdfPath
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "docs") -Filter "building_pdf*_archive.md" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$boundedSubsetScriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $boundedSubsetScriptPath

foreach ($sampleId in @($allBusinessSampleIds.ToArray())) {
    Assert-True -Condition ($checklistText -match [regex]::Escape($sampleId)) `
        -Message "PDF release readiness checklist should preserve real business sample '$sampleId'."
    Assert-True -Condition ($buildingPdfText -match [regex]::Escape($sampleId)) `
        -Message "BUILDING_PDF.md should preserve real business sample '$sampleId'."
}

$businessSubsetMatch = [regex]::Match(
    $boundedSubsetScriptText,
    '(?s)"regression-business-samples"\s*=\s*\[ordered\]@\{.*?tests\s*=\s*@\((?<tests>.*?)\)\s*\r?\n\s*\}'
)
Assert-True -Condition $businessSubsetMatch.Success `
    -Message "run_pdf_ctest_bounded_subset.ps1 should declare the regression-business-samples subset."
$boundedSubsetTests = @(
    [regex]::Matches($businessSubsetMatch.Groups["tests"].Value, '"([^"]+)"') |
        ForEach-Object { [string]$_.Groups[1].Value }
)
$expectedBoundedSubsetTests = @($boundedBusinessSampleIds | ForEach-Object { "pdf_regression_$_" })
Assert-True -Condition ($boundedSubsetTests.Count -eq $expectedBoundedSubsetTests.Count) `
    -Message "regression-business-samples subset should contain exactly $($expectedBoundedSubsetTests.Count) tests."
for ($index = 0; $index -lt $expectedBoundedSubsetTests.Count; $index++) {
    Assert-True -Condition ($boundedSubsetTests[$index] -eq $expectedBoundedSubsetTests[$index]) `
        -Message "regression-business-samples subset drifted at index $index. Expected '$($expectedBoundedSubsetTests[$index])', got '$($boundedSubsetTests[$index])'."
}

foreach ($expectedText in @(
        "verdict = pass",
        "90",
        "visual_baseline_manifest_count = 42",
        "baselines_count = 44",
        "cjk_manifest_count = 43",
        "cjk_copy_search_count = 43",
        "text-first",
        "opt-in",
        "OCR",
        "PDF"
    )) {
    Assert-True -Condition ($checklistText -match [regex]::Escape($expectedText)) `
        -Message "PDF release readiness checklist should contain '$expectedText'."
}

Assert-True -Condition ($checklistText -match [regex]::Escape("pdf_real_business_sample_release_entry_trace")) `
    -Message "PDF release readiness checklist should keep the real business sample release-entry trace marker."

foreach ($expectedText in @(
        "manifest ID",
        "support matrix",
        "release checklist",
        "text-first importer",
        "general PDF-to-Word",
        "ToUnicode",
        (New-TextFromCodePoints @("43 ", 0x4E2A, " CJK text-layer ", 0x6837, 0x672C)),
        (New-TextFromCodePoints @(0x6BB5, 0x843D, 0xFF1A)),
        (New-TextFromCodePoints @(0x8868, 0x683C, 0xFF1A)),
        (New-TextFromCodePoints @(0x56FE, 0x7247, 0xFF1A)),
        (New-TextFromCodePoints @(0x9875, 0x7709, 0x9875, 0x811A, 0xFF1A)),
        (New-TextFromCodePoints @(0x5B57, 0x4F53, 0xFF1A)),
        (New-TextFromCodePoints @("CJK", 0xFF1A)),
        (New-TextFromCodePoints @("RTL / Bidi", 0xFF1A)),
        (New-TextFromCodePoints @(0x5B57, 0x6BB5, 0xFF1A)),
        (New-TextFromCodePoints @("Word ", 0x5B57, 0x6BB5, 0x8BED, 0x4E49))
    )) {
    Assert-True -Condition ($buildingPdfText -match [regex]::Escape($expectedText)) `
        -Message "BUILDING_PDF.md should contain '$expectedText'."
}

Assert-True -Condition ($buildingPdfText -match [regex]::Escape("pdf_real_business_sample_release_entry_trace")) `
    -Message "BUILDING_PDF.md should keep the real business sample release-entry trace marker."

Write-Host "PDF real business sample manifest contract passed."
