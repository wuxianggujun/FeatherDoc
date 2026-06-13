if ($Scenario -in @("all", "core", "core_fields_style", "core_fields")) {
$fieldSourceDocx = Join-Path $resolvedWorkingDir "fields.source.docx"
$fieldPlanPath = Join-Path $resolvedWorkingDir "fields.edit_plan.json"
$fieldEditedDocx = Join-Path $resolvedWorkingDir "fields.edited.docx"
$fieldSummaryPath = Join-Path $resolvedWorkingDir "fields.edit.summary.json"

New-BookmarkRichFixtureDocx -Path $fieldSourceDocx

Set-Content -LiteralPath $fieldPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_page_number_field",
      "dirty": true,
      "locked": true
    },
    {
      "op": "append_total_pages_field",
      "part": "body"
    },
    {
      "op": "append_table_of_contents_field",
      "min_outline_level": 1,
      "max_outline_level": 2,
      "no_hyperlinks": true,
      "show_page_numbers_in_web_layout": true,
      "no_outline_levels": true,
      "result_text": "TOC placeholder"
    },
    {
      "op": "append_document_property_field",
      "property_name": "Title",
      "result_text": "FeatherDoc",
      "preserve_formatting": false
    },
    {
      "op": "append_date_field",
      "part": "body",
      "date_format": "yyyy-MM-dd",
      "text": "2026-05-13",
      "dirty": true
    },
    {
      "op": "set_update_fields_on_open",
      "enabled": true
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $fieldSourceDocx `
    -EditPlan $fieldPlanPath `
    -OutputDocx $fieldEditedDocx `
    -SummaryJson $fieldSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the fields edit plan."
}

$fieldSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $fieldSummaryPath | ConvertFrom-Json
$fieldXml = Read-DocxEntryText -DocxPath $fieldEditedDocx -EntryName "word/document.xml"
$fieldSettingsXml = Read-DocxEntryText -DocxPath $fieldEditedDocx -EntryName "word/settings.xml"

Assert-Equal -Actual $fieldSummary.status -Expected "completed" `
    -Message "Fields summary did not report status=completed."
Assert-Equal -Actual $fieldSummary.operation_count -Expected 6 `
    -Message "Fields summary should record six operations."
Assert-Equal -Actual $fieldSummary.operations[0].command -Expected "append-page-number-field" `
    -Message "Append-page-number-field should use the CLI field command."
Assert-Equal -Actual $fieldSummary.operations[1].command -Expected "append-total-pages-field" `
    -Message "Append-total-pages-field should use the CLI field command."
Assert-Equal -Actual $fieldSummary.operations[2].command -Expected "append-table-of-contents-field" `
    -Message "Append-table-of-contents-field should use the CLI field command."
Assert-Equal -Actual $fieldSummary.operations[3].command -Expected "append-document-property-field" `
    -Message "Append-document-property-field should use the CLI field command."
Assert-Equal -Actual $fieldSummary.operations[4].command -Expected "append-date-field" `
    -Message "Append-date-field should use the CLI field command."
Assert-Equal -Actual $fieldSummary.operations[5].command -Expected "set-update-fields-on-open" `
    -Message "Set-update-fields-on-open should use the CLI update-fields command."
Assert-ContainsText -Text $fieldXml -ExpectedText 'w:instr=" PAGE "' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText 'w:instr=" NUMPAGES "' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText 'TOC \o &quot;1-2&quot;' -Label "Fields document.xml"
Assert-NotContainsText -Text $fieldXml -UnexpectedText 'TOC \o &quot;1-2&quot; \h' -Label "Fields document.xml"
Assert-NotContainsText -Text $fieldXml -UnexpectedText 'TOC \o &quot;1-2&quot; \z' -Label "Fields document.xml"
Assert-NotContainsText -Text $fieldXml -UnexpectedText 'TOC \o &quot;1-2&quot; \u' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText "TOC placeholder" -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText "DOCPROPERTY Title" -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText "FeatherDoc" -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText 'DATE \@ &quot;yyyy-MM-dd&quot;' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText "2026-05-13" -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText 'w:dirty="true"' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText 'w:fldLock="true"' -Label "Fields document.xml"
Assert-NotContainsText -Text $fieldXml -UnexpectedText 'DOCPROPERTY Title \* MERGEFORMAT' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldSettingsXml -ExpectedText "<w:updateFields" -Label "Fields settings.xml"
Assert-ContainsText -Text $fieldSettingsXml -ExpectedText 'w:val="1"' -Label "Fields settings.xml"

$fieldClearPlanPath = Join-Path $resolvedWorkingDir "fields.clear_update_fields_plan.json"
$fieldClearedDocx = Join-Path $resolvedWorkingDir "fields.update_fields_cleared.docx"
$fieldClearSummaryPath = Join-Path $resolvedWorkingDir "fields.clear_update_fields.summary.json"

Set-Content -LiteralPath $fieldClearPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_update_fields_on_open"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $fieldEditedDocx `
    -EditPlan $fieldClearPlanPath `
    -OutputDocx $fieldClearedDocx `
    -SummaryJson $fieldClearSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the clear_update_fields_on_open plan."
}

$fieldClearSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $fieldClearSummaryPath | ConvertFrom-Json
$fieldClearedSettingsXml = Read-DocxEntryText -DocxPath $fieldClearedDocx -EntryName "word/settings.xml"

Assert-Equal -Actual $fieldClearSummary.status -Expected "completed" `
    -Message "Fields-clear summary did not report status=completed."
Assert-Equal -Actual $fieldClearSummary.operation_count -Expected 1 `
    -Message "Fields-clear summary should record one operation."
Assert-Equal -Actual $fieldClearSummary.operations[0].command -Expected "set-update-fields-on-open" `
    -Message "Clear-update-fields-on-open should use the CLI update-fields command."
Assert-NotContainsText -Text $fieldClearedSettingsXml -UnexpectedText "<w:updateFields" -Label "Fields cleared settings.xml"

$advancedFieldSourceDocx = Join-Path $resolvedWorkingDir "advanced_fields.source.docx"
$advancedFieldPlanPath = Join-Path $resolvedWorkingDir "advanced_fields.edit_plan.json"
$advancedFieldEditedDocx = Join-Path $resolvedWorkingDir "advanced_fields.edited.docx"
$advancedFieldSummaryPath = Join-Path $resolvedWorkingDir "advanced_fields.edit.summary.json"

New-BookmarkRichFixtureDocx -Path $advancedFieldSourceDocx
Set-Content -LiteralPath $advancedFieldPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_field",
      "instruction": " AUTHOR ",
      "result_text": "Plan author",
      "dirty": true,
      "locked": true
    },
    {
      "op": "append_reference_field",
      "bookmark_name": "metrics_table",
      "result_text": "Plan reference",
      "no_hyperlink": true,
      "preserve_formatting": false
    },
    {
      "op": "append_sequence_field",
      "identifier": "Figure",
      "number_format": "ROMAN",
      "restart": 4,
      "result_text": "IV",
      "preserve_formatting": false
    },
    {
      "op": "replace_field",
      "index": 2,
      "instruction": " SEQ Table \\* ARABIC \\r 1 ",
      "result_text": "1"
    },
    {
      "op": "append_page_reference_field",
      "bookmark_name": "metrics_table",
      "result_text": "Plan page reference",
      "no_hyperlink": true,
      "relative_position": true,
      "preserve_formatting": false
    },
    {
      "op": "append_style_reference_field",
      "style_name": "Heading 1",
      "result_text": "Plan style reference",
      "paragraph_number": true,
      "relative_position": true,
      "preserve_formatting": false
    },
    {
      "op": "append_hyperlink_field",
      "target": "https://example.com/report",
      "anchor": "metrics_table",
      "tooltip": "Open report",
      "result_text": "Plan hyperlink field",
      "preserve_formatting": false,
      "dirty": true,
      "locked": true
    },
    {
      "op": "append_caption",
      "label": "Figure",
      "text": "Plan caption body.",
      "number_result_text": "VII",
      "number_format": "ROMAN",
      "restart": 3,
      "separator": " - ",
      "preserve_formatting": false
    },
    {
      "op": "append_index_entry_field",
      "entry_text": "Plan entry",
      "subentry": "Sub entry",
      "bookmark": "metrics_table",
      "cross_reference": "See also",
      "bold_page_number": true,
      "italic_page_number": true
    },
    {
      "op": "append_index_field",
      "columns": 2,
      "result_text": "Plan index field",
      "preserve_formatting": false
    },
    {
      "op": "append_complex_field",
      "instruction": "AUTHOR",
      "result_text": "Ada Lovelace",
      "dirty": true
    },
    {
      "op": "append_complex_field",
      "instruction_before": "IF ",
      "nested_instruction": "PAGE",
      "nested_result_text": "1",
      "instruction_after": " = 1 one many",
      "result_text": "one"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $advancedFieldSourceDocx `
    -EditPlan $advancedFieldPlanPath `
    -OutputDocx $advancedFieldEditedDocx `
    -SummaryJson $advancedFieldSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the advanced fields edit plan."
}

$advancedFieldSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $advancedFieldSummaryPath | ConvertFrom-Json
$advancedFieldXml = Read-DocxEntryText -DocxPath $advancedFieldEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $advancedFieldSummary.status -Expected "completed" `
    -Message "Advanced-fields summary did not report status=completed."
Assert-Equal -Actual $advancedFieldSummary.operation_count -Expected 12 `
    -Message "Advanced-fields summary should record twelve operations."
Assert-Equal -Actual $advancedFieldSummary.operations[0].command -Expected "append-field" `
    -Message "Append-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[1].command -Expected "append-reference-field" `
    -Message "Append-reference-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[2].command -Expected "append-sequence-field" `
    -Message "Append-sequence-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[3].command -Expected "replace-field" `
    -Message "Replace-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[4].command -Expected "append-page-reference-field" `
    -Message "Append-page-reference-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[5].command -Expected "append-style-reference-field" `
    -Message "Append-style-reference-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[6].command -Expected "append-hyperlink-field" `
    -Message "Append-hyperlink-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[7].command -Expected "append-caption" `
    -Message "Append-caption should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[8].command -Expected "append-index-entry-field" `
    -Message "Append-index-entry-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[9].command -Expected "append-index-field" `
    -Message "Append-index-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[10].command -Expected "append-complex-field" `
    -Message "Append-complex-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[11].command -Expected "append-complex-field" `
    -Message "Nested append-complex-field should use the CLI field command."
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "AUTHOR" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "Plan author" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "REF metrics_table" -Label "Advanced-fields document.xml"
Assert-NotContainsText -Text $advancedFieldXml -UnexpectedText 'REF metrics_table \h' -Label "Advanced-fields document.xml"
Assert-NotContainsText -Text $advancedFieldXml -UnexpectedText 'REF metrics_table \* MERGEFORMAT' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'SEQ Table \* ARABIC \r 1' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText ">1<" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'PAGEREF metrics_table \p' -Label "Advanced-fields document.xml"
Assert-NotContainsText -Text $advancedFieldXml -UnexpectedText 'PAGEREF metrics_table \h' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'STYLEREF &quot;Heading 1&quot; \n \p' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'HYPERLINK &quot;https://example.com/report&quot; \l &quot;metrics_table&quot; \o &quot;Open report&quot;' -Label "Advanced-fields document.xml"
Assert-NotContainsText -Text $advancedFieldXml -UnexpectedText 'HYPERLINK &quot;https://example.com/report&quot; \l &quot;metrics_table&quot; \o &quot;Open report&quot; \* MERGEFORMAT' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'SEQ Figure \* ROMAN \r 3' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "Plan caption body." -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "VII" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'XE &quot;Plan entry:Sub entry&quot; \r metrics_table \t &quot;See also&quot; \b \i' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'INDEX \c &quot;2&quot;' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "Plan index field" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "Ada Lovelace" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "PAGE" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "one" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'w:fldLock="true"' -Label "Advanced-fields document.xml"
if ($Scenario -eq "core_fields") {
    Write-Host "Edit-from-plan fields core regression passed."
    exit 0
}
}
