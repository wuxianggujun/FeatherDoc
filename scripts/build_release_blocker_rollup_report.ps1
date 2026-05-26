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

function Resolve-RepoPathFromDisplayPath {
    param([string]$RepoRoot, [string]$DisplayPath)

    if ([string]::IsNullOrWhiteSpace($RepoRoot) -or [string]::IsNullOrWhiteSpace($DisplayPath)) {
        return ""
    }

    $normalized = $DisplayPath -replace '/', '\'
    if ($normalized.StartsWith('.\')) {
        $normalized = $normalized.Substring(2)
    }

    if ([System.IO.Path]::IsPathRooted($normalized)) {
        return [System.IO.Path]::GetFullPath($normalized)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $normalized))
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

function Get-FirstJsonProperty {
    param($Object, [string[]]$Names)

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Object -Name $name
        if ($null -eq $value) { continue }
        if ($value -is [string] -and [string]::IsNullOrWhiteSpace($value)) { continue }
        return $value
    }

    return $null
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

function Set-OptionalSourceReportField {
    param(
        [System.Collections.IDictionary]$Target,
        [string]$Name,
        $Value
    )

    if ($null -eq $Value) { return }
    if ($Value -is [string] -and [string]::IsNullOrWhiteSpace($Value)) { return }
    $Target[$Name] = $Value
}

function Set-JsonPropertyValue {
    param(
        $Object,
        [string]$Name,
        $Value
    )

    if ($null -eq $Object) { return }

    if ($Object -is [System.Collections.IDictionary]) {
        $Object[$Name] = $Value
        return
    }

    $existing = $Object.PSObject.Properties[$Name]
    if ($null -ne $existing) {
        $existing.Value = $Value
        return
    }

    Add-Member -InputObject $Object -MemberType NoteProperty -Name $Name -Value $Value
}

function Normalize-ReadinessActionEvidence {
    param(
        $Items,
        [string]$DefaultSourceSchema,
        [string]$DefaultSourceReport,
        [string]$DefaultSourceReportDisplay,
        [string]$DefaultSourceJson,
        [string]$DefaultSourceJsonDisplay,
        [string]$RepoRoot
    )

    foreach ($evidence in @($Items)) {
        if ($null -eq $evidence) {
            continue
        }
        if ($evidence -isnot [System.Collections.IDictionary] -and $evidence -isnot [pscustomobject]) {
            continue
        }

        $sourceSchema = Get-JsonString -Object $evidence -Name "source_schema"
        $sourceReport = Get-JsonString -Object $evidence -Name "source_report"
        $sourceReportDisplay = Get-JsonString -Object $evidence -Name "source_report_display"
        $sourceJson = Get-JsonString -Object $evidence -Name "source_json"
        $sourceJsonDisplay = Get-JsonString -Object $evidence -Name "source_json_display"

        if ([string]::IsNullOrWhiteSpace($sourceSchema)) {
            Set-JsonPropertyValue -Object $evidence -Name "source_schema" -Value $DefaultSourceSchema
        }

        if ([string]::IsNullOrWhiteSpace($sourceReport)) {
            if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
                $sourceReport = Resolve-RepoPathFromDisplayPath -RepoRoot $RepoRoot -DisplayPath $sourceReportDisplay
            } else {
                $sourceReport = $DefaultSourceReport
            }
            Set-JsonPropertyValue -Object $evidence -Name "source_report" -Value $sourceReport
        }

        if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
            if (-not [string]::IsNullOrWhiteSpace($DefaultSourceReportDisplay)) {
                $sourceReportDisplay = $DefaultSourceReportDisplay
            } elseif (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
                $sourceReportDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $sourceReport
            }

            if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
                Set-JsonPropertyValue -Object $evidence -Name "source_report_display" -Value $sourceReportDisplay
            }
        }

        if ([string]::IsNullOrWhiteSpace($sourceJson)) {
            if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                $sourceJson = Resolve-RepoPathFromDisplayPath -RepoRoot $RepoRoot -DisplayPath $sourceJsonDisplay
            } elseif (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
                $sourceJson = $sourceReport
            } else {
                $sourceJson = $DefaultSourceJson
            }

            if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
                Set-JsonPropertyValue -Object $evidence -Name "source_json" -Value $sourceJson
            }
        }

        if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
                $sourceJsonDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $sourceJson
            } elseif (-not [string]::IsNullOrWhiteSpace($DefaultSourceJsonDisplay)) {
                $sourceJsonDisplay = $DefaultSourceJsonDisplay
            }

            if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                Set-JsonPropertyValue -Object $evidence -Name "source_json_display" -Value $sourceJsonDisplay
            }
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

