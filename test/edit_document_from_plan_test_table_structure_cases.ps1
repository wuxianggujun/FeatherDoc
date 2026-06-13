$tableRowsPlanPath = Join-Path $resolvedWorkingDir "invoice.table_rows_plan.json"
$tableRowsEditedDocx = Join-Path $resolvedWorkingDir "invoice.table_rows_edited.docx"
$tableRowsSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_rows.summary.json"

Set-Content -LiteralPath $tableRowsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_table_row",
      "table_index": 0,
      "cell_count": 2
    },
    {
      "op": "insert_table_row_before",
      "table_index": 0,
      "row_index": 1
    },
    {
      "op": "insert_table_row_after",
      "table_index": 0,
      "row_index": 1
    },
    {
      "op": "remove_table_row",
      "table_index": 0,
      "row_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $tableRowsPlanPath `
    -OutputDocx $tableRowsEditedDocx `
    -SummaryJson $tableRowsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table row edit plan."
}

$tableRowsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableRowsSummaryPath | ConvertFrom-Json
$tableRowsEditedXml = Read-DocxEntryText -DocxPath $tableRowsEditedDocx -EntryName "word/document.xml"
$tableRowsEditedDocument = New-Object System.Xml.XmlDocument
$tableRowsEditedDocument.PreserveWhitespace = $true
$tableRowsEditedDocument.LoadXml($tableRowsEditedXml)
$tableRowsEditedNamespaceManager = New-WordNamespaceManager -Document $tableRowsEditedDocument

Assert-Equal -Actual $tableRowsSummary.status -Expected "completed" `
    -Message "Table-row summary did not report status=completed."
Assert-Equal -Actual $tableRowsSummary.operation_count -Expected 4 `
    -Message "Table-row summary should record four operations."
Assert-Equal -Actual $tableRowsSummary.operations[0].op -Expected "append_table_row" `
    -Message "Append-table-row operation should be recorded."
Assert-Equal -Actual $tableRowsSummary.operations[0].command -Expected "append-table-row" `
    -Message "Append-table-row operation should use the CLI append-table-row command."
Assert-Equal -Actual $tableRowsSummary.operations[1].command -Expected "insert-table-row-before" `
    -Message "Insert-table-row-before operation should use the CLI insert-table-row-before command."
Assert-Equal -Actual $tableRowsSummary.operations[2].command -Expected "insert-table-row-after" `
    -Message "Insert-table-row-after operation should use the CLI insert-table-row-after command."
Assert-Equal -Actual $tableRowsSummary.operations[3].command -Expected "remove-table-row" `
    -Message "Remove-table-row operation should use the CLI remove-table-row command."
Assert-Equal -Actual $tableRowsEditedDocument.SelectNodes("//w:tbl[1]/w:tr", $tableRowsEditedNamespaceManager).Count -Expected 5 `
    -Message "Table-row edit output should contain the expected number of rows."

$templateTableRowsPlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_rows_plan.json"
$templateTableRowsEditedDocx = Join-Path $resolvedWorkingDir "invoice.template_table_rows_edited.docx"
$templateTableRowsSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_rows.summary.json"

Set-Content -LiteralPath $templateTableRowsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_template_table_row",
      "table_index": 0,
      "cell_count": 2
    },
    {
      "op": "insert_template_table_row_before",
      "table_index": 0,
      "row_index": 1
    },
    {
      "op": "insert_template_table_row_after",
      "table_index": 0,
      "row_index": 1
    },
    {
      "op": "remove_template_table_row",
      "table_index": 0,
      "row_index": 0
    },
    {
      "op": "delete_template_table_row",
      "table_index": 0,
      "row_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableRowsPlanPath `
    -OutputDocx $templateTableRowsEditedDocx `
    -SummaryJson $templateTableRowsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table row edit plan."
}

$templateTableRowsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableRowsSummaryPath | ConvertFrom-Json
$templateTableRowsEditedXml = Read-DocxEntryText -DocxPath $templateTableRowsEditedDocx -EntryName "word/document.xml"
$templateTableRowsEditedDocument = New-Object System.Xml.XmlDocument
$templateTableRowsEditedDocument.PreserveWhitespace = $true
$templateTableRowsEditedDocument.LoadXml($templateTableRowsEditedXml)
$templateTableRowsEditedNamespaceManager = New-WordNamespaceManager -Document $templateTableRowsEditedDocument

Assert-Equal -Actual $templateTableRowsSummary.status -Expected "completed" `
    -Message "Template-table-row summary did not report status=completed."
Assert-Equal -Actual $templateTableRowsSummary.operation_count -Expected 5 `
    -Message "Template-table-row summary should record five operations."
Assert-Equal -Actual $templateTableRowsSummary.operations[0].op -Expected "append_template_table_row" `
    -Message "Append-template-table-row operation should be recorded."
Assert-Equal -Actual $templateTableRowsSummary.operations[0].command -Expected "append-template-table-row" `
    -Message "Append-template-table-row operation should use the CLI append-template-table-row command."
Assert-Equal -Actual $templateTableRowsSummary.operations[1].command -Expected "insert-template-table-row-before" `
    -Message "Insert-template-table-row-before operation should use the CLI template row command."
Assert-Equal -Actual $templateTableRowsSummary.operations[2].command -Expected "insert-template-table-row-after" `
    -Message "Insert-template-table-row-after operation should use the CLI template row command."
Assert-Equal -Actual $templateTableRowsSummary.operations[3].command -Expected "remove-template-table-row" `
    -Message "Remove-template-table-row operation should use the CLI template row command."
Assert-Equal -Actual $templateTableRowsSummary.operations[4].command -Expected "remove-template-table-row" `
    -Message "Delete-template-table-row alias should use the CLI remove-template-table-row command."
Assert-Equal -Actual $templateTableRowsEditedDocument.SelectNodes("//w:tbl[1]/w:tr", $templateTableRowsEditedNamespaceManager).Count -Expected 4 `
    -Message "Template-table-row edit output should contain the expected number of rows."

$templateTableColumnsPlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_columns_plan.json"
$templateTableColumnsEditedDocx = Join-Path $resolvedWorkingDir "invoice.template_table_columns_edited.docx"
$templateTableColumnsSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_columns.summary.json"

Set-Content -LiteralPath $templateTableColumnsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_template_table_column_before",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 1
    },
    {
      "op": "insert_template_table_column_after",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 1
    },
    {
      "op": "remove_template_table_column",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    },
    {
      "op": "delete_template_table_column",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableColumnsPlanPath `
    -OutputDocx $templateTableColumnsEditedDocx `
    -SummaryJson $templateTableColumnsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table column edit plan."
}

$templateTableColumnsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableColumnsSummaryPath | ConvertFrom-Json
$templateTableColumnsEditedXml = Read-DocxEntryText -DocxPath $templateTableColumnsEditedDocx -EntryName "word/document.xml"
$templateTableColumnsEditedDocument = New-Object System.Xml.XmlDocument
$templateTableColumnsEditedDocument.PreserveWhitespace = $true
$templateTableColumnsEditedDocument.LoadXml($templateTableColumnsEditedXml)
$templateTableColumnsEditedNamespaceManager = New-WordNamespaceManager -Document $templateTableColumnsEditedDocument

Assert-Equal -Actual $templateTableColumnsSummary.status -Expected "completed" `
    -Message "Template-table-column summary did not report status=completed."
Assert-Equal -Actual $templateTableColumnsSummary.operation_count -Expected 4 `
    -Message "Template-table-column summary should record four operations."
Assert-Equal -Actual $templateTableColumnsSummary.operations[0].op -Expected "insert_template_table_column_before" `
    -Message "Insert-template-table-column-before operation should be recorded."
Assert-Equal -Actual $templateTableColumnsSummary.operations[0].command -Expected "insert-template-table-column-before" `
    -Message "Insert-template-table-column-before operation should use the CLI template column command."
Assert-Equal -Actual $templateTableColumnsSummary.operations[1].command -Expected "insert-template-table-column-after" `
    -Message "Insert-template-table-column-after operation should use the CLI template column command."
Assert-Equal -Actual $templateTableColumnsSummary.operations[2].command -Expected "remove-template-table-column" `
    -Message "Remove-template-table-column operation should use the CLI template column command."
Assert-Equal -Actual $templateTableColumnsSummary.operations[3].command -Expected "remove-template-table-column" `
    -Message "Delete-template-table-column alias should use the CLI remove-template-table-column command."
Assert-Equal -Actual $templateTableColumnsEditedDocument.SelectNodes("//w:tbl[1]/w:tblGrid/w:gridCol", $templateTableColumnsEditedNamespaceManager).Count -Expected 3 `
    -Message "Template-table-column edit output should contain the expected grid column count."
