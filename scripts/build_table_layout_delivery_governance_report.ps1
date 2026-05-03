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

function Write-Step {
    param([string]$Message)
    Write-Host "[table-layout-delivery-governance] $Message"
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

function Get-JsonBool {
    param($Object, [string]$Name, [bool]$DefaultValue = $false)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    if ($value -is [bool]) { return [bool]$value }
    return [string]$value -in @("true", "True", "1", "yes", "Yes")
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

function Expand-ArgumentList {
    param([string[]]$Values)

    return @(
        foreach ($value in @($Values)) {
            if ([string]::IsNullOrWhiteSpace($value)) { continue }
            foreach ($part in ([string]$value -split ",")) {
                if (-not [string]::IsNullOrWhiteSpace($part)) { $part.Trim() }
            }
        }
    )
}

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @(Expand-ArgumentList -Values $ExplicitPaths)) {
        $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
    }

    $scanRoots = @(Expand-ArgumentList -Values $Roots)
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @("output/table-layout-delivery-rollup")
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) { continue }
        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter "*.json" |
                Where-Object { $_.Name -eq "summary.json" } |
                ForEach-Object { $paths.Add($_.FullName) | Out-Null }
        } else {
            $paths.Add($resolvedRoot) | Out-Null
        }
    }

    return @($paths.ToArray() | Sort-Object -Unique)
}

