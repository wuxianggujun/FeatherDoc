$linkFormulaSourceDocx = Join-Path $resolvedWorkingDir "link_formula.source.docx"
$linkFormulaPlanPath = Join-Path $resolvedWorkingDir "link_formula.edit_plan.json"
$linkFormulaEditedDocx = Join-Path $resolvedWorkingDir "link_formula.edited.docx"
$linkFormulaSummaryPath = Join-Path $resolvedWorkingDir "link_formula.edit.summary.json"

New-LinkFormulaFixtureDocx -Path $linkFormulaSourceDocx
Set-Content -LiteralPath $linkFormulaPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_hyperlink",
      "text": "Plan link v1",
      "url": "https://example.com/v1"
    },
    {
      "op": "replace_hyperlink",
      "index": 0,
      "text": "Plan link v2",
      "href": "https://example.com/v2"
    },
    {
      "op": "append_omml",
      "omml_xml": "<m:oMath xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\"><m:r><m:t>x=1</m:t></m:r></m:oMath>"
    },
    {
      "op": "replace_omml",
      "omml_index": 0,
      "omml": "<m:oMath xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\"><m:r><m:t>y=2</m:t></m:r></m:oMath>"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $linkFormulaSourceDocx `
    -EditPlan $linkFormulaPlanPath `
    -OutputDocx $linkFormulaEditedDocx `
    -SummaryJson $linkFormulaSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the hyperlink/OMML edit plan."
}

$linkFormulaSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $linkFormulaSummaryPath | ConvertFrom-Json
$linkFormulaDocumentXml = Read-DocxEntryText -DocxPath $linkFormulaEditedDocx -EntryName "word/document.xml"
$linkFormulaRelsXml = Read-DocxEntryText -DocxPath $linkFormulaEditedDocx -EntryName "word/_rels/document.xml.rels"

Assert-Equal -Actual $linkFormulaSummary.status -Expected "completed" `
    -Message "Hyperlink/OMML summary did not report status=completed."
Assert-Equal -Actual $linkFormulaSummary.operation_count -Expected 4 `
    -Message "Hyperlink/OMML summary should record four operations."
Assert-Equal -Actual $linkFormulaSummary.operations[0].command -Expected "append-hyperlink" `
    -Message "Append-hyperlink should use the CLI hyperlink command."
Assert-Equal -Actual $linkFormulaSummary.operations[1].command -Expected "replace-hyperlink" `
    -Message "Replace-hyperlink should use the CLI hyperlink command."
Assert-Equal -Actual $linkFormulaSummary.operations[2].command -Expected "append-omml" `
    -Message "Append-OMML should use the CLI OMML command."
Assert-Equal -Actual $linkFormulaSummary.operations[3].command -Expected "replace-omml" `
    -Message "Replace-OMML should use the CLI OMML command."
Assert-ContainsText -Text $linkFormulaDocumentXml -ExpectedText "w:hyperlink" -Label "Hyperlink/OMML document.xml"
Assert-ContainsText -Text $linkFormulaDocumentXml -ExpectedText "Plan link v2" -Label "Hyperlink/OMML document.xml"
Assert-NotContainsText -Text $linkFormulaDocumentXml -UnexpectedText "Plan link v1" -Label "Hyperlink/OMML document.xml"
Assert-ContainsText -Text $linkFormulaRelsXml -ExpectedText "https://example.com/v2" -Label "Hyperlink/OMML document.xml.rels"
Assert-NotContainsText -Text $linkFormulaRelsXml -UnexpectedText "https://example.com/v1" -Label "Hyperlink/OMML document.xml.rels"
Assert-ContainsText -Text $linkFormulaDocumentXml -ExpectedText "y=2" -Label "Hyperlink/OMML document.xml"
Assert-NotContainsText -Text $linkFormulaDocumentXml -UnexpectedText "x=1" -Label "Hyperlink/OMML document.xml"

$linkFormulaRemovePlanPath = Join-Path $resolvedWorkingDir "link_formula.remove_plan.json"
$linkFormulaRemovedDocx = Join-Path $resolvedWorkingDir "link_formula.removed.docx"
$linkFormulaRemoveSummaryPath = Join-Path $resolvedWorkingDir "link_formula.remove.summary.json"

Set-Content -LiteralPath $linkFormulaRemovePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "remove_hyperlink",
      "hyperlink_index": 0
    },
    {
      "op": "remove_omml",
      "formula_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $linkFormulaEditedDocx `
    -EditPlan $linkFormulaRemovePlanPath `
    -OutputDocx $linkFormulaRemovedDocx `
    -SummaryJson $linkFormulaRemoveSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the remove hyperlink/OMML edit plan."
}

$linkFormulaRemoveSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $linkFormulaRemoveSummaryPath | ConvertFrom-Json
$linkFormulaRemovedDocumentXml = Read-DocxEntryText -DocxPath $linkFormulaRemovedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $linkFormulaRemoveSummary.status -Expected "completed" `
    -Message "Hyperlink/OMML remove summary did not report status=completed."
Assert-Equal -Actual $linkFormulaRemoveSummary.operation_count -Expected 2 `
    -Message "Hyperlink/OMML remove summary should record two operations."
Assert-Equal -Actual $linkFormulaRemoveSummary.operations[0].command -Expected "remove-hyperlink" `
    -Message "Remove-hyperlink should use the CLI hyperlink command."
Assert-Equal -Actual $linkFormulaRemoveSummary.operations[1].command -Expected "remove-omml" `
    -Message "Remove-OMML should use the CLI OMML command."
Assert-NotContainsText -Text $linkFormulaRemovedDocumentXml -UnexpectedText "w:hyperlink" -Label "Hyperlink/OMML removed document.xml"
Assert-ContainsText -Text $linkFormulaRemovedDocumentXml -ExpectedText "Plan link v2" -Label "Hyperlink/OMML removed document.xml"
Assert-NotContainsText -Text $linkFormulaRemovedDocumentXml -UnexpectedText "<m:oMath" -Label "Hyperlink/OMML removed document.xml"
Assert-NotContainsText -Text $linkFormulaRemovedDocumentXml -UnexpectedText "y=2" -Label "Hyperlink/OMML removed document.xml"

$revisionAuthoringSourceDocx = Join-Path $resolvedWorkingDir "revision_authoring.source.docx"
$revisionAuthoringPlanPath = Join-Path $resolvedWorkingDir "revision_authoring.edit_plan.json"
$revisionAuthoringEditedDocx = Join-Path $resolvedWorkingDir "revision_authoring.edited.docx"
$revisionAuthoringSummaryPath = Join-Path $resolvedWorkingDir "revision_authoring.edit.summary.json"

