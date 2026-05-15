param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "empty", "failed_source_report", "malformed", "fail_on_issue", "fail_on_blocker")]
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
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-LayoutSummary {
    param(
        [string]$InputDocx,
        [string]$Preset = "paragraph-callout",
        [string]$PlanPath,
        [string]$Status = "ready",
        [int]$IssueCount = 0,
        [int]$AutomaticFixCount = 0,
        [int]$ManualFixCount = 0,
        [int]$PositionAutomaticCount = 0,
        [int]$PositionReviewCount = 0,
        [int]$PositionAlreadyMatchingCount = 0,
        [object[]]$Issues = @(),
        [object[]]$ReleaseBlockers = @(),
        [object[]]$ActionItems = @()
    )

    return [ordered]@{
        schema = "featherdoc.table_layout_delivery_report.v1"
        generated_at = "2026-05-03T00:00:00"
        status = $Status
        ready = ($Status -eq "ready")
        input_docx = $InputDocx
        output_dir = "output/table-layout-delivery"
        summary_json = "output/table-layout-delivery/summary.json"
        report_markdown = "output/table-layout-delivery/table_layout_delivery_report.md"
        preset = $Preset
        table_style_quality_audit = [ordered]@{
            command = "audit-table-style-quality"
            issue_count = $IssueCount
            issues = @($Issues)
        }
        table_style_quality_fix_plan = [ordered]@{
            command = "plan-table-style-quality-fixes"
            automatic_fix_count = $AutomaticFixCount
            manual_fix_count = $ManualFixCount
        }
        table_position_plan = [ordered]@{
            command = "plan-table-position-presets"
            preset = $Preset
            automatic_count = $PositionAutomaticCount
            review_count = $PositionReviewCount
            already_matching_count = $PositionAlreadyMatchingCount
        }
        table_position_plan_path = $PlanPath
        positioned_output_docx = "output/table-layout-delivery/positioned.docx"
        table_style_issue_count = $IssueCount
        automatic_tblLook_fix_count = $AutomaticFixCount
        manual_table_style_fix_count = $ManualFixCount
        table_position_automatic_count = $PositionAutomaticCount
        table_position_review_count = $PositionReviewCount
        table_position_already_matching_count = $PositionAlreadyMatchingCount
        release_blockers = @($ReleaseBlockers)
        release_blocker_count = @($ReleaseBlockers).Count
        action_items = @($ActionItems)
        commands = @()
        command_failure_count = 0
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_table_layout_delivery_rollup_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "aggregate") {
    $aggregateRoot = Join-Path $resolvedWorkingDir "aggregate-input"
    $aggregateOutputDir = Join-Path $resolvedWorkingDir "aggregate-output"
    $readyReportPath = Join-Path $aggregateRoot "invoice\summary.json"
    $reviewReportPath = Join-Path $aggregateRoot "contract\table_layout_delivery.summary.json"

    Write-JsonFile -Path $readyReportPath -Value (New-LayoutSummary `
        -InputDocx "samples/invoice.docx" `
        -Preset "paragraph-callout" `
        -PlanPath "output/table-layout-delivery/invoice/table-position-paragraph-callout.plan.json" `
        -Status "ready" `
        -PositionAlreadyMatchingCount 1 `
        -ActionItems @(
            [ordered]@{
                id = "review_table_position_preset"
                title = "Review invoice table position preset"
                command = "featherdoc_cli apply-table-position-plan invoice.plan.json --dry-run --json"
            }
        ))

    Write-JsonFile -Path $reviewReportPath -Value (New-LayoutSummary `
        -InputDocx "samples/contract.docx" `
        -Preset "margin-anchor" `
        -PlanPath "output/table-layout-delivery/contract/table-position-margin-anchor.plan.json" `
        -Status "needs_review" `
        -IssueCount 3 `
        -AutomaticFixCount 2 `
        -ManualFixCount 1 `
        -PositionAutomaticCount 2 `
        -PositionReviewCount 1 `
        -Issues @(
            [ordered]@{ kind = "missing_region" },
            [ordered]@{ kind = "bad_tblLook" },
            [ordered]@{ kind = "bad_tblLook" }
        ) `
        -ReleaseBlockers @(
            [ordered]@{
                id = "table_layout.manual_table_style_quality_work"
                severity = "warning"
                message = "Table style quality has issues that need manual style-definition work."
                action = "review_table_style_quality_plan"
                issue_count = 3
                manual_fix_count = 1
            },
            [ordered]@{
                id = "table_layout.positioned_tables_need_review"
                severity = "warning"
                message = "Some existing floating table positions need manual review before applying the preset."
                action = "review_table_position_plan"
                review_count = 1
            }
        ) `
        -ActionItems @(
            [ordered]@{
                id = "apply_safe_tblLook_fixes"
                title = "Apply safe tblLook-only fixes"
                command = "featherdoc_cli apply-table-style-quality-fixes contract.docx --look-only --json"
            },
            [ordered]@{
                id = "run_table_style_quality_visual_regression"
                title = "Run table visual regression"
                command = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_table_style_quality_visual_regression.ps1"
            }
        ))

    $aggregateResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $aggregateRoot,
        "-OutputDir", $aggregateOutputDir
    )
    Assert-Equal -Actual $aggregateResult.ExitCode -Expected 0 `
        -Message "Aggregate layout rollup should pass without fail switches. Output: $($aggregateResult.Text)"
    Assert-ContainsText -Text $aggregateResult.Text -ExpectedText "Summary JSON:" `
        -Message "Aggregate layout rollup should print summary path."

    $summaryPath = Join-Path $aggregateOutputDir "summary.json"
    $markdownPath = Join-Path $aggregateOutputDir "table_layout_delivery_rollup.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Aggregate layout rollup should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Aggregate layout rollup should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.table_layout_delivery_rollup_report.v1" `
        -Message "Aggregate layout rollup should use the layout rollup schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Aggregate layout rollup should require review when one source has layout work."
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 2 `
        -Message "Aggregate layout rollup should count source reports."
    Assert-Equal -Actual ([int]$summary.document_count) -Expected 2 `
        -Message "Aggregate layout rollup should count documents."
    Assert-Equal -Actual ([int]$summary.total_table_style_issue_count) -Expected 3 `
        -Message "Aggregate layout rollup should sum table style issues."
    Assert-Equal -Actual ([int]$summary.total_automatic_tblLook_fix_count) -Expected 2 `
        -Message "Aggregate layout rollup should sum automatic tblLook fixes."
    Assert-Equal -Actual ([int]$summary.total_manual_table_style_fix_count) -Expected 1 `
        -Message "Aggregate layout rollup should sum manual table style fixes."
    Assert-Equal -Actual ([int]$summary.total_table_position_automatic_count) -Expected 2 `
        -Message "Aggregate layout rollup should sum automatic floating table plans."
    Assert-Equal -Actual ([int]$summary.total_table_position_review_count) -Expected 1 `
        -Message "Aggregate layout rollup should sum floating table review plans."
    Assert-Equal -Actual ([int]$summary.total_table_position_already_matching_count) -Expected 1 `
        -Message "Aggregate layout rollup should sum already-matching floating table plans."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
        -Message "Aggregate layout rollup should preserve release blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 3 `
        -Message "Aggregate layout rollup should preserve action items."
    Assert-Equal -Actual ([int]$summary.table_position_plan_count) -Expected 2 `
        -Message "Aggregate layout rollup should expose per-document position plans."
    Assert-ContainsText -Text (($summary.table_position_plans | ForEach-Object { [string]$_.table_position_plan_path }) -join "`n") `
        -ExpectedText "contract/table-position-margin-anchor.plan.json" `
        -Message "Aggregate layout rollup should include contract floating position plan path."

    $issueSummaryText = ($summary.table_style_issue_summary | ForEach-Object { "$($_.issue):$($_.count)" }) -join "`n"
    Assert-ContainsText -Text $issueSummaryText -ExpectedText "bad_tblLook:2" `
        -Message "Aggregate layout rollup should group tblLook issues."
    Assert-ContainsText -Text $issueSummaryText -ExpectedText "missing_region:1" `
        -Message "Aggregate layout rollup should group missing region issues."
    Assert-Equal -Actual ([string]$summary.release_blockers[0].document_name) -Expected "contract.docx" `
        -Message "Release blockers should carry the source document name."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Floating Table Plans" `
        -Message "Markdown report should include floating table plan section."
    Assert-ContainsText -Text $markdown -ExpectedText "contract.docx" `
        -Message "Markdown report should include source document names."
    Assert-ContainsText -Text $markdown -ExpectedText "bad_tblLook" `
        -Message "Markdown report should include issue summary."
}

