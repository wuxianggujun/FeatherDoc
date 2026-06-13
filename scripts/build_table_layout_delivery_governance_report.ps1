<#
.SYNOPSIS
Builds a final table layout delivery governance report.

.DESCRIPTION
Reads table layout delivery rollup summaries and writes one JSON/Markdown gate
for table style quality, safe tblLook repair readiness, floating table preset
plans, visual-regression handoff, release blockers, and action items. The script
is read-only for source artifacts.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/table-layout-delivery-governance",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnIssue,
    [switch]$FailOnBlocker
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")
. (Join-Path $PSScriptRoot "build_table_layout_delivery_governance_report_helpers.ps1")

$repoRoot = Resolve-RepoRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "table_layout_delivery_governance.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) table layout governance input JSON file(s)"

$sourceFiles = New-Object 'System.Collections.Generic.List[object]'
$documents = New-Object 'System.Collections.Generic.List[object]'
$positionPlans = New-Object 'System.Collections.Generic.List[object]'
$pdfFloatingTableSupport = New-Object 'System.Collections.Generic.List[object]'
$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$deliveryActions = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'

$rollupCount = 0
$totalIssueCount = 0
$totalAutomaticFixCount = 0
$totalManualFixCount = 0
$totalPositionAutomaticCount = 0
$totalPositionReviewCount = 0
$totalPositionAlreadyMatchingCount = 0
$totalCommandFailureCount = 0

