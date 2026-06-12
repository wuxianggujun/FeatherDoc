function Invoke-EditPlanReviewOperationArguments {
    param(
        $Operation,
        [string]$InputPath,
        [string]$OutputPath,
        [string]$Label,
        [int]$OperationIndex,
        [string]$TemporaryRoot,
        [string]$NormalizedOp,
        [System.Collections.Generic.List[string]]$Arguments
    )

    switch ($NormalizedOp) {
        "delete_omml" {
            $formulaIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("formula_index", "omml_index", "index") `
                -Label $Label
            $arguments.Add("remove-omml") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($formulaIndex) | Out-Null
        }
        "append_insertion_revision" {
            $arguments.Add("append-insertion-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "append_deletion_revision" {
            $arguments.Add("append-deletion-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "insert_run_revision_after" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $runIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("run_index", "run") `
                -Label $Label
            $arguments.Add("insert-run-revision-after") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "delete_run_revision" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $runIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("run_index", "run") `
                -Label $Label
            $arguments.Add("delete-run-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "replace_run_revision" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $runIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("run_index", "run") `
                -Label $Label
            $arguments.Add("replace-run-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "insert_paragraph_text_revision" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $textOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_offset", "offset") `
                -Label $Label
            $arguments.Add("insert-paragraph-text-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($textOffset) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "delete_paragraph_text_revision" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $textOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_offset", "offset") `
                -Label $Label
            $textLength = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_length", "length") `
                -Label $Label
            $arguments.Add("delete-paragraph-text-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($textOffset) | Out-Null
            $arguments.Add($textLength) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowExpectedText
        }
        "replace_paragraph_text_revision" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $textOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_offset", "offset") `
                -Label $Label
            $textLength = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_length", "length") `
                -Label $Label
            $arguments.Add("replace-paragraph-text-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($textOffset) | Out-Null
            $arguments.Add($textLength) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText `
                -AllowExpectedText
        }
        "insert_text_range_revision" {
            $startParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_paragraph_index", "paragraph_index", "paragraph") `
                -Label $Label
            $startTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_text_offset", "start_offset", "text_offset", "offset") `
                -Label $Label
            $arguments.Add("insert-text-range-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($startParagraphIndex) | Out-Null
            $arguments.Add($startTextOffset) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "delete_text_range_revision" {
            $startParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_paragraph_index", "paragraph_index") `
                -Label $Label
            $startTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_text_offset", "start_offset") `
                -Label $Label
            $endParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_paragraph_index") `
                -Label $Label
            $endTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_text_offset", "end_offset") `
                -Label $Label
            $arguments.Add("delete-text-range-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($startParagraphIndex) | Out-Null
            $arguments.Add($startTextOffset) | Out-Null
            $arguments.Add($endParagraphIndex) | Out-Null
            $arguments.Add($endTextOffset) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowExpectedText
        }
        "replace_text_range_revision" {
            $startParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_paragraph_index", "paragraph_index") `
                -Label $Label
            $startTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_text_offset", "start_offset") `
                -Label $Label
            $endParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_paragraph_index") `
                -Label $Label
            $endTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_text_offset", "end_offset") `
                -Label $Label
            $arguments.Add("replace-text-range-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($startParagraphIndex) | Out-Null
            $arguments.Add($startTextOffset) | Out-Null
            $arguments.Add($endParagraphIndex) | Out-Null
            $arguments.Add($endTextOffset) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText `
                -AllowExpectedText
        }
        "accept_revision" {
            $revisionIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("revision_index", "index") `
                -Label $Label
            $arguments.Add("accept-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($revisionIndex) | Out-Null
        }
        "reject_revision" {
            $revisionIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("revision_index", "index") `
                -Label $Label
            $arguments.Add("reject-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($revisionIndex) | Out-Null
        }
        "set_revision_metadata" {
            $revisionIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("revision_index", "index") `
                -Label $Label
            $arguments.Add("set-revision-metadata") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($revisionIndex) | Out-Null
            Add-RevisionMetadataArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "apply_review_mutation_plan" {
            Add-ApplyReviewMutationPlanArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -InputPath $InputPath `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
        }
        "append_comment" {
            $arguments.Add("append-comment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-CommentAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireSelectedText `
                -RequireCommentText
        }
        "append_comment_reply" {
            $parentCommentIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("parent_comment_index", "comment_index", "parent_index", "index") `
                -Label $Label
            $arguments.Add("append-comment-reply") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($parentCommentIndex) | Out-Null
            Add-CommentAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireCommentText
        }
        "append_footnote" {
            $arguments.Add("append-footnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireReferenceText `
                -RequireNoteText
        }
        "replace_footnote" {
            $noteIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("note_index", "footnote_index", "index") `
                -Label $Label
            $arguments.Add("replace-footnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($noteIndex) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireNoteText
        }
        "remove_footnote" {
            $noteIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("note_index", "footnote_index", "index") `
                -Label $Label
            $arguments.Add("remove-footnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($noteIndex) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "append_endnote" {
            $arguments.Add("append-endnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireReferenceText `
                -RequireNoteText
        }
        "replace_endnote" {
            $noteIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("note_index", "endnote_index", "index") `
                -Label $Label
            $arguments.Add("replace-endnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($noteIndex) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireNoteText
        }
        "remove_endnote" {
            $noteIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("note_index", "endnote_index", "index") `
                -Label $Label
            $arguments.Add("remove-endnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($noteIndex) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "append_paragraph_text_comment" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $textOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_offset", "offset") `
                -Label $Label
            $textLength = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_length", "length") `
                -Label $Label
            $arguments.Add("append-paragraph-text-comment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($textOffset) | Out-Null
            $arguments.Add($textLength) | Out-Null
            Add-CommentAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireCommentText
        }
        "append_text_range_comment" {
            $startParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_paragraph_index", "paragraph_index") `
                -Label $Label
            $startTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_text_offset", "start_offset") `
                -Label $Label
            $endParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_paragraph_index") `
                -Label $Label
            $endTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_text_offset", "end_offset") `
                -Label $Label
            $arguments.Add("append-text-range-comment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($startParagraphIndex) | Out-Null
            $arguments.Add($startTextOffset) | Out-Null
            $arguments.Add($endParagraphIndex) | Out-Null
            $arguments.Add($endTextOffset) | Out-Null
            Add-CommentAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireCommentText
        }
        "set_paragraph_text_comment_range" {
            $commentIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("comment_index", "index") `
                -Label $Label
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $textOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_offset", "offset") `
                -Label $Label
            $textLength = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_length", "length") `
                -Label $Label
            $arguments.Add("set-paragraph-text-comment-range") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($commentIndex) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($textOffset) | Out-Null
            $arguments.Add($textLength) | Out-Null
        }
        "set_text_range_comment_range" {
            $commentIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("comment_index", "index") `
                -Label $Label
            $startParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_paragraph_index", "paragraph_index") `
                -Label $Label
            $startTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_text_offset", "start_offset") `
                -Label $Label
            $endParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_paragraph_index") `
                -Label $Label
            $endTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_text_offset", "end_offset") `
                -Label $Label
            $arguments.Add("set-text-range-comment-range") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($commentIndex) | Out-Null
            $arguments.Add($startParagraphIndex) | Out-Null
            $arguments.Add($startTextOffset) | Out-Null
            $arguments.Add($endParagraphIndex) | Out-Null
            $arguments.Add($endTextOffset) | Out-Null
        }
        default {
            return $false
        }
    }

    return $true
}
