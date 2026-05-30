<#
.SYNOPSIS
Builds a unified numbering catalog governance report.

.DESCRIPTION
Reads document skeleton governance rollups plus numbering catalog manifest
check summaries, then writes one JSON/Markdown gate for exemplar numbering
catalog coverage, style-numbering issues, baseline drift, dirty catalog
baselines, release blockers, and action items. The script is read-only for
source artifacts.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/numbering-catalog-governance",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnIssue,
    [switch]$FailOnDrift,
    [switch]$FailOnBlocker
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[numbering-catalog-governance] $Message"
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

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @(Expand-TemplateSchemaArgumentList -Values $ExplicitPaths)) {
        $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
    }

    $scanRoots = @(Expand-TemplateSchemaArgumentList -Values $Roots)
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @(
            "output/document-skeleton-governance-rollup",
            "output/numbering-catalog-manifest-checks"
        )
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) { continue }

        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter "summary.json" |
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
    if ($schema -eq "featherdoc.numbering_catalog_governance_report.v1") {
        return "numbering_catalog_governance_report"
    }
    if ($schema -eq "featherdoc.document_skeleton_governance_rollup_report.v1" -or
        $null -ne (Get-JsonProperty -Object $Json -Name "catalog_exemplars")) {
        return "document_skeleton_governance_rollup"
    }
    if ($null -ne (Get-JsonProperty -Object $Json -Name "manifest_path") -and
        $null -ne (Get-JsonProperty -Object $Json -Name "entries") -and
        $null -ne (Get-JsonProperty -Object $Json -Name "drift_count")) {
        return "numbering_catalog_manifest_summary"
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

function Add-IssueSummary {
    param([object[]]$Items)

    $totals = @{}
    foreach ($item in @($Items | Where-Object { $null -ne $_ })) {
        $issue = Get-JsonString -Object $item -Name "issue" -DefaultValue "unspecified"
        $count = Get-JsonInt -Object $item -Name "count" -DefaultValue 1
        if ($count -lt 1) { $count = 1 }
        if (-not $totals.ContainsKey($issue)) { $totals[$issue] = 0 }
        $totals[$issue] = [int]$totals[$issue] + $count
    }

    $rows = New-Object 'System.Collections.Generic.List[object]'
    foreach ($key in $totals.Keys) {
        $rows.Add([ordered]@{
            issue = [string]$key
            count = [int]$totals[$key]
        }) | Out-Null
    }
    return @($rows.ToArray() |
        Sort-Object -Property @{ Expression = { $_.count }; Descending = $true },
            @{ Expression = { $_.issue }; Ascending = $true })
}

function Get-Percent {
    param([int]$Numerator, [int]$Denominator)

    if ($Denominator -le 0) { return 0 }
    $ratio = [Math]::Min(1.0, [double]$Numerator / [double]$Denominator)
    return [int][Math]::Round($ratio * 100, 0)
}

function Get-CanonicalDocumentKey {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) { return "" }
    $leaf = [System.IO.Path]::GetFileName([string]$Value)
    if ([string]::IsNullOrWhiteSpace($leaf)) {
        $leaf = [string]$Value
    }
    $stem = [System.IO.Path]::GetFileNameWithoutExtension($leaf)
    if ([string]::IsNullOrWhiteSpace($stem)) {
        $stem = $leaf
    }
    return $stem.Trim().ToLowerInvariant()
}

function Get-RealCorpusConfidenceLevel {
    param(
        [int]$Score,
        [int]$DocumentCount,
        [int]$CatalogExemplarCount,
        [int]$BaselineEntryCount
    )

    if ($DocumentCount -le 0 -or $CatalogExemplarCount -le 0 -or $BaselineEntryCount -le 0) {
        return "insufficient_evidence"
    }
    if ($Score -ge 90) { return "high" }
    if ($Score -ge 70) { return "medium" }
    if ($Score -ge 40) { return "low" }
    return "insufficient_evidence"
}

