<#
.SYNOPSIS
Builds a project-template workflow dashboard from existing governance reports.

.DESCRIPTION
Reads the project-template onboarding governance and delivery readiness reports,
then writes a compact JSON/Markdown dashboard with status, blockers, warnings,
schema approval state, next action, and source links. The script is read-only
for source artifacts and does not rerun smoke, CMake, Word, PDF, or visual
automation.
#>
param(
    [string]$ReleaseCandidateSummaryJson = "",
    [string]$OnboardingGovernanceJson = "",
    [string]$DeliveryReadinessJson = "",
    [string]$OutputDir = "output/project-template-workflow-dashboard",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnBlocker
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$dashboardSchema = "featherdoc.project_template_workflow_dashboard.v1"
$onboardingGovernanceSchema = "featherdoc.project_template_onboarding_governance_report.v1"
$deliveryReadinessSchema = "featherdoc.project_template_delivery_readiness_report.v1"
$dashboardOpenCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_workflow_dashboard.ps1"

function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-workflow-dashboard] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $RepoRoot $Path
    }
    $fullPath = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $fullPath)) {
        throw "Path does not exist: $Path"
    }
    return $fullPath
}

function Ensure-Directory {
    param([string]$Path)
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Get-DisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    if (-not $resolvedRepoRoot.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedRepoRoot.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedRepoRoot += [System.IO.Path]::DirectorySeparatorChar
    }

    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }
        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Get-JsonProperty {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Get-JsonString {
    param(
        $Object,
        [string]$Name,
        [string]$DefaultValue = ""
    )

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) {
        return $DefaultValue
    }

    return [string]$value
}

function Get-JsonBool {
    param(
        $Object,
        [string]$Name,
        [bool]$DefaultValue = $false
    )

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) {
        return $DefaultValue
    }

    if ($value -is [bool]) {
        return $value
    }

    return [string]$value -eq "true"
}

function Get-JsonInt {
    param(
        $Object,
        [string]$Name,
        [int]$DefaultValue = 0
    )

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) {
        return $DefaultValue
    }

    $parsed = 0
    if ([int]::TryParse([string]$value, [ref]$parsed)) {
        return $parsed
    }

    return $DefaultValue
}

function Get-JsonArray {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }

    if ($value -is [array]) {
        return @($value)
    }

    return @($value)
}

function Convert-NextActionToSummary {
    param($Action)

    if ($null -eq $Action) {
        return [ordered]@{
            action = ""
            reason = ""
            blocker_id = ""
            command = ""
        }
    }

    return [ordered]@{
        action = Get-JsonString -Object $Action -Name "action"
        reason = Get-JsonString -Object $Action -Name "reason"
        blocker_id = Get-JsonString -Object $Action -Name "blocker_id"
        command = Get-JsonString -Object $Action -Name "command"
    }
}

function Convert-NextActionSummaryEntry {
    param(
        $Entry,
        $SourceReport
    )

    $sourceNextAction = $null
    if ($null -ne $SourceReport) {
        $sourceNextAction = $SourceReport.next_action
    }

    $action = Get-JsonString -Object $Entry -Name "action"
    if ([string]::IsNullOrWhiteSpace($action)) {
        $action = Get-JsonString -Object $sourceNextAction -Name "action"
    }

    $reason = Get-JsonString -Object $Entry -Name "reason"
    if ([string]::IsNullOrWhiteSpace($reason)) {
        $reason = Get-JsonString -Object $sourceNextAction -Name "reason"
    }

    $blockerId = Get-JsonString -Object $Entry -Name "blocker_id"
    if ([string]::IsNullOrWhiteSpace($blockerId)) {
        $blockerId = Get-JsonString -Object $sourceNextAction -Name "blocker_id"
    }

    $command = Get-JsonString -Object $Entry -Name "command"
    if ([string]::IsNullOrWhiteSpace($command)) {
        $command = Get-JsonString -Object $sourceNextAction -Name "command"
    }

    $entryNames = @(
        Get-JsonArray -Object $Entry -Name "entry_names" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )

    return [ordered]@{
        source_report_id = [string]$SourceReport.id
        action = $action
        reason = $reason
        blocker_id = $blockerId
        command = $command
        entry_names = $entryNames
        source_report_display = [string]$SourceReport.source_report_display
        source_json_display = [string]$SourceReport.source_json_display
    }
}

