$sectionPartsSourceDocx = Join-Path $resolvedWorkingDir "section_parts.source.docx"
$sectionPartsPlanPath = Join-Path $resolvedWorkingDir "section_parts.edit_plan.json"
$sectionPartsEditedDocx = Join-Path $resolvedWorkingDir "section_parts.edited.docx"
$sectionPartsSummaryPath = Join-Path $resolvedWorkingDir "section_parts.edit.summary.json"

New-SectionPartsFixtureDocx -Path $sectionPartsSourceDocx
Set-Content -LiteralPath $sectionPartsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_section",
      "section_index": 1,
      "no_inherit": true
    },
    {
      "op": "copy_section_layout",
      "source_index": 2,
      "target_index": 1
    },
    {
      "op": "move_section",
      "source_index": 3,
      "target_index": 0
    },
    {
      "op": "set_section_header",
      "section_index": 2,
      "kind": "even",
      "text": "Plan even header"
    },
    {
      "op": "set_section_footer",
      "section_index": 0,
      "text": "Plan footer from plan"
    },
    {
      "op": "assign_section_header",
      "section_index": 1,
      "header_index": 0
    },
    {
      "op": "assign_section_footer",
      "section_index": 1,
      "footer_index": 0,
      "kind": "first"
    },
    {
      "op": "delete_section",
      "section_index": 3
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sectionPartsSourceDocx `
    -EditPlan $sectionPartsPlanPath `
    -OutputDocx $sectionPartsEditedDocx `
    -SummaryJson $sectionPartsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the section/header/footer edit plan."
}

$sectionPartsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $sectionPartsSummaryPath | ConvertFrom-Json
$sectionPartsDocumentXml = Read-DocxEntryText -DocxPath $sectionPartsEditedDocx -EntryName "word/document.xml"
$sectionPartsHeaderXml = Read-DocxEntryTextsMatching -DocxPath $sectionPartsEditedDocx -EntryNamePattern '^word/header[0-9]+\.xml$'
$sectionPartsFooterXml = Read-DocxEntryTextsMatching -DocxPath $sectionPartsEditedDocx -EntryNamePattern '^word/footer[0-9]+\.xml$'

Assert-Equal -Actual $sectionPartsSummary.status -Expected "completed" `
    -Message "Section/header/footer summary did not report status=completed."
Assert-Equal -Actual $sectionPartsSummary.operation_count -Expected 8 `
    -Message "Section/header/footer summary should record eight operations."
Assert-Equal -Actual $sectionPartsSummary.operations[0].command -Expected "insert-section" `
    -Message "insert_section should use the CLI insert-section command."
Assert-Equal -Actual $sectionPartsSummary.operations[1].command -Expected "copy-section-layout" `
    -Message "copy_section_layout should use the CLI copy-section-layout command."
Assert-Equal -Actual $sectionPartsSummary.operations[2].command -Expected "move-section" `
    -Message "move_section should use the CLI move-section command."
Assert-Equal -Actual $sectionPartsSummary.operations[3].command -Expected "set-section-header" `
    -Message "set_section_header should use the CLI set-section-header command."
Assert-Equal -Actual $sectionPartsSummary.operations[4].command -Expected "set-section-footer" `
    -Message "set_section_footer should use the CLI set-section-footer command."
Assert-Equal -Actual $sectionPartsSummary.operations[5].command -Expected "assign-section-header" `
    -Message "assign_section_header should use the CLI assign-section-header command."
Assert-Equal -Actual $sectionPartsSummary.operations[6].command -Expected "assign-section-footer" `
    -Message "assign_section_footer should use the CLI assign-section-footer command."
Assert-Equal -Actual $sectionPartsSummary.operations[7].command -Expected "remove-section" `
    -Message "delete_section should use the CLI remove-section command."
Assert-ContainsText -Text $sectionPartsDocumentXml -ExpectedText "section 2 body" -Label "Section/header/footer document.xml"
Assert-ContainsText -Text $sectionPartsHeaderXml -ExpectedText "Plan even header" -Label "Section/header/footer header parts"
Assert-ContainsText -Text $sectionPartsFooterXml -ExpectedText "Plan footer from plan" -Label "Section/header/footer footer parts"

$sectionReferenceRemoveSourceDocx = Join-Path $resolvedWorkingDir "section_reference_remove.source.docx"
$sectionReferenceRemovePlanPath = Join-Path $resolvedWorkingDir "section_reference_remove.edit_plan.json"
$sectionReferenceRemoveEditedDocx = Join-Path $resolvedWorkingDir "section_reference_remove.edited.docx"
$sectionReferenceRemoveSummaryPath = Join-Path $resolvedWorkingDir "section_reference_remove.edit.summary.json"

New-SectionPartsFixtureDocx -Path $sectionReferenceRemoveSourceDocx
Set-Content -LiteralPath $sectionReferenceRemovePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "remove_section_header",
      "section_index": 0
    },
    {
      "op": "remove_section_footer",
      "section_index": 1,
      "kind": "first"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sectionReferenceRemoveSourceDocx `
    -EditPlan $sectionReferenceRemovePlanPath `
    -OutputDocx $sectionReferenceRemoveEditedDocx `
    -SummaryJson $sectionReferenceRemoveSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the section header/footer reference removal edit plan."
}

$sectionReferenceRemoveSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $sectionReferenceRemoveSummaryPath | ConvertFrom-Json
$sectionReferenceRemoveDocumentXml = Read-DocxEntryText -DocxPath $sectionReferenceRemoveEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $sectionReferenceRemoveSummary.status -Expected "completed" `
    -Message "Section header/footer reference removal summary did not report status=completed."
Assert-Equal -Actual $sectionReferenceRemoveSummary.operation_count -Expected 2 `
    -Message "Section header/footer reference removal summary should record two operations."
Assert-Equal -Actual $sectionReferenceRemoveSummary.operations[0].command -Expected "remove-section-header" `
    -Message "remove_section_header should use the CLI remove-section-header command."
Assert-Equal -Actual $sectionReferenceRemoveSummary.operations[1].command -Expected "remove-section-footer" `
    -Message "remove_section_footer should use the CLI remove-section-footer command."
Assert-ContainsText -Text $sectionReferenceRemoveDocumentXml -ExpectedText "section 2 body" -Label "Section header/footer reference removal document.xml"

$sectionPartReorderSourceDocx = Join-Path $resolvedWorkingDir "section_part_reorder.source.docx"
$sectionPartReorderPlanPath = Join-Path $resolvedWorkingDir "section_part_reorder.edit_plan.json"
$sectionPartReorderEditedDocx = Join-Path $resolvedWorkingDir "section_part_reorder.edited.docx"
$sectionPartReorderSummaryPath = Join-Path $resolvedWorkingDir "section_part_reorder.edit.summary.json"

New-SectionPartsFixtureDocx -Path $sectionPartReorderSourceDocx
Set-Content -LiteralPath $sectionPartReorderPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "move_header_part",
      "source_index": 1,
      "target_index": 0
    },
    {
      "op": "move_footer_part",
      "source_index": 1,
      "target_index": 0
    },
    {
      "op": "remove_header_part",
      "header_index": 1
    },
    {
      "op": "remove_footer_part",
      "footer_index": 1
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sectionPartReorderSourceDocx `
    -EditPlan $sectionPartReorderPlanPath `
    -OutputDocx $sectionPartReorderEditedDocx `
    -SummaryJson $sectionPartReorderSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the header/footer part reorder edit plan."
}

$sectionPartReorderSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $sectionPartReorderSummaryPath | ConvertFrom-Json
$sectionPartReorderDocumentXml = Read-DocxEntryText -DocxPath $sectionPartReorderEditedDocx -EntryName "word/document.xml"
$sectionPartReorderHeaderXml = Read-DocxEntryTextsMatching -DocxPath $sectionPartReorderEditedDocx -EntryNamePattern '^word/header[0-9]+\.xml$'
$sectionPartReorderFooterXml = Read-DocxEntryTextsMatching -DocxPath $sectionPartReorderEditedDocx -EntryNamePattern '^word/footer[0-9]+\.xml$'

Assert-Equal -Actual $sectionPartReorderSummary.status -Expected "completed" `
    -Message "Header/footer part reorder summary did not report status=completed."
Assert-Equal -Actual $sectionPartReorderSummary.operation_count -Expected 4 `
    -Message "Header/footer part reorder summary should record four operations."
Assert-Equal -Actual $sectionPartReorderSummary.operations[0].command -Expected "move-header-part" `
    -Message "move_header_part should use the CLI move-header-part command."
Assert-Equal -Actual $sectionPartReorderSummary.operations[1].command -Expected "move-footer-part" `
    -Message "move_footer_part should use the CLI move-footer-part command."
Assert-Equal -Actual $sectionPartReorderSummary.operations[2].command -Expected "remove-header-part" `
    -Message "remove_header_part should use the CLI remove-header-part command."
Assert-Equal -Actual $sectionPartReorderSummary.operations[3].command -Expected "remove-footer-part" `
    -Message "remove_footer_part should use the CLI remove-footer-part command."
Assert-ContainsText -Text $sectionPartReorderDocumentXml -ExpectedText "section 2 body" -Label "Header/footer part reorder document.xml"
Assert-ContainsText -Text $sectionPartReorderHeaderXml -ExpectedText "section 1 header" -Label "Header/footer part reorder header parts"
Assert-ContainsText -Text $sectionPartReorderFooterXml -ExpectedText "section 1 first footer" -Label "Header/footer part reorder footer parts"
