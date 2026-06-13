param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "package_release_assets_allow_incomplete_test_helpers.ps1")
. (Join-Path $PSScriptRoot "package_release_assets_allow_incomplete_test_assertions.ps1")

Assert-PackageReleaseAssetsAllowIncompleteRegression

