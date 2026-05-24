<#
.SYNOPSIS
Audits release-facing FeatherDoc materials for draft wording and local path leaks.

.DESCRIPTION
Scans one or more files or directories and fails if release-facing text files
contain draft/草稿 wording or machine-local absolute paths. Directories are
scanned recursively but only text-like files are considered.

.PARAMETER Path
One or more files or directories to scan.

.PARAMETER AdditionalForbiddenPattern
Optional extra regex patterns to audit in addition to the built-in rules.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\assert_release_material_safety.ps1 `
    -Path .\output\release-candidate-checks\START_HERE.md, .\output\release-candidate-checks\report
#>
param(
    [Parameter(Mandatory = $true)]
    [string[]]$Path,
    [string[]]$AdditionalForbiddenPattern = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[assert-release-material-safety] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-FullPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    return [System.IO.Path]::GetFullPath($candidate)
}

function Test-TextLikeFile {
    param([System.IO.FileInfo]$File)

    $textExtensions = @(
        ".md",
        ".txt",
        ".json",
        ".xml",
        ".yml",
        ".yaml",
        ".csv",
        ".log",
        ".rst"
    )

    return $textExtensions -contains $File.Extension.ToLowerInvariant()
}

function Get-ScanFiles {
    param(
        [string]$RepoRoot,
        [string[]]$InputPaths
    )

    $files = [System.Collections.Generic.List[string]]::new()
    foreach ($inputPath in $InputPaths) {
        $resolvedPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $inputPath
        if (-not (Test-Path -LiteralPath $resolvedPath)) {
            throw "Audit path does not exist: $resolvedPath"
        }

        $item = Get-Item -LiteralPath $resolvedPath
        if ($item.PSIsContainer) {
            foreach ($file in Get-ChildItem -LiteralPath $resolvedPath -Recurse -File) {
                if (Test-TextLikeFile -File $file) {
                    [void]$files.Add($file.FullName)
                }
            }
        } else {
            [void]$files.Add($item.FullName)
        }
    }

    return @($files | Sort-Object -Unique)
}

function New-Rule {
    param(
        [string]$Label,
        [string]$Pattern
    )

    return [ordered]@{
        label = $Label
        pattern = $Pattern
    }
}

function Get-JsonPropertyValue {
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

function Get-JsonArray {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-JsonPropertyValue -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }

    if ($value -is [string]) {
        return @($value)
    }

    if ($value -is [System.Collections.IDictionary]) {
        return @($value)
    }

    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Get-JsonObjectNodes {
    param($Value)

    if ($null -eq $Value) {
        return
    }

    if ($Value -is [string]) {
        return
    }

    if ($Value -is [System.Collections.IDictionary]) {
        Write-Output $Value
        foreach ($key in $Value.Keys) {
            Get-JsonObjectNodes -Value $Value[$key]
        }

        return
    }

    if ($Value -is [pscustomobject]) {
        Write-Output $Value
        foreach ($property in $Value.PSObject.Properties) {
            Get-JsonObjectNodes -Value $property.Value
        }

        return
    }

    if ($Value -is [System.Collections.IEnumerable]) {
        foreach ($item in $Value) {
            Get-JsonObjectNodes -Value $item
        }
    }
}

function Add-AuditViolation {
    param(
        $Violations,
        [string]$File,
        [string]$Label,
        [string]$Text
    )

    [void]$Violations.Add([ordered]@{
        file = $File
        line = 1
        label = $Label
        text = $Text
    })
}

function Test-TextContainsAny {
    param(
        [string]$Text,
        [string[]]$Needles
    )

    foreach ($needle in $Needles) {
        if (-not [string]::IsNullOrWhiteSpace($needle) -and $Text.Contains($needle)) {
            return $true
        }
    }

    return $false
}

function Add-ReleaseEntryDocumentGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md")) {
        return
    }

    $governanceMarkers = @(
        "content_control_data_binding.bound_placeholder",
        "project_template_delivery_readiness",
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance",
        "table_layout_delivery_governance.delivery_quality",
        "real_corpus_confidence",
        "delivery_quality",
        "Release Governance Rollup Details",
        "Release Governance Handoff Details"
    )
    if (-not (Test-TextContainsAny -Text $Content -Needles $governanceMarkers)) {
        return
    }

    $label = "release entry governance trace"

    if ($Content.Contains("content_control_data_binding.bound_placeholder")) {
        foreach ($needle in @(
            "featherdoc.content_control_data_binding_governance_report.v1",
            "source_json_display",
            "input_docx",
            "template_name",
            "schema_target",
            "target_mode",
            "repair_strategy",
            "repair_hint",
            "Rerun Custom XML sync",
            "sync_bound_content_control",
            "command_template",
            "sync-content-controls-from-custom-xml"
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Entry document mentions content_control_data_binding.bound_placeholder without required repair workflow marker '$needle'."
            }
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_delivery_readiness", "project_template_onboarding_governance")) {
        foreach ($needle in @(
            "project_template_delivery_readiness",
            "project_template_delivery_readiness_contract",
            "featherdoc.project_template_delivery_readiness_report.v1",
            "latest_schema_approval_gate_status",
            "source_report_display",
            "source_json_display"
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Entry document lost project template delivery readiness marker '$needle'."
            }
        }

        foreach ($needle in @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance_contract",
            "featherdoc.project_template_onboarding_governance_report.v1",
            "schema_approval_status_summary",
            "source_report_display",
            "source_json_display"
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Entry document lost project template onboarding governance marker '$needle'."
            }
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @("delivery_quality", "table_layout_delivery_governance")) {
        if (-not $Content.Contains("table_layout_delivery_governance.delivery_quality")) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document mentions delivery_quality without table_layout_delivery_governance.delivery_quality."
        }
    }

    if ($Content.Contains("numbering_catalog_governance.real_corpus_confidence")) {
        foreach ($needle in @(
            "catalog_coverage_percent",
            "baseline_coverage_percent",
            "coverage_score",
            "matched_document_count",
            "unmatched_catalog_document_count",
            "unmatched_baseline_document_count",
            "alignment_gap_count",
            "catalog_document_keys",
            "baseline_document_keys",
            "matched_document_keys",
            "penalty_summary",
            "style_numbering_issues"
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Entry document mentions numbering_catalog_governance.real_corpus_confidence without required detail marker '$needle'."
            }
        }
    }

    if ($Content.Contains("table_layout_delivery_governance.delivery_quality")) {
        foreach ($needle in @(
            "table_style_issue_count",
            "automatic_tblLook_fix_count",
            "manual_table_style_fix_count",
            "table_position_automatic_count",
            "table_position_review_count",
            "command_failure_count",
            "ready_document_percent",
            "unresolved_item_count",
            "penalty_summary",
            "floating_table_plans_pending"
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Entry document mentions table_layout_delivery_governance.delivery_quality without required detail marker '$needle'."
            }
        }
    }
}

function Add-ReleaseSummaryProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_summary.zh-cn.md") {
        return
    }

    $label = "release summary project template governance trace"

    if ($Content.Contains("project-template readiness governance contract")) {
        foreach ($needle in @(
            "project-template readiness governance contract",
            "status=",
            "release_ready=",
            "latest_schema_approval_gate_status=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release summary lost project template readiness trace marker '$needle'."
            }
        }
    }

    if ($Content.Contains("project-template onboarding governance contract")) {
        foreach ($needle in @(
            "project-template onboarding governance contract",
            "schema_approval_status_summary=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release summary lost project template onboarding trace marker '$needle'."
            }
        }
    }
}

function Add-ReleaseBodyProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_body.zh-cn.md") {
        return
    }

    $label = "release body project template governance trace"

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_delivery_readiness:", "project_template_delivery_readiness_contract")) {
        foreach ($needle in @(
            "project_template_delivery_readiness",
            "project_template_delivery_readiness_contract",
            "source_schema=featherdoc.project_template_delivery_readiness_report.v1",
            "status=",
            "release_ready=",
            "latest_schema_approval_gate_status=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release body lost project template readiness trace marker '$needle'."
            }
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_onboarding.schema_approval", "project_template_onboarding_governance_contract")) {
        foreach ($needle in @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance",
            "project_template_onboarding_governance_contract",
            "source_schema=featherdoc.project_template_onboarding_governance_report.v1",
            "schema_approval_status_summary=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release body lost project template onboarding trace marker '$needle'."
            }
        }
    }
}

function Add-ReleaseHandoffProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_handoff.md") {
        return
    }

    $label = "release handoff project template governance trace"

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_delivery_readiness", "project_template_delivery_readiness_contract")) {
        foreach ($needle in @(
            "project_template_delivery_readiness",
            "project_template_delivery_readiness_contract",
            "source_schema: featherdoc.project_template_delivery_readiness_report.v1",
            "status:",
            "release_ready:",
            "latest_schema_approval_gate_status:",
            "source_report_display:",
            "source_json_display:"
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release handoff lost project template readiness trace marker '$needle'."
            }
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_onboarding.schema_approval", "project_template_onboarding_governance")) {
        foreach ($needle in @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance_contract",
            "source_schema: featherdoc.project_template_onboarding_governance_report.v1",
            "schema_approval_status_summary:",
            "source_report_display:",
            "source_json_display:"
        )) {
            if (-not $Content.Contains($needle)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release handoff lost project template onboarding trace marker '$needle'."
            }
        }
    }
}

function Add-ContentControlRepairContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $contentControlBlockerId = "content_control_data_binding.bound_placeholder"
    $expectedRepairStrategy = "sync_bound_content_control"
    $expectedRepairHintMarker = "Rerun Custom XML sync"
    $expectedCommand = "sync-content-controls-from-custom-xml"
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
                try {
                    $declaredCount = [int]$countValue
                    if ($declaredCount -ne $contracts.Count) {
                        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "content_control_repair_contract_count does not match content_control_repair_contracts length."
                    }
                } catch {
                    Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "content_control_repair_contract_count must be an integer."
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

        if ([string]$manifestValue -ne [string]$sourceValue) {
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

    foreach ($marker in @("release_handoff", "artifact_guide", "reviewer_checklist", "governance_metrics")) {
        if ($null -ne (Get-JsonPropertyValue -Object $Json -Name $marker)) {
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
        try {
            $declaredCount = [int]$countValue
            if ($declaredCount -ne $metrics.Count) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "governance_metric_count does not match governance_metrics length."
            }
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "governance_metric_count must be an integer."
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
                    "unresolved_item_count"
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

    $latestGateStatus = Get-JsonPropertyValue -Object $contract -Name "latest_schema_approval_gate_status"
    if ([string]::IsNullOrWhiteSpace([string]$latestGateStatus)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.latest_schema_approval_gate_status is missing."
    }

    $sourceJsonDisplay = Get-JsonPropertyValue -Object $contract -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceJsonDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.source_json_display is missing."
    }

    $sourceReportDisplay = Get-JsonPropertyValue -Object $contract -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceReportDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.source_report_display is missing."
    }

    $releaseReady = Get-JsonPropertyValue -Object $contract -Name "release_ready"
    if ($null -eq $releaseReady) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.release_ready is missing."
    }

    $integerValues = @{}
    foreach ($fieldName in @(
        "schema_history_blocked_run_count",
        "schema_history_pending_run_count",
        "schema_history_passed_run_count",
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

        try {
            $integerValues[$fieldName] = [int]$fieldValue
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.$fieldName must be an integer."
        }
    }

    $releaseReadyIsFalse = $false
    if ($releaseReady -is [bool]) {
        $releaseReadyIsFalse = -not $releaseReady
    } elseif ($null -ne $releaseReady) {
        $releaseReadyIsFalse = ([string]$releaseReady).Trim().ToLowerInvariant() -eq "false"
    }

    if ($releaseReadyIsFalse -and $integerValues.ContainsKey("release_blocker_count") -and $integerValues["release_blocker_count"] -le 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.release_blocker_count must be greater than 0 when release_ready is false."
    }
}

function Add-ProjectTemplateOnboardingGovernanceContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "project template onboarding governance contract"
    $contract = Get-JsonPropertyValue -Object $Json -Name "project_template_onboarding_governance_contract"
    if ($null -eq $contract) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing project_template_onboarding_governance_contract."
        return
    }

    $schema = Get-JsonPropertyValue -Object $contract -Name "schema"
    if ([string]$schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.schema is invalid."
    }

    $sourceSchema = Get-JsonPropertyValue -Object $contract -Name "source_schema"
    if ([string]$sourceSchema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.source_schema is invalid."
    }

    $status = Get-JsonPropertyValue -Object $contract -Name "status"
    if ([string]::IsNullOrWhiteSpace([string]$status)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.status is missing."
    }

    $sourceJsonDisplay = Get-JsonPropertyValue -Object $contract -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceJsonDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.source_json_display is missing."
    }

    $sourceReportDisplay = Get-JsonPropertyValue -Object $contract -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceReportDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.source_report_display is missing."
    }

    $releaseReady = Get-JsonPropertyValue -Object $contract -Name "release_ready"
    if ($null -eq $releaseReady) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.release_ready is missing."
    }

    $statusSummary = Get-JsonPropertyValue -Object $contract -Name "schema_approval_status_summary"
    if ($null -eq $statusSummary) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.schema_approval_status_summary is missing."
    }

    $integerValues = @{}
    foreach ($fieldName in @(
        "source_file_count",
        "source_failure_count",
        "entry_count",
        "blocked_entry_count",
        "pending_review_entry_count",
        "not_evaluated_entry_count",
        "approved_entry_count",
        "not_required_entry_count",
        "release_blocker_count",
        "action_item_count",
        "manual_review_recommendation_count"
    )) {
        $fieldValue = Get-JsonPropertyValue -Object $contract -Name $fieldName
        if ($null -eq $fieldValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.$fieldName is missing."
            continue
        }

        try {
            $integerValues[$fieldName] = [int]$fieldValue
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.$fieldName must be an integer."
        }
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
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.status must be ready when release_ready is true."
    }

    if ($integerValues.ContainsKey("entry_count")) {
        $entryStatusCountFields = @(
            "blocked_entry_count",
            "pending_review_entry_count",
            "not_evaluated_entry_count",
            "approved_entry_count",
            "not_required_entry_count"
        )
        $hasAllEntryStatusCounts = $true
        $entryStatusCountTotal = 0
        foreach ($fieldName in $entryStatusCountFields) {
            if (-not $integerValues.ContainsKey($fieldName)) {
                $hasAllEntryStatusCounts = $false
                break
            }
            $entryStatusCountTotal += $integerValues[$fieldName]
        }

        if ($hasAllEntryStatusCounts -and $entryStatusCountTotal -ne $integerValues["entry_count"]) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract entry status counts do not match entry_count."
        }
    }

    if ($releaseReadyIsFalse -and
        $integerValues.ContainsKey("release_blocker_count") -and
        $integerValues.ContainsKey("source_failure_count") -and
        (($integerValues["release_blocker_count"] + $integerValues["source_failure_count"]) -le 0)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract must have release blockers or source failures when release_ready is false."
    }
}

function Add-PdfVisualGateManifestContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "PDF visual gate manifest contract"
    $includedValue = Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_evidence_included"
    $status = [string](Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_status")
    $included = $false
    if ($includedValue -is [bool]) {
        $included = $includedValue
    } elseif ($null -ne $includedValue) {
        $included = ([string]$includedValue).Trim().ToLowerInvariant() -eq "true"
    }

    if (-not $included) {
        if ($status -eq "loaded") {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_status=loaded requires pdf_visual_gate_evidence_included=true."
        }
        return
    }

    if ($status -ne "loaded") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence_included=true requires pdf_visual_gate_status=loaded."
    }

    $topLevelSummary = [string](Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_summary_json")
    if ([string]::IsNullOrWhiteSpace($topLevelSummary)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_summary_json is missing."
    }

    $topLevelOutputDir = [string](Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_output_dir")
    if ([string]::IsNullOrWhiteSpace($topLevelOutputDir)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_output_dir is missing."
    }

    $evidence = Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_evidence"
    if ($null -eq $evidence) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence is missing."
        return
    }

    $evidenceStatus = [string](Get-JsonPropertyValue -Object $evidence -Name "status")
    if ($evidenceStatus -ne "loaded") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.status must be loaded."
    }

    $evidenceVerdict = ([string](Get-JsonPropertyValue -Object $evidence -Name "verdict")).Trim().ToLowerInvariant()
    if ([string]::IsNullOrWhiteSpace($evidenceVerdict)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.verdict is missing."
    } elseif ($evidenceVerdict -notin @("pass", "fail")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.verdict must be pass or fail."
    }

    $evidenceSummary = [string](Get-JsonPropertyValue -Object $evidence -Name "summary_json")
    if ([string]::IsNullOrWhiteSpace($evidenceSummary)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.summary_json is missing."
    } elseif (-not [string]::IsNullOrWhiteSpace($topLevelSummary) -and $evidenceSummary -ne $topLevelSummary) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.summary_json must match pdf_visual_gate_summary_json."
    }

    foreach ($fieldName in @(
            "aggregate_contact_sheet",
            "pdf_cli_export_log",
            "pdf_regression_log",
            "cjk_copy_search_log_dir",
            "unicode_font_log"
        )) {
        $fieldValue = [string](Get-JsonPropertyValue -Object $evidence -Name $fieldName)
        if ([string]::IsNullOrWhiteSpace($fieldValue)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName is missing."
        }
    }

    foreach ($countContract in @(
            @{ Name = "cjk_copy_search_count"; Minimum = 1 },
            @{ Name = "cjk_missing_text_count"; Minimum = 0 },
            @{ Name = "visual_baseline_count"; Minimum = 1 }
        )) {
        $fieldName = $countContract.Name
        $fieldValue = Get-JsonPropertyValue -Object $evidence -Name $fieldName
        if ($null -eq $fieldValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName is missing."
            continue
        }

        try {
            $integerValue = [int]$fieldValue
            if ($integerValue -lt [int]$countContract.Minimum) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName must be at least $($countContract.Minimum)."
            }
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName must be an integer."
        }
    }
}

$repoRoot = Resolve-RepoRoot
$scanFiles = @(Get-ScanFiles -RepoRoot $repoRoot -InputPaths $Path)
if ($scanFiles.Count -eq 0) {
    Write-Step "No text-like files matched the requested paths."
    exit 0
}

$rules = @(
    (New-Rule `
        -Label "draft wording" `
        -Pattern '(?i)发布说明草稿|请在发布前补齐|草稿|release body draft|release-note drafts|release drafts|public release drafts|draft release notes|still drafting')
    (New-Rule `
        -Label "local absolute path" `
        -Pattern '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|]+|(?<!\w)/(?:Users|home)/[^\s"''`<>|]+')
)

