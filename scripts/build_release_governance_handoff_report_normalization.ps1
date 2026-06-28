function New-ReportEntry {
    param(
        [string]$RepoRoot,
        [string]$Id,
        [string]$Title,
        [string]$ExpectedSummaryPath,
        [string]$BuildCommand,
        [string]$Source = "default",
        [object]$Json = $null,
        [string]$Status = "missing",
        [string]$ErrorMessage = ""
    )

    $schema = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "schema" }
    $metrics = if ($null -eq $Json) {
        @()
    } else {
        @(New-GovernanceMetrics `
            -Summary $Json `
            -ReportId $Id `
            -ReportTitle $Title `
            -SourceSchema $schema `
            -SourceReport $ExpectedSummaryPath `
            -SourceReportDisplay (Get-DisplayPath -RepoRoot $RepoRoot -Path $ExpectedSummaryPath))
    }
    $isDocxFunctionalSmokeReadiness = [string]::Equals(
        $schema,
        "featherdoc.docx_functional_smoke_readiness.v1",
        [System.StringComparison]::OrdinalIgnoreCase)
    return [ordered]@{
        id = $Id
        title = $Title
        source = $Source
        expected_summary = $ExpectedSummaryPath
        expected_summary_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $ExpectedSummaryPath
        source_report = $ExpectedSummaryPath
        source_report_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $ExpectedSummaryPath
        source_json = $ExpectedSummaryPath
        source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $ExpectedSummaryPath
        build_command = $BuildCommand
        schema = $schema
        status_present = ($null -ne $Json -and $null -ne (Get-JsonProperty -Object $Json -Name "status"))
        status = if ($null -eq $Json) { $Status } else { Get-JsonString -Object $Json -Name "status" -DefaultValue $Status }
        release_ready_present = ($null -ne $Json -and $null -ne (Get-JsonProperty -Object $Json -Name "release_ready"))
        release_ready = if ($null -eq $Json) { $false } else { Get-JsonBool -Object $Json -Name "release_ready" }
        release_blocker_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "release_blocker_count" }
        action_item_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "action_item_count" }
        warning_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "warning_count" }
        source_failure_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "source_failure_count" }
        latest_schema_approval_gate_status = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "latest_schema_approval_gate_status" }
        schema_approval_status_summary = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "schema_approval_status_summary") }
        business_document_type_summary = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "business_document_type_summary") }
        corpus_role_summary = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "corpus_role_summary") }
        onboarding_governance_next_action = if ($null -eq $Json) { $null } else { Get-JsonProperty -Object $Json -Name "onboarding_governance_next_action" }
        onboarding_governance_next_action_summary = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "onboarding_governance_next_action_summary") }
        onboarding_governance_next_action_group_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "onboarding_governance_next_action_group_count" }
        report_markdown = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "report_markdown" }
        report_markdown_display = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "report_markdown_display" }
        docx_functional_smoke_ready = if ($isDocxFunctionalSmokeReadiness -and $null -ne $Json) { Get-JsonBool -Object $Json -Name "docx_functional_smoke_ready" } else { $false }
        evidence_scope = if ($isDocxFunctionalSmokeReadiness -and $null -ne $Json) { Get-JsonString -Object $Json -Name "evidence_scope" } else { "" }
        evidence_scope_note = if ($isDocxFunctionalSmokeReadiness -and $null -ne $Json) { Get-JsonString -Object $Json -Name "evidence_scope_note" } else { "" }
        boundary = if ($isDocxFunctionalSmokeReadiness -and $null -ne $Json) { Get-JsonString -Object $Json -Name "boundary" } else { "" }
        marker = if ($isDocxFunctionalSmokeReadiness -and $null -ne $Json) { Get-JsonString -Object $Json -Name "marker" } else { "" }
        summary_json_display = if ($isDocxFunctionalSmokeReadiness -and $null -ne $Json) { Get-JsonString -Object $Json -Name "summary_json_display" } else { "" }
        release_blockers = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "release_blockers") }
        action_items = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "action_items") }
        informational_action_item_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "informational_action_item_count" }
        informational_action_items = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "informational_action_items") }
        warnings = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "warnings") }
        governance_metric_count = @($metrics).Count
        governance_metrics = @($metrics)
        error = $ErrorMessage
    }
}

