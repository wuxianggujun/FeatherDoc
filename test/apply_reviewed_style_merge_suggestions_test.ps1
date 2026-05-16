param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "valid", "pending_review", "partial_review")]
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

function Invoke-ApplyScript {
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
    param([string]$Path)

    $mock = @'
param([Parameter(ValueFromRemainingArguments = $true)][string[]]$CliArgs)

function Get-ArgumentValue {
    param([string]$Name)

    $index = [Array]::IndexOf($CliArgs, $Name)
    if ($index -lt 0 -or $index + 1 -ge $CliArgs.Count) {
        return ""
    }
    return $CliArgs[$index + 1]
}

$command = $CliArgs[0]
if ($command -ne "apply-style-refactor") {
    Write-Output ('{"command":"' + $command + '","error":"unexpected command"}')
    exit 2
}

$planPath = Get-ArgumentValue -Name "--plan-file"
$rollbackPath = Get-ArgumentValue -Name "--rollback-plan"
$outputPath = Get-ArgumentValue -Name "--output"
if ([string]::IsNullOrWhiteSpace($planPath) -or
    [string]::IsNullOrWhiteSpace($rollbackPath) -or
    [string]::IsNullOrWhiteSpace($outputPath)) {
    Write-Output '{"command":"apply-style-refactor","error":"missing required paths"}'
    exit 2
}

New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($outputPath)) -Force | Out-Null
New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($rollbackPath)) -Force | Out-Null
Set-Content -LiteralPath $outputPath -Encoding UTF8 -Value "mock docx"
Set-Content -LiteralPath $rollbackPath -Encoding UTF8 -Value '{"command":"apply-style-refactor","rollback_count":2,"restorable":true}'

$argsPath = Join-Path ([System.IO.Path]::GetDirectoryName($MyInvocation.MyCommand.Path)) "mock-cli-args.json"
(ConvertTo-Json -InputObject @($CliArgs) -Depth 8) | Set-Content -LiteralPath $argsPath -Encoding UTF8
Write-Output '{"command":"apply-style-refactor","ok":true,"changed":true,"requested_count":2,"applied_count":2,"skipped_count":0,"rollback_count":2,"clean":true}'
exit 0
'@

    Set-Content -LiteralPath $Path -Encoding UTF8 -Value $mock
}

function Write-Plan {
    param([string]$Path, [int]$SuggestionCount)

    $operations = for ($index = 0; $index -lt $SuggestionCount; ++$index) {
        [ordered]@{
            action = "merge"
            source_style_id = "DuplicateBody$index"
            target_style_id = "BodyText"
            applyable = $true
        }
    }

    Write-JsonFile -Path $Path -Value ([ordered]@{
        command = "suggest-style-merges"
        operation_count = $SuggestionCount
        suggestion_confidence_summary = [ordered]@{
            suggestion_count = $SuggestionCount
            min_confidence = 92
            max_confidence = 96
        }
        operations = @($operations)
    })
}

