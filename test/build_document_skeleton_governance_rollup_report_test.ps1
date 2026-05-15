param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "style_merge_review", "empty", "failed_source_report", "malformed", "warning_metadata", "fail_on_issue")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Test-Scenario {
    param([string]$Name)
    return ($Scenario -eq "all" -or $Scenario -eq $Name)
}

function Invoke-RollupScript {
    param([string[]]$Arguments)

    $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
    $powerShellPath = if ($powerShellCommand) { $powerShellCommand.Source } else { (Get-Process -Id $PID).Path }
    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

function Write-JsonFile {
    param(
        [string]$Path,
        $Value
    )

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-SkeletonSummary {
    param(
        [string]$InputDocx,
        [string]$CatalogPath,
        [string]$Status = "clean",
        [int]$DefinitionCount = 1,
        [int]$InstanceCount = 1,
        [int]$IssueCount = 0,
        [int]$SuggestionCount = 0,
        [int]$StyleMergeSuggestionCount = 0,
        [int]$StyleMergeSuggestionPendingCount = -1,
        [string]$StyleMergeReviewStatus = "missing",
        [int]$UsageTotal = 0,
        [object[]]$IssueSummary = @(),
        [object[]]$ReleaseBlockers = @(),
        [object[]]$ActionItems = @()
    )

    if ($StyleMergeSuggestionPendingCount -lt 0) {
        $StyleMergeSuggestionPendingCount = $StyleMergeSuggestionCount
    }

    return [ordered]@{
        schema = "featherdoc.document_skeleton_governance_report.v1"
        generated_at = "2026-05-03T00:00:00"
        status = $Status
        clean = ($Status -eq "clean")
        input_docx = $InputDocx
        input_docx_relative = $InputDocx
        exemplar_catalog_path = $CatalogPath
        exemplar_catalog_relative_path = $CatalogPath
        numbering_catalog_path = $CatalogPath
        numbering_catalog = [ordered]@{
            definition_count = $DefinitionCount
            instance_count = $InstanceCount
        }
        style_numbering_issue_count = $IssueCount
        style_numbering_suggestion_count = $SuggestionCount
        style_merge_suggestion_count = $StyleMergeSuggestionCount
        style_merge_suggestion_pending_count = $StyleMergeSuggestionPendingCount
        style_merge_suggestion_review = [ordered]@{
            requested = ($StyleMergeReviewStatus -ne "missing")
            status = $StyleMergeReviewStatus
            reviewed_suggestion_count = $StyleMergeSuggestionCount - $StyleMergeSuggestionPendingCount
            pending_suggestion_count = $StyleMergeSuggestionPendingCount
        }
        numbered_style_count = 2
        issue_summary = @($IssueSummary)
        style_usage = [ordered]@{
            style_count = 3
            entry_count = 2
            usage_total = $UsageTotal
        }
        command_failure_count = 0
        release_blockers = @($ReleaseBlockers)
        release_blocker_count = @($ReleaseBlockers).Count
        action_items = @($ActionItems)
        next_steps = @($ActionItems)
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_document_skeleton_governance_rollup_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "aggregate") {
    $aggregateRoot = Join-Path $resolvedWorkingDir "aggregate-input"
    $aggregateOutputDir = Join-Path $resolvedWorkingDir "aggregate-output"
    $cleanReportPath = Join-Path $aggregateRoot "invoice\document_skeleton_governance.summary.json"
    $needsReviewReportPath = Join-Path $aggregateRoot "contract\summary.json"

    Write-JsonFile -Path $cleanReportPath -Value (New-SkeletonSummary `
        -InputDocx "samples/invoice.docx" `
        -CatalogPath "output/document-skeleton-governance/invoice/exemplar.numbering-catalog.json" `
        -Status "clean" `
        -DefinitionCount 2 `
        -InstanceCount 3 `
        -StyleMergeSuggestionCount 1 `
        -UsageTotal 5 `
        -ActionItems @(
            [ordered]@{
                id = "promote_numbering_catalog_exemplar"
                title = "Promote invoice catalog"
                command = "featherdoc_cli check-numbering-catalog samples/invoice.docx"
            }
        ))

    Write-JsonFile -Path $needsReviewReportPath -Value (New-SkeletonSummary `
        -InputDocx "samples/contract.docx" `
        -CatalogPath "output/document-skeleton-governance/contract/exemplar.numbering-catalog.json" `
        -Status "needs_review" `
        -DefinitionCount 4 `
        -InstanceCount 6 `
        -IssueCount 3 `
        -SuggestionCount 2 `
        -StyleMergeSuggestionCount 2 `
        -UsageTotal 9 `
        -IssueSummary @(
            [ordered]@{ issue = "missing_numbering_definition"; count = 2 },
            [ordered]@{ issue = "dangling_numbering_instance"; count = 1 }
        ) `
        -ReleaseBlockers @(
            [ordered]@{
                id = "document_skeleton.style_numbering_issues"
                severity = "error"
                message = "Style numbering audit reported issues."
                action = "review_style_numbering_audit"
                issue_count = 3
            }
        ) `
        -ActionItems @(
            [ordered]@{
                id = "review_style_numbering_audit"
                title = "Review contract style numbering audit"
                command = "featherdoc_cli audit-style-numbering samples/contract.docx --fail-on-issue --json"
            },
            [ordered]@{
                id = "preview_style_numbering_repair"
                title = "Preview contract style numbering repair"
                command = "featherdoc_cli repair-style-numbering samples/contract.docx --plan-only --json"
            }
        ))

    $aggregateResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $aggregateRoot,
        "-OutputDir", $aggregateOutputDir
    )
    Assert-Equal -Actual $aggregateResult.ExitCode -Expected 0 `
        -Message "Aggregate rollup should pass without fail switches. Output: $($aggregateResult.Text)"
    Assert-ContainsText -Text $aggregateResult.Text -ExpectedText "Summary JSON:" `
        -Message "Aggregate rollup should print summary path."

    $summaryPath = Join-Path $aggregateOutputDir "summary.json"
    $markdownPath = Join-Path $aggregateOutputDir "document_skeleton_governance_rollup.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Aggregate rollup should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Aggregate rollup should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Aggregate rollup should use the skeleton rollup schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Aggregate rollup should require review when one source has issues."
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 2 `
        -Message "Aggregate rollup should count source reports."
    Assert-Equal -Actual ([int]$summary.document_count) -Expected 2 `
        -Message "Aggregate rollup should count documents."
    Assert-Equal -Actual ([int]$summary.total_style_numbering_issue_count) -Expected 3 `
        -Message "Aggregate rollup should sum style-numbering issues."
    Assert-Equal -Actual ([int]$summary.total_style_numbering_suggestion_count) -Expected 2 `
        -Message "Aggregate rollup should sum style-numbering suggestions."
    Assert-Equal -Actual ([int]$summary.total_style_merge_suggestion_count) -Expected 3 `
        -Message "Aggregate rollup should sum style merge suggestions."
    Assert-Equal -Actual ([int]$summary.total_style_merge_suggestion_pending_count) -Expected 3 `
        -Message "Aggregate rollup should sum pending style merge suggestions."
    Assert-Equal -Actual ([int]$summary.total_numbering_definition_count) -Expected 6 `
        -Message "Aggregate rollup should sum catalog definition counts."
    Assert-Equal -Actual ([int]$summary.total_numbering_instance_count) -Expected 9 `
        -Message "Aggregate rollup should sum catalog instance counts."
    Assert-Equal -Actual ([int]$summary.total_style_usage_count) -Expected 14 `
        -Message "Aggregate rollup should sum style usage totals."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 1 `
        -Message "Aggregate rollup should preserve release blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 3 `
        -Message "Aggregate rollup should preserve action items."
    Assert-Equal -Actual ([int]$summary.catalog_exemplar_count) -Expected 2 `
        -Message "Aggregate rollup should expose per-document exemplar catalogs."
    $invoiceEntry = @($summary.document_entries | Where-Object { $_.document_name -eq "invoice.docx" } | Select-Object -First 1)
    $contractEntry = @($summary.document_entries | Where-Object { $_.document_name -eq "contract.docx" } | Select-Object -First 1)
    Assert-True -Condition ($null -ne $invoiceEntry) `
        -Message "Aggregate rollup should include the invoice document entry."
    Assert-True -Condition ($null -ne $contractEntry) `
        -Message "Aggregate rollup should include the contract document entry."
    Assert-Equal -Actual ([int]$invoiceEntry.style_merge_suggestion_count) -Expected 1 `
        -Message "Aggregate rollup should expose invoice style merge suggestion counts."
    Assert-Equal -Actual ([int]$invoiceEntry.style_merge_suggestion_pending_count) -Expected 1 `
        -Message "Aggregate rollup should expose invoice pending style merge suggestion counts."
    Assert-Equal -Actual ([int]$contractEntry.style_merge_suggestion_count) -Expected 2 `
        -Message "Aggregate rollup should expose contract style merge suggestion counts."
    Assert-Equal -Actual ([string]$contractEntry.style_merge_review_status) -Expected "missing" `
        -Message "Aggregate rollup should expose source style merge review status."
    Assert-ContainsText -Text (($summary.catalog_exemplars | ForEach-Object { [string]$_.exemplar_catalog_path }) -join "`n") `
        -ExpectedText "contract/exemplar.numbering-catalog.json" `
        -Message "Aggregate rollup should include contract exemplar catalog path."
    $styleMergeWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "document_skeleton.style_merge_suggestions_pending" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$styleMergeWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Aggregate rollup should preserve warning source schema."
    Assert-Equal -Actual ([int]$styleMergeWarning[0].style_merge_suggestion_count) -Expected 3 `
        -Message "Aggregate rollup should preserve warning style merge counts."
    Assert-Equal -Actual ([int]$styleMergeWarning[0].style_merge_suggestion_pending_count) -Expected 3 `
        -Message "Aggregate rollup should preserve warning pending style merge counts."

    $issueSummaryText = ($summary.issue_summary | ForEach-Object { "$($_.issue):$($_.count)" }) -join "`n"
    Assert-ContainsText -Text $issueSummaryText -ExpectedText "missing_numbering_definition:2" `
        -Message "Aggregate rollup should group missing definition issues."
    Assert-ContainsText -Text $issueSummaryText -ExpectedText "dangling_numbering_instance:1" `
        -Message "Aggregate rollup should group dangling instance issues."
    Assert-Equal -Actual ([string]$summary.release_blockers[0].document_name) -Expected "contract.docx" `
        -Message "Release blockers should carry the source document name."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Catalog Exemplars" `
        -Message "Markdown report should include catalog exemplar section."
    Assert-ContainsText -Text $markdown -ExpectedText "contract.docx" `
        -Message "Markdown report should include source document names."
    Assert-ContainsText -Text $markdown -ExpectedText "missing_numbering_definition" `
        -Message "Markdown report should include issue summary."
    Assert-ContainsText -Text $markdown -ExpectedText "style_merge_suggestions=``2``" `
        -Message "Markdown report should include per-document style merge suggestion counts."
    Assert-ContainsText -Text $markdown -ExpectedText "pending_style_merge_suggestions=``2``" `
        -Message "Markdown report should include per-document pending style merge suggestion counts."
    Assert-ContainsText -Text $markdown -ExpectedText "### Document skeleton governance rollup warnings" `
        -Message "Markdown report should include warning subsection."
    Assert-ContainsText -Text $markdown -ExpectedText '- warning_count: `1`' `
        -Message "Markdown report should include warning count."
    Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.document_skeleton_governance_rollup_report.v1`' `
        -Message "Markdown report should include warning source schema."
    Assert-ContainsText -Text $markdown -ExpectedText 'style_merge_suggestion_count: `3`' `
        -Message "Markdown report should include warning style merge counts."
}

if (Test-Scenario -Name "style_merge_review") {
    $reviewRoot = Join-Path $resolvedWorkingDir "style-merge-review-input"
    $reviewOutputDir = Join-Path $resolvedWorkingDir "style-merge-review-output"
    $reviewedReportPath = Join-Path $reviewRoot "reviewed\document_skeleton_governance.summary.json"

    Write-JsonFile -Path $reviewedReportPath -Value (New-SkeletonSummary `
        -InputDocx "samples/reviewed.docx" `
        -CatalogPath "output/document-skeleton-governance/reviewed/exemplar.numbering-catalog.json" `
        -Status "clean" `
        -StyleMergeSuggestionCount 2 `
        -StyleMergeSuggestionPendingCount 0 `
        -StyleMergeReviewStatus "reviewed" `
        -UsageTotal 3)

    $reviewResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $reviewRoot,
        "-OutputDir", $reviewOutputDir
    )
    Assert-Equal -Actual $reviewResult.ExitCode -Expected 0 `
        -Message "Reviewed style merge suggestions should not keep rollup in needs_review. Output: $($reviewResult.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $reviewOutputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "clean" `
        -Message "Reviewed style merge suggestions should allow a clean rollup status."
    Assert-Equal -Actual ([int]$summary.total_style_merge_suggestion_count) -Expected 2 `
        -Message "Rollup should preserve total reviewed style merge suggestions."
    Assert-Equal -Actual ([int]$summary.total_style_merge_suggestion_pending_count) -Expected 0 `
        -Message "Rollup should not report reviewed style merge suggestions as pending."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 0 `
        -Message "Rollup should not emit pending warning after full review."
    Assert-Equal -Actual ([string]$summary.document_entries[0].style_merge_review_status) -Expected "reviewed" `
        -Message "Rollup should expose reviewed style merge status per document."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $reviewOutputDir "document_skeleton_governance_rollup.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Pending style-merge suggestions: ``0``" `
        -Message "Markdown should show zero pending style merge suggestions."
    Assert-ContainsText -Text $markdown -ExpectedText "style_merge_review=``reviewed``" `
        -Message "Markdown should show reviewed style merge status."
}

