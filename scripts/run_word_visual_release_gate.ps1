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

function Write-Step {
    param([string]$Message)
    Write-Host "[word-visual-release-gate] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-FullPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    return [System.IO.Path]::GetFullPath($candidate)
}

function Get-RelativePathCompat {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $resolvedBase = [System.IO.Path]::GetFullPath($BasePath)
    $resolvedTarget = [System.IO.Path]::GetFullPath($TargetPath)

    if ([System.IO.Path]::GetPathRoot($resolvedBase) -ne
        [System.IO.Path]::GetPathRoot($resolvedTarget)) {
        return $resolvedTarget
    }

    if ((Test-Path $resolvedBase -PathType Container) -and
        -not $resolvedBase.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedBase.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedBase += [System.IO.Path]::DirectorySeparatorChar
    }

    if ((Test-Path $resolvedTarget -PathType Container) -and
        -not $resolvedTarget.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedTarget.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedTarget += [System.IO.Path]::DirectorySeparatorChar
    }

    $baseUri = New-Object System.Uri($resolvedBase)
    $targetUri = New-Object System.Uri($resolvedTarget)
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)

    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace("/", "\")
}

function Convert-ToChildScriptPath {
    param(
        [string]$RepoRoot,
        [string]$TargetPath,
        [string]$Label
    )

    $relativePath = Get-RelativePathCompat -BasePath $RepoRoot -TargetPath $TargetPath
    if ([System.IO.Path]::IsPathRooted($relativePath)) {
        throw "$Label must stay on the same drive as the workspace: $TargetPath"
    }

    return $relativePath
}

function Invoke-ChildPowerShell {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments,
        [string]$FailureMessage
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    foreach ($line in $commandOutput) {
        Write-Host $line
    }

    if ($exitCode -ne 0) {
        throw $FailureMessage
    }

    return @($commandOutput | ForEach-Object { $_.ToString() })
}

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path $Path)) {
        throw "Expected $Label was not found: $Path"
    }
}

function Get-OptionalPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Add-OptionalSummaryValue {
    param(
        [System.Collections.IDictionary]$Target,
        [string]$Name,
        $Value
    )

    if ($null -ne $Value -and -not [string]::IsNullOrWhiteSpace([string]$Value)) {
        $Target[$Name] = $Value
    }
}

function Read-ReviewResult {
    param([string]$ReviewResultPath)

    Assert-PathExists -Path $ReviewResultPath -Label "smoke review result JSON"
    return Get-Content -Raw -LiteralPath $ReviewResultPath | ConvertFrom-Json
}

function Parse-SmokeRunOutput {
    param([string[]]$Lines)

    $result = [ordered]@{}
    foreach ($line in $Lines) {
        if ($line -match '^DOCX:\s*(.+)$') {
            $result.docx_path = $Matches[1].Trim()
        } elseif ($line -match '^PDF:\s*(.+)$') {
            $result.pdf_path = $Matches[1].Trim()
        } elseif ($line -match '^Evidence:\s*(.+)$') {
            $result.evidence_dir = $Matches[1].Trim()
        } elseif ($line -match '^Pages:\s*(.+)$') {
            $result.pages_dir = $Matches[1].Trim()
        } elseif ($line -match '^Contact sheet:\s*(.+)$') {
            $result.contact_sheet = $Matches[1].Trim()
        } elseif ($line -match '^Report:\s*(.+)$') {
            $result.report_dir = $Matches[1].Trim()
        } elseif ($line -match '^Summary:\s*(.+)$') {
            $result.summary_path = $Matches[1].Trim()
        } elseif ($line -match '^Checklist:\s*(.+)$') {
            $result.checklist_path = $Matches[1].Trim()
        } elseif ($line -match '^Review result:\s*(.+)$') {
            $result.review_result_path = $Matches[1].Trim()
        } elseif ($line -match '^Final review:\s*(.+)$') {
            $result.final_review_path = $Matches[1].Trim()
        } elseif ($line -match '^Repair:\s*(.+)$') {
            $result.repair_dir = $Matches[1].Trim()
        }
    }

    $required = @(
        @{ Name = "DOCX path"; Value = $result.docx_path },
        @{ Name = "PDF path"; Value = $result.pdf_path },
        @{ Name = "evidence directory"; Value = $result.evidence_dir },
        @{ Name = "pages directory"; Value = $result.pages_dir },
        @{ Name = "contact sheet"; Value = $result.contact_sheet },
        @{ Name = "report directory"; Value = $result.report_dir },
        @{ Name = "summary JSON"; Value = $result.summary_path },
        @{ Name = "review checklist"; Value = $result.checklist_path },
        @{ Name = "review result JSON"; Value = $result.review_result_path },
        @{ Name = "final review Markdown"; Value = $result.final_review_path },
        @{ Name = "repair directory"; Value = $result.repair_dir }
    )

    foreach ($item in $required) {
        Assert-PathExists -Path $item.Value -Label $item.Name
    }

    return $result
}

