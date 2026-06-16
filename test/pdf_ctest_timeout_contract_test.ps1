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

$expectedTimeouts = @{
    pdf_cli_import = "120"
}
$pdfTests = @()
$invalidTimeoutTests = @()
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
    $expectedTimeout = if ($expectedTimeouts.ContainsKey($testName)) {
        $expectedTimeouts[$testName]
    } else {
        "60"
    }
    $timeoutMatch = [regex]::Match($properties, '\bTIMEOUT\s+"?(?<timeout>\d+)"?')
    if (-not $timeoutMatch.Success -or $timeoutMatch.Groups["timeout"].Value -ne $expectedTimeout) {
        $actualTimeout = if ($timeoutMatch.Success) {
            $timeoutMatch.Groups["timeout"].Value
        } else {
            "<missing>"
        }
        $invalidTimeoutTests += "$testName expected=$expectedTimeout actual=$actualTimeout"
    }
}

if ($pdfTests.Count -eq 0) {
    throw "No tests labelled 'pdf' were found in '$resolvedCTestTestfile'."
}

if ($invalidTimeoutTests.Count -gt 0) {
    $joinedNames = $invalidTimeoutTests -join ", "
    throw "Tests labelled 'pdf' must set TIMEOUT 60 unless listed as a measured exception. Invalid: $joinedNames."
}

Write-Host "Verified bounded TIMEOUT values for $($pdfTests.Count) pdf-labelled CTest entries."
