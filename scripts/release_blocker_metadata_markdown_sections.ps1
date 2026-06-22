function Add-ReleaseGovernanceMetricDetailLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Metric
    )

    $details = Get-ReleaseBlockerPropertyObject -Object $Metric -Name "details"
    if ($null -eq $details) {
        return
    }

    $detailFields = @(
        "document_count",
        "catalog_exemplar_count",
        "baseline_entry_count",
        "matched_document_count",
        "unmatched_catalog_document_count",
        "unmatched_baseline_document_count",
        "alignment_gap_count",
        "catalog_coverage_percent",
        "baseline_coverage_percent",
        "coverage_score",
        "ready_document_count",
        "ready_document_percent",
        "needs_review_document_count",
        "failed_document_count",
        "table_style_issue_count",
        "automatic_tblLook_fix_count",
        "manual_table_style_fix_count",
        "table_position_automatic_count",
        "table_position_review_count",
        "fixed_layout_table_count",
        "autofit_layout_table_count",
        "unspecified_layout_table_count",
        "pdf_floating_table_supported_geometry_count",
        "pdf_floating_table_metadata_only_count",
        "pdf_floating_table_tracked_geometry_count",
        "pdf_floating_table_supported_geometry_percent",
        "pdf_floating_table_support_coverage",
        "pdf_floating_table_reviewer_focus",
        "metadata_only_fields",
        "review_required_fields",
        "command_failure_count",
        "unresolved_item_count"
    )
    $detailParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in $detailFields) {
        $value = Get-ReleaseBlockerPropertyObject -Object $details -Name $fieldName
        if ($null -eq $value) {
            continue
        }
        if (($value -is [System.Collections.IEnumerable]) -and -not ($value -is [string])) {
            $values = @(
                $value |
                    ForEach-Object { [string]$_ } |
                    Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
            )
            if ($values.Count -gt 0) {
                [void]$detailParts.Add("$fieldName=$($values -join ',')")
            }
            continue
        }
        if (-not [string]::IsNullOrWhiteSpace([string]$value)) {
            [void]$detailParts.Add("$fieldName=$value")
        }
    }
    if ($detailParts.Count -gt 0) {
        [void]$Lines.Add("  - details: $($detailParts -join ', ')")
    }

    $pdfFloatingTableSupportedGeometryCount = Get-ReleaseBlockerIntPropertyValue -Object $details -Name "pdf_floating_table_supported_geometry_count"
    $pdfFloatingTableMetadataOnlyCount = Get-ReleaseBlockerIntPropertyValue -Object $details -Name "pdf_floating_table_metadata_only_count"
    $pdfFloatingTableTrackedGeometryCount = Get-ReleaseBlockerIntPropertyValue -Object $details -Name "pdf_floating_table_tracked_geometry_count"
    $pdfFloatingTableSupportedGeometryPercent = Get-ReleaseBlockerIntPropertyValue -Object $details -Name "pdf_floating_table_supported_geometry_percent"
    $pdfFloatingTableSupportCoverage = Get-ReleaseBlockerPropertyValue -Object $details -Name "pdf_floating_table_support_coverage"
    $pdfFloatingTableReviewerFocus = Get-ReleaseBlockerPropertyValue -Object $details -Name "pdf_floating_table_reviewer_focus"
    if ($pdfFloatingTableTrackedGeometryCount -gt 0 -or -not [string]::IsNullOrWhiteSpace($pdfFloatingTableSupportCoverage)) {
        if ([string]::IsNullOrWhiteSpace($pdfFloatingTableSupportCoverage)) {
            $pdfFloatingTableSupportCoverage = "$pdfFloatingTableSupportedGeometryCount/$pdfFloatingTableTrackedGeometryCount supported ($pdfFloatingTableSupportedGeometryPercent%); metadata_only=$pdfFloatingTableMetadataOnlyCount"
        }
        [void]$Lines.Add("  - pdf_floating_table_support_coverage: $pdfFloatingTableSupportCoverage")
        if (-not [string]::IsNullOrWhiteSpace($pdfFloatingTableReviewerFocus)) {
            [void]$Lines.Add("  - pdf_floating_table_reviewer_focus: $pdfFloatingTableReviewerFocus")
        } elseif ($pdfFloatingTableSupportedGeometryPercent -lt 100) {
            [void]$Lines.Add("  - pdf_floating_table_reviewer_focus: review metadata-only tblpPr fields before approving PDF-layout-sensitive release.")
        }
        $pdfFloatingTableMetadataOnlyFields = @(
            Get-ReleaseBlockerArrayProperty -Object $details -Name "pdf_floating_table_metadata_only_fields" |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        if ($pdfFloatingTableMetadataOnlyFields.Count -gt 0) {
            [void]$Lines.Add("  - pdf_floating_table_metadata_only_fields: $($pdfFloatingTableMetadataOnlyFields -join ', ')")
        }
        $pdfFloatingTableReviewRequiredFields = @(
            Get-ReleaseBlockerArrayProperty -Object $details -Name "pdf_floating_table_review_required_fields" |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        if ($pdfFloatingTableReviewRequiredFields.Count -gt 0) {
            [void]$Lines.Add("  - pdf_floating_table_review_required_fields: $($pdfFloatingTableReviewRequiredFields -join ', ')")
        }
        $metadataOnlyFields = @(
            Get-ReleaseBlockerArrayProperty -Object $details -Name "metadata_only_fields" |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        if ($metadataOnlyFields.Count -gt 0) {
            [void]$Lines.Add("  - metadata_only_fields: $($metadataOnlyFields -join ', ')")
        }
        $reviewRequiredFields = @(
            Get-ReleaseBlockerArrayProperty -Object $details -Name "review_required_fields" |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        if ($reviewRequiredFields.Count -gt 0) {
            [void]$Lines.Add("  - review_required_fields: $($reviewRequiredFields -join ', ')")
        }
    }

    foreach ($fieldName in @("catalog_document_keys", "baseline_document_keys", "matched_document_keys")) {
        $values = @(
            Get-ReleaseBlockerArrayProperty -Object $details -Name $fieldName |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        if ($values.Count -gt 0) {
            [void]$Lines.Add("  - ${fieldName}: $($values -join ',')")
        }
    }

    $penaltyParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($penalty in @(Get-ReleaseBlockerArrayProperty -Object $details -Name "penalty_summary")) {
        $factor = Get-ReleaseBlockerPropertyValue -Object $penalty -Name "factor"
        if ([string]::IsNullOrWhiteSpace($factor)) {
            continue
        }

        $count = Get-ReleaseBlockerPropertyObject -Object $penalty -Name "count"
        $penaltyValue = Get-ReleaseBlockerPropertyObject -Object $penalty -Name "penalty"
        [void]$penaltyParts.Add("$factor(count=$count, penalty=$penaltyValue)")
    }
    if ($penaltyParts.Count -gt 0) {
        [void]$Lines.Add("  - penalty_summary: $($penaltyParts -join '; ')")
    }
}

function Add-ReleaseGovernanceRollupMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$RepoRoot,
        [string]$Heading = "## Release Governance Rollup Details"
    )

    $rollup = Get-ReleaseGovernanceRollup -Summary $Summary
    if ($null -eq $rollup) {
        return
    }

    $requested = Get-ReleaseBlockerPropertyValue -Object $rollup -Name "requested"
    $status = Get-ReleaseBlockerPropertyValue -Object $rollup -Name "status"
    $releaseBlockers = @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "release_blockers")
    $warnings = @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "warnings")
    $actionItems = @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "action_items")
    if ($requested -eq "False" -and
        $status -eq "not_requested" -and
        $releaseBlockers.Count -eq 0 -and
        $warnings.Count -eq 0 -and
        $actionItems.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    [void]$Lines.Add("- Status: $(Get-ReleaseBlockerDisplayValue -Value $status)")
    [void]$Lines.Add("- Source reports: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $rollup -Name "source_report_count") -Fallback "0")")
    [void]$Lines.Add("- Source failures: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $rollup -Name "source_failure_count") -Fallback "0")")
    [void]$Lines.Add("- source_failure_count: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $rollup -Name "source_failure_count") -Fallback "0")")
    [void]$Lines.Add("- Blockers: $($releaseBlockers.Count)")
    [void]$Lines.Add("- Warnings: $($warnings.Count)")
    [void]$Lines.Add("- Action items: $($actionItems.Count)")
    [void]$Lines.Add("- Blocker source schemas: $(Get-ReleaseBlockerSummaryGroupDisplay -Items @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "blocker_source_schema_summary"))")
    [void]$Lines.Add("- Action item source schemas: $(Get-ReleaseBlockerSummaryGroupDisplay -Items @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "action_item_source_schema_summary"))")
    [void]$Lines.Add("- Informational action item source schemas: $(Get-ReleaseBlockerSummaryGroupDisplay -Items @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "informational_action_item_source_schema_summary"))")
    [void]$Lines.Add("- Warning source schemas: $(Get-ReleaseBlockerSummaryGroupDisplay -Items @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "warning_source_schema_summary"))")

    Add-ReleaseGovernanceMetricsMarkdownSection -Lines $Lines -Summary $rollup

    Add-ReleaseGovernanceSourceReportContractLines `
        -Lines $Lines `
        -Reports @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "source_reports") `
        -Heading "### Rollup Source Report Contracts" `
        -IdPropertyName "schema" `
        -PathPropertyName "path" `
        -DisplayPropertyName "path_display" `
        -RepoRoot $RepoRoot

    Add-ReleaseGovernanceReportIssueLines `
        -Lines $Lines `
        -Reports @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "source_reports") `
        -Heading "### Rollup Source Report Issues" `
        -IdPropertyName "schema" `
        -PathPropertyName "path" `
        -DisplayPropertyName "path_display" `
        -RepoRoot $RepoRoot

    if ($releaseBlockers.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Rollup Blockers")
        [void]$Lines.Add("")
        foreach ($blocker in $releaseBlockers) {
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id") -Fallback "(unknown blocker)"): action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $blocker -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $blocker
            Add-ReleaseGovernanceReadinessActionEvidenceLines -Lines $Lines -Item $blocker
        }
    }

    if ($warnings.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Rollup Warnings")
        [void]$Lines.Add("")
        foreach ($warning in $warnings) {
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "id") -Fallback "(unknown warning)"): action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $warning -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $warning -RepoRoot $RepoRoot
        }
    }

    if ($actionItems.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Rollup Action Items")
        [void]$Lines.Add("")
        foreach ($item in $actionItems) {
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "id") -Fallback "(unknown action item)"): action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema"))")
            $openCommand = Get-ReleaseBlockerPropertyValue -Object $item -Name "open_command"
            if (-not [string]::IsNullOrWhiteSpace($openCommand)) {
                [void]$Lines.Add("  - open_command: $openCommand")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $item -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $item
            Add-ReleaseGovernanceReadinessActionEvidenceLines -Lines $Lines -Item $item
        }
    }
}

function Add-ReleaseGovernanceHandoffMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$RepoRoot,
        [string]$Heading = "## Release Governance Handoff Details"
    )

    $handoff = Get-ReleaseGovernanceHandoff -Summary $Summary
    if ($null -eq $handoff) {
        return
    }

    $requested = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "requested"
    $status = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "status"
    $releaseBlockers = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "release_blockers")
    $warnings = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "warnings")
    $actionItems = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "action_items")
    if ($requested -eq "False" -and
        $status -eq "not_requested" -and
        $releaseBlockers.Count -eq 0 -and
        $warnings.Count -eq 0 -and
        $actionItems.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    [void]$Lines.Add("- Status: $(Get-ReleaseBlockerDisplayValue -Value $status)")
    [void]$Lines.Add("- Reports loaded: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "loaded_report_count") -Fallback "0") / $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "expected_report_count") -Fallback "0")")
    [void]$Lines.Add("- Missing reports: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "missing_report_count") -Fallback "0")")
    [void]$Lines.Add("- Failed reports: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "failed_report_count") -Fallback "0")")
    [void]$Lines.Add("- Blockers: $($releaseBlockers.Count)")
    [void]$Lines.Add("- Warnings: $($warnings.Count)")
    [void]$Lines.Add("- Action items: $($actionItems.Count)")
    $handoffReports = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "reports")
    Add-ReleaseGovernanceSourceReportContractLines `
        -Lines $Lines `
        -Reports $handoffReports `
        -Heading "### Handoff Source Report Contracts" `
        -IdPropertyName "id" `
        -PathPropertyName "expected_summary" `
        -DisplayPropertyName "expected_summary_display" `
        -RepoRoot $RepoRoot

    Add-ProjectTemplateGovernanceCombinedContractLines `
        -Lines $Lines `
        -ReadinessReport (Get-ReleaseBlockerPropertyObject -Object $handoff -Name "project_template_delivery_readiness_contract") `
        -OnboardingReport (Get-ReleaseBlockerPropertyObject -Object $handoff -Name "project_template_onboarding_governance_contract")

    $manifestSignoffReports = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "manifest_signoff_entrypoints_source_reports")
    $manifestSignoffCount = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "manifest_signoff_entrypoints_source_report_count"
    if ([string]::IsNullOrWhiteSpace($manifestSignoffCount)) {
        $manifestSignoffCount = [string]$manifestSignoffReports.Count
    }
    if ($manifestSignoffReports.Count -gt 0 -or $manifestSignoffCount -ne "0") {
        [void]$Lines.Add("- Manifest signoff entrypoints evidence source reports: $manifestSignoffCount")
        foreach ($report in $manifestSignoffReports) {
            $sourceReportDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "path_display")
            $schema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")
            [void]$Lines.Add("  - source_report: $sourceReportDisplay schema=$schema")
            Add-ReleaseGovernanceManifestSignoffSourceReportLines `
                -Lines $Lines `
                -Report $report `
                -Indent "    "
        }
    }
    $projectTemplateChecklistReports = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "project_template_readiness_checklist_entrypoints_source_reports")
    $projectTemplateChecklistCount = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "project_template_readiness_checklist_entrypoints_source_report_count"
    if ([string]::IsNullOrWhiteSpace($projectTemplateChecklistCount)) {
        $projectTemplateChecklistCount = [string]$projectTemplateChecklistReports.Count
    }
    if ($projectTemplateChecklistReports.Count -gt 0 -or $projectTemplateChecklistCount -ne "0") {
        [void]$Lines.Add("- Project-template readiness checklist entrypoints evidence source reports: $projectTemplateChecklistCount")
        foreach ($report in $projectTemplateChecklistReports) {
            $sourceReportDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "path_display")
            $schema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")
            [void]$Lines.Add("  - source_report: $sourceReportDisplay schema=$schema")
            Add-ReleaseGovernanceProjectTemplateReadinessChecklistSourceReportLines `
                -Lines $Lines `
                -Report $report `
                -Indent "    "
        }
    }
    $releaseEntryChecklistAuditReports = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports")
    $releaseEntryChecklistAuditCount = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count"
    if ([string]::IsNullOrWhiteSpace($releaseEntryChecklistAuditCount)) {
        $releaseEntryChecklistAuditCount = [string]$releaseEntryChecklistAuditReports.Count
    }
    if ($releaseEntryChecklistAuditReports.Count -gt 0 -or $releaseEntryChecklistAuditCount -ne "0") {
        [void]$Lines.Add("- Release-entry project-template readiness checklist material-safety audit source reports: $releaseEntryChecklistAuditCount")
        foreach ($report in $releaseEntryChecklistAuditReports) {
            $sourceReportDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "path_display")
            $schema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")
            [void]$Lines.Add("  - source_report: $sourceReportDisplay schema=$schema")
            Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditSourceReportLines `
                -Lines $Lines `
                -Report $report `
                -Indent "    "
        }
    }
    $wordVisualMetadataReports = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "word_visual_standard_review_metadata_source_reports")
    $wordVisualMetadataCount = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "word_visual_standard_review_metadata_source_report_count"
    if ([string]::IsNullOrWhiteSpace($wordVisualMetadataCount)) {
        $wordVisualMetadataCount = [string]$wordVisualMetadataReports.Count
    }
    if ($wordVisualMetadataReports.Count -gt 0 -or $wordVisualMetadataCount -ne "0") {
        [void]$Lines.Add("- Word visual standard review metadata source reports: $wordVisualMetadataCount")
        foreach ($report in $wordVisualMetadataReports) {
            $sourceReportDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "path_display")
            $schema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")
            [void]$Lines.Add("  - source_report: $sourceReportDisplay schema=$schema")
            Add-ReleaseGovernanceWordVisualStandardReviewMetadataSourceReportLines `
                -Lines $Lines `
                -Report $report `
                -Indent "    "
        }
    }

    Add-ReleaseGovernanceMetricsMarkdownSection -Lines $Lines -Summary $handoff

    Add-ReleaseGovernanceReportIssueLines `
        -Lines $Lines `
        -Reports $handoffReports `
        -Heading "### Handoff Report Issues" `
        -IdPropertyName "id" `
        -PathPropertyName "expected_summary" `
        -DisplayPropertyName "expected_summary_display" `
        -RepoRoot $RepoRoot

    if ($releaseBlockers.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Handoff Blockers")
        [void]$Lines.Add("")
        foreach ($blocker in $releaseBlockers) {
            $reportId = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "report_id"
            $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id") -Fallback "(unknown blocker)"
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value $reportId -Fallback "(unknown report)") / ${id}: action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $blocker -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $blocker
            Add-ReleaseGovernanceReadinessActionEvidenceLines -Lines $Lines -Item $blocker
        }
    }

    if ($warnings.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Handoff Warnings")
        [void]$Lines.Add("")
        foreach ($warning in $warnings) {
            $reportId = Get-ReleaseBlockerPropertyValue -Object $warning -Name "report_id"
            $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "id") -Fallback "(unknown warning)"
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value $reportId -Fallback "(unknown report)") / ${id}: action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $warning -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $warning -RepoRoot $RepoRoot
        }
    }

    if ($actionItems.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Handoff Action Items")
        [void]$Lines.Add("")
        foreach ($item in $actionItems) {
            $reportId = Get-ReleaseBlockerPropertyValue -Object $item -Name "report_id"
            $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "id") -Fallback "(unknown action item)"
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value $reportId -Fallback "(unknown report)") / ${id}: action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema"))")
            $openCommand = Get-ReleaseBlockerPropertyValue -Object $item -Name "open_command"
            if (-not [string]::IsNullOrWhiteSpace($openCommand)) {
                [void]$Lines.Add("  - open_command: $openCommand")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $item -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $item
            Add-ReleaseGovernanceReadinessActionEvidenceLines -Lines $Lines -Item $item
        }
    }
}