function New-RealCorpusConfidence {
    param(
        [int]$DocumentCount,
        [int]$CatalogExemplarCount,
        [int]$BaselineEntryCount,
        [int]$MatchedDocumentCount,
        [int]$TotalStyleNumberingIssueCount,
        [int]$TotalStyleNumberingSuggestionCount,
        [int]$DriftCount,
        [int]$DirtyBaselineCount,
        [int]$IssueEntryCount,
        [int]$TotalCommandFailureCount
    )

    $catalogCoveragePercent = Get-Percent -Numerator $MatchedDocumentCount -Denominator $DocumentCount
    $baselineCoveragePercent = Get-Percent -Numerator $MatchedDocumentCount -Denominator $BaselineEntryCount
    $coverageScore = [Math]::Min($catalogCoveragePercent, $baselineCoveragePercent)
    $unmatchedCatalogDocumentCount = [Math]::Max(0, $CatalogExemplarCount - $MatchedDocumentCount)
    $unmatchedBaselineDocumentCount = [Math]::Max(0, $BaselineEntryCount - $MatchedDocumentCount)

    $styleIssuePenalty = [Math]::Min(25, $TotalStyleNumberingIssueCount * 5)
    $suggestionPenalty = [Math]::Min(10, $TotalStyleNumberingSuggestionCount * 2)
    $catalogQualityPenalty = [Math]::Min(25, ($DriftCount + $DirtyBaselineCount + $IssueEntryCount) * 10)
    $commandFailurePenalty = [Math]::Min(40, $TotalCommandFailureCount * 20)
    $score = [int][Math]::Max(0, $coverageScore - $styleIssuePenalty - $suggestionPenalty - $catalogQualityPenalty - $commandFailurePenalty)
    $level = Get-RealCorpusConfidenceLevel `
        -Score $score `
        -DocumentCount $DocumentCount `
        -CatalogExemplarCount $CatalogExemplarCount `
        -BaselineEntryCount $BaselineEntryCount

    return [ordered]@{
        score = $score
        level = $level
        document_count = $DocumentCount
        catalog_exemplar_count = $CatalogExemplarCount
        baseline_entry_count = $BaselineEntryCount
        matched_document_count = $MatchedDocumentCount
        unmatched_catalog_document_count = $unmatchedCatalogDocumentCount
        unmatched_baseline_document_count = $unmatchedBaselineDocumentCount
        alignment_gap_count = ($unmatchedCatalogDocumentCount + $unmatchedBaselineDocumentCount)
        catalog_coverage_percent = $catalogCoveragePercent
        baseline_coverage_percent = $baselineCoveragePercent
        coverage_score = $coverageScore
        penalty_summary = @(
            [ordered]@{ factor = "style_numbering_issues"; count = $TotalStyleNumberingIssueCount; penalty = $styleIssuePenalty }
            [ordered]@{ factor = "style_numbering_suggestions"; count = $TotalStyleNumberingSuggestionCount; penalty = $suggestionPenalty }
            [ordered]@{ factor = "catalog_drift_or_dirty_baseline"; count = ($DriftCount + $DirtyBaselineCount + $IssueEntryCount); penalty = $catalogQualityPenalty }
            [ordered]@{ factor = "command_failures"; count = $TotalCommandFailureCount; penalty = $commandFailurePenalty }
        )
    }
}

function New-ReleaseBlocker {
    param(
        [string]$Id,
        [string]$Scope,
        [string]$SourceKind,
        [string]$Severity = "error",
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
        [string]$Command = "",
        [string]$Category = "remediation",
        [string]$Severity = "warning",
        [bool]$ReleaseBlocking = $true,
        [bool]$Optional = $false
    )

    return [ordered]@{
        id = $Id
        scope = $Scope
        source_kind = $SourceKind
        action = $Action
        title = $Title
        command = $Command
        open_command = $Command
        audit_command = ""
        review_command = ""
        category = $Category
        severity = $Severity
        release_blocking = $ReleaseBlocking
        optional = $Optional
    }
}

function Copy-ActionItemWithReleaseChecklistDefaults {
    param($Item)

    $copy = [ordered]@{}
    if ($Item -is [System.Collections.IDictionary]) {
        foreach ($key in @($Item.Keys)) {
            $copy[[string]$key] = $Item[$key]
        }
    } else {
        foreach ($property in @($Item.PSObject.Properties)) {
            $copy[$property.Name] = $property.Value
        }
    }

    if ([string]::IsNullOrWhiteSpace((Get-JsonString -Object $copy -Name "category"))) {
        $copy["category"] = "release_checklist"
    }
    if ([string]::IsNullOrWhiteSpace((Get-JsonString -Object $copy -Name "severity"))) {
        $copy["severity"] = "info"
    }
    $copy["release_blocking"] = $false
    $copy["optional"] = $true

    return $copy
}

