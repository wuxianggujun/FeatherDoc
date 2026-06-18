param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\publish_github_release_test"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$reportDir = Join-Path $resolvedWorkingDir "output\release-candidate-checks\report"
$summaryPath = Join-Path $reportDir "summary.json"
$fakePackagePath = Join-Path $resolvedWorkingDir "fake_package_release_assets.ps1"
$fakeSyncPath = Join-Path $resolvedWorkingDir "fake_sync_github_release_notes.ps1"
$callLogPath = Join-Path $resolvedWorkingDir "call-log.jsonl"

New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
Set-Content -LiteralPath $callLogPath -Encoding UTF8 -Value ""

$summary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    release_version = "1.6.4"
    execution_status = "pass"
    visual_verdict = "pass"
}
($summary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

Set-Content -LiteralPath $fakePackagePath -Encoding UTF8 -Value @'
param(
    [string]$SummaryJson,
    [string]$ReleaseVersion,
    [string]$UploadReleaseTag,
    [string]$OutputRoot = "",
    [string]$InstallPrefix = "",
    [string]$ReadmeAssetsDir = "",
    [switch]$KeepStaging,
    [switch]$AllowIncomplete
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$payload = [ordered]@{
    step = "package"
    summary_json = $SummaryJson
    release_version = $ReleaseVersion
    upload_release_tag = $UploadReleaseTag
    output_root = $OutputRoot
    install_prefix = $InstallPrefix
    readme_assets_dir = $ReadmeAssetsDir
    keep_staging = [bool]$KeepStaging
    allow_incomplete = [bool]$AllowIncomplete
}
Add-Content -LiteralPath $env:FEATHERDOC_PUBLISH_RELEASE_LOG -Value (($payload | ConvertTo-Json -Compress))

if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_PUBLISH_RELEASE_WRITE_MANIFEST)) {
    $manifestDir = Join-Path $OutputRoot ("v{0}" -f $ReleaseVersion)
    New-Item -ItemType Directory -Path $manifestDir -Force | Out-Null

    $manifest = [ordered]@{
        release_version = $ReleaseVersion
        assets = @(
            [ordered]@{
                label = "msvc_install"
                path = (Join-Path $manifestDir ("FeatherDoc-v{0}-msvc-install.zip" -f $ReleaseVersion))
            },
            [ordered]@{
                label = "visual_validation_gallery"
                path = (Join-Path $manifestDir ("FeatherDoc-v{0}-visual-validation-gallery.zip" -f $ReleaseVersion))
            },
            [ordered]@{
                label = "release_evidence"
                path = (Join-Path $manifestDir ("FeatherDoc-v{0}-release-evidence.zip" -f $ReleaseVersion))
            }
        )
        upload = [ordered]@{
            requested_tag = $UploadReleaseTag
            uploaded = $false
            release_url = ""
        }
    }
    ($manifest | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath (Join-Path $manifestDir "release_assets_manifest.json") -Encoding UTF8
}
'@

Set-Content -LiteralPath $fakeSyncPath -Encoding UTF8 -Value @'
param(
    [string]$SummaryJson,
    [string]$ReleaseVersion,
    [string]$ReleaseTag,
    [string]$BodyPath = "",
    [string]$Title = "",
    [switch]$AllowCiArtifactPublish,
    [switch]$Publish
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$payload = [ordered]@{
    step = "sync"
    summary_json = $SummaryJson
    release_version = $ReleaseVersion
    release_tag = $ReleaseTag
    body_path = $BodyPath
    title = $Title
    allow_ci_artifact_publish = [bool]$AllowCiArtifactPublish
    publish = [bool]$Publish
}
Add-Content -LiteralPath $env:FEATHERDOC_PUBLISH_RELEASE_LOG -Value (($payload | ConvertTo-Json -Compress))
'@

$bodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$outputRoot = Join-Path $resolvedWorkingDir "release-assets"
$installPrefix = Join-Path $resolvedWorkingDir "build-msvc-install"
$readmeAssetsDir = Join-Path $resolvedWorkingDir "docs\assets\readme"
$releaseTitle = "FeatherDoc v1.6.4"

$env:FEATHERDOC_PUBLISH_RELEASE_LOG = $callLogPath
try {
    $publishScript = Join-Path $resolvedRepoRoot "scripts\publish_github_release.ps1"
    & $publishScript `
        -SummaryJson $summaryPath `
        -OutputRoot $outputRoot `
        -InstallPrefix $installPrefix `
        -ReadmeAssetsDir $readmeAssetsDir `
        -BodyPath $bodyPath `
        -Title $releaseTitle `
        -KeepStaging `
        -Publish `
        -PackageScriptPath $fakePackagePath `
        -SyncNotesScriptPath $fakeSyncPath
} finally {
    Remove-Item Env:FEATHERDOC_PUBLISH_RELEASE_LOG -ErrorAction SilentlyContinue
}

$entries = @(Get-Content -LiteralPath $callLogPath | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | ForEach-Object {
    $_ | ConvertFrom-Json
})
if ($entries.Count -ne 2) {
    throw "Expected exactly 2 wrapper calls, found $($entries.Count)."
}

$packageEntry = $entries[0]
$syncEntry = $entries[1]
if ($packageEntry.step -ne "package" -or $syncEntry.step -ne "sync") {
    throw "publish_github_release.ps1 did not invoke package -> sync in the expected order."
}

if ($packageEntry.summary_json -ne $summaryPath) {
    throw "Package step did not receive the expected summary path."
}
if ($packageEntry.release_version -ne "1.6.4") {
    throw "Package step did not receive release_version=1.6.4."
}
if ($packageEntry.upload_release_tag -ne "v1.6.4") {
    throw "Package step did not derive the expected release tag."
}
if (-not [bool]$packageEntry.keep_staging) {
    throw "Package step did not receive -KeepStaging."
}
if ([bool]$packageEntry.allow_incomplete) {
    throw "Package step unexpectedly received -AllowIncomplete for a full visual release summary."
}
if ($syncEntry.release_tag -ne "v1.6.4") {
    throw "Sync step did not derive the expected release tag."
}
if ([bool]$syncEntry.allow_ci_artifact_publish) {
    throw "Sync step unexpectedly received -AllowCiArtifactPublish for a full visual release summary."
}
if (-not [bool]$syncEntry.publish) {
    throw "Sync step did not receive -Publish."
}
if ($syncEntry.title -ne $releaseTitle) {
    throw "Sync step did not receive the explicit release title."
}

$ciOnlySummary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    release_version = "1.6.4"
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    steps = [ordered]@{
        visual_gate = [ordered]@{
            status = "skipped"
        }
    }
}
($ciOnlySummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Set-Content -LiteralPath $callLogPath -Encoding UTF8 -Value ""

$failedBeforeUpload = $false
$errorMessage = ""
$env:FEATHERDOC_PUBLISH_RELEASE_LOG = $callLogPath
try {
    $publishScript = Join-Path $resolvedRepoRoot "scripts\publish_github_release.ps1"
    & $publishScript `
        -SummaryJson $summaryPath `
        -OutputRoot $outputRoot `
        -InstallPrefix $installPrefix `
        -ReadmeAssetsDir $readmeAssetsDir `
        -BodyPath $bodyPath `
        -Title $releaseTitle `
        -Publish `
        -PackageScriptPath $fakePackagePath `
        -SyncNotesScriptPath $fakeSyncPath
} catch {
    $errorMessage = $_.Exception.Message
    $failedBeforeUpload = $_.Exception.Message -like "*Refusing to publish GitHub release*"
} finally {
    Remove-Item Env:FEATHERDOC_PUBLISH_RELEASE_LOG -ErrorAction SilentlyContinue
}

if (-not $failedBeforeUpload) {
    throw "publish_github_release.ps1 unexpectedly allowed a CI-only visual summary without -AllowCiArtifactPublish. Error: $errorMessage"
}

$ciOnlyBlockedLog = Get-Content -Raw -LiteralPath $callLogPath
if (-not [string]::IsNullOrWhiteSpace($ciOnlyBlockedLog)) {
    throw "publish_github_release.ps1 invoked package/sync before rejecting a CI-only visual summary."
}

Set-Content -LiteralPath $callLogPath -Encoding UTF8 -Value ""
$env:FEATHERDOC_PUBLISH_RELEASE_LOG = $callLogPath
try {
    $publishScript = Join-Path $resolvedRepoRoot "scripts\publish_github_release.ps1"
    & $publishScript `
        -SummaryJson $summaryPath `
        -OutputRoot $outputRoot `
        -InstallPrefix $installPrefix `
        -ReadmeAssetsDir $readmeAssetsDir `
        -BodyPath $bodyPath `
        -Title $releaseTitle `
        -Publish `
        -AllowCiArtifactPublish `
        -PackageScriptPath $fakePackagePath `
        -SyncNotesScriptPath $fakeSyncPath
} finally {
    Remove-Item Env:FEATHERDOC_PUBLISH_RELEASE_LOG -ErrorAction SilentlyContinue
}

$ciEntries = @(Get-Content -LiteralPath $callLogPath | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | ForEach-Object {
    $_ | ConvertFrom-Json
})
if ($ciEntries.Count -ne 2) {
    throw "Expected exactly 2 CI artifact wrapper calls, found $($ciEntries.Count)."
}

if (-not [bool]$ciEntries[0].allow_incomplete) {
    throw "CI artifact package step did not receive -AllowIncomplete."
}
if (-not [bool]$ciEntries[1].allow_ci_artifact_publish) {
    throw "CI artifact sync step did not receive -AllowCiArtifactPublish."
}
if (-not [bool]$ciEntries[1].publish) {
    throw "CI artifact sync step did not receive -Publish."
}

$manifestOutputRoot = Join-Path $resolvedWorkingDir "release-assets-with-manifest"
$manifestPath = Join-Path (Join-Path $manifestOutputRoot "v1.6.4") "release_assets_manifest.json"
Set-Content -LiteralPath $callLogPath -Encoding UTF8 -Value ""
$env:FEATHERDOC_PUBLISH_RELEASE_LOG = $callLogPath
$env:FEATHERDOC_PUBLISH_RELEASE_WRITE_MANIFEST = "1"
function global:gh {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments)

    $global:LASTEXITCODE = 0
    if ($Arguments.Count -lt 4 -or
        $Arguments[0] -ne "release" -or
        $Arguments[1] -ne "view" -or
        $Arguments[2] -ne "v1.6.4") {
        throw "Unexpected gh invocation: $($Arguments -join ' ')"
    }

    [ordered]@{
        url = "https://github.example/releases/tag/v1.6.4"
        assets = @(
            [ordered]@{
                name = "FeatherDoc-v1.6.4-msvc-install.zip"
                url = "https://github.example/assets/msvc-install"
                size = 1234
                downloadCount = 2
            },
            [ordered]@{
                name = "FeatherDoc-v1.6.4-visual-validation-gallery.zip"
                url = "https://github.example/assets/visual-gallery"
                size = 5678
                downloadCount = 3
            },
            [ordered]@{
                name = "FeatherDoc-v1.6.4-release-evidence.zip"
                url = "https://github.example/assets/release-evidence"
                size = 9012
                downloadCount = 4
            },
            [ordered]@{
                name = "release_assets_manifest.json"
                url = "https://github.example/assets/release-assets-manifest"
                size = 4321
                downloadCount = 5
            },
            [ordered]@{
                name = "unrelated.zip"
                url = "https://github.example/assets/unrelated"
                size = 1
                downloadCount = 99
            }
        )
    } | ConvertTo-Json -Depth 8
}

try {
    $publishScript = Join-Path $resolvedRepoRoot "scripts\publish_github_release.ps1"
    & $publishScript `
        -SummaryJson $summaryPath `
        -OutputRoot $manifestOutputRoot `
        -PackageScriptPath $fakePackagePath `
        -SyncNotesScriptPath $fakeSyncPath
} finally {
    Remove-Item Env:FEATHERDOC_PUBLISH_RELEASE_LOG -ErrorAction SilentlyContinue
    Remove-Item Env:FEATHERDOC_PUBLISH_RELEASE_WRITE_MANIFEST -ErrorAction SilentlyContinue
    Remove-Item Function:\gh -ErrorAction SilentlyContinue
}

if (-not (Test-Path -LiteralPath $manifestPath)) {
    throw "publish_github_release.ps1 did not leave a release_assets_manifest.json for upload evidence."
}

$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
if ([string]$manifest.upload.requested_tag -ne "v1.6.4") {
    throw "Updated manifest did not preserve requested upload tag."
}
if (-not [bool]$manifest.upload.uploaded) {
    throw "Updated manifest did not record upload=true."
}
if ([string]$manifest.upload.release_url -ne "https://github.example/releases/tag/v1.6.4") {
    throw "Updated manifest did not record the GitHub Release URL."
}

$remoteAssets = @($manifest.upload.remote_assets)
if ($remoteAssets.Count -ne 3) {
    throw "Updated manifest should record exactly the three packaged remote assets, found $($remoteAssets.Count)."
}

$remoteAssetsByName = @{}
foreach ($asset in $remoteAssets) {
    $remoteAssetsByName[[string]$asset.name] = $asset
}

foreach ($expectedAsset in @(
        [pscustomobject]@{ Name = "FeatherDoc-v1.6.4-msvc-install.zip"; Url = "https://github.example/assets/msvc-install"; Size = 1234; Downloads = 2 },
        [pscustomobject]@{ Name = "FeatherDoc-v1.6.4-visual-validation-gallery.zip"; Url = "https://github.example/assets/visual-gallery"; Size = 5678; Downloads = 3 },
        [pscustomobject]@{ Name = "FeatherDoc-v1.6.4-release-evidence.zip"; Url = "https://github.example/assets/release-evidence"; Size = 9012; Downloads = 4 }
    )) {
    if (-not $remoteAssetsByName.ContainsKey($expectedAsset.Name)) {
        throw "Updated manifest lost remote asset '$($expectedAsset.Name)'."
    }
    $actualAsset = $remoteAssetsByName[$expectedAsset.Name]
    if ([string]$actualAsset.url -ne $expectedAsset.Url) {
        throw "Updated manifest recorded wrong URL for '$($expectedAsset.Name)'."
    }
    if ([int]$actualAsset.size_bytes -ne $expectedAsset.Size) {
        throw "Updated manifest recorded wrong size for '$($expectedAsset.Name)'."
    }
    if ([int]$actualAsset.download_count -ne $expectedAsset.Downloads) {
        throw "Updated manifest recorded wrong download count for '$($expectedAsset.Name)'."
    }
}

if ($remoteAssetsByName.ContainsKey("unrelated.zip")) {
    throw "Updated manifest included a remote asset that is not listed in manifest.assets."
}
if ($remoteAssetsByName.ContainsKey("release_assets_manifest.json")) {
    throw "Updated manifest should not list release_assets_manifest.json as a packaged remote asset."
}

Write-Host "Publish GitHub release wrapper regression passed."
