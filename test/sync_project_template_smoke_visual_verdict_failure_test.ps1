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

    $dir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-SyncScript {
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

    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = (@($output | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\sync_project_template_smoke_visual_verdict.ps1"
$summaryPath = Join-Path $resolvedWorkingDir "summary.json"
$summaryMarkdownPath = Join-Path $resolvedWorkingDir "summary.md"
$releaseSummaryPath = Join-Path $resolvedWorkingDir "release-candidate-checks\report\summary.json"
$releaseFinalReviewPath = Join-Path $resolvedWorkingDir "release-candidate-checks\report\final_review.md"
$manifestPath = Join-Path $resolvedWorkingDir "project_template_smoke.manifest.json"
$outputDir = Join-Path $resolvedWorkingDir "project-template-smoke-output"
$reviewResultPath = Join-Path $resolvedWorkingDir "visual\failed-review.json"
$contactSheetPath = Join-Path $resolvedWorkingDir "visual\contact-sheet.png"

Write-JsonFile -Path $reviewResultPath -Value ([ordered]@{
    schema = "featherdoc.word_visual_review_result.v1"
    status = "completed"
    verdict = "fail"
    findings = @(
        [ordered]@{
            id = "visual.regression"
            severity = "error"
        },
        [ordered]@{
            id = "visual.layout-shift"
            severity = "warning"
        }
    )
})

Write-JsonFile -Path $summaryPath -Value ([ordered]@{
    schema = "featherdoc.project_template_smoke_summary.v1"
    generated_at = "2026-01-01T00:00:00"
    manifest_path = $manifestPath
    output_dir = $outputDir
    passed = $true
    overall_status = "passed"
    entries = @(
        [ordered]@{
            name = "visual-fail"
            input_docx = Join-Path $resolvedWorkingDir "visual-fail.docx"
            artifact_dir = Join-Path $resolvedWorkingDir "artifacts\visual-fail"
            status = "passed"
            passed = $true
            manual_review_pending = $false
            visual_review_undetermined = $false
            checks = [ordered]@{
                template_validations = @()
                schema_validation = [ordered]@{
                    enabled = $false
                }
                schema_baseline = [ordered]@{
                    enabled = $true
                    matches = $true
                    schema_lint_clean = $true
                }
                visual_smoke = [ordered]@{
                    enabled = $true
                    review_status = "pending_review"
                    review_verdict = "pending_manual_review"
                    review_result_json = $reviewResultPath
                    contact_sheet = $contactSheetPath
                }
            }
            issues = @()
        }
    )
})

Write-JsonFile -Path $releaseSummaryPath -Value ([ordered]@{
    execution_status = "completed"
    visual_verdict = "pending_manual_review"
    failed_step = ""
    error = ""
    msvc_bootstrap_mode = "not_used"
    build_dir = Join-Path $resolvedWorkingDir "build"
    install_dir = Join-Path $resolvedWorkingDir "install"
    consumer_build_dir = Join-Path $resolvedWorkingDir "consumer-build"
    gate_output_dir = Join-Path $resolvedWorkingDir "visual-gate"
    task_output_root = Join-Path $resolvedWorkingDir "tasks"
    release_handoff = Join-Path $resolvedWorkingDir "release-handoff.md"
    release_body_zh_cn = Join-Path $resolvedWorkingDir "release-body.zh-CN.md"
    release_summary_zh_cn = Join-Path $resolvedWorkingDir "release-summary.zh-CN.md"
    artifact_guide = Join-Path $resolvedWorkingDir "artifact-guide.md"
    reviewer_checklist = Join-Path $resolvedWorkingDir "reviewer-checklist.md"
    start_here = Join-Path $resolvedWorkingDir "START_HERE.md"
    readme_gallery = [ordered]@{
        status = "not_requested"
    }
    steps = [ordered]@{
        configure = [ordered]@{ status = "completed" }
        build = [ordered]@{ status = "completed" }
        tests = [ordered]@{ status = "completed" }
        template_schema = [ordered]@{ status = "completed" }
        template_schema_manifest = [ordered]@{ status = "completed" }
        project_template_smoke = [ordered]@{ status = "completed" }
        install_smoke = [ordered]@{ status = "completed" }
        visual_gate = [ordered]@{ status = "completed" }
    }
})

$result = Invoke-SyncScript -Arguments @(
    "-SummaryJson", $summaryPath,
    "-SummaryMarkdown", $summaryMarkdownPath,
    "-ReleaseCandidateSummaryJson", $releaseSummaryPath
)

Assert-Equal -Actual $result.ExitCode -Expected 1 `
    -Message "Failing visual verdict should make project-template smoke sync exit 1. Output: $($result.Text)"
Assert-ContainsText -Text $result.Text -ExpectedText "Overall status: failed" `
    -Message "Sync output should report failed overall status."
Assert-ContainsText -Text $result.Text -ExpectedText "Visual verdict: fail" `
    -Message "Sync output should report the failing visual verdict."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$entry = $summary.entries[0]
Assert-Equal -Actual ([bool]$summary.passed) -Expected $false `
    -Message "Summary passed flag should be false after a failing visual verdict."
Assert-Equal -Actual ([string]$summary.overall_status) -Expected "failed" `
    -Message "Summary overall status should be failed."
Assert-Equal -Actual ([string]$summary.visual_verdict) -Expected "fail" `
    -Message "Summary visual verdict should be fail."
Assert-Equal -Actual ([int]$summary.failed_entry_count) -Expected 1 `
    -Message "Summary should count the failed entry."
Assert-Equal -Actual ([int]$summary.manual_review_pending_count) -Expected 0 `
    -Message "Failing visual verdict should not be counted as pending."
Assert-Equal -Actual ([int]$summary.visual_review_undetermined_count) -Expected 0 `
    -Message "Failing visual verdict should not be counted as undetermined."

Assert-Equal -Actual ([string]$entry.status) -Expected "failed" `
    -Message "Entry status should be failed after a failing visual verdict."
Assert-Equal -Actual ([bool]$entry.passed) -Expected $false `
    -Message "Entry passed flag should be false after a failing visual verdict."
Assert-Equal -Actual ([bool]$entry.manual_review_pending) -Expected $false `
    -Message "Entry manual_review_pending should be false for a failing verdict."
Assert-Equal -Actual ([bool]$entry.visual_review_undetermined) -Expected $false `
    -Message "Entry visual_review_undetermined should be false for a failing verdict."
Assert-Equal -Actual ([string]$entry.checks.visual_smoke.review_status) -Expected "completed" `
    -Message "Entry should refresh review_status from the review result."
Assert-Equal -Actual ([string]$entry.checks.visual_smoke.review_verdict) -Expected "fail" `
    -Message "Entry should refresh review_verdict from the review result."
Assert-Equal -Actual ([int]$entry.checks.visual_smoke.findings_count) -Expected 2 `
    -Message "Entry should refresh findings_count from the review result."

$summaryMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryMarkdownPath
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Overall status: failed" `
    -Message "Summary Markdown should expose failed status."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Visual verdict: fail" `
    -Message "Summary Markdown should expose failing visual verdict."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Visual smoke: review_status=completed review_verdict=fail findings=2" `
    -Message "Summary Markdown should include failing visual review metadata."

$releaseSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([bool]$releaseSummary.steps.project_template_smoke.passed) -Expected $false `
    -Message "Release summary step should mirror failed project-template smoke status."
Assert-Equal -Actual ([string]$releaseSummary.steps.project_template_smoke.visual_verdict) -Expected "fail" `
    -Message "Release summary step should mirror the failing visual verdict."
Assert-Equal -Actual ([int]$releaseSummary.steps.project_template_smoke.failed_entry_count) -Expected 1 `
    -Message "Release summary step should mirror failed entry count."
Assert-Equal -Actual ([string]$releaseSummary.project_template_smoke.overall_status) -Expected "failed" `
    -Message "Release summary project_template_smoke block should mirror failed status."
Assert-Equal -Actual ([string]$releaseSummary.project_template_smoke.visual_verdict) -Expected "fail" `
    -Message "Release summary project_template_smoke block should mirror failing visual verdict."
Assert-True -Condition (Test-Path -LiteralPath $releaseFinalReviewPath) `
    -Message "Sync should still refresh release final_review.md before exiting 1."

$releaseFinalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseFinalReviewPath
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "Project template smoke summary:" `
    -Message "Final review should mention the project-template smoke summary."

Write-Host "Project template smoke visual verdict failure regression passed."
