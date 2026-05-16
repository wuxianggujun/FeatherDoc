<#
.SYNOPSIS
Builds a content-control data-binding governance report.

.DESCRIPTION
Reads existing inspect-content-controls and sync-content-controls-from-custom-xml
JSON outputs, then writes a read-only JSON/Markdown handoff for Custom XML data
binding coverage, sync issues, placeholder risk, lock review, and action items.
The script does not open DOCX files or rerun CLI commands.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/content-control-data-binding-governance",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnBlocker,
    [switch]$FailOnWarning
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "release_blocker_metadata_helpers.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[content-control-data-binding-governance] $Message"
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
        $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path -AllowMissing)) | Out-Null
    }

    $scanRoots = @(Expand-ArgumentList -Values $Roots)
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @(
            "output/content-control-data-binding",
            "output/content-control-rich-replacement",
            "output/project-template-smoke"
        )
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) { continue }
        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter "*.json" |
                ForEach-Object { $paths.Add($_.FullName) | Out-Null }
        } else {
            $paths.Add($resolvedRoot) | Out-Null
        }
    }

    return @($paths.ToArray() | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique)
}

function Get-EvidenceKind {
    param($Json)

    $schema = Get-JsonString -Object $Json -Name "schema"
    if ($schema -eq "featherdoc.content_control_data_binding_governance_report.v1") {
        return "content_control_data_binding_governance_report"
    }
    if ((Get-JsonProperty -Object $Json -Name "content_controls") -ne $null) {
        return "inspect_content_controls"
    }
    if ((Get-JsonProperty -Object $Json -Name "synced_items") -ne $null -or
        (Get-JsonProperty -Object $Json -Name "scanned_content_controls") -ne $null -or
        (Get-JsonProperty -Object $Json -Name "bound_content_controls") -ne $null) {
        return "custom_xml_sync_result"
    }
    return $schema
}

function New-BindingKey {
    param([string]$StoreItemId, [string]$XPath)

    if ([string]::IsNullOrWhiteSpace($StoreItemId) -or [string]::IsNullOrWhiteSpace($XPath)) {
        return ""
    }
    return ($StoreItemId.Trim().ToLowerInvariant() + "|" + $XPath.Trim())
}

function New-ReleaseBlocker {
    param(
        [string]$Id,
        [string]$SourceJson,
        [string]$SourceReportDisplay,
        [string]$PartEntryName,
        [object]$ContentControlIndex,
        [string]$Tag,
        [string]$Alias,
        [string]$StoreItemId,
        [string]$XPath,
        [string]$Status,
        [string]$Action,
        [string]$Message
    )

    return [ordered]@{
        id = $Id
        severity = "error"
        status = $Status
        action = $Action
        message = $Message
        source_json = $SourceJson
        source_json_display = $SourceReportDisplay
        source_report_display = $SourceReportDisplay
        source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
        part_entry_name = $PartEntryName
        content_control_index = $ContentControlIndex
        tag = $Tag
        alias = $Alias
        store_item_id = $StoreItemId
        xpath = $XPath
    }
}

function New-ActionItem {
    param(
        [string]$Id,
        [string]$Action,
        [string]$Title,
        [string]$SourceJson,
        [string]$SourceReportDisplay,
        [string]$PartEntryName,
        [object]$ContentControlIndex,
        [string]$Tag,
        [string]$Alias,
        [string]$StoreItemId,
        [string]$XPath
    )

    return [ordered]@{
        id = $Id
        action = $Action
        title = $Title
        source_json = $SourceJson
        source_json_display = $SourceReportDisplay
        source_report_display = $SourceReportDisplay
        source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
        part_entry_name = $PartEntryName
        content_control_index = $ContentControlIndex
        tag = $Tag
        alias = $Alias
        store_item_id = $StoreItemId
        xpath = $XPath
    }
}

function Add-SummaryGroup {
    param([object[]]$Items, [string]$PropertyName, [string]$OutputName)

    return @(
        $Items |
            Group-Object { [string](Get-JsonProperty -Object $_ -Name $PropertyName) } |
            Sort-Object Name |
            ForEach-Object {
                [ordered]@{
                    $OutputName = $_.Name
                    count = $_.Count
                }
            }
    )
}

