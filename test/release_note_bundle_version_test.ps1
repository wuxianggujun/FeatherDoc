param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = (Join-Path $PSScriptRoot "..")
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $resolvedRepoRoot "output\test\release-note-bundle-version"
}

. (Join-Path $PSScriptRoot "release_note_bundle_version_helpers.ps1")
. (Join-Path $PSScriptRoot "release_note_bundle_version_fixture.ps1")
. (Join-Path $PSScriptRoot "release_note_bundle_version_assertions.ps1")
. (Join-Path $PSScriptRoot "release_note_bundle_version_negative_cases.ps1")
