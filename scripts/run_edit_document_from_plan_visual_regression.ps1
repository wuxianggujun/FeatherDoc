param(
    [string]$BuildDir = "build-codex-clang-compat",
    [string]$OutputDir = "output/edit-document-from-plan-visual-regression",
    [string]$InputDocx = "samples/chinese_invoice_template.docx",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[edit-document-visual] $Message"
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

function Write-Utf8NoBom {
    param(
        [string]$Path,
        [string]$Text
    )

    $parent = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }

    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $utf8NoBom)
}

function Convert-JsonEscapedText {
    param([string]$EscapedText)

    return ConvertFrom-Json -InputObject ('"' + $EscapedText + '"')
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
    if (Test-PythonImport -PythonCommand $basePython -ModuleName "PIL") {
        return $basePython
    }

    $venvDir = Join-Path $RepoRoot ".venv-word-visual-smoke"
    $venvPython = Join-Path $venvDir "Scripts\python.exe"
    if (-not (Test-Path -LiteralPath $venvPython)) {
        Write-Step "Creating local Python environment at $venvDir"
        $null = & $basePython -m venv $venvDir
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to create local Python virtual environment."
        }
    }

    if (-not (Test-PythonImport -PythonCommand $venvPython -ModuleName "PIL")) {
        Write-Step "Installing Pillow into the local environment"
        & $venvPython -m pip install --disable-pip-version-check pillow 2>&1 |
            ForEach-Object { Write-Host $_ }
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to install Pillow into the local environment."
        }
    }

    return $venvPython
}

