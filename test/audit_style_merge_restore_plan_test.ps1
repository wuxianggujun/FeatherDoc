param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "clean", "issue")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\audit_style_merge_restore_plan_test"
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

function Invoke-AuditScript {
    param([string[]]$Arguments)

    $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
    $powerShellPath = if ($powerShellCommand) { $powerShellCommand.Source } else { (Get-Process -Id $PID).Path }
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
        $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Write-MockCli {
    param([string]$Path, [int]$IssueCount)

    $issueSummary = if ($IssueCount -eq 0) {
        "[]"
    } else {
        '[{"code":"missing_source_style","count":1},{"issue":"missing_source_style","count":2}]'
    }
    $operations = if ($IssueCount -eq 0) {
        '[{"action":"merge","source_style_id":"OldBody","target_style_id":"Normal","restorable":true,"restored":true,"style_restored":true,"restored_reference_count":4,"issues":[]}]'
    } else {
        '[{"action":"merge","source_style_id":"MissingBody","target_style_id":"Normal","restorable":false,"restored":false,"style_restored":false,"restored_reference_count":0,"issues":[{"code":"missing_source_style","message":"source style is missing","suggestion":"Review this rollback entry."},{"code":"missing_source_style","message":"source style is still missing","suggestion":"Review this rollback entry again."}]}]'
    }

    $mock = @'
param([Parameter(ValueFromRemainingArguments = $true)][string[]]$CliArgs)

function Has-Argument {
    param([string]$Name)
    return ([Array]::IndexOf($CliArgs, $Name) -ge 0)
}

function Get-ArgumentValue {
    param([string]$Name)

    $index = [Array]::IndexOf($CliArgs, $Name)
    if ($index -lt 0 -or $index + 1 -ge $CliArgs.Count) {
        return ""
    }
    return $CliArgs[$index + 1]
}

$command = $CliArgs[0]
if ($command -ne "restore-style-merge") {
    Write-Output ('{"command":"' + $command + '","error":"unexpected command"}')
    exit 2
}
if (-not (Has-Argument -Name "--dry-run") -or (Has-Argument -Name "--output")) {
    Write-Output '{"command":"restore-style-merge","error":"expected dry-run without output"}'
    exit 2
}
$rollbackPath = Get-ArgumentValue -Name "--rollback-plan"
if ([string]::IsNullOrWhiteSpace($rollbackPath)) {
    Write-Output '{"command":"restore-style-merge","error":"missing rollback plan"}'
    exit 2
}

$argsPath = Join-Path ([System.IO.Path]::GetDirectoryName($MyInvocation.MyCommand.Path)) "mock-cli-args.json"
(ConvertTo-Json -InputObject @($CliArgs) -Depth 8) | Set-Content -LiteralPath $argsPath -Encoding UTF8
Write-Output '{"command":"restore-style-merge","dry_run":true,"changed":false,"requested_count":3,"issue_count":__ISSUE_COUNT__,"issue_summary":__ISSUE_SUMMARY__,"restored_count":2,"restored_style_count":1,"restored_reference_count":4,"operations":__OPERATIONS__}'
exit 0
'@

    $mock = $mock.Replace("__ISSUE_COUNT__", [string]$IssueCount)
    $mock = $mock.Replace("__ISSUE_SUMMARY__", $issueSummary)
    $mock = $mock.Replace("__OPERATIONS__", $operations)
    Set-Content -LiteralPath $Path -Encoding UTF8 -Value $mock
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\audit_style_merge_restore_plan.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "clean") {
    $scenarioDir = Join-Path $resolvedWorkingDir "clean"
    $inputPath = Join-Path $scenarioDir "merged.docx"
    $rollbackPath = Join-Path $scenarioDir "style-merge.apply.rollback.json"
    $applySummaryPath = Join-Path $scenarioDir "style-merge.apply.summary.json"
    $summaryPath = Join-Path $scenarioDir "style-merge.restore-audit.summary.json"
    $mockCliPath = Join-Path $scenarioDir "mock-featherdoc-cli.ps1"
    $mockArgsPath = Join-Path $scenarioDir "mock-cli-args.json"

    New-Item -ItemType Directory -Path $scenarioDir -Force | Out-Null
    Set-Content -LiteralPath $inputPath -Encoding UTF8 -Value "mock merged docx"
    Write-JsonFile -Path $rollbackPath -Value ([ordered]@{
        command = "apply-style-refactor"
        rollback_count = 2
        rollback_operations = @(
            [ordered]@{
                action = "rename"
                source_style_id = "RenamedHeading"
                target_style_id = "OldHeading"
                automatic = $true
                restorable = $true
            },
            [ordered]@{
                action = "merge"
                source_style_id = "OldBody"
                target_style_id = "Normal"
                automatic = $false
                restorable = $true
            },
            [ordered]@{
                action = "merge"
                source_style_id = "ArchiveBody"
                target_style_id = "Normal"
                automatic = $false
                restorable = $true
            },
            [ordered]@{
                action = "merge"
                source_style_id = "BrokenBody"
                target_style_id = "Normal"
                automatic = $false
                restorable = $false
            }
        )
    })
    Write-JsonFile -Path $applySummaryPath -Value ([ordered]@{
        schema = "featherdoc.reviewed_style_merge_apply.v1"
        output_docx_relative_path = "merged.docx"
        rollback_plan_relative_path = "style-merge.apply.rollback.json"
    })
    Write-MockCli -Path $mockCliPath -IssueCount 0

    $result = Invoke-AuditScript -Arguments @(
        "-ApplySummaryJson", $applySummaryPath,
        "-SummaryJson", $summaryPath,
        "-CliPath", $mockCliPath,
        "-Entry", 0,
        "-SourceStyle", "OldBody",
        "-TargetStyle", "Normal",
        "-SkipBuild"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Clean restore audit should succeed. Output: $($result.Text)"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Clean restore audit should write a summary JSON."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.style_merge_restore_audit.v1" `
        -Message "Restore audit summary should expose a stable schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "clean" `
        -Message "Clean restore audit should report clean status."
    Assert-Equal -Actual ([string]$summary.status_reason) -Expected "style_merge_restore_audit_clean" `
        -Message "Clean restore audit should expose a stable status reason."
    Assert-Equal -Actual ([bool]$summary.dry_run) -Expected $true `
        -Message "Restore audit should run in dry-run mode."
    Assert-Equal -Actual ([int]$summary.issue_count) -Expected 0 `
        -Message "Clean restore audit should preserve zero issue count."
    Assert-Equal -Actual ([int]$summary.issue_review_group_count) -Expected 0 `
        -Message "Clean restore audit should not emit issue review groups."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Clean restore audit should not emit release blockers."
    Assert-Equal -Actual (@($summary.release_blockers).Count) -Expected 0 `
        -Message "Clean restore audit release blocker list should be empty."
    Assert-Equal -Actual (@($summary.action_items).Count) -Expected 1 `
        -Message "Clean restore audit should emit a visual review action item."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 1 `
        -Message "Clean restore audit should expose action item count."
    $actionItem = @($summary.action_items)[0]
    Assert-Equal -Actual ([string]$actionItem.action) -Expected "review_style_merge_restore_audit" `
        -Message "Restore audit action item should expose a stable action."
    Assert-Equal -Actual ([string]$summary.minimum_risk_next_action) -Expected "prepare_word_visual_review" `
        -Message "Clean restore audit should recommend the visual review next action."
    Assert-ContainsText -Text ([string]$summary.minimum_risk_next_action_command) -ExpectedText "prepare_word_review_task.ps1" `
        -Message "Clean restore audit should expose the visual review next command."
    Assert-Equal -Actual ([string]$actionItem.minimum_risk_next_action) -Expected "prepare_word_visual_review" `
        -Message "Clean restore audit action item should preserve the minimum-risk next action."
    Assert-ContainsText -Text ([string]$actionItem.minimum_risk_next_action_command) -ExpectedText "prepare_word_review_task.ps1" `
        -Message "Clean restore audit action item should preserve the minimum-risk next command."
    Assert-Equal -Actual ([string]$actionItem.source_schema) -Expected "featherdoc.style_merge_restore_audit.v1" `
        -Message "Restore audit action item should expose release governance source schema."
    Assert-ContainsText -Text ([string]$actionItem.source_report_display) -ExpectedText "style-merge.restore-audit.summary.json" `
        -Message "Restore audit action item should point at the source report display path."
    Assert-ContainsText -Text ([string]$actionItem.source_json_display) -ExpectedText "style-merge.restore-audit.summary.json" `
        -Message "Restore audit action item should point at the source JSON display path."
    Assert-ContainsText -Text ([string]$actionItem.rollback_plan_display) -ExpectedText "style-merge.apply.rollback.json" `
        -Message "Restore audit action item should preserve the rollback plan display path."
    Assert-Equal -Actual ([int]$actionItem.review_handoff_step_count) -Expected 3 `
        -Message "Restore audit action item should expose reviewer handoff step count."
    Assert-Equal -Actual ([string]$actionItem.review_handoff_steps[1].id) -Expected "prepare_word_visual_review" `
        -Message "Restore audit action item should preserve reviewer handoff steps."
    Assert-ContainsText -Text ([string]$summary.visual_review_command) -ExpectedText "prepare_word_review_task.ps1" `
        -Message "Restore audit should expose a Word visual review command."
    Assert-ContainsText -Text ([string]$summary.visual_review_command) -ExpectedText "-DocumentSourceKind style-merge-restore-audit" `
        -Message "Restore audit visual command should write a source-specific latest pointer."
    Assert-ContainsText -Text ([string]$summary.open_visual_review_command) -ExpectedText "-SourceKind style-merge-restore-audit" `
        -Message "Restore audit should expose a source-specific open-latest command."
    Assert-ContainsText -Text ([string]$actionItem.command) -ExpectedText "merged.docx" `
        -Message "Restore audit visual action should target the merged DOCX."
    Assert-ContainsText -Text ([string]$actionItem.open_command) -ExpectedText "open_latest_word_review_task.ps1" `
        -Message "Restore audit action item should preserve the open-latest command."
    Assert-Equal -Actual ([int]$summary.restored_count) -Expected 2 `
        -Message "Restore audit should preserve restored count."
    Assert-Equal -Actual ([int]$summary.requested_count) -Expected 3 `
        -Message "Restore audit should preserve requested count."
    Assert-Equal -Actual ([int]$summary.selection_summary.entry_filter_count) -Expected 1 `
        -Message "Restore audit should summarize entry filter count."
    Assert-Equal -Actual ([int]$summary.selection_summary.source_style_filter_count) -Expected 1 `
        -Message "Restore audit should summarize source style filter count."
    Assert-Equal -Actual ([int]$summary.selection_summary.target_style_filter_count) -Expected 1 `
        -Message "Restore audit should summarize target style filter count."
    Assert-Equal -Actual ([bool]$summary.selection_summary.has_entry_filter) -Expected $true `
        -Message "Restore audit should mark entry filter usage."
    Assert-Equal -Actual ([int]$summary.selection_summary.requested_count) -Expected 3 `
        -Message "Restore audit selection summary should preserve requested count."
    Assert-Equal -Actual ([int]$summary.restored_reference_count) -Expected 4 `
        -Message "Restore audit should preserve restored reference count."
    Assert-Equal -Actual ([int]$summary.review_handoff_step_count) -Expected 3 `
        -Message "Clean restore audit should expose three reviewer handoff steps."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[0].id) -Expected "review_restore_audit_dry_run" `
        -Message "Clean restore audit handoff should start from the completed dry-run review."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[1].id) -Expected "prepare_word_visual_review" `
        -Message "Clean restore audit handoff should route the reviewer to visual review next."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[1].source_schema) -Expected "featherdoc.style_merge_restore_audit.v1" `
        -Message "Clean restore audit handoff step should expose source schema."
    Assert-ContainsText -Text ([string]$summary.review_handoff_steps[1].source_json_display) -ExpectedText "style-merge.restore-audit.summary.json" `
        -Message "Clean restore audit handoff step should expose source JSON display path."
    Assert-ContainsText -Text ([string]$summary.review_handoff_steps[1].rollback_plan_display) -ExpectedText "style-merge.apply.rollback.json" `
        -Message "Clean restore audit handoff step should expose rollback plan display path."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[1].status) -Expected "next" `
        -Message "Clean restore audit visual review handoff should be the next step."
    Assert-ContainsText -Text ([string]$summary.review_handoff_steps[1].command) -ExpectedText "prepare_word_review_task.ps1" `
        -Message "Clean restore audit visual review handoff should expose the review command."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[2].id) -Expected "restore_selected_style_merges" `
        -Message "Clean restore audit handoff should still expose the selected restore step."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[2].status) -Expected "ready" `
        -Message "Clean restore audit selected restore step should be ready."
    Assert-ContainsText -Text ([string]$summary.review_handoff_steps[2].command_template) -ExpectedText "--output <output.docx>" `
        -Message "Clean restore audit selected restore step should expose an output template."
    Assert-ContainsText -Text ([string]$summary.review_handoff_steps[2].batch_restore_command_template) -ExpectedText "--entry 1" `
        -Message "Clean restore audit handoff should expose the batch restore template."
    Assert-ContainsText -Text ([string]$summary.selected_restore_command_template) -ExpectedText "--entry 0" `
        -Message "Restore audit should expose the selected restore command template."
    Assert-ContainsText -Text ([string]$summary.selected_restore_command_template) -ExpectedText "--source-style OldBody" `
        -Message "Restore audit selected restore command should preserve source filters."
    Assert-ContainsText -Text ([string]$summary.selected_restore_command_template) -ExpectedText "--target-style Normal" `
        -Message "Restore audit selected restore command should preserve target filters."
    Assert-ContainsText -Text ([string]$summary.selected_restore_command_template) -ExpectedText "--output <output.docx>" `
        -Message "Restore audit selected restore command should include an output template."
    Assert-Equal -Actual ([int]$summary.restorable_rollback_entry_count) -Expected 2 `
        -Message "Restore audit should count restorable merge rollback entries."
    Assert-ContainsText -Text ((@($summary.restorable_rollback_entry_indexes) | ForEach-Object { [string]$_ }) -join ",") -ExpectedText "1,2" `
        -Message "Restore audit should list restorable merge rollback entry indexes."
    Assert-Equal -Actual (@($summary.per_entry_dry_run_commands).Count) -Expected 2 `
        -Message "Restore audit should expose per-entry dry-run commands."
    Assert-ContainsText -Text ([string]$summary.per_entry_dry_run_commands[0].command) -ExpectedText "--entry 1" `
        -Message "Restore audit per-entry dry-run command should select the first merge entry."
    Assert-ContainsText -Text ([string]$summary.per_entry_dry_run_commands[0].command) -ExpectedText "--dry-run" `
        -Message "Restore audit per-entry dry-run command should remain read-only."
    Assert-Equal -Actual (@($summary.per_entry_restore_command_templates).Count) -Expected 2 `
        -Message "Restore audit should expose per-entry restore command templates."
    Assert-ContainsText -Text ([string]$summary.per_entry_restore_command_templates[1].command_template) -ExpectedText "--entry 2" `
        -Message "Restore audit per-entry restore template should select the second merge entry."
    Assert-ContainsText -Text ([string]$summary.per_entry_restore_command_templates[1].command_template) -ExpectedText "--output <output.docx>" `
        -Message "Restore audit per-entry restore template should include an output placeholder."
    Assert-ContainsText -Text ([string]$summary.batch_restorable_dry_run_command) -ExpectedText "--entry 1" `
        -Message "Restore audit batch dry-run command should include the first restorable entry."
    Assert-ContainsText -Text ([string]$summary.batch_restorable_dry_run_command) -ExpectedText "--entry 2" `
        -Message "Restore audit batch dry-run command should include the second restorable entry."
    Assert-ContainsText -Text ([string]$summary.batch_restorable_restore_command_template) -ExpectedText "--output <output.docx>" `
        -Message "Restore audit batch restore template should include an output placeholder."

    $mockArgs = @((Get-Content -Raw -Encoding UTF8 -LiteralPath $mockArgsPath | ConvertFrom-Json) |
        ForEach-Object { $_ })
    Assert-Equal -Actual ([string]$mockArgs[0]) -Expected "restore-style-merge" `
        -Message "Restore audit should call restore-style-merge."
    Assert-ContainsText -Text ($mockArgs -join "`n") -ExpectedText "--dry-run" `
        -Message "Restore audit should force dry-run mode."
    Assert-ContainsText -Text ($mockArgs -join "`n") -ExpectedText "--entry" `
        -Message "Restore audit should pass entry selection."
    Assert-ContainsText -Text ($mockArgs -join "`n") -ExpectedText "OldBody" `
        -Message "Restore audit should pass source style selection."
}

