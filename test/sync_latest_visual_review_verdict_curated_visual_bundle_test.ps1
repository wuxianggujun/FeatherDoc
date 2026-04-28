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

function Assert-PathNotExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if (Test-Path -LiteralPath $Path) {
        throw "$Label should not have been generated: $Path"
    }
}

function New-TaskReviewSeed {
    param(
        [string]$Root,
        [string]$TaskId,
        [string]$Status,
        [string]$Verdict,
        [string]$SourcePath,
        [string]$GeneratedAt
    )

    $reportDir = Join-Path $Root "report"
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

    (@{
        task_id = $TaskId
        status = $Status
        verdict = $Verdict
        findings = @()
        notes = @("seed")
    } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath (Join-Path $reportDir "review_result.json") -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $reportDir "final_review.md") -Encoding UTF8 -Value "# final $TaskId"
    Set-Content -LiteralPath (Join-Path $Root "task_prompt.md") -Encoding UTF8 -Value "# prompt"
    (@{
        task_id = $TaskId
        task_dir = $Root
        generated_at = $GeneratedAt
        source = @{
            kind = "visual-regression-bundle"
            path = $SourcePath
        }
    } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath (Join-Path $Root "task_manifest.json") -Encoding UTF8
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$workingDirLeaf = Split-Path -Leaf ([System.IO.Path]::GetFullPath($WorkingDir))
$resolvedWorkingDir = Join-Path $resolvedRepoRoot ("output\codex-{0}-{1}" -f $workingDirLeaf, [System.Guid]::NewGuid().ToString("N"))
$tasksRoot = Join-Path $resolvedWorkingDir "tasks"
$gateRoot = Join-Path $resolvedWorkingDir "word-visual-release-gate"
$gateReportDir = Join-Path $gateRoot "report"
$releaseRoot = Join-Path $resolvedWorkingDir "release-candidate-checks"
$releaseReportDir = Join-Path $releaseRoot "report"
$bundleId = "template-table-cli-selector"
$bundleLabel = "Template table CLI selector"
$bundleRoot = Join-Path $gateRoot $bundleId
$bundleSummaryPath = Join-Path $bundleRoot "summary.json"
$syncLatestScript = Join-Path $resolvedRepoRoot "scripts\sync_latest_visual_review_verdict.ps1"

New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $releaseReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $bundleRoot -Force | Out-Null

(@{
    generated_at = "2026-04-21T21:15:00"
    status = "completed"
} | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath $bundleSummaryPath -Encoding UTF8

$oldTaskDir = Join-Path $tasksRoot "$bundleId-task-old"
$newTaskDir = Join-Path $tasksRoot "$bundleId-task-new"
New-TaskReviewSeed -Root $oldTaskDir -TaskId "$bundleId-task-old" -Status "pending_review" -Verdict "undecided" `
    -SourcePath $bundleRoot -GeneratedAt "2026-04-21T21:00:00"
New-TaskReviewSeed -Root $newTaskDir -TaskId "$bundleId-task-new" -Status "reviewed" -Verdict "pass" `
    -SourcePath $bundleRoot -GeneratedAt "2026-04-21T21:05:00"

$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$releaseSummaryPath = Join-Path $releaseReportDir "summary.json"

(@{
    generated_at = "2026-04-21T21:00:00"
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
            task = @{
                task_id = "$bundleId-task-old"
                source_kind = "visual-regression-bundle"
                source_path = $bundleRoot
                mode = "review-only"
                task_dir = $oldTaskDir
                prompt_path = (Join-Path $oldTaskDir "task_prompt.md")
                manifest_path = (Join-Path $oldTaskDir "task_manifest.json")
                review_result_path = (Join-Path $oldTaskDir "report\review_result.json")
                final_review_path = (Join-Path $oldTaskDir "report\final_review.md")
                latest_task_pointer_path = (Join-Path $tasksRoot "latest_task.json")
                latest_source_kind_task_pointer_path = (Join-Path $tasksRoot "latest_${bundleId}-visual-regression-bundle_task.json")
            }
        }
    )
    review_tasks = @{
        curated_visual_regressions = @(
            @{
                id = $bundleId
                label = $bundleLabel
                task = @{
                    task_id = "$bundleId-task-old"
                    source_kind = "visual-regression-bundle"
                    source_path = $bundleRoot
                    mode = "review-only"
                    task_dir = $oldTaskDir
                    prompt_path = (Join-Path $oldTaskDir "task_prompt.md")
                    manifest_path = (Join-Path $oldTaskDir "task_manifest.json")
                    review_result_path = (Join-Path $oldTaskDir "report\review_result.json")
                    final_review_path = (Join-Path $oldTaskDir "report\final_review.md")
                    latest_task_pointer_path = (Join-Path $tasksRoot "latest_task.json")
                    latest_source_kind_task_pointer_path = (Join-Path $tasksRoot "latest_${bundleId}-visual-regression-bundle_task.json")
                }
            }
        )
    }
} | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-21T21:00:00"
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    gate_output_dir = $gateRoot
    task_output_root = $tasksRoot
    build_dir = (Join-Path $resolvedWorkingDir "build")
    install_dir = (Join-Path $resolvedWorkingDir "install")
    consumer_build_dir = (Join-Path $resolvedWorkingDir "consumer")
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
            curated_visual_regressions = @(
                @{
                    id = $bundleId
                    source_kind = "visual-regression-bundle"
                    source_path = $bundleRoot
                    task_dir = $oldTaskDir
                }
            )
        }
    }
} | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-21T21:05:00"
    workspace = $resolvedRepoRoot
    task_root = $tasksRoot
    task_root_relative = "tasks"
    task_id = "$bundleId-task-new"
    mode = "review-only"
    source = @{
        kind = "visual-regression-bundle"
        label = "$bundleLabel bundle"
        path = $bundleRoot
    }
    document = @{
        name = $bundleId
        path = $bundleRoot
    }
    task_dir = $newTaskDir
    task_dir_relative = "tasks\$bundleId-task-new"
    manifest_path = (Join-Path $newTaskDir "task_manifest.json")
    manifest_path_relative = "tasks\$bundleId-task-new\task_manifest.json"
    prompt_path = (Join-Path $newTaskDir "task_prompt.md")
    prompt_path_relative = "tasks\$bundleId-task-new\task_prompt.md"
    evidence_dir = (Join-Path $newTaskDir "evidence")
    report_dir = (Join-Path $newTaskDir "report")
    repair_dir = (Join-Path $newTaskDir "repair")
    review_result_path = (Join-Path $newTaskDir "report\review_result.json")
    review_result_path_relative = "tasks\$bundleId-task-new\report\review_result.json"
    final_review_path = (Join-Path $newTaskDir "report\final_review.md")
    final_review_path_relative = "tasks\$bundleId-task-new\report\final_review.md"
    pointer_files = @{
        generic = (Join-Path $tasksRoot "latest_task.json")
        source_kind = (Join-Path $tasksRoot "latest_${bundleId}-visual-regression-bundle_task.json")
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath (Join-Path $tasksRoot "latest_${bundleId}-visual-regression-bundle_task.json") -Encoding UTF8

& $syncLatestScript `
    -TaskOutputRoot $tasksRoot `
    -ReleaseCandidateSummaryJson $releaseSummaryPath `
    -SkipReleaseBundle

$gateSummary = Get-Content -Raw -LiteralPath $gateSummaryPath | ConvertFrom-Json
$releaseSummary = Get-Content -Raw -LiteralPath $releaseSummaryPath | ConvertFrom-Json
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$releaseFinalReviewPath = Join-Path $releaseReportDir "final_review.md"
$releaseHandoffPath = Join-Path $releaseReportDir "release_handoff.md"
$releaseBodyPath = Join-Path $releaseReportDir "release_body.zh-CN.md"
$releaseSummaryZhPath = Join-Path $releaseReportDir "release_summary.zh-CN.md"
$artifactGuidePath = Join-Path $releaseReportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistPath = Join-Path $releaseReportDir "REVIEWER_CHECKLIST.md"
$startHerePath = Join-Path $releaseRoot "START_HERE.md"
$supersededReviewTasksReportPath = Join-Path $tasksRoot "superseded_review_tasks.json"
$curatedGateTasks = @($gateSummary.review_tasks.curated_visual_regressions)
$curatedManualReviews = @($gateSummary.manual_review.tasks.curated_visual_regressions)
$curatedReleaseTasks = @($releaseSummary.steps.visual_gate.curated_visual_regressions)

Assert-True -Condition (Test-Path -LiteralPath $supersededReviewTasksReportPath) `
    -Message "Superseded review-task audit report was not generated."

$supersededReviewTasksReport = Get-Content -Raw -LiteralPath $supersededReviewTasksReportPath | ConvertFrom-Json

Assert-True -Condition ($curatedGateTasks.Count -eq 1) `
    -Message "Gate summary review_tasks is missing the curated visual bundle entry."
Assert-True -Condition ($curatedGateTasks[0].task.task_dir -eq $newTaskDir) `
    -Message "Gate summary curated bundle task_dir was not refreshed from the latest pointer."
Assert-True -Condition ($gateSummary.curated_visual_regressions[0].task.task_dir -eq $newTaskDir) `
    -Message "Gate summary curated_visual_regressions flow task was not refreshed from the latest pointer."
Assert-True -Condition ($curatedManualReviews.Count -eq 1) `
    -Message "Gate summary manual_review is missing the curated visual bundle entry."
Assert-True -Condition ($curatedManualReviews[0].task_dir -eq $newTaskDir) `
    -Message "Gate summary manual_review did not follow the refreshed curated task pointer."
Assert-True -Condition ($gateSummary.visual_verdict -eq "pass") `
    -Message "Gate summary visual verdict was not recomputed from the refreshed curated task."
Assert-True -Condition ($supersededReviewTasksReport.superseded_task_count -eq 1) `
    -Message "Superseded review-task audit did not record the expected stale curated task count."
Assert-True -Condition ($gateSummary.selected_release_summary_path -eq $releaseSummaryPath) `
    -Message "Gate summary selected_release_summary_path did not record the explicit release summary."
Assert-True -Condition ($gateSummary.release_summary_discovery.mode -eq "explicit") `
    -Message "Gate summary release_summary_discovery.mode did not record explicit discovery."
Assert-True -Condition ($gateSummary.release_summary_discovery.reason -eq "explicit_path") `
    -Message "Gate summary release_summary_discovery.reason did not record the explicit path."
Assert-True -Condition ($gateSummary.release_summary_discovery.release_bundle_refresh_requested -eq $false) `
    -Message "Gate summary release_summary_discovery.release_bundle_refresh_requested should be false with -SkipReleaseBundle."
Assert-True -Condition ($curatedReleaseTasks.Count -eq 1) `
    -Message "Release summary visual_gate is missing the curated visual bundle entry."
Assert-True -Condition ($curatedReleaseTasks[0].id -eq $bundleId) `
    -Message "Release summary visual_gate did not record the curated bundle id."
Assert-True -Condition ($curatedReleaseTasks[0].task_dir -eq $newTaskDir) `
    -Message "Release summary visual_gate did not refresh the curated bundle task dir."

Assert-Contains -Path $gateFinalReviewPath -ExpectedText "$bundleLabel verdict: pass" -Label "gate_final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "Visual verdict: pass" -Label "release final_review.md"
foreach ($pathToCheck in @($releaseHandoffPath, $releaseBodyPath, $releaseSummaryZhPath, $artifactGuidePath, $reviewerChecklistPath, $startHerePath)) {
    Assert-PathNotExists -Path $pathToCheck -Label $pathToCheck
}

Write-Host "Sync latest visual review verdict curated visual bundle regression passed."
