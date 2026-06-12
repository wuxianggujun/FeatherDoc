if ($Scenario -in @("all", "core", "core_content")) {
$contentControlSourceDocx = Join-Path $resolvedWorkingDir "content_control.source.docx"
$contentControlPlanPath = Join-Path $resolvedWorkingDir "content_control.edit_plan.json"
$contentControlEditedDocx = Join-Path $resolvedWorkingDir "content_control.edited.docx"
$contentControlSummaryPath = Join-Path $resolvedWorkingDir "content_control.edit.summary.json"
$contentControlImagePath = Join-Path $resolvedWorkingDir "content_control.logo.png"

New-ContentControlFixtureDocx -Path $contentControlSourceDocx
[System.IO.File]::WriteAllBytes(
    $contentControlImagePath,
    [System.Convert]::FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGP4z8AAAAMBAQDJ/pLvAAAAAElFTkSuQmCC"))
$escapedContentControlImagePath = $contentControlImagePath.Replace("\", "\\")

Set-Content -LiteralPath $contentControlPlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "replace_content_control_text",
      "content_control_tag": "order_no",
      "text": "INV-002"
    },
    {
      "op": "replace_content_control_paragraphs",
      "content_control_tag": "summary",
      "paragraphs": [
        "Summary paragraph one",
        "Summary paragraph two"
      ]
    },
    {
      "op": "replace_content_control_table_rows",
      "content_control_tag": "line_items",
      "rows": [
        ["SKU-1", "Ready"],
        ["SKU-2", "Queued"]
      ]
    },
    {
      "op": "replace_content_control_table",
      "content_control_alias": "Metrics",
      "rows": [
        ["Metric", "Value"],
        ["Quality", "Pass"]
      ]
    },
    {
      "op": "set_content_control_form_state",
      "content_control_tag": "approved",
      "checked": false,
      "clear_lock": true
    },
    {
      "op": "set_content_control_form_state",
      "content_control_alias": "Status",
      "selected_item": "draft",
      "lock": "sdtLocked"
    },
    {
      "op": "set_content_control_form_state",
      "content_control_tag": "due_date",
      "date_text": "2026-06-01",
      "date_format": "yyyy/MM/dd",
      "date_locale": "zh-CN"
    },
    {
      "op": "replace_content_control_image",
      "content_control_tag": "logo",
      "image_path": "$escapedContentControlImagePath",
      "width": 24,
      "height": 24
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $contentControlSourceDocx `
    -EditPlan $contentControlPlanPath `
    -OutputDocx $contentControlEditedDocx `
    -SummaryJson $contentControlSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the content-control edit plan."
}

$contentControlSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $contentControlSummaryPath | ConvertFrom-Json
$contentControlXml = Read-DocxEntryText -DocxPath $contentControlEditedDocx -EntryName "word/document.xml"
$contentControlDocument = New-Object System.Xml.XmlDocument
$contentControlDocument.PreserveWhitespace = $true
$contentControlDocument.LoadXml($contentControlXml)
$contentControlNamespaceManager = New-WordNamespaceManager -Document $contentControlDocument

Assert-Equal -Actual $contentControlSummary.status -Expected "completed" `
    -Message "Content-control summary did not report status=completed."
Assert-Equal -Actual $contentControlSummary.operation_count -Expected 8 `
    -Message "Content-control summary should record eight operations."
Assert-Equal -Actual $contentControlSummary.operations[0].command -Expected "replace-content-control-text" `
    -Message "Content-control text edit should use the CLI text command."
Assert-Equal -Actual $contentControlSummary.operations[1].command -Expected "replace-content-control-paragraphs" `
    -Message "Content-control paragraph edit should use the CLI paragraph command."
Assert-Equal -Actual $contentControlSummary.operations[2].command -Expected "replace-content-control-table-rows" `
    -Message "Content-control table-row edit should use the CLI table-row command."
Assert-Equal -Actual $contentControlSummary.operations[3].command -Expected "replace-content-control-table" `
    -Message "Content-control table edit should use the CLI table command."
Assert-Equal -Actual $contentControlSummary.operations[4].command -Expected "set-content-control-form-state" `
    -Message "Content-control checkbox edit should use the CLI form-state command."
Assert-Equal -Actual $contentControlSummary.operations[7].command -Expected "replace-content-control-image" `
    -Message "Content-control image edit should use the CLI image command."
Assert-ContainsText -Text $contentControlXml -ExpectedText "INV-002" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "Summary paragraph one" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "Summary paragraph two" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "SKU-2" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "Queued" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "Metric" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "Quality" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText 'w14:checked w14:val="0"' -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText '<w:t>Draft</w:t>' -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "2026-06-01" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText 'w:dateFormat w:val="yyyy/MM/dd"' -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText 'w:lid w:val="zh-CN"' -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText 'w:lock w:val="sdtLocked"' -Label "Content-control document.xml"
Assert-DocxXPath `
    -Document $contentControlDocument `
    -NamespaceManager $contentControlNamespaceManager `
    -XPath "//w:drawing" `
    -Message "Content-control image replacement should add a drawing."
Assert-DocxEntryExists `
    -DocxPath $contentControlEditedDocx `
    -EntryName "word/media/image1.png" `
    -Message "Content-control image replacement should add image media."
Assert-NotContainsText -Text $contentControlXml -UnexpectedText "INV-001" -Label "Content-control document.xml"
Assert-NotContainsText -Text $contentControlXml -UnexpectedText "Summary placeholder" -Label "Content-control document.xml"
Assert-NotContainsText -Text $contentControlXml -UnexpectedText "Metrics placeholder" -Label "Content-control document.xml"
Assert-NotContainsText -Text $contentControlXml -UnexpectedText "Logo placeholder" -Label "Content-control document.xml"

$customXmlContentControlSourceDocx = Join-Path $resolvedWorkingDir "custom_xml_content_control.source.docx"
$customXmlContentControlPlanPath = Join-Path $resolvedWorkingDir "custom_xml_content_control.edit_plan.json"
$customXmlContentControlEditedDocx = Join-Path $resolvedWorkingDir "custom_xml_content_control.edited.docx"
$customXmlContentControlSummaryPath = Join-Path $resolvedWorkingDir "custom_xml_content_control.edit.summary.json"

New-CustomXmlContentControlFixtureDocx -Path $customXmlContentControlSourceDocx

Set-Content -LiteralPath $customXmlContentControlPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "sync_content_controls_from_custom_xml"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $customXmlContentControlSourceDocx `
    -EditPlan $customXmlContentControlPlanPath `
    -OutputDocx $customXmlContentControlEditedDocx `
    -SummaryJson $customXmlContentControlSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the custom XML content-control sync edit plan."
}

$customXmlContentControlSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $customXmlContentControlSummaryPath | ConvertFrom-Json
$customXmlContentControlXml = Read-DocxEntryText -DocxPath $customXmlContentControlEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $customXmlContentControlSummary.status -Expected "completed" `
    -Message "Custom XML content-control sync summary did not report status=completed."
Assert-Equal -Actual $customXmlContentControlSummary.operation_count -Expected 1 `
    -Message "Custom XML content-control sync summary should record one operation."
Assert-Equal -Actual $customXmlContentControlSummary.operations[0].command -Expected "sync-content-controls-from-custom-xml" `
    -Message "Custom XML content-control sync should use the CLI sync command."
Assert-ContainsText -Text $customXmlContentControlXml -ExpectedText "<w:t>2026-08-20</w:t>" -Label "Custom XML content-control document.xml"
Assert-NotContainsText -Text $customXmlContentControlXml -UnexpectedText "Pending date" -Label "Custom XML content-control document.xml"

$bookmarkRichSourceDocx = Join-Path $resolvedWorkingDir "bookmark_rich.source.docx"
$bookmarkRichPlanPath = Join-Path $resolvedWorkingDir "bookmark_rich.edit_plan.json"
$bookmarkRichEditedDocx = Join-Path $resolvedWorkingDir "bookmark_rich.edited.docx"
$bookmarkRichSummaryPath = Join-Path $resolvedWorkingDir "bookmark_rich.edit.summary.json"
$bookmarkRichImagePath = Join-Path $resolvedWorkingDir "bookmark_rich.logo.png"

New-BookmarkRichFixtureDocx -Path $bookmarkRichSourceDocx
[System.IO.File]::WriteAllBytes(
    $bookmarkRichImagePath,
    [System.Convert]::FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGP4z8AAAAMBAQDJ/pLvAAAAAElFTkSuQmCC"))
$escapedBookmarkRichImagePath = $bookmarkRichImagePath.Replace("\", "\\")

Set-Content -LiteralPath $bookmarkRichPlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "fill_bookmarks",
      "values": {
        "fill_only": "Filled bookmark value"
      }
    },
    {
      "op": "replace_bookmark_table",
      "bookmark": "metrics_table",
      "rows": [
        ["Metric", "Value"],
        ["Quality", "Pass"]
      ]
    },
    {
      "op": "remove_bookmark_block",
      "bookmark": "optional_note"
    },
    {
      "op": "replace_bookmark_image",
      "bookmark": "inline_logo",
      "image_path": "$escapedBookmarkRichImagePath",
      "width": 24,
      "height": 24
    },
    {
      "op": "replace_bookmark_floating_image",
      "bookmark": "floating_logo",
      "image_path": "$escapedBookmarkRichImagePath",
      "width": 32,
      "height": 32,
      "horizontal_reference": "margin",
      "horizontal_offset": 18,
      "vertical_reference": "paragraph",
      "vertical_offset": 24,
      "behind_text": false,
      "allow_overlap": false,
      "z_order": 7,
      "wrap_mode": "square",
      "wrap_distance_left": 2,
      "wrap_distance_right": 2,
      "wrap_distance_top": 1,
      "wrap_distance_bottom": 1,
      "crop_left": 0,
      "crop_top": 0,
      "crop_right": 0,
      "crop_bottom": 0
    },
    {
      "op": "set_bookmark_block_visibility",
      "bookmark": "hide_block",
      "visible": false
    },
    {
      "op": "apply_bookmark_block_visibility",
      "show": "keep_block",
      "hide": "hide_batch_block"
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $bookmarkRichSourceDocx `
    -EditPlan $bookmarkRichPlanPath `
    -OutputDocx $bookmarkRichEditedDocx `
    -SummaryJson $bookmarkRichSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the bookmark rich edit plan."
}

$bookmarkRichSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $bookmarkRichSummaryPath | ConvertFrom-Json
$bookmarkRichXml = Read-DocxEntryText -DocxPath $bookmarkRichEditedDocx -EntryName "word/document.xml"
$bookmarkRichDocument = New-Object System.Xml.XmlDocument
$bookmarkRichDocument.PreserveWhitespace = $true
$bookmarkRichDocument.LoadXml($bookmarkRichXml)
$bookmarkRichNamespaceManager = New-WordNamespaceManager -Document $bookmarkRichDocument

Assert-Equal -Actual $bookmarkRichSummary.status -Expected "completed" `
    -Message "Bookmark-rich summary did not report status=completed."
Assert-Equal -Actual $bookmarkRichSummary.operation_count -Expected 7 `
    -Message "Bookmark-rich summary should record seven operations."
Assert-Equal -Actual $bookmarkRichSummary.operations[0].command -Expected "fill-bookmarks" `
    -Message "Fill-bookmarks should use the CLI fill-bookmarks command."
Assert-Equal -Actual $bookmarkRichSummary.operations[1].command -Expected "replace-bookmark-table" `
    -Message "Bookmark table edit should use the CLI replace-bookmark-table command."
Assert-Equal -Actual $bookmarkRichSummary.operations[2].command -Expected "remove-bookmark-block" `
    -Message "Bookmark block removal should use the CLI remove-bookmark-block command."
Assert-Equal -Actual $bookmarkRichSummary.operations[3].command -Expected "replace-bookmark-image" `
    -Message "Bookmark image edit should use the CLI replace-bookmark-image command."
Assert-Equal -Actual $bookmarkRichSummary.operations[4].command -Expected "replace-bookmark-floating-image" `
    -Message "Bookmark floating-image edit should use the CLI replace-bookmark-floating-image command."
Assert-Equal -Actual $bookmarkRichSummary.operations[5].command -Expected "set-bookmark-block-visibility" `
    -Message "Bookmark block visibility edit should use the CLI set-bookmark-block-visibility command."
Assert-Equal -Actual $bookmarkRichSummary.operations[6].command -Expected "apply-bookmark-block-visibility" `
    -Message "Bookmark block visibility batch edit should use the CLI apply-bookmark-block-visibility command."
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "Metric" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "Quality" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "Filled bookmark value" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "fill only placeholder" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "Keep me" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "wp:inline" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "wp:anchor" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText 'relativeHeight="7"' -Label "Bookmark-rich document.xml"
Assert-DocxXPath `
    -Document $bookmarkRichDocument `
    -NamespaceManager $bookmarkRichNamespaceManager `
    -XPath "//w:tbl[1]//w:t[.='Pass']" `
    -Message "Bookmark table replacement should write the generated table cells."
Assert-DocxEntryExists `
    -DocxPath $bookmarkRichEditedDocx `
    -EntryName "word/media/image1.png" `
    -Message "Bookmark image replacement should add the first image media."
Assert-DocxEntryExists `
    -DocxPath $bookmarkRichEditedDocx `
    -EntryName "word/media/image2.png" `
    -Message "Bookmark floating-image replacement should add the second image media."
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "metrics table placeholder" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "optional note placeholder" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "inline image placeholder" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "floating image placeholder" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "Secret Cell" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "Hide me" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "Batch hide me" -Label "Bookmark-rich document.xml"

$genericImageSourceDocx = Join-Path $resolvedWorkingDir "generic_image.source.docx"
$genericImagePlanPath = Join-Path $resolvedWorkingDir "generic_image.edit_plan.json"
$genericImageEditedDocx = Join-Path $resolvedWorkingDir "generic_image.edited.docx"
$genericImageSummaryPath = Join-Path $resolvedWorkingDir "generic_image.edit.summary.json"
$genericImageRemovePlanPath = Join-Path $resolvedWorkingDir "generic_image.remove_plan.json"
$genericImageRemovedDocx = Join-Path $resolvedWorkingDir "generic_image.removed.docx"
$genericImageRemoveSummaryPath = Join-Path $resolvedWorkingDir "generic_image.remove.summary.json"
$genericImagePath = Join-Path $resolvedWorkingDir "generic_image.initial.png"
$genericImageReplacementPath = Join-Path $resolvedWorkingDir "generic_image.replacement.gif"

New-BookmarkRichFixtureDocx -Path $genericImageSourceDocx
[System.IO.File]::WriteAllBytes(
    $genericImagePath,
    [System.Convert]::FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGP4z8AAAAMBAQDJ/pLvAAAAAElFTkSuQmCC"))
[System.IO.File]::WriteAllBytes(
    $genericImageReplacementPath,
    [System.Convert]::FromBase64String("R0lGODlhAQABAIABAP///wAAACwAAAAAAQABAAACAkQBADs="))
$escapedGenericImagePath = $genericImagePath.Replace("\", "\\")
$escapedGenericImageReplacementPath = $genericImageReplacementPath.Replace("\", "\\")

Set-Content -LiteralPath $genericImagePlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "append_image",
      "image_path": "$escapedGenericImagePath",
      "width": 16,
      "height": 12
    },
    {
      "op": "replace_image",
      "image_path": "$escapedGenericImageReplacementPath",
      "image_index": 0
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $genericImageSourceDocx `
    -EditPlan $genericImagePlanPath `
    -OutputDocx $genericImageEditedDocx `
    -SummaryJson $genericImageSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the generic image append/replace plan."
}

$genericImageSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $genericImageSummaryPath | ConvertFrom-Json
$genericImageXml = Read-DocxEntryText -DocxPath $genericImageEditedDocx -EntryName "word/document.xml"
$genericImageContentTypesXml = Read-DocxEntryText -DocxPath $genericImageEditedDocx -EntryName "[Content_Types].xml"
$genericImageRelsXml = Read-DocxEntryText -DocxPath $genericImageEditedDocx -EntryName "word/_rels/document.xml.rels"

Assert-Equal -Actual $genericImageSummary.status -Expected "completed" `
    -Message "Generic-image summary did not report status=completed."
Assert-Equal -Actual $genericImageSummary.operation_count -Expected 2 `
    -Message "Generic-image summary should record two operations."
Assert-Equal -Actual $genericImageSummary.operations[0].command -Expected "append-image" `
    -Message "Generic append_image should use the CLI append-image command."
Assert-Equal -Actual $genericImageSummary.operations[1].command -Expected "replace-image" `
    -Message "Generic replace_image should use the CLI replace-image command."
Assert-ContainsText -Text $genericImageXml -ExpectedText "wp:inline" -Label "Generic-image document.xml"
Assert-ContainsText -Text $genericImageContentTypesXml -ExpectedText "image/gif" -Label "Generic-image [Content_Types].xml"
Assert-ContainsText -Text $genericImageRelsXml -ExpectedText "media/image1.gif" -Label "Generic-image document.xml.rels"
Assert-DocxEntryExists `
    -DocxPath $genericImageEditedDocx `
    -EntryName "word/media/image1.gif" `
    -Message "Generic replace_image should replace the appended media with GIF data."

Set-Content -LiteralPath $genericImageRemovePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "remove_image",
      "image_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $genericImageEditedDocx `
    -EditPlan $genericImageRemovePlanPath `
    -OutputDocx $genericImageRemovedDocx `
    -SummaryJson $genericImageRemoveSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the generic remove_image plan."
}

$genericImageRemoveSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $genericImageRemoveSummaryPath | ConvertFrom-Json
$genericImageRemovedXml = Read-DocxEntryText -DocxPath $genericImageRemovedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $genericImageRemoveSummary.status -Expected "completed" `
    -Message "Generic-image remove summary did not report status=completed."
Assert-Equal -Actual $genericImageRemoveSummary.operation_count -Expected 1 `
    -Message "Generic-image remove summary should record one operation."
Assert-Equal -Actual $genericImageRemoveSummary.operations[0].command -Expected "remove-image" `
    -Message "Generic remove_image should use the CLI remove-image command."
Assert-NotContainsText -Text $genericImageRemovedXml -UnexpectedText "w:drawing" -Label "Generic-image removed document.xml"
if ($Scenario -eq "core_content") {
    Write-Host "Edit-from-plan content core regression passed."
    exit 0
}
}