function Get-EvidenceKind {
    param($Json)

    $schema = Get-JsonString -Object $Json -Name "schema"
    if ($schema -eq "featherdoc.table_layout_delivery_governance_report.v1") {
        return "table_layout_delivery_governance_report"
    }
    if ($schema -eq "featherdoc.table_layout_delivery_rollup_report.v1" -or
        $null -ne (Get-JsonProperty -Object $Json -Name "table_position_plans")) {
        return "table_layout_delivery_rollup_report"
    }
    return "unknown"
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

function New-ReleaseBlocker {
    param(
        [string]$Id,
        [string]$Scope,
        [string]$SourceKind,
        [string]$Severity = "warning",
        [string]$Status = "needs_review",
        [string]$Action,
        [string]$Message
    )

    return [ordered]@{
        id = $Id
        scope = $Scope
        source_kind = $SourceKind
        severity = $Severity
        status = $Status
        action = $Action
        message = $Message
    }
}

function New-ActionItem {
    param(
        [string]$Id,
        [string]$Scope,
        [string]$SourceKind,
        [string]$Action,
        [string]$Title,
        [string]$Command = ""
    )

    return [ordered]@{
        id = $Id
        scope = $Scope
        source_kind = $SourceKind
        action = $Action
        title = $Title
        command = $Command
    }
}

function Add-UniqueBlocker {
    param($Collection, $Blocker)

    $key = "$($Blocker.scope)|$($Blocker.id)|$($Blocker.action)"
    foreach ($existing in @($Collection.ToArray())) {
        $existingKey = "$($existing.scope)|$($existing.id)|$($existing.action)"
        if ($existingKey -eq $key) { return }
    }
    $Collection.Add($Blocker) | Out-Null
}

function Add-UniqueAction {
    param($Collection, $Action)

    $key = "$($Action.scope)|$($Action.id)|$($Action.action)"
    foreach ($existing in @($Collection.ToArray())) {
        $existingKey = "$($existing.scope)|$($existing.id)|$($existing.action)"
        if ($existingKey -eq $key) { return }
    }
    $Collection.Add($Action) | Out-Null
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Table Layout Delivery Governance Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Documents: ``$($Summary.document_count)``") | Out-Null
    $lines.Add("- Ready documents: ``$($Summary.ready_document_count)``") | Out-Null
    $lines.Add("- Needs review documents: ``$($Summary.needs_review_document_count)``") | Out-Null
    $lines.Add("- Table style issues: ``$($Summary.total_table_style_issue_count)``") | Out-Null
    $lines.Add("- Safe tblLook fixes: ``$($Summary.total_automatic_tblLook_fix_count)``") | Out-Null
    $lines.Add("- Manual table style fixes: ``$($Summary.total_manual_table_style_fix_count)``") | Out-Null
    $lines.Add("- Floating table automatic plans: ``$($Summary.total_table_position_automatic_count)``") | Out-Null
    $lines.Add("- Floating table review plans: ``$($Summary.total_table_position_review_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Documents") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.documents).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($document in @($Summary.documents)) {
            $lines.Add(("- ``{0}``: status=``{1}`` preset=``{2}`` issues=``{3}`` tblLook=``{4}`` manual=``{5}`` position_review=``{6}``" -f
                $document.document_name,
                $document.status,
                $document.preset,
                $document.table_style_issue_count,
                $document.automatic_tblLook_fix_count,
                $document.manual_table_style_fix_count,
                $document.table_position_review_count)) | Out-Null
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

    $lines.Add("## Delivery Actions") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.delivery_actions).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.delivery_actions)) {
            $lines.Add("- ``$($item.scope)`` / ``$($item.id)``: action=``$($item.action)`` title=$($item.title)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.scope)`` / ``$($blocker.id)``: action=``$($blocker.action)`` message=$($blocker.message)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.warnings).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($warning in @($Summary.warnings)) {
            $lines.Add("- ``$($warning.id)``: $($warning.message)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Source Files") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.source_files).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($source in @($Summary.source_files)) {
            $lines.Add("- ``$($source.path_display)``: kind=``$($source.kind)`` status=``$($source.status)``") | Out-Null
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
                        command_failure_count = Get-JsonInt -Object $document -Name "command_failure_count"
                        table_position_plan_path = Get-JsonString -Object $document -Name "table_position_plan_path"
                        table_position_plan_display = Get-JsonString -Object $document -Name "table_position_plan_display"
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
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "warning"
                        status = Get-JsonString -Object $blocker -Name "status" -DefaultValue "needs_review"
                        action = Get-JsonString -Object $blocker -Name "action"
                        message = Get-JsonString -Object $blocker -Name "message"
                    })
                }
                foreach ($item in @(Get-JsonArray -Object $json -Name "action_items")) {
                    Add-UniqueAction -Collection $deliveryActions -Action ([ordered]@{
                        id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
                        scope = Get-FirstJsonString -Object $item -Names @("document_name", "scope") -DefaultValue "table_layout_delivery"
                        source_kind = "table_layout_delivery_rollup_report"
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        action = Get-JsonString -Object $item -Name "action"
                        title = Get-JsonString -Object $item -Name "title"
                        command = Get-JsonString -Object $item -Name "command"
                    })
                }
            }
            "table_layout_delivery_governance_report" {
                $status = "skipped"
            }
            default {
                $warnings.Add([ordered]@{
                    id = "source_json_schema_skipped"
                    source_json = $path
                    source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    message = "Input JSON kind '$kind' is not table layout delivery governance evidence."
                }) | Out-Null
            }
        }
    } catch {
        $status = "failed"
        $errorMessage = $_.Exception.Message
        $warnings.Add([ordered]@{
            id = "source_json_read_failed"
            source_json = $path
            source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
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
        -Message "Safe tblLook-only fixes are available and need application plus visual regression.")
    Add-UniqueAction -Collection $deliveryActions -Action (New-ActionItem `
        -Id "apply_safe_tblLook_fixes" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Action "apply_safe_tblLook_fixes_then_visual_regression" `
        -Title "Apply safe tblLook-only fixes and regenerate visual evidence")
}
if ($totalManualFixCount -gt 0) {
    Add-UniqueBlocker -Collection $releaseBlockers -Blocker (New-ReleaseBlocker `
        -Id "table_layout_delivery.manual_table_style_work" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Status "needs_review" `
        -Action "review_manual_table_style_definition_work" `
        -Message "Some table style quality issues require manual style-definition work.")
}
if ($totalPositionReviewCount -gt 0) {
    Add-UniqueBlocker -Collection $releaseBlockers -Blocker (New-ReleaseBlocker `
        -Id "table_layout_delivery.floating_table_review_pending" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Status "needs_review" `
        -Action "review_floating_table_position_plans" `
        -Message "Floating table position plans include entries requiring manual review.")
    Add-UniqueAction -Collection $deliveryActions -Action (New-ActionItem `
        -Id "review_floating_table_position_plans" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Action "review_floating_table_position_plans" `
        -Title "Review floating table position plans before applying presets")
}
if ($totalPositionAutomaticCount -gt 0) {
    Add-UniqueAction -Collection $deliveryActions -Action (New-ActionItem `
        -Id "dry_run_apply_table_position_plans" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Action "dry_run_apply_table_position_plans" `
        -Title "Dry-run saved floating table position plans before writing DOCX")
}
if ($totalIssueCount -gt 0 -or $totalAutomaticFixCount -gt 0 -or $totalPositionAutomaticCount -gt 0 -or $totalPositionReviewCount -gt 0) {
    Add-UniqueAction -Collection $deliveryActions -Action (New-ActionItem `
        -Id "run_table_style_quality_visual_regression" `
        -Scope "table_layout_delivery" `
        -SourceKind "table_layout_delivery_governance" `
        -Action "run_table_style_quality_visual_regression" `
        -Title "Generate Word-rendered table layout visual regression evidence" `
        -Command "pwsh -ExecutionPolicy Bypass -File .\scripts\run_table_style_quality_visual_regression.ps1")
}

$sourceFailureCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$readyDocumentCount = @($documents.ToArray() | Where-Object { [bool]$_.ready }).Count
$needsReviewDocumentCount = @($documents.ToArray() | Where-Object { $_.status -eq "needs_review" }).Count
$failedDocumentCount = @($documents.ToArray() | Where-Object { $_.status -eq "failed" }).Count
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
    preset_summary = @(Add-SummaryGroup -Items $documents.ToArray() -PropertyName "preset" -OutputName "preset")
    status_summary = @(Add-SummaryGroup -Items $documents.ToArray() -PropertyName "status" -OutputName "status")
    table_position_plan_count = $positionPlans.Count
    table_position_plans = @($positionPlans.ToArray())
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
    action_item_count = $deliveryActions.Count
    action_items = @($deliveryActions.ToArray())
    delivery_actions = @($deliveryActions.ToArray())
    next_steps = @($deliveryActions.ToArray())
    action_item_summary = @(Add-SummaryGroup -Items $deliveryActions.ToArray() -PropertyName "action" -OutputName "action")
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