function Read-ReleaseCandidateSummaryInput {
    param(
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $null
    }

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Release candidate summary does not exist: $Path"
    }

    try {
        return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
    } catch {
        throw "Release candidate summary is not valid JSON: $Path"
    }
}

function Get-ReleaseCandidateWorkflowReportPath {
    param(
        $Summary,
        [string]$TopLevelName,
        [string]$ContractName
    )

    $topLevelPath = Get-JsonString -Object $Summary -Name $TopLevelName
    if (-not [string]::IsNullOrWhiteSpace($topLevelPath)) {
        return $topLevelPath
    }

    $contractContainers = @(
        Get-JsonProperty -Object $Summary -Name "release_governance_handoff"
    )
    $steps = Get-JsonProperty -Object $Summary -Name "steps"
    if ($null -ne $steps) {
        $contractContainers += Get-JsonProperty -Object $steps -Name "release_governance_handoff"
    }

    foreach ($container in @($contractContainers)) {
        $contract = Get-JsonProperty -Object $container -Name $ContractName
        foreach ($fieldName in @("source_json", "source_json_display", "source_report", "source_report_display")) {
            $contractPath = Get-JsonString -Object $contract -Name $fieldName
            if (-not [string]::IsNullOrWhiteSpace($contractPath)) {
                return $contractPath
            }
        }
    }

    return ""
}

function Read-WorkflowReport {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [string]$Id,
        [string]$ExpectedSchema
    )

    $displayPath = if ([string]::IsNullOrWhiteSpace($Path)) {
        ""
    } else {
        Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
    }

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return [ordered]@{
            id = $Id
            status = "missing_input"
            release_ready = $false
            schema = ""
            expected_schema = $ExpectedSchema
            source_json = ""
            source_json_display = ""
            source_report_display = ""
            release_blocker_count = 1
            warning_count = 0
            schema_approval_status_summary = ""
            latest_schema_approval_gate_status = ""
            next_action = [ordered]@{
                action = "provide_project_template_workflow_evidence"
                reason = "Required input report path was not provided."
                blocker_id = "$Id.missing_input"
                command = $dashboardOpenCommand
            }
            next_action_summary = @()
            next_action_group_count = 0
            issue = "missing_input"
        }
    }

    if (-not (Test-Path -LiteralPath $Path)) {
        return [ordered]@{
            id = $Id
            status = "missing_input"
            release_ready = $false
            schema = ""
            expected_schema = $ExpectedSchema
            source_json = $Path
            source_json_display = $displayPath
            source_report_display = $displayPath
            release_blocker_count = 1
            warning_count = 0
            schema_approval_status_summary = ""
            latest_schema_approval_gate_status = ""
            next_action = [ordered]@{
                action = "rebuild_project_template_workflow_evidence"
                reason = "Required input report path does not exist."
                blocker_id = "$Id.missing_input"
                command = $dashboardOpenCommand
            }
            next_action_summary = @()
            next_action_group_count = 0
            issue = "missing_input"
        }
    }

    try {
        $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
    } catch {
        return [ordered]@{
            id = $Id
            status = "invalid_json"
            release_ready = $false
            schema = ""
            expected_schema = $ExpectedSchema
            source_json = $Path
            source_json_display = $displayPath
            source_report_display = $displayPath
            release_blocker_count = 1
            warning_count = 0
            schema_approval_status_summary = ""
            latest_schema_approval_gate_status = ""
            next_action = [ordered]@{
                action = "repair_project_template_workflow_evidence"
                reason = "Input report is not valid JSON."
                blocker_id = "$Id.invalid_json"
                command = $dashboardOpenCommand
            }
            next_action_summary = @()
            next_action_group_count = 0
            issue = $_.Exception.Message
        }
    }

    $schema = Get-JsonString -Object $json -Name "schema"
    $schemaMatches = [string]::Equals($schema, $ExpectedSchema, [System.StringComparison]::OrdinalIgnoreCase)
    $status = Get-JsonString -Object $json -Name "status" -DefaultValue "unknown"
    $releaseReady = Get-JsonBool -Object $json -Name "release_ready"
    $releaseBlockerCount = Get-JsonInt -Object $json -Name "release_blocker_count"
    $warningCount = Get-JsonInt -Object $json -Name "warning_count"
    $nextAction = Convert-NextActionToSummary -Action (Get-JsonProperty -Object $json -Name "next_action")
    $nextActionSummary = @(Get-JsonArray -Object $json -Name "next_action_summary")
    $nextActionGroupCount = Get-JsonInt -Object $json -Name "next_action_group_count" -DefaultValue $nextActionSummary.Count
    $schemaApprovalStatusSummary = Get-JsonString -Object $json -Name "schema_approval_status_summary"
    $latestSchemaApprovalGateStatus = Get-JsonString -Object $json -Name "latest_schema_approval_gate_status"

    if ($Id -eq "project_template_delivery_readiness") {
        $onboardingNextAction = Get-JsonProperty -Object $json -Name "onboarding_governance_next_action"
        if ($null -ne $onboardingNextAction) {
            $nextAction = Convert-NextActionToSummary -Action $onboardingNextAction
        }
        $onboardingSummary = @(Get-JsonArray -Object $json -Name "onboarding_governance_next_action_summary")
        if ($onboardingSummary.Count -gt 0) {
            $nextActionSummary = $onboardingSummary
            $nextActionGroupCount = Get-JsonInt -Object $json -Name "onboarding_governance_next_action_group_count" -DefaultValue $onboardingSummary.Count
        }
    }

    if (-not $schemaMatches) {
        $status = "schema_mismatch"
        $releaseReady = $false
        if ($releaseBlockerCount -lt 1) {
            $releaseBlockerCount = 1
        }
    }

    $sourceReportDisplay = Get-JsonString -Object $json -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = $displayPath
    }
    $sourceJsonDisplay = Get-JsonString -Object $json -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = $displayPath
    }

    return [ordered]@{
        id = $Id
        status = $status
        release_ready = $releaseReady
        schema = $schema
        expected_schema = $ExpectedSchema
        schema_matches = $schemaMatches
        source_json = $Path
        source_json_display = $sourceJsonDisplay
        source_report_display = $sourceReportDisplay
        release_blocker_count = $releaseBlockerCount
        warning_count = $warningCount
        schema_approval_status_summary = $schemaApprovalStatusSummary
        latest_schema_approval_gate_status = $latestSchemaApprovalGateStatus
        next_action = $nextAction
        next_action_summary = $nextActionSummary
        next_action_group_count = $nextActionGroupCount
        issue = ""
    }
}

