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
. (Join-Path $PSScriptRoot "build_numbering_catalog_governance_report_helpers.ps1")

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

$catalogAlignmentRows = @(
    foreach ($catalog in @($catalogExemplars.ToArray())) {
        $documentKey = Get-CanonicalDocumentKey -Value (Get-FirstJsonString -Object $catalog -Names @("input_docx", "document_name"))
        if ([string]::IsNullOrWhiteSpace($documentKey)) { continue }

        [ordered]@{
            document_key = $documentKey
            document_name = Get-JsonString -Object $catalog -Name "document_name"
            input_docx = Get-JsonString -Object $catalog -Name "input_docx"
            input_docx_display = Get-JsonString -Object $catalog -Name "input_docx_display"
            exemplar_catalog_path = Get-JsonString -Object $catalog -Name "exemplar_catalog_path"
            exemplar_catalog_display = Get-JsonString -Object $catalog -Name "exemplar_catalog_display"
            definition_count = Get-JsonInt -Object $catalog -Name "definition_count"
            instance_count = Get-JsonInt -Object $catalog -Name "instance_count"
            source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
            source_json = Get-JsonString -Object $catalog -Name "source_json"
            source_json_display = Get-JsonString -Object $catalog -Name "source_json_display"
            source_report = Get-JsonString -Object $catalog -Name "source_report"
            source_report_display = Get-JsonString -Object $catalog -Name "source_report_display"
        }
    }
)
$baselineAlignmentRows = @(
    foreach ($entry in @($baselineEntries.ToArray())) {
        $documentKey = Get-CanonicalDocumentKey -Value (Get-FirstJsonString -Object $entry -Names @("input_docx", "name"))
        if ([string]::IsNullOrWhiteSpace($documentKey)) { continue }

        [ordered]@{
            document_key = $documentKey
            name = Get-JsonString -Object $entry -Name "name"
            input_docx = Get-JsonString -Object $entry -Name "input_docx"
            input_docx_display = Get-JsonString -Object $entry -Name "input_docx_display"
            catalog_file = Get-JsonString -Object $entry -Name "catalog_file"
            catalog_file_display = Get-JsonString -Object $entry -Name "catalog_file_display"
            matches = Get-JsonBool -Object $entry -Name "matches" -DefaultValue $true
            clean = Get-JsonBool -Object $entry -Name "clean" -DefaultValue $true
            catalog_lint_clean = Get-JsonBool -Object $entry -Name "catalog_lint_clean" -DefaultValue $true
            source_schema = "featherdoc.numbering_catalog_manifest_summary.v1"
            source_json = Get-JsonString -Object $entry -Name "source_json"
            source_json_display = Get-JsonString -Object $entry -Name "source_json_display"
            source_report = Get-JsonString -Object $entry -Name "source_report"
            source_report_display = Get-JsonString -Object $entry -Name "source_report_display"
        }
    }
)
$catalogDocumentKeys = @(
    $catalogAlignmentRows |
        ForEach-Object { [string]$_.document_key } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Select-Object -Unique
)
$baselineDocumentKeys = @(
    $baselineAlignmentRows |
        ForEach-Object { [string]$_.document_key } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Select-Object -Unique
)
$matchedDocumentKeys = @($catalogDocumentKeys | Where-Object { $baselineDocumentKeys -contains $_ })
$matchedDocumentCount = $matchedDocumentKeys.Count

$realCorpusAlignment = New-Object 'System.Collections.Generic.List[object]'
$alignmentKeys = @(@($catalogDocumentKeys) + @($baselineDocumentKeys) |
    Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
    Sort-Object -Unique)
