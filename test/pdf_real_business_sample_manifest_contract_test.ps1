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

foreach ($group in $businessSampleGroups.GetEnumerator()) {
    foreach ($sampleId in @($group.Value)) {
        Assert-True -Condition $sampleIdSet.ContainsKey($sampleId) `
            -Message "PDF business sample group '$($group.Key)' should reference existing manifest sample '$sampleId'."
    }
}

$checklistText = Get-Content -Raw -Encoding UTF8 -LiteralPath $checklistPath
$buildingPdfText = Get-Content -Raw -Encoding UTF8 -LiteralPath $buildingPdfPath
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

Write-Host "PDF real business sample manifest contract passed."
