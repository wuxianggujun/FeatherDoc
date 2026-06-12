$tableColumnWidthPlanPath = Join-Path $resolvedWorkingDir "invoice.table_column_width_plan.json"
$tableColumnWidthDocx = Join-Path $resolvedWorkingDir "invoice.table_column_width.docx"
$tableColumnWidthSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_column_width.summary.json"

Set-Content -LiteralPath $tableColumnWidthPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_table_column_width",
      "table_index": 0,
      "column_index": 1,
      "width_twips": 2600
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $tableColumnWidthPlanPath `
    -OutputDocx $tableColumnWidthDocx `
    -SummaryJson $tableColumnWidthSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table column width plan."
}

$tableColumnWidthSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableColumnWidthSummaryPath | ConvertFrom-Json
$tableColumnWidthXml = Read-DocxEntryText -DocxPath $tableColumnWidthDocx -EntryName "word/document.xml"
$tableColumnWidthDocument = New-Object System.Xml.XmlDocument
$tableColumnWidthDocument.PreserveWhitespace = $true
$tableColumnWidthDocument.LoadXml($tableColumnWidthXml)
$tableColumnWidthNamespaceManager = New-WordNamespaceManager -Document $tableColumnWidthDocument

Assert-Equal -Actual $tableColumnWidthSummary.status -Expected "completed" `
    -Message "Table-column-width summary did not report status=completed."
Assert-Equal -Actual $tableColumnWidthSummary.operation_count -Expected 1 `
    -Message "Table-column-width summary should record one operation."
Assert-Equal -Actual $tableColumnWidthSummary.operations[0].command -Expected "set-table-column-width" `
    -Message "Set-table-column-width operation should use the table-column-width command."
Assert-DocxXPath `
    -Document $tableColumnWidthDocument `
    -NamespaceManager $tableColumnWidthNamespaceManager `
    -XPath "//w:tbl[1]/w:tblGrid/w:gridCol[2][@w:w='2600']" `
    -Message "Table-column-width output should set the second grid column width."
Assert-DocxXPath `
    -Document $tableColumnWidthDocument `
    -NamespaceManager $tableColumnWidthNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[2]/w:tcPr/w:tcW[@w:w='2600' and @w:type='dxa']" `
    -Message "Table-column-width output should set the second first-row cell width."

$tableColumnWidthClearPlanPath = Join-Path $resolvedWorkingDir "invoice.table_column_width_clear_plan.json"
$tableColumnWidthClearedDocx = Join-Path $resolvedWorkingDir "invoice.table_column_width_cleared.docx"
$tableColumnWidthClearSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_column_width_cleared.summary.json"

Set-Content -LiteralPath $tableColumnWidthClearPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_table_column_width",
      "table_index": 0,
      "column_index": 1
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $tableColumnWidthDocx `
    -EditPlan $tableColumnWidthClearPlanPath `
    -OutputDocx $tableColumnWidthClearedDocx `
    -SummaryJson $tableColumnWidthClearSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table column width clear plan."
}

$tableColumnWidthClearSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableColumnWidthClearSummaryPath | ConvertFrom-Json
$tableColumnWidthClearedXml = Read-DocxEntryText -DocxPath $tableColumnWidthClearedDocx -EntryName "word/document.xml"
$tableColumnWidthClearedDocument = New-Object System.Xml.XmlDocument
$tableColumnWidthClearedDocument.PreserveWhitespace = $true
$tableColumnWidthClearedDocument.LoadXml($tableColumnWidthClearedXml)
$tableColumnWidthClearedNamespaceManager = New-WordNamespaceManager -Document $tableColumnWidthClearedDocument

Assert-Equal -Actual $tableColumnWidthClearSummary.status -Expected "completed" `
    -Message "Table-column-width-clear summary did not report status=completed."
Assert-Equal -Actual $tableColumnWidthClearSummary.operation_count -Expected 1 `
    -Message "Table-column-width-clear summary should record one operation."
Assert-Equal -Actual $tableColumnWidthClearSummary.operations[0].command -Expected "clear-table-column-width" `
    -Message "Clear-table-column-width operation should use the table-column-width command."

if ($null -ne $tableColumnWidthClearedDocument.SelectSingleNode("//w:tbl[1]/w:tblGrid/w:gridCol[2][@w:w]", $tableColumnWidthClearedNamespaceManager)) {
    throw "Table-column-width clear output should remove the second grid column width."
}
if ($null -ne $tableColumnWidthClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[2]/w:tcPr/w:tcW", $tableColumnWidthClearedNamespaceManager)) {
    throw "Table-column-width clear output should remove the second first-row cell width."
}

