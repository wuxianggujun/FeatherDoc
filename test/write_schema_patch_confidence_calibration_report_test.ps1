param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "fail_on_pending")]
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

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$fixtureRoot = Join-Path $resolvedWorkingDir "fixtures"
$smokeSummaryPath = Join-Path $fixtureRoot "smoke\summary.json"
$historyPath = Join-Path $fixtureRoot "history\project_template_schema_approval_history.json"

Write-JsonFile -Path $smokeSummaryPath -Value ([ordered]@{
    schema_patch_review_count = 4
    schema_patch_review_changed_count = 4
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
        -Message "Summary should aggregate entries from smoke and history."
    Assert-Equal -Actual ([string]$summary.confidence_source) -Expected "mixed" `
        -Message "Summary should detect mixed explicit and unscored confidence."
    Assert-Equal -Actual ([int]$summary.recommended_min_confidence) -Expected 96 `
        -Message "Approved floor should become recommended min confidence."
    Assert-Equal -Actual ([int]$summary.project_count) -Expected 4 `
        -Message "Summary should expose the number of real business projects."
    Assert-Equal -Actual ([int]$summary.template_count) -Expected 5 `
        -Message "Summary should expose the number of calibrated business templates."

    $invoiceEntry = @($summary.entries | Where-Object { $_.name -eq "invoice-template" })[0]
    Assert-Equal -Actual ([string]$invoiceEntry.project_id) -Expected "project-finance" `
        -Message "Entries should preserve project id from business-template fixtures."
    Assert-Equal -Actual ([string]$invoiceEntry.template_name) -Expected "invoice-template" `
        -Message "Entries should preserve template name from business-template fixtures."
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
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
        -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
        -Message "Calibration action items should expose the reviewer open command."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Schema Patch Confidence Calibration Report" `
        -Message "Markdown should include title."
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
    Assert-ContainsText -Text $markdown -ExpectedText "open_command:" `
        -Message "Markdown should include action item open commands."
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