function Write-Review {
    param(
        [string]$Path,
        [string]$Decision,
        [int]$ReviewedSuggestionCount,
        [string]$PlanFile,
        [string]$RollbackPlanFile = "style-merge.apply.rollback.json"
    )

    Write-JsonFile -Path $Path -Value ([ordered]@{
        schema = "featherdoc.style_merge_suggestion_review.v1"
        decision = $Decision
        reviewed_by = "release-reviewer"
        reviewed_at = "2026-05-16T08:00:00"
        reviewed_suggestion_count = $ReviewedSuggestionCount
        plan_file = $PlanFile
        rollback_plan_file = $RollbackPlanFile
    })
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\apply_reviewed_style_merge_suggestions.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "valid") {
    $scenarioDir = Join-Path $resolvedWorkingDir "valid"
    $inputPath = Join-Path $scenarioDir "input.docx"
    $planPath = Join-Path $scenarioDir "style-merge-suggestions.json"
    $reviewPath = Join-Path $scenarioDir "style-merge.review.json"
    $outputPath = Join-Path $scenarioDir "merged.docx"
    $rollbackPath = Join-Path $scenarioDir "style-merge.apply.rollback.json"
    $summaryPath = Join-Path $scenarioDir "style-merge.apply.summary.json"
    $mockCliPath = Join-Path $scenarioDir "mock-featherdoc-cli.ps1"
    $mockArgsPath = Join-Path $scenarioDir "mock-cli-args.json"

    New-Item -ItemType Directory -Path $scenarioDir -Force | Out-Null
    Set-Content -LiteralPath $inputPath -Encoding UTF8 -Value "mock input"
    Write-Plan -Path $planPath -SuggestionCount 2
    Write-Review -Path $reviewPath -Decision "Approved" -ReviewedSuggestionCount 2 -PlanFile "style-merge-suggestions.json"
    Write-MockCli -Path $mockCliPath

    $result = Invoke-ApplyScript -Arguments @(
        "-InputDocx", $inputPath,
        "-ReviewJson", $reviewPath,
        "-OutputDocx", $outputPath,
        "-SummaryJson", $summaryPath,
        "-CliPath", $mockCliPath,
        "-SkipBuild"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Reviewed style merge apply should succeed. Output: $($result.Text)"
    Assert-True -Condition (Test-Path -LiteralPath $outputPath) `
        -Message "Reviewed style merge apply should write the output DOCX."
    Assert-True -Condition (Test-Path -LiteralPath $rollbackPath) `
        -Message "Reviewed style merge apply should write the rollback plan."
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Reviewed style merge apply should write a summary JSON."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.reviewed_style_merge_apply.v1" `
        -Message "Apply summary should expose a stable schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "applied" `
        -Message "Apply summary should report applied status."
    Assert-Equal -Actual ([string]$summary.decision) -Expected "approved" `
        -Message "Apply summary should normalize review decisions."
    Assert-Equal -Actual ([int]$summary.reviewed_suggestion_count) -Expected 2 `
        -Message "Apply summary should preserve reviewed suggestion count."
    Assert-Equal -Actual ([int]$summary.style_merge_suggestion_count) -Expected 2 `
        -Message "Apply summary should preserve plan suggestion count."
    Assert-Equal -Actual ([bool]$summary.output_docx_exists) -Expected $true `
        -Message "Apply summary should mark the output DOCX as present."
    Assert-Equal -Actual ([bool]$summary.rollback_plan_exists) -Expected $true `
        -Message "Apply summary should mark the rollback plan as present."
    Assert-Equal -Actual ([bool]$summary.cli_result.ok) -Expected $true `
        -Message "Apply summary should include the CLI JSON result."

    $mockArgs = @((Get-Content -Raw -Encoding UTF8 -LiteralPath $mockArgsPath | ConvertFrom-Json) |
        ForEach-Object { $_ })
    Assert-Equal -Actual ([string]$mockArgs[0]) -Expected "apply-style-refactor" `
        -Message "Apply script should call apply-style-refactor."
    Assert-ContainsText -Text ($mockArgs -join "`n") -ExpectedText $planPath `
        -Message "Apply script should pass the reviewed plan path to the CLI."
    Assert-ContainsText -Text ($mockArgs -join "`n") -ExpectedText $rollbackPath `
        -Message "Apply script should pass the rollback plan path to the CLI."
}

if (Test-Scenario -Name "pending_review") {
    $scenarioDir = Join-Path $resolvedWorkingDir "pending-review"
    $inputPath = Join-Path $scenarioDir "input.docx"
    $planPath = Join-Path $scenarioDir "style-merge-suggestions.json"
    $reviewPath = Join-Path $scenarioDir "style-merge.review.json"
    $outputPath = Join-Path $scenarioDir "merged.docx"
    $mockCliPath = Join-Path $scenarioDir "mock-featherdoc-cli.ps1"

    New-Item -ItemType Directory -Path $scenarioDir -Force | Out-Null
    Set-Content -LiteralPath $inputPath -Encoding UTF8 -Value "mock input"
    Write-Plan -Path $planPath -SuggestionCount 1
    Write-Review -Path $reviewPath -Decision "pending" -ReviewedSuggestionCount 0 -PlanFile "style-merge-suggestions.json"
    Write-MockCli -Path $mockCliPath

    if (Test-Path -LiteralPath $outputPath) {
        Remove-Item -LiteralPath $outputPath -Force
    }

    $result = Invoke-ApplyScript -Arguments @(
        "-InputDocx", $inputPath,
        "-ReviewJson", $reviewPath,
        "-OutputDocx", $outputPath,
        "-CliPath", $mockCliPath,
        "-SkipBuild"
    )
    Assert-True -Condition ($result.ExitCode -ne 0) `
        -Message "Pending reviews should not be applied."
    Assert-True -Condition (-not (Test-Path -LiteralPath $outputPath)) `
        -Message "Pending reviews should not write an output DOCX."
    Assert-ContainsText -Text $result.Text -ExpectedText "not approved for apply" `
        -Message "Pending review failure should explain the gate."
}

if (Test-Scenario -Name "partial_review") {
    $scenarioDir = Join-Path $resolvedWorkingDir "partial-review"
    $inputPath = Join-Path $scenarioDir "input.docx"
    $planPath = Join-Path $scenarioDir "style-merge-suggestions.json"
    $reviewPath = Join-Path $scenarioDir "style-merge.review.json"
    $outputPath = Join-Path $scenarioDir "merged.docx"
    $mockCliPath = Join-Path $scenarioDir "mock-featherdoc-cli.ps1"

    New-Item -ItemType Directory -Path $scenarioDir -Force | Out-Null
    Set-Content -LiteralPath $inputPath -Encoding UTF8 -Value "mock input"
    Write-Plan -Path $planPath -SuggestionCount 2
    Write-Review -Path $reviewPath -Decision "approved" -ReviewedSuggestionCount 1 -PlanFile "style-merge-suggestions.json"
    Write-MockCli -Path $mockCliPath

    if (Test-Path -LiteralPath $outputPath) {
        Remove-Item -LiteralPath $outputPath -Force
    }

    $result = Invoke-ApplyScript -Arguments @(
        "-InputDocx", $inputPath,
        "-ReviewJson", $reviewPath,
        "-OutputDocx", $outputPath,
        "-CliPath", $mockCliPath,
        "-SkipBuild"
    )
    Assert-True -Condition ($result.ExitCode -ne 0) `
        -Message "Partial reviews should not be applied."
    Assert-True -Condition (-not (Test-Path -LiteralPath $outputPath)) `
        -Message "Partial reviews should not write an output DOCX."
    Assert-ContainsText -Text $result.Text -ExpectedText "does not cover plan suggestion count" `
        -Message "Partial review failure should explain the count mismatch."
}

Write-Host "Reviewed style merge apply regression passed."
