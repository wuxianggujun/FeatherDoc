<#
.SYNOPSIS
Runs curated visual flows and writes the Word visual release gate reports.
#>

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

$gateSummary.review_task_summary = Get-ReviewTaskSummary -ReviewTasks $gateSummary.review_tasks

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
- Review task count: $($gateSummary.review_task_summary.total_count) total ($($gateSummary.review_task_summary.standard_count) standard, $($gateSummary.review_task_summary.curated_count) curated)

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
