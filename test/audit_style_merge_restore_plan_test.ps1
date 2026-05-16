param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "clean", "issue")]
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
        '[{"issue":"missing_source_style","count":1}]'
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
Write-Output '{"command":"restore-style-merge","dry_run":true,"changed":false,"issue_count":__ISSUE_COUNT__,"issue_summary":__ISSUE_SUMMARY__,"restored_count":2,"restored_style_count":1,"restored_reference_count":4}'
exit 0
'@

    $mock = $mock.Replace("__ISSUE_COUNT__", [string]$IssueCount)
    $mock = $mock.Replace("__ISSUE_SUMMARY__", $issueSummary)
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
    Assert-Equal -Actual ([bool]$summary.dry_run) -Expected $true `
        -Message "Restore audit should run in dry-run mode."
    Assert-Equal -Actual ([int]$summary.issue_count) -Expected 0 `
        -Message "Clean restore audit should preserve zero issue count."
    Assert-Equal -Actual ([int]$summary.restored_count) -Expected 2 `
        -Message "Restore audit should preserve restored count."
    Assert-Equal -Actual ([int]$summary.restored_reference_count) -Expected 4 `
        -Message "Restore audit should preserve restored reference count."

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
    Assert-Equal -Actual ([int]$summary.issue_count) -Expected 1 `
        -Message "Issue restore audit should preserve issue count."
}

Write-Host "Style merge restore audit regression passed."
