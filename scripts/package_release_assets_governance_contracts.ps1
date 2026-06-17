function Resolve-GateRoot {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    $summaryJson = Get-OptionalPropertyValue -Object $visualGate -Name "summary_json"
    if (-not [string]::IsNullOrWhiteSpace($summaryJson)) {
        $resolvedSummaryJson = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $summaryJson
        if (Test-Path -LiteralPath $resolvedSummaryJson) {
            return Split-Path -Parent (Split-Path -Parent $resolvedSummaryJson)
        }
    }

    $gateOutputDir = Get-OptionalPropertyValue -Object $Summary -Name "gate_output_dir"
    if (-not [string]::IsNullOrWhiteSpace($gateOutputDir)) {
        return Resolve-FullPath -RepoRoot $RepoRoot -InputPath $gateOutputDir
    }

    return ""
}

function Get-OptionalArrayProperty {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    $value = Get-OptionalPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }
    if ($value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($value)) {
            return @()
        }

        return @($value)
    }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Resolve-PdfVisualGateSummaryJson {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $summaryJson = Get-PdfVisualGateSummaryPath -Summary $Summary
    if ([string]::IsNullOrWhiteSpace($summaryJson)) {
        return ""
    }

    return Resolve-FullPath -RepoRoot $RepoRoot -InputPath $summaryJson
}

function Resolve-PdfVisualGateRoot {
    param(
        [string]$SummaryJson
    )

    if ([string]::IsNullOrWhiteSpace($SummaryJson) -or -not (Test-Path -LiteralPath $SummaryJson)) {
        return ""
    }

    return Split-Path -Parent (Split-Path -Parent $SummaryJson)
}

function Get-GalleryFileNames {
    return @(
        "visual-smoke-contact-sheet.png",
        "visual-smoke-page-05.png",
        "visual-smoke-page-06.png",
        "reopened-fixed-layout-column-widths-page-01.png",
        "fixed-grid-merge-right-page-01.png",
        "fixed-grid-merge-down-page-01.png",
        "fixed-grid-aggregate-contact-sheet.png",
        "sample-chinese-template-page-01.png"
    )
}

function Resolve-GallerySourceFile {
    param(
        [string]$ReadmeAssetsDir,
        [string]$InstallPrefix,
        [string]$FileName
    )

    $readmeCandidate = Join-Path $ReadmeAssetsDir $FileName
    if (Test-Path -LiteralPath $readmeCandidate) {
        return $readmeCandidate
    }

    $installCandidate = Join-Path $InstallPrefix ("share\FeatherDoc\visual-validation\{0}" -f $FileName)
    if (Test-Path -LiteralPath $installCandidate) {
        return $installCandidate
    }

    throw "Could not resolve gallery asset '$FileName' from README assets or install prefix."
}

function Get-VisualVerdict {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $summaryVerdict = Get-OptionalPropertyValue -Object $Summary -Name "visual_verdict"
    if (-not [string]::IsNullOrWhiteSpace($summaryVerdict)) {
        return $summaryVerdict
    }

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    $stepVerdict = Get-OptionalPropertyValue -Object $visualGate -Name "visual_verdict"
    if (-not [string]::IsNullOrWhiteSpace($stepVerdict)) {
        return $stepVerdict
    }

    $gateSummaryPath = Get-OptionalPropertyValue -Object $visualGate -Name "summary_json"
    if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath)) {
        $resolvedGateSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $gateSummaryPath
        if (Test-Path -LiteralPath $resolvedGateSummaryPath) {
            $gateSummary = Get-Content -Raw -LiteralPath $resolvedGateSummaryPath | ConvertFrom-Json
            return Get-OptionalPropertyValue -Object $gateSummary -Name "visual_verdict"
        }
    }

    return ""
}

function Get-VisualGateStatus {
    param($Summary)

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    return Get-OptionalPropertyValue -Object $visualGate -Name "status"
}

