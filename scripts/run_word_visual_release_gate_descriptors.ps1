<#
.SYNOPSIS
Declares curated visual flow descriptors and README refresh preconditions.
#>

$curatedVisualFlowDescriptors = @(
    [ordered]@{
        id = "bookmark-floating-image"
        label = "Bookmark floating image"
        skip = $SkipBookmarkFloatingImage.IsPresent
        build_dir = $BookmarkFloatingImageBuildDir
        output_dir = $resolvedBookmarkFloatingImageOutputDir
        output_dir_for_child = $bookmarkFloatingImageOutputDirForChild
        script_path = $bookmarkFloatingImageScript
    },
    [ordered]@{
        id = "bookmark-image"
        label = "Bookmark image"
        skip = $SkipBookmarkImage.IsPresent
        build_dir = $BookmarkImageBuildDir
        output_dir = $resolvedBookmarkImageOutputDir
        output_dir_for_child = $bookmarkImageOutputDirForChild
        script_path = $bookmarkImageScript
    },
    [ordered]@{
        id = "bookmark-block-visibility"
        label = "Bookmark block visibility"
        skip = $SkipBookmarkBlockVisibility.IsPresent
        build_dir = $BookmarkBlockVisibilityBuildDir
        output_dir = $resolvedBookmarkBlockVisibilityOutputDir
        output_dir_for_child = $bookmarkBlockVisibilityOutputDirForChild
        script_path = $bookmarkBlockVisibilityScript
    },
    [ordered]@{
        id = "template-bookmark-paragraphs"
        label = "Template bookmark paragraphs"
        skip = $SkipTemplateBookmarkParagraphs.IsPresent
        build_dir = $TemplateBookmarkParagraphsBuildDir
        output_dir = $resolvedTemplateBookmarkParagraphsOutputDir
        output_dir_for_child = $templateBookmarkParagraphsOutputDirForChild
        script_path = $templateBookmarkParagraphsScript
    },
    [ordered]@{
        id = "bookmark-table-replacement"
        label = "Bookmark table replacement"
        skip = $SkipBookmarkTableReplacement.IsPresent
        build_dir = $BookmarkTableReplacementBuildDir
        output_dir = $resolvedBookmarkTableReplacementOutputDir
        output_dir_for_child = $bookmarkTableReplacementOutputDirForChild
        script_path = $bookmarkTableReplacementScript
    },
    [ordered]@{
        id = "paragraph-list"
        label = "Paragraph list"
        skip = $SkipParagraphList.IsPresent
        build_dir = $ParagraphListBuildDir
        output_dir = $resolvedParagraphListOutputDir
        output_dir_for_child = $paragraphListOutputDirForChild
        script_path = $paragraphListScript
    },
    [ordered]@{
        id = "paragraph-numbering"
        label = "Paragraph numbering"
        skip = $SkipParagraphNumbering.IsPresent
        build_dir = $ParagraphNumberingBuildDir
        output_dir = $resolvedParagraphNumberingOutputDir
        output_dir_for_child = $paragraphNumberingOutputDirForChild
        script_path = $paragraphNumberingScript
    },
    [ordered]@{
        id = "paragraph-run-style"
        label = "Paragraph run style"
        skip = $SkipParagraphRunStyle.IsPresent
        build_dir = $ParagraphRunStyleBuildDir
        output_dir = $resolvedParagraphRunStyleOutputDir
        output_dir_for_child = $paragraphRunStyleOutputDirForChild
        script_path = $paragraphRunStyleScript
    },
    [ordered]@{
        id = "paragraph-style-numbering"
        label = "Paragraph style numbering"
        skip = $SkipParagraphStyleNumbering.IsPresent
        build_dir = $ParagraphStyleNumberingBuildDir
        output_dir = $resolvedParagraphStyleNumberingOutputDir
        output_dir_for_child = $paragraphStyleNumberingOutputDirForChild
        script_path = $paragraphStyleNumberingScript
    },
    [ordered]@{
        id = "fill-bookmarks"
        label = "Fill bookmarks"
        skip = $SkipFillBookmarks.IsPresent
        build_dir = $FillBookmarksBuildDir
        output_dir = $resolvedFillBookmarksOutputDir
        output_dir_for_child = $fillBookmarksOutputDirForChild
        script_path = $fillBookmarksScript
    },
    [ordered]@{
        id = "append-image"
        label = "Append image"
        skip = $SkipAppendImage.IsPresent
        build_dir = $AppendImageBuildDir
        output_dir = $resolvedAppendImageOutputDir
        output_dir_for_child = $appendImageOutputDirForChild
        script_path = $appendImageScript
    },
    [ordered]@{
        id = "floating-image-z-order"
        label = "Floating image z-order"
        skip = $SkipFloatingImageZOrder.IsPresent
        build_dir = $FloatingImageZOrderBuildDir
        output_dir = $resolvedFloatingImageZOrderOutputDir
        output_dir_for_child = $floatingImageZOrderOutputDirForChild
        script_path = $floatingImageZOrderScript
    },
    [ordered]@{
        id = "table-row"
        label = "Table row"
        skip = $SkipTableRow.IsPresent
        build_dir = $TableRowBuildDir
        output_dir = $resolvedTableRowOutputDir
        output_dir_for_child = $tableRowOutputDirForChild
        script_path = $tableRowScript
    },
    [ordered]@{
        id = "table-row-height"
        label = "Table row height"
        skip = $SkipTableRowHeight.IsPresent
        build_dir = $TableRowHeightBuildDir
        output_dir = $resolvedTableRowHeightOutputDir
        output_dir_for_child = $tableRowHeightOutputDirForChild
        script_path = $tableRowHeightScript
    },
    [ordered]@{
        id = "table-row-cant-split"
        label = "Table row cant split"
        skip = $SkipTableRowCantSplit.IsPresent
        build_dir = $TableRowCantSplitBuildDir
        output_dir = $resolvedTableRowCantSplitOutputDir
        output_dir_for_child = $tableRowCantSplitOutputDirForChild
        script_path = $tableRowCantSplitScript
    },
    [ordered]@{
        id = "table-row-repeat-header"
        label = "Table row repeat header"
        skip = $SkipTableRowRepeatHeader.IsPresent
        build_dir = $TableRowRepeatHeaderBuildDir
        output_dir = $resolvedTableRowRepeatHeaderOutputDir
        output_dir_for_child = $tableRowRepeatHeaderOutputDirForChild
        script_path = $tableRowRepeatHeaderScript
    },
    [ordered]@{
        id = "table-cell-fill"
        label = "Table cell fill"
        skip = $SkipTableCellFill.IsPresent
        build_dir = $TableCellFillBuildDir
        output_dir = $resolvedTableCellFillOutputDir
        output_dir_for_child = $tableCellFillOutputDirForChild
        script_path = $tableCellFillScript
    },
    [ordered]@{
        id = "table-cell-border"
        label = "Table cell border"
        skip = $SkipTableCellBorder.IsPresent
        build_dir = $TableCellBorderBuildDir
        output_dir = $resolvedTableCellBorderOutputDir
        output_dir_for_child = $tableCellBorderOutputDirForChild
        script_path = $tableCellBorderScript
    },
    [ordered]@{
        id = "table-cell-width"
        label = "Table cell width"
        skip = $SkipTableCellWidth.IsPresent
        build_dir = $TableCellWidthBuildDir
        output_dir = $resolvedTableCellWidthOutputDir
        output_dir_for_child = $tableCellWidthOutputDirForChild
        script_path = $tableCellWidthScript
    },
    [ordered]@{
        id = "table-cell-margin"
        label = "Table cell margin"
        skip = $SkipTableCellMargin.IsPresent
        build_dir = $TableCellMarginBuildDir
        output_dir = $resolvedTableCellMarginOutputDir
        output_dir_for_child = $tableCellMarginOutputDirForChild
        script_path = $tableCellMarginScript
    },
    [ordered]@{
        id = "table-cell-vertical-alignment"
        label = "Table cell vertical alignment"
        skip = $SkipTableCellVerticalAlignment.IsPresent
        build_dir = $TableCellVerticalAlignmentBuildDir
        output_dir = $resolvedTableCellVerticalAlignmentOutputDir
        output_dir_for_child = $tableCellVerticalAlignmentOutputDirForChild
        script_path = $tableCellVerticalAlignmentScript
    },
    [ordered]@{
        id = "table-cell-text-direction"
        label = "Table cell text direction"
        skip = $SkipTableCellTextDirection.IsPresent
        build_dir = $TableCellTextDirectionBuildDir
        output_dir = $resolvedTableCellTextDirectionOutputDir
        output_dir_for_child = $tableCellTextDirectionOutputDirForChild
        script_path = $tableCellTextDirectionScript
    },
    [ordered]@{
        id = "table-cell-merge"
        label = "Table cell merge"
        skip = $SkipTableCellMerge.IsPresent
        build_dir = $TableCellMergeBuildDir
        output_dir = $resolvedTableCellMergeOutputDir
        output_dir_for_child = $tableCellMergeOutputDirForChild
        script_path = $tableCellMergeScript
    },
    [ordered]@{
        id = "template-bookmark-multiline"
        label = "Template bookmark multiline"
        skip = $SkipTemplateBookmarkMultiline.IsPresent
        build_dir = $TemplateBookmarkMultilineBuildDir
        output_dir = $resolvedTemplateBookmarkMultilineOutputDir
        output_dir_for_child = $templateBookmarkMultilineOutputDirForChild
        script_path = $templateBookmarkMultilineScript
    },
    [ordered]@{
        id = "section-text-multiline"
        label = "Section text multiline"
        skip = $SkipSectionTextMultiline.IsPresent
        build_dir = $SectionTextMultilineBuildDir
        output_dir = $resolvedSectionTextMultilineOutputDir
        output_dir_for_child = $sectionTextMultilineOutputDirForChild
        script_path = $sectionTextMultilineScript
    },
    [ordered]@{
        id = "remove-bookmark-block"
        label = "Remove bookmark block"
        skip = $SkipRemoveBookmarkBlock.IsPresent
        build_dir = $RemoveBookmarkBlockBuildDir
        output_dir = $resolvedRemoveBookmarkBlockOutputDir
        output_dir_for_child = $removeBookmarkBlockOutputDirForChild
        script_path = $removeBookmarkBlockScript
    },
    [ordered]@{
        id = "template-bookmark-paragraphs-pagination"
        label = "Template bookmark paragraphs pagination"
        skip = $SkipTemplateBookmarkParagraphsPagination.IsPresent
        build_dir = $TemplateBookmarkParagraphsPaginationBuildDir
        output_dir = $resolvedTemplateBookmarkParagraphsPaginationOutputDir
        output_dir_for_child = $templateBookmarkParagraphsPaginationOutputDirForChild
        script_path = $templateBookmarkParagraphsPaginationScript
    },
    [ordered]@{
        id = "section-order"
        label = "Section order"
        skip = $SkipSectionOrder.IsPresent
        build_dir = $SectionOrderBuildDir
        output_dir = $resolvedSectionOrderOutputDir
        output_dir_for_child = $sectionOrderOutputDirForChild
        script_path = $sectionOrderScript
    },
    [ordered]@{
        id = "section-part-refs"
        label = "Section part refs"
        skip = $SkipSectionPartRefs.IsPresent
        build_dir = $SectionPartRefsBuildDir
        output_dir = $resolvedSectionPartRefsOutputDir
        output_dir_for_child = $sectionPartRefsOutputDirForChild
        script_path = $sectionPartRefsScript
    },
    [ordered]@{
        id = "run-font-language"
        label = "Run font language"
        skip = $SkipRunFontLanguage.IsPresent
        build_dir = $RunFontLanguageBuildDir
        output_dir = $resolvedRunFontLanguageOutputDir
        output_dir_for_child = $runFontLanguageOutputDirForChild
        script_path = $runFontLanguageScript
    },
    [ordered]@{
        id = "ensure-style"
        label = "Ensure style"
        skip = $SkipEnsureStyle.IsPresent
        build_dir = $EnsureStyleBuildDir
        output_dir = $resolvedEnsureStyleOutputDir
        output_dir_for_child = $ensureStyleOutputDirForChild
        script_path = $ensureStyleScript
    },
    [ordered]@{
        id = "table-style-quality"
        label = "Table style quality"
        skip = (-not $IncludeTableStyleQuality.IsPresent)
        build_dir = $TableStyleQualityBuildDir
        output_dir = $resolvedTableStyleQualityOutputDir
        output_dir_for_child = $tableStyleQualityOutputDirForChild
        script_path = $tableStyleQualityScript
    },
    [ordered]@{
        id = "template-table-cli-bookmark"
        label = "Template table CLI bookmark"
        skip = $SkipTemplateTableCliBookmark.IsPresent
        build_dir = $TemplateTableCliBookmarkBuildDir
        output_dir = $resolvedTemplateTableCliBookmarkOutputDir
        output_dir_for_child = $templateTableCliBookmarkOutputDirForChild
        script_path = $templateTableCliBookmarkScript
    },
    [ordered]@{
        id = "template-table-cli-column"
        label = "Template table CLI column"
        skip = $SkipTemplateTableCliColumn.IsPresent
        build_dir = $TemplateTableCliColumnBuildDir
        output_dir = $resolvedTemplateTableCliColumnOutputDir
        output_dir_for_child = $templateTableCliColumnOutputDirForChild
        script_path = $templateTableCliColumnScript
    },
    [ordered]@{
        id = "template-table-cli-direct-column"
        label = "Template table CLI direct column"
        skip = $SkipTemplateTableCliDirectColumn.IsPresent
        build_dir = $TemplateTableCliDirectColumnBuildDir
        output_dir = $resolvedTemplateTableCliDirectColumnOutputDir
        output_dir_for_child = $templateTableCliDirectColumnOutputDirForChild
        script_path = $templateTableCliDirectColumnScript
    },
    [ordered]@{
        id = "template-table-cli"
        label = "Template table CLI"
        skip = $SkipTemplateTableCli.IsPresent
        build_dir = $TemplateTableCliBuildDir
        output_dir = $resolvedTemplateTableCliOutputDir
        output_dir_for_child = $templateTableCliOutputDirForChild
        script_path = $templateTableCliScript
    },
    [ordered]@{
        id = "template-table-cli-merge-unmerge"
        label = "Template table CLI merge/unmerge"
        skip = $SkipTemplateTableCliMergeUnmerge.IsPresent
        build_dir = $TemplateTableCliMergeUnmergeBuildDir
        output_dir = $resolvedTemplateTableCliMergeUnmergeOutputDir
        output_dir_for_child = $templateTableCliMergeUnmergeOutputDirForChild
        script_path = $templateTableCliMergeUnmergeScript
    },
    [ordered]@{
        id = "template-table-cli-direct"
        label = "Template table CLI direct"
        skip = $SkipTemplateTableCliDirect.IsPresent
        build_dir = $TemplateTableCliDirectBuildDir
        output_dir = $resolvedTemplateTableCliDirectOutputDir
        output_dir_for_child = $templateTableCliDirectOutputDirForChild
        script_path = $templateTableCliDirectScript
    },
    [ordered]@{
        id = "template-table-cli-direct-merge-unmerge"
        label = "Template table CLI direct merge/unmerge"
        skip = $SkipTemplateTableCliDirectMergeUnmerge.IsPresent
        build_dir = $TemplateTableCliDirectMergeUnmergeBuildDir
        output_dir = $resolvedTemplateTableCliDirectMergeUnmergeOutputDir
        output_dir_for_child = $templateTableCliDirectMergeUnmergeOutputDirForChild
        script_path = $templateTableCliDirectMergeUnmergeScript
    },
    [ordered]@{
        id = "template-table-cli-section-kind"
        label = "Template table CLI section kind"
        skip = $SkipTemplateTableCliSectionKind.IsPresent
        build_dir = $TemplateTableCliSectionKindBuildDir
        output_dir = $resolvedTemplateTableCliSectionKindOutputDir
        output_dir_for_child = $templateTableCliSectionKindOutputDirForChild
        script_path = $templateTableCliSectionKindScript
    },
    [ordered]@{
        id = "template-table-cli-section-kind-row"
        label = "Template table CLI section kind row"
        skip = $SkipTemplateTableCliSectionKindRow.IsPresent
        build_dir = $TemplateTableCliSectionKindRowBuildDir
        output_dir = $resolvedTemplateTableCliSectionKindRowOutputDir
        output_dir_for_child = $templateTableCliSectionKindRowOutputDirForChild
        script_path = $templateTableCliSectionKindRowScript
    },
    [ordered]@{
        id = "template-table-cli-section-kind-column"
        label = "Template table CLI section kind column"
        skip = $SkipTemplateTableCliSectionKindColumn.IsPresent
        build_dir = $TemplateTableCliSectionKindColumnBuildDir
        output_dir = $resolvedTemplateTableCliSectionKindColumnOutputDir
        output_dir_for_child = $templateTableCliSectionKindColumnOutputDirForChild
        script_path = $templateTableCliSectionKindColumnScript
    },
    [ordered]@{
        id = "template-table-cli-section-kind-merge-unmerge"
        label = "Template table CLI section kind merge/unmerge"
        skip = $SkipTemplateTableCliSectionKindMergeUnmerge.IsPresent
        build_dir = $TemplateTableCliSectionKindMergeUnmergeBuildDir
        output_dir = $resolvedTemplateTableCliSectionKindMergeUnmergeOutputDir
        output_dir_for_child = $templateTableCliSectionKindMergeUnmergeOutputDirForChild
        script_path = $templateTableCliSectionKindMergeUnmergeScript
    },
    [ordered]@{
        id = "template-table-cli-selector"
        label = "Template table CLI selector"
        skip = $SkipTemplateTableCliSelector.IsPresent
        build_dir = $TemplateTableCliSelectorBuildDir
        output_dir = $resolvedTemplateTableCliSelectorOutputDir
        output_dir_for_child = $templateTableCliSelectorOutputDirForChild
        script_path = $templateTableCliSelectorScript
    },
    [ordered]@{
        id = "replace-remove-image"
        label = "Replace/remove image"
        skip = $SkipReplaceRemoveImage.IsPresent
        build_dir = $ReplaceRemoveImageBuildDir
        output_dir = $resolvedReplaceRemoveImageOutputDir
        output_dir_for_child = $replaceRemoveImageOutputDirForChild
        script_path = $replaceRemoveImageScript
    }
)

if ($RefreshReadmeAssets) {
    if ($SkipReviewTasks) {
        throw "README gallery refresh requires review tasks. Re-run without -SkipReviewTasks."
    }
    if ($SkipSmoke -or $SkipFixedGrid) {
        throw "README gallery refresh requires both smoke and fixed-grid flows. Re-run without -SkipSmoke and -SkipFixedGrid."
    }
}
