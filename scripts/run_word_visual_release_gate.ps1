<#
.SYNOPSIS
Runs the local Word visual release gate for FeatherDoc.

.DESCRIPTION
Executes the standard smoke flow, the fixed-grid merge/unmerge regression
bundle, the section page setup regression bundle, the page number fields
regression bundle, selected curated visual regression bundles, packages
screenshot-backed review tasks, and can optionally refresh the repository
README gallery assets from the generated evidence.

.PARAMETER RefreshReadmeAssets
Copies the latest visual smoke and fixed-grid evidence into
docs/assets/readme. This requires both flows plus review-task packaging.

.PARAMETER ReadmeAssetsDir
Target repository directory for the refreshed README gallery PNG files.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1 `
    -SmokeBuildDir build-msvc-nmake `
    -FixedGridBuildDir build-msvc-nmake `
    -SectionPageSetupBuildDir build-msvc-nmake `
    -PageNumberFieldsBuildDir build-msvc-nmake `
    -SkipBuild `
    -GateOutputDir output/word-visual-release-gate

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1 `
    -SmokeBuildDir build-msvc-nmake `
    -FixedGridBuildDir build-msvc-nmake `
    -SectionPageSetupBuildDir build-msvc-nmake `
    -PageNumberFieldsBuildDir build-msvc-nmake `
    -SkipBuild `
    -GateOutputDir output/word-visual-release-gate `
    -TaskOutputRoot output/word-visual-smoke/tasks-release-gate `
    -RefreshReadmeAssets

.PARAMETER SmokeReviewVerdict
Optional screenshot-backed verdict to pass into the standard Word visual smoke
flow. Use this when the smoke contact sheet is reviewed during the same gate run.

.PARAMETER SmokeReviewNote
Optional note recorded with SmokeReviewVerdict in the smoke review artifacts.

.PARAMETER FixedGridReviewVerdict
Optional screenshot-backed verdict to seed into the fixed-grid review task.

.PARAMETER FixedGridReviewNote
Optional note recorded with FixedGridReviewVerdict in the fixed-grid review task.

.PARAMETER SectionPageSetupReviewVerdict
Optional screenshot-backed verdict to seed into the section page setup review task.

.PARAMETER SectionPageSetupReviewNote
Optional note recorded with SectionPageSetupReviewVerdict in the section page
setup review task.

.PARAMETER PageNumberFieldsReviewVerdict
Optional screenshot-backed verdict to seed into the page number fields review
task.

.PARAMETER PageNumberFieldsReviewNote
Optional note recorded with PageNumberFieldsReviewVerdict in the page number
fields review task.

.PARAMETER CuratedVisualReviewVerdict
Optional screenshot-backed verdict to seed into every curated visual regression
review task.

.PARAMETER CuratedVisualReviewNote
Optional note recorded with CuratedVisualReviewVerdict in each curated visual
regression review task.

