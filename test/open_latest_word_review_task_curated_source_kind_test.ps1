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

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$taskRoot = Join-Path $resolvedWorkingDir "tasks"
$taskDir = Join-Path $taskRoot "table-style-quality-task"
$reportDir = Join-Path $taskDir "report"
$promptPath = Join-Path $taskDir "task_prompt.md"
$manifestPath = Join-Path $taskDir "task_manifest.json"
$reviewResultPath = Join-Path $reportDir "review_result.json"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$bundleRoot = Join-Path $resolvedWorkingDir "table-style-quality-bundle"
$sourceKind = "table-style-quality-visual-regression-bundle"
$pointerPath = Join-Path $taskRoot "latest_table-style-quality-visual-regression-bundle_task.json"
$openScript = Join-Path $resolvedRepoRoot "scripts\open_latest_word_review_task.ps1"

New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $bundleRoot -Force | Out-Null

Set-Content -LiteralPath $promptPath -Encoding UTF8 -Value @"
# Table Style Quality Visual Review Task

- before
- after
"@
Set-Content -LiteralPath $manifestPath -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath $reviewResultPath -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath $finalReviewPath -Encoding UTF8 -Value "# final review"

(@{
    task_id = "table-style-quality-task"
    generated_at = "2026-04-13T12:00:00"
    mode = "review-only"
    task_dir = $taskDir
    prompt_path = $promptPath
    manifest_path = $manifestPath
    review_result_path = $reviewResultPath
    final_review_path = $finalReviewPath
    source = @{
        kind = "visual-regression-bundle"
        path = $bundleRoot
    }
} | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $pointerPath -Encoding UTF8

Assert-True -Condition (Test-Path -LiteralPath $openScript) `
    -Message "Generic latest task resolver script was not found: $openScript"

$resolvedPointerPath = (& $openScript -TaskOutputRoot $taskRoot -SourceKind $sourceKind -PrintField pointer_path | Out-String).Trim()
Assert-True -Condition ([System.IO.Path]::GetFullPath($resolvedPointerPath) -eq [System.IO.Path]::GetFullPath($pointerPath)) `
    -Message "Resolver returned an unexpected pointer_path: $resolvedPointerPath"

$resolvedTaskDir = (& $openScript -TaskOutputRoot $taskRoot -SourceKind $sourceKind -PrintField task_dir | Out-String).Trim()
Assert-True -Condition ([System.IO.Path]::GetFullPath($resolvedTaskDir) -eq [System.IO.Path]::GetFullPath($taskDir)) `
    -Message "Resolver returned an unexpected task_dir: $resolvedTaskDir"

$resolvedSourceKind = (& $openScript -TaskOutputRoot $taskRoot -SourceKind $sourceKind -PrintField source_kind | Out-String).Trim()
Assert-True -Condition ($resolvedSourceKind -eq "visual-regression-bundle") `
    -Message "Resolver returned an unexpected source_kind: $resolvedSourceKind"

$resolvedDocumentPath = (& $openScript -TaskOutputRoot $taskRoot -SourceKind $sourceKind -PrintField document_path | Out-String).Trim()
Assert-True -Condition ($resolvedDocumentPath -eq "") `
    -Message "Resolver should return an empty document_path for curated visual bundles: $resolvedDocumentPath"

$promptText = (& $openScript -TaskOutputRoot $taskRoot -SourceKind $sourceKind -PromptOnly | Out-String)
Assert-True -Condition ($promptText.Contains("Table Style Quality Visual Review Task")) `
    -Message "PromptOnly did not return the expected curated prompt content."
Assert-True -Condition ($promptText.Contains("before")) `
    -Message "PromptOnly output did not include before."
Assert-True -Condition ($promptText.Contains("after")) `
    -Message "PromptOnly output did not include after."

$jsonOutput = (& $openScript -TaskOutputRoot $taskRoot -SourceKind $sourceKind -AsJson | Out-String)
$payload = $jsonOutput | ConvertFrom-Json

Assert-True -Condition ($payload.task_id -eq "table-style-quality-task") `
    -Message "AsJson returned an unexpected task id."
Assert-True -Condition ($payload.source.kind -eq "visual-regression-bundle") `
    -Message "AsJson returned an unexpected source kind."
Assert-True -Condition ([System.IO.Path]::GetFullPath($payload.pointer_path) -eq [System.IO.Path]::GetFullPath($pointerPath)) `
    -Message "AsJson returned an unexpected pointer path."
Assert-True -Condition ([System.IO.Path]::GetFullPath($payload.source.path) -eq [System.IO.Path]::GetFullPath($bundleRoot)) `
    -Message "AsJson returned an unexpected source path."

Write-Host "Open latest Word review task curated SourceKind passed."