foreach ($extraPattern in $AdditionalForbiddenPattern) {
    if (-not [string]::IsNullOrWhiteSpace($extraPattern)) {
        $rules += New-Rule -Label "custom forbidden pattern" -Pattern $extraPattern
    }
}

$violations = [System.Collections.Generic.List[object]]::new()
foreach ($file in $scanFiles) {
    foreach ($rule in $rules) {
        $matches = Select-String -LiteralPath $file -Pattern $rule.pattern -AllMatches
        foreach ($match in $matches) {
            [void]$violations.Add([ordered]@{
                file = $file
                line = $match.LineNumber
                label = $rule.label
                text = $match.Line.Trim()
            })
        }
    }

    if ([System.IO.Path]::GetExtension($file).ToLowerInvariant() -eq ".md") {
        $content = Get-Content -Raw -LiteralPath $file
        Add-ReleaseEntryDocumentGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseSummaryProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBodyProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseHandoffProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
    }

    if ([System.IO.Path]::GetExtension($file).ToLowerInvariant() -eq ".json") {
        $leafName = (Split-Path -Leaf $file).ToLowerInvariant()
        try {
            $json = Get-Content -Raw -LiteralPath $file | ConvertFrom-Json
            Add-GovernanceMetricContractViolations -File $file -Json $json -Violations $violations
            Add-ContentControlRepairContractViolations -File $file -Json $json -Violations $violations
            Add-ProjectTemplateDeliveryReadinessContractViolations -File $file -Json $json -Violations $violations
            Add-ProjectTemplateOnboardingGovernanceContractViolations -File $file -Json $json -Violations $violations
            Add-PdfVisualGateManifestContractViolations -File $file -Json $json -Violations $violations
        } catch {
            if ($leafName -eq "summary.json" -or $leafName -eq "release_assets_manifest.json") {
                Add-AuditViolation `
                    -Violations $violations `
                    -File $file `
                    -Label "governance metrics contract" `
                    -Text "Release governance JSON could not be parsed."
            }
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Step "Detected forbidden release material content:"
    foreach ($violation in $violations) {
        Write-Host ("- [{0}] {1}:{2}" -f $violation.label, $violation.file, $violation.line)
        Write-Host ("  {0}" -f $violation.text)
    }

    throw "Release material safety audit failed with $($violations.Count) violation(s)."
}

Write-Step ("Audit passed for {0} file(s)." -f $scanFiles.Count)
