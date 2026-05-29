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
$taskDir = Join-Path $taskRoot "fixed-grid-print-task"
$reportDir = Join-Path $taskDir "report"
$promptPath = Join-Path $taskDir "task_prompt.md"
$manifestPath = Join-Path $taskDir "task_manifest.json"
$reviewResultPath = Join-Path $reportDir "review_result.json"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$bundleRoot = Join-Path $resolvedWorkingDir "fixed-grid-print-bundle"
$pointerPath = Join-Path $taskRoot "latest_fixed-grid-regression-bundle_task.json"
$printScript = Join-Path $resolvedRepoRoot "scripts\print_latest_fixed_grid_review_prompt.ps1"

New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $bundleRoot -Force | Out-Null

Set-Content -LiteralPath $promptPath -Encoding UTF8 -Value @"
# Fixed Grid Prompt Shortcut

- review merge-right and merge-down evidence
- keep the verdict in review_result.json
"@
Set-Content -LiteralPath $manifestPath -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath $reviewResultPath -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath $finalReviewPath -Encoding UTF8 -Value "# final review"

(@{
    task_id = "fixed-grid-print-task"
    generated_at = "2026-04-13T12:00:00"
    mode = "review-only"
    task_dir = $taskDir
    prompt_path = $promptPath
    manifest_path = $manifestPath
    review_result_path = $reviewResultPath
    final_review_path = $finalReviewPath
    source = @{
        kind = "fixed-grid-regression-bundle"
        path = $bundleRoot
    }
} | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $pointerPath -Encoding UTF8

Assert-True -Condition (Test-Path -LiteralPath $printScript) `
    -Message "Fixed-grid prompt shortcut script was not found: $printScript"

$promptText = (& $printScript -TaskOutputRoot $taskRoot | Out-String)
Assert-True -Condition ($promptText.Contains("Fixed Grid Prompt Shortcut")) `
    -Message "Prompt shortcut did not return the expected heading."
Assert-True -Condition ($promptText.Contains("review merge-right and merge-down evidence")) `
    -Message "Prompt shortcut did not return the expected review instruction."
Assert-True -Condition ($promptText.Contains("keep the verdict in review_result.json")) `
    -Message "Prompt shortcut did not return the expected verdict instruction."
Assert-True -Condition (-not $promptText.Contains("Pointer file:")) `
    -Message "Prompt shortcut should print only the prompt content."

Write-Host "Print latest fixed-grid review prompt shortcut passed."
