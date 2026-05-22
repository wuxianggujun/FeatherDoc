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

function Write-Step {
    param([string]$Message)
    Write-Host "[release-blocker-rollup] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$Path, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }
    return $resolved
}

function Ensure-Directory {
    param([string]$Path)
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Get-DisplayPath {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $root = [System.IO.Path]::GetFullPath($RepoRoot)
    if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $root.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $root += [System.IO.Path]::DirectorySeparatorChar
    }
    $resolved = [System.IO.Path]::GetFullPath($Path)
    if ($resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolved.Substring($root.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
        return ".\" + ($relative -replace '/', '\')
    }
    return $resolved
}

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) { return $Object[$Name] }
        return $null
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-JsonString {
    param($Object, [string]$Name, [string]$DefaultValue = "")

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    return [string]$value
}

function Get-FirstJsonString {
    param($Object, [string[]]$Names, [string]$DefaultValue = "")

    foreach ($name in @($Names)) {
        $value = Get-JsonString -Object $Object -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }
    return $DefaultValue
}

function Get-JsonInt {
    param($Object, [string]$Name, [int]$DefaultValue = 0)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    $parsed = 0
    if ([int]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $DefaultValue
}

function Get-JsonBool {
    param($Object, [string]$Name, [bool]$DefaultValue = $false)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    if ($value -is [bool]) { return [bool]$value }
    $parsed = $false
    if ([bool]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $DefaultValue
}

function Get-JsonArray {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Copy-OptionalJsonProperties {
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

function Get-ReportIdFromSchema {
    param([string]$SourceSchema)

    switch ($SourceSchema) {
        "featherdoc.numbering_catalog_governance_report.v1" { return "numbering_catalog_governance" }
        "featherdoc.table_layout_delivery_governance_report.v1" { return "table_layout_delivery_governance" }
        default { return "" }
    }
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
        [string]$_.metric -eq $Metric -and
        [string]$_.id -eq $Id -and
        [string]$_.report_id -eq $ReportId -and
        [string]$_.source_schema -eq $SourceSchema
    }) | Select-Object -First 1
}

function Add-GovernanceMetricDetailLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        $Metric
    )

    $details = Get-JsonProperty -Object $Metric -Name "details"
    if ($null -eq $details) { return }

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
        "command_failure_count",
        "unresolved_item_count"
    )
    $detailParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in $detailFields) {
        $value = Get-JsonProperty -Object $details -Name $fieldName
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            $detailParts.Add("$fieldName=$value") | Out-Null
        }
    }
    if ($detailParts.Count -gt 0) {
        $Lines.Add("  - details: ``$($detailParts -join ', ')``") | Out-Null
    }

    $penaltyParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($penalty in @(Get-JsonArray -Object $details -Name "penalty_summary")) {
        $factor = Get-JsonString -Object $penalty -Name "factor"
        if ([string]::IsNullOrWhiteSpace($factor)) { continue }
        $count = Get-JsonProperty -Object $penalty -Name "count"
        $penaltyValue = Get-JsonProperty -Object $penalty -Name "penalty"
        $penaltyParts.Add("$factor(count=$count, penalty=$penaltyValue)") | Out-Null
    }
    if ($penaltyParts.Count -gt 0) {
        $Lines.Add("  - penalty_summary: ``$($penaltyParts -join '; ')``") | Out-Null
    }
}

function New-GovernanceMetrics {
    param(
        $Summary,
        [string]$SourceSchema,
        [string]$SourceReport,
        [string]$SourceReportDisplay
    )

    $metrics = New-Object 'System.Collections.Generic.List[object]'
    $reportId = Get-ReportIdFromSchema -SourceSchema $SourceSchema

    if ($SourceSchema -eq "featherdoc.numbering_catalog_governance_report.v1" -and
        ($null -ne (Get-JsonProperty -Object $Summary -Name "real_corpus_confidence_score") -or
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $Summary -Name "real_corpus_confidence_level")))) {
        $metrics.Add([ordered]@{
            id = "numbering_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = $reportId
            source_schema = $SourceSchema
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
            source_json = $SourceReport
            source_json_display = $SourceReportDisplay
            score = Get-JsonInt -Object $Summary -Name "real_corpus_confidence_score"
            level = Get-JsonString -Object $Summary -Name "real_corpus_confidence_level"
            details = Get-JsonProperty -Object $Summary -Name "real_corpus_confidence"
        }) | Out-Null
    }

    if ($SourceSchema -eq "featherdoc.table_layout_delivery_governance_report.v1" -and
        ($null -ne (Get-JsonProperty -Object $Summary -Name "delivery_quality_score") -or
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $Summary -Name "delivery_quality_level")))) {
        $metrics.Add([ordered]@{
            id = "table_layout_delivery_governance.delivery_quality"
            metric = "delivery_quality"
            report_id = $reportId
            source_schema = $SourceSchema
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
            source_json = $SourceReport
            source_json_display = $SourceReportDisplay
            score = Get-JsonInt -Object $Summary -Name "delivery_quality_score"
            level = Get-JsonString -Object $Summary -Name "delivery_quality_level"
            details = Get-JsonProperty -Object $Summary -Name "delivery_quality"
        }) | Out-Null
    }

    return @($metrics.ToArray())
}

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @(Expand-TemplateSchemaArgumentList -Values $ExplicitPaths)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
        }
    }

    $scanRoots = @(Expand-TemplateSchemaArgumentList -Values $Roots | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @("output")
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) { continue }

        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter "*.json" |
                Where-Object { $_.Name -in @("summary.json", "document_skeleton_governance.summary.json") } |
                ForEach-Object { $paths.Add($_.FullName) | Out-Null }
        } else {
            $paths.Add($resolvedRoot) | Out-Null
        }
    }

    return @($paths.ToArray() | Sort-Object -Unique)
}

