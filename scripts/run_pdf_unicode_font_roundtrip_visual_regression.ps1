param(
    [string]$BuildDir = "build-pdf-unicode-font-visual-regression",
    [string]$OutputDir = "output/pdf-unicode-font-visual-regression",
    [int]$Dpi = 144
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[pdf-unicode-font-visual] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        return [System.IO.Path]::GetFullPath($InputPath)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath))
}

function Resolve-GeneratedPdfPath {
    param(
        [string[]]$CandidatePaths,
        [string]$Label
    )

    foreach ($candidate in $CandidatePaths) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }

    $checked = ($CandidatePaths | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }) -join "; "
    throw "Expected generated PDF for $Label was not found. Checked: $checked"
}

function Get-BasePython {
    if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_PYTHON_EXECUTABLE) -and
        (Test-Path $env:FEATHERDOC_PYTHON_EXECUTABLE)) {
        return (Resolve-Path $env:FEATHERDOC_PYTHON_EXECUTABLE).Path
    }

    $bundledPython = Join-Path $env:USERPROFILE ".cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
    if (Test-Path $bundledPython) {
        return (Resolve-Path $bundledPython).Path
    }

    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($python) {
        return $python.Source
    }

    $python3 = Get-Command python3 -ErrorAction SilentlyContinue
    if ($python3) {
        return $python3.Source
    }

    throw "Python was not found in PATH."
}

function Test-PythonImport {
    param(
        [string]$PythonCommand,
        [string]$ModuleName
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        & $PythonCommand -c "import $ModuleName" 1> $null 2> $null
        return $LASTEXITCODE -eq 0
    } catch {
        return $false
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
}

function Ensure-RenderPython {
    param([string]$RepoRoot)

    if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE) -and
        (Test-Path $env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE)) {
        $renderPython = (Resolve-Path $env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE).Path
        if ((Test-PythonImport -PythonCommand $renderPython -ModuleName "PIL") -and
            (Test-PythonImport -PythonCommand $renderPython -ModuleName "fitz")) {
            return $renderPython
        }
        throw "FEATHERDOC_RENDER_PYTHON_EXECUTABLE does not provide PIL and fitz."
    }

    $basePython = Get-BasePython
    if ((Test-PythonImport -PythonCommand $basePython -ModuleName "PIL") -and
        (Test-PythonImport -PythonCommand $basePython -ModuleName "fitz")) {
        return $basePython
    }

    $existingRenderPython = Join-Path $RepoRoot "tmp\render-venv\Scripts\python.exe"
    if ((Test-Path $existingRenderPython) -and
        (Test-PythonImport -PythonCommand $existingRenderPython -ModuleName "PIL") -and
        (Test-PythonImport -PythonCommand $existingRenderPython -ModuleName "fitz")) {
        return (Resolve-Path $existingRenderPython).Path
    }

    $venvDir = Join-Path $RepoRoot ".venv-pdf-visual-smoke"
    $venvPython = Join-Path $venvDir "Scripts\python.exe"
    if (-not (Test-Path $venvPython)) {
        Write-Step "Creating local Python environment at $venvDir"
        & $basePython -m venv $venvDir
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to create local Python virtual environment."
        }
    }

    if (-not (Test-PythonImport -PythonCommand $venvPython -ModuleName "PIL") -or
        -not (Test-PythonImport -PythonCommand $venvPython -ModuleName "fitz")) {
        Write-Step "Installing Pillow and PyMuPDF into the local environment"
        & $venvPython -m pip install --disable-pip-version-check pillow pymupdf 2>&1 |
            ForEach-Object { Write-Host $_ }
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to install render dependencies into the local environment."
        }
    }

    return $venvPython
}

function Build-ContactSheet {
    param(
        [string]$Python,
        [string]$ScriptPath,
        [string]$OutputPath,
        [string[]]$Images,
        [string[]]$Labels
    )

    & $Python $ScriptPath --output $OutputPath --columns 2 --thumb-width 420 --images $Images --labels $Labels
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build contact sheet at $OutputPath."
    }
}

function Invoke-CommandCapture {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments,
        [string]$OutputPath,
        [string]$FailureMessage
    )

    $commandOutput = @(& $ExecutablePath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
        $lines | Set-Content -Path $OutputPath -Encoding UTF8
    }
    foreach ($line in $lines) {
        Write-Host $line
    }
    if ($exitCode -ne 0) {
        throw $FailureMessage
    }
}

function Find-CjkFont {
    $candidates = @()

    if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_TEST_CJK_FONT)) {
        $candidates += $env:FEATHERDOC_TEST_CJK_FONT
    }

    $candidates += @(
        "C:/Windows/Fonts/Deng.ttf"
        "C:/Windows/Fonts/NotoSansSC-VF.ttf"
        "C:/Windows/Fonts/msyh.ttc"
        "C:/Windows/Fonts/simhei.ttf"
        "C:/Windows/Fonts/simsun.ttc"
    )

    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }

    return ""
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$reportDir = Join-Path $resolvedOutputDir "report"
$evidenceDir = Join-Path $resolvedOutputDir "evidence"
$fullPagesDir = Join-Path $evidenceDir "full\pages"
$subsetPagesDir = Join-Path $evidenceDir "subset\pages"
$renderScriptPath = Join-Path $repoRoot "scripts\render_pdf_pages.py"
$contactSheetScriptPath = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"
$testLogPath = Join-Path $reportDir "test.log"
$fullSummaryPath = Join-Path $reportDir "full-summary.json"
$subsetSummaryPath = Join-Path $reportDir "subset-summary.json"
$comparisonContactSheetPath = Join-Path $reportDir "comparison-contact-sheet.png"
$summaryPath = Join-Path $reportDir "summary.json"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $evidenceDir -Force | Out-Null
New-Item -ItemType Directory -Path $fullPagesDir -Force | Out-Null
New-Item -ItemType Directory -Path $subsetPagesDir -Force | Out-Null