Assert-Equal -Actual $templateTableColumnsEditedDocument.SelectNodes("//w:tbl[1]/w:tr[1]/w:tc", $templateTableColumnsEditedNamespaceManager).Count -Expected 3 `
    -Message "Template-table-column edit output should contain the expected first-row cell count."

$templateTableCellTextPlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_cell_text_plan.json"
$templateTableCellTextDocx = Join-Path $resolvedWorkingDir "invoice.template_table_cell_text.docx"
$templateTableCellTextSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_cell_text.summary.json"

Set-Content -LiteralPath $templateTableCellTextPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_template_table_cell_text",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "text": "Template cell text from plan"
    },
    {
      "op": "set_template_table_cell_text",
      "table_index": 0,
      "row_index": 1,
      "grid_column": 1,
      "text": "Template grid-column text"
    },
    {
      "op": "set_template_table_cell_block_texts",
      "table_index": 0,
      "row_index": 1,
      "cell_index": 0,
      "rows": [
        ["Template block A", "Template block B"],
        ["Template block C", "Template block D"]
      ]
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableCellTextPlanPath `
    -OutputDocx $templateTableCellTextDocx `
    -SummaryJson $templateTableCellTextSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table cell text plan."
}

$templateTableCellTextSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableCellTextSummaryPath | ConvertFrom-Json
$templateTableCellTextXml = Read-DocxEntryText -DocxPath $templateTableCellTextDocx -EntryName "word/document.xml"

Assert-Equal -Actual $templateTableCellTextSummary.status -Expected "completed" `
    -Message "Template-table-cell-text summary did not report status=completed."
Assert-Equal -Actual $templateTableCellTextSummary.operation_count -Expected 3 `
    -Message "Template-table-cell-text summary should record three operations."
Assert-Equal -Actual $templateTableCellTextSummary.operations[0].command -Expected "set-template-table-cell-text" `
    -Message "Set-template-table-cell-text operation should use the CLI template cell text command."
Assert-Equal -Actual $templateTableCellTextSummary.operations[1].command -Expected "set-template-table-cell-text" `
    -Message "Set-template-table-cell-text grid-column operation should use the CLI template cell text command."
Assert-Equal -Actual $templateTableCellTextSummary.operations[2].command -Expected "set-template-table-cell-block-texts" `
    -Message "Set-template-table-cell-block-texts operation should use the CLI template cell block command."
Assert-ContainsText -Text $templateTableCellTextXml -ExpectedText "Template cell text from plan" -Label "Template table cell text output"
Assert-ContainsText -Text $templateTableCellTextXml -ExpectedText "Template block A" -Label "Template table cell text output"
Assert-ContainsText -Text $templateTableCellTextXml -ExpectedText "Template block B" -Label "Template table cell text output"
Assert-ContainsText -Text $templateTableCellTextXml -ExpectedText "Template block C" -Label "Template table cell text output"
Assert-ContainsText -Text $templateTableCellTextXml -ExpectedText "Template block D" -Label "Template table cell text output"

$templateTableRowTextsPlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_row_texts_plan.json"
$templateTableRowTextsDocx = Join-Path $resolvedWorkingDir "invoice.template_table_row_texts.docx"
$templateTableRowTextsSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_row_texts.summary.json"

Set-Content -LiteralPath $templateTableRowTextsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_template_table_row_texts",
      "table_index": 0,
      "row_index": 0,
      "rows": [
        ["Template row A", "Template row B", "Template row C"]
      ]
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableRowTextsPlanPath `
    -OutputDocx $templateTableRowTextsDocx `
    -SummaryJson $templateTableRowTextsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table row texts plan."
}

$templateTableRowTextsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableRowTextsSummaryPath | ConvertFrom-Json
$templateTableRowTextsXml = Read-DocxEntryText -DocxPath $templateTableRowTextsDocx -EntryName "word/document.xml"

Assert-Equal -Actual $templateTableRowTextsSummary.status -Expected "completed" `
    -Message "Template-table-row-texts summary did not report status=completed."
Assert-Equal -Actual $templateTableRowTextsSummary.operation_count -Expected 1 `
    -Message "Template-table-row-texts summary should record one operation."
Assert-Equal -Actual $templateTableRowTextsSummary.operations[0].op -Expected "set_template_table_row_texts" `
    -Message "Set-template-table-row-texts operation should be recorded."
Assert-Equal -Actual $templateTableRowTextsSummary.operations[0].command -Expected "set-template-table-row-texts" `
    -Message "Set-template-table-row-texts operation should use the CLI template row texts command."
Assert-ContainsText -Text $templateTableRowTextsXml -ExpectedText "Template row A" -Label "Template table row texts output"
Assert-ContainsText -Text $templateTableRowTextsXml -ExpectedText "Template row B" -Label "Template table row texts output"
Assert-ContainsText -Text $templateTableRowTextsXml -ExpectedText "Template row C" -Label "Template table row texts output"

$templateTableJsonPatchPlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_json_patch_plan.json"
$templateTableJsonPatchDocx = Join-Path $resolvedWorkingDir "invoice.template_table_json_patch.docx"
$templateTableJsonPatchSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_json_patch.summary.json"

Set-Content -LiteralPath $templateTableJsonPatchPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_template_table_from_json",
      "table_index": 0,
      "patch": {
        "mode": "rows",
        "start_row": 0,
        "rows": [
          ["Template JSON A", "Template JSON B", "Template JSON C"]
        ]
      }
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableJsonPatchPlanPath `
    -OutputDocx $templateTableJsonPatchDocx `
    -SummaryJson $templateTableJsonPatchSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table JSON patch plan."
}

$templateTableJsonPatchSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableJsonPatchSummaryPath | ConvertFrom-Json
$templateTableJsonPatchXml = Read-DocxEntryText -DocxPath $templateTableJsonPatchDocx -EntryName "word/document.xml"

Assert-Equal -Actual $templateTableJsonPatchSummary.status -Expected "completed" `
    -Message "Template-table-json-patch summary did not report status=completed."
Assert-Equal -Actual $templateTableJsonPatchSummary.operation_count -Expected 1 `
    -Message "Template-table-json-patch summary should record one operation."
Assert-Equal -Actual $templateTableJsonPatchSummary.operations[0].command -Expected "set-template-table-from-json" `
    -Message "Set-template-table-from-json operation should use the CLI JSON patch command."
Assert-ContainsText -Text $templateTableJsonPatchXml -ExpectedText "Template JSON A" -Label "Template table JSON patch output"
Assert-ContainsText -Text $templateTableJsonPatchXml -ExpectedText "Template JSON B" -Label "Template table JSON patch output"
Assert-ContainsText -Text $templateTableJsonPatchXml -ExpectedText "Template JSON C" -Label "Template table JSON patch output"

$templateTablesJsonPatchPlanPath = Join-Path $resolvedWorkingDir "invoice.template_tables_json_patch_plan.json"
$templateTablesJsonPatchDocx = Join-Path $resolvedWorkingDir "invoice.template_tables_json_patch.docx"
$templateTablesJsonPatchSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_tables_json_patch.summary.json"

Set-Content -LiteralPath $templateTablesJsonPatchPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_template_tables_from_json",
      "patch": {
        "operations": [
          {
            "table_index": 0,
            "mode": "rows",
            "start_row": 0,
            "rows": [
              ["Template batch A", "Template batch B", "Template batch C"]
            ]
          },
          {
            "table_index": 0,
            "mode": "block",
            "start_row": 1,
            "start_cell": 0,
            "rows": [
              ["Template batch block A"]
            ]
          }
        ]
      }
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTablesJsonPatchPlanPath `
    -OutputDocx $templateTablesJsonPatchDocx `
    -SummaryJson $templateTablesJsonPatchSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template tables JSON patch plan."
}

$templateTablesJsonPatchSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTablesJsonPatchSummaryPath | ConvertFrom-Json
$templateTablesJsonPatchXml = Read-DocxEntryText -DocxPath $templateTablesJsonPatchDocx -EntryName "word/document.xml"

Assert-Equal -Actual $templateTablesJsonPatchSummary.status -Expected "completed" `
    -Message "Template-tables-json-patch summary did not report status=completed."
Assert-Equal -Actual $templateTablesJsonPatchSummary.operation_count -Expected 1 `
    -Message "Template-tables-json-patch summary should record one operation."
Assert-Equal -Actual $templateTablesJsonPatchSummary.operations[0].command -Expected "set-template-tables-from-json" `
    -Message "Set-template-tables-from-json operation should use the CLI batch JSON patch command."
Assert-ContainsText -Text $templateTablesJsonPatchXml -ExpectedText "Template batch A" -Label "Template tables JSON patch output"
Assert-ContainsText -Text $templateTablesJsonPatchXml -ExpectedText "Template batch B" -Label "Template tables JSON patch output"
Assert-ContainsText -Text $templateTablesJsonPatchXml -ExpectedText "Template batch C" -Label "Template tables JSON patch output"
Assert-ContainsText -Text $templateTablesJsonPatchXml -ExpectedText "Template batch block A" -Label "Template tables JSON patch output"

$templateTableMergePlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_merge_plan.json"
$templateTableMergeDocx = Join-Path $resolvedWorkingDir "invoice.template_table_merge.docx"
$templateTableMergeSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_merge.summary.json"

