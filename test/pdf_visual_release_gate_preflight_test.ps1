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
$plainBuildRepoRoot = Join-Path $resolvedWorkingDir "plain-build-repo"
$plainBuildSummaryPath = Join-Path $resolvedWorkingDir "plain-build-summary.json"
$fakeBuildDir = Join-Path $resolvedWorkingDir "fake-pdf-build"
$fakeCtestPath = Join-Path $resolvedWorkingDir "fake-ctest.cmd"
$fakePythonPath = Join-Path $resolvedWorkingDir "fake-python.cmd"

New-Item -ItemType Directory -Path (Join-Path $plainBuildRepoRoot "build\tmp") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $plainBuildRepoRoot "scripts") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $plainBuildRepoRoot "test") -Force | Out-Null
Copy-Item -LiteralPath $scriptPath -Destination (Join-Path $plainBuildRepoRoot "scripts\check_pdf_visual_release_gate_preflight.ps1")
Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $plainBuildRepoRoot "test\pdf_regression_manifest.json")
& powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File (Join-Path $plainBuildRepoRoot "scripts\check_pdf_visual_release_gate_preflight.ps1") `
    -OutputJson $plainBuildSummaryPath `
    -MinFreeMemoryMB 1 | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "PDF visual release gate preflight should report not_ready for a plain build directory without failing."
}

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
        -MinFreeMemoryMB 1 `
        -Strict | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "PDF visual release gate preflight should pass against the contract-complete fake fixture."
    }

    $lowMemorySummaryPath = Join-Path $resolvedWorkingDir "preflight-low-memory-summary.json"
    & powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
        -File $scriptPath `
        -BuildDir $fakeBuildDir `
        -OutputJson $lowMemorySummaryPath `
        -CTestExecutable $fakeCtestPath `
        -MinFreeMemoryMB 999999 | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "PDF visual release gate preflight should report low-memory not_ready without failing when -Strict is not set."
    }

    $visualGatePreflightSummaryPath = Join-Path $resolvedWorkingDir "visual-gate-preflight-summary.json"
    $visualGateOutputDir = Join-Path $resolvedWorkingDir "visual-gate-output"
    & powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
        -File $visualGatePath `
        -BuildDir $fakeBuildDir `
        -OutputDir $visualGateOutputDir `
        -PreflightJson $visualGatePreflightSummaryPath `
        -MinFreeMemoryMB 1 `
        -PreflightOnly | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "PDF visual release gate -PreflightOnly should pass against the contract-complete fake fixture."
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

$plainBuildSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $plainBuildSummaryPath | ConvertFrom-Json
Assert-True -Condition ($plainBuildSummary.status -eq "not_ready") `
    -Message "Plain build directory preflight should stay not_ready."
Assert-True -Condition ([string]$plainBuildSummary.build_dir_source -eq "requested") `
    -Message "Plain build directory preflight should not auto-select build without CMake metadata."
Assert-True -Condition ([string]$plainBuildSummary.build_dir -like "*.bpdf-roundtrip-msvc") `
    -Message "Plain build directory preflight should keep the requested default build directory."
$plainBuildCandidates = @($plainBuildSummary.build_dir_auto_candidates)
Assert-True -Condition ($plainBuildCandidates.Count -eq 2) `
    -Message "Plain build directory preflight should record both automatic build directory candidates."
$plainBuildCandidate = @($plainBuildCandidates | Where-Object { [string]$_.relative_path -eq "build" }) | Select-Object -First 1
Assert-True -Condition ($null -ne $plainBuildCandidate) `
    -Message "Plain build directory preflight should record the build auto-candidate."
Assert-True -Condition ([bool]$plainBuildCandidate.exists -eq $true) `
    -Message "Plain build auto-candidate should report the existing build directory."
Assert-True -Condition ([bool]$plainBuildCandidate.cmake_cache_exists -eq $false) `
    -Message "Plain build auto-candidate should report the missing CMakeCache.txt."
Assert-True -Condition ([bool]$plainBuildCandidate.ctest_manifest_exists -eq $false) `
    -Message "Plain build auto-candidate should report the missing CTest manifest."
Assert-True -Condition ([bool]$plainBuildCandidate.looks_reusable -eq $false) `
    -Message "Plain build auto-candidate should not look reusable without CMake metadata."
$plainOutBuildCandidate = @($plainBuildCandidates | Where-Object { [string]$_.relative_path -eq "out\build" }) | Select-Object -First 1
Assert-True -Condition ($null -ne $plainOutBuildCandidate) `
    -Message "Plain build directory preflight should record the out\build auto-candidate."
Assert-True -Condition ([bool]$plainOutBuildCandidate.looks_reusable -eq $false) `
    -Message "Plain out\build auto-candidate should not look reusable."
Assert-True -Condition (($plainBuildSummary.blocking_checks | ForEach-Object { [string]$_ }) -contains "build_dir_exists") `
    -Message "Plain build directory preflight should report the missing requested build directory."
Assert-True -Condition ([int]$plainBuildSummary.blocking_summary.build_dir_entry_count -eq 0) `
    -Message "Plain build directory preflight should not count entries from an unrelated build directory."
Assert-True -Condition ([int]$plainBuildSummary.output_gap_count -eq 3) `
    -Message "Plain build directory preflight should report the three missing output groups."