function New-DashboardMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    [void]$lines.Add("# Project Template Workflow Dashboard")
    [void]$lines.Add("")
    [void]$lines.Add("- schema: ``$($Summary.schema)``")
    [void]$lines.Add("- status: ``$($Summary.status)``")
    [void]$lines.Add("- release_ready: ``$($Summary.release_ready)``")
    [void]$lines.Add("- release_blocker_count: ``$($Summary.release_blocker_count)``")
    [void]$lines.Add("- warning_count: ``$($Summary.warning_count)``")
    [void]$lines.Add("- next_action: ``$($Summary.next_action.action)``")
    [void]$lines.Add("- next_action_reason: $($Summary.next_action.reason)")
    [void]$lines.Add("")
    [void]$lines.Add("## Source reports")
    [void]$lines.Add("")
    foreach ($report in @($Summary.source_reports)) {
        [void]$lines.Add("- ``$($report.id)``: status=``$($report.status)`` release_ready=``$($report.release_ready)`` blockers=``$($report.release_blocker_count)`` warnings=``$($report.warning_count)`` source=``$($report.source_json_display)``")
        if (-not [string]::IsNullOrWhiteSpace([string]$report.schema_approval_status_summary)) {
            [void]$lines.Add("  - schema_approval_status_summary: ``$($report.schema_approval_status_summary)``")
        }
        if (-not [string]::IsNullOrWhiteSpace([string]$report.latest_schema_approval_gate_status)) {
            [void]$lines.Add("  - latest_schema_approval_gate_status: ``$($report.latest_schema_approval_gate_status)``")
        }
        if (-not [string]::IsNullOrWhiteSpace([string]$report.next_action.action)) {
            [void]$lines.Add("  - next_action: ``$($report.next_action.action)``")
            [void]$lines.Add("  - blocker_id: ``$($report.next_action.blocker_id)``")
        }
    }
    [void]$lines.Add("")
    [void]$lines.Add("## Next action groups")
    [void]$lines.Add("")
    if ([int]$Summary.next_action_group_count -eq 0) {
        [void]$lines.Add("- none")
    } else {
        foreach ($actionGroup in @($Summary.next_action_summary)) {
            $entryNameText = (@($actionGroup.entry_names) -join ", ")
            [void]$lines.Add("- source=``$($actionGroup.source_report_id)`` action=``$($actionGroup.action)`` blocker_id=``$($actionGroup.blocker_id)`` entries=``$entryNameText``")
            if (-not [string]::IsNullOrWhiteSpace([string]$actionGroup.reason)) {
                [void]$lines.Add("  - reason: $($actionGroup.reason)")
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$actionGroup.command)) {
                [void]$lines.Add("  - command: ``$($actionGroup.command)``")
            }
        }
    }
    [void]$lines.Add("")
    [void]$lines.Add("## Handoff")
    [void]$lines.Add("")
    [void]$lines.Add("- command: ``$($Summary.next_action.command)``")
    [void]$lines.Add("- blocker_id: ``$($Summary.next_action.blocker_id)``")

    return ($lines -join [System.Environment]::NewLine) + [System.Environment]::NewLine
}

