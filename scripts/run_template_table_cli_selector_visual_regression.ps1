param(
    [string]$BuildDir = "build-ttcli-selector-visual",
    [string]$OutputDir = "output/template-table-cli-selector-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "run_template_table_cli_selector_visual_regression_helpers.ps1")

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"

if (-not $SkipBuild) {
    $vcvarsPath = Get-VcvarsPath
    Write-Step "Configuring dedicated build directory $resolvedBuildDir"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"
    Write-Step "Building featherdoc_cli and selector visual sample"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_template_table_cli_bookmark_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_template_table_cli_bookmark_visual"
$renderPython = if ($SkipVisual) { $null } else { Ensure-RenderPython -RepoRoot $repoRoot }

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateFirstPagesDir = Join-Path $aggregateEvidenceDir "first-pages"
$aggregateSelectedPagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregateFirstPagesDir -Force | Out-Null
New-Item -ItemType Directory -Path $aggregateSelectedPagesDir -Force | Out-Null

$sharedBaselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$sharedBaselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"
Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($sharedBaselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared selector baseline sample."

$sharedBaselineFirstPage = $null
if (-not $SkipVisual) {
    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $sharedBaselineDocxPath `
        -OutputDir $sharedBaselineVisualDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent
    $sharedBaselineFirstPage = Join-Path $sharedBaselineVisualDir "evidence\pages\page-01.png"
}

$selectorArgs = @(
    "--after-text", "Bookmark paragraph immediately before the target table.",
    "--header-cell", "Region",
    "--header-cell", "Qty",
    "--header-cell", "Amount"
)

$retainedRows = @(
    @("Retained", "Qty", "Amount"),
    @("keep-left", "4", "40")
)

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    cases = @()
}

$aggregateImages = @()
$aggregateLabels = @()
$script:selectedPagesByCase = @{}

. (Join-Path $PSScriptRoot "run_template_table_cli_selector_visual_regression_cases_core.ps1")
. (Join-Path $PSScriptRoot "run_template_table_cli_selector_visual_regression_cases_data.ps1")

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
Write-SummaryArtifacts `
    -SummaryPath $summaryPath `
    -SkipVisual $SkipVisual.IsPresent `
    -Python $renderPython `
    -ScriptPath $contactSheetScript `
    -AggregateContactSheetPath $aggregateContactSheetPath
