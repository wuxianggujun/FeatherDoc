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

function Assert-FileHasNoBom {
    param(
        [string]$Path,
        [string]$Message
    )

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        throw "$Message Path='$Path'."
    }
}

function Assert-PdfPreflightOutputEncoding {
    param(
        [object]$Summary,
        [string]$Message
    )

    Assert-True -Condition ([string]$Summary.output_encoding -eq "UTF-8 without BOM") `
        -Message $Message
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
$renderPdfPagesPath = Join-Path $resolvedRepoRoot "scripts\render_pdf_pages.py"
$manifestPath = Join-Path $resolvedRepoRoot "test\pdf_regression_manifest.json"
$summaryPath = Join-Path $resolvedWorkingDir "preflight-summary.json"
$plainBuildRepoRoot = Join-Path $resolvedWorkingDir "plain-build-repo"
$plainBuildSummaryPath = Join-Path $resolvedWorkingDir "plain-build-summary.json"
$plainBuildDefaultSummaryPath = Join-Path $plainBuildRepoRoot "output\pdf-visual-release-gate-preflight-current\summary.json"
$malformedManifestRepoRoot = Join-Path $resolvedWorkingDir "malformed-manifest-repo"
$malformedManifestSummaryPath = Join-Path $resolvedWorkingDir "malformed-manifest-summary.json"
$disabledBuildDir = Join-Path $resolvedWorkingDir "disabled-pdf-build"
$disabledBuildSummaryPath = Join-Path $resolvedWorkingDir "disabled-pdf-build-summary.json"
$disabledOverrideSummaryPath = Join-Path $resolvedWorkingDir "disabled-pdf-build-override-summary.json"
$disabledCtestMarkerPath = Join-Path $resolvedWorkingDir "disabled-ctest-invoked.txt"
$disabledCtestPath = Join-Path $resolvedWorkingDir "disabled-ctest.cmd"
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
& powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File (Join-Path $plainBuildRepoRoot "scripts\check_pdf_visual_release_gate_preflight.ps1") `
    -MinFreeMemoryMB 1 | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "PDF visual release gate preflight should report not_ready and write the default summary without failing."
}
if (-not (Test-Path -LiteralPath $plainBuildDefaultSummaryPath -PathType Leaf)) {
    throw "PDF visual release gate preflight should write the default current raw summary when -OutputJson is omitted."
}
Assert-FileHasNoBom -Path $plainBuildDefaultSummaryPath `
    -Message "Default current raw summary should be UTF-8 without BOM."
$plainBuildDefaultSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $plainBuildDefaultSummaryPath | ConvertFrom-Json
Assert-True -Condition ([string]$plainBuildDefaultSummary.schema -eq "featherdoc.pdf_visual_release_gate_preflight.v1") `
    -Message "Default current raw summary should expose the stable preflight schema."
Assert-PdfPreflightOutputEncoding -Summary $plainBuildDefaultSummary `
    -Message "Default current raw summary should expose UTF-8 without BOM output encoding."
Assert-True -Condition ([string]$plainBuildDefaultSummary.status -eq "not_ready") `
    -Message "Default current raw summary should preserve the preflight status."
Assert-True -Condition (($plainBuildDefaultSummary.blocking_checks | ForEach-Object { [string]$_ }) -contains "build_dir_exists") `
    -Message "Default current raw summary should preserve blocking checks from the omitted -OutputJson run."
Assert-True -Condition ($null -ne $plainBuildDefaultSummary.PSObject.Properties["recommended_recovery_steps"]) `
    -Message "Default current raw summary should expose ordered recovery steps."
Assert-True -Condition ([string]$plainBuildDefaultSummary.recommended_recovery_steps[0].id -eq "prepare_pdf_dependencies") `
    -Message "Plain-build recovery should start by preparing PDF dependencies."
Assert-True -Condition ([string]$plainBuildDefaultSummary.minimum_risk_next_action -match [regex]::Escape("Provide reusable PDFio/PDFium inputs first")) `
    -Message "Plain-build minimum-risk next action should explain that PDF dependency inputs come first."

