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

function Write-JsonFile {
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$inputRoot = Join-Path $resolvedWorkingDir "governance-input"
$numberingSummaryPath = Join-Path $inputRoot "numbering-catalog-governance\summary.json"
$tableSummaryPath = Join-Path $inputRoot "table-layout-governance\summary.json"
$summaryOutputDir = Join-Path $resolvedWorkingDir "release-candidate"

Write-JsonFile -Path $numberingSummaryPath -Value ([ordered]@{
    schema = "featherdoc.numbering_catalog_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "numbering_catalog_governance.style_numbering_issues"
            severity = "error"
            status = "blocked"
            message = "Style numbering audit reported issues."
            action = "review_style_numbering_audit"
        }
    )
    action_items = @(
        [ordered]@{
            id = "preview_style_numbering_repair"
            action = "preview_style_numbering_repair"
            title = "Preview style numbering repair"
        }
    )
})

Write-JsonFile -Path $tableSummaryPath -Value ([ordered]@{
    schema = "featherdoc.table_layout_delivery_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "table_layout_delivery.floating_table_review_pending"
            severity = "warning"
            status = "needs_review"
            message = "Floating table plans still need review."
            action = "review_table_position_plan"
        }
    )
    action_items = @(
        [ordered]@{
            id = "run_table_style_quality_visual_regression"
            action = "run_table_style_quality_visual_regression"
            title = "Generate Word-rendered table layout evidence"
        }
    )
})

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_release_candidate_checks.ps1"
$scriptArguments = @(
    "-NoProfile",
    "-NonInteractive",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    $scriptPath,
    "-SkipConfigure",
    "-SkipBuild",
    "-SkipTests",
    "-SkipInstallSmoke",
    "-SkipVisualGate",
    "-SummaryOutputDir",
    $summaryOutputDir,
    "-ReleaseBlockerRollupInputJson",
    "$numberingSummaryPath,$tableSummaryPath"
)
$result = @(& (Get-Process -Id $PID).Path @scriptArguments 2>&1)
$exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
$text = (@($result | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
Assert-Equal -Actual $exitCode -Expected 0 `
    -Message "Release candidate rollup-only run should pass. Output: $text"

$summaryPath = Join-Path $summaryOutputDir "report\summary.json"
$finalReviewPath = Join-Path $summaryOutputDir "report\final_review.md"
$rollupSummaryPath = Join-Path $summaryOutputDir "report\release-blocker-rollup\summary.json"
$rollupMarkdownPath = Join-Path $summaryOutputDir "report\release-blocker-rollup\release_blocker_rollup.md"

foreach ($path in @($summaryPath, $finalReviewPath, $rollupSummaryPath, $rollupMarkdownPath)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected release candidate rollup artifact to exist: $path"
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.msvc_bootstrap_mode) -Expected "not_required" `
    -Message "Rollup-only release candidate run should not require MSVC discovery."
Assert-Equal -Actual ([string]$summary.release_blocker_rollup.status) -Expected "blocked" `
    -Message "Release candidate summary should surface the rollup status."
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.source_report_count) -Expected 2 `
    -Message "Release candidate summary should surface source report count."
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.release_blocker_count) -Expected 2 `
    -Message "Release candidate summary should surface blocker count."
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.action_item_count) -Expected 2 `
    -Message "Release candidate summary should surface action item count."
Assert-Equal -Actual ([string]$summary.steps.release_blocker_rollup.status) -Expected "blocked" `
    -Message "Release candidate step status should mirror rollup status."

$rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$rollupSummary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
    -Message "Nested release blocker rollup should write its schema."
Assert-ContainsText -Text (($rollupSummary.action_items | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "run_table_style_quality_visual_regression" `
    -Message "Nested release blocker rollup should retain table layout action items."

$finalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $finalReviewPath
Assert-ContainsText -Text $finalReview -ExpectedText "- Release blocker rollup: blocked" `
    -Message "Final review should include release blocker rollup step status."
Assert-ContainsText -Text $finalReview -ExpectedText "Release blocker rollup counts: 2 blockers, 2 actions" `
    -Message "Final review should include release blocker rollup counts."

$gateOutputDir = Join-Path $resolvedWorkingDir "release-candidate-fail-on-blocker"
$gateArguments = @($scriptArguments)
$summaryOutputIndex = [Array]::IndexOf($gateArguments, "-SummaryOutputDir")
$gateArguments[$summaryOutputIndex + 1] = $gateOutputDir
$gateArguments += "-ReleaseBlockerRollupFailOnBlocker"
$gateResult = @(& (Get-Process -Id $PID).Path @gateArguments 2>&1)
$gateExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
if ($gateExitCode -eq 0) {
    $gateText = (@($gateResult | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
    throw "Release candidate rollup fail-on-blocker run should fail. Output: $gateText"
}
$gateSummaryPath = Join-Path $gateOutputDir "report\summary.json"
Assert-True -Condition (Test-Path -LiteralPath $gateSummaryPath) `
    -Message "Fail-on-blocker run should still write release candidate summary."
$gateSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $gateSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$gateSummary.release_blocker_rollup.status) -Expected "failed" `
    -Message "Fail-on-blocker run should mark rollup as failed in release summary."

Write-Host "Release candidate blocker rollup regression passed."
