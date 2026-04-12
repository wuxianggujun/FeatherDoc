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
    if ($null -eq $property -or $null -eq $property.Value) {
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

function Get-DisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    return Get-RepoRelativePath -RepoRoot $RepoRoot -Path $Path
}

function Get-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Add-CheckboxLine {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Text
    )

    [void]$Lines.Add("- [ ] $Text")
}

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
if (-not (Test-Path -LiteralPath $resolvedSummaryPath)) {
    throw "Summary JSON does not exist: $resolvedSummaryPath"
}
$summaryCommandPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedSummaryPath

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path (Split-Path -Parent $resolvedSummaryPath) "REVIEWER_CHECKLIST.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputPath
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$reportDir = Split-Path -Parent $resolvedSummaryPath
$artifactGuidePath = Get-OptionalPropertyValue -Object $summary -Name "artifact_guide"
if ([string]::IsNullOrWhiteSpace($artifactGuidePath)) {
    $artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
}
$releaseHandoffPath = Get-OptionalPropertyValue -Object $summary -Name "release_handoff"
if ([string]::IsNullOrWhiteSpace($releaseHandoffPath)) {
    $releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
}
$releaseBodyPath = Get-OptionalPropertyValue -Object $summary -Name "release_body_zh_cn"
if ([string]::IsNullOrWhiteSpace($releaseBodyPath)) {
    $releaseBodyPath = Join-Path $reportDir "release_body.zh-CN.md"
}
$releaseSummaryPath = Get-OptionalPropertyValue -Object $summary -Name "release_summary_zh_cn"
if ([string]::IsNullOrWhiteSpace($releaseSummaryPath)) {
    $releaseSummaryPath = Join-Path $reportDir "release_summary.zh-CN.md"
}
$finalReviewPath = Join-Path $reportDir "final_review.md"

$installPrefix = Get-OptionalPropertyValue -Object $summary.steps.install_smoke -Name "install_prefix"
$consumerDocument = Get-OptionalPropertyValue -Object $summary.steps.install_smoke -Name "consumer_document"
$gateSummaryPath = Get-OptionalPropertyValue -Object $summary.steps.visual_gate -Name "summary_json"
$gateFinalReviewPath = Get-OptionalPropertyValue -Object $summary.steps.visual_gate -Name "final_review"

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
if ([string]::IsNullOrWhiteSpace($visualVerdict)) {
    $visualVerdict = "pending_manual_review"
}

$syncLatestCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1"
$releaseVersion = Get-OptionalPropertyValue -Object $summary -Name "release_version"
$packageAssetsCommand = if ($releaseVersion) {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}" -UploadReleaseTag "v{1}"' -f `
        $summaryCommandPath, $releaseVersion
} else {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}"' -f `
        $summaryCommandPath
}
$syncReleaseNotesCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\sync_github_release_notes.ps1 -SummaryJson "{0}"' -f `
    $summaryCommandPath
if ($releaseVersion) {
    $syncReleaseNotesCommand += (' -ReleaseTag "v{0}"' -f $releaseVersion)
}
$publishReleaseCommand = $syncReleaseNotesCommand + " -Publish"
$publishWorkflowCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\publish_github_release.ps1 -SummaryJson "{0}"' -f `
    $summaryCommandPath
