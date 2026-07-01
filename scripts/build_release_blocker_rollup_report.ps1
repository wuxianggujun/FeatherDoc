<#
.SYNOPSIS
Builds a release blocker rollup from existing report summaries.

.DESCRIPTION
Aggregates release_blockers and action_items from multiple JSON reports into a
single JSON/Markdown handoff. The script normalizes heterogeneous report
schemas and does not rerun CLI, CMake, Word, or visual automation.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/release-blocker-rollup",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnBlocker,
    [switch]$FailOnWarning
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_core.ps1")
. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_source_normalization.ps1")
. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_governance_metrics.ps1")
. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_input_discovery.ps1")
. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_pdf_evidence.ps1")
. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_release_entry_evidence.ps1")
. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_markdown_helpers.ps1")
. (Join-Path $PSScriptRoot "build_release_blocker_rollup_report_report_markdown.ps1")

$repoRoot = Resolve-RepoRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "release_blocker_rollup.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) report summary file(s)"

$sourceReports = New-Object 'System.Collections.Generic.List[object]'
$blockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'
$informationalActionItems = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'
$governanceMetrics = New-Object 'System.Collections.Generic.List[object]'
$sourceIndex = 0

foreach ($path in @($inputPaths)) {
    $sourceIndex++
    $kind = "unknown"
    $status = "loaded"
    $errorMessage = ""
    $payloadPreview = ""
    $sourceMetrics = @()
    $releaseReady = $false
    $sourceReportStatus = ""
    $sourceReportReleaseReady = ""
    $latestSchemaApprovalGateStatus = ""
    $schemaApprovalStatusSummary = @()
    $businessDocumentTypeSummary = @()
    $corpusRoleSummary = @()
    $summaryObject = $null
    try {
        $summaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $kind = Get-ReportKind -Summary $summaryObject
        $sourceReportStatus = Get-JsonString -Object $summaryObject -Name "status"
        $releaseReady = Get-JsonBool -Object $summaryObject -Name "release_ready"
        if ($null -ne (Get-JsonProperty -Object $summaryObject -Name "release_ready")) {
            $sourceReportReleaseReady = [string]$releaseReady
        }
        $latestSchemaApprovalGateStatus = Get-JsonString -Object $summaryObject -Name "latest_schema_approval_gate_status"
        $schemaApprovalStatusSummary = @(Get-JsonArray -Object $summaryObject -Name "schema_approval_status_summary")
        $businessDocumentTypeSummary = @(Get-JsonArray -Object $summaryObject -Name "business_document_type_summary")
        $corpusRoleSummary = @(Get-JsonArray -Object $summaryObject -Name "corpus_role_summary")
        $sourceMetrics = @(New-GovernanceMetrics `
            -Summary $summaryObject `
            -SourceSchema $kind `
            -SourceReport $path `
            -SourceReportDisplay (Get-DisplayPath -RepoRoot $repoRoot -Path $path))
        foreach ($metric in @($sourceMetrics)) {
            $governanceMetrics.Add($metric) | Out-Null
        }
        $sourceBlockers = @(Get-JsonArray -Object $summaryObject -Name "release_blockers")
        $declaredBlockerCount = Get-JsonProperty -Object $summaryObject -Name "release_blocker_count"
        if ($null -ne $declaredBlockerCount -and [int]$declaredBlockerCount -ne $sourceBlockers.Count) {
            $warnings.Add([ordered]@{
                id = "release_blocker_count_mismatch"
                project_id = ""
                template_name = ""
                candidate_type = ""
                action = "review_release_blocker_rollup_metadata"
                source_report = $path
                source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                source_json = $path
                source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                source_schema = $kind
                message = "release_blocker_count is $declaredBlockerCount but release_blockers contains $($sourceBlockers.Count) item(s)."
            }) | Out-Null
        }

        $sourceActions = @(Get-JsonArray -Object $summaryObject -Name "action_items")
        if ($sourceActions.Count -eq 0) {
            $sourceActions = @(Get-JsonArray -Object $summaryObject -Name "next_steps")
        }
        $sourceInformationalActions = @(Get-JsonArray -Object $summaryObject -Name "informational_action_items")
        $sourceWarnings = @(Get-JsonArray -Object $summaryObject -Name "warnings")

        $blockerIndex = 0
        foreach ($blocker in $sourceBlockers) {
            $blockerIndex++
            $id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
            $sourceJson = Get-JsonString -Object $blocker -Name "source_json"
            $sourceJsonDisplay = Get-JsonString -Object $blocker -Name "source_json_display"
            $originSourceReport = Get-JsonString -Object $blocker -Name "source_report"
            $originSourceReportDisplay = Get-JsonString -Object $blocker -Name "source_report_display"
            $rollupBlocker = [ordered]@{
                composite_id = ("source{0}.blocker{1}.{2}" -f $sourceIndex, $blockerIndex, $id)
                id = $id
                project_id = Get-JsonString -Object $blocker -Name "project_id"
                template_name = Get-JsonString -Object $blocker -Name "template_name"
                candidate_type = Get-JsonString -Object $blocker -Name "candidate_type"
                source = Get-JsonString -Object $blocker -Name "source" -DefaultValue $kind
                source_report = $path
                source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                origin_source_report = $originSourceReport
                origin_source_report_display = $originSourceReportDisplay
                source_json = if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
                    $sourceJson
                } elseif (-not [string]::IsNullOrWhiteSpace($originSourceReport)) {
                    $originSourceReport
                } else {
                    $path
                }
                source_json_display = if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                    $sourceJsonDisplay
                } elseif (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
                    Get-DisplayPath -RepoRoot $repoRoot -Path $sourceJson
                } elseif (-not [string]::IsNullOrWhiteSpace($originSourceReportDisplay)) {
                    $originSourceReportDisplay
                } elseif (-not [string]::IsNullOrWhiteSpace($originSourceReport)) {
                    Get-DisplayPath -RepoRoot $repoRoot -Path $originSourceReport
                } else {
                    Get-DisplayPath -RepoRoot $repoRoot -Path $path
                }
                source_schema = Get-FirstJsonString -Object $blocker -Names @("source_schema") -DefaultValue $kind
                severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "error"
                status = Get-JsonString -Object $blocker -Name "status"
                action = Get-JsonString -Object $blocker -Name "action"
                message = Get-JsonString -Object $blocker -Name "message"
                input_docx = Get-JsonString -Object $blocker -Name "input_docx"
                input_docx_display = Get-JsonString -Object $blocker -Name "input_docx_display"
                schema_target = Get-JsonString -Object $blocker -Name "schema_target"
                target_mode = Get-JsonString -Object $blocker -Name "target_mode"
                repair_strategy = Get-JsonString -Object $blocker -Name "repair_strategy"
                repair_hint = Get-JsonString -Object $blocker -Name "repair_hint"
                command_template = Get-JsonString -Object $blocker -Name "command_template"
                repair_action_classes = @(Get-JsonArray -Object $blocker -Name "repair_action_classes")
            }
            if ([string]::Equals($kind, "featherdoc.project_template_delivery_readiness_report.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
                if (-not [string]::IsNullOrWhiteSpace($sourceReportStatus)) {
                    $rollupBlocker["readiness_status"] = $sourceReportStatus
                }
                if (-not [string]::IsNullOrWhiteSpace($sourceReportReleaseReady)) {
                    $rollupBlocker["readiness_release_ready"] = $sourceReportReleaseReady
                }
            }
            Copy-OptionalJsonProperties `
                -Target $rollupBlocker `
                -Source $blocker `
                -Names @(
                    "readiness_status",
                    "readiness_release_ready",
                    "schema_approval_status_summary",
                    "onboarding_governance_status",
                    "onboarding_governance_release_ready",
                    "onboarding_governance_schema_approval_status_summary",
                    "onboarding_governance_source_report",
                    "onboarding_governance_source_report_display",
                    "onboarding_governance_source_json",
                    "onboarding_governance_source_json_display",
                    "blocking_summary",
                    "output_gap_count",
                    "missing_output_count",
                    "output_gap_summary",
                    "build_dir_auto_candidates",
                    "pdf_dependency_inputs",
                    "readiness_action_evidence_count",
                    "readiness_action_evidence",
                    "requires_reviewer_action",
                    "reviewer_action_summary",
                    "reviewer_action_reason",
                    "reviewer_actions",
                    "business_document_type",
                    "source_business_document_type",
                    "corpus_role",
                    "source_corpus_role",
                    "business_document_type_mismatch",
                    "corpus_role_mismatch",
                    "missing_business_document_type_count",
                    "missing_corpus_role_count",
                    "mismatched_corpus_metadata_count",
                    "mismatched_business_document_type_count",
                    "mismatched_corpus_role_count",
                    "candidate_name",
                    "schema_update_candidate",
                    "matched_document_count",
                    "unmatched_catalog_document_count",
                    "unmatched_baseline_document_count",
                    "alignment_gap_count",
                    "catalog_coverage_percent",
                    "baseline_coverage_percent",
                    "coverage_score",
                    "catalog_document_keys",
                    "baseline_document_keys",
                    "matched_document_keys"
                )
            Normalize-ReadinessActionEvidence `
                -Items (Get-JsonArray -Object $rollupBlocker -Name "readiness_action_evidence") `
                -DefaultSourceSchema ([string]$rollupBlocker.source_schema) `
                -DefaultSourceReport ([string]$rollupBlocker.origin_source_report) `
                -DefaultSourceReportDisplay ([string]$rollupBlocker.origin_source_report_display) `
                -DefaultSourceJson ([string]$rollupBlocker.source_json) `
                -DefaultSourceJsonDisplay ([string]$rollupBlocker.source_json_display) `
                -RepoRoot $repoRoot
            $blockers.Add($rollupBlocker) | Out-Null
        }

        $actionIndex = 0
        $sourceActionsToNormalize = @(
            foreach ($sourceAction in @($sourceActions)) {
                [pscustomobject]@{ Item = $sourceAction; ForceInformational = $false }
            }
            foreach ($sourceAction in @($sourceInformationalActions)) {
                [pscustomobject]@{ Item = $sourceAction; ForceInformational = $true }
            }
        )
        foreach ($sourceAction in $sourceActionsToNormalize) {
            $actionIndex++
            $item = $sourceAction.Item
            $id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
            $sourceJson = Get-JsonString -Object $item -Name "source_json"
            $sourceJsonDisplay = Get-JsonString -Object $item -Name "source_json_display"
            $originSourceReport = Get-JsonString -Object $item -Name "source_report"
            $originSourceReportDisplay = Get-JsonString -Object $item -Name "source_report_display"
            $rollupSourceReportDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            $openCommand = Get-FirstJsonString -Object $item -Names @("open_command", "command")
            if ([string]::IsNullOrWhiteSpace($openCommand)) {
                $openCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_release_blocker_rollup_report.ps1 -InputJson `"$rollupSourceReportDisplay`" -OutputDir .\output\release-blocker-rollup"
            }
            $rollupActionItem = [ordered]@{
                composite_id = ("source{0}.action{1}.{2}" -f $sourceIndex, $actionIndex, $id)
                id = $id
                project_id = Get-JsonString -Object $item -Name "project_id"
                template_name = Get-JsonString -Object $item -Name "template_name"
                candidate_type = Get-JsonString -Object $item -Name "candidate_type"
                source_report = $path
                source_report_display = $rollupSourceReportDisplay
                origin_source_report = $originSourceReport
                origin_source_report_display = $originSourceReportDisplay
                source_json = if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
                    $sourceJson
                } elseif (-not [string]::IsNullOrWhiteSpace($originSourceReport)) {
                    $originSourceReport
                } else {
                    $path
                }
                source_json_display = if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                    $sourceJsonDisplay
                } elseif (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
                    Get-DisplayPath -RepoRoot $repoRoot -Path $sourceJson
                } elseif (-not [string]::IsNullOrWhiteSpace($originSourceReportDisplay)) {
                    $originSourceReportDisplay
                } elseif (-not [string]::IsNullOrWhiteSpace($originSourceReport)) {
                    Get-DisplayPath -RepoRoot $repoRoot -Path $originSourceReport
                } else {
                    Get-DisplayPath -RepoRoot $repoRoot -Path $path
                }
                source_schema = Get-FirstJsonString -Object $item -Names @("source_schema") -DefaultValue $kind
                action = Get-FirstJsonString -Object $item -Names @("action", "id") -DefaultValue "action_item"
                title = Get-JsonString -Object $item -Name "title"
                command = Get-JsonString -Object $item -Name "command"
                open_command = $openCommand
                category = Get-JsonString -Object $item -Name "category"
                severity = Get-JsonString -Object $item -Name "severity"
                release_blocking = Get-JsonBool -Object $item -Name "release_blocking" -DefaultValue $true
                optional = Get-JsonBool -Object $item -Name "optional"
                input_docx = Get-JsonString -Object $item -Name "input_docx"
                input_docx_display = Get-JsonString -Object $item -Name "input_docx_display"
                schema_target = Get-JsonString -Object $item -Name "schema_target"
                target_mode = Get-JsonString -Object $item -Name "target_mode"
                repair_strategy = Get-JsonString -Object $item -Name "repair_strategy"
                repair_hint = Get-JsonString -Object $item -Name "repair_hint"
                command_template = Get-JsonString -Object $item -Name "command_template"
                repair_action_classes = @(Get-JsonArray -Object $item -Name "repair_action_classes")
            }
            if ([string]::Equals($kind, "featherdoc.project_template_delivery_readiness_report.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
                if (-not [string]::IsNullOrWhiteSpace($sourceReportStatus)) {
                    $rollupActionItem["readiness_status"] = $sourceReportStatus
                }
                if (-not [string]::IsNullOrWhiteSpace($sourceReportReleaseReady)) {
                    $rollupActionItem["readiness_release_ready"] = $sourceReportReleaseReady
                }
            }
            Copy-OptionalJsonProperties `
                -Target $rollupActionItem `
                -Source $item `
                -Names @(
                    "readiness_status",
                    "readiness_release_ready",
                    "schema_approval_status_summary",
                    "onboarding_governance_status",
                    "onboarding_governance_release_ready",
                    "onboarding_governance_schema_approval_status_summary",
                    "onboarding_governance_source_report",
                    "onboarding_governance_source_report_display",
                    "onboarding_governance_source_json",
                    "onboarding_governance_source_json_display",
                    "blocking_summary",
                    "output_gap_count",
                    "missing_output_count",
                    "output_gap_summary",
                    "build_dir_auto_candidates",
                    "pdf_dependency_inputs",
                    "readiness_action_evidence_count",
                    "readiness_action_evidence",
                    "requires_reviewer_action",
                    "reviewer_action_summary",
                    "reviewer_action_reason",
                    "reviewer_actions",
                    "business_document_type",
                    "source_business_document_type",
                    "corpus_role",
                    "source_corpus_role",
                    "business_document_type_mismatch",
                    "corpus_role_mismatch",
                    "missing_business_document_type_count",
                    "missing_corpus_role_count",
                    "mismatched_corpus_metadata_count",
                    "mismatched_business_document_type_count",
                    "mismatched_corpus_role_count",
                    "candidate_name",
                    "schema_update_candidate",
                    "style_merge_suggestion_count",
                    "style_merge_suggestion_pending_count",
                    "style_merge_manual_review_required",
                    "style_merge_manual_review_reason_count",
                    "manual_review_required",
                    "manual_review_reason_count",
                    "manual_review_reasons"
                )
            Normalize-ReadinessActionEvidence `
                -Items (Get-JsonArray -Object $rollupActionItem -Name "readiness_action_evidence") `
                -DefaultSourceSchema ([string]$rollupActionItem.source_schema) `
                -DefaultSourceReport ([string]$rollupActionItem.origin_source_report) `
                -DefaultSourceReportDisplay ([string]$rollupActionItem.origin_source_report_display) `
                -DefaultSourceJson ([string]$rollupActionItem.source_json) `
                -DefaultSourceJsonDisplay ([string]$rollupActionItem.source_json_display) `
                -RepoRoot $repoRoot
            if ([bool]$sourceAction.ForceInformational -or (Test-InformationalActionItem -Item $rollupActionItem)) {
                $informationalActionItems.Add((Copy-ActionItemWithReleaseChecklistDefaults -Item $rollupActionItem)) | Out-Null
            } else {
                $actionItems.Add($rollupActionItem) | Out-Null
            }
        }

        $warningIndex = 0
        foreach ($warning in $sourceWarnings) {
            $warningIndex++
            $id = Get-JsonString -Object $warning -Name "id" -DefaultValue "warning"
            $sourceJson = Get-JsonString -Object $warning -Name "source_json"
            $sourceJsonDisplay = Get-JsonString -Object $warning -Name "source_json_display"
            $originSourceReport = Get-JsonString -Object $warning -Name "source_report"
            $originSourceReportDisplay = Get-JsonString -Object $warning -Name "source_report_display"
            $rollupWarning = [ordered]@{
                composite_id = ("source{0}.warning{1}.{2}" -f $sourceIndex, $warningIndex, $id)
                id = $id
                project_id = Get-JsonString -Object $warning -Name "project_id"
                template_name = Get-JsonString -Object $warning -Name "template_name"
                candidate_type = Get-JsonString -Object $warning -Name "candidate_type"
                action = Get-JsonString -Object $warning -Name "action" -DefaultValue "review_release_governance_warning"
                repair_strategy = Get-JsonString -Object $warning -Name "repair_strategy"
                repair_hint = Get-JsonString -Object $warning -Name "repair_hint"
                command_template = Get-JsonString -Object $warning -Name "command_template"
                repair_action_classes = @(Get-JsonArray -Object $warning -Name "repair_action_classes")
                source_report = $path
                source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                origin_source_report = $originSourceReport
                origin_source_report_display = $originSourceReportDisplay
                source_json = if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
                    $sourceJson
                } elseif (-not [string]::IsNullOrWhiteSpace($originSourceReport)) {
                    $originSourceReport
                } else {
                    $path
                }
                source_json_display = if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                    $sourceJsonDisplay
                } elseif (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
                    Get-DisplayPath -RepoRoot $repoRoot -Path $sourceJson
                } elseif (-not [string]::IsNullOrWhiteSpace($originSourceReportDisplay)) {
                    $originSourceReportDisplay
                } elseif (-not [string]::IsNullOrWhiteSpace($originSourceReport)) {
                    Get-DisplayPath -RepoRoot $repoRoot -Path $originSourceReport
                } else {
                    Get-DisplayPath -RepoRoot $repoRoot -Path $path
                }
                source_schema = Get-FirstJsonString -Object $warning -Names @("source_schema") -DefaultValue $kind
                input_docx = Get-JsonString -Object $warning -Name "input_docx"
                input_docx_display = Get-JsonString -Object $warning -Name "input_docx_display"
                schema_target = Get-JsonString -Object $warning -Name "schema_target"
                target_mode = Get-JsonString -Object $warning -Name "target_mode"
                message = Get-JsonString -Object $warning -Name "message"
            }
            Copy-OptionalJsonProperties `
                -Target $rollupWarning `
                -Source $warning `
                -Names @(
                    "business_document_type",
                    "source_business_document_type",
                    "corpus_role",
                    "source_corpus_role",
                    "business_document_type_mismatch",
                    "corpus_role_mismatch",
                    "missing_business_document_type_count",
                    "missing_corpus_role_count",
                    "mismatched_corpus_metadata_count",
                    "mismatched_business_document_type_count",
                    "mismatched_corpus_role_count",
                    "candidate_name",
                    "schema_update_candidate",
                    "style_merge_suggestion_count",
                    "style_merge_suggestion_pending_count",
                    "style_merge_manual_review_required",
                    "style_merge_manual_review_reason_count",
                    "manual_review_required",
                    "manual_review_reason_count",
                    "manual_review_reasons"
                )
            $warnings.Add($rollupWarning) | Out-Null
        }
    } catch {
        $status = "failed"
        $errorMessage = $_.Exception.Message
        $payloadPreview = Get-SourcePayloadPreview -Path $path
        $warnings.Add([ordered]@{
            id = "source_report_read_failed"
            project_id = ""
            template_name = ""
            candidate_type = ""
            action = "review_release_blocker_rollup_metadata"
            source_report = $path
            source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            source_json = $path
            source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            source_schema = $kind
            message = $errorMessage
        }) | Out-Null
    }

    $sourceReport = [ordered]@{
        path = $path
        path_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
        schema = $kind
        status = $status
        release_ready = $releaseReady
        error = $errorMessage
        payload_preview = $payloadPreview
        latest_schema_approval_gate_status = $latestSchemaApprovalGateStatus
        schema_approval_status_summary = @($schemaApprovalStatusSummary)
        business_document_type_summary = @($businessDocumentTypeSummary)
        corpus_role_summary = @($corpusRoleSummary)
        governance_metric_count = @($sourceMetrics).Count
        governance_metrics = @($sourceMetrics)
    }
    Copy-OptionalJsonProperties `
        -Target $sourceReport `
        -Source $summaryObject `
        -Names @(
            "preflight_ready",
            "full_visual_gate_required",
            "full_visual_gate_status",
            "controlled_visual_smoke_available",
            "controlled_visual_smoke_status",
            "controlled_visual_smoke_passed",
            "controlled_visual_smoke_case_count",
            "controlled_visual_smoke_json",
            "controlled_visual_smoke_json_display"
        )
    Add-PdfVisualGateEvidenceFields -Target $sourceReport -Summary $summaryObject -RepoRoot $repoRoot
    Add-DocxFunctionalSmokeReadinessEvidenceFields -Target $sourceReport -Summary $summaryObject
    Add-PdfBoundedCtestEvidenceFields -Target $sourceReport -Summary $summaryObject
    Add-PdfFullCtestReadinessEvidenceFields -Target $sourceReport -Summary $summaryObject -RepoRoot $repoRoot
    Add-PdfVisualGateAttemptEvidenceFields -Target $sourceReport -Summary $summaryObject -RepoRoot $repoRoot
    Add-PdfVisualSegmentedGateEvidenceFields -Target $sourceReport -Summary $summaryObject -RepoRoot $repoRoot
    Add-ManifestSignoffEntrypointsEvidenceFields -Target $sourceReport -Summary $summaryObject -RepoRoot $repoRoot
    Add-ProjectTemplateReadinessChecklistEntrypointsEvidenceFields -Target $sourceReport -Summary $summaryObject
    Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceFields -Target $sourceReport -Summary $summaryObject
    Add-WordVisualStandardReviewMetadataEvidenceFields -Target $sourceReport -Summary $summaryObject
    $sourceReports.Add($sourceReport) | Out-Null
}

