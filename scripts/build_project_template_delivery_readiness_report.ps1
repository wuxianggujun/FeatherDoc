<#
.SYNOPSIS
Builds a project-template delivery readiness report from onboarding evidence.

.DESCRIPTION
Reads project-template onboarding governance summaries, direct onboarding
summaries, onboarding plans, and schema approval history reports, then writes a
single JSON/Markdown delivery gate report. The script is read-only for source
artifacts and does not rerun smoke, CMake, Word, or visual automation.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/project-template-delivery-readiness",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnBlocker
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "release_blocker_metadata_helpers.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-delivery-readiness] $Message"
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
                if (-not [string]::IsNullOrWhiteSpace($part)) {
                    $part.Trim()
                }
            }
        }
    )
}

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @(Expand-ArgumentList -Values $ExplicitPaths)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
        }
    }

    $scanRoots = @(Expand-ArgumentList -Values $Roots)
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @(
            "output/project-template-onboarding-governance",
            "output/project-template-onboarding",
            "output/project-template-smoke-onboarding-plan",
            "output/project-template-schema-approval-history"
        )
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) { continue }

        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter "*.json" |
                Where-Object {
                    $_.Name -in @("summary.json", "onboarding_summary.json", "plan.json", "history.json", "schema-approval-history.json")
                } |
                ForEach-Object { $paths.Add($_.FullName) | Out-Null }
        } else {
            $paths.Add($resolvedRoot) | Out-Null
        }
    }

    return @($paths.ToArray() | Sort-Object -Unique)
}