if ($releaseVersion) {
    $publishWorkflowCommand += (' -ReleaseTag "v{0}"' -f $releaseVersion)
}
$publishWorkflowFinalCommand = $publishWorkflowCommand + " -Publish"

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("# Release Reviewer Checklist")
[void]$lines.Add("")
[void]$lines.Add("- Execution status: $($summary.execution_status)")
[void]$lines.Add("- Visual gate status: $($summary.steps.visual_gate.status)")
[void]$lines.Add("- Visual verdict: $visualVerdict")
[void]$lines.Add("- README gallery refresh: $(Get-DisplayValue -Value $readmeGalleryStatus)")
[void]$lines.Add("- Artifact guide: $(Get-DisplayPath -RepoRoot $repoRoot -Path $artifactGuidePath)")
[void]$lines.Add("")
[void]$lines.Add("## Step 1: Read The Release Notes")
[void]$lines.Add("")
Add-CheckboxLine -Lines $lines -Text ('Open `ARTIFACT_GUIDE.md` and confirm the file map still matches this artifact: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $artifactGuidePath))
Add-CheckboxLine -Lines $lines -Text ('Open `release_summary.zh-CN.md` and confirm the front-page bullets are still accurate: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $releaseSummaryPath))
Add-CheckboxLine -Lines $lines -Text ('Open `release_body.zh-CN.md` and confirm the longer release body does not contradict the short summary: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBodyPath))
Add-CheckboxLine -Lines $lines -Text ('Open `release_handoff.md` and confirm the evidence links and repro commands are still valid: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $releaseHandoffPath))
[void]$lines.Add("")
[void]$lines.Add("## Step 2: Check The Verification State")
[void]$lines.Add("")
Add-CheckboxLine -Lines $lines -Text ('Confirm `summary.json` reports the expected step statuses: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedSummaryPath))
Add-CheckboxLine -Lines $lines -Text ('Confirm `final_review.md` still matches the current release status: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $finalReviewPath))

if ($summary.steps.visual_gate.status -eq "skipped") {
    Add-CheckboxLine -Lines $lines -Text 'Treat this as a CI metadata artifact only; do not treat the visual verdict as the final Word screenshot-backed release signoff.'
} elseif ($visualVerdict -eq "pass") {
    Add-CheckboxLine -Lines $lines -Text ('Confirm the local Word visual evidence is already signed off as `pass`: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $gateSummaryPath))
} else {
    Add-CheckboxLine -Lines $lines -Text ('Stop here until the local Word visual review is completed and the visual verdict changes from `{0}`.' -f $visualVerdict)
}

if (-not [string]::IsNullOrWhiteSpace($consumerDocument)) {
    Add-CheckboxLine -Lines $lines -Text ('Confirm the install smoke consumer document exists for spot checks: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $consumerDocument))
}

if (-not [string]::IsNullOrWhiteSpace($gateFinalReviewPath)) {
    Add-CheckboxLine -Lines $lines -Text ('Spot-check the visual gate final review notes if anything in the release notes feels risky: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $gateFinalReviewPath))
}
if ($readmeGalleryStatus -eq "completed") {
    Add-CheckboxLine -Lines $lines -Text ('Confirm the repository README gallery PNGs were refreshed from the latest Word evidence: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $readmeGalleryAssetsDir))
}

[void]$lines.Add("")
[void]$lines.Add("## Step 3: Publish Or Refresh")
[void]$lines.Add("")
Add-CheckboxLine -Lines $lines -Text 'Use `release_summary.zh-CN.md` for the GitHub Release first-screen bullets.'
Add-CheckboxLine -Lines $lines -Text 'Use `release_body.zh-CN.md` for the full release notes or translated handoff notes.'
Add-CheckboxLine -Lines $lines -Text ('Generate or refresh the public release ZIP files before publishing: `{0}`' -f $packageAssetsCommand)
Add-CheckboxLine -Lines $lines -Text ('Sync the audited full release body into the GitHub Release notes: `{0}`' -f $syncReleaseNotesCommand)
Add-CheckboxLine -Lines $lines -Text ('When all gates pass and the GitHub Release is ready to go live, publish it with: `{0}`' -f $publishReleaseCommand)
Add-CheckboxLine -Lines $lines -Text ('Use the one-shot wrapper when you want ZIP upload plus note sync together: `{0}`' -f $publishWorkflowCommand)
Add-CheckboxLine -Lines $lines -Text ('Use the same wrapper with final publish enabled when the release is ready to go live: `{0}`' -f $publishWorkflowFinalCommand)
Add-CheckboxLine -Lines $lines -Text ('If the visual verdict changes later, rerun the verdict sync command so the gate summary and release notes stay in sync: `{0}`' -f $syncLatestCommand)

if (-not [string]::IsNullOrWhiteSpace($installPrefix)) {
    $installedQuickstartZh = Join-Path $installPrefix "share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
    $installedTemplateZh = Join-Path $installPrefix "share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
    Add-CheckboxLine -Lines $lines -Text ('Use the installed quickstart for the shortest preview -> command -> review-task path: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $installedQuickstartZh))
    Add-CheckboxLine -Lines $lines -Text ('Use the installed release template when the reviewer needs a copy-paste release-note skeleton: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $installedTemplateZh))
}

[void]$lines.Add("")
[void]$lines.Add("## Stop Conditions")
[void]$lines.Add("")
[void]$lines.Add("- Do not approve for public release when `execution_status` is not `pass`.")
[void]$lines.Add("- Do not approve for public release when the final local visual verdict is not `pass`.")
[void]$lines.Add("- Do not treat a CI-only artifact with visual gate = skipped as the final screenshot-backed release signoff.")

New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputPath) -Force | Out-Null
($lines -join [Environment]::NewLine) | Set-Content -Path $resolvedOutputPath -Encoding UTF8

Write-Host "Reviewer checklist: $resolvedOutputPath"
