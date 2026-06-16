    if (-not $SkipInstallSmoke) {
        $activeStep = "install_smoke"
        Write-Step "Running install + find_package smoke"
        $installSmokeOutput = Invoke-ChildPowerShellInMsvcEnv -MsvcBootstrap $msvcBootstrap `
            -ScriptPath $installSmokeScript `
            -Arguments @(
                "-BuildDir"
                $resolvedBuildDir
                "-InstallDir"
                $resolvedInstallDir
                "-ConsumerBuildDir"
                $resolvedConsumerBuildDir
                "-Generator"
                $Generator
                "-Config"
                $Config
            ) `
            -FailureMessage "Install + find_package smoke failed."
        $installSmokeInfo = Parse-InstallSmokeOutput -Lines $installSmokeOutput
        $summary.steps.install_smoke.status = "completed"
        $summary.steps.install_smoke.install_prefix = $installSmokeInfo.install_prefix
        $summary.steps.install_smoke.consumer_document = $installSmokeInfo.consumer_document
    }

    if (-not $SkipVisualGate) {
        $activeStep = "visual_gate"
        Write-Step "Running Word visual release gate"
        $visualGateBuildDirArguments = @(
            "-SmokeBuildDir",
            "-FixedGridBuildDir",
            "-SectionPageSetupBuildDir",
            "-PageNumberFieldsBuildDir",
            "-BookmarkFloatingImageBuildDir",
            "-BookmarkImageBuildDir",
            "-BookmarkBlockVisibilityBuildDir",
            "-TemplateBookmarkParagraphsBuildDir",
            "-BookmarkTableReplacementBuildDir",
            "-ParagraphListBuildDir",
            "-ParagraphNumberingBuildDir",
            "-ParagraphRunStyleBuildDir",
            "-ParagraphStyleNumberingBuildDir",
            "-FillBookmarksBuildDir",
            "-AppendImageBuildDir",
            "-FloatingImageZOrderBuildDir",
            "-TableRowBuildDir",
            "-TableRowHeightBuildDir",
            "-TableRowCantSplitBuildDir",
            "-TableRowRepeatHeaderBuildDir",
            "-TableCellFillBuildDir",
            "-TableCellBorderBuildDir",
            "-TableCellWidthBuildDir",
            "-TableCellMarginBuildDir",
            "-TableCellVerticalAlignmentBuildDir",
            "-TableCellTextDirectionBuildDir",
            "-TableCellMergeBuildDir",
            "-TemplateBookmarkMultilineBuildDir",
            "-SectionTextMultilineBuildDir",
            "-RemoveBookmarkBlockBuildDir",
            "-TemplateBookmarkParagraphsPaginationBuildDir",
            "-SectionOrderBuildDir",
            "-SectionPartRefsBuildDir",
            "-RunFontLanguageBuildDir",
            "-EnsureStyleBuildDir",
            "-TableStyleQualityBuildDir",
            "-TemplateTableCliBookmarkBuildDir",
            "-TemplateTableCliColumnBuildDir",
            "-TemplateTableCliDirectColumnBuildDir",
            "-TemplateTableCliBuildDir",
            "-TemplateTableCliMergeUnmergeBuildDir",
            "-TemplateTableCliDirectBuildDir",
            "-TemplateTableCliDirectMergeUnmergeBuildDir",
            "-TemplateTableCliSectionKindBuildDir",
            "-TemplateTableCliSectionKindRowBuildDir",
            "-TemplateTableCliSectionKindColumnBuildDir",
            "-TemplateTableCliSectionKindMergeUnmergeBuildDir",
            "-TemplateTableCliSelectorBuildDir",
            "-ReplaceRemoveImageBuildDir"
        )
        $visualGateArgs = @(
            "-GateOutputDir"
            $resolvedGateOutputDir
            "-TaskOutputRoot"
            $resolvedTaskOutputRoot
            "-ReviewMode"
            $ReviewMode
            "-Dpi"
            $Dpi.ToString()
            "-SkipBuild"
        )
        $visualGateArgs = Add-OptionalVisualReviewArguments -Arguments $visualGateArgs `
            -VerdictArgument "-SmokeReviewVerdict" `
            -Verdict $SmokeReviewVerdict `
            -NoteArgument "-SmokeReviewNote" `
            -Note $SmokeReviewNote
        $visualGateArgs = Add-OptionalVisualReviewArguments -Arguments $visualGateArgs `
            -VerdictArgument "-FixedGridReviewVerdict" `
            -Verdict $FixedGridReviewVerdict `
            -NoteArgument "-FixedGridReviewNote" `
            -Note $FixedGridReviewNote
        $visualGateArgs = Add-OptionalVisualReviewArguments -Arguments $visualGateArgs `
            -VerdictArgument "-SectionPageSetupReviewVerdict" `
            -Verdict $SectionPageSetupReviewVerdict `
            -NoteArgument "-SectionPageSetupReviewNote" `
            -Note $SectionPageSetupReviewNote
        $visualGateArgs = Add-OptionalVisualReviewArguments -Arguments $visualGateArgs `
            -VerdictArgument "-PageNumberFieldsReviewVerdict" `
            -Verdict $PageNumberFieldsReviewVerdict `
            -NoteArgument "-PageNumberFieldsReviewNote" `
            -Note $PageNumberFieldsReviewNote
        $visualGateArgs = Add-OptionalVisualReviewArguments -Arguments $visualGateArgs `
            -VerdictArgument "-CuratedVisualReviewVerdict" `
            -Verdict $CuratedVisualReviewVerdict `
            -NoteArgument "-CuratedVisualReviewNote" `
            -Note $CuratedVisualReviewNote
        foreach ($argumentName in $visualGateBuildDirArguments) {
            $visualGateArgs += @(
                $argumentName
                $resolvedBuildDir
            )
        }
        if ($IncludeTableStyleQuality) {
            $visualGateArgs += "-IncludeTableStyleQuality"
        }
        if ($SkipReviewTasks) {
            $visualGateArgs += "-SkipReviewTasks"
        }
        if ($SkipSectionPageSetup) {
            $visualGateArgs += "-SkipSectionPageSetup"
        }
        if ($SkipPageNumberFields) {
            $visualGateArgs += "-SkipPageNumberFields"
        }
        if ($RefreshReadmeAssets) {
            $visualGateArgs += @(
                "-RefreshReadmeAssets"
                "-ReadmeAssetsDir"
                $ReadmeAssetsDir
            )
        }
        if ($KeepWordOpen) {
            $visualGateArgs += "-KeepWordOpen"
        }
        if ($VisibleWord) {
            $visualGateArgs += "-VisibleWord"
        }

        $visualGateOutput = Invoke-ChildPowerShell -ScriptPath $visualGateScript `
            -Arguments $visualGateArgs `
            -FailureMessage "Word visual release gate failed."
        $visualGateInfo = Parse-VisualGateOutput -Lines $visualGateOutput
        $summary.steps.visual_gate.status = "completed"
        $summary.steps.visual_gate.summary_json = $visualGateInfo.gate_summary
        $summary.steps.visual_gate.final_review = $visualGateInfo.gate_final_review
        if ($visualGateInfo.document_task_dir) {
            $summary.steps.visual_gate.document_task_dir = $visualGateInfo.document_task_dir
        }
        if ($visualGateInfo.fixed_grid_task_dir) {
            $summary.steps.visual_gate.fixed_grid_task_dir = $visualGateInfo.fixed_grid_task_dir
        }
        if ($visualGateInfo.section_page_setup_task_dir) {
            $summary.steps.visual_gate.section_page_setup_task_dir = $visualGateInfo.section_page_setup_task_dir
        }
        if ($visualGateInfo.page_number_fields_task_dir) {
            $summary.steps.visual_gate.page_number_fields_task_dir = $visualGateInfo.page_number_fields_task_dir
        }

        $gateSummary = Get-Content -Raw -LiteralPath $visualGateInfo.gate_summary | ConvertFrom-Json
        $gateVisualVerdict = Get-OptionalPropertyValue -Object $gateSummary -Name "visual_verdict"
        if (-not [string]::IsNullOrWhiteSpace([string]$gateVisualVerdict)) {
            $summary.visual_verdict = [string]$gateVisualVerdict
            $summary.steps.visual_gate.visual_verdict = [string]$gateVisualVerdict
        }
        $reviewTaskSummary = Get-CompleteVisualGateReviewTaskSummary -Summary (Get-OptionalPropertyValue -Object $gateSummary -Name "review_task_summary")
        if ($null -ne $reviewTaskSummary) {
            $summary.steps.visual_gate.review_task_summary = $reviewTaskSummary
        }
        Add-GateFlowReviewSummary -Target $summary.steps.visual_gate -Prefix "smoke" `
            -FlowInfo (Get-OptionalPropertyValue -Object $gateSummary -Name "smoke")
        Add-GateFlowReviewSummary -Target $summary.steps.visual_gate -Prefix "fixed_grid" `
            -FlowInfo (Get-OptionalPropertyValue -Object $gateSummary -Name "fixed_grid")
        Add-GateFlowReviewSummary -Target $summary.steps.visual_gate -Prefix "section_page_setup" `
            -FlowInfo (Get-OptionalPropertyValue -Object $gateSummary -Name "section_page_setup")
        Add-GateFlowReviewSummary -Target $summary.steps.visual_gate -Prefix "page_number_fields" `
            -FlowInfo (Get-OptionalPropertyValue -Object $gateSummary -Name "page_number_fields")
        $curatedVisualReviewEntries = @(Get-CuratedVisualReviewSummaryEntries -GateSummary $gateSummary)
        if ($curatedVisualReviewEntries.Count -gt 0) {
            $summary.steps.visual_gate.curated_visual_regressions = $curatedVisualReviewEntries
        }

        $readmeGalleryInfo = Get-ReadmeGalleryInfoFromGateSummary -GateSummaryPath $visualGateInfo.gate_summary
        $summary.readme_gallery = $readmeGalleryInfo
        $summary.steps.visual_gate.readme_gallery_status = $readmeGalleryInfo.status
        if ($readmeGalleryInfo.Contains("assets_dir")) {
            $summary.steps.visual_gate.readme_gallery_assets_dir = $readmeGalleryInfo.assets_dir
        }
    }

    $summary.execution_status = "pass"
