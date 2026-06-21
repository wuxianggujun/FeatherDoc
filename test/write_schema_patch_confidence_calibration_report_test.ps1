param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "source_metadata", "business_dimension_metadata", "fail_on_pending")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

function Assert-HasProperty {
    param($Object, [string]$Name, [string]$Message)
    if (-not ($Object.PSObject.Properties.Name -contains $Name)) {
        throw "$Message Missing property '$Name'."
    }
}

function Assert-NonEmptyString {
    param($Value, [string]$Message)
    if ([string]::IsNullOrWhiteSpace([string]$Value)) {
        throw $Message
    }
}

function Get-TraceItemLabel {
    param($Item)

    foreach ($property in @("id", "project_id", "template_name", "action")) {
        if ($Item.PSObject.Properties.Name -contains $property) {
            $value = [string]$Item.$property
            if (-not [string]::IsNullOrWhiteSpace($value)) {
                return $value
            }
        }
    }

    return "<missing-id>"
}

function Assert-GovernanceTraceMetadata {
    param(
        [object[]]$Items,
        [string]$CollectionName,
        [bool]$ExpectOpenCommandProperty = $false
    )

    foreach ($item in @($Items)) {
        $itemLabel = Get-TraceItemLabel -Item $item
        $context = "Schema calibration $CollectionName item $itemLabel"

        foreach ($property in @("source_schema", "source_report_display", "source_json_display")) {
            Assert-HasProperty -Object $item -Name $property `
                -Message "$context should expose $property."
            Assert-NonEmptyString -Value $item.$property `
                -Message "$context should keep non-empty $property."
        }

        if ($ExpectOpenCommandProperty) {
            Assert-HasProperty -Object $item -Name "open_command" `
                -Message "$context should expose reviewer open command metadata."
            Assert-NonEmptyString -Value $item.open_command `
                -Message "$context should keep a non-empty reviewer open command."
        }
    }
}

function Test-Scenario {
    param([string]$Name)
    return ($Scenario -eq "all" -or $Scenario -eq $Name)
}

