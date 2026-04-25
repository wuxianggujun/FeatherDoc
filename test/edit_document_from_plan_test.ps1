param(
    [string]$RepoRoot,
    [string]$BuildDir,
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

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Label
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText'."
    }
}

function Assert-NotContainsText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Label
    )

    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw "$Label unexpectedly contains '$UnexpectedText'."
    }
}

function Convert-JsonEscapedText {
    param([string]$EscapedText)

    return ConvertFrom-Json -InputObject ('"' + $EscapedText + '"')
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

function New-WordNamespaceManager {
    param([System.Xml.XmlDocument]$Document)

    $namespaceManager = New-Object System.Xml.XmlNamespaceManager($Document.NameTable)
    $null = $namespaceManager.AddNamespace("w", "http://schemas.openxmlformats.org/wordprocessingml/2006/main")
    Write-Output -NoEnumerate $namespaceManager
}

function Assert-DocxXPath {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$XPath,
        [string]$Message
    )

    if ($null -eq $Document.SelectSingleNode($XPath, $NamespaceManager)) {
        throw $Message
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$editPlanPath = Join-Path $resolvedWorkingDir "invoice.edit_plan.json"
$editedDocx = Join-Path $resolvedWorkingDir "invoice.edited.docx"
$summaryPath = Join-Path $resolvedWorkingDir "invoice.edit.summary.json"

$expectedCustomerName = Convert-JsonEscapedText "\u4e0a\u6d77\u7fbd\u6587\u6863\u79d1\u6280\u6709\u9650\u516c\u53f8"
$expectedInvoiceNumber = Convert-JsonEscapedText "\u7f16\u8f91\u8ba1\u5212-2026-0424"
$expectedIssueDate = Convert-JsonEscapedText "2026\u5e744\u670824\u65e5"
$expectedNotePrefix = Convert-JsonEscapedText "\u76f4\u63a5\u7f16\u8f91\u5df2\u6709 DOCX"
$expectedTableItem = Convert-JsonEscapedText "\u8868\u683c\u4fee\u6539"
$expectedReplacementText = Convert-JsonEscapedText "JSON \u53ef\u590d\u7528\u5e76\u53ef\u5ba1\u9605"
$unexpectedReplacementText = Convert-JsonEscapedText "JSON \u53ef\u4ee5\u590d\u7528"
$deletedParagraphText = Convert-JsonEscapedText "\u4e34\u65f6\u8bf4\u660e\u4f1a\u88ab\u5220\u9664"

Set-Content -LiteralPath $editPlanPath -Encoding UTF8 -Value @'
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

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $editPlanPath `
    -OutputDocx $editedDocx `
    -SummaryJson $summaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $editedDocx) `
    -Message "Edited DOCX was not created."
Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "Edit summary JSON was not created."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$documentXml = Read-DocxEntryText -DocxPath $editedDocx -EntryName "word/document.xml"
$document = New-Object System.Xml.XmlDocument
$document.PreserveWhitespace = $true
$document.LoadXml($documentXml)
$namespaceManager = New-WordNamespaceManager -Document $document

Assert-Equal -Actual $summary.status -Expected "completed" `
    -Message "Edit summary did not report status=completed."
Assert-Equal -Actual $summary.operation_count -Expected 18 `
    -Message "Edit summary should record eighteen operations."
Assert-Equal -Actual $summary.operations[0].op -Expected "replace_bookmark_text" `
    -Message "First operation should be bookmark text replacement."
Assert-Equal -Actual $summary.operations[3].op -Expected "replace_bookmark_paragraphs" `
    -Message "Fourth operation should be bookmark paragraph replacement."
Assert-Equal -Actual $summary.operations[4].op -Expected "replace_bookmark_table_rows" `
    -Message "Fifth operation should be bookmark table-row replacement."
Assert-Equal -Actual $summary.operations[4].status -Expected "completed" `
    -Message "Table-row edit operation did not complete."
Assert-Equal -Actual $summary.operations[5].op -Expected "set_table_cell_text" `
    -Message "Sixth operation should be table-cell text replacement."
Assert-Equal -Actual $summary.operations[5].status -Expected "completed" `
    -Message "Table-cell edit operation did not complete."
Assert-Equal -Actual $summary.operations[6].op -Expected "set_table_cell_fill" `
    -Message "Seventh operation should set table-cell fill color."
Assert-Equal -Actual $summary.operations[6].command -Expected "set-table-cell-fill" `
    -Message "Table-cell fill should use the CLI fill command."
Assert-Equal -Actual $summary.operations[7].op -Expected "set_table_cell_border" `
    -Message "Eighth operation should set table-cell border."
Assert-Equal -Actual $summary.operations[7].command -Expected "set-table-cell-border" `
    -Message "Table-cell border should use the CLI border command."
Assert-Equal -Actual $summary.operations[8].op -Expected "set_table_row_height" `
    -Message "Ninth operation should be table-row height replacement."
Assert-Equal -Actual $summary.operations[9].op -Expected "set_table_cell_vertical_alignment" `
    -Message "Tenth operation should be table-cell vertical alignment."
Assert-Equal -Actual $summary.operations[10].op -Expected "set_table_cell_horizontal_alignment" `
    -Message "Eleventh operation should be table-cell horizontal alignment."
Assert-Equal -Actual $summary.operations[10].command -Expected "set-table-cell-horizontal-alignment" `
    -Message "Horizontal alignment should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[11].op -Expected "replace_text" `
    -Message "Twelfth operation should be ordinary text replacement."
Assert-Equal -Actual $summary.operations[11].command -Expected "replace-text" `
    -Message "Ordinary text replacement should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[12].op -Expected "set_text_style" `
    -Message "Thirteenth operation should style matching text."
Assert-Equal -Actual $summary.operations[12].command -Expected "set-text-style" `
    -Message "Text style should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[13].op -Expected "delete_paragraph_contains" `
    -Message "Fourteenth operation should delete a paragraph by content."
Assert-Equal -Actual $summary.operations[13].command -Expected "delete-paragraph" `
    -Message "Paragraph deletion should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[14].op -Expected "set_paragraph_horizontal_alignment" `
    -Message "Fifteenth operation should be paragraph horizontal alignment."
Assert-Equal -Actual $summary.operations[14].command -Expected "set-paragraph-horizontal-alignment" `
    -Message "Paragraph horizontal alignment should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[15].op -Expected "set_paragraph_spacing" `
    -Message "Sixteenth operation should be paragraph spacing."
Assert-Equal -Actual $summary.operations[15].command -Expected "set-paragraph-spacing" `
    -Message "Paragraph spacing should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[16].op -Expected "set_table_column_width" `
    -Message "Seventeenth operation should be table-column width."
Assert-Equal -Actual $summary.operations[16].command -Expected "set-table-column-width" `
    -Message "Table-column width should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[17].op -Expected "merge_table_cells" `
    -Message "Eighteenth operation should merge table cells."
Assert-Equal -Actual $summary.operations[17].command -Expected "merge-table-cells" `
    -Message "Table-cell merge should use the CLI merge command."

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
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tcPr/w:shd[@w:fill='FFF2CC']" `
    -Message "Edited document.xml should contain the table-cell fill color."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tcPr/w:tcBorders/w:right[@w:val='single' and @w:sz='16' and @w:color='FF0000']" `
    -Message "Edited document.xml should contain the right table-cell border."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:b[@w:val='1']" `
    -Message "Edited document.xml should contain bold text style."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:rFonts[@w:ascii='Segoe UI' and @w:hAnsi='Segoe UI' and @w:eastAsia='Microsoft YaHei']" `
    -Message "Edited document.xml should contain Latin and Chinese font settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:lang[@w:val='en-US' and @w:eastAsia='zh-CN']" `
    -Message "Edited document.xml should contain Latin and Chinese language settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:color[@w:val='C00000']" `
    -Message "Edited document.xml should contain text color settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:sz[@w:val='28']" `
    -Message "Edited document.xml should contain primary font-size settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:szCs[@w:val='28']" `
    -Message "Edited document.xml should contain complex-script font-size settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:pPr/w:spacing[@w:before='120' and @w:after='240' and @w:line='360' and @w:lineRule='exact']" `
    -Message "Edited document.xml should contain paragraph spacing settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblGrid/w:gridCol[2][@w:w='5200']" `
    -Message "Edited document.xml should contain the updated table-column grid width."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tr/w:tc[2]/w:tcPr/w:tcW[@w:w='5200' and @w:type='dxa']" `
    -Message "Edited document.xml should contain table-cell widths for the updated column."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tr[4]/w:tc[1]/w:tcPr/w:gridSpan[@w:val='2']" `
    -Message "Edited document.xml should contain the horizontal table-cell merge."
Assert-NotContainsText -Text $documentXml -UnexpectedText "12,800.00" -Label "Edited document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText $unexpectedReplacementText -Label "Edited document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText $deletedParagraphText -Label "Edited document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "TODO:" -Label "Edited document.xml"

Write-Host "Edit-from-plan regression passed."
