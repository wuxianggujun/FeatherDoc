
$styleRefactorSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleRefactorSummaryPath | ConvertFrom-Json
$styleRefactorDocumentXml = Read-DocxEntryText -DocxPath $styleRefactorEditedDocx -EntryName "word/document.xml"
$styleRefactorStylesXml = Read-DocxEntryText -DocxPath $styleRefactorEditedDocx -EntryName "word/styles.xml"

Assert-Equal -Actual $styleRefactorSummary.status -Expected "completed" `
    -Message "Style-refactor summary did not report status=completed."
Assert-Equal -Actual $styleRefactorSummary.operation_count -Expected 4 `
    -Message "Style-refactor summary should record four operations."
Assert-Equal -Actual $styleRefactorSummary.operations[0].command -Expected "rename-style" `
    -Message "rename_style should use the CLI rename-style command."
Assert-Equal -Actual $styleRefactorSummary.operations[1].command -Expected "merge-style" `
    -Message "merge_style should use the CLI merge-style command."
Assert-Equal -Actual $styleRefactorSummary.operations[2].command -Expected "apply-style-refactor" `
    -Message "apply_style_refactor should use the CLI apply-style-refactor command."
Assert-Equal -Actual $styleRefactorSummary.operations[3].command -Expected "prune-unused-styles" `
    -Message "prune_unused_styles should use the CLI prune-unused-styles command."
Assert-ContainsText -Text $styleRefactorDocumentXml -ExpectedText 'w:pStyle w:val="ReviewBody"' -Label "Style-refactor document.xml"
Assert-ContainsText -Text $styleRefactorDocumentXml -ExpectedText 'w:pStyle w:val="MergeTarget"' -Label "Style-refactor document.xml"
Assert-ContainsText -Text $styleRefactorDocumentXml -ExpectedText 'w:pStyle w:val="BulkNew"' -Label "Style-refactor document.xml"
Assert-ContainsText -Text $styleRefactorDocumentXml -ExpectedText 'w:pStyle w:val="BulkMergeTarget"' -Label "Style-refactor document.xml"
Assert-ContainsText -Text $styleRefactorStylesXml -ExpectedText 'w:styleId="ReviewBody"' -Label "Style-refactor styles.xml"
Assert-ContainsText -Text $styleRefactorStylesXml -ExpectedText 'w:styleId="MergeTarget"' -Label "Style-refactor styles.xml"
Assert-ContainsText -Text $styleRefactorStylesXml -ExpectedText 'w:styleId="BulkNew"' -Label "Style-refactor styles.xml"
Assert-ContainsText -Text $styleRefactorStylesXml -ExpectedText 'w:styleId="BulkMergeTarget"' -Label "Style-refactor styles.xml"
Assert-NotContainsText -Text $styleRefactorStylesXml -UnexpectedText 'w:styleId="UnusedBody"' -Label "Style-refactor styles.xml"
Assert-NotContainsText -Text $styleRefactorStylesXml -UnexpectedText 'w:styleId="UnusedCharacter"' -Label "Style-refactor styles.xml"
Assert-True -Condition (Test-Path -LiteralPath $styleRefactorRollbackPath) `
    -Message "apply_style_refactor should write the requested rollback plan."

Set-Content -LiteralPath $styleRefactorRestorePlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "restore_style_merge",
      "rollback_plan": "$($styleRefactorRollbackPath.Replace('\', '\\'))",
      "entries": [1]
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $styleRefactorEditedDocx `
    -EditPlan $styleRefactorRestorePlanPath `
    -OutputDocx $styleRefactorRestoredDocx `
    -SummaryJson $styleRefactorRestoreSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the restore style merge edit plan."
}

$styleRefactorRestoreSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleRefactorRestoreSummaryPath | ConvertFrom-Json
$styleRefactorRestoredDocumentXml = Read-DocxEntryText -DocxPath $styleRefactorRestoredDocx -EntryName "word/document.xml"
$styleRefactorRestoredStylesXml = Read-DocxEntryText -DocxPath $styleRefactorRestoredDocx -EntryName "word/styles.xml"

