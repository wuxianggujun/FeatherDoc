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
    [switch]$IncludeReleaseBlockerRollup,
    [switch]$FailOnMissing,
    [switch]$FailOnBlocker,
    [switch]$FailOnWarning
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "release_blocker_metadata_helpers.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[release-governance-handoff] $Message"
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

function Get-ReportKind {
    param($Summary)

    $schema = Get-JsonString -Object $Summary -Name "schema"
    switch ($schema) {
        "featherdoc.numbering_catalog_governance_report.v1" { return "numbering_catalog_governance" }
        "featherdoc.table_layout_delivery_governance_report.v1" { return "table_layout_delivery_governance" }
        "featherdoc.content_control_data_binding_governance_report.v1" { return "content_control_data_binding_governance" }
        "featherdoc.project_template_delivery_readiness_report.v1" { return "project_template_delivery_readiness" }
        default { return $schema }
    }
}

function New-ExpectedReport {
    param(
        [string]$Id,
        [string]$Title,
        [string]$RelativeSummary,
        [string]$BuildCommand
    )

    return [ordered]@{
        id = $Id
        title = $Title
        relative_summary = $RelativeSummary
        build_command = $BuildCommand
    }
}

function Invoke-ReleaseBlockerRollup {
    param(
        [string]$RepoRoot,
        [string]$OutputDir,
        [string[]]$InputJson
    )

    if (@($InputJson).Count -eq 0) {
        throw "Release blocker rollup requires at least one loaded governance report."
    }

    $scriptPath = Join-Path $RepoRoot "scripts\build_release_blocker_rollup_report.ps1"
    $arguments = @(
        "-InputJson"
        (@($InputJson) -join ",")
        "-OutputDir"
        $OutputDir
    )
    $commandOutput = @(& (Get-Process -Id $PID).Path -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @arguments 2>&1)
    if ($LASTEXITCODE -ne 0) {
        $errorText = (@($commandOutput | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
        throw "Failed to build release blocker rollup. Output: $errorText"
    }
}

function New-ReportEntry {
    param(
        [string]$RepoRoot,
        [string]$Id,
        [string]$Title,
        [string]$ExpectedSummaryPath,
        [string]$BuildCommand,
        [string]$Source = "default",
        [object]$Json = $null,
        [string]$Status = "missing",
        [string]$ErrorMessage = ""
    )

    $schema = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "schema" }
    return [ordered]@{
        id = $Id
        title = $Title
        source = $Source
        expected_summary = $ExpectedSummaryPath
        expected_summary_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $ExpectedSummaryPath
        build_command = $BuildCommand
        schema = $schema
        status = if ($null -eq $Json) { $Status } else { Get-JsonString -Object $Json -Name "status" -DefaultValue $Status }
        release_ready = if ($null -eq $Json) { $false } else { Get-JsonBool -Object $Json -Name "release_ready" }
        release_blocker_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "release_blocker_count" }
        action_item_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "action_item_count" }
        warning_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "warning_count" }
        source_failure_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "source_failure_count" }
        report_markdown = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "report_markdown" }
        report_markdown_display = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "report_markdown_display" }
        release_blockers = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "release_blockers") }
        action_items = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "action_items") }
        warnings = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "warnings") }
        error = $ErrorMessage
    }
}

function Add-NormalizedBlockers {
    param(
        [System.Collections.Generic.List[object]]$Collection,
        [object]$Report
    )

    foreach ($blocker in @($Report.release_blockers)) {
        $Collection.Add([ordered]@{
            report_id = [string]$Report.id
            report_title = [string]$Report.title
            id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
            severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "error"
            status = Get-JsonString -Object $blocker -Name "status"
            action = Get-JsonString -Object $blocker -Name "action"
            message = Get-JsonString -Object $blocker -Name "message"
        }) | Out-Null
    }
}

function Add-NormalizedActions {
    param(
        [System.Collections.Generic.List[object]]$Collection,
        [object]$Report
    )

    foreach ($item in @($Report.action_items)) {
        $Collection.Add([ordered]@{
            report_id = [string]$Report.id
            report_title = [string]$Report.title
            id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
            action = Get-JsonString -Object $item -Name "action"
            title = Get-JsonString -Object $item -Name "title"
            command = Get-JsonString -Object $item -Name "command"
        }) | Out-Null
    }
}