function Add-OptionalJsonProperties {
    param(
        [System.Collections.IDictionary]$Target,
        $Source,
        [string[]]$Names
    )

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Source -Name $name
        if ($null -ne $value) {
            $Target[$name] = $value
        }
    }
}

function Add-SchemaCorpusMetadataProperties {
    param(
        [System.Collections.IDictionary]$Target,
        $Source
    )

    Add-OptionalJsonProperties `
        -Target $Target `
        -Source $Source `
        -Names @(
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
        )
}

function Add-NormalizedBlockers {
    param(
        [System.Collections.Generic.List[object]]$Collection,
        [object]$Report,
        [AllowNull()]$ProjectTemplateOnboardingGovernanceContract
    )

    $isProjectTemplateDeliveryReadiness = Test-ProjectTemplateDeliveryReadinessReportEntry -Report $Report
    $readinessStatus = if ($isProjectTemplateDeliveryReadiness -and (Get-JsonBool -Object $Report -Name "status_present")) {
        Get-JsonString -Object $Report -Name "status"
    } else {
        ""
    }
    $readinessReleaseReady = if ($isProjectTemplateDeliveryReadiness -and (Get-JsonBool -Object $Report -Name "release_ready_present")) {
        [string](Get-JsonBool -Object $Report -Name "release_ready")
    } else {
        ""
    }

    foreach ($blocker in @($Report.release_blockers)) {
        $sourceReportDisplay = Get-JsonString -Object $blocker -Name "source_report_display" -DefaultValue ([string]$Report.expected_summary_display)
        $sourceReport = Get-JsonString -Object $blocker -Name "source_report" -DefaultValue ([string]$Report.expected_summary)
        $sourceJsonDisplay = Get-JsonString -Object $blocker -Name "source_json_display" -DefaultValue $sourceReportDisplay
        $sourceJson = Get-JsonString -Object $blocker -Name "source_json" -DefaultValue $sourceReport
        $sourceSchema = Get-JsonString -Object $blocker -Name "source_schema" -DefaultValue ([string]$Report.schema)
        $onboardingStatus = Get-JsonString -Object $blocker -Name "onboarding_governance_status"
        $onboardingReleaseReady = Get-JsonString -Object $blocker -Name "onboarding_governance_release_ready"
        $onboardingSchemaApprovalSummary = @(Get-JsonArray -Object $blocker -Name "onboarding_governance_schema_approval_status_summary")
        $onboardingNextAction = Get-JsonProperty -Object $blocker -Name "onboarding_governance_next_action"
        $onboardingNextActionSummary = @(Get-JsonArray -Object $blocker -Name "onboarding_governance_next_action_summary")
        $onboardingNextActionGroupCount = Get-JsonInt -Object $blocker -Name "onboarding_governance_next_action_group_count"
        $onboardingSourceReportDisplay = Get-JsonString -Object $blocker -Name "onboarding_governance_source_report_display"
        $onboardingSourceJsonDisplay = Get-JsonString -Object $blocker -Name "onboarding_governance_source_json_display"
        if ([string]::Equals($sourceSchema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase) -and $null -ne $ProjectTemplateOnboardingGovernanceContract) {
            if ([string]::IsNullOrWhiteSpace($onboardingStatus)) {
                $onboardingStatus = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "status"
            }
            if ([string]::IsNullOrWhiteSpace($onboardingReleaseReady)) {
                $onboardingReleaseReady = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "release_ready"
            }
            if ($onboardingSchemaApprovalSummary.Count -eq 0) {
                $onboardingSchemaApprovalSummary = @(Get-JsonArray -Object $ProjectTemplateOnboardingGovernanceContract -Name "schema_approval_status_summary")
            }
            if ($null -eq $onboardingNextAction) {
                $onboardingNextAction = Get-JsonProperty -Object $ProjectTemplateOnboardingGovernanceContract -Name "next_action"
            }
            if ($onboardingNextActionSummary.Count -eq 0) {
                $onboardingNextActionSummary = @(Get-JsonArray -Object $ProjectTemplateOnboardingGovernanceContract -Name "next_action_summary")
            }
            if ($onboardingNextActionGroupCount -eq 0 -and $onboardingNextActionSummary.Count -gt 0) {
                $onboardingNextActionGroupCount = $onboardingNextActionSummary.Count
            }
            if ($onboardingNextActionGroupCount -eq 0) {
                $onboardingNextActionGroupCount = Get-JsonInt -Object $ProjectTemplateOnboardingGovernanceContract -Name "next_action_group_count"
            }
            if ([string]::IsNullOrWhiteSpace($onboardingSourceReportDisplay)) {
                $onboardingSourceReportDisplay = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "source_report_display"
            }
            if ([string]::IsNullOrWhiteSpace($onboardingSourceJsonDisplay)) {
                $onboardingSourceJsonDisplay = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "source_json_display"
            }
        }
        $normalizedBlocker = [ordered]@{
            report_id = [string]$Report.id
            report_title = [string]$Report.title
            id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
            blocker_id = Get-JsonString -Object $blocker -Name "blocker_id" -DefaultValue (Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker")
            project_id = Get-JsonString -Object $blocker -Name "project_id"
            template_name = Get-JsonString -Object $blocker -Name "template_name"
            candidate_type = Get-JsonString -Object $blocker -Name "candidate_type"
            severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "error"
            status = Get-JsonString -Object $blocker -Name "status"
            action = Get-JsonString -Object $blocker -Name "action"
            message = Get-JsonString -Object $blocker -Name "message"
            reason = Get-JsonString -Object $blocker -Name "reason" -DefaultValue (Get-JsonString -Object $blocker -Name "message")
            source_schema = $sourceSchema
            source_report = $sourceReport
            source_report_display = $sourceReportDisplay
            source_json = $sourceJson
            source_json_display = $sourceJsonDisplay
            readiness_status = $readinessStatus
            readiness_release_ready = $readinessReleaseReady
            schema_approval_status_summary = @(Get-JsonArray -Object $blocker -Name "schema_approval_status_summary")
            requires_reviewer_action = Get-JsonBool -Object $blocker -Name "requires_reviewer_action"
            reviewer_action_summary = Get-JsonString -Object $blocker -Name "reviewer_action_summary"
            reviewer_action_reason = Get-JsonString -Object $blocker -Name "reviewer_action_reason"
            reviewer_actions = @(Get-JsonArray -Object $blocker -Name "reviewer_actions")
            onboarding_governance_status = $onboardingStatus
            onboarding_governance_release_ready = $onboardingReleaseReady
            onboarding_governance_schema_approval_status_summary = @($onboardingSchemaApprovalSummary)
            onboarding_governance_next_action = $onboardingNextAction
            onboarding_governance_next_action_summary = @($onboardingNextActionSummary)
            onboarding_governance_next_action_group_count = $onboardingNextActionGroupCount
            onboarding_governance_source_report_display = $onboardingSourceReportDisplay
            onboarding_governance_source_json_display = $onboardingSourceJsonDisplay
            input_docx = Get-JsonString -Object $blocker -Name "input_docx"
            input_docx_display = Get-JsonString -Object $blocker -Name "input_docx_display"
            schema_target = Get-JsonString -Object $blocker -Name "schema_target"
            target_mode = Get-JsonString -Object $blocker -Name "target_mode"
            repair_strategy = Get-JsonString -Object $blocker -Name "repair_strategy"
            repair_hint = Get-JsonString -Object $blocker -Name "repair_hint"
            command_template = Get-JsonString -Object $blocker -Name "command_template"
            repair_action_classes = @(Get-JsonArray -Object $blocker -Name "repair_action_classes")
            matched_document_count = Get-JsonProperty -Object $blocker -Name "matched_document_count"
            unmatched_catalog_document_count = Get-JsonProperty -Object $blocker -Name "unmatched_catalog_document_count"
            unmatched_baseline_document_count = Get-JsonProperty -Object $blocker -Name "unmatched_baseline_document_count"
            alignment_gap_count = Get-JsonProperty -Object $blocker -Name "alignment_gap_count"
            catalog_coverage_percent = Get-JsonProperty -Object $blocker -Name "catalog_coverage_percent"
            baseline_coverage_percent = Get-JsonProperty -Object $blocker -Name "baseline_coverage_percent"
            coverage_score = Get-JsonProperty -Object $blocker -Name "coverage_score"
            catalog_document_keys = @(Get-JsonArray -Object $blocker -Name "catalog_document_keys")
            baseline_document_keys = @(Get-JsonArray -Object $blocker -Name "baseline_document_keys")
            matched_document_keys = @(Get-JsonArray -Object $blocker -Name "matched_document_keys")
        }
        Add-SchemaCorpusMetadataProperties -Target $normalizedBlocker -Source $blocker
        $Collection.Add($normalizedBlocker) | Out-Null
    }
}

function Add-NormalizedActions {
    param(
        [System.Collections.Generic.List[object]]$Collection,
        [System.Collections.Generic.List[object]]$InformationalCollection,
        [object]$Report,
        [AllowNull()]$ProjectTemplateOnboardingGovernanceContract,
        [switch]$ForceInformational
    )

    $isProjectTemplateDeliveryReadiness = Test-ProjectTemplateDeliveryReadinessReportEntry -Report $Report
    $readinessStatus = if ($isProjectTemplateDeliveryReadiness -and (Get-JsonBool -Object $Report -Name "status_present")) {
        Get-JsonString -Object $Report -Name "status"
    } else {
        ""
    }
    $readinessReleaseReady = if ($isProjectTemplateDeliveryReadiness -and (Get-JsonBool -Object $Report -Name "release_ready_present")) {
        [string](Get-JsonBool -Object $Report -Name "release_ready")
    } else {
        ""
    }

    $sourceItems = if ($ForceInformational) { @($Report.informational_action_items) } else { @($Report.action_items) }
    foreach ($item in @($sourceItems)) {
        $sourceReportDisplay = Get-JsonString -Object $item -Name "source_report_display" -DefaultValue ([string]$Report.expected_summary_display)
        $sourceReport = Get-JsonString -Object $item -Name "source_report" -DefaultValue ([string]$Report.expected_summary)
        $sourceJsonDisplay = Get-JsonString -Object $item -Name "source_json_display" -DefaultValue $sourceReportDisplay
        $sourceJson = Get-JsonString -Object $item -Name "source_json" -DefaultValue $sourceReport
        $openCommand = Get-JsonString -Object $item -Name "open_command" -DefaultValue (Get-JsonString -Object $item -Name "command")
        if ([string]::IsNullOrWhiteSpace($openCommand)) {
            $openCommand = [string]$Report.build_command
        }
        $sourceSchema = Get-JsonString -Object $item -Name "source_schema" -DefaultValue ([string]$Report.schema)
        $onboardingStatus = Get-JsonString -Object $item -Name "onboarding_governance_status"
        $onboardingReleaseReady = Get-JsonString -Object $item -Name "onboarding_governance_release_ready"
        $onboardingSchemaApprovalSummary = @(Get-JsonArray -Object $item -Name "onboarding_governance_schema_approval_status_summary")
        $onboardingNextAction = Get-JsonProperty -Object $item -Name "onboarding_governance_next_action"
        $onboardingNextActionSummary = @(Get-JsonArray -Object $item -Name "onboarding_governance_next_action_summary")
        $onboardingNextActionGroupCount = Get-JsonInt -Object $item -Name "onboarding_governance_next_action_group_count"
        $onboardingSourceReportDisplay = Get-JsonString -Object $item -Name "onboarding_governance_source_report_display"
        $onboardingSourceJsonDisplay = Get-JsonString -Object $item -Name "onboarding_governance_source_json_display"
        if ([string]::Equals($sourceSchema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase) -and $null -ne $ProjectTemplateOnboardingGovernanceContract) {
            if ([string]::IsNullOrWhiteSpace($onboardingStatus)) {
                $onboardingStatus = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "status"
            }
            if ([string]::IsNullOrWhiteSpace($onboardingReleaseReady)) {
                $onboardingReleaseReady = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "release_ready"
            }
            if ($onboardingSchemaApprovalSummary.Count -eq 0) {
                $onboardingSchemaApprovalSummary = @(Get-JsonArray -Object $ProjectTemplateOnboardingGovernanceContract -Name "schema_approval_status_summary")
            }
            if ($null -eq $onboardingNextAction) {
                $onboardingNextAction = Get-JsonProperty -Object $ProjectTemplateOnboardingGovernanceContract -Name "next_action"
            }
            if ($onboardingNextActionSummary.Count -eq 0) {
                $onboardingNextActionSummary = @(Get-JsonArray -Object $ProjectTemplateOnboardingGovernanceContract -Name "next_action_summary")
            }
            if ($onboardingNextActionGroupCount -eq 0 -and $onboardingNextActionSummary.Count -gt 0) {
                $onboardingNextActionGroupCount = $onboardingNextActionSummary.Count
            }
            if ($onboardingNextActionGroupCount -eq 0) {
                $onboardingNextActionGroupCount = Get-JsonInt -Object $ProjectTemplateOnboardingGovernanceContract -Name "next_action_group_count"
            }
            if ([string]::IsNullOrWhiteSpace($onboardingSourceReportDisplay)) {
                $onboardingSourceReportDisplay = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "source_report_display"
            }
            if ([string]::IsNullOrWhiteSpace($onboardingSourceJsonDisplay)) {
                $onboardingSourceJsonDisplay = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "source_json_display"
            }
        }
        $actionId = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
        $action = Get-JsonString -Object $item -Name "action"
        if ([string]::IsNullOrWhiteSpace($action)) {
            $action = $actionId
        }
        $normalizedAction = [ordered]@{
            report_id = [string]$Report.id
            report_title = [string]$Report.title
            id = $actionId
            blocker_id = Get-JsonString -Object $item -Name "blocker_id" -DefaultValue (Get-JsonString -Object $item -Name "id" -DefaultValue "action_item")
            project_id = Get-JsonString -Object $item -Name "project_id"
            template_name = Get-JsonString -Object $item -Name "template_name"
            candidate_type = Get-JsonString -Object $item -Name "candidate_type"
            action = $action
            title = Get-JsonString -Object $item -Name "title"
            reason = Get-JsonString -Object $item -Name "reason" -DefaultValue (Get-JsonString -Object $item -Name "title")
            command = Get-JsonString -Object $item -Name "command"
            open_command = $openCommand
            category = Get-JsonString -Object $item -Name "category"
            severity = Get-JsonString -Object $item -Name "severity"
            release_blocking = Get-JsonBool -Object $item -Name "release_blocking" -DefaultValue $true
            optional = Get-JsonBool -Object $item -Name "optional"
            source_schema = $sourceSchema
            source_report = $sourceReport
            source_report_display = $sourceReportDisplay
            source_json = $sourceJson
            source_json_display = $sourceJsonDisplay
            readiness_status = $readinessStatus
            readiness_release_ready = $readinessReleaseReady
            schema_approval_status_summary = @(Get-JsonArray -Object $item -Name "schema_approval_status_summary")
            requires_reviewer_action = Get-JsonBool -Object $item -Name "requires_reviewer_action"
            reviewer_action_summary = Get-JsonString -Object $item -Name "reviewer_action_summary"
            reviewer_action_reason = Get-JsonString -Object $item -Name "reviewer_action_reason"
            reviewer_actions = @(Get-JsonArray -Object $item -Name "reviewer_actions")
            onboarding_governance_status = $onboardingStatus
            onboarding_governance_release_ready = $onboardingReleaseReady
            onboarding_governance_schema_approval_status_summary = @($onboardingSchemaApprovalSummary)
            onboarding_governance_next_action = $onboardingNextAction
            onboarding_governance_next_action_summary = @($onboardingNextActionSummary)
            onboarding_governance_next_action_group_count = $onboardingNextActionGroupCount
            onboarding_governance_source_report_display = $onboardingSourceReportDisplay
            onboarding_governance_source_json_display = $onboardingSourceJsonDisplay
            input_docx = Get-JsonString -Object $item -Name "input_docx"
            input_docx_display = Get-JsonString -Object $item -Name "input_docx_display"
            schema_target = Get-JsonString -Object $item -Name "schema_target"
            target_mode = Get-JsonString -Object $item -Name "target_mode"
            repair_strategy = Get-JsonString -Object $item -Name "repair_strategy"
            repair_hint = Get-JsonString -Object $item -Name "repair_hint"
            command_template = Get-JsonString -Object $item -Name "command_template"
            repair_action_classes = @(Get-JsonArray -Object $item -Name "repair_action_classes")
        }
        Add-SchemaCorpusMetadataProperties -Target $normalizedAction -Source $item
        if ($ForceInformational -or (Test-InformationalActionItem -Item $normalizedAction)) {
            $InformationalCollection.Add((Copy-ActionItemWithReleaseChecklistDefaults -Item $normalizedAction)) | Out-Null
        } else {
            $Collection.Add($normalizedAction) | Out-Null
        }
    }
}

function Add-NormalizedWarnings {
    param(
        [System.Collections.Generic.List[object]]$Collection,
        [object]$Report
    )

    foreach ($warning in @($Report.warnings)) {
        $sourceReportDisplay = Get-JsonString -Object $warning -Name "source_report_display" -DefaultValue ([string]$Report.expected_summary_display)
        $sourceReport = Get-JsonString -Object $warning -Name "source_report" -DefaultValue ([string]$Report.expected_summary)
        $sourceJsonDisplay = Get-JsonString -Object $warning -Name "source_json_display" -DefaultValue $sourceReportDisplay
        $sourceJson = Get-JsonString -Object $warning -Name "source_json" -DefaultValue $sourceReport
        $normalizedWarning = [ordered]@{
            report_id = [string]$Report.id
            report_title = [string]$Report.title
            id = Get-JsonString -Object $warning -Name "id" -DefaultValue "warning"
            project_id = Get-JsonString -Object $warning -Name "project_id"
            template_name = Get-JsonString -Object $warning -Name "template_name"
            candidate_type = Get-JsonString -Object $warning -Name "candidate_type"
            action = Get-JsonString -Object $warning -Name "action" -DefaultValue "review_release_governance_warning"
            message = Get-JsonString -Object $warning -Name "message"
            repair_strategy = Get-JsonString -Object $warning -Name "repair_strategy"
            repair_hint = Get-JsonString -Object $warning -Name "repair_hint"
            command_template = Get-JsonString -Object $warning -Name "command_template"
            repair_action_classes = @(Get-JsonArray -Object $warning -Name "repair_action_classes")
            source_schema = Get-JsonString -Object $warning -Name "source_schema" -DefaultValue ([string]$Report.schema)
            source_report = $sourceReport
            source_report_display = $sourceReportDisplay
            source_json = $sourceJson
            source_json_display = $sourceJsonDisplay
            input_docx = Get-JsonString -Object $warning -Name "input_docx"
            input_docx_display = Get-JsonString -Object $warning -Name "input_docx_display"
            schema_target = Get-JsonString -Object $warning -Name "schema_target"
            target_mode = Get-JsonString -Object $warning -Name "target_mode"
        }
        Add-SchemaCorpusMetadataProperties -Target $normalizedWarning -Source $warning
        $Collection.Add($normalizedWarning) | Out-Null
    }
}

function New-ExplicitReleaseCandidateWarningReport {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [object]$Json,
        [int]$Index
    )

    return [ordered]@{
        id = ("release_candidate_summary_{0}" -f $Index)
        title = "Release Candidate Summary"
        source = "explicit"
        expected_summary = $Path
        expected_summary_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
        source_report = $Path
        source_report_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
        source_json = $Path
        source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
        schema = Get-JsonString -Object $Json -Name "schema"
        warnings = @(Get-JsonArray -Object $Json -Name "warnings")
    }
}