Assert-Equal -Actual $styleRefactorRestoreSummary.status -Expected "completed" `
    -Message "Style-refactor restore summary did not report status=completed."
Assert-Equal -Actual $styleRefactorRestoreSummary.operation_count -Expected 1 `
    -Message "Style-refactor restore summary should record one operation."
Assert-Equal -Actual $styleRefactorRestoreSummary.operations[0].command -Expected "restore-style-merge" `
    -Message "restore_style_merge should use the CLI restore-style-merge command."
Assert-ContainsText -Text $styleRefactorRestoredDocumentXml -ExpectedText 'w:pStyle w:val="BulkMergeSource"' -Label "Style-refactor restored document.xml"
Assert-ContainsText -Text $styleRefactorRestoredStylesXml -ExpectedText 'w:styleId="BulkMergeSource"' -Label "Style-refactor restored styles.xml"

$tableStyleQualitySourceDocx = Join-Path $resolvedWorkingDir "table_style_quality.source.docx"
$tableStyleQualityPlanPath = Join-Path $resolvedWorkingDir "table_style_quality.edit_plan.json"
$tableStyleQualityEditedDocx = Join-Path $resolvedWorkingDir "table_style_quality.edited.docx"
$tableStyleQualitySummaryPath = Join-Path $resolvedWorkingDir "table_style_quality.edit.summary.json"

New-TableStyleRepairFixtureDocx -Path $tableStyleQualitySourceDocx
Set-Content -LiteralPath $tableStyleQualityPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "apply_table_style_quality_fixes"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $tableStyleQualitySourceDocx `
    -EditPlan $tableStyleQualityPlanPath `
    -OutputDocx $tableStyleQualityEditedDocx `
    -SummaryJson $tableStyleQualitySummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table style quality edit plan."
}

$tableStyleQualitySummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableStyleQualitySummaryPath | ConvertFrom-Json
$tableStyleQualityDocumentXml = Read-DocxEntryText -DocxPath $tableStyleQualityEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $tableStyleQualitySummary.status -Expected "completed" `
    -Message "Table-style-quality summary did not report status=completed."
Assert-Equal -Actual $tableStyleQualitySummary.operations[0].command -Expected "apply-table-style-quality-fixes" `
    -Message "apply_table_style_quality_fixes should use the CLI apply-table-style-quality-fixes command."
Assert-ContainsText -Text $tableStyleQualityDocumentXml -ExpectedText 'w:firstRow="1"' -Label "Table-style-quality document.xml"

$tableStyleLookSourceDocx = Join-Path $resolvedWorkingDir "table_style_look.source.docx"
$tableStyleLookPlanPath = Join-Path $resolvedWorkingDir "table_style_look.edit_plan.json"
$tableStyleLookEditedDocx = Join-Path $resolvedWorkingDir "table_style_look.edited.docx"
$tableStyleLookSummaryPath = Join-Path $resolvedWorkingDir "table_style_look.edit.summary.json"

New-TableStyleRepairFixtureDocx -Path $tableStyleLookSourceDocx
Set-Content -LiteralPath $tableStyleLookPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "repair_table_style_look"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $tableStyleLookSourceDocx `
    -EditPlan $tableStyleLookPlanPath `
    -OutputDocx $tableStyleLookEditedDocx `
    -SummaryJson $tableStyleLookSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table style look edit plan."
}

$tableStyleLookSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableStyleLookSummaryPath | ConvertFrom-Json
$tableStyleLookDocumentXml = Read-DocxEntryText -DocxPath $tableStyleLookEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $tableStyleLookSummary.status -Expected "completed" `
    -Message "Table-style-look summary did not report status=completed."
