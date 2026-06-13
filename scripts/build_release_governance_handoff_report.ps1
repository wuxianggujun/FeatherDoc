<#
.SYNOPSIS
Builds a release governance handoff from the default delivery reports.

.DESCRIPTION
Reads the final numbering catalog governance, table layout delivery governance,
content-control data-binding governance, and project-template delivery readiness
summaries, then writes one read-only JSON/Markdown handoff for release reviewers.
The script does not rerun CLI, CMake, Word, or visual automation.
#>
param(
    [string]$InputRoot = "output",
    [string[]]$InputJson = @(),
    [string]$OutputDir = "output/release-governance-handoff",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [ValidateSet("full", "explicit-only")]
    [string]$ExpectedReportProfile = "full",
    [switch]$IncludeReleaseBlockerRollup,
    [switch]$FailOnMissing,
    [switch]$FailOnBlocker,
    [switch]$FailOnWarning
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_core.ps1")
. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_rollup_evidence.ps1")
. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_governance_metrics.ps1")
. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_input_contracts.ps1")
. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_normalization.ps1")
. (Join-Path $PSScriptRoot "build_release_governance_handoff_report_report_markdown.ps1")

$repoRoot = Resolve-RepoRoot
$resolvedInputRoot = Resolve-RepoPath -RepoRoot $repoRoot -Path $InputRoot -AllowMissing
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "release_governance_handoff.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))
$releaseBlockerRollupOutputDir = Join-Path $resolvedOutputDir "release-blocker-rollup"
$releaseBlockerRollupSummaryPath = Join-Path $releaseBlockerRollupOutputDir "summary.json"
$releaseBlockerRollupMarkdownPath = Join-Path $releaseBlockerRollupOutputDir "release_blocker_rollup.md"

$defaultExpectedReports = @(
    New-ExpectedReport `
        -Id "numbering_catalog_governance" `
        -Title "Numbering Catalog Governance" `
        -RelativeSummary "numbering-catalog-governance/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\build_numbering_catalog_governance_report.ps1 -InputJson .\output\document-skeleton-governance-rollup\summary.json,.\output\numbering-catalog-manifest-checks\summary.json -OutputDir .\output\numbering-catalog-governance"
    New-ExpectedReport `
        -Id "table_layout_delivery_governance" `
        -Title "Table Layout Delivery Governance" `
        -RelativeSummary "table-layout-delivery-governance/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1 -InputJson .\output\table-layout-delivery-rollup\summary.json -OutputDir .\output\table-layout-delivery-governance"
    New-ExpectedReport `
        -Id "content_control_data_binding_governance" `
        -Title "Content Control Data Binding Governance" `
        -RelativeSummary "content-control-data-binding-governance/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1 -InputJson .\output\content-control-data-binding\inspect-content-controls.json,.\output\content-control-data-binding\sync-content-controls-from-custom-xml.json -OutputDir .\output\content-control-data-binding-governance"
    New-ExpectedReport `
        -Id "project_template_delivery_readiness" `
        -Title "Project Template Delivery Readiness" `
        -RelativeSummary "project-template-delivery-readiness/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1 -InputJson .\output\project-template-onboarding-governance\summary.json,.\output\project-template-schema-approval-history\history.json -OutputDir .\output\project-template-delivery-readiness"
    New-ExpectedReport `
        -Id "schema_patch_confidence_calibration" `
        -Title "Schema Patch Confidence Calibration" `
        -RelativeSummary "schema-patch-confidence-calibration/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1 -InputJson .\output\project-template-smoke\summary.json,.\output\project-template-schema-approval-history\history.json -OutputDir .\output\schema-patch-confidence-calibration"
    New-ExpectedReport `
        -Id "docx_functional_smoke_readiness" `
        -Title "DOCX Functional Smoke Readiness" `
        -RelativeSummary "docx-functional-smoke-readiness/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\check_docx_functional_smoke_readiness.ps1 -OutputDir .\output\docx-functional-smoke-readiness"
)
$expectedReports = @(
    if ($ExpectedReportProfile -ne "explicit-only") {
        $defaultExpectedReports
    }
)

$reports = New-Object 'System.Collections.Generic.List[object]'
$reportById = @{}
$knownExpectedReportById = @{}
foreach ($expected in @($defaultExpectedReports)) {
    $knownExpectedReportById[[string]$expected.id] = $expected
}
$explicitWarningReports = New-Object 'System.Collections.Generic.List[object]'
foreach ($expected in @($expectedReports)) {
    $path = Join-Path $resolvedInputRoot ([string]$expected.relative_summary)
    if (-not (Test-Path -LiteralPath $path)) {
        $entry = New-ReportEntry `
            -RepoRoot $repoRoot `
            -Id ([string]$expected.id) `
            -Title ([string]$expected.title) `
            -ExpectedSummaryPath $path `
            -BuildCommand ([string]$expected.build_command) `
            -Status "missing"
        $reports.Add($entry) | Out-Null
        $reportById[[string]$expected.id] = $entry
        continue
    }

    try {
        $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $entry = New-ReportEntry `
            -RepoRoot $repoRoot `
            -Id ([string]$expected.id) `
            -Title ([string]$expected.title) `
            -ExpectedSummaryPath $path `
            -BuildCommand ([string]$expected.build_command) `
            -Source "default" `
            -Json $json `
            -Status "loaded"
        $reports.Add($entry) | Out-Null
        $reportById[[string]$expected.id] = $entry
    } catch {
        $entry = New-ReportEntry `
            -RepoRoot $repoRoot `
            -Id ([string]$expected.id) `
            -Title ([string]$expected.title) `
            -ExpectedSummaryPath $path `
            -BuildCommand ([string]$expected.build_command) `
            -Status "failed" `
            -ErrorMessage $_.Exception.Message
        $reports.Add($entry) | Out-Null
        $reportById[[string]$expected.id] = $entry
    }
}

foreach ($input in @(Expand-InputPathList -Paths $InputJson)) {
    $path = Resolve-RepoPath -RepoRoot $repoRoot -Path $input -AllowMissing
    if (-not (Test-Path -LiteralPath $path)) { continue }
    try {
        $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $kind = Get-ReportKind -Summary $json
        if (-not $knownExpectedReportById.ContainsKey($kind)) {
            if ([string]::Equals($kind, "featherdoc.release_candidate_summary", [System.StringComparison]::OrdinalIgnoreCase)) {
                $explicitWarningReports.Add((New-ExplicitReleaseCandidateWarningReport `
                            -RepoRoot $repoRoot `
                            -Path $path `
                            -Json $json `
                            -Index ($explicitWarningReports.Count + 1))) | Out-Null
            }
            continue
        }
        $expected = $knownExpectedReportById[$kind]
        $entry = New-ReportEntry `
            -RepoRoot $repoRoot `
            -Id ([string]$expected.id) `
            -Title ([string]$expected.title) `
            -ExpectedSummaryPath $path `
            -BuildCommand ([string]$expected.build_command) `
            -Source "explicit" `
            -Json $json `
            -Status "loaded"
        $replacedDefaultReport = $false
        for ($index = 0; $index -lt $reports.Count; $index++) {
            if ([string]$reports[$index].id -eq [string]$expected.id) {
                $reports[$index] = $entry
                $replacedDefaultReport = $true
                break
            }
        }
        if (-not $replacedDefaultReport) {
            $reports.Add($entry) | Out-Null
        }
        $reportById[[string]$expected.id] = $entry
    } catch {
        Write-Step "Skipping explicit report '$path': $($_.Exception.Message)"
    }
}

$projectTemplateOnboardingGovernanceContract = Get-ProjectTemplateOnboardingGovernanceContract `
    -RepoRoot $repoRoot `
    -InputRoot $resolvedInputRoot `
    -InputJson $InputJson

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'
$informationalActionItems = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'
foreach ($report in @($reports.ToArray())) {
    Add-NormalizedBlockers `
        -Collection $releaseBlockers `
        -Report $report `
        -ProjectTemplateOnboardingGovernanceContract $projectTemplateOnboardingGovernanceContract
    Add-NormalizedActions `
        -Collection $actionItems `
        -InformationalCollection $informationalActionItems `
        -Report $report `
        -ProjectTemplateOnboardingGovernanceContract $projectTemplateOnboardingGovernanceContract
    Add-NormalizedActions `
        -Collection $actionItems `
        -InformationalCollection $informationalActionItems `
        -Report $report `
        -ProjectTemplateOnboardingGovernanceContract $projectTemplateOnboardingGovernanceContract `
        -ForceInformational
    Add-NormalizedWarnings -Collection $warnings -Report $report
}
foreach ($report in @($explicitWarningReports.ToArray())) {
    Add-NormalizedWarnings -Collection $warnings -Report $report
}

$loadedReportCount = @($reports.ToArray() | Where-Object { $_.status -notin @("missing", "failed") }).Count
$missingReportCount = @($reports.ToArray() | Where-Object { $_.status -eq "missing" }).Count
$failedReportCount = @($reports.ToArray() | Where-Object { $_.status -eq "failed" -or $_.source_failure_count -gt 0 }).Count
$warningCount = 0
foreach ($report in @($reports.ToArray())) {
    $warningCount += [int]$report.warning_count
}
foreach ($report in @($explicitWarningReports.ToArray())) {
    $warningCount += @($report.warnings).Count
}
$governanceMetrics = New-Object 'System.Collections.Generic.List[object]'
foreach ($report in @($reports.ToArray())) {
    foreach ($metric in @($report.governance_metrics)) {
        $governanceMetrics.Add($metric) | Out-Null
    }
}

$status = if ($failedReportCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0) {
    "blocked"
} elseif ($missingReportCount -gt 0 -or $warningCount -gt 0) {
    "needs_review"
} else {
    "ready"
}

$rollupCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1 -ReleaseBlockerRollupAutoDiscover'
$nextCommands = New-Object 'System.Collections.Generic.List[string]'
foreach ($report in @($reports.ToArray() | Where-Object { $_.status -eq "missing" })) {
    $nextCommands.Add([string]$report.build_command) | Out-Null
}
$nextCommands.Add($rollupCommand) | Out-Null

$summary = [ordered]@{
    schema = "featherdoc.release_governance_handoff_report.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    input_root = $resolvedInputRoot
    input_root_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedInputRoot
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    expected_report_profile = $ExpectedReportProfile
    release_blocker_rollup = [ordered]@{
        included = [bool]$IncludeReleaseBlockerRollup
        status = if ($IncludeReleaseBlockerRollup) { "pending" } else { "not_requested" }
        output_dir = $releaseBlockerRollupOutputDir
        output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBlockerRollupOutputDir
        summary_json = $releaseBlockerRollupSummaryPath
        summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBlockerRollupSummaryPath
        report_markdown = $releaseBlockerRollupMarkdownPath
        report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBlockerRollupMarkdownPath
        source_report_count = 0
        source_failure_count = 0
        release_blocker_count = 0
        blocker_source_schema_summary = @()
        action_item_count = 0
        action_item_source_schema_summary = @()
        informational_action_item_count = 0
        informational_action_item_source_schema_summary = @()
        warning_count = 0
        warning_source_schema_summary = @()
        governance_metric_count = 0
        governance_metrics = @()
        docx_functional_smoke_readiness_evidence_source_report_count = 0
        docx_functional_smoke_readiness_evidence_source_reports = @()
        pdf_visual_gate_evidence_source_report_count = 0
        pdf_visual_gate_evidence_source_reports = @()
        manifest_signoff_entrypoints_source_report_count = 0
        manifest_signoff_entrypoints_source_reports = @()
        project_template_readiness_checklist_entrypoints_source_report_count = 0
        project_template_readiness_checklist_entrypoints_source_reports = @()
        release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = 0
        release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @()
        word_visual_standard_review_metadata_source_report_count = 0
        word_visual_standard_review_metadata_source_reports = @()
    }
    expected_report_count = $expectedReports.Count
    loaded_report_count = $loadedReportCount
    missing_report_count = $missingReportCount
    failed_report_count = $failedReportCount
    reports = @($reports.ToArray())
    project_template_delivery_readiness_contract = ($reports.ToArray() | Where-Object { [string]$_.id -eq "project_template_delivery_readiness" } | Select-Object -First 1)
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
    governance_metric_count = $governanceMetrics.Count
    governance_metrics = @($governanceMetrics.ToArray())
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    informational_action_item_count = $informationalActionItems.Count
    informational_action_items = @($informationalActionItems.ToArray())
    warning_count = $warningCount
    warnings = @($warnings.ToArray())
    next_commands = @($nextCommands.ToArray())
}

Write-ReleaseMaterialFiles -Summary $summary -SummaryPath $summaryPath -MarkdownPath $markdownPath -JsonDepth 32

if ($IncludeReleaseBlockerRollup) {
    $loadedReports = @($reports.ToArray() | Where-Object { $_.status -notin @("missing", "failed") })
    $loadedReportInputs = @(
        foreach ($report in $loadedReports) {
            [string]$report.expected_summary
        }
        foreach ($input in @(Expand-InputPathList -Paths $InputJson)) {
            $path = Resolve-RepoPath -RepoRoot $repoRoot -Path $input -AllowMissing
            if (Test-Path -LiteralPath $path) {
                $path
            }
        }
    )
    Invoke-ReleaseBlockerRollup `
        -RepoRoot $repoRoot `
        -OutputDir $releaseBlockerRollupOutputDir `
        -InputJson $loadedReportInputs
    if (-not (Test-Path -LiteralPath $releaseBlockerRollupSummaryPath)) {
        throw "Release blocker rollup was not written: $releaseBlockerRollupSummaryPath"
    }
    if (-not (Test-Path -LiteralPath $releaseBlockerRollupMarkdownPath)) {
        throw "Release blocker rollup Markdown was not written: $releaseBlockerRollupMarkdownPath"
    }

    $rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseBlockerRollupSummaryPath | ConvertFrom-Json
    $rollupGovernanceMetrics = @(Get-JsonArray -Object $rollupSummary -Name "governance_metrics")
    $rollupGovernanceMetricCount = [int](@($rollupGovernanceMetrics).Count)
    $rollupGovernanceMetricCount = Get-JsonInt -Object $rollupSummary -Name "governance_metric_count" -DefaultValue $rollupGovernanceMetricCount
    $docxFunctionalSmokeReadinessEvidence = @(Get-DocxFunctionalSmokeReadinessRollupEvidence -RollupSummary $rollupSummary)
    $summary.release_blocker_rollup.docx_functional_smoke_readiness_evidence_source_report_count = @($docxFunctionalSmokeReadinessEvidence).Count
    $summary.release_blocker_rollup.docx_functional_smoke_readiness_evidence_source_reports = @($docxFunctionalSmokeReadinessEvidence)
    $pdfVisualGateEvidence = @(Get-PdfVisualGateRollupEvidence -RollupSummary $rollupSummary)
    $summary.release_blocker_rollup.status = [string]$rollupSummary.status
    $summary.release_blocker_rollup.source_report_count = [int]$rollupSummary.source_report_count
    $summary.release_blocker_rollup.source_failure_count = [int]$rollupSummary.source_failure_count
    $summary.release_blocker_rollup.release_blocker_count = [int]$rollupSummary.release_blocker_count
    $summary.release_blocker_rollup.blocker_source_schema_summary = @(Get-JsonArray -Object $rollupSummary -Name "blocker_source_schema_summary")
    $summary.release_blocker_rollup.action_item_count = [int]$rollupSummary.action_item_count
    $summary.release_blocker_rollup.action_item_source_schema_summary = @(Get-JsonArray -Object $rollupSummary -Name "action_item_source_schema_summary")
    $summary.release_blocker_rollup.informational_action_item_count = [int](Get-JsonInt -Object $rollupSummary -Name "informational_action_item_count")
    $summary.release_blocker_rollup.informational_action_item_source_schema_summary = @(Get-JsonArray -Object $rollupSummary -Name "informational_action_item_source_schema_summary")
    $summary.release_blocker_rollup.warning_count = [int]$rollupSummary.warning_count
    $summary.release_blocker_rollup.warning_source_schema_summary = @(Get-JsonArray -Object $rollupSummary -Name "warning_source_schema_summary")
    $summary.release_blocker_rollup.governance_metric_count = [int]$rollupGovernanceMetricCount
    $summary.release_blocker_rollup.governance_metrics = @($rollupGovernanceMetrics)
    $summary.release_blocker_rollup.pdf_visual_gate_evidence_source_report_count = @($pdfVisualGateEvidence).Count
    $summary.release_blocker_rollup.pdf_visual_gate_evidence_source_reports = @($pdfVisualGateEvidence)
    $manifestSignoffEvidence = @(Get-ManifestSignoffEntrypointsRollupEvidence -RollupSummary $rollupSummary)
    $summary.release_blocker_rollup.manifest_signoff_entrypoints_source_report_count = @($manifestSignoffEvidence).Count
    $summary.release_blocker_rollup.manifest_signoff_entrypoints_source_reports = @($manifestSignoffEvidence)
    $projectTemplateChecklistEvidence = @(Get-ProjectTemplateReadinessChecklistEntrypointsRollupEvidence -RollupSummary $rollupSummary)
    $summary.release_blocker_rollup.project_template_readiness_checklist_entrypoints_source_report_count = @($projectTemplateChecklistEvidence).Count
    $summary.release_blocker_rollup.project_template_readiness_checklist_entrypoints_source_reports = @($projectTemplateChecklistEvidence)
    $releaseEntryChecklistAuditEvidence = @(Get-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditRollupEvidence -RollupSummary $rollupSummary)
    $summary.release_blocker_rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = @($releaseEntryChecklistAuditEvidence).Count
    $summary.release_blocker_rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @($releaseEntryChecklistAuditEvidence)
    $wordVisualStandardReviewMetadataEvidence = @(Get-WordVisualStandardReviewMetadataRollupEvidence -RollupSummary $rollupSummary)
    $summary.release_blocker_rollup.word_visual_standard_review_metadata_source_report_count = @($wordVisualStandardReviewMetadataEvidence).Count
    $summary.release_blocker_rollup.word_visual_standard_review_metadata_source_reports = @($wordVisualStandardReviewMetadataEvidence)
    Write-ReleaseMaterialFiles -Summary $summary -SummaryPath $summaryPath -MarkdownPath $markdownPath -JsonDepth 32
}

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($FailOnMissing -and $missingReportCount -gt 0) { exit 1 }
if ($FailOnWarning -and $warningCount -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
if ($failedReportCount -gt 0) { exit 1 }
