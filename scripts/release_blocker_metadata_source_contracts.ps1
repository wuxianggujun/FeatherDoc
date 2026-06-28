function Get-ReleaseGovernanceRollup {
    param([AllowNull()]$Summary)

    return Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup"
}

function Get-ReleaseGovernanceHandoff {
    param([AllowNull()]$Summary)

    return Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff"
}

function Add-ReleaseGovernanceReviewerActionLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item
    )

    $requiresReviewerAction = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $Item -Name "requires_reviewer_action"
    if (-not [string]::IsNullOrWhiteSpace($requiresReviewerAction)) {
        [void]$Lines.Add("  - requires_reviewer_action: $requiresReviewerAction")
    }

    $reviewerActionSummary = Get-ReleaseBlockerPropertyValue -Object $Item -Name "reviewer_action_summary"
    if (-not [string]::IsNullOrWhiteSpace($reviewerActionSummary)) {
        [void]$Lines.Add("  - reviewer_action: $reviewerActionSummary")
    }

    $reviewerActionReason = Get-ReleaseBlockerPropertyValue -Object $Item -Name "reviewer_action_reason"
    if (-not [string]::IsNullOrWhiteSpace($reviewerActionReason)) {
        [void]$Lines.Add("  - reviewer_action_reason: $reviewerActionReason")
    }

    $reviewerActions = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "reviewer_actions" |
        ForEach-Object { [string]$_ } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($reviewerActions.Count -gt 0) {
        [void]$Lines.Add("  - reviewer_actions: $($reviewerActions -join ', ')")
    }
}

function Add-ReleaseGovernanceSchemaCorpusMetadataLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item
    )

    foreach ($fieldName in @(
            "business_document_type",
            "source_business_document_type",
            "corpus_role",
            "source_corpus_role",
            "business_document_type_mismatch",
            "corpus_role_mismatch",
            "mismatched_corpus_metadata_count",
            "mismatched_business_document_type_count",
            "mismatched_corpus_role_count",
            "candidate_name",
            "schema_update_candidate"
        )) {
        if (-not (Test-ReleaseBlockerPropertyExists -Object $Item -Name $fieldName)) {
            continue
        }

        $fieldValues = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name $fieldName |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        if ($fieldValues.Count -gt 0) {
            [void]$Lines.Add("  - ${fieldName}: $($fieldValues -join ', ')")
        }
    }
}

function Add-ReleaseGovernanceRollupSourceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$RepoRoot
    )

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerDisplayPath `
            -RepoRoot $RepoRoot `
            -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report")
    }

    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath `
            -RepoRoot $RepoRoot `
            -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json")
    }

    $projectId = Get-ReleaseBlockerPropertyValue -Object $Item -Name "project_id"
    $templateName = Get-ReleaseBlockerPropertyValue -Object $Item -Name "template_name"
    if (-not [string]::IsNullOrWhiteSpace($projectId) -or -not [string]::IsNullOrWhiteSpace($templateName)) {
        [void]$Lines.Add("  - project_template: project_id=$projectId template_name=$templateName")
    }

    $candidateType = Get-ReleaseBlockerPropertyValue -Object $Item -Name "candidate_type"
    if (-not [string]::IsNullOrWhiteSpace($candidateType)) {
        [void]$Lines.Add("  - candidate_type: $candidateType")
    }
    Add-ReleaseGovernanceSchemaCorpusMetadataLines -Lines $Lines -Item $Item

    foreach ($fieldName in @("source_report", "source_json")) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            $displayValue = if ($fieldName -eq "source_report") {
                $sourceReportDisplay
            } else {
                $sourceJsonDisplay
            }
            if ([string]::IsNullOrWhiteSpace($displayValue)) {
                $displayValue = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $fieldValue
            }
            [void]$Lines.Add("  - ${fieldName}: $displayValue")
        }
    }

    [void]$Lines.Add("  - source_report_display: $sourceReportDisplay")
    [void]$Lines.Add("  - source_json_display: $sourceJsonDisplay")
    $readinessStatus = Get-ReleaseBlockerPropertyValue -Object $Item -Name "readiness_status"
    if (-not [string]::IsNullOrWhiteSpace($readinessStatus)) {
        [void]$Lines.Add("  - readiness_status: $readinessStatus")
    }
    $readinessReleaseReady = Get-ReleaseBlockerPropertyValue -Object $Item -Name "readiness_release_ready"
    if (-not [string]::IsNullOrWhiteSpace($readinessReleaseReady)) {
        [void]$Lines.Add("  - readiness_release_ready: $readinessReleaseReady")
    }
    Add-ReleaseGovernanceReviewerActionLines -Lines $Lines -Item $Item
    Add-ProjectTemplateOnboardingGovernanceContractLines `
        -Lines $Lines `
        -Item $Item `
        -SourceReportDisplay $sourceReportDisplay `
        -SourceJsonDisplay $sourceJsonDisplay
    Add-ReleaseGovernanceProvenanceLines -Lines $Lines -Item $Item
}