New-Item -ItemType Directory -Path (Join-Path $malformedManifestRepoRoot "scripts") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $malformedManifestRepoRoot "test") -Force | Out-Null
Copy-Item -LiteralPath $scriptPath -Destination (Join-Path $malformedManifestRepoRoot "scripts\check_pdf_visual_release_gate_preflight.ps1")
"{ invalid json" | Set-Content -LiteralPath (Join-Path $malformedManifestRepoRoot "test\pdf_regression_manifest.json") -Encoding UTF8
& powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File (Join-Path $malformedManifestRepoRoot "scripts\check_pdf_visual_release_gate_preflight.ps1") `
    -OutputJson $malformedManifestSummaryPath `
    -MinFreeMemoryMB 1 | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "PDF visual release gate preflight should report malformed manifest evidence without failing when -Strict is not set."
}

New-Item -ItemType Directory -Path $disabledBuildDir -Force | Out-Null
@"
FEATHERDOC_BUILD_PDF:BOOL=OFF
FEATHERDOC_BUILD_PDF_IMPORT:BOOL=OFF
"@ | Set-Content -LiteralPath (Join-Path $disabledBuildDir "CMakeCache.txt") -Encoding UTF8
@"
add_test(pdf_cli_export "cmd" "/c" "exit 0")
add_test(pdf_regression_manifest "cmd" "/c" "exit 0")
"@ | Set-Content -LiteralPath (Join-Path $disabledBuildDir "CTestTestfile.cmake") -Encoding UTF8
@"
@echo off
echo invoked > "$disabledCtestMarkerPath"
exit /b 0
"@ | Set-Content -LiteralPath $disabledCtestPath -Encoding ASCII
$overridePdfioSourceDir = Join-Path $resolvedWorkingDir "override-pdfio-src"
$overridePdfiumPrebuiltDir = Join-Path $resolvedWorkingDir "override-pdfium-prebuilt"
$overridePdfiumIncludeDir = Join-Path $overridePdfiumPrebuiltDir "include"
$overridePdfiumLibraryDir = Join-Path $overridePdfiumPrebuiltDir "lib"
$overridePdfiumBinDir = Join-Path $overridePdfiumPrebuiltDir "bin"
New-Item -ItemType Directory -Path $overridePdfioSourceDir -Force | Out-Null
New-Item -ItemType Directory -Path $overridePdfiumIncludeDir -Force | Out-Null
New-Item -ItemType Directory -Path $overridePdfiumLibraryDir -Force | Out-Null
New-Item -ItemType Directory -Path $overridePdfiumBinDir -Force | Out-Null
Set-Content -LiteralPath (Join-Path $overridePdfioSourceDir "pdfio.h") -Encoding UTF8 -Value "/* fake pdfio override */"
Set-Content -LiteralPath (Join-Path $overridePdfiumIncludeDir "fpdfview.h") -Encoding UTF8 -Value "/* fake pdfium override */"
Set-Content -LiteralPath (Join-Path $overridePdfiumLibraryDir "pdfium.dll.lib") -Encoding UTF8 -Value "fake import lib"
Set-Content -LiteralPath (Join-Path $overridePdfiumBinDir "pdfium.dll") -Encoding UTF8 -Value "fake runtime"
& powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File $scriptPath `
    -BuildDir $disabledBuildDir `
    -OutputJson $disabledBuildSummaryPath `
    -CTestExecutable $disabledCtestPath `
    -MinFreeMemoryMB 1 | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "PDF visual release gate preflight should report not_ready for disabled PDF build options without failing."
}
if (Test-Path -LiteralPath $disabledCtestMarkerPath -PathType Leaf) {
    throw "PDF visual release gate preflight should skip ctest -N when PDF build options are disabled."
}
& powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File $scriptPath `
    -BuildDir $disabledBuildDir `
    -OutputJson $disabledOverrideSummaryPath `
    -CTestExecutable $disabledCtestPath `
    -PdfioSourceDir $overridePdfioSourceDir `
    -PdfiumProvider "prebuilt" `
    -PdfiumPrebuiltRoot $overridePdfiumPrebuiltDir `
    -MinFreeMemoryMB 1 | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "PDF visual release gate preflight should accept dependency input overrides without failing."
}

New-Item -ItemType Directory -Path $fakeBuildDir -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $fakeBuildDir "test\pdf_cli_export") -Force | Out-Null
$fakePdfioSourceDir = Join-Path $resolvedWorkingDir "fake-pdfio-src"
$fakePdfiumPrebuiltDir = Join-Path $resolvedWorkingDir "fake-pdfium-prebuilt"
$fakePdfiumIncludeDir = Join-Path $fakePdfiumPrebuiltDir "include"
$fakePdfiumLibraryDir = Join-Path $fakePdfiumPrebuiltDir "lib"
$fakePdfiumBinDir = Join-Path $fakePdfiumPrebuiltDir "bin"
New-Item -ItemType Directory -Path $fakePdfioSourceDir -Force | Out-Null
New-Item -ItemType Directory -Path $fakePdfiumIncludeDir -Force | Out-Null
New-Item -ItemType Directory -Path $fakePdfiumLibraryDir -Force | Out-Null
New-Item -ItemType Directory -Path $fakePdfiumBinDir -Force | Out-Null
Set-Content -LiteralPath (Join-Path $fakePdfioSourceDir "pdfio.h") -Encoding UTF8 -Value "/* fake pdfio */"
Set-Content -LiteralPath (Join-Path $fakePdfiumIncludeDir "fpdfview.h") -Encoding UTF8 -Value "/* fake pdfium */"
Set-Content -LiteralPath (Join-Path $fakePdfiumLibraryDir "pdfium.lib") -Encoding UTF8 -Value "fake lib"
Set-Content -LiteralPath (Join-Path $fakePdfiumBinDir "pdfium.dll") -Encoding UTF8 -Value "fake runtime"
@"
FEATHERDOC_BUILD_PDF:BOOL=ON
FEATHERDOC_BUILD_PDF_IMPORT:BOOL=ON
FEATHERDOC_PDFIO_SOURCE_DIR:PATH=$fakePdfioSourceDir
FEATHERDOC_PDFIUM_PROVIDER:STRING=prebuilt
FEATHERDOC_PDFIUM_PREBUILT_ROOT:PATH=$fakePdfiumPrebuiltDir
FEATHERDOC_PDFIUM_LIBRARY:FILEPATH=$(Join-Path $fakePdfiumLibraryDir "pdfium.lib")
FEATHERDOC_PDFIUM_INCLUDE_DIR:PATH=$fakePdfiumIncludeDir
"@ | Set-Content -LiteralPath (Join-Path $fakeBuildDir "CMakeCache.txt") -Encoding UTF8
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
        -MinFreeMemoryMB 1 | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "PDF visual release gate preflight should write a not_ready summary for the contract-complete fake fixture."
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
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $visualGatePreflightOnlyOutput = @(& powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
            -File $visualGatePath `
            -BuildDir $fakeBuildDir `
            -OutputDir $visualGateOutputDir `
            -PreflightJson $visualGatePreflightSummaryPath `
            -MinFreeMemoryMB 1 `
            -PreflightOnly 2>&1)
        $visualGatePreflightOnlyExitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    $visualGatePreflightOnlyOutput | ForEach-Object { $_.ToString() } | Out-Host
    if ($visualGatePreflightOnlyExitCode -eq 0) {
        throw "PDF visual release gate -PreflightOnly should reject synthetic fake fixture evidence."
    }
    if (-not (Test-Path -LiteralPath $visualGatePreflightSummaryPath -PathType Leaf)) {
        throw "PDF visual release gate -PreflightOnly should write its rejected preflight summary."
    }
    Assert-FileHasNoBom -Path $visualGatePreflightSummaryPath `
        -Message "PDF visual release gate -PreflightOnly preflight summary should be UTF-8 without BOM."
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
Assert-FileHasNoBom -Path $plainBuildSummaryPath `
    -Message "Plain build summary should be UTF-8 without BOM."
