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

    $rows = @(
        foreach ($item in @($Items | Where-Object { $null -ne $_ })) {
            [pscustomobject]@{
                value = Get-JsonString -Object $item -Name $PropertyName
            }
        }
    )

    return @(
        foreach ($group in @($rows | Group-Object value |
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
        [int]$CatalogDocumentCount = -1,
        [int]$BaselineEntryCount,
        [int]$MatchedDocumentCount,
        [int]$TotalStyleNumberingIssueCount,
        [int]$TotalStyleNumberingSuggestionCount,
        [int]$DriftCount,
        [int]$DirtyBaselineCount,
        [int]$IssueEntryCount,
        [int]$TotalCommandFailureCount
    )

    # A document may have multiple exemplar rows while the confidence metric
    # measures document coverage. Keep the raw exemplar count for compatibility
    # but use the unique document count for denominators and gap calculations.
    $catalogDocumentCountForCoverage = if ($CatalogDocumentCount -ge 0) {
        $CatalogDocumentCount
    } else {
        $CatalogExemplarCount
    }
    $catalogCoveragePercent = Get-Percent -Numerator $MatchedDocumentCount -Denominator $catalogDocumentCountForCoverage
    $baselineCoveragePercent = Get-Percent -Numerator $MatchedDocumentCount -Denominator $BaselineEntryCount
    $coverageScore = [Math]::Min($catalogCoveragePercent, $baselineCoveragePercent)
    $unmatchedCatalogDocumentCount = [Math]::Max(0, $catalogDocumentCountForCoverage - $MatchedDocumentCount)
    $unmatchedBaselineDocumentCount = [Math]::Max(0, $BaselineEntryCount - $MatchedDocumentCount)

    $styleIssuePenalty = [Math]::Min(25, $TotalStyleNumberingIssueCount * 5)
    $suggestionPenalty = [Math]::Min(10, $TotalStyleNumberingSuggestionCount * 2)
    $catalogQualityPenalty = [Math]::Min(25, ($DriftCount + $DirtyBaselineCount + $IssueEntryCount) * 10)
    $commandFailurePenalty = [Math]::Min(40, $TotalCommandFailureCount * 20)
    $score = [int][Math]::Max(0, $coverageScore - $styleIssuePenalty - $suggestionPenalty - $catalogQualityPenalty - $commandFailurePenalty)
    $level = Get-RealCorpusConfidenceLevel `
        -Score $score `
        -DocumentCount $DocumentCount `
        -CatalogExemplarCount $catalogDocumentCountForCoverage `
        -BaselineEntryCount $BaselineEntryCount

    return [ordered]@{
        score = $score
        level = $level
        document_count = $DocumentCount
        catalog_exemplar_count = $CatalogExemplarCount
        catalog_document_count = $catalogDocumentCountForCoverage
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

function New-ExemplarCatalogPatchPlan {
    param(
        [string]$DocumentKey,
        [string[]]$CatalogPaths,
        [string[]]$CatalogDisplays
    )

    $candidatePaths = @($CatalogPaths |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
        ForEach-Object { ([string]$_).Trim() } |
        Sort-Object -Unique)
    $candidateDisplays = @($CatalogDisplays |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
        ForEach-Object { ([string]$_).Trim() } |
        Sort-Object -Unique)

    $diffCommands = New-Object 'System.Collections.Generic.List[string]'
    if ($candidatePaths.Count -gt 1) {
        $anchorPath = $candidatePaths[0]
        foreach ($candidatePath in @($candidatePaths | Select-Object -Skip 1)) {
            $diffCommands.Add((ConvertTo-TemplateSchemaCommandLine -Arguments @(
                    "featherdoc_cli",
                    "diff-numbering-catalog",
                    $anchorPath,
                    $candidatePath,
                    "--json"
                ))) | Out-Null
        }
    }

    $reviewCommand = if ($diffCommands.Count -gt 0) {
        [string]$diffCommands[0]
    } else {
        "featherdoc_cli diff-numbering-catalog <left-catalog.json> <right-catalog.json> --json"
    }
    $patchCommand = "featherdoc_cli patch-numbering-catalog <authoritative-catalog.json> --patch-file <reviewed-patch.json> --output <patched-catalog.json> --json"
    $lintCommand = "featherdoc_cli lint-numbering-catalog <patched-catalog.json> --json"
    $verificationCommand = "featherdoc_cli diff-numbering-catalog <patched-catalog.json> <reviewed-expected-catalog.json> --fail-on-diff --json"
    $unsupportedChanges = @(
        [ordered]@{
            change_kind = "definition_topology_changes"
            automatic_action = "manual_review_required"
            reason = "The catalog patch CLI does not add or remove numbering definitions."
        }
        [ordered]@{
            change_kind = "instance_topology_changes"
            automatic_action = "manual_review_required"
            reason = "The catalog patch CLI does not add or remove numbering instances."
        }
    )
    $requiredSteps = @(
        [ordered]@{
            sequence = 1
            action = "select_authoritative_catalog"
            required = $true
            command_template = ""
            description = "Choose exactly one candidate catalog as the authoritative source."
        }
        [ordered]@{
            sequence = 2
            action = "review_candidate_diffs"
            required = $true
            commands = @($diffCommands.ToArray())
            description = "Compare the selected source with every other candidate before editing."
        }
        [ordered]@{
            sequence = 3
            action = "author_reviewed_patch"
            required = $true
            command_template = ""
            description = "Write a patch containing only supported level and override operations."
        }
        [ordered]@{
            sequence = 4
            action = "apply_reviewed_patch"
            required = $true
            command_template = $patchCommand
            description = "Apply the reviewed patch to a copied catalog output."
        }
        [ordered]@{
            sequence = 5
            action = "lint_patched_catalog"
            required = $true
            command_template = $lintCommand
            description = "Lint the patched catalog before it is promoted."
        }
        [ordered]@{
            sequence = 6
            action = "verify_patched_catalog"
            required = $true
            command_template = $verificationCommand
            description = "Diff the patched output against the reviewer-approved expected catalog."
        }
    )

    return [ordered]@{
        schema = "featherdoc.numbering_catalog_governance_patch_plan.v1"
        id = "numbering_catalog_governance.exemplar_catalog_conflict_patch_plan"
        document_key = $DocumentKey
        status = "awaiting_authoritative_catalog"
        safe_to_apply = $false
        automatic_patch_available = $false
        patch_apply_supported = $false
        manual_review_required = $true
        requires_authoritative_catalog_selection = $true
        candidate_catalog_count = $candidatePaths.Count
        candidate_catalog_paths = @($candidatePaths)
        candidate_catalog_displays = @($candidateDisplays)
        reviewer_inputs = @(
            "authoritative_catalog_path"
            "reviewed_patch_path"
            "reviewed_expected_catalog_path"
        )
        supported_patch_operations = @(
            "upsert_levels"
            "upsert_overrides"
            "remove_overrides"
        )
        unsupported_automatic_changes = @($unsupportedChanges)
        unsupported_change_count = $unsupportedChanges.Count
        patch_counts = [ordered]@{
            upsert_levels = 0
            upsert_overrides = 0
            remove_overrides = 0
        }
        diff_commands = @($diffCommands.ToArray())
        review_command = $reviewCommand
        patch_command_template = $patchCommand
        lint_command_template = $lintCommand
        verification_command_template = $verificationCommand
        required_steps = @($requiredSteps)
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
    $inputDocx = Get-JsonString -Object $Entry -Name "input_docx"
    $catalogFile = Get-JsonString -Object $Entry -Name "catalog_file"
    $generatedCatalogOutput = Get-JsonString -Object $Entry -Name "generated_output_path"
    $command = "pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_manifest.ps1 -ManifestPath .\baselines\numbering-catalog\manifest.json -BuildDir <build-dir> -OutputDir .\output\numbering-catalog-manifest-checks -SkipBuild"
    if (-not [string]::IsNullOrWhiteSpace($inputDocx) -and
        -not [string]::IsNullOrWhiteSpace($catalogFile)) {
        $command = "pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 -InputDocx $inputDocx -CatalogFile $catalogFile"
        if (-not [string]::IsNullOrWhiteSpace($generatedCatalogOutput)) {
            $command += " -GeneratedCatalogOutput $generatedCatalogOutput"
        }
        $command += " -BuildDir <build-dir> -SkipBuild"
    }

    return New-ActionItem `
        -Id ([string]$Blocker.id) `
        -Scope $name `
        -SourceKind "numbering_catalog_manifest_summary" `
        -Action ([string]$Blocker.action) `
        -Title ([string]$Blocker.message) `
        -Command $command
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
    $lines.Add("- Exemplar catalog conflicts: ``$($Summary.exemplar_conflict_count)``") | Out-Null
    $lines.Add("- Catalog patch plans: ``$($Summary.catalog_patch_plan_count)``") | Out-Null
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

    $lines.Add("## Exemplar Catalog Conflicts") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.exemplar_conflicts).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($conflict in @($Summary.exemplar_conflicts)) {
            $paths = Format-MarkdownCodeList -Values @($conflict.exemplar_catalog_displays)
            $lines.Add(("- ``{0}``: status=``{1}`` paths={2} action=``{3}`` source_report_display=``{4}`` source_json_display=``{5}``" -f
                $conflict.document_key,
                $conflict.status,
                $paths,
                $conflict.action,
                $conflict.source_report_display,
                $conflict.source_json_display)) | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$conflict.message)) {
                $lines.Add("  - message: $($conflict.message)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$conflict.open_command)) {
                $lines.Add("  - open_command: ``$($conflict.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$conflict.catalog_patch_plan_id)) {
                $plan = $conflict.catalog_patch_plan
                $lines.Add("  - catalog_patch_plan_id: ``$($conflict.catalog_patch_plan_id)`` status=``$($plan.status)`` safe_to_apply=``$($plan.safe_to_apply)``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Catalog Patch Plans") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.catalog_patch_plans).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($plan in @($Summary.catalog_patch_plans)) {
            $lines.Add(("- ``{0}``: schema=``{1}`` document_key=``{2}`` status=``{3}`` safe_to_apply=``{4}`` automatic_patch_available=``{5}`` patch_apply_supported=``{6}`` manual_review_required=``{7}`` requires_authoritative_catalog_selection=``{8}``" -f
                $plan.id,
                $plan.schema,
                $plan.document_key,
                $plan.status,
                $plan.safe_to_apply,
                $plan.automatic_patch_available,
                $plan.patch_apply_supported,
                $plan.manual_review_required,
                $plan.requires_authoritative_catalog_selection)) | Out-Null
            $lines.Add("  - candidates: $(Format-MarkdownCodeList -Values @($plan.candidate_catalog_displays))") | Out-Null
            $lines.Add("  - supported_patch_operations: $(Format-MarkdownCodeList -Values @($plan.supported_patch_operations))") | Out-Null
            $lines.Add("  - unsupported_automatic_changes: $(Format-MarkdownCodeList -Values @($plan.unsupported_automatic_changes | ForEach-Object { $_.change_kind }))") | Out-Null
            foreach ($commandName in @("review_command", "patch_command_template", "lint_command_template", "verification_command_template")) {
                $commandValue = Get-JsonString -Object $plan -Name $commandName
                if (-not [string]::IsNullOrWhiteSpace($commandValue)) {
                    $lines.Add("  - ${commandName}: ``$commandValue``") | Out-Null
                }
            }
            foreach ($step in @($plan.required_steps)) {
                $lines.Add("  - required_step ``$($step.sequence)``: ``$($step.action)`` - $($step.description)") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Real Corpus Alignment") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.real_corpus_alignment).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($entry in @($Summary.real_corpus_alignment)) {
            $catalogInputs = @($entry.catalog_exemplars |
                ForEach-Object { Get-FirstJsonString -Object $_ -Names @("input_docx_display", "input_docx", "document_name") })
            $baselineInputs = @($entry.baseline_entries |
                ForEach-Object { Get-FirstJsonString -Object $_ -Names @("input_docx_display", "input_docx", "name") })
            $lines.Add(("- ``{0}``: status=``{1}`` exemplar_conflict=``{2}`` catalog_inputs={3} baseline_inputs={4} action=``{5}`` source_report_display=``{6}`` source_json_display=``{7}``" -f
                $entry.document_key,
                $entry.status,
                $entry.exemplar_conflict,
                (Format-MarkdownCodeList -Values $catalogInputs),
                (Format-MarkdownCodeList -Values $baselineInputs),
                $entry.action,
                $entry.source_report_display,
                $entry.source_json_display)) | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$entry.message)) {
                $lines.Add("  - message: $($entry.message)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$entry.open_command)) {
                $lines.Add("  - open_command: ``$($entry.open_command)``") | Out-Null
            }
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
