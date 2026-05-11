<#
.SYNOPSIS
Builds a table layout delivery rollup from existing report summaries.

.DESCRIPTION
Aggregates table layout delivery summaries into one JSON/Markdown handoff for
multi-template table style quality, safe tblLook repair planning, floating table
preset review, release blockers, and action items. The script is read-only for
source reports and does not rerun CLI, CMake, Word, or visual automation.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/table-layout-delivery-rollup",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnIssue,
    [switch]$FailOnBlocker
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[table-layout-delivery-rollup] $Message"
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

    try {
        $resolved = [System.IO.Path]::GetFullPath($Path)
        if ($resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = $resolved.Substring($root.Length).TrimStart('\', '/')
            if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
            return ".\" + ($relative -replace '/', '\')
        }
        return $resolved
    } catch {
        return $Path
    }
}

function Get-ReportPathDisplay {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    return Get-DisplayPath -RepoRoot $RepoRoot -Path $candidate
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
        if (-not [string]::IsNullOrWhiteSpace($value)) { return $value }
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
    if ($value -is [System.Collections.IDictionary]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @($ExplicitPaths)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
        }
    }

    $scanRoots = @($Roots | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @("output/table-layout-delivery")
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) { continue }

        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File |
                Where-Object { $_.Name -in @("summary.json", "table_layout_delivery.summary.json") } |
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
    return "unknown"
}

function Get-DocumentName {
    param([string]$DocumentPath, [int]$Index)

    if ([string]::IsNullOrWhiteSpace($DocumentPath)) {
        return "document_$Index"
    }

    $normalized = $DocumentPath -replace '/', '\'
    $leaf = Split-Path -Leaf $normalized
    if ([string]::IsNullOrWhiteSpace($leaf)) { return $DocumentPath }
    return $leaf
}

function Add-SummaryGroup {
    param([object[]]$Items, [string]$PropertyName, [string]$OutputName)

    return @(
        foreach ($group in @($Items | Group-Object $PropertyName |
            Sort-Object -Property @{ Expression = "Count"; Descending = $true },
                @{ Expression = "Name"; Ascending = $true })) {
            $summary = [ordered]@{}
            $summary[$OutputName] = [string]$group.Name
            $summary["count"] = [int]$group.Count
            $summary
        }
    )
}

