param(
    [string]$RepoRoot,
    [string]$WorkingDir
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

function Convert-ToStableDateText {
    param($Value)

    if ($Value -is [System.DateTime]) {
        return $Value.ToString("s", [System.Globalization.CultureInfo]::InvariantCulture)
    }

    return [string]$Value
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
$smokeSummaryDir = Join-Path $resolvedWorkingDir "smoke"
$releaseSummaryDir = Join-Path $resolvedWorkingDir "release"
New-Item -ItemType Directory -Path $smokeSummaryDir -Force | Out-Null
New-Item -ItemType Directory -Path $releaseSummaryDir -Force | Out-Null
$smokeSummaryPath = Join-Path $smokeSummaryDir "summary.json"
$releaseSummaryPath = Join-Path $releaseSummaryDir "summary.json"
$ignoredJsonPath = Join-Path $resolvedWorkingDir "schema_patch_approval_result.json"
$outputJsonPath = Join-Path $resolvedWorkingDir "schema-approval-history.json"
$outputMarkdownPath = Join-Path $resolvedWorkingDir "schema-approval-history.md"
$listOutputJsonPath = Join-Path $resolvedWorkingDir "schema-approval-history-list.json"
$listOutputMarkdownPath = Join-Path $resolvedWorkingDir "schema-approval-history-list.md"
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
        schema_patch_review_count = 1
        schema_patch_review_changed_count = 1
        schema_patch_approval_pending_count = 0
        schema_patch_approval_approved_count = 1
        schema_patch_approval_rejected_count = 0
        schema_patch_approval_compliance_issue_count = 0
        schema_patch_approval_invalid_result_count = 0
        schema_patch_approval_gate_blocked = $false
        schema_patch_approval_items = @(
            [ordered]@{
                name = "schema-review-invalid"
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
([ordered]@{ decision = "approved"; reviewer = "alice" } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath $ignoredJsonPath -Encoding UTF8

$result = Invoke-ScriptAndCapture -ScriptPath $scriptPath -Arguments @(
    "-SummaryJsonDir", $resolvedWorkingDir,
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
Assert-Equal -Actual ([string]$history.schema) -Expected "featherdoc.project_template_schema_approval_history.v1" `
    -Message "History JSON should expose the stable schema id."
Assert-Equal -Actual ([int]$history.summary_count) -Expected 2 `
    -Message "History should include both source summaries."
Assert-Equal -Actual ([int]$history.blocked_run_count) -Expected 1 `
    -Message "History should count blocked runs."
Assert-Equal -Actual ([int]$history.passed_run_count) -Expected 1 `
    -Message "History should count passed runs."
Assert-Equal -Actual ([int]$history.total_schema_patch_review_changed_count) -Expected 3 `
    -Message "History should sum changed review counts."
Assert-Equal -Actual ([int]$history.total_schema_patch_approval_item_count) -Expected 3 `
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
Assert-True -Condition (@($entryHistory.issue_keys) -contains "missing_reviewer") `
    -Message "Entry history should preserve historical issue keys."

$passedRun = $history.runs | Where-Object { $_.schema_patch_approval_gate_status -eq "passed" } | Select-Object -First 1
Assert-Equal -Actual ([string]$passedRun.source_kind) -Expected "release_candidate_summary" `
    -Message "Release summary should be classified as release_candidate_summary."
Assert-Equal -Actual ([string]$passedRun.execution_status) -Expected "pass" `
    -Message "Release run should preserve execution status."
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
Assert-ContainsText -Text $markdown -ExpectedText "## Entry History" `
    -Message "Markdown should include entry history section."
Assert-ContainsText -Text $markdown -ExpectedText "schema-review-invalid: latest=approved/approved runs=2 blocked_runs=1" `
    -Message "Markdown should summarize entry-level approval history."

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

Write-Host "Project template schema approval history regression passed."