$repoRoot = Resolve-RepoRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "project_template_workflow_dashboard.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "project_template_workflow_dashboard.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$resolvedOnboardingPath = Resolve-RepoPath -RepoRoot $repoRoot -Path $OnboardingGovernanceJson -AllowMissing
$resolvedDeliveryPath = Resolve-RepoPath -RepoRoot $repoRoot -Path $DeliveryReadinessJson -AllowMissing
$resolvedReleaseCandidateSummaryPath = Resolve-RepoPath -RepoRoot $repoRoot -Path $ReleaseCandidateSummaryJson -AllowMissing
$releaseCandidateSummary = Read-ReleaseCandidateSummaryInput -Path $resolvedReleaseCandidateSummaryPath
if ($null -ne $releaseCandidateSummary) {
    if ([string]::IsNullOrWhiteSpace($resolvedOnboardingPath)) {
        $onboardingPathFromSummary = Get-ReleaseCandidateWorkflowReportPath `
            -Summary $releaseCandidateSummary `
            -TopLevelName "project_template_onboarding_governance" `
            -ContractName "project_template_onboarding_governance_contract"
        $resolvedOnboardingPath = Resolve-RepoPath `
            -RepoRoot $repoRoot `
            -Path $onboardingPathFromSummary `
            -AllowMissing
    }
    if ([string]::IsNullOrWhiteSpace($resolvedDeliveryPath)) {
        $deliveryPathFromSummary = Get-ReleaseCandidateWorkflowReportPath `
            -Summary $releaseCandidateSummary `
            -TopLevelName "project_template_delivery_readiness" `
            -ContractName "project_template_delivery_readiness_contract"
        $resolvedDeliveryPath = Resolve-RepoPath `
            -RepoRoot $repoRoot `
            -Path $deliveryPathFromSummary `
            -AllowMissing
    }
}
$releaseCandidateSummaryDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedReleaseCandidateSummaryPath

Write-Step "Reading workflow evidence"
$onboardingReport = Read-WorkflowReport -RepoRoot $repoRoot -Path $resolvedOnboardingPath -Id "project_template_onboarding_governance" -ExpectedSchema $onboardingGovernanceSchema
$deliveryReport = Read-WorkflowReport -RepoRoot $repoRoot -Path $resolvedDeliveryPath -Id "project_template_delivery_readiness" -ExpectedSchema $deliveryReadinessSchema
$sourceReports = @($onboardingReport, $deliveryReport)

$releaseBlockerCount = 0
$warningCount = 0
$missingInputCount = 0
$schemaMismatchCount = 0
$allReleaseReady = $true
foreach ($report in $sourceReports) {
    $releaseBlockerCount += [int]$report.release_blocker_count
    $warningCount += [int]$report.warning_count
    if (-not [bool]$report.release_ready) {
        $allReleaseReady = $false
    }
    if ([string]$report.status -eq "missing_input") {
        $missingInputCount += 1
    }
    if ($null -ne $report.PSObject.Properties["schema_matches"] -and -not [bool]$report.schema_matches) {
        $schemaMismatchCount += 1
    }
}