function Get-WordVisualStandardReviewMetadata {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    if ($null -eq $visualGate) {
        return @()
    }

    $visualGateStatus = Get-OptionalPropertyValue -Object $visualGate -Name "status"
    if ($visualGateStatus -eq "skipped") {
        return @()
    }

    $gateSummary = [pscustomobject]@{}
    $gateSummaryPath = Get-OptionalPropertyValue -Object $visualGate -Name "summary_json"
    if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath)) {
        $resolvedGateSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $gateSummaryPath
        if (Test-Path -LiteralPath $resolvedGateSummaryPath) {
            try {
                $gateSummary = Get-Content -Raw -LiteralPath $resolvedGateSummaryPath | ConvertFrom-Json
            } catch {
                $gateSummary = [pscustomobject]@{}
            }
        }
    }

    $tasks = @(
        [ordered]@{ key = "smoke"; label = "Word visual smoke" },
        [ordered]@{ key = "fixed_grid"; label = "Fixed-grid merge/unmerge" },
        [ordered]@{ key = "section_page_setup"; label = "Section page setup" },
        [ordered]@{ key = "page_number_fields"; label = "Page number fields" }
    )

    $metadata = New-Object 'System.Collections.Generic.List[object]'
    foreach ($task in $tasks) {
        $taskKey = [string]$task.key
        [void]$metadata.Add([ordered]@{
            task_key = $taskKey
            review_task_key = Get-VisualTaskReviewTaskKey -TaskKey $taskKey
            label = [string]$task.label
            verdict = Get-VisualTaskVerdict -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
            review_status = Get-VisualTaskReviewStatus -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
            reviewed_at = Get-VisualTaskReviewedAt -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
            review_method = Get-VisualTaskReviewMethod -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
            review_result_path = Get-VisualTaskReviewResultPath -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
            final_review_path = Get-VisualTaskFinalReviewPath -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
        })
    }

    return @($metadata.ToArray())
}

function Get-GovernanceMetrics {
    param($Summary)

    $metrics = Get-OptionalPropertyObject -Object $Summary -Name "governance_metrics"
    if ($null -eq $metrics) {
        return @()
    }

    return @($metrics)
}

function Get-GovernanceMetricByContract {
    param(
        $GovernanceMetrics,
        [string]$Metric,
        [string]$Id,
        [string]$ReportId,
        [string]$SourceSchema
    )

    return @($GovernanceMetrics | Where-Object {
        (Get-OptionalPropertyValue -Object $_ -Name "metric") -eq $Metric -and
        (Get-OptionalPropertyValue -Object $_ -Name "id") -eq $Id -and
        (Get-OptionalPropertyValue -Object $_ -Name "report_id") -eq $ReportId -and
        (Get-OptionalPropertyValue -Object $_ -Name "source_schema") -eq $SourceSchema
    }) | Select-Object -First 1
}

function Get-TableLayoutDeliveryQuality {
    param($GovernanceMetrics)

    $deliveryMetric = Get-GovernanceMetricByContract `
        -GovernanceMetrics $GovernanceMetrics `
        -Metric "delivery_quality" `
        -Id "table_layout_delivery_governance.delivery_quality" `
        -ReportId "table_layout_delivery_governance" `
        -SourceSchema "featherdoc.table_layout_delivery_governance_report.v1"

    if ($null -eq $deliveryMetric) {
        return [ordered]@{}
    }

    return [ordered]@{
        id = Get-OptionalPropertyValue -Object $deliveryMetric -Name "id"
        metric = Get-OptionalPropertyValue -Object $deliveryMetric -Name "metric"
        report_id = Get-OptionalPropertyValue -Object $deliveryMetric -Name "report_id"
        source_schema = Get-OptionalPropertyValue -Object $deliveryMetric -Name "source_schema"
        source_report = Get-OptionalPropertyValue -Object $deliveryMetric -Name "source_report"
        source_report_display = Get-OptionalPropertyValue -Object $deliveryMetric -Name "source_report_display"
        source_json = Get-OptionalPropertyValue -Object $deliveryMetric -Name "source_json"
        source_json_display = Get-OptionalPropertyValue -Object $deliveryMetric -Name "source_json_display"
        score = Get-OptionalPropertyObject -Object $deliveryMetric -Name "score"
        level = Get-OptionalPropertyValue -Object $deliveryMetric -Name "level"
        details = Get-OptionalPropertyObject -Object $deliveryMetric -Name "details"
    }
}

function Get-NumberingCatalogRealCorpusConfidence {
    param($GovernanceMetrics)

    $confidenceMetric = Get-GovernanceMetricByContract `
        -GovernanceMetrics $GovernanceMetrics `
        -Metric "real_corpus_confidence" `
        -Id "numbering_catalog_governance.real_corpus_confidence" `
        -ReportId "numbering_catalog_governance" `
        -SourceSchema "featherdoc.numbering_catalog_governance_report.v1"

    if ($null -eq $confidenceMetric) {
        return [ordered]@{}
    }

    return [ordered]@{
        id = Get-OptionalPropertyValue -Object $confidenceMetric -Name "id"
        metric = Get-OptionalPropertyValue -Object $confidenceMetric -Name "metric"
        report_id = Get-OptionalPropertyValue -Object $confidenceMetric -Name "report_id"
        source_schema = Get-OptionalPropertyValue -Object $confidenceMetric -Name "source_schema"
        source_report = Get-OptionalPropertyValue -Object $confidenceMetric -Name "source_report"
        source_report_display = Get-OptionalPropertyValue -Object $confidenceMetric -Name "source_report_display"
        source_json = Get-OptionalPropertyValue -Object $confidenceMetric -Name "source_json"
        source_json_display = Get-OptionalPropertyValue -Object $confidenceMetric -Name "source_json_display"
        score = Get-OptionalPropertyObject -Object $confidenceMetric -Name "score"
        level = Get-OptionalPropertyValue -Object $confidenceMetric -Name "level"
        details = Get-OptionalPropertyObject -Object $confidenceMetric -Name "details"
    }
}