function Invoke-CalibrationScript {
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
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\write_schema_patch_confidence_calibration_report.ps1"
$scriptHelperPath = Join-Path $resolvedRepoRoot "scripts\write_schema_patch_confidence_calibration_report_helpers.ps1"

foreach ($pathToParse in @($scriptPath, $scriptHelperPath)) {
    $tokens = $null
    $errors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($pathToParse, [ref]$tokens, [ref]$errors) | Out-Null
    if ($errors.Count -gt 0) {
        throw "Schema patch confidence calibration report script has parse errors: $pathToParse"
    }
}

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$fixtureRoot = Join-Path $resolvedWorkingDir "fixtures"
$smokeSummaryPath = Join-Path $fixtureRoot "smoke\summary.json"
$noopSmokeSummaryPath = Join-Path $fixtureRoot "noop-smoke\summary.json"
$historyPath = Join-Path $fixtureRoot "history\project_template_schema_approval_history.json"
$missingSourceSummaryPath = Join-Path $resolvedWorkingDir "missing-source-fixture\summary.json"
$missingBusinessDocumentTypeSummaryPath = Join-Path $resolvedWorkingDir "missing-business-document-type-fixture\summary.json"

foreach ($fixtureDir in @($fixtureRoot, (Join-Path $resolvedWorkingDir "missing-source-fixture"), (Join-Path $resolvedWorkingDir "missing-business-document-type-fixture"))) {
    if (Test-Path -LiteralPath $fixtureDir) {
        Remove-Item -LiteralPath $fixtureDir -Recurse -Force
    }
}

Write-JsonFile -Path $smokeSummaryPath -Value ([ordered]@{
    schema_patch_review_count = 4
    schema_patch_review_changed_count = 4
    entries = @(
        [ordered]@{
            name = "invoice-template"
            business_document_type = "invoice"
        },
        [ordered]@{
            name = "contract-template"
            business_document_type = "contract"
        },
        [ordered]@{
            name = "policy-handbook-template"
            business_document_type = "policy"
        },
        [ordered]@{
            name = "project-report-template"
            business_document_type = "report"
        }
    )
    schema_patch_reviews = @(
        [ordered]@{
            name = "invoice-template"
            project_id = "project-finance"
            template_name = "invoice-template"
            candidate_type = "rename"
            review_json = "invoice.review.json"
            changed = $true
            baseline_slot_count = 3
            generated_slot_count = 4
            upsert_slot_count = 1
            remove_target_count = 0
            remove_slot_count = 0
            rename_slot_count = 1
            update_slot_count = 0
            inserted_slots = 1
            replaced_slots = 0
            confidence = 96
            reason_code = "shape_match"
            differences = @("bookmark renamed")
        },
        [ordered]@{
            name = "contract-template"
            project_id = "project-legal"
            template_name = "contract-template"
            candidate_type = "remove"
            review_json = "contract.review.json"
            changed = $true
            baseline_slot_count = 6
            generated_slot_count = 5
            upsert_slot_count = 0
            remove_target_count = 0
            remove_slot_count = 1
            rename_slot_count = 0
            update_slot_count = 1
            inserted_slots = 0
            replaced_slots = 1
        },
        [ordered]@{
            name = "policy-handbook-template"
            project_id = "project-operations"
            template_name = "policy-handbook-template"
            candidate_type = "type_update"
            review_json = "policy-handbook.review.json"
            changed = $true
            baseline_slot_count = 5
            generated_slot_count = 5
            upsert_slot_count = 0
            remove_target_count = 0
            remove_slot_count = 0
            rename_slot_count = 0
            update_slot_count = 2
            inserted_slots = 0
            replaced_slots = 2
            confidence = 74
            reason_code = "type_update_conflict"
            differences = @("two policy fields changed type")
        },
        [ordered]@{
            name = "project-report-template"
            project_id = "project-delivery"
            template_name = "project-report-template"
            candidate_type = "required_change"
            review_json = "project-report.review.json"
            changed = $true
            baseline_slot_count = 4
            generated_slot_count = 6
            upsert_slot_count = 2
            remove_target_count = 0
            remove_slot_count = 0
            rename_slot_count = 0
            update_slot_count = 0
            inserted_slots = 2
            replaced_slots = 0
            confidence = 48
            reason_code = "invalid_approval_fixture"
            differences = @("approval result failed compliance validation")
        }
    )
    schema_patch_approval_items = @(
        [ordered]@{
            name = "invoice-template"
            project_id = "project-finance"
            template_name = "invoice-template"
            candidate_type = "rename"
            status = "approved"
            decision = "approved"
            approved = $true
            pending = $false
            confidence = 96
            reason_code = "shape_match"
            approval_result = "invoice.approval.json"
            schema_update_candidate = "invoice.schema.json"
            review_json = "invoice.review.json"
            compliance_issue_count = 0
        },
        [ordered]@{
            name = "contract-template"
            project_id = "project-legal"
            template_name = "contract-template"
            candidate_type = "remove"
            status = "pending_review"
            decision = "pending"
            approved = $false
            pending = $true
            approval_result = "contract.approval.json"
            schema_update_candidate = "contract.schema.json"
            review_json = "contract.review.json"
            compliance_issue_count = 0
        },
        [ordered]@{
            name = "policy-handbook-template"
            project_id = "project-operations"
            template_name = "policy-handbook-template"
            candidate_type = "type_update"
            status = "needs_changes"
            decision = "needs_changes"
            approved = $false
            pending = $false
            confidence = 74
            reason_code = "type_update_conflict"
            approval_result = "policy-handbook.approval.json"
            schema_update_candidate = "policy-handbook.schema.json"
            review_json = "policy-handbook.review.json"
            compliance_issue_count = 0
        },
        [ordered]@{
            name = "project-report-template"
            project_id = "project-delivery"
            template_name = "project-report-template"
            candidate_type = "required_change"
            status = "invalid_result"
            decision = "unknown"
            approved = $false
            pending = $false
            confidence = 48
            reason_code = "invalid_approval_fixture"
            approval_result = "project-report.approval.json"
            schema_update_candidate = "project-report.schema.json"
            review_json = "project-report.review.json"
            compliance_issue_count = 1
        }
    )
})

Write-JsonFile -Path $noopSmokeSummaryPath -Value ([ordered]@{
    schema = "featherdoc.project_template_smoke_summary.v1"
    manifest_path = "samples/project_template_smoke.manifest.json"
    entry_count = 1
    overall_status = "passed"
    passed = $true
    failed_entry_count = 0
    entries = @(
        [ordered]@{
            name = "unchanged-contract-template"
            input_docx = "samples/unchanged-contract.docx"
            status = "passed"
            passed = $true
        }
    )
    schema_patch_review_count = 1
    schema_patch_review_changed_count = 0
    schema_patch_reviews = @(
        [ordered]@{
            name = "unchanged-contract-template"
            project_id = "project-legal"
            template_name = "unchanged-contract-template"
            changed = $false
            baseline_slot_count = 5
            generated_slot_count = 5
            upsert_slot_count = 0
            remove_target_count = 0
            remove_slot_count = 0
            rename_slot_count = 0
            update_slot_count = 0
            inserted_slots = 0
            replaced_slots = 0
        }
    )
})

Write-JsonFile -Path $historyPath -Value ([ordered]@{
    schema = "featherdoc.project_template_schema_approval_history.v1"
    entry_histories = @(
        [ordered]@{
            name = "delivery-status-report-template"
            project_id = "project-delivery"
            template_name = "delivery-status-report-template"
            runs = @(
                [ordered]@{
                    schema_patch_review_count = 1
                    schema_patch_review_changed_count = 1
                    entries = @(
                        [ordered]@{
                            name = "delivery-status-report-template"
                            business_document_type = "report"
                        }
                    )
                    schema_patch_reviews = @(
                        [ordered]@{
                            name = "delivery-status-report-template"
                            candidate_type = "add"
                            changed = $true
                            baseline_slot_count = 2
                            generated_slot_count = 3
                            upsert_slot_count = 1
                            remove_target_count = 0
                            remove_slot_count = 0
                            rename_slot_count = 0
                            update_slot_count = 0
                            inserted_slots = 1
                            replaced_slots = 0
                            confidence = 82
                            reason_code = "new_slot"
                        }
                    )
                    schema_patch_approval_items = @(
                        [ordered]@{
                            name = "delivery-status-report-template"
                            candidate_type = "add"
                            status = "needs_changes"
                            decision = "needs_changes"
                            approved = $false
                            pending = $false
                            confidence = 82
                            reason_code = "new_slot"
                            compliance_issue_count = 0
                        }
                    )
                }
            )
        }
    )
})

Write-JsonFile -Path $missingSourceSummaryPath -Value ([ordered]@{
    schema = "featherdoc.project_template_smoke_summary.v1"
    schema_patch_review_count = 1
    schema_patch_review_changed_count = 1
    schema_patch_reviews = @(
        [ordered]@{
            name = "legacy-notice-template"
            template_name = "legacy-notice-template"
            business_document_type = "notice"
            candidate_type = "add"
            review_json = "legacy-notice.review.json"
            changed = $true
            baseline_slot_count = 1
            generated_slot_count = 2
            upsert_slot_count = 1
            remove_target_count = 0
            remove_slot_count = 0
            rename_slot_count = 0
            update_slot_count = 0
            inserted_slots = 1
            replaced_slots = 0
            confidence = 91
            reason_code = "legacy_fixture_without_project_id"
        }
    )
    schema_patch_approval_items = @(
        [ordered]@{
            name = "legacy-notice-template"
            template_name = "legacy-notice-template"
            business_document_type = "notice"
            candidate_type = "add"
            status = "approved"
            decision = "approved"
            approved = $true
            pending = $false
            confidence = 91
            reason_code = "legacy_fixture_without_project_id"
            approval_result = "legacy-notice.approval.json"
            schema_update_candidate = "legacy-notice.schema.json"
            review_json = "legacy-notice.review.json"
            compliance_issue_count = 0
        }
    )
})

Write-JsonFile -Path $missingBusinessDocumentTypeSummaryPath -Value ([ordered]@{
    schema = "featherdoc.project_template_smoke_summary.v1"
    schema_patch_review_count = 1
    schema_patch_review_changed_count = 1
    schema_patch_reviews = @(
        [ordered]@{
            name = "office-notice-template"
            project_id = "project-office"
            template_name = "office-notice-template"
            candidate_type = "add"
            review_json = "office-notice.review.json"
            changed = $true
            baseline_slot_count = 1
            generated_slot_count = 2
            upsert_slot_count = 1
            remove_target_count = 0
            remove_slot_count = 0
            rename_slot_count = 0
            update_slot_count = 0
            inserted_slots = 1
            replaced_slots = 0
            confidence = 91
            reason_code = "business_document_type_missing"
        }
    )
    schema_patch_approval_items = @(
        [ordered]@{
            name = "office-notice-template"
            project_id = "project-office"
            template_name = "office-notice-template"
            candidate_type = "add"
            status = "approved"
            decision = "approved"
            approved = $true
            pending = $false
            confidence = 91
            reason_code = "business_document_type_missing"
            approval_result = "office-notice.approval.json"
            schema_update_candidate = "office-notice.schema.json"
            review_json = "office-notice.review.json"
            compliance_issue_count = 0
        }
    )
})

if (Test-Scenario -Name "aggregate") {
    $outputDir = Join-Path $resolvedWorkingDir "aggregate-report"
    $result = Invoke-CalibrationScript -Arguments @(
        "-InputRoot", $fixtureRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Calibration report should succeed without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "schema_patch_confidence_calibration.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Calibration report should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Calibration report should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Summary should expose calibration schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Invalid approval records should block the report."
    Assert-Equal -Actual ([int]$summary.entry_count) -Expected 5 `
        -Message "Summary should aggregate only real schema patch candidates from smoke and history."
    Assert-Equal -Actual (@($summary.entries | Where-Object { [string]$_.name -eq "unchanged-contract-template" }).Count) -Expected 0 `
        -Message "No-op schema baseline reviews should not become calibration candidates."
    Assert-Equal -Actual ([string]$summary.confidence_source) -Expected "mixed" `
        -Message "Summary should detect mixed explicit and unscored confidence."
    Assert-Equal -Actual ([int]$summary.recommended_min_confidence) -Expected 96 `
        -Message "Approved floor should become recommended min confidence."
    Assert-Equal -Actual ([int]$summary.project_count) -Expected 4 `
        -Message "Summary should expose the number of real business projects."
    Assert-Equal -Actual ([int]$summary.template_count) -Expected 5 `
        -Message "Summary should expose the number of calibrated business templates."
    Assert-HasProperty -Object $summary -Name "business_template_corpus_summary" `
        -Message "Summary should expose business template corpus traceability."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.entry_count) -Expected 5 `
        -Message "Corpus summary should count calibration entries."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.traced_entry_count) -Expected 5 `
        -Message "Corpus summary should count entries with complete project/template/source metadata."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.project_count) -Expected 4 `
        -Message "Corpus summary should expose business project coverage."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.template_count) -Expected 5 `
        -Message "Corpus summary should expose business template coverage."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.business_document_type_count) -Expected 4 `
        -Message "Corpus summary should count distinct business document types."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.corpus_role_count) -Expected 0 `
        -Message "Corpus summary should tolerate missing corpus roles in calibration entries."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.source_json_count) -Expected 2 `
        -Message "Corpus summary should expose distinct source JSON count."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.missing_source_metadata_count) -Expected 0 `
        -Message "Corpus summary should report missing source metadata count."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.missing_business_document_type_count) -Expected 0 `
        -Message "Corpus summary should report missing business document type metadata count."
    Assert-True -Condition (@($summary.business_template_corpus_summary.template_sources | Where-Object { $_.template_scope -eq "project-finance/invoice-template" }).Count -eq 1) `
        -Message "Corpus summary should route invoice fixture back to its project/template scope."
    Assert-True -Condition (@($summary.business_template_corpus_summary.business_document_types) -contains "invoice") `
        -Message "Corpus summary should preserve distinct business document types."
    Assert-Equal -Actual ([string](@($summary.entries | Where-Object { $_.name -eq "delivery-status-report-template" })[0].business_document_type)) -Expected "report" `
        -Message "History-derived entry should preserve business document type."

    $invoiceEntry = @($summary.entries | Where-Object { $_.name -eq "invoice-template" })[0]
    Assert-Equal -Actual ([string]$invoiceEntry.project_id) -Expected "project-finance" `
        -Message "Entries should preserve project id from business-template fixtures."
    Assert-Equal -Actual ([string]$invoiceEntry.template_name) -Expected "invoice-template" `
        -Message "Entries should preserve template name from business-template fixtures."
    Assert-Equal -Actual ([string]$invoiceEntry.business_document_type) -Expected "invoice" `
        -Message "Entries should preserve business document type from source entries."
    Assert-Equal -Actual ([string]$invoiceEntry.candidate_type) -Expected "rename" `
        -Message "Entries should preserve explicit candidate type."
    Assert-Equal -Actual ([int]$invoiceEntry.operation_summary.rename_count) -Expected 1 `
        -Message "Entries should expose operation summaries for reviewer triage."

    Assert-True -Condition (@($summary.candidate_type_summary | Where-Object { $_.candidate_type -eq "rename" }).Count -eq 1) `
        -Message "Candidate type summary should include rename candidates."
    Assert-True -Condition (@($summary.candidate_type_summary | Where-Object { $_.candidate_type -eq "type_update" }).Count -eq 1) `
        -Message "Candidate type summary should include type update candidates."
    Assert-True -Condition (@($summary.candidate_type_summary | Where-Object { $_.candidate_type -eq "remove" }).Count -eq 1) `
        -Message "Candidate type summary should include remove candidates."
    Assert-True -Condition (@($summary.candidate_type_summary | Where-Object { $_.candidate_type -eq "required_change" }).Count -eq 1) `
        -Message "Candidate type summary should include required change candidates."

    $highBucket = @($summary.confidence_buckets | Where-Object { $_.name -eq "95-100" })[0]
    Assert-Equal -Actual ([int]$highBucket.candidate_count) -Expected 1 `
        -Message "High confidence bucket should include the approved invoice candidate."
    Assert-Equal -Actual ([int]$highBucket.approved_count) -Expected 1 `
        -Message "High confidence bucket should count approved candidate."

    $mediumBucket = @($summary.confidence_buckets | Where-Object { $_.name -eq "80-94" })[0]
    Assert-Equal -Actual ([int]$mediumBucket.candidate_count) -Expected 1 `
        -Message "80-94 confidence bucket should include the rejected report candidate."
    Assert-Equal -Actual ([int]$mediumBucket.rejected_count) -Expected 1 `
        -Message "80-94 confidence bucket should count rejected candidate."

    $lowMidBucket = @($summary.confidence_buckets | Where-Object { $_.name -eq "60-79" })[0]
    Assert-Equal -Actual ([int]$lowMidBucket.candidate_count) -Expected 1 `
        -Message "60-79 confidence bucket should include the policy handbook candidate."
    Assert-Equal -Actual ([int]$lowMidBucket.rejected_count) -Expected 1 `
        -Message "60-79 confidence bucket should count needs-changes candidate."

    $lowBucket = @($summary.confidence_buckets | Where-Object { $_.name -eq "0-59" })[0]
    Assert-Equal -Actual ([int]$lowBucket.candidate_count) -Expected 1 `
        -Message "0-59 confidence bucket should include the invalid project report candidate."
    Assert-Equal -Actual ([int]$lowBucket.invalid_result_count) -Expected 1 `
        -Message "0-59 confidence bucket should count invalid approval candidate."

    $unscoredBucket = @($summary.confidence_buckets | Where-Object { $_.name -eq "unscored" })[0]
    Assert-Equal -Actual ([int]$unscoredBucket.pending_count) -Expected 1 `
        -Message "Unscored bucket should count pending candidate."
    Assert-True -Condition (@($summary.recommendations | Where-Object { $_.id -eq "resolve_pending_schema_approvals" }).Count -eq 1) `
        -Message "Recommendations should ask to resolve pending approvals."
    Assert-True -Condition (@($summary.recommendations | Where-Object { $_.id -eq "fix_invalid_approval_records" }).Count -eq 1) `
        -Message "Recommendations should ask to fix invalid approval records."
    Assert-True -Condition (@($summary.recommendations | Where-Object { $_.id -eq "add_explicit_confidence_metadata" }).Count -eq 1) `
        -Message "Recommendations should ask for explicit confidence metadata."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $false `
        -Message "Blocked calibration should not be release-ready."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
        -Message "Pending and invalid approvals should produce release blockers for rollup."
    $pendingBlockers = @($summary.release_blockers | Where-Object { $_.id -eq "schema_patch_confidence_calibration.pending_schema_approvals" })
    Assert-Equal -Actual $pendingBlockers.Count -Expected 1 `
        -Message "Pending approvals should produce a stable release blocker."
    Assert-Equal -Actual ([string]$pendingBlockers[0].project_id) -Expected "project-legal" `
        -Message "Pending blockers should preserve project id."
    Assert-Equal -Actual ([string]$pendingBlockers[0].template_name) -Expected "contract-template" `
        -Message "Pending blockers should preserve template name."
    Assert-Equal -Actual ([string]$pendingBlockers[0].candidate_type) -Expected "remove" `
        -Message "Pending blockers should preserve candidate type."
    $invalidBlockers = @($summary.release_blockers | Where-Object { $_.id -eq "schema_patch_confidence_calibration.invalid_approval_records" })
    Assert-Equal -Actual $invalidBlockers.Count -Expected 1 `
        -Message "Invalid approvals should produce a stable release blocker."
    Assert-Equal -Actual ([string]$invalidBlockers[0].action) -Expected "fix_invalid_approval_records" `
        -Message "Invalid approval blocker should route to the repair action."
    Assert-Equal -Actual ([string]$invalidBlockers[0].project_id) -Expected "project-delivery" `
        -Message "Invalid approval blockers should preserve project id."
    Assert-Equal -Actual ([string]$invalidBlockers[0].template_name) -Expected "project-report-template" `
        -Message "Invalid approval blockers should preserve template name."
    Assert-Equal -Actual ([string]$invalidBlockers[0].candidate_type) -Expected "required_change" `
        -Message "Invalid approval blockers should preserve candidate type."
    Assert-Equal -Actual ([string]$summary.release_blockers[0].source_schema) -Expected "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Calibration blockers should expose the source schema."
    Assert-ContainsText -Text ([string]$summary.release_blockers[0].source_json_display) -ExpectedText "aggregate-report\summary.json" `
        -Message "Calibration blockers should expose the report JSON display path."
    Assert-ContainsText -Text ([string]$summary.release_blockers[0].source_report) -ExpectedText "aggregate-report\summary.json" `
        -Message "Calibration blockers should expose the report source path."
    Assert-ContainsText -Text ([string]$summary.release_blockers[0].source_report_display) -ExpectedText "aggregate-report\summary.json" `
        -Message "Calibration blockers should expose the report display path."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Unscored candidates should produce the only warning for rollup."
    Assert-Equal -Actual ([string]$summary.warnings[0].id) -Expected "schema_patch_confidence_calibration.unscored_candidates" `
        -Message "Invalid approvals should be blockers rather than warnings."
    Assert-Equal -Actual ([string]$summary.warnings[0].source_schema) -Expected "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Calibration warnings should expose the source schema."
    Assert-Equal -Actual ([string]$summary.warnings[0].project_id) -Expected "project-legal" `
        -Message "Unscored warnings should preserve project id."
    Assert-Equal -Actual ([string]$summary.warnings[0].template_name) -Expected "contract-template" `
        -Message "Unscored warnings should preserve template name."
    Assert-Equal -Actual ([string]$summary.warnings[0].candidate_type) -Expected "remove" `
        -Message "Unscored warnings should preserve candidate type."
    Assert-ContainsText -Text ([string]$summary.warnings[0].source_report_display) -ExpectedText "aggregate-report\summary.json" `
        -Message "Calibration warnings should expose the report display path."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 4 `
        -Message "Recommendations should be mirrored as action items."
    Assert-True -Condition (@($summary.action_items | Where-Object { $_.id -eq "fix_invalid_approval_records" }).Count -eq 1) `
        -Message "Invalid approval recommendation should be mirrored as an action item."
    $pendingAction = @($summary.action_items | Where-Object { $_.id -eq "resolve_pending_schema_approvals" })[0]
    Assert-Equal -Actual ([string]$pendingAction.project_id) -Expected "project-legal" `
        -Message "Pending action item should preserve project id."
    Assert-Equal -Actual ([string]$pendingAction.template_name) -Expected "contract-template" `
        -Message "Pending action item should preserve template name."
    Assert-Equal -Actual ([string]$pendingAction.candidate_type) -Expected "remove" `
        -Message "Pending action item should preserve candidate type."
    Assert-ContainsText -Text ([string]$pendingAction.source_report_display) -ExpectedText "aggregate-report\summary.json" `
        -Message "Calibration action items should expose the report display path."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
        -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
        -Message "Calibration action items should expose the reviewer open command."
    Assert-GovernanceTraceMetadata -Items @($summary.release_blockers) -CollectionName "release_blockers"
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "warnings"
    Assert-GovernanceTraceMetadata -Items @($summary.action_items) -CollectionName "action_items" `
        -ExpectOpenCommandProperty $true

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Schema Patch Confidence Calibration Report" `
        -Message "Markdown should include title."
    Assert-ContainsText -Text $markdown -ExpectedText "Business Template Corpus" `
        -Message "Markdown should include business template corpus summary."
    Assert-ContainsText -Text $markdown -ExpectedText "business_document_types=" `
        -Message "Markdown should include business document type counts."
    Assert-ContainsText -Text $markdown -ExpectedText "source_jsons=2" `
        -Message "Markdown should include source JSON coverage."
    Assert-ContainsText -Text $markdown -ExpectedText "Confidence Buckets" `
        -Message "Markdown should include confidence buckets."
    Assert-ContainsText -Text $markdown -ExpectedText "Project Templates" `
        -Message "Markdown should include project template summaries."
    Assert-ContainsText -Text $markdown -ExpectedText "Candidate Types" `
        -Message "Markdown should include candidate type summaries."
    Assert-ContainsText -Text $markdown -ExpectedText "resolve_pending_schema_approvals" `
        -Message "Markdown should include pending approval recommendation."
    Assert-ContainsText -Text $markdown -ExpectedText 'project=`project-legal` template=`contract-template` candidate=`remove`' `
        -Message "Markdown should include project/template/candidate routing details."
    Assert-ContainsText -Text $markdown -ExpectedText "schema_patch_confidence_calibration.invalid_approval_records" `
        -Message "Markdown should include invalid approval blocker."
    Assert-ContainsText -Text $markdown -ExpectedText "fix_invalid_approval_records" `
        -Message "Markdown should include invalid approval action."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display=" `
        -Message "Markdown should include source JSON display fields."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report_display=" `
        -Message "Markdown should include source report display fields."
    Assert-ContainsText -Text $markdown -ExpectedText "open_command:" `
        -Message "Markdown should include action item open commands."
}

if (Test-Scenario -Name "source_metadata") {
    $outputDir = Join-Path $resolvedWorkingDir "source-metadata-report"
    $result = Invoke-CalibrationScript -Arguments @(
        "-InputJson", $missingSourceSummaryPath,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Calibration report should not fail only because source metadata is incomplete. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "schema_patch_confidence_calibration.md"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Missing source metadata should be a warning, not a release blocker."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Missing source metadata alone should not fail release readiness."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.entry_count) -Expected 1 `
        -Message "Source metadata scenario should expose corpus entry count."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.traced_entry_count) -Expected 0 `
        -Message "Missing project id should make the entry untraced."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.missing_source_metadata_count) -Expected 1 `
        -Message "Corpus summary should count missing source metadata."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.missing_project_id_count) -Expected 1 `
        -Message "Corpus summary should count missing project ids."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.missing_business_document_type_count) -Expected 0 `
        -Message "Review/approval-level business document type should satisfy business dimension traceability."
    Assert-Equal -Actual ([string]$summary.entries[0].business_document_type) -Expected "notice" `
        -Message "Entries should preserve business document type from review or approval metadata."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Missing source metadata should produce one warning."
    Assert-Equal -Actual ([string]$summary.warnings[0].id) -Expected "schema_patch_confidence_calibration.missing_business_template_source_metadata" `
        -Message "Missing source metadata warning should use a stable id."
    Assert-Equal -Actual ([string]$summary.warnings[0].action) -Expected "add_business_template_source_metadata" `
        -Message "Missing source metadata warning should route to repair action."
    Assert-Equal -Actual ([bool]$summary.warnings[0].missing_project_id) -Expected $true `
        -Message "Missing source metadata warning should expose the missing project id flag."
    Assert-True -Condition (@($summary.action_items | Where-Object { $_.id -eq "add_business_template_source_metadata" }).Count -eq 1) `
        -Message "Missing source metadata recommendation should be mirrored as an action item."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Business Template Corpus" `
        -Message "Markdown should include business template corpus section for missing source scenario."
    Assert-ContainsText -Text $markdown -ExpectedText "missing_source_metadata=1" `
        -Message "Markdown should expose missing source metadata count."
    Assert-ContainsText -Text $markdown -ExpectedText "add_business_template_source_metadata" `
        -Message "Markdown should include missing source metadata action."
}