if (Test-Scenario -Name "empty") {
    $emptyRoot = Join-Path $resolvedWorkingDir "empty-input"
    $emptyOutputDir = Join-Path $resolvedWorkingDir "empty-output"
    $emptyReportPath = Join-Path $emptyRoot "document_skeleton_governance.summary.json"

    Write-JsonFile -Path $emptyReportPath -Value (New-SkeletonSummary `
        -InputDocx "samples/clean.docx" `
        -CatalogPath "output/document-skeleton-governance/clean/exemplar.numbering-catalog.json" `
        -Status "clean" `
        -DefinitionCount 1 `
        -InstanceCount 1 `
        -UsageTotal 2)

    $emptyResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $emptyRoot,
        "-OutputDir", $emptyOutputDir
    )
    Assert-Equal -Actual $emptyResult.ExitCode -Expected 0 `
        -Message "Clean rollup should pass. Output: $($emptyResult.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $emptyOutputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "clean" `
        -Message "Clean rollup should set status=clean."
    Assert-Equal -Actual ([bool]$summary.clean) -Expected $true `
        -Message "Clean rollup should set clean=true."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Clean rollup should not invent release blockers."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 0 `
        -Message "Clean rollup should not warn for a valid skeleton summary."
}

if (Test-Scenario -Name "failed_source_report") {
    $failedSourceRoot = Join-Path $resolvedWorkingDir "failed-source-input"
    $failedSourceOutputDir = Join-Path $resolvedWorkingDir "failed-source-output"
    $failedSourceReportPath = Join-Path $failedSourceRoot "document_skeleton_governance.summary.json"

    $failedSummary = New-SkeletonSummary `
        -InputDocx "samples/failed.docx" `
        -CatalogPath "output/document-skeleton-governance/failed/exemplar.numbering-catalog.json" `
        -Status "failed" `
        -IssueCount 0 `
        -SuggestionCount 0
    $failedSummary.command_failure_count = 1

    Write-JsonFile -Path $failedSourceReportPath -Value $failedSummary

    $failedSourceResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $failedSourceRoot,
        "-OutputDir", $failedSourceOutputDir
    )
    Assert-Equal -Actual $failedSourceResult.ExitCode -Expected 1 `
        -Message "Rollup should fail when a readable source report is already failed. Output: $($failedSourceResult.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $failedSourceOutputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "failed" `
        -Message "Rollup should inherit failed single-document report status."
    Assert-Equal -Actual ([int]$summary.source_report_failure_count) -Expected 1 `
        -Message "Rollup should count failed single-document summaries separately from read failures."
    Assert-Equal -Actual ([int]$summary.source_failure_count) -Expected 0 `
        -Message "Rollup should keep source read failures separate from failed report statuses."
    Assert-Equal -Actual ([int]$summary.total_command_failure_count) -Expected 1 `
        -Message "Rollup should preserve command failure totals."
}

if (Test-Scenario -Name "malformed") {
    $malformedRoot = Join-Path $resolvedWorkingDir "malformed-input"
    $malformedOutputDir = Join-Path $resolvedWorkingDir "malformed-output"
    $malformedReportPath = Join-Path $malformedRoot "document_skeleton_governance.summary.json"
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($malformedReportPath)) -Force | Out-Null
    Set-Content -LiteralPath $malformedReportPath -Encoding UTF8 -Value "{ this is not valid json"

    $malformedResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $malformedRoot,
        "-OutputDir", $malformedOutputDir
    )
    Assert-Equal -Actual $malformedResult.ExitCode -Expected 1 `
        -Message "Malformed rollup should fail after writing summary. Output: $($malformedResult.Text)"
    $summaryPath = Join-Path $malformedOutputDir "summary.json"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Malformed rollup should still write summary.json."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "failed" `
        -Message "Malformed rollup should set status=failed."
    Assert-Equal -Actual ([int]$summary.source_failure_count) -Expected 1 `
        -Message "Malformed rollup should count failed source reads."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "source_report_read_failed" `
        -Message "Malformed rollup should include a source read warning."
    $sourceReadWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "source_report_read_failed" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$sourceReadWarning[0].action) -Expected "fix_document_skeleton_governance_rollup_input_json" `
        -Message "Malformed rollup should expose a fixed remediation action."
    Assert-Equal -Actual ([string]$sourceReadWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Malformed rollup should expose the rollup source schema."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $malformedOutputDir "document_skeleton_governance_rollup.md")
    Assert-ContainsText -Text $markdown -ExpectedText "### Document skeleton governance rollup warnings" `
        -Message "Malformed Markdown should include the rollup warning subsection."
    Assert-ContainsText -Text $markdown -ExpectedText '- warning_count: `1`' `
        -Message "Malformed Markdown should include warning count."
    Assert-ContainsText -Text $markdown -ExpectedText 'id: `source_report_read_failed`' `
        -Message "Malformed Markdown should include warning id."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `fix_document_skeleton_governance_rollup_input_json`' `
        -Message "Malformed Markdown should include warning action."
    Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.document_skeleton_governance_rollup_report.v1`' `
        -Message "Malformed Markdown should include warning source schema."
}