$status = "ready"
if ($missingInputCount -gt 0) {
    $status = "missing_input"
} elseif ($schemaMismatchCount -gt 0) {
    $status = "schema_mismatch"
} elseif (-not $allReleaseReady -or $releaseBlockerCount -gt 0) {
    $status = "blocked"
} elseif ($warningCount -gt 0) {
    $status = "warning"
}
$releaseReady = $status -eq "ready"

$firstBlockingReport = @($sourceReports | Where-Object { -not [bool]$_.release_ready -or [int]$_.release_blocker_count -gt 0 }) | Select-Object -First 1
$nextAction = if ($null -ne $firstBlockingReport) {
    [ordered]@{
        action = if ([string]::IsNullOrWhiteSpace([string]$firstBlockingReport.next_action.action)) { "review_project_template_workflow_evidence" } else { [string]$firstBlockingReport.next_action.action }
        reason = if ([string]::IsNullOrWhiteSpace([string]$firstBlockingReport.next_action.reason)) { "A project-template workflow source report is not release-ready." } else { [string]$firstBlockingReport.next_action.reason }
        blocker_id = if ([string]::IsNullOrWhiteSpace([string]$firstBlockingReport.next_action.blocker_id)) { "$($firstBlockingReport.id).not_release_ready" } else { [string]$firstBlockingReport.next_action.blocker_id }
        command = $dashboardOpenCommand
        source_report_id = [string]$firstBlockingReport.id
        source_report_display = [string]$firstBlockingReport.source_report_display
        source_json_display = [string]$firstBlockingReport.source_json_display
    }
} else {
    [ordered]@{
        action = "project_template_workflow_ready"
        reason = "All required project-template workflow reports are release-ready."
        blocker_id = ""
        command = $dashboardOpenCommand
        source_report_id = ""
        source_report_display = ""
        source_json_display = ""
    }
}

$nextActionSummary = @()
foreach ($report in $sourceReports) {
    foreach ($entry in @($report.next_action_summary)) {
        $normalizedEntry = Convert-NextActionSummaryEntry -Entry $entry -SourceReport $report
        $hasActionDetails = -not [string]::IsNullOrWhiteSpace([string]$normalizedEntry.action) -or
            -not [string]::IsNullOrWhiteSpace([string]$normalizedEntry.blocker_id) -or
            @($normalizedEntry.entry_names).Count -gt 0
        if ($hasActionDetails) {
            $nextActionSummary += $normalizedEntry
        }
    }
}
$nextActionGroupCount = $nextActionSummary.Count

$summary = [ordered]@{
    schema = $dashboardSchema
    summary_schema_version = 1
    generated_at = [System.DateTime]::UtcNow.ToString("o", [System.Globalization.CultureInfo]::InvariantCulture)
    status = $status
    release_ready = $releaseReady
    release_blocker_count = $releaseBlockerCount
    warning_count = $warningCount
    source_report_count = $sourceReports.Count
    missing_input_count = $missingInputCount
    schema_mismatch_count = $schemaMismatchCount
    release_candidate_summary_json = $resolvedReleaseCandidateSummaryPath
    release_candidate_summary_json_display = $releaseCandidateSummaryDisplay
    next_action = $nextAction
    next_action_summary = $nextActionSummary
    next_action_group_count = $nextActionGroupCount
    source_reports = $sourceReports
    source_schema_summary = @(
        [ordered]@{
            id = "project_template_onboarding_governance"
            schema = $onboardingGovernanceSchema
            status = [string]$onboardingReport.status
            release_ready = [bool]$onboardingReport.release_ready
        },
        [ordered]@{
            id = "project_template_delivery_readiness"
            schema = $deliveryReadinessSchema
            status = [string]$deliveryReport.status
            release_ready = [bool]$deliveryReport.release_ready
        }
    )
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
New-DashboardMarkdown -Summary $summary | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Dashboard JSON: $(Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath)"
Write-Step "Dashboard Markdown: $(Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath)"
Write-Step "Status: $status"

if ($FailOnBlocker -and -not $releaseReady) {
    throw "Project-template workflow dashboard is not release-ready: $status."
}