if (Test-Scenario -Name "empty") {
    $emptyRoot = Join-Path $resolvedWorkingDir "empty-input"
    $emptyOutputDir = Join-Path $resolvedWorkingDir "empty-output"
    $emptyReportPath = Join-Path $emptyRoot "summary.json"

    Write-JsonFile -Path $emptyReportPath -Value (New-LayoutSummary `
        -InputDocx "samples/clean.docx" `
        -Preset "paragraph-callout" `
        -PlanPath "output/table-layout-delivery/clean/table-position-paragraph-callout.plan.json" `
        -Status "ready")

    $emptyResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $emptyRoot,
        "-OutputDir", $emptyOutputDir
    )
    Assert-Equal -Actual $emptyResult.ExitCode -Expected 0 `
        -Message "Clean layout rollup should pass. Output: $($emptyResult.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $emptyOutputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Clean layout rollup should set status=ready."
    Assert-Equal -Actual ([bool]$summary.ready) -Expected $true `
        -Message "Clean layout rollup should set ready=true."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Clean layout rollup should not invent release blockers."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 0 `
        -Message "Clean layout rollup should not warn for a valid layout summary."
}

if (Test-Scenario -Name "failed_source_report") {
    $failedSourceRoot = Join-Path $resolvedWorkingDir "failed-source-input"
    $failedSourceOutputDir = Join-Path $resolvedWorkingDir "failed-source-output"
    $failedSourceReportPath = Join-Path $failedSourceRoot "summary.json"

    $failedSummary = New-LayoutSummary `
        -InputDocx "samples/failed.docx" `
        -Preset "page-corner" `
        -PlanPath "output/table-layout-delivery/failed/table-position-page-corner.plan.json" `
        -Status "failed"
    $failedSummary.command_failure_count = 1

    Write-JsonFile -Path $failedSourceReportPath -Value $failedSummary

    $failedSourceResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $failedSourceRoot,
        "-OutputDir", $failedSourceOutputDir
    )
    Assert-Equal -Actual $failedSourceResult.ExitCode -Expected 1 `
        -Message "Layout rollup should fail when a readable source report is already failed. Output: $($failedSourceResult.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $failedSourceOutputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "failed" `
        -Message "Layout rollup should inherit failed single-document report status."
    Assert-Equal -Actual ([int]$summary.source_report_failure_count) -Expected 1 `
        -Message "Layout rollup should count failed single-document summaries separately from read failures."
    Assert-Equal -Actual ([int]$summary.source_failure_count) -Expected 0 `
        -Message "Layout rollup should keep source read failures separate from failed report statuses."
    Assert-Equal -Actual ([int]$summary.total_command_failure_count) -Expected 1 `
        -Message "Layout rollup should preserve command failure totals."
}

if (Test-Scenario -Name "malformed") {
    $malformedRoot = Join-Path $resolvedWorkingDir "malformed-input"
    $malformedOutputDir = Join-Path $resolvedWorkingDir "malformed-output"
    $malformedReportPath = Join-Path $malformedRoot "summary.json"
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($malformedReportPath)) -Force | Out-Null
    Set-Content -LiteralPath $malformedReportPath -Encoding UTF8 -Value "{ this is not valid json"

    $malformedResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $malformedRoot,
        "-OutputDir", $malformedOutputDir
    )
    Assert-Equal -Actual $malformedResult.ExitCode -Expected 1 `
        -Message "Malformed layout rollup should fail after writing summary. Output: $($malformedResult.Text)"
    $summaryPath = Join-Path $malformedOutputDir "summary.json"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Malformed layout rollup should still write summary.json."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "failed" `
        -Message "Malformed layout rollup should set status=failed."
    Assert-Equal -Actual ([int]$summary.source_failure_count) -Expected 1 `
        -Message "Malformed layout rollup should count failed source reads."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "source_report_read_failed" `
        -Message "Malformed layout rollup should include a source read warning."
    $sourceReadWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "source_report_read_failed" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$sourceReadWarning[0].action) -Expected "fix_table_layout_delivery_rollup_input_json" `
        -Message "Malformed layout rollup should expose a fixed remediation action."
    Assert-Equal -Actual ([string]$sourceReadWarning[0].source_schema) -Expected "featherdoc.table_layout_delivery_rollup_report.v1" `
        -Message "Malformed layout rollup should expose the source schema."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $malformedOutputDir "table_layout_delivery_rollup.md")
    Assert-ContainsText -Text $markdown -ExpectedText "### Table layout delivery rollup warnings" `
        -Message "Markdown should include the rollup warning subsection."
    Assert-ContainsText -Text $markdown -ExpectedText '- warning_count: `1`' `
        -Message "Markdown should include warning count."
    Assert-ContainsText -Text $markdown -ExpectedText 'id: `source_report_read_failed`' `
        -Message "Markdown should include warning id."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `fix_table_layout_delivery_rollup_input_json`' `
        -Message "Markdown should include warning action."
    Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.table_layout_delivery_rollup_report.v1`' `
        -Message "Markdown should include warning source schema."
}

