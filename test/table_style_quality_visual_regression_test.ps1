param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) { throw "$Message Missing='$ExpectedText'." }
}

function Assert-DocxCoreEntries {
    param([string]$Path)
    Assert-True -Condition (Test-Path -LiteralPath $Path) -Message "DOCX is missing: $Path"
    $archive = [System.IO.Compression.ZipFile]::OpenRead($Path)
    try {
        $entryNames = @($archive.Entries | ForEach-Object { $_.FullName })
        foreach ($requiredEntry in @("[Content_Types].xml", "_rels/.rels", "word/document.xml", "word/styles.xml")) {
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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_table_style_quality_visual_regression.ps1"
$outputDir = Join-Path $resolvedWorkingDir "table-style-quality-visual"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

& $scriptPath `
    -BuildDir $resolvedBuildDir `
    -OutputDir $outputDir `
    -SkipBuild `
    -SkipVisual
if ($LASTEXITCODE -ne 0) {
    throw "Table style quality visual regression script failed."
}

$summaryPath = Join-Path $outputDir "summary.json"
$manifestPath = Join-Path $outputDir "review_manifest.json"
$checklistPath = Join-Path $outputDir "review_checklist.md"
$finalReviewPath = Join-Path $outputDir "final_review.md"
$summaryMarkdownPath = Join-Path $outputDir "summary.md"
$aggregateEvidenceDir = Join-Path $outputDir "aggregate-evidence"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "contact_sheet.png"

foreach ($path in @($summaryPath, $manifestPath, $checklistPath, $finalReviewPath, $summaryMarkdownPath)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) -Message "Expected artifact was not generated: $path"
}

$summary = Get-Content -Raw -LiteralPath $summaryPath | ConvertFrom-Json
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$applyResult = Get-Content -Raw -LiteralPath $summary.apply_result_json | ConvertFrom-Json
$beforeAudit = Get-Content -Raw -LiteralPath $summary.before_audit_json | ConvertFrom-Json
$afterAudit = Get-Content -Raw -LiteralPath $summary.after_audit_json | ConvertFrom-Json
$checklist = Get-Content -Raw -LiteralPath $checklistPath
$finalReview = Get-Content -Raw -LiteralPath $finalReviewPath

Assert-Equal -Actual ([bool]$summary.visual_enabled) -Expected $false `
    -Message "SkipVisual summary should report visual_enabled=false."
Assert-Equal -Actual ([bool]$manifest.visual_enabled) -Expected $false `
    -Message "SkipVisual manifest should report visual_enabled=false."
Assert-True -Condition ($null -eq $summary.visual_artifacts) `
    -Message "SkipVisual summary should not record visual artifacts."
Assert-True -Condition ($null -eq $manifest.aggregate_evidence) `
    -Message "SkipVisual manifest should not record aggregate evidence."
Assert-True -Condition (-not (Test-Path -LiteralPath $aggregateEvidenceDir)) `
    -Message "SkipVisual mode should not generate aggregate visual evidence."
Assert-True -Condition (-not (Test-Path -LiteralPath $aggregateContactSheetPath)) `
    -Message "SkipVisual mode should not generate an aggregate contact sheet."
Assert-Equal -Actual ([int]$summary.before_issue_count) -Expected 1 `
    -Message "Before audit should expose one quality issue."
Assert-Equal -Actual ([int]$summary.after_issue_count) -Expected 0 `
    -Message "After audit should be clean."
Assert-Equal -Actual ([int]$summary.before_automatic_fix_count) -Expected 1 `
    -Message "Before plan should expose one automatic fix."
Assert-Equal -Actual ([int]$summary.before_manual_fix_count) -Expected 0 `
    -Message "Before plan should expose zero manual fixes."
Assert-Equal -Actual ([int]$summary.changed_table_count) -Expected 1 `
    -Message "Look-only apply should change one table."

Assert-Equal -Actual ([string]$applyResult.command) -Expected "apply-table-style-quality-fixes" `
    -Message "Apply result command mismatch."
Assert-Equal -Actual ([bool]$applyResult.ok) -Expected $true `
    -Message "Apply result should be ok."
Assert-Equal -Actual ([int]$beforeAudit.style_look_issue_count) -Expected 1 `
    -Message "Before audit should report one style look issue."
Assert-Equal -Actual ([int]$afterAudit.style_look_issue_count) -Expected 0 `
    -Message "After audit should report zero style look issues."

Assert-DocxCoreEntries -Path $summary.before_docx
Assert-DocxCoreEntries -Path $summary.after_docx
Assert-ContainsText -Text $checklist -ExpectedText "first table row has no dark blue fill" `
    -Message "Review checklist should describe the before visual cue."
Assert-ContainsText -Text $checklist -ExpectedText "first table row has dark blue fill" `
    -Message "Review checklist should describe the after visual cue."
Assert-ContainsText -Text $finalReview -ExpectedText "Visual evidence generated: False" `
    -Message "Final review should record SkipVisual mode."

$scriptText = Get-Content -Raw -LiteralPath $scriptPath
Assert-ContainsText -Text $scriptText -ExpectedText "apply-table-style-quality-fixes" `
    -Message "Regression script should exercise apply-table-style-quality-fixes."
Assert-ContainsText -Text $scriptText -ExpectedText "summarize_table_style_quality_pixels.py" `
    -Message "Regression script should wire the pixel summary helper."

Write-Host "Table style quality visual regression bundle structure passed."
