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

function Assert-Contains {
    param([string]$Path, [string]$ExpectedText, [string]$Label)
    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText': $Path"
    }
}

function New-ReviewTask {
    param(
        [string]$TaskDir,
        [string]$TaskId,
        [string]$BundleRoot,
        [string]$GeneratedAt,
        [string]$Verdict
    )

    $reportDir = Join-Path $TaskDir "report"
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $TaskDir "task_prompt.md") -Encoding UTF8 -Value "# table style quality prompt"
    (@{
        task_id = $TaskId
        task_dir = $TaskDir
        generated_at = $GeneratedAt
        source = @{ kind = "visual-regression-bundle"; path = $BundleRoot }
    } | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath (Join-Path $TaskDir "task_manifest.json") -Encoding UTF8
    (@{
        task_id = $TaskId
        status = "reviewed"
        verdict = $Verdict
        reviewed_at = $GeneratedAt
        review_method = "operator_supplied"
        review_note = "table style quality evidence checked"
        findings = @()
        notes = @("table style quality evidence checked")
    } | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath (Join-Path $reportDir "review_result.json") -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $reportDir "final_review.md") -Encoding UTF8 -Value "# reviewed $TaskId"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$workingDirLeaf = Split-Path -Leaf ([System.IO.Path]::GetFullPath($WorkingDir))
$resolvedWorkingDir = Join-Path $resolvedRepoRoot ("output\codex-{0}-{1}" -f $workingDirLeaf, [System.Guid]::NewGuid().ToString("N"))
$tasksRoot = Join-Path $resolvedWorkingDir "tasks"
$gateRoot = Join-Path $resolvedWorkingDir "word-visual-release-gate"
$gateReportDir = Join-Path $gateRoot "report"
$releaseRoot = Join-Path $resolvedWorkingDir "release-candidate-checks"
$releaseReportDir = Join-Path $releaseRoot "report"
$bundleId = "table-style-quality"
$bundleLabel = "Table style quality"
$bundleRoot = Join-Path $gateRoot $bundleId
$bundleSummaryPath = Join-Path $bundleRoot "summary.json"
$taskDir = Join-Path $tasksRoot "$bundleId-task-reviewed"
$taskId = "$bundleId-task-reviewed"
$pointerPath = Join-Path $tasksRoot "latest_table-style-quality-visual-regression-bundle_task.json"
$syncLatestScript = Join-Path $resolvedRepoRoot "scripts\sync_latest_visual_review_verdict.ps1"

New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $releaseReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $bundleRoot -Force | Out-Null

(@{ generated_at = "2026-04-30T12:40:00"; status = "completed" } | ConvertTo-Json -Depth 4) |
    Set-Content -LiteralPath $bundleSummaryPath -Encoding UTF8
New-ReviewTask -TaskDir $taskDir -TaskId $taskId -BundleRoot $bundleRoot -GeneratedAt "2026-04-30T12:45:00" -Verdict "pass"

(@{
    task_id = $taskId
    task_dir = $taskDir
    generated_at = "2026-04-30T12:45:00"
    source = @{ kind = "visual-regression-bundle"; path = $bundleRoot }
} | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $pointerPath -Encoding UTF8

$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$releaseSummaryPath = Join-Path $releaseReportDir "summary.json"
(@{
    generated_at = "2026-04-30T12:30:00"
    gate_output_dir = $gateRoot
    report_dir = $gateReportDir
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    readme_gallery = @{ status = "not_requested" }
    curated_visual_regressions = @(
        @{
            id = $bundleId
            label = $bundleLabel
            status = "completed"
            output_dir = $bundleRoot
            summary_json = $bundleSummaryPath
            aggregate_contact_sheet = (Join-Path $bundleRoot "aggregate-evidence\contact_sheet.png")
        }
    )
    review_tasks = @{ curated_visual_regressions = @() }
} | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-30T12:30:00"
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
        visual_gate = @{ status = "completed"; summary_json = $gateSummaryPath; final_review = (Join-Path $gateReportDir "gate_final_review.md") }
    }
} | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8

& $syncLatestScript `
    -GateSummaryJson $gateSummaryPath `
    -ReleaseCandidateSummaryJson $releaseSummaryPath `
    -TaskOutputRoot $tasksRoot `
    -SkipSupersededReviewTaskAudit
if ($LASTEXITCODE -ne 0) { throw "sync_latest_visual_review_verdict.ps1 failed." }

$gateSummary = Get-Content -Raw -LiteralPath $gateSummaryPath | ConvertFrom-Json
$releaseSummary = Get-Content -Raw -LiteralPath $releaseSummaryPath | ConvertFrom-Json
$curatedGateFlow = @($gateSummary.curated_visual_regressions) | Where-Object { $_.id -eq $bundleId } | Select-Object -First 1
$curatedGateReview = @($gateSummary.manual_review.tasks.curated_visual_regressions) | Where-Object { $_.id -eq $bundleId } | Select-Object -First 1
$curatedReleaseReview = @($releaseSummary.steps.visual_gate.curated_visual_regressions) | Where-Object { $_.id -eq $bundleId } | Select-Object -First 1
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$releaseFinalReviewPath = Join-Path $releaseReportDir "final_review.md"

Assert-True -Condition ($gateSummary.visual_verdict -eq "pass") `
    -Message "Gate summary visual verdict should be promoted by table-style-quality review."
Assert-True -Condition ($releaseSummary.visual_verdict -eq "pass") `
    -Message "Release summary visual verdict should be promoted by table-style-quality review."
Assert-True -Condition ($curatedGateFlow.task.task_id -eq $taskId) `
    -Message "Gate summary curated flow should be refreshed from latest table-style-quality pointer."
Assert-True -Condition ($curatedGateReview.verdict -eq "pass") `
    -Message "Gate manual review should include table-style-quality verdict."
Assert-True -Condition ($curatedReleaseReview.verdict -eq "pass") `
    -Message "Release summary should include table-style-quality verdict."
Assert-True -Condition ($curatedReleaseReview.review_note -eq "table style quality evidence checked") `
    -Message "Release summary should include table-style-quality review note."
Assert-Contains -Path $gateFinalReviewPath -ExpectedText "Table style quality flow: completed" -Label "gate_final_review.md"
Assert-Contains -Path $gateFinalReviewPath -ExpectedText "Table style quality verdict: pass" -Label "gate_final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "Curated - Table style quality: verdict=pass" -Label "release final_review.md"
Assert-Contains -Path $releaseSummary.release_summary_zh_cn -ExpectedText 'Table style quality=`pass`' -Label "release_summary.zh-CN.md"

Write-Host "Sync latest visual review verdict table style quality bundle regression passed."
