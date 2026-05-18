<#
.SYNOPSIS
Builds a project-level onboarding governance report from existing JSON evidence.

.DESCRIPTION
Reads project template onboarding summaries, onboarding plan files, and
project-template-smoke summaries, then aggregates schema approval state,
release blockers, action items, and manual-review recommendations into one
JSON/Markdown handoff. The script is read-only for input artifacts.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/project-template-onboarding-governance",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnBlocker
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$onboardingGovernanceSchema = "featherdoc.project_template_onboarding_governance_report.v1"
$onboardingGovernanceOpenCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_onboarding_governance_report.ps1"

function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-onboarding-governance] $Message"
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

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return $Object[$Name]
        }
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
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    return [string]$value
}

function Get-JsonInt {
    param(
        $Object,
        [string]$Name,
        [int]$DefaultValue = 0
    )

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }

    $parsed = 0
    if ([int]::TryParse([string]$value, [ref]$parsed)) {
        return $parsed
    }
    return $DefaultValue
}

function Get-JsonBool {
    param(
        $Object,
        [string]$Name,
        [bool]$DefaultValue = $false
    )

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }

    if ($value -is [bool]) {
        return [bool]$value
    }

    return [string]$value -in @("true", "True", "1", "yes", "Yes")
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

    if ($value -is [string]) {
        return @($value)
    }

    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Get-InputJsonPaths {
    param(
        [string]$RepoRoot,
        [string[]]$ExplicitPaths,
        [string[]]$Roots
    )

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @($ExplicitPaths)) {
        if ([string]::IsNullOrWhiteSpace($path)) {
            continue
        }
        $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
    }

    $scanRoots = @($Roots | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @(
            "output/project-template-onboarding",
            "output/project-template-smoke-onboarding-plan",
            "output/project-template-smoke"
        )
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) {
            continue
        }

        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter "*.json" |
                Where-Object {
                    $_.Name -in @("onboarding_summary.json", "plan.json", "summary.json")
                } |
                ForEach-Object { $paths.Add($_.FullName) | Out-Null }
        } else {
            $paths.Add($resolvedRoot) | Out-Null
        }
    }

    return @($paths.ToArray() | Sort-Object -Unique)
}

function Read-JsonEvidence {
    param([string]$Path)

    $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
    return $json
}

function Get-EvidenceKind {
    param(
        $Json,
        [string]$Path
    )

    $leaf = [System.IO.Path]::GetFileName($Path)
    if ($null -ne (Get-JsonProperty -Object $Json -Name "onboarding_entry_count") -and
        $null -ne (Get-JsonProperty -Object $Json -Name "entries")) {
        return "onboarding_plan"
    }
    if ($leaf -eq "onboarding_summary.json" -or
        $null -ne (Get-JsonProperty -Object $Json -Name "schema_approval_state")) {
        return "onboarding_summary"
    }
    if ($null -ne (Get-JsonProperty -Object $Json -Name "schema_patch_approval_gate_status") -or
        $null -ne (Get-JsonProperty -Object $Json -Name "schema_patch_approval_items") -or
        $null -ne (Get-JsonProperty -Object $Json -Name "schema_patch_review_count")) {
        return "project_template_smoke_summary"
    }

    return "unknown"
}

