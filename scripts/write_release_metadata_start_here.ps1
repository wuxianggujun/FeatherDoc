param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$OutputPath = "",
    [switch]$ArtifactRootLayout
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

$reportDir = Split-Path -Parent $resolvedSummaryPath
$summaryRootDir = Split-Path -Parent $reportDir
$summaryRootLeaf = Split-Path -Leaf $summaryRootDir

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path $summaryRootDir "START_HERE.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputPath
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$releaseVersion = Get-OptionalPropertyValue -Object $summary -Name "release_version"
$installDir = Get-OptionalPropertyValue -Object $summary -Name "install_dir"
$installDirLeaf = if ([string]::IsNullOrWhiteSpace($installDir)) {
    "build-msvc-install"
} else {
    Split-Path -Leaf $installDir
}

$guideLocalPath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistLocalPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$releaseSummaryLocalPath = Join-Path $reportDir "release_summary.zh-CN.md"
$releaseBodyLocalPath = Join-Path $reportDir "release_body.zh-CN.md"
$quickstartLocalPath = Join-Path $installDir "share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
$templateLocalPath = Join-Path $installDir "share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"

$startHereArtifactPath = Join-Path ("output\{0}" -f $summaryRootLeaf) "START_HERE.md"
$guideArtifactPath = Join-Path ("output\{0}\report" -f $summaryRootLeaf) "ARTIFACT_GUIDE.md"
$reviewerChecklistArtifactPath = Join-Path ("output\{0}\report" -f $summaryRootLeaf) "REVIEWER_CHECKLIST.md"
$releaseSummaryArtifactPath = Join-Path ("output\{0}\report" -f $summaryRootLeaf) "release_summary.zh-CN.md"
$releaseBodyArtifactPath = Join-Path ("output\{0}\report" -f $summaryRootLeaf) "release_body.zh-CN.md"
$quickstartArtifactPath = Join-Path $installDirLeaf "share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
$templateArtifactPath = Join-Path $installDirLeaf "share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"

$syncLatestCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1"
$packageAssetsCommand = if ($releaseVersion) {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}" -UploadReleaseTag "v{1}"' -f `
        $summaryCommandPath, $releaseVersion
} else {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}"' -f `
        $summaryCommandPath
}

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("# Release Metadata Start Here")
[void]$lines.Add("")
if ($ArtifactRootLayout) {
    [void]$lines.Add("If you just downloaded the release metadata artifact, use this file as the shortest jump into the generated report bundle.")
} else {
    [void]$lines.Add('Use this file as the shortest jump into the generated release-metadata bundle after a local `run_release_candidate_checks.ps1` run.')
}
[void]$lines.Add("")
[void]$lines.Add("## Open These First")
[void]$lines.Add("")

if ($ArtifactRootLayout) {
    [void]$lines.Add(('- `START_HERE.md`: `{0}`' -f $startHereArtifactPath))
    [void]$lines.Add(('- `ARTIFACT_GUIDE.md`: `{0}`' -f $guideArtifactPath))
    [void]$lines.Add(('- `REVIEWER_CHECKLIST.md`: `{0}`' -f $reviewerChecklistArtifactPath))
    [void]$lines.Add(('- `release_summary.zh-CN.md`: `{0}`' -f $releaseSummaryArtifactPath))
    [void]$lines.Add(('- `release_body.zh-CN.md`: `{0}`' -f $releaseBodyArtifactPath))
    [void]$lines.Add(('- Installed quickstart: `{0}`' -f $quickstartArtifactPath))
    [void]$lines.Add(('- Installed release template: `{0}`' -f $templateArtifactPath))
} else {
    [void]$lines.Add('- `ARTIFACT_GUIDE.md`: `report/ARTIFACT_GUIDE.md`')
    [void]$lines.Add('- `REVIEWER_CHECKLIST.md`: `report/REVIEWER_CHECKLIST.md`')
    [void]$lines.Add('- `release_summary.zh-CN.md`: `report/release_summary.zh-CN.md`')
    [void]$lines.Add('- `release_body.zh-CN.md`: `report/release_body.zh-CN.md`')
    [void]$lines.Add(('- Installed quickstart: `{0}`' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $quickstartLocalPath)))
    [void]$lines.Add(('- Installed release template: `{0}`' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $templateLocalPath)))
}

[void]$lines.Add("")
[void]$lines.Add("## Reviewer Flow")
[void]$lines.Add("")
if ($ArtifactRootLayout) {
    [void]$lines.Add('1. Open `START_HERE.md` next for the summary-root view of the generated bundle.')
    [void]$lines.Add('2. Open `ARTIFACT_GUIDE.md` for the artifact file map, then follow `REVIEWER_CHECKLIST.md` line by line.')
    [void]$lines.Add('3. Use `release_summary.zh-CN.md` and `release_body.zh-CN.md` as the final release notes.')
} else {
    [void]$lines.Add('1. Open `ARTIFACT_GUIDE.md` for the artifact file map.')
    [void]$lines.Add('2. Follow `REVIEWER_CHECKLIST.md` line by line.')
    [void]$lines.Add('3. Use `release_summary.zh-CN.md` and `release_body.zh-CN.md` as the final release notes.')
}
[void]$lines.Add("")
[void]$lines.Add("## Refresh After A Later Visual Verdict Update")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($syncLatestCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("Use that command locally after the screenshot-backed manual review changes the final visual verdict. It will sync the detected latest task verdict back into the gate summary and refresh the release bundle.")
[void]$lines.Add("")
[void]$lines.Add("## Package The Public Release Assets")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($packageAssetsCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("Run that command after the release notes are finalized so the install ZIP, visual gallery ZIP, and release-evidence ZIP are regenerated from the current summary.")

New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputPath) -Force | Out-Null
($lines -join [Environment]::NewLine) | Set-Content -Path $resolvedOutputPath -Encoding UTF8

Write-Host "Start here: $resolvedOutputPath"
