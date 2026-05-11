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
        [string]$Verdict
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
    Set-Content -LiteralPath (Join-Path $Root "task_manifest.json") -Encoding UTF8 -Value "{}"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$workingDirLeaf = Split-Path -Leaf ([System.IO.Path]::GetFullPath($WorkingDir))
$resolvedWorkingDir = Join-Path $resolvedRepoRoot ("output\codex-{0}-{1}" -f $workingDirLeaf, [System.Guid]::NewGuid().ToString("N"))
$tasksRoot = Join-Path $resolvedWorkingDir "tasks"
$documentGateRoot = Join-Path $resolvedWorkingDir "document-word-visual-release-gate"
$fixedGridGateRoot = Join-Path $resolvedWorkingDir "fixed-grid-word-visual-release-gate"
$documentGateReportDir = Join-Path $documentGateRoot "report"
$fixedGridGateReportDir = Join-Path $fixedGridGateRoot "report"
$documentSmokeDir = Join-Path $documentGateRoot "smoke"
$fixedGridBundleDir = Join-Path $fixedGridGateRoot "fixed-grid-regression-bundle"
$syncLatestScript = Join-Path $resolvedRepoRoot "scripts\sync_latest_visual_review_verdict.ps1"

New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null
New-Item -ItemType Directory -Path $documentGateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $fixedGridGateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $documentSmokeDir -Force | Out-Null
New-Item -ItemType Directory -Path $fixedGridBundleDir -Force | Out-Null

$documentPath = Join-Path $documentSmokeDir "table_visual_smoke.docx"
Set-Content -LiteralPath $documentPath -Encoding UTF8 -Value "docx"

$documentTaskDir = Join-Path $tasksRoot "document-task"
$fixedGridTaskDir = Join-Path $tasksRoot "fixed-grid-task"
New-TaskReviewSeed -Root $documentTaskDir -TaskId "document-task" -Status "reviewed" -Verdict "pass"
New-TaskReviewSeed -Root $fixedGridTaskDir -TaskId "fixed-grid-task" -Status "reviewed" -Verdict "pass"

$documentGateSummaryPath = Join-Path $documentGateReportDir "gate_summary.json"
$fixedGridGateSummaryPath = Join-Path $fixedGridGateReportDir "gate_summary.json"

(@{
    generated_at = "2026-04-16T11:00:00"
    gate_output_dir = $documentGateRoot
    report_dir = $documentGateReportDir
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    smoke = @{
        status = "completed"
        docx_path = $documentPath
    }
    review_tasks = @{
        document = @{
            task_id = "stale-document-task"
            task_dir = (Join-Path $tasksRoot "stale-document-task")
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $documentGateSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-16T11:00:00"
    gate_output_dir = $fixedGridGateRoot
    report_dir = $fixedGridGateReportDir
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    fixed_grid = @{
        status = "completed"
        output_dir = $fixedGridBundleDir
    }
    review_tasks = @{
        fixed_grid = @{
            task_id = "stale-fixed-grid-task"
            task_dir = (Join-Path $tasksRoot "stale-fixed-grid-task")
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $fixedGridGateSummaryPath -Encoding UTF8

$documentGateSummaryBefore = Get-Content -Raw -LiteralPath $documentGateSummaryPath
$fixedGridGateSummaryBefore = Get-Content -Raw -LiteralPath $fixedGridGateSummaryPath

(@{
    generated_at = "2026-04-16T11:05:00"
    workspace = $resolvedRepoRoot
    task_root = $tasksRoot
    task_id = "document-task"
    mode = "review-only"
    source = @{
        kind = "document"
        label = "Target document"
        path = $documentPath
    }
    document = @{
        name = "table_visual_smoke.docx"
        path = $documentPath
    }
    task_dir = $documentTaskDir
    manifest_path = (Join-Path $documentTaskDir "task_manifest.json")
    prompt_path = (Join-Path $documentTaskDir "task_prompt.md")
    evidence_dir = (Join-Path $documentTaskDir "evidence")
    report_dir = (Join-Path $documentTaskDir "report")
    repair_dir = (Join-Path $documentTaskDir "repair")
    review_result_path = (Join-Path $documentTaskDir "report\review_result.json")
    final_review_path = (Join-Path $documentTaskDir "report\final_review.md")
    pointer_files = @{
        generic = (Join-Path $tasksRoot "latest_task.json")
        source_kind = (Join-Path $tasksRoot "latest_document_task.json")
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath (Join-Path $tasksRoot "latest_document_task.json") -Encoding UTF8

(@{
    generated_at = "2026-04-16T11:06:00"
    workspace = $resolvedRepoRoot
    task_root = $tasksRoot
    task_id = "fixed-grid-task"
    mode = "review-only"
    source = @{
        kind = "fixed-grid-regression-bundle"
        label = "Fixed-grid bundle"
        path = $fixedGridBundleDir
    }
    document = @{
        name = "fixed-grid-regression-bundle"
        path = $fixedGridBundleDir
    }
    task_dir = $fixedGridTaskDir
    manifest_path = (Join-Path $fixedGridTaskDir "task_manifest.json")
    prompt_path = (Join-Path $fixedGridTaskDir "task_prompt.md")
    evidence_dir = (Join-Path $fixedGridTaskDir "evidence")
    report_dir = (Join-Path $fixedGridTaskDir "report")
    repair_dir = (Join-Path $fixedGridTaskDir "repair")
    review_result_path = (Join-Path $fixedGridTaskDir "report\review_result.json")
    final_review_path = (Join-Path $fixedGridTaskDir "report\final_review.md")
    pointer_files = @{
        generic = (Join-Path $tasksRoot "latest_task.json")
        source_kind = (Join-Path $tasksRoot "latest_fixed-grid-regression-bundle_task.json")
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath (Join-Path $tasksRoot "latest_fixed-grid-regression-bundle_task.json") -Encoding UTF8

$failedAsExpected = $false
$errorText = ""
try {
    & $syncLatestScript `
        -TaskOutputRoot $tasksRoot `
        -OutputSearchRoot $resolvedWorkingDir
} catch {
    $failedAsExpected = $true
    $errorText = $_.Exception.Message
}

Assert-True -Condition $failedAsExpected `
    -Message "sync_latest_visual_review_verdict.ps1 unexpectedly accepted conflicting latest task gates."
Assert-True -Condition ($errorText -match [regex]::Escape("Latest task pointers do not agree on one gate output directory")) `
    -Message "Unexpected conflict error message: $errorText"

Assert-True -Condition ((Get-Content -Raw -LiteralPath $documentGateSummaryPath) -eq $documentGateSummaryBefore) `
    -Message "Document gate summary should not be mutated after latest-task gate conflict."
Assert-True -Condition ((Get-Content -Raw -LiteralPath $fixedGridGateSummaryPath) -eq $fixedGridGateSummaryBefore) `
    -Message "Fixed-grid gate summary should not be mutated after latest-task gate conflict."
Assert-PathNotExists -Path (Join-Path $tasksRoot "superseded_review_tasks.json") -Label "superseded review-task report"
Assert-PathNotExists -Path (Join-Path $documentGateReportDir "gate_final_review.md") -Label "document gate final review"
Assert-PathNotExists -Path (Join-Path $fixedGridGateReportDir "gate_final_review.md") -Label "fixed-grid gate final review"

Write-Host "Sync latest visual review verdict conflicting gate regression passed."