New-LinkFormulaFixtureDocx -Path $revisionAuthoringSourceDocx
Set-Content -LiteralPath $revisionAuthoringPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_insertion_revision",
      "text": "Plan inserted revision",
      "author": "Plan Insert Author",
      "date": "2026-05-06T09:00:00Z"
    },
    {
      "op": "append_deletion_revision",
      "text": "Plan deleted revision",
      "author": "Plan Delete Author",
      "date": "2026-05-06T10:00:00Z"
    },
    {
      "op": "set_revision_metadata",
      "revision_index": 0,
      "author": "Plan Metadata Author",
      "date": "2026-05-06T11:00:00Z"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $revisionAuthoringSourceDocx `
    -EditPlan $revisionAuthoringPlanPath `
    -OutputDocx $revisionAuthoringEditedDocx `
    -SummaryJson $revisionAuthoringSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the revision authoring edit plan."
}

$revisionAuthoringSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $revisionAuthoringSummaryPath | ConvertFrom-Json
$revisionAuthoringDocumentXml = Read-DocxEntryText -DocxPath $revisionAuthoringEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $revisionAuthoringSummary.status -Expected "completed" `
    -Message "Revision-authoring summary did not report status=completed."
Assert-Equal -Actual $revisionAuthoringSummary.operation_count -Expected 3 `
    -Message "Revision-authoring summary should record three operations."
Assert-Equal -Actual $revisionAuthoringSummary.operations[0].command -Expected "append-insertion-revision" `
    -Message "Append-insertion-revision should use the CLI revision command."
Assert-Equal -Actual $revisionAuthoringSummary.operations[1].command -Expected "append-deletion-revision" `
    -Message "Append-deletion-revision should use the CLI revision command."
Assert-Equal -Actual $revisionAuthoringSummary.operations[2].command -Expected "set-revision-metadata" `
    -Message "Set-revision-metadata should use the CLI revision command."
Assert-ContainsText -Text $revisionAuthoringDocumentXml -ExpectedText "Plan inserted revision" -Label "Revision-authoring document.xml"
Assert-ContainsText -Text $revisionAuthoringDocumentXml -ExpectedText "Plan deleted revision" -Label "Revision-authoring document.xml"
Assert-ContainsText -Text $revisionAuthoringDocumentXml -ExpectedText 'w:author="Plan Metadata Author"' -Label "Revision-authoring document.xml"
Assert-ContainsText -Text $revisionAuthoringDocumentXml -ExpectedText 'w:date="2026-05-06T11:00:00Z"' -Label "Revision-authoring document.xml"

$runRevisionSourceDocx = Join-Path $resolvedWorkingDir "run_revision.source.docx"
$runRevisionPlanPath = Join-Path $resolvedWorkingDir "run_revision.edit_plan.json"
$runRevisionEditedDocx = Join-Path $resolvedWorkingDir "run_revision.edited.docx"
$runRevisionSummaryPath = Join-Path $resolvedWorkingDir "run_revision.edit.summary.json"

New-RunRevisionFixtureDocx -Path $runRevisionSourceDocx
Set-Content -LiteralPath $runRevisionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_run_revision_after",
      "paragraph_index": 0,
      "run_index": 0,
      "text": "Plan run insertion",
      "author": "Plan Run Author",
      "date": "2026-05-06T12:00:00Z"
    },
    {
      "op": "delete_run_revision",
      "paragraph_index": 0,
      "run_index": 1,
      "author": "Plan Run Reviewer",
      "date": "2026-05-06T13:00:00Z"
    },
    {
      "op": "replace_run_revision",
      "paragraph_index": 0,
      "run_index": 1,
      "text": "Plan run replacement",
      "author": "Plan Run Editor",
      "date": "2026-05-06T14:00:00Z"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $runRevisionSourceDocx `
    -EditPlan $runRevisionPlanPath `
    -OutputDocx $runRevisionEditedDocx `
    -SummaryJson $runRevisionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the run revision edit plan."
}

$runRevisionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $runRevisionSummaryPath | ConvertFrom-Json
$runRevisionDocumentXml = Read-DocxEntryText -DocxPath $runRevisionEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $runRevisionSummary.status -Expected "completed" `
    -Message "Run-revision summary did not report status=completed."
Assert-Equal -Actual $runRevisionSummary.operation_count -Expected 3 `
    -Message "Run-revision summary should record three operations."
Assert-Equal -Actual $runRevisionSummary.operations[0].command -Expected "insert-run-revision-after" `
    -Message "Insert-run-revision-after should use the CLI run revision command."
Assert-Equal -Actual $runRevisionSummary.operations[1].command -Expected "delete-run-revision" `
    -Message "Delete-run-revision should use the CLI run revision command."
Assert-Equal -Actual $runRevisionSummary.operations[2].command -Expected "replace-run-revision" `
    -Message "Replace-run-revision should use the CLI run revision command."
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "Plan run insertion" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "Plan run replacement" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "Plan Run Author" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "Plan Run Reviewer" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "Plan Run Editor" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "<w:ins" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "<w:del" -Label "Run-revision document.xml"

$commentAuthoringSourceDocx = Join-Path $resolvedWorkingDir "comment_authoring.source.docx"
$commentAuthoringPlanPath = Join-Path $resolvedWorkingDir "comment_authoring.edit_plan.json"
$commentAuthoringEditedDocx = Join-Path $resolvedWorkingDir "comment_authoring.edited.docx"
$commentAuthoringSummaryPath = Join-Path $resolvedWorkingDir "comment_authoring.edit.summary.json"

New-LinkFormulaFixtureDocx -Path $commentAuthoringSourceDocx
Set-Content -LiteralPath $commentAuthoringPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_comment",
      "selected_text": "Link and formula source",
      "comment_text": "Plan appended comment.",
      "author": "Plan Commenter",
      "initials": "PC",
      "date": "2026-05-06T15:00:00Z"
    },
    {
      "op": "append_comment_reply",
      "parent_comment_index": 0,
      "text": "Plan reply comment.",
      "author": "Plan Replier",
      "initials": "PR",
      "date": "2026-05-06T16:00:00Z"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $commentAuthoringSourceDocx `
    -EditPlan $commentAuthoringPlanPath `
    -OutputDocx $commentAuthoringEditedDocx `
    -SummaryJson $commentAuthoringSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the comment authoring edit plan."
}

$commentAuthoringSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $commentAuthoringSummaryPath | ConvertFrom-Json
$commentAuthoringDocumentXml = Read-DocxEntryText -DocxPath $commentAuthoringEditedDocx -EntryName "word/document.xml"
$commentAuthoringCommentsXml = Read-DocxEntryText -DocxPath $commentAuthoringEditedDocx -EntryName "word/comments.xml"

Assert-Equal -Actual $commentAuthoringSummary.status -Expected "completed" `
    -Message "Comment-authoring summary did not report status=completed."
Assert-Equal -Actual $commentAuthoringSummary.operation_count -Expected 2 `
    -Message "Comment-authoring summary should record two operations."
Assert-Equal -Actual $commentAuthoringSummary.operations[0].command -Expected "append-comment" `
    -Message "Append-comment should use the CLI comment command."
Assert-Equal -Actual $commentAuthoringSummary.operations[1].command -Expected "append-comment-reply" `
    -Message "Append-comment-reply should use the CLI comment command."
Assert-ContainsText -Text $commentAuthoringDocumentXml -ExpectedText "commentRangeStart" -Label "Comment-authoring document.xml"
Assert-ContainsText -Text $commentAuthoringDocumentXml -ExpectedText "commentReference" -Label "Comment-authoring document.xml"
Assert-ContainsText -Text $commentAuthoringCommentsXml -ExpectedText "Plan appended comment." -Label "Comment-authoring comments.xml"
Assert-ContainsText -Text $commentAuthoringCommentsXml -ExpectedText "Plan reply comment." -Label "Comment-authoring comments.xml"
Assert-ContainsText -Text $commentAuthoringCommentsXml -ExpectedText 'w:author="Plan Commenter"' -Label "Comment-authoring comments.xml"
Assert-ContainsText -Text $commentAuthoringCommentsXml -ExpectedText 'w:author="Plan Replier"' -Label "Comment-authoring comments.xml"

$paragraphRangeRevisionSourceDocx = Join-Path $resolvedWorkingDir "paragraph_range_revision.source.docx"
$paragraphRangeRevisionPlanPath = Join-Path $resolvedWorkingDir "paragraph_range_revision.edit_plan.json"
$paragraphRangeRevisionEditedDocx = Join-Path $resolvedWorkingDir "paragraph_range_revision.edited.docx"
$paragraphRangeRevisionSummaryPath = Join-Path $resolvedWorkingDir "paragraph_range_revision.edit.summary.json"

New-RunRevisionFixtureDocx -Path $paragraphRangeRevisionSourceDocx
Set-Content -LiteralPath $paragraphRangeRevisionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_paragraph_text_revision",
      "paragraph_index": 0,
      "text_offset": 5,
      "text": "Plan paragraph insertion ",
      "author": "Plan Paragraph Author",
      "date": "2026-05-06T17:00:00Z"
    },
    {
      "op": "delete_paragraph_text_revision",
      "paragraph_index": 0,
      "offset": 5,
      "length": 6,
      "expected_text": "Middle",
      "author": "Plan Paragraph Reviewer",
      "date": "2026-05-06T18:00:00Z"
    },
    {
      "op": "replace_paragraph_text_revision",
      "paragraph_index": 0,
      "offset": 5,
      "length": 6,
      "text": "Plan paragraph replacement",
      "author": "Plan Paragraph Editor",
      "date": "2026-05-06T19:00:00Z"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $paragraphRangeRevisionSourceDocx `
    -EditPlan $paragraphRangeRevisionPlanPath `
    -OutputDocx $paragraphRangeRevisionEditedDocx `
    -SummaryJson $paragraphRangeRevisionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the paragraph range revision edit plan."
}

$paragraphRangeRevisionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $paragraphRangeRevisionSummaryPath | ConvertFrom-Json
$paragraphRangeRevisionDocumentXml = Read-DocxEntryText -DocxPath $paragraphRangeRevisionEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $paragraphRangeRevisionSummary.status -Expected "completed" `
    -Message "Paragraph-range-revision summary did not report status=completed."
Assert-Equal -Actual $paragraphRangeRevisionSummary.operation_count -Expected 3 `
    -Message "Paragraph-range-revision summary should record three operations."
Assert-Equal -Actual $paragraphRangeRevisionSummary.operations[0].command -Expected "insert-paragraph-text-revision" `
    -Message "Insert-paragraph-text-revision should use the CLI paragraph revision command."
Assert-Equal -Actual $paragraphRangeRevisionSummary.operations[1].command -Expected "delete-paragraph-text-revision" `
    -Message "Delete-paragraph-text-revision should use the CLI paragraph revision command."
Assert-Equal -Actual $paragraphRangeRevisionSummary.operations[2].command -Expected "replace-paragraph-text-revision" `
    -Message "Replace-paragraph-text-revision should use the CLI paragraph revision command."
Assert-ContainsText -Text $paragraphRangeRevisionDocumentXml -ExpectedText "Plan paragraph insertion" -Label "Paragraph-range-revision document.xml"
Assert-ContainsText -Text $paragraphRangeRevisionDocumentXml -ExpectedText "Plan paragraph replacement" -Label "Paragraph-range-revision document.xml"
Assert-ContainsText -Text $paragraphRangeRevisionDocumentXml -ExpectedText "Plan Paragraph Reviewer" -Label "Paragraph-range-revision document.xml"

$textRangeRevisionSourceDocx = Join-Path $resolvedWorkingDir "text_range_revision.source.docx"
$textRangeRevisionPlanPath = Join-Path $resolvedWorkingDir "text_range_revision.edit_plan.json"
$textRangeRevisionEditedDocx = Join-Path $resolvedWorkingDir "text_range_revision.edited.docx"
$textRangeRevisionSummaryPath = Join-Path $resolvedWorkingDir "text_range_revision.edit.summary.json"

New-RangeFixtureDocx -Path $textRangeRevisionSourceDocx
Set-Content -LiteralPath $textRangeRevisionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_text_range_revision",
      "paragraph_index": 0,
      "offset": 6,
      "text": "Plan text insertion ",
      "author": "Plan Text Inserter",
      "date": "2026-05-06T17:30:00Z"
    },
    {
      "op": "delete_text_range_revision",
      "start_paragraph_index": 1,
      "start_offset": 0,
      "end_paragraph_index": 1,
      "end_offset": 6,
      "expected_text": "Middle",
      "author": "Plan Text Deleter",
      "date": "2026-05-06T18:30:00Z"
    },
    {
      "op": "replace_text_range_revision",
      "start_paragraph_index": 2,
      "start_offset": 0,
      "end_paragraph_index": 2,
      "end_offset": 5,
      "text": "Plan text replacement",
      "expected_text": "Gamma",
      "author": "Plan Text Replacer",
      "date": "2026-05-06T19:30:00Z"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $textRangeRevisionSourceDocx `
    -EditPlan $textRangeRevisionPlanPath `
    -OutputDocx $textRangeRevisionEditedDocx `
    -SummaryJson $textRangeRevisionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the text range revision edit plan."
}

$textRangeRevisionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $textRangeRevisionSummaryPath | ConvertFrom-Json
$textRangeRevisionDocumentXml = Read-DocxEntryText -DocxPath $textRangeRevisionEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $textRangeRevisionSummary.status -Expected "completed" `
    -Message "Text-range-revision summary did not report status=completed."
Assert-Equal -Actual $textRangeRevisionSummary.operation_count -Expected 3 `
    -Message "Text-range-revision summary should record three operations."
Assert-Equal -Actual $textRangeRevisionSummary.operations[0].command -Expected "insert-text-range-revision" `
    -Message "Insert-text-range-revision should use the CLI text range revision command."
Assert-Equal -Actual $textRangeRevisionSummary.operations[1].command -Expected "delete-text-range-revision" `
    -Message "Delete-text-range-revision should use the CLI text range revision command."
Assert-Equal -Actual $textRangeRevisionSummary.operations[2].command -Expected "replace-text-range-revision" `
    -Message "Replace-text-range-revision should use the CLI text range revision command."
Assert-ContainsText -Text $textRangeRevisionDocumentXml -ExpectedText "Plan text insertion" -Label "Text-range-revision document.xml"
Assert-ContainsText -Text $textRangeRevisionDocumentXml -ExpectedText "Plan text replacement" -Label "Text-range-revision document.xml"
Assert-ContainsText -Text $textRangeRevisionDocumentXml -ExpectedText "Plan Text Deleter" -Label "Text-range-revision document.xml"

$rangeCommentSourceDocx = Join-Path $resolvedWorkingDir "range_comment.source.docx"
$rangeCommentPlanPath = Join-Path $resolvedWorkingDir "range_comment.edit_plan.json"
$rangeCommentEditedDocx = Join-Path $resolvedWorkingDir "range_comment.edited.docx"
$rangeCommentSummaryPath = Join-Path $resolvedWorkingDir "range_comment.edit.summary.json"

New-RangeFixtureDocx -Path $rangeCommentSourceDocx
Set-Content -LiteralPath $rangeCommentPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_paragraph_text_comment",
      "paragraph_index": 0,
      "offset": 0,
      "length": 3,
      "comment_text": "Plan paragraph range comment.",
      "author": "Plan Range Commenter",
      "initials": "RC",
      "date": "2026-05-06T20:00:00Z"
    },
    {
      "op": "append_text_range_comment",
      "start_paragraph_index": 0,
      "start_offset": 6,
      "end_paragraph_index": 2,
      "end_offset": 5,
      "text": "Plan cross range comment.",
      "author": "Plan Cross Commenter",
      "initials": "XC",
      "date": "2026-05-06T21:00:00Z"
    },
    {
      "op": "set_paragraph_text_comment_range",
      "comment_index": 0,
      "paragraph_index": 0,
      "offset": 6,
      "length": 4
    },
    {
      "op": "set_text_range_comment_range",
      "comment_index": 1,
      "start_paragraph_index": 1,
      "start_offset": 0,
      "end_paragraph_index": 2,
      "end_offset": 5
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $rangeCommentSourceDocx `
    -EditPlan $rangeCommentPlanPath `
    -OutputDocx $rangeCommentEditedDocx `
    -SummaryJson $rangeCommentSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the range comment edit plan."
}

$rangeCommentSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rangeCommentSummaryPath | ConvertFrom-Json
$rangeCommentDocumentXml = Read-DocxEntryText -DocxPath $rangeCommentEditedDocx -EntryName "word/document.xml"
$rangeCommentCommentsXml = Read-DocxEntryText -DocxPath $rangeCommentEditedDocx -EntryName "word/comments.xml"

Assert-Equal -Actual $rangeCommentSummary.status -Expected "completed" `
    -Message "Range-comment summary did not report status=completed."
Assert-Equal -Actual $rangeCommentSummary.operation_count -Expected 4 `
    -Message "Range-comment summary should record four operations."
Assert-Equal -Actual $rangeCommentSummary.operations[0].command -Expected "append-paragraph-text-comment" `
    -Message "Append-paragraph-text-comment should use the CLI range comment command."
Assert-Equal -Actual $rangeCommentSummary.operations[1].command -Expected "append-text-range-comment" `
    -Message "Append-text-range-comment should use the CLI range comment command."
Assert-Equal -Actual $rangeCommentSummary.operations[2].command -Expected "set-paragraph-text-comment-range" `
    -Message "Set-paragraph-text-comment-range should use the CLI range comment command."
Assert-Equal -Actual $rangeCommentSummary.operations[3].command -Expected "set-text-range-comment-range" `
    -Message "Set-text-range-comment-range should use the CLI range comment command."