function Test-InformationalActionItem {
    param($Item)

    $category = Get-JsonString -Object $Item -Name "category"
    if ($category -in @("release_checklist", "manual_release_checklist", "informational")) {
        return $true
    }

    if (Get-JsonBool -Object $Item -Name "optional") {
        return $true
    }

    $releaseBlockingProperty = Get-JsonProperty -Object $Item -Name "release_blocking"
    if ($null -ne $releaseBlockingProperty -and -not (Get-JsonBool -Object $Item -Name "release_blocking" -DefaultValue $true)) {
        return $true
    }

    $id = Get-JsonString -Object $Item -Name "id"
    $action = Get-JsonString -Object $Item -Name "action"
    return ($id -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline") -or
        $action -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline"))
}

function New-BaselineBlocker {
    param($Entry)

    $name = Get-JsonString -Object $Entry -Name "name" -DefaultValue "numbering-catalog-baseline"
    $matches = Get-JsonBool -Object $Entry -Name "matches" -DefaultValue $true
    $clean = Get-JsonBool -Object $Entry -Name "clean" -DefaultValue $true
    $catalogLintClean = Get-JsonBool -Object $Entry -Name "catalog_lint_clean" -DefaultValue $true

    if (-not $catalogLintClean) {
        return New-ReleaseBlocker `
            -Id "numbering_catalog_governance.dirty_baseline" `
            -Scope $name `
            -SourceKind "numbering_catalog_manifest_summary" `
            -Status "dirty_baseline" `
            -Action "fix_numbering_catalog_baseline_lint" `
            -Message "Committed numbering catalog baseline is not lint-clean."
    }
    if (-not $matches) {
        return New-ReleaseBlocker `
            -Id "numbering_catalog_governance.catalog_drift" `
            -Scope $name `
            -SourceKind "numbering_catalog_manifest_summary" `
            -Status "drift" `
            -Action "refresh_numbering_catalog_baseline_or_repair_docx" `
            -Message "Generated numbering catalog does not match the committed baseline."
    }
    if (-not $clean) {
        return New-ReleaseBlocker `
            -Id "numbering_catalog_governance.catalog_check_issue" `
            -Scope $name `
            -SourceKind "numbering_catalog_manifest_summary" `
            -Status "needs_review" `
            -Action "review_numbering_catalog_check_issues" `
            -Message "Numbering catalog check reported baseline or generated catalog issues."
    }
    return $null
}

function New-ActionForBaselineBlocker {
    param($Blocker, $Entry)

    if ($null -eq $Blocker) { return $null }
    $name = Get-JsonString -Object $Entry -Name "name" -DefaultValue "numbering-catalog-baseline"
    return New-ActionItem `
        -Id ([string]$Blocker.id) `
        -Scope $name `
        -SourceKind "numbering_catalog_manifest_summary" `
        -Action ([string]$Blocker.action) `
        -Title ([string]$Blocker.message)
}

function Format-MarkdownCodeList {
    param([object[]]$Values)

    $items = @(
        $Values |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Sort-Object -Unique
    )
    if ($items.Count -eq 0) { return "none" }

    return (($items | ForEach-Object { "``$_``" }) -join ", ")
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Numbering Catalog Governance Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Clean: ``$($Summary.clean)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Documents: ``$($Summary.document_count)``") | Out-Null
    $lines.Add("- Baseline entries: ``$($Summary.baseline_entry_count)``") | Out-Null
    $lines.Add("- Catalog exemplars: ``$($Summary.catalog_exemplar_count)``") | Out-Null
    $lines.Add("- Style-numbering issues: ``$($Summary.total_style_numbering_issue_count)``") | Out-Null
    $lines.Add("- Real corpus confidence: ``$($Summary.real_corpus_confidence_level)`` (score=``$($Summary.real_corpus_confidence_score)``)") | Out-Null
    $lines.Add("- Catalog drift: ``$($Summary.drift_count)``") | Out-Null
    $lines.Add("- Dirty baselines: ``$($Summary.dirty_baseline_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($Summary.action_item_count)``") | Out-Null
    $lines.Add("- Informational action items: ``$($Summary.informational_action_item_count)``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Real Corpus Confidence") | Out-Null
    $lines.Add("") | Out-Null
    $confidence = $Summary.real_corpus_confidence
    $lines.Add("- Level: ``$($confidence.level)``") | Out-Null
    $lines.Add("- Score: ``$($confidence.score)``") | Out-Null
    $lines.Add("- Matched documents: ``$($confidence.matched_document_count)`` / ``$($confidence.document_count)``") | Out-Null
    $lines.Add("- Unmatched catalog documents: ``$($confidence.unmatched_catalog_document_count)``") | Out-Null
    $lines.Add("- Unmatched baseline entries: ``$($confidence.unmatched_baseline_document_count)``") | Out-Null
    $lines.Add("- Catalog coverage: ``$($confidence.catalog_coverage_percent)%``") | Out-Null
    $lines.Add("- Baseline coverage: ``$($confidence.baseline_coverage_percent)%``") | Out-Null
    $catalogDocumentKeys = @($confidence.catalog_document_keys)
    $baselineDocumentKeys = @($confidence.baseline_document_keys)
    $matchedDocumentKeys = @($confidence.matched_document_keys)
    $unmatchedCatalogKeys = @($catalogDocumentKeys | Where-Object { $matchedDocumentKeys -notcontains $_ })
    $unmatchedBaselineKeys = @($baselineDocumentKeys | Where-Object { $matchedDocumentKeys -notcontains $_ })
    $lines.Add("- Catalog document keys: $(Format-MarkdownCodeList -Values $catalogDocumentKeys)") | Out-Null
    $lines.Add("- Baseline document keys: $(Format-MarkdownCodeList -Values $baselineDocumentKeys)") | Out-Null
    $lines.Add("- Matched document keys: $(Format-MarkdownCodeList -Values $matchedDocumentKeys)") | Out-Null
    $lines.Add("- Unmatched catalog keys: $(Format-MarkdownCodeList -Values $unmatchedCatalogKeys)") | Out-Null
    $lines.Add("- Unmatched baseline keys: $(Format-MarkdownCodeList -Values $unmatchedBaselineKeys)") | Out-Null
    if (@($confidence.penalty_summary).Count -eq 0) {
        $lines.Add("- Penalties: none") | Out-Null
    } else {
        foreach ($penalty in @($confidence.penalty_summary)) {
            $lines.Add(("- Penalty ``{0}``: count=``{1}`` penalty=``{2}``" -f
                $penalty.factor,
                $penalty.count,
                $penalty.penalty)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Catalog Exemplars") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.catalog_exemplars).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($catalog in @($Summary.catalog_exemplars)) {
            $lines.Add(("- ``{0}``: catalog=``{1}`` definitions=``{2}`` instances=``{3}``" -f
                $catalog.document_name,
                $catalog.exemplar_catalog_display,
                $catalog.definition_count,
                $catalog.instance_count)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Baseline Manifest Entries") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.baseline_entries).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($entry in @($Summary.baseline_entries)) {
            $lines.Add(("- ``{0}``: matches=``{1}`` clean=``{2}`` lint=``{3}`` changed=``{4}`` removed=``{5}`` added=``{6}``" -f
                $entry.name,
                $entry.matches,
                $entry.clean,
                $entry.catalog_lint_clean,
                $entry.changed_definition_count,
                $entry.removed_definition_count,
                $entry.added_definition_count)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Style Numbering Issues") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.style_issue_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($issue in @($Summary.style_issue_summary)) {
            $lines.Add(("- ``{0}``: {1}" -f $issue.issue, $issue.count)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.scope)`` / ``$($blocker.id)``: action=``$($blocker.action)`` schema=``$($blocker.source_schema)`` source_report_display=``$($blocker.source_report_display)`` source_json_display=``$($blocker.source_json_display)`` message=$($blocker.message)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.scope)`` / ``$($item.id)``: action=``$($item.action)`` schema=``$($item.source_schema)`` source_report_display=``$($item.source_report_display)`` source_json_display=``$($item.source_json_display)`` title=$($item.title)") | Out-Null
            foreach ($commandName in @("open_command", "audit_command", "review_command")) {
                $commandValue = Get-JsonString -Object $item -Name $commandName
                if (-not [string]::IsNullOrWhiteSpace($commandValue)) {
                    $lines.Add("  - ${commandName}: ``$commandValue``") | Out-Null
                }
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Informational Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.informational_action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.informational_action_items)) {
            $lines.Add("- ``$($item.scope)`` / ``$($item.id)``: action=``$($item.action)`` category=``$($item.category)`` release_blocking=``$($item.release_blocking)`` optional=``$($item.optional)`` schema=``$($item.source_schema)`` source_report_display=``$($item.source_report_display)`` title=$($item.title)") | Out-Null
            foreach ($commandName in @("open_command", "audit_command", "review_command")) {
                $commandValue = Get-JsonString -Object $item -Name $commandName
                if (-not [string]::IsNullOrWhiteSpace($commandValue)) {
                    $lines.Add("  - ${commandName}: ``$commandValue``") | Out-Null
                }
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
            $lines.Add("- ``$($warning.id)``: action=``$($warning.action)`` schema=``$($warning.source_schema)`` source_report_display=``$($warning.source_report_display)`` source_json_display=``$($warning.source_json_display)`` message=$($warning.message)") | Out-Null
            $repairStrategy = Get-JsonString -Object $warning -Name "repair_strategy"
            $repairHint = Get-JsonString -Object $warning -Name "repair_hint"
            $commandTemplate = Get-JsonString -Object $warning -Name "command_template"
            if (-not [string]::IsNullOrWhiteSpace($repairStrategy)) {
                $lines.Add("  - repair_strategy: ``$repairStrategy``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($repairHint)) {
                $lines.Add("  - repair_hint: $repairHint") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($commandTemplate)) {
                $lines.Add("  - command_template: ``$commandTemplate``") | Out-Null
            }
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
    Join-Path $resolvedOutputDir "numbering_catalog_governance.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) governance input JSON file(s)"

$sourceFiles = New-Object 'System.Collections.Generic.List[object]'
$catalogExemplars = New-Object 'System.Collections.Generic.List[object]'
$baselineEntries = New-Object 'System.Collections.Generic.List[object]'
$styleIssueRows = New-Object 'System.Collections.Generic.List[object]'
$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'

$skeletonRollupCount = 0
$manifestSummaryCount = 0
$documentCount = 0
$totalStyleNumberingIssueCount = 0
$totalStyleNumberingSuggestionCount = 0
$totalNumberingDefinitionCount = 0
$totalNumberingInstanceCount = 0
$totalStyleUsageCount = 0
$totalCommandFailureCount = 0
$driftCount = 0
$dirtyBaselineCount = 0
$issueEntryCount = 0

foreach ($path in @($inputPaths)) {
    $kind = "unknown"
    $status = "skipped"
    $errorMessage = ""
    try {
        $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $kind = Get-EvidenceKind -Json $json
        switch ($kind) {
            "document_skeleton_governance_rollup" {
                $skeletonRollupCount++
                $status = Get-JsonString -Object $json -Name "status" -DefaultValue "loaded"
                $documentCount += Get-JsonInt -Object $json -Name "document_count"
                $totalStyleNumberingIssueCount += Get-JsonInt -Object $json -Name "total_style_numbering_issue_count"
                $totalStyleNumberingSuggestionCount += Get-JsonInt -Object $json -Name "total_style_numbering_suggestion_count"
                $totalNumberingDefinitionCount += Get-JsonInt -Object $json -Name "total_numbering_definition_count"
                $totalNumberingInstanceCount += Get-JsonInt -Object $json -Name "total_numbering_instance_count"
                $totalStyleUsageCount += Get-JsonInt -Object $json -Name "total_style_usage_count"
                $totalCommandFailureCount += Get-JsonInt -Object $json -Name "total_command_failure_count"

                foreach ($catalog in @(Get-JsonArray -Object $json -Name "catalog_exemplars")) {
                    $catalogExemplars.Add([ordered]@{
                        document_name = Get-JsonString -Object $catalog -Name "document_name"
                        input_docx = Get-JsonString -Object $catalog -Name "input_docx"
                        input_docx_display = Get-JsonString -Object $catalog -Name "input_docx_display"
                        exemplar_catalog_path = Get-JsonString -Object $catalog -Name "exemplar_catalog_path"
                        exemplar_catalog_display = Get-JsonString -Object $catalog -Name "exemplar_catalog_display"
                        definition_count = Get-JsonInt -Object $catalog -Name "definition_count"
                        instance_count = Get-JsonInt -Object $catalog -Name "instance_count"
                        source_json = $path
                        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    }) | Out-Null
                }
                foreach ($issue in @(Get-JsonArray -Object $json -Name "issue_summary")) {
                    $styleIssueRows.Add([ordered]@{
                        issue = Get-JsonString -Object $issue -Name "issue" -DefaultValue "unspecified"
                        count = Get-JsonInt -Object $issue -Name "count" -DefaultValue 1
                    }) | Out-Null
                }
                foreach ($blocker in @(Get-JsonArray -Object $json -Name "release_blockers")) {
                    $releaseBlockers.Add([ordered]@{
                        id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
                        scope = Get-FirstJsonString -Object $blocker -Names @("document_name", "scope") -DefaultValue "document_skeleton"
                        source_kind = "document_skeleton_governance_rollup"
                        source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                        source_json = $path
                        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "error"
                        status = Get-JsonString -Object $blocker -Name "status" -DefaultValue "needs_review"
                        action = Get-JsonString -Object $blocker -Name "action"
                        message = Get-JsonString -Object $blocker -Name "message"
                    }) | Out-Null
                }
                foreach ($item in @(Get-JsonArray -Object $json -Name "action_items")) {
                    $actionItems.Add([ordered]@{
                        id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
                        scope = Get-FirstJsonString -Object $item -Names @("document_name", "scope") -DefaultValue "document_skeleton"
                        source_kind = "document_skeleton_governance_rollup"
                        source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                        source_json = $path
                        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        action = Get-JsonString -Object $item -Name "action"
                        title = Get-JsonString -Object $item -Name "title"
                        command = Get-JsonString -Object $item -Name "command"
                        open_command = Get-JsonString -Object $item -Name "open_command" -DefaultValue (Get-JsonString -Object $item -Name "command")
                        audit_command = Get-JsonString -Object $item -Name "audit_command"
                        review_command = Get-JsonString -Object $item -Name "review_command"
                        category = Get-JsonString -Object $item -Name "category"
                        severity = Get-JsonString -Object $item -Name "severity"
                        release_blocking = Get-JsonBool -Object $item -Name "release_blocking" -DefaultValue $true
                        optional = Get-JsonBool -Object $item -Name "optional"
                    }) | Out-Null
                }
            }
            "numbering_catalog_manifest_summary" {
                $manifestSummaryCount++
                $status = if (Get-JsonBool -Object $json -Name "passed") { "passed" } else { "needs_review" }
                $driftCount += Get-JsonInt -Object $json -Name "drift_count"
                $dirtyBaselineCount += Get-JsonInt -Object $json -Name "dirty_baseline_count"
                $issueEntryCount += Get-JsonInt -Object $json -Name "issue_entry_count"

                foreach ($entry in @(Get-JsonArray -Object $json -Name "entries")) {
                    $name = Get-JsonString -Object $entry -Name "name" -DefaultValue "numbering-catalog-baseline"
                    $baselineEntries.Add([ordered]@{
                        name = $name
                        input_docx = Get-JsonString -Object $entry -Name "input_docx"
                        input_docx_display = Get-ReportPathDisplay -RepoRoot $repoRoot -Path (Get-JsonString -Object $entry -Name "input_docx")
                        matches = Get-JsonBool -Object $entry -Name "matches" -DefaultValue $true
                        clean = Get-JsonBool -Object $entry -Name "clean" -DefaultValue $true
                        catalog_file = Get-JsonString -Object $entry -Name "catalog_file"
                        catalog_file_display = Get-ReportPathDisplay -RepoRoot $repoRoot -Path (Get-JsonString -Object $entry -Name "catalog_file")
                        catalog_lint_clean = Get-JsonBool -Object $entry -Name "catalog_lint_clean" -DefaultValue $true
                        catalog_lint_issue_count = Get-JsonInt -Object $entry -Name "catalog_lint_issue_count"
                        generated_output_path = Get-JsonString -Object $entry -Name "generated_output_path"
                        baseline_issue_count = Get-JsonInt -Object $entry -Name "baseline_issue_count"
                        generated_issue_count = Get-JsonInt -Object $entry -Name "generated_issue_count"
                        added_definition_count = Get-JsonInt -Object $entry -Name "added_definition_count"
                        removed_definition_count = Get-JsonInt -Object $entry -Name "removed_definition_count"
                        changed_definition_count = Get-JsonInt -Object $entry -Name "changed_definition_count"
                        source_json = $path
                        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        source_report = $path
                        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    }) | Out-Null

                    $blocker = New-BaselineBlocker -Entry $entry
                    if ($null -ne $blocker) {
                        $blocker["source_schema"] = "featherdoc.numbering_catalog_manifest_summary.v1"
                        $blocker["source_json"] = $path
                        $blocker["source_json_display"] = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        $blocker["source_report"] = $path
                        $blocker["source_report_display"] = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        $releaseBlockers.Add($blocker) | Out-Null
                        $action = New-ActionForBaselineBlocker -Blocker $blocker -Entry $entry
                        $action["source_schema"] = "featherdoc.numbering_catalog_manifest_summary.v1"
                        $action["source_json"] = $path
                        $action["source_json_display"] = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        $action["source_report"] = $path
                        $action["source_report_display"] = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        $actionItems.Add($action) | Out-Null
                    }
                }
            }
            "numbering_catalog_governance_report" {
                $status = "skipped"
            }
            default {
                $warnings.Add([ordered]@{
                    id = "source_json_schema_skipped"
                    action = "review_numbering_catalog_governance_sources"
                    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
                    source_json = $path
                    source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    source_report = $path
                    source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                    message = "Input JSON kind '$kind' is not numbering catalog governance evidence."
                }) | Out-Null
            }
        }
    } catch {
        $status = "failed"
        $errorMessage = $_.Exception.Message
        $warnings.Add([ordered]@{
            id = "source_json_read_failed"
            action = "review_numbering_catalog_governance_sources"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            source_json = $path
            source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            source_report = $path
            source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
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

if ($skeletonRollupCount -eq 0) {
    $warnings.Add([ordered]@{
        id = "document_skeleton_governance_rollup_missing"
        action = "rebuild_document_skeleton_governance_rollup"
        repair_strategy = "rebuild_document_skeleton_governance_rollup"
        repair_hint = "Generate the document skeleton governance rollup from current document skeleton summaries, then rerun numbering catalog governance."
        command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1 -OutputDir .\output\document-skeleton-governance-rollup"
        source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
        source_json = $summaryPath
        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        source_report = $summaryPath
        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        message = "No document skeleton governance rollup summary was loaded."
    }) | Out-Null
}
if ($manifestSummaryCount -eq 0) {
    $warnings.Add([ordered]@{
        id = "numbering_catalog_manifest_summary_missing"
        action = "rebuild_numbering_catalog_manifest_summary"
        repair_strategy = "rebuild_numbering_catalog_manifest_summary"
        repair_hint = "Restore the numbering catalog manifest and generate a real manifest check summary; do not synthesize a pass summary when the manifest or catalog outputs are absent."
        command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_manifest.ps1 -ManifestPath .\baselines\numbering-catalog\manifest.json -BuildDir <build-dir> -OutputDir .\output\numbering-catalog-manifest-checks -SkipBuild"
        source_schema = "featherdoc.numbering_catalog_manifest_summary.v1"
        source_json = $summaryPath
        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        source_report = $summaryPath
        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        message = "No numbering catalog manifest check summary was loaded."
    }) | Out-Null
}

$sourceFailureCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "failed" }).Count

$catalogDocumentKeys = @(
    $catalogExemplars.ToArray() |
        ForEach-Object { Get-CanonicalDocumentKey -Value (Get-FirstJsonString -Object $_ -Names @("input_docx", "document_name")) } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Select-Object -Unique
)
$baselineDocumentKeys = @(
    $baselineEntries.ToArray() |
        ForEach-Object { Get-CanonicalDocumentKey -Value (Get-FirstJsonString -Object $_ -Names @("input_docx", "name")) } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Select-Object -Unique
)
$matchedDocumentKeys = @($catalogDocumentKeys | Where-Object { $baselineDocumentKeys -contains $_ })
$matchedDocumentCount = $matchedDocumentKeys.Count

$realCorpusConfidence = New-RealCorpusConfidence `
    -DocumentCount $documentCount `
    -CatalogExemplarCount $catalogExemplars.Count `
    -BaselineEntryCount $baselineEntries.Count `
    -MatchedDocumentCount $matchedDocumentCount `
    -TotalStyleNumberingIssueCount $totalStyleNumberingIssueCount `
    -TotalStyleNumberingSuggestionCount $totalStyleNumberingSuggestionCount `
    -DriftCount $driftCount `
    -DirtyBaselineCount $dirtyBaselineCount `
    -IssueEntryCount $issueEntryCount `
    -TotalCommandFailureCount $totalCommandFailureCount
$realCorpusConfidence["catalog_document_keys"] = @($catalogDocumentKeys)
$realCorpusConfidence["baseline_document_keys"] = @($baselineDocumentKeys)
$realCorpusConfidence["matched_document_keys"] = @($matchedDocumentKeys)

if ($realCorpusConfidence.alignment_gap_count -gt 0) {
    $alignmentBlocker = New-ReleaseBlocker `
        -Id "numbering_catalog_governance.real_corpus_alignment_gap" `
        -Scope "numbering_catalog_governance" `
        -SourceKind "numbering_catalog_governance_report" `
        -Status "real_corpus_alignment_gap" `
        -Action "review_numbering_catalog_real_corpus_alignment" `
        -Message "Numbering catalog exemplars and baseline entries do not align on document identity."
    $alignmentBlocker["source_schema"] = "featherdoc.numbering_catalog_governance_report.v1"
    $alignmentBlocker["source_report"] = $summaryPath
    $alignmentBlocker["source_report_display"] = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    $alignmentBlocker["source_json"] = $summaryPath
    $alignmentBlocker["source_json_display"] = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    $alignmentBlocker["matched_document_count"] = $realCorpusConfidence.matched_document_count
    $alignmentBlocker["unmatched_catalog_document_count"] = $realCorpusConfidence.unmatched_catalog_document_count
    $alignmentBlocker["unmatched_baseline_document_count"] = $realCorpusConfidence.unmatched_baseline_document_count
    $alignmentBlocker["catalog_coverage_percent"] = $realCorpusConfidence.catalog_coverage_percent
    $alignmentBlocker["baseline_coverage_percent"] = $realCorpusConfidence.baseline_coverage_percent
    $alignmentBlocker["coverage_score"] = $realCorpusConfidence.coverage_score
    $releaseBlockers.Add($alignmentBlocker) | Out-Null
}

$status = if ($sourceFailureCount -gt 0 -or $totalCommandFailureCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0 -or $warnings.Count -gt 0 -or
    $totalStyleNumberingIssueCount -gt 0 -or $driftCount -gt 0 -or
    $dirtyBaselineCount -gt 0 -or $issueEntryCount -gt 0) {
    "needs_review"
} else {
    "clean"
}

$releaseActionItems = New-Object 'System.Collections.Generic.List[object]'
$informationalActionItems = New-Object 'System.Collections.Generic.List[object]'
foreach ($item in @($actionItems.ToArray())) {
    if (Test-InformationalActionItem -Item $item) {
        $informationalActionItems.Add((Copy-ActionItemWithReleaseChecklistDefaults -Item $item)) | Out-Null
    } else {
        $releaseActionItems.Add($item) | Out-Null
    }
}

$summary = [ordered]@{
    schema = "featherdoc.numbering_catalog_governance_report.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    clean = ($status -eq "clean")
    release_ready = ($status -eq "clean")
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    source_file_count = $sourceFiles.Count
    source_failure_count = $sourceFailureCount
    source_files = @($sourceFiles.ToArray())
    document_skeleton_rollup_count = $skeletonRollupCount
    numbering_catalog_manifest_summary_count = $manifestSummaryCount
    document_count = $documentCount
    baseline_entry_count = $baselineEntries.Count
    baseline_entries = @($baselineEntries.ToArray())
    baseline_status_summary = @(Add-SummaryGroup -Items $baselineEntries.ToArray() -PropertyName "matches" -OutputName "matches")
    catalog_exemplar_count = $catalogExemplars.Count
    catalog_exemplars = @($catalogExemplars.ToArray())
    real_corpus_confidence_score = $realCorpusConfidence.score
    real_corpus_confidence_level = $realCorpusConfidence.level
    real_corpus_confidence = $realCorpusConfidence
    total_numbering_definition_count = $totalNumberingDefinitionCount
    total_numbering_instance_count = $totalNumberingInstanceCount
    total_style_usage_count = $totalStyleUsageCount
    total_style_numbering_issue_count = $totalStyleNumberingIssueCount
    total_style_numbering_suggestion_count = $totalStyleNumberingSuggestionCount
    total_command_failure_count = $totalCommandFailureCount
    style_issue_summary = @(Add-IssueSummary -Items $styleIssueRows.ToArray())
    drift_count = $driftCount
    dirty_baseline_count = $dirtyBaselineCount
    issue_entry_count = $issueEntryCount
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    blocker_id_summary = @(Add-SummaryGroup -Items $releaseBlockers.ToArray() -PropertyName "id" -OutputName "id")
    action_item_count = $releaseActionItems.Count
    action_items = @($releaseActionItems.ToArray())
    action_item_summary = @(Add-SummaryGroup -Items $releaseActionItems.ToArray() -PropertyName "action" -OutputName "action")
    informational_action_item_count = $informationalActionItems.Count
    informational_action_items = @($informationalActionItems.ToArray())
    informational_action_item_summary = @(Add-SummaryGroup -Items $informationalActionItems.ToArray() -PropertyName "action" -OutputName "action")
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0 -or $totalCommandFailureCount -gt 0) { exit 1 }
if ($FailOnIssue -and ($totalStyleNumberingIssueCount -gt 0 -or $issueEntryCount -gt 0)) { exit 1 }
if ($FailOnDrift -and ($driftCount -gt 0 -or $dirtyBaselineCount -gt 0)) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
