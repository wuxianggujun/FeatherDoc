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

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$workingDirLeaf = Split-Path -Leaf ([System.IO.Path]::GetFullPath($WorkingDir))
$resolvedWorkingDir = Join-Path $resolvedRepoRoot ("output\codex-{0}-{1}" -f $workingDirLeaf, [System.Guid]::NewGuid().ToString("N"))
$tasksRoot = Join-Path $resolvedWorkingDir "tasks"
$gateRoot = Join-Path $resolvedWorkingDir "word-visual-release-gate"
$gateReportDir = Join-Path $gateRoot "report"
$smokeDir = Join-Path $gateRoot "smoke"
$releaseRoot = Join-Path $resolvedWorkingDir "release-candidate-checks"
$releaseReportDir = Join-Path $releaseRoot "report"
$oldDocumentTaskDir = Join-Path $tasksRoot "document-task-old"
$newDocumentTaskDir = Join-Path $tasksRoot "document-task-new"
$syncLatestScript = Join-Path $resolvedRepoRoot "scripts\sync_latest_visual_review_verdict.ps1"

New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $smokeDir -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $newDocumentTaskDir "report") -Force | Out-Null
New-Item -ItemType Directory -Path $releaseReportDir -Force | Out-Null

$docxPath = Join-Path $smokeDir "table_visual_smoke.docx"
Set-Content -LiteralPath $docxPath -Encoding UTF8 -Value "docx"
Set-Content -LiteralPath (Join-Path $newDocumentTaskDir "task_prompt.md") -Encoding UTF8 -Value "# prompt"
Set-Content -LiteralPath (Join-Path $newDocumentTaskDir "task_manifest.json") -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath (Join-Path $newDocumentTaskDir "report\final_review.md") -Encoding UTF8 -Value "# final document-task-new"
(@{
    task_id = "document-task-new"
    status = "reviewed"
    verdict = "pass"
    findings = @()
    notes = @("seed")
} | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath (Join-Path $newDocumentTaskDir "report\review_result.json") -Encoding UTF8

$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$releaseSummaryPath = Join-Path $releaseReportDir "summary.json"

(@{
    generated_at = "2026-04-19T14:00:00"
    gate_output_dir = $gateRoot
    report_dir = $gateReportDir
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    smoke = @{
        status = "completed"
        docx_path = $docxPath
        task = @{
            task_id = "document-task-old"
            task_dir = $oldDocumentTaskDir
        }
    }
    review_tasks = @{
        document = @{
            task_id = "document-task-old"
            task_dir = $oldDocumentTaskDir
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8
$gateSummaryBefore = Get-Content -Raw -LiteralPath $gateSummaryPath

(@{
    generated_at = "2026-04-19T14:05:00"
    workspace = $resolvedRepoRoot
    task_root = $tasksRoot
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
    manifest_path = (Join-Path $newDocumentTaskDir "task_manifest.json")
    prompt_path = (Join-Path $newDocumentTaskDir "task_prompt.md")
    evidence_dir = (Join-Path $newDocumentTaskDir "evidence")
    report_dir = (Join-Path $newDocumentTaskDir "report")
    repair_dir = (Join-Path $newDocumentTaskDir "repair")
    review_result_path = (Join-Path $newDocumentTaskDir "report\review_result.json")
    final_review_path = (Join-Path $newDocumentTaskDir "report\final_review.md")
    pointer_files = @{
        generic = (Join-Path $tasksRoot "latest_task.json")
        source_kind = (Join-Path $tasksRoot "latest_document_task.json")
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath (Join-Path $tasksRoot "latest_document_task.json") -Encoding UTF8

Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8 -Value '{"generated_at":"2026-04-19T14:00:00",'
$releaseSummaryBefore = Get-Content -Raw -LiteralPath $releaseSummaryPath

$failedAsExpected = $false
$errorText = ""
try {
    & $syncLatestScript `
        -TaskOutputRoot $tasksRoot `
        -ReleaseCandidateSummaryJson $releaseSummaryPath
} catch {
    $failedAsExpected = $true
    $errorText = $_.Exception.Message
}

Assert-True -Condition $failedAsExpected `
    -Message "sync_latest_visual_review_verdict.ps1 unexpectedly accepted malformed explicit release summary JSON."
Assert-True -Condition ($errorText -match [regex]::Escape("Failed to parse JSON file: $releaseSummaryPath")) `
    -Message "Malformed release summary error did not include the summary path: $errorText"

Assert-True -Condition ((Get-Content -Raw -LiteralPath $gateSummaryPath) -eq $gateSummaryBefore) `
    -Message "Gate summary should not be mutated when explicit release summary JSON is malformed."
Assert-True -Condition ((Get-Content -Raw -LiteralPath $releaseSummaryPath) -eq $releaseSummaryBefore) `
    -Message "Malformed release summary content should not be rewritten."
Assert-PathNotExists -Path (Join-Path $tasksRoot "superseded_review_tasks.json") -Label "superseded review-task report"
Assert-PathNotExists -Path (Join-Path $gateReportDir "gate_final_review.md") -Label "gate final review"
Assert-PathNotExists -Path (Join-Path $releaseReportDir "final_review.md") -Label "release final review"
Assert-PathNotExists -Path (Join-Path $releaseReportDir "release_handoff.md") -Label "release handoff"

Write-Host "Sync latest visual review verdict malformed release summary regression passed."
