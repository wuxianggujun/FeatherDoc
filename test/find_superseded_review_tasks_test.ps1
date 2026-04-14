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
$docxPath = Join-Path $sourceRoot "table_visual_smoke.docx"

New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null
New-Item -ItemType Directory -Path $sourceRoot -Force | Out-Null
Set-Content -LiteralPath $docxPath -Encoding UTF8 -Value "docx"

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
            kind = "document"
            path = $docxPath
        }
    } | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath (Join-Path $TaskDir "task_manifest.json") -Encoding UTF8
}

$taskOldDir = Join-Path $tasksRoot "table_visual_smoke-20260414-010000-001"
$taskMidDir = Join-Path $tasksRoot "table_visual_smoke-20260414-020000-002"
$taskNewDir = Join-Path $tasksRoot "table_visual_smoke-20260414-030000-003"

New-TaskManifest -TaskDir $taskOldDir -TaskId "table_visual_smoke-20260414-010000-001" -GeneratedAt "2026-04-14T01:00:00"
New-TaskManifest -TaskDir $taskMidDir -TaskId "table_visual_smoke-20260414-020000-002" -GeneratedAt "2026-04-14T02:00:00"
New-TaskManifest -TaskDir $taskNewDir -TaskId "table_visual_smoke-20260414-030000-003" -GeneratedAt "2026-04-14T03:00:00"

(@{
    task_id = "table_visual_smoke-20260414-030000-003"
    task_dir = $taskNewDir
    source = @{
        kind = "document"
        path = $docxPath
    }
} | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath (Join-Path $tasksRoot "latest_document_task.json") -Encoding UTF8

& $scriptPath `
    -TaskOutputRoot $tasksRoot `
    -ReportPath $reportPath

$report = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
$group = @($report.groups)[0]
$supersededTasks = @($group.superseded_tasks)

Assert-True -Condition ($report.superseded_task_count -eq 2) `
    -Message "Expected exactly two superseded tasks in the report."
Assert-True -Condition ($group.latest_task.task_dir -eq $taskNewDir) `
    -Message "Expected the newest task directory to be selected as latest."
Assert-True -Condition ($group.pointer.matches_group_latest -eq $true) `
    -Message "Expected latest_document_task.json to match the newest task in the group."
Assert-True -Condition ($supersededTasks.Count -eq 2) `
    -Message "Expected both older task directories to be marked as superseded."
Assert-True -Condition (($supersededTasks.task_dir -contains $taskOldDir) -and ($supersededTasks.task_dir -contains $taskMidDir)) `
    -Message "Superseded task list does not contain the expected older task directories."

Write-Host "Find superseded review tasks regression passed."