$sourceFailureCount = @($sourceReports.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($blockers.Count -gt 0) {
    "blocked"
} elseif ($warnings.Count -gt 0) {
    "ready_with_warnings"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = "featherdoc.release_blocker_rollup_report.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -in @("ready", "ready_with_warnings"))
    output_dir = $resolvedOutputDir
    summary_json = $summaryPath
    report_markdown = $markdownPath
    source_report_count = $sourceReports.Count
    source_failure_count = $sourceFailureCount
    source_reports = @($sourceReports.ToArray())
    governance_metric_count = $governanceMetrics.Count
    governance_metrics = @($governanceMetrics.ToArray())
    release_blocker_count = $blockers.Count
    release_blockers = @($blockers.ToArray())
    blocker_id_summary = @(Add-SummaryGroup -Items $blockers.ToArray() -PropertyName "id" -OutputName "id")
    blocker_action_summary = @(Add-SummaryGroup -Items $blockers.ToArray() -PropertyName "action" -OutputName "action")
    blocker_source_schema_summary = @(Add-SummaryGroup -Items $blockers.ToArray() -PropertyName "source_schema" -OutputName "source_schema")
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    action_item_summary = @(Add-SummaryGroup -Items $actionItems.ToArray() -PropertyName "action" -OutputName "action")
    action_item_source_schema_summary = @(Add-SummaryGroup -Items $actionItems.ToArray() -PropertyName "source_schema" -OutputName "source_schema")
    informational_action_item_count = $informationalActionItems.Count
    informational_action_items = @($informationalActionItems.ToArray())
    informational_action_item_summary = @(Add-SummaryGroup -Items $informationalActionItems.ToArray() -PropertyName "action" -OutputName "action")
    informational_action_item_source_schema_summary = @(Add-SummaryGroup -Items $informationalActionItems.ToArray() -PropertyName "source_schema" -OutputName "source_schema")
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
    warning_source_schema_summary = @(Add-SummaryGroup -Items $warnings.ToArray() -PropertyName "source_schema" -OutputName "source_schema")
}

Write-ReleaseMaterialFiles -Summary $summary -SummaryPath $summaryPath -MarkdownPath $markdownPath -JsonDepth 24

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnWarning -and $warnings.Count -gt 0) { exit 1 }
if ($FailOnBlocker -and $blockers.Count -gt 0) { exit 1 }