Assert-Equal -Actual $tableStyleLookSummary.operations[0].command -Expected "repair-table-style-look" `
    -Message "repair_table_style_look should use the CLI repair-table-style-look command."
Assert-ContainsText -Text $tableStyleLookDocumentXml -ExpectedText 'w:firstRow="1"' -Label "Table-style-look document.xml"

$styleNumberingRepairSourceDocx = Join-Path $resolvedWorkingDir "style_numbering_repair.source.docx"
$styleNumberingRepairPlanPath = Join-Path $resolvedWorkingDir "style_numbering_repair.edit_plan.json"
$styleNumberingRepairEditedDocx = Join-Path $resolvedWorkingDir "style_numbering_repair.edited.docx"
$styleNumberingRepairSummaryPath = Join-Path $resolvedWorkingDir "style_numbering_repair.edit.summary.json"

New-StyleNumberingRepairFixtureDocx -Path $styleNumberingRepairSourceDocx
Set-Content -LiteralPath $styleNumberingRepairPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "repair_style_numbering"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleNumberingRepairSourceDocx `
    -EditPlan $styleNumberingRepairPlanPath `
    -OutputDocx $styleNumberingRepairEditedDocx `
    -SummaryJson $styleNumberingRepairSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the style numbering repair edit plan."
}

$styleNumberingRepairSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleNumberingRepairSummaryPath | ConvertFrom-Json
$styleNumberingRepairStylesXml = Read-DocxEntryText -DocxPath $styleNumberingRepairEditedDocx -EntryName "word/styles.xml"

Assert-Equal -Actual $styleNumberingRepairSummary.status -Expected "completed" `
    -Message "Style-numbering-repair summary did not report status=completed."
Assert-Equal -Actual $styleNumberingRepairSummary.operations[0].command -Expected "repair-style-numbering" `
    -Message "repair_style_numbering should use the CLI repair-style-numbering command."
Assert-NotContainsText -Text $styleNumberingRepairStylesXml -UnexpectedText 'w:numId w:val="777"' -Label "Style-numbering-repair styles.xml"

$removeTablePlanPath = Join-Path $resolvedWorkingDir "invoice.remove_table_plan.json"
$tableRemovedDocx = Join-Path $resolvedWorkingDir "invoice.table_removed.docx"
$removeTableSummaryPath = Join-Path $resolvedWorkingDir "invoice.remove_table.summary.json"

Set-Content -LiteralPath $removeTablePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "remove_table",
      "table_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $removeTablePlanPath `
    -OutputDocx $tableRemovedDocx `
    -SummaryJson $removeTableSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the remove_table plan."
}

$removeTableSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $removeTableSummaryPath | ConvertFrom-Json
$removedTableXml = Read-DocxEntryText -DocxPath $tableRemovedDocx -EntryName "word/document.xml"
$removedTableDocument = New-Object System.Xml.XmlDocument
$removedTableDocument.PreserveWhitespace = $true
$removedTableDocument.LoadXml($removedTableXml)
$removedTableNamespaceManager = New-WordNamespaceManager -Document $removedTableDocument

Assert-Equal -Actual $removeTableSummary.status -Expected "completed" `
    -Message "Remove-table summary did not report status=completed."
Assert-Equal -Actual $removeTableSummary.operation_count -Expected 1 `
    -Message "Remove-table summary should record one operation."
Assert-Equal -Actual $removeTableSummary.operations[0].op -Expected "remove_table" `
    -Message "Remove-table operation should be recorded."
Assert-Equal -Actual $removeTableSummary.operations[0].command -Expected "remove-table" `
    -Message "Remove-table operation should use the CLI remove-table command."
Assert-True -Condition ($null -eq $removedTableDocument.SelectSingleNode("//w:tbl", $removedTableNamespaceManager)) `
    -Message "Remove-table output should not contain a body table."

