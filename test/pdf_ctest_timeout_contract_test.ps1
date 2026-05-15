param(
    [string]$CTestTestfile
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($CTestTestfile)) {
    throw "CTestTestfile is required."
}

$resolvedCTestTestfile = (Resolve-Path $CTestTestfile).Path
$content = Get-Content -Raw -LiteralPath $resolvedCTestTestfile
$propertyMatches = [regex]::Matches(
    $content,
    'set_tests_properties\(\[=\[(?<name>[^\]]+)\]=\]\s+PROPERTIES\s+(?<properties>.*?)\s+_BACKTRACE_TRIPLES',
    [System.Text.RegularExpressions.RegexOptions]::Singleline)

if ($propertyMatches.Count -eq 0) {
    throw "No set_tests_properties entries found in '$resolvedCTestTestfile'."
}

$pdfTests = @()
$missingTimeoutTests = @()
foreach ($match in $propertyMatches) {
    $testName = $match.Groups["name"].Value
    $properties = $match.Groups["properties"].Value
    $labelsMatch = [regex]::Match($properties, 'LABELS\s+"(?<labels>[^"]*)"')
    if (-not $labelsMatch.Success) {
        continue
    }

    $labels = $labelsMatch.Groups["labels"].Value -split ";"
    if ($labels -notcontains "pdf") {
        continue
    }

    $pdfTests += $testName
    if ($properties -notmatch '\bTIMEOUT\s+"?60"?') {
        $missingTimeoutTests += $testName
    }
}

if ($pdfTests.Count -eq 0) {
    throw "No tests labelled 'pdf' were found in '$resolvedCTestTestfile'."
}

if ($missingTimeoutTests.Count -gt 0) {
    $joinedNames = $missingTimeoutTests -join ", "
    throw "Tests labelled 'pdf' must set TIMEOUT 60. Missing: $joinedNames."
}

Write-Host "Verified TIMEOUT 60 for $($pdfTests.Count) pdf-labelled CTest entries."
