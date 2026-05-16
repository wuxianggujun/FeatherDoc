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
    schema_patch_review_count = 2
    schema_patch_review_changed_count = 2
    schema_patch_reviews = @(
        [ordered]@{
            name = "invoice-template"
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
        }
    )
    schema_patch_approval_items = @(
        [ordered]@{
            name = "invoice-template"
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
            status = "pending_review"
            decision = "pending"
            approved = $false
            pending = $true
            approval_result = "contract.approval.json"
            schema_update_candidate = "contract.schema.json"
            review_json = "contract.review.json"
            compliance_issue_count = 0
        }
    )
})

Write-JsonFile -Path $historyPath -Value ([ordered]@{
    schema = "featherdoc.project_template_schema_approval_history.v1"
    entry_histories = @(
        [ordered]@{
            name = "report-template"
            runs = @(
                [ordered]@{
                    schema_patch_review_count = 1
                    schema_patch_review_changed_count = 1
                    schema_patch_reviews = @(
                        [ordered]@{
                            name = "report-template"
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
                            name = "report-template"
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
    Assert-Equal -Actual ([string]$summary.status) -Expected "pending_review" `
        -Message "Pending approval should make the report pending_review."
    Assert-Equal -Actual ([int]$summary.entry_count) -Expected 3 `
        -Message "Summary should aggregate entries from smoke and history."
    Assert-Equal -Actual ([string]$summary.confidence_source) -Expected "mixed" `
        -Message "Summary should detect mixed explicit and unscored confidence."
    Assert-Equal -Actual ([int]$summary.recommended_min_confidence) -Expected 96 `
        -Message "Approved floor should become recommended min confidence."

    $highBucket = @($summary.confidence_buckets | Where-Object { $_.name -eq "95-100" })[0]
    Assert-Equal -Actual ([int]$highBucket.candidate_count) -Expected 1 `
        -Message "High confidence bucket should include the approved invoice candidate."
    Assert-Equal -Actual ([int]$highBucket.approved_count) -Expected 1 `
        -Message "High confidence bucket should count approved candidate."

    $unscoredBucket = @($summary.confidence_buckets | Where-Object { $_.name -eq "unscored" })[0]
    Assert-Equal -Actual ([int]$unscoredBucket.pending_count) -Expected 1 `
        -Message "Unscored bucket should count pending candidate."
    Assert-True -Condition (@($summary.recommendations | Where-Object { $_.id -eq "resolve_pending_schema_approvals" }).Count -eq 1) `
        -Message "Recommendations should ask to resolve pending approvals."
    Assert-True -Condition (@($summary.recommendations | Where-Object { $_.id -eq "add_explicit_confidence_metadata" }).Count -eq 1) `
        -Message "Recommendations should ask for explicit confidence metadata."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $false `
        -Message "Pending calibration should not be release-ready."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 1 `
        -Message "Pending approvals should produce a release blocker for rollup."
    Assert-Equal -Actual ([string]$summary.release_blockers[0].source_schema) -Expected "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Calibration blockers should expose the source schema."
    Assert-ContainsText -Text ([string]$summary.release_blockers[0].source_json_display) -ExpectedText "aggregate-report\summary.json" `
        -Message "Calibration blockers should expose the report JSON display path."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Unscored candidates should produce a warning for rollup."
    Assert-Equal -Actual ([string]$summary.warnings[0].source_schema) -Expected "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Calibration warnings should expose the source schema."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 3 `
        -Message "Recommendations should be mirrored as action items."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
        -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
        -Message "Calibration action items should expose the reviewer open command."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Schema Patch Confidence Calibration Report" `
        -Message "Markdown should include title."
    Assert-ContainsText -Text $markdown -ExpectedText "Confidence Buckets" `
        -Message "Markdown should include confidence buckets."
    Assert-ContainsText -Text $markdown -ExpectedText "resolve_pending_schema_approvals" `
        -Message "Markdown should include pending approval recommendation."
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
    Assert-Equal -Actual ([string]$summary.status) -Expected "pending_review" `
        -Message "Failing summary should preserve pending_review status."
}

Write-Host "Schema patch confidence calibration report regression passed."