function Add-ProjectTemplateOnboardingGovernanceContractLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$SourceReportDisplay,
        [string]$SourceJsonDisplay
    )

    $sourceSchema = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_schema"
    if (-not [string]::Equals($sourceSchema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
        return
    }

    $schemaApprovalSummaryValue = Get-ReleaseBlockerPropertyObject -Object $Item -Name "onboarding_governance_schema_approval_status_summary"
    if ($null -eq $schemaApprovalSummaryValue) {
        $schemaApprovalSummaryValue = Get-ReleaseBlockerPropertyObject -Object $Item -Name "schema_approval_status_summary"
    }
    $schemaApprovalSummary = Format-ProjectTemplateSchemaApprovalStatusSummary `
        -Value $schemaApprovalSummaryValue `
        -Fallback (Get-ReleaseBlockerPropertyValue -Object $Item -Name "onboarding_governance_status")
    if ([string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
        $schemaApprovalSummary = "unknown"
    }
    $status = Get-ReleaseBlockerPropertyValue -Object $Item -Name "onboarding_governance_status"
    $releaseReady = Get-ReleaseBlockerPropertyValue -Object $Item -Name "onboarding_governance_release_ready"
    $nextActionValue = Get-ReleaseBlockerPropertyObject -Object $Item -Name "onboarding_governance_next_action"
    if ($null -eq $nextActionValue) {
        $nextActionValue = Get-ReleaseBlockerPropertyObject -Object $Item -Name "next_action"
    }
    $nextAction = Format-ProjectTemplateNextActionSummary -Value $nextActionValue
    $nextActionSummaryValue = Get-ReleaseBlockerPropertyObject -Object $Item -Name "onboarding_governance_next_action_summary"
    if ($null -eq $nextActionSummaryValue) {
        $nextActionSummaryValue = Get-ReleaseBlockerPropertyObject -Object $Item -Name "next_action_summary"
    }
    $nextActionSummary = Format-ProjectTemplateNextActionSummary -Value $nextActionSummaryValue
    $nextActionGroupCount = Get-ReleaseBlockerPropertyValue -Object $Item -Name "onboarding_governance_next_action_group_count"
    if ([string]::IsNullOrWhiteSpace($nextActionGroupCount)) {
        $nextActionGroupCount = Get-ReleaseBlockerPropertyValue -Object $Item -Name "next_action_group_count"
    }
    $onboardingSourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "onboarding_governance_source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($onboardingSourceReportDisplay)) {
        $SourceReportDisplay = $onboardingSourceReportDisplay
    }
    $onboardingSourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "onboarding_governance_source_json_display"
    if (-not [string]::IsNullOrWhiteSpace($onboardingSourceJsonDisplay)) {
        $SourceJsonDisplay = $onboardingSourceJsonDisplay
    }

    [void]$Lines.Add("  - project_template_onboarding_governance_contract:")
    [void]$Lines.Add("    - source_schema: featherdoc.project_template_onboarding_governance_report.v1")
    if (-not [string]::IsNullOrWhiteSpace($status)) {
        [void]$Lines.Add("    - status: $status")
    }
    if (-not [string]::IsNullOrWhiteSpace($releaseReady)) {
        [void]$Lines.Add("    - release_ready: $releaseReady")
    }
    [void]$Lines.Add("    - schema_approval_status_summary: $schemaApprovalSummary")
    if (-not [string]::IsNullOrWhiteSpace($nextAction)) {
        [void]$Lines.Add("    - next_action: $nextAction")
    }
    if (-not [string]::IsNullOrWhiteSpace($nextActionSummary)) {
        [void]$Lines.Add("    - next_action_summary: $nextActionSummary")
    }
    if (-not [string]::IsNullOrWhiteSpace($nextActionGroupCount)) {
        [void]$Lines.Add("    - next_action_group_count: $nextActionGroupCount")
    }
    if (-not [string]::IsNullOrWhiteSpace($SourceReportDisplay)) {
        [void]$Lines.Add("    - source_report_display: $SourceReportDisplay")
    }
    if (-not [string]::IsNullOrWhiteSpace($SourceJsonDisplay)) {
        [void]$Lines.Add("    - source_json_display: $SourceJsonDisplay")
    }
}

function Test-ProjectTemplateOnboardingGovernanceReport {
    param([AllowNull()]$Report)

    $schema = Get-ReleaseBlockerPropertyValue -Object $Report -Name "schema"
    $sourceSchema = Get-ReleaseBlockerPropertyValue -Object $Report -Name "source_schema"

    return (
        [string]::Equals($schema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase) -or
        [string]::Equals($sourceSchema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase)
    )
}

function Add-ProjectTemplateBusinessDimensionContractLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Report,
        [string]$Indent
    )

    $businessDocumentTypeSummary = Get-ReleaseBlockerSummaryGroupDisplay `
        -Items @(Get-ReleaseBlockerArrayProperty -Object $Report -Name "business_document_type_summary") `
        -NameProperty "document_type"
    if ($businessDocumentTypeSummary -ne "(none)") {
        [void]$Lines.Add("${Indent}- business_document_type_summary: $businessDocumentTypeSummary")
    }

    $corpusRoleSummary = Get-ReleaseBlockerSummaryGroupDisplay `
        -Items @(Get-ReleaseBlockerArrayProperty -Object $Report -Name "corpus_role_summary") `
        -NameProperty "corpus_role"
    if ($corpusRoleSummary -ne "(none)") {
        [void]$Lines.Add("${Indent}- corpus_role_summary: $corpusRoleSummary")
    }
}