function Parse-PrepareTaskOutput {
    param([string[]]$Lines)

    $taskInfo = [ordered]@{}
    foreach ($line in $Lines) {
        if ($line -match '^Task id:\s*(.+)$') {
            $taskInfo.task_id = $Matches[1].Trim()
        } elseif ($line -match '^Source kind:\s*(.+)$') {
            $taskInfo.source_kind = $Matches[1].Trim()
        } elseif ($line -match '^Source:\s*(.+)$') {
            $taskInfo.source_path = $Matches[1].Trim()
        } elseif ($line -match '^Document:\s*(.+)$') {
            $taskInfo.document_path = $Matches[1].Trim()
        } elseif ($line -match '^Mode:\s*(.+)$') {
            $taskInfo.mode = $Matches[1].Trim()
        } elseif ($line -match '^Task directory:\s*(.+)$') {
            $taskInfo.task_dir = $Matches[1].Trim()
        } elseif ($line -match '^Prompt:\s*(.+)$') {
            $taskInfo.prompt_path = $Matches[1].Trim()
        } elseif ($line -match '^Manifest:\s*(.+)$') {
            $taskInfo.manifest_path = $Matches[1].Trim()
        } elseif ($line -match '^Review result:\s*(.+)$') {
            $taskInfo.review_result_path = $Matches[1].Trim()
        } elseif ($line -match '^Final review:\s*(.+)$') {
            $taskInfo.final_review_path = $Matches[1].Trim()
        } elseif ($line -match '^Latest task pointer:\s*(.+)$') {
            $taskInfo.latest_task_pointer_path = $Matches[1].Trim()
        } elseif ($line -match '^Latest source-kind task pointer:\s*(.+)$') {
            $taskInfo.latest_source_kind_task_pointer_path = $Matches[1].Trim()
        }
    }

    $required = @(
        @{ Name = "task directory"; Value = $taskInfo.task_dir },
        @{ Name = "task prompt"; Value = $taskInfo.prompt_path },
        @{ Name = "task manifest"; Value = $taskInfo.manifest_path },
        @{ Name = "task review result"; Value = $taskInfo.review_result_path },
        @{ Name = "task final review"; Value = $taskInfo.final_review_path },
        @{ Name = "latest task pointer"; Value = $taskInfo.latest_task_pointer_path },
        @{ Name = "latest source-kind task pointer"; Value = $taskInfo.latest_source_kind_task_pointer_path }
    )

    foreach ($item in $required) {
        Assert-PathExists -Path $item.Value -Label $item.Name
    }

    return $taskInfo
}

function Get-TaskSummaryBlock {
    param(
        [string]$Label,
        $TaskInfo
    )

    if ($null -eq $TaskInfo) {
        return "- $Label review task: not requested"
    }

    return @(
        "- $Label task id: $($TaskInfo.task_id)"
        "- $Label task dir: $($TaskInfo.task_dir)"
        "- $Label prompt: $($TaskInfo.prompt_path)"
        "- $Label latest pointer: $($TaskInfo.latest_source_kind_task_pointer_path)"
    ) -join [Environment]::NewLine
}

function Get-FlowStatusLine {
    param(
        [string]$Label,
        $FlowInfo,
        [string]$PathField
    )

    if ($null -eq $FlowInfo) {
        return "- $Label flow: not requested"
    }

    if ($FlowInfo.status -eq "completed") {
        return "- $Label flow: completed ($($FlowInfo.$PathField))"
    }

    return "- $Label flow: $($FlowInfo.status)"
}