function Get-ContentControlRepairContracts {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $contentControlSummaryPath = Get-OptionalPropertyValue -Object $Summary -Name "content_control_data_binding_governance"
    if ([string]::IsNullOrWhiteSpace($contentControlSummaryPath)) {
        return @()
    }

    $resolvedContentControlSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $contentControlSummaryPath
    Assert-PathExists -Path $resolvedContentControlSummaryPath -Label "content-control data-binding governance summary"

    $contentControlSummary = Get-Content -Raw -LiteralPath $resolvedContentControlSummaryPath | ConvertFrom-Json
    $schema = Get-OptionalPropertyValue -Object $contentControlSummary -Name "schema"
    $releaseBlockers = Get-OptionalPropertyObject -Object $contentControlSummary -Name "release_blockers"
    if ($null -eq $releaseBlockers) {
        return @()
    }

    $contracts = [System.Collections.Generic.List[object]]::new()
    foreach ($blocker in @($releaseBlockers)) {
        $blockerId = Get-OptionalPropertyValue -Object $blocker -Name "id"
        if ($blockerId -ne "content_control_data_binding.bound_placeholder") {
            continue
        }

        [void]$contracts.Add([ordered]@{
            id = $blockerId
            action = Get-OptionalPropertyValue -Object $blocker -Name "action"
            source_schema = $schema
            source_json_display = Convert-EvidencePathToPublicDisplay -Value $resolvedContentControlSummaryPath -RepoRoot $RepoRoot
            input_docx = Get-OptionalPropertyValue -Object $blocker -Name "input_docx"
            input_docx_display = Get-OptionalPropertyValue -Object $blocker -Name "input_docx_display"
            template_name = Get-OptionalPropertyValue -Object $blocker -Name "template_name"
            schema_target = Get-OptionalPropertyValue -Object $blocker -Name "schema_target"
            target_mode = Get-OptionalPropertyValue -Object $blocker -Name "target_mode"
            repair_strategy = Get-OptionalPropertyValue -Object $blocker -Name "repair_strategy"
            repair_hint = Get-OptionalPropertyValue -Object $blocker -Name "repair_hint"
            command_template = Get-OptionalPropertyValue -Object $blocker -Name "command_template"
            repair_action_classes = @(Get-OptionalPropertyObject -Object $blocker -Name "repair_action_classes")
        })
    }

    return @($contracts.ToArray())
}

