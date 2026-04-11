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

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
if (-not (Test-Path -LiteralPath $resolvedSummaryPath)) {
    throw "Summary JSON does not exist: $resolvedSummaryPath"
}
$summaryCommandPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedSummaryPath

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path (Split-Path -Parent $resolvedSummaryPath) "ARTIFACT_GUIDE.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputPath
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$reportDir = Split-Path -Parent $resolvedSummaryPath
$finalReviewPath = Join-Path $reportDir "final_review.md"
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
$reviewerChecklistPath = Get-OptionalPropertyValue -Object $summary -Name "reviewer_checklist"
if ([string]::IsNullOrWhiteSpace($reviewerChecklistPath)) {
    $reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
}

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

$installLeaf = ""
if (-not [string]::IsNullOrWhiteSpace($installPrefix)) {
    $installLeaf = Split-Path -Leaf $installPrefix
}
if ([string]::IsNullOrWhiteSpace($installLeaf)) {
    $installLeaf = "build-msvc-install"
}

$artifactQuickstartPath = Join-Path $installLeaf "share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
$artifactTemplatePath = Join-Path $installLeaf "share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
$artifactPreviewDir = Join-Path $installLeaf "share\FeatherDoc\visual-validation"

$syncLatestCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1"
$releaseVersion = Get-OptionalPropertyValue -Object $summary -Name "release_version"
$assetOutputDir = if ($releaseVersion) {
    Join-Path ("output\release-assets") ("v{0}" -f $releaseVersion)
} else {
    "output\release-assets\v<version>"
}
$packageAssetsCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}"' -f `
    $summaryCommandPath
$packageAndUploadCommand = ""
if ($releaseVersion) {
    $packageAndUploadCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}" -UploadReleaseTag "v{1}"' -f `
        $summaryCommandPath, $releaseVersion
}

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("# Release Metadata Artifact Guide")
[void]$lines.Add("")
[void]$lines.Add("- Generated at: $(Get-Date -Format s)")
[void]$lines.Add("- Execution status: $($summary.execution_status)")
[void]$lines.Add("- Visual verdict: $(Get-DisplayValue -Value $visualVerdict)")
[void]$lines.Add("- README gallery refresh: $(Get-DisplayValue -Value $readmeGalleryStatus)")
[void]$lines.Add("- Summary JSON: $(Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedSummaryPath)")
[void]$lines.Add("")
[void]$lines.Add("## Start Here")
[void]$lines.Add("")
[void]$lines.Add('1. Open `REVIEWER_CHECKLIST.md` first for the reviewer flow.')
[void]$lines.Add('2. Open `release_summary.zh-CN.md` next for the GitHub Release front-page bullets.')
[void]$lines.Add('3. Open `release_body.zh-CN.md` for the full Chinese release-body draft, then `release_handoff.md` for evidence and repro links.')
[void]$lines.Add("")
[void]$lines.Add("## Files In This Report Directory")
[void]$lines.Add("")
[void]$lines.Add("- Reviewer checklist: $(Get-DisplayPath -RepoRoot $repoRoot -Path $reviewerChecklistPath)")
[void]$lines.Add("- Short summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $releaseSummaryPath)")
[void]$lines.Add("- Full release body: $(Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBodyPath)")
[void]$lines.Add("- Release handoff: $(Get-DisplayPath -RepoRoot $repoRoot -Path $releaseHandoffPath)")
[void]$lines.Add("- Final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $finalReviewPath)")
[void]$lines.Add("- Consumer smoke document: $(Get-DisplayPath -RepoRoot $repoRoot -Path $consumerDocument)")
[void]$lines.Add("- Visual gate summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $gateSummaryPath)")
[void]$lines.Add("- Visual gate final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $gateFinalReviewPath)")
[void]$lines.Add("- README gallery assets: $(Get-DisplayPath -RepoRoot $repoRoot -Path $readmeGalleryAssetsDir)")
[void]$lines.Add("")
[void]$lines.Add("## Installed Package Entry Points")
[void]$lines.Add("")
[void]$lines.Add("If you are browsing the uploaded release-metadata artifact, start from these artifact-local paths:")
[void]$lines.Add("")
[void]$lines.Add(('- `{0}`' -f $artifactQuickstartPath))
[void]$lines.Add(('- `{0}`' -f $artifactTemplatePath))
[void]$lines.Add(('- `{0}`' -f $artifactPreviewDir))
[void]$lines.Add("")
[void]$lines.Add("Those files point from preview PNGs to the local repro scripts and review-task wrappers.")
[void]$lines.Add("")
[void]$lines.Add("## Package Release Assets")
[void]$lines.Add("")
[void]$lines.Add("- Release assets output dir: $assetOutputDir")
[void]$lines.Add('```powershell')
[void]$lines.Add($packageAssetsCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
if (-not [string]::IsNullOrWhiteSpace($packageAndUploadCommand)) {
    [void]$lines.Add("To upload the generated ZIP files to the matching GitHub Release:")
    [void]$lines.Add("")
    [void]$lines.Add('```powershell')
    [void]$lines.Add($packageAndUploadCommand)
    [void]$lines.Add('```')
    [void]$lines.Add("")
}
[void]$lines.Add("## Refresh After A Later Visual Verdict Update")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($syncLatestCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("Use that command when the screenshot-backed manual review changes the visual verdict and you want to sync the latest task verdict back into the gate summary while refreshing the detected release-facing drafts without rerunning the full preflight.")
[void]$lines.Add("")
[void]$lines.Add("## Notes")
[void]$lines.Add("")
[void]$lines.Add("- CI artifacts intentionally keep the Word visual gate skipped; the final screenshot-backed verdict still belongs to a local Windows + Microsoft Word run.")
[void]$lines.Add('- The installed `share/FeatherDoc` tree carries the preview PNGs, quickstarts, and release templates; the `report` directory carries the generated release drafts and evidence indexes.')

New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputPath) -Force | Out-Null
($lines -join [Environment]::NewLine) | Set-Content -Path $resolvedOutputPath -Encoding UTF8

Write-Host "Artifact guide: $resolvedOutputPath"
