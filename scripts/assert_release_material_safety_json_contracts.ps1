function Add-ContentControlRepairContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $contentControlBlockerId = "content_control_data_binding.bound_placeholder"
    $expectedAction = "sync_or_fill_bound_content_control"
    $expectedRepairStrategy = "sync_bound_content_control"
    $expectedRepairHintMarker = "Rerun Custom XML sync"
    $expectedCommand = "sync-content-controls-from-custom-xml"
    $expectedRepairActionClasses = @(
        "release_blocking",
        "auto_repair_candidate",
        "manual_confirmation_required"
    )
    $label = "content-control repair contract"
    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()

    if ($leafName -eq "release_assets_manifest.json") {
        $contractsValue = Get-JsonPropertyValue -Object $Json -Name "content_control_repair_contracts"
        if ($null -eq $contractsValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing content_control_repair_contracts."
        } else {
            $contracts = @($contractsValue)
            $countValue = Get-JsonPropertyValue -Object $Json -Name "content_control_repair_contract_count"
            if ($null -eq $countValue) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing content_control_repair_contract_count."
            } else {
                $declaredCount = $null
                if (-not (Test-StrictJsonInt64Value -Value $countValue -ParsedValue ([ref]$declaredCount))) {
                    Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "content_control_repair_contract_count must be an integer."
                } elseif ($declaredCount -ne $contracts.Count) {
                    Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "content_control_repair_contract_count does not match content_control_repair_contracts length."
                }
            }
        }
    }

    $contentControlBlockers = @(Get-JsonObjectNodes -Value $Json | Where-Object {
        ([string](Get-JsonPropertyValue -Object $_ -Name "id")) -eq $contentControlBlockerId
    })

    if ($contentControlBlockers.Count -eq 0) {
        return
    }

    foreach ($blocker in $contentControlBlockers) {
        $action = [string](Get-JsonPropertyValue -Object $blocker -Name "action")
        if ($action -ne $expectedAction) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder must carry action=$expectedAction."
        }

        $sourceSchema = [string](Get-JsonPropertyValue -Object $blocker -Name "source_schema")
        if ($sourceSchema -ne "featherdoc.content_control_data_binding_governance_report.v1") {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder must carry source_schema=featherdoc.content_control_data_binding_governance_report.v1."
        }

        $sourceJsonDisplay = [string](Get-JsonPropertyValue -Object $blocker -Name "source_json_display")
        if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder must carry source_json_display."
        }

        foreach ($fieldName in @("input_docx", "template_name", "schema_target", "target_mode")) {
            $fieldValue = [string](Get-JsonPropertyValue -Object $blocker -Name $fieldName)
            if ([string]::IsNullOrWhiteSpace($fieldValue)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "content_control_data_binding.bound_placeholder must carry $fieldName provenance."
            }
        }

        $repairStrategy = [string](Get-JsonPropertyValue -Object $blocker -Name "repair_strategy")
        if ($repairStrategy -ne $expectedRepairStrategy) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder must use repair_strategy=$expectedRepairStrategy."
        }

        $repairHint = [string](Get-JsonPropertyValue -Object $blocker -Name "repair_hint")
        if ([string]::IsNullOrWhiteSpace($repairHint) -or $repairHint -notlike "*$expectedRepairHintMarker*") {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder must carry repair_hint containing $expectedRepairHintMarker."
        }

        $commandTemplate = [string](Get-JsonPropertyValue -Object $blocker -Name "command_template")
        if ([string]::IsNullOrWhiteSpace($commandTemplate) -or $commandTemplate -notlike "*$expectedCommand*") {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder command_template must include $expectedCommand."
        }

        $repairActionClassValues = @(Get-JsonPropertyValue -Object $blocker -Name "repair_action_classes")
        $repairActionClasses = @(
            foreach ($value in $repairActionClassValues) {
                if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
                    [string]$value
                }
            }
        )
        foreach ($expectedClass in $expectedRepairActionClasses) {
            if ($repairActionClasses -notcontains $expectedClass) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "content_control_data_binding.bound_placeholder must carry repair_action_classes including $expectedClass."
            }
        }
    }
}

