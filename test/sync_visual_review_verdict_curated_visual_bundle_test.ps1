param(
    [string]$RepoRoot,
    [string]$WorkingDir
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

function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText': $Path"
    }
}

function New-TaskReviewSeed {
    param(
        [string]$Root,
        [string]$TaskId,
        [string]$ReviewedAt
    )

    $reportDir = Join-Path $Root "report"
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

    (@{
        task_id = $TaskId
        status = "reviewed"
        verdict = "pass"
        reviewed_at = $ReviewedAt
        review_method = "operator_supplied"
        findings = @()
        notes = @("verified")
    } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath (Join-Path $reportDir "review_result.json") -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $reportDir "final_review.md") -Encoding UTF8 -Value "# final"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$gateRoot = Join-Path $resolvedWorkingDir "word-visual-release-gate"
$gateReportDir = Join-Path $gateRoot "report"
$releaseRoot = Join-Path $resolvedWorkingDir "release-candidate-checks"
$releaseReportDir = Join-Path $releaseRoot "report"
$tasksRoot = Join-Path $resolvedWorkingDir "tasks"
$bundleId = "template-table-cli-selector"
$bundleLabel = "Template table CLI selector"
$bundleRoot = Join-Path $gateRoot $bundleId
$bundleReportDir = Join-Path $bundleRoot "report"
$bundleSummaryPath = Join-Path $bundleRoot "summary.json"
$syncScript = Join-Path $resolvedRepoRoot "scripts\sync_visual_review_verdict.ps1"

New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $releaseReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null
New-Item -ItemType Directory -Path $bundleReportDir -Force | Out-Null

(@{
    generated_at = "2026-04-21T21:00:00"
    status = "completed"
} | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath $bundleSummaryPath -Encoding UTF8

$taskDir = Join-Path $tasksRoot "$bundleId-task"
New-TaskReviewSeed -Root $taskDir -TaskId "$bundleId-task" -ReviewedAt "2026-04-21T21:35:00"

$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$releaseSummaryPath = Join-Path $releaseReportDir "summary.json"

(@{
    generated_at = "2026-04-21T21:00:00"
    gate_output_dir = $gateRoot
    report_dir = $gateReportDir
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    readme_gallery = @{ status = "not_requested" }
    review_task_summary = @{
        total_count = 1
        standard_count = 0
        curated_count = 1
    }
    curated_visual_regressions = @(
        @{
            id = $bundleId
            label = $bundleLabel
            status = "completed"
            output_dir = $bundleRoot
            summary_json = $bundleSummaryPath
        }
    )
    review_tasks = @{
        curated_visual_regressions = @(
            @{
                id = $bundleId
                label = $bundleLabel
                task = @{
                    task_id = "$bundleId-task"
                    task_dir = $taskDir
                    review_result_path = (Join-Path $taskDir "report\review_result.json")
                    final_review_path = (Join-Path $taskDir "report\final_review.md")
                }
            }
        )
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-21T21:00:00"
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    build_dir = (Join-Path $resolvedWorkingDir "build")
    install_dir = (Join-Path $resolvedWorkingDir "install")
    consumer_build_dir = (Join-Path $resolvedWorkingDir "consumer")
    gate_output_dir = $gateRoot
    task_output_root = $tasksRoot
    release_handoff = (Join-Path $releaseReportDir "release_handoff.md")
    release_body_zh_cn = (Join-Path $releaseReportDir "release_body.zh-CN.md")
    release_summary_zh_cn = (Join-Path $releaseReportDir "release_summary.zh-CN.md")
    artifact_guide = (Join-Path $releaseReportDir "ARTIFACT_GUIDE.md")
    reviewer_checklist = (Join-Path $releaseReportDir "REVIEWER_CHECKLIST.md")
    start_here = (Join-Path $releaseRoot "START_HERE.md")
    readme_gallery = @{ status = "not_requested" }
    steps = @{
        configure = @{ status = "completed" }
        build = @{ status = "completed" }
        tests = @{ status = "completed" }
        install_smoke = @{ status = "completed" }
        visual_gate = @{
            status = "completed"
            summary_json = $gateSummaryPath
            final_review = (Join-Path $gateReportDir "gate_final_review.md")
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8

& $syncScript `
    -GateSummaryJson $gateSummaryPath `
    -ReleaseCandidateSummaryJson $releaseSummaryPath

$gateSummary = Get-Content -Raw -LiteralPath $gateSummaryPath | ConvertFrom-Json
$releaseSummary = Get-Content -Raw -LiteralPath $releaseSummaryPath | ConvertFrom-Json
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$releaseFinalReviewPath = Join-Path $releaseReportDir "final_review.md"
$curatedManualReviews = @($gateSummary.manual_review.tasks.curated_visual_regressions)
$curatedReleaseReviews = @($releaseSummary.steps.visual_gate.curated_visual_regressions)

Assert-True -Condition ($gateSummary.visual_verdict -eq "pass") `
    -Message "Gate summary visual verdict was not promoted to pass for curated bundles."
Assert-True -Condition ($curatedManualReviews.Count -eq 1) `
    -Message "Gate summary manual_review is missing curated visual regression entries."
Assert-True -Condition ($curatedManualReviews[0].verdict -eq "pass") `
    -Message "Gate summary manual_review did not record a pass verdict for the curated bundle."
Assert-True -Condition ($curatedReleaseReviews.Count -eq 1) `
    -Message "Release summary visual_gate is missing curated visual regression entries."
Assert-True -Condition ($curatedReleaseReviews[0].id -eq $bundleId) `
    -Message "Release summary visual_gate did not record the curated bundle id."
Assert-True -Condition ($curatedReleaseReviews[0].verdict -eq "pass") `
    -Message "Release summary visual_gate did not record the curated bundle verdict."
Assert-True -Condition ($curatedManualReviews[0].review_method -eq "operator_supplied") `
    -Message "Gate summary manual_review did not record the curated review_method."
Assert-True -Condition ($curatedReleaseReviews[0].review_method -eq "operator_supplied") `
    -Message "Release summary visual_gate did not record the curated review_method."
Assert-True -Condition ($releaseSummary.steps.visual_gate.review_task_summary.total_count -eq 1) `
    -Message "Release summary visual_gate did not copy review_task_summary.total_count."
Assert-True -Condition ($releaseSummary.steps.visual_gate.review_task_summary.curated_count -eq 1) `
    -Message "Release summary visual_gate did not copy review_task_summary.curated_count."
Assert-Contains -Path $gateSummaryPath -ExpectedText "2026-04-21T21:35:00" -Label "gate_summary.json"
Assert-Contains -Path $releaseSummaryPath -ExpectedText "2026-04-21T21:35:00" -Label "summary.json"

Assert-Contains -Path $gateFinalReviewPath -ExpectedText "$bundleLabel flow: completed" -Label "gate_final_review.md"
Assert-Contains -Path $gateFinalReviewPath -ExpectedText "$bundleLabel verdict: pass" -Label "gate_final_review.md"
Assert-Contains -Path $gateFinalReviewPath -ExpectedText "$bundleLabel reviewed at: 2026-04-21T21:35:00" -Label "gate_final_review.md"
Assert-Contains -Path $gateFinalReviewPath -ExpectedText "$bundleLabel review method: operator_supplied" -Label "gate_final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "Visual verdict: pass" -Label "release final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "Review task count: 1 total (0 standard, 1 curated)" -Label "release final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "## Visual review provenance" -Label "release final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "Curated - $($bundleLabel): verdict=pass, review_status=reviewed, reviewed_at=2026-04-21T21:35:00, review_method=operator_supplied" -Label "release final_review.md"

Write-Host "Sync visual review verdict curated visual bundle regression passed."