foreach ($documentKey in $alignmentKeys) {
    $catalogMatches = @($catalogAlignmentRows | Where-Object { [string]$_.document_key -eq $documentKey })
    $baselineMatches = @($baselineAlignmentRows | Where-Object { [string]$_.document_key -eq $documentKey })
    $status = if ($catalogMatches.Count -gt 0 -and $baselineMatches.Count -gt 0) {
        "matched"
    } elseif ($catalogMatches.Count -gt 0) {
        "missing_baseline"
    } else {
        "missing_exemplar"
    }

    $sourceSchema = "featherdoc.numbering_catalog_governance_report.v1"
    $sourceJson = $summaryPath
    $sourceJsonDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    $sourceReport = $summaryPath
    $sourceReportDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    $action = ""
    $openCommand = ""
    $message = "Catalog exemplar and baseline entry are aligned."

    if ($status -eq "missing_baseline") {
        $catalogSource = $catalogMatches[0]
        $sourceSchema = [string]$catalogSource.source_schema
        $sourceJson = [string]$catalogSource.source_json
        $sourceJsonDisplay = [string]$catalogSource.source_json_display
        $sourceReport = [string]$catalogSource.source_report
        $sourceReportDisplay = [string]$catalogSource.source_report_display
        $action = "review_numbering_catalog_real_corpus_alignment"
        $inputDocx = [string]$catalogSource.input_docx
        $catalogPath = [string]$catalogSource.exemplar_catalog_path
        $openCommand = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 -InputDocx $inputDocx -CatalogFile $catalogPath -BuildDir <build-dir> -SkipBuild"
        $message = "Generated exemplar catalog has no registered numbering baseline entry."
    } elseif ($status -eq "missing_exemplar") {
        $baselineSource = $baselineMatches[0]
        $sourceSchema = [string]$baselineSource.source_schema
        $sourceJson = [string]$baselineSource.source_json
        $sourceJsonDisplay = [string]$baselineSource.source_json_display
        $sourceReport = [string]$baselineSource.source_report
        $sourceReportDisplay = [string]$baselineSource.source_report_display
        $action = "review_numbering_catalog_real_corpus_alignment"
        $inputDocx = [string]$baselineSource.input_docx
        $outputDirForDocument = ".\output\document-skeleton-governance\$documentKey"
        $openCommand = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_report.ps1 -InputDocx $inputDocx -OutputDir $outputDirForDocument -BuildDir <build-dir> -SkipBuild"
        $message = "Registered numbering baseline entry has no generated document skeleton exemplar."
    }

    $alignmentEntry = [ordered]@{
        document_key = $documentKey
        status = $status
        has_catalog_exemplar = ($catalogMatches.Count -gt 0)
        has_baseline_entry = ($baselineMatches.Count -gt 0)
        catalog_exemplar_count = $catalogMatches.Count
        baseline_entry_count = $baselineMatches.Count
        catalog_exemplars = @($catalogMatches)
        baseline_entries = @($baselineMatches)
        action = $action
        message = $message
        open_command = $openCommand
        source_schema = $sourceSchema
        source_json = $sourceJson
        source_json_display = $sourceJsonDisplay
        source_report = $sourceReport
        source_report_display = $sourceReportDisplay
    }
    $realCorpusAlignment.Add($alignmentEntry) | Out-Null

    if ($status -ne "matched") {
        $alignmentActionId = if ($status -eq "missing_baseline") {
            "numbering_catalog_governance.missing_baseline"
        } else {
            "numbering_catalog_governance.missing_exemplar"
        }
        $actionItems.Add([ordered]@{
            id = $alignmentActionId
            scope = $documentKey
            source_kind = "numbering_catalog_governance_report"
            source_schema = $sourceSchema
            source_json = $sourceJson
            source_json_display = $sourceJsonDisplay
            source_report = $sourceReport
            source_report_display = $sourceReportDisplay
            action = $action
            title = $message
            command = $openCommand
            open_command = $openCommand
            audit_command = ""
            review_command = ""
            category = "remediation"
            severity = "error"
            release_blocking = $true
            optional = $false
        }) | Out-Null
    }
}

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
$realCorpusConfidence["alignment_status_summary"] = @(Add-SummaryGroup -Items $realCorpusAlignment.ToArray() -PropertyName "status" -OutputName "status")

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
    $alignmentBlocker["catalog_document_keys"] = @($realCorpusConfidence.catalog_document_keys)
    $alignmentBlocker["baseline_document_keys"] = @($realCorpusConfidence.baseline_document_keys)
    $alignmentBlocker["matched_document_keys"] = @($realCorpusConfidence.matched_document_keys)
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
    real_corpus_alignment_count = $realCorpusAlignment.Count
    real_corpus_alignment_gap_count = @($realCorpusAlignment.ToArray() | Where-Object { [string]$_.status -ne "matched" }).Count
    real_corpus_alignment = @($realCorpusAlignment.ToArray())
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