function Get-PdfVisualGateEvidenceObject {
    param($Summary)

    $pdfVisualGate = Get-JsonProperty -Object $Summary -Name "pdf_visual_gate"
    if ($null -ne $pdfVisualGate) {
        return $pdfVisualGate
    }

    $steps = Get-JsonProperty -Object $Summary -Name "steps"
    if ($null -eq $steps) {
        return $null
    }

    return (Get-JsonProperty -Object $steps -Name "pdf_visual_gate")
}

function Add-PdfVisualGateEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary,
        [string]$RepoRoot
    )

    $pdfVisualGate = Get-PdfVisualGateEvidenceObject -Summary $Summary
    if ($null -eq $pdfVisualGate) {
        return
    }

    $summaryJson = Get-FirstJsonString -Object $pdfVisualGate -Names @("summary_json")
    if ([string]::IsNullOrWhiteSpace($summaryJson)) {
        $summaryJson = Get-FirstJsonString -Object $Summary -Names @("pdf_visual_gate_summary_json")
    }
    $aggregateContactSheet = Get-FirstJsonString -Object $pdfVisualGate -Names @("aggregate_contact_sheet")
    $verdict = Get-FirstJsonString -Object $pdfVisualGate -Names @("verdict")
    $fullVisualGateStatus = Get-FirstJsonString -Object $pdfVisualGate -Names @("full_visual_gate_status")
    if ([string]::IsNullOrWhiteSpace($fullVisualGateStatus) -and $verdict -in @("pass", "fail")) {
        $fullVisualGateStatus = $verdict
    }

    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_status" `
        -Value (Get-FirstJsonString -Object $pdfVisualGate -Names @("status"))
    Set-OptionalSourceReportField -Target $Target -Name "full_visual_gate_status" `
        -Value $fullVisualGateStatus
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_verdict" `
        -Value $verdict
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_finalizable" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("finalizable"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_summary_json" `
        -Value $summaryJson
    if (-not [string]::IsNullOrWhiteSpace($summaryJson)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_summary_json_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $summaryJson)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_aggregate_contact_sheet" `
        -Value $aggregateContactSheet
    if (-not [string]::IsNullOrWhiteSpace($aggregateContactSheet)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_aggregate_contact_sheet_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $aggregateContactSheet)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_cjk_manifest_count" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("cjk_manifest_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_cjk_copy_search_count" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("cjk_copy_search_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_cjk_missing_text_count" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("cjk_missing_text_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_visual_baseline_manifest_count" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("visual_baseline_manifest_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_visual_baseline_count" `
        -Value (Get-FirstJsonProperty -Object $pdfVisualGate -Names @("visual_baseline_count", "baselines_count"))
}

function Get-PdfBoundedCtestEvidenceObject {
    param($Summary)

    $pdfBoundedCtest = Get-JsonProperty -Object $Summary -Name "pdf_bounded_ctest"
    if ($null -ne $pdfBoundedCtest) {
        return $pdfBoundedCtest
    }

    $steps = Get-JsonProperty -Object $Summary -Name "steps"
    if ($null -eq $steps) {
        return $null
    }

    return (Get-JsonProperty -Object $steps -Name "pdf_bounded_ctest")
}

