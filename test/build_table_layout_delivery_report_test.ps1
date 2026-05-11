param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir,
    [ValidateSet("all", "passing", "failing", "fail_on_issue")]
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

function Invoke-DeliveryReportScript {
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

function ConvertTo-JsonPathText {
    param([string]$Path)
    return ($Path -replace '\\', '\\' -replace '"', '\"')
}

function Write-MockCli {
    param(
        [string]$Path,
        [switch]$FailPlan
    )

    $positionPlanBlock = if ($FailPlan) {
        @'
    "plan-table-position-presets" {
        Write-Output '{"command":"plan-table-position-presets","error":"mock position plan failure"}'
        exit 1
    }
'@
    } else {
        @'
    "plan-table-position-presets" {
        $planIndex = [Array]::IndexOf($Args, "--output-plan")
        if ($planIndex -lt 0 -or $planIndex + 1 -ge $Args.Count) {
            Write-Output '{"command":"plan-table-position-presets","error":"missing output plan"}'
            exit 2
        }
        $planPath = $Args[$planIndex + 1]
        New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($planPath)) -Force | Out-Null
        Set-Content -LiteralPath $planPath -Encoding UTF8 -Value '{"command":"plan-table-position-presets","preset":"paragraph-callout","automatic_count":2,"review_count":1,"already_matching_count":0,"table_count":3}'
        Write-Output ('{"command":"plan-table-position-presets","ok":true,"preset":"paragraph-callout","automatic_count":2,"review_count":1,"already_matching_count":0,"output_plan":"' + (ConvertTo-JsonPathText $planPath) + '"}')
        exit 0
    }
'@
    }

    $mock = @"
param([Parameter(ValueFromRemainingArguments = `$true)][string[]]`$Args)
function ConvertTo-JsonPathText {
    param([string]`$Path)
    return (`$Path -replace '\\', '\\' -replace '"', '\"')
}
`$command = `$Args[0]
switch (`$command) {
    "audit-table-style-quality" {
        Write-Output '{"command":"audit-table-style-quality","ok":false,"clean":false,"issue_count":3,"issues":[{"kind":"missing_region"},{"kind":"bad_tblLook"}]}'
        exit 0
    }
    "plan-table-style-quality-fixes" {
        Write-Output '{"command":"plan-table-style-quality-fixes","ok":true,"automatic_fix_count":2,"manual_fix_count":1,"items":[{"automatic":true},{"automatic":false}]}'
        exit 0
    }
$positionPlanBlock
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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_table_layout_delivery_report.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "test\my_test.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$mockCli = Join-Path $resolvedWorkingDir "mock-featherdoc-cli.ps1"
$passingOutputDir = Join-Path $resolvedWorkingDir "passing-report"
Write-MockCli -Path $mockCli

if (Test-Scenario -Name "passing") {
    $passingResult = Invoke-DeliveryReportScript -Arguments @(
        "-InputDocx", $sampleDocx,
        "-OutputDir", $passingOutputDir,
        "-CliPath", $mockCli,
        "-SkipBuild"
    )
    Assert-Equal -Actual $passingResult.ExitCode -Expected 0 `
        -Message "Table layout report should pass without -FailOnIssue even when review work exists. Output: $($passingResult.Text)"
    Assert-ContainsText -Text $passingResult.Text -ExpectedText "Summary JSON:" `
        -Message "Table layout report should print summary path."

    $summaryPath = Join-Path $passingOutputDir "summary.json"
    $markdownPath = Join-Path $passingOutputDir "table_layout_delivery_report.md"
    $positionPlanPath = Join-Path $passingOutputDir "table-position-paragraph-callout.plan.json"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Table layout report should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Table layout report should write Markdown report."
    Assert-True -Condition (Test-Path -LiteralPath $positionPlanPath) `
        -Message "Table layout report should write the floating table position plan."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.table_layout_delivery_report.v1" `
        -Message "Summary should expose the report schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Summary should require review when table actions exist."
    Assert-Equal -Actual ([int]$summary.table_style_issue_count) -Expected 3 `
        -Message "Summary should include table style issue count."
    Assert-Equal -Actual ([int]$summary.automatic_tblLook_fix_count) -Expected 2 `
        -Message "Summary should include automatic tblLook fix count."
    Assert-Equal -Actual ([int]$summary.manual_table_style_fix_count) -Expected 1 `
        -Message "Summary should include manual table style fix count."
    Assert-Equal -Actual ([int]$summary.table_position_automatic_count) -Expected 2 `
        -Message "Summary should include automatic table position count."
    Assert-Equal -Actual ([int]$summary.table_position_review_count) -Expected 1 `
        -Message "Summary should include table position review count."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
        -Message "Summary should expose both manual style and positioned-table review blockers."
    Assert-True -Condition (@($summary.action_items).Count -ge 4) `
        -Message "Summary should expose actionable next steps."
    Assert-ContainsText -Text ([string]$summary.action_items[1].command) -ExpectedText "apply-table-style-quality-fixes" `
        -Message "Action items should include safe tblLook application command."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Table Layout Delivery Report" `
        -Message "Markdown report should include a title."
    Assert-ContainsText -Text $markdown -ExpectedText "Suggested Actions" `
        -Message "Markdown report should include suggested actions."
    Assert-ContainsText -Text $markdown -ExpectedText "apply-table-position-plan" `
        -Message "Markdown report should include table position dry-run command."
}

if (Test-Scenario -Name "failing") {
    $failingCli = Join-Path $resolvedWorkingDir "mock-featherdoc-cli-failing-plan.ps1"
    $failingOutputDir = Join-Path $resolvedWorkingDir "failing-report"
    Write-MockCli -Path $failingCli -FailPlan
    $failingResult = Invoke-DeliveryReportScript -Arguments @(
        "-InputDocx", $sampleDocx,
        "-OutputDir", $failingOutputDir,
        "-CliPath", $failingCli,
        "-SkipBuild"
    )
    Assert-Equal -Actual $failingResult.ExitCode -Expected 1 `
        -Message "Table layout report should fail when a CLI command fails. Output: $($failingResult.Text)"

    $failingSummaryPath = Join-Path $failingOutputDir "summary.json"
    Assert-True -Condition (Test-Path -LiteralPath $failingSummaryPath) `
        -Message "Failing table layout report should still write summary.json."
    $failingSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $failingSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([int]$failingSummary.command_failure_count) -Expected 1 `
        -Message "Failing summary should count the failed command."
    Assert-Equal -Actual ([string]$failingSummary.status) -Expected "failed" `
        -Message "Failing summary should expose failed status."
    Assert-Equal -Actual ([int]$failingSummary.commands[2].exit_code) -Expected 1 `
        -Message "Failing summary should preserve the position plan exit code."
}

if (Test-Scenario -Name "fail_on_issue") {
    $failOnIssueOutputDir = Join-Path $resolvedWorkingDir "fail-on-issue-report"
    $failOnIssueResult = Invoke-DeliveryReportScript -Arguments @(
        "-InputDocx", $sampleDocx,
        "-OutputDir", $failOnIssueOutputDir,
        "-CliPath", $mockCli,
        "-SkipBuild",
        "-FailOnIssue"
    )
    Assert-Equal -Actual $failOnIssueResult.ExitCode -Expected 1 `
        -Message "Table layout report should fail with -FailOnIssue when review blockers exist. Output: $($failOnIssueResult.Text)"
}

Write-Host "Table layout delivery report regression passed."
