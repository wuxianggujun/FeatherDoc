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

function Expand-InputPathList {
    param([string[]]$Paths)

    return @(
        foreach ($path in @($Paths)) {
            foreach ($part in ([string]$path -split ",")) {
                $trimmed = $part.Trim()
                if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
                    $trimmed
                }
            }
        }
    )
}

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @(Expand-InputPathList -Paths $ExplicitPaths)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
        }
    }

    $scanRoots = @(Expand-InputPathList -Paths $Roots | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
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

function Get-DefaultOpenCommandForReportKind {
    param([string]$Kind)

    switch ($Kind) {
        "featherdoc.document_skeleton_governance_rollup_report.v1" {
            return "pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1"
        }
        "featherdoc.numbering_catalog_governance_report.v1" {
            return "pwsh -ExecutionPolicy Bypass -File .\scripts\build_numbering_catalog_governance_report.ps1"
        }
        "featherdoc.table_layout_delivery_governance_report.v1" {
            return "pwsh -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1"
        }
        "featherdoc.content_control_data_binding_governance_report.v1" {
            return "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
        }
        "featherdoc.project_template_delivery_readiness_report.v1" {
            return "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1"
        }
        "featherdoc.schema_patch_confidence_calibration_report.v1" {
            return "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
        }
        default {
            return ""
        }
    }
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

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Release Blocker Rollup Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Source reports: ``$($Summary.source_report_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($Summary.action_item_count)``") | Out-Null
    $lines.Add("- Warnings: ``$($Summary.warning_count)``") | Out-Null
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
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.source_json_display)) {
                $lines.Add("  - source_json_display: ``$($blocker.source_json_display)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.message)) {
                $lines.Add("  - $($blocker.message)") | Out-Null
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
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.source_json_display)) {
                $lines.Add("  - source_json_display: ``$($item.source_json_display)``") | Out-Null
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
$sourceIndex = 0

foreach ($path in @($inputPaths)) {
    $sourceIndex++
    $kind = "unknown"
    $status = "loaded"
    $errorMessage = ""
    try {
        $summaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $kind = Get-ReportKind -Summary $summaryObject
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
            $blockers.Add([ordered]@{
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
            }) | Out-Null
        }

        $actionIndex = 0
        foreach ($item in $sourceActions) {
            $actionIndex++
            $id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
            $sourceJson = Get-JsonString -Object $item -Name "source_json"
            $sourceJsonDisplay = Get-JsonString -Object $item -Name "source_json_display"
            $originSourceReport = Get-JsonString -Object $item -Name "source_report"
            $originSourceReportDisplay = Get-JsonString -Object $item -Name "source_report_display"
            $actionItems.Add([ordered]@{
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
                open_command = Get-FirstJsonString `
                    -Object $item `
                    -Names @("open_command", "command") `
                    -DefaultValue (Get-DefaultOpenCommandForReportKind -Kind $kind)
            }) | Out-Null
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

    $sourceReports.Add([ordered]@{
        path = $path
        path_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
        schema = $kind
        status = $status
        error = $errorMessage
    }) | Out-Null
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
