param(
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$OutputDir = "output/pdf-visual-release-gate",
    [int]$Dpi = 144,
    [switch]$SkipUnicodeBaseline,
    [int]$MinFreeMemoryMB = 2048,
    [string]$PreflightJson = "",
    [switch]$PreflightOnly,
    [switch]$SkipPreflight,
    [switch]$SkipMemoryGuard
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

    $candidatePythons = New-Object 'System.Collections.Generic.List[object]'
    if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE) -and
        (Test-Path $env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE)) {
        $candidatePythons.Add([ordered]@{
                Source = "FEATHERDOC_RENDER_PYTHON_EXECUTABLE"
                Path = (Resolve-Path $env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE).Path
            }) | Out-Null
    }

    $basePython = Get-BasePython
    $candidatePythons.Add([ordered]@{
            Source = "base Python"
            Path = $basePython
        }) | Out-Null

    foreach ($relativePath in @(
            ".venv-word-visual-smoke\Scripts\python.exe",
            "tmp\render-venv\Scripts\python.exe",
            ".venv-pdf-visual-smoke\Scripts\python.exe"
        )) {
        $candidatePath = Join-Path $RepoRoot $relativePath
        if (Test-Path $candidatePath) {
            $candidatePythons.Add([ordered]@{
                    Source = $relativePath
                    Path = (Resolve-Path $candidatePath).Path
                }) | Out-Null
        }
    }

    foreach ($candidate in @($candidatePythons.ToArray())) {
        $pythonPath = [string]$candidate.Path
        if ((Test-PythonImport -PythonCommand $pythonPath -ModuleName "PIL") -and
            (Test-PythonImport -PythonCommand $pythonPath -ModuleName "fitz")) {
            Write-Step "Using reusable render Python from $($candidate.Source): $pythonPath"
            return $pythonPath
        }
    }

    throw "No reusable render Python with PIL and fitz was found. Run the PDF preflight, set FEATHERDOC_RENDER_PYTHON_EXECUTABLE, or prepare an existing render environment before running the full PDF visual release gate."
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

if (-not $SkipPreflight) {
    $preflightScriptPath = Join-Path $repoRoot "scripts\check_pdf_visual_release_gate_preflight.ps1"
    if (-not (Test-Path $preflightScriptPath)) {
        throw "PDF visual release gate preflight script was not found: $preflightScriptPath"
    }

    $resolvedPreflightJson = if ([string]::IsNullOrWhiteSpace($PreflightJson)) {
        Join-Path $reportDir "preflight-summary.json"
    } else {
        Resolve-RepoPath -RepoRoot $repoRoot -InputPath $PreflightJson
    }

    Write-Step "Running preflight"
    & $preflightScriptPath `
        -BuildDir $resolvedBuildDir `
        -OutputJson $resolvedPreflightJson `
        -MinFreeMemoryMB $MinFreeMemoryMB `
        -SkipMemoryGuard:$SkipMemoryGuard `
        -Strict
    if ($LASTEXITCODE -ne 0) {
        throw "PDF visual release gate preflight failed. See $resolvedPreflightJson."
    }

    if ($PreflightOnly) {
        Write-Step "Preflight passed. Skipping full visual release gate because -PreflightOnly was set."
        return
    }
} elseif ($PreflightOnly) {
    throw "-PreflightOnly cannot be used with -SkipPreflight."
}

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
    Invoke-CapturedCommand `
        -ExecutablePath $unicodeScriptPath `
        -Arguments @("-BuildDir", $resolvedBuildDir, "-OutputDir", $unicodeOutputDir, "-Dpi", [string]$Dpi) `
        -LogPath $unicodeLog `
        -FailureMessage "Unicode font visual regression failed."
}

$renderPython = Get-RenderPython -RepoRoot $repoRoot

$regressionManifest = Get-Content -Path $pdfRegressionManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
$cjkSamples = @($regressionManifest.samples | Where-Object { $_.expect_cjk -eq $true })
if ($cjkSamples.Count -eq 0) {
    throw "No CJK samples were found in $pdfRegressionManifestPath."
}
$visualManifestSamples = @($regressionManifest.samples | Where-Object { $_.expect_visual_baseline -eq $true })
if ($visualManifestSamples.Count -eq 0) {
    throw "No visual baseline samples were found in $pdfRegressionManifestPath."
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
    $visualManifestSamples | ForEach-Object {
        [ordered]@{
            name = $_.id
            pdf = Join-Path $resolvedBuildDir "test\$($_.output_file)"
            output = Join-Path $baselineDir $_.id
            expected_pages = if ($null -ne $_.visual_expected_pages) {
                [int]$_.visual_expected_pages
            } else {
                [int]$_.expected_pages
            }
            style_focus = @($_.visual_style_focus)
        }
    }
    [ordered]@{
        name = "cli-font-map-source"
        pdf = Join-Path $resolvedBuildDir "test\pdf_cli_export\font-map-source.pdf"
        output = Join-Path $baselineDir "cli-font-map-source"
        expected_pages = 3
    }
    [ordered]@{
        name = "cli-cjk-font-source"
        pdf = Join-Path $resolvedBuildDir "test\pdf_cli_export\cjk-font-source.pdf"
        output = Join-Path $baselineDir "cli-cjk-font-source"
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