Assert-FileHasNoBom -Path $malformedManifestSummaryPath `
    -Message "Malformed manifest summary should be UTF-8 without BOM."
Assert-FileHasNoBom -Path $disabledBuildSummaryPath `
    -Message "Disabled build summary should be UTF-8 without BOM."
Assert-FileHasNoBom -Path $disabledOverrideSummaryPath `
    -Message "Disabled override summary should be UTF-8 without BOM."
Assert-FileHasNoBom -Path $summaryPath `
    -Message "Synthetic fixture summary should be UTF-8 without BOM."
Assert-FileHasNoBom -Path $lowMemorySummaryPath `
    -Message "Low-memory summary should be UTF-8 without BOM."

$plainBuildSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $plainBuildSummaryPath | ConvertFrom-Json
Assert-PdfPreflightOutputEncoding -Summary $plainBuildSummary `
    -Message "Plain build summary should expose UTF-8 without BOM output encoding."
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
Assert-True -Condition ([bool]$plainBuildCandidate.pdf_build_options_enabled -eq $false) `
    -Message "Plain build auto-candidate should report PDF build options as not enabled."
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

$malformedManifestSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $malformedManifestSummaryPath | ConvertFrom-Json
Assert-PdfPreflightOutputEncoding -Summary $malformedManifestSummary `
    -Message "Malformed manifest summary should expose UTF-8 without BOM output encoding."
