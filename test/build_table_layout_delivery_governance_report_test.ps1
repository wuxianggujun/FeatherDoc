param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "ready", "missing_rollup", "malformed", "fail_on_blocker")]
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
    ($Value | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-LayoutRollup {
    param([bool]$Ready)

    if ($Ready) {
        return [ordered]@{
            schema = "featherdoc.table_layout_delivery_rollup_report.v1"
            status = "ready"
            ready = $true
            document_count = 1
            document_entries = @(
                [ordered]@{
                    document_name = "invoice.docx"
                    input_docx = "samples/invoice.docx"
                    input_docx_display = ".\samples\invoice.docx"
                    status = "ready"
                    ready = $true
                    preset = "paragraph-callout"
                    table_style_issue_count = 0
                    automatic_tblLook_fix_count = 0
                    manual_table_style_fix_count = 0
                    table_position_automatic_count = 0
                    table_position_review_count = 0
                    table_position_already_matching_count = 1
                    command_failure_count = 0
                    table_position_plan_path = "output/table-layout-delivery/invoice/table-position.plan.json"
                    table_position_plan_display = ".\output\table-layout-delivery\invoice\table-position.plan.json"
                }
            )
            table_position_plans = @(
                [ordered]@{
                    document_name = "invoice.docx"
                    input_docx = "samples/invoice.docx"
                    preset = "paragraph-callout"
                    table_position_plan_path = "output/table-layout-delivery/invoice/table-position.plan.json"
                    table_position_plan_display = ".\output\table-layout-delivery\invoice\table-position.plan.json"
                    automatic_count = 0
                    review_count = 0
                    already_matching_count = 1
                }
            )
            total_table_style_issue_count = 0
            total_automatic_tblLook_fix_count = 0
            total_manual_table_style_fix_count = 0
            total_table_position_automatic_count = 0
            total_table_position_review_count = 0
            total_table_position_already_matching_count = 1
            total_command_failure_count = 0
            release_blockers = @()
            action_items = @()
        }
    }

    return [ordered]@{
        schema = "featherdoc.table_layout_delivery_rollup_report.v1"
        status = "needs_review"
        ready = $false
        document_count = 2
        document_entries = @(
            [ordered]@{
                document_name = "invoice.docx"
                input_docx = "samples/invoice.docx"
                input_docx_display = ".\samples\invoice.docx"
                status = "ready"
                ready = $true
                preset = "paragraph-callout"
                table_style_issue_count = 0
                automatic_tblLook_fix_count = 0
                manual_table_style_fix_count = 0
                table_position_automatic_count = 0
                table_position_review_count = 0
                table_position_already_matching_count = 1
                command_failure_count = 0
                table_position_plan_path = "output/table-layout-delivery/invoice/table-position.plan.json"
                table_position_plan_display = ".\output\table-layout-delivery\invoice\table-position.plan.json"
            },
            [ordered]@{
                document_name = "contract.docx"
                input_docx = "samples/contract.docx"
                input_docx_display = ".\samples\contract.docx"
                status = "needs_review"
                ready = $false
                preset = "margin-anchor"
                table_style_issue_count = 3
                automatic_tblLook_fix_count = 2
                manual_table_style_fix_count = 1
                table_position_automatic_count = 2
                table_position_review_count = 1
                table_position_already_matching_count = 0
                command_failure_count = 0
                table_position_plan_path = "output/table-layout-delivery/contract/table-position.plan.json"
                table_position_plan_display = ".\output\table-layout-delivery\contract\table-position.plan.json"
            }
        )
        table_position_plans = @(
            [ordered]@{
                document_name = "invoice.docx"
                input_docx = "samples/invoice.docx"
                preset = "paragraph-callout"
                table_position_plan_path = "output/table-layout-delivery/invoice/table-position.plan.json"
                table_position_plan_display = ".\output\table-layout-delivery\invoice\table-position.plan.json"
                automatic_count = 0
                review_count = 0
                already_matching_count = 1
            },
            [ordered]@{
                document_name = "contract.docx"
                input_docx = "samples/contract.docx"
                preset = "margin-anchor"
                table_position_plan_path = "output/table-layout-delivery/contract/table-position.plan.json"
                table_position_plan_display = ".\output\table-layout-delivery\contract\table-position.plan.json"
                automatic_count = 2
                review_count = 1
                already_matching_count = 0
            }
        )
        total_table_style_issue_count = 3
        total_automatic_tblLook_fix_count = 2
        total_manual_table_style_fix_count = 1
        total_table_position_automatic_count = 2
        total_table_position_review_count = 1
        total_table_position_already_matching_count = 1
        total_command_failure_count = 0
        release_blockers = @(
            [ordered]@{
                id = "table_layout.manual_table_style_quality_work"
                document_name = "contract.docx"
                severity = "warning"
                status = "needs_review"
                action = "review_table_style_quality_plan"
                message = "Table style quality has issues that need manual style-definition work."
                repair_strategy = "review_source_table_style_quality_plan"
                repair_hint = "Review the source rollup style quality findings before release."
                command_template = "featherdoc_cli inspect-table-style <input.docx> <style-id> --json"
            },
            [ordered]@{
                id = "table_layout.positioned_tables_need_review"
                document_name = "contract.docx"
                severity = "warning"
                status = "needs_review"
                action = "review_table_position_plan"
                message = "Some existing floating table positions need manual review before applying the preset."
                repair_strategy = "review_source_table_position_plan"
                repair_hint = "Review source table-position.plan.json entries with existing floating positions."
                command_template = "featherdoc_cli apply-table-position-plan <table-position.plan.json> --dry-run --json"
            }
        )
        action_items = @(
            [ordered]@{
                id = "apply_safe_tblLook_fixes"
                document_name = "contract.docx"
                action = "apply_safe_tblLook_fixes"
                title = "Apply safe tblLook-only fixes"
                command = "featherdoc_cli apply-table-style-quality-fixes contract.docx --look-only --json"
                repair_strategy = "source_apply_safe_tblLook_fixes"
                repair_hint = "Apply source rollup safe tblLook fixes only."
                command_template = "featherdoc_cli apply-table-style-quality-fixes <input.docx> --look-only --output <reviewed.docx> --json"
            }
        )
    }
}

function New-Evidence {
    param([string]$Root, [bool]$Ready)

    $rollupPath = Join-Path $Root "rollup\summary.json"
    Write-JsonFile -Path $rollupPath -Value (New-LayoutRollup -Ready:$Ready)
    return $rollupPath
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_table_layout_delivery_governance_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "aggregate") {
    $rollup = New-Evidence -Root (Join-Path $resolvedWorkingDir "aggregate-evidence") -Ready:$false
    $outputDir = Join-Path $resolvedWorkingDir "aggregate-report"
    $result = Invoke-GovernanceScript -Arguments @(
        "-InputJson", $rollup,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Governance report should pass without fail switches. Output: $($result.Text)"
    Assert-ContainsText -Text $result.Text -ExpectedText "Summary JSON:" `
        -Message "Script should print summary path."

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "table_layout_delivery_governance.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Governance report should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Governance report should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.table_layout_delivery_governance_report.v1" `
        -Message "Summary should expose the table layout governance schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Summary should require review with table style and floating table work."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $false `
        -Message "Summary should not be release-ready with blockers."
    Assert-Equal -Actual ([int]$summary.document_count) -Expected 2 `
        -Message "Summary should preserve document count."
    Assert-Equal -Actual ([int]$summary.ready_document_count) -Expected 1 `
        -Message "Summary should count ready documents."
    Assert-Equal -Actual ([int]$summary.needs_review_document_count) -Expected 1 `
        -Message "Summary should count needs-review documents."
    Assert-Equal -Actual ([int]$summary.delivery_quality_score) -Expected 0 `
        -Message "Unresolved table layout delivery work should reduce quality score."
    Assert-Equal -Actual ([string]$summary.delivery_quality_level) -Expected "blocked" `
        -Message "Unresolved table layout delivery work should produce blocked quality."
    Assert-Equal -Actual ([int]$summary.delivery_quality.ready_document_percent) -Expected 50 `
        -Message "Summary should expose ready document percentage."
    Assert-Equal -Actual ([int]$summary.delivery_quality.table_style_issue_count) -Expected 3 `
        -Message "Summary should expose table style issue count in delivery quality details."
    Assert-Equal -Actual ([int]$summary.delivery_quality.automatic_tblLook_fix_count) -Expected 2 `
        -Message "Summary should expose safe tblLook fix count in delivery quality details."
    Assert-Equal -Actual ([int]$summary.delivery_quality.manual_table_style_fix_count) -Expected 1 `
        -Message "Summary should expose manual table style fix count in delivery quality details."
    Assert-Equal -Actual ([int]$summary.delivery_quality.table_position_automatic_count) -Expected 2 `
        -Message "Summary should expose automatic floating table plan count in delivery quality details."
    Assert-Equal -Actual ([int]$summary.delivery_quality.table_position_review_count) -Expected 1 `
        -Message "Summary should expose floating table review count in delivery quality details."
    Assert-Equal -Actual ([int]$summary.delivery_quality.command_failure_count) -Expected 0 `
        -Message "Summary should expose command failure count in delivery quality details."
    Assert-Equal -Actual ([int]$summary.delivery_quality.unresolved_item_count) -Expected 10 `
        -Message "Summary should expose unresolved table layout delivery work count."
    Assert-ContainsText -Text (($summary.delivery_quality.penalty_summary | ForEach-Object { "$($_.factor):$($_.penalty)" }) -join "`n") `
        -ExpectedText "safe_tblLook_fixes_pending:8" `
        -Message "Summary should expose safe tblLook delivery quality penalties."
    Assert-ContainsText -Text (($summary.delivery_quality.penalty_summary | ForEach-Object { "$($_.factor):$($_.penalty)" }) -join "`n") `
        -ExpectedText "floating_table_plans_pending:14" `
        -Message "Summary should expose floating table delivery quality penalties."
    Assert-Equal -Actual ([int]$summary.total_table_style_issue_count) -Expected 3 `
        -Message "Summary should preserve table style issue count."
    Assert-Equal -Actual ([int]$summary.total_automatic_tblLook_fix_count) -Expected 2 `
        -Message "Summary should preserve safe tblLook fix count."
    Assert-Equal -Actual ([int]$summary.total_manual_table_style_fix_count) -Expected 1 `
        -Message "Summary should preserve manual style fix count."
    Assert-Equal -Actual ([int]$summary.total_table_position_automatic_count) -Expected 2 `
        -Message "Summary should preserve automatic floating table plan count."
    Assert-Equal -Actual ([int]$summary.total_table_position_review_count) -Expected 1 `
        -Message "Summary should preserve floating table review count."
    Assert-Equal -Actual ([int]$summary.table_position_plan_count) -Expected 2 `
        -Message "Summary should expose table position plans."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 5 `
        -Message "Summary should include source and governance blockers."
    Assert-True -Condition ([int]$summary.action_item_count -ge 5) `
        -Message "Summary should include source and governance action items."

    $blockerText = ($summary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n"
    Assert-ContainsText -Text $blockerText -ExpectedText "table_layout_delivery.safe_tblLook_fixes_pending" `
        -Message "Summary should add safe tblLook blocker."
    Assert-ContainsText -Text $blockerText -ExpectedText "table_layout_delivery.floating_table_review_pending" `
        -Message "Summary should add floating table review blocker."
    $blockerRepairText = ($summary.release_blockers | ForEach-Object {
            "$($_.id)|$($_.repair_strategy)|$($_.repair_hint)|$($_.command_template)"
        }) -join "`n"
    Assert-ContainsText -Text $blockerRepairText -ExpectedText "table_layout_delivery.safe_tblLook_fixes_pending|apply_safe_tblLook_fixes" `
        -Message "Safe tblLook blocker should expose repair strategy."
    Assert-ContainsText -Text $blockerRepairText -ExpectedText "apply-table-style-quality-fixes <input.docx> --look-only --output <reviewed.docx> --json" `
        -Message "Safe tblLook blocker should expose a command template."
    Assert-ContainsText -Text $blockerRepairText -ExpectedText "table_layout_delivery.manual_table_style_work|review_manual_table_style_definition_work" `
        -Message "Manual style blocker should expose repair strategy."
    Assert-ContainsText -Text $blockerRepairText -ExpectedText "table_layout_delivery.floating_table_review_pending|review_table_position_plan" `
        -Message "Floating table blocker should expose repair strategy."
    Assert-ContainsText -Text $blockerRepairText -ExpectedText "table_layout.manual_table_style_quality_work|review_source_table_style_quality_plan" `
        -Message "Rollup blocker repair strategy should be preserved."
    $safeTblLookBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "table_layout_delivery.safe_tblLook_fixes_pending" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$safeTblLookBlocker.source_schema) `
        -Expected "featherdoc.table_layout_delivery_governance_report.v1" `
        -Message "Synthetic safe tblLook blocker should carry the governance source schema."
    Assert-ContainsText -Text ([string]$safeTblLookBlocker.source_report_display) `
        -ExpectedText "aggregate-report\summary.json" `
        -Message "Synthetic safe tblLook blocker should point to the governance source report."
    Assert-ContainsText -Text ([string]$safeTblLookBlocker.source_json_display) `
        -ExpectedText "aggregate-report\summary.json" `
        -Message "Synthetic safe tblLook blocker should point to the governance source JSON."
    $floatingReviewBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "table_layout_delivery.floating_table_review_pending" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$floatingReviewBlocker.source_schema) `
        -Expected "featherdoc.table_layout_delivery_governance_report.v1" `
        -Message "Synthetic floating-table blocker should carry the governance source schema."

    $actionText = ($summary.delivery_actions | ForEach-Object { [string]$_.id }) -join "`n"
    Assert-ContainsText -Text $actionText -ExpectedText "run_table_style_quality_visual_regression" `
        -Message "Summary should add visual regression action."
    Assert-ContainsText -Text $actionText -ExpectedText "dry_run_apply_table_position_plans" `
        -Message "Summary should add position plan dry-run action."
    $actionRepairText = ($summary.delivery_actions | ForEach-Object {
            "$($_.id)|$($_.repair_strategy)|$($_.repair_hint)|$($_.command_template)"
        }) -join "`n"
    Assert-ContainsText -Text $actionRepairText -ExpectedText "apply_safe_tblLook_fixes|source_apply_safe_tblLook_fixes" `
        -Message "Rollup action repair strategy should be preserved."
    Assert-ContainsText -Text $actionRepairText -ExpectedText "review_floating_table_position_plans|review_table_position_plan" `
        -Message "Review action should expose repair strategy."
    Assert-ContainsText -Text $actionRepairText -ExpectedText "dry_run_apply_table_position_plans|dry_run_table_position_plan" `
        -Message "Dry-run action should expose repair strategy."
    Assert-ContainsText -Text $actionRepairText -ExpectedText "run_table_style_quality_visual_regression|generate_table_layout_visual_evidence" `
        -Message "Visual regression action should expose repair strategy."
    Assert-ContainsText -Text $actionRepairText -ExpectedText "run_table_style_quality_visual_regression.ps1" `
        -Message "Visual regression action should expose a command template."
    $visualRegressionAction = ($summary.delivery_actions |
        Where-Object { [string]$_.id -eq "run_table_style_quality_visual_regression" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$visualRegressionAction.source_schema) `
        -Expected "featherdoc.table_layout_delivery_governance_report.v1" `
        -Message "Synthetic visual regression action should carry the governance source schema."
    Assert-ContainsText -Text ([string]$visualRegressionAction.source_json_display) `
        -ExpectedText "aggregate-report\summary.json" `
        -Message "Synthetic visual regression action should point to the governance source JSON."
    Assert-ContainsText -Text ([string]$visualRegressionAction.open_command) `
        -ExpectedText "run_table_style_quality_visual_regression.ps1" `
        -Message "Synthetic visual regression action should expose an open command."
    $sourceSafeTblLookAction = ($summary.delivery_actions |
        Where-Object {
            [string]$_.id -eq "apply_safe_tblLook_fixes" -and
            [string]$_.repair_strategy -eq "source_apply_safe_tblLook_fixes"
        } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$sourceSafeTblLookAction.source_schema) `
        -Expected "featherdoc.table_layout_delivery_rollup_report.v1" `
        -Message "Source rollup action should keep the rollup source schema."
    Assert-ContainsText -Text ([string]$sourceSafeTblLookAction.open_command) `
        -ExpectedText "apply-table-style-quality-fixes contract.docx" `
        -Message "Source rollup action should expose its command as open_command."

    Assert-Equal -Actual ([int]$summary.action_items.Count) -Expected ([int]$summary.delivery_actions.Count) `
        -Message "Summary should mirror delivery actions as release rollup action items."
    Assert-Equal -Actual ([int]$summary.next_steps.Count) -Expected ([int]$summary.delivery_actions.Count) `
        -Message "Summary should mirror delivery actions as next-step fallback entries."
    $actionItemsText = ($summary.action_items | ForEach-Object { [string]$_.id }) -join "`n"
    Assert-ContainsText -Text $actionItemsText -ExpectedText "run_table_style_quality_visual_regression" `
        -Message "Release rollup action items should include visual regression action."
    $visualRegressionActionItem = ($summary.action_items |
        Where-Object { [string]$_.id -eq "run_table_style_quality_visual_regression" } |
        Select-Object -First 1)
    $visualRegressionNextStep = ($summary.next_steps |
        Where-Object { [string]$_.id -eq "run_table_style_quality_visual_regression" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$visualRegressionActionItem.source_json_display) `
        -Expected ([string]$visualRegressionAction.source_json_display) `
        -Message "Release rollup action item mirror should preserve source JSON display."
    Assert-Equal -Actual ([string]$visualRegressionNextStep.command_template) `
        -Expected ([string]$visualRegressionAction.command_template) `
        -Message "Next-step mirror should preserve command template."
    $nextStepRepairText = ($summary.next_steps | ForEach-Object {
            "$($_.id)|$($_.repair_strategy)|$($_.command_template)"
        }) -join "`n"
    Assert-ContainsText -Text $nextStepRepairText -ExpectedText "dry_run_apply_table_position_plans|dry_run_table_position_plan" `
        -Message "Next-step mirror should preserve repair strategy."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Table Layout Delivery Governance Report" `
        -Message "Markdown should include title."
    Assert-ContainsText -Text $markdown -ExpectedText "Floating Table Plans" `
        -Message "Markdown should include floating table plans."
    Assert-ContainsText -Text $markdown -ExpectedText "Delivery Actions" `
        -Message "Markdown should include delivery actions."
    Assert-ContainsText -Text $markdown -ExpectedText "Delivery Quality" `
        -Message "Markdown should include delivery quality section."
    Assert-ContainsText -Text $markdown -ExpectedText "safe_tblLook_fixes_pending" `
        -Message "Markdown should include delivery quality penalty details."
    Assert-ContainsText -Text $markdown -ExpectedText "source_schema" `
        -Message "Markdown should include traceable source schema fields for actions and blockers."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report_display" `
        -Message "Markdown should include source report display fields for actions and blockers."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display" `
        -Message "Markdown should include source JSON display fields for actions and blockers."
    Assert-ContainsText -Text $markdown -ExpectedText "repair_strategy" `
        -Message "Markdown should include repair strategy fields for actions and blockers."
    Assert-ContainsText -Text $markdown -ExpectedText "repair_hint" `
        -Message "Markdown should include repair hint fields for actions and blockers."
    Assert-ContainsText -Text $markdown -ExpectedText "command_template" `
        -Message "Markdown should include command template fields for actions and blockers."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.table_layout_delivery_rollup_report.v1" `
        -Message "Markdown should include the rollup source schema."
    Assert-ContainsText -Text $markdown -ExpectedText "aggregate-evidence\rollup\summary.json" `
        -Message "Markdown should include the source rollup summary display path."
}

if (Test-Scenario -Name "ready") {
    $rollup = New-Evidence -Root (Join-Path $resolvedWorkingDir "ready-evidence") -Ready:$true
    $outputDir = Join-Path $resolvedWorkingDir "ready-report"
    $result = Invoke-GovernanceScript -Arguments @(
        "-InputJson", $rollup,
        "-OutputDir", $outputDir,
        "-FailOnBlocker",
        "-FailOnIssue"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Ready governance evidence should pass fail switches. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Ready evidence should produce ready status."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Ready evidence should be release-ready."
    Assert-Equal -Actual ([int]$summary.delivery_quality_score) -Expected 100 `
        -Message "Ready table layout delivery evidence should produce full quality score."
    Assert-Equal -Actual ([string]$summary.delivery_quality_level) -Expected "release_ready" `
        -Message "Ready table layout delivery evidence should produce release-ready quality."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Ready evidence should not expose blockers."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 0 `
        -Message "Ready evidence should not warn."
}

if (Test-Scenario -Name "missing_rollup") {
    $emptyRoot = Join-Path $resolvedWorkingDir "missing-rollup-empty-root"
    New-Item -ItemType Directory -Path $emptyRoot -Force | Out-Null
    $outputDir = Join-Path $resolvedWorkingDir "missing-rollup-report"
    $result = Invoke-GovernanceScript -Arguments @(
        "-InputRoot", $emptyRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Missing rollup should write governance evidence without failing. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Missing rollup should require governance review."
    Assert-Equal -Actual ([int]$summary.table_layout_delivery_rollup_count) -Expected 0 `
        -Message "Missing rollup should record zero loaded rollups."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Missing rollup should emit one warning."

    $warning = @($summary.warnings | Where-Object { [string]$_.id -eq "table_layout_delivery_rollup_missing" })[0]
    Assert-True -Condition ($null -ne $warning) `
        -Message "Missing rollup should emit table_layout_delivery_rollup_missing."
    Assert-ContainsText -Text ([string]$warning.source_report) -ExpectedText "missing-rollup-report\summary.json" `
        -Message "Missing rollup warning should include source_report."
    Assert-ContainsText -Text ([string]$warning.source_report_display) -ExpectedText "missing-rollup-report\summary.json" `
        -Message "Missing rollup warning should include source_report_display."
    Assert-ContainsText -Text ([string]$warning.source_json) -ExpectedText "missing-rollup-report\summary.json" `
        -Message "Missing rollup warning should include source_json."
    Assert-ContainsText -Text ([string]$warning.source_json_display) -ExpectedText "missing-rollup-report\summary.json" `
        -Message "Missing rollup warning should include source_json_display."
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
    $readFailedWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "source_json_read_failed" })[0]
    Assert-True -Condition ($null -ne $readFailedWarning) `
        -Message "Malformed input should emit source_json_read_failed."
    Assert-ContainsText -Text ([string]$readFailedWarning.source_report) -ExpectedText "malformed-evidence\summary.json" `
        -Message "Read-failed warning should include source_report."
    Assert-ContainsText -Text ([string]$readFailedWarning.source_report_display) -ExpectedText "malformed-evidence\summary.json" `
        -Message "Read-failed warning should include source_report_display."
    Assert-ContainsText -Text ([string]$readFailedWarning.source_json) -ExpectedText "malformed-evidence\summary.json" `
        -Message "Read-failed warning should include source_json."
    Assert-ContainsText -Text ([string]$readFailedWarning.source_json_display) -ExpectedText "malformed-evidence\summary.json" `
        -Message "Read-failed warning should include source_json_display."
}

if (Test-Scenario -Name "fail_on_blocker") {
    $rollup = New-Evidence -Root (Join-Path $resolvedWorkingDir "fail-on-blocker-evidence") -Ready:$false
    $outputDir = Join-Path $resolvedWorkingDir "fail-on-blocker-report"
    $result = Invoke-GovernanceScript -Arguments @(
        "-InputJson", $rollup,
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
}

Write-Host "Table layout delivery governance report regression passed."