$insertTablePlanPath = Join-Path $resolvedWorkingDir "invoice.insert_table_plan.json"
$tableInsertedDocx = Join-Path $resolvedWorkingDir "invoice.table_inserted.docx"
$insertTableSummaryPath = Join-Path $resolvedWorkingDir "invoice.insert_table.summary.json"

Set-Content -LiteralPath $insertTablePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_table_after",
      "table_index": 0,
      "row_count": 1,
      "column_count": 2
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $insertTablePlanPath `
    -OutputDocx $tableInsertedDocx `
    -SummaryJson $insertTableSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the insert_table_after plan."
}

$insertTableSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $insertTableSummaryPath | ConvertFrom-Json
$insertedTableXml = Read-DocxEntryText -DocxPath $tableInsertedDocx -EntryName "word/document.xml"
$insertedTableDocument = New-Object System.Xml.XmlDocument
$insertedTableDocument.PreserveWhitespace = $true
$insertedTableDocument.LoadXml($insertedTableXml)
$insertedTableNamespaceManager = New-WordNamespaceManager -Document $insertedTableDocument

Assert-Equal -Actual $insertTableSummary.status -Expected "completed" `
    -Message "Insert-table summary did not report status=completed."
Assert-Equal -Actual $insertTableSummary.operation_count -Expected 1 `
    -Message "Insert-table summary should record one operation."
Assert-Equal -Actual $insertTableSummary.operations[0].op -Expected "insert_table_after" `
    -Message "Insert-table operation should be recorded."
Assert-Equal -Actual $insertTableSummary.operations[0].command -Expected "insert-table-after" `
    -Message "Insert-table operation should use the CLI insert-table-after command."
Assert-Equal -Actual $insertedTableDocument.SelectNodes("//w:tbl", $insertedTableNamespaceManager).Count -Expected 2 `
    -Message "Insert-table output should contain the original and inserted body tables."
Assert-DocxXPath `
    -Document $insertedTableDocument `
    -NamespaceManager $insertedTableNamespaceManager `
    -XPath "//w:tbl[2]/w:tr[1]/w:tc[2]" `
    -Message "Inserted table should contain the requested second cell."

$insertLikeTablePlanPath = Join-Path $resolvedWorkingDir "invoice.insert_table_like_plan.json"
$tableLikeInsertedDocx = Join-Path $resolvedWorkingDir "invoice.table_like_inserted.docx"
$insertLikeTableSummaryPath = Join-Path $resolvedWorkingDir "invoice.insert_table_like.summary.json"

Set-Content -LiteralPath $insertLikeTablePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_table_like_after",
      "table_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $insertLikeTablePlanPath `
    -OutputDocx $tableLikeInsertedDocx `
    -SummaryJson $insertLikeTableSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the insert_table_like_after plan."
}

$insertLikeTableSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $insertLikeTableSummaryPath | ConvertFrom-Json
$likeInsertedTableXml = Read-DocxEntryText -DocxPath $tableLikeInsertedDocx -EntryName "word/document.xml"
$likeInsertedTableDocument = New-Object System.Xml.XmlDocument
$likeInsertedTableDocument.PreserveWhitespace = $true
$likeInsertedTableDocument.LoadXml($likeInsertedTableXml)
$likeInsertedTableNamespaceManager = New-WordNamespaceManager -Document $likeInsertedTableDocument

Assert-Equal -Actual $insertLikeTableSummary.status -Expected "completed" `
    -Message "Insert-table-like summary did not report status=completed."
Assert-Equal -Actual $insertLikeTableSummary.operation_count -Expected 1 `
    -Message "Insert-table-like summary should record one operation."
Assert-Equal -Actual $insertLikeTableSummary.operations[0].op -Expected "insert_table_like_after" `
    -Message "Insert-table-like operation should be recorded."
Assert-Equal -Actual $insertLikeTableSummary.operations[0].command -Expected "insert-table-like-after" `
    -Message "Insert-table-like operation should use the CLI insert-table-like-after command."
Assert-Equal -Actual $likeInsertedTableDocument.SelectNodes("//w:tbl", $likeInsertedTableNamespaceManager).Count -Expected 2 `
    -Message "Insert-table-like output should contain the original and cloned body tables."
