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

function Assert-DocxCoreEntries {
    param([string]$Path)

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($Path)
    try {
        $entryNames = @($archive.Entries | ForEach-Object { $_.FullName })
        foreach ($requiredEntry in @("[Content_Types].xml", "_rels/.rels", "word/document.xml")) {
            Assert-True -Condition ($entryNames -contains $requiredEntry) `
                -Message "DOCX is missing required ZIP entry '$requiredEntry': $Path"
        }
    } finally {
        $archive.Dispose()
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$outputDir = Join-Path $resolvedWorkingDir "bundle_output"
$regressionScript = Join-Path $resolvedRepoRoot "scripts\run_section_page_setup_regression.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

& $regressionScript `
    -BuildDir $resolvedBuildDir `
    -OutputDir $outputDir `
    -SkipBuild `
    -SkipVisual

if ($LASTEXITCODE -ne 0) {
    throw "Section page setup regression bundle generation failed."
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
$summaryHasAggregateEvidence = $summary.PSObject.Properties.Name -contains "aggregate_evidence"
$manifestHasAggregateEvidence = $manifest.PSObject.Properties.Name -contains "aggregate_evidence"

Assert-True -Condition (-not $summary.visual_enabled) `
    -Message "summary.json unexpectedly reported visual_enabled=true."
Assert-True -Condition (-not $manifest.visual_enabled) `
    -Message "review_manifest.json unexpectedly reported visual_enabled=true."
Assert-True -Condition ((-not $summaryHasAggregateEvidence) -or $null -eq $summary.aggregate_evidence) `
    -Message "summary.json unexpectedly contained aggregate visual evidence."
Assert-True -Condition ((-not $manifestHasAggregateEvidence) -or $null -eq $manifest.aggregate_evidence) `
    -Message "review_manifest.json unexpectedly contained aggregate visual evidence."

$expectedCases = @(
    [ordered]@{ id = "api-sample"; docx = "section_page_setup.docx"; inspection = "inspect_page_setup.json" },
    [ordered]@{ id = "cli-rewrite"; docx = "section_page_setup_cli.docx"; inspection = "inspect_page_setup.json" }
)

Assert-True -Condition ($summary.cases.Count -eq $expectedCases.Count) `
    -Message "summary.json case count mismatch."
Assert-True -Condition ($manifest.cases.Count -eq $expectedCases.Count) `
    -Message "review_manifest.json case count mismatch."

for ($index = 0; $index -lt $expectedCases.Count; $index++) {
    $expectedCase = $expectedCases[$index]
    $summaryCase = $summary.cases[$index]
    $manifestCase = $manifest.cases[$index]
    $caseDir = Join-Path $outputDir $expectedCase.id
    $expectedDocxPath = [System.IO.Path]::GetFullPath((Join-Path $caseDir $expectedCase.docx))
    $expectedInspectionPath = [System.IO.Path]::GetFullPath((Join-Path $caseDir $expectedCase.inspection))

    Assert-True -Condition ($summaryCase.id -eq $expectedCase.id) `
        -Message "Unexpected summary case order at index $index."
    Assert-True -Condition ($manifestCase.id -eq $expectedCase.id) `
        -Message "Unexpected manifest case order at index $index."
    Assert-True -Condition ([System.IO.Path]::GetFullPath($summaryCase.docx_path) -eq $expectedDocxPath) `
        -Message "summary.json recorded an unexpected DOCX path for case '$($expectedCase.id)'."
    Assert-True -Condition ([System.IO.Path]::GetFullPath($manifestCase.docx_path) -eq $expectedDocxPath) `
        -Message "review_manifest.json recorded an unexpected DOCX path for case '$($expectedCase.id)'."
    Assert-True -Condition ([System.IO.Path]::GetFullPath($summaryCase.inspection_json) -eq $expectedInspectionPath) `
        -Message "summary.json recorded an unexpected inspection path for case '$($expectedCase.id)'."
    Assert-True -Condition (Test-Path -LiteralPath $expectedDocxPath) `
        -Message "Expected DOCX is missing for case '$($expectedCase.id)': $expectedDocxPath"
    Assert-True -Condition (Test-Path -LiteralPath $expectedInspectionPath) `
        -Message "Expected inspection JSON is missing for case '$($expectedCase.id)': $expectedInspectionPath"
    Assert-DocxCoreEntries -Path $expectedDocxPath

    Assert-True -Condition ($null -eq $summaryCase.visual_output_dir) `
        -Message "summary.json unexpectedly recorded a visual output directory for case '$($expectedCase.id)'."
    Assert-Contains -Path $expectedInspectionPath -ExpectedText '"count":2' -Label $expectedCase.inspection
}

$cliSetResultPath = Join-Path $outputDir "cli-rewrite\set_section_page_setup.json"
Assert-True -Condition (Test-Path -LiteralPath $cliSetResultPath) `
    -Message "Expected CLI mutation JSON is missing: $cliSetResultPath"
Assert-Contains -Path $cliSetResultPath -ExpectedText '"command":"set-section-page-setup"' -Label "set_section_page_setup.json"
Assert-Contains -Path $cliSetResultPath -ExpectedText '"orientation":"landscape"' -Label "set_section_page_setup.json"

Assert-Contains -Path $checklistPath -ExpectedText "api-sample" -Label "review_checklist.md"
Assert-Contains -Path $checklistPath -ExpectedText "cli-rewrite" -Label "review_checklist.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "## Verdict" -Label "final_review.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "api-sample:" -Label "final_review.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "cli-rewrite:" -Label "final_review.md"

Write-Host "Section page setup regression bundle structure passed."
