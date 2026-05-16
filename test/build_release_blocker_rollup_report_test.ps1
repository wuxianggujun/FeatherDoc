param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "passing", "style_merge_warning", "style_merge_review", "comma_input", "empty", "malformed", "dedupe")]
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

function Invoke-RollupScript {
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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_blocker_rollup_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$fixtureRoot = Join-Path $resolvedWorkingDir "fixtures"
$documentSkeletonPath = Join-Path $fixtureRoot "document-skeleton\document_skeleton_governance.summary.json"
$documentSkeletonRollupPath = Join-Path $fixtureRoot "document-skeleton-rollup\summary.json"
$documentSkeletonReviewedRollupPath = Join-Path $fixtureRoot "document-skeleton-reviewed-rollup\summary.json"
$styleMergeRestoreAuditPath = Join-Path $fixtureRoot "style-merge-restore-audit\summary.json"
$tableLayoutPath = Join-Path $fixtureRoot "table-layout\summary.json"
$releaseCandidatePath = Join-Path $fixtureRoot "release-candidate\summary.json"
$emptyPath = Join-Path $fixtureRoot "empty\summary.json"
$malformedPath = Join-Path $fixtureRoot "malformed\summary.json"
$dedupePath = Join-Path $fixtureRoot "dedupe\summary.json"

Write-JsonFile -Path $documentSkeletonPath -Value ([ordered]@{
    schema = "featherdoc.document_skeleton_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "document_skeleton.style_numbering_issues"
            severity = "error"
            message = "Style numbering audit reported issues."
            action = "review_style_numbering_audit"
        }
    )
    action_items = @(
        [ordered]@{
            id = "preview_style_numbering_repair"
            action = "preview_style_numbering_repair"
            title = "Preview style numbering repair"
            command = "featherdoc_cli repair-style-numbering input.docx --plan-only --json"
        }
    )
})

Write-JsonFile -Path $documentSkeletonRollupPath -Value ([ordered]@{
    schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
    status = "needs_review"
    document_count = 2
    total_style_numbering_issue_count = 0
    total_style_numbering_suggestion_count = 0
    total_style_merge_suggestion_count = 2
    total_style_merge_suggestion_pending_count = 2
    release_blocker_count = 0
    release_blockers = @()
    action_items = @(
        [ordered]@{
            id = "review_style_merge_suggestions"
            action = "review_style_merge_suggestions"
            title = "Review duplicate style merge suggestions"
            command = "featherdoc_cli suggest-style-merges input.docx --confidence-profile recommended --json"
        }
    )
})

Write-JsonFile -Path $documentSkeletonReviewedRollupPath -Value ([ordered]@{
    schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
    status = "clean"
    document_count = 1
    total_style_numbering_issue_count = 0
    total_style_numbering_suggestion_count = 0
    total_style_merge_suggestion_count = 2
    total_style_merge_suggestion_pending_count = 0
    release_blocker_count = 0
    release_blockers = @()
    action_items = @()
    warnings = @()
})

Write-JsonFile -Path $tableLayoutPath -Value ([ordered]@{
    schema = "featherdoc.table_layout_delivery_report.v1"
    release_blocker_count = 2
    release_blockers = @(
        [ordered]@{
            id = "table_layout.manual_table_style_quality_work"
            severity = "warning"
            message = "Manual table style work remains."
            action = "review_table_style_quality_plan"
        },
        [ordered]@{
            id = "table_layout.positioned_tables_need_review"
            severity = "warning"
            message = "Existing floating positions need review."
            action = "review_table_position_plan"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_table_position_preset"
            action = "review_table_position_preset"
            title = "Review table position preset"
            command = "featherdoc_cli apply-table-position-plan plan.json --dry-run --json"
        }
    )
})

Write-JsonFile -Path $releaseCandidatePath -Value ([ordered]@{
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_smoke.schema_approval"
            severity = "error"
            status = "blocked"
            message = "Schema approval blocks release."
            action = "fix_schema_patch_approval_result"
        }
    )
    action_items = @()
})

Write-JsonFile -Path $styleMergeRestoreAuditPath -Value ([ordered]@{
    schema = "featherdoc.style_merge_restore_audit.v1"
    status = "needs_review"
    issue_count = 1
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "style_merge.restore_audit_issues"
            severity = "error"
            status = "blocked"
            message = "Style merge restore dry-run reported 1 issue(s)."
            action = "review_style_merge_restore_audit"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_style_merge_restore_audit"
            action = "review_style_merge_restore_audit"
            title = "Review style merge restore audit and Word render"
            command = "pwsh -ExecutionPolicy Bypass -File .\scripts\prepare_word_review_task.ps1 -DocxPath output/document-skeleton-governance/merged-styles.docx -Mode review-only"
        }
    )
})

Write-JsonFile -Path $emptyPath -Value ([ordered]@{
    schema = "featherdoc.empty_report.v1"
    release_blocker_count = 0
    release_blockers = @()
    action_items = @()
})