Assert-DocxXPath `
    -Document $likeInsertedTableDocument `
    -NamespaceManager $likeInsertedTableNamespaceManager `
    -XPath "//w:tbl[2]/w:tr[1]/w:tc[1]" `
    -Message "Cloned table should contain a first cell."
$nonEmptyCloneTextNodes = @(
    $likeInsertedTableDocument.SelectNodes("//w:tbl[2]//w:t", $likeInsertedTableNamespaceManager) |
        Where-Object { -not [string]::IsNullOrEmpty($_.InnerText) }
)
Assert-Equal -Actual $nonEmptyCloneTextNodes.Count -Expected 0 `
    -Message "Cloned table should clear copied cell text."

$insertParagraphAfterTablePlanPath = Join-Path $resolvedWorkingDir "invoice.insert_paragraph_after_table_plan.json"
$paragraphAfterTableDocx = Join-Path $resolvedWorkingDir "invoice.paragraph_after_table.docx"
$insertParagraphAfterTableSummaryPath = Join-Path $resolvedWorkingDir "invoice.insert_paragraph_after_table.summary.json"

Set-Content -LiteralPath $insertParagraphAfterTablePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_paragraph_after_table",
      "table_index": 0,
      "text": "Inserted paragraph after table."
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $insertParagraphAfterTablePlanPath `
    -OutputDocx $paragraphAfterTableDocx `
    -SummaryJson $insertParagraphAfterTableSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the insert_paragraph_after_table plan."
}

$insertParagraphAfterTableSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $insertParagraphAfterTableSummaryPath | ConvertFrom-Json
$paragraphAfterTableXml = Read-DocxEntryText -DocxPath $paragraphAfterTableDocx -EntryName "word/document.xml"
$paragraphAfterTableDocument = New-Object System.Xml.XmlDocument
$paragraphAfterTableDocument.PreserveWhitespace = $true
$paragraphAfterTableDocument.LoadXml($paragraphAfterTableXml)
$paragraphAfterTableNamespaceManager = New-WordNamespaceManager -Document $paragraphAfterTableDocument

Assert-Equal -Actual $insertParagraphAfterTableSummary.status -Expected "completed" `
    -Message "Insert-paragraph-after-table summary did not report status=completed."
Assert-Equal -Actual $insertParagraphAfterTableSummary.operation_count -Expected 1 `
    -Message "Insert-paragraph-after-table summary should record one operation."
Assert-Equal -Actual $insertParagraphAfterTableSummary.operations[0].op -Expected "insert_paragraph_after_table" `
    -Message "Insert-paragraph-after-table operation should be recorded."
Assert-Equal -Actual $insertParagraphAfterTableSummary.operations[0].command -Expected "insert-paragraph-after-table" `
    -Message "Insert-paragraph-after-table operation should use the CLI insert-paragraph-after-table command."
Assert-DocxXPath `
    -Document $paragraphAfterTableDocument `
    -NamespaceManager $paragraphAfterTableNamespaceManager `
    -XPath "//w:tbl[1]/following-sibling::*[1][self::w:p]//w:t[.='Inserted paragraph after table.']" `
    -Message "Inserted paragraph should immediately follow the first body table."

$setTablePositionPlanPath = Join-Path $resolvedWorkingDir "invoice.set_table_position_plan.json"
$tablePositionedDocx = Join-Path $resolvedWorkingDir "invoice.table_positioned.docx"
$setTablePositionSummaryPath = Join-Path $resolvedWorkingDir "invoice.set_table_position.summary.json"

Set-Content -LiteralPath $setTablePositionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_table_position",
      "table_index": 0,
      "preset": "page-corner",
      "horizontal_spec": "center",
      "horizontal_offset": 360,
      "vertical_spec": "bottom",
      "bottom_from_text": 288
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $setTablePositionPlanPath `
    -OutputDocx $tablePositionedDocx `
    -SummaryJson $setTablePositionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the set_table_position plan."
}

$setTablePositionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $setTablePositionSummaryPath | ConvertFrom-Json
$tablePositionedXml = Read-DocxEntryText -DocxPath $tablePositionedDocx -EntryName "word/document.xml"
$tablePositionedDocument = New-Object System.Xml.XmlDocument
$tablePositionedDocument.PreserveWhitespace = $true
$tablePositionedDocument.LoadXml($tablePositionedXml)
$tablePositionedNamespaceManager = New-WordNamespaceManager -Document $tablePositionedDocument

Assert-Equal -Actual $setTablePositionSummary.status -Expected "completed" `
    -Message "Set-table-position summary did not report status=completed."
