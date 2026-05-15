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
$pdfTestsMissingSmoke = @()
$pdfCliTestsMissingLabels = @()
foreach ($match in $propertyMatches) {
    $testName = $match.Groups["name"].Value
    $properties = $match.Groups["properties"].Value
    $labelsMatch = [regex]::Match($properties, 'LABELS\s+"(?<labels>[^"]*)"')
    if (-not $labelsMatch.Success) {
        continue
    }

    $labels = $labelsMatch.Groups["labels"].Value -split ";"
    if ($labels -contains "pdf") {
        $pdfTests += $testName
        if ($labels -notcontains "smoke") {
            $pdfTestsMissingSmoke += $testName
        }
    }

    if ($testName -like "pdf_cli_*") {
        $missingLabels = @()
        foreach ($requiredLabel in @("cli", "smoke", "pdf")) {
            if ($labels -notcontains $requiredLabel) {
                $missingLabels += $requiredLabel
            }
        }

        if ($missingLabels.Count -gt 0) {
            $pdfCliTestsMissingLabels += ("{0} missing {1}" -f $testName, ($missingLabels -join ","))
        }
    }
}

if ($pdfTests.Count -eq 0) {
    throw "No tests labelled 'pdf' were found in '$resolvedCTestTestfile'."
}

if ($pdfTestsMissingSmoke.Count -gt 0) {
    throw "Tests labelled 'pdf' must also carry the 'smoke' label. Missing: $($pdfTestsMissingSmoke -join ", ")."
}

if ($pdfCliTestsMissingLabels.Count -gt 0) {
    throw "PDF CLI tests must carry 'cli;smoke;pdf' labels. Problems: $($pdfCliTestsMissingLabels -join "; ")."
}

Write-Host "Verified smoke labels for $($pdfTests.Count) pdf-labelled CTest entries."