function Add-GovernanceMetricDetailsViolations {
    param(
        [string]$File,
        [string]$Label,
        [string]$MetricName,
        [string]$PropertyName,
        $SourceMetric,
        $ManifestMetric,
        [string[]]$RequiredFields,
        $Violations
    )

    $sourceDetails = Get-JsonPropertyValue -Object $SourceMetric -Name "details"
    if ($null -eq $sourceDetails) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "Governance metric $MetricName is missing details."
        return
    }

    $manifestDetails = Get-JsonPropertyValue -Object $ManifestMetric -Name "details"
    if ($null -eq $manifestDetails) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$PropertyName is missing details."
        return
    }

    foreach ($fieldName in @($RequiredFields)) {
        $sourceValue = Get-JsonPropertyValue -Object $sourceDetails -Name $fieldName
        $manifestValue = Get-JsonPropertyValue -Object $manifestDetails -Name $fieldName

        if ($null -eq $manifestValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$PropertyName.details is missing $fieldName."
            continue
        }

        $sourceComparable = Convert-GovernanceMetricDetailValueToComparableString -Value $sourceValue
        $manifestComparable = Convert-GovernanceMetricDetailValueToComparableString -Value $manifestValue
        if ($manifestComparable -ne $sourceComparable) {
            Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$PropertyName.details.$fieldName does not match the source governance metric."
        }
    }

    $sourcePenaltySummary = @(Get-JsonArray -Object $sourceDetails -Name "penalty_summary")
    $manifestPenaltySummary = @(Get-JsonArray -Object $manifestDetails -Name "penalty_summary")
    if ($sourcePenaltySummary.Count -ne $manifestPenaltySummary.Count) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$PropertyName.details.penalty_summary does not match the source governance metric."
        return
    }

    for ($index = 0; $index -lt $sourcePenaltySummary.Count; $index++) {
        $sourcePenalty = $sourcePenaltySummary[$index]
        $manifestPenalty = $manifestPenaltySummary[$index]
        foreach ($fieldName in @("factor", "count", "penalty")) {
            $sourceValue = Get-JsonPropertyValue -Object $sourcePenalty -Name $fieldName
            $manifestValue = Get-JsonPropertyValue -Object $manifestPenalty -Name $fieldName
            if ($null -eq $manifestValue -or [string]$manifestValue -ne [string]$sourceValue) {
                Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$PropertyName.details.penalty_summary[$index].$fieldName does not match the source governance metric."
            }
        }
    }
}

