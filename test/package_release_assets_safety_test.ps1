param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

function Assert-NotContains {
    param(
        [string]$Path,
        [string]$UnexpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if (-not [string]::IsNullOrWhiteSpace($UnexpectedText) -and
        $content -match [regex]::Escape($UnexpectedText)) {
        throw "$Label unexpectedly contains '$UnexpectedText': $Path"
    }
}

function Assert-ZipContainsEntry {
    param(
        [string]$ZipPath,
        [string]$ExpectedEntryName
    )

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $normalizedExpectedEntryName = $ExpectedEntryName.Replace('\', '/')
    $zip = [System.IO.Compression.ZipFile]::OpenRead($ZipPath)
    try {
        foreach ($entry in $zip.Entries) {
            if ($entry.FullName.Replace('\', '/') -eq $normalizedExpectedEntryName) {
                return
            }
        }
    }
    finally {
        $zip.Dispose()
    }

    throw "Expected ZIP archive '$ZipPath' to contain '$ExpectedEntryName'."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$workingDirParent = Split-Path -Parent $resolvedWorkingDir
if (-not [string]::IsNullOrWhiteSpace($workingDirParent)) {
    New-Item -ItemType Directory -Path $workingDirParent -Force | Out-Null
}
if (Test-Path -LiteralPath $resolvedWorkingDir) {
    Remove-Item -LiteralPath $resolvedWorkingDir -Recurse -Force
}
$null = New-Item -ItemType Directory -Path $resolvedWorkingDir -Force
$relativeWorkingDir = $resolvedWorkingDir.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
$installPrefix = Join-Path $resolvedWorkingDir "build-msvc-install"
$summaryOutputDir = Join-Path $resolvedWorkingDir "output\release-candidate-checks"
$reportDir = Join-Path $summaryOutputDir "report"
$gateOutputDir = Join-Path $resolvedWorkingDir "output\word-visual-release-gate"
$gateReportDir = Join-Path $gateOutputDir "report"
$smokeEvidenceDir = Join-Path $gateOutputDir "smoke\evidence"
$fixedGridAggregateDir = Join-Path $gateOutputDir "fixed-grid\aggregate-evidence"
$sectionPageSetupAggregateDir = Join-Path $gateOutputDir "section-page-setup\aggregate-evidence"
$outputRoot = Join-Path $resolvedWorkingDir "release-assets"

New-Item -ItemType Directory -Path (Join-Path $installPrefix "share\FeatherDoc") -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $smokeEvidenceDir -Force | Out-Null
New-Item -ItemType Directory -Path $fixedGridAggregateDir -Force | Out-Null
New-Item -ItemType Directory -Path $sectionPageSetupAggregateDir -Force | Out-Null

$startHerePath = Join-Path $summaryOutputDir "START_HERE.md"
$releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
$releaseBodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$releaseSummaryPath = Join-Path $reportDir "release_summary.zh-CN.md"
$artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$summaryPath = Join-Path $reportDir "summary.json"
$installedReadmePath = Join-Path $installPrefix "share\FeatherDoc\README.md"
$installedChangelogPath = Join-Path $installPrefix "share\FeatherDoc\CHANGELOG.md"
$installedPdfImportDocsPath = Join-Path $installPrefix "share\FeatherDoc\docs\pdf_import.rst"
$installedPdfImportJsonDiagnosticsDocsPath = Join-Path $installPrefix "share\FeatherDoc\docs\pdf_import_json_diagnostics.rst"
$installedPdfImportScopeDocsPath = Join-Path $installPrefix "share\FeatherDoc\docs\pdf_import_scope.rst"

Set-Content -LiteralPath $startHerePath -Encoding UTF8 -Value @"
# START_HERE

- Evidence root: $resolvedRepoRoot\output\release-candidate-checks\report
"@

Set-Content -LiteralPath $releaseHandoffPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4

- Handoff path: $resolvedRepoRoot\output\release-candidate-checks\report\release_handoff.md
"@

Set-Content -LiteralPath $releaseBodyPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 发布说明

- 证据目录：$resolvedRepoRoot\output\word-visual-release-gate\report
"@

Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 发布摘要

- 摘要路径：$resolvedRepoRoot\output\release-candidate-checks\report\release_summary.zh-CN.md
"@

Set-Content -LiteralPath $artifactGuidePath -Encoding UTF8 -Value @"
# Artifact Guide

- Report root: $resolvedRepoRoot\output\release-candidate-checks\report
"@

Set-Content -LiteralPath $reviewerChecklistPath -Encoding UTF8 -Value @"
# Reviewer Checklist

- Review root: $resolvedRepoRoot\output\word-visual-release-gate\report
"@

Set-Content -LiteralPath $gateFinalReviewPath -Encoding UTF8 -Value @"
# Gate Final Review

- Local gate report: $resolvedRepoRoot\output\word-visual-release-gate\report
"@

Set-Content -LiteralPath $installedReadmePath -Encoding UTF8 -Value @'
# README

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare_word_review_task.ps1 `
    -DocxPath C:\path\to\target.docx `
    -Mode review-only
```
'@

Set-Content -LiteralPath $installedChangelogPath -Encoding UTF8 -Value @'
# Changelog

- Removed remaining public-facing "draft" wording from release docs.
- The release-note drafting helper still keeps the old draft boilerplate.
- `-Publish` flips `draft=false` after final signoff.
- Normalize `C:\Users\someone\workspace` before public release.
'@

New-Item -ItemType Directory -Path (Split-Path -Parent $installedPdfImportDocsPath) -Force | Out-Null
Set-Content -LiteralPath $installedPdfImportDocsPath -Encoding UTF8 -Value @'
PDF Import
==========

Package copy for release asset packaging regression.

- PDF Import JSON Diagnostics (:doc:`pdf_import_json_diagnostics`)
- PDF Import Supported Scope And Limits (:doc:`pdf_import_scope`)
'@
Set-Content -LiteralPath $installedPdfImportJsonDiagnosticsDocsPath -Encoding UTF8 -Value @'
PDF Import JSON Diagnostics
===========================

Command-line parse errors
-------------------------

Package copy for release asset packaging regression.
'@
Set-Content -LiteralPath $installedPdfImportScopeDocsPath -Encoding UTF8 -Value @'
PDF Import Supported Scope And Limits
=====================================

PDF import supported scope and limits.

Package copy for release asset packaging regression.
'@

Set-Content -LiteralPath (Join-Path $smokeEvidenceDir "README.md") -Encoding UTF8 -Value @"
# Smoke Evidence

- Local smoke evidence: $resolvedRepoRoot\output\word-visual-release-gate\smoke\evidence
"@

Set-Content -LiteralPath (Join-Path $fixedGridAggregateDir "README.md") -Encoding UTF8 -Value @"
# Fixed Grid Evidence

- Local aggregate evidence: $resolvedRepoRoot\output\word-visual-release-gate\fixed-grid\aggregate-evidence
"@

Set-Content -LiteralPath (Join-Path $sectionPageSetupAggregateDir "README.md") -Encoding UTF8 -Value @"
# Section Page Setup Evidence

- Local aggregate evidence: $resolvedRepoRoot\output\word-visual-release-gate\section-page-setup\aggregate-evidence
"@

$gateSummary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    gate_output_dir = $gateOutputDir
    report_dir = $gateReportDir
    execution_status = "pass"
    visual_verdict = "pass"
    smoke = [ordered]@{
        evidence_dir = $smokeEvidenceDir
    }
    fixed_grid = [ordered]@{
        aggregate_evidence_dir = $fixedGridAggregateDir
    }
}
($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

$summary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    build_dir = (Join-Path $resolvedWorkingDir "build-msvc-nmake")
    install_dir = $installPrefix
    consumer_build_dir = (Join-Path $resolvedWorkingDir "build-msvc-install-consumer")
    gate_output_dir = $gateOutputDir
    task_output_root = (Join-Path $resolvedWorkingDir "output\word-visual-smoke\tasks-release-gate")
    release_version = "1.6.4"
    execution_status = "pass"
    visual_verdict = "pass"
    release_handoff = $releaseHandoffPath
    release_body_zh_cn = $releaseBodyPath
    release_summary_zh_cn = $releaseSummaryPath
    artifact_guide = $artifactGuidePath
    reviewer_checklist = $reviewerChecklistPath
    start_here = $startHerePath
    readme_gallery = [ordered]@{
        status = "completed"
        assets_dir = (Join-Path $resolvedRepoRoot "docs\assets\readme")
    }
    steps = [ordered]@{
        install_smoke = [ordered]@{
            status = "completed"
            install_prefix = $installPrefix
            consumer_document = (Join-Path $resolvedWorkingDir "build-msvc-install-consumer\featherdoc-install-smoke.docx")
        }
        visual_gate = [ordered]@{
            status = "completed"
            summary_json = $gateSummaryPath
            final_review = $gateFinalReviewPath
            visual_verdict = "pass"
        }
    }
}
($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$packageScript = Join-Path $resolvedRepoRoot "scripts\package_release_assets.ps1"
& $packageScript `
    -SummaryJson $summaryPath `
    -OutputRoot $outputRoot `
    -KeepStaging

$stagingRoot = Join-Path $outputRoot "v1.6.4\staging"
$stagedSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\summary.json"
$stagedGateSummaryPath = Join-Path $stagingRoot "word-visual-release-gate\report\gate_summary.json"
$stagedHandoffPath = Join-Path $stagingRoot "release-candidate-checks\report\release_handoff.md"
$stagedInstalledReadmePath = Join-Path $stagingRoot "build-msvc-install\share\FeatherDoc\README.md"
$stagedInstalledChangelogPath = Join-Path $stagingRoot "build-msvc-install\share\FeatherDoc\CHANGELOG.md"
$stagedInstalledPdfImportDocsPath = Join-Path $stagingRoot "build-msvc-install\share\FeatherDoc\docs\pdf_import.rst"
$stagedInstalledPdfImportJsonDiagnosticsDocsPath = Join-Path $stagingRoot "build-msvc-install\share\FeatherDoc\docs\pdf_import_json_diagnostics.rst"
$stagedInstalledPdfImportScopeDocsPath = Join-Path $stagingRoot "build-msvc-install\share\FeatherDoc\docs\pdf_import_scope.rst"
$installZipPath = Join-Path $outputRoot "v1.6.4\FeatherDoc-v1.6.4-msvc-install.zip"
$galleryZipPath = Join-Path $outputRoot "v1.6.4\FeatherDoc-v1.6.4-visual-validation-gallery.zip"
$evidenceZipPath = Join-Path $outputRoot "v1.6.4\FeatherDoc-v1.6.4-release-evidence.zip"
$expectedRelativeHandoff = ".\$relativeWorkingDir\output\release-candidate-checks\report\release_handoff.md"
$expectedRelativeGateReport = ".\$relativeWorkingDir\output\word-visual-release-gate\report"
$stagedSummary = Get-Content -Raw -LiteralPath $stagedSummaryPath | ConvertFrom-Json
$stagedGateSummary = Get-Content -Raw -LiteralPath $stagedGateSummaryPath | ConvertFrom-Json

Assert-NotContains -Path $stagedSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged summary.json'
Assert-NotContains -Path $stagedGateSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged gate_summary.json'
Assert-NotContains -Path $stagedHandoffPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_handoff.md'
Assert-NotContains -Path $stagedInstalledReadmePath -UnexpectedText 'C:\path\to\target.docx' -Label 'staged installed README.md'
Assert-NotContains -Path $stagedInstalledChangelogPath -UnexpectedText 'draft' -Label 'staged installed CHANGELOG.md'
Assert-NotContains -Path $stagedInstalledChangelogPath -UnexpectedText 'C:\Users\someone\workspace' -Label 'staged installed CHANGELOG.md'
Assert-Contains -Path $stagedInstalledReadmePath -ExpectedText '<windows-absolute-path>' -Label 'staged installed README.md'
Assert-Contains -Path $stagedInstalledChangelogPath -ExpectedText 'preview' -Label 'staged installed CHANGELOG.md'
Assert-Contains -Path $stagedInstalledChangelogPath -ExpectedText '<windows-absolute-path>' -Label 'staged installed CHANGELOG.md'
Assert-Contains -Path $stagedInstalledPdfImportDocsPath -ExpectedText 'PDF Import' -Label 'staged installed PDF import docs'
Assert-Contains -Path $stagedInstalledPdfImportDocsPath -ExpectedText 'pdf_import_json_diagnostics' -Label 'staged installed PDF import docs'
Assert-Contains -Path $stagedInstalledPdfImportDocsPath -ExpectedText 'pdf_import_scope' -Label 'staged installed PDF import docs'
Assert-Contains -Path $stagedInstalledPdfImportJsonDiagnosticsDocsPath -ExpectedText 'PDF Import JSON Diagnostics' -Label 'staged installed PDF import JSON diagnostics docs'
Assert-Contains -Path $stagedInstalledPdfImportJsonDiagnosticsDocsPath -ExpectedText 'Command-line parse errors' -Label 'staged installed PDF import JSON diagnostics docs'
Assert-Contains -Path $stagedInstalledPdfImportScopeDocsPath -ExpectedText 'PDF Import Supported Scope And Limits' -Label 'staged installed PDF import scope docs'
Assert-Contains -Path $stagedInstalledPdfImportScopeDocsPath -ExpectedText 'PDF import supported scope and limits' -Label 'staged installed PDF import scope docs'
if ($stagedSummary.release_handoff -ne $expectedRelativeHandoff) {
    throw "staged summary.json did not rewrite release_handoff to the expected relative path."
}
if ($stagedGateSummary.report_dir -ne $expectedRelativeGateReport) {
    throw "staged gate_summary.json did not rewrite report_dir to the expected relative path."
}

foreach ($zipPath in @($installZipPath, $galleryZipPath, $evidenceZipPath)) {
    if (-not (Test-Path -LiteralPath $zipPath)) {
        throw "Expected ZIP archive was not created: $zipPath"
    }
}

Assert-ZipContainsEntry `
    -ZipPath $installZipPath `
    -ExpectedEntryName 'build-msvc-install/share/FeatherDoc/docs/pdf_import.rst'
Assert-ZipContainsEntry `
    -ZipPath $installZipPath `
    -ExpectedEntryName 'build-msvc-install/share/FeatherDoc/docs/pdf_import_json_diagnostics.rst'
Assert-ZipContainsEntry `
    -ZipPath $installZipPath `
    -ExpectedEntryName 'build-msvc-install/share/FeatherDoc/docs/pdf_import_scope.rst'

Write-Host "Package release assets safety regression passed."