function Resolve-AggregateContactSheetPath {
    param(
        [string]$AggregateEvidenceDir,
        [string]$Label
    )

    foreach ($fileName in @("before_after_contact_sheet.png", "contact_sheet.png")) {
        $candidate = Join-Path $AggregateEvidenceDir $fileName
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "Expected $Label aggregate contact sheet was not found under: $AggregateEvidenceDir"
}

function New-VisualRegressionRefreshCommand {
    param(
        [string]$ScriptPath,
        [string]$BuildDir,
        [string]$OutputDir,
        [int]$Dpi,
        [bool]$SkipBuild
    )

    $normalizedOutputDir = $OutputDir.TrimEnd('\', '/')

    $parts = @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$ScriptPath`""
        "-BuildDir"
        "`"$BuildDir`""
        "-OutputDir"
        "`"$normalizedOutputDir`""
        "-Dpi"
        $Dpi.ToString()
    )

    if ($SkipBuild) {
        $parts += "-SkipBuild"
    }

    return ($parts -join " ")
}

function Invoke-VisualRegressionBundleFlow {
    param(
        [string]$Label,
        [string]$Id,
        [string]$ScriptPath,
        [string]$BuildDir,
        [string]$OutputDirForChild,
        [string]$ResolvedOutputDir,
        [int]$Dpi,
        [bool]$SkipBuild,
        [bool]$KeepWordOpen,
        [bool]$VisibleWord
    )

    Write-Step "Running $Label visual regression flow"

    $arguments = @(
        "-BuildDir"
        $BuildDir
        "-OutputDir"
        $OutputDirForChild
        "-Dpi"
        $Dpi.ToString()
    )
    if ($SkipBuild) {
        $arguments += "-SkipBuild"
    }
    if ($KeepWordOpen) {
        $arguments += "-KeepWordOpen"
    }
    if ($VisibleWord) {
        $arguments += "-VisibleWord"
    }

    Invoke-ChildPowerShell -ScriptPath $ScriptPath `
        -Arguments $arguments `
        -FailureMessage "$Label visual regression gate step failed." | Out-Null

    $summaryPath = Join-Path $ResolvedOutputDir "summary.json"
    $aggregateEvidenceDir = Join-Path $ResolvedOutputDir "aggregate-evidence"
    Assert-PathExists -Path $summaryPath -Label "$Label summary JSON"
    Assert-PathExists -Path $aggregateEvidenceDir -Label "$Label aggregate evidence directory"

    $aggregateContactSheetPath = Resolve-AggregateContactSheetPath `
        -AggregateEvidenceDir $aggregateEvidenceDir `
        -Label $Label

    return [ordered]@{
        id = $Id
        label = $Label
        status = "completed"
        output_dir = $ResolvedOutputDir
        summary_json = $summaryPath
        aggregate_evidence_dir = $aggregateEvidenceDir
        aggregate_contact_sheet = $aggregateContactSheetPath
    }
}

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
    review_tasks = [ordered]@{
        document = $null
        fixed_grid = $null
        section_page_setup = $null
        page_number_fields = $null
        curated_visual_regressions = @()
    }
}

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

if (-not $SkipSmoke) {
    Write-Step "Running standard Word visual smoke flow"

    $smokeArgs = @(
        "-BuildDir"
        $SmokeBuildDir
        "-OutputDir"
        $smokeOutputDirForChild
        "-Dpi"
        $Dpi.ToString()
    )
    if ($SkipBuild) {
        $smokeArgs += "-SkipBuild"
    }
    if ($KeepWordOpen) {
        $smokeArgs += "-KeepWordOpen"
    }
    if ($VisibleWord) {
        $smokeArgs += "-VisibleWord"
    }
    if ($SmokeReviewVerdict -ne "undecided" -or -not [string]::IsNullOrWhiteSpace($SmokeReviewNote)) {
        $smokeArgs += @("-ReviewVerdict", $SmokeReviewVerdict)
    }
    if (-not [string]::IsNullOrWhiteSpace($SmokeReviewNote)) {
        $smokeArgs += @("-ReviewNote", $SmokeReviewNote)
    }

    $smokeRunOutput = Invoke-ChildPowerShell -ScriptPath $smokeScript `
        -Arguments $smokeArgs `
        -FailureMessage "Word visual smoke gate step failed."
    $smokeInfo = Parse-SmokeRunOutput -Lines $smokeRunOutput
    $smokeReviewResult = Read-ReviewResult -ReviewResultPath $smokeInfo.review_result_path

    $gateSummary.smoke = [ordered]@{
        status = "completed"
        output_dir = $resolvedSmokeOutputDir
        docx_path = $smokeInfo.docx_path
        pdf_path = $smokeInfo.pdf_path
        evidence_dir = $smokeInfo.evidence_dir
        pages_dir = $smokeInfo.pages_dir
        contact_sheet = $smokeInfo.contact_sheet
        report_dir = $smokeInfo.report_dir
        summary_json = $smokeInfo.summary_path
        review_checklist = $smokeInfo.checklist_path
        review_result = $smokeInfo.review_result_path
        final_review = $smokeInfo.final_review_path
        repair_dir = $smokeInfo.repair_dir
        review_status = Get-OptionalPropertyValue -Object $smokeReviewResult -Name "status"
        review_verdict = Get-OptionalPropertyValue -Object $smokeReviewResult -Name "verdict"
    }
    Add-OptionalSummaryValue -Target $gateSummary.smoke -Name "reviewed_at" `
        -Value (Get-OptionalPropertyValue -Object $smokeReviewResult -Name "reviewed_at")
    Add-OptionalSummaryValue -Target $gateSummary.smoke -Name "review_method" `
        -Value (Get-OptionalPropertyValue -Object $smokeReviewResult -Name "review_method")
    Add-OptionalSummaryValue -Target $gateSummary.smoke -Name "review_note" `
        -Value (Get-OptionalPropertyValue -Object $smokeReviewResult -Name "review_note")

    if (-not $SkipReviewTasks) {
        Write-Step "Preparing document review task from the smoke DOCX"

        $prepareTaskArgs = @(
            "-DocxPath"
            $smokeInfo.docx_path
            "-Mode"
            $ReviewMode
            "-TaskOutputRoot"
            $taskOutputRootForChild
        )
        if ($OpenTaskDirs) {
            $prepareTaskArgs += "-OpenTaskDir"
        }

        $documentTaskOutput = Invoke-ChildPowerShell -ScriptPath $prepareTaskScript `
            -Arguments $prepareTaskArgs `
            -FailureMessage "Document review task preparation failed."
        $documentTaskInfo = Parse-PrepareTaskOutput -Lines $documentTaskOutput

        $gateSummary.review_tasks.document = $documentTaskInfo
        $gateSummary.smoke.task = $documentTaskInfo
    }
} else {
    $gateSummary.smoke = [ordered]@{
        status = "skipped"
    }
}

if (-not $SkipFixedGrid) {
    Write-Step "Running fixed-grid merge/unmerge regression flow"

    $fixedGridArgs = @(
        "-BuildDir"
        $FixedGridBuildDir
        "-OutputDir"
        $fixedGridOutputDirForChild
        "-Dpi"
        $Dpi.ToString()
    )
    if ($SkipBuild) {
        $fixedGridArgs += "-SkipBuild"
    }
    if ($KeepWordOpen) {
        $fixedGridArgs += "-KeepWordOpen"
    }
    if ($VisibleWord) {
        $fixedGridArgs += "-VisibleWord"
    }

    Invoke-ChildPowerShell -ScriptPath $fixedGridScript `
        -Arguments $fixedGridArgs `
        -FailureMessage "Fixed-grid regression gate step failed." | Out-Null

    $fixedGridSummaryPath = Join-Path $resolvedFixedGridOutputDir "summary.json"
    $fixedGridReviewManifestPath = Join-Path $resolvedFixedGridOutputDir "review_manifest.json"
    $fixedGridChecklistPath = Join-Path $resolvedFixedGridOutputDir "review_checklist.md"
    $fixedGridFinalReviewPath = Join-Path $resolvedFixedGridOutputDir "final_review.md"
    $fixedGridAggregateContactSheetPath = Join-Path $resolvedFixedGridOutputDir `
        "aggregate-evidence\contact_sheet.png"
    $fixedGridAggregateFirstPagesDir = Join-Path $resolvedFixedGridOutputDir `
        "aggregate-evidence\first-pages"

    Assert-PathExists -Path $fixedGridSummaryPath -Label "fixed-grid summary JSON"
    Assert-PathExists -Path $fixedGridReviewManifestPath -Label "fixed-grid review manifest"
    Assert-PathExists -Path $fixedGridChecklistPath -Label "fixed-grid review checklist"
    Assert-PathExists -Path $fixedGridFinalReviewPath -Label "fixed-grid final review"
    Assert-PathExists -Path $fixedGridAggregateContactSheetPath `
        -Label "fixed-grid aggregate contact sheet"
    Assert-PathExists -Path $fixedGridAggregateFirstPagesDir `
        -Label "fixed-grid aggregate first-pages directory"

    $gateSummary.fixed_grid = [ordered]@{
        status = "completed"
        output_dir = $resolvedFixedGridOutputDir
        summary_json = $fixedGridSummaryPath
        review_manifest = $fixedGridReviewManifestPath
        review_checklist = $fixedGridChecklistPath
        final_review = $fixedGridFinalReviewPath
        aggregate_contact_sheet = $fixedGridAggregateContactSheetPath
        aggregate_first_pages_dir = $fixedGridAggregateFirstPagesDir
    }

    if (-not $SkipReviewTasks) {
        Write-Step "Preparing fixed-grid bundle review task"

        $prepareTaskArgs = @(
            "-FixedGridRegressionRoot"
            $resolvedFixedGridOutputDir
            "-Mode"
            $ReviewMode
            "-TaskOutputRoot"
            $taskOutputRootForChild
        )
        if ($FixedGridReviewVerdict -ne "undecided" -or -not [string]::IsNullOrWhiteSpace($FixedGridReviewNote)) {
            $prepareTaskArgs += @("-ReviewVerdict", $FixedGridReviewVerdict)
        }
        if (-not [string]::IsNullOrWhiteSpace($FixedGridReviewNote)) {
            $prepareTaskArgs += @("-ReviewNote", $FixedGridReviewNote)
        }
        if ($OpenTaskDirs) {
            $prepareTaskArgs += "-OpenTaskDir"
        }

        $fixedGridTaskOutput = Invoke-ChildPowerShell -ScriptPath $prepareTaskScript `
            -Arguments $prepareTaskArgs `
            -FailureMessage "Fixed-grid review task preparation failed."
        $fixedGridTaskInfo = Parse-PrepareTaskOutput -Lines $fixedGridTaskOutput
        $fixedGridTaskReview = Read-ReviewResult -ReviewResultPath $fixedGridTaskInfo.review_result_path

        $gateSummary.review_tasks.fixed_grid = $fixedGridTaskInfo
        $gateSummary.fixed_grid.task = $fixedGridTaskInfo
        $gateSummary.fixed_grid.review_status = Get-OptionalPropertyValue -Object $fixedGridTaskReview -Name "status"
        $gateSummary.fixed_grid.review_verdict = Get-OptionalPropertyValue -Object $fixedGridTaskReview -Name "verdict"
        Add-OptionalSummaryValue -Target $gateSummary.fixed_grid -Name "reviewed_at" `
            -Value (Get-OptionalPropertyValue -Object $fixedGridTaskReview -Name "reviewed_at")
        Add-OptionalSummaryValue -Target $gateSummary.fixed_grid -Name "review_method" `
            -Value (Get-OptionalPropertyValue -Object $fixedGridTaskReview -Name "review_method")
        Add-OptionalSummaryValue -Target $gateSummary.fixed_grid -Name "review_note" `
            -Value (Get-OptionalPropertyValue -Object $fixedGridTaskReview -Name "review_note")
    }
} else {
    $gateSummary.fixed_grid = [ordered]@{
        status = "skipped"
    }
}

if (-not $SkipSectionPageSetup) {
    Write-Step "Running section page setup regression flow"

    $sectionPageSetupArgs = @(
        "-BuildDir"
        $SectionPageSetupBuildDir
        "-OutputDir"
        $sectionPageSetupOutputDirForChild
        "-Dpi"
        $Dpi.ToString()
    )
    if ($SkipBuild) {
        $sectionPageSetupArgs += "-SkipBuild"
    }
    if ($KeepWordOpen) {
        $sectionPageSetupArgs += "-KeepWordOpen"
    }
    if ($VisibleWord) {
        $sectionPageSetupArgs += "-VisibleWord"
    }

    Invoke-ChildPowerShell -ScriptPath $sectionPageSetupScript `
        -Arguments $sectionPageSetupArgs `
        -FailureMessage "Section page setup regression gate step failed." | Out-Null

    $sectionPageSetupSummaryPath = Join-Path $resolvedSectionPageSetupOutputDir "summary.json"
    $sectionPageSetupReviewManifestPath = Join-Path $resolvedSectionPageSetupOutputDir "review_manifest.json"
    $sectionPageSetupChecklistPath = Join-Path $resolvedSectionPageSetupOutputDir "review_checklist.md"
    $sectionPageSetupFinalReviewPath = Join-Path $resolvedSectionPageSetupOutputDir "final_review.md"
    $sectionPageSetupAggregateContactSheetPath = Join-Path $resolvedSectionPageSetupOutputDir `
        "aggregate-evidence\contact_sheet.png"
    $sectionPageSetupAggregateContactSheetsDir = Join-Path $resolvedSectionPageSetupOutputDir `
        "aggregate-evidence\contact-sheets"

    Assert-PathExists -Path $sectionPageSetupSummaryPath -Label "section page setup summary JSON"
    Assert-PathExists -Path $sectionPageSetupReviewManifestPath -Label "section page setup review manifest"
    Assert-PathExists -Path $sectionPageSetupChecklistPath -Label "section page setup review checklist"
    Assert-PathExists -Path $sectionPageSetupFinalReviewPath -Label "section page setup final review"
    Assert-PathExists -Path $sectionPageSetupAggregateContactSheetPath `
        -Label "section page setup aggregate contact sheet"
    Assert-PathExists -Path $sectionPageSetupAggregateContactSheetsDir `
        -Label "section page setup aggregate contact-sheets directory"

    $gateSummary.section_page_setup = [ordered]@{
        status = "completed"
        output_dir = $resolvedSectionPageSetupOutputDir
        summary_json = $sectionPageSetupSummaryPath
        review_manifest = $sectionPageSetupReviewManifestPath
        review_checklist = $sectionPageSetupChecklistPath
        final_review = $sectionPageSetupFinalReviewPath
        aggregate_contact_sheet = $sectionPageSetupAggregateContactSheetPath
        aggregate_contact_sheets_dir = $sectionPageSetupAggregateContactSheetsDir
    }

    if (-not $SkipReviewTasks) {
        Write-Step "Preparing section page setup bundle review task"

        $prepareTaskArgs = @(
            "-SectionPageSetupRegressionRoot"
            $resolvedSectionPageSetupOutputDir
            "-Mode"
            $ReviewMode
            "-TaskOutputRoot"
            $taskOutputRootForChild
        )
        if ($SectionPageSetupReviewVerdict -ne "undecided" -or -not [string]::IsNullOrWhiteSpace($SectionPageSetupReviewNote)) {
            $prepareTaskArgs += @("-ReviewVerdict", $SectionPageSetupReviewVerdict)
        }
        if (-not [string]::IsNullOrWhiteSpace($SectionPageSetupReviewNote)) {
            $prepareTaskArgs += @("-ReviewNote", $SectionPageSetupReviewNote)
        }
        if ($OpenTaskDirs) {
            $prepareTaskArgs += "-OpenTaskDir"
        }

        $sectionPageSetupTaskOutput = Invoke-ChildPowerShell -ScriptPath $prepareTaskScript `
            -Arguments $prepareTaskArgs `
            -FailureMessage "Section page setup review task preparation failed."
        $sectionPageSetupTaskInfo = Parse-PrepareTaskOutput -Lines $sectionPageSetupTaskOutput
        $sectionPageSetupTaskReview = Read-ReviewResult -ReviewResultPath $sectionPageSetupTaskInfo.review_result_path

        $gateSummary.review_tasks.section_page_setup = $sectionPageSetupTaskInfo
        $gateSummary.section_page_setup.task = $sectionPageSetupTaskInfo
        $gateSummary.section_page_setup.review_status = Get-OptionalPropertyValue -Object $sectionPageSetupTaskReview -Name "status"
        $gateSummary.section_page_setup.review_verdict = Get-OptionalPropertyValue -Object $sectionPageSetupTaskReview -Name "verdict"
        Add-OptionalSummaryValue -Target $gateSummary.section_page_setup -Name "reviewed_at" `
            -Value (Get-OptionalPropertyValue -Object $sectionPageSetupTaskReview -Name "reviewed_at")
        Add-OptionalSummaryValue -Target $gateSummary.section_page_setup -Name "review_method" `
            -Value (Get-OptionalPropertyValue -Object $sectionPageSetupTaskReview -Name "review_method")
        Add-OptionalSummaryValue -Target $gateSummary.section_page_setup -Name "review_note" `
            -Value (Get-OptionalPropertyValue -Object $sectionPageSetupTaskReview -Name "review_note")
    }
} else {
    $gateSummary.section_page_setup = [ordered]@{
        status = "skipped"
    }
}

if (-not $SkipPageNumberFields) {
    Write-Step "Running page number fields regression flow"

    $pageNumberFieldsArgs = @(
        "-BuildDir"
        $PageNumberFieldsBuildDir
        "-OutputDir"
        $pageNumberFieldsOutputDirForChild
        "-Dpi"
        $Dpi.ToString()
    )
    if ($SkipBuild) {
        $pageNumberFieldsArgs += "-SkipBuild"
    }
    if ($KeepWordOpen) {
        $pageNumberFieldsArgs += "-KeepWordOpen"
    }
    if ($VisibleWord) {
        $pageNumberFieldsArgs += "-VisibleWord"
    }

    Invoke-ChildPowerShell -ScriptPath $pageNumberFieldsScript `
        -Arguments $pageNumberFieldsArgs `
        -FailureMessage "Page number fields regression gate step failed." | Out-Null

    $pageNumberFieldsSummaryPath = Join-Path $resolvedPageNumberFieldsOutputDir "summary.json"
    $pageNumberFieldsReviewManifestPath = Join-Path $resolvedPageNumberFieldsOutputDir "review_manifest.json"
    $pageNumberFieldsChecklistPath = Join-Path $resolvedPageNumberFieldsOutputDir "review_checklist.md"
    $pageNumberFieldsFinalReviewPath = Join-Path $resolvedPageNumberFieldsOutputDir "final_review.md"
    $pageNumberFieldsAggregateContactSheetPath = Join-Path $resolvedPageNumberFieldsOutputDir `
        "aggregate-evidence\contact_sheet.png"
    $pageNumberFieldsAggregateContactSheetsDir = Join-Path $resolvedPageNumberFieldsOutputDir `
        "aggregate-evidence\contact-sheets"

    Assert-PathExists -Path $pageNumberFieldsSummaryPath -Label "page number fields summary JSON"
    Assert-PathExists -Path $pageNumberFieldsReviewManifestPath -Label "page number fields review manifest"
    Assert-PathExists -Path $pageNumberFieldsChecklistPath -Label "page number fields review checklist"
    Assert-PathExists -Path $pageNumberFieldsFinalReviewPath -Label "page number fields final review"
    Assert-PathExists -Path $pageNumberFieldsAggregateContactSheetPath `
        -Label "page number fields aggregate contact sheet"
    Assert-PathExists -Path $pageNumberFieldsAggregateContactSheetsDir `
        -Label "page number fields aggregate contact-sheets directory"

    $gateSummary.page_number_fields = [ordered]@{
        status = "completed"
        output_dir = $resolvedPageNumberFieldsOutputDir
        summary_json = $pageNumberFieldsSummaryPath
        review_manifest = $pageNumberFieldsReviewManifestPath
        review_checklist = $pageNumberFieldsChecklistPath
        final_review = $pageNumberFieldsFinalReviewPath
        aggregate_contact_sheet = $pageNumberFieldsAggregateContactSheetPath
        aggregate_contact_sheets_dir = $pageNumberFieldsAggregateContactSheetsDir
    }

    if (-not $SkipReviewTasks) {
        Write-Step "Preparing page number fields bundle review task"

        $prepareTaskArgs = @(
            "-PageNumberFieldsRegressionRoot"
            $resolvedPageNumberFieldsOutputDir
            "-Mode"
            $ReviewMode
            "-TaskOutputRoot"
            $taskOutputRootForChild
        )
        if ($PageNumberFieldsReviewVerdict -ne "undecided" -or -not [string]::IsNullOrWhiteSpace($PageNumberFieldsReviewNote)) {
            $prepareTaskArgs += @("-ReviewVerdict", $PageNumberFieldsReviewVerdict)
        }
        if (-not [string]::IsNullOrWhiteSpace($PageNumberFieldsReviewNote)) {
            $prepareTaskArgs += @("-ReviewNote", $PageNumberFieldsReviewNote)
        }
        if ($OpenTaskDirs) {
            $prepareTaskArgs += "-OpenTaskDir"
        }

        $pageNumberFieldsTaskOutput = Invoke-ChildPowerShell -ScriptPath $prepareTaskScript `
            -Arguments $prepareTaskArgs `
            -FailureMessage "Page number fields review task preparation failed."
        $pageNumberFieldsTaskInfo = Parse-PrepareTaskOutput -Lines $pageNumberFieldsTaskOutput
        $pageNumberFieldsTaskReview = Read-ReviewResult -ReviewResultPath $pageNumberFieldsTaskInfo.review_result_path

        $gateSummary.review_tasks.page_number_fields = $pageNumberFieldsTaskInfo
        $gateSummary.page_number_fields.task = $pageNumberFieldsTaskInfo
        $gateSummary.page_number_fields.review_status = Get-OptionalPropertyValue -Object $pageNumberFieldsTaskReview -Name "status"
        $gateSummary.page_number_fields.review_verdict = Get-OptionalPropertyValue -Object $pageNumberFieldsTaskReview -Name "verdict"
        Add-OptionalSummaryValue -Target $gateSummary.page_number_fields -Name "reviewed_at" `
            -Value (Get-OptionalPropertyValue -Object $pageNumberFieldsTaskReview -Name "reviewed_at")
        Add-OptionalSummaryValue -Target $gateSummary.page_number_fields -Name "review_method" `
            -Value (Get-OptionalPropertyValue -Object $pageNumberFieldsTaskReview -Name "review_method")
        Add-OptionalSummaryValue -Target $gateSummary.page_number_fields -Name "review_note" `
            -Value (Get-OptionalPropertyValue -Object $pageNumberFieldsTaskReview -Name "review_note")
    }
} else {
    $gateSummary.page_number_fields = [ordered]@{
        status = "skipped"
    }
}

foreach ($descriptor in $curatedVisualFlowDescriptors) {
    if ($descriptor.skip) {
        $gateSummary.curated_visual_regressions += [ordered]@{
            id = $descriptor.id
            label = $descriptor.label
            status = "skipped"
        }
        continue
    }

    $flowInfo = Invoke-VisualRegressionBundleFlow `
        -Label $descriptor.label `
        -Id $descriptor.id `
        -ScriptPath $descriptor.script_path `
        -BuildDir $descriptor.build_dir `
        -OutputDirForChild $descriptor.output_dir_for_child `
        -ResolvedOutputDir $descriptor.output_dir `
        -Dpi $Dpi `
        -SkipBuild $SkipBuild.IsPresent `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent

    $flowInfo.refresh_command = New-VisualRegressionRefreshCommand `
        -ScriptPath $descriptor.script_path `
        -BuildDir $descriptor.build_dir `
        -OutputDir $descriptor.output_dir_for_child `
        -Dpi $Dpi `
        -SkipBuild $SkipBuild.IsPresent

    if (-not $SkipReviewTasks) {
        Write-Step "Preparing $($descriptor.label) visual regression review task"

        $prepareTaskArgs = @(
            "-VisualRegressionBundleRoot"
            $descriptor.output_dir
            "-VisualRegressionBundleLabel"
            $descriptor.label
            "-VisualRegressionBundleKey"
            "$($descriptor.id)-visual-regression-bundle"
            "-VisualRegressionRefreshCommand"
            $flowInfo.refresh_command
            "-Mode"
            $ReviewMode
            "-TaskOutputRoot"
            $taskOutputRootForChild
        )
        if ($CuratedVisualReviewVerdict -ne "undecided" -or -not [string]::IsNullOrWhiteSpace($CuratedVisualReviewNote)) {
            $prepareTaskArgs += @("-ReviewVerdict", $CuratedVisualReviewVerdict)
        }
        if (-not [string]::IsNullOrWhiteSpace($CuratedVisualReviewNote)) {
            $prepareTaskArgs += @("-ReviewNote", $CuratedVisualReviewNote)
        }
        if ($OpenTaskDirs) {
            $prepareTaskArgs += "-OpenTaskDir"
        }

        $bundleTaskOutput = Invoke-ChildPowerShell -ScriptPath $prepareTaskScript `
            -Arguments $prepareTaskArgs `
            -FailureMessage "$($descriptor.label) review task preparation failed."
        $bundleTaskInfo = Parse-PrepareTaskOutput -Lines $bundleTaskOutput
        $bundleTaskReview = Read-ReviewResult -ReviewResultPath $bundleTaskInfo.review_result_path

        $flowInfo.task = $bundleTaskInfo
        $flowInfo.review_status = Get-OptionalPropertyValue -Object $bundleTaskReview -Name "status"
        $flowInfo.review_verdict = Get-OptionalPropertyValue -Object $bundleTaskReview -Name "verdict"
        Add-OptionalSummaryValue -Target $flowInfo -Name "reviewed_at" `
            -Value (Get-OptionalPropertyValue -Object $bundleTaskReview -Name "reviewed_at")
        Add-OptionalSummaryValue -Target $flowInfo -Name "review_method" `
            -Value (Get-OptionalPropertyValue -Object $bundleTaskReview -Name "review_method")
        Add-OptionalSummaryValue -Target $flowInfo -Name "review_note" `
            -Value (Get-OptionalPropertyValue -Object $bundleTaskReview -Name "review_note")

        $bundleTaskSummary = [ordered]@{
            id = $descriptor.id
            label = $descriptor.label
            task = $bundleTaskInfo
            review_status = Get-OptionalPropertyValue -Object $bundleTaskReview -Name "status"
            review_verdict = Get-OptionalPropertyValue -Object $bundleTaskReview -Name "verdict"
        }
        Add-OptionalSummaryValue -Target $bundleTaskSummary -Name "reviewed_at" `
            -Value (Get-OptionalPropertyValue -Object $bundleTaskReview -Name "reviewed_at")
        Add-OptionalSummaryValue -Target $bundleTaskSummary -Name "review_method" `
            -Value (Get-OptionalPropertyValue -Object $bundleTaskReview -Name "review_method")
        Add-OptionalSummaryValue -Target $bundleTaskSummary -Name "review_note" `
            -Value (Get-OptionalPropertyValue -Object $bundleTaskReview -Name "review_note")
        $gateSummary.review_tasks.curated_visual_regressions += $bundleTaskSummary
    }

    $gateSummary.curated_visual_regressions += $flowInfo
}

if ($RefreshReadmeAssets) {
    Write-Step "Refreshing repository README gallery assets"
    Invoke-ChildPowerShell -ScriptPath $refreshReadmeAssetsScript `
        -Arguments @(
            "-TaskOutputRoot"
            $taskOutputRootForChild
            "-DocumentTaskDir"
            $gateSummary.review_tasks.document.task_dir
            "-FixedGridTaskDir"
            $gateSummary.review_tasks.fixed_grid.task_dir
            "-AssetsDir"
            $resolvedReadmeAssetsDir
        ) `
        -FailureMessage "README gallery refresh failed." | Out-Null

    $gateSummary.readme_gallery = [ordered]@{
        status = "completed"
        assets_dir = $resolvedReadmeAssetsDir
        visual_smoke_contact_sheet = Join-Path $resolvedReadmeAssetsDir "visual-smoke-contact-sheet.png"
        visual_smoke_page_05 = Join-Path $resolvedReadmeAssetsDir "visual-smoke-page-05.png"
        visual_smoke_page_06 = Join-Path $resolvedReadmeAssetsDir "visual-smoke-page-06.png"
        fixed_grid_merge_right_page_01 = Join-Path $resolvedReadmeAssetsDir "fixed-grid-merge-right-page-01.png"
        fixed_grid_merge_down_page_01 = Join-Path $resolvedReadmeAssetsDir "fixed-grid-merge-down-page-01.png"
        fixed_grid_aggregate_contact_sheet = Join-Path $resolvedReadmeAssetsDir "fixed-grid-aggregate-contact-sheet.png"
        sample_chinese_template_page_01 = Join-Path $resolvedReadmeAssetsDir "sample-chinese-template-page-01.png"
    }

    Assert-PathExists -Path $gateSummary.readme_gallery.visual_smoke_contact_sheet `
        -Label "README gallery visual smoke contact sheet"
    Assert-PathExists -Path $gateSummary.readme_gallery.visual_smoke_page_05 `
        -Label "README gallery visual smoke page 05"
    Assert-PathExists -Path $gateSummary.readme_gallery.visual_smoke_page_06 `
        -Label "README gallery visual smoke page 06"
    Assert-PathExists -Path $gateSummary.readme_gallery.fixed_grid_merge_right_page_01 `
        -Label "README gallery fixed-grid merge_right page 01"
    Assert-PathExists -Path $gateSummary.readme_gallery.fixed_grid_merge_down_page_01 `
        -Label "README gallery fixed-grid merge_down page 01"
    Assert-PathExists -Path $gateSummary.readme_gallery.fixed_grid_aggregate_contact_sheet `
        -Label "README gallery fixed-grid aggregate contact sheet"
    Assert-PathExists -Path $gateSummary.readme_gallery.sample_chinese_template_page_01 `
        -Label "README gallery sample Chinese template page 01"
} else {
    $gateSummary.readme_gallery = [ordered]@{
        status = "not_requested"
    }
}

$gateSummary.notes = @(
    "This gate validates execution, evidence generation, and review-task packaging.",
    "Visual pass/fail still depends on screenshot-backed review written into each task's report directory."
)

($gateSummary | ConvertTo-Json -Depth 8) | Set-Content -Path $gateSummaryPath -Encoding UTF8

$documentTaskSummary = Get-TaskSummaryBlock -Label "Document" -TaskInfo $gateSummary.review_tasks.document
$fixedGridTaskSummary = Get-TaskSummaryBlock -Label "Fixed-grid" -TaskInfo $gateSummary.review_tasks.fixed_grid
$sectionPageSetupTaskSummary = Get-TaskSummaryBlock -Label "Section page setup" -TaskInfo $gateSummary.review_tasks.section_page_setup
$pageNumberFieldsTaskSummary = Get-TaskSummaryBlock -Label "Page number fields" -TaskInfo $gateSummary.review_tasks.page_number_fields

$smokeStatusLine = Get-FlowStatusLine -Label "Smoke" -FlowInfo $gateSummary.smoke -PathField "docx_path"
$smokeReviewStatusLine = if ($gateSummary.smoke -and $gateSummary.smoke.status -eq "completed") {
    "- Smoke review verdict: $($gateSummary.smoke.review_verdict) ($($gateSummary.smoke.review_status))"
} else {
    "- Smoke review verdict: not available"
}
$fixedGridStatusLine = Get-FlowStatusLine -Label "Fixed-grid" -FlowInfo $gateSummary.fixed_grid -PathField "summary_json"
$fixedGridReviewStatusLine = if ($gateSummary.fixed_grid -and $gateSummary.fixed_grid.status -eq "completed") {
    if ($gateSummary.fixed_grid.review_verdict) {
        "- Fixed-grid review verdict: $($gateSummary.fixed_grid.review_verdict) ($($gateSummary.fixed_grid.review_status))"
    } else {
        "- Fixed-grid review verdict: not requested"
    }
} else {
    "- Fixed-grid review verdict: not available"
}
$sectionPageSetupStatusLine = Get-FlowStatusLine -Label "Section page setup" -FlowInfo $gateSummary.section_page_setup -PathField "summary_json"
$sectionPageSetupReviewStatusLine = if ($gateSummary.section_page_setup -and $gateSummary.section_page_setup.status -eq "completed") {
    if ($gateSummary.section_page_setup.review_verdict) {
        "- Section page setup review verdict: $($gateSummary.section_page_setup.review_verdict) ($($gateSummary.section_page_setup.review_status))"
    } else {
        "- Section page setup review verdict: not requested"
    }
} else {
    "- Section page setup review verdict: not available"
}
$pageNumberFieldsStatusLine = Get-FlowStatusLine -Label "Page number fields" -FlowInfo $gateSummary.page_number_fields -PathField "summary_json"
$pageNumberFieldsReviewStatusLine = if ($gateSummary.page_number_fields -and $gateSummary.page_number_fields.status -eq "completed") {
    if ($gateSummary.page_number_fields.review_verdict) {
        "- Page number fields review verdict: $($gateSummary.page_number_fields.review_verdict) ($($gateSummary.page_number_fields.review_status))"
    } else {
        "- Page number fields review verdict: not requested"
    }
} else {
    "- Page number fields review verdict: not available"
}
$curatedVisualStatusLines = if ($gateSummary.curated_visual_regressions.Count -gt 0) {
    ($gateSummary.curated_visual_regressions | ForEach-Object {
            if ($_.status -eq "completed") {
                "- $($_.label) flow: completed ($($_.summary_json))"
            } else {
                "- $($_.label) flow: $($_.status)"
            }
        }) -join [Environment]::NewLine
} else {
    "- Curated visual regression bundles: not requested"
}
$curatedVisualTaskSummary = if ($gateSummary.review_tasks.curated_visual_regressions.Count -gt 0) {
    ($gateSummary.review_tasks.curated_visual_regressions | ForEach-Object {
            @(
                "- $($_.label) task id: $($_.task.task_id)"
                "- $($_.label) task dir: $($_.task.task_dir)"
                "- $($_.label) prompt: $($_.task.prompt_path)"
                "- $($_.label) latest pointer: $($_.task.latest_source_kind_task_pointer_path)"
                "- $($_.label) review verdict: $($_.review_verdict) ($($_.review_status))"
            ) -join [Environment]::NewLine
        }) -join [Environment]::NewLine
} else {
    "- Curated visual regression review tasks: not requested"
}

$readmeGalleryStatusLine = if ($gateSummary.readme_gallery.status -eq "completed") {
    "- README gallery refresh: completed ($($gateSummary.readme_gallery.assets_dir))"
} else {
    "- README gallery refresh: not requested"
}

$nextSteps = if ($SkipReviewTasks) {
    @(
        "1. Review tasks were skipped, so only raw evidence is available."
        "2. Re-run without -SkipReviewTasks if you want stable task prompts and latest pointers."
    ) -join [Environment]::NewLine
} else {
    $steps = @()
    $stepIndex = 1

    if ($gateSummary.review_tasks.document) {
        $steps += "$stepIndex. Review the document task via open_latest_word_review_task.ps1 -SourceKind document -PrintPrompt."
        $stepIndex += 1
    }
    if ($gateSummary.review_tasks.fixed_grid) {
        $steps += "$stepIndex. Review the fixed-grid task via open_latest_fixed_grid_review_task.ps1 -PrintPrompt."
        $stepIndex += 1
    }
    if ($gateSummary.review_tasks.section_page_setup) {
        $steps += "$stepIndex. Review the section page setup task via open_latest_section_page_setup_review_task.ps1 -PrintPrompt."
        $stepIndex += 1
    }
    if ($gateSummary.review_tasks.page_number_fields) {
        $steps += "$stepIndex. Review the page number fields task via open_latest_page_number_fields_review_task.ps1 -PrintPrompt."
        $stepIndex += 1
    }
    foreach ($bundleTask in $gateSummary.review_tasks.curated_visual_regressions) {
        $steps += "$stepIndex. Review the $($bundleTask.label) task through $($bundleTask.task.prompt_path)."
        $stepIndex += 1
    }
    $steps += "$stepIndex. Write screenshot-backed verdicts into each task's report/ directory."
    $steps -join [Environment]::NewLine
}

$gateFinalReview = @"
# Word visual release gate

- Generated at: $(Get-Date -Format s)
- Workspace: $repoRoot
- Gate output directory: $resolvedGateOutputDir
- Gate summary JSON: $gateSummaryPath
- Execution status: pass
- Visual review status: $($gateSummary.visual_verdict)

## Included flows

$smokeStatusLine
$smokeReviewStatusLine
$fixedGridStatusLine
$fixedGridReviewStatusLine
$sectionPageSetupStatusLine
$sectionPageSetupReviewStatusLine
$pageNumberFieldsStatusLine
$pageNumberFieldsReviewStatusLine
$curatedVisualStatusLines
$readmeGalleryStatusLine

## Review tasks

$documentTaskSummary
$fixedGridTaskSummary
$sectionPageSetupTaskSummary
$pageNumberFieldsTaskSummary
$curatedVisualTaskSummary

## Next steps

$nextSteps
"@
$gateFinalReview | Set-Content -Path $gateFinalReviewPath -Encoding UTF8

Write-Step "Completed release gate"
Write-Host "Gate summary: $gateSummaryPath"
Write-Host "Gate final review: $gateFinalReviewPath"
if ($gateSummary.review_tasks.document) {
    Write-Host "Document task: $($gateSummary.review_tasks.document.task_dir)"
}
if ($gateSummary.review_tasks.fixed_grid) {
    Write-Host "Fixed-grid task: $($gateSummary.review_tasks.fixed_grid.task_dir)"
}
if ($gateSummary.review_tasks.section_page_setup) {
    Write-Host "Section page setup task: $($gateSummary.review_tasks.section_page_setup.task_dir)"
}
if ($gateSummary.review_tasks.page_number_fields) {
    Write-Host "Page number fields task: $($gateSummary.review_tasks.page_number_fields.task_dir)"
}
foreach ($bundleTask in $gateSummary.review_tasks.curated_visual_regressions) {
    Write-Host "$($bundleTask.label) task: $($bundleTask.task.task_dir)"
}
if ($gateSummary.readme_gallery.status -eq "completed") {
    Write-Host "README gallery assets: $($gateSummary.readme_gallery.assets_dir)"
}
