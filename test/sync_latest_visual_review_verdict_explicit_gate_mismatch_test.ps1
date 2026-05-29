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
        [string]$TaskId
    )

    $reportDir = Join-Path $Root "report"
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

    (@{
        task_id = $TaskId
        status = "reviewed"
        verdict = "pass"
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
$gateRoot = Join-Path $resolvedWorkingDir "word-visual-release-gate"
$gateReportDir = Join-Path $gateRoot "report"
$otherGateRoot = Join-Path $resolvedWorkingDir "other-word-visual-release-gate"
$otherGateReportDir = Join-Path $otherGateRoot "report"
$documentSmokeDir = Join-Path $gateRoot "smoke"
$documentTaskDir = Join-Path $tasksRoot "document-task"
$syncLatestScript = Join-Path $resolvedRepoRoot "scripts\sync_latest_visual_review_verdict.ps1"

New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $otherGateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $documentSmokeDir -Force | Out-Null
New-TaskReviewSeed -Root $documentTaskDir -TaskId "document-task"

$documentPath = Join-Path $documentSmokeDir "table_visual_smoke.docx"
Set-Content -LiteralPath $documentPath -Encoding UTF8 -Value "docx"

$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$otherGateSummaryPath = Join-Path $otherGateReportDir "gate_summary.json"

(@{
    generated_at = "2026-04-18T10:00:00"
    gate_output_dir = $gateRoot
    report_dir = $gateReportDir
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    smoke = @{ status = "completed"; docx_path = $documentPath }
    review_tasks = @{ document = @{ task_id = "stale-document-task"; task_dir = (Join-Path $tasksRoot "stale-document-task") } }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-18T09:00:00"
    gate_output_dir = $otherGateRoot
    report_dir = $otherGateReportDir
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    review_tasks = @{}
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $otherGateSummaryPath -Encoding UTF8

$gateSummaryBefore = Get-Content -Raw -LiteralPath $gateSummaryPath
$otherGateSummaryBefore = Get-Content -Raw -LiteralPath $otherGateSummaryPath

(@{
    generated_at = "2026-04-18T10:05:00"
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

$failedAsExpected = $false
$errorText = ""
try {
    & $syncLatestScript `
        -TaskOutputRoot $tasksRoot `
        -GateSummaryJson $otherGateSummaryPath `
        -OutputSearchRoot $resolvedWorkingDir `
        -SkipSupersededReviewTaskAudit
} catch {
    $failedAsExpected = $true
    $errorText = $_.Exception.Message
}

Assert-True -Condition $failedAsExpected `
    -Message "sync_latest_visual_review_verdict.ps1 unexpectedly accepted a mismatched explicit gate summary."
Assert-True -Condition ($errorText -match [regex]::Escape("Explicit gate summary does not match latest task pointers")) `
    -Message "Unexpected explicit gate mismatch error message: $errorText"

Assert-True -Condition ((Get-Content -Raw -LiteralPath $gateSummaryPath) -eq $gateSummaryBefore) `
    -Message "Inferred gate summary should not be mutated after explicit gate mismatch."
Assert-True -Condition ((Get-Content -Raw -LiteralPath $otherGateSummaryPath) -eq $otherGateSummaryBefore) `
    -Message "Explicit mismatched gate summary should not be mutated."
Assert-PathNotExists -Path (Join-Path $tasksRoot "superseded_review_tasks.json") -Label "superseded review-task report"
Assert-PathNotExists -Path (Join-Path $gateReportDir "gate_final_review.md") -Label "inferred gate final review"
Assert-PathNotExists -Path (Join-Path $otherGateReportDir "gate_final_review.md") -Label "explicit gate final review"

Write-Host "Sync latest visual review verdict explicit gate mismatch regression passed."
