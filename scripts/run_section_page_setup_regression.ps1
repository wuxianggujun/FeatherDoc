param(
    [string]$BuildDir = "build-section-page-setup-regression-nmake",
    [string]$OutputDir = "output/section-page-setup-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$scriptPath = Join-Path $PSScriptRoot "run_section_page_setup_visual_regression.ps1"
$parameters = @{
    BuildDir = $BuildDir
    OutputDir = $OutputDir
    Dpi = $Dpi
}

if ($SkipBuild) {
    $parameters.SkipBuild = $true
}
if ($SkipVisual) {
    $parameters.SkipVisual = $true
}
if ($KeepWordOpen) {
    $parameters.KeepWordOpen = $true
}
if ($VisibleWord) {
    $parameters.VisibleWord = $true
}

& $scriptPath @parameters
exit $LASTEXITCODE
