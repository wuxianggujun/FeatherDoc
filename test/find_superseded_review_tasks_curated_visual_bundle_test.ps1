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
$tasksRoot = Join-Path $resolvedWorkingDir "tasks"
$sourceRoot = Join-Path $resolvedWorkingDir "source"
$scriptPath = Join-Path $resolvedRepoRoot "scripts\find_superseded_review_tasks.ps1"
$reportPath = Join-Path $resolvedWorkingDir "superseded_review_tasks.json"
$bundleRoot = Join-Path $sourceRoot "template-table-cli-selector"
$bundlePointerPath = Join-Path $tasksRoot "latest_template-table-cli-selector-visual-regression-bundle_task.json"

New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null
New-Item -ItemType Directory -Path $bundleRoot -Force | Out-Null

function New-TaskManifest {
    param(
        [string]$TaskDir,
        [string]$TaskId,
        [string]$GeneratedAt
    )

    New-Item -ItemType Directory -Path $TaskDir -Force | Out-Null
    (@{
        task_id = $TaskId
        generated_at = $GeneratedAt
        task_dir = $TaskDir
        source = @{
            kind = "visual-regression-bundle"
            path = $bundleRoot
        }
    } | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath (Join-Path $TaskDir "task_manifest.json") -Encoding UTF8
}

$taskOldDir = Join-Path $tasksRoot "template-table-cli-selector-20260421-210000-001"
$taskNewDir = Join-Path $tasksRoot "template-table-cli-selector-20260421-220000-002"

New-TaskManifest -TaskDir $taskOldDir -TaskId "template-table-cli-selector-20260421-210000-001" -GeneratedAt "2026-04-21T21:00:00"
New-TaskManifest -TaskDir $taskNewDir -TaskId "template-table-cli-selector-20260421-220000-002" -GeneratedAt "2026-04-21T22:00:00"

(@{
    task_id = "template-table-cli-selector-20260421-220000-002"
    task_dir = $taskNewDir
    source = @{
        kind = "visual-regression-bundle"
        path = $bundleRoot
    }
} | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $bundlePointerPath -Encoding UTF8

& $scriptPath `
    -TaskOutputRoot $tasksRoot `
    -ReportPath $reportPath

$report = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
$group = @($report.groups)[0]
$supersededTasks = @($group.superseded_tasks)

Assert-True -Condition ($report.superseded_task_count -eq 1) `
    -Message "Expected exactly one superseded curated visual bundle task in the report."
Assert-True -Condition ($group.source_kind -eq "visual-regression-bundle") `
    -Message "Expected the group source_kind to be visual-regression-bundle."
Assert-True -Condition ($group.latest_task.task_dir -eq $taskNewDir) `
    -Message "Expected the newest curated bundle task directory to be selected as latest."
Assert-True -Condition ($group.pointer.pointer_path -eq $bundlePointerPath) `
    -Message "Expected the bundle-specific latest pointer to be reported."
Assert-True -Condition ($group.pointer.matches_group_latest -eq $true) `
    -Message "Expected the curated bundle latest pointer to match the newest task in the group."
Assert-True -Condition ($supersededTasks.Count -eq 1 -and $supersededTasks[0].task_dir -eq $taskOldDir) `
    -Message "Expected the older curated bundle task directory to be marked as superseded."

Write-Host "Find superseded curated visual bundle review tasks regression passed."