function Add-ProjectTemplateOnboardingGovernanceReportContractLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Report,
        [string]$SourceReportDisplay,
        [string]$SourceJsonDisplay
    )

    if (-not (Test-ProjectTemplateOnboardingGovernanceReport -Report $Report)) {
        return
    }

    $schemaApprovalSummary = Format-ProjectTemplateSchemaApprovalStatusSummary `
        -Value (Get-ReleaseBlockerPropertyObject -Object $Report -Name "schema_approval_status_summary") `
        -Fallback (Get-ReleaseBlockerPropertyValue -Object $Report -Name "status")
    if ([string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
        $schemaApprovalSummary = "unknown"
    }

    $status = Get-ReleaseBlockerPropertyValue -Object $Report -Name "status"
    $releaseReady = Get-ReleaseBlockerPropertyValue -Object $Report -Name "release_ready"
    $nextAction = Format-ProjectTemplateNextActionSummary `
        -Value (Get-ReleaseBlockerPropertyObject -Object $Report -Name "next_action")
    $nextActionSummary = Format-ProjectTemplateNextActionSummary `
        -Value (Get-ReleaseBlockerPropertyObject -Object $Report -Name "next_action_summary")
    $nextActionGroupCount = Get-ReleaseBlockerPropertyValue -Object $Report -Name "next_action_group_count"

    [void]$Lines.Add("  - project_template_onboarding.schema_approval:")
    [void]$Lines.Add("    - project_template_onboarding_governance_contract:")
    [void]$Lines.Add("      - source_schema: featherdoc.project_template_onboarding_governance_report.v1")
    if (-not [string]::IsNullOrWhiteSpace($status)) {
        [void]$Lines.Add("      - status: $status")
    }
    if (-not [string]::IsNullOrWhiteSpace($releaseReady)) {
        [void]$Lines.Add("      - release_ready: $releaseReady")
    }
    [void]$Lines.Add("      - schema_approval_status_summary: $schemaApprovalSummary")
    [void]$Lines.Add("      - schema_approval_status_summary_marker: schema_approval_status_summary=$schemaApprovalSummary")
    if (-not [string]::IsNullOrWhiteSpace($nextAction)) {
        [void]$Lines.Add("      - next_action: $nextAction")
    }
    if (-not [string]::IsNullOrWhiteSpace($nextActionSummary)) {
        [void]$Lines.Add("      - next_action_summary: $nextActionSummary")
    }
    if (-not [string]::IsNullOrWhiteSpace($nextActionGroupCount)) {
        [void]$Lines.Add("      - next_action_group_count: $nextActionGroupCount")
    }
    Add-ProjectTemplateBusinessDimensionContractLines `
        -Lines $Lines `
        -Report $Report `
        -Indent "      "
    if (-not [string]::IsNullOrWhiteSpace($SourceReportDisplay)) {
        [void]$Lines.Add("      - source_report_display: $SourceReportDisplay")
    }
    if (-not [string]::IsNullOrWhiteSpace($SourceJsonDisplay)) {
        [void]$Lines.Add("      - source_json_display: $SourceJsonDisplay")
    }

    foreach ($fieldName in @(
            "entry_count",
            "blocked_entry_count",
            "pending_review_entry_count",
            "not_evaluated_entry_count",
            "approved_entry_count",
            "not_required_entry_count",
            "release_blocker_count",
            "action_item_count",
            "warning_count",
            "source_failure_count"
        )) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Report -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            [void]$Lines.Add("      - ${fieldName}: $fieldValue")
        }
    }
}

function Add-ReleaseGovernanceProvenanceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item
    )

    foreach ($fieldName in @("input_docx_display", "input_docx", "schema_target", "target_mode")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$Lines.Add("  - ${fieldName}: $value")
        }
    }
}

function Add-ReleaseGovernanceRepairLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item
    )

    $repairActionClasses = @(
        Get-ReleaseBlockerArrayProperty -Object $Item -Name "repair_action_classes" |
            Where-Object { $null -ne $_ -and -not [string]::IsNullOrWhiteSpace([string]$_) } |
            ForEach-Object { [string]$_ }
    )
    if ($repairActionClasses.Count -gt 0) {
        [void]$Lines.Add("  - repair_action_classes: $($repairActionClasses -join ', ')")
    }

    foreach ($fieldName in @("repair_strategy", "repair_hint", "command_template")) {
        $value = Convert-ReleaseBlockerLocalPathsForPublicText `
            -Text (Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName)
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$Lines.Add("  - ${fieldName}: $value")
        }
    }
}

function Add-ReleaseGovernanceReadinessActionEvidenceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item
    )

    $evidenceItems = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "readiness_action_evidence")
    $evidenceCount = Get-ReleaseBlockerPropertyValue -Object $Item -Name "readiness_action_evidence_count"
    if ([string]::IsNullOrWhiteSpace($evidenceCount)) {
        if ($evidenceItems.Count -eq 0) {
            return
        }

        $evidenceCount = [string]$evidenceItems.Count
    }

    [void]$Lines.Add("  - readiness_action_evidence_count: $evidenceCount")
    if ($evidenceItems.Count -eq 0) {
        return
    }

    [void]$Lines.Add("  - readiness_action_evidence:")
    foreach ($evidence in $evidenceItems) {
        $id = Get-ReleaseBlockerDisplayValue `
            -Value (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "id") `
            -Fallback "(unknown evidence)"
        $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "action")
        $issueKey = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "issue_key")
        $item = Get-ReleaseBlockerDisplayValue `
            -Value (Convert-ReleaseBlockerLocalPathsForPublicText -Text (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "item"))
        [void]$Lines.Add("    - ${id}: action=$action issue_key=$issueKey item=$item")

        foreach ($fieldName in @("source_schema", "source_report", "source_report_display", "source_json", "source_json_display")) {
            $fieldValue = Convert-ReleaseBlockerLocalPathsForPublicText `
                -Text (Get-ReleaseBlockerPropertyValue -Object $evidence -Name $fieldName)
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                [void]$Lines.Add("      - ${fieldName}: $fieldValue")
            }
        }
    }
}

function Add-ReleaseGovernanceReportIssueLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object[]]$Reports,
        [string]$Heading,
        [string]$IdPropertyName,
        [string]$PathPropertyName,
        [string]$DisplayPropertyName,
        [string]$RepoRoot
    )

    $issueReports = @(
        foreach ($report in @($Reports)) {
            $status = Get-ReleaseBlockerPropertyValue -Object $report -Name "status"
            $errorText = Get-ReleaseBlockerPropertyValue -Object $report -Name "error"
            $sourceFailureCount = Get-ReleaseBlockerPropertyValue -Object $report -Name "source_failure_count"
            if ($status -in @("failed", "missing") -or
                -not [string]::IsNullOrWhiteSpace($errorText) -or
                ($sourceFailureCount -match '^[0-9]+$' -and [int]$sourceFailureCount -gt 0)) {
                $report
            }
        }
    )
    if ($issueReports.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    foreach ($report in $issueReports) {
        $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name $IdPropertyName) -Fallback "(unknown report)"
        $status = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "status")
        $releaseReady = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_ready")
        $sourceFailureCount = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "source_failure_count") -Fallback "0"
        $schema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")
        $sourceReport = Get-ReleaseBlockerPropertyValue -Object $report -Name $PathPropertyName
        $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $report -Name $DisplayPropertyName
        if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
            $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $sourceReport
        }

        [void]$Lines.Add("- ${id}: status=$status ready=$releaseReady source_failures=$sourceFailureCount schema=$schema")
        if (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
            [void]$Lines.Add("  - source_report: $sourceReport")
        }
        [void]$Lines.Add("  - source_report_display: $(Get-ReleaseBlockerDisplayValue -Value $sourceReportDisplay)")

        $sourceJson = Get-ReleaseBlockerPropertyValue -Object $report -Name "source_json"
        if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
            [void]$Lines.Add("  - source_json: $sourceJson")
        }

        $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $report -Name "source_json_display"
        if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay) -and -not [string]::IsNullOrWhiteSpace($sourceJson)) {
            $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $sourceJson
        }
        if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            [void]$Lines.Add("  - source_json_display: $(Get-ReleaseBlockerDisplayValue -Value $sourceJsonDisplay)")
        }
        [void]$Lines.Add("  - source_failure_count: $sourceFailureCount")
        Add-ProjectTemplateDeliveryReadinessContractLines `
            -Lines $Lines `
            -Report $report `
            -SourceReportDisplay $sourceReportDisplay `
            -SourceJsonDisplay $sourceJsonDisplay
        Add-ProjectTemplateOnboardingGovernanceReportContractLines `
            -Lines $Lines `
            -Report $report `
            -SourceReportDisplay $sourceReportDisplay `
            -SourceJsonDisplay $sourceJsonDisplay

        $errorText = Get-ReleaseBlockerPropertyValue -Object $report -Name "error"
        if (-not [string]::IsNullOrWhiteSpace($errorText)) {
            [void]$Lines.Add("  - error: $errorText")
        }

        $buildCommand = Get-ReleaseBlockerPropertyValue -Object $report -Name "build_command"
        if (-not [string]::IsNullOrWhiteSpace($buildCommand)) {
            [void]$Lines.Add("  - build: $buildCommand")
        }
    }
}

function Test-ProjectTemplateDeliveryReadinessReport {
    param([AllowNull()]$Report)

    $id = Get-ReleaseBlockerPropertyValue -Object $Report -Name "id"
    $schema = Get-ReleaseBlockerPropertyValue -Object $Report -Name "schema"

    return (
        [string]::Equals($id, "project_template_delivery_readiness", [System.StringComparison]::OrdinalIgnoreCase) -or
        [string]::Equals($schema, "featherdoc.project_template_delivery_readiness_report.v1", [System.StringComparison]::OrdinalIgnoreCase)
    )
}

function Add-ProjectTemplateDeliveryReadinessContractLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Report,
        [string]$SourceReportDisplay,
        [string]$SourceJsonDisplay
    )

    if (-not (Test-ProjectTemplateDeliveryReadinessReport -Report $Report)) {
        return
    }

    $sourceSchema = Get-ReleaseBlockerPropertyValue -Object $Report -Name "schema"
    if ([string]::IsNullOrWhiteSpace($sourceSchema)) {
        $sourceSchema = "featherdoc.project_template_delivery_readiness_report.v1"
    }

    [void]$Lines.Add("  - project_template_delivery_readiness_contract:")
    [void]$Lines.Add("    - source_schema: $sourceSchema")

    foreach ($fieldName in @(
            "status",
            "release_ready",
            "latest_schema_approval_gate_status",
            "schema_history_blocked_run_count",
            "schema_history_pending_run_count",
            "schema_history_passed_run_count",
            "template_count",
            "ready_template_count",
            "blocked_template_count",
            "release_blocker_count",
            "action_item_count",
            "warning_count",
            "source_failure_count"
        )) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Report -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            [void]$Lines.Add("    - ${fieldName}: $fieldValue")
        }

        if ($fieldName -eq "latest_schema_approval_gate_status") {
            $schemaApprovalSummary = Format-ProjectTemplateSchemaApprovalStatusSummary `
                -Value (Get-ReleaseBlockerPropertyObject -Object $Report -Name "schema_approval_status_summary")
            if (-not [string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
                [void]$Lines.Add("    - schema_approval_status_summary: $schemaApprovalSummary")
                [void]$Lines.Add("    - schema_approval_status_summary_marker: schema_approval_status_summary=$schemaApprovalSummary")
            }
        }
    }

    $onboardingNextAction = Format-ProjectTemplateNextActionSummary `
        -Value (Get-ReleaseBlockerPropertyObject -Object $Report -Name "onboarding_governance_next_action")
    $onboardingNextActionSummary = Format-ProjectTemplateNextActionSummary `
        -Value (Get-ReleaseBlockerPropertyObject -Object $Report -Name "onboarding_governance_next_action_summary")
    $onboardingNextActionGroupCount = Get-ReleaseBlockerPropertyValue `
        -Object $Report `
        -Name "onboarding_governance_next_action_group_count"
    if (-not [string]::IsNullOrWhiteSpace($onboardingNextAction)) {
        [void]$Lines.Add("    - onboarding_governance_next_action: $onboardingNextAction")
    }
    if (-not [string]::IsNullOrWhiteSpace($onboardingNextActionSummary)) {
        [void]$Lines.Add("    - onboarding_governance_next_action_summary: $onboardingNextActionSummary")
    }
    if (-not [string]::IsNullOrWhiteSpace($onboardingNextActionGroupCount)) {
        [void]$Lines.Add("    - onboarding_governance_next_action_group_count: $onboardingNextActionGroupCount")
    }

    Add-ProjectTemplateBusinessDimensionContractLines `
        -Lines $Lines `
        -Report $Report `
        -Indent "    "

    if (-not [string]::IsNullOrWhiteSpace($SourceReportDisplay)) {
        [void]$Lines.Add("    - source_report_display: $SourceReportDisplay")
    }
    if (-not [string]::IsNullOrWhiteSpace($SourceJsonDisplay)) {
        [void]$Lines.Add("    - source_json_display: $SourceJsonDisplay")
    }
}

