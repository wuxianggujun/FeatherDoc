param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$installPrefix = Join-Path $resolvedWorkingDir "build-msvc-install"
$summaryOutputDir = Join-Path $resolvedWorkingDir "output\release-candidate-checks-ci"
$reportDir = Join-Path $summaryOutputDir "report"
$outputRoot = Join-Path $resolvedWorkingDir "release-assets-ci"

New-Item -ItemType Directory -Path (Join-Path $installPrefix "share\FeatherDoc") -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

$startHerePath = Join-Path $summaryOutputDir "START_HERE.md"
$releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
$releaseBodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$releaseSummaryPath = Join-Path $reportDir "release_summary.zh-CN.md"
$artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$summaryPath = Join-Path $reportDir "summary.json"

Set-Content -LiteralPath $startHerePath -Encoding UTF8 -Value @"
# START_HERE

- CI summary root: $resolvedRepoRoot\output\release-candidate-checks-ci\report
"@

foreach ($filePath in @($releaseHandoffPath, $releaseBodyPath, $releaseSummaryPath, $artifactGuidePath, $reviewerChecklistPath)) {
    Set-Content -LiteralPath $filePath -Encoding UTF8 -Value @"
# Release Material

- Repo path: $resolvedRepoRoot\output\release-candidate-checks-ci\report
- Visual gate: skipped
"@
}

$summary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    install_dir = $installPrefix
    gate_output_dir = (Join-Path $resolvedWorkingDir "output\word-visual-release-gate")
    release_version = "1.6.4"
    execution_status = "pass"
    start_here = $startHerePath
    release_handoff = $releaseHandoffPath
    release_body_zh_cn = $releaseBodyPath
    release_summary_zh_cn = $releaseSummaryPath
    artifact_guide = $artifactGuidePath
    reviewer_checklist = $reviewerChecklistPath
    readme_gallery = [ordered]@{
        status = "visual_gate_skipped"
        assets_dir = (Join-Path $resolvedRepoRoot "docs\assets\readme")
    }
    steps = [ordered]@{
        install_smoke = [ordered]@{
            status = "completed"
            install_prefix = $installPrefix
        }
        visual_gate = [ordered]@{
            status = "skipped"
        }
    }
}
($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$packageScript = Join-Path $resolvedRepoRoot "scripts\package_release_assets.ps1"
& $packageScript `
    -SummaryJson $summaryPath `
    -OutputRoot $outputRoot `
    -AllowIncomplete `
    -KeepStaging

$stagingRoot = Join-Path $outputRoot "v1.6.4\staging"
$placeholderPath = Join-Path $stagingRoot "word-visual-release-gate\README.md"
$manifestPath = Join-Path $outputRoot "v1.6.4\release_assets_manifest.json"
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json

if (-not (Test-Path -LiteralPath $placeholderPath)) {
    throw "Expected incomplete visual-gate placeholder note was not created."
}

$placeholderContent = Get-Content -Raw -LiteralPath $placeholderPath
if ($placeholderContent -notmatch 'visual_gate=skipped') {
    throw "Incomplete visual-gate placeholder note does not mention skipped visual gate."
}

if ($manifest.visual_gate_status -ne "skipped") {
    throw "Release assets manifest did not record visual_gate_status=skipped."
}

if ([bool]$manifest.visual_gate_evidence_included) {
    throw "Release assets manifest incorrectly reported visual gate evidence as included."
}

Write-Host "Package release assets allow-incomplete regression passed."