if (Test-Scenario -Name "fail_on_issue") {
    $failOnIssueRoot = Join-Path $resolvedWorkingDir "fail-on-issue-input"
    $failOnIssueOutputDir = Join-Path $resolvedWorkingDir "fail-on-issue-output"
    $failOnIssueReportPath = Join-Path $failOnIssueRoot "summary.json"

    Write-JsonFile -Path $failOnIssueReportPath -Value (New-LayoutSummary `
        -InputDocx "samples/needs-review.docx" `
        -Preset "margin-anchor" `
        -PlanPath "output/table-layout-delivery/needs-review/table-position-margin-anchor.plan.json" `
        -Status "needs_review" `
        -IssueCount 1 `
        -Issues @([ordered]@{ kind = "bad_tblLook" }))

    $failOnIssueResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $failOnIssueRoot,
        "-OutputDir", $failOnIssueOutputDir,
        "-FailOnIssue"
    )
    Assert-Equal -Actual $failOnIssueResult.ExitCode -Expected 1 `
        -Message "Layout rollup should fail with -FailOnIssue when style issues exist. Output: $($failOnIssueResult.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $failOnIssueOutputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.total_table_style_issue_count) -Expected 1 `
        -Message "Fail-on-issue layout summary should still be written with issue counts."
}