foreach ($path in @($inputPaths)) {
    $kind = "unknown"
    $status = "skipped"
    $errorMessage = ""
    try {
        $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $kind = Get-EvidenceKind -Json $json
        switch ($kind) {
            "table_layout_delivery_rollup_report" {
                $rollupCount++
                $status = Get-JsonString -Object $json -Name "status" -DefaultValue "loaded"
                $totalIssueCount += Get-JsonInt -Object $json -Name "total_table_style_issue_count"
                $totalAutomaticFixCount += Get-JsonInt -Object $json -Name "total_automatic_tblLook_fix_count"
                $totalManualFixCount += Get-JsonInt -Object $json -Name "total_manual_table_style_fix_count"
                $totalPositionAutomaticCount += Get-JsonInt -Object $json -Name "total_table_position_automatic_count"
                $totalPositionReviewCount += Get-JsonInt -Object $json -Name "total_table_position_review_count"
                $totalPositionAlreadyMatchingCount += Get-JsonInt -Object $json -Name "total_table_position_already_matching_count"
                $totalCommandFailureCount += Get-JsonInt -Object $json -Name "total_command_failure_count"

                foreach ($document in @(Get-JsonArray -Object $json -Name "document_entries")) {
                    $documents.Add([ordered]@{
                        document_name = Get-JsonString -Object $document -Name "document_name"
                        input_docx = Get-JsonString -Object $document -Name "input_docx"
                        input_docx_display = Get-JsonString -Object $document -Name "input_docx_display"
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        status = Get-JsonString -Object $document -Name "status"
                        ready = Get-JsonBool -Object $document -Name "ready"
                        preset = Get-JsonString -Object $document -Name "preset"
                        table_style_issue_count = Get-JsonInt -Object $document -Name "table_style_issue_count"
                        automatic_tblLook_fix_count = Get-JsonInt -Object $document -Name "automatic_tblLook_fix_count"
                        manual_table_style_fix_count = Get-JsonInt -Object $document -Name "manual_table_style_fix_count"
                        table_position_automatic_count = Get-JsonInt -Object $document -Name "table_position_automatic_count"
                        table_position_review_count = Get-JsonInt -Object $document -Name "table_position_review_count"
                        table_position_already_matching_count = Get-JsonInt -Object $document -Name "table_position_already_matching_count"
                        pdf_floating_table_support_status = Get-JsonString -Object $document -Name "pdf_floating_table_support_status" -DefaultValue "not_reported"
                        pdf_floating_table_layout_boundary = Get-JsonString -Object $document -Name "pdf_floating_table_layout_boundary"
                        pdf_floating_table_supported_geometry_count = Get-JsonInt -Object $document -Name "pdf_floating_table_supported_geometry_count"
                        pdf_floating_table_metadata_only_count = Get-JsonInt -Object $document -Name "pdf_floating_table_metadata_only_count"
                        pdf_floating_table_tracked_geometry_count = Get-JsonInt -Object $document -Name "pdf_floating_table_tracked_geometry_count"
                        pdf_floating_table_supported_geometry_percent = Get-JsonInt -Object $document -Name "pdf_floating_table_supported_geometry_percent"
                        command_failure_count = Get-JsonInt -Object $document -Name "command_failure_count"
                        table_position_plan_path = Get-JsonString -Object $document -Name "table_position_plan_path"
                        table_position_plan_display = Get-JsonString -Object $document -Name "table_position_plan_display"
                    }) | Out-Null
                }
                foreach ($support in @(Get-JsonArray -Object $json -Name "pdf_floating_table_support")) {
                    $pdfFloatingTableSupport.Add([ordered]@{
                        document_name = Get-JsonString -Object $support -Name "document_name"
                        input_docx = Get-JsonString -Object $support -Name "input_docx"
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        source_json = $path
                        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        status = Get-JsonString -Object $support -Name "status" -DefaultValue "not_reported"
                        boundary = Get-JsonString -Object $support -Name "boundary"
                        supported_geometry_count = Get-JsonInt -Object $support -Name "supported_geometry_count"
                        metadata_only_count = Get-JsonInt -Object $support -Name "metadata_only_count"
                        tracked_geometry_count = Get-JsonInt -Object $support -Name "tracked_geometry_count"
                        supported_geometry_percent = Get-JsonInt -Object $support -Name "supported_geometry_percent"
                        review_required_count = Get-JsonInt -Object $support -Name "review_required_count"
                        supported_geometry = @(Get-JsonArray -Object $support -Name "supported_geometry")
                        metadata_only = @(Get-JsonArray -Object $support -Name "metadata_only")
                        review_required = @(Get-JsonArray -Object $support -Name "review_required")
                    }) | Out-Null
                }
                foreach ($plan in @(Get-JsonArray -Object $json -Name "table_position_plans")) {
                    $positionPlans.Add([ordered]@{
                        document_name = Get-JsonString -Object $plan -Name "document_name"
                        input_docx = Get-JsonString -Object $plan -Name "input_docx"
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        preset = Get-JsonString -Object $plan -Name "preset"
                        table_position_plan_path = Get-JsonString -Object $plan -Name "table_position_plan_path"
                        table_position_plan_display = Get-JsonString -Object $plan -Name "table_position_plan_display"
                        automatic_count = Get-JsonInt -Object $plan -Name "automatic_count"
                        review_count = Get-JsonInt -Object $plan -Name "review_count"
                        already_matching_count = Get-JsonInt -Object $plan -Name "already_matching_count"
                    }) | Out-Null
                }
                foreach ($blocker in @(Get-JsonArray -Object $json -Name "release_blockers")) {
                    Add-UniqueBlocker -Collection $releaseBlockers -Blocker ([ordered]@{
                        id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
                        scope = Get-FirstJsonString -Object $blocker -Names @("document_name", "scope") -DefaultValue "table_layout_delivery"
                        source_kind = "table_layout_delivery_rollup_report"
                        source_schema = "featherdoc.table_layout_delivery_rollup_report.v1"
                        source_json = $path
                        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "warning"
                        status = Get-JsonString -Object $blocker -Name "status" -DefaultValue "needs_review"
                        action = Get-JsonString -Object $blocker -Name "action"
                        message = Get-JsonString -Object $blocker -Name "message"
                        repair_strategy = Get-JsonString -Object $blocker -Name "repair_strategy"
                        repair_hint = Get-JsonString -Object $blocker -Name "repair_hint"
                        command_template = Get-JsonString -Object $blocker -Name "command_template"
                    })
                }
                foreach ($item in @(Get-JsonArray -Object $json -Name "action_items")) {
                    Add-UniqueAction -Collection $deliveryActions -Action ([ordered]@{
                        id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
                        scope = Get-FirstJsonString -Object $item -Names @("document_name", "scope") -DefaultValue "table_layout_delivery"
                        source_kind = "table_layout_delivery_rollup_report"
                        source_schema = "featherdoc.table_layout_delivery_rollup_report.v1"
                        source_json = $path
                        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        action = Get-JsonString -Object $item -Name "action"
                        title = Get-JsonString -Object $item -Name "title"
                        command = Get-JsonString -Object $item -Name "command"
                        open_command = Get-FirstJsonString -Object $item -Names @("open_command", "command", "command_template")
                        repair_strategy = Get-JsonString -Object $item -Name "repair_strategy"
                        repair_hint = Get-JsonString -Object $item -Name "repair_hint"
                        command_template = Get-JsonString -Object $item -Name "command_template"
                    })
                }
            }
            "table_layout_delivery_governance_report" {
                $status = "skipped"
            }
            default {
                $warnings.Add([ordered]@{
                    id = "source_json_schema_skipped"
                    action = "review_table_layout_delivery_governance_sources"
                    source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
                    source_json = $path
                    source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    source_report = $path
                    source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    message = "Input JSON kind '$kind' is not table layout delivery governance evidence."
                }) | Out-Null
            }
        }
    } catch {
        $status = "failed"
        $errorMessage = $_.Exception.Message
        $warnings.Add([ordered]@{
            id = "source_json_read_failed"
            action = "review_table_layout_delivery_governance_sources"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
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

if ($rollupCount -eq 0) {
    $warnings.Add([ordered]@{
        id = "table_layout_delivery_rollup_missing"
        action = "rebuild_table_layout_delivery_rollup"
        source_schema = "featherdoc.table_layout_delivery_rollup_report.v1"
        source_report = $summaryPath
        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        source_json = $summaryPath
        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        message = "No table layout delivery rollup summary was loaded."
    }) | Out-Null
}

if ($totalAutomaticFixCount -gt 0) {
    Add-UniqueBlocker -Collection $releaseBlockers -Blocker (New-ReleaseBlocker `
        -Id "table_layout_delivery.safe_tblLook_fixes_pending" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Status "needs_review" `
        -Action "apply_safe_tblLook_fixes_then_visual_regression" `
        -Message "Safe tblLook-only fixes are available and need application plus visual regression." `
        -RepairStrategy "apply_safe_tblLook_fixes" `
        -RepairHint "Apply only safe tblLook fixes, then regenerate visual evidence before release." `
        -CommandTemplate "featherdoc_cli apply-table-style-quality-fixes <input.docx> --look-only --output <reviewed.docx> --json")
    Add-UniqueAction -Collection $deliveryActions -Action (New-ActionItem `
        -Id "apply_safe_tblLook_fixes" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Action "apply_safe_tblLook_fixes_then_visual_regression" `
        -Title "Apply safe tblLook-only fixes and regenerate visual evidence" `
        -RepairStrategy "apply_safe_tblLook_fixes" `
        -RepairHint "Apply only safe tblLook fixes, then regenerate visual evidence before release." `
        -CommandTemplate "featherdoc_cli apply-table-style-quality-fixes <input.docx> --look-only --output <reviewed.docx> --json")
}
if ($totalManualFixCount -gt 0) {
    Add-UniqueBlocker -Collection $releaseBlockers -Blocker (New-ReleaseBlocker `
        -Id "table_layout_delivery.manual_table_style_work" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Status "needs_review" `
        -Action "review_manual_table_style_definition_work" `
        -Message "Some table style quality issues require manual style-definition work." `
        -RepairStrategy "review_manual_table_style_definition_work" `
        -RepairHint "Inspect the affected table style definitions and keep manual edits separate from safe tblLook-only fixes." `
        -CommandTemplate "featherdoc_cli inspect-table-style <input.docx> <style-id> --json")
}
if ($totalPositionReviewCount -gt 0) {
    Add-UniqueBlocker -Collection $releaseBlockers -Blocker (New-ReleaseBlocker `
        -Id "table_layout_delivery.floating_table_review_pending" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Status "needs_review" `
        -Action "review_floating_table_position_plans" `
        -Message "Floating table position plans include entries requiring manual review." `
        -RepairStrategy "review_table_position_plan" `
        -RepairHint "Review table-position.plan.json entries with review_count > 0 before applying presets." `
        -CommandTemplate "featherdoc_cli apply-table-position-plan <table-position.plan.json> --dry-run --json")
    Add-UniqueAction -Collection $deliveryActions -Action (New-ActionItem `
        -Id "review_floating_table_position_plans" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Action "review_floating_table_position_plans" `
        -Title "Review floating table position plans before applying presets" `
        -RepairStrategy "review_table_position_plan" `
        -RepairHint "Review table-position.plan.json entries with review_count > 0 before applying presets." `
        -CommandTemplate "featherdoc_cli apply-table-position-plan <table-position.plan.json> --dry-run --json")
}
if ($totalPositionAutomaticCount -gt 0) {
    Add-UniqueAction -Collection $deliveryActions -Action (New-ActionItem `
        -Id "dry_run_apply_table_position_plans" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Action "dry_run_apply_table_position_plans" `
        -Title "Dry-run saved floating table position plans before writing DOCX" `
        -RepairStrategy "dry_run_table_position_plan" `
        -RepairHint "Run a fingerprint-checked dry-run before writing any positioned DOCX output." `
        -CommandTemplate "featherdoc_cli apply-table-position-plan <table-position.plan.json> --dry-run --json")
}
if ($totalIssueCount -gt 0 -or $totalAutomaticFixCount -gt 0 -or $totalPositionAutomaticCount -gt 0 -or $totalPositionReviewCount -gt 0) {
    Add-UniqueAction -Collection $deliveryActions -Action (New-ActionItem `
        -Id "run_table_style_quality_visual_regression" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Action "run_table_style_quality_visual_regression" `
        -Title "Generate Word-rendered table layout visual regression evidence" `
        -Command "pwsh -ExecutionPolicy Bypass -File .\scripts\run_table_style_quality_visual_regression.ps1" `
        -RepairStrategy "generate_table_layout_visual_evidence" `
        -RepairHint "Generate before/after rendered evidence after safe fixes or floating-position plan changes." `
        -CommandTemplate "pwsh -ExecutionPolicy Bypass -File .\scripts\run_table_style_quality_visual_regression.ps1")
}

foreach ($blocker in @($releaseBlockers.ToArray())) {
    Set-GovernanceTraceMetadata -Item $blocker -RepoRoot $repoRoot -SummaryPath $summaryPath
}
foreach ($item in @($deliveryActions.ToArray())) {
    Set-GovernanceTraceMetadata -Item $item -RepoRoot $repoRoot -SummaryPath $summaryPath -EnsureOpenCommand
}
$deliveryActionItems = @($deliveryActions.ToArray())

$sourceFailureCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$readyDocumentCount = @($documents.ToArray() | Where-Object { [bool]$_.ready }).Count
$needsReviewDocumentCount = @($documents.ToArray() | Where-Object { $_.status -eq "needs_review" }).Count
$failedDocumentCount = @($documents.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$pdfFloatingTableSupportReportCount = @($pdfFloatingTableSupport.ToArray() | Where-Object {
    [string]$_.status -ne "not_reported"
}).Count
$pdfFloatingTableCapabilityStatus = if ($pdfFloatingTableSupportReportCount -gt 0) {
    $reportedSupport = @($pdfFloatingTableSupport.ToArray() | Where-Object {
        [string]$_.status -ne "not_reported"
    })
    if (@($reportedSupport | Where-Object { [string]$_.status -ne "partial" }).Count -eq 0) {
        "partial"
    } else {
        "mixed"
    }
} else {
    "not_reported"
}
$pdfFloatingTableLayoutBoundary = if ($pdfFloatingTableSupportReportCount -gt 0) {
    $boundaries = @($pdfFloatingTableSupport.ToArray() |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_.boundary) } |
        ForEach-Object { [string]$_.boundary } |
        Sort-Object -Unique)
    if ($boundaries.Count -eq 1) { $boundaries[0] } else { "mixed" }
} else {
    ""
}
$pdfFloatingTableSupportedGeometryCount = if ($pdfFloatingTableSupportReportCount -gt 0) {
    $maxCount = 0
    foreach ($support in @($pdfFloatingTableSupport.ToArray())) {
        $maxCount = [Math]::Max($maxCount, (Get-JsonInt -Object $support -Name "supported_geometry_count"))
    }
    $maxCount
} else {
    0
}
$pdfFloatingTableMetadataOnlyCount = if ($pdfFloatingTableSupportReportCount -gt 0) {
    $maxCount = 0
    foreach ($support in @($pdfFloatingTableSupport.ToArray())) {
        $maxCount = [Math]::Max($maxCount, (Get-JsonInt -Object $support -Name "metadata_only_count"))
    }
    $maxCount
} else {
    0
}
$pdfFloatingTableTrackedGeometryCount = if ($pdfFloatingTableSupportReportCount -gt 0) {
    $maxCount = 0
    foreach ($support in @($pdfFloatingTableSupport.ToArray())) {
        $trackedCount = Get-JsonInt -Object $support -Name "tracked_geometry_count"
        if ($trackedCount -le 0) {
            $trackedCount = (Get-JsonInt -Object $support -Name "supported_geometry_count") +
                (Get-JsonInt -Object $support -Name "metadata_only_count")
        }
        $maxCount = [Math]::Max($maxCount, $trackedCount)
    }
    $maxCount
} else {
    0
}
$pdfFloatingTableSupportedGeometryPercent = Get-Percent `
    -Numerator $pdfFloatingTableSupportedGeometryCount `
    -Denominator $pdfFloatingTableTrackedGeometryCount
$pdfFloatingTableMetadataOnlyFields = Get-UniqueStringValues -Values @(
    foreach ($support in @($pdfFloatingTableSupport.ToArray())) {
        if ([string]$support.status -eq "not_reported") { continue }
        Get-JsonArray -Object $support -Name "metadata_only"
    }
)
$pdfFloatingTableReviewRequiredFields = Get-UniqueStringValues -Values @(
    foreach ($support in @($pdfFloatingTableSupport.ToArray())) {
        if ([string]$support.status -eq "not_reported") { continue }
        Get-JsonArray -Object $support -Name "review_required"
    }
)
$deliveryQuality = New-DeliveryQuality `
    -DocumentCount $documents.Count `
    -ReadyDocumentCount $readyDocumentCount `
    -NeedsReviewDocumentCount $needsReviewDocumentCount `
    -FailedDocumentCount $failedDocumentCount `
    -TotalTableStyleIssueCount $totalIssueCount `
    -TotalAutomaticTblLookFixCount $totalAutomaticFixCount `
    -TotalManualTableStyleFixCount $totalManualFixCount `
    -TotalTablePositionAutomaticCount $totalPositionAutomaticCount `
    -TotalTablePositionReviewCount $totalPositionReviewCount `
    -TotalCommandFailureCount $totalCommandFailureCount `
    -PdfFloatingTableCapabilityStatus $pdfFloatingTableCapabilityStatus `
    -PdfFloatingTableLayoutBoundary $pdfFloatingTableLayoutBoundary `
    -PdfFloatingTableSupportedGeometryCount $pdfFloatingTableSupportedGeometryCount `
    -PdfFloatingTableMetadataOnlyCount $pdfFloatingTableMetadataOnlyCount `
    -PdfFloatingTableTrackedGeometryCount $pdfFloatingTableTrackedGeometryCount `
    -PdfFloatingTableSupportedGeometryPercent $pdfFloatingTableSupportedGeometryPercent `
    -PdfFloatingTableMetadataOnlyFields $pdfFloatingTableMetadataOnlyFields `
    -PdfFloatingTableReviewRequiredFields $pdfFloatingTableReviewRequiredFields
$status = if ($sourceFailureCount -gt 0 -or $failedDocumentCount -gt 0 -or $totalCommandFailureCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0 -or $warnings.Count -gt 0 -or $needsReviewDocumentCount -gt 0 -or
    $totalIssueCount -gt 0 -or $totalAutomaticFixCount -gt 0 -or $totalManualFixCount -gt 0 -or
    $totalPositionAutomaticCount -gt 0 -or $totalPositionReviewCount -gt 0) {
    "needs_review"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = "featherdoc.table_layout_delivery_governance_report.v1"
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
    table_layout_delivery_rollup_count = $rollupCount
    document_count = $documents.Count
    ready_document_count = $readyDocumentCount
    needs_review_document_count = $needsReviewDocumentCount
    failed_document_count = $failedDocumentCount
    documents = @($documents.ToArray())
    delivery_quality_score = $deliveryQuality.score
    delivery_quality_level = $deliveryQuality.level
    delivery_quality = $deliveryQuality
    preset_summary = @(Add-SummaryGroup -Items $documents.ToArray() -PropertyName "preset" -OutputName "preset")
    status_summary = @(Add-SummaryGroup -Items $documents.ToArray() -PropertyName "status" -OutputName "status")
    table_position_plan_count = $positionPlans.Count
    table_position_plans = @($positionPlans.ToArray())
    pdf_floating_table_support_report_count = $pdfFloatingTableSupportReportCount
    pdf_floating_table_supported_geometry_count = $pdfFloatingTableSupportedGeometryCount
    pdf_floating_table_metadata_only_count = $pdfFloatingTableMetadataOnlyCount
    pdf_floating_table_tracked_geometry_count = $pdfFloatingTableTrackedGeometryCount
    pdf_floating_table_supported_geometry_percent = $pdfFloatingTableSupportedGeometryPercent
    pdf_floating_table_support_coverage = $deliveryQuality.pdf_floating_table_support_coverage
    pdf_floating_table_reviewer_focus = $deliveryQuality.pdf_floating_table_reviewer_focus
    pdf_floating_table_metadata_only_fields = @($pdfFloatingTableMetadataOnlyFields)
    pdf_floating_table_review_required_fields = @($pdfFloatingTableReviewRequiredFields)
    metadata_only_fields = @($pdfFloatingTableMetadataOnlyFields)
    review_required_fields = @($pdfFloatingTableReviewRequiredFields)
    pdf_floating_table_support_summary = @(Add-PdfFloatingTableSupportSummary -Items $pdfFloatingTableSupport.ToArray())
    pdf_floating_table_support = @($pdfFloatingTableSupport.ToArray())
    total_table_style_issue_count = $totalIssueCount
    total_automatic_tblLook_fix_count = $totalAutomaticFixCount
    total_manual_table_style_fix_count = $totalManualFixCount
    total_table_position_automatic_count = $totalPositionAutomaticCount
    total_table_position_review_count = $totalPositionReviewCount
    total_table_position_already_matching_count = $totalPositionAlreadyMatchingCount
    total_command_failure_count = $totalCommandFailureCount
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    blocker_id_summary = @(Add-SummaryGroup -Items $releaseBlockers.ToArray() -PropertyName "id" -OutputName "id")
    action_item_count = $deliveryActionItems.Count
    action_items = @($deliveryActionItems)
    delivery_actions = @($deliveryActionItems)
    next_steps = @($deliveryActionItems)
    action_item_summary = @(Add-SummaryGroup -Items $deliveryActionItems -PropertyName "action" -OutputName "action")
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0 -or $failedDocumentCount -gt 0 -or $totalCommandFailureCount -gt 0) { exit 1 }
if ($FailOnIssue -and $totalIssueCount -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
