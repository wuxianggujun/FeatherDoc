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
$taskDir = Join-Path $taskRoot "page-number-fields-task"
$reportDir = Join-Path $taskDir "report"
$promptPath = Join-Path $taskDir "task_prompt.md"
$manifestPath = Join-Path $taskDir "task_manifest.json"
$reviewResultPath = Join-Path $reportDir "review_result.json"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$bundleRoot = Join-Path $resolvedWorkingDir "page-number-fields-bundle"
$pointerPath = Join-Path $taskRoot "latest_page-number-fields-regression-bundle_task.json"
$openScript = Join-Path $resolvedRepoRoot "scripts\open_latest_page_number_fields_review_task.ps1"

New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $bundleRoot -Force | Out-Null

Set-Content -LiteralPath $promptPath -Encoding UTF8 -Value @"
# Page Number Fields Visual Review Task

- api-sample
- cli-append
"@
Set-Content -LiteralPath $manifestPath -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath $reviewResultPath -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath $finalReviewPath -Encoding UTF8 -Value "# final review"

(@{
    task_id = "page-number-fields-task"
    generated_at = "2026-04-13T12:00:00"
    mode = "review-only"
    task_dir = $taskDir
    prompt_path = $promptPath
    manifest_path = $manifestPath
    review_result_path = $reviewResultPath
    final_review_path = $finalReviewPath
    source = @{
        kind = "page-number-fields-regression-bundle"
        path = $bundleRoot
    }
} | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $pointerPath -Encoding UTF8

Assert-True -Condition (Test-Path -LiteralPath $openScript) `
    -Message "Page number fields task resolver script was not found: $openScript"

$resolvedTaskDir = (& $openScript -TaskOutputRoot $taskRoot -PrintField task_dir | Out-String).Trim()
Assert-True -Condition ([System.IO.Path]::GetFullPath($resolvedTaskDir) -eq [System.IO.Path]::GetFullPath($taskDir)) `
    -Message "Resolver returned an unexpected task_dir: $resolvedTaskDir"

$resolvedSourceKind = (& $openScript -TaskOutputRoot $taskRoot -PrintField source_kind | Out-String).Trim()
Assert-True -Condition ($resolvedSourceKind -eq "page-number-fields-regression-bundle") `
    -Message "Resolver returned an unexpected source_kind: $resolvedSourceKind"

$promptText = (& $openScript -TaskOutputRoot $taskRoot -PromptOnly | Out-String)
Assert-True -Condition ($promptText.Contains("Page Number Fields Visual Review Task")) `
    -Message "PromptOnly did not return the expected prompt content."
Assert-True -Condition ($promptText.Contains("api-sample")) `
    -Message "PromptOnly output did not include api-sample."
Assert-True -Condition ($promptText.Contains("cli-append")) `
    -Message "PromptOnly output did not include cli-append."

$jsonOutput = (& $openScript -TaskOutputRoot $taskRoot -AsJson | Out-String)
$payload = $jsonOutput | ConvertFrom-Json

Assert-True -Condition ($payload.task_id -eq "page-number-fields-task") `
    -Message "AsJson returned an unexpected task id."
Assert-True -Condition ($payload.source.kind -eq "page-number-fields-regression-bundle") `
    -Message "AsJson returned an unexpected source kind."
Assert-True -Condition ([System.IO.Path]::GetFullPath($payload.pointer_path) -eq [System.IO.Path]::GetFullPath($pointerPath)) `
    -Message "AsJson returned an unexpected pointer path."

Write-Host "Open latest page number fields review task wrapper passed."
