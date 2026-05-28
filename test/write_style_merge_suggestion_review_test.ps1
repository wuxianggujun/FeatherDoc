param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "valid", "invalid_count")]
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

function Invoke-ReviewScript {
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
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\write_style_merge_suggestion_review.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "valid") {
    $planPath = Join-Path $resolvedWorkingDir "style-merge-suggestions.json"
    $rollbackPath = Join-Path $resolvedWorkingDir "style-merge.rollback.json"
    $outputPath = Join-Path $resolvedWorkingDir "style-merge.review.json"
    $defaultOutputPath = Join-Path $resolvedWorkingDir "style-merge-suggestions.review.json"

    Write-JsonFile -Path $planPath -Value ([ordered]@{
        command = "suggest-style-merges"
        operation_count = 2
        suggestion_confidence_summary = [ordered]@{
            suggestion_count = 2
            min_confidence = 92
            max_confidence = 96
        }
        operations = @(
            [ordered]@{ action = "merge"; source_style_id = "DuplicateBodyB"; target_style_id = "DuplicateBodyA" },
            [ordered]@{ action = "merge"; source_style_id = "DuplicateBodyC"; target_style_id = "DuplicateBodyA" }
        )
    })
    Write-JsonFile -Path $rollbackPath -Value ([ordered]@{
        command = "apply-style-refactor"
        operations = @()
    })

    $result = Invoke-ReviewScript -Arguments @(
        "-PlanFile", $planPath,
        "-OutputJson", $outputPath,
        "-Decision", "approved",
        "-Reviewer", "release-reviewer",
        "-RollbackPlanFile", $rollbackPath
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Review script should succeed for a valid approved review. Output: $($result.Text)"
    Assert-True -Condition (Test-Path -LiteralPath $outputPath) `
        -Message "Review script should write the requested review JSON."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.style_merge_suggestion_review.v1" `
        -Message "Review JSON should expose the review schema."
    Assert-Equal -Actual ([string]$summary.decision) -Expected "approved" `
        -Message "Review JSON should normalize the decision."
    Assert-Equal -Actual ([string]$summary.reviewed_by) -Expected "release-reviewer" `
        -Message "Review JSON should preserve reviewer metadata."
    Assert-Equal -Actual ([int]$summary.reviewed_suggestion_count) -Expected 2 `
        -Message "Review JSON should default reviewed count to the plan suggestion count."
    Assert-Equal -Actual ([int]$summary.style_merge_suggestion_count) -Expected 2 `
        -Message "Review JSON should preserve suggestion counts."
    Assert-Equal -Actual ([bool]$summary.plan_exists) -Expected $true `
        -Message "Review JSON should mark the plan as existing."
    Assert-Equal -Actual ([bool]$summary.rollback_plan_exists) -Expected $true `
        -Message "Review JSON should mark the rollback plan as existing."
    Assert-ContainsText -Text ([string]$summary.plan_file) -ExpectedText "style-merge-suggestions.json" `
        -Message "Review JSON should store the plan path relative to the output JSON."
    Assert-ContainsText -Text ([string]$summary.rollback_plan_file) -ExpectedText "style-merge.rollback.json" `
        -Message "Review JSON should store the rollback plan path relative to the output JSON."

    $defaultResult = Invoke-ReviewScript -Arguments @(
        "-PlanFile", $planPath,
        "-Decision", "deferred",
        "-Reviewer", "triage-reviewer"
    )
    Assert-Equal -Actual $defaultResult.ExitCode -Expected 0 `
        -Message "Review script should succeed when using the default output path."
    Assert-True -Condition (Test-Path -LiteralPath $defaultOutputPath) `
        -Message "Review script should write the default review JSON path."

    $defaultSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $defaultOutputPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$defaultSummary.decision) -Expected "deferred" `
        -Message "Default output review JSON should preserve the deferred decision."
    Assert-Equal -Actual ([int]$defaultSummary.reviewed_suggestion_count) -Expected 0 `
        -Message "Deferred reviews should default to zero reviewed suggestions."
    Assert-Equal -Actual ([string]$defaultSummary.plan_path) -Expected (Resolve-Path $planPath).Path `
        -Message "Default output review JSON should still resolve the plan path."
    Assert-Equal -Actual ([bool]$defaultSummary.rollback_plan_exists) -Expected $false `
        -Message "Default output review JSON should report missing rollback evidence as absent."
}

if (Test-Scenario -Name "invalid_count") {
    $planPath = Join-Path $resolvedWorkingDir "style-merge-suggestions.invalid.json"
    $outputPath = Join-Path $resolvedWorkingDir "style-merge.review.invalid.json"

    Write-JsonFile -Path $planPath -Value ([ordered]@{
        command = "suggest-style-merges"
        operation_count = 2
        suggestion_confidence_summary = [ordered]@{
            suggestion_count = 2
        }
        operations = @(
            [ordered]@{ action = "merge"; source_style_id = "DuplicateBodyB"; target_style_id = "DuplicateBodyA" },
            [ordered]@{ action = "merge"; source_style_id = "DuplicateBodyC"; target_style_id = "DuplicateBodyA" }
        )
    })

    if (Test-Path -LiteralPath $outputPath) {
        Remove-Item -LiteralPath $outputPath -Force
    }

    $result = Invoke-ReviewScript -Arguments @(
        "-PlanFile", $planPath,
        "-OutputJson", $outputPath,
        "-Decision", "approved",
        "-ReviewedSuggestionCount", 3
    )
    Assert-True -Condition ($result.ExitCode -ne 0) `
        -Message "Review script should reject reviewed counts larger than the plan suggestion count."
    Assert-True -Condition (-not (Test-Path -LiteralPath $outputPath)) `
        -Message "Review script should not write a review JSON when the count is invalid."
    Assert-ContainsText -Text $result.Text -ExpectedText "cannot exceed suggestion count" `
        -Message "Review script should explain the invalid count failure."
}

Write-Host "Style merge suggestion review writer regression passed."
