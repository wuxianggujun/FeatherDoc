<#
.SYNOPSIS
Finds superseded review tasks under a task root without deleting anything.

.DESCRIPTION
Scans task_manifest.json files under a review-task root, groups tasks by
source kind plus source path, identifies the newest task in each group, and
marks older siblings as superseded. When a latest pointer exists for that
source group, the report also shows whether it still points at the newest task
in that group.
#>
param(
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks",
    [string]$ReportPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-FullPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    return [System.IO.Path]::GetFullPath($candidate)
}

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        throw "Expected $Label was not found: $Path"
    }
}

function Get-OptionalPropertyValue {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return ""
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Get-OptionalPropertyObject {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Read-JsonFile {
    param([string]$Path)

    Assert-PathExists -Path $Path -Label "JSON file"
    return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
}

$repoRoot = Resolve-RepoRoot
$resolvedTaskOutputRoot = Resolve-FullPath -RepoRoot $repoRoot -InputPath $TaskOutputRoot
Assert-PathExists -Path $resolvedTaskOutputRoot -Label "task output root"

$resolvedReportPath = if ([string]::IsNullOrWhiteSpace($ReportPath)) {
    Join-Path $resolvedTaskOutputRoot "superseded_review_tasks.json"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReportPath
}

$manifestPaths = @(Get-ChildItem -Path $resolvedTaskOutputRoot -Directory | ForEach-Object {
        $candidate = Join-Path $_.FullName "task_manifest.json"
        if (Test-Path -LiteralPath $candidate) {
            $candidate
        }
    })

$taskEntries = @()
foreach ($manifestPath in $manifestPaths) {
    $manifest = Read-JsonFile -Path $manifestPath
    $source = Get-OptionalPropertyObject -Object $manifest -Name "source"
    $sourceKind = Get-OptionalPropertyValue -Object $source -Name "kind"
    $sourcePathValue = Get-OptionalPropertyValue -Object $source -Name "path"
    if ([string]::IsNullOrWhiteSpace($sourceKind) -or [string]::IsNullOrWhiteSpace($sourcePathValue)) {
        continue
    }
    $sourcePath = [System.IO.Path]::GetFullPath($sourcePathValue)

    $taskDir = Get-OptionalPropertyValue -Object $manifest -Name "task_dir"
    if ([string]::IsNullOrWhiteSpace($taskDir)) {
        $taskDir = Split-Path -Parent $manifestPath
    }

    $generatedAtRaw = Get-OptionalPropertyValue -Object $manifest -Name "generated_at"
    $generatedAt = $null
    if (-not [string]::IsNullOrWhiteSpace($generatedAtRaw)) {
        try {
            $generatedAt = [DateTime]::Parse($generatedAtRaw, [Globalization.CultureInfo]::InvariantCulture)
        } catch {
            $generatedAt = $null
        }
    }

    $taskEntries += [pscustomobject]@{
        task_id = Get-OptionalPropertyValue -Object $manifest -Name "task_id"
        task_dir = [System.IO.Path]::GetFullPath($taskDir)
        manifest_path = [System.IO.Path]::GetFullPath($manifestPath)
        generated_at = $generatedAtRaw
        generated_at_sort = if ($null -ne $generatedAt) { $generatedAt } else { [DateTime]::MinValue }
        source_kind = $sourceKind
        source_path = $sourcePath
        group_key = "$sourceKind|$sourcePath"
    }
}

$latestPointers = @{}
$pointerPaths = @(Get-ChildItem -Path $resolvedTaskOutputRoot -File |
    Where-Object {
        $_.Name -like "latest_*_task.json" -and $_.Name -ne "latest_task.json"
    } |
    Select-Object -ExpandProperty FullName)

foreach ($pointerPath in $pointerPaths) {
    $pointer = Read-JsonFile -Path $pointerPath
    $source = Get-OptionalPropertyObject -Object $pointer -Name "source"
    $sourceKind = Get-OptionalPropertyValue -Object $source -Name "kind"
    $pointerSourcePathValue = Get-OptionalPropertyValue -Object $source -Name "path"
    $pointerTaskDirValue = Get-OptionalPropertyValue -Object $pointer -Name "task_dir"
    if ([string]::IsNullOrWhiteSpace($sourceKind) -or
        [string]::IsNullOrWhiteSpace($pointerSourcePathValue) -or
        [string]::IsNullOrWhiteSpace($pointerTaskDirValue)) {
        continue
    }

    $pointerSourcePath = [System.IO.Path]::GetFullPath($pointerSourcePathValue)
    $groupKey = "$sourceKind|$pointerSourcePath"
    if (-not $latestPointers.ContainsKey($groupKey)) {
        $latestPointers[$groupKey] = @()
    }

    $latestPointers[$groupKey] += [pscustomobject]@{
        task_id = Get-OptionalPropertyValue -Object $pointer -Name "task_id"
        task_dir = [System.IO.Path]::GetFullPath($pointerTaskDirValue)
        pointer_path = $pointerPath
    }
}

$groupReports = @()
$supersededCount = 0

foreach ($group in ($taskEntries | Group-Object group_key)) {
    $entries = @($group.Group | Sort-Object `
            @{ Expression = "generated_at_sort"; Descending = $true }, `
            @{ Expression = "task_id"; Descending = $true })
    if ($entries.Count -eq 0) {
        continue
    }

    $latestTask = $entries[0]
    $supersededTasks = if ($entries.Count -gt 1) {
        @($entries[1..($entries.Count - 1)])
    } else {
        @()
    }
    $pointersForGroup = @($latestPointers[$group.Name])
    $pointer = if ($pointersForGroup.Count -gt 0) {
        $matchingPointer = $pointersForGroup |
            Where-Object { [System.StringComparer]::OrdinalIgnoreCase.Equals($_.task_dir, $latestTask.task_dir) } |
            Select-Object -First 1
        if ($null -ne $matchingPointer) {
            $matchingPointer
        } else {
            $pointersForGroup[0]
        }
    } else {
        $null
    }
    $pointerMatches = $false
    if ($null -ne $pointer) {
        $pointerMatches = [System.StringComparer]::OrdinalIgnoreCase.Equals($pointer.task_dir, $latestTask.task_dir)
    }

    $supersededCount += @($supersededTasks).Count
    $groupReports += [pscustomobject]@{
        source_kind = $latestTask.source_kind
        source_path = $latestTask.source_path
        task_count = $entries.Count
        latest_task = [pscustomobject]@{
            task_id = $latestTask.task_id
            task_dir = $latestTask.task_dir
            generated_at = $latestTask.generated_at
        }
        pointer = if ($null -ne $pointer) {
            [pscustomobject]@{
                task_id = $pointer.task_id
                task_dir = $pointer.task_dir
                pointer_path = $pointer.pointer_path
                matches_group_latest = $pointerMatches
            }
        } else {
            $null
        }
        superseded_tasks = @($supersededTasks | ForEach-Object {
                [pscustomobject]@{
                    task_id = $_.task_id
                    task_dir = $_.task_dir
                    generated_at = $_.generated_at
                }
            })
    }
}

$report = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    task_output_root = $resolvedTaskOutputRoot
    report_path = $resolvedReportPath
    group_count = $groupReports.Count
    superseded_task_count = $supersededCount
    groups = $groupReports
}

($report | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $resolvedReportPath -Encoding UTF8

Write-Host "Task root: $resolvedTaskOutputRoot"
Write-Host "Report: $resolvedReportPath"
Write-Host "Groups: $($groupReports.Count)"
Write-Host "Superseded tasks: $supersededCount"

foreach ($groupReport in $groupReports) {
    if (@($groupReport.superseded_tasks).Count -eq 0) {
        continue
    }

    Write-Host "Superseded group: $($groupReport.source_kind)"
    Write-Host "Source: $($groupReport.source_path)"
    Write-Host "Latest task: $($groupReport.latest_task.task_dir)"
    foreach ($supersededTask in $groupReport.superseded_tasks) {
        Write-Host "Older task: $($supersededTask.task_dir)"
    }
}