function Add-ReleaseGovernanceSourceReportContractLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object[]]$Reports,
        [string]$Heading,
        [string]$IdPropertyName,
        [string]$PathPropertyName,
        [string]$DisplayPropertyName,
        [string]$RepoRoot
    )

    $contractReports = @($Reports | Where-Object { $null -ne $_ })
    if ($contractReports.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    foreach ($report in $contractReports) {
        $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name $IdPropertyName) -Fallback "(unknown report)"
        $status = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "status")
        $releaseReady = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_ready")
        $sourceFailureCount = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "source_failure_count") -Fallback "0"
        $schema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")
        $sourceReport = Get-ReleaseBlockerPropertyValue -Object $report -Name $PathPropertyName
        $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $report -Name $DisplayPropertyName
        if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
            $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $sourceReport
        }

        [void]$Lines.Add("- ${id}: status=$status ready=$releaseReady source_failures=$sourceFailureCount schema=$schema")
        if (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
            [void]$Lines.Add("  - source_report: $sourceReport")
        }
        [void]$Lines.Add("  - source_report_display: $(Get-ReleaseBlockerDisplayValue -Value $sourceReportDisplay)")

        $sourceJson = Get-ReleaseBlockerPropertyValue -Object $report -Name "source_json"
        if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
            [void]$Lines.Add("  - source_json: $sourceJson")
        }

        $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $report -Name "source_json_display"
        if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay) -and -not [string]::IsNullOrWhiteSpace($sourceJson)) {
            $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $sourceJson
        }
        if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            [void]$Lines.Add("  - source_json_display: $(Get-ReleaseBlockerDisplayValue -Value $sourceJsonDisplay)")
        }
        [void]$Lines.Add("  - source_failure_count: $sourceFailureCount")
        Add-ProjectTemplateDeliveryReadinessContractLines `
            -Lines $Lines `
            -Report $report `
            -SourceReportDisplay $sourceReportDisplay `
            -SourceJsonDisplay $sourceJsonDisplay
        Add-ProjectTemplateOnboardingGovernanceReportContractLines `
            -Lines $Lines `
            -Report $report `
            -SourceReportDisplay $sourceReportDisplay `
            -SourceJsonDisplay $sourceJsonDisplay
        Add-ReleaseGovernanceManifestSignoffSourceReportLines `
            -Lines $Lines `
            -Report $report `
            -Indent "  "
        Add-ReleaseGovernanceProjectTemplateReadinessChecklistSourceReportLines `
            -Lines $Lines `
            -Report $report `
            -Indent "  "
        Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditSourceReportLines `
            -Lines $Lines `
            -Report $report `
            -Indent "  "
        Add-ReleaseGovernanceWordVisualStandardReviewMetadataSourceReportLines `
            -Lines $Lines `
            -Report $report `
            -Indent "  "

        foreach ($fieldName in @(
                "preflight_ready",
                "full_visual_gate_required",
                "full_visual_gate_status",
                "controlled_visual_smoke_available",
                "controlled_visual_smoke_status",
                "controlled_visual_smoke_passed",
                "controlled_visual_smoke_case_count",
                "controlled_visual_smoke_json",
                "controlled_visual_smoke_json_display",
                "error",
                "build_command"
            )) {
            $fieldValue = Get-ReleaseBlockerPropertyValue -Object $report -Name $fieldName
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                $label = if ($fieldName -eq "build_command") { "build" } else { $fieldName }
                [void]$Lines.Add("  - ${label}: $fieldValue")
            }
        }
    }
}

