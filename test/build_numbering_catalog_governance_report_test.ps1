param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "clean", "alignment_gap", "malformed", "fail_on_blocker", "missing_inputs")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\build_numbering_catalog_governance_report_test"
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Test-Scenario {
    param([string]$Name)
    return ($Scenario -eq "all" -or $Scenario -eq $Name)
}

function Invoke-GovernanceScript {
    param([string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellCommand = Get-Command pwsh -ErrorAction SilentlyContinue
        if (-not $powerShellCommand) {
            $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
        }
        if (-not $powerShellCommand) {
            throw "Unable to resolve a PowerShell executable for the nested script invocation."
        }
        $powerShellPath = $powerShellCommand.Source
    }
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
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-SkeletonRollup {
    param([bool]$Clean)

    if ($Clean) {
        return [ordered]@{
            schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
            status = "clean"
            clean = $true
            document_count = 1
            total_style_numbering_issue_count = 0
            total_style_numbering_suggestion_count = 0
            total_numbering_definition_count = 2
            total_numbering_instance_count = 3
            total_style_usage_count = 5
            total_command_failure_count = 0
            issue_summary = @()
            catalog_exemplars = @(
                [ordered]@{
                    document_name = "invoice.docx"
                    input_docx = "samples/invoice.docx"
                    input_docx_display = ".\samples\invoice.docx"
                    exemplar_catalog_path = "output/document-skeleton-governance/invoice/exemplar.numbering-catalog.json"
                    exemplar_catalog_display = ".\output\document-skeleton-governance\invoice\exemplar.numbering-catalog.json"
                    definition_count = 2
                    instance_count = 3
                }
            )
            release_blockers = @()
            action_items = @(
                [ordered]@{
                    id = "promote_numbering_catalog_exemplar"
                    document_name = "invoice.docx"
                    action = "promote_numbering_catalog_exemplar"
                    title = "Review and promote the generated exemplar numbering catalog"
                    command = "featherdoc_cli check-numbering-catalog samples/invoice.docx --json"
                },
                [ordered]@{
                    id = "register_numbering_catalog_baseline"
                    document_name = "invoice.docx"
                    action = "register_numbering_catalog_baseline"
                    title = "Register the exemplar catalog in the numbering catalog baseline flow"
                    command = "pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 -InputDocx samples/invoice.docx"
                }
            )
        }
    }

    return [ordered]@{
        schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
        status = "needs_review"
        clean = $false
        document_count = 2
        total_style_numbering_issue_count = 3
        total_style_numbering_suggestion_count = 2
        total_numbering_definition_count = 6
        total_numbering_instance_count = 9
        total_style_usage_count = 14
        total_command_failure_count = 0
        issue_summary = @(
            [ordered]@{ issue = "missing_numbering_definition"; count = 2 },
            [ordered]@{ issue = "dangling_numbering_instance"; count = 1 }
        )
        catalog_exemplars = @(
            [ordered]@{
                document_name = "invoice.docx"
                input_docx = "samples/invoice.docx"
                input_docx_display = ".\samples\invoice.docx"
                exemplar_catalog_path = "output/document-skeleton-governance/invoice/exemplar.numbering-catalog.json"
                exemplar_catalog_display = ".\output\document-skeleton-governance\invoice\exemplar.numbering-catalog.json"
                definition_count = 2
                instance_count = 3
            },
            [ordered]@{
                document_name = "contract.docx"
                input_docx = "samples/contract.docx"
                input_docx_display = ".\samples\contract.docx"
                exemplar_catalog_path = "output/document-skeleton-governance/contract/exemplar.numbering-catalog.json"
                exemplar_catalog_display = ".\output\document-skeleton-governance\contract\exemplar.numbering-catalog.json"
                definition_count = 4
                instance_count = 6
            }
        )
        release_blockers = @(
            [ordered]@{
                id = "document_skeleton.style_numbering_issues"
                document_name = "contract.docx"
                severity = "error"
                status = "needs_review"
                action = "review_style_numbering_audit"
                message = "Style numbering audit reported issues."
            }
        )
        action_items = @(
            [ordered]@{
                id = "review_style_numbering_audit"
                document_name = "contract.docx"
                action = "review_style_numbering_audit"
                title = "Review contract style numbering audit"
                command = "featherdoc_cli audit-style-numbering samples/contract.docx --fail-on-issue --json"
                audit_command = "featherdoc_cli audit-style-numbering samples/contract.docx --json"
                review_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1"
            }
        )
    }
}

function New-ManifestSummary {
    param([bool]$Clean)

    if ($Clean) {
        return [ordered]@{
            generated_at = "2026-05-03T00:00:00"
            manifest_path = "baselines/numbering-catalog/manifest.json"
            entry_count = 1
            drift_count = 0
            dirty_baseline_count = 0
            issue_entry_count = 0
            passed = $true
            entries = @(
                [ordered]@{
                    name = "invoice"
                    input_docx = "samples/invoice.docx"
                    matches = $true
                    clean = $true
                    catalog_file = "baselines/numbering-catalog/invoice.json"
                    catalog_lint_clean = $true
                    catalog_lint_issue_count = 0
                    generated_output_path = "output/numbering-catalog-manifest-checks/invoice.generated.numbering-catalog.json"
                    baseline_issue_count = 0
                    generated_issue_count = 0
                    added_definition_count = 0
                    removed_definition_count = 0
                    changed_definition_count = 0
                }
            )
        }
    }

    return [ordered]@{
        generated_at = "2026-05-03T00:00:00"
        manifest_path = "baselines/numbering-catalog/manifest.json"
        entry_count = 2
        drift_count = 1
        dirty_baseline_count = 1
        issue_entry_count = 1
        passed = $false
        entries = @(
            [ordered]@{
                name = "invoice"
                input_docx = "samples/invoice.docx"
                matches = $true
                clean = $true
                catalog_file = "baselines/numbering-catalog/invoice.json"
                catalog_lint_clean = $true
                catalog_lint_issue_count = 0
                generated_output_path = "output/numbering-catalog-manifest-checks/invoice.generated.numbering-catalog.json"
                baseline_issue_count = 0
                generated_issue_count = 0
                added_definition_count = 0
                removed_definition_count = 0
                changed_definition_count = 0
            },
            [ordered]@{
                name = "contract"
                input_docx = "samples/contract.docx"
                matches = $false
                clean = $false
                catalog_file = "baselines/numbering-catalog/contract.json"
                catalog_lint_clean = $false
                catalog_lint_issue_count = 2
                generated_output_path = "output/numbering-catalog-manifest-checks/contract.generated.numbering-catalog.json"
                baseline_issue_count = 2
                generated_issue_count = 0
                added_definition_count = 0
                removed_definition_count = 1
                changed_definition_count = 1
            }
        )
    }
}

function New-AlignmentGapManifestSummary {
    return [ordered]@{
        generated_at = "2026-05-03T00:00:00"
        manifest_path = "baselines/numbering-catalog/manifest.json"
        entry_count = 2
        drift_count = 0
        dirty_baseline_count = 0
        issue_entry_count = 0
        passed = $true
        entries = @(
            [ordered]@{
                name = "invoice"
                input_docx = "samples/invoice.docx"
                matches = $true
                clean = $true
                catalog_file = "baselines/numbering-catalog/invoice.json"
                catalog_lint_clean = $true
                catalog_lint_issue_count = 0
                generated_output_path = "output/numbering-catalog-manifest-checks/invoice.generated.numbering-catalog.json"
                baseline_issue_count = 0
                generated_issue_count = 0
                added_definition_count = 0
                removed_definition_count = 0
                changed_definition_count = 0
            },
            [ordered]@{
                name = "obsolete"
                input_docx = "samples/obsolete.docx"
                matches = $true
                clean = $true
                catalog_file = "baselines/numbering-catalog/obsolete.json"
                catalog_lint_clean = $true
                catalog_lint_issue_count = 0
                generated_output_path = "output/numbering-catalog-manifest-checks/obsolete.generated.numbering-catalog.json"
                baseline_issue_count = 0
                generated_issue_count = 0
                added_definition_count = 0
                removed_definition_count = 0
                changed_definition_count = 0
            }
        )
    }
}

function New-Evidence {
    param([string]$Root, [bool]$Clean)

    $skeletonPath = Join-Path $Root "skeleton\summary.json"
    $manifestPath = Join-Path $Root "manifest\summary.json"
    Write-JsonFile -Path $skeletonPath -Value (New-SkeletonRollup -Clean:$Clean)
    Write-JsonFile -Path $manifestPath -Value (New-ManifestSummary -Clean:$Clean)
    return [pscustomobject]@{
        Skeleton = $skeletonPath
        Manifest = $manifestPath
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_numbering_catalog_governance_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "aggregate") {
    $evidence = New-Evidence -Root (Join-Path $resolvedWorkingDir "aggregate-evidence") -Clean:$false
    $outputDir = Join-Path $resolvedWorkingDir "aggregate-report"
    $result = Invoke-GovernanceScript -Arguments @(
        "-InputJson", "$($evidence.Skeleton),$($evidence.Manifest)",
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Governance report should pass without fail switches. Output: $($result.Text)"
    Assert-ContainsText -Text $result.Text -ExpectedText "Summary JSON:" `
        -Message "Script should print summary path."

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "numbering_catalog_governance.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Governance report should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Governance report should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.numbering_catalog_governance_report.v1" `
        -Message "Summary should expose the numbering governance schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Summary should require review with style issues and baseline drift."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $false `
        -Message "Summary should not be release-ready with blockers."
    Assert-Equal -Actual ([int]$summary.document_count) -Expected 2 `
        -Message "Summary should preserve document count."
    Assert-Equal -Actual ([int]$summary.baseline_entry_count) -Expected 2 `
        -Message "Summary should preserve manifest entry count."
    Assert-Equal -Actual ([int]$summary.catalog_exemplar_count) -Expected 2 `
        -Message "Summary should preserve exemplar catalog entries."
    Assert-Equal -Actual ([int]$summary.real_corpus_confidence_score) -Expected 56 `
        -Message "Summary should score real-corpus confidence from coverage and issue penalties."
    Assert-Equal -Actual ([string]$summary.real_corpus_confidence_level) -Expected "low" `
        -Message "Issue-heavy real corpus evidence should produce low confidence."
    Assert-Equal -Actual ([int]$summary.real_corpus_confidence.catalog_coverage_percent) -Expected 100 `
        -Message "Summary should expose catalog exemplar coverage."
    Assert-Equal -Actual ([int]$summary.real_corpus_confidence.baseline_coverage_percent) -Expected 100 `
        -Message "Summary should expose numbering baseline coverage."
    Assert-Equal -Actual ([int]$summary.real_corpus_confidence.matched_document_count) -Expected 2 `
        -Message "Summary should expose matched real-corpus document count."
    Assert-ContainsText -Text (($summary.real_corpus_confidence.penalty_summary | ForEach-Object { "$($_.factor):$($_.penalty)" }) -join "`n") `
        -ExpectedText "style_numbering_issues:15" `
        -Message "Summary should expose style-numbering confidence penalties."
    Assert-Equal -Actual ([int]$summary.total_style_numbering_issue_count) -Expected 3 `
        -Message "Summary should preserve style-numbering issue totals."
    Assert-Equal -Actual ([int]$summary.drift_count) -Expected 1 `
        -Message "Summary should preserve catalog drift count."
    Assert-Equal -Actual ([int]$summary.dirty_baseline_count) -Expected 1 `
        -Message "Summary should preserve dirty baseline count."
    Assert-Equal -Actual ([int]$summary.issue_entry_count) -Expected 1 `
        -Message "Summary should preserve issue entry count."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
        -Message "Summary should include skeleton and manifest release blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 2 `
        -Message "Summary should include skeleton and manifest action items."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.audit_command }) -join "`n") `
        -ExpectedText "audit-style-numbering" `
        -Message "Summary should preserve upstream action audit commands."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.review_command }) -join "`n") `
        -ExpectedText "build_document_skeleton_governance_rollup_report.ps1" `
        -Message "Summary should preserve upstream action review commands."

    $issueSummaryText = ($summary.style_issue_summary | ForEach-Object { "$($_.issue):$($_.count)" }) -join "`n"
    Assert-ContainsText -Text $issueSummaryText -ExpectedText "missing_numbering_definition:2" `
        -Message "Summary should preserve missing definition issue summary."
    Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "numbering_catalog_governance.dirty_baseline" `
        -Message "Summary should create dirty baseline blocker."
    $skeletonBlocker = @($summary.release_blockers |
        Where-Object { [string]$_.id -eq "document_skeleton.style_numbering_issues" })[0]
    Assert-ContainsText -Text ([string]$skeletonBlocker.source_report_display) `
        -ExpectedText "skeleton\summary.json" `
        -Message "Skeleton blockers should expose the source report display path."
    Assert-ContainsText -Text ([string]$skeletonBlocker.source_json_display) `
        -ExpectedText "skeleton\summary.json" `
        -Message "Skeleton blockers should expose the source JSON display path."
    $dirtyBaselineBlocker = @($summary.release_blockers |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.dirty_baseline" })[0]
    Assert-ContainsText -Text ([string]$dirtyBaselineBlocker.source_report_display) `
        -ExpectedText "manifest\summary.json" `
        -Message "Baseline blockers should expose the source report display path."
    Assert-ContainsText -Text ([string]$dirtyBaselineBlocker.source_json_display) `
        -ExpectedText "manifest\summary.json" `
        -Message "Baseline blockers should expose the source JSON display path."
    $skeletonAction = @($summary.action_items |
        Where-Object { [string]$_.id -eq "review_style_numbering_audit" })[0]
    Assert-ContainsText -Text ([string]$skeletonAction.source_report_display) `
        -ExpectedText "skeleton\summary.json" `
        -Message "Skeleton action items should expose the source report display path."
    Assert-ContainsText -Text ([string]$skeletonAction.source_json_display) `
        -ExpectedText "skeleton\summary.json" `
        -Message "Skeleton action items should expose the source JSON display path."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Numbering Catalog Governance Report" `
        -Message "Markdown should include title."
    Assert-ContainsText -Text $markdown -ExpectedText "Catalog Exemplars" `
        -Message "Markdown should include exemplar section."
    Assert-ContainsText -Text $markdown -ExpectedText "Baseline Manifest Entries" `
        -Message "Markdown should include baseline manifest section."
    Assert-ContainsText -Text $markdown -ExpectedText "Real Corpus Confidence" `
        -Message "Markdown should include real corpus confidence section."
    Assert-ContainsText -Text $markdown -ExpectedText "Matched documents" `
        -Message "Markdown should include real corpus alignment details."
    Assert-ContainsText -Text $markdown -ExpectedText "Catalog document keys" `
        -Message "Markdown should include real corpus catalog document keys."
    Assert-ContainsText -Text $markdown -ExpectedText "Baseline document keys" `
        -Message "Markdown should include real corpus baseline document keys."
    Assert-ContainsText -Text $markdown -ExpectedText "Matched document keys" `
        -Message "Markdown should include real corpus matched document keys."
    Assert-ContainsText -Text $markdown -ExpectedText '`contract`' `
        -Message "Markdown should include the contract document key."
    Assert-ContainsText -Text $markdown -ExpectedText '`invoice`' `
        -Message "Markdown should include the invoice document key."
    Assert-ContainsText -Text $markdown -ExpectedText "style_numbering_issues" `
        -Message "Markdown should include real corpus confidence penalty details."
    Assert-ContainsText -Text $markdown -ExpectedText "missing_numbering_definition" `
        -Message "Markdown should include issue summary."
    Assert-ContainsText -Text $markdown -ExpectedText "audit_command:" `
        -Message "Markdown should include action audit commands."
    Assert-ContainsText -Text $markdown -ExpectedText "review_command:" `
        -Message "Markdown should include action review commands."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report_display=" `
        -Message "Markdown should include source report display fields."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display=" `
        -Message "Markdown should include source JSON display fields."
}