function Test-ReleaseGovernanceSummaryContainerActive {
    param([AllowNull()]$Container)

    if ($null -eq $Container) {
        return $false
    }

    $requested = Get-JsonPropertyValue -Object $Container -Name "requested"
    if ([string]::Equals([string]$requested, "True", [System.StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }

    $status = [string](Get-JsonPropertyValue -Object $Container -Name "status")
    if (-not [string]::IsNullOrWhiteSpace($status) -and
        -not [string]::Equals($status, "not_requested", [System.StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }

    foreach ($countProperty in @(
        "governance_metric_count",
        "release_blocker_count",
        "action_item_count",
        "warning_count",
        "source_report_count",
        "loaded_report_count"
    )) {
        $countValue = Get-JsonPropertyValue -Object $Container -Name $countProperty
        if ($null -eq $countValue -or [string]::IsNullOrWhiteSpace([string]$countValue)) {
            continue
        }

        $parsedCount = $null
        if (Test-StrictJsonInt64Value -Value $countValue -ParsedValue ([ref]$parsedCount)) {
            if ($parsedCount -gt 0) {
                return $true
            }
        }
    }

    return $false
}

function Test-ReleaseGovernanceContractTarget {
    param(
        [string]$File,
        $Json
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -eq "release_assets_manifest.json") {
        return $true
    }

    if ($leafName -ne "summary.json") {
        return $false
    }

    $releaseVersion = Get-JsonPropertyValue -Object $Json -Name "release_version"
    $executionStatus = Get-JsonPropertyValue -Object $Json -Name "execution_status"
    if ([string]::IsNullOrWhiteSpace([string]$releaseVersion) -or
        [string]::IsNullOrWhiteSpace([string]$executionStatus)) {
        return $false
    }

    $metricsValue = Get-JsonPropertyValue -Object $Json -Name "governance_metrics"
    if ($null -ne $metricsValue -and @($metricsValue).Count -gt 0) {
        return $true
    }

    foreach ($containerName in @("release_blocker_rollup", "release_governance_handoff")) {
        if (Test-ReleaseGovernanceSummaryContainerActive -Container (Get-JsonPropertyValue -Object $Json -Name $containerName)) {
            return $true
        }
    }

    $steps = Get-JsonPropertyValue -Object $Json -Name "steps"
    foreach ($containerName in @("release_blocker_rollup", "release_governance_handoff")) {
        if (Test-ReleaseGovernanceSummaryContainerActive -Container (Get-JsonPropertyValue -Object $steps -Name $containerName)) {
            return $true
        }
    }

    return $false
}

function Get-GovernanceMetricByContract {
    param(
        $Metrics,
        [string]$Metric,
        [string]$Id,
        [string]$ReportId,
        [string]$SourceSchema
    )

    return @($Metrics | Where-Object {
        ([string](Get-JsonPropertyValue -Object $_ -Name "metric")) -eq $Metric -and
        ([string](Get-JsonPropertyValue -Object $_ -Name "id")) -eq $Id -and
        ([string](Get-JsonPropertyValue -Object $_ -Name "report_id")) -eq $ReportId -and
        ([string](Get-JsonPropertyValue -Object $_ -Name "source_schema")) -eq $SourceSchema
    }) | Select-Object -First 1
}

function Add-GovernanceMetricContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    if (-not (Test-ReleaseGovernanceContractTarget -File $File -Json $Json)) {
        return
    }

    $label = "governance metrics contract"
    $metricsValue = Get-JsonPropertyValue -Object $Json -Name "governance_metrics"
    if ($null -eq $metricsValue) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing governance_metrics."
        return
    }

    $metrics = @($metricsValue)
    if ($metrics.Count -eq 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "governance_metrics must not be empty."
        return
    }

    $countValue = Get-JsonPropertyValue -Object $Json -Name "governance_metric_count"
    if ($null -eq $countValue) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing governance_metric_count."
    } else {
        $declaredCount = $null
        if (-not (Test-StrictJsonInt64Value -Value $countValue -ParsedValue ([ref]$declaredCount))) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "governance_metric_count must be an integer."
        } elseif ($declaredCount -ne $metrics.Count) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "governance_metric_count does not match governance_metrics length."
        }
    }

    $requiredMetricContracts = @(
        [ordered]@{
            metric = "real_corpus_confidence"
            id = "numbering_catalog_governance.real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
        },
        [ordered]@{
            metric = "delivery_quality"
            id = "table_layout_delivery_governance.delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
        }
    )

    foreach ($requiredMetric in $requiredMetricContracts) {
        $metricName = [string]$requiredMetric.metric
        $metric = Get-GovernanceMetricByContract `
            -Metrics $metrics `
            -Metric $metricName `
            -Id ([string]$requiredMetric.id) `
            -ReportId ([string]$requiredMetric.report_id) `
            -SourceSchema ([string]$requiredMetric.source_schema)

        if ($null -eq $metric) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing required governance metric: $($requiredMetric.id)."
            continue
        }

        $level = Get-JsonPropertyValue -Object $metric -Name "level"
        if ([string]::IsNullOrWhiteSpace([string]$level)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Governance metric $metricName is missing level."
        }

        $score = Get-JsonPropertyValue -Object $metric -Name "score"
        if ($null -eq $score) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Governance metric $metricName is missing score."
        }
    }

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $manifestMetricMirrors = @(
        [ordered]@{
            metric = "real_corpus_confidence"
            id = "numbering_catalog_governance.real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            property = "numbering_catalog_real_corpus_confidence"
        },
        [ordered]@{
            metric = "delivery_quality"
            id = "table_layout_delivery_governance.delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
            property = "table_layout_delivery_quality"
        }
    )

    foreach ($mirror in $manifestMetricMirrors) {
        $metricName = [string]$mirror.metric
        $propertyName = [string]$mirror.property
        $sourceMetric = Get-GovernanceMetricByContract `
            -Metrics $metrics `
            -Metric $metricName `
            -Id ([string]$mirror.id) `
            -ReportId ([string]$mirror.report_id) `
            -SourceSchema ([string]$mirror.source_schema)

        if ($null -eq $sourceMetric) {
            continue
        }

        $manifestMetric = Get-JsonPropertyValue -Object $Json -Name $propertyName
        if ($null -eq $manifestMetric) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing $propertyName."
            continue
        }

        foreach ($fieldName in @("id", "metric", "report_id", "source_schema", "level", "score")) {
            $manifestValue = Get-JsonPropertyValue -Object $manifestMetric -Name $fieldName
            $metricValue = Get-JsonPropertyValue -Object $sourceMetric -Name $fieldName
            if ($null -eq $manifestValue) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "$propertyName is missing $fieldName."
                continue
            }

            if ([string]$manifestValue -ne [string]$metricValue) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "$propertyName.$fieldName does not match $metricName governance metric."
            }
        }

        foreach ($fieldName in @("source_report", "source_report_display", "source_json", "source_json_display")) {
            $manifestValue = Get-JsonPropertyValue -Object $manifestMetric -Name $fieldName
            $metricValue = Get-JsonPropertyValue -Object $sourceMetric -Name $fieldName
            if ([string]::IsNullOrWhiteSpace([string]$metricValue)) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "$metricName governance metric is missing $fieldName."
                continue
            }

            if ([string]::IsNullOrWhiteSpace([string]$manifestValue)) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "$propertyName is missing $fieldName."
                continue
            }

            if ([string]$manifestValue -ne [string]$metricValue) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "$propertyName.$fieldName does not match $metricName governance metric."
            }
        }

        if ($metricName -eq "real_corpus_confidence") {
            Add-GovernanceMetricDetailsViolations `
                -File $File `
                -Label $label `
                -MetricName $metricName `
                -PropertyName $propertyName `
                -SourceMetric $sourceMetric `
                -ManifestMetric $manifestMetric `
                -RequiredFields @(
                    "score",
                    "level",
                    "document_count",
                    "catalog_exemplar_count",
                    "baseline_entry_count",
                    "catalog_coverage_percent",
                    "baseline_coverage_percent",
                    "coverage_score",
                    "matched_document_count",
                    "unmatched_catalog_document_count",
                    "unmatched_baseline_document_count",
                    "alignment_gap_count",
                    "catalog_document_keys",
                    "baseline_document_keys",
                    "matched_document_keys"
                ) `
                -Violations $Violations
        } elseif ($metricName -eq "delivery_quality") {
            Add-GovernanceMetricDetailsViolations `
                -File $File `
                -Label $label `
                -MetricName $metricName `
                -PropertyName $propertyName `
                -SourceMetric $sourceMetric `
                -ManifestMetric $manifestMetric `
                -RequiredFields @(
                    "score",
                    "level",
                    "document_count",
                    "ready_document_count",
                    "ready_document_percent",
                    "needs_review_document_count",
                    "failed_document_count",
                    "table_style_issue_count",
                    "automatic_tblLook_fix_count",
                    "manual_table_style_fix_count",
                    "table_position_automatic_count",
                    "table_position_review_count",
                    "command_failure_count",
                    "unresolved_item_count",
                    "metadata_only_fields",
                    "review_required_fields"
                ) `
                -Violations $Violations
        }
    }
}

