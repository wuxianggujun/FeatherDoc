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

function Assert-NotContains {
    param(
        [string]$Path,
        [string]$UnexpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -match [regex]::Escape($UnexpectedText)) {
        throw "$Label contains unexpected text '$UnexpectedText': $Path"
    }
}

function New-TaskReviewSeed {
    param(
        [string]$Root,
        [string]$TaskId,
        [string]$Status,
        [string]$Verdict,
        [string]$SourceKind,
        [string]$SourcePath,
        [string]$GeneratedAt,
        [string]$ReviewedAt = "",
        [string]$ReviewMethod = "",
        [string]$ReviewNote = ""
    )

    $reportDir = Join-Path $Root "report"
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

    (@{
        task_id = $TaskId
        status = $Status
        verdict = $Verdict
        reviewed_at = $ReviewedAt
        review_method = $ReviewMethod
        review_note = $ReviewNote
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
            kind = $SourceKind
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
$smokeDir = Join-Path $gateRoot "smoke"
$syncLatestScript = Join-Path $resolvedRepoRoot "scripts\sync_latest_visual_review_verdict.ps1"

New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $releaseReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $smokeDir -Force | Out-Null

$docxPath = Join-Path $smokeDir "table_visual_smoke.docx"
Set-Content -LiteralPath $docxPath -Encoding UTF8 -Value "docx"

$oldDocumentTaskDir = Join-Path $tasksRoot "document-task-old"
$newDocumentTaskDir = Join-Path $tasksRoot "document-task-new"
New-TaskReviewSeed -Root $oldDocumentTaskDir -TaskId "document-task-old" -Status "pending_review" -Verdict "undecided" `
    -SourceKind "document" -SourcePath $docxPath -GeneratedAt "2026-04-14T09:00:00"
New-TaskReviewSeed -Root $newDocumentTaskDir -TaskId "document-task-new" -Status "reviewed" -Verdict "pass" `
    -SourceKind "document" -SourcePath $docxPath -GeneratedAt "2026-04-14T09:05:00" `
    -ReviewedAt "2026-04-14T09:05:00" -ReviewMethod "operator_supplied" `
    -ReviewNote "latest document sign-off note"

$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$releaseSummaryPath = Join-Path $releaseReportDir "summary.json"

(@{
    generated_at = "2026-04-14T09:00:00"
    gate_output_dir = $gateRoot
    report_dir = $gateReportDir
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    readme_gallery = @{ status = "not_requested" }
    smoke = @{
        status = "completed"
        output_dir = $smokeDir
        docx_path = $docxPath
        task = @{
            task_id = "document-task-old"
            source_kind = "document"
            source_path = $docxPath
            document_path = $docxPath
            mode = "review-only"
            task_dir = $oldDocumentTaskDir
            prompt_path = (Join-Path $oldDocumentTaskDir "task_prompt.md")
            manifest_path = (Join-Path $oldDocumentTaskDir "task_manifest.json")
            review_result_path = (Join-Path $oldDocumentTaskDir "report\review_result.json")
            final_review_path = (Join-Path $oldDocumentTaskDir "report\final_review.md")
            latest_task_pointer_path = (Join-Path $tasksRoot "latest_task.json")
            latest_source_kind_task_pointer_path = (Join-Path $tasksRoot "latest_document_task.json")
        }
    }
    review_tasks = @{
        document = @{
            task_id = "document-task-old"
            source_kind = "document"
            source_path = $docxPath
            document_path = $docxPath
            mode = "review-only"
            task_dir = $oldDocumentTaskDir
            prompt_path = (Join-Path $oldDocumentTaskDir "task_prompt.md")
            manifest_path = (Join-Path $oldDocumentTaskDir "task_manifest.json")
            review_result_path = (Join-Path $oldDocumentTaskDir "report\review_result.json")
            final_review_path = (Join-Path $oldDocumentTaskDir "report\final_review.md")
            latest_task_pointer_path = (Join-Path $tasksRoot "latest_task.json")
            latest_source_kind_task_pointer_path = (Join-Path $tasksRoot "latest_document_task.json")
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-14T09:00:00"
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
            document_task_dir = $oldDocumentTaskDir
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-14T09:05:00"
    workspace = $resolvedRepoRoot
    task_root = $tasksRoot
    task_root_relative = "tasks"
    task_id = "document-task-new"
    mode = "review-only"
    source = @{
        kind = "document"
        label = "Target document"
        path = $docxPath
    }
    document = @{
        name = "table_visual_smoke.docx"
        path = $docxPath
    }
    task_dir = $newDocumentTaskDir
    task_dir_relative = "tasks\document-task-new"
    manifest_path = (Join-Path $newDocumentTaskDir "task_manifest.json")
    manifest_path_relative = "tasks\document-task-new\task_manifest.json"
    prompt_path = (Join-Path $newDocumentTaskDir "task_prompt.md")
    prompt_path_relative = "tasks\document-task-new\task_prompt.md"
    evidence_dir = (Join-Path $newDocumentTaskDir "evidence")
    report_dir = (Join-Path $newDocumentTaskDir "report")
    repair_dir = (Join-Path $newDocumentTaskDir "repair")
    review_result_path = (Join-Path $newDocumentTaskDir "report\review_result.json")
    review_result_path_relative = "tasks\document-task-new\report\review_result.json"
    final_review_path = (Join-Path $newDocumentTaskDir "report\final_review.md")
    final_review_path_relative = "tasks\document-task-new\report\final_review.md"
    pointer_files = @{
        generic = (Join-Path $tasksRoot "latest_task.json")
        source_kind = (Join-Path $tasksRoot "latest_document_task.json")
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath (Join-Path $tasksRoot "latest_document_task.json") -Encoding UTF8

& $syncLatestScript `
    -TaskOutputRoot $tasksRoot `
    -OutputSearchRoot $resolvedWorkingDir

$gateSummary = Get-Content -Raw -LiteralPath $gateSummaryPath | ConvertFrom-Json
$releaseSummary = Get-Content -Raw -LiteralPath $releaseSummaryPath | ConvertFrom-Json
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$releaseFinalReviewPath = Join-Path $releaseReportDir "final_review.md"
$releaseHandoffPath = Join-Path $releaseReportDir "release_handoff.md"
$artifactGuidePath = Join-Path $releaseReportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistPath = Join-Path $releaseReportDir "REVIEWER_CHECKLIST.md"
$startHerePath = Join-Path $releaseRoot "START_HERE.md"
$releaseBodyPath = Join-Path $releaseReportDir "release_body.zh-CN.md"
$supersededReviewTasksReportPath = Join-Path $tasksRoot "superseded_review_tasks.json"

Assert-True -Condition (Test-Path -LiteralPath $supersededReviewTasksReportPath) `
    -Message "Superseded review-task audit report was not generated."

$supersededReviewTasksReport = Get-Content -Raw -LiteralPath $supersededReviewTasksReportPath | ConvertFrom-Json

Assert-True -Condition ($gateSummary.review_tasks.document.task_dir -eq $newDocumentTaskDir) `
    -Message "Gate summary document task_dir was not refreshed from latest_document_task.json."
Assert-True -Condition ($gateSummary.smoke.task.task_dir -eq $newDocumentTaskDir) `
    -Message "Gate summary smoke.task.task_dir was not refreshed from latest_document_task.json."
Assert-True -Condition ($gateSummary.manual_review.tasks.document.task_dir -eq $newDocumentTaskDir) `
    -Message "Manual review document task_dir did not follow the refreshed task pointer."
Assert-True -Condition ($gateSummary.visual_verdict -eq "pass") `
    -Message "Gate summary visual verdict was not recomputed from the refreshed document task."
Assert-True -Condition ($supersededReviewTasksReport.superseded_task_count -eq 1) `
    -Message "Superseded review-task audit did not record the expected stale task count."
Assert-True -Condition ($gateSummary.superseded_review_tasks_report -eq $supersededReviewTasksReportPath) `
    -Message "Gate summary superseded_review_tasks_report was not updated."
Assert-True -Condition ($releaseSummary.steps.visual_gate.document_task_dir -eq $newDocumentTaskDir) `
    -Message "Release summary visual_gate.document_task_dir was not refreshed from latest_document_task.json."
Assert-True -Condition ($releaseSummary.steps.visual_gate.document_verdict -eq "pass") `
    -Message "Release summary visual_gate.document_verdict was not updated after refreshing the task pointer."
Assert-True -Condition ($releaseSummary.superseded_review_tasks_report -eq $supersededReviewTasksReportPath) `
    -Message "Release summary superseded_review_tasks_report was not updated."
Assert-True -Condition ($releaseSummary.steps.visual_gate.superseded_review_tasks_report -eq $supersededReviewTasksReportPath) `
    -Message "Release summary visual_gate superseded_review_tasks_report was not updated."

Assert-Contains -Path $gateFinalReviewPath -ExpectedText "Document verdict: pass" -Label "gate_final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "Visual verdict: pass" -Label "release final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "Smoke: verdict=pass, review_status=reviewed, reviewed_at=2026-04-14T09:05:00, review_method=operator_supplied" -Label "release final_review.md"

foreach ($pathToCheck in @($releaseHandoffPath, $artifactGuidePath, $reviewerChecklistPath, $startHerePath)) {
    Assert-Contains -Path $pathToCheck -ExpectedText "Smoke reviewed at: 2026-04-14T09:05:00" -Label $pathToCheck
    Assert-Contains -Path $pathToCheck -ExpectedText "Smoke review method: operator_supplied" -Label $pathToCheck
}

Assert-NotContains -Path $releaseBodyPath -UnexpectedText "2026-04-14T09:05:00" -Label "release_body.zh-CN.md"
Assert-NotContains -Path $releaseBodyPath -UnexpectedText "operator_supplied" -Label "release_body.zh-CN.md"
Assert-NotContains -Path $releaseBodyPath -UnexpectedText "latest document sign-off note" -Label "release_body.zh-CN.md"

Write-Host "Sync latest visual review verdict regression passed."