function New-SchemaApprovalStateFromSmoke {
    param(
        $SmokeSummary,
        $ApprovalItem = $null
    )

    $gateStatus = Get-JsonString -Object $SmokeSummary -Name "schema_patch_approval_gate_status" -DefaultValue "not_required"
    $pendingCount = Get-JsonInt -Object $SmokeSummary -Name "schema_patch_approval_pending_count"
    $approvedCount = Get-JsonInt -Object $SmokeSummary -Name "schema_patch_approval_approved_count"
    $rejectedCount = Get-JsonInt -Object $SmokeSummary -Name "schema_patch_approval_rejected_count"
    $complianceIssueCount = Get-JsonInt -Object $SmokeSummary -Name "schema_patch_approval_compliance_issue_count"
    $invalidResultCount = Get-JsonInt -Object $SmokeSummary -Name "schema_patch_approval_invalid_result_count"
    $changedReviewCount = Get-JsonInt -Object $SmokeSummary -Name "schema_patch_review_changed_count"
    $reviewCount = Get-JsonInt -Object $SmokeSummary -Name "schema_patch_review_count"
    $action = "none"

    if ($null -ne $ApprovalItem) {
        $itemStatus = Get-JsonString -Object $ApprovalItem -Name "status"
        $itemDecision = Get-JsonString -Object $ApprovalItem -Name "decision"
        $itemAction = Get-JsonString -Object $ApprovalItem -Name "action"
        if (-not [string]::IsNullOrWhiteSpace($itemAction)) {
            $action = $itemAction
        }

        if ($itemStatus -eq "approved" -or $itemDecision -eq "approved") {
            $status = "approved"
        } elseif ($itemStatus -eq "pending_review" -or $itemDecision -eq "pending") {
            $status = "pending_review"
        } elseif ($itemStatus -in @("rejected", "needs_changes", "invalid_result") -or
                  $itemDecision -in @("rejected", "needs_changes")) {
            $status = "blocked"
        } else {
            $status = if ($gateStatus -eq "not_required") { "not_required" } else { "pending_review" }
        }

        $itemComplianceIssueCount = Get-JsonInt -Object $ApprovalItem -Name "compliance_issue_count"
        if ($itemComplianceIssueCount -gt 0) {
            $status = "blocked"
        }
    } else {
        if ($gateStatus -eq "blocked" -or $complianceIssueCount -gt 0 -or $invalidResultCount -gt 0 -or $rejectedCount -gt 0) {
            $status = "blocked"
            $action = "fix_schema_patch_approval_result"
        } elseif ($gateStatus -eq "pending" -or $pendingCount -gt 0) {
            $status = "pending_review"
            $action = "review_schema_update_candidate"
        } elseif ($gateStatus -eq "passed" -or $approvedCount -gt 0 -or $changedReviewCount -gt 0) {
            $status = "approved"
            $action = "promote_schema_update_candidate"
        } else {
            $status = "not_required"
        }
    }

    return [ordered]@{
        schema = "featherdoc.project_template_onboarding_schema_approval_state.v1"
        status = $status
        gate_status = $gateStatus
        release_blocked = ($status -in @("not_evaluated", "pending_review", "blocked"))
        smoke_summary_available = $true
        smoke_skipped = $false
        approval_required = ($changedReviewCount -gt 0 -or @((Get-JsonArray -Object $SmokeSummary -Name "schema_patch_approval_items")).Count -gt 0)
        review_count = $reviewCount
        changed_review_count = $changedReviewCount
        pending_count = $pendingCount
        approved_count = $approvedCount
        rejected_count = $rejectedCount
        compliance_issue_count = $complianceIssueCount
        invalid_result_count = $invalidResultCount
        action = $action
    }
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
        source_schema = $onboardingGovernanceSchema
        severity = "error"
        status = $Status
        message = $Message
        action = $Action
    }
}

