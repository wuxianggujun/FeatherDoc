param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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
    [switch]$KeepStaging
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
}
Add-Content -LiteralPath $env:FEATHERDOC_PUBLISH_RELEASE_LOG -Value (($payload | ConvertTo-Json -Compress))
'@

Set-Content -LiteralPath $fakeSyncPath -Encoding UTF8 -Value @'
param(
    [string]$SummaryJson,
    [string]$ReleaseVersion,
    [string]$ReleaseTag,
    [string]$BodyPath = "",
    [string]$Title = "",
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
if ($syncEntry.release_tag -ne "v1.6.4") {
    throw "Sync step did not derive the expected release tag."
}
if (-not [bool]$syncEntry.publish) {
    throw "Sync step did not receive -Publish."
}
if ($syncEntry.title -ne $releaseTitle) {
    throw "Sync step did not receive the explicit release title."
}

Write-Host "Publish GitHub release wrapper regression passed."
