<#
.SYNOPSIS
Packages FeatherDoc release assets and syncs audited GitHub release notes.

.DESCRIPTION
Runs `package_release_assets.ps1` with `-UploadReleaseTag` and then
`sync_github_release_notes.ps1` against the same release tag. This gives the
release reviewer a single command for refreshing ZIP assets plus audited notes.

When `-Publish` is set, the second step passes `-Publish` through so the target
GitHub Release is also flipped to `draft=false` after the assets and notes were
updated successfully.

.PARAMETER SummaryJson
Path to the release-candidate summary JSON produced by
`run_release_candidate_checks.ps1`.

.PARAMETER ReleaseTag
Optional explicit GitHub release tag. Defaults to `v<release_version>`.

.PARAMETER ReleaseVersion
Optional explicit version override. Used only when deriving the default tag and
when forwarding arguments to the child scripts.

.PARAMETER OutputRoot
Optional override for `package_release_assets.ps1 -OutputRoot`.

.PARAMETER InstallPrefix
Optional override for `package_release_assets.ps1 -InstallPrefix`.

.PARAMETER ReadmeAssetsDir
Optional override for `package_release_assets.ps1 -ReadmeAssetsDir`.

.PARAMETER BodyPath
Optional override for `sync_github_release_notes.ps1 -BodyPath`.

.PARAMETER Title
Optional override for `sync_github_release_notes.ps1 -Title`.

.PARAMETER KeepStaging
Passes `-KeepStaging` through to `package_release_assets.ps1`.

.PARAMETER Publish
Also publishes the target GitHub Release by passing `-Publish` through to
`sync_github_release_notes.ps1`.

.PARAMETER PackageScriptPath
Optional explicit override for the packaging script path. Intended for
tooling/tests; defaults to `scripts/package_release_assets.ps1`.