function Convert-InspectContentControls {
    param([string]$RepoRoot, [string]$Path, $Json)

    $part = Get-JsonString -Object $Json -Name "part"
    $entryName = Get-JsonString -Object $Json -Name "entry_name"
    $items = New-Object 'System.Collections.Generic.List[object]'
    foreach ($control in @(Get-JsonArray -Object $Json -Name "content_controls")) {
        $storeItemId = Get-JsonString -Object $control -Name "data_binding_store_item_id"
        $xpath = Get-JsonString -Object $control -Name "data_binding_xpath"
        $items.Add([ordered]@{
            source_json = $Path
            source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
            part = $part
            part_entry_name = $entryName
            content_control_index = Get-JsonInt -Object $control -Name "index"
            kind = Get-JsonString -Object $control -Name "kind"
            form_kind = Get-JsonString -Object $control -Name "form_kind"
            tag = Get-JsonString -Object $control -Name "tag"
            alias = Get-JsonString -Object $control -Name "alias"
            lock = Get-JsonString -Object $control -Name "lock"
            store_item_id = $storeItemId
            xpath = $xpath
            prefix_mappings = Get-JsonString -Object $control -Name "data_binding_prefix_mappings"
            binding_key = New-BindingKey -StoreItemId $storeItemId -XPath $xpath
            showing_placeholder = Get-JsonBool -Object $control -Name "showing_placeholder"
            text = Get-JsonString -Object $control -Name "text"
        }) | Out-Null
    }
    return @($items.ToArray())
}

function Convert-SyncItems {
    param([string]$RepoRoot, [string]$Path, $Json)

    return @(
        foreach ($item in @(Get-JsonArray -Object $Json -Name "synced_items")) {
            [ordered]@{
                source_json = $Path
                source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
                part_entry_name = Get-JsonString -Object $item -Name "part_entry_name"
                content_control_index = Get-JsonInt -Object $item -Name "content_control_index"
                tag = Get-JsonString -Object $item -Name "tag"
                alias = Get-JsonString -Object $item -Name "alias"
                store_item_id = Get-JsonString -Object $item -Name "store_item_id"
                xpath = Get-JsonString -Object $item -Name "xpath"
                previous_text = Get-JsonString -Object $item -Name "previous_text"
                value = Get-JsonString -Object $item -Name "value"
            }
        }
    )
}