if (Test-Scenario -Name "fail_on_blocker") {
    $failOnBlockerRoot = Join-Path $resolvedWorkingDir "fail-on-blocker-input"
    $failOnBlockerOutputDir = Join-Path $resolvedWorkingDir "fail-on-blocker-output"
    $failOnBlockerReportPath = Join-Path $failOnBlockerRoot "summary.json"

    Write-JsonFile -Path $failOnBlockerReportPath -Value (New-LayoutSummary `
        -InputDocx "samples/blocked.docx" `
        -Preset "page-corner" `
        -PlanPath "output/table-layout-delivery/blocked/table-position-page-corner.plan.json" `
        -Status "needs_review" `
        -PositionReviewCount 1 `
        -ReleaseBlockers @(
            [ordered]@{
                id = "table_layout.positioned_tables_need_review"
                severity = "warning"
                message = "Some existing floating table positions need manual review before applying the preset."
                action = "review_table_position_plan"
                review_count = 1
            }
        ))

    $failOnBlockerResult = Invoke-RollupScript -Arguments @(
        "-InputRoot", $failOnBlockerRoot,
        "-OutputDir", $failOnBlockerOutputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $failOnBlockerResult.ExitCode -Expected 1 `
        -Message "Layout rollup should fail with -FailOnBlocker when blockers exist. Output: $($failOnBlockerResult.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $failOnBlockerOutputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 1 `
        -Message "Fail-on-blocker layout summary should still preserve blockers."
}

Write-Host "Table layout delivery rollup regression passed."
