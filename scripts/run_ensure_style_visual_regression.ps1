param(
    [string]$BuildDir = "build-ensure-style-visual-nmake",
    [string]$OutputDir = "output/ensure-style-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"
$script:WordMlNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"

Add-Type -AssemblyName System.IO.Compression.FileSystem

. (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_core.ps1")
. (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_state_assertions.ps1")
. (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_visual_artifacts.ps1")
. (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_style_xml_assertions.ps1")

. (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_initialize.ps1")
. (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_definitions.ps1")

foreach ($case in $caseDefinitions) {
    . (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_mutation.ps1")
    . (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_capture.ps1")
    . (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_assertions.ps1")
    . (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_visual_summary.ps1")
}

. (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_final_summary.ps1")
