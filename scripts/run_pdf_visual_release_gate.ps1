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
        [int]$ExpectedPageCount = 0
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
$textLayerScriptPath = Join-Path $repoRoot "scripts\check_pdf_text_layer.py"
$contactSheetScriptPath = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"
$pdfRegressionManifestPath = Join-Path $repoRoot "test\pdf_regression_manifest.json"
$unicodeScriptPath = Join-Path $repoRoot "scripts\run_pdf_unicode_font_roundtrip_visual_regression.ps1"
$pdfCliExportLog = Join-Path $reportDir "pdf-cli-export-test.log"
$pdfRegressionLog = Join-Path $reportDir "pdf-regression-test.log"
$cjkCopySearchDir = Join-Path $reportDir "cjk-copy-search"
$unicodeLog = Join-Path $reportDir "unicode-font.log"
$aggregateContactSheetPath = Join-Path $reportDir "aggregate-contact-sheet.png"
$summaryPath = Join-Path $reportDir "summary.json"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $baselineDir -Force | Out-Null
New-Item -ItemType Directory -Path $unicodeOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $cjkCopySearchDir -Force | Out-Null

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
    & $unicodeScriptPath -BuildDir $resolvedBuildDir -OutputDir $unicodeOutputDir -Dpi $Dpi
    if ($LASTEXITCODE -ne 0) {
        throw "Unicode font visual regression failed."
    }
}

$renderPython = Get-RenderPython -RepoRoot $repoRoot

$regressionManifest = Get-Content -Path $pdfRegressionManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
$cjkSamples = @($regressionManifest.samples | Where-Object { $_.expect_cjk -eq $true })
if ($cjkSamples.Count -eq 0) {
    throw "No CJK samples were found in $pdfRegressionManifestPath."
}

Write-Step "Checking CJK copy/search text layers"
$cjkCopySearchResults = New-Object System.Collections.Generic.List[object]
foreach ($sample in $cjkSamples) {
    $samplePdfPath = Join-Path $resolvedBuildDir "test\$($sample.output_file)"
    if (-not (Test-Path $samplePdfPath)) {
        throw "Expected PDF sample not found: $samplePdfPath"
    }

    $sampleSummaryPath = Join-Path $cjkCopySearchDir "$($sample.id)-summary.json"
    $sampleTextPath = Join-Path $cjkCopySearchDir "$($sample.id)-text.txt"
    $textLayerArguments = @(
        $textLayerScriptPath,
        "--input", $samplePdfPath,
        "--output-text", $sampleTextPath,
        "--summary", $sampleSummaryPath
    )
    foreach ($expectedText in @($sample.expected_text)) {
        $textLayerArguments += @("--expect-text", $expectedText)
    }

    & $renderPython @textLayerArguments
    if ($LASTEXITCODE -ne 0) {
        throw "CJK copy/search text layer check failed for '$($sample.id)'."
    }

    $sampleTextSummary = Get-Content -Path $sampleSummaryPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $cjkCopySearchResults.Add(
        [ordered]@{
            sample_id = $sample.id
            pdf_path = $samplePdfPath
            summary_path = $sampleSummaryPath
            text_output_path = $sampleTextPath
            expected_text = @($sample.expected_text)
            matched_text = @($sampleTextSummary.matched_text)
            missing_text = @($sampleTextSummary.missing_text)
            page_count = $sampleTextSummary.page_count
        }
    ) | Out-Null
}

$samples = @(
    [ordered]@{
        name = "document-contract-cjk-style"
        pdf = Join-Path $resolvedBuildDir "test\featherdoc-pdf-regression-document-contract-cjk-style.pdf"
        output = Join-Path $baselineDir "document-contract-cjk-style"
        expected_pages = 1
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
    $renderedSamples.Add(
        (Render-PdfSample `
            -Python $renderPython `
            -RenderScript $renderScriptPath `
            -SampleName $sample.name `
            -InputPdf $sample.pdf `
            -SampleOutputDir $sample.output `
            -ExpectedPageCount $sample.expected_pages)
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

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    repo_root = $repoRoot
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    unicode_output_dir = $unicodeOutputDir
    aggregate_contact_sheet = $aggregateContactSheetPath
    logs = [ordered]@{
        pdf_cli_export = $pdfCliExportLog
        pdf_regression = $pdfRegressionLog
        cjk_copy_search = $cjkCopySearchDir
        unicode_font = $unicodeLog
    }
    cjk_copy_search = $cjkCopySearchResults
    baselines = $renderedSamples
}

($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8
Write-Step "Visual gate summary written to $summaryPath"
