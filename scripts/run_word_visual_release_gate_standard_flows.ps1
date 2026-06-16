<#
.SYNOPSIS
Runs the standard Word visual release gate flows.
#>

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
