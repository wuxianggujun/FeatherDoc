param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\write_project_template_schema_approval_history_test"
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

function Assert-NotContainsText {
    param([string]$Text, [string]$UnexpectedText, [string]$Message)
    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw "$Message Unexpected='$UnexpectedText'."
    }
}

function Assert-TextOrder {
    param([string]$Text, [string[]]$ExpectedTexts, [string]$Message)

    $lastIndex = -1
    foreach ($expectedText in $ExpectedTexts) {
        $currentIndex = $Text.IndexOf($expectedText, [System.StringComparison]::Ordinal)
        if ($currentIndex -lt 0) {
            throw "$Message Missing='$expectedText'."
        }
        if ($currentIndex -le $lastIndex) {
            throw "$Message OutOfOrder='$expectedText'."
        }

        $lastIndex = $currentIndex
    }
}

function Convert-ToStableDateText {
    param($Value)

    if ($Value -is [System.DateTime]) {
        return $Value.ToString("s", [System.Globalization.CultureInfo]::InvariantCulture)
    }

    return [string]$Value
}

function Convert-ToRepoDisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Invoke-ScriptAndCapture {
    param([string]$ScriptPath, [string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    $output = @(& $powerShellPath -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\write_project_template_schema_approval_history.ps1"
$primaryFixtureDir = Join-Path $resolvedWorkingDir "primary-fixtures"
$smokeSummaryDir = Join-Path $primaryFixtureDir "smoke"
$releaseSummaryDir = Join-Path $primaryFixtureDir "release"
$unrelatedSummaryDir = Join-Path $primaryFixtureDir "unrelated"
New-Item -ItemType Directory -Path $smokeSummaryDir -Force | Out-Null
New-Item -ItemType Directory -Path $releaseSummaryDir -Force | Out-Null
New-Item -ItemType Directory -Path $unrelatedSummaryDir -Force | Out-Null
$smokeSummaryPath = Join-Path $smokeSummaryDir "summary.json"
$releaseSummaryPath = Join-Path $releaseSummaryDir "summary.json"
$unrelatedSummaryPath = Join-Path $unrelatedSummaryDir "summary.json"
$ignoredJsonPath = Join-Path $resolvedWorkingDir "schema_patch_approval_result.json"
$outputJsonPath = Join-Path $resolvedWorkingDir "schema-approval-history.json"
$outputMarkdownPath = Join-Path $resolvedWorkingDir "schema-approval-history.md"
$listOutputJsonPath = Join-Path $resolvedWorkingDir "schema-approval-history-list.json"
$listOutputMarkdownPath = Join-Path $resolvedWorkingDir "schema-approval-history-list.md"
$arrayOutputJsonPath = Join-Path $resolvedWorkingDir "schema-approval-history-array.json"
$arrayOutputMarkdownPath = Join-Path $resolvedWorkingDir "schema-approval-history-array.md"
$reviewStateFixtureDir = Join-Path $resolvedWorkingDir "review-state-fixture"
$reviewStateSummaryPath = Join-Path $reviewStateFixtureDir "summary.json"
$reviewStateOutputJsonPath = Join-Path $resolvedWorkingDir "schema-approval-history-review-state.json"
$reviewStateOutputMarkdownPath = Join-Path $resolvedWorkingDir "schema-approval-history-review-state.md"
$dirModeFixtureDir = Join-Path $resolvedWorkingDir "dir-mode"
$dirModeNestedSummaryDir = Join-Path $dirModeFixtureDir "nested"
$dirModeSummaryPath = Join-Path $dirModeFixtureDir "summary.json"
$dirModeNestedSummaryPath = Join-Path $dirModeNestedSummaryDir "summary.json"
$dirOutputJsonPath = Join-Path $resolvedWorkingDir "schema-approval-history-dir.json"
$dirOutputMarkdownPath = Join-Path $resolvedWorkingDir "schema-approval-history-dir.md"
$approvalResultPath = Join-Path $resolvedWorkingDir "schema_patch_approval_invalid_result.json"

$smokeSummary = [ordered]@{
    generated_at = "2026-04-28T10:00:00"
    manifest_path = Join-Path $resolvedWorkingDir "project_template_smoke.manifest.json"
    output_dir = $resolvedWorkingDir
    schema_patch_review_count = 2
    schema_patch_review_changed_count = 2
    schema_patch_approval_pending_count = 1
    schema_patch_approval_approved_count = 0
    schema_patch_approval_rejected_count = 1
    schema_patch_approval_compliance_issue_count = 2
    schema_patch_approval_invalid_result_count = 1
    schema_patch_approval_gate_status = "blocked"
    schema_patch_approval_gate_blocked = $true
    schema_patch_approval_items = @(
        [ordered]@{
            name = "schema-review-invalid"
            project_id = "project-finance"
            template_name = "invoice-template"
            candidate_type = "update"
            status = "invalid_result"
            decision = "needs_changes"
            action = "fix_schema_patch_approval_result"
            compliance_issue_count = 2
            compliance_issues = @("missing_reviewer", "missing_reviewed_at")
            approval_result = $approvalResultPath
        }
    )
}
($smokeSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $smokeSummaryPath -Encoding UTF8

$releaseSummary = [ordered]@{
    generated_at = "2026-04-29T10:00:00"
    execution_status = "pass"
    steps = [ordered]@{
        project_template_smoke = [ordered]@{
            status = "completed"
            schema_patch_approval_gate_status = "passed"
        }
    }
    project_template_smoke = [ordered]@{
        summary_json = $smokeSummaryPath
        schema_patch_review_count = 2
        schema_patch_review_changed_count = 2
        schema_patch_approval_pending_count = 0
        schema_patch_approval_approved_count = 2
        schema_patch_approval_rejected_count = 0
        schema_patch_approval_compliance_issue_count = 0
        schema_patch_approval_invalid_result_count = 0
        schema_patch_approval_gate_blocked = $false
        schema_patch_approval_items = @(
            [ordered]@{
                name = "schema-review-invalid"
                project_id = "project-finance"
                template_name = "invoice-template"
                candidate_type = "update"
                status = "approved"
                decision = "approved"
                action = "promote_schema_update_candidate"
                compliance_issue_count = 0
                compliance_issues = @()
            },
            [ordered]@{
                name = "schema-review-contract"
                project_id = "project-legal"
                template_name = "contract-template"
                candidate_type = "add"
                status = "approved"
                decision = "approved"
                action = "promote_schema_update_candidate"
                compliance_issue_count = 0
                compliance_issues = @()
            }
        )
    }
}
($releaseSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8
([ordered]@{
    generated_at = "2026-04-30T10:00:00"
    execution_status = "pass"
    steps = [ordered]@{
        unrelated_governance_report = [ordered]@{
            status = "completed"
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $unrelatedSummaryPath -Encoding UTF8
([ordered]@{ decision = "approved"; reviewer = "alice" } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath $ignoredJsonPath -Encoding UTF8

$result = Invoke-ScriptAndCapture -ScriptPath $scriptPath -Arguments @(
    "-SummaryJsonDir", $primaryFixtureDir,
    "-Recurse",
    "-OutputJson", $outputJsonPath,
    "-OutputMarkdown", $outputMarkdownPath
)
Assert-Equal -Actual $result.ExitCode -Expected 0 `
    -Message "Schema approval history writer should succeed. Output: $($result.Text)"
Assert-True -Condition (Test-Path -LiteralPath $outputJsonPath) `
    -Message "History JSON should be written."
Assert-True -Condition (Test-Path -LiteralPath $outputMarkdownPath) `
    -Message "History Markdown should be written."

$history = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputJsonPath | ConvertFrom-Json
$expectedSmokeSummaryDisplay = Convert-ToRepoDisplayPath -RepoRoot $resolvedRepoRoot -Path $smokeSummaryPath
$expectedReleaseSummaryDisplay = Convert-ToRepoDisplayPath -RepoRoot $resolvedRepoRoot -Path $releaseSummaryPath
Assert-Equal -Actual ([string]$history.schema) -Expected "featherdoc.project_template_schema_approval_history.v1" `
    -Message "History JSON should expose the stable schema id."
Assert-Equal -Actual ([int]$history.summary_count) -Expected 2 `
    -Message "History should include project-template summaries and ignore unrelated directory summaries."
Assert-Equal -Actual ([int]$history.blocked_run_count) -Expected 1 `
    -Message "History should count blocked runs."
Assert-Equal -Actual ([int]$history.passed_run_count) -Expected 1 `
    -Message "History should count passed runs."
Assert-Equal -Actual ([int]$history.not_required_run_count) -Expected 0 `
    -Message "Unrelated directory summaries should not be counted as not_required runs."
Assert-Equal -Actual ([int]$history.total_schema_patch_review_changed_count) -Expected 4 `
    -Message "History should sum changed review counts."
Assert-Equal -Actual ([int]$history.total_schema_patch_approval_item_count) -Expected 4 `
    -Message "History should sum approval item counts from rollup counts and item arrays."
Assert-Equal -Actual ([int]$history.total_schema_patch_approval_compliance_issue_count) -Expected 2 `
    -Message "History should sum compliance issue counts."
Assert-Equal -Actual ([int]$history.total_schema_patch_approval_invalid_result_count) -Expected 1 `
    -Message "History should sum invalid approval-result counts."
Assert-Equal -Actual ([string]$history.latest_gate_status) -Expected "passed" `
    -Message "History should expose the latest run gate status."

$blockedRun = $history.runs | Where-Object { $_.schema_patch_approval_gate_status -eq "blocked" } | Select-Object -First 1
Assert-Equal -Actual ([string]$blockedRun.source_kind) -Expected "project_template_smoke_summary" `
    -Message "Smoke summary should be classified as project_template_smoke_summary."
Assert-Equal -Actual ([int]$blockedRun.blocked_items.Count) -Expected 1 `
    -Message "Blocked run should list invalid approval items."
Assert-Equal -Actual ([string]$blockedRun.blocked_items[0].name) -Expected "schema-review-invalid" `
    -Message "Blocked item should preserve its entry name."
Assert-True -Condition (@($blockedRun.blocked_items[0].compliance_issues) -contains "missing_reviewer") `
    -Message "Blocked item should preserve compliance issues."

Assert-Equal -Actual ([string]$history.latest_blocking_summary.status) -Expected "blocked" `
    -Message "History should expose the latest blocked run summary."
Assert-Equal -Actual (Convert-ToStableDateText -Value $history.latest_blocking_summary.generated_at) -Expected "2026-04-28T10:00:00" `
    -Message "Latest blocking summary should point to the most recent blocked run."
Assert-Equal -Actual ([int]$history.latest_blocking_summary.blocked_item_count) -Expected 1 `
    -Message "Latest blocking summary should count blocked approval items."
Assert-Equal -Actual ([string]$history.latest_blocking_summary.summary_json_display) -Expected $expectedSmokeSummaryDisplay `
    -Message "Latest blocking summary should expose the stable source summary display path."
Assert-True -Condition (@($history.latest_blocking_summary.issue_keys) -contains "missing_reviewed_at") `
    -Message "Latest blocking summary should aggregate compliance issue keys."

$entryHistory = $history.entry_histories | Where-Object { $_.name -eq "schema-review-invalid" } | Select-Object -First 1
Assert-Equal -Actual ([int]$entryHistory.run_count) -Expected 2 `
    -Message "Entry history should include both blocked and approved runs."
Assert-Equal -Actual ([int]$entryHistory.blocked_run_count) -Expected 1 `
    -Message "Entry history should count blocked runs."
Assert-Equal -Actual ([int]$entryHistory.approved_run_count) -Expected 1 `
    -Message "Entry history should count approved decisions."
Assert-Equal -Actual ([int]$entryHistory.needs_changes_run_count) -Expected 1 `
    -Message "Entry history should count needs_changes decisions."
Assert-Equal -Actual ([string]$entryHistory.latest_status) -Expected "approved" `
    -Message "Entry history should expose the latest item status."
Assert-Equal -Actual ([string]$entryHistory.latest_decision) -Expected "approved" `
    -Message "Entry history should expose the latest item decision."
Assert-Equal -Actual ([string]$entryHistory.latest_summary_json_display) -Expected $expectedReleaseSummaryDisplay `
    -Message "Entry history should expose the latest source summary display path."
Assert-True -Condition (@($entryHistory.issue_keys) -contains "missing_reviewer") `
    -Message "Entry history should preserve historical issue keys."
Assert-Equal -Actual ([string]$entryHistory.project_id) -Expected "project-finance" `
    -Message "Entry history should preserve project id metadata."
Assert-Equal -Actual ([string]$entryHistory.template_name) -Expected "invoice-template" `
    -Message "Entry history should preserve template name metadata."
Assert-Equal -Actual ([string]$entryHistory.template_scope) -Expected "project-finance/invoice-template" `
    -Message "Entry history should expose a stable project/template scope."

$contractEntryHistory = $history.entry_histories | Where-Object { $_.name -eq "schema-review-contract" } | Select-Object -First 1
Assert-Equal -Actual ([string]$contractEntryHistory.project_id) -Expected "project-legal" `
    -Message "Entry history should preserve project metadata for additional templates."
Assert-Equal -Actual ([string]$contractEntryHistory.template_scope) -Expected "project-legal/contract-template" `
    -Message "Entry history should expose additional template scopes."

Assert-Equal -Actual ([int]$history.project_template_approval_matrix_template_count) -Expected 2 `
    -Message "Approval review matrix should count project/template scopes."
Assert-Equal -Actual ([int]$history.project_template_approval_matrix_project_count) -Expected 2 `
    -Message "Approval review matrix should count projects with metadata."
Assert-Equal -Actual ([int]$history.project_template_approval_matrix_latest_approved_template_count) -Expected 2 `
    -Message "Approval review matrix should count latest approved templates."
Assert-Equal -Actual ([int]$history.project_template_approval_matrix_latest_blocked_template_count) -Expected 0 `
    -Message "Approval review matrix should not treat historically fixed templates as currently blocked."
Assert-Equal -Actual ([int]$history.project_template_approval_matrix_missing_project_metadata_count) -Expected 0 `
    -Message "Approval review matrix should expose missing project metadata counts."

$invoiceMatrix = $history.project_template_approval_matrix | Where-Object { $_.template_scope -eq "project-finance/invoice-template" } | Select-Object -First 1
Assert-Equal -Actual ([string]$invoiceMatrix.latest_review_state) -Expected "approved" `
    -Message "Approval review matrix should expose the latest invoice template state."
Assert-Equal -Actual ([int]$invoiceMatrix.run_count) -Expected 2 `
    -Message "Approval review matrix should count invoice approval runs."
Assert-Equal -Actual ([int]$invoiceMatrix.historical_blocked_run_count) -Expected 1 `
    -Message "Approval review matrix should preserve historical blocked run counts."
Assert-Equal -Actual ([bool]$invoiceMatrix.requires_reviewer_action) -Expected $false `
    -Message "Approval review matrix should not require action after latest approval."

$contractMatrix = $history.project_template_approval_matrix | Where-Object { $_.template_scope -eq "project-legal/contract-template" } | Select-Object -First 1
Assert-Equal -Actual ([string]$contractMatrix.latest_review_state) -Expected "approved" `
    -Message "Approval review matrix should expose the latest contract template state."
Assert-Equal -Actual ([int]$contractMatrix.run_count) -Expected 1 `
    -Message "Approval review matrix should count contract approval runs."

$passedRun = $history.runs | Where-Object { $_.schema_patch_approval_gate_status -eq "passed" } | Select-Object -First 1
Assert-Equal -Actual ([string]$passedRun.source_kind) -Expected "release_candidate_summary" `
    -Message "Release summary should be classified as release_candidate_summary."
Assert-Equal -Actual ([string]$passedRun.execution_status) -Expected "pass" `
    -Message "Release run should preserve execution status."
Assert-Equal -Actual ([string]$passedRun.summary_json_display) -Expected $expectedReleaseSummaryDisplay `
    -Message "Release run should expose the stable source summary display path."
Assert-Equal -Actual ([string]$passedRun.approval_items[0].name) -Expected "schema-review-invalid" `
    -Message "Release run should preserve approval item snapshots."

$markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputMarkdownPath
Assert-ContainsText -Text $markdown -ExpectedText "Project Template Schema Approval History" `
    -Message "Markdown should include the report heading."
Assert-ContainsText -Text $markdown -ExpectedText "Latest gate status: passed" `
    -Message "Markdown should include latest gate status."
Assert-ContainsText -Text $markdown -ExpectedText "gate=blocked" `
    -Message "Markdown should include blocked run summary."
Assert-ContainsText -Text $markdown -ExpectedText "schema-review-invalid" `
    -Message "Markdown should include blocked approval item names."
Assert-ContainsText -Text $markdown -ExpectedText "missing_reviewer,missing_reviewed_at" `
    -Message "Markdown should include blocked approval item issues."
Assert-ContainsText -Text $markdown -ExpectedText "## Latest Blocking Summary" `
    -Message "Markdown should include latest blocking summary section."
Assert-ContainsText -Text $markdown -ExpectedText "Latest blocking summary: 2026-04-28T10:00:00" `
    -Message "Markdown should include the latest blocked run timestamp."
Assert-ContainsText -Text $markdown -ExpectedText "summary=$expectedSmokeSummaryDisplay" `
    -Message "Markdown should include the latest blocked run source summary display path."
Assert-ContainsText -Text $markdown -ExpectedText "## Project Template Approval Matrix" `
    -Message "Markdown should include the project/template approval matrix section."
Assert-ContainsText -Text $markdown -ExpectedText "project-finance/invoice-template: state=approved project=project-finance template=invoice-template" `
    -Message "Markdown should summarize invoice approval matrix status."
Assert-ContainsText -Text $markdown -ExpectedText "project-legal/contract-template: state=approved project=project-legal template=contract-template" `
    -Message "Markdown should summarize contract approval matrix status."
Assert-ContainsText -Text $markdown -ExpectedText "## Entry History" `
    -Message "Markdown should include entry history section."
Assert-ContainsText -Text $markdown -ExpectedText "schema-review-invalid: latest=approved/approved runs=2 blocked_runs=1" `
    -Message "Markdown should summarize entry-level approval history."
Assert-ContainsText -Text $markdown -ExpectedText "summary=$expectedReleaseSummaryDisplay" `
    -Message "Markdown should include the latest entry source summary display path."
Assert-TextOrder -Text $markdown -ExpectedTexts @(
    "## Latest Blocking Summary",
    "## Project Template Approval Matrix",
    "## Entry History",
    "## Runs",
    "## Blocked Approval Items"
) -Message "Markdown should keep reviewer sections in a stable order."

$listResult = Invoke-ScriptAndCapture -ScriptPath $scriptPath -Arguments @(
    "-SummaryJson", "$smokeSummaryPath,$releaseSummaryPath",
    "-OutputJson", $listOutputJsonPath,
    "-OutputMarkdown", $listOutputMarkdownPath
)
Assert-Equal -Actual $listResult.ExitCode -Expected 0 `
    -Message "Schema approval history writer should accept comma-separated SummaryJson. Output: $($listResult.Text)"
$listHistory = Get-Content -Raw -Encoding UTF8 -LiteralPath $listOutputJsonPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$listHistory.summary_count) -Expected 2 `
    -Message "Comma-separated SummaryJson should include both source summaries."

$arrayResult = Invoke-ScriptAndCapture -ScriptPath $scriptPath -Arguments @(
    "-SummaryJson", $smokeSummaryPath, $releaseSummaryPath,
    "-OutputJson", $arrayOutputJsonPath,
    "-OutputMarkdown", $arrayOutputMarkdownPath
)
Assert-Equal -Actual $arrayResult.ExitCode -Expected 0 `
    -Message "Schema approval history writer should accept array SummaryJson values. Output: $($arrayResult.Text)"
$arrayHistory = Get-Content -Raw -Encoding UTF8 -LiteralPath $arrayOutputJsonPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$arrayHistory.summary_count) -Expected 2 `
    -Message "Array SummaryJson should include both source summaries."
Assert-Equal -Actual ([string]$arrayHistory.latest_gate_status) -Expected "passed" `
    -Message "Array SummaryJson should preserve latest run ordering."
Assert-Equal -Actual ([string]$arrayHistory.latest_blocking_summary.summary_json_display) -Expected $expectedSmokeSummaryDisplay `
    -Message "Array SummaryJson should preserve blocking source summary display paths."
Assert-True -Condition (@($arrayHistory.runs | ForEach-Object { [string]$_.summary_json_display }) -contains $expectedSmokeSummaryDisplay) `
    -Message "Array SummaryJson should include the smoke summary source."
Assert-True -Condition (@($arrayHistory.runs | ForEach-Object { [string]$_.summary_json_display }) -contains $expectedReleaseSummaryDisplay) `
    -Message "Array SummaryJson should include the release summary source."

New-Item -ItemType Directory -Path $reviewStateFixtureDir -Force | Out-Null
$reviewStateSummary = [ordered]@{
    generated_at = "2026-05-03T10:00:00"
    schema_patch_review_count = 2
    schema_patch_review_changed_count = 2
    schema_patch_approval_pending_count = 1
    schema_patch_approval_approved_count = 0
    schema_patch_approval_rejected_count = 1
    schema_patch_approval_compliance_issue_count = 0
    schema_patch_approval_invalid_result_count = 0
    schema_patch_approval_gate_status = "pending"
    schema_patch_approval_gate_blocked = $false
    schema_patch_approval_items = @(
        [ordered]@{
            name = "schema-review-policy-pending"
            project_id = "project-policy"
            template_name = "policy-template"
            candidate_type = "update"
            status = "pending_review"
            decision = "pending"
            action = "review_schema_update_candidate"
            compliance_issue_count = 0
            compliance_issues = @()
        },
        [ordered]@{
            name = "schema-review-notice-rejected"
            project_id = "project-notice"
            template_name = "notice-template"
            candidate_type = "update"
            status = "rejected"
            decision = "rejected"
            action = "revise_schema_update_candidate"
            compliance_issue_count = 0
            compliance_issues = @()
        }
    )
}
($reviewStateSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $reviewStateSummaryPath -Encoding UTF8
$reviewStateResult = Invoke-ScriptAndCapture -ScriptPath $scriptPath -Arguments @(
    "-SummaryJson", $reviewStateSummaryPath,
    "-OutputJson", $reviewStateOutputJsonPath,
    "-OutputMarkdown", $reviewStateOutputMarkdownPath
)
Assert-Equal -Actual $reviewStateResult.ExitCode -Expected 0 `
    -Message "Schema approval history writer should expose reviewer action guidance for pending/rejected matrix rows. Output: $($reviewStateResult.Text)"
$reviewStateHistory = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewStateOutputJsonPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$reviewStateHistory.project_template_approval_matrix_latest_pending_template_count) -Expected 1 `
    -Message "Approval matrix should count currently pending template scopes."
Assert-Equal -Actual ([int]$reviewStateHistory.project_template_approval_matrix_latest_rejected_template_count) -Expected 1 `
    -Message "Approval matrix should count currently rejected template scopes."

$pendingMatrix = $reviewStateHistory.project_template_approval_matrix | Where-Object { $_.template_scope -eq "project-policy/policy-template" } | Select-Object -First 1
Assert-Equal -Actual ([string]$pendingMatrix.latest_review_state) -Expected "pending" `
    -Message "Approval matrix should preserve the pending state for reviewer action routing."
Assert-Equal -Actual ([bool]$pendingMatrix.requires_reviewer_action) -Expected $true `
    -Message "Pending matrix rows should require reviewer action."
Assert-Equal -Actual ([string]$pendingMatrix.reviewer_action_summary) -Expected "review_schema_update_candidate" `
    -Message "Pending matrix rows should expose the source reviewer action."
Assert-True -Condition (@($pendingMatrix.reviewer_actions) -contains "review_schema_update_candidate") `
    -Message "Pending matrix rows should expose reviewer action arrays."
Assert-ContainsText -Text ([string]$pendingMatrix.reviewer_action_reason) -ExpectedText "latest_review_state=pending" `
    -Message "Pending matrix rows should explain why reviewer action is required."

$rejectedMatrix = $reviewStateHistory.project_template_approval_matrix | Where-Object { $_.template_scope -eq "project-notice/notice-template" } | Select-Object -First 1
Assert-Equal -Actual ([string]$rejectedMatrix.latest_review_state) -Expected "rejected" `
    -Message "Approval matrix should preserve the rejected state for reviewer action routing."
Assert-Equal -Actual ([bool]$rejectedMatrix.requires_reviewer_action) -Expected $true `
    -Message "Rejected matrix rows should require reviewer action."
Assert-Equal -Actual ([string]$rejectedMatrix.reviewer_action_summary) -Expected "revise_schema_update_candidate" `
    -Message "Rejected matrix rows should expose the source reviewer action."

$reviewStateMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewStateOutputMarkdownPath
Assert-ContainsText -Text $reviewStateMarkdown -ExpectedText "project-policy/policy-template: state=pending" `
    -Message "Markdown matrix should include pending template scope."
Assert-ContainsText -Text $reviewStateMarkdown -ExpectedText "reviewer_action=review_schema_update_candidate" `
    -Message "Markdown matrix should include pending reviewer action guidance."
Assert-ContainsText -Text $reviewStateMarkdown -ExpectedText "project-notice/notice-template: state=rejected" `
    -Message "Markdown matrix should include rejected template scope."
Assert-ContainsText -Text $reviewStateMarkdown -ExpectedText "reviewer_action=revise_schema_update_candidate" `
    -Message "Markdown matrix should include rejected reviewer action guidance."

New-Item -ItemType Directory -Path $dirModeNestedSummaryDir -Force | Out-Null
$dirModeSummary = [ordered]@{
    generated_at = "2026-05-01T10:00:00"
    schema_patch_review_count = 1
    schema_patch_review_changed_count = 1
    schema_patch_approval_pending_count = 0
    schema_patch_approval_approved_count = 0
    schema_patch_approval_rejected_count = 1
    schema_patch_approval_compliance_issue_count = 1
    schema_patch_approval_invalid_result_count = 1
    schema_patch_approval_gate_status = "blocked"
    schema_patch_approval_gate_blocked = $true
    schema_patch_approval_items = @(
        [ordered]@{
            name = "dir-top-only"
            status = "invalid_result"
            decision = "needs_changes"
            action = "fix_dir_top_schema_approval"
            compliance_issue_count = 1
            compliance_issues = @("dir_top_issue")
        }
    )
}
($dirModeSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $dirModeSummaryPath -Encoding UTF8
$dirModeNestedSummary = [ordered]@{
    generated_at = "2026-05-02T10:00:00"
    schema_patch_review_count = 1
    schema_patch_review_changed_count = 1
    schema_patch_approval_pending_count = 0
    schema_patch_approval_approved_count = 1
    schema_patch_approval_rejected_count = 0
    schema_patch_approval_compliance_issue_count = 0
    schema_patch_approval_invalid_result_count = 0
    schema_patch_approval_gate_status = "passed"
    schema_patch_approval_gate_blocked = $false
    schema_patch_approval_items = @(
        [ordered]@{
            name = "dir-nested-should-not-load"
            status = "approved"
            decision = "approved"
            action = "promote_nested_schema_update"
            compliance_issue_count = 0
            compliance_issues = @()
        }
    )
}
($dirModeNestedSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $dirModeNestedSummaryPath -Encoding UTF8

$dirResult = Invoke-ScriptAndCapture -ScriptPath $scriptPath -Arguments @(
    "-SummaryJsonDir", $dirModeFixtureDir,
    "-OutputJson", $dirOutputJsonPath,
    "-OutputMarkdown", $dirOutputMarkdownPath
)
Assert-Equal -Actual $dirResult.ExitCode -Expected 0 `
    -Message "Schema approval history writer should accept non-recursive SummaryJsonDir. Output: $($dirResult.Text)"
$dirHistory = Get-Content -Raw -Encoding UTF8 -LiteralPath $dirOutputJsonPath | ConvertFrom-Json
$expectedDirModeSummaryDisplay = Convert-ToRepoDisplayPath -RepoRoot $resolvedRepoRoot -Path $dirModeSummaryPath
Assert-Equal -Actual ([int]$dirHistory.summary_count) -Expected 1 `
    -Message "Non-recursive SummaryJsonDir should only collect top-level summary.json."
Assert-Equal -Actual ([string]$dirHistory.latest_gate_status) -Expected "blocked" `
    -Message "Non-recursive SummaryJsonDir should ignore nested summaries unless -Recurse is set."
Assert-Equal -Actual ([string]@($dirHistory.runs)[0].summary_json_display) -Expected $expectedDirModeSummaryDisplay `
    -Message "Non-recursive SummaryJsonDir should preserve the top-level source summary display path."
$dirMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $dirOutputMarkdownPath
Assert-ContainsText -Text $dirMarkdown -ExpectedText "dir-top-only" `
    -Message "Non-recursive SummaryJsonDir Markdown should include top-level approval items."
Assert-ContainsText -Text $dirMarkdown -ExpectedText "summary=$expectedDirModeSummaryDisplay" `
    -Message "Non-recursive SummaryJsonDir Markdown should include the top-level source summary display path."
Assert-NotContainsText -Text $dirMarkdown -UnexpectedText "dir-nested-should-not-load" `
    -Message "Non-recursive SummaryJsonDir should not include nested approval items."

Write-Host "Project template schema approval history regression passed."