Write-JsonFile -Path $malformedPath -Value ([ordered]@{
    schema = "featherdoc.malformed_report.v1"
    release_blocker_count = 3
    release_blockers = @(
        [ordered]@{
            id = "malformed.actual_blocker"
            severity = "error"
            message = "Only one blocker is present."
            action = "fix_malformed_fixture"
        }
    )
    action_items = @()
})

Write-JsonFile -Path $dedupePath -Value ([ordered]@{
    schema = "featherdoc.dedupe_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_smoke.schema_approval"
            severity = "error"
            status = "blocked"
            message = "A second report has the same id."
            action = "fix_schema_patch_approval_result"
        }
    )
    action_items = @(
        [ordered]@{
            id = "same_action"
            action = "fix_schema_patch_approval_result"
            title = "Fix schema approval"
        }
    )
})

if (Test-Scenario -Name "passing") {
    $outputDir = Join-Path $resolvedWorkingDir "passing-report"
    $passingInputRoot = Join-Path $resolvedWorkingDir "passing-input"
    Write-JsonFile -Path (Join-Path $passingInputRoot "document-skeleton\document_skeleton_governance.summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $documentSkeletonPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "document-skeleton-rollup\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $documentSkeletonRollupPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "table-layout\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $tableLayoutPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "release-candidate\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseCandidatePath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "style-merge-restore-audit\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $styleMergeRestoreAuditPath | ConvertFrom-Json)
    $result = Invoke-RollupScript -Arguments @(
        "-InputRoot", $passingInputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Rollup should succeed without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "release_blocker_rollup.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Rollup should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Rollup should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Summary should expose rollup schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Rollup should be blocked when blockers exist."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 5 `
        -Message "Rollup should aggregate all blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 4 `
        -Message "Rollup should aggregate action items."
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 5 `
        -Message "Rollup should keep source report count."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Rollup should surface non-blocking skeleton review warnings."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
        -Message "Rollup should warn about pending style merge suggestion review."
    $styleMergeWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "document_skeleton.style_merge_suggestions_pending" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$styleMergeWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Rollup should preserve warning source schema from source reports."
    Assert-Equal -Actual ([int]$styleMergeWarning[0].style_merge_suggestion_count) -Expected 2 `
        -Message "Rollup should preserve warning style merge counts from source reports."
    Assert-Equal -Actual ([int]$styleMergeWarning[0].style_merge_suggestion_pending_count) -Expected 2 `
        -Message "Rollup should preserve warning pending style merge counts from source reports."
    Assert-ContainsText -Text (($summary.source_reports | ForEach-Object { "$($_.schema):$($_.style_merge_suggestion_count)" }) -join "`n") `
        -ExpectedText "featherdoc.document_skeleton_governance_rollup_report.v1:2" `
        -Message "Rollup should preserve skeleton rollup style merge counts."
    Assert-ContainsText -Text (($summary.source_reports | ForEach-Object { "$($_.schema):$($_.style_merge_suggestion_pending_count)" }) -join "`n") `
        -ExpectedText "featherdoc.document_skeleton_governance_rollup_report.v1:2" `
        -Message "Rollup should preserve skeleton rollup pending style merge counts."
    Assert-ContainsText -Text ([string]$summary.release_blockers[0].composite_id) `
        -ExpectedText "source1.blocker1" `
        -Message "Rollup should generate composite blocker ids."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blocker Rollup Report" `
        -Message "Markdown should include title."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_smoke.schema_approval" `
        -Message "Markdown should include release candidate blocker."
    Assert-ContainsText -Text $markdown -ExpectedText "style_merge.restore_audit_issues" `
        -Message "Markdown should include style merge restore audit blockers."
    Assert-ContainsText -Text $markdown -ExpectedText "review_style_merge_restore_audit" `
        -Message "Markdown should include style merge restore audit actions."
    Assert-ContainsText -Text $markdown -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
        -Message "Markdown should include style merge review warning."
    Assert-ContainsText -Text $markdown -ExpectedText "### Release blocker rollup warnings" `
        -Message "Markdown should include the release blocker rollup warning subsection."
    Assert-ContainsText -Text $markdown -ExpectedText '- warning_count: `1`' `
        -Message "Markdown should show the rollup warning count."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `review_style_merge_suggestions`' `
        -Message "Markdown should include warning actions."
    Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.document_skeleton_governance_rollup_report.v1`' `
        -Message "Markdown should include warning source schema."
    Assert-ContainsText -Text $markdown -ExpectedText 'style_merge_suggestion_count: `2`' `
        -Message "Markdown should include warning style merge suggestion counts."
    Assert-ContainsText -Text $markdown -ExpectedText 'style_merge_suggestion_pending_count: `2`' `
        -Message "Markdown should include warning pending style merge suggestion counts."
}

if (Test-Scenario -Name "empty") {
    $outputDir = Join-Path $resolvedWorkingDir "empty-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $emptyPath,
        "-OutputDir", $outputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Empty rollup should pass fail-on-blocker. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Empty rollup should be ready."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Empty rollup should be release-ready."
}

if (Test-Scenario -Name "style_merge_warning") {
    $outputDir = Join-Path $resolvedWorkingDir "style-merge-warning-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $documentSkeletonRollupPath,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Style merge warnings should not fail by default. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready_with_warnings" `
        -Message "Style merge suggestions should produce a non-blocking warning status."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Style merge warnings should keep release_ready=true by default."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Style merge suggestions should not be converted into release blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 1 `
        -Message "Style merge suggestions should preserve review action items."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Style merge suggestions should produce exactly one warning."

    $gatedOutputDir = Join-Path $resolvedWorkingDir "style-merge-warning-gated-report"
    $gatedResult = Invoke-RollupScript -Arguments @(
        "-InputJson", $documentSkeletonRollupPath,
        "-OutputDir", $gatedOutputDir,
        "-FailOnWarning"
    )
    Assert-Equal -Actual $gatedResult.ExitCode -Expected 1 `
        -Message "Style merge warnings should fail when -FailOnWarning is set. Output: $($gatedResult.Text)"
}

if (Test-Scenario -Name "style_merge_review") {
    $outputDir = Join-Path $resolvedWorkingDir "style-merge-review-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $documentSkeletonReviewedRollupPath,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Reviewed style merge suggestions should not warn. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Reviewed style merge suggestions should keep the rollup ready."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Reviewed style merge suggestions should remain release-ready."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 0 `
        -Message "Reviewed style merge suggestions should not produce release warnings."
    Assert-Equal -Actual ([int]$summary.source_reports[0].style_merge_suggestion_count) -Expected 2 `
        -Message "Rollup should preserve reviewed source suggestion totals."
    Assert-Equal -Actual ([int]$summary.source_reports[0].style_merge_suggestion_pending_count) -Expected 0 `
        -Message "Rollup should preserve reviewed source pending suggestion totals."
}

if (Test-Scenario -Name "comma_input") {
    $outputDir = Join-Path $resolvedWorkingDir "comma-input-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", "$documentSkeletonPath,$tableLayoutPath,$releaseCandidatePath",
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Rollup should accept comma-separated input JSON paths. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 3 `
        -Message "Comma-separated input should keep all three source reports."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 4 `
        -Message "Comma-separated input should aggregate all blockers."
    Assert-ContainsText -Text (($summary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
        -ExpectedText "release-candidate" `
        -Message "Comma-separated input should include the final source path."
}

if (Test-Scenario -Name "malformed") {
    $outputDir = Join-Path $resolvedWorkingDir "malformed-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $malformedPath,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Malformed count should warn but not fail by default. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Malformed count should produce one warning."
    Assert-Equal -Actual ([string]$summary.warnings[0].action) -Expected "reconcile_release_blocker_rollup_counts" `
        -Message "Mismatch warning should expose a remediation action."
    Assert-Equal -Actual ([string]$summary.warnings[0].source_schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Mismatch warning should expose the rollup source schema."
    Assert-ContainsText -Text ([string]$summary.warnings[0].message) -ExpectedText "release_blocker_count is 3" `
        -Message "Warning should explain count mismatch."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_blocker_rollup.md")
    Assert-ContainsText -Text $markdown -ExpectedText "### Release blocker rollup warnings" `
        -Message "Markdown should include the rollup warning subsection for malformed counts."
    Assert-ContainsText -Text $markdown -ExpectedText '- warning_count: `1`' `
        -Message "Markdown should show the malformed warning count."
    Assert-ContainsText -Text $markdown -ExpectedText 'id: `release_blocker_count_mismatch`' `
        -Message "Markdown should include the mismatch warning id."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `reconcile_release_blocker_rollup_counts`' `
        -Message "Markdown should include the mismatch warning action."
    Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.release_blocker_rollup_report.v1`' `
        -Message "Markdown should include the mismatch warning source schema."
}

if (Test-Scenario -Name "dedupe") {
    $outputDir = Join-Path $resolvedWorkingDir "dedupe-report"
    $dedupeInputRoot = Join-Path $resolvedWorkingDir "dedupe-input"
    Write-JsonFile -Path (Join-Path $dedupeInputRoot "release-candidate\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseCandidatePath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $dedupeInputRoot "dedupe\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $dedupePath | ConvertFrom-Json)
    $result = Invoke-RollupScript -Arguments @(
        "-InputRoot", $dedupeInputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Duplicate ids across reports should remain traceable. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
        -Message "Duplicate blocker ids from different reports should both be retained."
    Assert-Equal -Actual ([int]$summary.blocker_id_summary[0].count) -Expected 2 `
        -Message "Blocker id summary should count duplicates."
    Assert-True -Condition ([string]$summary.release_blockers[0].composite_id -ne [string]$summary.release_blockers[1].composite_id) `
        -Message "Composite ids should keep duplicate blockers distinct."
}

Write-Host "Release blocker rollup report regression passed."