function Convert-SyncIssues {
    param([string]$RepoRoot, [string]$Path, $Json)

    return @(
        foreach ($issue in @(Get-JsonArray -Object $Json -Name "issues")) {
            [ordered]@{
                source_json = $Path
                source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
                part_entry_name = Get-JsonString -Object $issue -Name "part_entry_name"
                content_control_index = Get-JsonProperty -Object $issue -Name "content_control_index"
                tag = Get-JsonString -Object $issue -Name "tag"
                alias = Get-JsonString -Object $issue -Name "alias"
                store_item_id = Get-JsonString -Object $issue -Name "store_item_id"
                xpath = Get-JsonString -Object $issue -Name "xpath"
                reason = Get-JsonString -Object $issue -Name "reason" -DefaultValue "custom_xml_sync_issue"
            }
        }
    )
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Content Control Data Binding Governance") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Content controls: ``$($Summary.content_control_count)``") | Out-Null
    $lines.Add("- Bound controls: ``$($Summary.bound_content_control_count)``") | Out-Null
    $lines.Add("- Synced controls: ``$($Summary.synced_content_control_count)``") | Out-Null
    $lines.Add("- Sync issues: ``$($Summary.sync_issue_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($Summary.action_item_count)``") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("## Binding Coverage") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.binding_status_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($entry in @($Summary.binding_status_summary)) {
            $lines.Add("- ``$($entry.status)``: ``$($entry.count)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.id)``: action=``$($blocker.action)`` source=``$($blocker.source_report_display)`` message=$($blocker.message)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.id)``: action=``$($item.action)`` source=``$($item.source_report_display)`` title=$($item.title)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    $warningLines = New-Object 'System.Collections.Generic.List[string]'
    if (-not (Add-ReleaseGovernanceWarningMarkdownSubsection `
                -Lines $warningLines `
                -Heading "Content control data-binding governance warnings" `
                -SummaryObject $Summary)) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($line in $warningLines) {
            $lines.Add($line) | Out-Null
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
    Join-Path $resolvedOutputDir "content_control_data_binding_governance.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) input JSON file(s)"

$contentControls = New-Object 'System.Collections.Generic.List[object]'
$syncItems = New-Object 'System.Collections.Generic.List[object]'
$syncIssues = New-Object 'System.Collections.Generic.List[object]'
$sourceFiles = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'

foreach ($path in @($inputPaths)) {
    $kind = "unknown"
    $status = "ignored"
    $errorMessage = ""
    if (-not (Test-Path -LiteralPath $path)) {
        $kind = "missing"
        $status = "missing"
        $warnings.Add([ordered]@{
            id = "input_json_missing"
            action = "provide_content_control_data_binding_evidence"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json = $path
            source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            message = "Input JSON was not found."
        }) | Out-Null
    } else {
        try {
            $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
            $kind = Get-EvidenceKind -Json $json
            switch ($kind) {
                "inspect_content_controls" {
                    foreach ($item in @(Convert-InspectContentControls -RepoRoot $repoRoot -Path $path -Json $json)) {
                        $contentControls.Add($item) | Out-Null
                    }
                    $status = "loaded"
                }
                "custom_xml_sync_result" {
                    foreach ($item in @(Convert-SyncItems -RepoRoot $repoRoot -Path $path -Json $json)) {
                        $syncItems.Add($item) | Out-Null
                    }
                    foreach ($issue in @(Convert-SyncIssues -RepoRoot $repoRoot -Path $path -Json $json)) {
                        $syncIssues.Add($issue) | Out-Null
                    }
                    $status = "loaded"
                }
                "content_control_data_binding_governance_report" {
                    foreach ($item in @(Get-JsonArray -Object $json -Name "content_controls")) {
                        $contentControls.Add($item) | Out-Null
                    }
                    foreach ($item in @(Get-JsonArray -Object $json -Name "sync_items")) {
                        $syncItems.Add($item) | Out-Null
                    }
                    foreach ($issue in @(Get-JsonArray -Object $json -Name "sync_issues")) {
                        $syncIssues.Add($issue) | Out-Null
                    }
                    $status = "loaded"
                }
                default {
                    $status = "skipped"
                    $warnings.Add([ordered]@{
                        id = "source_json_schema_skipped"
                        action = "provide_content_control_data_binding_evidence"
                        source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
                        source_json = $path
                        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        message = "Input JSON kind '$kind' is not content-control data-binding evidence."
                    }) | Out-Null
                }
            }
        } catch {
            $status = "failed"
            $errorMessage = $_.Exception.Message
            $warnings.Add([ordered]@{
                id = "source_json_read_failed"
                action = "fix_content_control_data_binding_input_json"
                source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
                source_json = $path
                source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                message = $errorMessage
            }) | Out-Null
        }
    }

    $sourceFiles.Add([ordered]@{
        path = $path
        path_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
        kind = $kind
        status = $status
        error = $errorMessage
    }) | Out-Null
}

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'

foreach ($issue in @($syncIssues.ToArray())) {
    $releaseBlockers.Add((New-ReleaseBlocker `
                -Id "content_control_data_binding.custom_xml_sync_issue" `
                -SourceJson ([string]$issue.source_json) `
                -SourceReportDisplay ([string]$issue.source_json_display) `
                -PartEntryName ([string]$issue.part_entry_name) `
                -ContentControlIndex $issue.content_control_index `
                -Tag ([string]$issue.tag) `
                -Alias ([string]$issue.alias) `
                -StoreItemId ([string]$issue.store_item_id) `
                -XPath ([string]$issue.xpath) `
                -Status ([string]$issue.reason) `
                -Action "fix_custom_xml_data_binding_source" `
                -Message "Custom XML sync failed for a bound content control.")) | Out-Null
}

foreach ($control in @($contentControls.ToArray())) {
    $hasBinding = -not [string]::IsNullOrWhiteSpace([string]$control.binding_key)
    if ($hasBinding -and [bool]$control.showing_placeholder) {
        $releaseBlockers.Add((New-ReleaseBlocker `
                    -Id "content_control_data_binding.bound_placeholder" `
                    -SourceJson ([string]$control.source_json) `
                    -SourceReportDisplay ([string]$control.source_json_display) `
                    -PartEntryName ([string]$control.part_entry_name) `
                    -ContentControlIndex $control.content_control_index `
                    -Tag ([string]$control.tag) `
                    -Alias ([string]$control.alias) `
                    -StoreItemId ([string]$control.store_item_id) `
                    -XPath ([string]$control.xpath) `
                    -Status "placeholder_visible" `
                    -Action "sync_or_fill_bound_content_control" `
                    -Message "A data-bound content control is still showing placeholder text.")) | Out-Null
    }

    if ($hasBinding -and -not [string]::IsNullOrWhiteSpace([string]$control.lock)) {
        $actionItems.Add((New-ActionItem `
                    -Id "review_content_control_lock_strategy" `
                    -Action "review_content_control_lock_strategy" `
                    -Title "Review lock state for data-bound content control" `
                    -SourceJson ([string]$control.source_json) `
                    -SourceReportDisplay ([string]$control.source_json_display) `
                    -PartEntryName ([string]$control.part_entry_name) `
                    -ContentControlIndex $control.content_control_index `
                    -Tag ([string]$control.tag) `
                    -Alias ([string]$control.alias) `
                    -StoreItemId ([string]$control.store_item_id) `
                    -XPath ([string]$control.xpath))) | Out-Null
    }

    if (-not $hasBinding -and [string]$control.form_kind -notin @("", "rich_text", "plain_text")) {
        $actionItems.Add((New-ActionItem `
                    -Id "review_unbound_form_content_control" `
                    -Action "review_unbound_form_content_control" `
                    -Title "Review whether form content control should bind to template data" `
                    -SourceJson ([string]$control.source_json) `
                    -SourceReportDisplay ([string]$control.source_json_display) `
                    -PartEntryName ([string]$control.part_entry_name) `
                    -ContentControlIndex $control.content_control_index `
                    -Tag ([string]$control.tag) `
                    -Alias ([string]$control.alias) `
                    -StoreItemId "" `
                    -XPath "")) | Out-Null
    }
}

$bindingGroups = @($contentControls.ToArray() | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_.binding_key)
    } | Group-Object { [string]$_.binding_key } | Where-Object { $_.Count -gt 1 })
foreach ($group in @($bindingGroups)) {
    $first = @($group.Group)[0]
    $actionItems.Add((New-ActionItem `
                -Id "review_duplicate_content_control_binding" `
                -Action "review_duplicate_content_control_binding" `
                -Title "Review repeated content controls that share one Custom XML binding" `
                -SourceJson ([string]$first.source_json) `
                -SourceReportDisplay ([string]$first.source_json_display) `
                -PartEntryName ([string]$first.part_entry_name) `
                -ContentControlIndex $first.content_control_index `
                -Tag ([string]$first.tag) `
                -Alias ([string]$first.alias) `
                -StoreItemId ([string]$first.store_item_id) `
                -XPath ([string]$first.xpath))) | Out-Null
}

$boundControls = @($contentControls.ToArray() | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_.binding_key)
    })
