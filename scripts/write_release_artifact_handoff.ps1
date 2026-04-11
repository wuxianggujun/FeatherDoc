param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$OutputPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-FullPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    return [System.IO.Path]::GetFullPath($candidate)
}

function Get-OptionalPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return ""
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return ""
    }

    if ($null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Get-OptionalPropertyObject {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Get-DisplayValue {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "(not available)"
    }

    return $Value
}

function Get-ProjectVersion {
    param([string]$RepoRoot)

    $cmakePath = Join-Path $RepoRoot "CMakeLists.txt"
    if (-not (Test-Path -LiteralPath $cmakePath)) {
        return ""
    }

    $content = Get-Content -Raw $cmakePath
    $match = [regex]::Match($content, 'VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)')
    if (-not $match.Success) {
        return ""
    }

    return $match.Groups[1].Value
}

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
if (-not (Test-Path -LiteralPath $resolvedSummaryPath)) {
    throw "Summary JSON does not exist: $resolvedSummaryPath"
}

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path (Split-Path -Parent $resolvedSummaryPath) "release_handoff.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputPath
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$projectVersion = Get-ProjectVersion -RepoRoot $repoRoot
$reportDir = Split-Path -Parent $resolvedSummaryPath
$finalReviewPath = Join-Path $reportDir "final_review.md"
$releaseBodyPath = Get-OptionalPropertyValue -Object $summary -Name "release_body_zh_cn"
if ([string]::IsNullOrWhiteSpace($releaseBodyPath)) {
    $releaseBodyPath = Join-Path $reportDir "release_body.zh-CN.md"
}
$releaseSummaryPath = Get-OptionalPropertyValue -Object $summary -Name "release_summary_zh_cn"
if ([string]::IsNullOrWhiteSpace($releaseSummaryPath)) {
    $releaseSummaryPath = Join-Path $reportDir "release_summary.zh-CN.md"
}
$artifactGuidePath = Get-OptionalPropertyValue -Object $summary -Name "artifact_guide"
if ([string]::IsNullOrWhiteSpace($artifactGuidePath)) {
    $artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
}
$reviewerChecklistPath = Get-OptionalPropertyValue -Object $summary -Name "reviewer_checklist"
if ([string]::IsNullOrWhiteSpace($reviewerChecklistPath)) {
    $reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
}

$installPrefix = Get-OptionalPropertyValue -Object $summary.steps.install_smoke -Name "install_prefix"
$consumerDocument = Get-OptionalPropertyValue -Object $summary.steps.install_smoke -Name "consumer_document"
$gateSummaryPath = Get-OptionalPropertyValue -Object $summary.steps.visual_gate -Name "summary_json"
$gateFinalReviewPath = Get-OptionalPropertyValue -Object $summary.steps.visual_gate -Name "final_review"
$documentTaskDir = Get-OptionalPropertyValue -Object $summary.steps.visual_gate -Name "document_task_dir"
$fixedGridTaskDir = Get-OptionalPropertyValue -Object $summary.steps.visual_gate -Name "fixed_grid_task_dir"

$visualVerdict = ""
$readmeGalleryStatus = ""
$readmeGalleryAssetsDir = ""
if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath) -and (Test-Path -LiteralPath $gateSummaryPath)) {
    $gateSummary = Get-Content -Raw $gateSummaryPath | ConvertFrom-Json
    $visualVerdict = Get-OptionalPropertyValue -Object $gateSummary -Name "visual_verdict"
    $readmeGallery = Get-OptionalPropertyObject -Object $gateSummary -Name "readme_gallery"
    $readmeGalleryStatus = Get-OptionalPropertyValue -Object $readmeGallery -Name "status"
    $readmeGalleryAssetsDir = Get-OptionalPropertyValue -Object $readmeGallery -Name "assets_dir"
}

$installedDataDir = ""
$installedQuickstartEn = ""
$installedQuickstartZh = ""
$installedTemplateEn = ""
$installedTemplateZh = ""

if (-not [string]::IsNullOrWhiteSpace($installPrefix)) {
    $installedDataDir = Join-Path $installPrefix "share\FeatherDoc"
    $installedQuickstartEn = Join-Path $installedDataDir "VISUAL_VALIDATION_QUICKSTART.md"
    $installedQuickstartZh = Join-Path $installedDataDir "VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
    $installedTemplateEn = Join-Path $installedDataDir "RELEASE_ARTIFACT_TEMPLATE.md"
    $installedTemplateZh = Join-Path $installedDataDir "RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
}

$syncLatestCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1"
$syncExplicitCommand = ""
if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath)) {
    $syncExplicitCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\sync_visual_review_verdict.ps1 -GateSummaryJson "{0}" -ReleaseCandidateSummaryJson "{1}" -RefreshReleaseBundle' -f `
        $gateSummaryPath, $resolvedSummaryPath
}
$releaseGateCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1"
$releaseChecksCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1"
$openDocumentTaskCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1"
$openFixedGridTaskCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_fixed_grid_review_task.ps1 -PrintPrompt"

$handoffLines = New-Object 'System.Collections.Generic.List[string]'

[void]$handoffLines.Add("# Release Artifact Handoff")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("- Generated at: $(Get-Date -Format s)")
[void]$handoffLines.Add("- Project version: $(Get-DisplayValue -Value $projectVersion)")
[void]$handoffLines.Add("- Release candidate summary: $resolvedSummaryPath")
[void]$handoffLines.Add("- Release candidate final review: $(Get-DisplayValue -Value $finalReviewPath)")
[void]$handoffLines.Add("- Release handoff: $resolvedOutputPath")
[void]$handoffLines.Add("- Release body draft: $(Get-DisplayValue -Value $releaseBodyPath)")
[void]$handoffLines.Add("- Release summary draft: $(Get-DisplayValue -Value $releaseSummaryPath)")
[void]$handoffLines.Add("- Artifact guide: $(Get-DisplayValue -Value $artifactGuidePath)")
[void]$handoffLines.Add("- Reviewer checklist: $(Get-DisplayValue -Value $reviewerChecklistPath)")
[void]$handoffLines.Add("- Execution status: $($summary.execution_status)")
[void]$handoffLines.Add("- Visual verdict: $(Get-DisplayValue -Value $visualVerdict)")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Verification Snapshot")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("- Configure: $($summary.steps.configure.status)")
[void]$handoffLines.Add("- Build: $($summary.steps.build.status)")
[void]$handoffLines.Add("- Tests: $($summary.steps.tests.status)")
[void]$handoffLines.Add("- Install smoke: $($summary.steps.install_smoke.status)")
[void]$handoffLines.Add("- Visual gate: $($summary.steps.visual_gate.status)")
[void]$handoffLines.Add("- README gallery refresh: $(Get-DisplayValue -Value $readmeGalleryStatus)")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Installed Package Entry Points")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("- Installed data dir: $(Get-DisplayValue -Value $installedDataDir)")
[void]$handoffLines.Add("- Quickstart (EN): $(Get-DisplayValue -Value $installedQuickstartEn)")
[void]$handoffLines.Add("- Quickstart (ZH-CN): $(Get-DisplayValue -Value $installedQuickstartZh)")
[void]$handoffLines.Add("- Release template (EN): $(Get-DisplayValue -Value $installedTemplateEn)")
[void]$handoffLines.Add("- Release template (ZH-CN): $(Get-DisplayValue -Value $installedTemplateZh)")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Evidence Files")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("- Consumer smoke document: $(Get-DisplayValue -Value $consumerDocument)")
[void]$handoffLines.Add("- Release body draft: $(Get-DisplayValue -Value $releaseBodyPath)")
[void]$handoffLines.Add("- Release summary draft: $(Get-DisplayValue -Value $releaseSummaryPath)")
[void]$handoffLines.Add("- Artifact guide: $(Get-DisplayValue -Value $artifactGuidePath)")
[void]$handoffLines.Add("- Reviewer checklist: $(Get-DisplayValue -Value $reviewerChecklistPath)")
[void]$handoffLines.Add("- Visual gate summary: $(Get-DisplayValue -Value $gateSummaryPath)")
[void]$handoffLines.Add("- Visual gate final review: $(Get-DisplayValue -Value $gateFinalReviewPath)")
[void]$handoffLines.Add("- README gallery assets: $(Get-DisplayValue -Value $readmeGalleryAssetsDir)")
[void]$handoffLines.Add("- Document review task: $(Get-DisplayValue -Value $documentTaskDir)")
[void]$handoffLines.Add("- Fixed-grid review task: $(Get-DisplayValue -Value $fixedGridTaskDir)")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Reproduction Commands")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```powershell')
[void]$handoffLines.Add($releaseChecksCommand)
[void]$handoffLines.Add($releaseGateCommand)
[void]$handoffLines.Add($openDocumentTaskCommand)
[void]$handoffLines.Add($openFixedGridTaskCommand)
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add("If the visual verdict changes after screenshot-backed manual review, rerun the shortest verdict sync first:")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```powershell')
[void]$handoffLines.Add($syncLatestCommand)
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add("That command auto-detects the latest review task, syncs the final verdict back into the gate summary, and refreshes the detected release bundle.")
[void]$handoffLines.Add("")
if (-not [string]::IsNullOrWhiteSpace($syncExplicitCommand)) {
    [void]$handoffLines.Add("If the inferred gate or release paths need to be overridden manually, use:")
    [void]$handoffLines.Add("")
    [void]$handoffLines.Add('```powershell')
    [void]$handoffLines.Add($syncExplicitCommand)
    [void]$handoffLines.Add('```')
    [void]$handoffLines.Add("")
}
[void]$handoffLines.Add("## Release Body Seed")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```md')
[void]$handoffLines.Add("# FeatherDoc v$(if ($projectVersion) { $projectVersion } else { '<version>' })")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Highlights")
[void]$handoffLines.Add("- Fill from CHANGELOG.md.")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Validation")
[void]$handoffLines.Add("- MSVC configure/build: $($summary.steps.configure.status) / $($summary.steps.build.status)")
[void]$handoffLines.Add("- ctest: $($summary.steps.tests.status)")
[void]$handoffLines.Add("- install + find_package smoke: $($summary.steps.install_smoke.status)")
[void]$handoffLines.Add("- Word visual release gate: $($summary.steps.visual_gate.status)")
[void]$handoffLines.Add("- Visual verdict: $(if ($visualVerdict) { $visualVerdict } else { '<pass|fail|pending_manual_review>' })")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Installed Package Entry Points")
[void]$handoffLines.Add("- $(if ($installedQuickstartZh) { $installedQuickstartZh } else { 'share/FeatherDoc/VISUAL_VALIDATION_QUICKSTART.zh-CN.md' })")
[void]$handoffLines.Add("- $(if ($installedTemplateZh) { $installedTemplateZh } else { 'share/FeatherDoc/RELEASE_ARTIFACT_TEMPLATE.zh-CN.md' })")
[void]$handoffLines.Add("- $(if ($installedDataDir) { Join-Path $installedDataDir 'visual-validation' } else { 'share/FeatherDoc/visual-validation' })")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Evidence Files")
[void]$handoffLines.Add("- $resolvedSummaryPath")
[void]$handoffLines.Add("- $(if (Test-Path -LiteralPath $finalReviewPath) { $finalReviewPath } else { 'output/release-candidate-checks/report/final_review.md' })")
[void]$handoffLines.Add("- $resolvedOutputPath")
[void]$handoffLines.Add("- $(if ($releaseBodyPath) { $releaseBodyPath } else { 'output/release-candidate-checks/report/release_body.zh-CN.md' })")
[void]$handoffLines.Add("- $(if ($releaseSummaryPath) { $releaseSummaryPath } else { 'output/release-candidate-checks/report/release_summary.zh-CN.md' })")
[void]$handoffLines.Add("- $(if ($artifactGuidePath) { $artifactGuidePath } else { 'output/release-candidate-checks/report/ARTIFACT_GUIDE.md' })")
[void]$handoffLines.Add("- $(if ($reviewerChecklistPath) { $reviewerChecklistPath } else { 'output/release-candidate-checks/report/REVIEWER_CHECKLIST.md' })")
[void]$handoffLines.Add("- $(if ($gateSummaryPath) { $gateSummaryPath } else { 'output/word-visual-release-gate/report/gate_summary.json' })")
[void]$handoffLines.Add("- $(if ($gateFinalReviewPath) { $gateFinalReviewPath } else { 'output/word-visual-release-gate/report/gate_final_review.md' })")
[void]$handoffLines.Add('```')

$handoff = $handoffLines -join [Environment]::NewLine

$handoff | Set-Content -Path $resolvedOutputPath -Encoding UTF8

Write-Host "Release handoff: $resolvedOutputPath"