Assert-True -Condition ($malformedManifestSummary.status -eq "not_ready") `
    -Message "Malformed manifest preflight should stay not_ready."
Assert-True -Condition (($malformedManifestSummary.blocking_checks | ForEach-Object { [string]$_ }) -contains "pdf_regression_manifest_readable") `
    -Message "Malformed manifest preflight should expose pdf_regression_manifest_readable as a blocker."
$malformedManifestCheck = @($malformedManifestSummary.checks | Where-Object { [string]$_.name -eq "pdf_regression_manifest_readable" })[0]
Assert-True -Condition ([string]$malformedManifestCheck.status -eq "missing") `
    -Message "Malformed manifest readable check should be missing."
Assert-True -Condition ([bool]$malformedManifestCheck.details.manifest_exists -eq $true) `
    -Message "Malformed manifest readable check should preserve that the manifest file exists."
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace([string]$malformedManifestCheck.details.error_message)) `
    -Message "Malformed manifest readable check should include a parse error message."

$disabledBuildSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $disabledBuildSummaryPath | ConvertFrom-Json
Assert-PdfPreflightOutputEncoding -Summary $disabledBuildSummary `
    -Message "Disabled build summary should expose UTF-8 without BOM output encoding."
Assert-True -Condition ($disabledBuildSummary.status -eq "not_ready") `
    -Message "Disabled PDF build preflight should stay not_ready."
Assert-True -Condition (($disabledBuildSummary.blocking_checks | ForEach-Object { [string]$_ }) -contains "pdf_build_options_enabled") `
    -Message "Disabled PDF build preflight should report the disabled PDF build options blocker."
Assert-True -Condition (($disabledBuildSummary.blocking_checks | ForEach-Object { [string]$_ }) -contains "pdf_dependency_inputs_ready") `
    -Message "Disabled PDF build preflight should report missing PDF dependency inputs as a blocker."
Assert-True -Condition (($disabledBuildSummary.blocking_checks | ForEach-Object { [string]$_ }) -contains "ctest_list_contains_pdf_gate_tests") `
    -Message "Disabled PDF build preflight should keep ctest registration as blocked until PDF options are enabled."
$disabledPdfBuildOptionsCheck = @($disabledBuildSummary.checks | Where-Object { [string]$_.name -eq "pdf_build_options_enabled" })[0]
Assert-True -Condition ([string]$disabledPdfBuildOptionsCheck.status -eq "missing") `
    -Message "Disabled PDF build options check should be missing."
Assert-True -Condition ([string]$disabledPdfBuildOptionsCheck.message -match [regex]::Escape("Prepare real PDFio/PDFium inputs")) `
    -Message "Disabled PDF build options check should explain that real PDF dependencies must be prepared first."
Assert-True -Condition ([string]$disabledPdfBuildOptionsCheck.message -match [regex]::Escape("-DFEATHERDOC_BUILD_PDF=ON")) `
    -Message "Disabled PDF build options check should explain how to reconfigure FEATHERDOC_BUILD_PDF."
Assert-True -Condition ([string]$disabledPdfBuildOptionsCheck.message -match [regex]::Escape("-DFEATHERDOC_BUILD_PDF_IMPORT=ON")) `
    -Message "Disabled PDF build options check should explain how to reconfigure FEATHERDOC_BUILD_PDF_IMPORT."
Assert-True -Condition ([string]$disabledPdfBuildOptionsCheck.message -match [regex]::Escape("before starting the full PDF visual gate")) `
    -Message "Disabled PDF build options check should prevent starting the full PDF visual gate before preflight is repaired."
$disabledDependencyCheck = @($disabledBuildSummary.checks | Where-Object { [string]$_.name -eq "pdf_dependency_inputs_ready" })[0]
Assert-True -Condition ([string]$disabledDependencyCheck.status -eq "missing") `
    -Message "Disabled PDF build dependency check should be missing."
Assert-True -Condition ([string]$disabledDependencyCheck.details.status -eq "not_ready") `
    -Message "Disabled PDF build dependency check should expose the dependency summary status."
Assert-True -Condition ([int]$disabledDependencyCheck.details.missing_input_count -gt 0) `
    -Message "Disabled PDF build dependency check should expose missing dependency input count."