if (Test-Scenario -Name "business_dimension_metadata") {
    $outputDir = Join-Path $resolvedWorkingDir "business-dimension-metadata-report"
    $result = Invoke-CalibrationScript -Arguments @(
        "-InputJson", $missingBusinessDocumentTypeSummaryPath,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Calibration report should not fail only because business document type metadata is incomplete. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "schema_patch_confidence_calibration.md"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Missing business document type metadata should be a warning, not a release blocker."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Missing business document type metadata alone should not fail release readiness."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.entry_count) -Expected 1 `
        -Message "Business dimension scenario should expose corpus entry count."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.traced_entry_count) -Expected 1 `
        -Message "Business dimension scenario should keep source identity traced."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.missing_source_metadata_count) -Expected 0 `
        -Message "Business dimension warning should not be confused with missing source identity."
    Assert-Equal -Actual ([int]$summary.business_template_corpus_summary.missing_business_document_type_count) -Expected 1 `
        -Message "Corpus summary should count missing business document type metadata."
    Assert-Equal -Actual (@($summary.business_template_corpus_summary.missing_business_document_type_entries).Count) -Expected 1 `
        -Message "Corpus summary should keep missing business document type entry details."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Missing business document type metadata should produce one warning."
    Assert-Equal -Actual ([string]$summary.warnings[0].id) -Expected "schema_patch_confidence_calibration.missing_business_document_type_metadata" `
        -Message "Missing business document type warning should use a stable id."
    Assert-Equal -Actual ([string]$summary.warnings[0].action) -Expected "add_business_template_document_type_metadata" `
        -Message "Missing business document type warning should route to repair action."
    Assert-Equal -Actual ([int]$summary.warnings[0].missing_business_document_type_count) -Expected 1 `
        -Message "Missing business document type warning should expose the affected count."
    Assert-True -Condition (@($summary.action_items | Where-Object { $_.id -eq "add_business_template_document_type_metadata" }).Count -eq 1) `
        -Message "Missing business document type recommendation should be mirrored as an action item."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "missing_business_document_types=1" `
        -Message "Markdown should expose missing business document type count."
    Assert-ContainsText -Text $markdown -ExpectedText "missing_business_document_type_entries:" `
        -Message "Markdown should expose missing business document type entry details."
    Assert-ContainsText -Text $markdown -ExpectedText "add_business_template_document_type_metadata" `
        -Message "Markdown should include missing business document type action."
}

if (Test-Scenario -Name "fail_on_pending") {
    $outputDir = Join-Path $resolvedWorkingDir "fail-on-pending-report"
    $result = Invoke-CalibrationScript -Arguments @(
        "-InputRoot", $fixtureRoot,
        "-OutputDir", $outputDir,
        "-FailOnPending"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Calibration report should fail with -FailOnPending when pending entries exist. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Failing calibration report should still write summary.json."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Failing summary should preserve blocked status when invalid approvals exist."
}

Write-Host "Schema patch confidence calibration report regression passed."