Assert-True -Condition ([int]$plainBuildSummary.missing_output_count -gt 0) `
    -Message "Plain build directory preflight should report missing output totals."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-True -Condition ($summary.status -eq "ready") `
    -Message "Preflight summary should be ready for a reusable fake PDF build."
Assert-True -Condition ([int]$summary.blocking_summary.blocking_check_count -eq 0) `
    -Message "Ready preflight should expose zero blocking checks."
Assert-True -Condition ([int]$summary.output_gap_count -eq 0) `
    -Message "Ready preflight should expose zero missing output groups."
Assert-True -Condition ([int]$summary.missing_output_count -eq 0) `
    -Message "Ready preflight should expose zero missing outputs."
Assert-True -Condition ([int]$summary.blocking_summary.missing_cli_pdf_count -eq 0) `
    -Message "Ready preflight should expose zero missing CLI PDFs."
Assert-True -Condition ([int]$summary.blocking_summary.missing_visual_baseline_pdf_count -eq 0) `
    -Message "Ready preflight should expose zero missing visual baseline PDFs."
Assert-True -Condition ([int]$summary.blocking_summary.missing_cjk_text_layer_pdf_count -eq 0) `
    -Message "Ready preflight should expose zero missing CJK text-layer PDFs."
Assert-True -Condition ([int]$summary.blocking_summary.visual_baseline_sample_count -gt 0) `
    -Message "Ready preflight should report the visual baseline sample count."
Assert-True -Condition ([int]$summary.blocking_summary.cjk_text_layer_sample_count -gt 0) `
    -Message "Ready preflight should report the CJK text-layer sample count."

$lowMemorySummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $lowMemorySummaryPath | ConvertFrom-Json
Assert-True -Condition ($lowMemorySummary.status -eq "not_ready") `
    -Message "Low-memory preflight should be not_ready."
Assert-True -Condition ([bool]$lowMemorySummary.blocking_summary.memory_guard_blocked -eq $true) `
    -Message "Low-memory preflight should expose the memory guard blocker summary."
Assert-True -Condition (($lowMemorySummary.blocking_checks | ForEach-Object { [string]$_ }) -contains "workstation_free_memory_available") `
    -Message "Low-memory preflight should expose workstation_free_memory_available as a blocker."

foreach ($name in @(
    "build_dir_exists",
    "cmake_cache_exists",
    "ctest_manifest_exists",
    "ctest_list_contains_pdf_gate_tests",
    "workstation_free_memory_available",
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
    "[int]`$MinFreeMemoryMB = 2048",
    "[switch]`$SkipMemoryGuard",
    "scripts\check_pdf_visual_release_gate_preflight.ps1",
    "-MinFreeMemoryMB `$MinFreeMemoryMB",
    "-Strict",
    "PDF visual release gate preflight failed",
    "-PreflightOnly cannot be used with -SkipPreflight"
)) {
    Assert-True -Condition ($visualGateText -match [regex]::Escape($expectedText)) `
        -Message "PDF visual release gate should keep preflight contract marker '$expectedText'."
}
foreach ($expectedText in @(
    "FEATHERDOC_RENDER_PYTHON_EXECUTABLE",
    "base Python",
    ".venv-word-visual-smoke\Scripts\python.exe",
    "tmp\render-venv\Scripts\python.exe",
    ".venv-pdf-visual-smoke\Scripts\python.exe",
    "No reusable render Python with PIL and fitz was found"
)) {
    Assert-True -Condition ($visualGateText -match [regex]::Escape($expectedText)) `
        -Message "PDF visual release gate should keep reusable render Python marker '$expectedText'."
}
foreach ($forbiddenText in @(
    "-m venv",
    "pip install",
    "pillow pymupdf",
    "Creating local Python environment",
    "Installing Pillow and PyMuPDF"
)) {
    Assert-True -Condition ($visualGateText -notmatch [regex]::Escape($forbiddenText)) `
        -Message "PDF visual release gate should not create or install render Python dependencies during the full gate: '$forbiddenText'."
}

$preflightText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
foreach ($expectedText in @(
    "Resolve-PreferredBuildDir",
    "Get-BuildDirectorySnapshot",
    '"build", "out\build"',
    "candidateLooksReusable",
    "build_dir_auto_candidates",
    "relative_path",
    "looks_reusable",
    'Source = "auto:$candidate"',
    "cmake_cache_exists",
    "build_dir_source",
    "requested_build_dir",
    "blocking_summary",
    "output_gap_count",
    "missing_output_count",
    "missing_visual_baseline_pdf_count",
    "missing_cjk_text_layer_pdf_count",
    "[int]`$MinFreeMemoryMB = 2048",
    "[switch]`$SkipMemoryGuard",
    "Get-LocalMemorySnapshot",
    "Get-CimInstance Win32_OperatingSystem",
    "workstation_free_memory_available",
    "memory_guard_blocked",
    "entries_preview",
    "Resolve-BasePythonCandidate",
    "FEATHERDOC_PYTHON_EXECUTABLE",
    ".cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe",
    "Get-Command python",
    "Get-Command python3",
    'source = "base Python"'
)) {
    Assert-True -Condition ($preflightText -match [regex]::Escape($expectedText)) `
        -Message "PDF visual release gate preflight should keep build-dir selection marker '$expectedText'."
}

Write-Host "PDF visual release gate preflight contract passed."
