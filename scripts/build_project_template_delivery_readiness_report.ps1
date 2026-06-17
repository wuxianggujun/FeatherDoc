<#
.SYNOPSIS
Builds a project-template delivery readiness report from onboarding evidence.

.DESCRIPTION
Reads project-template onboarding governance summaries, direct onboarding
summaries, onboarding plans, project-template-smoke summaries, smoke manifest
descriptions, and schema approval history reports, then writes a single
JSON/Markdown delivery gate report. The script is read-only for source
artifacts and does not rerun smoke, CMake, Word, or visual automation.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/project-template-delivery-readiness",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnBlocker
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

$deliveryReadinessSchema = "featherdoc.project_template_delivery_readiness_report.v1"
$onboardingGovernanceSchema = "featherdoc.project_template_onboarding_governance_report.v1"
$onboardingPlanSchema = "featherdoc.project_template_smoke_onboarding_plan.v1"
$onboardingSummarySchema = "featherdoc.project_template_onboarding_summary.v1"
$projectTemplateSmokeSummarySchema = "featherdoc.project_template_smoke_summary.v1"
$deliveryReadinessOpenCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1"
. (Join-Path $PSScriptRoot "build_project_template_delivery_readiness_report_helpers.ps1")

$repoRoot = Resolve-RepoRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "project_template_delivery_readiness.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) input JSON file(s)"

$templates = New-Object 'System.Collections.Generic.List[object]'
$histories = New-Object 'System.Collections.Generic.List[object]'
$manifestDescriptions = New-Object 'System.Collections.Generic.List[object]'
$smokeSummaries = New-Object 'System.Collections.Generic.List[object]'
$sourceFiles = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'

foreach ($path in @($inputPaths)) {
    $kind = "unknown"
    $status = "ignored"
    $errorMessage = ""
    try {
        $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $kind = Get-EvidenceKind -Json $json -Path $path
        switch ($kind) {
            "onboarding_governance_report" {
                foreach ($template in @(Convert-OnboardingGovernanceToTemplates -RepoRoot $repoRoot -Path $path -Json $json)) {
                    $templates.Add($template) | Out-Null
                }
                $status = "loaded"
            }
            "onboarding_summary" {
                foreach ($template in @(Convert-OnboardingSummaryToTemplates -RepoRoot $repoRoot -Path $path -Json $json)) {
                    $templates.Add($template) | Out-Null
                }
                $status = "loaded"
            }
            "onboarding_plan" {
                foreach ($template in @(Convert-OnboardingPlanToTemplates -RepoRoot $repoRoot -Path $path -Json $json)) {
                    $templates.Add($template) | Out-Null
                }
                $status = "loaded"
            }
            "project_template_smoke_summary" {
                foreach ($template in @(Convert-ProjectTemplateSmokeSummaryToTemplates -RepoRoot $repoRoot -Path $path -Json $json)) {
                    $templates.Add($template) | Out-Null
                }
                $smokeSummaries.Add($json) | Out-Null
                $status = "loaded"
            }
            "schema_approval_history" {
                $json | Add-Member -NotePropertyName "source_json" -NotePropertyValue $path -Force
                $json | Add-Member -NotePropertyName "source_json_display" -NotePropertyValue (Get-DisplayPath -RepoRoot $repoRoot -Path $path) -Force
                $histories.Add($json) | Out-Null
                $status = "loaded"
            }
            "project_template_smoke_manifest_description" {
                $json | Add-Member -NotePropertyName "source_json" -NotePropertyValue $path -Force
                $json | Add-Member -NotePropertyName "source_json_display" -NotePropertyValue (Get-DisplayPath -RepoRoot $repoRoot -Path $path) -Force
                $manifestDescriptions.Add($json) | Out-Null
                $status = "loaded"
            }
            "project_template_delivery_readiness_report" {
                $status = "skipped"
            }
            default {
                $status = "skipped"
                $warnings.Add([ordered]@{
                    id = "source_json_schema_skipped"
                    action = "review_project_template_delivery_readiness_evidence"
                    source_schema = $deliveryReadinessSchema
                    source_json = $path
                    source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    source_report = $path
                    source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    message = "Input JSON kind '$kind' is not project-template readiness evidence."
                }) | Out-Null
            }
        }
    } catch {
        $status = "failed"
        $errorMessage = $_.Exception.Message
        $warnings.Add([ordered]@{
            id = "source_json_read_failed"
            action = "review_project_template_delivery_readiness_evidence"
            source_schema = $deliveryReadinessSchema
            source_json = $path
            source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            source_report = $path
            source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            message = $errorMessage
        }) | Out-Null
    }

    $sourceFiles.Add([ordered]@{
        path = $path
        path_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
        kind = $kind
        status = $status
        error = $errorMessage
    }) | Out-Null
}

