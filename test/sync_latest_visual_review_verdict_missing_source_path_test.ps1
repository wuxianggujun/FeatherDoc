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
$documentTaskDir = Join-Path $tasksRoot "document-task"
$syncLatestScript = Join-Path $resolvedRepoRoot "scripts\sync_latest_visual_review_verdict.ps1"

New-Item -ItemType Directory -Path (Join-Path $documentTaskDir "report") -Force | Out-Null
Set-Content -LiteralPath (Join-Path $documentTaskDir "task_prompt.md") -Encoding UTF8 -Value "# prompt"
Set-Content -LiteralPath (Join-Path $documentTaskDir "task_manifest.json") -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath (Join-Path $documentTaskDir "report\final_review.md") -Encoding UTF8 -Value "# final document-task"
(@{
    task_id = "document-task"
    status = "reviewed"
    verdict = "pass"
    findings = @()
    notes = @("seed")
} | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath (Join-Path $documentTaskDir "report\review_result.json") -Encoding UTF8

(@{
    generated_at = "2026-04-17T12:05:00"
    workspace = $resolvedRepoRoot
    task_root = $tasksRoot
    task_id = "document-task"
    mode = "review-only"
    source = @{
        kind = "document"
        label = "Target document"
    }
    document = @{
        name = "table_visual_smoke.docx"
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
        -OutputSearchRoot $resolvedWorkingDir
} catch {
    $failedAsExpected = $true
    $errorText = $_.Exception.Message
}

Assert-True -Condition $failedAsExpected `
    -Message "sync_latest_visual_review_verdict.ps1 unexpectedly accepted a latest task without source.path."
Assert-True -Condition ($errorText -match [regex]::Escape("Expected document source path was not found")) `
    -Message "Unexpected missing source path error message: $errorText"

Assert-PathNotExists -Path (Join-Path $tasksRoot "superseded_review_tasks.json") -Label "superseded review-task report"
Assert-PathNotExists -Path (Join-Path $resolvedWorkingDir "word-visual-release-gate\report\gate_summary.json") -Label "inferred gate summary"
Assert-PathNotExists -Path (Join-Path $resolvedWorkingDir "release-candidate-checks\report\summary.json") -Label "release summary"

Write-Host "Sync latest visual review verdict missing source path regression passed."
