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

function Get-DocxFieldCounts {
    param([string]$Path)

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $pageFieldCount = 0
    $totalPagesFieldCount = 0
    $archive = [System.IO.Compression.ZipFile]::OpenRead($Path)
    try {
        foreach ($entry in $archive.Entries) {
            if ($entry.FullName -ne "word/document.xml" -and
                $entry.FullName -notmatch "^word/header\d+\.xml$" -and
                $entry.FullName -notmatch "^word/footer\d+\.xml$") {
                continue
            }

            $reader = [System.IO.StreamReader]::new($entry.Open())
            try {
                $content = $reader.ReadToEnd()
            } finally {
                $reader.Dispose()
            }

            $pageFieldCount += ([regex]::Matches(
                    $content,
                    'w:instr="[^"]*\bPAGE\b[^"]*"')).Count
            $totalPagesFieldCount += ([regex]::Matches(
                    $content,
                    'w:instr="[^"]*\bNUMPAGES\b[^"]*"')).Count
        }
    } finally {
        $archive.Dispose()
    }

    return [ordered]@{
        page = $pageFieldCount
        total_pages = $totalPagesFieldCount
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$outputDir = Join-Path $resolvedWorkingDir "bundle_output"
$regressionScript = Join-Path $resolvedRepoRoot "scripts\run_page_number_fields_regression.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

& $regressionScript `
    -BuildDir $resolvedBuildDir `
    -OutputDir $outputDir `
    -SkipBuild `
    -SkipVisual

if ($LASTEXITCODE -ne 0) {
    throw "Page number fields regression bundle generation failed."
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
    [ordered]@{
        id = "api-sample"
        docx = "page_number_fields.docx"
        field_summary = "field_summary.json"
        page_count = 1
        total_pages_count = 1
    },
    [ordered]@{
        id = "cli-append"
        docx = "page_number_fields_cli.docx"
        field_summary = "field_summary.json"
        page_count = 2
        total_pages_count = 2
    }
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
    $expectedFieldSummaryPath = [System.IO.Path]::GetFullPath((Join-Path $caseDir $expectedCase.field_summary))

    Assert-True -Condition ($summaryCase.id -eq $expectedCase.id) `
        -Message "Unexpected summary case order at index $index."
    Assert-True -Condition ($manifestCase.id -eq $expectedCase.id) `
        -Message "Unexpected manifest case order at index $index."
    Assert-True -Condition ([System.IO.Path]::GetFullPath($summaryCase.docx_path) -eq $expectedDocxPath) `
        -Message "summary.json recorded an unexpected DOCX path for case '$($expectedCase.id)'."
    Assert-True -Condition ([System.IO.Path]::GetFullPath($manifestCase.docx_path) -eq $expectedDocxPath) `
        -Message "review_manifest.json recorded an unexpected DOCX path for case '$($expectedCase.id)'."
    Assert-True -Condition ([System.IO.Path]::GetFullPath($summaryCase.field_summary_json) -eq $expectedFieldSummaryPath) `
        -Message "summary.json recorded an unexpected field summary path for case '$($expectedCase.id)'."
    Assert-True -Condition (Test-Path -LiteralPath $expectedDocxPath) `
        -Message "Expected DOCX is missing for case '$($expectedCase.id)': $expectedDocxPath"
    Assert-True -Condition (Test-Path -LiteralPath $expectedFieldSummaryPath) `
        -Message "Expected field summary JSON is missing for case '$($expectedCase.id)': $expectedFieldSummaryPath"
    Assert-DocxCoreEntries -Path $expectedDocxPath

    Assert-True -Condition ($null -eq $summaryCase.visual_output_dir) `
        -Message "summary.json unexpectedly recorded a visual output directory for case '$($expectedCase.id)'."

    $fieldSummary = Get-Content -Raw -LiteralPath $expectedFieldSummaryPath | ConvertFrom-Json
    Assert-True -Condition ($fieldSummary.total_page_field_count -eq $expectedCase.page_count) `
        -Message "Unexpected PAGE field count in field summary for case '$($expectedCase.id)'."
    Assert-True -Condition ($fieldSummary.total_total_pages_field_count -eq $expectedCase.total_pages_count) `
        -Message "Unexpected NUMPAGES field count in field summary for case '$($expectedCase.id)'."

    $docxFieldCounts = Get-DocxFieldCounts -Path $expectedDocxPath
    Assert-True -Condition ($docxFieldCounts.page -eq $expectedCase.page_count) `
        -Message "Unexpected PAGE field count inside DOCX for case '$($expectedCase.id)'."
    Assert-True -Condition ($docxFieldCounts.total_pages -eq $expectedCase.total_pages_count) `
        -Message "Unexpected NUMPAGES field count inside DOCX for case '$($expectedCase.id)'."
}

$cliBaseDocxPath = Join-Path $outputDir "cli-append\section_page_setup_base.docx"
Assert-True -Condition (Test-Path -LiteralPath $cliBaseDocxPath) `
    -Message "Expected CLI base sample DOCX is missing: $cliBaseDocxPath"

$cliMutationFiles = @(
    [ordered]@{
        path = (Join-Path $outputDir "cli-append\append_page_number_section_0.json")
        command = '"command":"append-page-number-field"'
        field = '"field":"page_number"'
    },
    [ordered]@{
        path = (Join-Path $outputDir "cli-append\append_total_pages_section_0.json")
        command = '"command":"append-total-pages-field"'
        field = '"field":"total_pages"'
    },
    [ordered]@{
        path = (Join-Path $outputDir "cli-append\append_page_number_section_1.json")
        command = '"command":"append-page-number-field"'
        field = '"field":"page_number"'
    },
    [ordered]@{
        path = (Join-Path $outputDir "cli-append\append_total_pages_section_1.json")
        command = '"command":"append-total-pages-field"'
        field = '"field":"total_pages"'
    }
)

foreach ($mutationFile in $cliMutationFiles) {
    Assert-True -Condition (Test-Path -LiteralPath $mutationFile.path) `
        -Message "Expected CLI mutation JSON is missing: $($mutationFile.path)"
    Assert-Contains -Path $mutationFile.path -ExpectedText $mutationFile.command -Label ([System.IO.Path]::GetFileName($mutationFile.path))
    Assert-Contains -Path $mutationFile.path -ExpectedText $mutationFile.field -Label ([System.IO.Path]::GetFileName($mutationFile.path))
}

Assert-Contains -Path $checklistPath -ExpectedText "api-sample" -Label "review_checklist.md"
Assert-Contains -Path $checklistPath -ExpectedText "cli-append" -Label "review_checklist.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "## Verdict" -Label "final_review.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "api-sample:" -Label "final_review.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "cli-append:" -Label "final_review.md"

Write-Host "Page number fields regression bundle structure passed."