function Add-ProjectTemplateGovernanceCombinedContractLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$ReadinessReport,
        [AllowNull()]$OnboardingReport
    )

    if (-not (Test-ProjectTemplateDeliveryReadinessReport -Report $ReadinessReport) -or
        -not (Test-ProjectTemplateOnboardingGovernanceReport -Report $OnboardingReport)) {
        return
    }

    $readinessStatus = Get-ReleaseBlockerPropertyValue -Object $ReadinessReport -Name "status"
    $readinessReleaseReady = Get-ReleaseBlockerPropertyValue -Object $ReadinessReport -Name "release_ready"
    $onboardingStatus = Get-ReleaseBlockerPropertyValue -Object $OnboardingReport -Name "status"
    $onboardingReleaseReady = Get-ReleaseBlockerPropertyValue -Object $OnboardingReport -Name "release_ready"
    $schemaApprovalSummary = Format-ProjectTemplateSchemaApprovalStatusSummary `
        -Value (Get-ReleaseBlockerPropertyObject -Object $OnboardingReport -Name "schema_approval_status_summary") `
        -Fallback $onboardingStatus
    if ([string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
        $schemaApprovalSummary = "unknown"
    }

    $readinessSourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $ReadinessReport -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($readinessSourceReportDisplay)) {
        $readinessSourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $ReadinessReport -Name "expected_summary_display"
    }
    $readinessSourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $ReadinessReport -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($readinessSourceJsonDisplay)) {
        $readinessSourceJsonDisplay = $readinessSourceReportDisplay
    }

    $onboardingSourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $OnboardingReport -Name "source_report_display"
    $onboardingSourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $OnboardingReport -Name "source_json_display"

    [void]$Lines.Add("- project_template_delivery_readiness / project_template_onboarding.schema_approval:")
    [void]$Lines.Add("  - project_template_delivery_readiness_contract:")
    [void]$Lines.Add("    - source_schema: featherdoc.project_template_delivery_readiness_report.v1")
    if (-not [string]::IsNullOrWhiteSpace($readinessStatus)) {
        [void]$Lines.Add("    - readiness_status: $readinessStatus")
    }
    if (-not [string]::IsNullOrWhiteSpace($readinessReleaseReady)) {
        [void]$Lines.Add("    - readiness_release_ready: $readinessReleaseReady")
    }
    if (-not [string]::IsNullOrWhiteSpace($readinessSourceReportDisplay)) {
        [void]$Lines.Add("    - readiness_source_report_display: $readinessSourceReportDisplay")
    }
    if (-not [string]::IsNullOrWhiteSpace($readinessSourceJsonDisplay)) {
        [void]$Lines.Add("    - readiness_source_json_display: $readinessSourceJsonDisplay")
    }
    [void]$Lines.Add("  - project_template_onboarding_governance_contract:")
    [void]$Lines.Add("    - source_schema: featherdoc.project_template_onboarding_governance_report.v1")
    if (-not [string]::IsNullOrWhiteSpace($onboardingStatus)) {
        [void]$Lines.Add("    - status: $onboardingStatus")
    }
    if (-not [string]::IsNullOrWhiteSpace($onboardingReleaseReady)) {
        [void]$Lines.Add("    - release_ready: $onboardingReleaseReady")
    }
    [void]$Lines.Add("    - schema_approval_status_summary: $schemaApprovalSummary")
    [void]$Lines.Add("    - schema_approval_status_summary_marker: schema_approval_status_summary=$schemaApprovalSummary")
    if (-not [string]::IsNullOrWhiteSpace($onboardingSourceReportDisplay)) {
        [void]$Lines.Add("    - source_report_display: $onboardingSourceReportDisplay")
    }
    if (-not [string]::IsNullOrWhiteSpace($onboardingSourceJsonDisplay)) {
        [void]$Lines.Add("    - source_json_display: $onboardingSourceJsonDisplay")
    }
}

function Add-ReleaseGovernanceManifestSignoffSourceReportLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Report,
        [string]$Indent = "  "
    )

    $status = Get-ReleaseBlockerPropertyValue -Object $Report -Name "manifest_signoff_entrypoints_status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        return
    }

    foreach ($fieldName in @(
            "manifest_signoff_entrypoints_status",
            "manifest_signoff_entrypoints_release_assets_manifest",
            "manifest_signoff_entrypoints_release_assets_manifest_display",
            "manifest_signoff_entrypoints_required_entrypoint_count",
            "manifest_signoff_entrypoints_checklist_marker"
        )) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Report -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            [void]$Lines.Add("${Indent}- ${fieldName}: $fieldValue")
        }
    }

    foreach ($fieldName in @(
            "manifest_signoff_entrypoints_entrypoint_ids",
            "manifest_signoff_entrypoints_required_contracts",
            "manifest_signoff_entrypoints_required_fields"
        )) {
        $values = @(
            Get-ReleaseBlockerArrayProperty -Object $Report -Name $fieldName |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        if ($values.Count -gt 0) {
            [void]$Lines.Add("${Indent}- ${fieldName}: $($values -join ', ')")
        }
    }

    $entrypoints = @(Get-ReleaseBlockerArrayProperty -Object $Report -Name "manifest_signoff_entrypoints_entrypoints")
    if ($entrypoints.Count -eq 0) {
        return
    }

    [void]$Lines.Add("${Indent}- manifest_signoff_entrypoints:")
    $entryIndent = "${Indent}  "
    foreach ($entrypoint in $entrypoints) {
        $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entrypoint -Name "id") -Fallback "(unknown entrypoint)"
        $required = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entrypoint -Name "required")
        $pathDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entrypoint -Name "path_display")
        [void]$Lines.Add("${entryIndent}- ${id}: required=$required path_display=$pathDisplay")
    }
}

