$reviewSourceDocx = Join-Path $resolvedWorkingDir "review.source.docx"
$reviewPlanPath = Join-Path $resolvedWorkingDir "review.edit_plan.json"
$reviewEditedDocx = Join-Path $resolvedWorkingDir "review.edited.docx"
$reviewSummaryPath = Join-Path $resolvedWorkingDir "review.edit.summary.json"

New-ReviewFixtureDocx -Path $reviewSourceDocx

Set-Content -LiteralPath $reviewPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "accept_all_revisions"
    },
    {
      "op": "set_comment_resolved",
      "comment_index": 0,
      "resolved": true,
      "expected_text": "Commented text",
      "expected_comment_text": "Reviewer note",
      "expected_resolved": false
    },
    {
      "op": "set_comment_metadata",
      "comment_index": 0,
      "author": "Plan Reviewer",
      "clear_initials": true,
      "date": "2026-05-05T10:00:00Z",
      "expected_comment_text": "Reviewer note",
      "expected_resolved": true
    },
    {
      "op": "replace_comment",
      "comment_index": 0,
      "text": "Plan replaced comment.",
      "expected_text": "Commented text",
      "expected_comment_text": "Reviewer note",
      "expected_resolved": true
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $reviewSourceDocx `
    -EditPlan $reviewPlanPath `
    -OutputDocx $reviewEditedDocx `
    -SummaryJson $reviewSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the review/comment edit plan."
}

$reviewSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewSummaryPath | ConvertFrom-Json
$reviewDocumentXml = Read-DocxEntryText -DocxPath $reviewEditedDocx -EntryName "word/document.xml"
$reviewCommentsXml = Read-DocxEntryText -DocxPath $reviewEditedDocx -EntryName "word/comments.xml"
$reviewCommentsExtendedXml = Read-DocxEntryText -DocxPath $reviewEditedDocx -EntryName "word/commentsExtended.xml"

Assert-Equal -Actual $reviewSummary.status -Expected "completed" `
    -Message "Review summary did not report status=completed."
Assert-Equal -Actual $reviewSummary.operation_count -Expected 4 `
    -Message "Review summary should record four operations."
Assert-Equal -Actual $reviewSummary.operations[0].command -Expected "accept-all-revisions" `
    -Message "Accept-all-revisions should use the CLI revision cleanup command."
Assert-Equal -Actual $reviewSummary.operations[1].command -Expected "apply-review-mutation-plan" `
    -Message "Set-comment-resolved should use the review mutation plan command."
Assert-Equal -Actual $reviewSummary.operations[2].command -Expected "apply-review-mutation-plan" `
    -Message "Set-comment-metadata should use the review mutation plan command."
Assert-Equal -Actual $reviewSummary.operations[3].command -Expected "apply-review-mutation-plan" `
    -Message "Replace-comment should use the review mutation plan command."
Assert-ContainsText -Text $reviewDocumentXml -ExpectedText "Inserted review text" -Label "Review document.xml"
Assert-NotContainsText -Text $reviewDocumentXml -UnexpectedText "Deleted review text" -Label "Review document.xml"
Assert-NotContainsText -Text $reviewDocumentXml -UnexpectedText "<w:ins" -Label "Review document.xml"
Assert-NotContainsText -Text $reviewDocumentXml -UnexpectedText "<w:del" -Label "Review document.xml"
Assert-ContainsText -Text $reviewCommentsXml -ExpectedText "Plan replaced comment." -Label "Review comments.xml"
Assert-ContainsText -Text $reviewCommentsXml -ExpectedText 'w:author="Plan Reviewer"' -Label "Review comments.xml"
Assert-ContainsText -Text $reviewCommentsXml -ExpectedText 'w:date="2026-05-05T10:00:00Z"' -Label "Review comments.xml"
Assert-NotContainsText -Text $reviewCommentsXml -UnexpectedText 'w:initials=' -Label "Review comments.xml"
Assert-ContainsText -Text $reviewCommentsExtendedXml -ExpectedText 'w15:done="1"' -Label "Review commentsExtended.xml"

$reviewRemovePlanPath = Join-Path $resolvedWorkingDir "review.remove_comment_plan.json"
$reviewRemovedDocx = Join-Path $resolvedWorkingDir "review.comment_removed.docx"
$reviewRemoveSummaryPath = Join-Path $resolvedWorkingDir "review.remove_comment.summary.json"

Set-Content -LiteralPath $reviewRemovePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "remove_comment",
      "comment_index": 0,
      "expected_text": "Commented text",
      "expected_comment_text": "Plan replaced comment.",
      "expected_resolved": false
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $reviewEditedDocx `
    -EditPlan $reviewRemovePlanPath `
    -OutputDocx $reviewRemovedDocx `
    -SummaryJson $reviewRemoveSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the remove_comment review plan."
}

$reviewRemoveSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewRemoveSummaryPath | ConvertFrom-Json
$reviewRemovedDocumentXml = Read-DocxEntryText -DocxPath $reviewRemovedDocx -EntryName "word/document.xml"
$reviewRemovedCommentsXml = Read-DocxEntryText -DocxPath $reviewRemovedDocx -EntryName "word/comments.xml"

Assert-Equal -Actual $reviewRemoveSummary.status -Expected "completed" `
    -Message "Remove-comment summary did not report status=completed."
Assert-Equal -Actual $reviewRemoveSummary.operation_count -Expected 1 `
    -Message "Remove-comment summary should record one operation."
Assert-Equal -Actual $reviewRemoveSummary.operations[0].command -Expected "apply-review-mutation-plan" `
    -Message "Remove-comment should use the review mutation plan command."
Assert-NotContainsText -Text $reviewRemovedDocumentXml -UnexpectedText "commentRangeStart" -Label "Review removed document.xml"
Assert-NotContainsText -Text $reviewRemovedDocumentXml -UnexpectedText "commentReference" -Label "Review removed document.xml"
Assert-NotContainsText -Text $reviewRemovedCommentsXml -UnexpectedText "Plan replaced comment." -Label "Review removed comments.xml"

$reviewInlinePlanSourceDocx = Join-Path $resolvedWorkingDir "review.inline_plan.source.docx"
$reviewInlinePlanPath = Join-Path $resolvedWorkingDir "review.inline_plan.edit_plan.json"
$reviewInlinePlanEditedDocx = Join-Path $resolvedWorkingDir "review.inline_plan.edited.docx"
$reviewInlinePlanSummaryPath = Join-Path $resolvedWorkingDir "review.inline_plan.summary.json"

New-ReviewFixtureDocx -Path $reviewInlinePlanSourceDocx
Set-Content -LiteralPath $reviewInlinePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "apply_review_mutation_plan",
      "operations": [
        {
          "kind": "set_comment_resolved",
          "comment_index": 0,
          "resolved": true,
          "expected_text": "Commented text",
          "expected_comment_text": "Reviewer note",
          "expected_resolved": false
        },
        {
          "kind": "replace_comment",
          "comment_index": 0,
          "comment_text": "Inline review mutation plan comment.",
          "expected_text": "Commented text",
          "expected_comment_text": "Reviewer note",
          "expected_resolved": false
        }
      ]
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $reviewInlinePlanSourceDocx `
    -EditPlan $reviewInlinePlanPath `
    -OutputDocx $reviewInlinePlanEditedDocx `
    -SummaryJson $reviewInlinePlanSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the inline review mutation plan."
}

$reviewInlinePlanSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewInlinePlanSummaryPath | ConvertFrom-Json
$reviewInlinePlanCommentsXml = Read-DocxEntryText -DocxPath $reviewInlinePlanEditedDocx -EntryName "word/comments.xml"
$reviewInlinePlanCommentsExtendedXml = Read-DocxEntryText -DocxPath $reviewInlinePlanEditedDocx -EntryName "word/commentsExtended.xml"

Assert-Equal -Actual $reviewInlinePlanSummary.status -Expected "completed" `
    -Message "Inline review-mutation-plan summary did not report status=completed."
Assert-Equal -Actual $reviewInlinePlanSummary.operation_count -Expected 1 `
    -Message "Inline review-mutation-plan summary should record one operation."