if (Test-Scenario -Name "missing_inputs") {
    $missingInputRoot = Join-Path $resolvedWorkingDir "missing-inputs"
    $missingOutputDir = Join-Path $resolvedWorkingDir "missing-inputs-report"
    New-Item -ItemType Directory -Path $missingInputRoot -Force | Out-Null
    $result = Invoke-GovernanceScript -Arguments @(
        "-InputRoot", $missingInputRoot,
        "-OutputDir", $missingOutputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Missing input governance report should pass with actionable warnings. Output: $($result.Text)"

    $summaryPath = Join-Path $missingOutputDir "summary.json"
    $markdownPath = Join-Path $missingOutputDir "numbering_catalog_governance.md"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 2 `
        -Message "Missing input report should warn for skeleton rollup and manifest summary."

    $manifestWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "numbering_catalog_manifest_summary_missing" })[0]
    Assert-True -Condition ($null -ne $manifestWarning) `
        -Message "Missing input report should include numbering_catalog_manifest_summary_missing."
    Assert-Equal -Actual ([string]$manifestWarning.repair_strategy) -Expected "rebuild_numbering_catalog_manifest_summary" `
        -Message "Manifest warning should expose the repair strategy."
    Assert-ContainsText -Text ([string]$manifestWarning.repair_hint) -ExpectedText "do not synthesize" `
        -Message "Manifest warning should preserve the evidence-boundary repair hint."
    Assert-ContainsText -Text ([string]$manifestWarning.command_template) -ExpectedText "check_numbering_catalog_manifest.ps1" `
        -Message "Manifest warning should include the manifest-check command template."
    Assert-ContainsText -Text $markdown -ExpectedText "command_template:" `
        -Message "Missing input Markdown should expose warning command templates."
}

if (Test-Scenario -Name "clean") {
    $evidence = New-Evidence -Root (Join-Path $resolvedWorkingDir "clean-evidence") -Clean:$true
    $outputDir = Join-Path $resolvedWorkingDir "clean-report"
    $result = Invoke-GovernanceScript -Arguments @(
        "-InputJson", "$($evidence.Skeleton),$($evidence.Manifest)",
        "-OutputDir", $outputDir,
        "-FailOnBlocker",
        "-FailOnDrift",
        "-FailOnIssue"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Clean governance evidence should pass all fail switches. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "clean" `
        -Message "Clean evidence should produce clean status."
    Assert-Equal -Actual ([bool]$summary.clean) -Expected $true `
        -Message "Clean evidence should set clean=true."
    Assert-Equal -Actual ([int]$summary.real_corpus_confidence_score) -Expected 100 `
        -Message "Clean covered real corpus evidence should produce full confidence."
    Assert-Equal -Actual ([string]$summary.real_corpus_confidence_level) -Expected "high" `
        -Message "Clean covered real corpus evidence should produce high confidence."
    Assert-Equal -Actual ([int]$summary.real_corpus_confidence.matched_document_count) -Expected 1 `
        -Message "Clean evidence should align the single catalog exemplar with its baseline."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Clean evidence should not expose blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 0 `
        -Message "Clean release checklist actions should not count as actionable release handoff items."
    Assert-Equal -Actual ([int]$summary.informational_action_item_count) -Expected 2 `
        -Message "Clean release checklist actions should remain as informational handoff evidence."
    $informationalActionText = ($summary.informational_action_items | ForEach-Object { "$($_.id):$($_.category):$($_.release_blocking):$($_.optional)" }) -join "`n"
    Assert-ContainsText -Text $informationalActionText -ExpectedText "promote_numbering_catalog_exemplar:release_checklist:False:True" `
        -Message "Promote exemplar checklist action should be non-blocking informational evidence."
    Assert-ContainsText -Text $informationalActionText -ExpectedText "register_numbering_catalog_baseline:release_checklist:False:True" `
        -Message "Register baseline checklist action should be non-blocking informational evidence."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 0 `
        -Message "Clean evidence should not warn."
}

if (Test-Scenario -Name "alignment_gap") {
    $evidenceRoot = Join-Path $resolvedWorkingDir "alignment-gap-evidence"
    $skeletonPath = Join-Path $evidenceRoot "skeleton\summary.json"
    $manifestPath = Join-Path $evidenceRoot "manifest\summary.json"
    Write-JsonFile -Path $skeletonPath -Value (New-SkeletonRollup -Clean:$false)
    Write-JsonFile -Path $manifestPath -Value (New-AlignmentGapManifestSummary)

    $outputDir = Join-Path $resolvedWorkingDir "alignment-gap-report"
    $result = Invoke-GovernanceScript -Arguments @(
        "-InputJson", "$skeletonPath,$manifestPath",
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Alignment gap governance report should pass without fail switches. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "numbering_catalog_governance.md"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json

    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Alignment gaps should require review."
    Assert-Equal -Actual ([int]$summary.real_corpus_confidence.matched_document_count) -Expected 1 `
        -Message "Real corpus confidence should match catalog and baseline by document identity."
    Assert-Equal -Actual ([int]$summary.real_corpus_confidence.catalog_coverage_percent) -Expected 50 `
        -Message "Catalog coverage should fall when one real-corpus document has no matching baseline."
    Assert-Equal -Actual ([int]$summary.real_corpus_confidence.baseline_coverage_percent) -Expected 50 `
        -Message "Baseline coverage should fall when one baseline does not match a real-corpus document."
    Assert-Equal -Actual ([int]$summary.real_corpus_confidence.coverage_score) -Expected 50 `
        -Message "Coverage score should use document identity alignment, not only entry counts."
    Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "numbering_catalog_governance.real_corpus_alignment_gap" `
        -Message "Alignment gaps should create a release blocker."

    $alignmentBlocker = @($summary.release_blockers |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_alignment_gap" })[0]
    Assert-Equal -Actual ([string]$alignmentBlocker.source_schema) `
        -Expected "featherdoc.numbering_catalog_governance_report.v1" `
        -Message "Alignment blocker should preserve the numbering governance source schema."
    Assert-ContainsText -Text ([string]$alignmentBlocker.source_report_display) `
        -ExpectedText "summary.json" `
        -Message "Alignment blocker should expose the governance source report display path."
    Assert-ContainsText -Text ([string]$alignmentBlocker.source_json_display) `
        -ExpectedText "summary.json" `
        -Message "Alignment blocker should expose the governance source JSON display path."
    Assert-Equal -Actual ([int]$alignmentBlocker.matched_document_count) -Expected 1 `
        -Message "Alignment blocker should expose matched document count."
    Assert-Equal -Actual ([int]$alignmentBlocker.unmatched_catalog_document_count) -Expected 1 `
        -Message "Alignment blocker should expose unmatched catalog document count."
    Assert-Equal -Actual ([int]$alignmentBlocker.unmatched_baseline_document_count) -Expected 1 `
        -Message "Alignment blocker should expose unmatched baseline document count."
    Assert-Equal -Actual (@($alignmentBlocker.catalog_document_keys).Count) -Expected 2 `
        -Message "Alignment blocker should expose catalog document keys."
    Assert-ContainsText -Text (($alignmentBlocker.catalog_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract" `
        -Message "Alignment blocker should expose the unmatched catalog document key."
    Assert-Equal -Actual (@($alignmentBlocker.baseline_document_keys).Count) -Expected 2 `
        -Message "Alignment blocker should expose baseline document keys."
    Assert-ContainsText -Text (($alignmentBlocker.baseline_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "obsolete" `
        -Message "Alignment blocker should expose the unmatched baseline document key."
    Assert-Equal -Actual (@($alignmentBlocker.matched_document_keys).Count) -Expected 1 `
        -Message "Alignment blocker should expose matched document keys."
    Assert-ContainsText -Text (($alignmentBlocker.matched_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "invoice" `
        -Message "Alignment blocker should expose the matched document key."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Matched documents" `
        -Message "Markdown should include real corpus alignment details."
    Assert-ContainsText -Text $markdown -ExpectedText "Unmatched catalog keys" `
        -Message "Markdown should include unmatched catalog key details."
    Assert-ContainsText -Text $markdown -ExpectedText "Unmatched baseline keys" `
        -Message "Markdown should include unmatched baseline key details."
    Assert-ContainsText -Text $markdown -ExpectedText '`contract`' `
        -Message "Markdown should include the unmatched catalog document key."
    Assert-ContainsText -Text $markdown -ExpectedText '`obsolete`' `
        -Message "Markdown should include the unmatched baseline document key."
    Assert-ContainsText -Text $markdown -ExpectedText "real_corpus_alignment_gap" `
        -Message "Markdown should include alignment gap blocker."
}

if (Test-Scenario -Name "malformed") {
    $evidenceRoot = Join-Path $resolvedWorkingDir "malformed-evidence"
    New-Item -ItemType Directory -Path $evidenceRoot -Force | Out-Null
    $badJson = Join-Path $evidenceRoot "summary.json"
    "{ not valid json" | Set-Content -LiteralPath $badJson -Encoding UTF8
    $outputDir = Join-Path $resolvedWorkingDir "malformed-report"
    $result = Invoke-GovernanceScript -Arguments @(
        "-InputJson", $badJson,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Malformed input should make the governance report fail. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "failed" `
        -Message "Malformed input should produce failed status."
    Assert-Equal -Actual ([int]$summary.source_failure_count) -Expected 1 `
        -Message "Malformed input should count one source failure."
}

if (Test-Scenario -Name "fail_on_blocker") {
    $evidence = New-Evidence -Root (Join-Path $resolvedWorkingDir "fail-on-blocker-evidence") -Clean:$false
    $outputDir = Join-Path $resolvedWorkingDir "fail-on-blocker-report"
    $result = Invoke-GovernanceScript -Arguments @(
        "-InputJson", "$($evidence.Skeleton),$($evidence.Manifest)",
        "-OutputDir", $outputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Governance report should fail with -FailOnBlocker when blockers exist. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Failing governance report should preserve needs_review status."
    Assert-True -Condition ([int]$summary.release_blocker_count -gt 0) `
        -Message "Failing governance report should write blockers."
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "numbering_catalog_governance.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blockers" `
        -Message "Failing governance report Markdown should include the blocker section."
    Assert-ContainsText -Text $markdown -ExpectedText "numbering_catalog_governance.dirty_baseline" `
        -Message "Failing governance report Markdown should include the dirty baseline blocker."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report_display=" `
        -Message "Failing governance report Markdown should include source report display fields."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display=" `
        -Message "Failing governance report Markdown should include source JSON display fields."
}

Write-Host "Numbering catalog governance report regression passed."