if (Test-Scenario -Name "issue") {
    $scenarioDir = Join-Path $resolvedWorkingDir "issue"
    $inputPath = Join-Path $scenarioDir "merged.docx"
    $rollbackPath = Join-Path $scenarioDir "style-merge.apply.rollback.json"
    $summaryPath = Join-Path $scenarioDir "style-merge.restore-audit.summary.json"
    $mockCliPath = Join-Path $scenarioDir "mock-featherdoc-cli.ps1"

    New-Item -ItemType Directory -Path $scenarioDir -Force | Out-Null
    Set-Content -LiteralPath $inputPath -Encoding UTF8 -Value "mock merged docx"
    Write-JsonFile -Path $rollbackPath -Value ([ordered]@{
        command = "apply-style-refactor"
        rollback_count = 1
        rollback_operations = @(
            [ordered]@{
                action = "merge"
                source_style_id = "MissingBody"
                target_style_id = "Normal"
                automatic = $false
                restorable = $true
            }
        )
    })
    Write-MockCli -Path $mockCliPath -IssueCount 1

    $result = Invoke-AuditScript -Arguments @(
        "-InputDocx", $inputPath,
        "-RollbackPlan", $rollbackPath,
        "-SummaryJson", $summaryPath,
        "-CliPath", $mockCliPath,
        "-FailOnIssue",
        "-SkipBuild"
    )
    Assert-True -Condition ($result.ExitCode -ne 0) `
        -Message "Restore audit should fail when -FailOnIssue sees dry-run issues."
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Restore audit should write summary before failing on issues."
    Assert-ContainsText -Text $result.Text -ExpectedText "found 1 issue" `
        -Message "Restore audit failure should explain issue count."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Issue restore audit should report needs_review status."
    Assert-Equal -Actual ([string]$summary.status_reason) -Expected "style_merge_restore_audit_issues" `
        -Message "Issue restore audit should expose a stable status reason."
    Assert-Equal -Actual ([int]$summary.issue_count) -Expected 1 `
        -Message "Issue restore audit should preserve issue count."
    Assert-Equal -Actual ([string]$summary.minimum_risk_next_action) -Expected "review_style_merge_restore_audit_issues" `
        -Message "Issue restore audit should recommend issue review as the next action."
    Assert-ContainsText -Text ([string]$summary.minimum_risk_next_action_command) -ExpectedText "restore-style-merge" `
        -Message "Issue restore audit should expose the dry-run command as the next action command."
    Assert-Equal -Actual ([int]$summary.issue_summary_group_count) -Expected 1 `
        -Message "Issue restore audit should group issue summary codes."
    Assert-Equal -Actual ([string]$summary.issue_summary_groups[0].code) -Expected "missing_source_style" `
        -Message "Issue restore audit should prefer code and fall back to issue keys."
    Assert-Equal -Actual ([int]$summary.issue_summary_groups[0].count) -Expected 3 `
        -Message "Issue restore audit should aggregate duplicate issue summary counts."
    Assert-Equal -Actual ([int]$summary.review_handoff_step_count) -Expected 3 `
        -Message "Issue restore audit should expose three reviewer handoff steps."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[1].id) -Expected "review_issue_groups" `
        -Message "Issue restore audit handoff should route reviewer to issue groups."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[1].source_schema) -Expected "featherdoc.style_merge_restore_audit.v1" `
        -Message "Issue restore audit handoff step should expose source schema."
    Assert-ContainsText -Text ([string]$summary.review_handoff_steps[1].source_report_display) -ExpectedText "style-merge.restore-audit.summary.json" `
        -Message "Issue restore audit handoff step should expose source report display path."
    Assert-ContainsText -Text ([string]$summary.review_handoff_steps[1].rollback_plan_display) -ExpectedText "style-merge.apply.rollback.json" `
        -Message "Issue restore audit handoff step should expose rollback plan display path."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[1].status) -Expected "next" `
        -Message "Issue restore audit issue group handoff should be the next step."
    Assert-Equal -Actual ([int]$summary.review_handoff_steps[1].issue_group_count) -Expected 1 `
        -Message "Issue restore audit handoff should preserve issue group count."
    Assert-ContainsText -Text ((@($summary.review_handoff_steps[1].issue_review_commands) | ForEach-Object { [string]$_ }) -join "`n") -ExpectedText "--source-style MissingBody" `
        -Message "Issue restore audit handoff should expose issue-specific review commands."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[2].id) -Expected "prepare_word_visual_review_after_clean_audit" `
        -Message "Issue restore audit handoff should keep visual review blocked until audit is clean."
    Assert-Equal -Actual ([string]$summary.review_handoff_steps[2].status) -Expected "blocked" `
        -Message "Issue restore audit visual review step should be blocked."
    Assert-ContainsText -Text ((@($summary.review_handoff_steps[2].blocked_by) | ForEach-Object { [string]$_ }) -join "`n") -ExpectedText "style_merge.restore_audit_issues" `
        -Message "Issue restore audit visual review blocker should point at restore audit issues."
    Assert-Equal -Actual ([int]$summary.issue_review_group_count) -Expected 1 `
        -Message "Issue restore audit should group affected operations by issue code."
    $issueReviewGroup = @($summary.issue_review_groups)[0]
    Assert-Equal -Actual ([string]$issueReviewGroup.code) -Expected "missing_source_style" `
        -Message "Issue restore audit review group should expose the issue code."
    Assert-Equal -Actual ([int]$issueReviewGroup.count) -Expected 2 `
        -Message "Issue restore audit review group should preserve per-operation issue count."
    Assert-Equal -Actual ([int]$issueReviewGroup.affected_operation_count) -Expected 1 `
        -Message "Issue restore audit review group should deduplicate affected operations."
    Assert-Equal -Actual ([string]$issueReviewGroup.affected_operations[0].source_style_id) -Expected "MissingBody" `
        -Message "Issue restore audit review group should expose the source style id."
    Assert-Equal -Actual ([string]$issueReviewGroup.affected_operations[0].target_style_id) -Expected "Normal" `
        -Message "Issue restore audit review group should expose the target style id."
    Assert-ContainsText -Text ([string]$issueReviewGroup.affected_operations[0].review_command) -ExpectedText "--source-style MissingBody" `
        -Message "Issue restore audit review group should expose a source-style filtered command."
    Assert-ContainsText -Text ([string]$issueReviewGroup.affected_operations[0].review_command) -ExpectedText "--target-style Normal" `
        -Message "Issue restore audit review group should expose a target-style filtered command."
    Assert-ContainsText -Text ((@($issueReviewGroup.review_commands) | ForEach-Object { [string]$_ }) -join "`n") -ExpectedText "restore-style-merge" `
        -Message "Issue restore audit review group should list review commands."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 1 `
        -Message "Issue restore audit should emit one release blocker."
    $blocker = @($summary.release_blockers)[0]
    Assert-Equal -Actual ([string]$blocker.id) -Expected "style_merge.restore_audit_issues" `
        -Message "Issue restore audit blocker should expose a stable id."
    Assert-Equal -Actual ([string]$blocker.action) -Expected "review_style_merge_restore_audit" `
        -Message "Issue restore audit blocker should point to the restore review action."
    Assert-Equal -Actual ([string]$blocker.source_schema) -Expected "featherdoc.style_merge_restore_audit.v1" `
        -Message "Issue restore audit blocker should expose release governance source schema."
    Assert-ContainsText -Text ([string]$blocker.source_report_display) -ExpectedText "style-merge.restore-audit.summary.json" `
        -Message "Issue restore audit blocker should point at the source report display path."
    Assert-ContainsText -Text ([string]$blocker.source_json_display) -ExpectedText "style-merge.restore-audit.summary.json" `
        -Message "Issue restore audit blocker should point at the source JSON display path."
    Assert-ContainsText -Text ([string]$blocker.rollback_plan_display) -ExpectedText "style-merge.apply.rollback.json" `
        -Message "Issue restore audit blocker should preserve the rollback plan display path."
    Assert-Equal -Actual ([int]$blocker.review_handoff_step_count) -Expected 3 `
        -Message "Issue restore audit blocker should expose reviewer handoff step count."
    Assert-Equal -Actual ([string]$blocker.review_handoff_steps[1].id) -Expected "review_issue_groups" `
        -Message "Issue restore audit blocker should preserve issue review handoff steps."
    Assert-ContainsText -Text ((@($blocker.issue_keys) | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "style_merge_restore_audit_issues" `
        -Message "Issue restore audit blocker should expose stable issue keys."
    Assert-Equal -Actual ([string]$blocker.repair_strategy) -Expected "review_style_merge_restore_audit" `
        -Message "Issue restore audit blocker should expose a repair strategy."
    Assert-ContainsText -Text ([string]$blocker.repair_hint) -ExpectedText "issue_summary_groups" `
        -Message "Issue restore audit blocker should explain where to review grouped issues."
    Assert-ContainsText -Text ([string]$blocker.command) -ExpectedText "restore-style-merge" `
        -Message "Issue restore audit blocker should preserve the dry-run command."
    Assert-ContainsText -Text ([string]$blocker.open_command) -ExpectedText "open_latest_word_review_task.ps1" `
        -Message "Issue restore audit blocker should preserve the open-latest review command."
    Assert-ContainsText -Text ([string]$blocker.message) -ExpectedText "1 issue" `
        -Message "Issue restore audit blocker should explain the issue count."
    Assert-ContainsText -Text ((@($summary.action_items) | ForEach-Object { [string]$_.action }) -join "`n") `
        -ExpectedText "review_style_merge_restore_audit" `
        -Message "Issue restore audit should keep the restore review action item."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 1 `
        -Message "Issue restore audit should expose action item count."
    $issueActionItem = @($summary.action_items)[0]
    Assert-Equal -Actual ([int]$issueActionItem.review_handoff_step_count) -Expected 3 `
        -Message "Issue restore audit action item should expose reviewer handoff step count."
    Assert-Equal -Actual ([string]$issueActionItem.review_handoff_steps[1].id) -Expected "review_issue_groups" `
        -Message "Issue restore audit action item should preserve issue review handoff steps."
}

Write-Host "Style merge restore audit regression passed."