Assert-Equal -Actual $reviewInlinePlanSummary.operations[0].command -Expected "apply-review-mutation-plan" `
    -Message "Inline review-mutation-plan should use the CLI review mutation plan command."
Assert-ContainsText -Text $reviewInlinePlanCommentsXml -ExpectedText "Inline review mutation plan comment." -Label "Inline review plan comments.xml"
Assert-ContainsText -Text $reviewInlinePlanCommentsExtendedXml -ExpectedText 'w15:done="1"' -Label "Inline review plan commentsExtended.xml"

$reviewPlanFileSourceDocx = Join-Path $resolvedWorkingDir "review.plan_file.source.docx"
$reviewPlanFilePath = Join-Path $resolvedWorkingDir "review.plan_file.review_mutation_plan.json"
$reviewPlanFileEditPlanPath = Join-Path $resolvedWorkingDir "review.plan_file.edit_plan.json"
$reviewPlanFileEditedDocx = Join-Path $resolvedWorkingDir "review.plan_file.edited.docx"
$reviewPlanFileSummaryPath = Join-Path $resolvedWorkingDir "review.plan_file.summary.json"

New-ReviewFixtureDocx -Path $reviewPlanFileSourceDocx
Write-Utf8NoBomFile -Path $reviewPlanFilePath -Content @'
{
  "operations": [
    {
      "kind": "set_comment_metadata",
      "comment_index": 0,
      "author": "Plan File Reviewer",
      "clear_initials": true,
      "date": "2026-05-07T09:00:00Z",
      "expected_text": "Commented text",
      "expected_comment_text": "Reviewer note"
    }
  ]
}
'@
Set-Content -LiteralPath $reviewPlanFileEditPlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "apply_review_mutation_plan",
      "plan_file": "$($reviewPlanFilePath.Replace('\', '\\'))"
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $reviewPlanFileSourceDocx `
    -EditPlan $reviewPlanFileEditPlanPath `
    -OutputDocx $reviewPlanFileEditedDocx `
    -SummaryJson $reviewPlanFileSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the review mutation plan_file operation."
}

$reviewPlanFileSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewPlanFileSummaryPath | ConvertFrom-Json
$reviewPlanFileCommentsXml = Read-DocxEntryText -DocxPath $reviewPlanFileEditedDocx -EntryName "word/comments.xml"

Assert-Equal -Actual $reviewPlanFileSummary.status -Expected "completed" `
    -Message "Review mutation plan_file summary did not report status=completed."
Assert-Equal -Actual $reviewPlanFileSummary.operation_count -Expected 1 `
    -Message "Review mutation plan_file summary should record one operation."
Assert-Equal -Actual $reviewPlanFileSummary.operations[0].command -Expected "apply-review-mutation-plan" `
    -Message "Review mutation plan_file should use the CLI review mutation plan command."
Assert-ContainsText -Text $reviewPlanFileCommentsXml -ExpectedText 'w:author="Plan File Reviewer"' -Label "Review plan_file comments.xml"
Assert-ContainsText -Text $reviewPlanFileCommentsXml -ExpectedText 'w:date="2026-05-07T09:00:00Z"' -Label "Review plan_file comments.xml"
Assert-NotContainsText -Text $reviewPlanFileCommentsXml -UnexpectedText 'w:initials=' -Label "Review plan_file comments.xml"

$reviewRejectSourceDocx = Join-Path $resolvedWorkingDir "review.reject.source.docx"
$reviewRejectPlanPath = Join-Path $resolvedWorkingDir "review.reject_plan.json"
$reviewRejectedDocx = Join-Path $resolvedWorkingDir "review.rejected.docx"
$reviewRejectSummaryPath = Join-Path $resolvedWorkingDir "review.reject.summary.json"

New-ReviewFixtureDocx -Path $reviewRejectSourceDocx
Set-Content -LiteralPath $reviewRejectPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "reject_all_revisions"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $reviewRejectSourceDocx `
    -EditPlan $reviewRejectPlanPath `
    -OutputDocx $reviewRejectedDocx `
    -SummaryJson $reviewRejectSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the reject_all_revisions plan."
}

$reviewRejectSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewRejectSummaryPath | ConvertFrom-Json
$reviewRejectedDocumentXml = Read-DocxEntryText -DocxPath $reviewRejectedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $reviewRejectSummary.status -Expected "completed" `
    -Message "Reject-all-revisions summary did not report status=completed."
Assert-Equal -Actual $reviewRejectSummary.operation_count -Expected 1 `
    -Message "Reject-all-revisions summary should record one operation."
Assert-Equal -Actual $reviewRejectSummary.operations[0].command -Expected "reject-all-revisions" `
    -Message "Reject-all-revisions should use the CLI revision cleanup command."
Assert-ContainsText -Text $reviewRejectedDocumentXml -ExpectedText "Deleted review text" -Label "Rejected review document.xml"
Assert-NotContainsText -Text $reviewRejectedDocumentXml -UnexpectedText "Inserted review text" -Label "Rejected review document.xml"
Assert-NotContainsText -Text $reviewRejectedDocumentXml -UnexpectedText "<w:ins" -Label "Rejected review document.xml"
Assert-NotContainsText -Text $reviewRejectedDocumentXml -UnexpectedText "<w:del" -Label "Rejected review document.xml"
