param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "ready", "malformed", "warning_metadata", "fail_on_blocker")]
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
            },
            [ordered]@{
                id = "table_layout.positioned_tables_need_review"
                document_name = "contract.docx"
                severity = "warning"
                status = "needs_review"
                action = "review_table_position_plan"
                message = "Some existing floating table positions need manual review before applying the preset."
            }
        )
        action_items = @(
            [ordered]@{
                id = "apply_safe_tblLook_fixes"
                document_name = "contract.docx"
                action = "apply_safe_tblLook_fixes"
                title = "Apply safe tblLook-only fixes"
                command = "featherdoc_cli apply-table-style-quality-fixes contract.docx --look-only --json"
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

    $actionText = ($summary.delivery_actions | ForEach-Object { [string]$_.id }) -join "`n"
    Assert-ContainsText -Text $actionText -ExpectedText "run_table_style_quality_visual_regression" `
        -Message "Summary should add visual regression action."
    Assert-ContainsText -Text $actionText -ExpectedText "dry_run_apply_table_position_plans" `
        -Message "Summary should add position plan dry-run action."

    Assert-Equal -Actual ([int]$summary.action_items.Count) -Expected ([int]$summary.delivery_actions.Count) `
        -Message "Summary should mirror delivery actions as release rollup action items."
    Assert-Equal -Actual ([int]$summary.next_steps.Count) -Expected ([int]$summary.delivery_actions.Count) `
        -Message "Summary should mirror delivery actions as next-step fallback entries."
    $actionItemsText = ($summary.action_items | ForEach-Object { [string]$_.id }) -join "`n"
    Assert-ContainsText -Text $actionItemsText -ExpectedText "run_table_style_quality_visual_regression" `
        -Message "Release rollup action items should include visual regression action."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Table Layout Delivery Governance Report" `
        -Message "Markdown should include title."
    Assert-ContainsText -Text $markdown -ExpectedText "Floating Table Plans" `
        -Message "Markdown should include floating table plans."
    Assert-ContainsText -Text $markdown -ExpectedText "Delivery Actions" `
        -Message "Markdown should include delivery actions."
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
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Ready evidence should not expose blockers."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 0 `
        -Message "Ready evidence should not warn."
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
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 2 `
        -Message "Malformed input should produce read-failure and missing-rollup warnings."
    $sourceReadWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "source_json_read_failed" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$sourceReadWarning[0].action) -Expected "fix_table_layout_delivery_governance_input_json" `
        -Message "Malformed input warning should expose a fixed remediation action."
    Assert-Equal -Actual ([string]$sourceReadWarning[0].source_schema) -Expected "featherdoc.table_layout_delivery_governance_report.v1" `
        -Message "Malformed input warning should expose the governance source schema."
    $missingRollupWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "table_layout_delivery_rollup_missing" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$missingRollupWarning[0].action) -Expected "run_table_layout_delivery_rollup" `
        -Message "Missing rollup warning should expose a fixed remediation action."
    Assert-Equal -Actual ([string]$missingRollupWarning[0].source_schema) -Expected "featherdoc.table_layout_delivery_governance_report.v1" `
        -Message "Missing rollup warning should expose the governance source schema."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "table_layout_delivery_governance.md")
    Assert-ContainsText -Text $markdown -ExpectedText "### Table layout delivery governance warnings" `
        -Message "Markdown should include the table layout warning subsection."
    Assert-ContainsText -Text $markdown -ExpectedText '- warning_count: `2`' `
        -Message "Markdown should include warning count."
    Assert-ContainsText -Text $markdown -ExpectedText 'id: `source_json_read_failed`' `
        -Message "Markdown should include warning id."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `fix_table_layout_delivery_governance_input_json`' `
        -Message "Markdown should include read failure warning action."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `run_table_layout_delivery_rollup`' `
        -Message "Markdown should include missing rollup warning action."
    Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.table_layout_delivery_governance_report.v1`' `
        -Message "Markdown should include warning source schema."
}

if (Test-Scenario -Name "warning_metadata") {
    $evidenceRoot = Join-Path $resolvedWorkingDir "warning-metadata-evidence"
    $wrongSchema = Join-Path $evidenceRoot "wrong-schema\summary.json"
    $outputDir = Join-Path $resolvedWorkingDir "warning-metadata-report"
    Write-JsonFile -Path $wrongSchema -Value ([ordered]@{
        schema = "featherdoc.unrelated_report.v1"
        status = "ready"
    })

    $result = Invoke-GovernanceScript -Arguments @(
        "-InputJson", $wrongSchema,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Warning metadata input should not fail without fail switches. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 2 `
        -Message "Warning metadata input should produce schema and missing-rollup warnings."

    $schemaWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "source_json_schema_skipped" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$schemaWarning[0].action) -Expected "provide_table_layout_delivery_governance_evidence" `
        -Message "Schema skipped warning should expose a fixed remediation action."
    Assert-Equal -Actual ([string]$schemaWarning[0].source_schema) -Expected "featherdoc.table_layout_delivery_governance_report.v1" `
        -Message "Schema skipped warning should expose the governance source schema."
    $missingRollupWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "table_layout_delivery_rollup_missing" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$missingRollupWarning[0].action) -Expected "run_table_layout_delivery_rollup" `
        -Message "Missing rollup warning should expose a fixed remediation action."
    Assert-Equal -Actual ([string]$missingRollupWarning[0].source_schema) -Expected "featherdoc.table_layout_delivery_governance_report.v1" `
        -Message "Missing rollup warning should expose the governance source schema."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "table_layout_delivery_governance.md")
    Assert-ContainsText -Text $markdown -ExpectedText '- warning_count: `2`' `
        -Message "Warning metadata Markdown should include warning count."
    Assert-ContainsText -Text $markdown -ExpectedText 'id: `source_json_schema_skipped`' `
        -Message "Warning metadata Markdown should include schema warning id."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `provide_table_layout_delivery_governance_evidence`' `
        -Message "Warning metadata Markdown should include schema warning action."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `run_table_layout_delivery_rollup`' `
        -Message "Warning metadata Markdown should include missing rollup warning action."
    Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.table_layout_delivery_governance_report.v1`' `
        -Message "Warning metadata Markdown should include warning source schema."
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
