param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.IO.Compression.FileSystem

. (Join-Path $PSScriptRoot "package_release_assets_safety_helpers.ps1")
. (Join-Path $PSScriptRoot "package_release_assets_safety_fixture.ps1")
. (Join-Path $PSScriptRoot "package_release_assets_safety_staged_material_assertions.ps1")
. (Join-Path $PSScriptRoot "package_release_assets_safety_manifest_metrics_assertions.ps1")
. (Join-Path $PSScriptRoot "package_release_assets_safety_warning_signoff_assertions.ps1")
