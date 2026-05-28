param(
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$OutputDir = "output/pdf-visual-release-gate",
    [int]$Dpi = 144,
    [switch]$SkipUnicodeBaseline,
    [int]$MinFreeMemoryMB = 2048,
    [string]$PdfioSourceDir = "",
    [ValidateSet("", "auto", "source", "prebuilt", "package")]
    [string]$PdfiumProvider = "",
    [string]$PdfiumSourceDir = "",
    [string]$PdfiumPrebuiltRoot = "",
    [string]$PdfiumLibrary = "",
    [string]$PdfiumIncludeDir = "",
    [string]$PdfiumRuntimeDll = "",
    [string]$PdfiumRuntimeDir = "",
    [string]$PreflightJson = "",
    [switch]$PreflightOnly,
    [switch]$FinalizeOnly,
    [switch]$SkipPreflight,
    [switch]$SkipMemoryGuard,
    [ValidateRange(0, 2147483647)]
    [int]$VisualBaselineOffset = 0,
    [ValidateRange(0, 2147483647)]
    [int]$VisualBaselineLimit = 0,
    [switch]$VisualBaselineSliceOnly,
    [switch]$RebuildAggregateContactSheetOnly,
    [string]$CtestExecutable = "ctest"
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

function Resolve-CtestExecutable {
    param([string]$Executable)

    if (-not [string]::IsNullOrWhiteSpace($Executable)) {
        if ([System.IO.Path]::IsPathRooted($Executable)) {
            if (Test-Path -LiteralPath $Executable) {
                return (Resolve-Path -LiteralPath $Executable).Path
            }
            throw "CTest executable was not found: $Executable"
        }

        $resolved = Get-Command $Executable -ErrorAction SilentlyContinue
        if ($resolved) {
            return $resolved.Source
        }
    }

    throw "CTest executable was not found. Pass -CtestExecutable or ensure ctest is in PATH."
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

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = @(& $ExecutablePath @Arguments 2>&1)
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
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
    $renderArguments = @(
        $RenderScript,
        "--input",
        $InputPdf,
        "--output-dir",
        $pagesDir,
        "--summary",
        $summaryPath,
        "--contact-sheet",
        $contactSheetPath,
        "--dpi",
        [string]$Dpi
    )
    if (-not $VisualBaselineSliceOnly) {
        $renderArguments += "--skip-contact-sheet"
    }

    & $Python @renderArguments
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

function Get-ExpectedTextLayerText {
    param([object]$Sample)

    if ($null -ne $Sample.PSObject.Properties["text_layer_expected_text"]) {
        return @($Sample.text_layer_expected_text)
    }

    return @($Sample.expected_text)
}

function Read-ExistingCjkCopySearchResult {
    param(
        [object]$Sample,
        [string]$BuildDir,
        [string]$CjkCopySearchDir
    )

    $samplePdfPath = Join-Path $BuildDir "test\$($Sample.output_file)"
    $sampleSummaryPath = Join-Path $CjkCopySearchDir "$($Sample.id)-summary.json"
    $sampleTextPath = Join-Path $CjkCopySearchDir "$($Sample.id)-text.txt"

    foreach ($requiredPath in @($samplePdfPath, $sampleSummaryPath, $sampleTextPath)) {
        if (-not (Test-Path $requiredPath)) {
            throw "Missing existing CJK copy/search evidence for '$($Sample.id)': $requiredPath"
        }
    }

    $sampleTextSummary = Get-Content -Path $sampleSummaryPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $missingText = @($sampleTextSummary.missing_text)
    if ($missingText.Count -gt 0) {
        throw "Existing CJK copy/search evidence still has missing text for '$($Sample.id)'."
    }

    return [ordered]@{
        sample_id = $Sample.id
        pdf_path = $samplePdfPath
        summary_path = $sampleSummaryPath
        text_output_path = $sampleTextPath
        expected_text = @(Get-ExpectedTextLayerText -Sample $Sample)
        matched_text = @($sampleTextSummary.matched_text)
        missing_text = $missingText
        page_count = $sampleTextSummary.page_count
    }
}

function Read-ExistingRenderedSample {
    param(
        [object]$Sample
    )

    $summaryPath = Join-Path $Sample.output "summary.json"
    $contactSheetPath = Join-Path $Sample.output "contact-sheet.png"
    $pagesDir = Join-Path $Sample.output "pages"
    $page1 = Join-Path $pagesDir "page-01.png"

    foreach ($requiredPath in @($Sample.pdf, $summaryPath, $page1)) {
        if (-not (Test-Path $requiredPath)) {
            throw "Missing existing rendered visual evidence for '$($Sample.name)': $requiredPath"
        }
    }

    $summary = Get-Content -Path $summaryPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($Sample.expected_pages -gt 0 -and $summary.page_count -ne $Sample.expected_pages) {
        throw "Unexpected existing page count for '$($Sample.name)': $($summary.page_count) (expected $($Sample.expected_pages))."
    }

    $styleFocus = @()
    if ($Sample.Contains("style_focus")) {
        $styleFocus = @($Sample.style_focus)
    }

    return [ordered]@{
        sample = $Sample.name
        pdf_path = $Sample.pdf
        page_count = $summary.page_count
        expected_page_count = if ($Sample.expected_pages -gt 0) { $Sample.expected_pages } else { $null }
        style_focus = $styleFocus
        bytes = (Get-Item $Sample.pdf).Length
        pages_dir = $pagesDir
        first_page = $page1
        summary_path = $summaryPath
        contact_sheet_path = if (Test-Path $contactSheetPath) { $contactSheetPath } else { "" }
    }
}

function Select-VisualBaselineSlice {
    param(
        [object[]]$Samples,
        [int]$Offset,
        [int]$Limit
    )

    if ($Offset -ge $Samples.Count) {
        throw "Visual baseline slice offset $Offset is outside the available sample count $($Samples.Count)."
    }

    $selectedCount = [Math]::Min($Limit, $Samples.Count - $Offset)
    return @($Samples | Select-Object -Skip $Offset -First $selectedCount)
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$resolvedCtestExecutable = ""
if (-not $FinalizeOnly -and -not $VisualBaselineSliceOnly -and -not $RebuildAggregateContactSheetOnly -and -not $PreflightOnly) {
    $resolvedCtestExecutable = Resolve-CtestExecutable -Executable $CtestExecutable
}
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

if ($FinalizeOnly -and $PreflightOnly) {
    throw "-FinalizeOnly cannot be used with -PreflightOnly."
}

if ($VisualBaselineSliceOnly -and $FinalizeOnly) {
    throw "-VisualBaselineSliceOnly cannot be used with -FinalizeOnly."
}

if ($RebuildAggregateContactSheetOnly -and $FinalizeOnly) {
    throw "-RebuildAggregateContactSheetOnly cannot be used with -FinalizeOnly."
}

if ($RebuildAggregateContactSheetOnly -and $VisualBaselineSliceOnly) {
    throw "-RebuildAggregateContactSheetOnly cannot be used with -VisualBaselineSliceOnly."
}

if ($VisualBaselineSliceOnly -and $PreflightOnly) {
    throw "-VisualBaselineSliceOnly cannot be used with -PreflightOnly."
}

if ($RebuildAggregateContactSheetOnly -and $PreflightOnly) {
    throw "-RebuildAggregateContactSheetOnly cannot be used with -PreflightOnly."
}

if ($VisualBaselineSliceOnly -and $VisualBaselineLimit -le 0) {
    throw "-VisualBaselineSliceOnly requires -VisualBaselineLimit greater than 0."
}

if (-not $VisualBaselineSliceOnly -and ($VisualBaselineOffset -ne 0 -or $VisualBaselineLimit -ne 0)) {
    throw "-VisualBaselineOffset and -VisualBaselineLimit require -VisualBaselineSliceOnly."
}

if (-not $SkipPreflight -and -not $FinalizeOnly) {
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
    $preflightArguments = @{
        BuildDir = $resolvedBuildDir
        OutputJson = $resolvedPreflightJson
        MinFreeMemoryMB = $MinFreeMemoryMB
        SkipMemoryGuard = [bool]$SkipMemoryGuard
        Strict = $true
    }
    foreach ($entry in @(
        @{ Name = "PdfioSourceDir"; Value = $PdfioSourceDir },
        @{ Name = "PdfiumProvider"; Value = $PdfiumProvider },
        @{ Name = "PdfiumSourceDir"; Value = $PdfiumSourceDir },
        @{ Name = "PdfiumPrebuiltRoot"; Value = $PdfiumPrebuiltRoot },
        @{ Name = "PdfiumLibrary"; Value = $PdfiumLibrary },
        @{ Name = "PdfiumIncludeDir"; Value = $PdfiumIncludeDir },
        @{ Name = "PdfiumRuntimeDll"; Value = $PdfiumRuntimeDll },
        @{ Name = "PdfiumRuntimeDir"; Value = $PdfiumRuntimeDir }
    )) {
        if (-not [string]::IsNullOrWhiteSpace([string]$entry.Value)) {
            $preflightArguments[$entry.Name] = [string]$entry.Value
        }
    }

    & $preflightScriptPath @preflightArguments
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

$regressionManifest = Get-Content -Path $pdfRegressionManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
$cjkSamples = @($regressionManifest.samples | Where-Object { $_.expect_cjk -eq $true })
if ($cjkSamples.Count -eq 0) {
    throw "No CJK samples were found in $pdfRegressionManifestPath."
}
$visualManifestSamples = @($regressionManifest.samples | Where-Object { $_.expect_visual_baseline -eq $true })
if ($visualManifestSamples.Count -eq 0) {
    throw "No visual baseline samples were found in $pdfRegressionManifestPath."
}

$cjkCopySearchResults = New-Object System.Collections.Generic.List[object]
$renderPython = $null

if ($VisualBaselineSliceOnly) {
    Write-Step "Rendering visual baseline slice only; skipping CTest, unicode font, CJK copy/search, and full-gate summary finalization"
    $renderPython = Get-RenderPython -RepoRoot $repoRoot
} elseif ($RebuildAggregateContactSheetOnly) {
    Write-Step "Rebuilding aggregate contact sheet only; reusing existing rendered visual baselines"
    $renderPython = Get-RenderPython -RepoRoot $repoRoot
} elseif ($FinalizeOnly) {
    Write-Step "Finalizing existing PDF visual release gate output"
    foreach ($requiredPath in @($pdfCliExportLog, $pdfRegressionLog, $aggregateContactSheetPath)) {
        if (-not (Test-Path $requiredPath)) {
            throw "Missing existing PDF visual release gate evidence: $requiredPath"
        }
    }
    if (-not $SkipUnicodeBaseline -and -not (Test-Path $unicodeLog)) {
        throw "Missing existing unicode font visual regression log: $unicodeLog"
    }
    foreach ($sample in $cjkSamples) {
        $cjkCopySearchResults.Add(
            (Read-ExistingCjkCopySearchResult `
                -Sample $sample `
                -BuildDir $resolvedBuildDir `
                -CjkCopySearchDir $cjkCopySearchDir)
        ) | Out-Null
    }
} else {
    Write-Step "Running pdf_cli_export regression"
    Invoke-CapturedCommand `
        -ExecutablePath $resolvedCtestExecutable `
        -Arguments @("--test-dir", $resolvedBuildDir, "-R", "^pdf_cli_export$", "--output-on-failure", "--timeout", "60") `
        -LogPath $pdfCliExportLog `
        -FailureMessage "pdf_cli_export regression failed."

    Write-Step "Running pdf regression suite"
    Invoke-CapturedCommand `
        -ExecutablePath $resolvedCtestExecutable `
        -Arguments @("--test-dir", $resolvedBuildDir, "-R", "pdf_regression_", "--output-on-failure", "--timeout", "60") `
        -LogPath $pdfRegressionLog `
        -FailureMessage "pdf_regression_ suite failed."

    if (-not $SkipUnicodeBaseline) {
        Write-Step "Running unicode font visual regression"
        Invoke-CapturedCommand `
            -ExecutablePath "powershell" `
            -Arguments @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $unicodeScriptPath, "-BuildDir", $resolvedBuildDir, "-OutputDir", $unicodeOutputDir, "-Dpi", [string]$Dpi, "-CtestExecutable", $resolvedCtestExecutable) `
            -LogPath $unicodeLog `
            -FailureMessage "Unicode font visual regression failed."
    }

    $renderPython = Get-RenderPython -RepoRoot $repoRoot

    Write-Step "Checking CJK copy/search text layers"
    foreach ($sample in $cjkSamples) {
        $samplePdfPath = Join-Path $resolvedBuildDir "test\$($sample.output_file)"
        if (-not (Test-Path $samplePdfPath)) {
            throw "Expected PDF sample not found: $samplePdfPath"
        }

        $sampleSummaryPath = Join-Path $cjkCopySearchDir "$($sample.id)-summary.json"
        $sampleTextPath = Join-Path $cjkCopySearchDir "$($sample.id)-text.txt"
        $expectedTextLayerText = @(Get-ExpectedTextLayerText -Sample $sample)
        $textLayerArguments = @(
            $textLayerScriptPath,
            "--input", $samplePdfPath,
            "--output-text", $sampleTextPath,
            "--summary", $sampleSummaryPath
        )
        foreach ($expectedText in $expectedTextLayerText) {
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
                expected_text = $expectedTextLayerText
                matched_text = @($sampleTextSummary.matched_text)
                missing_text = @($sampleTextSummary.missing_text)
                page_count = $sampleTextSummary.page_count
            }
        ) | Out-Null
    }
}

$allVisualSamples = @(
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

$samples = if ($VisualBaselineSliceOnly) {
    Select-VisualBaselineSlice -Samples $allVisualSamples -Offset $VisualBaselineOffset -Limit $VisualBaselineLimit
} else {
    $allVisualSamples
}

$renderedSamples = New-Object System.Collections.Generic.List[object]
foreach ($sample in $samples) {
    if (-not (Test-Path $sample.pdf)) {
        throw "Expected PDF sample not found: $($sample.pdf)"
    }
    $styleFocus = @()
    if ($sample.Contains("style_focus")) {
        $styleFocus = @($sample.style_focus)
    }
    if ($FinalizeOnly) {
        $renderedSamples.Add((Read-ExistingRenderedSample -Sample $sample)) | Out-Null
    } elseif ($RebuildAggregateContactSheetOnly) {
        $renderedSamples.Add((Read-ExistingRenderedSample -Sample $sample)) | Out-Null
    } else {
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
}

if ($VisualBaselineSliceOnly) {
    $sliceId = "visual-baseline-slice-offset-$VisualBaselineOffset-limit-$VisualBaselineLimit"
    $sliceContactSheetPath = Join-Path $reportDir "$sliceId-contact-sheet.png"
    $sliceSummaryPath = Join-Path $reportDir "$sliceId-summary.json"
    $slicePagePaths = @()
    $sliceLabels = @()
    foreach ($entry in $renderedSamples) {
        $slicePagePaths += $entry.first_page
        $sliceLabels += $entry.sample
    }

    Write-Step "Building visual baseline slice contact sheet"
    & $renderPython $contactSheetScriptPath `
        --output $sliceContactSheetPath `
        --columns 2 `
        --thumb-width 420 `
        --images $slicePagePaths `
        --labels $sliceLabels
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build visual baseline slice contact sheet."
    }

    $sliceSummary = [ordered]@{
        schema = "featherdoc.pdf_visual_baseline_slice.v1"
        generated_at = (Get-Date).ToString("s")
        status = "pass"
        verdict = "pass"
        full_visual_gate_status = "not_complete"
        evidence_scope = "visual_baseline_slice_only"
        boundary = "slice_summary_does_not_replace_full_visual_gate_verdict"
        repo_root = $repoRoot
        build_dir = $resolvedBuildDir
        output_dir = $resolvedOutputDir
        report_dir = $reportDir
        visual_baseline_offset = $VisualBaselineOffset
        visual_baseline_limit = $VisualBaselineLimit
        selected_baseline_count = $renderedSamples.Count
        total_baseline_count = $allVisualSamples.Count
        visual_baseline_manifest_count = $visualManifestSamples.Count
        expected_visual_render_count = $allVisualSamples.Count
        slice_contact_sheet = $sliceContactSheetPath
        baselines = $renderedSamples
    }

    ($sliceSummary | ConvertTo-Json -Depth 8) | Set-Content -Path $sliceSummaryPath -Encoding UTF8
    Write-Step "Visual baseline slice summary written to $sliceSummaryPath"
    exit 0
}

$aggregatePagePaths = @()
$aggregateLabels = @()
foreach ($entry in $renderedSamples) {
    $aggregatePagePaths += $entry.first_page
    $aggregateLabels += $entry.sample
}

if ($RebuildAggregateContactSheetOnly) {
    $rebuildSummaryPath = Join-Path $reportDir "aggregate-contact-sheet-rebuild-summary.json"

    Write-Step "Rebuilding aggregate contact sheet from existing rendered visual baselines"
    & $renderPython $contactSheetScriptPath `
        --output $aggregateContactSheetPath `
        --columns 2 `
        --thumb-width 420 `
        --images $aggregatePagePaths `
        --labels $aggregateLabels
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to rebuild aggregate contact sheet."
    }

    $rebuildSummary = [ordered]@{
        schema = "featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1"
        generated_at = (Get-Date).ToString("s")
        status = "pass"
        verdict = "pass"
        full_visual_gate_status = "not_complete"
        evidence_scope = "aggregate_contact_sheet_rebuild_only"
        boundary = "aggregate_rebuild_summary_does_not_replace_full_visual_gate_verdict"
        repo_root = $repoRoot
        build_dir = $resolvedBuildDir
        output_dir = $resolvedOutputDir
        report_dir = $reportDir
        aggregate_contact_sheet = $aggregateContactSheetPath
        selected_baseline_count = $renderedSamples.Count
        total_baseline_count = $allVisualSamples.Count
        visual_baseline_manifest_count = $visualManifestSamples.Count
        expected_visual_render_count = $allVisualSamples.Count
        baselines = $renderedSamples
    }

    ($rebuildSummary | ConvertTo-Json -Depth 8) | Set-Content -Path $rebuildSummaryPath -Encoding UTF8
    Write-Step "Aggregate contact sheet rebuild summary written to $rebuildSummaryPath"
    exit 0
}

if ($FinalizeOnly) {
    Write-Step "Reusing existing aggregate contact sheet"
    if (-not (Test-Path $aggregateContactSheetPath)) {
        throw "Missing aggregate contact sheet: $aggregateContactSheetPath"
    }
} else {
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
}

$summaryCore = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    status = "pass"
    verdict = "pass"
    finalize_only = [bool]$FinalizeOnly
    skip_preflight = [bool]$SkipPreflight
    skip_unicode_baseline = [bool]$SkipUnicodeBaseline
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
    cjk_manifest_count = $cjkSamples.Count
    cjk_copy_search_count = $cjkCopySearchResults.Count
    visual_baseline_manifest_count = $visualManifestSamples.Count
    baselines_count = $renderedSamples.Count
    summary_detail_payload_included = $false
    summary_detail_status = "core_pass_written_before_detail_payload"
    marker = "pdf_visual_gate_core_pass_summary_trace"
}

($summaryCore | ConvertTo-Json -Depth 6) | Set-Content -Path $summaryPath -Encoding UTF8
Write-Step "Visual gate core pass summary written to $summaryPath"

$summary = [ordered]@{}
foreach ($entry in $summaryCore.GetEnumerator()) {
    $summary[$entry.Key] = $entry.Value
}
$summary.generated_at = (Get-Date).ToString("s")
$summary.summary_detail_payload_included = $true
$summary.summary_detail_status = "complete"
$summary.cjk_copy_search = $cjkCopySearchResults
$summary.baselines = $renderedSamples

$summaryDetailPath = "$summaryPath.detail.tmp"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryDetailPath -Encoding UTF8
Move-Item -LiteralPath $summaryDetailPath -Destination $summaryPath -Force
Write-Step "Visual gate summary written to $summaryPath"
exit 0