function Add-ReleaseGovernanceProjectTemplateReadinessChecklistSourceReportLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Report,
        [string]$Indent = "  "
    )

    $status = Get-ReleaseBlockerPropertyValue -Object $Report -Name "project_template_readiness_checklist_entrypoints_status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        return
    }

    foreach ($fieldName in @(
            "project_template_readiness_checklist_entrypoints_status",
            "project_template_readiness_checklist_entrypoints_checklist_label",
            "project_template_readiness_checklist_entrypoints_checklist_path",
            "project_template_readiness_checklist_entrypoints_required_entrypoint_count",
            "project_template_readiness_checklist_entrypoints_checklist_marker"
        )) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Report -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            [void]$Lines.Add("${Indent}- ${fieldName}: $fieldValue")
        }
    }

    $entrypointIds = @(
        Get-ReleaseBlockerArrayProperty -Object $Report -Name "project_template_readiness_checklist_entrypoints_entrypoint_ids" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
    if ($entrypointIds.Count -gt 0) {
        [void]$Lines.Add("${Indent}- project_template_readiness_checklist_entrypoints_entrypoint_ids: $($entrypointIds -join ', ')")
    }

    $entrypoints = @(Get-ReleaseBlockerArrayProperty -Object $Report -Name "project_template_readiness_checklist_entrypoints_entrypoints")
    if ($entrypoints.Count -eq 0) {
        return
    }

    [void]$Lines.Add("${Indent}- project_template_readiness_checklist_entrypoints:")
    $entryIndent = "${Indent}  "
    foreach ($entrypoint in $entrypoints) {
        $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entrypoint -Name "id") -Fallback "(unknown entrypoint)"
        $required = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entrypoint -Name "required")
        $pathDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entrypoint -Name "path_display")
        [void]$Lines.Add("${entryIndent}- ${id}: required=$required path_display=$pathDisplay")
    }
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditSourceReportLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Report,
        [string]$Indent = "  "
    )

    $status = Get-ReleaseBlockerPropertyValue -Object $Report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        return
    }

    foreach ($fieldName in @(
            "release_entry_project_template_readiness_checklist_material_safety_audit_status",
            "release_entry_project_template_readiness_checklist_material_safety_audit_script",
            "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count",
            "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label",
            "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field",
            "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema",
            "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path",
            "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker",
            "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
        )) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Report -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            [void]$Lines.Add("${Indent}- ${fieldName}: $fieldValue")
        }
    }

    $auditedEntrypoints = @(
        Get-ReleaseBlockerArrayProperty -Object $Report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
    if ($auditedEntrypoints.Count -gt 0) {
        [void]$Lines.Add("${Indent}- release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: $($auditedEntrypoints -join ', ')")
    }
}

function Add-ReleaseGovernanceWordVisualStandardReviewMetadataSourceReportLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Report,
        [string]$Indent = "  "
    )

    $metadataCount = Get-ReleaseBlockerPropertyValue -Object $Report -Name "word_visual_standard_review_metadata_count"
    $metadata = @(Get-ReleaseBlockerArrayProperty -Object $Report -Name "word_visual_standard_review_metadata")
    if ([string]::IsNullOrWhiteSpace($metadataCount) -and $metadata.Count -eq 0) {
        return
    }

    foreach ($fieldName in @(
            "word_visual_standard_review_metadata_count",
            "word_visual_standard_review_status_summary",
            "word_visual_standard_review_verdict_summary"
        )) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Report -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            [void]$Lines.Add("${Indent}- ${fieldName}: $fieldValue")
        }
    }

    $taskKeys = @(
        Get-ReleaseBlockerArrayProperty -Object $Report -Name "word_visual_standard_review_task_keys" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
    if ($taskKeys.Count -gt 0) {
        [void]$Lines.Add("${Indent}- word_visual_standard_review_task_keys: $($taskKeys -join ', ')")
    }

    if ($metadata.Count -eq 0) {
        return
    }

    [void]$Lines.Add("${Indent}- word_visual_standard_review_metadata:")
    $entryIndent = "${Indent}  "
    foreach ($entry in $metadata) {
        $taskKey = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entry -Name "task_key") -Fallback "(unknown task)"
        $reviewTaskKey = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entry -Name "review_task_key")
        $label = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entry -Name "label")
        $verdict = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entry -Name "verdict")
        $reviewStatus = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entry -Name "review_status")
        $reviewMethod = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entry -Name "review_method")
        $reviewResultPath = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entry -Name "review_result_path")
        $finalReviewPath = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $entry -Name "final_review_path")

        [void]$Lines.Add("${entryIndent}- ${taskKey}: review_task_key=$reviewTaskKey label=$label verdict=$verdict review_status=$reviewStatus review_method=$reviewMethod review_result_path=$reviewResultPath final_review_path=$finalReviewPath")
    }
}

function Select-ReleaseGovernancePreferredReleaseCandidateSourceReport {
    param([AllowNull()]$Reports)

    $reportItems = @($Reports | Where-Object { $null -ne $_ })
    foreach ($pathPattern in @(
            "release-candidate-checks[\\/]+report[\\/]+summary\.json$",
            "release-candidate-checks[\\/]+summary\.json$",
            "release_candidate_summary"
        )) {
        $report = $reportItems |
            Where-Object {
                $schema = Get-ReleaseBlockerPropertyValue -Object $_ -Name "schema"
                $pathDisplay = Get-ReleaseBlockerPropertyValue -Object $_ -Name "path_display"

                $schema -eq "featherdoc.release_candidate_summary" -and
                    $pathDisplay -match $pathPattern
            } |
            Select-Object -First 1
        if ($null -ne $report) {
            return $report
        }
    }

    return $reportItems | Select-Object -First 1
}

function Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine {
    param([AllowNull()]$Summary)

    $handoff = Get-ReleaseGovernanceHandoff -Summary $Summary
    $reports = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "project_template_readiness_checklist_entrypoints_source_reports")
    $count = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "project_template_readiness_checklist_entrypoints_source_report_count"
    if ([string]::IsNullOrWhiteSpace($count)) {
        $count = [string]$reports.Count
    }

    if ($reports.Count -eq 0 -and $count -eq "0") {
        return ""
    }

    $report = Select-ReleaseGovernancePreferredReleaseCandidateSourceReport -Reports $reports
    $entrypointIds = @(
        Get-ReleaseBlockerArrayProperty -Object $report -Name "project_template_readiness_checklist_entrypoints_entrypoint_ids" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
    $entrypointPathParts = @(
        Get-ReleaseBlockerArrayProperty -Object $report -Name "project_template_readiness_checklist_entrypoints_entrypoints" |
            ForEach-Object {
                $id = Get-ReleaseBlockerPropertyValue -Object $_ -Name "id"
                if (-not [string]::IsNullOrWhiteSpace($id)) {
                    $required = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $_ -Name "required")
                    $pathDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $_ -Name "path_display")

                    "${id}:required=${required}:path_display=${pathDisplay}"
                }
            } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )

    return "Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=$count, status=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "project_template_readiness_checklist_entrypoints_status")), checklist_path=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "project_template_readiness_checklist_entrypoints_checklist_path")), required_entrypoint_count=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "project_template_readiness_checklist_entrypoints_required_entrypoint_count")), entrypoints=$(Get-ReleaseBlockerDisplayValue -Value ($entrypointIds -join ', ')), entrypoint_paths=$(Get-ReleaseBlockerDisplayValue -Value ($entrypointPathParts -join '; ')), marker=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "project_template_readiness_checklist_entrypoints_checklist_marker")), source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")), source_report=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "path_display"))"
}

function Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine {
    param([AllowNull()]$Summary)

    $handoff = Get-ReleaseGovernanceHandoff -Summary $Summary
    $reports = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports")
    $count = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count"
    if ([string]::IsNullOrWhiteSpace($count)) {
        $count = [string]$reports.Count
    }

    if ($reports.Count -eq 0 -and $count -eq "0") {
        return ""
    }

    $report = Select-ReleaseGovernancePreferredReleaseCandidateSourceReport -Reports $reports
    $rollup = Get-ReleaseBlockerPropertyObject -Object $handoff -Name "release_blocker_rollup"
    $sourceReport = ""
    foreach ($fieldName in @("report_markdown_display", "report_markdown", "summary_json_display", "summary_json")) {
        $sourceReport = Get-ReleaseBlockerPropertyValue -Object $rollup -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
            break
        }
    }
    if ([string]::IsNullOrWhiteSpace($sourceReport)) {
        $sourceReport = Get-ReleaseBlockerPropertyValue -Object $report -Name "path_display"
    }

    $auditedEntrypoints = @(
        Get-ReleaseBlockerArrayProperty -Object $report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )

    return "Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=$count, status=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_status")), audit_script=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_script")), audited_entrypoint_count=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count")), audited_entrypoints=$(Get-ReleaseBlockerDisplayValue -Value ($auditedEntrypoints -join ', ')), compact_evidence_label=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label")), compact_evidence_field=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field")), compact_evidence_source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema")), checklist_path=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path")), checklist_marker=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker")), material_safety_marker=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker")), source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")), source_report=$(Get-ReleaseBlockerDisplayValue -Value $sourceReport)"
}

function Get-ReleaseGovernanceWordVisualStandardReviewMetadataEvidenceLine {
    param([AllowNull()]$Summary)

    $handoff = Get-ReleaseGovernanceHandoff -Summary $Summary
    $reports = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "word_visual_standard_review_metadata_source_reports")
    $count = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "word_visual_standard_review_metadata_source_report_count"
    if ([string]::IsNullOrWhiteSpace($count)) {
        $count = [string]$reports.Count
    }

    if ($reports.Count -eq 0 -and $count -eq "0") {
        return ""
    }

    $report = Select-ReleaseGovernancePreferredReleaseCandidateSourceReport -Reports $reports

    $taskKeys = @(
        Get-ReleaseBlockerArrayProperty -Object $report -Name "word_visual_standard_review_task_keys" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
    $taskReviewParts = @(
        Get-ReleaseBlockerArrayProperty -Object $report -Name "word_visual_standard_review_metadata" |
            ForEach-Object {
                $taskKey = Get-ReleaseBlockerPropertyValue -Object $_ -Name "task_key"
                if (-not [string]::IsNullOrWhiteSpace($taskKey)) {
                    $reviewTaskKey = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $_ -Name "review_task_key")
                    $verdict = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $_ -Name "verdict")
                    $reviewStatus = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $_ -Name "review_status")
                    $reviewMethod = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $_ -Name "review_method")
                    $reviewResultPath = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $_ -Name "review_result_path")
                    $finalReviewPath = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $_ -Name "final_review_path")

                    "${taskKey}:review_task_key=${reviewTaskKey}:verdict=${verdict}:review_status=${reviewStatus}:review_method=${reviewMethod}:review_result_path=${reviewResultPath}:final_review_path=${finalReviewPath}"
                }
            } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )

    return "Word visual standard review metadata evidence: word_visual_standard_review_metadata_source_reports=$count, metadata_count=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "word_visual_standard_review_metadata_count")), task_keys=$(Get-ReleaseBlockerDisplayValue -Value ($taskKeys -join ', ')), status_summary=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "word_visual_standard_review_status_summary")), verdict_summary=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "word_visual_standard_review_verdict_summary")), task_reviews=$(Get-ReleaseBlockerDisplayValue -Value ($taskReviewParts -join '; ')), source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")), source_report=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "path_display"))"
}
