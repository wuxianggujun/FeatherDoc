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

Write-Host "Publish GitHub release wrapper regression passed."