$tableRowPropertiesPlanPath = Join-Path $resolvedWorkingDir "invoice.table_row_properties_plan.json"
$tableRowPropertiesDocx = Join-Path $resolvedWorkingDir "invoice.table_row_properties.docx"
$tableRowPropertiesSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_row_properties.summary.json"

Set-Content -LiteralPath $tableRowPropertiesPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_table_row_height",
      "table_index": 0,
      "row_index": 0,
      "height_twips": 720,
      "rule": "exact"
    },
    {
      "op": "set_table_row_cant_split",
      "table_index": 0,
      "row_index": 0
    },
    {
      "op": "set_table_row_repeat_header",
      "table_index": 0,
      "row_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $tableRowPropertiesPlanPath `
    -OutputDocx $tableRowPropertiesDocx `
    -SummaryJson $tableRowPropertiesSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table row properties plan."
}

$tableRowPropertiesSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableRowPropertiesSummaryPath | ConvertFrom-Json
$tableRowPropertiesXml = Read-DocxEntryText -DocxPath $tableRowPropertiesDocx -EntryName "word/document.xml"
$tableRowPropertiesDocument = New-Object System.Xml.XmlDocument
$tableRowPropertiesDocument.PreserveWhitespace = $true
$tableRowPropertiesDocument.LoadXml($tableRowPropertiesXml)
$tableRowPropertiesNamespaceManager = New-WordNamespaceManager -Document $tableRowPropertiesDocument

Assert-Equal -Actual $tableRowPropertiesSummary.status -Expected "completed" `
    -Message "Table-row-properties summary did not report status=completed."
Assert-Equal -Actual $tableRowPropertiesSummary.operation_count -Expected 3 `
    -Message "Table-row-properties summary should record three operations."
Assert-Equal -Actual $tableRowPropertiesSummary.operations[0].command -Expected "set-table-row-height" `
    -Message "Set-table-row-height operation should use the CLI set-table-row-height command."
Assert-Equal -Actual $tableRowPropertiesSummary.operations[1].command -Expected "set-table-row-cant-split" `
    -Message "Set-table-row-cant-split operation should use the CLI set-table-row-cant-split command."
Assert-Equal -Actual $tableRowPropertiesSummary.operations[2].command -Expected "set-table-row-repeat-header" `
    -Message "Set-table-row-repeat-header operation should use the CLI set-table-row-repeat-header command."
Assert-DocxXPath `
    -Document $tableRowPropertiesDocument `
    -NamespaceManager $tableRowPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:trPr/w:trHeight[@w:val='720' and @w:hRule='exact']" `
    -Message "Table-row-properties output should set the first row height."
Assert-DocxXPath `
    -Document $tableRowPropertiesDocument `
    -NamespaceManager $tableRowPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:trPr/w:cantSplit" `
    -Message "Table-row-properties output should set cantSplit on the first row."
Assert-DocxXPath `
    -Document $tableRowPropertiesDocument `
    -NamespaceManager $tableRowPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:trPr/w:tblHeader" `
    -Message "Table-row-properties output should set repeat-header on the first row."

$tableRowPropertiesClearPlanPath = Join-Path $resolvedWorkingDir "invoice.table_row_properties_clear_plan.json"
$tableRowPropertiesClearedDocx = Join-Path $resolvedWorkingDir "invoice.table_row_properties_cleared.docx"
$tableRowPropertiesClearSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_row_properties_cleared.summary.json"

Set-Content -LiteralPath $tableRowPropertiesClearPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_table_row_height",
      "table_index": 0,
      "row_index": 0
    },
    {
      "op": "clear_table_row_cant_split",
      "table_index": 0,
      "row_index": 0
    },
    {
      "op": "clear_table_row_repeat_header",
      "table_index": 0,
      "row_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $tableRowPropertiesDocx `
    -EditPlan $tableRowPropertiesClearPlanPath `
    -OutputDocx $tableRowPropertiesClearedDocx `
    -SummaryJson $tableRowPropertiesClearSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table row properties clear plan."
}

$tableRowPropertiesClearSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableRowPropertiesClearSummaryPath | ConvertFrom-Json
$tableRowPropertiesClearedXml = Read-DocxEntryText -DocxPath $tableRowPropertiesClearedDocx -EntryName "word/document.xml"
$tableRowPropertiesClearedDocument = New-Object System.Xml.XmlDocument
$tableRowPropertiesClearedDocument.PreserveWhitespace = $true
$tableRowPropertiesClearedDocument.LoadXml($tableRowPropertiesClearedXml)
$tableRowPropertiesClearedNamespaceManager = New-WordNamespaceManager -Document $tableRowPropertiesClearedDocument