function Add-PdfBoundedCtestEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary
    )

    $pdfBoundedCtest = Get-PdfBoundedCtestEvidenceObject -Summary $Summary
    if ($null -eq $pdfBoundedCtest) {
        return
    }

    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_summary_count" `
        -Value (Get-FirstJsonProperty -Object $pdfBoundedCtest -Names @("summary_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_pass_count" `
        -Value (Get-FirstJsonProperty -Object $pdfBoundedCtest -Names @("pass_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_skipped_test_count" `
        -Value (Get-FirstJsonProperty -Object $pdfBoundedCtest -Names @("skipped_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_selected_test_count" `
        -Value (Get-FirstJsonProperty -Object $pdfBoundedCtest -Names @("selected_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_subsets" `
        -Value @(Get-JsonArray -Object $pdfBoundedCtest -Name "subsets")
    Set-OptionalSourceReportField -Target $Target -Name "pdf_bounded_ctest_summary_json_display" `
        -Value @(Get-JsonArray -Object $pdfBoundedCtest -Name "summary_json_display")
}

function Get-PdfVisualGateAttemptEvidenceObject {
    param($Summary)

    $attempt = Get-JsonProperty -Object $Summary -Name "pdf_visual_gate_attempt"
    if ($null -ne $attempt) {
        return $attempt
    }

    $steps = Get-JsonProperty -Object $Summary -Name "steps"
    if ($null -eq $steps) {
        return $null
    }

    return (Get-JsonProperty -Object $steps -Name "pdf_visual_gate_attempt")
}

function Add-PdfVisualGateAttemptEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary,
        [string]$RepoRoot
    )

    $attempt = Get-PdfVisualGateAttemptEvidenceObject -Summary $Summary
    if ($null -eq $attempt) {
        return
    }

    $summaryJson = Get-FirstJsonString -Object $attempt -Names @("summary_json")
    if ([string]::IsNullOrWhiteSpace($summaryJson)) {
        $summaryJson = Get-FirstJsonString -Object $Summary -Names @("pdf_visual_gate_attempt_summary_json")
    }
    $status = Get-FirstJsonString -Object $attempt -Names @("status")
    if ([string]::IsNullOrWhiteSpace($summaryJson) -and $status -eq "not_requested") {
        return
    }
    $aggregateContactSheet = Get-FirstJsonString -Object $attempt -Names @("aggregate_contact_sheet")

    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_status" `
        -Value $status
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_verdict" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("verdict"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_full_visual_gate_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("full_visual_gate_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_evidence_scope" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("evidence_scope"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_summary_json" `
        -Value $summaryJson
    if (-not [string]::IsNullOrWhiteSpace($summaryJson)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_summary_json_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $summaryJson)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_stage_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_passed_stage_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("passed_stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_failed_stage_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("failed_stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_incomplete_stage_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("incomplete_stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_outer_guard_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("outer_guard_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_outer_guard_timed_out" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("outer_guard_timed_out"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_outer_guard_timeout_seconds" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("outer_guard_timeout_seconds"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_pdf_cli_export_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("pdf_cli_export_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_pdf_regression_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("pdf_regression_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_pdf_regression_selected_test_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("pdf_regression_selected_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_pdf_regression_failed_test_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("pdf_regression_failed_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_pdf_regression_skipped_test_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("pdf_regression_skipped_test_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_unicode_font_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("unicode_font_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_cjk_copy_search_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("cjk_copy_search_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_cjk_copy_search_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("cjk_copy_search_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_cjk_copy_search_missing_text_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("cjk_copy_search_missing_text_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_visual_baseline_render_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("visual_baseline_render_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("visual_baseline_fresh_rendered_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_expected_visual_render_count" `
        -Value (Get-FirstJsonProperty -Object $attempt -Names @("expected_visual_render_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_aggregate_contact_sheet_status" `
        -Value (Get-FirstJsonString -Object $attempt -Names @("aggregate_contact_sheet_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_aggregate_contact_sheet" `
        -Value $aggregateContactSheet
    if (-not [string]::IsNullOrWhiteSpace($aggregateContactSheet)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_gate_attempt_aggregate_contact_sheet_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $aggregateContactSheet)
    }
}

function Get-PdfVisualSegmentedGateEvidenceObject {
    param($Summary)

    $segmented = Get-JsonProperty -Object $Summary -Name "pdf_visual_segmented_gate"
    if ($null -ne $segmented) {
        return $segmented
    }

    $steps = Get-JsonProperty -Object $Summary -Name "steps"
    if ($null -eq $steps) {
        return $null
    }

    return (Get-JsonProperty -Object $steps -Name "pdf_visual_segmented_gate")
}

function Add-PdfVisualSegmentedGateEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary,
        [string]$RepoRoot
    )

    $segmented = Get-PdfVisualSegmentedGateEvidenceObject -Summary $Summary
    if ($null -eq $segmented) {
        return
    }

    $summaryJson = Get-FirstJsonString -Object $segmented -Names @("summary_json")
    if ([string]::IsNullOrWhiteSpace($summaryJson)) {
        $summaryJson = Get-FirstJsonString -Object $Summary -Names @("pdf_visual_segmented_gate_summary_json")
    }
    $status = Get-FirstJsonString -Object $segmented -Names @("status")
    if ([string]::IsNullOrWhiteSpace($summaryJson) -and $status -eq "not_requested") {
        return
    }
    $aggregateContactSheet = Get-FirstJsonString -Object $segmented -Names @("aggregate_contact_sheet")

    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_status" `
        -Value $status
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_verdict" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("verdict"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_full_visual_gate_status" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("full_visual_gate_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_evidence_scope" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("evidence_scope"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_boundary" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("boundary"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_summary_json" `
        -Value $summaryJson
    if (-not [string]::IsNullOrWhiteSpace($summaryJson)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_summary_json_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $summaryJson)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_slice_summary_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("slice_summary_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_slice_pass_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("slice_pass_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_slice_failed_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("slice_failed_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_covered_baseline_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("covered_baseline_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_expected_visual_render_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("expected_visual_render_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_attempt_stage_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("attempt_stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_attempt_passed_stage_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("attempt_passed_stage_count"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_visual_baseline_render_status" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("visual_baseline_render_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_contact_sheet_status" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("aggregate_contact_sheet_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_contact_sheet" `
        -Value $aggregateContactSheet
    if (-not [string]::IsNullOrWhiteSpace($aggregateContactSheet)) {
        Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_contact_sheet_display" `
            -Value (Get-DisplayPath -RepoRoot $RepoRoot -Path $aggregateContactSheet)
    }
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_contact_sheet_bytes" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("aggregate_contact_sheet_bytes"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_rebuild_status" `
        -Value (Get-FirstJsonString -Object $segmented -Names @("aggregate_rebuild_status"))
    Set-OptionalSourceReportField -Target $Target -Name "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count" `
        -Value (Get-FirstJsonProperty -Object $segmented -Names @("aggregate_rebuild_selected_baseline_count"))
}

function Add-ManifestSignoffEntrypointsEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary,
        [string]$RepoRoot
    )

    $signoff = Get-JsonProperty -Object $Summary -Name "manifest_signoff_entrypoints"
    if ($null -eq $signoff) {
        return
    }

    $releaseAssetsManifest = Get-JsonString -Object $signoff -Name "release_assets_manifest"
    $releaseAssetsManifestDisplay = ""
    if (-not [string]::IsNullOrWhiteSpace($releaseAssetsManifest)) {
        try {
            $resolvedManifest = Resolve-RepoPath -RepoRoot $RepoRoot -Path $releaseAssetsManifest -AllowMissing
            $releaseAssetsManifestDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $resolvedManifest
        } catch {
            $normalizedManifest = $releaseAssetsManifest -replace '/', '\'
            $looksRooted = $normalizedManifest.StartsWith('\\') -or $normalizedManifest -match '^[A-Za-z]:\\'
            $releaseAssetsManifestDisplay = if ($normalizedManifest.StartsWith('.\') -or $looksRooted) {
                $normalizedManifest
            } else {
                ".\$normalizedManifest"
            }
        }
    }

    $entrypointEvidence = @(
        foreach ($entrypoint in @(Get-JsonArray -Object $signoff -Name "entrypoints")) {
            $id = Get-JsonString -Object $entrypoint -Name "id"
            if ([string]::IsNullOrWhiteSpace($id)) {
                continue
            }

            [ordered]@{
                id = $id
                required = Get-JsonBool -Object $entrypoint -Name "required"
                path_display = Get-JsonString -Object $entrypoint -Name "path_display"
            }
        }
    )

    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_status" `
        -Value (Get-JsonString -Object $signoff -Name "status")
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_release_assets_manifest" `
        -Value $releaseAssetsManifest
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_release_assets_manifest_display" `
        -Value $releaseAssetsManifestDisplay
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_required_entrypoint_count" `
        -Value (Get-FirstJsonProperty -Object $signoff -Names @("required_entrypoint_count"))
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_entrypoint_ids" `
        -Value @($entrypointEvidence | ForEach-Object { [string]$_.id })
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_entrypoints" `
        -Value @($entrypointEvidence)
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_required_contracts" `
        -Value @(Get-JsonArray -Object $signoff -Name "required_contracts")
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_required_fields" `
        -Value @(Get-JsonArray -Object $signoff -Name "required_fields")
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_checklist_marker" `
        -Value (Get-JsonString -Object $signoff -Name "checklist_marker")
}

function Add-ProjectTemplateReadinessChecklistEntrypointsEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary
    )

    $entrypointsContract = Get-JsonProperty -Object $Summary -Name "project_template_readiness_checklist_entrypoints"
    if ($null -eq $entrypointsContract) {
        return
    }

    $entrypointEvidence = @(
        foreach ($entrypoint in @(Get-JsonArray -Object $entrypointsContract -Name "entrypoints")) {
            $id = Get-JsonString -Object $entrypoint -Name "id"
            if ([string]::IsNullOrWhiteSpace($id)) {
                continue
            }

            [ordered]@{
                id = $id
                required = Get-JsonBool -Object $entrypoint -Name "required"
                path_display = Get-JsonString -Object $entrypoint -Name "path_display"
            }
        }
    )

    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_status" `
        -Value (Get-JsonString -Object $entrypointsContract -Name "status")
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_checklist_label" `
        -Value (Get-JsonString -Object $entrypointsContract -Name "checklist_label")
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_checklist_path" `
        -Value (Get-JsonString -Object $entrypointsContract -Name "checklist_path")
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_required_entrypoint_count" `
        -Value (Get-FirstJsonProperty -Object $entrypointsContract -Names @("required_entrypoint_count"))
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_entrypoint_ids" `
        -Value @($entrypointEvidence | ForEach-Object { [string]$_.id })
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_entrypoints" `
        -Value @($entrypointEvidence)
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_checklist_marker" `
        -Value (Get-JsonString -Object $entrypointsContract -Name "checklist_marker")
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary
    )

    $audit = Get-JsonProperty -Object $Summary -Name "release_entry_project_template_readiness_checklist_material_safety_audit"
    if ($null -eq $audit) {
        return
    }

    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_status" `
        -Value (Get-JsonString -Object $audit -Name "status")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_script" `
        -Value (Get-JsonString -Object $audit -Name "audit_script")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count" `
        -Value (Get-FirstJsonProperty -Object $audit -Names @("audited_entrypoint_count"))
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints" `
        -Value @(Get-JsonArray -Object $audit -Name "audited_entrypoints" | ForEach-Object { [string]$_ })
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label" `
        -Value (Get-JsonString -Object $audit -Name "compact_evidence_label")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field" `
        -Value (Get-JsonString -Object $audit -Name "compact_evidence_field")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema" `
        -Value (Get-JsonString -Object $audit -Name "compact_evidence_source_schema")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path" `
        -Value (Get-JsonString -Object $audit -Name "checklist_path")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker" `
        -Value (Get-JsonString -Object $audit -Name "checklist_marker")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker" `
        -Value (Get-JsonString -Object $audit -Name "material_safety_marker")
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

function Add-ReadinessActionEvidenceMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Item
    )

    $evidenceItems = @(Get-JsonArray -Object $Item -Name "readiness_action_evidence")
    $evidenceCount = Get-JsonString -Object $Item -Name "readiness_action_evidence_count"
    if ([string]::IsNullOrWhiteSpace($evidenceCount)) {
        if ($evidenceItems.Count -eq 0) {
            return
        }

        $evidenceCount = [string]$evidenceItems.Count
    }

    $Lines.Add("  - readiness_action_evidence_count: ``$evidenceCount``") | Out-Null
    if ($evidenceItems.Count -eq 0) {
        return
    }

    $Lines.Add("  - readiness_action_evidence:") | Out-Null
    foreach ($evidence in $evidenceItems) {
        $id = Get-JsonString -Object $evidence -Name "id" -DefaultValue "(unknown evidence)"
        $action = Get-JsonString -Object $evidence -Name "action"
        $issueKey = Get-JsonString -Object $evidence -Name "issue_key"
        $item = Get-JsonString -Object $evidence -Name "item"
        $Lines.Add("    - ``${id}``: action=``$action`` issue_key=``$issueKey`` item=``$item``") | Out-Null
        foreach ($fieldName in @("source_schema", "source_report", "source_report_display", "source_json", "source_json_display")) {
            $fieldValue = Get-JsonString -Object $evidence -Name $fieldName
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                $Lines.Add("      - ${fieldName}: ``$fieldValue``") | Out-Null
            }
        }
    }
}

function Add-ManifestSignoffEntrypointsMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Report
    )

    $status = Get-JsonString -Object $Report -Name "manifest_signoff_entrypoints_status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        return
    }

    foreach ($fieldName in @(
            "manifest_signoff_entrypoints_status",
            "manifest_signoff_entrypoints_release_assets_manifest",
            "manifest_signoff_entrypoints_release_assets_manifest_display",
            "manifest_signoff_entrypoints_required_entrypoint_count",
            "manifest_signoff_entrypoints_entrypoint_ids",
            "manifest_signoff_entrypoints_required_contracts",
            "manifest_signoff_entrypoints_required_fields",
            "manifest_signoff_entrypoints_checklist_marker"
        )) {
        $fieldValue = Get-JsonProperty -Object $Report -Name $fieldName
        $fieldDisplay = if ($fieldValue -is [System.Collections.IEnumerable] -and $fieldValue -isnot [string]) {
            @($fieldValue | ForEach-Object { [string]$_ }) -join ", "
        } else {
            [string]$fieldValue
        }
        if ($null -ne $fieldValue -and -not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
            $Lines.Add("  - ${fieldName}: ``$fieldDisplay``") | Out-Null
        }
    }

    $entrypoints = @(Get-JsonArray -Object $Report -Name "manifest_signoff_entrypoints_entrypoints")
    if ($entrypoints.Count -eq 0) {
        return
    }

    $Lines.Add("  - manifest_signoff_entrypoints:") | Out-Null
    foreach ($entrypoint in $entrypoints) {
        $id = Get-JsonString -Object $entrypoint -Name "id" -DefaultValue "(unknown entrypoint)"
        $required = Get-JsonProperty -Object $entrypoint -Name "required"
        $pathDisplay = Get-JsonString -Object $entrypoint -Name "path_display"
        $Lines.Add("    - ``${id}``: required=``$required`` path_display=``$pathDisplay``") | Out-Null
    }
}

