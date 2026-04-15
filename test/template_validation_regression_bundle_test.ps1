param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText': $Path"
    }
}

function Get-StringArray {
    param($Value)

    if ($null -eq $Value) {
        return ,@()
    }

    return ,@($Value | ForEach-Object { $_.ToString() })
}

function Assert-StringArrayEquals {
    param(
        [string[]]$Expected,
        [string[]]$Actual,
        [string]$Label
    )

    if ($Expected.Count -ne $Actual.Count) {
        throw "$Label count mismatch."
    }

    for ($index = 0; $index -lt $Expected.Count; $index++) {
        if ($Expected[$index] -ne $Actual[$index]) {
            throw "$Label mismatch at index $index."
        }
    }
}

function Assert-ValidationResultMatches {
    param(
        $Expected,
        $Actual,
        [string]$Label
    )

    Assert-True -Condition ([bool]$Expected.passed -eq [bool]$Actual.passed) `
        -Message "$Label passed flag mismatch."
    Assert-StringArrayEquals `
        -Expected (Get-StringArray $Expected.missing_required) `
        -Actual (Get-StringArray $Actual.missing_required) `
        -Label "$Label missing_required"
    Assert-StringArrayEquals `
        -Expected (Get-StringArray $Expected.duplicate_bookmarks) `
        -Actual (Get-StringArray $Actual.duplicate_bookmarks) `
        -Label "$Label duplicate_bookmarks"
    Assert-StringArrayEquals `
        -Expected (Get-StringArray $Expected.malformed_placeholders) `
        -Actual (Get-StringArray $Actual.malformed_placeholders) `
        -Label "$Label malformed_placeholders"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$outputDir = Join-Path $resolvedWorkingDir "bundle_output"
$regressionScript = Join-Path $resolvedRepoRoot "scripts\run_template_validation_regression.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

& $regressionScript `
    -BuildDir $resolvedBuildDir `
    -OutputDir $outputDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "Template validation regression bundle generation failed."
}

$summaryPath = Join-Path $outputDir "summary.json"
$manifestPath = Join-Path $outputDir "review_manifest.json"
$checklistPath = Join-Path $outputDir "review_checklist.md"
$finalReviewPath = Join-Path $outputDir "final_review.md"

foreach ($path in @($summaryPath, $manifestPath, $checklistPath, $finalReviewPath)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected regression artifact was not generated: $path"
}

$summary = Get-Content -Raw -LiteralPath $summaryPath | ConvertFrom-Json
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json

Assert-True -Condition (-not $summary.visual_enabled) `
    -Message "summary.json unexpectedly reported visual_enabled=true."
Assert-True -Condition (-not $manifest.visual_enabled) `
    -Message "review_manifest.json unexpectedly reported visual_enabled=true."
Assert-True -Condition ($summary.cases.Count -eq 2) `
    -Message "summary.json case count mismatch."
Assert-True -Condition ($manifest.cases.Count -eq 2) `
    -Message "review_manifest.json case count mismatch."

$bodyCase = $summary.cases[0]
$partCase = $summary.cases[1]

Assert-True -Condition ($bodyCase.id -eq "body-template") `
    -Message "Unexpected first case id in summary.json."
Assert-True -Condition ($partCase.id -eq "part-template") `
    -Message "Unexpected second case id in summary.json."

foreach ($path in @(
        $bodyCase.valid_docx_path,
        $bodyCase.invalid_docx_path,
        $bodyCase.sample_report_json,
        $bodyCase.sample_report_markdown,
        $bodyCase.cli_valid_json,
        $bodyCase.cli_invalid_json,
        $partCase.docx_path,
        $partCase.sample_report_json,
        $partCase.sample_report_markdown,
        $partCase.cli_header_json,
        $partCase.cli_footer_json)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected template validation artifact is missing: $path"
}

$bodySampleReport = Get-Content -Raw -LiteralPath $bodyCase.sample_report_json | ConvertFrom-Json
$bodyCliValid = Get-Content -Raw -LiteralPath $bodyCase.cli_valid_json | ConvertFrom-Json
$bodyCliInvalid = Get-Content -Raw -LiteralPath $bodyCase.cli_invalid_json | ConvertFrom-Json

Assert-True -Condition ($bodyCliValid.part -eq "body") `
    -Message "CLI valid body result reported an unexpected part."
Assert-True -Condition ($bodyCliValid.entry_name -eq "word/document.xml") `
    -Message "CLI valid body result reported an unexpected entry_name."
Assert-True -Condition ($bodyCliInvalid.part -eq "body") `
    -Message "CLI invalid body result reported an unexpected part."
Assert-True -Condition ($bodyCliInvalid.entry_name -eq "word/document.xml") `
    -Message "CLI invalid body result reported an unexpected entry_name."
Assert-ValidationResultMatches `
    -Expected $bodySampleReport.valid_template `
    -Actual $bodyCliValid `
    -Label "body valid template"
Assert-ValidationResultMatches `
    -Expected $bodySampleReport.invalid_template `
    -Actual $bodyCliInvalid `
    -Label "body invalid template"

$partSampleReport = Get-Content -Raw -LiteralPath $partCase.sample_report_json | ConvertFrom-Json
$partCliHeader = Get-Content -Raw -LiteralPath $partCase.cli_header_json | ConvertFrom-Json
$partCliFooter = Get-Content -Raw -LiteralPath $partCase.cli_footer_json | ConvertFrom-Json

Assert-True -Condition ($partCliHeader.part -eq "header") `
    -Message "CLI header result reported an unexpected part."
Assert-True -Condition ($partCliHeader.part_index -eq 0) `
    -Message "CLI header result reported an unexpected part_index."
Assert-True -Condition ($partCliHeader.entry_name -eq "word/header1.xml") `
    -Message "CLI header result reported an unexpected entry_name."
Assert-True -Condition ($partCliFooter.part -eq "section-footer") `
    -Message "CLI footer result reported an unexpected part."
Assert-True -Condition ($partCliFooter.section -eq 0) `
    -Message "CLI footer result reported an unexpected section index."
Assert-True -Condition ($partCliFooter.kind -eq "default") `
    -Message "CLI footer result reported an unexpected kind."
Assert-True -Condition ($partCliFooter.entry_name -eq "word/footer1.xml") `
    -Message "CLI footer result reported an unexpected entry_name."
Assert-ValidationResultMatches `
    -Expected $partSampleReport.header `
    -Actual $partCliHeader `
    -Label "part header template"
Assert-ValidationResultMatches `
    -Expected $partSampleReport.footer `
    -Actual $partCliFooter `
    -Label "part footer template"

Assert-Contains -Path $checklistPath -ExpectedText "body-template" -Label "review_checklist.md"
Assert-Contains -Path $checklistPath -ExpectedText "part-template" -Label "review_checklist.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "## Verdict" -Label "final_review.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "body-template:" -Label "final_review.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "part-template:" -Label "final_review.md"

Write-Host "Template validation regression bundle structure passed."
