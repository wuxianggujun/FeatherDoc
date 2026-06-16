<#
.SYNOPSIS
Initializes paths, child script arguments, and the Word visual release gate summary.
#>

$repoRoot = Resolve-RepoRoot

if (
    $SkipSmoke -and
    $SkipFixedGrid -and
    $SkipSectionPageSetup -and
    $SkipPageNumberFields -and
    $SkipBookmarkFloatingImage -and
    $SkipBookmarkImage -and
    $SkipBookmarkBlockVisibility -and
    $SkipTemplateBookmarkParagraphs -and
    $SkipBookmarkTableReplacement -and
    $SkipParagraphList -and
    $SkipParagraphNumbering -and
    $SkipParagraphRunStyle -and
    $SkipParagraphStyleNumbering -and
    $SkipFillBookmarks -and
    $SkipAppendImage -and
    $SkipFloatingImageZOrder -and
    $SkipTableRow -and
    $SkipTableRowHeight -and
    $SkipTableRowCantSplit -and
    $SkipTableRowRepeatHeader -and
    $SkipTableCellFill -and
    $SkipTableCellBorder -and
    $SkipTableCellWidth -and
    $SkipTableCellMargin -and
    $SkipTableCellVerticalAlignment -and
    $SkipTableCellTextDirection -and
    $SkipTableCellMerge -and
    $SkipTemplateBookmarkMultiline -and
    $SkipSectionTextMultiline -and
    $SkipRemoveBookmarkBlock -and
    $SkipTemplateBookmarkParagraphsPagination -and
    $SkipSectionOrder -and
    $SkipSectionPartRefs -and
    $SkipRunFontLanguage -and
    $SkipEnsureStyle -and
    (-not $IncludeTableStyleQuality.IsPresent) -and
    $SkipTemplateTableCliBookmark -and
    $SkipTemplateTableCliColumn -and
    $SkipTemplateTableCliDirectColumn -and
    $SkipTemplateTableCli -and
    $SkipTemplateTableCliMergeUnmerge -and
    $SkipTemplateTableCliDirect -and
    $SkipTemplateTableCliDirectMergeUnmerge -and
    $SkipTemplateTableCliSectionKind -and
    $SkipTemplateTableCliSectionKindRow -and
    $SkipTemplateTableCliSectionKindColumn -and
    $SkipTemplateTableCliSectionKindMergeUnmerge -and
    $SkipTemplateTableCliSelector -and
    $SkipReplaceRemoveImage
) {
    throw "At least one release-gate flow must remain enabled."
}

$resolvedGateOutputDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $GateOutputDir
$resolvedTaskOutputRoot = Resolve-FullPath -RepoRoot $repoRoot -InputPath $TaskOutputRoot
$resolvedReadmeAssetsDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReadmeAssetsDir
$resolvedSmokeOutputDir = Join-Path $resolvedGateOutputDir "smoke"
$resolvedFixedGridOutputDir = Join-Path $resolvedGateOutputDir "fixed-grid"
$resolvedSectionPageSetupOutputDir = Join-Path $resolvedGateOutputDir "section-page-setup"
$resolvedPageNumberFieldsOutputDir = Join-Path $resolvedGateOutputDir "page-number-fields"
$resolvedBookmarkFloatingImageOutputDir = Join-Path $resolvedGateOutputDir "bookmark-floating-image"
$resolvedBookmarkImageOutputDir = Join-Path $resolvedGateOutputDir "bookmark-image"
$resolvedBookmarkBlockVisibilityOutputDir = Join-Path $resolvedGateOutputDir "bookmark-block-visibility"
$resolvedTemplateBookmarkParagraphsOutputDir = Join-Path $resolvedGateOutputDir "template-bookmark-paragraphs"
$resolvedBookmarkTableReplacementOutputDir = Join-Path $resolvedGateOutputDir "bookmark-table-replacement"
$resolvedParagraphListOutputDir = Join-Path $resolvedGateOutputDir "paragraph-list"
$resolvedParagraphNumberingOutputDir = Join-Path $resolvedGateOutputDir "paragraph-numbering"
$resolvedParagraphRunStyleOutputDir = Join-Path $resolvedGateOutputDir "paragraph-run-style"
$resolvedParagraphStyleNumberingOutputDir = Join-Path $resolvedGateOutputDir "paragraph-style-numbering"
$resolvedFillBookmarksOutputDir = Join-Path $resolvedGateOutputDir "fill-bookmarks"
$resolvedAppendImageOutputDir = Join-Path $resolvedGateOutputDir "append-image"
$resolvedFloatingImageZOrderOutputDir = Join-Path $resolvedGateOutputDir "floating-image-z-order"
$resolvedTableRowOutputDir = Join-Path $resolvedGateOutputDir "table-row"
$resolvedTableRowHeightOutputDir = Join-Path $resolvedGateOutputDir "table-row-height"
$resolvedTableRowCantSplitOutputDir = Join-Path $resolvedGateOutputDir "table-row-cant-split"
$resolvedTableRowRepeatHeaderOutputDir = Join-Path $resolvedGateOutputDir "table-row-repeat-header"
$resolvedTableCellFillOutputDir = Join-Path $resolvedGateOutputDir "table-cell-fill"
$resolvedTableCellBorderOutputDir = Join-Path $resolvedGateOutputDir "table-cell-border"
$resolvedTableCellWidthOutputDir = Join-Path $resolvedGateOutputDir "table-cell-width"
$resolvedTableCellMarginOutputDir = Join-Path $resolvedGateOutputDir "table-cell-margin"
$resolvedTableCellVerticalAlignmentOutputDir = Join-Path $resolvedGateOutputDir "table-cell-vertical-alignment"
$resolvedTableCellTextDirectionOutputDir = Join-Path $resolvedGateOutputDir "table-cell-text-direction"
$resolvedTableCellMergeOutputDir = Join-Path $resolvedGateOutputDir "table-cell-merge"
$resolvedTemplateBookmarkMultilineOutputDir = Join-Path $resolvedGateOutputDir "template-bookmark-multiline"
$resolvedSectionTextMultilineOutputDir = Join-Path $resolvedGateOutputDir "section-text-multiline"
$resolvedRemoveBookmarkBlockOutputDir = Join-Path $resolvedGateOutputDir "remove-bookmark-block"
$resolvedTemplateBookmarkParagraphsPaginationOutputDir = Join-Path $resolvedGateOutputDir "template-bookmark-paragraphs-pagination"
$resolvedSectionOrderOutputDir = Join-Path $resolvedGateOutputDir "section-order"
$resolvedSectionPartRefsOutputDir = Join-Path $resolvedGateOutputDir "section-part-refs"
$resolvedRunFontLanguageOutputDir = Join-Path $resolvedGateOutputDir "run-font-language"
$resolvedEnsureStyleOutputDir = Join-Path $resolvedGateOutputDir "ensure-style"
$resolvedTableStyleQualityOutputDir = Join-Path $resolvedGateOutputDir "table-style-quality"
$resolvedTemplateTableCliBookmarkOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-bookmark"
$resolvedTemplateTableCliColumnOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-column"
$resolvedTemplateTableCliDirectColumnOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-direct-column"
$resolvedTemplateTableCliOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli"
$resolvedTemplateTableCliMergeUnmergeOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-merge-unmerge"
$resolvedTemplateTableCliDirectOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-direct"
$resolvedTemplateTableCliDirectMergeUnmergeOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-direct-merge-unmerge"
$resolvedTemplateTableCliSectionKindOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-section-kind"
$resolvedTemplateTableCliSectionKindRowOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-section-kind-row"
$resolvedTemplateTableCliSectionKindColumnOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-section-kind-column"
$resolvedTemplateTableCliSectionKindMergeUnmergeOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-section-kind-merge-unmerge"
$resolvedTemplateTableCliSelectorOutputDir = Join-Path $resolvedGateOutputDir "template-table-cli-selector"
$resolvedReplaceRemoveImageOutputDir = Join-Path $resolvedGateOutputDir "replace-remove-image"
$reportDir = Join-Path $resolvedGateOutputDir "report"
$gateSummaryPath = Join-Path $reportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $reportDir "gate_final_review.md"

New-Item -ItemType Directory -Path $resolvedGateOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $resolvedTaskOutputRoot -Force | Out-Null

$smokeOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedSmokeOutputDir -Label "Smoke output directory"
$fixedGridOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedFixedGridOutputDir -Label "Fixed-grid output directory"
$sectionPageSetupOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedSectionPageSetupOutputDir -Label "Section page setup output directory"
$pageNumberFieldsOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedPageNumberFieldsOutputDir -Label "Page number fields output directory"
$bookmarkFloatingImageOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedBookmarkFloatingImageOutputDir -Label "Bookmark floating image output directory"
$bookmarkImageOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedBookmarkImageOutputDir -Label "Bookmark image output directory"
$bookmarkBlockVisibilityOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedBookmarkBlockVisibilityOutputDir -Label "Bookmark block visibility output directory"
$templateBookmarkParagraphsOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateBookmarkParagraphsOutputDir -Label "Template bookmark paragraphs output directory"
$bookmarkTableReplacementOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedBookmarkTableReplacementOutputDir -Label "Bookmark table replacement output directory"
$paragraphListOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedParagraphListOutputDir -Label "Paragraph list output directory"
$paragraphNumberingOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedParagraphNumberingOutputDir -Label "Paragraph numbering output directory"
$paragraphRunStyleOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedParagraphRunStyleOutputDir -Label "Paragraph run style output directory"
$paragraphStyleNumberingOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedParagraphStyleNumberingOutputDir -Label "Paragraph style numbering output directory"
$fillBookmarksOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedFillBookmarksOutputDir -Label "Fill bookmarks output directory"
$appendImageOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedAppendImageOutputDir -Label "Append image output directory"
$floatingImageZOrderOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedFloatingImageZOrderOutputDir -Label "Floating image z-order output directory"
$tableRowOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableRowOutputDir -Label "Table row output directory"
$tableRowHeightOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableRowHeightOutputDir -Label "Table row height output directory"
$tableRowCantSplitOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableRowCantSplitOutputDir -Label "Table row cant split output directory"
$tableRowRepeatHeaderOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableRowRepeatHeaderOutputDir -Label "Table row repeat header output directory"
$tableCellFillOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableCellFillOutputDir -Label "Table cell fill output directory"
$tableCellBorderOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableCellBorderOutputDir -Label "Table cell border output directory"
$tableCellWidthOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableCellWidthOutputDir -Label "Table cell width output directory"
$tableCellMarginOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableCellMarginOutputDir -Label "Table cell margin output directory"
$tableCellVerticalAlignmentOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableCellVerticalAlignmentOutputDir -Label "Table cell vertical alignment output directory"
$tableCellTextDirectionOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableCellTextDirectionOutputDir -Label "Table cell text direction output directory"
$tableCellMergeOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableCellMergeOutputDir -Label "Table cell merge output directory"
$templateBookmarkMultilineOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateBookmarkMultilineOutputDir -Label "Template bookmark multiline output directory"
$sectionTextMultilineOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedSectionTextMultilineOutputDir -Label "Section text multiline output directory"
$removeBookmarkBlockOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedRemoveBookmarkBlockOutputDir -Label "Remove bookmark block output directory"
$templateBookmarkParagraphsPaginationOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateBookmarkParagraphsPaginationOutputDir -Label "Template bookmark paragraphs pagination output directory"
$sectionOrderOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedSectionOrderOutputDir -Label "Section order output directory"
$sectionPartRefsOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedSectionPartRefsOutputDir -Label "Section part refs output directory"
$runFontLanguageOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedRunFontLanguageOutputDir -Label "Run font language output directory"
$ensureStyleOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedEnsureStyleOutputDir -Label "Ensure style output directory"
$tableStyleQualityOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTableStyleQualityOutputDir -Label "Table style quality output directory"
$templateTableCliBookmarkOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliBookmarkOutputDir -Label "Template table CLI bookmark output directory"
$templateTableCliColumnOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliColumnOutputDir -Label "Template table CLI column output directory"
$templateTableCliDirectColumnOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliDirectColumnOutputDir -Label "Template table CLI direct column output directory"
$templateTableCliOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliOutputDir -Label "Template table CLI output directory"
$templateTableCliMergeUnmergeOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliMergeUnmergeOutputDir -Label "Template table CLI merge/unmerge output directory"
$templateTableCliDirectOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliDirectOutputDir -Label "Template table CLI direct output directory"
$templateTableCliDirectMergeUnmergeOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliDirectMergeUnmergeOutputDir -Label "Template table CLI direct merge/unmerge output directory"
$templateTableCliSectionKindOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliSectionKindOutputDir -Label "Template table CLI section-kind output directory"
$templateTableCliSectionKindRowOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliSectionKindRowOutputDir -Label "Template table CLI section-kind row output directory"
$templateTableCliSectionKindColumnOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliSectionKindColumnOutputDir -Label "Template table CLI section-kind column output directory"
$templateTableCliSectionKindMergeUnmergeOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliSectionKindMergeUnmergeOutputDir -Label "Template table CLI section-kind merge/unmerge output directory"
$templateTableCliSelectorOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTemplateTableCliSelectorOutputDir -Label "Template table CLI selector output directory"
$replaceRemoveImageOutputDirForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedReplaceRemoveImageOutputDir -Label "Replace/remove image output directory"
$taskOutputRootForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTaskOutputRoot -Label "Task output root"

$smokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$fixedGridScript = Join-Path $repoRoot "scripts\run_fixed_grid_merge_unmerge_regression.ps1"
$sectionPageSetupScript = Join-Path $repoRoot "scripts\run_section_page_setup_visual_regression.ps1"
$pageNumberFieldsScript = Join-Path $repoRoot "scripts\run_page_number_fields_visual_regression.ps1"
$bookmarkFloatingImageScript = Join-Path $repoRoot "scripts\run_bookmark_floating_image_visual_regression.ps1"
$bookmarkImageScript = Join-Path $repoRoot "scripts\run_bookmark_image_visual_regression.ps1"
$bookmarkBlockVisibilityScript = Join-Path $repoRoot "scripts\run_bookmark_block_visibility_visual_regression.ps1"
$templateBookmarkParagraphsScript = Join-Path $repoRoot "scripts\run_template_bookmark_paragraphs_visual_regression.ps1"
$bookmarkTableReplacementScript = Join-Path $repoRoot "scripts\run_bookmark_table_replacement_visual_regression.ps1"
$paragraphListScript = Join-Path $repoRoot "scripts\run_paragraph_list_visual_regression.ps1"
$paragraphNumberingScript = Join-Path $repoRoot "scripts\run_paragraph_numbering_visual_regression.ps1"
$paragraphRunStyleScript = Join-Path $repoRoot "scripts\run_paragraph_run_style_visual_regression.ps1"
$paragraphStyleNumberingScript = Join-Path $repoRoot "scripts\run_paragraph_style_numbering_visual_regression.ps1"
$fillBookmarksScript = Join-Path $repoRoot "scripts\run_fill_bookmarks_visual_regression.ps1"
$appendImageScript = Join-Path $repoRoot "scripts\run_append_image_visual_regression.ps1"
$floatingImageZOrderScript = Join-Path $repoRoot "scripts\run_floating_image_z_order_visual_regression.ps1"
$tableRowScript = Join-Path $repoRoot "scripts\run_table_row_visual_regression.ps1"
$tableRowHeightScript = Join-Path $repoRoot "scripts\run_table_row_height_visual_regression.ps1"
$tableRowCantSplitScript = Join-Path $repoRoot "scripts\run_table_row_cant_split_visual_regression.ps1"
$tableRowRepeatHeaderScript = Join-Path $repoRoot "scripts\run_table_row_repeat_header_visual_regression.ps1"
$tableCellFillScript = Join-Path $repoRoot "scripts\run_table_cell_fill_visual_regression.ps1"
$tableCellBorderScript = Join-Path $repoRoot "scripts\run_table_cell_border_visual_regression.ps1"
$tableCellWidthScript = Join-Path $repoRoot "scripts\run_table_cell_width_visual_regression.ps1"
$tableCellMarginScript = Join-Path $repoRoot "scripts\run_table_cell_margin_visual_regression.ps1"
$tableCellVerticalAlignmentScript = Join-Path $repoRoot "scripts\run_table_cell_vertical_alignment_visual_regression.ps1"
$tableCellTextDirectionScript = Join-Path $repoRoot "scripts\run_table_cell_text_direction_visual_regression.ps1"
$tableCellMergeScript = Join-Path $repoRoot "scripts\run_table_cell_merge_visual_regression.ps1"
$templateBookmarkMultilineScript = Join-Path $repoRoot "scripts\run_template_bookmark_multiline_visual_regression.ps1"
$sectionTextMultilineScript = Join-Path $repoRoot "scripts\run_section_text_multiline_visual_regression.ps1"
$removeBookmarkBlockScript = Join-Path $repoRoot "scripts\run_remove_bookmark_block_visual_regression.ps1"
$templateBookmarkParagraphsPaginationScript = Join-Path $repoRoot "scripts\run_template_bookmark_paragraphs_pagination_visual_regression.ps1"
$sectionOrderScript = Join-Path $repoRoot "scripts\run_section_order_visual_regression.ps1"
$sectionPartRefsScript = Join-Path $repoRoot "scripts\run_section_part_refs_visual_regression.ps1"
$runFontLanguageScript = Join-Path $repoRoot "scripts\run_run_font_language_visual_regression.ps1"
$ensureStyleScript = Join-Path $repoRoot "scripts\run_ensure_style_visual_regression.ps1"
$tableStyleQualityScript = Join-Path $repoRoot "scripts\run_table_style_quality_visual_regression.ps1"
$templateTableCliBookmarkScript = Join-Path $repoRoot "scripts\run_template_table_cli_bookmark_visual_regression.ps1"
$templateTableCliColumnScript = Join-Path $repoRoot "scripts\run_template_table_cli_column_visual_regression.ps1"
$templateTableCliDirectColumnScript = Join-Path $repoRoot "scripts\run_template_table_cli_direct_column_visual_regression.ps1"
$templateTableCliScript = Join-Path $repoRoot "scripts\run_template_table_cli_visual_regression.ps1"
$templateTableCliMergeUnmergeScript = Join-Path $repoRoot "scripts\run_template_table_cli_merge_unmerge_visual_regression.ps1"
$templateTableCliDirectScript = Join-Path $repoRoot "scripts\run_template_table_cli_direct_visual_regression.ps1"
$templateTableCliDirectMergeUnmergeScript = Join-Path $repoRoot "scripts\run_template_table_cli_direct_merge_unmerge_visual_regression.ps1"
$templateTableCliSectionKindScript = Join-Path $repoRoot "scripts\run_template_table_cli_section_kind_visual_regression.ps1"
$templateTableCliSectionKindRowScript = Join-Path $repoRoot "scripts\run_template_table_cli_section_kind_row_visual_regression.ps1"
$templateTableCliSectionKindColumnScript = Join-Path $repoRoot "scripts\run_template_table_cli_section_kind_column_visual_regression.ps1"
$templateTableCliSectionKindMergeUnmergeScript = Join-Path $repoRoot "scripts\run_template_table_cli_section_kind_merge_unmerge_visual_regression.ps1"
$templateTableCliSelectorScript = Join-Path $repoRoot "scripts\run_template_table_cli_selector_visual_regression.ps1"
$replaceRemoveImageScript = Join-Path $repoRoot "scripts\run_replace_remove_image_visual_regression.ps1"
$prepareTaskScript = Join-Path $repoRoot "scripts\prepare_word_review_task.ps1"
$refreshReadmeAssetsScript = Join-Path $repoRoot "scripts\refresh_readme_visual_assets.ps1"

$gateSummary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    gate_output_dir = $resolvedGateOutputDir
    report_dir = $reportDir
    review_mode = if ($SkipReviewTasks) { "" } else { $ReviewMode }
    skip_build = $SkipBuild.IsPresent
    execution_status = "pass"
    visual_verdict = if ($SkipReviewTasks) {
        "evidence_only"
    } else {
        "pending_manual_review"
    }
    smoke = $null
    fixed_grid = $null
    section_page_setup = $null
    page_number_fields = $null
    curated_visual_regressions = @()
    review_task_summary = $null
    review_tasks = [ordered]@{
        document = $null
        fixed_grid = $null
        section_page_setup = $null
        page_number_fields = $null
        curated_visual_regressions = @()
    }
}
