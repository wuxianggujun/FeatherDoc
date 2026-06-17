    ($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

    if ($projectTemplateSmokeRequested) {
        try {
            $historyInfo = Invoke-ProjectTemplateSchemaApprovalHistory `
                -ScriptPath $projectTemplateSchemaApprovalHistoryScript `
                -ReleaseSummaryPath $summaryPath `
                -ProjectTemplateSmokeSummaryPath $summary.project_template_smoke.summary_json `
                -OutputJson $schemaApprovalHistoryJsonPath `
                -OutputMarkdown $schemaApprovalHistoryMarkdownPath
            $summary.project_template_smoke.schema_patch_approval_history_status = [string]$historyInfo.status
            $summary.project_template_smoke.schema_patch_approval_history_input_count = [int]$historyInfo.input_count
            $summary.project_template_smoke.schema_patch_approval_history_error = ""
            $summary.steps.project_template_smoke.schema_patch_approval_history_status = [string]$historyInfo.status
            $summary.steps.project_template_smoke.schema_patch_approval_history_input_count = [int]$historyInfo.input_count
            $summary.steps.project_template_smoke.schema_patch_approval_history_error = ""
        } catch {
            $historyError = $_.Exception.Message
            $summary.project_template_smoke.schema_patch_approval_history_status = "failed"
            $summary.project_template_smoke.schema_patch_approval_history_error = $historyError
            $summary.steps.project_template_smoke.schema_patch_approval_history_status = "failed"
            $summary.steps.project_template_smoke.schema_patch_approval_history_error = $historyError
            Write-Step "Project template schema approval history generation failed: $historyError"
        }

        $schemaApprovalHistorySummary = Read-ProjectTemplateSchemaApprovalHistorySummary -Path $schemaApprovalHistoryJsonPath
        $projectTemplateSmokeSummaryForBlocker = if (-not [string]::IsNullOrWhiteSpace($summary.project_template_smoke.summary_json) -and
            (Test-Path -LiteralPath $summary.project_template_smoke.summary_json)) {
            Get-Content -Raw -Encoding UTF8 -LiteralPath $summary.project_template_smoke.summary_json | ConvertFrom-Json
        } else {
            $summary.project_template_smoke
        }
        $schemaApprovalReleaseBlocker = New-ProjectTemplateSchemaApprovalReleaseBlocker `
            -ProjectTemplateSmokeSummary $projectTemplateSmokeSummaryForBlocker `
            -HistorySummary $schemaApprovalHistorySummary `
            -SummaryJsonPath $summary.project_template_smoke.summary_json `
            -HistoryJsonPath $summary.project_template_smoke.schema_patch_approval_history_json `
            -HistoryMarkdownPath $summary.project_template_smoke.schema_patch_approval_history_markdown
        Set-ProjectTemplateSchemaApprovalReleaseBlocker `
            -ReleaseSummary $summary `
            -Blocker $schemaApprovalReleaseBlocker
        ($summary | ConvertTo-Json -Depth 10) | Set-Content -Path $summaryPath -Encoding UTF8
    }

    $releaseWarnings = @(Get-PdfVisualGateAttemptReleaseWarnings -ReleaseSummary $summary -RepoRoot $repoRoot)
    Set-ReleaseSummaryWarnings -ReleaseSummary $summary -Warnings $releaseWarnings
    ($summary | ConvertTo-Json -Depth 10) | Set-Content -Path $summaryPath -Encoding UTF8

    $effectiveReleaseGovernanceHandoffInputJson = @(Get-ReleaseGovernanceHandoffEffectiveInputJson `
            -InputJson $resolvedReleaseGovernanceHandoffInputJson `
            -ReleaseSummaryPath $summaryPath)
    if ($releaseGovernanceHandoffRequested) {
        $summary.release_governance_handoff.input_json = @($effectiveReleaseGovernanceHandoffInputJson)
    }

    if ($releaseBlockerRollupRequested) {
        try {
            Write-Step "Building release blocker rollup"
            Invoke-ReleaseBlockerRollup `
                -ScriptPath $releaseBlockerRollupScript `
                -InputJson $resolvedReleaseBlockerRollupInputJson `
                -InputRoot $resolvedReleaseBlockerRollupInputRoot `
                -OutputDir $resolvedReleaseBlockerRollupOutputDir `
                -SummaryJson $releaseBlockerRollupSummaryPath `
                -ReportMarkdown $releaseBlockerRollupMarkdownPath `
                -FailOnBlocker ([bool]$ReleaseBlockerRollupFailOnBlocker) `
                -FailOnWarning ([bool]$ReleaseBlockerRollupFailOnWarning)
            $rollupSummary = Read-ReleaseBlockerRollupSummary -Path $releaseBlockerRollupSummaryPath
            [object[]]$rollupGovernanceMetrics = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "governance_metrics") }
            [object[]]$rollupBlockerSourceSchemaSummary = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "blocker_source_schema_summary") }
            [object[]]$rollupActionItemSourceSchemaSummary = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "action_item_source_schema_summary") }
            [object[]]$rollupInformationalActionItemSourceSchemaSummary = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "informational_action_item_source_schema_summary") }
            [object[]]$rollupWarningSourceSchemaSummary = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "warning_source_schema_summary") }
            $summary.release_blocker_rollup.status = if ($null -eq $rollupSummary) { "missing_summary" } else { [string]$rollupSummary.status }
            $summary.release_blocker_rollup.source_report_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.source_report_count }
            $summary.release_blocker_rollup.source_failure_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.source_failure_count }
            $summary.release_blocker_rollup.release_blocker_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.release_blocker_count }
            $summary.release_blocker_rollup.release_blockers = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "release_blockers") }
            $summary.release_blocker_rollup.blocker_source_schema_summary = @($rollupBlockerSourceSchemaSummary)
            $summary.release_blocker_rollup.action_item_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.action_item_count }
            $summary.release_blocker_rollup.action_items = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "action_items") }
            $summary.release_blocker_rollup.action_item_source_schema_summary = @($rollupActionItemSourceSchemaSummary)
            $summary.release_blocker_rollup.informational_action_item_source_schema_summary = @($rollupInformationalActionItemSourceSchemaSummary)
            $summary.release_blocker_rollup.warning_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.warning_count }
            $summary.release_blocker_rollup.warnings = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "warnings") }
            $summary.release_blocker_rollup.warning_source_schema_summary = @($rollupWarningSourceSchemaSummary)
            $summary.release_blocker_rollup.governance_metric_count = if ($null -eq $rollupSummary) { 0 } else { [int](Get-OptionalIntegerProperty -Object $rollupSummary -Name "governance_metric_count" -DefaultValue @($rollupGovernanceMetrics).Count) }
            $summary.release_blocker_rollup.governance_metrics = @($rollupGovernanceMetrics)
            $summary.release_blocker_rollup.error = ""
            $summary.steps.release_blocker_rollup.status = $summary.release_blocker_rollup.status
            $summary.steps.release_blocker_rollup.source_report_count = $summary.release_blocker_rollup.source_report_count
            $summary.steps.release_blocker_rollup.source_failure_count = $summary.release_blocker_rollup.source_failure_count
            $summary.steps.release_blocker_rollup.release_blocker_count = $summary.release_blocker_rollup.release_blocker_count
            $summary.steps.release_blocker_rollup.release_blockers = @($summary.release_blocker_rollup.release_blockers)
            $summary.steps.release_blocker_rollup.blocker_source_schema_summary = @($summary.release_blocker_rollup.blocker_source_schema_summary)
            $summary.steps.release_blocker_rollup.action_item_count = $summary.release_blocker_rollup.action_item_count
            $summary.steps.release_blocker_rollup.action_items = @($summary.release_blocker_rollup.action_items)
            $summary.steps.release_blocker_rollup.action_item_source_schema_summary = @($summary.release_blocker_rollup.action_item_source_schema_summary)
            $summary.steps.release_blocker_rollup.informational_action_item_source_schema_summary = @($summary.release_blocker_rollup.informational_action_item_source_schema_summary)
            $summary.steps.release_blocker_rollup.warning_count = $summary.release_blocker_rollup.warning_count
            $summary.steps.release_blocker_rollup.warnings = @($summary.release_blocker_rollup.warnings)
            $summary.steps.release_blocker_rollup.warning_source_schema_summary = @($summary.release_blocker_rollup.warning_source_schema_summary)
            $summary.steps.release_blocker_rollup.governance_metric_count = $summary.release_blocker_rollup.governance_metric_count
            $summary.steps.release_blocker_rollup.governance_metrics = @($summary.release_blocker_rollup.governance_metrics)
            $summary.steps.release_blocker_rollup.error = ""
            ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8
        } catch {
            $rollupError = $_.Exception.Message
            $rollupSummary = Read-ReleaseBlockerRollupSummary -Path $releaseBlockerRollupSummaryPath
            [object[]]$rollupGovernanceMetrics = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "governance_metrics") }
            [object[]]$rollupBlockerSourceSchemaSummary = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "blocker_source_schema_summary") }
            [object[]]$rollupActionItemSourceSchemaSummary = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "action_item_source_schema_summary") }
            [object[]]$rollupInformationalActionItemSourceSchemaSummary = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "informational_action_item_source_schema_summary") }
            [object[]]$rollupWarningSourceSchemaSummary = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "warning_source_schema_summary") }
            $summary.release_blocker_rollup.status = "failed"
            $summary.release_blocker_rollup.source_report_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.source_report_count }
            $summary.release_blocker_rollup.source_failure_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.source_failure_count }
            $summary.release_blocker_rollup.release_blocker_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.release_blocker_count }
            $summary.release_blocker_rollup.release_blockers = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "release_blockers") }
            $summary.release_blocker_rollup.blocker_source_schema_summary = @($rollupBlockerSourceSchemaSummary)
            $summary.release_blocker_rollup.action_item_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.action_item_count }
            $summary.release_blocker_rollup.action_items = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "action_items") }
            $summary.release_blocker_rollup.action_item_source_schema_summary = @($rollupActionItemSourceSchemaSummary)
            $summary.release_blocker_rollup.informational_action_item_source_schema_summary = @($rollupInformationalActionItemSourceSchemaSummary)
            $summary.release_blocker_rollup.warning_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.warning_count }
            $summary.release_blocker_rollup.warnings = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "warnings") }
            $summary.release_blocker_rollup.warning_source_schema_summary = @($rollupWarningSourceSchemaSummary)
            $summary.release_blocker_rollup.governance_metric_count = if ($null -eq $rollupSummary) { 0 } else { [int](Get-OptionalIntegerProperty -Object $rollupSummary -Name "governance_metric_count" -DefaultValue @($rollupGovernanceMetrics).Count) }
            $summary.release_blocker_rollup.governance_metrics = @($rollupGovernanceMetrics)
            $summary.release_blocker_rollup.error = $rollupError
            $summary.steps.release_blocker_rollup.status = "failed"
            $summary.steps.release_blocker_rollup.source_report_count = $summary.release_blocker_rollup.source_report_count
            $summary.steps.release_blocker_rollup.source_failure_count = $summary.release_blocker_rollup.source_failure_count
            $summary.steps.release_blocker_rollup.release_blocker_count = $summary.release_blocker_rollup.release_blocker_count
            $summary.steps.release_blocker_rollup.release_blockers = @($summary.release_blocker_rollup.release_blockers)
            $summary.steps.release_blocker_rollup.blocker_source_schema_summary = @($summary.release_blocker_rollup.blocker_source_schema_summary)
            $summary.steps.release_blocker_rollup.action_item_count = $summary.release_blocker_rollup.action_item_count
            $summary.steps.release_blocker_rollup.action_items = @($summary.release_blocker_rollup.action_items)
            $summary.steps.release_blocker_rollup.action_item_source_schema_summary = @($summary.release_blocker_rollup.action_item_source_schema_summary)
            $summary.steps.release_blocker_rollup.informational_action_item_source_schema_summary = @($summary.release_blocker_rollup.informational_action_item_source_schema_summary)
            $summary.steps.release_blocker_rollup.warning_count = $summary.release_blocker_rollup.warning_count
            $summary.steps.release_blocker_rollup.warnings = @($summary.release_blocker_rollup.warnings)
            $summary.steps.release_blocker_rollup.warning_source_schema_summary = @($summary.release_blocker_rollup.warning_source_schema_summary)
            $summary.steps.release_blocker_rollup.governance_metric_count = $summary.release_blocker_rollup.governance_metric_count
            $summary.steps.release_blocker_rollup.governance_metrics = @($summary.release_blocker_rollup.governance_metrics)
            $summary.steps.release_blocker_rollup.error = $rollupError
            ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8
            Write-Step "Release blocker rollup failed: $rollupError"
            if ($ReleaseBlockerRollupFailOnBlocker -or $ReleaseBlockerRollupFailOnWarning) {
                $releaseBlockerRollupFailure = $rollupError
            }
        }
    }

    if ($releaseGovernanceHandoffRequested) {
        try {
            Write-Step "Building release governance handoff"
            Invoke-ReleaseGovernanceHandoff `
                -ScriptPath $releaseGovernanceHandoffScript `
                -InputRoot $resolvedReleaseGovernanceHandoffInputRoot `
                -InputJson $effectiveReleaseGovernanceHandoffInputJson `
                -OutputDir $resolvedReleaseGovernanceHandoffOutputDir `
                -SummaryJson $releaseGovernanceHandoffSummaryPath `
                -ReportMarkdown $releaseGovernanceHandoffMarkdownPath `
                -ExpectedReportProfile $ReleaseGovernanceHandoffExpectedReportProfile `
                -IncludeRollup ([bool]$ReleaseGovernanceHandoffIncludeRollup) `
                -FailOnMissing ([bool]$ReleaseGovernanceHandoffFailOnMissing) `
                -FailOnBlocker ([bool]$ReleaseGovernanceHandoffFailOnBlocker) `
                -FailOnWarning ([bool]$ReleaseGovernanceHandoffFailOnWarning)
            $handoffSummary = Read-ReleaseGovernanceHandoffSummary -Path $releaseGovernanceHandoffSummaryPath
            $summary.release_governance_handoff.status = if ($null -eq $handoffSummary) { "missing_summary" } else { [string]$handoffSummary.status }
            $summary.release_governance_handoff.expected_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.expected_report_count }
            $summary.release_governance_handoff.loaded_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.loaded_report_count }
            $summary.release_governance_handoff.missing_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.missing_report_count }
            $summary.release_governance_handoff.failed_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.failed_report_count }
            $summary.release_governance_handoff.release_blocker_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.release_blocker_count }
            $summary.release_governance_handoff.release_blockers = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "release_blockers") }
            $summary.release_governance_handoff.action_item_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.action_item_count }
            $summary.release_governance_handoff.action_items = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "action_items") }
            $summary.release_governance_handoff.warning_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.warning_count }
            $summary.release_governance_handoff.warnings = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "warnings") }
            [object[]]$handoffReports = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "reports") }
            [object[]]$handoffGovernanceMetrics = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "governance_metrics") }
            $summary.release_governance_handoff.report_count = @($handoffReports).Count
            $summary.release_governance_handoff.reports = @($handoffReports)
            $summary.release_governance_handoff.governance_metric_count = if ($null -eq $handoffSummary) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffSummary -Name "governance_metric_count" -DefaultValue @($handoffGovernanceMetrics).Count) }
            $summary.release_governance_handoff.governance_metrics = @($handoffGovernanceMetrics)
            $summary.release_governance_handoff.project_template_delivery_readiness_contract = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "project_template_delivery_readiness_contract" }
            $summary.release_governance_handoff.project_template_onboarding_governance_contract = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "project_template_onboarding_governance_contract" }
            $summary.governance_metric_count = $summary.release_governance_handoff.governance_metric_count
            $summary.governance_metrics = @($summary.release_governance_handoff.governance_metrics)
            $handoffRollup = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "release_blocker_rollup" }
            $summary.release_governance_handoff.release_blocker_rollup = $handoffRollup
            $summary.release_governance_handoff.manifest_signoff_entrypoints_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "manifest_signoff_entrypoints_source_report_count") }
            $summary.release_governance_handoff.manifest_signoff_entrypoints_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "manifest_signoff_entrypoints_source_reports") }
            $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "project_template_readiness_checklist_entrypoints_source_report_count") }
            $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "project_template_readiness_checklist_entrypoints_source_reports") }
            $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count") }
            $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports") }
            $summary.release_governance_handoff.word_visual_standard_review_metadata_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "word_visual_standard_review_metadata_source_report_count") }
            $summary.release_governance_handoff.word_visual_standard_review_metadata_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "word_visual_standard_review_metadata_source_reports") }
            $summary.release_governance_handoff.error = ""
            $summary.steps.release_governance_handoff.status = $summary.release_governance_handoff.status
            $summary.steps.release_governance_handoff.expected_report_count = $summary.release_governance_handoff.expected_report_count
            $summary.steps.release_governance_handoff.loaded_report_count = $summary.release_governance_handoff.loaded_report_count
            $summary.steps.release_governance_handoff.missing_report_count = $summary.release_governance_handoff.missing_report_count
            $summary.steps.release_governance_handoff.failed_report_count = $summary.release_governance_handoff.failed_report_count
            $summary.steps.release_governance_handoff.release_blocker_count = $summary.release_governance_handoff.release_blocker_count
            $summary.steps.release_governance_handoff.release_blockers = @($summary.release_governance_handoff.release_blockers)
            $summary.steps.release_governance_handoff.action_item_count = $summary.release_governance_handoff.action_item_count
            $summary.steps.release_governance_handoff.action_items = @($summary.release_governance_handoff.action_items)
            $summary.steps.release_governance_handoff.warning_count = $summary.release_governance_handoff.warning_count
            $summary.steps.release_governance_handoff.warnings = @($summary.release_governance_handoff.warnings)
            $summary.steps.release_governance_handoff.report_count = $summary.release_governance_handoff.report_count
            $summary.steps.release_governance_handoff.reports = @($summary.release_governance_handoff.reports)
            $summary.steps.release_governance_handoff.governance_metric_count = $summary.release_governance_handoff.governance_metric_count
            $summary.steps.release_governance_handoff.governance_metrics = @($summary.release_governance_handoff.governance_metrics)
            $summary.steps.release_governance_handoff.project_template_delivery_readiness_contract = $summary.release_governance_handoff.project_template_delivery_readiness_contract
            $summary.steps.release_governance_handoff.project_template_onboarding_governance_contract = $summary.release_governance_handoff.project_template_onboarding_governance_contract
            $summary.steps.release_governance_handoff.release_blocker_rollup = $summary.release_governance_handoff.release_blocker_rollup
            $summary.steps.release_governance_handoff.manifest_signoff_entrypoints_source_report_count = $summary.release_governance_handoff.manifest_signoff_entrypoints_source_report_count
            $summary.steps.release_governance_handoff.manifest_signoff_entrypoints_source_reports = @($summary.release_governance_handoff.manifest_signoff_entrypoints_source_reports)
            $summary.steps.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count = $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count
            $summary.steps.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports = @($summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports)
            $summary.steps.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count
            $summary.steps.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @($summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports)
            $summary.steps.release_governance_handoff.word_visual_standard_review_metadata_source_report_count = $summary.release_governance_handoff.word_visual_standard_review_metadata_source_report_count
            $summary.steps.release_governance_handoff.word_visual_standard_review_metadata_source_reports = @($summary.release_governance_handoff.word_visual_standard_review_metadata_source_reports)
            $summary.steps.release_governance_handoff.error = ""
            ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8
        } catch {
            $handoffError = $_.Exception.Message
            $handoffSummary = Read-ReleaseGovernanceHandoffSummary -Path $releaseGovernanceHandoffSummaryPath
            $summary.release_governance_handoff.status = "failed"
            $summary.release_governance_handoff.expected_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.expected_report_count }
            $summary.release_governance_handoff.loaded_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.loaded_report_count }
            $summary.release_governance_handoff.missing_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.missing_report_count }
            $summary.release_governance_handoff.failed_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.failed_report_count }
            $summary.release_governance_handoff.release_blocker_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.release_blocker_count }
            $summary.release_governance_handoff.release_blockers = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "release_blockers") }
            $summary.release_governance_handoff.action_item_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.action_item_count }
            $summary.release_governance_handoff.action_items = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "action_items") }
            $summary.release_governance_handoff.warning_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.warning_count }
            $summary.release_governance_handoff.warnings = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "warnings") }
            [object[]]$handoffReports = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "reports") }
            [object[]]$handoffGovernanceMetrics = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "governance_metrics") }
            $summary.release_governance_handoff.report_count = @($handoffReports).Count
            $summary.release_governance_handoff.reports = @($handoffReports)
            $summary.release_governance_handoff.governance_metric_count = if ($null -eq $handoffSummary) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffSummary -Name "governance_metric_count" -DefaultValue @($handoffGovernanceMetrics).Count) }
            $summary.release_governance_handoff.governance_metrics = @($handoffGovernanceMetrics)
            $summary.release_governance_handoff.project_template_delivery_readiness_contract = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "project_template_delivery_readiness_contract" }
            $summary.release_governance_handoff.project_template_onboarding_governance_contract = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "project_template_onboarding_governance_contract" }
            $summary.governance_metric_count = $summary.release_governance_handoff.governance_metric_count
            $summary.governance_metrics = @($summary.release_governance_handoff.governance_metrics)
            $handoffRollup = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "release_blocker_rollup" }
            $summary.release_governance_handoff.release_blocker_rollup = $handoffRollup
            $summary.release_governance_handoff.manifest_signoff_entrypoints_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "manifest_signoff_entrypoints_source_report_count") }
            $summary.release_governance_handoff.manifest_signoff_entrypoints_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "manifest_signoff_entrypoints_source_reports") }
            $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "project_template_readiness_checklist_entrypoints_source_report_count") }
            $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "project_template_readiness_checklist_entrypoints_source_reports") }
            $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count") }
            $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports") }
            $summary.release_governance_handoff.word_visual_standard_review_metadata_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "word_visual_standard_review_metadata_source_report_count") }
            $summary.release_governance_handoff.word_visual_standard_review_metadata_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "word_visual_standard_review_metadata_source_reports") }
            $summary.release_governance_handoff.error = $handoffError
            $summary.steps.release_governance_handoff.status = "failed"
            $summary.steps.release_governance_handoff.expected_report_count = $summary.release_governance_handoff.expected_report_count
            $summary.steps.release_governance_handoff.loaded_report_count = $summary.release_governance_handoff.loaded_report_count
            $summary.steps.release_governance_handoff.missing_report_count = $summary.release_governance_handoff.missing_report_count
            $summary.steps.release_governance_handoff.failed_report_count = $summary.release_governance_handoff.failed_report_count
            $summary.steps.release_governance_handoff.release_blocker_count = $summary.release_governance_handoff.release_blocker_count
            $summary.steps.release_governance_handoff.release_blockers = @($summary.release_governance_handoff.release_blockers)
            $summary.steps.release_governance_handoff.action_item_count = $summary.release_governance_handoff.action_item_count
            $summary.steps.release_governance_handoff.action_items = @($summary.release_governance_handoff.action_items)
            $summary.steps.release_governance_handoff.warning_count = $summary.release_governance_handoff.warning_count
            $summary.steps.release_governance_handoff.warnings = @($summary.release_governance_handoff.warnings)
            $summary.steps.release_governance_handoff.report_count = $summary.release_governance_handoff.report_count
            $summary.steps.release_governance_handoff.reports = @($summary.release_governance_handoff.reports)
            $summary.steps.release_governance_handoff.governance_metric_count = $summary.release_governance_handoff.governance_metric_count
            $summary.steps.release_governance_handoff.governance_metrics = @($summary.release_governance_handoff.governance_metrics)
            $summary.steps.release_governance_handoff.project_template_delivery_readiness_contract = $summary.release_governance_handoff.project_template_delivery_readiness_contract
            $summary.steps.release_governance_handoff.project_template_onboarding_governance_contract = $summary.release_governance_handoff.project_template_onboarding_governance_contract
            $summary.steps.release_governance_handoff.release_blocker_rollup = $summary.release_governance_handoff.release_blocker_rollup
            $summary.steps.release_governance_handoff.manifest_signoff_entrypoints_source_report_count = $summary.release_governance_handoff.manifest_signoff_entrypoints_source_report_count
            $summary.steps.release_governance_handoff.manifest_signoff_entrypoints_source_reports = @($summary.release_governance_handoff.manifest_signoff_entrypoints_source_reports)
            $summary.steps.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count = $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count
            $summary.steps.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports = @($summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports)
            $summary.steps.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count
            $summary.steps.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @($summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports)
            $summary.steps.release_governance_handoff.word_visual_standard_review_metadata_source_report_count = $summary.release_governance_handoff.word_visual_standard_review_metadata_source_report_count
            $summary.steps.release_governance_handoff.word_visual_standard_review_metadata_source_reports = @($summary.release_governance_handoff.word_visual_standard_review_metadata_source_reports)
            $summary.steps.release_governance_handoff.error = $handoffError
            ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8
            Write-Step "Release governance handoff failed: $handoffError"
            if ($ReleaseGovernanceHandoffFailOnMissing -or
                $ReleaseGovernanceHandoffFailOnBlocker -or
                $ReleaseGovernanceHandoffFailOnWarning) {
                $releaseGovernanceHandoffFailure = $handoffError
            }
        }
    }

    if ($projectTemplateWorkflowDashboardRequested) {
        try {
            Write-Step "Building project-template workflow dashboard"
            ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8
            Invoke-ProjectTemplateWorkflowDashboard `
                -ScriptPath $projectTemplateWorkflowDashboardScript `
                -ReleaseCandidateSummaryJson $summaryPath `
                -OutputDir $resolvedProjectTemplateWorkflowDashboardOutputDir `
                -SummaryJson $projectTemplateWorkflowDashboardSummaryPath `
                -ReportMarkdown $projectTemplateWorkflowDashboardMarkdownPath
            $workflowDashboardSummary = Read-ProjectTemplateWorkflowDashboardSummary -Path $projectTemplateWorkflowDashboardSummaryPath
            $workflowDashboardReleaseReadyValue = if ($null -eq $workflowDashboardSummary) { $false } else { Get-OptionalPropertyValue -Object $workflowDashboardSummary -Name "release_ready" }
            $workflowDashboardReleaseReady = if ($workflowDashboardReleaseReadyValue -is [bool]) {
                [bool]$workflowDashboardReleaseReadyValue
            } else {
                [string]$workflowDashboardReleaseReadyValue -eq "true"
            }

            $summary.project_template_workflow_dashboard_report.status = if ($null -eq $workflowDashboardSummary) { "missing_summary" } else { [string]$workflowDashboardSummary.status }
            $summary.project_template_workflow_dashboard_report.release_ready = $workflowDashboardReleaseReady
            $summary.project_template_workflow_dashboard_report.release_blocker_count = if ($null -eq $workflowDashboardSummary) { 0 } else { [int](Get-OptionalIntegerProperty -Object $workflowDashboardSummary -Name "release_blocker_count") }
            $summary.project_template_workflow_dashboard_report.warning_count = if ($null -eq $workflowDashboardSummary) { 0 } else { [int](Get-OptionalIntegerProperty -Object $workflowDashboardSummary -Name "warning_count") }
            $summary.project_template_workflow_dashboard_report.source_report_count = if ($null -eq $workflowDashboardSummary) { 0 } else { [int](Get-OptionalIntegerProperty -Object $workflowDashboardSummary -Name "source_report_count") }
            $summary.project_template_workflow_dashboard_report.next_action = if ($null -eq $workflowDashboardSummary) { $null } else { Get-OptionalPropertyValue -Object $workflowDashboardSummary -Name "next_action" }
            $summary.project_template_workflow_dashboard_report.error = ""

            $summary.steps.project_template_workflow_dashboard.status = $summary.project_template_workflow_dashboard_report.status
            $summary.steps.project_template_workflow_dashboard.release_ready = $summary.project_template_workflow_dashboard_report.release_ready
            $summary.steps.project_template_workflow_dashboard.release_blocker_count = $summary.project_template_workflow_dashboard_report.release_blocker_count
            $summary.steps.project_template_workflow_dashboard.warning_count = $summary.project_template_workflow_dashboard_report.warning_count
            $summary.steps.project_template_workflow_dashboard.source_report_count = $summary.project_template_workflow_dashboard_report.source_report_count
            $summary.steps.project_template_workflow_dashboard.next_action = $summary.project_template_workflow_dashboard_report.next_action
            $summary.steps.project_template_workflow_dashboard.error = ""
        } catch {
            $workflowDashboardError = $_.Exception.Message
            $summary.project_template_workflow_dashboard_report.status = "failed"
            $summary.project_template_workflow_dashboard_report.release_ready = $false
            $summary.project_template_workflow_dashboard_report.error = $workflowDashboardError
            $summary.steps.project_template_workflow_dashboard.status = "failed"
            $summary.steps.project_template_workflow_dashboard.release_ready = $false
            $summary.steps.project_template_workflow_dashboard.error = $workflowDashboardError
            Write-Step "Project-template workflow dashboard failed: $workflowDashboardError"
        }

        ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8
    }
