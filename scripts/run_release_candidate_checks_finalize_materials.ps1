    function Get-FinalReviewWorkflowDashboardValue {
        param(
            $Step,
            $Report,
            [string]$Name
        )

        $value = Get-OptionalPropertyValue -Object $Step -Name $Name
        if ([string]::IsNullOrWhiteSpace($value)) {
            $value = Get-OptionalPropertyValue -Object $Report -Name $Name
        }

        return $value
    }

    function Get-FinalReviewWorkflowDashboardObject {
        param(
            $Step,
            $Report,
            [string]$Name
        )

        $value = Get-OptionalPropertyValue -Object $Step -Name $Name
        if ($null -eq $value) {
            $value = Get-OptionalPropertyValue -Object $Report -Name $Name
        }

        return $value
    }

    function Get-FinalReviewWorkflowDashboardObjectArray {
        param(
            $Step,
            $Report,
            [string]$Name
        )

        $value = Get-FinalReviewWorkflowDashboardObject -Step $Step -Report $Report -Name $Name
        if ($null -eq $value) {
            return @()
        }

        if ($value -is [string]) {
            if ([string]::IsNullOrWhiteSpace($value)) {
                return @()
            }

            return @($value)
        }

        if ($value -is [System.Collections.IEnumerable] -and -not ($value -is [System.Collections.IDictionary])) {
            return @($value | Where-Object { $null -ne $_ })
        }

        return @($value)
    }

    function Get-FinalReviewWorkflowDashboardActionSummaryText {
        param($ActionGroup)

        if ($null -eq $ActionGroup) {
            return ""
        }

        if ($ActionGroup -is [string]) {
            return [string]$ActionGroup
        }

        $sourceReportId = Get-OptionalPropertyValue -Object $ActionGroup -Name "source_report_id"
        $action = Get-OptionalPropertyValue -Object $ActionGroup -Name "action"
        $blockerId = Get-OptionalPropertyValue -Object $ActionGroup -Name "blocker_id"
        $entryNameValues = Get-OptionalPropertyValue -Object $ActionGroup -Name "entry_names"
        [string[]]$entryNames = @()
        if ($entryNameValues -is [string]) {
            if (-not [string]::IsNullOrWhiteSpace($entryNameValues)) {
                $entryNames = @($entryNameValues)
            }
        } elseif ($entryNameValues -is [System.Collections.IEnumerable]) {
            $entryNames = @($entryNameValues | Where-Object { $null -ne $_ -and -not [string]::IsNullOrWhiteSpace([string]$_) } | ForEach-Object { [string]$_ })
        } elseif ($null -ne $entryNameValues) {
            $entryNames = @([string]$entryNameValues)
        }

        [string[]]$parts = @()
        if (-not [string]::IsNullOrWhiteSpace($sourceReportId)) {
            $parts += "source=$sourceReportId"
        }
        if (-not [string]::IsNullOrWhiteSpace($action)) {
            $parts += "action=$action"
        }
        $blockerText = if (-not [string]::IsNullOrWhiteSpace($blockerId)) { [string]$blockerId } else { "(none)" }
        $entryNamesText = if (@($entryNames).Count -gt 0) { @($entryNames) -join "," } else { "(none)" }
        $parts += "blocker=$blockerText"
        $parts += "entries=$entryNamesText"

        return ($parts -join " ")
    }

    function Get-FinalReviewWorkflowDashboardActionSourceSummaryText {
        param($SourceGroup)

        if ($null -eq $SourceGroup) {
            return ""
        }

        if ($SourceGroup -is [string]) {
            return [string]$SourceGroup
        }

        $sourceReportId = Get-OptionalPropertyValue -Object $SourceGroup -Name "source_report_id"
        $actionGroupCount = Get-OptionalPropertyValue -Object $SourceGroup -Name "action_group_count"
        $sourceJsonDisplay = Get-OptionalPropertyValue -Object $SourceGroup -Name "source_json_display"

        [string[]]$parts = @()
        if (-not [string]::IsNullOrWhiteSpace($sourceReportId)) {
            $parts += "source=$sourceReportId"
        }
        if (-not [string]::IsNullOrWhiteSpace($actionGroupCount)) {
            $parts += "action_group_count=$actionGroupCount"
        }
        if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            $parts += "source_json=$sourceJsonDisplay"
        }

        return ($parts -join " ")
    }

    ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8

    $repoRootDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $repoRoot
    $summaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summaryPath
    $buildDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedBuildDir
    $installDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedInstallDir
    $consumerBuildDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedConsumerBuildDir
    $gateOutputDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedGateOutputDir
    $taskOutputRootDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedTaskOutputRoot
    $templateSchemaBaselineDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.template_schema.baseline
    $templateSchemaGeneratedDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.template_schema.generated_output
    $templateSchemaManifestDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.template_schema_manifest.manifest_path
    $templateSchemaManifestSummaryDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.template_schema_manifest.summary_json
    $templateSchemaManifestOutputDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.template_schema_manifest.output_dir
    $projectTemplateSmokeManifestDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.manifest_path
    $projectTemplateSmokeSummaryDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.summary_json
    $projectTemplateSmokeOutputDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.output_dir
    $projectTemplateSmokeCandidateDiscoveryDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.candidate_discovery_json
    $projectTemplateWorkflowDashboardSummaryDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_workflow_dashboard_report.summary_json
    $projectTemplateWorkflowDashboardReportDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_workflow_dashboard_report.report_markdown
    $projectTemplateWorkflowDashboardStep = Get-OptionalPropertyValue -Object $summary.steps -Name "project_template_workflow_dashboard"
    $projectTemplateWorkflowDashboardReport = Get-OptionalPropertyValue -Object $summary -Name "project_template_workflow_dashboard_report"
    $projectTemplateWorkflowDashboardNextActionSummary = @(Get-FinalReviewWorkflowDashboardObjectArray `
            -Step $projectTemplateWorkflowDashboardStep `
            -Report $projectTemplateWorkflowDashboardReport `
            -Name "next_action_summary")
    $projectTemplateWorkflowDashboardNextActionGroupCount = Get-FinalReviewWorkflowDashboardValue `
        -Step $projectTemplateWorkflowDashboardStep `
        -Report $projectTemplateWorkflowDashboardReport `
        -Name "next_action_group_count"
    if ([string]::IsNullOrWhiteSpace($projectTemplateWorkflowDashboardNextActionGroupCount)) {
        $projectTemplateWorkflowDashboardNextActionGroupCount = @($projectTemplateWorkflowDashboardNextActionSummary).Count
    }
    $projectTemplateWorkflowDashboardActionSummaryMarkdown = @($projectTemplateWorkflowDashboardNextActionSummary |
        ForEach-Object { Get-FinalReviewWorkflowDashboardActionSummaryText -ActionGroup $_ } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        ForEach-Object { "- Project template workflow dashboard action group: $_" }) -join [Environment]::NewLine
    $projectTemplateWorkflowDashboardNextActionSummaryBySource = @(Get-FinalReviewWorkflowDashboardObjectArray `
            -Step $projectTemplateWorkflowDashboardStep `
            -Report $projectTemplateWorkflowDashboardReport `
            -Name "next_action_summary_by_source")
    $projectTemplateWorkflowDashboardActionSourceSummaryMarkdown = @($projectTemplateWorkflowDashboardNextActionSummaryBySource |
        ForEach-Object { Get-FinalReviewWorkflowDashboardActionSourceSummaryText -SourceGroup $_ } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        ForEach-Object { "- Project template workflow dashboard action source: $_" }) -join [Environment]::NewLine
    $releaseGovernanceHandoffSummaryDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.release_governance_handoff.summary_json
    $releaseGovernanceHandoffReportDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.release_governance_handoff.report_markdown
    $releaseHandoffDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseHandoffPath
    $releaseBodyDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseBodyZhCnPath
    $releaseSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseSummaryZhCnPath
    $artifactGuideDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $artifactGuidePath
    $reviewerChecklistDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $reviewerChecklistPath
    $schemaApprovalHistoryJsonDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.schema_patch_approval_history_json
    $schemaApprovalHistoryMarkdownDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.schema_patch_approval_history_markdown
    $startHereDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $startHerePath
    $pdfVisualGateSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_gate.summary_json
    $pdfVisualGateContactSheetDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_gate.aggregate_contact_sheet
    $pdfVisualGateAttemptSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_gate_attempt.summary_json
    $pdfVisualGateAttemptContactSheetDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_gate_attempt.aggregate_contact_sheet
    $pdfVisualSegmentedGateSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_segmented_gate.summary_json
    $pdfVisualSegmentedGateContactSheetDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_segmented_gate.aggregate_contact_sheet
    $pdfFullCtestReadinessSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_full_ctest_readiness.summary_json
    $pdfFullCtestSummaryDisplayPath = if (-not [string]::IsNullOrWhiteSpace([string]$summary.steps.pdf_full_ctest_readiness.full_ctest_summary_json_display)) {
        [string]$summary.steps.pdf_full_ctest_readiness.full_ctest_summary_json_display
    } else {
        Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_full_ctest_readiness.full_ctest_summary_json
    }
    $pdfBoundedCtestSubsetsDisplay = if (@($summary.steps.pdf_bounded_ctest.subsets).Count -gt 0) {
        @($summary.steps.pdf_bounded_ctest.subsets) -join ", "
    } else {
        "(not available)"
    }
    $pdfBoundedCtestSummaryDisplay = if (@($summary.steps.pdf_bounded_ctest.summary_json_display).Count -gt 0) {
        @($summary.steps.pdf_bounded_ctest.summary_json_display) -join ", "
    } else {
        "(not available)"
    }
    $pdfBoundedCtestImportDiagnosticsContractTests = @(Get-OptionalPropertyValue -Object $summary.steps.pdf_bounded_ctest -Name "import_diagnostics_contract_tests")
    $pdfBoundedCtestImportDiagnosticsContractFields = @(Get-OptionalPropertyValue -Object $summary.steps.pdf_bounded_ctest -Name "import_diagnostics_contract_fields")
    $pdfBoundedCtestImportNegativeBoundaryCases = @(Get-OptionalPropertyValue -Object $summary.steps.pdf_bounded_ctest -Name "import_negative_boundary_contract_cases")
    $pdfBoundedCtestImportDiagnosticsContractTestsDisplay = if ($pdfBoundedCtestImportDiagnosticsContractTests.Count -gt 0) {
        $pdfBoundedCtestImportDiagnosticsContractTests -join ", "
    } else {
        "(not available)"
    }
    $pdfBoundedCtestImportDiagnosticsContractFieldsDisplay = if ($pdfBoundedCtestImportDiagnosticsContractFields.Count -gt 0) {
        $pdfBoundedCtestImportDiagnosticsContractFields -join ", "
    } else {
        "(not available)"
    }
    $pdfBoundedCtestImportNegativeBoundaryCasesDisplay = if ($pdfBoundedCtestImportNegativeBoundaryCases.Count -gt 0) {
        $pdfBoundedCtestImportNegativeBoundaryCases -join ", "
    } else {
        "(not available)"
    }

    $readmeGalleryStatusLine = switch ($summary.readme_gallery.status) {
        "completed" {
            "- README gallery refresh: completed ($(Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.readme_gallery.assets_dir))"
        }
        "not_requested" {
            "- README gallery refresh: not requested"
        }
        "visual_gate_skipped" {
            "- README gallery refresh: unavailable (visual gate skipped)"
        }
        default {
            "- README gallery refresh: $($summary.readme_gallery.status)"
        }
    }
    $visualGateReviewTaskSummaryLine = Get-VisualGateReviewTaskSummaryLine `
        -VisualGateStep $summary.steps.visual_gate
    $visualGateReviewSummary = Get-VisualGateReviewSummaryMarkdown -RepoRoot $repoRoot `
        -VisualGateStep $summary.steps.visual_gate
    $releaseGovernanceRollupLines = New-Object 'System.Collections.Generic.List[string]'
    Add-ReleaseGovernanceRollupMarkdownSection `
        -Lines $releaseGovernanceRollupLines `
        -Summary $summary `
        -RepoRoot $repoRoot `
        -Heading "## Release blocker rollup details"
    $releaseGovernanceRollupMarkdown = if ($releaseGovernanceRollupLines.Count -gt 0) {
        ($releaseGovernanceRollupLines.ToArray() -join [Environment]::NewLine) + [Environment]::NewLine
    } else {
        ""
    }
    $releaseGovernanceHandoffLines = New-Object 'System.Collections.Generic.List[string]'
    Add-ReleaseGovernanceHandoffMarkdownSection `
        -Lines $releaseGovernanceHandoffLines `
        -Summary $summary `
        -RepoRoot $repoRoot `
        -Heading "## Release governance handoff details"
    $releaseGovernanceHandoffMarkdown = if ($releaseGovernanceHandoffLines.Count -gt 0) {
        ($releaseGovernanceHandoffLines.ToArray() -join [Environment]::NewLine) + [Environment]::NewLine
    } else {
        ""
    }
    $projectTemplateChecklistHandoffEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine -Summary $summary
    $projectTemplateChecklistMaterialSafetyAuditEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine -Summary $summary
    $wordVisualStandardReviewMetadataEvidenceLine = Get-ReleaseGovernanceWordVisualStandardReviewMetadataEvidenceLine -Summary $summary
    $projectTemplateChecklistEvidenceLines = New-Object 'System.Collections.Generic.List[string]'
    foreach ($evidenceLine in @(
            $projectTemplateChecklistHandoffEvidenceLine,
            $projectTemplateChecklistMaterialSafetyAuditEvidenceLine
        )) {
        if (-not [string]::IsNullOrWhiteSpace($evidenceLine)) {
            [void]$projectTemplateChecklistEvidenceLines.Add("- $evidenceLine")
        }
    }
    $projectTemplateChecklistEvidenceMarkdown = if ($projectTemplateChecklistEvidenceLines.Count -gt 0) {
        "## Project-template release entry evidence" + [Environment]::NewLine +
        [Environment]::NewLine +
        ($projectTemplateChecklistEvidenceLines.ToArray() -join [Environment]::NewLine) +
        [Environment]::NewLine
    } else {
        ""
    }
    $wordVisualStandardReviewMetadataEvidenceMarkdown = if (-not [string]::IsNullOrWhiteSpace($wordVisualStandardReviewMetadataEvidenceLine)) {
        "## Word visual standard review metadata evidence" + [Environment]::NewLine +
        [Environment]::NewLine +
        "- $wordVisualStandardReviewMetadataEvidenceLine" +
        [Environment]::NewLine
    } else {
        ""
    }

    $finalReview = @"
# Release Candidate Checks

- Generated at: $(Get-Date -Format s)
- Workspace: $repoRootDisplay
- Summary JSON: $summaryDisplayPath
- Execution status: $($summary.execution_status)
- Failed step: $($summary.failed_step)
- Error: $($summary.error)
- Release blockers: $($summary.release_blocker_count)
- Warnings: $($summary.warning_count)
- MSVC bootstrap mode: $($summary.msvc_bootstrap_mode)

## Step status

- Configure: $($summary.steps.configure.status)
- Build: $($summary.steps.build.status)
- Tests: $($summary.steps.tests.status)
- Template schema: $($summary.steps.template_schema.status)
- Template schema manifest: $($summary.steps.template_schema_manifest.status)
- Project template smoke: $($summary.steps.project_template_smoke.status)
- Install smoke: $($summary.steps.install_smoke.status)
- Visual gate: $($summary.steps.visual_gate.status)
- PDF visual gate: $($summary.steps.pdf_visual_gate.status)
- PDF visual gate verdict: $($summary.steps.pdf_visual_gate.verdict)
- PDF visual gate counts: $($summary.steps.pdf_visual_gate.visual_baseline_count) visual baselines, $($summary.steps.pdf_visual_gate.cjk_copy_search_count) CJK copy/search
- PDF visual gate manifest counts: $($summary.steps.pdf_visual_gate.visual_baseline_manifest_count) visual baseline manifest samples, $($summary.steps.pdf_visual_gate.cjk_manifest_count) CJK manifest samples
- PDF visual gate finalizable: $($summary.steps.pdf_visual_gate.finalizable)
- PDF visual gate attempt: $($summary.steps.pdf_visual_gate_attempt.status)
- PDF visual gate attempt verdict: $($summary.steps.pdf_visual_gate_attempt.verdict)
- PDF visual gate attempt full status: $($summary.steps.pdf_visual_gate_attempt.full_visual_gate_status)
- PDF visual gate attempt stages: $($summary.steps.pdf_visual_gate_attempt.passed_stage_count)/$($summary.steps.pdf_visual_gate_attempt.stage_count) passed, $($summary.steps.pdf_visual_gate_attempt.incomplete_stage_count) incomplete
- PDF visual gate attempt pdf_regression: $($summary.steps.pdf_visual_gate_attempt.pdf_regression_selected_test_count) selected, $($summary.steps.pdf_visual_gate_attempt.pdf_regression_failed_test_count) failed, $($summary.steps.pdf_visual_gate_attempt.pdf_regression_skipped_test_count) skipped
- PDF visual gate attempt render: $($summary.steps.pdf_visual_gate_attempt.visual_baseline_fresh_rendered_count)/$($summary.steps.pdf_visual_gate_attempt.expected_visual_render_count) fresh baselines, contact sheet $($summary.steps.pdf_visual_gate_attempt.aggregate_contact_sheet_status)
- PDF visual segmented gate: $($summary.steps.pdf_visual_segmented_gate.status)
- PDF visual segmented gate verdict: $($summary.steps.pdf_visual_segmented_gate.verdict)
- PDF visual segmented gate full status: $($summary.steps.pdf_visual_segmented_gate.full_visual_gate_status)
- PDF visual segmented gate scope: $($summary.steps.pdf_visual_segmented_gate.evidence_scope)
- PDF visual segmented gate slices: $($summary.steps.pdf_visual_segmented_gate.slice_pass_count)/$($summary.steps.pdf_visual_segmented_gate.slice_summary_count) pass
- PDF visual segmented gate coverage: $($summary.steps.pdf_visual_segmented_gate.covered_baseline_count)/$($summary.steps.pdf_visual_segmented_gate.expected_visual_render_count) baselines, contact sheet $($summary.steps.pdf_visual_segmented_gate.aggregate_contact_sheet_status)
- PDF visual release evidence accepted: $($summary.steps.pdf_full_ctest_readiness.visual_gate_release_evidence_accepted) (fresh full guarded $($summary.steps.pdf_full_ctest_readiness.visual_gate_fresh_full_guarded_evidence), pass summary before outer timeout $($summary.steps.pdf_full_ctest_readiness.visual_gate_pass_summary_before_outer_timeout), segmented full coverage $($summary.steps.pdf_full_ctest_readiness.visual_gate_segmented_full_coverage_evidence))
- PDF bounded CTest summaries: $($summary.steps.pdf_bounded_ctest.summary_count) summaries, $($summary.steps.pdf_bounded_ctest.pass_count) pass
- PDF bounded CTest subsets: $pdfBoundedCtestSubsetsDisplay
- PDF bounded CTest selected tests: $($summary.steps.pdf_bounded_ctest.selected_test_count)
- PDF bounded CTest skipped tests: $($summary.steps.pdf_bounded_ctest.skipped_test_count)
- PDF bounded CTest import diagnostics contract tests: $pdfBoundedCtestImportDiagnosticsContractTestsDisplay
- PDF bounded CTest import diagnostics contract fields: $pdfBoundedCtestImportDiagnosticsContractFieldsDisplay
- PDF bounded CTest import negative boundary cases: $pdfBoundedCtestImportNegativeBoundaryCasesDisplay
- PDF full CTest readiness: $($summary.steps.pdf_full_ctest_readiness.full_ctest_status) ($($summary.steps.pdf_full_ctest_readiness.completion_percent)% complete)
- PDF full CTest progress: $($summary.steps.pdf_full_ctest_readiness.completed_test_count)/$($summary.steps.pdf_full_ctest_readiness.selected_test_count) completed, $($summary.steps.pdf_full_ctest_readiness.not_run_test_count) not run
- PDF full CTest observed failures: $($summary.steps.pdf_full_ctest_readiness.failed_test_count), zero failed observed $($summary.steps.pdf_full_ctest_readiness.zero_failed_tests_observed)
- Release blocker rollup: $($summary.steps.release_blocker_rollup.status)
- Release governance handoff: $($summary.steps.release_governance_handoff.status)
- Project template workflow dashboard: $($summary.steps.project_template_workflow_dashboard.status)
- Project template workflow dashboard next action groups: $projectTemplateWorkflowDashboardNextActionGroupCount
$projectTemplateWorkflowDashboardActionSummaryMarkdown
$projectTemplateWorkflowDashboardActionSourceSummaryMarkdown
$visualGateReviewTaskSummaryLine
$readmeGalleryStatusLine
$visualGateReviewSummary
$releaseGovernanceRollupMarkdown
$releaseGovernanceHandoffMarkdown
$projectTemplateChecklistEvidenceMarkdown
$wordVisualStandardReviewMetadataEvidenceMarkdown
## Key outputs

- Build directory: $buildDirDisplay
- Install directory: $installDirDisplay
- Consumer build directory: $consumerBuildDirDisplay
- Visual gate output: $gateOutputDirDisplay
- Review task root: $taskOutputRootDisplay
- Template schema baseline: $templateSchemaBaselineDisplay
- Template schema generated output: $templateSchemaGeneratedDisplay
- Template schema manifest: $templateSchemaManifestDisplay
- Template schema manifest summary: $templateSchemaManifestSummaryDisplay
- Template schema manifest output dir: $templateSchemaManifestOutputDirDisplay
- Project template smoke manifest: $projectTemplateSmokeManifestDisplay
- Project template smoke summary: $projectTemplateSmokeSummaryDisplay
- Project template smoke output dir: $projectTemplateSmokeOutputDirDisplay
- Project template smoke candidate discovery: $projectTemplateSmokeCandidateDiscoveryDisplay
- Project template smoke candidate coverage: $($summary.project_template_smoke.registered_candidate_count)/$($summary.project_template_smoke.unregistered_candidate_count)/$($summary.project_template_smoke.excluded_candidate_count) registered/unregistered/excluded
- Project template smoke dirty schema baselines: $($summary.project_template_smoke.dirty_schema_baseline_count)
- Project template smoke schema baseline drifts: $($summary.project_template_smoke.schema_baseline_drift_count)
- Project template smoke schema approval gate: $($summary.project_template_smoke.schema_patch_approval_gate_status)
- Project template smoke schema approval compliance issues: $($summary.project_template_smoke.schema_patch_approval_compliance_issue_count)
- Project template smoke schema approval invalid results: $($summary.project_template_smoke.schema_patch_approval_invalid_result_count)
- Project template smoke schema approval history: $($summary.project_template_smoke.schema_patch_approval_history_status) ($schemaApprovalHistoryJsonDisplayPath, $schemaApprovalHistoryMarkdownDisplayPath)
- Release blocker rollup summary: $(Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.release_blocker_rollup.summary_json)
- Release blocker rollup report: $(Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.release_blocker_rollup.report_markdown)
- Release blocker rollup counts: $($summary.release_blocker_rollup.release_blocker_count) blockers, $($summary.release_blocker_rollup.action_item_count) actions, $($summary.release_blocker_rollup.warning_count) warnings
- Release governance handoff summary: $releaseGovernanceHandoffSummaryDisplay
- Release governance handoff report: $releaseGovernanceHandoffReportDisplay
- Release governance handoff counts: $($summary.release_governance_handoff.loaded_report_count)/$($summary.release_governance_handoff.expected_report_count) reports, $($summary.release_governance_handoff.missing_report_count) missing, $($summary.release_governance_handoff.release_blocker_count) blockers, $($summary.release_governance_handoff.action_item_count) actions
- Project template workflow dashboard summary: $projectTemplateWorkflowDashboardSummaryDisplay
- Project template workflow dashboard report: $projectTemplateWorkflowDashboardReportDisplay
- Project template workflow dashboard counts: $($summary.project_template_workflow_dashboard_report.source_report_count) reports, $($summary.project_template_workflow_dashboard_report.release_blocker_count) blockers, $($summary.project_template_workflow_dashboard_report.warning_count) warnings
- Release handoff: $releaseHandoffDisplayPath
- Release body: $releaseBodyDisplayPath
- Release summary: $releaseSummaryDisplayPath
- Artifact guide: $artifactGuideDisplayPath
- Reviewer checklist: $reviewerChecklistDisplayPath
- Start here: $startHereDisplayPath
- PDF visual gate summary: $pdfVisualGateSummaryDisplayPath
- PDF visual gate contact sheet: $pdfVisualGateContactSheetDisplayPath
- PDF visual gate attempt summary: $pdfVisualGateAttemptSummaryDisplayPath
- PDF visual gate attempt contact sheet: $pdfVisualGateAttemptContactSheetDisplayPath
- PDF visual segmented gate summary: $pdfVisualSegmentedGateSummaryDisplayPath
- PDF visual segmented gate contact sheet: $pdfVisualSegmentedGateContactSheetDisplayPath
- PDF bounded CTest summaries: $pdfBoundedCtestSummaryDisplay
- PDF bounded CTest import diagnostics contract fields: $pdfBoundedCtestImportDiagnosticsContractFieldsDisplay
- PDF release readiness summary: $pdfFullCtestReadinessSummaryDisplayPath
- PDF full CTest summary: $pdfFullCtestSummaryDisplayPath
"@
    $finalReview | Set-Content -Path $finalReviewPath -Encoding UTF8

    try {
        Invoke-ChildPowerShell -ScriptPath $releaseNoteBundleScript `
            -Arguments @(
                "-SummaryJson"
                $summaryPath
                "-ReleaseVersion"
                $projectVersion
                "-HandoffOutputPath"
                $releaseHandoffPath
                "-BodyOutputPath"
                $releaseBodyZhCnPath
                "-ShortOutputPath"
                $releaseSummaryZhCnPath
                "-GuideOutputPath"
                $artifactGuidePath
                "-ChecklistOutputPath"
                $reviewerChecklistPath
                "-StartHereOutputPath"
                $startHerePath
                "-SkipMaterialSafetyAudit"
            ) `
            -FailureMessage "Failed to write release note bundle."
    } catch {
        Write-Step $_.Exception.Message
    }

    foreach ($releaseMaterialPath in @(
            $summaryPath,
            $finalReviewPath,
            $releaseHandoffPath,
            $releaseBodyZhCnPath,
            $releaseSummaryZhCnPath,
            $artifactGuidePath,
            $reviewerChecklistPath,
            $startHerePath
        )) {
        Convert-ReleaseMaterialFile -RepoRoot $repoRoot -Path $releaseMaterialPath
    }

    if (-not $SkipMaterialSafetyAudit.IsPresent) {
        & $releaseMaterialAuditScript -Path @(
            $summaryPath,
            $finalReviewPath,
            $releaseHandoffPath,
            $releaseBodyZhCnPath,
            $releaseSummaryZhCnPath,
            $artifactGuidePath,
            $reviewerChecklistPath,
            $startHerePath
        )
    } else {
        Write-Step "Skipping final release material safety audit"
    }