function Get-ReportKind {
    param($Summary)

    $schema = Get-JsonString -Object $Summary -Name "schema"
    if (-not [string]::IsNullOrWhiteSpace($schema)) { return $schema }
    if ($null -ne (Get-JsonProperty -Object $Summary -Name "table_style_quality_audit")) {
        return "featherdoc.table_layout_delivery_report.v1"
    }
    if ($null -ne (Get-JsonProperty -Object $Summary -Name "style_numbering_audit")) {
        return "featherdoc.document_skeleton_governance_report.v1"
    }
    if ($null -ne (Get-JsonProperty -Object $Summary -Name "project_template_smoke")) {
        return "featherdoc.release_candidate_summary"
    }
    return "unknown"
}

function Add-SummaryGroup {
    param([object[]]$Items, [string]$PropertyName, [string]$OutputName)

    return @(
        foreach ($group in @($Items | Group-Object $PropertyName |
            Sort-Object -Property @{ Expression = "Count"; Descending = $true }, @{ Expression = "Name"; Ascending = $true })) {
            $summary = [ordered]@{}
            $summary[$OutputName] = [string]$group.Name
            $summary["count"] = [int]$group.Count
            $summary
        }
    )
}

function Add-TraceabilityMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Item
    )

    foreach ($fieldName in @(
            "source_report",
            "source_json",
            "origin_source_report",
            "origin_source_report_display")) {
        $fieldValue = Get-JsonString -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            $Lines.Add("  - ${fieldName}: ``$fieldValue``") | Out-Null
        }
    }
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Release Blocker Rollup Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Source reports: ``$($Summary.source_report_count)``") | Out-Null
    $lines.Add("- Source failures: ``$($Summary.source_failure_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($Summary.action_item_count)``") | Out-Null
    $lines.Add("- Warnings: ``$($Summary.warning_count)``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Governance Metric Review Focus") | Out-Null
    $lines.Add("") | Out-Null
    $focusMetrics = @(
        [ordered]@{
            metric = "real_corpus_confidence"
            id = "numbering_catalog_governance.real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            label = "Numbering real-corpus confidence"
        },
        [ordered]@{
            metric = "delivery_quality"
            id = "table_layout_delivery_governance.delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
            label = "Table/layout delivery quality"
        }
    )
    foreach ($focusMetric in $focusMetrics) {
        $metric = Get-GovernanceMetricByContract `
            -Metrics $Summary.governance_metrics `
            -Metric ([string]$focusMetric.metric) `
            -Id ([string]$focusMetric.id) `
            -ReportId ([string]$focusMetric.report_id) `
            -SourceSchema ([string]$focusMetric.source_schema)

        if ($null -eq $metric) {
            $lines.Add("- **$($focusMetric.label)** (``$($focusMetric.metric)``): missing") | Out-Null
            continue
        }

        $lines.Add("- **$($focusMetric.label)** ``$($metric.id)``: metric=``$($metric.metric)`` level=``$($metric.level)`` score=``$($metric.score)`` report=``$($metric.report_id)`` source_schema=``$($metric.source_schema)``") | Out-Null
        Add-TraceabilityMarkdownLines -Lines $lines -Item $metric
        $lines.Add("  - source_report_display: ``$($metric.source_report_display)``") | Out-Null
        $lines.Add("  - source_json_display: ``$($metric.source_json_display)``") | Out-Null
        Add-GovernanceMetricDetailLines -Lines $lines -Metric $metric
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Governance Metrics") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.governance_metrics).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($metric in @($Summary.governance_metrics)) {
            $lines.Add("- ``$($metric.id)``: report=``$($metric.report_id)`` metric=``$($metric.metric)`` level=``$($metric.level)`` score=``$($metric.score)`` schema=``$($metric.source_schema)`` source_report_display=``$($metric.source_report_display)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $metric
            $lines.Add("  - source_json_display: ``$($metric.source_json_display)``") | Out-Null
            Add-GovernanceMetricDetailLines -Lines $lines -Metric $metric
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Source Report Contracts") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.source_reports).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($report in @($Summary.source_reports)) {
            $lines.Add("- ``$($report.schema)``: status=``$($report.status)`` ready=``$($report.release_ready)`` path=``$($report.path_display)``") | Out-Null
            $preflightReady = Get-JsonProperty -Object $report -Name "preflight_ready"
            if ($null -ne $preflightReady) {
                $lines.Add("  - preflight_ready: ``$preflightReady``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$report.error)) {
                $lines.Add("  - error: ``$($report.error)``") | Out-Null
            }
            $fullVisualGateRequired = Get-JsonProperty -Object $report -Name "full_visual_gate_required"
            if ($null -ne $fullVisualGateRequired) {
                $lines.Add("  - full_visual_gate_required: ``$fullVisualGateRequired``") | Out-Null
            }
            $fullVisualGateStatus = Get-JsonString -Object $report -Name "full_visual_gate_status"
            if (-not [string]::IsNullOrWhiteSpace($fullVisualGateStatus)) {
                $lines.Add("  - full_visual_gate_status: ``$fullVisualGateStatus``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$report.latest_schema_approval_gate_status)) {
                $lines.Add("  - latest_schema_approval_gate_status: ``$($report.latest_schema_approval_gate_status)``") | Out-Null
            }
            if (@($report.schema_approval_status_summary).Count -gt 0) {
                $statusParts = @($report.schema_approval_status_summary | ForEach-Object { "$($_.status)=$($_.count)" })
                $lines.Add("  - schema_approval_status_summary: ``$($statusParts -join ', ')``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Blocker Summary") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.blocker_id_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.blocker_id_summary)) {
            $lines.Add("- ``$($item.id)``: $($item.count)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.composite_id)``: project=``$($blocker.project_id)`` template=``$($blocker.template_name)`` candidate=``$($blocker.candidate_type)`` action=``$($blocker.action)`` schema=``$($blocker.source_schema)`` source_report_display=``$($blocker.source_report_display)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $blocker
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.source_json_display)) {
                $lines.Add("  - source_json_display: ``$($blocker.source_json_display)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.message)) {
                $lines.Add("  - $($blocker.message)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($blocker.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.repair_hint)) {
                $lines.Add("  - repair_hint: $($blocker.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.command_template)) {
                $lines.Add("  - command_template: ``$($blocker.command_template)``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.composite_id)``: project=``$($item.project_id)`` template=``$($item.template_name)`` candidate=``$($item.candidate_type)`` action=``$($item.action)`` schema=``$($item.source_schema)`` source_report_display=``$($item.source_report_display)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $item
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.source_json_display)) {
                $lines.Add("  - source_json_display: ``$($item.source_json_display)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($item.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_hint)) {
                $lines.Add("  - repair_hint: $($item.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.command_template)) {
                $lines.Add("  - command_template: ``$($item.command_template)``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.warnings).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($warning in @($Summary.warnings)) {
            $lines.Add("- ``$($warning.id)``: project=``$($warning.project_id)`` template=``$($warning.template_name)`` candidate=``$($warning.candidate_type)`` action=``$($warning.action)`` schema=``$($warning.source_schema)`` source_report_display=``$($warning.source_report_display)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $warning
            if (-not [string]::IsNullOrWhiteSpace([string]$warning.source_json_display)) {
                $lines.Add("  - source_json_display: ``$($warning.source_json_display)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$warning.message)) {
                $lines.Add("  - $($warning.message)") | Out-Null
            }
        }
    }
    return @($lines)
}

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
$warnings = New-Object 'System.Collections.Generic.List[object]'
$governanceMetrics = New-Object 'System.Collections.Generic.List[object]'
$sourceIndex = 0

foreach ($path in @($inputPaths)) {
    $sourceIndex++
    $kind = "unknown"
    $status = "loaded"
    $errorMessage = ""
    $sourceMetrics = @()
    $releaseReady = $false
    $latestSchemaApprovalGateStatus = ""
    $schemaApprovalStatusSummary = @()
    $summaryObject = $null
    try {
        $summaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $kind = Get-ReportKind -Summary $summaryObject
        $releaseReady = Get-JsonBool -Object $summaryObject -Name "release_ready"
        $latestSchemaApprovalGateStatus = Get-JsonString -Object $summaryObject -Name "latest_schema_approval_gate_status"
        $schemaApprovalStatusSummary = @(Get-JsonArray -Object $summaryObject -Name "schema_approval_status_summary")
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
                repair_strategy = Get-JsonString -Object $blocker -Name "repair_strategy"
                repair_hint = Get-JsonString -Object $blocker -Name "repair_hint"
                command_template = Get-JsonString -Object $blocker -Name "command_template"
            }
            Copy-OptionalJsonProperties `
                -Target $rollupBlocker `
                -Source $blocker `
                -Names @(
                    "blocking_summary",
                    "output_gap_count",
                    "missing_output_count",
                    "output_gap_summary",
                    "build_dir_auto_candidates",
                    "pdf_dependency_inputs"
                )
            $blockers.Add($rollupBlocker) | Out-Null
        }

        $actionIndex = 0
        foreach ($item in $sourceActions) {
            $actionIndex++
            $id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
            $sourceJson = Get-JsonString -Object $item -Name "source_json"
            $sourceJsonDisplay = Get-JsonString -Object $item -Name "source_json_display"
            $originSourceReport = Get-JsonString -Object $item -Name "source_report"
            $originSourceReportDisplay = Get-JsonString -Object $item -Name "source_report_display"
            $rollupActionItem = [ordered]@{
                composite_id = ("source{0}.action{1}.{2}" -f $sourceIndex, $actionIndex, $id)
                id = $id
                project_id = Get-JsonString -Object $item -Name "project_id"
                template_name = Get-JsonString -Object $item -Name "template_name"
                candidate_type = Get-JsonString -Object $item -Name "candidate_type"
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
                source_schema = Get-FirstJsonString -Object $item -Names @("source_schema") -DefaultValue $kind
                action = Get-JsonString -Object $item -Name "action"
                title = Get-JsonString -Object $item -Name "title"
                command = Get-JsonString -Object $item -Name "command"
                open_command = Get-FirstJsonString -Object $item -Names @("open_command", "command")
                repair_strategy = Get-JsonString -Object $item -Name "repair_strategy"
                repair_hint = Get-JsonString -Object $item -Name "repair_hint"
                command_template = Get-JsonString -Object $item -Name "command_template"
            }
            Copy-OptionalJsonProperties `
                -Target $rollupActionItem `
                -Source $item `
                -Names @(
                    "blocking_summary",
                    "output_gap_count",
                    "missing_output_count",
                    "output_gap_summary",
                    "build_dir_auto_candidates",
                    "pdf_dependency_inputs"
                )
            $actionItems.Add($rollupActionItem) | Out-Null
        }

        $warningIndex = 0
        foreach ($warning in $sourceWarnings) {
            $warningIndex++
            $id = Get-JsonString -Object $warning -Name "id" -DefaultValue "warning"
            $sourceJson = Get-JsonString -Object $warning -Name "source_json"
            $sourceJsonDisplay = Get-JsonString -Object $warning -Name "source_json_display"
            $originSourceReport = Get-JsonString -Object $warning -Name "source_report"
            $originSourceReportDisplay = Get-JsonString -Object $warning -Name "source_report_display"
            $warnings.Add([ordered]@{
                composite_id = ("source{0}.warning{1}.{2}" -f $sourceIndex, $warningIndex, $id)
                id = $id
                project_id = Get-JsonString -Object $warning -Name "project_id"
                template_name = Get-JsonString -Object $warning -Name "template_name"
                candidate_type = Get-JsonString -Object $warning -Name "candidate_type"
                action = Get-JsonString -Object $warning -Name "action" -DefaultValue "review_release_governance_warning"
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
                message = Get-JsonString -Object $warning -Name "message"
            }) | Out-Null
        }
    } catch {
        $status = "failed"
        $errorMessage = $_.Exception.Message
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
        latest_schema_approval_gate_status = $latestSchemaApprovalGateStatus
        schema_approval_status_summary = @($schemaApprovalStatusSummary)
        governance_metric_count = @($sourceMetrics).Count
        governance_metrics = @($sourceMetrics)
    }
    Copy-OptionalJsonProperties `
        -Target $sourceReport `
        -Source $summaryObject `
        -Names @(
            "preflight_ready",
            "full_visual_gate_required",
            "full_visual_gate_status"
        )
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
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnWarning -and $warnings.Count -gt 0) { exit 1 }
if ($FailOnBlocker -and $blockers.Count -gt 0) { exit 1 }