Assert-Equal -Actual $tableRowPropertiesClearSummary.status -Expected "completed" `
    -Message "Table-row-properties-clear summary did not report status=completed."
Assert-Equal -Actual $tableRowPropertiesClearSummary.operation_count -Expected 3 `
    -Message "Table-row-properties-clear summary should record three operations."
Assert-Equal -Actual $tableRowPropertiesClearSummary.operations[0].command -Expected "clear-table-row-height" `
    -Message "Clear-table-row-height operation should use the CLI clear-table-row-height command."
Assert-Equal -Actual $tableRowPropertiesClearSummary.operations[1].command -Expected "clear-table-row-cant-split" `
    -Message "Clear-table-row-cant-split operation should use the CLI clear-table-row-cant-split command."
Assert-Equal -Actual $tableRowPropertiesClearSummary.operations[2].command -Expected "clear-table-row-repeat-header" `
    -Message "Clear-table-row-repeat-header operation should use the CLI clear-table-row-repeat-header command."

if ($null -ne $tableRowPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:trPr/w:trHeight", $tableRowPropertiesClearedNamespaceManager)) {
    throw "Table-row-properties clear output should remove the first row height."
}
if ($null -ne $tableRowPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:trPr/w:cantSplit", $tableRowPropertiesClearedNamespaceManager)) {
    throw "Table-row-properties clear output should remove cantSplit from the first row."
}
if ($null -ne $tableRowPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:trPr/w:tblHeader", $tableRowPropertiesClearedNamespaceManager)) {
    throw "Table-row-properties clear output should remove repeat-header from the first row."
}

$tableCellPropertiesPlanPath = Join-Path $resolvedWorkingDir "invoice.table_cell_properties_plan.json"
$tableCellPropertiesDocx = Join-Path $resolvedWorkingDir "invoice.table_cell_properties.docx"
$tableCellPropertiesSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_cell_properties.summary.json"

Set-Content -LiteralPath $tableCellPropertiesPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_table_cell_width",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "width_twips": 2400
    },
    {
      "op": "set_table_cell_margin",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "edge": "left",
      "margin_twips": 180
    },
    {
      "op": "set_table_cell_text_direction",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "direction": "top_to_bottom_right_to_left"
    },
    {
      "op": "set_table_cell_fill",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "color": "D9EAD3"
    },
    {
      "op": "set_table_cell_border",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "edge": "right",
      "style": "single",
      "size": 12,
      "color": "0088CC"
    },
    {
      "op": "set_table_cell_vertical_alignment",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "alignment": "bottom"
    },
    {
      "op": "set_table_cell_horizontal_alignment",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "alignment": "right"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $tableCellPropertiesPlanPath `
    -OutputDocx $tableCellPropertiesDocx `
    -SummaryJson $tableCellPropertiesSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table cell properties plan."
}

$tableCellPropertiesSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableCellPropertiesSummaryPath | ConvertFrom-Json
$tableCellPropertiesXml = Read-DocxEntryText -DocxPath $tableCellPropertiesDocx -EntryName "word/document.xml"
$tableCellPropertiesDocument = New-Object System.Xml.XmlDocument
$tableCellPropertiesDocument.PreserveWhitespace = $true
$tableCellPropertiesDocument.LoadXml($tableCellPropertiesXml)
$tableCellPropertiesNamespaceManager = New-WordNamespaceManager -Document $tableCellPropertiesDocument

Assert-Equal -Actual $tableCellPropertiesSummary.status -Expected "completed" `
    -Message "Table-cell-properties summary did not report status=completed."