$disabledCtestCheck = @($disabledBuildSummary.checks | Where-Object { [string]$_.name -eq "ctest_list_contains_pdf_gate_tests" })[0]
Assert-True -Condition ([string]$disabledCtestCheck.status -eq "skipped") `
    -Message "Disabled PDF build preflight should skip ctest list enumeration."
Assert-True -Condition ([string]$disabledCtestCheck.message -match "PDF build/import options") `
    -Message "Disabled PDF build ctest check should explain that PDF options must be enabled first."
Assert-True -Condition ([bool]$disabledCtestCheck.details.pdf_build_options_enabled -eq $false) `
    -Message "Disabled PDF build ctest check should expose the PDF build option readiness flag."
Assert-True -Condition ((@($disabledCtestCheck.details.disabled_pdf_build_options) | ForEach-Object { [string]$_ }) -contains "FEATHERDOC_BUILD_PDF") `
    -Message "Disabled PDF build ctest check should expose FEATHERDOC_BUILD_PDF as disabled."
Assert-True -Condition ((@($disabledCtestCheck.details.disabled_pdf_build_options) | ForEach-Object { [string]$_ }) -contains "FEATHERDOC_BUILD_PDF_IMPORT") `
    -Message "Disabled PDF build ctest check should expose FEATHERDOC_BUILD_PDF_IMPORT as disabled."

$disabledOverrideSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $disabledOverrideSummaryPath | ConvertFrom-Json
Assert-PdfPreflightOutputEncoding -Summary $disabledOverrideSummary `
    -Message "Disabled override summary should expose UTF-8 without BOM output encoding."
Assert-True -Condition ($disabledOverrideSummary.status -eq "not_ready") `
    -Message "Disabled PDF build override preflight should still be not_ready because build options are disabled."
Assert-True -Condition (($disabledOverrideSummary.blocking_checks | ForEach-Object { [string]$_ }) -notcontains "pdf_dependency_inputs_ready") `
    -Message "Dependency overrides should clear the dependency input blocker."
Assert-True -Condition (($disabledOverrideSummary.blocking_checks | ForEach-Object { [string]$_ }) -contains "pdf_build_options_enabled") `
    -Message "Dependency overrides should not bypass disabled PDF build options."
Assert-True -Condition ([string]$disabledOverrideSummary.pdf_dependency_inputs.status -eq "ready") `
    -Message "Dependency overrides should make the dependency input summary ready."
Assert-True -Condition ([string]$disabledOverrideSummary.pdf_dependency_inputs.selected_pdfium_provider -eq "prebuilt") `
    -Message "Dependency overrides should select the prebuilt PDFium provider."
Assert-True -Condition ([string]$disabledOverrideSummary.pdf_dependency_inputs.dependency_overrides.PdfiumProvider -eq "prebuilt") `
    -Message "Dependency override summary should expose the provider override."
Assert-True -Condition ([string]$disabledOverrideSummary.pdf_dependency_inputs.pdfium_prebuilt_root -eq [System.IO.Path]::GetFullPath($overridePdfiumPrebuiltDir)) `
    -Message "Dependency overrides should replace CMakeCache dependency inputs for the dependency check."
Assert-True -Condition ([string]$disabledOverrideSummary.recommended_recovery_steps[0].id -eq "restore_pdf_build_options") `
    -Message "Disabled PDF build override recovery should focus on restoring PDF build options first."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-True -Condition ([string]$summary.schema -eq "featherdoc.pdf_visual_release_gate_preflight.v1") `
    -Message "Synthetic fixture preflight should expose the stable raw summary schema."
Assert-PdfPreflightOutputEncoding -Summary $summary `
    -Message "Synthetic fixture summary should expose UTF-8 without BOM output encoding."
Assert-True -Condition ($summary.status -eq "not_ready") `
    -Message "Preflight summary should reject reusable fake PDF build evidence."
Assert-True -Condition ([int]$summary.blocking_summary.blocking_check_count -eq 1) `
    -Message "Synthetic fixture preflight should expose one blocking check."
Assert-True -Condition (($summary.blocking_checks | ForEach-Object { [string]$_ }) -contains "synthetic_preflight_evidence") `
    -Message "Synthetic fixture preflight should expose synthetic_preflight_evidence as a blocker."
$syntheticEvidenceCheck = @($summary.checks | Where-Object { [string]$_.name -eq "synthetic_preflight_evidence" })[0]
Assert-True -Condition ([string]$syntheticEvidenceCheck.status -eq "blocked") `
    -Message "Synthetic evidence check should be blocked for fake fixture evidence."
