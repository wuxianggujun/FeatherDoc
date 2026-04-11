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
$regressionScript = Join-Path $resolvedRepoRoot "scripts\run_fixed_grid_merge_unmerge_regression.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

& $regressionScript `
    -BuildDir $resolvedBuildDir `
    -OutputDir $outputDir `
    -SkipBuild `
    -SkipVisual

if ($LASTEXITCODE -ne 0) {
    throw "Fixed-grid regression bundle generation failed."
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
    [ordered]@{ id = "merge-right"; docx = "merge_right_fixed_grid.docx" },
    [ordered]@{ id = "merge-down"; docx = "merge_down_fixed_grid.docx" },
    [ordered]@{ id = "unmerge-right"; docx = "unmerge_right_fixed_grid.docx" },
    [ordered]@{ id = "unmerge-down"; docx = "unmerge_down_fixed_grid.docx" }
)

Assert-True -Condition ($summary.cases.Count -eq $expectedCases.Count) `
    -Message "summary.json case count mismatch."
Assert-True -Condition ($manifest.cases.Count -eq $expectedCases.Count) `
    -Message "review_manifest.json case count mismatch."

for ($index = 0; $index -lt $expectedCases.Count; $index++) {
    $expectedCase = $expectedCases[$index]
    $summaryCase = $summary.cases[$index]
    $manifestCase = $manifest.cases[$index]
    $expectedDocxPath = [System.IO.Path]::GetFullPath(
        (Join-Path $outputDir (Join-Path $expectedCase.id $expectedCase.docx))
    )
    $visualDir = Join-Path $outputDir (Join-Path $expectedCase.id "visual")

    Assert-True -Condition ($summaryCase.id -eq $expectedCase.id) `
        -Message "Unexpected summary case order at index $index."
    Assert-True -Condition ($manifestCase.id -eq $expectedCase.id) `
        -Message "Unexpected manifest case order at index $index."
    Assert-True -Condition ([System.IO.Path]::GetFullPath($summaryCase.docx_path) -eq $expectedDocxPath) `
        -Message "summary.json recorded an unexpected DOCX path for case '$($expectedCase.id)'."
    Assert-True -Condition ([System.IO.Path]::GetFullPath($manifestCase.docx_path) -eq $expectedDocxPath) `
        -Message "review_manifest.json recorded an unexpected DOCX path for case '$($expectedCase.id)'."
    Assert-True -Condition (Test-Path -LiteralPath $expectedDocxPath) `
        -Message "Expected DOCX is missing for case '$($expectedCase.id)': $expectedDocxPath"
    Assert-DocxCoreEntries -Path $expectedDocxPath

    Assert-True -Condition ($null -eq $summaryCase.visual_output_dir) `
        -Message "summary.json unexpectedly recorded a visual output directory for case '$($expectedCase.id)'."
    Assert-True -Condition (-not (Test-Path -LiteralPath $visualDir)) `
        -Message "Visual directory should not exist when -SkipVisual is used: $visualDir"
    Assert-True -Condition ($summaryCase.PSObject.Properties.Name -notcontains "visual_artifacts") `
        -Message "summary.json unexpectedly recorded visual artifacts for case '$($expectedCase.id)'."
}

Assert-Contains -Path $checklistPath -ExpectedText "merge-right" -Label "review_checklist.md"
Assert-Contains -Path $checklistPath -ExpectedText "merge-down" -Label "review_checklist.md"
Assert-Contains -Path $checklistPath -ExpectedText "unmerge-right" -Label "review_checklist.md"
Assert-Contains -Path $checklistPath -ExpectedText "unmerge-down" -Label "review_checklist.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "## Verdict" -Label "final_review.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "merge-right:" -Label "final_review.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "unmerge-down:" -Label "final_review.md"

Write-Host "Fixed-grid regression bundle structure passed."
