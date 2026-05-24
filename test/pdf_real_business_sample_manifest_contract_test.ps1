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
        "42",
        "43",
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
        "general PDF-to-Word"
    )) {
    Assert-True -Condition ($buildingPdfText -match [regex]::Escape($expectedText)) `
        -Message "BUILDING_PDF.md should contain '$expectedText'."
}

Write-Host "PDF real business sample manifest contract passed."
