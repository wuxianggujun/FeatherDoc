param(
    [string]$BuildDir = "build-word-visual-smoke-nmake",
    [string]$OutputDir = "output/word-visual-smoke",
    [string]$InputDocx = "",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[word-smoke] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Get-VcvarsPath {
    $candidates = @(
        "D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    throw "Could not locate vcvars64.bat for MSVC command-line builds."
}

function Invoke-MsvcCommand {
    param(
        [string]$VcvarsPath,
        [string]$CommandText
    )

    & cmd.exe /c "call `"$VcvarsPath`" && $CommandText"
    if ($LASTEXITCODE -ne 0) {
        throw "MSVC command failed: $CommandText"
    }
}

function Get-BasePython {
    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($python) {
        return $python.Source
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

    $basePython = Get-BasePython
    if ((Test-PythonImport -PythonCommand $basePython -ModuleName "fitz") -and
        (Test-PythonImport -PythonCommand $basePython -ModuleName "PIL")) {
        return $basePython
    }

    $venvDir = Join-Path $RepoRoot ".venv-word-visual-smoke"
    $venvPython = Join-Path $venvDir "Scripts\python.exe"
    if (-not (Test-Path $venvPython)) {
        Write-Step "Creating local Python environment at $venvDir"
        $null = & $basePython -m venv $venvDir
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to create local Python virtual environment."
        }
    }

    if ((-not (Test-PythonImport -PythonCommand $venvPython -ModuleName "fitz")) -or
        (-not (Test-PythonImport -PythonCommand $venvPython -ModuleName "PIL"))) {
        Write-Step "Installing PyMuPDF and Pillow into the local environment"
        & $venvPython -m pip install --disable-pip-version-check pymupdf pillow 2>&1 |
            ForEach-Object { Write-Host $_ }
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to install rendering dependencies into the local environment."
        }
    }

    return $venvPython
}

function Find-SampleExecutable {
    param(
        [string]$BuildRoot,
        [string]$TargetName
    )

    $candidates = Get-ChildItem -Path $BuildRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName } |
        Sort-Object LastWriteTime -Descending

    if (-not $candidates) {
        throw "Could not find built executable for target '$TargetName' under $BuildRoot."
    }

    return $candidates[0].FullName
}

function Export-DocxToPdf {
    param(
        [string]$DocxPath,
        [string]$PdfPath,
        [bool]$Visible,
        [bool]$KeepOpen
    )

    $word = $null
    $document = $null
    try {
        $word = New-Object -ComObject Word.Application
        $word.Visible = $Visible
        $word.DisplayAlerts = 0
        $document = $word.Documents.Open($DocxPath, $false, $true)
        $document.ExportAsFixedFormat($PdfPath, 17)
        if (-not $KeepOpen) {
            $document.Close([ref]$false)
            $word.Quit()
        }
    } catch {
        if ($document -ne $null -and -not $KeepOpen) {
            try {
                $document.Close([ref]$false) | Out-Null
            } catch {
            }
        }
        if ($word -ne $null -and -not $KeepOpen) {
            try {
                $word.Quit() | Out-Null
            } catch {
            }
        }
        throw
    }
}

$repoRoot = Resolve-RepoRoot
$vcvarsPath = Get-VcvarsPath
$resolvedBuildDir = Join-Path $repoRoot $BuildDir
$resolvedOutputDir = Join-Path $repoRoot $OutputDir
$evidenceDir = Join-Path $resolvedOutputDir "evidence"
$pagesDir = Join-Path $evidenceDir "pages"
$reportDir = Join-Path $resolvedOutputDir "report"
$repairDir = Join-Path $resolvedOutputDir "repair"
$docxPath = if ($InputDocx) {
    (Resolve-Path $InputDocx).Path
} else {
    Join-Path $resolvedOutputDir "table_visual_smoke.docx"
}
$pdfPath = Join-Path $resolvedOutputDir "table_visual_smoke.pdf"
$contactSheetPath = Join-Path $evidenceDir "contact_sheet.png"
$summaryPath = Join-Path $reportDir "summary.json"
$checklistPath = Join-Path $reportDir "review_checklist.md"
$reviewResultPath = Join-Path $reportDir "review_result.json"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$renderScriptPath = Join-Path $repoRoot "scripts\render_pdf_pages.py"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $evidenceDir -Force | Out-Null
New-Item -ItemType Directory -Path $pagesDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $repairDir -Force | Out-Null

if (-not $InputDocx) {
    if (-not $SkipBuild) {
        Write-Step "Configuring dedicated build directory $resolvedBuildDir"
        Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=OFF -DBUILD_TESTING=OFF"

        Write-Step "Building featherdoc_visual_smoke_tables"
        Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_visual_smoke_tables"
    }

    $samplePath = Find-SampleExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_visual_smoke_tables"
    Write-Step "Generating smoke-check DOCX via $samplePath"
    & $samplePath $docxPath
    if ($LASTEXITCODE -ne 0) {
        throw "Smoke sample generation failed."
    }
} else {
    Write-Step "Using existing DOCX: $docxPath"
}

Write-Step "Exporting DOCX to PDF through Microsoft Word"
Export-DocxToPdf -DocxPath $docxPath -PdfPath $pdfPath -Visible $VisibleWord.IsPresent -KeepOpen $KeepWordOpen.IsPresent

$renderPython = Ensure-RenderPython -RepoRoot $repoRoot
Write-Step "Rendering PDF pages to PNG"
& $renderPython $renderScriptPath `
    --input $pdfPath `
    --output-dir $pagesDir `
    --summary $summaryPath `
    --contact-sheet $contactSheetPath `
    --dpi $Dpi
if ($LASTEXITCODE -ne 0) {
    throw "PDF render step failed."
}

$summary = Get-Content -Path $summaryPath -Raw | ConvertFrom-Json
$reviewResult = [ordered]@{
    document_path = $docxPath
    pdf_path = $pdfPath
    evidence_dir = $evidenceDir
    report_dir = $reportDir
    repair_dir = $repairDir
    generated_at = (Get-Date).ToString("s")
    status = "pending_review"
    verdict = "undecided"
    page_count = $summary.page_count
    evidence = [ordered]@{
        summary_json = $summaryPath
        checklist = $checklistPath
        contact_sheet = $contactSheetPath
        page_images = @($summary.pages)
    }
    findings = @()
    notes = @(
        "Fill findings after the visual Word review is complete.",
        "Keep verdict as pass/fail/undetermined according to screenshot evidence."
    )
}
($reviewResult | ConvertTo-Json -Depth 6) | Set-Content -Path $reviewResultPath -Encoding UTF8

$finalReview = @"
# Word Visual Review

- Document: $docxPath
- PDF: $pdfPath
- Evidence directory: $evidenceDir
- Report directory: $reportDir
- Repair directory: $repairDir
- Generated at: $(Get-Date -Format s)
- Current status: pending_review
- Verdict: pending_manual_review

## Summary

- Verdict:
- Notes:

## Findings

- Page:
  Element type:
  Description:
  Severity:
  Screenshot evidence:
  Confidence:

## Repair Decision

- Enter repair loop:
- Suggested fix:
- Current best candidate:
- Notes:
"@
$finalReview | Set-Content -Path $finalReviewPath -Encoding UTF8

$checklist = @"
# Word visual smoke checklist

- Verify the overview table banner spans the full first row without broken borders.
- Verify the yellow vertical-merge block spans two rows without duplicate content.
- Verify the multi-page audit table repeats its header row on every later page.
- Verify the highlighted cantSplit row (`R16`) stays entirely on one page.
- Verify fills, margins, and centered cells still look intentional after Word export.
- Verify the final merge matrix has no clipped text, border gaps, or missing shading.

Artifacts:

- DOCX: $docxPath
- PDF: $pdfPath
- Evidence directory: $evidenceDir
- PNG pages: $pagesDir
- Contact sheet: $contactSheetPath
- Report directory: $reportDir
- Summary JSON: $summaryPath
- Review result JSON: $reviewResultPath
- Final review Markdown: $finalReviewPath
- Reserved repair directory: $repairDir
"@
$checklist | Set-Content -Path $checklistPath -Encoding UTF8

Write-Step "Completed visual smoke run"
Write-Host "DOCX: $docxPath"
Write-Host "PDF: $pdfPath"
Write-Host "Evidence: $evidenceDir"
Write-Host "Pages: $pagesDir"
Write-Host "Contact sheet: $contactSheetPath"
Write-Host "Report: $reportDir"
Write-Host "Summary: $summaryPath"
Write-Host "Checklist: $checklistPath"
Write-Host "Review result: $reviewResultPath"
Write-Host "Final review: $finalReviewPath"
Write-Host "Repair: $repairDir"