function Get-ProjectTemplateDeliveryReadinessContract {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $readinessSummaryPath = Get-OptionalPropertyValue -Object $Summary -Name "project_template_delivery_readiness"
    if ([string]::IsNullOrWhiteSpace($readinessSummaryPath)) {
        return [ordered]@{}
    }

    $resolvedReadinessSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $readinessSummaryPath
    Assert-PathExists -Path $resolvedReadinessSummaryPath -Label "project template delivery readiness summary"

    $readinessSummary = Get-Content -Raw -LiteralPath $resolvedReadinessSummaryPath | ConvertFrom-Json
    $readinessSummaryDisplay = Convert-EvidencePathToPublicDisplay -Value $resolvedReadinessSummaryPath -RepoRoot $RepoRoot
    return [ordered]@{
        schema = Get-OptionalPropertyValue -Object $readinessSummary -Name "schema"
        source_schema = Get-OptionalPropertyValue -Object $readinessSummary -Name "schema"
        status = Get-OptionalPropertyValue -Object $readinessSummary -Name "status"
        release_ready = Get-OptionalPropertyObject -Object $readinessSummary -Name "release_ready"
        latest_schema_approval_gate_status = Get-OptionalPropertyValue -Object $readinessSummary -Name "latest_schema_approval_gate_status"
        schema_approval_status_summary = Get-OptionalPropertyObject -Object $readinessSummary -Name "schema_approval_status_summary"
        onboarding_governance_next_action = Get-OptionalPropertyObject -Object $readinessSummary -Name "onboarding_governance_next_action"
        onboarding_governance_next_action_summary = Get-OptionalPropertyObject -Object $readinessSummary -Name "onboarding_governance_next_action_summary"
        onboarding_governance_next_action_group_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "onboarding_governance_next_action_group_count"
        requires_reviewer_action = Get-OptionalPropertyObject -Object $readinessSummary -Name "requires_reviewer_action"
        reviewer_action_summary = Get-OptionalPropertyValue -Object $readinessSummary -Name "reviewer_action_summary"
        reviewer_action_reason = Get-OptionalPropertyValue -Object $readinessSummary -Name "reviewer_action_reason"
        reviewer_actions = @(Get-OptionalArrayProperty -Object $readinessSummary -Name "reviewer_actions")
        schema_history_blocked_run_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "schema_history_blocked_run_count"
        schema_history_pending_run_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "schema_history_pending_run_count"
        schema_history_passed_run_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "schema_history_passed_run_count"
        template_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "template_count"
        ready_template_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "ready_template_count"
        blocked_template_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "blocked_template_count"
        release_blocker_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "release_blocker_count"
        action_item_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "action_item_count"
        warning_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "warning_count"
        source_report_display = $readinessSummaryDisplay
        source_json_display = $readinessSummaryDisplay
    }
}

function Get-ProjectTemplateOnboardingGovernanceContract {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $onboardingSummaryPath = Get-OptionalPropertyValue -Object $Summary -Name "project_template_onboarding_governance"
    if ([string]::IsNullOrWhiteSpace($onboardingSummaryPath)) {
        return [ordered]@{}
    }

    $resolvedOnboardingSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $onboardingSummaryPath
    Assert-PathExists -Path $resolvedOnboardingSummaryPath -Label "project template onboarding governance summary"

    $onboardingSummary = Get-Content -Raw -LiteralPath $resolvedOnboardingSummaryPath | ConvertFrom-Json
    $onboardingSummaryDisplay = Convert-EvidencePathToPublicDisplay -Value $resolvedOnboardingSummaryPath -RepoRoot $RepoRoot
    return [ordered]@{
        schema = Get-OptionalPropertyValue -Object $onboardingSummary -Name "schema"
        source_schema = Get-OptionalPropertyValue -Object $onboardingSummary -Name "schema"
        status = Get-OptionalPropertyValue -Object $onboardingSummary -Name "status"
        release_ready = Get-OptionalPropertyObject -Object $onboardingSummary -Name "release_ready"
        source_file_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "source_file_count"
        source_failure_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "source_failure_count"
        entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "entry_count"
        schema_approval_status_summary = Get-OptionalPropertyObject -Object $onboardingSummary -Name "schema_approval_status_summary"
        next_action = Get-OptionalPropertyObject -Object $onboardingSummary -Name "next_action"
        next_action_summary = Get-OptionalPropertyObject -Object $onboardingSummary -Name "next_action_summary"
        next_action_group_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "next_action_group_count"
        requires_reviewer_action = Get-OptionalPropertyObject -Object $onboardingSummary -Name "requires_reviewer_action"
        reviewer_action_summary = Get-OptionalPropertyValue -Object $onboardingSummary -Name "reviewer_action_summary"
        reviewer_action_reason = Get-OptionalPropertyValue -Object $onboardingSummary -Name "reviewer_action_reason"
        reviewer_actions = @(Get-OptionalArrayProperty -Object $onboardingSummary -Name "reviewer_actions")
        blocked_entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "blocked_entry_count"
        pending_review_entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "pending_review_entry_count"
        not_evaluated_entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "not_evaluated_entry_count"
        approved_entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "approved_entry_count"
        not_required_entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "not_required_entry_count"
        release_blocker_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "release_blocker_count"
        action_item_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "action_item_count"
        manual_review_recommendation_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "manual_review_recommendation_count"
        source_report_display = $onboardingSummaryDisplay
        source_json_display = $onboardingSummaryDisplay
    }
}