Assert-Equal -Actual $setTablePositionSummary.operation_count -Expected 1 `
    -Message "Set-table-position summary should record one operation."
Assert-Equal -Actual $setTablePositionSummary.operations[0].op -Expected "set_table_position" `
    -Message "Set-table-position operation should be recorded."
Assert-Equal -Actual $setTablePositionSummary.operations[0].command -Expected "set-table-position" `
    -Message "Set-table-position operation should use the CLI set-table-position command."
Assert-DocxXPath `
    -Document $tablePositionedDocument `
    -NamespaceManager $tablePositionedNamespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblpPr[@w:horzAnchor='page' and @w:tblpX='360' and @w:tblpXSpec='center' and @w:vertAnchor='page' and @w:tblpYSpec='bottom' and @w:bottomFromText='288' and @w:tblOverlap='never']" `
    -Message "Set-table-position output should contain the requested floating table position."

$clearTablePositionPlanPath = Join-Path $resolvedWorkingDir "invoice.clear_table_position_plan.json"
$tablePositionClearedDocx = Join-Path $resolvedWorkingDir "invoice.table_position_cleared.docx"
$clearTablePositionSummaryPath = Join-Path $resolvedWorkingDir "invoice.clear_table_position.summary.json"

Set-Content -LiteralPath $clearTablePositionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_table_position",
      "table_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $tablePositionedDocx `
    -EditPlan $clearTablePositionPlanPath `
    -OutputDocx $tablePositionClearedDocx `
    -SummaryJson $clearTablePositionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the clear_table_position plan."
}

$clearTablePositionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $clearTablePositionSummaryPath | ConvertFrom-Json
$tablePositionClearedXml = Read-DocxEntryText -DocxPath $tablePositionClearedDocx -EntryName "word/document.xml"
$tablePositionClearedDocument = New-Object System.Xml.XmlDocument
$tablePositionClearedDocument.PreserveWhitespace = $true
$tablePositionClearedDocument.LoadXml($tablePositionClearedXml)
$tablePositionClearedNamespaceManager = New-WordNamespaceManager -Document $tablePositionClearedDocument

Assert-Equal -Actual $clearTablePositionSummary.status -Expected "completed" `
    -Message "Clear-table-position summary did not report status=completed."
Assert-Equal -Actual $clearTablePositionSummary.operation_count -Expected 1 `
    -Message "Clear-table-position summary should record one operation."
Assert-Equal -Actual $clearTablePositionSummary.operations[0].op -Expected "clear_table_position" `
    -Message "Clear-table-position operation should be recorded."
Assert-Equal -Actual $clearTablePositionSummary.operations[0].command -Expected "clear-table-position" `
    -Message "Clear-table-position operation should use the CLI clear-table-position command."
Assert-True -Condition ($null -eq $tablePositionClearedDocument.SelectSingleNode("//w:tbl[1]/w:tblPr/w:tblpPr", $tablePositionClearedNamespaceManager)) `
    -Message "Clear-table-position output should remove the floating table position."
