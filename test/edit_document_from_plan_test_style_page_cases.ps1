if ($Scenario -in @("all", "core", "core_fields_style", "core_style_page")) {
$styleListSourceDocx = Join-Path $resolvedWorkingDir "style_list.source.docx"
$styleListPlanPath = Join-Path $resolvedWorkingDir "style_list.edit_plan.json"
$styleListEditedDocx = Join-Path $resolvedWorkingDir "style_list.edited.docx"
$styleListSummaryPath = Join-Path $resolvedWorkingDir "style_list.edit.summary.json"

New-StyleListFixtureDocx -Path $styleListSourceDocx
Set-Content -LiteralPath $styleListPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_paragraph_style",
      "paragraph_index": 0,
      "style_id": "Heading1"
    },
    {
      "op": "clear_paragraph_style",
      "paragraph_index": 1
    },
    {
      "op": "set_run_style",
      "paragraph_index": 2,
      "run_index": 0,
      "style_id": "Strong"
    },
    {
      "op": "clear_run_style",
      "paragraph_index": 2,
      "run_index": 1
    },
    {
      "op": "set_run_font_family",
      "paragraph_index": 2,
      "run_index": 2,
      "font_family": "Consolas"
    },
    {
      "op": "clear_run_font_family",
      "paragraph_index": 2,
      "run_index": 3
    },
    {
      "op": "set_run_language",
      "paragraph_index": 2,
      "run_index": 4,
      "language": "en-US"
    },
    {
      "op": "clear_run_language",
      "paragraph_index": 2,
      "run_index": 5
    },
    {
      "op": "set_paragraph_list",
      "paragraph_index": 3,
      "kind": "bullet",
      "level": 1
    },
    {
      "op": "restart_paragraph_list",
      "paragraph_index": 4,
      "kind": "decimal",
      "level": 0
    },
    {
      "op": "set_paragraph_numbering",
      "paragraph_index": 5,
      "definition": 1,
      "level": 0
    },
    {
      "op": "clear_paragraph_list",
      "paragraph_index": 6
    },
    {
      "op": "set_section_page_setup",
      "section_index": 0,
      "orientation": "landscape",
      "width": 15840,
      "height": 12240,
      "margin_top": 1200,
      "margin_bottom": 1260,
      "margin_left": 1800,
      "margin_right": 1860,
      "margin_header": 720,
      "margin_footer": 760,
      "page_number_start": 5
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleListSourceDocx `
    -EditPlan $styleListPlanPath `
    -OutputDocx $styleListEditedDocx `
    -SummaryJson $styleListSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the style/list/page-setup edit plan."
}

$styleListSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleListSummaryPath | ConvertFrom-Json
$styleListXml = Read-DocxEntryText -DocxPath $styleListEditedDocx -EntryName "word/document.xml"
$styleListDocument = New-Object System.Xml.XmlDocument
$styleListDocument.PreserveWhitespace = $true
$styleListDocument.LoadXml($styleListXml)
$styleListNamespaceManager = New-WordNamespaceManager -Document $styleListDocument
$wordNamespace = $styleListNamespaceManager.LookupNamespace("w")

Assert-Equal -Actual $styleListSummary.status -Expected "completed" `
    -Message "Style/list/page-setup summary did not report status=completed."
Assert-Equal -Actual $styleListSummary.operation_count -Expected 13 `
    -Message "Style/list/page-setup summary should record thirteen operations."
Assert-Equal -Actual $styleListSummary.operations[0].command -Expected "set-paragraph-style" `
    -Message "Set-paragraph-style should use the CLI paragraph-style command."
Assert-Equal -Actual $styleListSummary.operations[1].command -Expected "clear-paragraph-style" `
    -Message "Clear-paragraph-style should use the CLI paragraph-style command."
Assert-Equal -Actual $styleListSummary.operations[2].command -Expected "set-run-style" `
    -Message "Set-run-style should use the CLI run-style command."
Assert-Equal -Actual $styleListSummary.operations[3].command -Expected "clear-run-style" `
    -Message "Clear-run-style should use the CLI run-style command."
Assert-Equal -Actual $styleListSummary.operations[4].command -Expected "set-run-font-family" `
    -Message "Set-run-font-family should use the CLI font-family command."
Assert-Equal -Actual $styleListSummary.operations[5].command -Expected "clear-run-font-family" `
    -Message "Clear-run-font-family should use the CLI font-family command."
Assert-Equal -Actual $styleListSummary.operations[6].command -Expected "set-run-language" `
    -Message "Set-run-language should use the CLI run-language command."
Assert-Equal -Actual $styleListSummary.operations[7].command -Expected "clear-run-language" `
    -Message "Clear-run-language should use the CLI run-language command."
Assert-Equal -Actual $styleListSummary.operations[8].command -Expected "set-paragraph-list" `
    -Message "Set-paragraph-list should use the CLI list command."
Assert-Equal -Actual $styleListSummary.operations[9].command -Expected "restart-paragraph-list" `
    -Message "Restart-paragraph-list should use the CLI list command."
Assert-Equal -Actual $styleListSummary.operations[10].command -Expected "set-paragraph-numbering" `
    -Message "Set-paragraph-numbering should use the CLI numbering command."
Assert-Equal -Actual $styleListSummary.operations[11].command -Expected "clear-paragraph-list" `
    -Message "Clear-paragraph-list should use the CLI list command."
Assert-Equal -Actual $styleListSummary.operations[12].command -Expected "set-section-page-setup" `
    -Message "Set-section-page-setup should use the CLI page-setup command."

$setParagraphStyleNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[1]/w:pPr/w:pStyle", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setParagraphStyleNode) `
    -Message "Set-paragraph-style should create a paragraph style reference."
Assert-Equal -Actual $setParagraphStyleNode.GetAttribute("val", $wordNamespace) -Expected "Heading1" `
    -Message "Set-paragraph-style should set the requested style id."
$clearedParagraphStyleNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[2]/w:pPr/w:pStyle", $styleListNamespaceManager)
Assert-True -Condition ($null -eq $clearedParagraphStyleNode) `
    -Message "Clear-paragraph-style should remove the paragraph style reference."

$setRunStyleNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[1]/w:rPr/w:rStyle", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setRunStyleNode) `
    -Message "Set-run-style should create a run style reference."
Assert-Equal -Actual $setRunStyleNode.GetAttribute("val", $wordNamespace) -Expected "Strong" `
    -Message "Set-run-style should set the requested style id."
$clearedRunStyleNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[2]/w:rPr/w:rStyle", $styleListNamespaceManager)
Assert-True -Condition ($null -eq $clearedRunStyleNode) `
    -Message "Clear-run-style should remove the existing run style reference."

$setRunFontsNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[3]/w:rPr/w:rFonts", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setRunFontsNode) `
    -Message "Set-run-font-family should create a run font-family override."
Assert-Equal -Actual $setRunFontsNode.GetAttribute("ascii", $wordNamespace) -Expected "Consolas" `
    -Message "Set-run-font-family should update the ASCII font family."
Assert-Equal -Actual $setRunFontsNode.GetAttribute("hAnsi", $wordNamespace) -Expected "Consolas" `
    -Message "Set-run-font-family should update the hAnsi font family."
$clearedRunFontsNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[4]/w:rPr/w:rFonts", $styleListNamespaceManager)
Assert-True -Condition ($null -eq $clearedRunFontsNode) `
    -Message "Clear-run-font-family should remove the run font-family override."

$setRunLanguageNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[5]/w:rPr/w:lang", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setRunLanguageNode) `
    -Message "Set-run-language should create a run language override."
Assert-Equal -Actual $setRunLanguageNode.GetAttribute("val", $wordNamespace) -Expected "en-US" `
    -Message "Set-run-language should update the requested language."
$clearedRunLanguageNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[6]/w:rPr/w:lang", $styleListNamespaceManager)
Assert-True -Condition ($null -eq $clearedRunLanguageNode) `
    -Message "Clear-run-language should remove the run language override."

$setListLevelNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[4]/w:pPr/w:numPr/w:ilvl", $styleListNamespaceManager)
$setListNumIdNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[4]/w:pPr/w:numPr/w:numId", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setListLevelNode -and $null -ne $setListNumIdNode) `
    -Message "Set-paragraph-list should add numbering metadata to the target paragraph."
Assert-Equal -Actual $setListLevelNode.GetAttribute("val", $wordNamespace) -Expected "1" `
    -Message "Set-paragraph-list should preserve the requested list level."

$restartListLevelNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[5]/w:pPr/w:numPr/w:ilvl", $styleListNamespaceManager)
$restartListNumIdNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[5]/w:pPr/w:numPr/w:numId", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $restartListLevelNode -and $null -ne $restartListNumIdNode) `
    -Message "Restart-paragraph-list should add numbering metadata to the target paragraph."
Assert-Equal -Actual $restartListLevelNode.GetAttribute("val", $wordNamespace) -Expected "0" `
    -Message "Restart-paragraph-list should preserve the requested list level."
Assert-True -Condition ($restartListNumIdNode.GetAttribute("val", $wordNamespace) -ne $setListNumIdNode.GetAttribute("val", $wordNamespace)) `
    -Message "Restart-paragraph-list should allocate a fresh numbering instance."

$setParagraphNumberingLevelNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[6]/w:pPr/w:numPr/w:ilvl", $styleListNamespaceManager)
$setParagraphNumberingNumIdNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[6]/w:pPr/w:numPr/w:numId", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setParagraphNumberingLevelNode -and $null -ne $setParagraphNumberingNumIdNode) `
    -Message "Set-paragraph-numbering should add numbering metadata to the target paragraph."
Assert-Equal -Actual $setParagraphNumberingLevelNode.GetAttribute("val", $wordNamespace) -Expected "0" `
    -Message "Set-paragraph-numbering should preserve the requested numbering level."
Assert-Equal -Actual $setParagraphNumberingNumIdNode.GetAttribute("val", $wordNamespace) -Expected "7" `
    -Message "Set-paragraph-numbering should reference the requested numbering definition."

$clearedParagraphListNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[7]/w:pPr/w:numPr", $styleListNamespaceManager)
Assert-True -Condition ($null -eq $clearedParagraphListNode) `
    -Message "Clear-paragraph-list should remove numbering metadata from the target paragraph."

$sectionPageSizeNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:sectPr/w:pgSz", $styleListNamespaceManager)
$sectionMarginsNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:sectPr/w:pgMar", $styleListNamespaceManager)
$sectionPageNumberNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:sectPr/w:pgNumType", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $sectionPageSizeNode -and $null -ne $sectionMarginsNode -and $null -ne $sectionPageNumberNode) `
    -Message "Set-section-page-setup should update the section page-setup nodes."
Assert-Equal -Actual $sectionPageSizeNode.GetAttribute("orient", $wordNamespace) -Expected "landscape" `
    -Message "Set-section-page-setup should set the requested orientation."
Assert-Equal -Actual $sectionPageSizeNode.GetAttribute("w", $wordNamespace) -Expected "15840" `
    -Message "Set-section-page-setup should set the requested page width."
Assert-Equal -Actual $sectionPageSizeNode.GetAttribute("h", $wordNamespace) -Expected "12240" `
    -Message "Set-section-page-setup should set the requested page height."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("top", $wordNamespace) -Expected "1200" `
    -Message "Set-section-page-setup should set the top margin."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("bottom", $wordNamespace) -Expected "1260" `
    -Message "Set-section-page-setup should set the bottom margin."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("left", $wordNamespace) -Expected "1800" `
    -Message "Set-section-page-setup should set the left margin."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("right", $wordNamespace) -Expected "1860" `
    -Message "Set-section-page-setup should set the right margin."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("header", $wordNamespace) -Expected "720" `
    -Message "Set-section-page-setup should set the header margin."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("footer", $wordNamespace) -Expected "760" `
    -Message "Set-section-page-setup should set the footer margin."
Assert-Equal -Actual $sectionPageNumberNode.GetAttribute("start", $wordNamespace) -Expected "5" `
    -Message "Set-section-page-setup should set the starting page number."

$styleListClearPageNumberPlanPath = Join-Path $resolvedWorkingDir "style_list.clear_page_number_start.edit_plan.json"
$styleListClearPageNumberDocx = Join-Path $resolvedWorkingDir "style_list.clear_page_number_start.edited.docx"
$styleListClearPageNumberSummaryPath = Join-Path $resolvedWorkingDir "style_list.clear_page_number_start.summary.json"

Set-Content -LiteralPath $styleListClearPageNumberPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_section_page_setup",
      "section_index": 0,
      "clear_page_number_start": true
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleListEditedDocx `
    -EditPlan $styleListClearPageNumberPlanPath `
    -OutputDocx $styleListClearPageNumberDocx `
    -SummaryJson $styleListClearPageNumberSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the clear page-number-start plan."
}

$styleListClearPageNumberSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleListClearPageNumberSummaryPath | ConvertFrom-Json
$styleListClearPageNumberXml = Read-DocxEntryText -DocxPath $styleListClearPageNumberDocx -EntryName "word/document.xml"
$styleListClearPageNumberDocument = New-Object System.Xml.XmlDocument
$styleListClearPageNumberDocument.PreserveWhitespace = $true
$styleListClearPageNumberDocument.LoadXml($styleListClearPageNumberXml)
$styleListClearPageNumberNamespaceManager = New-WordNamespaceManager -Document $styleListClearPageNumberDocument
$clearedSectionPageNumberNode = $styleListClearPageNumberDocument.SelectSingleNode("/w:document/w:body/w:sectPr/w:pgNumType", $styleListClearPageNumberNamespaceManager)

Assert-Equal -Actual $styleListClearPageNumberSummary.status -Expected "completed" `
    -Message "Clear-page-number-start summary did not report status=completed."
Assert-Equal -Actual $styleListClearPageNumberSummary.operation_count -Expected 1 `
    -Message "Clear-page-number-start summary should record one operation."
Assert-Equal -Actual $styleListClearPageNumberSummary.operations[0].command -Expected "set-section-page-setup" `
    -Message "Clear-page-number-start should use the CLI page-setup command."
Assert-True -Condition ($null -eq $clearedSectionPageNumberNode) `
    -Message "Clear-page-number-start should remove the section page-number-start node."

if ($Scenario -in @("core", "core_fields_style", "core_style_page")) {
    Write-Host "Edit-from-plan core regression passed."
    exit 0
}
}
