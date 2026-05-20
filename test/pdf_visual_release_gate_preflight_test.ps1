param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-CheckPassed {
    param(
        [object]$Summary,
        [string]$Name
    )

    $check = @($Summary.checks | Where-Object { $_.name -eq $Name })
    if ($check.Count -ne 1) {
        throw "Expected exactly one preflight check named '$Name'."
    }
    if ($check[0].status -ne "pass") {
        throw "Preflight check '$Name' should pass, got '$($check[0].status)'."
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    throw "WorkingDir is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_pdf_visual_release_gate_preflight.ps1"
$visualGatePath = Join-Path $resolvedRepoRoot "scripts\run_pdf_visual_release_gate.ps1"
$manifestPath = Join-Path $resolvedRepoRoot "test\pdf_regression_manifest.json"
$summaryPath = Join-Path $resolvedWorkingDir "preflight-summary.json"
$fakeBuildDir = Join-Path $resolvedWorkingDir "fake-pdf-build"
$fakeCtestPath = Join-Path $resolvedWorkingDir "fake-ctest.cmd"
$fakePythonPath = Join-Path $resolvedWorkingDir "fake-python.cmd"

New-Item -ItemType Directory -Path $fakeBuildDir -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $fakeBuildDir "test\pdf_cli_export") -Force | Out-Null
Set-Content -LiteralPath (Join-Path $fakeBuildDir "CMakeCache.txt") -Encoding UTF8 -Value "FEATHERDOC_BUILD_PDF:BOOL=ON"
@"
add_test(pdf_cli_export "cmd" "/c" "exit 0")
add_test(pdf_regression_manifest "cmd" "/c" "exit 0")
add_test(pdf_visual_release_gate_style_baselines "cmd" "/c" "exit 0")
add_test(pdf_visual_release_gate_text_shaping_baselines "cmd" "/c" "exit 0")
"@ | Set-Content -LiteralPath (Join-Path $fakeBuildDir "CTestTestfile.cmake") -Encoding UTF8
Set-Content -LiteralPath (Join-Path $fakeBuildDir "test\pdf_cli_export\font-map-source.pdf") -Encoding UTF8 -Value "fake"
Set-Content -LiteralPath (Join-Path $fakeBuildDir "test\pdf_cli_export\cjk-font-source.pdf") -Encoding UTF8 -Value "fake"

$manifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath | ConvertFrom-Json
foreach ($sample in $manifest.samples) {
    $expectVisual = $false
    $expectCjk = $false
    if ($null -ne $sample.PSObject.Properties["expect_visual_baseline"]) {
        $expectVisual = $sample.expect_visual_baseline -eq $true
    }
    if ($null -ne $sample.PSObject.Properties["expect_cjk"]) {
        $expectCjk = $sample.expect_cjk -eq $true
    }
    if ($expectVisual -or $expectCjk) {
        $pdfPath = Join-Path $fakeBuildDir "test\$($sample.output_file)"
        New-Item -ItemType Directory -Path (Split-Path -Parent $pdfPath) -Force | Out-Null
        Set-Content -LiteralPath $pdfPath -Encoding UTF8 -Value "fake"
    }
}

@"
@echo off
echo Test #1: pdf_cli_export
echo Test #2: pdf_regression_manifest
echo Test #3: pdf_visual_release_gate_style_baselines
echo Test #4: pdf_visual_release_gate_text_shaping_baselines
exit /b 0
"@ | Set-Content -LiteralPath $fakeCtestPath -Encoding ASCII

@"
@echo off
exit /b 0
"@ | Set-Content -LiteralPath $fakePythonPath -Encoding ASCII

$previousRenderPython = $env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE
$env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE = $fakePythonPath
try {
    & powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
        -File $scriptPath `
        -BuildDir $fakeBuildDir `
        -OutputJson $summaryPath `
        -CTestExecutable $fakeCtestPath `
        -Strict | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "PDF visual release gate preflight should pass against the fake reusable build."
    }

    $visualGatePreflightSummaryPath = Join-Path $resolvedWorkingDir "visual-gate-preflight-summary.json"
    $visualGateOutputDir = Join-Path $resolvedWorkingDir "visual-gate-output"
    & powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
        -File $visualGatePath `
        -BuildDir $fakeBuildDir `
        -OutputDir $visualGateOutputDir `
        -PreflightJson $visualGatePreflightSummaryPath `
        -PreflightOnly | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "PDF visual release gate -PreflightOnly should pass against the fake reusable build."
    }
    if (-not (Test-Path -LiteralPath $visualGatePreflightSummaryPath -PathType Leaf)) {
        throw "PDF visual release gate -PreflightOnly should write its preflight summary."
    }
    if (Test-Path -LiteralPath (Join-Path $visualGateOutputDir "report\summary.json") -PathType Leaf) {
        throw "PDF visual release gate -PreflightOnly should not write the full visual gate summary."
    }
} finally {
    if ($null -eq $previousRenderPython) {
        Remove-Item Env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE -ErrorAction SilentlyContinue
    } else {
        $env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE = $previousRenderPython
    }
}

Assert-True -Condition (Test-Path -LiteralPath $summaryPath -PathType Leaf) `
    -Message "Preflight should write a JSON summary."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-True -Condition ($summary.status -eq "ready") `
    -Message "Preflight summary should be ready for a reusable fake PDF build."

foreach ($name in @(
    "build_dir_exists",
    "cmake_cache_exists",
    "ctest_manifest_exists",
    "ctest_list_contains_pdf_gate_tests",
    "pdf_visual_gate_scripts_exist",
    "pdf_regression_manifest_exists",
    "pdf_cli_export_baseline_pdfs_exist",
    "visual_baseline_manifest_pdfs_exist",
    "cjk_text_layer_manifest_pdfs_exist",
    "render_python_reusable"
)) {
    Assert-CheckPassed -Summary $summary -Name $name
}

$visualGateText = Get-Content -Raw -Encoding UTF8 -LiteralPath $visualGatePath
foreach ($expectedText in @(
    "[string]`$PreflightJson",
    "[switch]`$PreflightOnly",
    "[switch]`$SkipPreflight",
    "scripts\check_pdf_visual_release_gate_preflight.ps1",
    "-Strict",
    "PDF visual release gate preflight failed",
    "-PreflightOnly cannot be used with -SkipPreflight"
)) {
    Assert-True -Condition ($visualGateText -match [regex]::Escape($expectedText)) `
        -Message "PDF visual release gate should keep preflight contract marker '$expectedText'."
}

$preflightText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
foreach ($expectedText in @(
    "Resolve-PreferredBuildDir",
    "Get-BuildDirectorySnapshot",
    '"build", "out\build"',
    'Source = "auto:$candidate"',
    "cmake_cache_exists",
    "build_dir_source",
    "requested_build_dir",
    "entries_preview"
)) {
    Assert-True -Condition ($preflightText -match [regex]::Escape($expectedText)) `
        -Message "PDF visual release gate preflight should keep build-dir selection marker '$expectedText'."
}

Write-Host "PDF visual release gate preflight contract passed."
