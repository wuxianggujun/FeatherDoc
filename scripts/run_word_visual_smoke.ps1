param(
    [string]$BuildDir = "build-word-visual-smoke-nmake",
    [string]$OutputDir = "output/word-visual-smoke",
    [string]$InputDocx = "",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$ShowRevisions,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord,
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "undecided",
    [string]$ReviewNote = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "word_visual_review_report.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[word-smoke] $Message"
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

function Test-WordComAvailability {
    try {
        $word = New-Object -ComObject Word.Application
        try {
            $word.Visible = $false
            $word.DisplayAlerts = 0
        } catch {
        }

        try {
            $word.Quit()
        } catch {
        }

        return $true
    } catch {
        return $false
    }
}

function Assert-WordVisualPrerequisites {
    param(
        [string]$DocxPath,
        [switch]$NeedsBuild,
        [switch]$HasInputDocx
    )

    Write-Step "Checking local Word visual prerequisites"

    $issues = New-Object System.Collections.Generic.List[string]

    try {
        $basePython = Get-BasePython
        Write-Step "Python found: $basePython"
    } catch {
        $issues.Add("Python was not found in PATH.")
    }

    if ($NeedsBuild) {
        try {
            $null = Get-VcvarsPath
            Write-Step "MSVC developer environment found"
        } catch {
            $issues.Add($_.Exception.Message)
        }
    }

    if ($HasInputDocx -and
        -not (Test-Path -LiteralPath $DocxPath)) {
        $issues.Add("Input DOCX was not found: $DocxPath")
    }

    if (-not (Test-WordComAvailability)) {
        $issues.Add(
            "Microsoft Word COM is unavailable. Install Microsoft Word or run this flow on a Windows host with Office."
        )
    }

    if ($issues.Count -gt 0) {
        $message = $issues -join "`n- "
        throw "Word visual smoke prerequisites are not satisfied:`n- $message"
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

function Get-DocxEntryNames {
    param([string]$DocxPath)

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        return @($archive.Entries | ForEach-Object { $_.FullName })
    } finally {
        $archive.Dispose()
    }
}

function Assert-GeneratedSmokeDocx {
    param([string]$DocxPath)

    if (-not (Test-Path $DocxPath)) {
        throw "Smoke sample was not created: $DocxPath"
    }

    $entries = Get-DocxEntryNames -DocxPath $DocxPath
    $requiredEntries = @(
        "[Content_Types].xml",
        "_rels/.rels",
        "word/document.xml",
        "word/_rels/document.xml.rels",
        "word/styles.xml"
    )

    $missingEntries = @($requiredEntries | Where-Object { $_ -notin $entries })
    if ($missingEntries.Count -gt 0) {
        $entryList = ($entries | Sort-Object) -join ", "
        $missingList = $missingEntries -join ", "
        throw "Generated smoke DOCX is missing required entries ($missingList). Existing entries: $entryList. Rebuild featherdoc_visual_smoke_tables before trusting this output."
    }
}

function Export-DocxToPdf {
    param(
        [string]$DocxPath,
        [string]$PdfPath,
        [bool]$Visible,
        [bool]$ShowRevisions,
        [bool]$KeepOpen
    )

    $word = $null
    $document = $null
    try {
        $word = New-Object -ComObject Word.Application
        $word.Visible = $Visible
        $word.DisplayAlerts = 0
        $document = $word.Documents.Open($DocxPath, $false, $true)
        $exportItem = 0
        if ($ShowRevisions) {
            $exportItem = 7
            try {
                $document.PrintRevisions = $true
            } catch {
            }
            try {
                $document.ShowRevisions = $true
            } catch {
            }
            try {
                $view = $document.ActiveWindow.View
                $view.ShowRevisionsAndComments = $true
                $view.RevisionsView = 0
                $view.ShowInsertionsAndDeletions = $true
                $view.ShowFormatChanges = $false
                $view.RevisionsMode = 1
            } catch {
            }
        }
        $document.ExportAsFixedFormat($PdfPath, 17, $false, 0, 0, 1, 1, $exportItem)
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

function Get-PdfPageCount {
    param(
        [string]$PythonCommand,
        [string]$PdfPath
    )

    $pageCountOutput = & $PythonCommand -c "import fitz, sys; document = fitz.open(sys.argv[1]); print(document.page_count)" $PdfPath
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to inspect PDF page count for $PdfPath"
    }

    $pageCountText = ($pageCountOutput | Select-Object -Last 1).ToString().Trim()
    $pageCount = 0
    if (-not [int]::TryParse($pageCountText, [ref]$pageCount)) {
        throw "Unexpected PDF page count output: $pageCountText"
    }

    return $pageCount
}

function Assert-RenderedEvidence {
    param(
        $Summary,
        [string]$PagesDir,
        [string]$ContactSheetPath,
        [int]$PdfPageCount
    )

    if (-not (Test-Path $ContactSheetPath)) {
        throw "Contact sheet was not created: $ContactSheetPath"
    }

    $pageImages = @($Summary.pages)
    if ($pageImages.Count -eq 0) {
        throw "Render step produced zero page images."
    }

    foreach ($pageImage in $pageImages) {
        if (-not (Test-Path $pageImage)) {
            throw "Summary references a missing page image: $pageImage"
        }
    }

    $actualPages = @(Get-ChildItem -Path $PagesDir -Filter "page-*.png" -File | Sort-Object Name)
    if ($actualPages.Count -ne $Summary.page_count) {
        throw "Summary page count ($($Summary.page_count)) does not match PNG count on disk ($($actualPages.Count))."
    }

    if ($Summary.page_count -ne $PdfPageCount) {
        throw "Rendered PNG count ($($Summary.page_count)) does not match PDF page count ($PdfPageCount)."
    }

    $lastExpectedPage = Join-Path $PagesDir ("page-{0:D2}.png" -f $Summary.page_count)
    if (-not (Test-Path $lastExpectedPage)) {
        throw "Expected last page PNG is missing: $lastExpectedPage"
    }
}

function Get-ReviewNotes {
    param(
        [switch]$CustomInput
    )

    if ($CustomInput) {
        return @(
            "Fill findings after the visual Word review is complete.",
            "Keep verdict as pass/fail/undetermined according to screenshot evidence.",
            "Confirm the exported PDF and rendered PNG pages match the intended Word layout for this custom input document.",
            "Check page breaks, text flow, tables, headers/footers, images, and spacing for unexpected drift after Word export.",
            "Use the generated contact sheet plus page PNGs as the source of truth when recording findings."
        )
    }

    return @(
        "Fill findings after the visual Word review is complete.",
        "Keep verdict as pass/fail/undetermined according to screenshot evidence.",
        "Confirm the Chinese/CJK and RTL/bidi sample text inherits readable fonts plus w:lang markers from docDefaults and style-based run formatting without tofu, broken RTL order, or unstable line wrapping.",
        "Confirm table-cell w:textDirection samples keep vertical or rotated text readable without clipped glyphs, border drift, or row-height collapse.",
        "Confirm narrow table cells with mixed RTL/LTR/CJK content keep sane ordering, punctuation, line wrapping, and no overlap.",
        "Confirm the fixed-grid merge_right() cue keeps the blue merged cell visibly wider than the 1000-twip base column and still narrower than the green 4100-twip tail column; if it collapses to the narrow width, tcW synchronization likely regressed.",
        "Confirm the unmerge showcase restores standalone orange and green cells after unmerge_right()/unmerge_down() without leftover merge artifacts, border breaks, or row-height collapse.",
        "Confirm the column-insertion showcase keeps inserted columns aligned after insert_cell_before()/insert_cell_after() and after the merged-boundary insert without broken borders, missing fills, or misplaced cell order.",
        "Confirm the column-width showcase keeps the left key column narrow, the middle column medium, and the right evidence column visibly widest after Table::set_column_width_twips(...) edits."
    )
}

function Get-ReviewChecklist {
    param(
        [switch]$CustomInput,
        [string]$DocxPath,
        [string]$PdfPath,
        [string]$EvidenceDir,
        [string]$PagesDir,
        [string]$ContactSheetPath,
        [string]$ReportDir,
        [string]$SummaryPath,
        [string]$ReviewResultPath,
        [string]$FinalReviewPath,
        [string]$RepairDir
    )

    if ($CustomInput) {
        return @"
# Word visual smoke checklist

- Verify the rendered pages match the expected Word layout for this custom input document.
- Verify page breaks, paragraph spacing, and line wrapping stay stable after Word export.
- Verify tables, borders, fills, alignment, and cell sizing do not drift unexpectedly.
- Verify images, floating objects, and any overlap or z-order-dependent layout remain visually correct.
- Verify headers, footers, and page-number fields still appear where expected.
- Record any page-specific regressions in final_review.md and review_result.json.

Artifacts:

- DOCX: $DocxPath
- PDF: $PdfPath
- Evidence directory: $EvidenceDir
- PNG pages: $PagesDir
- Contact sheet: $ContactSheetPath
- Report directory: $ReportDir
- Summary JSON: $SummaryPath
- Review result JSON: $ReviewResultPath
- Final review Markdown: $FinalReviewPath
- Reserved repair directory: $RepairDir
"@
    }

    return @"
# Word visual smoke checklist

- Verify the overview table banner spans the full first row without broken borders.
- Verify the yellow vertical-merge block spans two rows without duplicate content.
- Verify the multi-page audit table repeats its header row on every later page.
- Verify the highlighted cantSplit row (``R16``) stays entirely on one page.
- Verify fills, margins, and centered cells still look intentional after Word export.
- Verify the Chinese/CJK and RTL/bidi samples inherit readable glyphs plus ``w:lang`` language markers from docDefaults and ``Strong`` style formatting with stable line breaks, sane RTL order, and no obvious fallback-font drift.
- Verify the direction stress table keeps table-cell ``w:textDirection`` vertical/rotated text readable, with stable row heights and no clipped glyphs.
- Verify the narrow mixed RTL/LTR/CJK cells keep sane wrap order, punctuation placement, border continuity, and no overlap beside rotated cells.
- Verify the fixed-grid ``merge_right()`` cue keeps the blue merged cell visibly wider than the gray ``1000`` base column and still narrower than the green ``4100`` tail column.
- Verify the unmerge showcase restores the orange and green cells as clean standalone cells after ``unmerge_right()`` / ``unmerge_down()`` without stray merge artifacts.
- Verify the yellow/orange inserted columns stay in the expected order after ``insert_cell_after()`` and ``insert_cell_before()`` without width collapse, broken borders, or misplaced fills.
- Verify the merged-boundary insertion keeps a yellow cell between the blue merged block and the green tail without stray merge artifacts.
- Verify the column-width showcase keeps the blue key column narrow, the yellow middle column medium, and the green evidence column clearly widest.
- Verify the final merge matrix has no clipped text, border gaps, or missing shading.

Artifacts:

- DOCX: $DocxPath
- PDF: $PdfPath
- Evidence directory: $EvidenceDir
- PNG pages: $PagesDir
- Contact sheet: $ContactSheetPath
- Report directory: $ReportDir
- Summary JSON: $SummaryPath
- Review result JSON: $ReviewResultPath
- Final review Markdown: $FinalReviewPath
- Reserved repair directory: $RepairDir
"@
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$evidenceDir = Join-Path $resolvedOutputDir "evidence"
$pagesDir = Join-Path $evidenceDir "pages"
$reportDir = Join-Path $resolvedOutputDir "report"
$repairDir = Join-Path $resolvedOutputDir "repair"
$docxPath = if ($InputDocx) {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $InputDocx
} else {
    Join-Path $resolvedOutputDir "table_visual_smoke.docx"
}
$needsBuild = ([string]::IsNullOrWhiteSpace($InputDocx) -and -not $SkipBuild)
$hasInputDocx = -not [string]::IsNullOrWhiteSpace($InputDocx)
Assert-WordVisualPrerequisites -DocxPath $docxPath -NeedsBuild:($needsBuild) -HasInputDocx:($hasInputDocx)
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
        $vcvarsPath = Get-VcvarsPath
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
    Assert-GeneratedSmokeDocx -DocxPath $docxPath
} else {
    Write-Step "Using existing DOCX: $docxPath"
}

Write-Step "Exporting DOCX to PDF through Microsoft Word"
Export-DocxToPdf -DocxPath $docxPath -PdfPath $pdfPath -Visible $VisibleWord.IsPresent -ShowRevisions $ShowRevisions.IsPresent -KeepOpen $KeepWordOpen.IsPresent

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
$pdfPageCount = Get-PdfPageCount -PythonCommand $renderPython -PdfPath $pdfPath
Assert-RenderedEvidence -Summary $summary -PagesDir $pagesDir -ContactSheetPath $contactSheetPath -PdfPageCount $pdfPageCount
$customInput = [bool]$InputDocx
$generatedAt = (Get-Date).ToString("s")
$reviewNotes = @(Get-ReviewNotes -CustomInput:$customInput)
if ($ShowRevisions) {
    $reviewNotes += "ShowRevisions was enabled, so the Word PDF/PNG evidence should display tracked insertion and deletion markup instead of only the final accepted text."
}
$reviewResult = New-WordVisualReviewResult `
    -DocumentPath $docxPath `
    -PdfPath $pdfPath `
    -EvidenceDir $evidenceDir `
    -ReportDir $reportDir `
    -RepairDir $repairDir `
    -GeneratedAt $generatedAt `
    -ReviewVerdict $ReviewVerdict `
    -PageCount $summary.page_count `
    -SummaryPath $summaryPath `
    -ChecklistPath $checklistPath `
    -ContactSheetPath $contactSheetPath `
    -PageImages @($summary.pages) `
    -Notes $reviewNotes `
    -ReviewNote $ReviewNote
($reviewResult | ConvertTo-Json -Depth 6) | Set-Content -Path $reviewResultPath -Encoding UTF8

$finalReview = New-WordVisualFinalReviewMarkdown `
    -DocumentPath $docxPath `
    -PdfPath $pdfPath `
    -EvidenceDir $evidenceDir `
    -ReportDir $reportDir `
    -RepairDir $repairDir `
    -GeneratedAt $generatedAt `
    -ReviewVerdict $ReviewVerdict `
    -ReviewNote $ReviewNote
$finalReview | Set-Content -Path $finalReviewPath -Encoding UTF8

$checklist = Get-ReviewChecklist -CustomInput:$customInput `
    -DocxPath $docxPath `
    -PdfPath $pdfPath `
    -EvidenceDir $evidenceDir `
    -PagesDir $pagesDir `
    -ContactSheetPath $contactSheetPath `
    -ReportDir $reportDir `
    -SummaryPath $summaryPath `
    -ReviewResultPath $reviewResultPath `
    -FinalReviewPath $finalReviewPath `
    -RepairDir $repairDir
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