if ($templates.Count -eq 0) {
    if ($manifestDescriptions.Count -gt 0) {
        $warnings.Add((New-ProjectTemplateSmokeSummaryMissingWarning `
                    -RepoRoot $repoRoot `
                    -SummaryPath $summaryPath `
                    -ManifestDescriptions $manifestDescriptions.ToArray())) | Out-Null
    } else {
        $warnings.Add([ordered]@{
            id = "template_evidence_missing"
            action = "collect_project_template_onboarding_governance_evidence"
            repair_strategy = "collect_project_template_onboarding_governance_evidence"
            repair_hint = "Run or restore project-template onboarding governance evidence for the release templates, including schema approval history where applicable; do not treat an empty feeder summary as template evidence."
            command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_project_template_onboarding_governance_report.ps1 -InputRoot .\output\project-template-smoke -OutputDir .\output\project-template-onboarding-governance"
            source_schema = $deliveryReadinessSchema
            source_json = $summaryPath
            source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
            source_report = $summaryPath
            source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
            message = "No onboarding template evidence was loaded."
        }) | Out-Null
    }
}

Add-HistoryToTemplates -Templates $templates.ToArray() -Histories $histories.ToArray() -Warnings $warnings

$latestHistory = Get-LatestHistory -Histories $histories.ToArray()
$latestGateStatus = if ($null -ne $latestHistory) {
    Get-JsonString -Object $latestHistory -Name "latest_gate_status" -DefaultValue "not_available"
} else {
    "not_available"
}
$historyBlockedRunCount = if ($null -ne $latestHistory) { Get-JsonInt -Object $latestHistory -Name "blocked_run_count" } else { 0 }
$historyPendingRunCount = if ($null -ne $latestHistory) { Get-JsonInt -Object $latestHistory -Name "pending_run_count" } else { 0 }
$historyPassedRunCount = if ($null -ne $latestHistory) { Get-JsonInt -Object $latestHistory -Name "passed_run_count" } else { 0 }
$latestHistorySourceJson = if ($null -ne $latestHistory) { Get-JsonString -Object $latestHistory -Name "source_json" } else { "" }
$latestHistorySourceJsonDisplay = if ($null -ne $latestHistory) { Get-JsonString -Object $latestHistory -Name "source_json_display" } else { "" }

$globalReleaseBlockers = New-Object 'System.Collections.Generic.List[object]'
if ($latestGateStatus -in @("blocked", "pending")) {
    $globalReleaseBlockers.Add((New-ReleaseBlocker `
        -Id "project_template_delivery_readiness.schema_approval_history_gate" `
        -Source "schema_approval_history" `
        -Status $latestGateStatus `
        -Action "review_schema_approval_history" `
        -Message "Latest schema approval history gate is not release-ready." `
        -SourceJson $latestHistorySourceJson `
        -SourceJsonDisplay $latestHistorySourceJsonDisplay `
        -SourceReport $latestHistorySourceJson `
        -SourceReportDisplay $latestHistorySourceJsonDisplay)) | Out-Null
}

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'
$manualReviewRecommendations = New-Object 'System.Collections.Generic.List[object]'

foreach ($blocker in @($globalReleaseBlockers.ToArray())) {
    $releaseBlockers.Add([ordered]@{
        scope = "global"
        template_name = ""
        source_kind = Get-JsonString -Object $blocker -Name "source" -DefaultValue "project_template_delivery_readiness_report"
        source_schema = Get-JsonString -Object $blocker -Name "source_schema" -DefaultValue $deliveryReadinessSchema
        source_json = Get-JsonString -Object $blocker -Name "source_json" -DefaultValue $summaryPath
        source_json_display = Get-JsonString -Object $blocker -Name "source_json_display" -DefaultValue (Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath)
        source_report = Get-JsonString -Object $blocker -Name "source_report" -DefaultValue $summaryPath
        source_report_display = Get-JsonString -Object $blocker -Name "source_report_display" -DefaultValue (Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath)
            id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
            blocker_id = Get-JsonString -Object $blocker -Name "blocker_id" -DefaultValue (Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker")
            severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "error"
            status = Get-JsonString -Object $blocker -Name "status"
            action = Get-JsonString -Object $blocker -Name "action"
            message = Get-JsonString -Object $blocker -Name "message"
            reason = Get-JsonString -Object $blocker -Name "reason" -DefaultValue (Get-JsonString -Object $blocker -Name "message")
            requires_reviewer_action = Get-JsonBool -Object $blocker -Name "requires_reviewer_action"
            reviewer_action_summary = Get-JsonString -Object $blocker -Name "reviewer_action_summary"
            reviewer_action_reason = Get-JsonString -Object $blocker -Name "reviewer_action_reason"
            reviewer_actions = @(Get-JsonArray -Object $blocker -Name "reviewer_actions")
        }) | Out-Null
}

foreach ($template in @($templates.ToArray())) {
    foreach ($blocker in @($template.release_blockers)) {
        $releaseBlockers.Add([ordered]@{
            scope = [string]$template.template_name
            template_name = [string]$template.template_name
            source_kind = Get-JsonString -Object $blocker -Name "source" -DefaultValue ([string]$template.source_kind)
            source_schema = Get-JsonString -Object $blocker -Name "source_schema" -DefaultValue (Get-DefaultSourceSchema -SourceKind ([string]$template.source_kind))
            source_json = Get-JsonString -Object $blocker -Name "source_json" -DefaultValue ([string]$template.source_json)
            source_json_display = Get-JsonString -Object $blocker -Name "source_json_display" -DefaultValue ([string]$template.source_json_display)
            source_report = Get-JsonString -Object $blocker -Name "source_report" -DefaultValue ([string]$template.source_report)
            source_report_display = Get-JsonString -Object $blocker -Name "source_report_display" -DefaultValue ([string]$template.source_report_display)
            id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
            blocker_id = Get-JsonString -Object $blocker -Name "blocker_id" -DefaultValue (Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker")
            severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "error"
            status = Get-JsonString -Object $blocker -Name "status" -DefaultValue ([string]$template.schema_approval_status)
            action = Get-JsonString -Object $blocker -Name "action" -DefaultValue ([string]$template.schema_approval_action)
            message = Get-JsonString -Object $blocker -Name "message" -DefaultValue "Release blocker requires review."
            reason = Get-JsonString -Object $blocker -Name "reason" -DefaultValue (Get-JsonString -Object $blocker -Name "message" -DefaultValue "Release blocker requires review.")
            requires_reviewer_action = Get-JsonBool -Object $blocker -Name "requires_reviewer_action"
            reviewer_action_summary = Get-JsonString -Object $blocker -Name "reviewer_action_summary"
            reviewer_action_reason = Get-JsonString -Object $blocker -Name "reviewer_action_reason"
            reviewer_actions = @(Get-JsonArray -Object $blocker -Name "reviewer_actions")
            onboarding_governance_status = Get-JsonString -Object $blocker -Name "onboarding_governance_status" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_status")
            onboarding_governance_release_ready = Get-JsonString -Object $blocker -Name "onboarding_governance_release_ready" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_release_ready")
            onboarding_governance_schema_approval_status_summary = @(Get-JsonArray -Object $blocker -Name "onboarding_governance_schema_approval_status_summary")
            onboarding_governance_next_action = if ($null -ne (Get-JsonProperty -Object $blocker -Name "onboarding_governance_next_action")) { Get-JsonProperty -Object $blocker -Name "onboarding_governance_next_action" } else { Get-JsonProperty -Object $template -Name "onboarding_governance_next_action" }
            onboarding_governance_next_action_summary = @(Get-JsonArray -Object $blocker -Name "onboarding_governance_next_action_summary")
            onboarding_governance_next_action_group_count = Get-JsonInt -Object $blocker -Name "onboarding_governance_next_action_group_count" -DefaultValue (Get-JsonInt -Object $template -Name "onboarding_governance_next_action_group_count")
            onboarding_governance_source_report = Get-JsonString -Object $blocker -Name "onboarding_governance_source_report" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_source_report")
            onboarding_governance_source_report_display = Get-JsonString -Object $blocker -Name "onboarding_governance_source_report_display" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_source_report_display")
            onboarding_governance_source_json = Get-JsonString -Object $blocker -Name "onboarding_governance_source_json" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_source_json")
            onboarding_governance_source_json_display = Get-JsonString -Object $blocker -Name "onboarding_governance_source_json_display" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_source_json_display")
        }) | Out-Null
        $lastBlocker = $releaseBlockers[$releaseBlockers.Count - 1]
        if (@($lastBlocker.onboarding_governance_schema_approval_status_summary).Count -eq 0) {
            $lastBlocker["onboarding_governance_schema_approval_status_summary"] = @(Get-JsonArray -Object $template -Name "onboarding_governance_schema_approval_status_summary")
        }
        if (@($lastBlocker.onboarding_governance_next_action_summary).Count -eq 0) {
            $lastBlocker["onboarding_governance_next_action_summary"] = @(Get-JsonArray -Object $template -Name "onboarding_governance_next_action_summary")
            $lastBlocker["onboarding_governance_next_action_group_count"] = @($lastBlocker.onboarding_governance_next_action_summary).Count
        }
    }
    foreach ($item in @($template.action_items)) {
        $actionItems.Add([ordered]@{
            template_name = [string]$template.template_name
            source_kind = Get-JsonString -Object $item -Name "source" -DefaultValue ([string]$template.source_kind)
            source_schema = Get-JsonString -Object $item -Name "source_schema" -DefaultValue (Get-DefaultSourceSchema -SourceKind ([string]$template.source_kind))
            source_json = Get-JsonString -Object $item -Name "source_json" -DefaultValue ([string]$template.source_json)
            source_json_display = Get-JsonString -Object $item -Name "source_json_display" -DefaultValue ([string]$template.source_json_display)
            source_report = Get-JsonString -Object $item -Name "source_report" -DefaultValue ([string]$template.source_report)
            source_report_display = Get-JsonString -Object $item -Name "source_report_display" -DefaultValue ([string]$template.source_report_display)
            id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
            blocker_id = Get-JsonString -Object $item -Name "blocker_id" -DefaultValue (Get-JsonString -Object $item -Name "id" -DefaultValue "action_item")
            action = Get-JsonString -Object $item -Name "action"
            title = Get-JsonString -Object $item -Name "title"
            reason = Get-JsonString -Object $item -Name "reason" -DefaultValue (Get-JsonString -Object $item -Name "title")
            command = Get-JsonString -Object $item -Name "command"
            open_command = Get-JsonString -Object $item -Name "open_command" -DefaultValue (Get-JsonString -Object $item -Name "command" -DefaultValue $deliveryReadinessOpenCommand)
            requires_reviewer_action = Get-JsonBool -Object $item -Name "requires_reviewer_action"
            reviewer_action_summary = Get-JsonString -Object $item -Name "reviewer_action_summary"
            reviewer_action_reason = Get-JsonString -Object $item -Name "reviewer_action_reason"
            reviewer_actions = @(Get-JsonArray -Object $item -Name "reviewer_actions")
            onboarding_governance_status = Get-JsonString -Object $item -Name "onboarding_governance_status" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_status")
            onboarding_governance_release_ready = Get-JsonString -Object $item -Name "onboarding_governance_release_ready" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_release_ready")
            onboarding_governance_schema_approval_status_summary = @(Get-JsonArray -Object $item -Name "onboarding_governance_schema_approval_status_summary")
            onboarding_governance_next_action = if ($null -ne (Get-JsonProperty -Object $item -Name "onboarding_governance_next_action")) { Get-JsonProperty -Object $item -Name "onboarding_governance_next_action" } else { Get-JsonProperty -Object $template -Name "onboarding_governance_next_action" }
            onboarding_governance_next_action_summary = @(Get-JsonArray -Object $item -Name "onboarding_governance_next_action_summary")
            onboarding_governance_next_action_group_count = Get-JsonInt -Object $item -Name "onboarding_governance_next_action_group_count" -DefaultValue (Get-JsonInt -Object $template -Name "onboarding_governance_next_action_group_count")
            onboarding_governance_source_report = Get-JsonString -Object $item -Name "onboarding_governance_source_report" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_source_report")
            onboarding_governance_source_report_display = Get-JsonString -Object $item -Name "onboarding_governance_source_report_display" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_source_report_display")
            onboarding_governance_source_json = Get-JsonString -Object $item -Name "onboarding_governance_source_json" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_source_json")
            onboarding_governance_source_json_display = Get-JsonString -Object $item -Name "onboarding_governance_source_json_display" -DefaultValue (Get-JsonString -Object $template -Name "onboarding_governance_source_json_display")
        }) | Out-Null
        $lastActionItem = $actionItems[$actionItems.Count - 1]
        if (@($lastActionItem.onboarding_governance_schema_approval_status_summary).Count -eq 0) {
            $lastActionItem["onboarding_governance_schema_approval_status_summary"] = @(Get-JsonArray -Object $template -Name "onboarding_governance_schema_approval_status_summary")
        }
        if (@($lastActionItem.onboarding_governance_next_action_summary).Count -eq 0) {
            $lastActionItem["onboarding_governance_next_action_summary"] = @(Get-JsonArray -Object $template -Name "onboarding_governance_next_action_summary")
            $lastActionItem["onboarding_governance_next_action_group_count"] = @($lastActionItem.onboarding_governance_next_action_summary).Count
        }
    }
    foreach ($recommendation in @($template.manual_review_recommendations)) {
        $manualReviewRecommendations.Add([ordered]@{
            template_name = [string]$template.template_name
            source_kind = [string]$template.source_kind
            id = Get-JsonString -Object $recommendation -Name "id" -DefaultValue "manual_review"
            priority = Get-JsonString -Object $recommendation -Name "priority"
            action = Get-JsonString -Object $recommendation -Name "action"
            title = Get-JsonString -Object $recommendation -Name "title"
            artifact = Get-JsonString -Object $recommendation -Name "artifact"
        }) | Out-Null
    }
}

$sourceFailureCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$blockedTemplateCount = @($templates.ToArray() | Where-Object { [bool]$_.release_blocked }).Count
$readyTemplateCount = @($templates.ToArray() | Where-Object { -not [bool]$_.release_blocked }).Count
$onboardingGovernanceNextAction = Get-FirstJsonObjectValue -Items $templates.ToArray() -PropertyName "onboarding_governance_next_action"
$onboardingGovernanceNextActionSummary = @(Get-OnboardingGovernanceNextActionSummary -Templates $templates.ToArray())
$onboardingGovernanceNextActionGroupCount = $onboardingGovernanceNextActionSummary.Count
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0 -or $blockedTemplateCount -gt 0) {
    "blocked"
} elseif ($warnings.Count -gt 0) {
    "needs_review"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = $deliveryReadinessSchema
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    source_file_count = $sourceFiles.Count
    source_failure_count = $sourceFailureCount
    source_files = @($sourceFiles.ToArray())
    smoke_summary_count = $smokeSummaries.Count
    manifest_description_count = $manifestDescriptions.Count
    schema_history_count = $histories.Count
    latest_schema_approval_gate_status = $latestGateStatus
    schema_history_blocked_run_count = $historyBlockedRunCount
    schema_history_pending_run_count = $historyPendingRunCount
    schema_history_passed_run_count = $historyPassedRunCount
    template_count = $templates.Count
    ready_template_count = $readyTemplateCount
    blocked_template_count = $blockedTemplateCount
    templates = @($templates.ToArray())
    template_status_summary = @(Add-SummaryGroup -Items $templates.ToArray() -PropertyName "onboarding_status" -OutputName "status")
    schema_approval_status_summary = @(Add-SummaryGroup -Items $templates.ToArray() -PropertyName "schema_approval_status" -OutputName "status")
    onboarding_governance_next_action = $onboardingGovernanceNextAction
    onboarding_governance_next_action_summary = @($onboardingGovernanceNextActionSummary)
    onboarding_governance_next_action_group_count = $onboardingGovernanceNextActionGroupCount
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    blocker_id_summary = @(Add-SummaryGroup -Items $releaseBlockers.ToArray() -PropertyName "id" -OutputName "id")
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    action_item_summary = @(Add-SummaryGroup -Items $actionItems.ToArray() -PropertyName "action" -OutputName "action")
    manual_review_recommendation_count = $manualReviewRecommendations.Count
    manual_review_recommendations = @($manualReviewRecommendations.ToArray())
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