.PARAMETER SyncNotesScriptPath
Optional explicit override for the release-note sync script path. Intended for
tooling/tests; defaults to `scripts/sync_github_release_notes.ps1`.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\publish_github_release.ps1 `
    -SummaryJson .\output\release-candidate-checks\report\summary.json

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\publish_github_release.ps1 `
    -SummaryJson .\output\release-candidate-checks\report\summary.json `
    -Publish
#>
param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$ReleaseTag = "",
    [string]$ReleaseVersion = "",
    [string]$OutputRoot = "",
    [string]$InstallPrefix = "",
    [string]$ReadmeAssetsDir = "",
    [string]$BodyPath = "",
    [string]$Title = "",
    [switch]$KeepStaging,
    [switch]$Publish,
    [string]$PackageScriptPath = "",
    [string]$SyncNotesScriptPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[publish-github-release] $Message"
}

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

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        throw "Missing ${Label}: $Path"
    }
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

function Get-ProjectVersion {
    param([string]$RepoRoot)

    $cmakePath = Join-Path $RepoRoot "CMakeLists.txt"
    if (-not (Test-Path -LiteralPath $cmakePath)) {
        return ""
    }

    $content = Get-Content -Raw -LiteralPath $cmakePath
    $match = [regex]::Match($content, 'VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)')
    if (-not $match.Success) {
        return ""
    }

    return $match.Groups[1].Value
}

function Get-ResolvedReleaseVersion {
    param(
        [string]$ExplicitVersion,
        $Summary,
        [string]$RepoRoot
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitVersion)) {
        return $ExplicitVersion
    }

    $summaryVersion = Get-OptionalPropertyValue -Object $Summary -Name "release_version"
    if (-not [string]::IsNullOrWhiteSpace($summaryVersion)) {
        return $summaryVersion
    }

    return Get-ProjectVersion -RepoRoot $RepoRoot
}

function Update-UploadedAssetManifest {
    param(
        [string]$RepoRoot,
        [string]$OutputRoot,
        [string]$ReleaseVersion,
        [string]$ReleaseTag
    )

    $resolvedOutputRoot = if (-not [string]::IsNullOrWhiteSpace($OutputRoot)) {
        Resolve-FullPath -RepoRoot $RepoRoot -InputPath $OutputRoot
    } else {
        Join-Path $RepoRoot "output\release-assets"
    }
    $manifestPath = Join-Path (Join-Path $resolvedOutputRoot ("v{0}" -f $ReleaseVersion)) "release_assets_manifest.json"
    if (-not (Test-Path -LiteralPath $manifestPath)) {
        return
    }
    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
        return
    }

    $releaseViewJson = & gh release view $ReleaseTag --json url,assets
    if ($LASTEXITCODE -ne 0) {
        throw "gh release view failed while refreshing release asset manifest."
    }

    $releaseView = $releaseViewJson | ConvertFrom-Json
    $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    if ($null -eq $manifest.PSObject.Properties["upload"]) {
        Add-Member -InputObject $manifest -NotePropertyName upload -NotePropertyValue ([pscustomobject]@{})
    }
    $manifest.upload.requested_tag = $ReleaseTag
    $manifest.upload.uploaded = $true
    $manifest.upload.release_url = Get-OptionalPropertyValue -Object $releaseView -Name "url"

    $remoteAssets = @()
    foreach ($asset in @($manifest.assets)) {
        $assetName = Split-Path -Leaf ([string]$asset.path)
        $remote = @($releaseView.assets | Where-Object { $_.name -eq $assetName }) | Select-Object -First 1
        if ($null -ne $remote) {
            $remoteAssets += [ordered]@{
                name = $remote.name
                url = $remote.url
                size_bytes = $remote.size
                download_count = $remote.downloadCount
            }
        }
    }
    $manifest.upload.remote_assets = $remoteAssets
    ($manifest | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    Write-Step "Refreshed uploaded asset manifest from GitHub release view"
}

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
Assert-PathExists -Path $resolvedSummaryPath -Label "release summary JSON"

$summary = Get-Content -Raw -LiteralPath $resolvedSummaryPath | ConvertFrom-Json
$resolvedReleaseVersion = Get-ResolvedReleaseVersion -ExplicitVersion $ReleaseVersion -Summary $summary -RepoRoot $repoRoot
if ([string]::IsNullOrWhiteSpace($resolvedReleaseVersion)) {
    throw "Could not resolve the release version from the summary or CMakeLists.txt."
}

$resolvedReleaseTag = if (-not [string]::IsNullOrWhiteSpace($ReleaseTag)) {
    $ReleaseTag
} else {
    "v{0}" -f $resolvedReleaseVersion
}

$resolvedPackageScriptPath = if ([string]::IsNullOrWhiteSpace($PackageScriptPath)) {
    Join-Path $repoRoot "scripts\package_release_assets.ps1"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $PackageScriptPath
}
$resolvedSyncNotesScriptPath = if ([string]::IsNullOrWhiteSpace($SyncNotesScriptPath)) {
    Join-Path $repoRoot "scripts\sync_github_release_notes.ps1"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $SyncNotesScriptPath
}

Assert-PathExists -Path $resolvedPackageScriptPath -Label "package release assets script"
Assert-PathExists -Path $resolvedSyncNotesScriptPath -Label "sync GitHub release notes script"

$packageArgs = @{
    SummaryJson = $resolvedSummaryPath
    ReleaseVersion = $resolvedReleaseVersion
    UploadReleaseTag = $resolvedReleaseTag
}
if (-not [string]::IsNullOrWhiteSpace($OutputRoot)) {
    $packageArgs.OutputRoot = $OutputRoot
}
if (-not [string]::IsNullOrWhiteSpace($InstallPrefix)) {
    $packageArgs.InstallPrefix = $InstallPrefix
}
if (-not [string]::IsNullOrWhiteSpace($ReadmeAssetsDir)) {
    $packageArgs.ReadmeAssetsDir = $ReadmeAssetsDir
}
if ($KeepStaging) {
    $packageArgs.KeepStaging = $true
}

$syncArgs = @{
    SummaryJson = $resolvedSummaryPath
    ReleaseVersion = $resolvedReleaseVersion
    ReleaseTag = $resolvedReleaseTag
}
if (-not [string]::IsNullOrWhiteSpace($BodyPath)) {
    $syncArgs.BodyPath = $BodyPath
}
if (-not [string]::IsNullOrWhiteSpace($Title)) {
    $syncArgs.Title = $Title
}
if ($Publish) {
    $syncArgs.Publish = $true
}

Write-Step "Uploading release assets to GitHub release $resolvedReleaseTag"
& $resolvedPackageScriptPath @packageArgs

Write-Step "Syncing audited GitHub release notes to $resolvedReleaseTag"
& $resolvedSyncNotesScriptPath @syncArgs

Update-UploadedAssetManifest `
    -RepoRoot $repoRoot `
    -OutputRoot $OutputRoot `
    -ReleaseVersion $resolvedReleaseVersion `
    -ReleaseTag $resolvedReleaseTag

Write-Host "Release tag: $resolvedReleaseTag"
Write-Host "Release version: $resolvedReleaseVersion"
Write-Host "Published: $Publish"