$fontPath = Find-CjkFont
if ([string]::IsNullOrWhiteSpace($fontPath)) {
    Write-Step "Skipping visual regression: no CJK font found. Set FEATHERDOC_TEST_CJK_FONT to run it."
    exit 77
}

$previousFontEnv = $env:FEATHERDOC_TEST_CJK_FONT
$env:FEATHERDOC_TEST_CJK_FONT = $fontPath
try {
    Write-Step "Running pdf_unicode_font_roundtrip via ctest with $fontPath"
    Invoke-CommandCapture `
        -ExecutablePath "ctest" `
        -Arguments @("--test-dir", $resolvedBuildDir, "-R", "^pdf_unicode_font_roundtrip$", "--output-on-failure", "--timeout", "60") `
        -OutputPath $testLogPath `
        -FailureMessage "PDF unicode font roundtrip tests failed."
} finally {
    if ($null -eq $previousFontEnv) {
        Remove-Item Env:\FEATHERDOC_TEST_CJK_FONT -ErrorAction SilentlyContinue
    } else {
        $env:FEATHERDOC_TEST_CJK_FONT = $previousFontEnv
    }
}

$buildTestDir = Join-Path $resolvedBuildDir "test"
$fullPdfPath = Resolve-GeneratedPdfPath `
    -CandidatePaths @(
        (Join-Path $buildTestDir "featherdoc-cjk-font-full.pdf")
        (Join-Path $resolvedBuildDir "featherdoc-cjk-font-full.pdf")
        (Join-Path $resolvedOutputDir "featherdoc-cjk-font-full.pdf")
    ) `
    -Label "full PDF"
$subsetPdfPath = Resolve-GeneratedPdfPath `
    -CandidatePaths @(
        (Join-Path $buildTestDir "featherdoc-cjk-font-subset.pdf")
        (Join-Path $resolvedBuildDir "featherdoc-cjk-font-subset.pdf")
        (Join-Path $resolvedOutputDir "featherdoc-cjk-font-subset.pdf")
    ) `
    -Label "subset PDF"
$roundtripPdfPath = Resolve-GeneratedPdfPath `
    -CandidatePaths @(
        (Join-Path $buildTestDir "featherdoc-cjk-font-roundtrip.pdf")
        (Join-Path $resolvedBuildDir "featherdoc-cjk-font-roundtrip.pdf")
        (Join-Path $resolvedOutputDir "featherdoc-cjk-font-roundtrip.pdf")
    ) `
    -Label "roundtrip PDF"

foreach ($path in @($fullPdfPath, $subsetPdfPath, $roundtripPdfPath)) {
    if (-not (Test-Path $path)) {
        throw "Expected generated PDF not found: $path"
    }
}

$fullBytes = (Get-Item $fullPdfPath).Length
$subsetBytes = (Get-Item $subsetPdfPath).Length
if ($subsetBytes -ge $fullBytes) {
    throw "Subset PDF was not smaller than the full PDF."
}

$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

Write-Step "Rendering full PDF pages to PNG"
& $renderPython $renderScriptPath `
    --input $fullPdfPath `
    --output-dir $fullPagesDir `
    --summary $fullSummaryPath `
    --contact-sheet (Join-Path $reportDir "full-contact-sheet.png") `
    --dpi $Dpi
if ($LASTEXITCODE -ne 0) {
    throw "Failed to render full PDF pages."
}

Write-Step "Rendering subset PDF pages to PNG"
& $renderPython $renderScriptPath `
    --input $subsetPdfPath `
    --output-dir $subsetPagesDir `
    --summary $subsetSummaryPath `
    --contact-sheet (Join-Path $reportDir "subset-contact-sheet.png") `
    --dpi $Dpi
if ($LASTEXITCODE -ne 0) {
    throw "Failed to render subset PDF pages."
}

$fullPage1 = Join-Path $fullPagesDir "page-01.png"
$subsetPage1 = Join-Path $subsetPagesDir "page-01.png"
foreach ($path in @($fullPage1, $subsetPage1)) {
    if (-not (Test-Path $path)) {
        throw "Expected rendered page not found: $path"
    }
}

Write-Step "Building comparison contact sheet"
Build-ContactSheet `
    -Python $renderPython `
    -ScriptPath $contactSheetScriptPath `
    -OutputPath $comparisonContactSheetPath `
    -Images @($fullPage1, $subsetPage1) `
    -Labels @("full", "subset")

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    repo_root = $repoRoot
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    font_path = $fontPath
    test_executable = $testExecutable
    test_log = $testLogPath
    roundtrip_pdf = $roundtripPdfPath
    full_pdf = [ordered]@{
        path = $fullPdfPath
        bytes = $fullBytes
        render_summary = $fullSummaryPath
    }
    subset_pdf = [ordered]@{
        path = $subsetPdfPath
        bytes = $subsetBytes
        render_summary = $subsetSummaryPath
    }
    comparison_contact_sheet = $comparisonContactSheetPath
}

($summary | ConvertTo-Json -Depth 6) | Set-Content -Path $summaryPath -Encoding UTF8
Write-Step "Visual summary written to $summaryPath"
Write-Step "Comparison contact sheet: $comparisonContactSheetPath"
