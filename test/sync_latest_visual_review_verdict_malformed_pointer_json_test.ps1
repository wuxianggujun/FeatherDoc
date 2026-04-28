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
$syncLatestScript = Join-Path $resolvedRepoRoot "scripts\sync_latest_visual_review_verdict.ps1"
$pointerPath = Join-Path $tasksRoot "latest_document_task.json"

New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null
Set-Content -LiteralPath $pointerPath -Encoding UTF8 -Value '{"task_id":"document-task",'

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
    -Message "sync_latest_visual_review_verdict.ps1 unexpectedly accepted malformed latest-task JSON."
Assert-True -Condition ($errorText -match [regex]::Escape("Failed to parse JSON file: $pointerPath")) `
    -Message "Malformed pointer error did not include the pointer path: $errorText"

Assert-PathNotExists -Path (Join-Path $tasksRoot "superseded_review_tasks.json") -Label "superseded review-task report"
Assert-PathNotExists -Path (Join-Path $resolvedWorkingDir "word-visual-release-gate\report\gate_summary.json") -Label "inferred gate summary"
Assert-PathNotExists -Path (Join-Path $resolvedWorkingDir "release-candidate-checks\report\summary.json") -Label "release summary"

Write-Host "Sync latest visual review verdict malformed pointer JSON regression passed."