if (Test-Scenario -Name "warning_metadata") {
    $warningMetadataRoot = Join-Path $resolvedWorkingDir "warning-metadata-input"
    $warningMetadataOutputDir = Join-Path $resolvedWorkingDir "warning-metadata-output"
    $wrongSchemaPath = Join-Path $warningMetadataRoot "wrong-schema\summary.json"
    $mismatchPath = Join-Path $warningMetadataRoot "mismatch\summary.json"

    Write-JsonFile -Path $wrongSchemaPath -Value ([ordered]@{
        schema = "featherdoc.unrelated_report.v1"
        status = "ready"
    })

    $mismatchSummary = New-SkeletonSummary `
        -InputDocx "samples/mismatch.docx" `
        -CatalogPath "output/document-skeleton-governance/mismatch/exemplar.numbering-catalog.json" `
        -Status "needs_review" `
        -ReleaseBlockers @(
            [ordered]@{
                id = "document_skeleton.style_numbering_issues"
                severity = "error"
                message = "Style numbering audit reported issues."
                action = "review_style_numbering_audit"
                issue_count = 1
            }
        )
    $mismatchSummary.release_blocker_count = 3
    Write-JsonFile -Path $mismatchPath -Value $mismatchSummary

    $result = Invoke-RollupScript -Arguments @(
        "-InputRoot", $warningMetadataRoot,
        "-OutputDir", $warningMetadataOutputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Warning metadata rollup should not fail without fail switches. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $warningMetadataOutputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 2 `
        -Message "Warning metadata rollup should produce schema and mismatch warnings."

    $schemaWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "source_report_schema_skipped" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$schemaWarning[0].action) -Expected "provide_document_skeleton_governance_report" `
        -Message "Schema skipped warning should expose a remediation action."
    Assert-Equal -Actual ([string]$schemaWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Schema skipped warning should expose the rollup source schema."

    $countWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "release_blocker_count_mismatch" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$countWarning[0].action) -Expected "reconcile_document_skeleton_governance_rollup_counts" `
        -Message "Count mismatch warning should expose a remediation action."
    Assert-Equal -Actual ([string]$countWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Count mismatch warning should expose the rollup source schema."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $warningMetadataOutputDir "document_skeleton_governance_rollup.md")
    Assert-ContainsText -Text $markdown -ExpectedText '- warning_count: `2`' `
        -Message "Warning metadata Markdown should include warning count."
    Assert-ContainsText -Text $markdown -ExpectedText 'id: `source_report_schema_skipped`' `
        -Message "Warning metadata Markdown should include schema warning id."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `provide_document_skeleton_governance_report`' `
        -Message "Warning metadata Markdown should include schema warning action."
    Assert-ContainsText -Text $markdown -ExpectedText 'id: `release_blocker_count_mismatch`' `
        -Message "Warning metadata Markdown should include count warning id."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `reconcile_document_skeleton_governance_rollup_counts`' `
        -Message "Warning metadata Markdown should include count warning action."
    Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.document_skeleton_governance_rollup_report.v1`' `
        -Message "Warning metadata Markdown should include warning source schema."
}

if (Test-Scenario -Name "fail_on_issue") {
    $failOnIssueRoot = Join-Path $resolvedWorkingDir "fail-on-issue-input"
    $failOnIssueOutputDir = Join-Path $resolvedWorkingDir "fail-on-issue-output"
    $failOnIssueReportPath = Join-Path $failOnIssueRoot "document_skeleton_governance.summary.json"

    Write-JsonFile -Path $failOnIssueReportPath -Value (New-SkeletonSummary `
        -InputDocx "samples/needs-review.docx" `
        -CatalogPath "output/document-skeleton-governance/needs-review/exemplar.numbering-catalog.json" `
        -Status "needs_review" `
        -IssueCount 1 `
        -SuggestionCount 1 `
        -IssueSummary @([ordered]@{ issue = "missing_numbering_definition"; count = 1 }))

    $failOnIssueResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $failOnIssueRoot,
        "-OutputDir", $failOnIssueOutputDir,
        "-FailOnIssue"
    )
    Assert-Equal -Actual $failOnIssueResult.ExitCode -Expected 1 `
        -Message "Rollup should fail with -FailOnIssue when issues exist. Output: $($failOnIssueResult.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $failOnIssueOutputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.total_style_numbering_issue_count) -Expected 1 `
        -Message "Fail-on-issue summary should still be written with issue counts."
}

Write-Host "Document skeleton governance rollup regression passed."