Assert-True -Condition ([string]$syntheticEvidenceCheck.message -match [regex]::Escape("cannot release the real PDF visual gate")) `
    -Message "Synthetic evidence check should explain that fake fixture evidence cannot release the real gate."
Assert-True -Condition ([int]$summary.output_gap_count -eq 0) `
    -Message "Synthetic fixture preflight should still expose zero missing output groups."
Assert-True -Condition ([int]$summary.missing_output_count -eq 0) `
    -Message "Synthetic fixture preflight should still expose zero missing outputs."
Assert-True -Condition ([int]$summary.blocking_summary.missing_cli_pdf_count -eq 0) `
    -Message "Synthetic fixture preflight should expose zero missing CLI PDFs."
Assert-True -Condition ([int]$summary.blocking_summary.missing_visual_baseline_pdf_count -eq 0) `
    -Message "Synthetic fixture preflight should expose zero missing visual baseline PDFs."
Assert-True -Condition ([int]$summary.blocking_summary.missing_cjk_text_layer_pdf_count -eq 0) `
    -Message "Synthetic fixture preflight should expose zero missing CJK text-layer PDFs."
Assert-True -Condition ([int]$summary.blocking_summary.visual_baseline_sample_count -gt 0) `
    -Message "Synthetic fixture preflight should report the visual baseline sample count."
Assert-True -Condition ([int]$summary.blocking_summary.cjk_text_layer_sample_count -gt 0) `
    -Message "Synthetic fixture preflight should report the CJK text-layer sample count."
Assert-True -Condition ($null -ne $summary.PSObject.Properties["pdf_dependency_inputs"]) `
    -Message "Synthetic fixture preflight should expose the PDF dependency input summary."
Assert-True -Condition ($null -ne $summary.pdf_dependency_inputs.PSObject.Properties["status"]) `
    -Message "PDF dependency input summary should expose status."
Assert-True -Condition ($null -ne $summary.pdf_dependency_inputs.PSObject.Properties["selected_pdfium_provider"]) `
    -Message "PDF dependency input summary should expose the selected PDFium provider."
Assert-True -Condition ([string]$summary.pdf_dependency_inputs.status -eq "ready") `
    -Message "Synthetic fixture preflight should pass dependency input values from CMakeCache."
Assert-True -Condition ([string]$summary.pdf_dependency_inputs.selected_pdfium_provider -eq "prebuilt") `
    -Message "Synthetic fixture preflight should preserve the CMakeCache PDFium provider selection."
Assert-True -Condition ([bool]$summary.pdf_dependency_inputs.pdfio_ready -eq $true) `
    -Message "Synthetic fixture preflight should report the CMakeCache PDFio source input as ready."
Assert-True -Condition ([bool]$summary.pdf_dependency_inputs.pdfium_ready -eq $true) `
    -Message "Synthetic fixture preflight should report the CMakeCache PDFium prebuilt inputs as ready."
Assert-True -Condition ([string]$summary.pdf_dependency_inputs.cmake_cache_variables.FEATHERDOC_PDFIUM_PROVIDER -eq "prebuilt") `
    -Message "Dependency input summary should retain CMakeCache PDFium provider evidence."
$expectedPdfiumPrebuiltRoot = [System.IO.Path]::GetFullPath($fakePdfiumPrebuiltDir)
Assert-True -Condition ([string]$summary.pdf_dependency_inputs.cmake_cache_variables.FEATHERDOC_PDFIUM_PREBUILT_ROOT -eq $expectedPdfiumPrebuiltRoot) `
    -Message "Dependency input summary should retain CMakeCache PDFium prebuilt root evidence."
Assert-True -Condition ([string]$summary.pdf_dependency_inputs.pdfium_prebuilt_root -eq $expectedPdfiumPrebuiltRoot) `
    -Message "Synthetic fixture preflight should expose the PDFium prebuilt root from CMakeCache."
Assert-True -Condition ([bool]$summary.pdf_dependency_inputs.pdfium_prebuilt_root_exists -eq $true) `
    -Message "Synthetic fixture preflight should expose whether the PDFium prebuilt root exists."
Assert-True -Condition ([string]$summary.blocking_summary.pdf_dependency_inputs_status -eq [string]$summary.pdf_dependency_inputs.status) `
    -Message "Synthetic fixture preflight should mirror PDF dependency input status into blocking_summary."