function Add-IssueSummary {
    param([object[]]$Items)

    $totals = @{}
    foreach ($item in @($Items)) {
        $issue = Get-JsonString -Object $item -Name "issue" -DefaultValue "unspecified"
        if ([string]::IsNullOrWhiteSpace($issue)) { $issue = "unspecified" }
        $count = Get-JsonInt -Object $item -Name "count" -DefaultValue 1
        if ($count -lt 1) { $count = 1 }
        if (-not $totals.ContainsKey($issue)) { $totals[$issue] = 0 }
        $totals[$issue] = [int]$totals[$issue] + $count
    }

    $rows = New-Object 'System.Collections.Generic.List[object]'
    foreach ($key in $totals.Keys) {
        $rows.Add([pscustomobject]@{
            issue = [string]$key
            count = [int]$totals[$key]
        }) | Out-Null
    }

    return @($rows.ToArray() |
        Sort-Object -Property @{ Expression = { $_.count }; Descending = $true },
            @{ Expression = { $_.issue }; Ascending = $true })
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Table Layout Delivery Rollup Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Ready: ``$($Summary.ready)``") | Out-Null
    $lines.Add("- Source reports: ``$($Summary.source_report_count)``") | Out-Null
    $lines.Add("- Documents: ``$($Summary.document_count)``") | Out-Null
    $lines.Add("- Source read failures: ``$($Summary.source_failure_count)``") | Out-Null
    $lines.Add("- Source report failures: ``$($Summary.source_report_failure_count)``") | Out-Null
    $lines.Add("- Table style issues: ``$($Summary.total_table_style_issue_count)``") | Out-Null
    $lines.Add("- Automatic tblLook fixes: ``$($Summary.total_automatic_tblLook_fix_count)``") | Out-Null
    $lines.Add("- Manual table style fixes: ``$($Summary.total_manual_table_style_fix_count)``") | Out-Null
    $lines.Add("- Floating table automatic plans: ``$($Summary.total_table_position_automatic_count)``") | Out-Null
    $lines.Add("- Floating table review plans: ``$($Summary.total_table_position_review_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Documents") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.document_entries).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($document in @($Summary.document_entries)) {
            $lines.Add(("- ``{0}``: status=``{1}`` preset=``{2}`` issues=``{3}`` tblLook=``{4}`` position_review=``{5}`` source=``{6}``" -f
                $document.document_name,
                $document.status,
                $document.preset,
                $document.table_style_issue_count,
                $document.automatic_tblLook_fix_count,
                $document.table_position_review_count,
                $document.source_report_display)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Presets") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.preset_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($preset in @($Summary.preset_summary)) {
            $lines.Add(("- ``{0}``: {1}" -f $preset.preset, $preset.count)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Table Style Issue Summary") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.table_style_issue_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($issue in @($Summary.table_style_issue_summary)) {
            $lines.Add(("- ``{0}``: {1}" -f $issue.issue, $issue.count)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Floating Table Plans") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.table_position_plans).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($plan in @($Summary.table_position_plans)) {
            $lines.Add(("- ``{0}``: preset=``{1}`` plan=``{2}`` automatic=``{3}`` review=``{4}``" -f
                $plan.document_name,
                $plan.preset,
                $plan.table_position_plan_display,
                $plan.automatic_count,
                $plan.review_count)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add(("- ``{0}``: document=``{1}`` action=``{2}`` source=``{3}``" -f
                $blocker.composite_id,
                $blocker.document_name,
                $blocker.action,
                $blocker.source_report_display)) | Out-Null
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
            $lines.Add(("- ``{0}``: document=``{1}`` title=``{2}``" -f
                $item.composite_id,
                $item.document_name,
                $item.title)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.warnings).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($warning in @($Summary.warnings)) {
            $lines.Add(("- ``{0}``: source=``{1}`` {2}" -f
                $warning.id,
                $warning.source_report_display,
                $warning.message)) | Out-Null
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
    Join-Path $resolvedOutputDir "table_layout_delivery_rollup.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) table layout delivery summary file(s)"

$sourceReports = New-Object 'System.Collections.Generic.List[object]'
$documents = New-Object 'System.Collections.Generic.List[object]'
$positionPlans = New-Object 'System.Collections.Generic.List[object]'
$issueRows = New-Object 'System.Collections.Generic.List[object]'
$blockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'
$sourceIndex = 0
$totalIssueCount = 0
$totalAutomaticFixCount = 0
$totalManualFixCount = 0
$totalPositionAutomaticCount = 0
$totalPositionReviewCount = 0
$totalPositionAlreadyMatchingCount = 0
$totalCommandFailureCount = 0

foreach ($path in @($inputPaths)) {
    $sourceIndex++
    $kind = "unknown"
    $sourceStatus = "loaded"
    $errorMessage = ""
    $documentName = ""
    $inputDocx = ""
    $inputDocxDisplay = ""
    $issueCount = 0
    $releaseBlockerCount = 0

    try {
        $summaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $kind = Get-ReportKind -Summary $summaryObject
        if ($kind -ne "featherdoc.table_layout_delivery_report.v1") {
            $sourceStatus = "skipped"
            $warnings.Add([ordered]@{
                id = "source_report_schema_skipped"
                source_report = $path
                source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                message = "Report schema '$kind' is not a table layout delivery summary."
            }) | Out-Null
        } else {
            $sourceStatus = Get-JsonString -Object $summaryObject -Name "status" -DefaultValue "loaded"
            $inputDocx = Get-JsonString -Object $summaryObject -Name "input_docx"
            $inputDocxDisplay = Get-ReportPathDisplay -RepoRoot $repoRoot -Path $inputDocx
            $documentName = Get-DocumentName -DocumentPath $inputDocx -Index ($documents.Count + 1)
            $preset = Get-JsonString -Object $summaryObject -Name "preset" -DefaultValue "unspecified"
            $issueCount = Get-JsonInt -Object $summaryObject -Name "table_style_issue_count"
            $automaticFixCount = Get-JsonInt -Object $summaryObject -Name "automatic_tblLook_fix_count"
            $manualFixCount = Get-JsonInt -Object $summaryObject -Name "manual_table_style_fix_count"
            $positionAutomaticCount = Get-JsonInt -Object $summaryObject -Name "table_position_automatic_count"
            $positionReviewCount = Get-JsonInt -Object $summaryObject -Name "table_position_review_count"
            $positionAlreadyMatchingCount = Get-JsonInt -Object $summaryObject -Name "table_position_already_matching_count"
            $commandFailureCount = Get-JsonInt -Object $summaryObject -Name "command_failure_count"
            $positionPlanPath = Get-JsonString -Object $summaryObject -Name "table_position_plan_path"
            $positionPlanDisplay = Get-ReportPathDisplay -RepoRoot $repoRoot -Path $positionPlanPath
            $sourceBlockers = @(Get-JsonArray -Object $summaryObject -Name "release_blockers")
            $releaseBlockerCount = $sourceBlockers.Count
            $declaredBlockerCount = Get-JsonProperty -Object $summaryObject -Name "release_blocker_count"
            if ($null -ne $declaredBlockerCount -and [int]$declaredBlockerCount -ne $releaseBlockerCount) {
                $warnings.Add([ordered]@{
                    id = "release_blocker_count_mismatch"
                    source_report = $path
                    source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    message = "release_blocker_count is $declaredBlockerCount but release_blockers contains $releaseBlockerCount item(s)."
                }) | Out-Null
            }

            $sourceActions = @(Get-JsonArray -Object $summaryObject -Name "action_items")
            $documentEntry = [ordered]@{
                document_index = $documents.Count + 1
                document_name = $documentName
                input_docx = $inputDocx
                input_docx_display = $inputDocxDisplay
                source_report = $path
                source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                status = $sourceStatus
                ready = ($sourceStatus -eq "ready")
                preset = $preset
                table_style_issue_count = $issueCount
                automatic_tblLook_fix_count = $automaticFixCount
                manual_table_style_fix_count = $manualFixCount
                table_position_automatic_count = $positionAutomaticCount
                table_position_review_count = $positionReviewCount
                table_position_already_matching_count = $positionAlreadyMatchingCount
                command_failure_count = $commandFailureCount
                release_blocker_count = $releaseBlockerCount
                action_item_count = $sourceActions.Count
                table_position_plan_path = $positionPlanPath
                table_position_plan_display = $positionPlanDisplay
            }
            $documents.Add($documentEntry) | Out-Null

            $positionPlans.Add([ordered]@{
                document_name = $documentName
                input_docx = $inputDocx
                source_report = $path
                source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                preset = $preset
                table_position_plan_path = $positionPlanPath
                table_position_plan_display = $positionPlanDisplay
                automatic_count = $positionAutomaticCount
                review_count = $positionReviewCount
                already_matching_count = $positionAlreadyMatchingCount
            }) | Out-Null

            $audit = Get-JsonProperty -Object $summaryObject -Name "table_style_quality_audit"
            $auditIssues = @(Get-JsonArray -Object $audit -Name "issues")
            if ($auditIssues.Count -eq 0 -and $issueCount -gt 0) {
                $issueRows.Add([ordered]@{
                    issue = "unspecified"
                    count = $issueCount
                    document_name = $documentName
                    source_report = $path
                }) | Out-Null
            } else {
                foreach ($issue in @($auditIssues)) {
                    $issueName = Get-FirstJsonString -Object $issue -Names @("issue", "kind", "code") -DefaultValue "unspecified"
                    $issueRows.Add([ordered]@{
                        issue = $issueName
                        count = 1
                        document_name = $documentName
                        source_report = $path
                    }) | Out-Null
                }
            }

            $blockerIndex = 0
            foreach ($blocker in @($sourceBlockers)) {
                $blockerIndex++
                $id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
                $blockers.Add([ordered]@{
                    composite_id = ("document{0}.blocker{1}.{2}" -f $documents.Count, $blockerIndex, $id)
                    id = $id
                    document_name = $documentName
                    input_docx = $inputDocx
                    source_report = $path
                    source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "warning"
                    status = Get-JsonString -Object $blocker -Name "status"
                    action = Get-JsonString -Object $blocker -Name "action"
                    message = Get-JsonString -Object $blocker -Name "message"
                    issue_count = Get-JsonInt -Object $blocker -Name "issue_count"
                    manual_fix_count = Get-JsonInt -Object $blocker -Name "manual_fix_count"
                    review_count = Get-JsonInt -Object $blocker -Name "review_count"
                }) | Out-Null
            }

            $actionIndex = 0
            foreach ($item in @($sourceActions)) {
                $actionIndex++
                $id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
                $actionItems.Add([ordered]@{
                    composite_id = ("document{0}.action{1}.{2}" -f $documents.Count, $actionIndex, $id)
                    id = $id
                    document_name = $documentName
                    input_docx = $inputDocx
                    source_report = $path
                    source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    action = Get-JsonString -Object $item -Name "action"
                    title = Get-JsonString -Object $item -Name "title"
                    command = Get-JsonString -Object $item -Name "command"
                }) | Out-Null
            }

            $totalIssueCount += $issueCount
            $totalAutomaticFixCount += $automaticFixCount
            $totalManualFixCount += $manualFixCount
            $totalPositionAutomaticCount += $positionAutomaticCount
            $totalPositionReviewCount += $positionReviewCount
            $totalPositionAlreadyMatchingCount += $positionAlreadyMatchingCount
            $totalCommandFailureCount += $commandFailureCount
        }
    } catch {
        $sourceStatus = "failed"
        $errorMessage = $_.Exception.Message
        $warnings.Add([ordered]@{
            id = "source_report_read_failed"
            source_report = $path
            source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            message = $errorMessage
        }) | Out-Null
    }

    $sourceReports.Add([ordered]@{
        path = $path
        path_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
        schema = $kind
        status = $sourceStatus
        document_name = $documentName
        input_docx = $inputDocx
        input_docx_display = $inputDocxDisplay
        table_style_issue_count = $issueCount
        release_blocker_count = $releaseBlockerCount
        error = $errorMessage
    }) | Out-Null
}

$sourceFailureCount = @($sourceReports.ToArray() | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_.error)
}).Count
$sourceReportFailureCount = @($documents.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$needsReviewCount = @($documents.ToArray() | Where-Object { $_.status -eq "needs_review" }).Count
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($sourceReportFailureCount -gt 0 -or $totalCommandFailureCount -gt 0) {
    "failed"
} elseif ($warnings.Count -gt 0 -or $blockers.Count -gt 0 -or $needsReviewCount -gt 0 -or
    $totalIssueCount -gt 0 -or $totalAutomaticFixCount -gt 0 -or $totalManualFixCount -gt 0 -or
    $totalPositionAutomaticCount -gt 0 -or $totalPositionReviewCount -gt 0) {
    "needs_review"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = "featherdoc.table_layout_delivery_rollup_report.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    ready = ($status -eq "ready")
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    source_report_count = $sourceReports.Count
    source_failure_count = $sourceFailureCount
    source_report_failure_count = $sourceReportFailureCount
    source_reports = @($sourceReports.ToArray())
    document_count = $documents.Count
    document_entries = @($documents.ToArray())
    preset_summary = @(Add-SummaryGroup -Items $documents.ToArray() -PropertyName "preset" -OutputName "preset")
    status_summary = @(Add-SummaryGroup -Items $documents.ToArray() -PropertyName "status" -OutputName "status")
    total_table_style_issue_count = $totalIssueCount
    total_automatic_tblLook_fix_count = $totalAutomaticFixCount
    total_manual_table_style_fix_count = $totalManualFixCount
    total_table_position_automatic_count = $totalPositionAutomaticCount
    total_table_position_review_count = $totalPositionReviewCount
    total_table_position_already_matching_count = $totalPositionAlreadyMatchingCount
    total_command_failure_count = $totalCommandFailureCount
    table_style_issue_summary = @(Add-IssueSummary -Items $issueRows.ToArray())
    table_position_plan_count = $positionPlans.Count
    table_position_plans = @($positionPlans.ToArray())
    release_blocker_count = $blockers.Count
    release_blockers = @($blockers.ToArray())
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0 -or $sourceReportFailureCount -gt 0 -or $totalCommandFailureCount -gt 0) { exit 1 }
if ($FailOnIssue -and $totalIssueCount -gt 0) { exit 1 }
if ($FailOnBlocker -and $blockers.Count -gt 0) { exit 1 }