Assert-Equal -Actual $tableCellPropertiesSummary.operation_count -Expected 7 `
    -Message "Table-cell-properties summary should record seven operations."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[0].command -Expected "set-table-cell-width" `
    -Message "Set-table-cell-width operation should use the CLI set-table-cell-width command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[1].command -Expected "set-table-cell-margin" `
    -Message "Set-table-cell-margin operation should use the CLI set-table-cell-margin command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[2].command -Expected "set-table-cell-text-direction" `
    -Message "Set-table-cell-text-direction operation should use the CLI set-table-cell-text-direction command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[3].command -Expected "set-table-cell-fill" `
    -Message "Set-table-cell-fill operation should use the CLI set-table-cell-fill command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[4].command -Expected "set-table-cell-border" `
    -Message "Set-table-cell-border operation should use the CLI set-table-cell-border command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[5].command -Expected "set-table-cell-vertical-alignment" `
    -Message "Set-table-cell-vertical-alignment operation should use the CLI set-table-cell-vertical-alignment command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[6].command -Expected "set-table-cell-horizontal-alignment" `
    -Message "Set-table-cell-horizontal-alignment operation should use the direct table-cell horizontal alignment command."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcW[@w:w='2400' and @w:type='dxa']" `
    -Message "Table-cell-properties output should set the first cell width."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcMar/w:left[@w:w='180' and @w:type='dxa']" `
    -Message "Table-cell-properties output should set the first cell left margin."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:textDirection[@w:val='tbRl']" `
    -Message "Table-cell-properties output should set the first cell text direction."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:shd[@w:fill='D9EAD3']" `
    -Message "Table-cell-properties output should set the first cell fill."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcBorders/w:right[@w:val='single' and @w:sz='12' and @w:color='0088CC']" `
    -Message "Table-cell-properties output should set the first cell right border."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:vAlign[@w:val='bottom']" `
    -Message "Table-cell-properties output should set the first cell vertical alignment."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:p[1]/w:pPr/w:jc[@w:val='right']" `
    -Message "Table-cell-properties output should set the first cell horizontal alignment."

$tableCellPropertiesClearPlanPath = Join-Path $resolvedWorkingDir "invoice.table_cell_properties_clear_plan.json"
$tableCellPropertiesClearedDocx = Join-Path $resolvedWorkingDir "invoice.table_cell_properties_cleared.docx"
$tableCellPropertiesClearSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_cell_properties_cleared.summary.json"

Set-Content -LiteralPath $tableCellPropertiesClearPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_table_cell_width",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    },
    {
      "op": "clear_table_cell_margin",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "edge": "left"
    },
    {
      "op": "clear_table_cell_text_direction",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    },
    {
      "op": "clear_table_cell_fill",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    },
    {
      "op": "clear_table_cell_border",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "edge": "right"
    },
    {
      "op": "clear_table_cell_vertical_alignment",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    },
    {
      "op": "clear_table_cell_horizontal_alignment",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $tableCellPropertiesDocx `
    -EditPlan $tableCellPropertiesClearPlanPath `
    -OutputDocx $tableCellPropertiesClearedDocx `
    -SummaryJson $tableCellPropertiesClearSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table cell properties clear plan."
}

$tableCellPropertiesClearSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableCellPropertiesClearSummaryPath | ConvertFrom-Json
$tableCellPropertiesClearedXml = Read-DocxEntryText -DocxPath $tableCellPropertiesClearedDocx -EntryName "word/document.xml"
$tableCellPropertiesClearedDocument = New-Object System.Xml.XmlDocument
$tableCellPropertiesClearedDocument.PreserveWhitespace = $true
$tableCellPropertiesClearedDocument.LoadXml($tableCellPropertiesClearedXml)
$tableCellPropertiesClearedNamespaceManager = New-WordNamespaceManager -Document $tableCellPropertiesClearedDocument

Assert-Equal -Actual $tableCellPropertiesClearSummary.status -Expected "completed" `
    -Message "Table-cell-properties-clear summary did not report status=completed."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operation_count -Expected 7 `
    -Message "Table-cell-properties-clear summary should record seven operations."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[0].command -Expected "clear-table-cell-width" `
    -Message "Clear-table-cell-width operation should use the CLI clear-table-cell-width command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[1].command -Expected "clear-table-cell-margin" `
    -Message "Clear-table-cell-margin operation should use the CLI clear-table-cell-margin command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[2].command -Expected "clear-table-cell-text-direction" `
    -Message "Clear-table-cell-text-direction operation should use the CLI clear-table-cell-text-direction command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[3].command -Expected "clear-table-cell-fill" `
    -Message "Clear-table-cell-fill operation should use the CLI clear-table-cell-fill command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[4].command -Expected "clear-table-cell-border" `
    -Message "Clear-table-cell-border operation should use the CLI clear-table-cell-border command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[5].command -Expected "clear-table-cell-vertical-alignment" `
    -Message "Clear-table-cell-vertical-alignment operation should use the CLI clear-table-cell-vertical-alignment command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[6].command -Expected "clear-table-cell-horizontal-alignment" `
    -Message "Clear-table-cell-horizontal-alignment operation should use the direct table-cell horizontal alignment command."

if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcW", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell width."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcMar/w:left", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell left margin."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:textDirection", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell text direction."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:shd", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell fill."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcBorders/w:right", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell right border."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:vAlign", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell vertical alignment."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:p[1]/w:pPr/w:jc", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell horizontal alignment."
}