Assert-True -Condition ([int]$summary.blocking_summary.pdf_dependency_missing_input_count -eq [int]$summary.pdf_dependency_inputs.missing_input_count) `
    -Message "Synthetic fixture preflight should mirror the PDF dependency missing input count into blocking_summary."
Assert-True -Condition ([string]$summary.blocking_summary.selected_pdfium_provider -eq [string]$summary.pdf_dependency_inputs.selected_pdfium_provider) `
    -Message "Synthetic fixture preflight should mirror the selected PDFium provider into blocking_summary."
Assert-True -Condition ([bool]$summary.blocking_summary.pdf_build_options_enabled -eq $true) `
    -Message "Synthetic fixture preflight should report enabled PDF build options."
Assert-True -Condition (@($summary.blocking_summary.disabled_pdf_build_options).Count -eq 0) `
    -Message "Synthetic fixture preflight should report zero disabled PDF build options."
Assert-True -Condition (@($summary.blocking_summary.missing_pdf_build_options).Count -eq 0) `
    -Message "Synthetic fixture preflight should report zero missing PDF build options."
Assert-True -Condition ([string]$summary.evidence_kind -eq "synthetic_fixture") `
    -Message "Synthetic fixture preflight should mark fake fixture evidence as synthetic."
Assert-True -Condition ([int]$summary.evidence_kind_details.synthetic_marker_count -gt 0) `
    -Message "Synthetic fixture preflight should expose synthetic evidence markers for fake fixtures."
Assert-True -Condition ([int]$summary.blocking_summary.synthetic_evidence_marker_count -gt 0) `
    -Message "Ready preflight blocking summary should mirror synthetic evidence marker count."
Assert-True -Condition ([string]$summary.recommended_recovery_steps[0].id -eq "preflight_ready") `
    -Message "Synthetic fixture summary should expose the ready-state recovery sentinel when no real blockers remain."
Assert-True -Condition ([string]$summary.minimum_risk_next_action -match [regex]::Escape("No blocking preflight recovery steps remain")) `
    -Message "Synthetic fixture minimum-risk next action should explain that no preflight blockers remain."
$syntheticMarkers = ($summary.evidence_kind_details.synthetic_markers | ForEach-Object { [string]$_ }) -join "`n"
Assert-True -Condition ($syntheticMarkers -match [regex]::Escape("fake-pdf-build")) `
    -Message "Ready fake fixture preflight should identify the fake build directory evidence marker."
Assert-True -Condition ($syntheticMarkers -match [regex]::Escape("fake-pdfio-src")) `
    -Message "Ready fake fixture preflight should identify the fake PDFio dependency evidence marker."
Assert-True -Condition ($syntheticMarkers -match [regex]::Escape("fake-ctest.cmd")) `
    -Message "Ready fake fixture preflight should identify the fake CTest evidence marker."
Assert-True -Condition ($syntheticMarkers -match [regex]::Escape("fake-python.cmd")) `
    -Message "Ready fake fixture preflight should identify the fake render Python evidence marker."

$lowMemorySummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $lowMemorySummaryPath | ConvertFrom-Json
Assert-PdfPreflightOutputEncoding -Summary $lowMemorySummary `
    -Message "Low-memory summary should expose UTF-8 without BOM output encoding."
Assert-True -Condition ($lowMemorySummary.status -eq "not_ready") `
    -Message "Low-memory preflight should be not_ready."
Assert-True -Condition ([bool]$lowMemorySummary.blocking_summary.memory_guard_blocked -eq $true) `
    -Message "Low-memory preflight should expose the memory guard blocker summary."
Assert-True -Condition (($lowMemorySummary.blocking_checks | ForEach-Object { [string]$_ }) -contains "workstation_free_memory_available") `
    -Message "Low-memory preflight should expose workstation_free_memory_available as a blocker."
Assert-True -Condition ([string]$lowMemorySummary.recommended_recovery_steps[0].id -eq "stay_read_only_until_memory_recovers") `
    -Message "Low-memory preflight should prioritize the read-only memory recovery step."
Assert-True -Condition ([string]$lowMemorySummary.minimum_risk_next_action -match [regex]::Escape("read-only mode")) `
    -Message "Low-memory minimum-risk next action should explain the read-only recovery posture."

foreach ($name in @(
    "build_dir_exists",
    "cmake_cache_exists",
    "pdf_dependency_inputs_ready",
    "pdf_build_options_enabled",
    "ctest_manifest_exists",
    "ctest_list_contains_pdf_gate_tests",
    "workstation_free_memory_available",
    "pdf_visual_gate_scripts_exist",
    "pdf_regression_manifest_exists",
    "pdf_regression_manifest_readable",
    "pdf_cli_export_baseline_pdfs_exist",
    "visual_baseline_manifest_pdfs_exist",
    "cjk_text_layer_manifest_pdfs_exist",
    "render_python_reusable"
)) {
    Assert-CheckPassed -Summary $summary -Name $name
}

