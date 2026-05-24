param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("aggregate", "fail_on_blocker", "fail_on_warning")]
    [string]$Scenario = "aggregate"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\build_content_control_data_binding_governance_report_test"
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

function Write-JsonFile {
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-Report {
    param([string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$inputDir = Join-Path $resolvedWorkingDir "input"
$outputDir = Join-Path $resolvedWorkingDir "report"
$inspectPath = Join-Path $inputDir "inspect-content-controls.json"
$syncPath = Join-Path $inputDir "sync-content-controls-from-custom-xml.json"
$skippedPath = Join-Path $inputDir "unrelated.json"

Write-JsonFile -Path $inspectPath -Value ([ordered]@{
        part = "body"
        entry_name = "word/document.xml"
        count = 3
        content_controls = @(
            [ordered]@{
                index = 0
                kind = "block"
                form_kind = "date"
                tag = "due_date"
                alias = "Due Date"
                id = "10"
                lock = "sdtLocked"
                data_binding_store_item_id = "{55555555-5555-5555-5555-555555555555}"
                data_binding_xpath = "/invoice/dueDate"
                data_binding_prefix_mappings = "xmlns:fd=`"urn:featherdoc`""
                checked = $null
                date_format = "yyyy/MM/dd"
                date_locale = "zh-CN"
                selected_list_item = $null
                list_items = @()
                showing_placeholder = $true
                text = "Due date placeholder"
            },
            [ordered]@{
                index = 1
                kind = "block"
                form_kind = "drop_down_list"
                tag = ""
                alias = "Status Owner's Choice"
                id = "11"
                lock = ""
                data_binding_store_item_id = ""
                data_binding_xpath = ""
                data_binding_prefix_mappings = ""
                checked = $null
                date_format = ""
                date_locale = ""
                selected_list_item = 0
                list_items = @([ordered]@{ display_text = "Draft"; value = "draft" })
                showing_placeholder = $false
                text = "Draft"
            },
            [ordered]@{
                index = 2
                kind = "block"
                form_kind = "plain_text"
                tag = "due_date_copy"
                alias = "Due Date Copy"
                id = "12"
                lock = ""
                data_binding_store_item_id = "{55555555-5555-5555-5555-555555555555}"
                data_binding_xpath = "/invoice/dueDate"
                data_binding_prefix_mappings = ""
                checked = $null
                date_format = ""
                date_locale = ""
                selected_list_item = $null
                list_items = @()
                showing_placeholder = $false
                text = "Due date: 2026-07-15"
            }
        )
    })

Write-JsonFile -Path $syncPath -Value ([ordered]@{
        scanned_content_controls = 3
        bound_content_controls = 2
        synced_content_controls = 1
        issue_count = 1
        synced_items = @(
            [ordered]@{
                part_entry_name = "word/document.xml"
                content_control_index = 2
                tag = "due_date_copy"
                alias = "Due Date Copy"
                store_item_id = "{55555555-5555-5555-5555-555555555555}"
                xpath = "/invoice/dueDate"
                previous_text = "old"
                value = "Due date: 2026-07-15"
            }
        )
        issues = @(
            [ordered]@{
                part_entry_name = "word/document.xml"
                content_control_index = 0
                tag = "due_date"
                alias = "Due Date"
                store_item_id = "{55555555-5555-5555-5555-555555555555}"
                xpath = "/invoice/missingDueDate"
                reason = "custom_xml_value_not_found"
            }
        )
    })

Write-JsonFile -Path $skippedPath -Value ([ordered]@{
        schema = "featherdoc.unrelated_report.v1"
        status = "ready"
    })

$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_content_control_data_binding_governance_report.ps1"
$arguments = @(
    "-InputJson"
    "$inspectPath,$syncPath,$skippedPath"
    "-OutputDir"
    $outputDir
)
if ($Scenario -eq "fail_on_blocker") {
    $arguments += "-FailOnBlocker"
}
if ($Scenario -eq "fail_on_warning") {
    $arguments += "-FailOnWarning"
}

$result = Invoke-Report -Arguments $arguments
if ($Scenario -in @("fail_on_blocker", "fail_on_warning")) {
    if ($result.ExitCode -eq 0) {
        throw "Content-control governance fail run should fail. Output: $($result.Text)"
    }
} else {
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Content-control governance aggregate run should pass. Output: $($result.Text)"
}

$summaryPath = Join-Path $outputDir "summary.json"
$markdownPath = Join-Path $outputDir "content_control_data_binding_governance.md"
Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "Summary JSON was not created."
Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
    -Message "Markdown report was not created."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.content_control_data_binding_governance_report.v1" `
    -Message "Summary should expose schema."
Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
    -Message "Summary should be blocked by sync issue and placeholder evidence."
Assert-Equal -Actual ([int]$summary.content_control_count) -Expected 3 `
    -Message "Summary should count inspected content controls."
Assert-Equal -Actual ([int]$summary.bound_content_control_count) -Expected 2 `
    -Message "Summary should count data-bound controls."
Assert-Equal -Actual ([int]$summary.sync_issue_count) -Expected 1 `
    -Message "Summary should count sync issues."
Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
    -Message "Summary should create blockers for sync issue and bound placeholder."
Assert-True -Condition ([int]$summary.action_item_count -ge 2) `
    -Message "Summary should emit lock/unbound/duplicate review actions."
Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
    -Message "Summary should expose skipped source warnings."
Assert-Equal -Actual ([string]$summary.repair_plan_schema) -Expected "featherdoc.content_control_data_binding_repair_plan.v1" `
    -Message "Summary should expose repair plan schema."
Assert-Equal -Actual ([int]$summary.repair_plan_item_count) -Expected 5 `
    -Message "Summary should create repair feasibility items for blockers and actions."
Assert-Equal -Actual ([int]$summary.repair_plan_apply_supported_count) -Expected 3 `
    -Message "Summary should count apply-supported repair paths."
Assert-Equal -Actual ([int]$summary.repair_plan_native_dry_run_supported_count) -Expected 0 `
    -Message "Summary should make native dry-run support explicit."
Assert-Equal -Actual ([int]$summary.repair_plan_requires_user_values_count) -Expected 1 `
    -Message "Summary should count repair paths that need user-provided binding values."
Assert-Equal -Actual ([int]$summary.repair_plan_requires_visual_verification_count) -Expected 4 `
    -Message "Summary should count apply paths that require visual verification."

$bindingCoverage = @{}
foreach ($entry in @($summary.binding_coverage_summary)) {
    $bindingCoverage[[string]$entry.coverage] = [int]$entry.count
}
Assert-Equal -Actual ([int]$bindingCoverage["bound"]) -Expected 2 `
    -Message "Summary should count content controls with data bindings."
Assert-Equal -Actual ([int]$bindingCoverage["bound_placeholder"]) -Expected 1 `
    -Message "Summary should count data-bound placeholders separately."
Assert-Equal -Actual ([int]$bindingCoverage["locked_bound"]) -Expected 1 `
    -Message "Summary should count locked data-bound controls separately."
Assert-Equal -Actual ([int]$bindingCoverage["unbound"]) -Expected 1 `
    -Message "Summary should count unbound controls separately."
Assert-True -Condition ($null -eq $summary.PSObject.Properties["binding_status_summary"]) `
    -Message "Summary should not emit the removed legacy binding_status_summary field."

$blockerIds = @($summary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n"
Assert-ContainsText -Text $blockerIds -ExpectedText "content_control_data_binding.custom_xml_sync_issue" `
    -Message "Summary should include Custom XML sync blocker."
Assert-ContainsText -Text $blockerIds -ExpectedText "content_control_data_binding.bound_placeholder" `
    -Message "Summary should include placeholder blocker."
$firstBlocker = @($summary.release_blockers)[0]
Assert-Equal -Actual ([string]$firstBlocker.source_schema) -Expected "featherdoc.content_control_data_binding_governance_report.v1" `
    -Message "Blocker should carry content-control governance source schema."
Assert-ContainsText -Text ([string]$firstBlocker.source_json) -ExpectedText "sync-content-controls-from-custom-xml.json" `
    -Message "Blocker should carry raw source JSON."
Assert-ContainsText -Text ([string]$firstBlocker.source_json_display) -ExpectedText "sync-content-controls-from-custom-xml.json" `
    -Message "Blocker should carry source JSON display."
Assert-ContainsText -Text ([string]$firstBlocker.source_report) -ExpectedText "summary.json" `
    -Message "Blocker should carry governance summary source report."
Assert-ContainsText -Text ([string]$firstBlocker.source_report_display) -ExpectedText "report\summary.json" `
    -Message "Blocker source report display should point at the governance summary."
Assert-Equal -Actual ([string]$firstBlocker.repair_strategy) -Expected "fix_custom_xml_source" `
    -Message "Sync blockers should carry a repair strategy."
Assert-ContainsText -Text ([string]$firstBlocker.repair_hint) -ExpectedText "Custom XML" `
    -Message "Sync blockers should carry a repair hint."
Assert-ContainsText -Text ([string]$firstBlocker.command_template) -ExpectedText "sync-content-controls-from-custom-xml" `
    -Message "Sync blockers should carry a CLI command template."
$placeholderBlocker = @($summary.release_blockers | Where-Object { [string]$_.id -eq "content_control_data_binding.bound_placeholder" })[0]
Assert-Equal -Actual ([string]$placeholderBlocker.source_schema) -Expected "featherdoc.content_control_data_binding_governance_report.v1" `
    -Message "Placeholder blockers should carry content-control governance source schema."
Assert-ContainsText -Text ([string]$placeholderBlocker.source_json) -ExpectedText "inspect-content-controls.json" `
    -Message "Placeholder blockers should carry raw source JSON."
Assert-ContainsText -Text ([string]$placeholderBlocker.source_json_display) -ExpectedText "inspect-content-controls.json" `
    -Message "Placeholder blockers should carry source JSON display."
Assert-ContainsText -Text ([string]$placeholderBlocker.source_report) -ExpectedText "summary.json" `
    -Message "Placeholder blockers should carry governance summary source report."
Assert-ContainsText -Text ([string]$placeholderBlocker.source_report_display) -ExpectedText "report\summary.json" `
    -Message "Placeholder blocker source report display should point at the governance summary."
Assert-Equal -Actual ([string]$placeholderBlocker.repair_strategy) -Expected "sync_bound_content_control" `
    -Message "Placeholder blockers should carry sync repair strategy."
Assert-ContainsText -Text ([string]$placeholderBlocker.repair_hint) -ExpectedText "Rerun Custom XML sync" `
    -Message "Placeholder blockers should carry sync repair hint."
Assert-ContainsText -Text ([string]$placeholderBlocker.command_template) -ExpectedText "sync-content-controls-from-custom-xml" `
    -Message "Placeholder blockers should carry sync command template."

$actionOpenCommands = @($summary.action_items | ForEach-Object { [string]$_.open_command }) -join "`n"
Assert-ContainsText -Text $actionOpenCommands -ExpectedText "build_content_control_data_binding_governance_report.ps1" `
    -Message "Action items should carry reviewer open commands."
$lockAction = @($summary.action_items | Where-Object { [string]$_.id -eq "review_content_control_lock_strategy" })[0]
Assert-Equal -Actual ([string]$lockAction.repair_strategy) -Expected "review_lock_state" `
    -Message "Lock review actions should carry a repair strategy."
Assert-ContainsText -Text ([string]$lockAction.command_template) -ExpectedText "--clear-lock" `
    -Message "Lock review actions should carry a clear-lock command template."
$unboundAction = @($summary.action_items | Where-Object { [string]$_.id -eq "review_unbound_form_content_control" })[0]
Assert-Equal -Actual ([string]$unboundAction.repair_strategy) -Expected "bind_or_exempt_form_control" `
    -Message "Unbound form actions should carry a repair strategy."
Assert-ContainsText -Text ([string]$unboundAction.command_template) -ExpectedText "--data-binding-store-item-id" `
    -Message "Unbound form actions should carry a binding command template."
Assert-ContainsText -Text ([string]$unboundAction.command_template) -ExpectedText '--alias "Status Owner''s Choice"' `
    -Message "Unbound form actions should quote alias arguments with the shared command-line helper."
$duplicateAction = @($summary.action_items | Where-Object { [string]$_.id -eq "review_duplicate_content_control_binding" })[0]
Assert-Equal -Actual ([string]$duplicateAction.repair_strategy) -Expected "deduplicate_or_confirm_shared_binding" `
    -Message "Duplicate binding actions should carry a repair strategy."
Assert-ContainsText -Text ([string]$duplicateAction.command_template) -ExpectedText "inspect-content-controls" `
    -Message "Duplicate binding actions should carry an inspection command template."
Assert-ContainsText -Text ([string]$duplicateAction.source_json) -ExpectedText "inspect-content-controls.json" `
    -Message "Duplicate binding actions should keep raw source JSON."
Assert-ContainsText -Text ([string]$duplicateAction.source_json_display) -ExpectedText "inspect-content-controls.json" `
    -Message "Duplicate binding actions should keep source JSON display."
Assert-ContainsText -Text ([string]$duplicateAction.source_report) -ExpectedText "summary.json" `
    -Message "Duplicate binding actions should carry governance summary source report."
Assert-ContainsText -Text ([string]$duplicateAction.source_report_display) -ExpectedText "report\summary.json" `
    -Message "Duplicate binding action source report display should point at the governance summary."
Assert-Equal -Actual ([string]$duplicateAction.duplicate_binding_key) -Expected "{55555555-5555-5555-5555-555555555555}|/invoice/dueDate" `
    -Message "Duplicate binding actions should carry a duplicate binding key."
Assert-Equal -Actual ([int]$duplicateAction.duplicate_member_count) -Expected 2 `
    -Message "Duplicate binding actions should carry the shared-binding group size."
Assert-Equal -Actual (@($duplicateAction.duplicate_members).Count) -Expected 2 `
    -Message "Duplicate binding actions should carry the full duplicate member list."
Assert-ContainsText -Text ((@($duplicateAction.duplicate_members) | ForEach-Object { [string]$_.alias }) -join "`n") -ExpectedText "Due Date Copy" `
    -Message "Duplicate binding actions should include the second shared-binding member."
Assert-ContainsText -Text ([string]$duplicateAction.command_template) -ExpectedText "inspect-content-controls <input.docx> --json" `
    -Message "Duplicate binding actions should inspect the whole document."
$repairStatuses = @($summary.repair_plan_status_summary | ForEach-Object { [string]$_.plan_status }) -join "`n"
Assert-ContainsText -Text $repairStatuses -ExpectedText "source_fix_required" `
    -Message "Repair plan should flag Custom XML source fixes."
Assert-ContainsText -Text $repairStatuses -ExpectedText "review_then_apply" `
    -Message "Repair plan should flag review-then-apply paths."
Assert-ContainsText -Text $repairStatuses -ExpectedText "requires_user_values" `
    -Message "Repair plan should flag value-dependent binding paths."
Assert-ContainsText -Text $repairStatuses -ExpectedText "review_only" `
    -Message "Repair plan should flag review-only paths."
$syncPlan = @($summary.repair_plan_items | Where-Object { [string]$_.repair_strategy -eq "sync_bound_content_control" })[0]
Assert-Equal -Actual ([string]$syncPlan.source_id) -Expected "content_control_data_binding.bound_placeholder" `
    -Message "Bound placeholder sync plan should keep the source blocker id."
Assert-ContainsText -Text ([string]$syncPlan.source_json) -ExpectedText "inspect-content-controls.json" `
    -Message "Bound placeholder sync plan should keep raw source JSON."
Assert-ContainsText -Text ([string]$syncPlan.source_json_display) -ExpectedText "inspect-content-controls.json" `
    -Message "Bound placeholder sync plan should keep source JSON display."
Assert-ContainsText -Text ([string]$syncPlan.source_report) -ExpectedText "summary.json" `
    -Message "Bound placeholder sync plan should keep governance summary source report."
Assert-ContainsText -Text ([string]$syncPlan.source_report_display) -ExpectedText "report\summary.json" `
    -Message "Bound placeholder sync plan source report display should point at the governance summary."
Assert-Equal -Actual ([string]$syncPlan.open_command) -Expected "" `
    -Message "Release-blocker repair plans should not invent an open command."
Assert-ContainsText -Text ([string]$syncPlan.command_template) -ExpectedText "sync-content-controls-from-custom-xml" `
    -Message "Bound placeholder sync plan should carry sync command template."
Assert-ContainsText -Text ([string]$syncPlan.repair_hint) -ExpectedText "Rerun Custom XML sync" `
    -Message "Bound placeholder sync plan should carry repair hint."
Assert-Equal -Actual ([string]$syncPlan.plan_status) -Expected "review_then_apply" `
    -Message "Bound placeholder sync should be review-then-apply."
$unboundPlan = @($summary.repair_plan_items | Where-Object { [string]$_.repair_strategy -eq "bind_or_exempt_form_control" })[0]
Assert-Equal -Actual ([string]$unboundPlan.plan_status) -Expected "requires_user_values" `
    -Message "Unbound form repair should require user-provided binding values."
Assert-ContainsText -Text ([string]$unboundPlan.open_command) `
    -ExpectedText "build_content_control_data_binding_governance_report.ps1" `
    -Message "Action-derived repair plans should keep reviewer open commands."
Assert-ContainsText -Text ((@($unboundPlan.required_user_values) | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "data_binding_xpath" `
    -Message "Unbound form repair should name required binding values."
$duplicatePlan = @($summary.repair_plan_items | Where-Object { [string]$_.repair_strategy -eq "deduplicate_or_confirm_shared_binding" })[0]
Assert-Equal -Actual ([bool]$duplicatePlan.apply_supported) -Expected $false `
    -Message "Duplicate binding repair should stay review-only."
Assert-Equal -Actual ([string]$duplicatePlan.duplicate_binding_key) -Expected "{55555555-5555-5555-5555-555555555555}|/invoice/dueDate" `
    -Message "Duplicate binding repair plan should keep the shared-binding key."
Assert-Equal -Actual ([int]$duplicatePlan.duplicate_member_count) -Expected 2 `
    -Message "Duplicate binding repair plan should keep the member count."
Assert-Equal -Actual (@($duplicatePlan.duplicate_members).Count) -Expected 2 `
    -Message "Duplicate binding repair plan should keep the full duplicate member list."
$warning = @($summary.warnings)[0]
Assert-Equal -Actual ([string]$warning.source_schema) -Expected "featherdoc.content_control_data_binding_governance_report.v1" `
    -Message "Warnings should carry content-control governance source schema."
Assert-Equal -Actual ([string]$warning.action) -Expected "review_content_control_data_binding_evidence" `
    -Message "Warnings should carry reviewer action."
Assert-ContainsText -Text ([string]$warning.source_json_display) -ExpectedText "unrelated.json" `
    -Message "Warnings should keep source JSON display."
Assert-ContainsText -Text ([string]$warning.source_report) -ExpectedText "summary.json" `
    -Message "Warnings should carry governance summary source report."
Assert-ContainsText -Text ([string]$warning.source_report_display) -ExpectedText "report\summary.json" `
    -Message "Warning source report display should point at the governance summary."

$missingSyncDir = Join-Path $resolvedWorkingDir "missing-sync"
$missingSyncOutputDir = Join-Path $missingSyncDir "report"
$missingSyncResult = Invoke-Report -Arguments @(
    "-InputJson"
    $inspectPath
    "-OutputDir"
    $missingSyncOutputDir
)
Assert-Equal -Actual $missingSyncResult.ExitCode -Expected 0 `
    -Message "Content-control governance missing-sync run should pass. Output: $($missingSyncResult.Text)"
$missingSyncSummaryPath = Join-Path $missingSyncOutputDir "summary.json"
$missingSyncSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $missingSyncSummaryPath | ConvertFrom-Json
$missingSyncWarning = @($missingSyncSummary.warnings | Where-Object { [string]$_.id -eq "custom_xml_sync_evidence_missing" })[0]
Assert-ContainsText -Text ([string]$missingSyncWarning.source_json_display) -ExpectedText "inspect-content-controls.json" `
    -Message "Missing sync warnings should point source_json_display at the inspect evidence."
Assert-ContainsText -Text ([string]$missingSyncWarning.source_report_display) -ExpectedText "report\summary.json" `
    -Message "Missing sync warnings should keep source_report_display on the governance summary."
Assert-ContainsText -Text ([string]$missingSyncWarning.source_report) -ExpectedText "summary.json" `
    -Message "Missing sync warnings should keep source_report on the governance summary."
Assert-ContainsText -Text ([string]$missingSyncWarning.source_json) -ExpectedText "inspect-content-controls.json" `
    -Message "Missing sync warnings should preserve the inspect evidence source_json."

$emptyEvidenceDir = Join-Path $resolvedWorkingDir "empty-evidence"
$emptyEvidenceInputRoot = Join-Path $emptyEvidenceDir "empty-input"
$emptyEvidenceOutputDir = Join-Path $emptyEvidenceDir "report"
New-Item -ItemType Directory -Path $emptyEvidenceInputRoot -Force | Out-Null
$emptyEvidenceResult = Invoke-Report -Arguments @(
    "-InputRoot"
    $emptyEvidenceInputRoot
    "-OutputDir"
    $emptyEvidenceOutputDir
)
Assert-Equal -Actual $emptyEvidenceResult.ExitCode -Expected 0 `
    -Message "Content-control governance empty-evidence run should pass. Output: $($emptyEvidenceResult.Text)"
$emptyEvidenceSummaryPath = Join-Path $emptyEvidenceOutputDir "summary.json"
$emptyEvidenceSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $emptyEvidenceSummaryPath | ConvertFrom-Json
$emptyEvidenceWarning = @($emptyEvidenceSummary.warnings | Where-Object { [string]$_.id -eq "content_control_binding_evidence_missing" })[0]
Assert-True -Condition ($null -ne $emptyEvidenceWarning) `
    -Message "Empty evidence should emit content_control_binding_evidence_missing."
Assert-ContainsText -Text ([string]$emptyEvidenceWarning.source_report) -ExpectedText "summary.json" `
    -Message "Empty evidence warnings should keep source_report on the governance summary."
Assert-ContainsText -Text ([string]$emptyEvidenceWarning.source_report_display) -ExpectedText "report\summary.json" `
    -Message "Empty evidence warnings should keep source_report_display on the governance summary."
Assert-ContainsText -Text ([string]$emptyEvidenceWarning.source_json) -ExpectedText "summary.json" `
    -Message "Empty evidence warnings should keep source_json on the governance summary."
Assert-ContainsText -Text ([string]$emptyEvidenceWarning.source_json_display) -ExpectedText "report\summary.json" `
    -Message "Empty evidence warnings should keep source_json_display on the governance summary."

$markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
Assert-ContainsText -Text $markdown -ExpectedText "# Content Control Data Binding Governance" `
    -Message "Markdown should include title."
Assert-ContainsText -Text $markdown -ExpectedText "Repair plan schema: ``featherdoc.content_control_data_binding_repair_plan.v1``" `
    -Message "Markdown should expose repair plan schema."
Assert-ContainsText -Text $markdown -ExpectedText "Repair plan items: ``5``" `
    -Message "Markdown should expose repair plan item count."
Assert-ContainsText -Text $markdown -ExpectedText "Apply-supported repair plans: ``3``" `
    -Message "Markdown should expose apply-supported repair plan count."
Assert-ContainsText -Text $markdown -ExpectedText "Repair plans requiring user values: ``1``" `
    -Message "Markdown should expose repair plans requiring user-provided values."
Assert-ContainsText -Text $markdown -ExpectedText "Repair plans requiring visual verification: ``4``" `
    -Message "Markdown should expose repair plans requiring visual verification."
Assert-ContainsText -Text $markdown -ExpectedText "fix_custom_xml_data_binding_source" `
    -Message "Markdown should include remediation action."
Assert-ContainsText -Text $markdown -ExpectedText "command_template" `
    -Message "Markdown should include command templates."
Assert-ContainsText -Text $markdown -ExpectedText "review_lock_state" `
    -Message "Markdown should include repair strategies."
Assert-ContainsText -Text $markdown -ExpectedText "sync_bound_content_control" `
    -Message "Markdown should include bound content-control sync strategy."
Assert-ContainsText -Text $markdown -ExpectedText "sync-content-controls-from-custom-xml" `
    -Message "Markdown should include bound content-control sync command."
Assert-ContainsText -Text $markdown -ExpectedText "duplicate_binding_key" `
    -Message "Markdown should include duplicate binding evidence."
Assert-ContainsText -Text $markdown -ExpectedText "bound_placeholder" `
    -Message "Markdown should include explicit binding coverage statuses."
Assert-ContainsText -Text $markdown -ExpectedText "locked_bound" `
    -Message "Markdown should include locked binding coverage."
Assert-ContainsText -Text $markdown -ExpectedText "## Repair Plan Feasibility" `
    -Message "Markdown should include repair plan feasibility."
Assert-ContainsText -Text $markdown -ExpectedText "native_dry_run_supported" `
    -Message "Markdown should state native dry-run support."
Assert-ContainsText -Text $markdown -ExpectedText "requires_visual_verification" `
    -Message "Markdown should call out visual verification for apply paths."
Assert-ContainsText -Text $markdown -ExpectedText "open_command" `
    -Message "Markdown repair plans should include reviewer open commands when available."
Assert-ContainsText -Text $markdown -ExpectedText "## Release Blockers" `
    -Message "Markdown should include release blocker details."
Assert-ContainsText -Text $markdown -ExpectedText "content_control_data_binding.custom_xml_sync_issue" `
    -Message "Markdown should include Custom XML sync blockers."
Assert-ContainsText -Text $markdown -ExpectedText "content_control_data_binding.bound_placeholder" `
    -Message "Markdown should include bound placeholder blockers."
Assert-ContainsText -Text $markdown -ExpectedText "source_report_display" `
    -Message "Markdown should expose source report display fields."
Assert-ContainsText -Text $markdown -ExpectedText "source_json_display" `
    -Message "Markdown should expose source JSON display fields."

Write-Host "Content-control data-binding governance report regression passed."