#>
param(
    [string]$GateOutputDir = "output/word-visual-release-gate",
    [string]$SmokeBuildDir = "build-word-visual-smoke-nmake",
    [string]$FixedGridBuildDir = "build-fixed-grid-regression-nmake",
    [string]$SectionPageSetupBuildDir = "build-section-page-setup-regression-nmake",
    [string]$PageNumberFieldsBuildDir = "build-page-number-fields-regression-nmake",
    [string]$BookmarkFloatingImageBuildDir = "build-msvc-nmake-bookmark-floating-image-visual",
    [string]$BookmarkImageBuildDir = "build-msvc-nmake-list-visual",
    [string]$BookmarkBlockVisibilityBuildDir = "build-codex-clang-compat",
    [string]$TemplateBookmarkParagraphsBuildDir = "build-codex-clang-compat",
    [string]$BookmarkTableReplacementBuildDir = "build-codex-clang-compat",
    [string]$ParagraphListBuildDir = "build-paragraph-list-visual-nmake",
    [string]$ParagraphNumberingBuildDir = "build-paragraph-numbering-visual-nmake",
    [string]$ParagraphRunStyleBuildDir = "build-paragraph-run-style-visual-nmake",
    [string]$ParagraphStyleNumberingBuildDir = "build-paragraph-style-numbering-visual-nmake",
    [string]$FillBookmarksBuildDir = "build-fill-bookmarks-visual-nmake",
    [string]$AppendImageBuildDir = "build-append-image-visual-nmake",
    [string]$FloatingImageZOrderBuildDir = "build-floating-image-z-order-visual-nmake",
    [string]$TableRowBuildDir = "build-table-row-visual-nmake",
    [string]$TableRowHeightBuildDir = "build-table-row-height-visual-nmake",
    [string]$TableRowCantSplitBuildDir = "build-table-row-cant-split-visual-nmake",
    [string]$TableRowRepeatHeaderBuildDir = "build-table-row-repeat-header-visual-nmake",
    [string]$TableCellFillBuildDir = "build-table-cell-fill-visual-nmake",
    [string]$TableCellBorderBuildDir = "build-table-cell-border-visual-nmake",
    [string]$TableCellWidthBuildDir = "build-table-cell-width-visual-nmake",
    [string]$TableCellMarginBuildDir = "build-table-cell-margin-visual-nmake",
    [string]$TableCellVerticalAlignmentBuildDir = "build-table-cell-vertical-alignment-visual-nmake",
    [string]$TableCellTextDirectionBuildDir = "build-table-cell-text-direction-visual-nmake",
    [string]$TableCellMergeBuildDir = "build-table-cell-merge-visual-nmake",
    [string]$TemplateBookmarkMultilineBuildDir = "build-codex-clang-compat",
    [string]$SectionTextMultilineBuildDir = "build-codex-clang-compat",
    [string]$RemoveBookmarkBlockBuildDir = "build-codex-clang-compat",
    [string]$TemplateBookmarkParagraphsPaginationBuildDir = "build-codex-clang-compat",
    [string]$SectionOrderBuildDir = "build-section-order-visual-nmake",
    [string]$SectionPartRefsBuildDir = "build-section-part-refs-visual-nmake",
    [string]$RunFontLanguageBuildDir = "build-run-font-language-visual-nmake",
    [string]$EnsureStyleBuildDir = "build-ensure-style-visual-nmake",
    [string]$TableStyleQualityBuildDir = "build-codex-clang-compat",
    [string]$TemplateTableCliBookmarkBuildDir = "build-template-table-cli-bookmark-visual-nmake",
    [string]$TemplateTableCliColumnBuildDir = "build-template-table-cli-column-visual-nmake",
    [string]$TemplateTableCliDirectColumnBuildDir = "build-template-table-cli-direct-column-visual-nmake",
    [string]$TemplateTableCliBuildDir = "build-ttcli-visual",
    [string]$TemplateTableCliMergeUnmergeBuildDir = "build-ttcli-merge-visual",
    [string]$TemplateTableCliDirectBuildDir = "build-ttcli-direct-visual",
    [string]$TemplateTableCliDirectMergeUnmergeBuildDir = "build-ttcli-direct-merge-visual",
    [string]$TemplateTableCliSectionKindBuildDir = "build-ttcli-section-kind-visual",
    [string]$TemplateTableCliSectionKindRowBuildDir = "build-ttcli-section-kind-row-visual",
    [string]$TemplateTableCliSectionKindColumnBuildDir = "build-ttcli-section-kind-column-visual",
    [string]$TemplateTableCliSectionKindMergeUnmergeBuildDir = "build-ttcli-section-kind-merge-visual",
    [string]$TemplateTableCliSelectorBuildDir = "build-ttcli-selector-visual",
    [string]$ReplaceRemoveImageBuildDir = "build-image-mutate-visual-nmake",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipSmoke,
    [switch]$SkipFixedGrid,
    [switch]$SkipSectionPageSetup,
    [switch]$SkipPageNumberFields,
    [switch]$SkipBookmarkFloatingImage,
    [switch]$SkipBookmarkImage,
    [switch]$SkipBookmarkBlockVisibility,
    [switch]$SkipTemplateBookmarkParagraphs,
    [switch]$SkipBookmarkTableReplacement,
    [switch]$SkipParagraphList,
    [switch]$SkipParagraphNumbering,
    [switch]$SkipParagraphRunStyle,
    [switch]$SkipParagraphStyleNumbering,
    [switch]$SkipFillBookmarks,
    [switch]$SkipAppendImage,
    [switch]$SkipFloatingImageZOrder,
    [switch]$SkipTableRow,
    [switch]$SkipTableRowHeight,
    [switch]$SkipTableRowCantSplit,
    [switch]$SkipTableRowRepeatHeader,
    [switch]$SkipTableCellFill,
    [switch]$SkipTableCellBorder,
    [switch]$SkipTableCellWidth,
    [switch]$SkipTableCellMargin,
    [switch]$SkipTableCellVerticalAlignment,
    [switch]$SkipTableCellTextDirection,
    [switch]$SkipTableCellMerge,
    [switch]$SkipTemplateBookmarkMultiline,
    [switch]$SkipSectionTextMultiline,
    [switch]$SkipRemoveBookmarkBlock,
    [switch]$SkipTemplateBookmarkParagraphsPagination,
    [switch]$SkipSectionOrder,
    [switch]$SkipSectionPartRefs,
    [switch]$SkipRunFontLanguage,
    [switch]$SkipEnsureStyle,
    [switch]$IncludeTableStyleQuality,
    [switch]$SkipTemplateTableCliBookmark,
    [switch]$SkipTemplateTableCliColumn,
    [switch]$SkipTemplateTableCliDirectColumn,
    [switch]$SkipTemplateTableCli,
    [switch]$SkipTemplateTableCliMergeUnmerge,
    [switch]$SkipTemplateTableCliDirect,
    [switch]$SkipTemplateTableCliDirectMergeUnmerge,
    [switch]$SkipTemplateTableCliSectionKind,
    [switch]$SkipTemplateTableCliSectionKindRow,
    [switch]$SkipTemplateTableCliSectionKindColumn,
    [switch]$SkipTemplateTableCliSectionKindMergeUnmerge,
    [switch]$SkipTemplateTableCliSelector,
    [switch]$SkipReplaceRemoveImage,
    [switch]$SkipReviewTasks,
    [ValidateSet("review-only", "review-and-repair")]
    [string]$ReviewMode = "review-only",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$SmokeReviewVerdict = "undecided",
    [string]$SmokeReviewNote = "",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$FixedGridReviewVerdict = "undecided",
    [string]$FixedGridReviewNote = "",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$SectionPageSetupReviewVerdict = "undecided",
    [string]$SectionPageSetupReviewNote = "",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$PageNumberFieldsReviewVerdict = "undecided",
    [string]$PageNumberFieldsReviewNote = "",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$CuratedVisualReviewVerdict = "undecided",
    [string]$CuratedVisualReviewNote = "",
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks",
    [switch]$RefreshReadmeAssets,
    [string]$ReadmeAssetsDir = "docs/assets/readme",
    [switch]$OpenTaskDirs,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

$script:WordVisualReleaseGateScriptRoot = $PSScriptRoot
. (Join-Path $script:WordVisualReleaseGateScriptRoot "run_word_visual_release_gate_helpers.ps1")

. (Join-Path $script:WordVisualReleaseGateScriptRoot "run_word_visual_release_gate_setup.ps1")
. (Join-Path $script:WordVisualReleaseGateScriptRoot "run_word_visual_release_gate_descriptors.ps1")
. (Join-Path $script:WordVisualReleaseGateScriptRoot "run_word_visual_release_gate_standard_flows.ps1")
. (Join-Path $script:WordVisualReleaseGateScriptRoot "run_word_visual_release_gate_curated_report.ps1")