$visualGateText = Get-Content -Raw -Encoding UTF8 -LiteralPath $visualGatePath
$renderPdfPagesText = Get-Content -Raw -Encoding UTF8 -LiteralPath $renderPdfPagesPath
foreach ($expectedText in @(
    "[string]`$PreflightJson",
    "[switch]`$PreflightOnly",
    "[switch]`$FinalizeOnly",
    "[switch]`$SkipPreflight",
    "[string]`$PdfioSourceDir = `"`"",
    "[string]`$PdfiumPrebuiltRoot = `"`"",
    "[int]`$MinFreeMemoryMB = 2048",
    "[switch]`$SkipMemoryGuard",
    "scripts\check_pdf_visual_release_gate_preflight.ps1",
    "preflightArguments",
    "MinFreeMemoryMB = `$MinFreeMemoryMB",
    "SkipMemoryGuard = [bool]`$SkipMemoryGuard",
    "Strict = `$true",
    "PDF visual release gate preflight failed",
    "-FinalizeOnly cannot be used with -PreflightOnly",
    "-PreflightOnly cannot be used with -SkipPreflight"
)) {
    Assert-True -Condition ($visualGateText -match [regex]::Escape($expectedText)) `
        -Message "PDF visual release gate should keep preflight contract marker '$expectedText'."
}
foreach ($expectedText in @(
    "pdf_visual_gate_core_pass_summary_trace",
    "summary_detail_payload_included",
    "summary_detail_status",
    "core_pass_written_before_detail_payload",
    "Visual gate core pass summary written",
    "--skip-contact-sheet"
)) {
    Assert-True -Condition ($visualGateText -match [regex]::Escape($expectedText)) `
        -Message "PDF visual release gate should write a core pass summary before large detail payload serialization: '$expectedText'."
}
foreach ($expectedText in @(
    "--skip-contact-sheet",
    "contact_sheet_skipped"
)) {
    Assert-True -Condition ($renderPdfPagesText -match [regex]::Escape($expectedText)) `
        -Message "PDF render helper should preserve the skipped per-sample contact sheet marker '$expectedText'."
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
    '[string]$OutputJson = "output/pdf-visual-release-gate-preflight-current/summary.json"',
    'schema = "featherdoc.pdf_visual_release_gate_preflight.v1"',
    'output_encoding = "UTF-8 without BOM"',
    'Write-Utf8NoBomFile',
    'if ([string]::IsNullOrWhiteSpace($OutputJson))',
    "Resolve-PreferredBuildDir",
    "Get-BuildDirectorySnapshot",
    "Get-CMakeCachePdfBuildOptions",
    "Test-CMakeCacheBoolOn",
    '"build", "out\build"',
    "candidateLooksReusable",
    "build_dir_auto_candidates",
    "relative_path",
    "looks_reusable",
    "pdf_build_options = @(`$pdfOptionSnapshot)",
    "candidateExists -and `$cmakeCacheExists -and `$ctestManifestExists -and `$pdfBuildOptionsReady",
    'Source = "auto:$candidate"',
    "cmake_cache_exists",
    "pdf_build_options_enabled",
    "FEATHERDOC_BUILD_PDF",
    "FEATHERDOC_BUILD_PDF_IMPORT",
    "disabled_pdf_build_options",
    "missing_pdf_build_options",
    "pdf_dependency_inputs_ready",
    "OverrideArguments",
    "dependency_overrides",
    "PdfioSourceDir",
    "PdfiumPrebuiltRoot",
    "PDF dependency inputs are not ready for the full visual gate.",
    "PDF dependency inputs are ready for the full visual gate.",
    "Skipped ctest -N because PDF build/import options are not enabled in CMakeCache.txt.",
    "pdf_build_options_enabled = [bool]`$pdfBuildOptionsReady",
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
    'source = "base Python"',
    "Get-PreflightEvidenceKind",
    "evidence_kind",
    "synthetic_fixture",
    "synthetic_markers"
)) {
    Assert-True -Condition ($preflightText -match [regex]::Escape($expectedText)) `
        -Message "PDF visual release gate preflight should keep build-dir selection marker '$expectedText'."
}

Write-Host "PDF visual release gate preflight contract passed."
exit 0