Assert-ContainsText -Text $rangeCommentDocumentXml -ExpectedText "commentRangeStart" -Label "Range-comment document.xml"
Assert-ContainsText -Text $rangeCommentCommentsXml -ExpectedText "Plan paragraph range comment." -Label "Range-comment comments.xml"
Assert-ContainsText -Text $rangeCommentCommentsXml -ExpectedText "Plan cross range comment." -Label "Range-comment comments.xml"

$footnoteReviewSourceDocx = Join-Path $resolvedWorkingDir "footnote_review.source.docx"
$footnoteReviewPlanPath = Join-Path $resolvedWorkingDir "footnote_review.edit_plan.json"
$footnoteReviewEditedDocx = Join-Path $resolvedWorkingDir "footnote_review.edited.docx"
$footnoteReviewSummaryPath = Join-Path $resolvedWorkingDir "footnote_review.edit.summary.json"

New-RangeFixtureDocx -Path $footnoteReviewSourceDocx
Set-Content -LiteralPath $footnoteReviewPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_footnote",
      "reference_text": "Alpha",
      "note_text": "Plan footnote one."
    },
    {
      "op": "append_footnote",
      "anchor_text": "Middle",
      "body": "Plan footnote two."
    },
    {
      "op": "replace_footnote",
      "footnote_index": 0,
      "text": "Plan footnote replaced."
    },
    {
      "op": "remove_footnote",
      "note_index": 1
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $footnoteReviewSourceDocx `
    -EditPlan $footnoteReviewPlanPath `
    -OutputDocx $footnoteReviewEditedDocx `
    -SummaryJson $footnoteReviewSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the footnote review edit plan."
}

$footnoteReviewSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $footnoteReviewSummaryPath | ConvertFrom-Json
$footnoteReviewDocumentXml = Read-DocxEntryText -DocxPath $footnoteReviewEditedDocx -EntryName "word/document.xml"
$footnoteReviewNotesXml = Read-DocxEntryText -DocxPath $footnoteReviewEditedDocx -EntryName "word/footnotes.xml"

Assert-Equal -Actual $footnoteReviewSummary.status -Expected "completed" `
    -Message "Footnote-review summary did not report status=completed."
Assert-Equal -Actual $footnoteReviewSummary.operation_count -Expected 4 `
    -Message "Footnote-review summary should record four operations."
Assert-Equal -Actual $footnoteReviewSummary.operations[0].command -Expected "append-footnote" `
    -Message "Append-footnote should use the CLI review-note command."
Assert-Equal -Actual $footnoteReviewSummary.operations[1].command -Expected "append-footnote" `
    -Message "Second append-footnote should use the CLI review-note command."
Assert-Equal -Actual $footnoteReviewSummary.operations[2].command -Expected "replace-footnote" `
    -Message "Replace-footnote should use the CLI review-note command."
Assert-Equal -Actual $footnoteReviewSummary.operations[3].command -Expected "remove-footnote" `
    -Message "Remove-footnote should use the CLI review-note command."
Assert-ContainsText -Text $footnoteReviewDocumentXml -ExpectedText "footnoteReference" -Label "Footnote-review document.xml"
Assert-ContainsText -Text $footnoteReviewNotesXml -ExpectedText "Plan footnote replaced." -Label "Footnote-review footnotes.xml"
Assert-NotContainsText -Text $footnoteReviewNotesXml -UnexpectedText "Plan footnote two." -Label "Footnote-review footnotes.xml"

$endnoteReviewSourceDocx = Join-Path $resolvedWorkingDir "endnote_review.source.docx"
$endnoteReviewPlanPath = Join-Path $resolvedWorkingDir "endnote_review.edit_plan.json"
$endnoteReviewEditedDocx = Join-Path $resolvedWorkingDir "endnote_review.edited.docx"
$endnoteReviewSummaryPath = Join-Path $resolvedWorkingDir "endnote_review.edit.summary.json"

New-RangeFixtureDocx -Path $endnoteReviewSourceDocx
Set-Content -LiteralPath $endnoteReviewPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_endnote",
      "selected_text": "Beta",
      "text": "Plan endnote one."
    },
    {
      "op": "append_endnote",
      "reference_text": "Text",
      "note_text": "Plan endnote two."
    },
    {
      "op": "replace_endnote",
      "endnote_index": 1,
      "body": "Plan endnote replaced."
    },
    {
      "op": "remove_endnote",
      "index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $endnoteReviewSourceDocx `
    -EditPlan $endnoteReviewPlanPath `
    -OutputDocx $endnoteReviewEditedDocx `
    -SummaryJson $endnoteReviewSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the endnote review edit plan."
}

$endnoteReviewSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $endnoteReviewSummaryPath | ConvertFrom-Json
$endnoteReviewDocumentXml = Read-DocxEntryText -DocxPath $endnoteReviewEditedDocx -EntryName "word/document.xml"
$endnoteReviewNotesXml = Read-DocxEntryText -DocxPath $endnoteReviewEditedDocx -EntryName "word/endnotes.xml"

Assert-Equal -Actual $endnoteReviewSummary.status -Expected "completed" `
    -Message "Endnote-review summary did not report status=completed."
Assert-Equal -Actual $endnoteReviewSummary.operation_count -Expected 4 `
    -Message "Endnote-review summary should record four operations."
Assert-Equal -Actual $endnoteReviewSummary.operations[0].command -Expected "append-endnote" `
    -Message "Append-endnote should use the CLI review-note command."
Assert-Equal -Actual $endnoteReviewSummary.operations[1].command -Expected "append-endnote" `
    -Message "Second append-endnote should use the CLI review-note command."
Assert-Equal -Actual $endnoteReviewSummary.operations[2].command -Expected "replace-endnote" `
    -Message "Replace-endnote should use the CLI review-note command."
Assert-Equal -Actual $endnoteReviewSummary.operations[3].command -Expected "remove-endnote" `
    -Message "Remove-endnote should use the CLI review-note command."
Assert-ContainsText -Text $endnoteReviewDocumentXml -ExpectedText "endnoteReference" -Label "Endnote-review document.xml"
Assert-ContainsText -Text $endnoteReviewNotesXml -ExpectedText "Plan endnote replaced." -Label "Endnote-review endnotes.xml"
Assert-NotContainsText -Text $endnoteReviewNotesXml -UnexpectedText "Plan endnote one." -Label "Endnote-review endnotes.xml"

Write-Host "Edit-from-plan regression passed."