function Get-EvidenceKind {
    param($Json, [string]$Path)

    $schema = Get-JsonString -Object $Json -Name "schema"
    if ($schema -eq "featherdoc.project_template_delivery_readiness_report.v1") {
        return "project_template_delivery_readiness_report"
    }
    if ($schema -eq "featherdoc.project_template_onboarding_governance_report.v1") {
        return "onboarding_governance_report"
    }
    if ($schema -eq "featherdoc.project_template_schema_approval_history.v1") {
        return "schema_approval_history"
    }
    if ($null -ne (Get-JsonProperty -Object $Json -Name "onboarding_entry_count") -and
        $null -ne (Get-JsonProperty -Object $Json -Name "entries")) {
        return "onboarding_plan"
    }

    $leaf = [System.IO.Path]::GetFileName($Path)
    if ($leaf -eq "onboarding_summary.json" -or
        $null -ne (Get-JsonProperty -Object $Json -Name "schema_approval_state")) {
        return "onboarding_summary"
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

function New-ReleaseBlocker {
    param(
        [string]$Id,
        [string]$Source,
        [string]$Status,
        [string]$Action,
        [string]$Message
    )

    return [ordered]@{
        id = $Id
        source = $Source
        severity = "error"
        status = $Status
        action = $Action
        message = $Message
    }
}

function New-ReadinessTemplate {
    param(
        [string]$RepoRoot,
        [string]$TemplateName,
        [string]$InputDocx,
        [string]$SourceKind,
        [string]$SourceJson,
        $SchemaApprovalState,
        [string]$OnboardingStatus = "",
        [object[]]$ReleaseBlockers = @(),
        [object[]]$ActionItems = @(),
        [object[]]$ManualReviewRecommendations = @()
    )

    $schemaStatus = Get-JsonString -Object $SchemaApprovalState -Name "status" -DefaultValue "not_evaluated"
    $gateStatus = Get-JsonString -Object $SchemaApprovalState -Name "gate_status" -DefaultValue $schemaStatus
    $schemaAction = Get-JsonString -Object $SchemaApprovalState -Name "action" -DefaultValue "run_project_template_smoke_then_review_schema_patch_approval"
    $releaseBlocked = Get-JsonBool -Object $SchemaApprovalState -Name "release_blocked" -DefaultValue ($schemaStatus -in @("not_evaluated", "pending_review", "blocked"))
    $blockers = @($ReleaseBlockers | Where-Object { $null -ne $_ })
    $items = @($ActionItems | Where-Object { $null -ne $_ })
    $recommendations = @($ManualReviewRecommendations | Where-Object { $null -ne $_ })
    if ($blockers.Count -eq 0 -and $releaseBlocked) {
        $blockerId = if ($schemaStatus -eq "not_evaluated") {
            "project_template_delivery_readiness.schema_approval_not_evaluated"
        } else {
            "project_template_delivery_readiness.schema_approval"
        }
        $blockers = @(
            New-ReleaseBlocker `
                -Id $blockerId `
                -Source $SourceKind `
                -Status $schemaStatus `
                -Action $schemaAction `
                -Message "Schema approval is not release-ready for this template."
        )
    }
    if ([string]::IsNullOrWhiteSpace($OnboardingStatus)) {
        $OnboardingStatus = if ($releaseBlocked) { "blocked" } else { "ready" }
    }

    return [ordered]@{
        template_name = $TemplateName
        input_docx = $InputDocx
        input_docx_display = if ([string]::IsNullOrWhiteSpace($InputDocx)) { "" } else { Get-DisplayPath -RepoRoot $RepoRoot -Path (Resolve-RepoPath -RepoRoot $RepoRoot -Path $InputDocx -AllowMissing) }
        onboarding_status = $OnboardingStatus
        schema_approval_status = $schemaStatus
        gate_status = $gateStatus
        schema_approval_action = $schemaAction
        release_blocked = $releaseBlocked
        release_ready = (-not $releaseBlocked)
        release_blockers = @($blockers)
        release_blocker_count = @($blockers).Count
        action_items = @($items)
        action_item_count = @($items).Count
        manual_review_recommendations = @($recommendations)
        manual_review_recommendation_count = @($recommendations).Count
        schema_approval_state = $SchemaApprovalState
        schema_history_available = $false
        schema_history = [ordered]@{
            status = "not_available"
            latest_status = ""
            latest_decision = ""
            latest_action = ""
            run_count = 0
            blocked_run_count = 0
        }
        source_kind = $SourceKind
        source_json = $SourceJson
        source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $SourceJson
    }
}

function Convert-OnboardingGovernanceToTemplates {
    param([string]$RepoRoot, [string]$Path, $Json)

    return @(
        foreach ($entry in @(Get-JsonArray -Object $Json -Name "entries")) {
            $state = Get-JsonProperty -Object $entry -Name "schema_approval_state"
            $name = Get-JsonString -Object $entry -Name "name" -DefaultValue "project-template"
            New-ReadinessTemplate `
                -RepoRoot $RepoRoot `
                -TemplateName $name `
                -InputDocx (Get-JsonString -Object $entry -Name "input_docx") `
                -SourceKind "onboarding_governance_report" `
                -SourceJson $Path `
                -SchemaApprovalState $state `
                -ReleaseBlockers (Get-JsonArray -Object $entry -Name "release_blockers") `
                -ActionItems (Get-JsonArray -Object $entry -Name "action_items") `
                -ManualReviewRecommendations (Get-JsonArray -Object $entry -Name "manual_review_recommendations")
        }
    )
}

function Convert-OnboardingSummaryToTemplates {
    param([string]$RepoRoot, [string]$Path, $Json)

    $name = Get-JsonString -Object $Json -Name "template_name"
    if ([string]::IsNullOrWhiteSpace($name)) {
        $name = Split-Path -Leaf (Split-Path -Parent $Path)
    }

    return @(
        New-ReadinessTemplate `
            -RepoRoot $RepoRoot `
            -TemplateName $name `
            -InputDocx (Get-JsonString -Object $Json -Name "input_docx") `
            -SourceKind "onboarding_summary" `
            -SourceJson $Path `
            -SchemaApprovalState (Get-JsonProperty -Object $Json -Name "schema_approval_state") `
            -OnboardingStatus (Get-JsonString -Object $Json -Name "status") `
            -ReleaseBlockers (Get-JsonArray -Object $Json -Name "release_blockers") `
            -ActionItems (Get-JsonArray -Object $Json -Name "action_items") `
            -ManualReviewRecommendations (Get-JsonArray -Object $Json -Name "manual_review_recommendations")
    )
}

function Convert-OnboardingPlanToTemplates {
    param([string]$RepoRoot, [string]$Path, $Json)

    return @(
        foreach ($entry in @(Get-JsonArray -Object $Json -Name "entries")) {
            New-ReadinessTemplate `
                -RepoRoot $RepoRoot `
                -TemplateName (Get-JsonString -Object $entry -Name "name" -DefaultValue "project-template") `
                -InputDocx (Get-JsonString -Object $entry -Name "input_docx") `
                -SourceKind "onboarding_plan" `
                -SourceJson $Path `
                -SchemaApprovalState (Get-JsonProperty -Object $entry -Name "schema_approval_state") `
                -OnboardingStatus (Get-JsonString -Object $entry -Name "status") `
                -ReleaseBlockers (Get-JsonArray -Object $entry -Name "release_blockers") `
                -ActionItems (Get-JsonArray -Object $entry -Name "action_items") `
                -ManualReviewRecommendations (Get-JsonArray -Object $entry -Name "manual_review_recommendations")
        }
    )
}

function Get-HistoryReadinessStatus {
    param($HistoryEntry)

    $latestStatus = Get-JsonString -Object $HistoryEntry -Name "latest_status"
    $latestDecision = Get-JsonString -Object $HistoryEntry -Name "latest_decision"
    if ($latestStatus -in @("invalid_result", "rejected", "needs_changes", "blocked") -or
        $latestDecision -in @("rejected", "needs_changes")) {
        return "blocked"
    }
    if ($latestStatus -eq "pending_review" -or $latestDecision -eq "pending") {
        return "pending_review"
    }
    if ($latestStatus -eq "approved" -or $latestDecision -eq "approved") {
        return "approved"
    }
    if ([string]::IsNullOrWhiteSpace($latestStatus) -and [string]::IsNullOrWhiteSpace($latestDecision)) {
        return "not_available"
    }
    return $latestStatus
}

function Add-HistoryToTemplates {
    param([object[]]$Templates, [object[]]$Histories, $Warnings)

    $historyByName = @{}
    foreach ($history in @($Histories)) {
        foreach ($entry in @(Get-JsonArray -Object $history -Name "entry_histories")) {
            $name = Get-JsonString -Object $entry -Name "name"
            if ([string]::IsNullOrWhiteSpace($name)) { continue }
            $historyByName[$name.ToLowerInvariant()] = $entry
        }
    }

    foreach ($template in @($Templates)) {
        $key = ([string]$template.template_name).ToLowerInvariant()
        if (-not $historyByName.ContainsKey($key)) {
            $Warnings.Add([ordered]@{
                id = "schema_approval_history_missing_for_template"
                action = "review_schema_approval_history"
                source_schema = "featherdoc.project_template_delivery_readiness_report.v1"
                template_name = [string]$template.template_name
                message = "No schema approval history entry matched this template name."
            }) | Out-Null
            continue
        }

        $historyEntry = $historyByName[$key]
        $historyStatus = Get-HistoryReadinessStatus -HistoryEntry $historyEntry
        $template["schema_history_available"] = $true
        $template["schema_history"] = [ordered]@{
            status = $historyStatus
            latest_status = Get-JsonString -Object $historyEntry -Name "latest_status"
            latest_decision = Get-JsonString -Object $historyEntry -Name "latest_decision"
            latest_action = Get-JsonString -Object $historyEntry -Name "latest_action"
            latest_generated_at = Get-JsonString -Object $historyEntry -Name "latest_generated_at"
            latest_summary_json = Get-JsonString -Object $historyEntry -Name "latest_summary_json"
            run_count = Get-JsonInt -Object $historyEntry -Name "run_count"
            blocked_run_count = Get-JsonInt -Object $historyEntry -Name "blocked_run_count"
            pending_run_count = Get-JsonInt -Object $historyEntry -Name "pending_run_count"
            approved_run_count = Get-JsonInt -Object $historyEntry -Name "approved_run_count"
            issue_keys = @(Get-JsonArray -Object $historyEntry -Name "issue_keys")
        }

        if ($historyStatus -in @("blocked", "pending_review")) {
            $blockers = New-Object 'System.Collections.Generic.List[object]'
            foreach ($blocker in @($template.release_blockers)) {
                $blockers.Add($blocker) | Out-Null
            }
            $blockers.Add((New-ReleaseBlocker `
                -Id "project_template_delivery_readiness.schema_approval_history" `
                -Source "schema_approval_history" `
                -Status $historyStatus `
                -Action (Get-JsonString -Object $historyEntry -Name "latest_action" -DefaultValue "review_schema_approval_history") `
                -Message "Latest schema approval history entry is not release-ready.")) | Out-Null
            $template["release_blocked"] = $true
            $template["release_ready"] = $false
            $template["release_blockers"] = @($blockers.ToArray())
            $template["release_blocker_count"] = $blockers.Count
        }
    }
}

function Get-LatestHistory {
    param([object[]]$Histories)

    $sorted = @($Histories | Sort-Object @{ Expression = { Get-JsonString -Object $_ -Name "generated_at" } })
    if ($sorted.Count -eq 0) { return $null }
    return $sorted[-1]
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Project Template Delivery Readiness Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Templates: ``$($Summary.template_count)``") | Out-Null
    $lines.Add("- Ready templates: ``$($Summary.ready_template_count)``") | Out-Null
    $lines.Add("- Blocked templates: ``$($Summary.blocked_template_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Latest schema approval gate: ``$($Summary.latest_schema_approval_gate_status)``") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("## Schema Approval History") | Out-Null
    $lines.Add("") | Out-Null
    if ($Summary.schema_history_count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        $lines.Add("- History reports: ``$($Summary.schema_history_count)``") | Out-Null
        $lines.Add("- Latest gate: ``$($Summary.latest_schema_approval_gate_status)``") | Out-Null
        $lines.Add("- Blocked runs: ``$($Summary.schema_history_blocked_run_count)``") | Out-Null
        $lines.Add("- Pending runs: ``$($Summary.schema_history_pending_run_count)``") | Out-Null
        $lines.Add("- Passed runs: ``$($Summary.schema_history_passed_run_count)``") | Out-Null
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Templates") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.templates).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($template in @($Summary.templates)) {
            $lines.Add(("- ``{0}``: onboarding=``{1}`` schema=``{2}`` history=``{3}`` blockers=``{4}`` source=``{5}``" -f
                $template.template_name,
                $template.onboarding_status,
                $template.schema_approval_status,
                $template.schema_history.status,
                $template.release_blocker_count,
                $template.source_kind)) | Out-Null
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
    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.template_name)`` / ``$($item.id)``: action=``$($item.action)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    $warningLines = New-Object 'System.Collections.Generic.List[string]'
    if (-not (Add-ReleaseGovernanceWarningMarkdownSubsection `
                -Lines $warningLines `
                -Heading "Project template delivery readiness warnings" `
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
    foreach ($source in @($Summary.source_files)) {
        $lines.Add("- ``$($source.path_display)``: kind=``$($source.kind)`` status=``$($source.status)``") | Out-Null
    }
    if (@($Summary.source_files).Count -eq 0) {
        $lines.Add("- none") | Out-Null
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
    Join-Path $resolvedOutputDir "project_template_delivery_readiness.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) input JSON file(s)"

$templates = New-Object 'System.Collections.Generic.List[object]'
$histories = New-Object 'System.Collections.Generic.List[object]'
$sourceFiles = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'

foreach ($path in @($inputPaths)) {
    $kind = "unknown"
    $status = "ignored"
    $errorMessage = ""
    try {
        $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $kind = Get-EvidenceKind -Json $json -Path $path
        switch ($kind) {
            "onboarding_governance_report" {
                foreach ($template in @(Convert-OnboardingGovernanceToTemplates -RepoRoot $repoRoot -Path $path -Json $json)) {
                    $templates.Add($template) | Out-Null
                }
                $status = "loaded"
            }
            "onboarding_summary" {
                foreach ($template in @(Convert-OnboardingSummaryToTemplates -RepoRoot $repoRoot -Path $path -Json $json)) {
                    $templates.Add($template) | Out-Null
                }
                $status = "loaded"
            }
            "onboarding_plan" {
                foreach ($template in @(Convert-OnboardingPlanToTemplates -RepoRoot $repoRoot -Path $path -Json $json)) {
                    $templates.Add($template) | Out-Null
                }
                $status = "loaded"
            }
            "schema_approval_history" {
                $histories.Add($json) | Out-Null
                $status = "loaded"
            }
            "project_template_delivery_readiness_report" {
                $status = "skipped"
            }
            default {
                $status = "skipped"
                $warnings.Add([ordered]@{
                    id = "source_json_schema_skipped"
                    action = "provide_project_template_readiness_evidence"
                    source_schema = "featherdoc.project_template_delivery_readiness_report.v1"
                    source_json = $path
                    source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    message = "Input JSON kind '$kind' is not project-template readiness evidence."
                }) | Out-Null
            }
        }
    } catch {
        $status = "failed"
        $errorMessage = $_.Exception.Message
        $warnings.Add([ordered]@{
            id = "source_json_read_failed"
            action = "fix_project_template_readiness_input_json"
            source_schema = "featherdoc.project_template_delivery_readiness_report.v1"
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

if ($templates.Count -eq 0) {
    $warnings.Add([ordered]@{
        id = "template_evidence_missing"
        action = "provide_project_template_readiness_evidence"
        source_schema = "featherdoc.project_template_delivery_readiness_report.v1"
        message = "No onboarding template evidence was loaded."
    }) | Out-Null
}

Add-HistoryToTemplates -Templates $templates.ToArray() -Histories $histories.ToArray() -Warnings $warnings

$latestHistory = Get-LatestHistory -Histories $histories.ToArray()
$latestGateStatus = if ($null -ne $latestHistory) {
    Get-JsonString -Object $latestHistory -Name "latest_gate_status" -DefaultValue "not_available"
} else {
    "not_available"
}
$historyBlockedRunCount = if ($null -ne $latestHistory) { Get-JsonInt -Object $latestHistory -Name "blocked_run_count" } else { 0 }
$historyPendingRunCount = if ($null -ne $latestHistory) { Get-JsonInt -Object $latestHistory -Name "pending_run_count" } else { 0 }
$historyPassedRunCount = if ($null -ne $latestHistory) { Get-JsonInt -Object $latestHistory -Name "passed_run_count" } else { 0 }

$globalReleaseBlockers = New-Object 'System.Collections.Generic.List[object]'
if ($latestGateStatus -in @("blocked", "pending")) {
    $globalReleaseBlockers.Add((New-ReleaseBlocker `
        -Id "project_template_delivery_readiness.schema_approval_history_gate" `
        -Source "schema_approval_history" `
        -Status $latestGateStatus `
        -Action "review_schema_approval_history" `
        -Message "Latest schema approval history gate is not release-ready.")) | Out-Null
}

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'
$manualReviewRecommendations = New-Object 'System.Collections.Generic.List[object]'

foreach ($blocker in @($globalReleaseBlockers.ToArray())) {
    $releaseBlockers.Add([ordered]@{
        scope = "global"
        template_name = ""
        source_kind = Get-JsonString -Object $blocker -Name "source"
        id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
        severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "error"
        status = Get-JsonString -Object $blocker -Name "status"
        action = Get-JsonString -Object $blocker -Name "action"
        message = Get-JsonString -Object $blocker -Name "message"
    }) | Out-Null
}

foreach ($template in @($templates.ToArray())) {
    foreach ($blocker in @($template.release_blockers)) {
        $releaseBlockers.Add([ordered]@{
            scope = [string]$template.template_name
            template_name = [string]$template.template_name
            source_kind = [string]$template.source_kind
            source_json = [string]$template.source_json
            id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
            severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "error"
            status = Get-JsonString -Object $blocker -Name "status" -DefaultValue ([string]$template.schema_approval_status)
            action = Get-JsonString -Object $blocker -Name "action" -DefaultValue ([string]$template.schema_approval_action)
            message = Get-JsonString -Object $blocker -Name "message" -DefaultValue "Release blocker requires review."
        }) | Out-Null
    }
    foreach ($item in @($template.action_items)) {
        $actionItems.Add([ordered]@{
            template_name = [string]$template.template_name
            source_kind = [string]$template.source_kind
            id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
            action = Get-JsonString -Object $item -Name "action"
            title = Get-JsonString -Object $item -Name "title"
            command = Get-JsonString -Object $item -Name "command"
        }) | Out-Null
    }
    foreach ($recommendation in @($template.manual_review_recommendations)) {
        $manualReviewRecommendations.Add([ordered]@{
            template_name = [string]$template.template_name
            source_kind = [string]$template.source_kind
            id = Get-JsonString -Object $recommendation -Name "id" -DefaultValue "manual_review"
            priority = Get-JsonString -Object $recommendation -Name "priority"
            action = Get-JsonString -Object $recommendation -Name "action"
            title = Get-JsonString -Object $recommendation -Name "title"
            artifact = Get-JsonString -Object $recommendation -Name "artifact"
        }) | Out-Null
    }
}

$sourceFailureCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$blockedTemplateCount = @($templates.ToArray() | Where-Object { [bool]$_.release_blocked }).Count
$readyTemplateCount = @($templates.ToArray() | Where-Object { -not [bool]$_.release_blocked }).Count
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0 -or $blockedTemplateCount -gt 0) {
    "blocked"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = "featherdoc.project_template_delivery_readiness_report.v1"
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
    schema_history_count = $histories.Count
    latest_schema_approval_gate_status = $latestGateStatus
    schema_history_blocked_run_count = $historyBlockedRunCount
    schema_history_pending_run_count = $historyPendingRunCount
    schema_history_passed_run_count = $historyPassedRunCount
    template_count = $templates.Count
    ready_template_count = $readyTemplateCount
    blocked_template_count = $blockedTemplateCount
    templates = @($templates.ToArray())
    template_status_summary = @(Add-SummaryGroup -Items $templates.ToArray() -PropertyName "onboarding_status" -OutputName "status")
    schema_approval_status_summary = @(Add-SummaryGroup -Items $templates.ToArray() -PropertyName "schema_approval_status" -OutputName "status")
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    blocker_id_summary = @(Add-SummaryGroup -Items $releaseBlockers.ToArray() -PropertyName "id" -OutputName "id")
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    action_item_summary = @(Add-SummaryGroup -Items $actionItems.ToArray() -PropertyName "action" -OutputName "action")
    manual_review_recommendation_count = $manualReviewRecommendations.Count
    manual_review_recommendations = @($manualReviewRecommendations.ToArray())
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
