param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir,
    [ValidateSet("all", "passing", "style_merge_review", "failing", "fail_on_issue")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Test-Scenario {
    param([string]$Name)
    return ($Scenario -eq "all" -or $Scenario -eq $Name)
}

function Invoke-GovernanceReportScript {
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

function Write-MockCli {
    param(
        [string]$Path,
        [switch]$FailAudit,
        [switch]$CleanAudit
    )

    $auditBlock = if ($FailAudit) {
        @'
    "audit-style-numbering" {
        Write-Output '{"command":"audit-style-numbering","error":"mock audit failure"}'
        exit 1
    }
'@
    } elseif ($CleanAudit) {
        @'
    "audit-style-numbering" {
        Write-Output '{"command":"audit-style-numbering","clean":true,"paragraph_style_count":3,"numbered_style_count":2,"issue_count":0,"suggestion_count":0,"styles":[],"issues":[],"suggestions":[]}'
        exit 0
    }
'@
    } else {
        @'
    "audit-style-numbering" {
        Write-Output '{"command":"audit-style-numbering","clean":false,"paragraph_style_count":3,"numbered_style_count":2,"issue_count":2,"suggestion_count":1,"styles":[],"issues":[{"issue":"missing_numbering_definition","style_id":"Heading1"},{"issue":"missing_numbering_definition","style_id":"Heading2"}],"suggestions":[{"command":"inspect-style-numbering"}]}'
        exit 0
    }
'@
    }

    $mock = @"
param([Parameter(ValueFromRemainingArguments = `$true)][string[]]`$Args)
`$command = `$Args[0]
switch (`$command) {
    "export-numbering-catalog" {
        `$outputIndex = [Array]::IndexOf(`$Args, "--output")
        if (`$outputIndex -lt 0 -or `$outputIndex + 1 -ge `$Args.Count) {
            Write-Output '{"command":"export-numbering-catalog","error":"missing output"}'
            exit 2
        }
        `$catalogPath = `$Args[`$outputIndex + 1]
        New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName(`$catalogPath)) -Force | Out-Null
        Set-Content -LiteralPath `$catalogPath -Encoding UTF8 -Value '{"definition_count":2,"instance_count":3,"definitions":[{"name":"HeadingOutline","levels":[],"instances":[]}]}'
        Write-Output ('{"command":"export-numbering-catalog","ok":true,"output_path":"' + (`$catalogPath -replace '\\','\\' -replace '"','\"') + '","definition_count":2,"instance_count":3}')
        exit 0
    }
    "inspect-styles" {
        Write-Output '{"command":"inspect-styles","style_count":3,"entries":[{"style":{"style_id":"Heading1"},"usage":{"style_id":"Heading1","paragraph_count":2,"run_count":0,"table_count":0,"total_count":2,"hits":[]}},{"style":{"style_id":"BodyText"},"usage":{"style_id":"BodyText","paragraph_count":1,"run_count":1,"table_count":1,"total_count":3,"hits":[]}}]}'
        exit 0
    }
    "suggest-style-merges" {
        `$outputIndex = [Array]::IndexOf(`$Args, "--output-plan")
        if (`$outputIndex -lt 0 -or `$outputIndex + 1 -ge `$Args.Count) {
            Write-Output '{"command":"suggest-style-merges","error":"missing output plan"}'
            exit 2
        }
        `$planPath = `$Args[`$outputIndex + 1]
        New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName(`$planPath)) -Force | Out-Null
        Set-Content -LiteralPath `$planPath -Encoding UTF8 -Value '{"command":"suggest-style-merges","clean":true,"operation_count":1,"applyable_count":1,"issue_count":0,"suggestion_confidence_summary":{"suggestion_count":1,"min_confidence":95,"max_confidence":95,"exact_xml_match_count":1,"xml_difference_count":0,"recommended_min_confidence":95,"recommendation":"review duplicate style merge suggestions before automation gates"},"operations":[{"action":"merge","source_style_id":"DuplicateBodyB","target_style_id":"DuplicateBodyA","applyable":true,"suggestion":{"reason_code":"matching_style_signature_and_xml","confidence":95}}]}'
        Write-Output '{"command":"suggest-style-merges","clean":true,"operation_count":1,"applyable_count":1,"issue_count":0,"suggestion_confidence_summary":{"suggestion_count":1,"min_confidence":95,"max_confidence":95,"exact_xml_match_count":1,"xml_difference_count":0,"recommended_min_confidence":95,"recommendation":"review duplicate style merge suggestions before automation gates"},"operations":[{"action":"merge","source_style_id":"DuplicateBodyB","target_style_id":"DuplicateBodyA","applyable":true,"suggestion":{"reason_code":"matching_style_signature_and_xml","confidence":95}}]}'
        exit 0
    }
$auditBlock
    default {
        Write-Output ('{"command":"' + `$command + '","error":"unexpected command"}')
        exit 2
    }
}
"@

    Set-Content -LiteralPath $Path -Encoding UTF8 -Value $mock
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_document_skeleton_governance_report.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "test\my_test.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$mockCli = Join-Path $resolvedWorkingDir "mock-featherdoc-cli.ps1"
$passingOutputDir = Join-Path $resolvedWorkingDir "passing-report"
Write-MockCli -Path $mockCli

if (Test-Scenario -Name "passing") {
    $passingResult = Invoke-GovernanceReportScript -Arguments @(
        "-InputDocx", $sampleDocx,
        "-OutputDir", $passingOutputDir,
        "-CliPath", $mockCli,
        "-SkipBuild"
    )
    Assert-Equal -Actual $passingResult.ExitCode -Expected 0 `
        -Message "Governance report should pass without -FailOnIssue even when audit has issues. Output: $($passingResult.Text)"
    Assert-ContainsText -Text $passingResult.Text -ExpectedText "Summary JSON:" `
        -Message "Governance report should print summary path."

    $summaryPath = Join-Path $passingOutputDir "document_skeleton_governance.summary.json"
    $markdownPath = Join-Path $passingOutputDir "document_skeleton_governance.md"
    $catalogPath = Join-Path $passingOutputDir "exemplar.numbering-catalog.json"
    $styleMergePlanPath = Join-Path $passingOutputDir "style-merge-suggestions.json"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Governance report should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Governance report should write Markdown report."
    Assert-True -Condition (Test-Path -LiteralPath $catalogPath) `
        -Message "Governance report should export the exemplar numbering catalog."
    Assert-True -Condition (Test-Path -LiteralPath $styleMergePlanPath) `
        -Message "Governance report should persist style merge suggestions."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.numbering_catalog.definition_count) -Expected 2 `
        -Message "Summary should include catalog definition count."
    Assert-Equal -Actual ([int]$summary.numbering_catalog.instance_count) -Expected 3 `
        -Message "Summary should include catalog instance count."
    Assert-Equal -Actual ([int]$summary.style_usage.usage_total) -Expected 5 `
        -Message "Summary should aggregate style usage totals."
    Assert-Equal -Actual ([int]$summary.style_numbering_audit.issue_count) -Expected 2 `
        -Message "Summary should include style-numbering audit issue count."
    Assert-Equal -Actual ([int]$summary.style_merge_suggestion_count) -Expected 1 `
        -Message "Summary should include style merge suggestion count."
    Assert-Equal -Actual ([int]$summary.style_merge_suggestion_confidence_summary.recommended_min_confidence) -Expected 95 `
        -Message "Summary should include style merge confidence summary."
    Assert-ContainsText -Text ([string]$summary.style_merge_suggestion_plan_relative_path) -ExpectedText "style-merge-suggestions.json" `
        -Message "Summary should expose the style merge plan path."
    Assert-Equal -Actual ([int]$summary.issue_summary[0].count) -Expected 2 `
        -Message "Summary should group style-numbering issues."
    Assert-Equal -Actual ([int]$summary.next_steps.Count) -Expected 5 `
        -Message "Summary should include suggested next-step commands."
    Assert-ContainsText -Text (($summary.next_steps | ForEach-Object { [string]$_.command }) -join "`n") -ExpectedText "repair-style-numbering" `
        -Message "Next steps should include repair preview command."
    Assert-ContainsText -Text (($summary.next_steps | ForEach-Object { [string]$_.id }) -join "`n") -ExpectedText "review_style_merge_suggestions" `
        -Message "Next steps should include style merge review command."
    Assert-ContainsText -Text (($summary.commands | ForEach-Object { [string]$_.command }) -join "`n") -ExpectedText "suggest-style-merges" `
        -Message "Summary should include suggest-style-merges command results."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "exemplar_catalog:" `
        -Message "Markdown report should show exemplar catalog path."
    Assert-ContainsText -Text $markdown -ExpectedText "style_merge_suggestion_count:" `
        -Message "Markdown report should show style merge suggestion count."
    Assert-ContainsText -Text $markdown -ExpectedText "Issue Summary" `
        -Message "Markdown report should include issue summary section."
    Assert-ContainsText -Text $markdown -ExpectedText "repair-style-numbering" `
        -Message "Markdown report should include suggested commands."
    Assert-ContainsText -Text $markdown -ExpectedText "suggest-style-merges" `
        -Message "Markdown report should include style merge suggestion commands."
}

if (Test-Scenario -Name "style_merge_review") {
    $reviewCli = Join-Path $resolvedWorkingDir "mock-featherdoc-cli-clean-audit.ps1"
    $reviewOutputDir = Join-Path $resolvedWorkingDir "style-merge-review-report"
    $reviewJsonPath = Join-Path $resolvedWorkingDir "style-merge-review.json"
    $reviewPlanPath = Join-Path $resolvedWorkingDir "style-merge-suggestions.reviewed.json"
    $reviewRollbackPlanPath = Join-Path $resolvedWorkingDir "style-merge.reviewed.rollback.json"
    Write-MockCli -Path $reviewCli -CleanAudit
    Set-Content -LiteralPath $reviewPlanPath -Encoding UTF8 -Value '{"command":"suggest-style-merges","operations":[{"action":"merge","source_style_id":"DuplicateBodyB","target_style_id":"DuplicateBodyA"}]}'
    Set-Content -LiteralPath $reviewRollbackPlanPath -Encoding UTF8 -Value '{"command":"apply-style-refactor","operations":[]}'
    Set-Content -LiteralPath $reviewJsonPath -Encoding UTF8 -Value (@{
            schema = "featherdoc.style_merge_suggestion_review.v1"
            decision = "Approved"
            reviewed_by = "release-reviewer"
            reviewed_at = "2026-05-16T08:00:00"
            reviewed_suggestion_count = 1
            plan_file = "style-merge-suggestions.reviewed.json"
            rollback_plan_file = "style-merge.reviewed.rollback.json"
        } | ConvertTo-Json -Depth 6)

    $reviewResult = Invoke-GovernanceReportScript -Arguments @(
        "-InputDocx", $sampleDocx,
        "-OutputDir", $reviewOutputDir,
        "-CliPath", $reviewCli,
        "-StyleMergeReviewJson", $reviewJsonPath,
        "-SkipBuild"
    )
    Assert-Equal -Actual $reviewResult.ExitCode -Expected 0 `
        -Message "Reviewed style merge suggestions should not keep the report in needs_review. Output: $($reviewResult.Text)"

    $summaryPath = Join-Path $reviewOutputDir "document_skeleton_governance.summary.json"
    $markdownPath = Join-Path $reviewOutputDir "document_skeleton_governance.md"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "clean" `
        -Message "Reviewed style merge suggestions should allow a clean skeleton governance status."
    Assert-Equal -Actual ([int]$summary.style_merge_suggestion_count) -Expected 1 `
        -Message "Summary should retain the original style merge suggestion count."
    Assert-Equal -Actual ([int]$summary.style_merge_suggestion_pending_count) -Expected 0 `
        -Message "Summary should clear pending style merge suggestions after full review."
    Assert-Equal -Actual ([string]$summary.style_merge_suggestion_review.status) -Expected "reviewed" `
        -Message "Summary should expose reviewed style merge status."
    Assert-Equal -Actual ([string]$summary.style_merge_suggestion_review.reviewed_by) -Expected "release-reviewer" `
        -Message "Summary should preserve style merge reviewer metadata."
    Assert-Equal -Actual ([bool]$summary.style_merge_suggestion_review.plan_exists) -Expected $true `
        -Message "Summary should verify the reviewed style merge plan path."
    Assert-ContainsText -Text ([string]$summary.style_merge_suggestion_review.plan_relative_path) -ExpectedText "style-merge-suggestions.reviewed.json" `
        -Message "Summary should expose the reviewed style merge plan path."
    Assert-Equal -Actual ([bool]$summary.style_merge_suggestion_review.rollback_plan_exists) -Expected $true `
        -Message "Summary should verify the reviewed style merge rollback plan path."
    Assert-ContainsText -Text ([string]$summary.style_merge_suggestion_review.rollback_plan_relative_path) -ExpectedText "style-merge.reviewed.rollback.json" `
        -Message "Summary should expose the reviewed style merge rollback plan path."
    Assert-True -Condition (-not ((@($summary.next_steps) | ForEach-Object { [string]$_.id }) -contains "review_style_merge_suggestions")) `
        -Message "Reviewed style merge suggestions should not emit a pending review next step."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "style_merge_suggestion_pending_count:" `
        -Message "Markdown should show pending style merge suggestion count."
    Assert-ContainsText -Text $markdown -ExpectedText "style_merge_review_status:" `
        -Message "Markdown should show style merge review status."
    Assert-ContainsText -Text $markdown -ExpectedText "style-merge-review.json" `
        -Message "Markdown should link the style merge review JSON."
    Assert-ContainsText -Text $markdown -ExpectedText "style_merge_review_plan:" `
        -Message "Markdown should link the reviewed style merge plan."
    Assert-ContainsText -Text $markdown -ExpectedText "style_merge_review_rollback_plan:" `
        -Message "Markdown should link the reviewed style merge rollback plan."
}

if (Test-Scenario -Name "failing") {
    $failingCli = Join-Path $resolvedWorkingDir "mock-featherdoc-cli-failing-audit.ps1"
    $failingOutputDir = Join-Path $resolvedWorkingDir "failing-report"
    Write-MockCli -Path $failingCli -FailAudit
    $failingResult = Invoke-GovernanceReportScript -Arguments @(
        "-InputDocx", $sampleDocx,
        "-OutputDir", $failingOutputDir,
        "-CliPath", $failingCli,
        "-SkipBuild"
    )
    Assert-Equal -Actual $failingResult.ExitCode -Expected 1 `
        -Message "Governance report should fail when a CLI command fails. Output: $($failingResult.Text)"

    $failingSummaryPath = Join-Path $failingOutputDir "document_skeleton_governance.summary.json"
    Assert-True -Condition (Test-Path -LiteralPath $failingSummaryPath) `
        -Message "Failing governance report should still write summary.json."
    $failingSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $failingSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([int]$failingSummary.command_failure_count) -Expected 1 `
        -Message "Failing summary should count the failed command."
    Assert-Equal -Actual ([string]$failingSummary.status) -Expected "failed" `
        -Message "Failing summary should set status=failed."
    $failingAuditCommand = @($failingSummary.commands | Where-Object { $_.command -eq "audit-style-numbering" } | Select-Object -First 1)
    Assert-True -Condition ($null -ne $failingAuditCommand) `
        -Message "Failing summary should include the audit-style-numbering command result."
    Assert-Equal -Actual ([int]$failingAuditCommand.exit_code) -Expected 1 `
        -Message "Failing summary should preserve the audit command exit code."
}

if (Test-Scenario -Name "fail_on_issue") {
    $failOnIssueOutputDir = Join-Path $resolvedWorkingDir "fail-on-issue-report"
    $failOnIssueResult = Invoke-GovernanceReportScript -Arguments @(
        "-InputDocx", $sampleDocx,
        "-OutputDir", $failOnIssueOutputDir,
        "-CliPath", $mockCli,
        "-SkipBuild",
        "-FailOnIssue"
    )
    Assert-Equal -Actual $failOnIssueResult.ExitCode -Expected 1 `
        -Message "Governance report should fail with -FailOnIssue when audit issues exist. Output: $($failOnIssueResult.Text)"
}

Write-Host "Document skeleton governance report regression passed."