function Add-NormalizedWarnings {
    param(
        [System.Collections.Generic.List[object]]$Collection,
        [object]$Report
    )

    foreach ($warning in @($Report.warnings)) {
        $entry = [ordered]@{
            report_id = [string]$Report.id
            report_title = [string]$Report.title
            id = Get-JsonString -Object $warning -Name "id" -DefaultValue "warning"
            action = Get-JsonString -Object $warning -Name "action"
            message = Get-JsonString -Object $warning -Name "message"
            source_schema = Get-JsonString -Object $warning -Name "source_schema" -DefaultValue ([string]$Report.schema)
        }

        $styleMergeSuggestionCount = Get-JsonProperty -Object $warning -Name "style_merge_suggestion_count"
        if ($null -ne $styleMergeSuggestionCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionCount)) {
            $entry.style_merge_suggestion_count = [int]$styleMergeSuggestionCount
        }

        $Collection.Add([pscustomobject]$entry) | Out-Null
    }
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Release Governance Handoff") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Reports loaded: ``$($Summary.loaded_report_count)`` / ``$($Summary.expected_report_count)``") | Out-Null
    $lines.Add("- Missing reports: ``$($Summary.missing_report_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($Summary.action_item_count)``") | Out-Null
    $lines.Add("- Warnings: ``$($Summary.warning_count)``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Report Status") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($report in @($Summary.reports)) {
        $lines.Add("- ``$($report.id)``: status=``$($report.status)`` ready=``$($report.release_ready)`` blockers=``$($report.release_blocker_count)`` actions=``$($report.action_item_count)``") | Out-Null
        $lines.Add("  - summary: ``$($report.expected_summary_display)``") | Out-Null
        if ([string]::IsNullOrWhiteSpace([string]$report.schema)) {
            $lines.Add("  - build: ``$($report.build_command)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.report_id)`` / ``$($blocker.id)``: action=``$($blocker.action)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.message)) {
                $lines.Add("  - $($blocker.message)") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    $warningLines = New-Object 'System.Collections.Generic.List[string]'
    $hasWarnings = $false
    if (Add-ReleaseGovernanceWarningMarkdownSubsection `
            -Lines $warningLines `
            -Heading "Release governance handoff warnings" `
            -SummaryObject $Summary) {
        $hasWarnings = $true
    }
    if (Add-ReleaseGovernanceWarningMarkdownSubsection `
            -Lines $warningLines `
            -Heading "Release blocker rollup warnings" `
            -SummaryObject $Summary.release_blocker_rollup) {
        $hasWarnings = $true
    }
    if (-not $hasWarnings) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($line in $warningLines) {
            $lines.Add($line) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blocker Rollup") | Out-Null
    $lines.Add("") | Out-Null
    $rollup = $Summary.release_blocker_rollup
    $lines.Add("- Included: ``$($rollup.included)``") | Out-Null
    $lines.Add("- Status: ``$($rollup.status)``") | Out-Null
    $lines.Add("- Source reports: ``$($rollup.source_report_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($rollup.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($rollup.action_item_count)``") | Out-Null
    $lines.Add("- Warnings: ``$($rollup.warning_count)``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.report_id)`` / ``$($item.id)``: action=``$($item.action)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Next Commands") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($command in @($Summary.next_commands)) {
        $lines.Add("- ``$command``") | Out-Null
    }
    if (@($Summary.next_commands).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    }

    return @($lines)
}

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

$expectedReports = @(
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
)

$reports = New-Object 'System.Collections.Generic.List[object]'
$reportById = @{}
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
        if (-not $reportById.ContainsKey($kind)) {
            continue
        }
        $expected = @($expectedReports | Where-Object { $_.id -eq $kind } | Select-Object -First 1)
        $entry = New-ReportEntry `
            -RepoRoot $repoRoot `
            -Id ([string]$expected.id) `
            -Title ([string]$expected.title) `
            -ExpectedSummaryPath $path `
            -BuildCommand ([string]$expected.build_command) `
            -Source "explicit" `
            -Json $json `
            -Status "loaded"
        for ($index = 0; $index -lt $reports.Count; $index++) {
            if ([string]$reports[$index].id -eq [string]$expected.id) {
                $reports[$index] = $entry
                break
            }
        }
        $reportById[[string]$expected.id] = $entry
    } catch {
        Write-Step "Skipping explicit report '$path': $($_.Exception.Message)"
    }
}

$releaseBlockerRollupSummary = $null
if ($IncludeReleaseBlockerRollup) {
    $loadedReports = @($reports.ToArray() | Where-Object { $_.status -notin @("missing", "failed") })
    $loadedReportInputs = @(
        foreach ($report in $loadedReports) {
            [string]$report.expected_summary
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
    $releaseBlockerRollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseBlockerRollupSummaryPath | ConvertFrom-Json
}

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'
foreach ($report in @($reports.ToArray())) {
    Add-NormalizedBlockers -Collection $releaseBlockers -Report $report
    Add-NormalizedActions -Collection $actionItems -Report $report
    Add-NormalizedWarnings -Collection $warnings -Report $report
}

$loadedReportCount = @($reports.ToArray() | Where-Object { $_.status -notin @("missing", "failed") }).Count
$missingReportCount = @($reports.ToArray() | Where-Object { $_.status -eq "missing" }).Count
$failedReportCount = @($reports.ToArray() | Where-Object { $_.status -eq "failed" -or $_.source_failure_count -gt 0 }).Count
$warningCount = 0
foreach ($report in @($reports.ToArray())) {
    $warningCount += [int]$report.warning_count
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
    release_blocker_rollup = [ordered]@{
        included = [bool]$IncludeReleaseBlockerRollup
        status = if ($null -eq $releaseBlockerRollupSummary) {
            if ($IncludeReleaseBlockerRollup) { "missing_summary" } else { "not_requested" }
        } else {
            Get-JsonString -Object $releaseBlockerRollupSummary -Name "status" -DefaultValue "completed"
        }
        output_dir = $releaseBlockerRollupOutputDir
        output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBlockerRollupOutputDir
        summary_json = $releaseBlockerRollupSummaryPath
        summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBlockerRollupSummaryPath
        report_markdown = $releaseBlockerRollupMarkdownPath
        report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBlockerRollupMarkdownPath
        release_ready = if ($null -eq $releaseBlockerRollupSummary) {
            $false
        } else {
            Get-JsonBool -Object $releaseBlockerRollupSummary -Name "release_ready"
        }
        source_report_count = if ($null -eq $releaseBlockerRollupSummary) {
            0
        } else {
            Get-JsonInt -Object $releaseBlockerRollupSummary -Name "source_report_count"
        }
        source_failure_count = if ($null -eq $releaseBlockerRollupSummary) {
            0
        } else {
            Get-JsonInt -Object $releaseBlockerRollupSummary -Name "source_failure_count"
        }
        release_blocker_count = if ($null -eq $releaseBlockerRollupSummary) {
            0
        } else {
            Get-JsonInt -Object $releaseBlockerRollupSummary -Name "release_blocker_count"
        }
        action_item_count = if ($null -eq $releaseBlockerRollupSummary) {
            0
        } else {
            Get-JsonInt -Object $releaseBlockerRollupSummary -Name "action_item_count"
        }
        warning_count = if ($null -eq $releaseBlockerRollupSummary) {
            0
        } else {
            Get-JsonInt -Object $releaseBlockerRollupSummary -Name "warning_count"
        }
        warnings = if ($null -eq $releaseBlockerRollupSummary) {
            @()
        } else {
            @(Get-JsonArray -Object $releaseBlockerRollupSummary -Name "warnings")
        }
    }
    expected_report_count = $expectedReports.Count
    loaded_report_count = $loadedReportCount
    missing_report_count = $missingReportCount
    failed_report_count = $failedReportCount
    reports = @($reports.ToArray())
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    warning_count = $warningCount
    warnings = @($warnings.ToArray())
    next_commands = @($nextCommands.ToArray())
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($FailOnMissing -and $missingReportCount -gt 0) { exit 1 }
if ($FailOnWarning -and $warningCount -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
if ($failedReportCount -gt 0) { exit 1 }