function Add-ProjectTemplateDeliveryReadinessContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "project template delivery readiness contract"
    $contract = Get-JsonPropertyValue -Object $Json -Name "project_template_delivery_readiness_contract"
    if ($null -eq $contract) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing project_template_delivery_readiness_contract."
        return
    }

    $schema = Get-JsonPropertyValue -Object $contract -Name "schema"
    if ([string]$schema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.schema is invalid."
    }

    $sourceSchema = Get-JsonPropertyValue -Object $contract -Name "source_schema"
    if ([string]$sourceSchema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.source_schema is invalid."
    }

    $status = Get-JsonPropertyValue -Object $contract -Name "status"
    if ([string]::IsNullOrWhiteSpace([string]$status)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.status is missing."
    }
    if (-not (Test-StringValueInSet -Value $status -AllowedValues $ProjectTemplateReadinessStatusValues)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.status must be a recognized readiness state."
    }

    $latestGateStatus = Get-JsonPropertyValue -Object $contract -Name "latest_schema_approval_gate_status"
    if ([string]::IsNullOrWhiteSpace([string]$latestGateStatus)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.latest_schema_approval_gate_status is missing."
    }

    $statusSummary = Get-JsonPropertyValue -Object $contract -Name "schema_approval_status_summary"
    if ($null -eq $statusSummary -or
        ($statusSummary -is [string] -and [string]::IsNullOrWhiteSpace($statusSummary)) -or
        @($statusSummary).Count -eq 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.schema_approval_status_summary is missing."
    }

    $onboardingNextAction = Get-JsonPropertyValue -Object $contract -Name "onboarding_governance_next_action"
    if ($null -eq $onboardingNextAction -or
        ($onboardingNextAction -is [string] -and [string]::IsNullOrWhiteSpace($onboardingNextAction)) -or
        @($onboardingNextAction).Count -eq 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.onboarding_governance_next_action is missing."
    }

    $onboardingNextActionSummary = Get-JsonPropertyValue -Object $contract -Name "onboarding_governance_next_action_summary"
    if ($null -eq $onboardingNextActionSummary -or
        ($onboardingNextActionSummary -is [string] -and [string]::IsNullOrWhiteSpace($onboardingNextActionSummary)) -or
        @($onboardingNextActionSummary).Count -eq 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.onboarding_governance_next_action_summary is missing."
    }
    $onboardingNextActionSummaryCount = @($onboardingNextActionSummary).Count

    $sourceJsonDisplay = Get-JsonPropertyValue -Object $contract -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceJsonDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.source_json_display is missing."
    }

    $sourceReportDisplay = Get-JsonPropertyValue -Object $contract -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceReportDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.source_report_display is missing."
    }

    $deliveryReadinessSourceNeedles = @("project_template_delivery_readiness", "project-template-delivery-readiness")
    Add-SourceDisplayIdentityViolations `
        -File $File `
        -Violations $Violations `
        -Label $label `
        -FieldName "project_template_delivery_readiness_contract.source_json_display" `
        -Value $sourceJsonDisplay `
        -Needles $deliveryReadinessSourceNeedles `
        -EvidenceDescription "project template delivery readiness"
    Add-SourceDisplayIdentityViolations `
        -File $File `
        -Violations $Violations `
        -Label $label `
        -FieldName "project_template_delivery_readiness_contract.source_report_display" `
        -Value $sourceReportDisplay `
        -Needles $deliveryReadinessSourceNeedles `
        -EvidenceDescription "project template delivery readiness"

    $releaseReady = Get-JsonPropertyValue -Object $contract -Name "release_ready"
    if ($null -eq $releaseReady) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.release_ready is missing."
    }
    if (-not (Test-StringValueInSet -Value $releaseReady -AllowedValues $ProjectTemplateBooleanValues)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.release_ready must be true or false."
    }

    Add-ProjectTemplateReviewerActionContractViolations `
        -File $File `
        -Contract $contract `
        -Violations $Violations `
        -Label $label `
        -ContractName "project_template_delivery_readiness_contract"

    $integerValues = @{}
    foreach ($fieldName in @(
        "schema_history_blocked_run_count",
        "schema_history_pending_run_count",
        "schema_history_passed_run_count",
        "onboarding_governance_next_action_group_count",
        "template_count",
        "ready_template_count",
        "blocked_template_count",
        "release_blocker_count",
        "action_item_count",
        "warning_count"
    )) {
        $fieldValue = Get-JsonPropertyValue -Object $contract -Name $fieldName
        if ($null -eq $fieldValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.$fieldName is missing."
            continue
        }

        $parsedInteger = $null
        if (Test-StrictJsonInt64Value -Value $fieldValue -ParsedValue ([ref]$parsedInteger)) {
            $integerValues[$fieldName] = $parsedInteger
        } else {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.$fieldName must be an integer."
        }
    }

    if ($integerValues.ContainsKey("onboarding_governance_next_action_group_count") -and
        $onboardingNextActionSummaryCount -gt 0 -and
        $integerValues["onboarding_governance_next_action_group_count"] -lt $onboardingNextActionSummaryCount) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.onboarding_governance_next_action_group_count must cover onboarding_governance_next_action_summary."
    }

    $releaseReadyIsTrue = $false
    $releaseReadyIsFalse = $false
    if ($releaseReady -is [bool]) {
        $releaseReadyIsTrue = $releaseReady
        $releaseReadyIsFalse = -not $releaseReady
    } elseif ($null -ne $releaseReady) {
        $normalizedReleaseReady = ([string]$releaseReady).Trim().ToLowerInvariant()
        $releaseReadyIsTrue = ($normalizedReleaseReady -eq "true")
        $releaseReadyIsFalse = ($normalizedReleaseReady -eq "false")
    }

    if ($releaseReadyIsTrue -and [string]$status -ne "ready") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.status must be ready when release_ready is true."
    }

    if ($releaseReadyIsFalse -and [string]$status -eq "ready") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.status must not be ready when release_ready is false."
    }

    if ($releaseReadyIsFalse -and
        $integerValues.ContainsKey("release_blocker_count") -and
        $integerValues.ContainsKey("warning_count") -and
        $integerValues["release_blocker_count"] -le 0 -and
        $integerValues["warning_count"] -le 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.release_blocker_count or warning_count must be greater than 0 when release_ready is false."
    }
}
