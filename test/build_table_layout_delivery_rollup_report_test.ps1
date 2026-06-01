param(
    [string]$RepoRoot,
    [string]$BuildDir = "",
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "empty", "failed_source_report", "malformed", "fail_on_issue", "fail_on_blocker")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot "build\build_table_layout_delivery_rollup_report_test"
}

if (-not $WorkingDir) {
    $WorkingDir = $BuildDir
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

function New-PdfFloatingTableSupport {
    return [ordered]@{
        schema = "featherdoc.pdf_floating_table_support.v1"
        status = "partial"
        boundary = "stable_pdf_geometry_subset_not_full_word_wrapping"
        supported_geometry = @(
            "tblpXSpec left/inside/center/right/outside within page, margin, and column reference frames",
            "tblpYSpec top/center/bottom within page and margin reference frames",
            "topFromText for paragraph-anchored positioned tables",
            "bottomFromText as spacing before following body text"
        )
        metadata_only = @(
            "leftFromText",
            "rightFromText",
            "topFromText outside paragraph anchoring",
            "tblOverlap",
            "vertical paragraph/inside/outside Word page-side context"
        )
        review_required = @(
            "full Word-compatible floating table text wrapping",
            "table overlap avoidance and collision resolution",
            "inside/outside parity for page-side aware layout"
        )
    }
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

    $pdfSupport = New-PdfFloatingTableSupport
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
        pdf_floating_table_support = $pdfSupport
        pdf_floating_table_support_status = $pdfSupport.status
        pdf_floating_table_layout_boundary = $pdfSupport.boundary
        pdf_floating_table_supported_geometry_count = @($pdfSupport.supported_geometry).Count
        pdf_floating_table_metadata_only_count = @($pdfSupport.metadata_only).Count
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
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_support_report_count) -Expected 2 `
        -Message "Aggregate layout rollup should count PDF floating table support reports."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_supported_geometry_count) -Expected 8 `
        -Message "Aggregate layout rollup should sum supported PDF floating table geometry rows."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_metadata_only_count) -Expected 10 `
        -Message "Aggregate layout rollup should sum metadata-only PDF floating table rows."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_tracked_geometry_count) -Expected 18 `
        -Message "Aggregate layout rollup should expose tracked PDF floating table geometry rows."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_supported_geometry_percent) -Expected 44 `
        -Message "Aggregate layout rollup should expose supported PDF floating table geometry percentage."
    Assert-Equal -Actual ([string]$summary.pdf_floating_table_support_coverage) `
        -Expected "8/18 supported (44 percent); metadata_only=10" `
        -Message "Aggregate layout rollup should expose PDF floating table support coverage."
    Assert-Equal -Actual ([string]$summary.pdf_floating_table_reviewer_focus) `
        -Expected "review metadata-only tblpPr fields before approving PDF-layout-sensitive release." `
        -Message "Aggregate layout rollup should expose PDF floating table reviewer focus."
    Assert-ContainsText -Text (($summary.pdf_floating_table_metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "topFromText outside paragraph anchoring" `
        -Message "Aggregate layout rollup should expose machine-readable metadata-only PDF floating table fields."
    Assert-ContainsText -Text (($summary.pdf_floating_table_review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "full Word-compatible floating table text wrapping" `
        -Message "Aggregate layout rollup should expose machine-readable reviewer-required PDF floating table fields."
    Assert-ContainsText -Text (($summary.pdf_floating_table_support_summary | ForEach-Object { "$($_.status):$($_.count)" }) -join "`n") `
        -ExpectedText "partial:2" `
        -Message "Aggregate layout rollup should group PDF floating table support statuses."
    Assert-Equal -Actual ([string]$summary.document_entries[0].pdf_floating_table_support_status) -Expected "partial" `
        -Message "Document entries should preserve PDF floating table support status."
    Assert-Equal -Actual ([int]$summary.document_entries[0].pdf_floating_table_supported_geometry_percent) -Expected 44 `
        -Message "Document entries should preserve PDF floating table support percentage."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_support[0].supported_geometry_count) -Expected 4 `
        -Message "Rollup should preserve supported PDF floating table geometry count."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_support[0].tracked_geometry_count) -Expected 9 `
        -Message "Rollup should preserve tracked PDF floating table geometry count."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_support[0].supported_geometry_percent) -Expected 44 `
        -Message "Rollup should preserve supported PDF floating table geometry percentage."
    Assert-Equal -Actual ([string]$summary.pdf_floating_table_support[0].support_coverage) `
        -Expected "4/9 supported (44 percent); metadata_only=5" `
        -Message "Rollup should preserve PDF floating table support coverage."
    Assert-Equal -Actual ([string]$summary.pdf_floating_table_support[0].reviewer_focus) `
        -Expected "review metadata-only tblpPr fields before approving PDF-layout-sensitive release." `
        -Message "Rollup should preserve PDF floating table reviewer focus."
    Assert-ContainsText -Text (($summary.pdf_floating_table_support[0].metadata_only | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "tblOverlap" `
        -Message "Rollup should preserve metadata-only PDF floating table rows."
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
    Assert-ContainsText -Text $markdown -ExpectedText "PDF Floating Table Support" `
        -Message "Markdown report should include PDF floating table support section."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_support_coverage" `
        -Message "Markdown report should include PDF floating table support coverage marker."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_reviewer_focus" `
        -Message "Markdown report should include PDF floating table reviewer focus marker."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_metadata_only_fields" `
        -Message "Markdown report should include aggregate metadata-only PDF floating table field marker."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_review_required_fields" `
        -Message "Markdown report should include aggregate reviewer-required PDF floating table field marker."
    Assert-ContainsText -Text $markdown -ExpectedText "review_required_fields" `
        -Message "Markdown report should include per-document reviewer-required PDF floating table field marker."
    Assert-ContainsText -Text $markdown -ExpectedText "metadata-only tblpPr" `
        -Message "Markdown report should include metadata-only tblpPr reviewer guidance."
    Assert-ContainsText -Text $markdown -ExpectedText "stable_pdf_geometry_subset_not_full_word_wrapping" `
        -Message "Markdown report should include PDF floating table boundary."
    Assert-ContainsText -Text $markdown -ExpectedText "contract.docx" `
        -Message "Markdown report should include source document names."
    Assert-ContainsText -Text $markdown -ExpectedText "bad_tblLook" `
        -Message "Markdown report should include issue summary."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failure_count: ``0``" `
        -Message "Markdown report should expose a machine-readable source failure count."
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
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_support_report_count) -Expected 1 `
        -Message "Clean layout rollup should still preserve PDF floating table support evidence."
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
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $malformedOutputDir "table_layout_delivery_rollup.md")
    Assert-ContainsText -Text $markdown -ExpectedText "source_failure_count: ``1``" `
        -Message "Malformed layout Markdown should expose a machine-readable source failure count."
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
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $failOnBlockerOutputDir "table_layout_delivery_rollup.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blockers" `
        -Message "Fail-on-blocker layout Markdown should include the blocker section."
    Assert-ContainsText -Text $markdown -ExpectedText "table_layout.positioned_tables_need_review" `
        -Message "Fail-on-blocker layout Markdown should include the blocker id."
    Assert-ContainsText -Text $markdown -ExpectedText "review_table_position_plan" `
        -Message "Fail-on-blocker layout Markdown should include the blocker action."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failure_count: ``0``" `
        -Message "Fail-on-blocker layout Markdown should expose a machine-readable source failure count."
}

Write-Host "Table layout delivery rollup regression passed."