function New-GovernanceEntry {
    param(
        [string]$Name,
        [string]$SourceKind,
        [string]$SourceJson,
        [string]$InputDocx,
        $SchemaApprovalState,
        [object[]]$ReleaseBlockers = @(),
        [object[]]$ActionItems = @(),
        [object[]]$ManualReviewRecommendations = @(),
        [object[]]$SchemaPatchApprovalItems = @()
    )

    $stateStatus = Get-JsonString -Object $SchemaApprovalState -Name "status" -DefaultValue "not_evaluated"
    $stateAction = Get-JsonString -Object $SchemaApprovalState -Name "action" -DefaultValue "run_project_template_smoke_then_review_schema_patch_approval"
    $stateBlocked = Get-JsonBool -Object $SchemaApprovalState -Name "release_blocked" -DefaultValue ($stateStatus -in @("not_evaluated", "pending_review", "blocked"))

    $blockers = @($ReleaseBlockers)
    if ($blockers.Count -eq 0 -and $stateBlocked) {
        $blockerId = if ($stateStatus -eq "not_evaluated") {
            "project_template_onboarding.schema_approval_not_evaluated"
        } else {
            "project_template_onboarding.schema_approval"
        }
        $blockers = @(
            New-ReleaseBlocker `
                -Id $blockerId `
                -Source $SourceKind `
                -Status $stateStatus `
                -Action $stateAction `
                -Message "Schema approval is not release-ready for this template."
        )
    }

    return [ordered]@{
        name = $Name
        source_kind = $SourceKind
        source_json = $SourceJson
        input_docx = $InputDocx
        schema_approval_state = $SchemaApprovalState
        schema_approval_status = $stateStatus
        schema_approval_action = $stateAction
        release_blocked = $stateBlocked
        release_blockers = @($blockers)
        release_blocker_count = @($blockers).Count
        action_items = @($ActionItems)
        manual_review_recommendations = @($ManualReviewRecommendations)
        schema_patch_approval_items = @($SchemaPatchApprovalItems)
    }
}

function Convert-OnboardingSummaryToEntries {
    param(
        [string]$Path,
        $Json
    )

    $name = Get-JsonString -Object $Json -Name "template_name"
    if ([string]::IsNullOrWhiteSpace($name)) {
        $name = [System.IO.Path]::GetFileNameWithoutExtension([System.IO.Path]::GetDirectoryName($Path))
    }

    return @(
        New-GovernanceEntry `
            -Name $name `
            -SourceKind "onboarding_summary" `
            -SourceJson $Path `
            -InputDocx (Get-JsonString -Object $Json -Name "input_docx") `
            -SchemaApprovalState (Get-JsonProperty -Object $Json -Name "schema_approval_state") `
            -ReleaseBlockers (Get-JsonArray -Object $Json -Name "release_blockers") `
            -ActionItems (Get-JsonArray -Object $Json -Name "action_items") `
            -ManualReviewRecommendations (Get-JsonArray -Object $Json -Name "manual_review_recommendations") `
            -SchemaPatchApprovalItems (Get-JsonArray -Object $Json -Name "schema_patch_approval_items")
    )
}

function Convert-OnboardingPlanToEntries {
    param(
        [string]$Path,
        $Json
    )

    return @(
        foreach ($entry in @(Get-JsonArray -Object $Json -Name "entries")) {
            New-GovernanceEntry `
                -Name (Get-JsonString -Object $entry -Name "name" -DefaultValue "candidate") `
                -SourceKind "onboarding_plan" `
                -SourceJson $Path `
                -InputDocx (Get-JsonString -Object $entry -Name "input_docx") `
                -SchemaApprovalState (Get-JsonProperty -Object $entry -Name "schema_approval_state") `
                -ReleaseBlockers (Get-JsonArray -Object $entry -Name "release_blockers") `
                -ActionItems (Get-JsonArray -Object $entry -Name "action_items") `
                -ManualReviewRecommendations (Get-JsonArray -Object $entry -Name "manual_review_recommendations")
        }
    )
}

function Convert-SmokeSummaryToEntries {
    param(
        [string]$Path,
        $Json
    )

    $approvalItems = @(Get-JsonArray -Object $Json -Name "schema_patch_approval_items")
    if ($approvalItems.Count -eq 0) {
        return @(
            New-GovernanceEntry `
                -Name "project-template-smoke" `
                -SourceKind "project_template_smoke_summary" `
                -SourceJson $Path `
                -InputDocx "" `
                -SchemaApprovalState (New-SchemaApprovalStateFromSmoke -SmokeSummary $Json) `
                -SchemaPatchApprovalItems @()
        )
    }

    return @(
        foreach ($approval in $approvalItems) {
            $name = Get-JsonString -Object $approval -Name "name" -DefaultValue "schema_patch_approval_item"
            New-GovernanceEntry `
                -Name $name `
                -SourceKind "project_template_smoke_summary" `
                -SourceJson $Path `
                -InputDocx "" `
                -SchemaApprovalState (New-SchemaApprovalStateFromSmoke -SmokeSummary $Json -ApprovalItem $approval) `
                -SchemaPatchApprovalItems @($approval)
        }
    )
}

function New-StatusSummary {
    param([object[]]$Entries)

    return @(
        foreach ($group in @($Entries | Group-Object schema_approval_status | Sort-Object Name)) {
            [ordered]@{
                status = [string]$group.Name
                count = [int]$group.Count
            }
        }
    )
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Project Template Onboarding Governance Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Source files: ``$($Summary.source_file_count)``") | Out-Null
    $lines.Add("- Template entries: ``$($Summary.entry_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("## Schema Approval Status") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($item in @($Summary.schema_approval_status_summary)) {
        $lines.Add("- ``$($item.status)``: $($item.count)") | Out-Null
    }
    if (@($Summary.schema_approval_status_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Entries") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($entry in @($Summary.entries)) {
        $lines.Add("- ``$($entry.name)``: status=``$($entry.schema_approval_status)`` blockers=``$($entry.release_blocker_count)`` source=``$($entry.source_kind)``") | Out-Null
    }
    if (@($Summary.entries).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.entry_name)`` / ``$($blocker.id)``: action=``$($blocker.action)`` schema=``$($blocker.source_schema)`` source_json_display=``$($blocker.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.message)) {
                $lines.Add("  - message: $($blocker.message)") | Out-Null
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
            $lines.Add("- ``$($item.entry_name)`` / ``$($item.id)``: action=``$($item.action)`` schema=``$($item.source_schema)`` source_json_display=``$($item.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Source Files") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($source in @($Summary.source_files)) {
        $lines.Add("- ``$($source.path_display)``: kind=``$($source.kind)`` status=``$($source.status)``") | Out-Null
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
    Join-Path $resolvedOutputDir "project_template_onboarding_governance.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$jsonPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($jsonPaths.Count) input JSON file(s)"

$entries = New-Object 'System.Collections.Generic.List[object]'
$sourceFiles = New-Object 'System.Collections.Generic.List[object]'

foreach ($path in @($jsonPaths)) {
    $kind = "unknown"
    $status = "ignored"
    $errorMessage = ""
    try {
        $json = Read-JsonEvidence -Path $path
        $kind = Get-EvidenceKind -Json $json -Path $path
        switch ($kind) {
            "onboarding_summary" {
                foreach ($entry in @(Convert-OnboardingSummaryToEntries -Path $path -Json $json)) {
                    $entries.Add($entry) | Out-Null
                }
                $status = "loaded"
            }
            "onboarding_plan" {
                foreach ($entry in @(Convert-OnboardingPlanToEntries -Path $path -Json $json)) {
                    $entries.Add($entry) | Out-Null
                }
                $status = "loaded"
            }
            "project_template_smoke_summary" {
                foreach ($entry in @(Convert-SmokeSummaryToEntries -Path $path -Json $json)) {
                    $entries.Add($entry) | Out-Null
                }
                $status = "loaded"
            }
            default {
                $status = "ignored"
            }
        }
    } catch {
        $status = "failed"
        $errorMessage = $_.Exception.Message
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
$manualReviewRecommendations = New-Object 'System.Collections.Generic.List[object]'

foreach ($entry in @($entries.ToArray())) {
    foreach ($blocker in @($entry.release_blockers)) {
        $releaseBlockers.Add([ordered]@{
            entry_name = [string]$entry.name
            source_kind = [string]$entry.source_kind
            source_schema = Get-JsonString -Object $blocker -Name "source_schema" -DefaultValue $onboardingGovernanceSchema
            source_json = Get-JsonString -Object $blocker -Name "source_json" -DefaultValue ([string]$entry.source_json)
            source_json_display = Get-JsonString -Object $blocker -Name "source_json_display" -DefaultValue (Get-DisplayPath -RepoRoot $repoRoot -Path ([string]$entry.source_json))
            id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
            severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "error"
            status = Get-JsonString -Object $blocker -Name "status" -DefaultValue ([string]$entry.schema_approval_status)
            action = Get-JsonString -Object $blocker -Name "action" -DefaultValue ([string]$entry.schema_approval_action)
            message = Get-JsonString -Object $blocker -Name "message" -DefaultValue "Release blocker requires review."
        }) | Out-Null
    }
    foreach ($item in @($entry.action_items)) {
        $actionItems.Add([ordered]@{
            entry_name = [string]$entry.name
            source_kind = [string]$entry.source_kind
            source_schema = Get-JsonString -Object $item -Name "source_schema" -DefaultValue $onboardingGovernanceSchema
            source_json = Get-JsonString -Object $item -Name "source_json" -DefaultValue ([string]$entry.source_json)
            source_json_display = Get-JsonString -Object $item -Name "source_json_display" -DefaultValue (Get-DisplayPath -RepoRoot $repoRoot -Path ([string]$entry.source_json))
            id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
            action = Get-JsonString -Object $item -Name "action"
            title = Get-JsonString -Object $item -Name "title"
            command = Get-JsonString -Object $item -Name "command"
            open_command = Get-JsonString -Object $item -Name "open_command" -DefaultValue (Get-JsonString -Object $item -Name "command" -DefaultValue $onboardingGovernanceOpenCommand)
        }) | Out-Null
    }
    foreach ($recommendation in @($entry.manual_review_recommendations)) {
        $manualReviewRecommendations.Add([ordered]@{
            entry_name = [string]$entry.name
            source_kind = [string]$entry.source_kind
            id = Get-JsonString -Object $recommendation -Name "id" -DefaultValue "manual_review"
            priority = Get-JsonString -Object $recommendation -Name "priority"
            action = Get-JsonString -Object $recommendation -Name "action"
            title = Get-JsonString -Object $recommendation -Name "title"
            artifact = Get-JsonString -Object $recommendation -Name "artifact"
        }) | Out-Null
    }
}

$sourceFailureCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$releaseBlockerCount = $releaseBlockers.Count
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($releaseBlockerCount -gt 0) {
    "blocked"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = $onboardingGovernanceSchema
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    output_dir = $resolvedOutputDir
    summary_json = $summaryPath
    report_markdown = $markdownPath
    source_file_count = $sourceFiles.Count
    source_failure_count = $sourceFailureCount
    source_files = @($sourceFiles.ToArray())
    entry_count = $entries.Count
    entries = @($entries.ToArray())
    schema_approval_status_summary = @(New-StatusSummary -Entries $entries.ToArray())
    blocked_entry_count = @($entries.ToArray() | Where-Object { $_.schema_approval_status -eq "blocked" }).Count
    pending_review_entry_count = @($entries.ToArray() | Where-Object { $_.schema_approval_status -eq "pending_review" }).Count
    not_evaluated_entry_count = @($entries.ToArray() | Where-Object { $_.schema_approval_status -eq "not_evaluated" }).Count
    approved_entry_count = @($entries.ToArray() | Where-Object { $_.schema_approval_status -eq "approved" }).Count
    not_required_entry_count = @($entries.ToArray() | Where-Object { $_.schema_approval_status -eq "not_required" }).Count
    release_blockers = @($releaseBlockers.ToArray())
    release_blocker_count = $releaseBlockerCount
    action_items = @($actionItems.ToArray())
    action_item_count = $actionItems.Count
    manual_review_recommendations = @($manualReviewRecommendations.ToArray())
    manual_review_recommendation_count = $manualReviewRecommendations.Count
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) {
    exit 1
}
if ($FailOnBlocker -and $releaseBlockerCount -gt 0) {
    exit 1
}