if ($contentControls.Count -eq 0 -and $syncItems.Count -eq 0 -and $syncIssues.Count -eq 0) {
    $warnings.Add([ordered]@{
        id = "content_control_binding_evidence_missing"
        action = "provide_content_control_data_binding_evidence"
        source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
        message = "No content-control inspection or Custom XML sync evidence was loaded."
    }) | Out-Null
}
if ($boundControls.Count -gt 0 -and $syncItems.Count -eq 0 -and $syncIssues.Count -eq 0) {
    $warnings.Add([ordered]@{
        id = "custom_xml_sync_evidence_missing"
        action = "run_custom_xml_sync_evidence"
        source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
        message = "Data-bound content controls were inspected, but no Custom XML sync result was provided."
    }) | Out-Null
}

$sourceFailureCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$missingInputCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "missing" }).Count
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0) {
    "blocked"
} elseif ($warnings.Count -gt 0 -or $actionItems.Count -gt 0) {
    "needs_review"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
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
    missing_input_count = $missingInputCount
    source_files = @($sourceFiles.ToArray())
    inspection_file_count = @($sourceFiles.ToArray() | Where-Object { $_.kind -eq "inspect_content_controls" -and $_.status -eq "loaded" }).Count
    sync_file_count = @($sourceFiles.ToArray() | Where-Object { $_.kind -eq "custom_xml_sync_result" -and $_.status -eq "loaded" }).Count
    content_control_count = $contentControls.Count
    bound_content_control_count = $boundControls.Count
    placeholder_bound_content_control_count = @($boundControls | Where-Object { [bool]$_.showing_placeholder }).Count
    locked_bound_content_control_count = @($boundControls | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_.lock) }).Count
    duplicate_binding_count = @($bindingGroups).Count
    synced_content_control_count = $syncItems.Count
    sync_issue_count = $syncIssues.Count
    content_controls = @($contentControls.ToArray())
    sync_items = @($syncItems.ToArray())
    sync_issues = @($syncIssues.ToArray())
    binding_status_summary = @(Add-SummaryGroup -Items $contentControls.ToArray() -PropertyName "binding_key" -OutputName "status")
    form_kind_summary = @(Add-SummaryGroup -Items $contentControls.ToArray() -PropertyName "form_kind" -OutputName "form_kind")
    sync_issue_reason_summary = @(Add-SummaryGroup -Items $syncIssues.ToArray() -PropertyName "reason" -OutputName "reason")
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    blocker_id_summary = @(Add-SummaryGroup -Items $releaseBlockers.ToArray() -PropertyName "id" -OutputName "id")
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    action_item_summary = @(Add-SummaryGroup -Items $actionItems.ToArray() -PropertyName "action" -OutputName "action")
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnWarning -and $warnings.Count -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