function Add-ProjectTemplateReadinessChecklistEntrypointsMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Report
    )

    $status = Get-JsonString -Object $Report -Name "project_template_readiness_checklist_entrypoints_status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        return
    }

    foreach ($fieldName in @(
            "project_template_readiness_checklist_entrypoints_status",
            "project_template_readiness_checklist_entrypoints_checklist_label",
            "project_template_readiness_checklist_entrypoints_checklist_path",
            "project_template_readiness_checklist_entrypoints_required_entrypoint_count",
            "project_template_readiness_checklist_entrypoints_entrypoint_ids",
            "project_template_readiness_checklist_entrypoints_checklist_marker"
        )) {
        $fieldValue = Get-JsonProperty -Object $Report -Name $fieldName
        $fieldDisplay = if ($fieldValue -is [System.Collections.IEnumerable] -and $fieldValue -isnot [string]) {
            @($fieldValue | ForEach-Object { [string]$_ }) -join ", "
        } else {
            [string]$fieldValue
        }
        if ($null -ne $fieldValue -and -not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
            $Lines.Add("  - ${fieldName}: ``$fieldDisplay``") | Out-Null
        }
    }

    $entrypoints = @(Get-JsonArray -Object $Report -Name "project_template_readiness_checklist_entrypoints_entrypoints")
    if ($entrypoints.Count -eq 0) {
        return
    }

    $Lines.Add("  - project_template_readiness_checklist_entrypoints:") | Out-Null
    foreach ($entrypoint in $entrypoints) {
        $id = Get-JsonString -Object $entrypoint -Name "id" -DefaultValue "(unknown entrypoint)"
        $required = Get-JsonProperty -Object $entrypoint -Name "required"
        $pathDisplay = Get-JsonString -Object $entrypoint -Name "path_display"
        $Lines.Add("    - ``${id}``: required=``$required`` path_display=``$pathDisplay``") | Out-Null
    }
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Report
    )

    $status = Get-JsonString -Object $Report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        return
    }

    foreach ($fieldName in @(
            "release_entry_project_template_readiness_checklist_material_safety_audit_status",
            "release_entry_project_template_readiness_checklist_material_safety_audit_script",
            "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count",
            "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints",
            "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label",
            "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field",
            "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema",
            "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path",
            "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker",
            "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
        )) {
        $fieldValue = Get-JsonProperty -Object $Report -Name $fieldName
        $fieldDisplay = if ($fieldValue -is [System.Collections.IEnumerable] -and $fieldValue -isnot [string]) {
            @($fieldValue | ForEach-Object { [string]$_ }) -join ", "
        } else {
            [string]$fieldValue
        }
        if ($null -ne $fieldValue -and -not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
            $Lines.Add("  - ${fieldName}: ``$fieldDisplay``") | Out-Null
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
    $lines.Add("- source_failure_count: ``$($Summary.source_failure_count)``") | Out-Null
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
            foreach ($fieldName in @(
                    "pdf_visual_gate_status",
                    "pdf_visual_gate_verdict",
                    "pdf_visual_gate_finalizable",
                    "pdf_visual_gate_summary_json_display",
                    "pdf_visual_gate_aggregate_contact_sheet_display",
                    "pdf_visual_gate_cjk_manifest_count",
                    "pdf_visual_gate_cjk_copy_search_count",
                    "pdf_visual_gate_cjk_missing_text_count",
                    "pdf_visual_gate_visual_baseline_manifest_count",
                    "pdf_visual_gate_visual_baseline_count",
                    "pdf_bounded_ctest_summary_count",
                    "pdf_bounded_ctest_pass_count",
                    "pdf_bounded_ctest_skipped_test_count",
                    "pdf_bounded_ctest_selected_test_count",
                    "pdf_bounded_ctest_subsets",
                    "pdf_bounded_ctest_summary_json_display",
                    "pdf_visual_gate_attempt_status",
                    "pdf_visual_gate_attempt_verdict",
                    "pdf_visual_gate_attempt_full_visual_gate_status",
                    "pdf_visual_gate_attempt_evidence_scope",
                    "pdf_visual_gate_attempt_summary_json_display",
                    "pdf_visual_gate_attempt_stage_count",
                    "pdf_visual_gate_attempt_passed_stage_count",
                    "pdf_visual_gate_attempt_failed_stage_count",
                    "pdf_visual_gate_attempt_incomplete_stage_count",
                    "pdf_visual_gate_attempt_outer_guard_status",
                    "pdf_visual_gate_attempt_outer_guard_timed_out",
                    "pdf_visual_gate_attempt_outer_guard_timeout_seconds",
                    "pdf_visual_gate_attempt_pdf_cli_export_status",
                    "pdf_visual_gate_attempt_pdf_regression_status",
                    "pdf_visual_gate_attempt_pdf_regression_selected_test_count",
                    "pdf_visual_gate_attempt_pdf_regression_failed_test_count",
                    "pdf_visual_gate_attempt_pdf_regression_skipped_test_count",
                    "pdf_visual_gate_attempt_unicode_font_status",
                    "pdf_visual_gate_attempt_cjk_copy_search_status",
                    "pdf_visual_gate_attempt_cjk_copy_search_count",
                    "pdf_visual_gate_attempt_cjk_copy_search_missing_text_count",
                    "pdf_visual_gate_attempt_visual_baseline_render_status",
                    "pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count",
                    "pdf_visual_gate_attempt_expected_visual_render_count",
                    "pdf_visual_gate_attempt_aggregate_contact_sheet_status",
                    "pdf_visual_gate_attempt_aggregate_contact_sheet_display",
                    "pdf_visual_segmented_gate_status",
                    "pdf_visual_segmented_gate_verdict",
                    "pdf_visual_segmented_gate_full_visual_gate_status",
                    "pdf_visual_segmented_gate_evidence_scope",
                    "pdf_visual_segmented_gate_boundary",
                    "pdf_visual_segmented_gate_summary_json_display",
                    "pdf_visual_segmented_gate_slice_summary_count",
                    "pdf_visual_segmented_gate_slice_pass_count",
                    "pdf_visual_segmented_gate_slice_failed_count",
                    "pdf_visual_segmented_gate_covered_baseline_count",
                    "pdf_visual_segmented_gate_expected_visual_render_count",
                    "pdf_visual_segmented_gate_attempt_stage_count",
                    "pdf_visual_segmented_gate_attempt_passed_stage_count",
                    "pdf_visual_segmented_gate_visual_baseline_render_status",
                    "pdf_visual_segmented_gate_aggregate_contact_sheet_status",
                    "pdf_visual_segmented_gate_aggregate_contact_sheet_display",
                    "pdf_visual_segmented_gate_aggregate_contact_sheet_bytes",
                    "pdf_visual_segmented_gate_aggregate_rebuild_status",
                    "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count"
                )) {
                $fieldValue = Get-JsonProperty -Object $report -Name $fieldName
                $fieldDisplay = if ($fieldValue -is [System.Collections.IEnumerable] -and $fieldValue -isnot [string]) {
                    @($fieldValue | ForEach-Object { [string]$_ }) -join ", "
                } else {
                    [string]$fieldValue
                }
                if ($null -ne $fieldValue -and -not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
                    $lines.Add("  - ${fieldName}: ``$fieldDisplay``") | Out-Null
                }
            }
            Add-ManifestSignoffEntrypointsMarkdownLines -Lines $lines -Report $report
            Add-ProjectTemplateReadinessChecklistEntrypointsMarkdownLines -Lines $lines -Report $report
            Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditMarkdownLines -Lines $lines -Report $report
            $controlledVisualSmokeAvailable = Get-JsonProperty -Object $report -Name "controlled_visual_smoke_available"
            if ($null -ne $controlledVisualSmokeAvailable) {
                $lines.Add("  - controlled_visual_smoke_available: ``$controlledVisualSmokeAvailable``") | Out-Null
            }
            $controlledVisualSmokeStatus = Get-JsonString -Object $report -Name "controlled_visual_smoke_status"
            if (-not [string]::IsNullOrWhiteSpace($controlledVisualSmokeStatus)) {
                $lines.Add("  - controlled_visual_smoke_status: ``$controlledVisualSmokeStatus``") | Out-Null
            }
            $controlledVisualSmokePassed = Get-JsonProperty -Object $report -Name "controlled_visual_smoke_passed"
            if ($null -ne $controlledVisualSmokePassed) {
                $lines.Add("  - controlled_visual_smoke_passed: ``$controlledVisualSmokePassed``") | Out-Null
            }
            $controlledVisualSmokeCaseCount = Get-JsonProperty -Object $report -Name "controlled_visual_smoke_case_count"
            if ($null -ne $controlledVisualSmokeCaseCount) {
                $lines.Add("  - controlled_visual_smoke_case_count: ``$controlledVisualSmokeCaseCount``") | Out-Null
            }
            $controlledVisualSmokeJsonDisplay = Get-JsonString -Object $report -Name "controlled_visual_smoke_json_display"
            if (-not [string]::IsNullOrWhiteSpace($controlledVisualSmokeJsonDisplay)) {
                $lines.Add("  - controlled_visual_smoke_json_display: ``$controlledVisualSmokeJsonDisplay``") | Out-Null
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
            Add-ReadinessActionEvidenceMarkdownLines -Lines $lines -Item $blocker
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
            Add-ReadinessActionEvidenceMarkdownLines -Lines $lines -Item $item
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
    $sourceReportStatus = ""
    $sourceReportReleaseReady = ""
    $latestSchemaApprovalGateStatus = ""
    $schemaApprovalStatusSummary = @()
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
                input_docx = Get-JsonString -Object $blocker -Name "input_docx"
                input_docx_display = Get-JsonString -Object $blocker -Name "input_docx_display"
                schema_target = Get-JsonString -Object $blocker -Name "schema_target"
                target_mode = Get-JsonString -Object $blocker -Name "target_mode"
                repair_strategy = Get-JsonString -Object $blocker -Name "repair_strategy"
                repair_hint = Get-JsonString -Object $blocker -Name "repair_hint"
                command_template = Get-JsonString -Object $blocker -Name "command_template"
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
                    "readiness_action_evidence"
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
                input_docx = Get-JsonString -Object $item -Name "input_docx"
                input_docx_display = Get-JsonString -Object $item -Name "input_docx_display"
                schema_target = Get-JsonString -Object $item -Name "schema_target"
                target_mode = Get-JsonString -Object $item -Name "target_mode"
                repair_strategy = Get-JsonString -Object $item -Name "repair_strategy"
                repair_hint = Get-JsonString -Object $item -Name "repair_hint"
                command_template = Get-JsonString -Object $item -Name "command_template"
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
                    "readiness_action_evidence"
                )
            Normalize-ReadinessActionEvidence `
                -Items (Get-JsonArray -Object $rollupActionItem -Name "readiness_action_evidence") `
                -DefaultSourceSchema ([string]$rollupActionItem.source_schema) `
                -DefaultSourceReport ([string]$rollupActionItem.origin_source_report) `
                -DefaultSourceReportDisplay ([string]$rollupActionItem.origin_source_report_display) `
                -DefaultSourceJson ([string]$rollupActionItem.source_json) `
                -DefaultSourceJsonDisplay ([string]$rollupActionItem.source_json_display) `
                -RepoRoot $repoRoot
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
                input_docx = Get-JsonString -Object $warning -Name "input_docx"
                input_docx_display = Get-JsonString -Object $warning -Name "input_docx_display"
                schema_target = Get-JsonString -Object $warning -Name "schema_target"
                target_mode = Get-JsonString -Object $warning -Name "target_mode"
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
            "full_visual_gate_status",
            "controlled_visual_smoke_available",
            "controlled_visual_smoke_status",
            "controlled_visual_smoke_passed",
            "controlled_visual_smoke_case_count",
            "controlled_visual_smoke_json",
            "controlled_visual_smoke_json_display"
        )
    Add-PdfVisualGateEvidenceFields -Target $sourceReport -Summary $summaryObject -RepoRoot $repoRoot
    Add-PdfBoundedCtestEvidenceFields -Target $sourceReport -Summary $summaryObject
    Add-PdfVisualGateAttemptEvidenceFields -Target $sourceReport -Summary $summaryObject -RepoRoot $repoRoot
    Add-PdfVisualSegmentedGateEvidenceFields -Target $sourceReport -Summary $summaryObject -RepoRoot $repoRoot
    Add-ManifestSignoffEntrypointsEvidenceFields -Target $sourceReport -Summary $summaryObject -RepoRoot $repoRoot
    Add-ProjectTemplateReadinessChecklistEntrypointsEvidenceFields -Target $sourceReport -Summary $summaryObject
    Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceFields -Target $sourceReport -Summary $summaryObject
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
