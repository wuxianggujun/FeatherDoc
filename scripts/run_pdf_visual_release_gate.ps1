param(
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$OutputDir = "output/pdf-visual-release-gate",
    [int]$Dpi = 144,
    [switch]$SkipUnicodeBaseline
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[pdf-visual-release-gate] $Message"
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

function Get-RenderPython {
    param([string]$RepoRoot)

    if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE) -and
        (Test-Path $env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE)) {
        $renderPython = (Resolve-Path $env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE).Path
        if ((Test-PythonImport -PythonCommand $renderPython -ModuleName "PIL") -and
            (Test-PythonImport -PythonCommand $renderPython -ModuleName "fitz")) {
            return $renderPython
        }
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

function Invoke-CapturedCommand {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments,
        [string]$LogPath,
        [string]$FailureMessage
    )

    $output = @(& $ExecutablePath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $lines = @($output | ForEach-Object { $_.ToString() })
    if (-not [string]::IsNullOrWhiteSpace($LogPath)) {
        $lines | Set-Content -Path $LogPath -Encoding UTF8
    }
    foreach ($line in $lines) {
        Write-Host $line
    }
    if ($exitCode -ne 0) {
        throw $FailureMessage
    }
}

function Render-PdfSample {
    param(
        [string]$Python,
        [string]$RenderScript,
        [string]$SampleName,
        [string]$InputPdf,
        [string]$SampleOutputDir,
        [int]$ExpectedPageCount = 0,
        [string[]]$StyleFocus = @()
    )

    New-Item -ItemType Directory -Path $SampleOutputDir -Force | Out-Null
    $pagesDir = Join-Path $SampleOutputDir "pages"
    New-Item -ItemType Directory -Path $pagesDir -Force | Out-Null

    $summaryPath = Join-Path $SampleOutputDir "summary.json"
    $contactSheetPath = Join-Path $SampleOutputDir "contact-sheet.png"

    Write-Step "Rendering $SampleName"
    & $Python $RenderScript `
        --input $InputPdf `
        --output-dir $pagesDir `
        --summary $summaryPath `
        --contact-sheet $contactSheetPath `
        --dpi $Dpi
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to render PDF sample '$SampleName'."
    }

    if (-not (Test-Path $summaryPath)) {
        throw "Missing render summary for '$SampleName'."
    }

    $summary = Get-Content -Path $summaryPath -Raw | ConvertFrom-Json
    if ($ExpectedPageCount -gt 0 -and $summary.page_count -ne $ExpectedPageCount) {
        throw "Unexpected page count for '$SampleName': $($summary.page_count) (expected $ExpectedPageCount)."
    }

    $page1 = Join-Path $pagesDir "page-01.png"
    if (-not (Test-Path $page1)) {
        throw "Missing first page image for '$SampleName'."
    }

    return [ordered]@{
        sample = $SampleName
        pdf_path = $InputPdf
        page_count = $summary.page_count
        expected_page_count = if ($ExpectedPageCount -gt 0) { $ExpectedPageCount } else { $null }
        style_focus = @($StyleFocus)
        bytes = (Get-Item $InputPdf).Length
        pages_dir = $pagesDir
        first_page = $page1
        summary_path = $summaryPath
        contact_sheet_path = $contactSheetPath
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$reportDir = Join-Path $resolvedOutputDir "report"
$baselineDir = Join-Path $resolvedOutputDir "baseline"
$unicodeOutputDir = Join-Path $resolvedOutputDir "unicode-font"
$renderScriptPath = Join-Path $repoRoot "scripts\render_pdf_pages.py"
$contactSheetScriptPath = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"
$unicodeScriptPath = Join-Path $repoRoot "scripts\run_pdf_unicode_font_roundtrip_visual_regression.ps1"
$pdfCliExportLog = Join-Path $reportDir "pdf-cli-export-test.log"
$pdfRegressionLog = Join-Path $reportDir "pdf-regression-test.log"
$unicodeRoundtripLog = Join-Path $reportDir "pdf-unicode-roundtrip-test.log"
$unicodeLog = Join-Path $reportDir "unicode-font.log"
$aggregateContactSheetPath = Join-Path $reportDir "aggregate-contact-sheet.png"
$summaryPath = Join-Path $reportDir "summary.json"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $baselineDir -Force | Out-Null
New-Item -ItemType Directory -Path $unicodeOutputDir -Force | Out-Null

Write-Step "Running pdf_cli_export regression"
Invoke-CapturedCommand `
    -ExecutablePath "ctest" `
    -Arguments @("--test-dir", $resolvedBuildDir, "-R", "^pdf_cli_export$", "--output-on-failure", "--timeout", "60") `
    -LogPath $pdfCliExportLog `
    -FailureMessage "pdf_cli_export regression failed."

Write-Step "Running pdf regression suite"
Invoke-CapturedCommand `
    -ExecutablePath "ctest" `
    -Arguments @("--test-dir", $resolvedBuildDir, "-R", "pdf_regression_", "--output-on-failure", "--timeout", "60") `
    -LogPath $pdfRegressionLog `
    -FailureMessage "pdf_regression_ suite failed."

if (-not $SkipUnicodeBaseline) {
    Write-Step "Running unicode font visual regression"
    Invoke-CapturedCommand `
        -ExecutablePath "powershell" `
        -Arguments @("-NoLogo", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-File", $unicodeScriptPath, "-BuildDir", $resolvedBuildDir, "-OutputDir", $unicodeOutputDir, "-Dpi", [string]$Dpi) `
        -LogPath $unicodeLog `
        -FailureMessage "Unicode font visual regression failed."
} else {
    Write-Step "Running unicode font roundtrip regression for RTL baseline PDFs"
    Invoke-CapturedCommand `
        -ExecutablePath "ctest" `
        -Arguments @("--test-dir", $resolvedBuildDir, "-R", "^pdf_unicode_font_roundtrip$", "--output-on-failure", "--timeout", "60") `
        -LogPath $unicodeRoundtripLog `
        -FailureMessage "pdf_unicode_font_roundtrip regression failed."
}

$renderPython = Get-RenderPython -RepoRoot $repoRoot

$samples = @(
    [ordered]@{
        name = "styled-text"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-styled-text.pdf"
        output = Join-Path $baselineDir "styled-text"
        expected_pages = 1
        style_focus = @("font-size", "color", "bold-italic", "underline")
    },
    [ordered]@{
        name = "mixed-style-text"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-mixed-style-text.pdf"
        output = Join-Path $baselineDir "mixed-style-text"
        expected_pages = 1
        style_focus = @("plain", "bold", "italic", "bold-italic-underline")
    },
    [ordered]@{
        name = "underline-text"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-underline-text.pdf"
        output = Join-Path $baselineDir "underline-text"
        expected_pages = 1
        style_focus = @("underline", "bold-underline", "italic-underline")
    },
    [ordered]@{
        name = "document-contract-cjk-style"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-document-contract-cjk-style.pdf"
        output = Join-Path $baselineDir "document-contract-cjk-style"
        expected_pages = 1
        style_focus = @("document-style-inheritance", "cjk-font-mapping", "bold", "color", "underline-signature")
    },
    [ordered]@{
        name = "document-eastasia-style-probe"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-document-eastasia-style-probe.pdf"
        output = Join-Path $baselineDir "document-eastasia-style-probe"
        expected_pages = 1
        style_focus = @("east-asia-font-mapping", "cjk-color", "bold-italic", "underline")
    },
    [ordered]@{
        name = "mixed-cjk-punctuation-text"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-mixed-cjk-punctuation-text.pdf"
        output = Join-Path $baselineDir "mixed-cjk-punctuation-text"
        expected_pages = 1
    },
    [ordered]@{
        name = "latin-ligature-text"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-latin-ligature-text.pdf"
        output = Join-Path $baselineDir "latin-ligature-text"
        expected_pages = 1
    },
    [ordered]@{
        name = "rtl-table-cell-hebrew-mixed-boundaries"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-hebrew-rtl-table-cell-boundary.pdf"
        output = Join-Path $baselineDir "rtl-table-cell-hebrew-mixed-boundaries"
        expected_pages = 1
        style_focus = @("rtl-table-cell-normalization", "hebrew", "arabic-indic-digits", "punctuation")
    },
    [ordered]@{
        name = "rtl-table-cell-persian-digit-boundaries"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-hebrew-rtl-table-cell-persian-digit-boundary.pdf"
        output = Join-Path $baselineDir "rtl-table-cell-persian-digit-boundaries"
        expected_pages = 1
        style_focus = @("rtl-table-cell-normalization", "hebrew", "extended-arabic-indic-digits", "punctuation")
    },
    [ordered]@{
        name = "rtl-table-cell-arabic-core-boundaries"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-arabic-rtl-table-cell-boundary.pdf"
        output = Join-Path $baselineDir "rtl-table-cell-arabic-core-boundaries"
        expected_pages = 1
        style_focus = @("rtl-table-cell-normalization", "arabic", "arabic-indic-digits", "extended-arabic-indic-digits", "punctuation")
    },
    [ordered]@{
        name = "rtl-table-cell-split-mixed-bidi-raw-spans"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-hebrew-rtl-table-cell-split-mixed-bidi-boundary.pdf"
        output = Join-Path $baselineDir "rtl-table-cell-split-mixed-bidi-raw-spans"
        expected_pages = 1
        style_focus = @("rtl-table-cell-raw-preservation", "split-mixed-bidi", "raw-spans")
    },
    [ordered]@{
        name = "rtl-table-cell-split-pure-rtl-raw-spans"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-hebrew-rtl-table-cell-split-pure-rtl-boundary.pdf"
        output = Join-Path $baselineDir "rtl-table-cell-split-pure-rtl-raw-spans"
        expected_pages = 1
        style_focus = @("rtl-table-cell-raw-preservation", "split-pure-rtl", "raw-spans")
    },
    [ordered]@{
        name = "rtl-table-cell-arabic-split-pure-rtl-normalized"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-arabic-rtl-table-cell-split-pure-rtl-boundary.pdf"
        output = Join-Path $baselineDir "rtl-table-cell-arabic-split-pure-rtl-normalized"
        expected_pages = 1
        style_focus = @("rtl-table-cell-normalization", "arabic", "split-pure-rtl")
    },
    [ordered]@{
        name = "rtl-table-cell-hebrew-combining-mark-raw-spans"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-hebrew-rtl-table-cell-combining-mark-boundary.pdf"
        output = Join-Path $baselineDir "rtl-table-cell-hebrew-combining-mark-raw-spans"
        expected_pages = 1
        style_focus = @("rtl-table-cell-raw-preservation", "hebrew", "combining-mark", "raw-spans")
    },
    [ordered]@{
        name = "rtl-table-cell-arabic-combining-mark-raw-spans"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-arabic-rtl-table-cell-combining-mark-boundary.pdf"
        output = Join-Path $baselineDir "rtl-table-cell-arabic-combining-mark-raw-spans"
        expected_pages = 1
        style_focus = @("rtl-table-cell-raw-preservation", "arabic", "combining-mark", "raw-spans")
    },
    [ordered]@{
        name = "document-invoice-table-text"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-document-invoice-table-text.pdf"
        output = Join-Path $baselineDir "document-invoice-table-text"
        expected_pages = 1
    },
    [ordered]@{
        name = "document-long-flow-text"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-document-long-flow-text.pdf"
        output = Join-Path $baselineDir "document-long-flow-text"
        expected_pages = 5
    },
    [ordered]@{
        name = "document-image-semantics-text"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-document-image-semantics-text.pdf"
        output = Join-Path $baselineDir "document-image-semantics-text"
        expected_pages = 0
    },
    [ordered]@{
        name = "cli-font-map-source"
        pdf = Join-Path $resolvedBuildDir "test\pdf_cli_export\font-map-source.pdf"
        output = Join-Path $baselineDir "cli-font-map-source"
        expected_pages = 3
    }
)

$renderedSamples = New-Object System.Collections.Generic.List[object]
foreach ($sample in $samples) {
    if (-not (Test-Path $sample.pdf)) {
        throw "Expected PDF sample not found: $($sample.pdf)"
    }
    $styleFocus = @()
    if ($sample.Contains("style_focus")) {
        $styleFocus = @($sample.style_focus)
    }
    $renderedSamples.Add(
        (Render-PdfSample `
            -Python $renderPython `
            -RenderScript $renderScriptPath `
            -SampleName $sample.name `
            -InputPdf $sample.pdf `
            -SampleOutputDir $sample.output `
            -ExpectedPageCount $sample.expected_pages `
            -StyleFocus $styleFocus)
    ) | Out-Null
}

$aggregatePagePaths = @()
$aggregateLabels = @()
foreach ($entry in $renderedSamples) {
    $aggregatePagePaths += $entry.first_page
    $aggregateLabels += $entry.sample
}

Write-Step "Building aggregate contact sheet"
& $renderPython $contactSheetScriptPath `
    --output $aggregateContactSheetPath `
    --columns 2 `
    --thumb-width 420 `
    --images $aggregatePagePaths `
    --labels $aggregateLabels
if ($LASTEXITCODE -ne 0) {
    throw "Failed to build aggregate contact sheet."
}

$logPaths = [ordered]@{
    pdf_cli_export = $pdfCliExportLog
    pdf_regression = $pdfRegressionLog
}
if ($SkipUnicodeBaseline) {
    $logPaths["pdf_unicode_roundtrip"] = $unicodeRoundtripLog
} else {
    $logPaths["unicode_font"] = $unicodeLog
}

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    repo_root = $repoRoot
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    unicode_output_dir = $unicodeOutputDir
    aggregate_contact_sheet = $aggregateContactSheetPath
    logs = $logPaths
    baselines = $renderedSamples
}

($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8
Write-Step "Visual gate summary written to $summaryPath"