Set-Content -LiteralPath $templateTableMergePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "merge_template_table_cells",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "direction": "right",
      "count": 2
    },
    {
      "op": "unmerge_template_table_cells",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "direction": "right"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableMergePlanPath `
    -OutputDocx $templateTableMergeDocx `
    -SummaryJson $templateTableMergeSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table merge plan."
}

$templateTableMergeSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableMergeSummaryPath | ConvertFrom-Json
$templateTableMergeXml = Read-DocxEntryText -DocxPath $templateTableMergeDocx -EntryName "word/document.xml"
$templateTableMergeDocument = New-Object System.Xml.XmlDocument
$templateTableMergeDocument.PreserveWhitespace = $true
$templateTableMergeDocument.LoadXml($templateTableMergeXml)
$templateTableMergeNamespaceManager = New-WordNamespaceManager -Document $templateTableMergeDocument

Assert-Equal -Actual $templateTableMergeSummary.status -Expected "completed" `
    -Message "Template-table-merge summary did not report status=completed."
Assert-Equal -Actual $templateTableMergeSummary.operation_count -Expected 2 `
    -Message "Template-table-merge summary should record two operations."
Assert-Equal -Actual $templateTableMergeSummary.operations[0].command -Expected "merge-template-table-cells" `
    -Message "Merge-template-table-cells operation should use the CLI template merge command."
Assert-Equal -Actual $templateTableMergeSummary.operations[1].command -Expected "unmerge-template-table-cells" `
    -Message "Unmerge-template-table-cells operation should use the CLI template unmerge command."
if ($null -ne $templateTableMergeDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:gridSpan", $templateTableMergeNamespaceManager)) {
    throw "Template-table merge/unmerge output should remove the first cell gridSpan."
}
Assert-Equal -Actual $templateTableMergeDocument.SelectNodes("//w:tbl[1]/w:tr[1]/w:tc", $templateTableMergeNamespaceManager).Count -Expected 3 `
    -Message "Template-table merge/unmerge output should preserve the first-row cell count."

$tableColumnsPlanPath = Join-Path $resolvedWorkingDir "invoice.table_columns_plan.json"
$tableColumnsEditedDocx = Join-Path $resolvedWorkingDir "invoice.table_columns_edited.docx"
$tableColumnsSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_columns.summary.json"

Set-Content -LiteralPath $tableColumnsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_table_column_before",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 1
    },
    {
      "op": "insert_table_column_after",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 1
    },
    {
      "op": "remove_table_column",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $tableColumnsPlanPath `
    -OutputDocx $tableColumnsEditedDocx `
    -SummaryJson $tableColumnsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table column edit plan."
}

$tableColumnsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableColumnsSummaryPath | ConvertFrom-Json
$tableColumnsEditedXml = Read-DocxEntryText -DocxPath $tableColumnsEditedDocx -EntryName "word/document.xml"
$tableColumnsEditedDocument = New-Object System.Xml.XmlDocument
$tableColumnsEditedDocument.PreserveWhitespace = $true
$tableColumnsEditedDocument.LoadXml($tableColumnsEditedXml)
$tableColumnsEditedNamespaceManager = New-WordNamespaceManager -Document $tableColumnsEditedDocument

Assert-Equal -Actual $tableColumnsSummary.status -Expected "completed" `
    -Message "Table-column summary did not report status=completed."
Assert-Equal -Actual $tableColumnsSummary.operation_count -Expected 3 `
    -Message "Table-column summary should record three operations."
Assert-Equal -Actual $tableColumnsSummary.operations[0].op -Expected "insert_table_column_before" `
    -Message "Insert-table-column-before operation should be recorded."
Assert-Equal -Actual $tableColumnsSummary.operations[0].command -Expected "insert-table-column-before" `
    -Message "Insert-table-column-before operation should use the CLI insert-table-column-before command."
Assert-Equal -Actual $tableColumnsSummary.operations[1].command -Expected "insert-table-column-after" `
    -Message "Insert-table-column-after operation should use the CLI insert-table-column-after command."
Assert-Equal -Actual $tableColumnsSummary.operations[2].command -Expected "remove-table-column" `
    -Message "Remove-table-column operation should use the CLI remove-table-column command."
Assert-Equal -Actual $tableColumnsEditedDocument.SelectNodes("//w:tbl[1]/w:tblGrid/w:gridCol", $tableColumnsEditedNamespaceManager).Count -Expected 4 `
    -Message "Table-column edit output should contain the expected grid column count."
Assert-Equal -Actual $tableColumnsEditedDocument.SelectNodes("//w:tbl[1]/w:tr[1]/w:tc", $tableColumnsEditedNamespaceManager).Count -Expected 4 `
    -Message "Table-column edit output should contain the expected first-row cell count."