function Invoke-WordSmoke {
    param(
        [string]$ScriptPath,
        [string]$BuildDir,
        [string]$DocxPath,
        [string]$OutputDir,
        [int]$Dpi,
        [bool]$KeepWordOpen,
        [bool]$VisibleWord
    )

    & $ScriptPath `
        -BuildDir $BuildDir `
        -InputDocx $DocxPath `
        -OutputDir $OutputDir `
        -SkipBuild `
        -Dpi $Dpi `
        -KeepWordOpen:$KeepWordOpen `
        -VisibleWord:$VisibleWord
    if ($LASTEXITCODE -ne 0) {
        throw "Word visual smoke failed for $DocxPath."
    }
}

function Build-ContactSheet {
    param(
        [string]$Python,
        [string]$ScriptPath,
        [string]$OutputPath,
        [string[]]$Images,
        [string[]]$Labels
    )

    & $Python $ScriptPath --output $OutputPath --columns 2 --thumb-width 520 --images $Images --labels $Labels
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build contact sheet at $OutputPath."
    }
}

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Label was not created: $Path"
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Label
    )

    if (-not $Text.Contains($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText'."
    }
}

function Assert-NotContainsText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Label
    )

    if ($Text.Contains($UnexpectedText)) {
        throw "$Label unexpectedly contains '$UnexpectedText'."
    }
}

function Read-DocxEntryText {
    param(
        [string]$DocxPath,
        [string]$EntryName
    )

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        $entry = $archive.GetEntry($EntryName)
        if ($null -eq $entry) {
            throw "Entry '$EntryName' was not found in $DocxPath."
        }

        $reader = New-Object System.IO.StreamReader($entry.Open())
        try {
            return $reader.ReadToEnd()
        } finally {
            $reader.Dispose()
        }
    } finally {
        $archive.Dispose()
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$resolvedInputDocx = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $InputDocx
$editScript = Join-Path $repoRoot "scripts\edit_document_from_plan.ps1"
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"

$editPlanPath = Join-Path $resolvedOutputDir "invoice.edit_plan.json"
$editedDocxPath = Join-Path $resolvedOutputDir "invoice.edited.docx"
$editSummaryPath = Join-Path $resolvedOutputDir "invoice.edit.summary.json"
$baselineVisualDir = Join-Path $resolvedOutputDir "baseline-visual"
$editedVisualDir = Join-Path $resolvedOutputDir "edited-visual"
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$selectedPagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$beforePagePath = Join-Path $selectedPagesDir "baseline-page-01.png"
$afterPagePath = Join-Path $selectedPagesDir "edited-page-01.png"
$contactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$reviewMarkdownPath = Join-Path $resolvedOutputDir "VISUAL_REVIEW.md"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $selectedPagesDir -Force | Out-Null

if (-not $SkipBuild) {
    Write-Step "Configuring build directory $resolvedBuildDir"
    & cmake -S $repoRoot -B $resolvedBuildDir -DBUILD_CLI=ON
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to configure build directory."
    }

    Write-Step "Building featherdoc_cli"
    & cmake --build $resolvedBuildDir --target featherdoc_cli --parallel 4
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build featherdoc_cli."
    }
}

$expectedCustomerName = Convert-JsonEscapedText "\u4e0a\u6d77\u7fbd\u6587\u6863\u79d1\u6280\u6709\u9650\u516c\u53f8"
$expectedInvoiceNumber = Convert-JsonEscapedText "\u7f16\u8f91\u8ba1\u5212-2026-0424"
$expectedIssueDate = Convert-JsonEscapedText "2026\u5e744\u670824\u65e5"
$expectedNotePrefix = Convert-JsonEscapedText "\u76f4\u63a5\u7f16\u8f91\u5df2\u6709 DOCX"
$expectedTableItem = Convert-JsonEscapedText "\u8868\u683c\u4fee\u6539"
$expectedReplacementText = Convert-JsonEscapedText "JSON \u53ef\u590d\u7528\u5e76\u53ef\u5ba1\u9605"
$unexpectedReplacementText = Convert-JsonEscapedText "JSON \u53ef\u4ee5\u590d\u7528"
$deletedParagraphText = Convert-JsonEscapedText "\u4e34\u65f6\u8bf4\u660e\u4f1a\u88ab\u5220\u9664"

Write-Step "Writing sample edit plan"
Write-Utf8NoBom -Path $editPlanPath -Text @'
{
  "operations": [
    {
      "op": "replace_bookmark_text",
      "bookmark": "customer_name",
      "text": "\u4e0a\u6d77\u7fbd\u6587\u6863\u79d1\u6280\u6709\u9650\u516c\u53f8"
    },
    {
      "op": "replace_bookmark_text",
      "bookmark": "invoice_number",
      "text": "\u7f16\u8f91\u8ba1\u5212-2026-0424"
    },
    {
      "op": "replace_bookmark_text",
      "bookmark": "issue_date",
      "text": "2026\u5e744\u670824\u65e5"
    },
    {
      "op": "replace_bookmark_paragraphs",
      "bookmark": "note_lines",
      "paragraphs": [
        "1. \u76f4\u63a5\u7f16\u8f91\u5df2\u6709 DOCX\u3002",
        "2. \u4fee\u6539\u8ba1\u5212 JSON \u53ef\u4ee5\u590d\u7528\u3002",
        "3. \u4e34\u65f6\u8bf4\u660e\u4f1a\u88ab\u5220\u9664\u3002"
      ]
    },
    {
      "op": "replace_bookmark_table_rows",
      "bookmark": "line_item_row",
      "rows": [
        [
          "\u6587\u672c\u66ff\u6362",
          "\u6309\u4e66\u7b7e\u66ff\u6362\u6b63\u6587\u4e2d\u7684\u4e1a\u52a1\u5b57\u6bb5",
          "1,200.00"
        ],
        [
          "\u8868\u683c\u4fee\u6539",
          "\u6309\u4e66\u7b7e\u66ff\u6362\u6a21\u677f\u8868\u683c\u884c",
          "2,800.00"
        ]
      ]
    },
    {
      "op": "set_table_cell_text",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "text": "4,000.00"
    },
    {
      "op": "set_table_cell_fill",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "background_color": "FFF2CC"
    },
    {
      "op": "set_table_cell_border",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "edge": "right",
      "style": "single",
      "thickness": 16,
      "color": "FF0000"
    },
    {
      "op": "set_table_row_height",
      "table_index": 0,
      "row_index": 3,
      "height_twips": 720,
      "rule": "exact"
    },
    {
      "op": "set_table_cell_vertical_alignment",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "alignment": "center"
    },
    {
      "op": "set_table_cell_horizontal_alignment",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "alignment": "right"
    },
    {
      "op": "replace_text",
      "find": "JSON \u53ef\u4ee5\u590d\u7528",
      "replace": "JSON \u53ef\u590d\u7528\u5e76\u53ef\u5ba1\u9605"
    },
    {
      "op": "set_text_style",
      "text_contains": "JSON \u53ef\u590d\u7528\u5e76\u53ef\u5ba1\u9605",
      "bold": true,
      "font_family": "Segoe UI",
      "east_asia_font_family": "Microsoft YaHei",
      "language": "en-US",
      "east_asia_language": "zh-CN",
      "color": "C00000",
      "font_size_points": 14
    },
    {
      "op": "delete_paragraph_contains",
      "text_contains": "\u4e34\u65f6\u8bf4\u660e"
    },
    {
      "op": "set_paragraph_horizontal_alignment",
      "text_contains": "\u76f4\u63a5\u7f16\u8f91\u5df2\u6709 DOCX",
      "alignment": "center"
    },
    {
      "op": "set_paragraph_spacing",
      "text_contains": "\u76f4\u63a5\u7f16\u8f91\u5df2\u6709 DOCX",
      "before_twips": 120,
      "after_twips": 240,
      "line_twips": 360,
      "line_rule": "exact"
    },
    {
      "op": "set_table_column_width",
      "table_index": 0,
      "column_index": 1,
      "width_twips": 5200
    },
    {
      "op": "merge_table_cells",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 0,
      "direction": "right"
    }
  ]
}
'@

Write-Step "Applying edit plan to DOCX"
& $editScript `
    -InputDocx $resolvedInputDocx `
    -EditPlan $editPlanPath `
    -OutputDocx $editedDocxPath `
    -SummaryJson $editSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild
if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed."
}

Assert-PathExists -Path $editedDocxPath -Label "Edited DOCX"
Assert-PathExists -Path $editSummaryPath -Label "Edit summary JSON"

$editSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $editSummaryPath | ConvertFrom-Json
if ($editSummary.status -ne "completed") {
    throw "Edit summary did not report status=completed."
}
if ($editSummary.operation_count -ne 18) {
    throw "Edit summary should record eighteen operations."
}

$documentXml = Read-DocxEntryText -DocxPath $editedDocxPath -EntryName "word/document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedCustomerName -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedInvoiceNumber -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedIssueDate -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedNotePrefix -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedTableItem -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "4,000.00" -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedReplacementText -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:trHeight w:val="720" w:hRule="exact"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:vAlign w:val="center"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:jc w:val="right"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:jc w:val="center"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:fill="FFF2CC"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:right w:val="single" w:sz="16" w:space="0" w:color="FF0000"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:b w:val="1"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:ascii="Segoe UI"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:eastAsia="Microsoft YaHei"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:eastAsia="zh-CN"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:color w:val="C00000"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:sz w:val="28"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:szCs w:val="28"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:spacing w:before="120" w:after="240" w:line="360" w:lineRule="exact"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:gridCol w:w="5200"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:tcW w:w="5200" w:type="dxa"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:gridSpan w:val="2"' -Label "Edited document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "12,800.00" -Label "Edited document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText $unexpectedReplacementText -Label "Edited document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText $deletedParagraphText -Label "Edited document.xml"

Write-Step "Rendering baseline DOCX through Word"
Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $resolvedInputDocx `
    -OutputDir $baselineVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

Write-Step "Rendering edited DOCX through Word"
Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $editedDocxPath `
    -OutputDir $editedVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$baselineFirstPage = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
$editedFirstPage = Join-Path $editedVisualDir "evidence\pages\page-01.png"
Assert-PathExists -Path $baselineFirstPage -Label "Baseline first page PNG"
Assert-PathExists -Path $editedFirstPage -Label "Edited first page PNG"
Copy-Item -Path $baselineFirstPage -Destination $beforePagePath -Force
Copy-Item -Path $editedFirstPage -Destination $afterPagePath -Force

$renderPython = Ensure-RenderPython -RepoRoot $repoRoot
Write-Step "Building before/after contact sheet"
Build-ContactSheet `
    -Python $renderPython `
    -ScriptPath $contactSheetScript `
    -OutputPath $contactSheetPath `
    -Images @($beforePagePath, $afterPagePath) `
    -Labels @("baseline template", "edited from plan")

$expectedVisualCues = @(
    "Customer name, invoice number, and issue date are no longer template placeholders.",
    "The line-item template row expands into two business rows and the description column is wider.",
    "The total row merges its first two cells, and the total amount changes from 12,800.00 to 4,000.00.",
    "The total amount cell gains a light yellow background and a red right border.",
    "The total amount row is taller, vertically centered, and right-aligned.",
    "The first note paragraph is centered and has explicit paragraph spacing.",
    "The second note paragraph is updated, bold, red, larger, and carries Latin/CJK font metadata.",
    "A temporary third note paragraph is removed by content.",
    "The note block ends with two user-facing paragraphs."
)

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    input_docx = $resolvedInputDocx
    edit_plan = $editPlanPath
    edited_docx = $editedDocxPath
    edit_summary = $editSummaryPath
    visual_enabled = $true
    expected_text = [ordered]@{
        customer_name = $expectedCustomerName
        invoice_number = $expectedInvoiceNumber
        issue_date = $expectedIssueDate
        total_amount = "4,000.00"
    }
    expected_visual_cues = $expectedVisualCues
    baseline_visual = [ordered]@{
        root = $baselineVisualDir
        page_01 = $baselineFirstPage
        contact_sheet = (Join-Path $baselineVisualDir "evidence\contact_sheet.png")
        review_markdown = (Join-Path $baselineVisualDir "report\final_review.md")
    }
    edited_visual = [ordered]@{
        root = $editedVisualDir
        page_01 = $editedFirstPage
        contact_sheet = (Join-Path $editedVisualDir "evidence\contact_sheet.png")
        review_markdown = (Join-Path $editedVisualDir "report\final_review.md")
    }
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $selectedPagesDir
        contact_sheet = $contactSheetPath
    }
    review_markdown = $reviewMarkdownPath
}
Write-Utf8NoBom -Path $summaryPath -Text (($summary | ConvertTo-Json -Depth 8) + [Environment]::NewLine)

$reviewMarkdown = @"
# Edit Document From Plan Visual Review

- Status: pending_manual_review
- Baseline DOCX: $resolvedInputDocx
- Edited DOCX: $editedDocxPath
- Edit summary: $editSummaryPath
- Before/after contact sheet: $contactSheetPath

## What To Verify

- Customer name, invoice number, and issue date changed from template placeholders.
- The line-item table has two generated business rows, with a wider description column.
- The total row merges its first two cells, and the amount shows 4,000.00 instead of 12,800.00.
- The total amount cell has a light yellow fill and a visible red right border.
- The total amount row is taller; the total cell is vertically centered and right-aligned.
- The first delivery-note paragraph is centered and has extra paragraph spacing.
- The second delivery-note paragraph says that JSON is reusable/reviewable and appears bold, red, and slightly larger.
- The temporary third delivery-note paragraph has been removed.
- The delivery note block contains two edited paragraphs.

## Verdict

- Result:
- Notes:
"@
Write-Utf8NoBom -Path $reviewMarkdownPath -Text $reviewMarkdown

Write-Step "Completed edit-document visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Review: $reviewMarkdownPath"
Write-Host "Before/after contact sheet: $contactSheetPath"
Write-Host "Edited DOCX: $editedDocxPath"
