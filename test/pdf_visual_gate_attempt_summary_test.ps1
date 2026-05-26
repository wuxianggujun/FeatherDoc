param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $RepoRoot "build\pdf_visual_gate_attempt_summary_test"
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw $Message
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Write-TextFile {
    param([string]$Path, [string]$Text)
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    $Text | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-AttemptSummaryScript {
    param(
        [string]$ReportDir,
        [string]$OutputJson
    )

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellCommand = Get-Command pwsh -ErrorAction SilentlyContinue
        if (-not $powerShellCommand) {
            $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
        }
        if (-not $powerShellCommand) {
            throw "Unable to resolve PowerShell for nested script invocation."
        }
        $powerShellPath = $powerShellCommand.Source
    }

    $scriptPath = Join-Path $resolvedRepoRoot "scripts\write_pdf_visual_gate_attempt_summary.ps1"
    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath `
            -RepoRoot $resolvedRepoRoot `
            -ReportDir $ReportDir `
            -OutputJson $OutputJson 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    if ($exitCode -ne 0) {
        throw "write_pdf_visual_gate_attempt_summary.ps1 failed: $($output -join [Environment]::NewLine)"
    }
}

function New-AttemptFixture {
    param(
        [string]$Root,
        [bool]$FreshFullPass
    )

    $reportDir = Join-Path $Root "output\pdf-visual-release-gate-current\report"
    $baselineDir = Join-Path $Root "output\pdf-visual-release-gate-current\baseline"
    $cjkDir = Join-Path $reportDir "cjk-copy-search"
    New-Item -ItemType Directory -Path $reportDir, $baselineDir, $cjkDir -Force | Out-Null

    $attemptStart = Get-Date "2026-05-26T12:50:00"
    $oldTime = Get-Date "2026-05-26T12:00:00"
    $freshTime = Get-Date "2026-05-26T12:55:00"

    Write-JsonFile -Path (Join-Path $reportDir "preflight-summary.json") -Value ([ordered]@{
        status = "ready"
    })
    (Get-Item -LiteralPath (Join-Path $reportDir "preflight-summary.json")).LastWriteTime = $attemptStart

    Write-TextFile -Path (Join-Path $reportDir "pdf-cli-export-test.log") -Text @"
100% tests passed, 0 tests failed out of 1
Total Test time (real) =  10.82 sec
"@
    (Get-Item -LiteralPath (Join-Path $reportDir "pdf-cli-export-test.log")).LastWriteTime = $freshTime

    $skippedLines = @(
        "1/91 Test #39: pdf_regression_document-table-font-matrix-text ........***Skipped 0.01 sec",
        "2/91 Test #68: pdf_regression_document-table-cjk-wrap-flow-text ......***Skipped 0.01 sec",
        "3/91 Test #69: pdf_regression_document-table-cjk-vertical-merged-cant-split-text ......***Skipped 0.01 sec",
        "4/91 Test #70: pdf_regression_document-table-cjk-merged-repeat-text ..***Skipped 0.01 sec",
        "5/91 Test #87: pdf_regression_document-cjk-style-overlay-page-flow-text ......***Skipped 0.01 sec",
        "6/91 Test #88: pdf_regression_document-cjk-complex-layout-text .......***Skipped 0.01 sec",
        "7/91 Test #99: pdf_regression_document-cjk-table-wrap-page-flow-text .***Skipped 0.01 sec"
    ) -join [Environment]::NewLine
    Write-TextFile -Path (Join-Path $reportDir "pdf-regression-test.log") -Text @"
$skippedLines
100% tests passed, 0 tests failed out of 91
Total Test time (real) =  17.16 sec
"@
    (Get-Item -LiteralPath (Join-Path $reportDir "pdf-regression-test.log")).LastWriteTime = $freshTime

    Write-TextFile -Path (Join-Path $reportDir "unicode-font.log") -Text @"
100% tests passed, 0 tests failed out of 1
Visual summary written to output/pdf-visual-release-gate-current/unicode-font/report/summary.json
"@
    (Get-Item -LiteralPath (Join-Path $reportDir "unicode-font.log")).LastWriteTime = $freshTime

    foreach ($index in 1..43) {
        $summaryPath = Join-Path $cjkDir ("sample-{0:D2}-summary.json" -f $index)
        Write-JsonFile -Path $summaryPath -Value ([ordered]@{
            matched_text = @("示例")
            missing_text = @()
            page_count = 1
        })
        (Get-Item -LiteralPath $summaryPath).LastWriteTime = $freshTime
    }

    foreach ($index in 1..44) {
        $sampleDir = Join-Path $baselineDir ("sample-{0:D2}" -f $index)
        $summaryPath = Join-Path $sampleDir "summary.json"
        Write-JsonFile -Path $summaryPath -Value ([ordered]@{
            page_count = 1
        })
        $timestamp = if ($FreshFullPass -or $index -le 22) { $freshTime } else { $oldTime }
        (Get-Item -LiteralPath $summaryPath).LastWriteTime = $timestamp
    }

    $contactSheetPath = Join-Path $reportDir "aggregate-contact-sheet.png"
    [System.IO.File]::WriteAllBytes($contactSheetPath, [byte[]](1, 2, 3, 4))
    (Get-Item -LiteralPath $contactSheetPath).LastWriteTime = if ($FreshFullPass) { $freshTime } else { $oldTime }

    Write-JsonFile -Path (Join-Path $reportDir "summary.json") -Value ([ordered]@{
        status = "pass"
        verdict = "pass"
        finalize_only = -not $FreshFullPass
        aggregate_contact_sheet = $contactSheetPath
    })
    (Get-Item -LiteralPath (Join-Path $reportDir "summary.json")).LastWriteTime = if ($FreshFullPass) { $freshTime.AddMinutes(1) } else { $oldTime }

    return [pscustomobject]@{
        ReportDir = $reportDir
        OutputJson = Join-Path $reportDir "attempt-summary.json"
    }
}

$resolvedRepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$partialFixture = New-AttemptFixture -Root (Join-Path $resolvedWorkingDir "partial") -FreshFullPass $false
Invoke-AttemptSummaryScript -ReportDir $partialFixture.ReportDir -OutputJson $partialFixture.OutputJson
$partialSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $partialFixture.OutputJson | ConvertFrom-Json

Assert-Equal -Actual $partialSummary.status -Expected "partial" `
    -Message "Partial attempt should not be promoted to pass."
Assert-Equal -Actual $partialSummary.verdict -Expected "not_complete" `
    -Message "Partial attempt should carry an explicit not_complete verdict."
Assert-Equal -Actual $partialSummary.full_visual_gate_status -Expected "not_complete" `
    -Message "Partial attempt should not set full_visual_gate_status to pass."
Assert-Equal -Actual $partialSummary.pdf_cli_export_status -Expected "pass" `
    -Message "Attempt summary should preserve pdf_cli_export pass evidence."
Assert-Equal -Actual $partialSummary.pdf_regression_status -Expected "pass" `
    -Message "Attempt summary should preserve pdf_regression pass evidence."
Assert-Equal -Actual ([int]$partialSummary.pdf_regression_selected_test_count) -Expected 91 `
    -Message "Attempt summary should preserve the pdf_regression selected test count."
Assert-Equal -Actual ([int]$partialSummary.pdf_regression_failed_test_count) -Expected 0 `
    -Message "Attempt summary should preserve the pdf_regression failed test count."
Assert-Equal -Actual ([int]$partialSummary.pdf_regression_skipped_test_count) -Expected 7 `
    -Message "Attempt summary should preserve skipped boundary cases."
Assert-Equal -Actual ([int]$partialSummary.cjk_copy_search_count) -Expected 43 `
    -Message "Attempt summary should preserve CJK copy/search evidence."
Assert-Equal -Actual $partialSummary.visual_baseline_render_status -Expected "partial" `
    -Message "Attempt summary should identify incomplete visual baseline rendering."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_fresh_rendered_count) -Expected 22 `
    -Message "Attempt summary should count only fresh rendered baselines for the attempt."
Assert-Equal -Actual $partialSummary.aggregate_contact_sheet_status -Expected "stale" `
    -Message "Attempt summary should not treat stale contact sheets as fresh attempt evidence."
Assert-Equal -Actual $partialSummary.evidence_scope -Expected "bounded_attempt_auxiliary_only" `
    -Message "Attempt summary should declare its auxiliary scope."

$passFixture = New-AttemptFixture -Root (Join-Path $resolvedWorkingDir "pass") -FreshFullPass $true
Invoke-AttemptSummaryScript -ReportDir $passFixture.ReportDir -OutputJson $passFixture.OutputJson
$passSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $passFixture.OutputJson | ConvertFrom-Json

Assert-Equal -Actual $passSummary.status -Expected "pass" `
    -Message "Fresh non-finalize full summary should allow a pass status."
Assert-Equal -Actual $passSummary.verdict -Expected "pass" `
    -Message "Fresh non-finalize full summary should allow a pass verdict."
Assert-Equal -Actual $passSummary.full_visual_gate_status -Expected "pass" `
    -Message "Fresh non-finalize full summary should preserve full_visual_gate_status=pass."
Assert-True -Condition ([bool]$passSummary.full_summary_fresh_for_attempt) `
    -Message "Fresh full summary should be marked fresh for the attempt."
