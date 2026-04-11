<#
.SYNOPSIS
Runs the local Word visual release gate for FeatherDoc.

.DESCRIPTION
Executes the standard smoke flow, the fixed-grid merge/unmerge regression
bundle, packages screenshot-backed review tasks, and can optionally refresh
the repository README gallery assets from the generated evidence.

.PARAMETER RefreshReadmeAssets
Copies the latest visual smoke and fixed-grid evidence into
docs/assets/readme. This requires both flows plus review-task packaging.

.PARAMETER ReadmeAssetsDir
Target repository directory for the refreshed README gallery PNG files.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1 `
    -SmokeBuildDir build-msvc-nmake `
    -FixedGridBuildDir build-msvc-nmake `
    -SkipBuild `
    -GateOutputDir output/word-visual-release-gate

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1 `
    -SmokeBuildDir build-msvc-nmake `
    -FixedGridBuildDir build-msvc-nmake `
    -SkipBuild `
    -GateOutputDir output/word-visual-release-gate `
    -TaskOutputRoot output/word-visual-smoke/tasks-release-gate `
    -RefreshReadmeAssets
#>
param(
    [string]$GateOutputDir = "output/word-visual-release-gate",
    [string]$SmokeBuildDir = "build-word-visual-smoke-nmake",
    [string]$FixedGridBuildDir = "build-fixed-grid-regression-nmake",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipSmoke,
    [switch]$SkipFixedGrid,
    [switch]$SkipReviewTasks,
    [ValidateSet("review-only", "review-and-repair")]
    [string]$ReviewMode = "review-only",
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

    $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE

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

$repoRoot = Resolve-RepoRoot

if ($SkipSmoke -and $SkipFixedGrid) {
    throw "At least one of smoke or fixed-grid flows must remain enabled."
}

$resolvedGateOutputDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $GateOutputDir
$resolvedTaskOutputRoot = Resolve-FullPath -RepoRoot $repoRoot -InputPath $TaskOutputRoot
$resolvedReadmeAssetsDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReadmeAssetsDir
$resolvedSmokeOutputDir = Join-Path $resolvedGateOutputDir "smoke"
$resolvedFixedGridOutputDir = Join-Path $resolvedGateOutputDir "fixed-grid"
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
$taskOutputRootForChild = Convert-ToChildScriptPath -RepoRoot $repoRoot `
    -TargetPath $resolvedTaskOutputRoot -Label "Task output root"

$smokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$fixedGridScript = Join-Path $repoRoot "scripts\run_fixed_grid_merge_unmerge_regression.ps1"
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
    review_tasks = [ordered]@{
        document = $null
        fixed_grid = $null
    }
}

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

    $smokeRunOutput = Invoke-ChildPowerShell -ScriptPath $smokeScript `
        -Arguments $smokeArgs `
        -FailureMessage "Word visual smoke gate step failed."
    $smokeInfo = Parse-SmokeRunOutput -Lines $smokeRunOutput

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
    }

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
        if ($OpenTaskDirs) {
            $prepareTaskArgs += "-OpenTaskDir"
        }

        $fixedGridTaskOutput = Invoke-ChildPowerShell -ScriptPath $prepareTaskScript `
            -Arguments $prepareTaskArgs `
            -FailureMessage "Fixed-grid review task preparation failed."
        $fixedGridTaskInfo = Parse-PrepareTaskOutput -Lines $fixedGridTaskOutput

        $gateSummary.review_tasks.fixed_grid = $fixedGridTaskInfo
        $gateSummary.fixed_grid.task = $fixedGridTaskInfo
    }
} else {
    $gateSummary.fixed_grid = [ordered]@{
        status = "skipped"
    }
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

$documentTaskSummary = if ($gateSummary.review_tasks.document) {
    @(
        "- Document task id: $($gateSummary.review_tasks.document.task_id)"
        "- Document task dir: $($gateSummary.review_tasks.document.task_dir)"
        "- Document prompt: $($gateSummary.review_tasks.document.prompt_path)"
        "- Document latest pointer: $($gateSummary.review_tasks.document.latest_source_kind_task_pointer_path)"
    ) -join [Environment]::NewLine
} else {
    "- Document review task: not requested"
}

$fixedGridTaskSummary = if ($gateSummary.review_tasks.fixed_grid) {
    @(
        "- Fixed-grid task id: $($gateSummary.review_tasks.fixed_grid.task_id)"
        "- Fixed-grid task dir: $($gateSummary.review_tasks.fixed_grid.task_dir)"
        "- Fixed-grid prompt: $($gateSummary.review_tasks.fixed_grid.prompt_path)"
        "- Fixed-grid latest pointer: $($gateSummary.review_tasks.fixed_grid.latest_source_kind_task_pointer_path)"
    ) -join [Environment]::NewLine
} else {
    "- Fixed-grid review task: not requested"
}

$smokeStatusLine = if ($gateSummary.smoke.status -eq "completed") {
    "- Smoke flow: completed ($($gateSummary.smoke.docx_path))"
} else {
    "- Smoke flow: skipped"
}

$fixedGridStatusLine = if ($gateSummary.fixed_grid.status -eq "completed") {
    "- Fixed-grid flow: completed ($($gateSummary.fixed_grid.summary_json))"
} else {
    "- Fixed-grid flow: skipped"
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
    @(
        '1. Review the document task via open_latest_word_review_task.ps1 -SourceKind document -PrintPrompt.'
        '2. Review the fixed-grid task via open_latest_fixed_grid_review_task.ps1 -PrintPrompt.'
        '3. Write screenshot-backed verdicts into each task''s report/ directory.'
    ) -join [Environment]::NewLine
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
$fixedGridStatusLine
$readmeGalleryStatusLine

## Review tasks

$documentTaskSummary
$fixedGridTaskSummary

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
if ($gateSummary.readme_gallery.status -eq "completed") {
    Write-Host "README gallery assets: $($gateSummary.readme_gallery.assets_dir)"
}
