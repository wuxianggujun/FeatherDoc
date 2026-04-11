param(
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks",
    [string]$SourceKind = "",
    [switch]$OpenTaskDir,
    [switch]$OpenPrompt,
    [switch]$PrintPrompt,
    [switch]$PromptOnly,
    [ValidateSet("", "pointer_path", "task_id", "generated_at", "mode", "source_kind", "source_path", "document_path", "task_dir", "prompt_path", "manifest_path", "review_result_path", "final_review_path")]
    [string]$PrintField = "",
    [switch]$AsJson
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Get-SafePathSegment {
    param([string]$Name)

    $safe = [System.Text.RegularExpressions.Regex]::Replace($Name, "[^A-Za-z0-9._-]+", "-")
    $safe = $safe.Trim("-")
    if ([string]::IsNullOrWhiteSpace($safe)) {
        return "document"
    }

    return $safe
}

function Resolve-TaskOutputRoot {
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

function Get-LatestPointerPath {
    param(
        [string]$TaskRoot,
        [string]$SourceKind
    )

    if ([string]::IsNullOrWhiteSpace($SourceKind)) {
        return Join-Path $TaskRoot "latest_task.json"
    }

    $safeSourceKind = Get-SafePathSegment -Name $SourceKind
    return Join-Path $TaskRoot "latest_$($safeSourceKind)_task.json"
}

function Read-LatestTaskInfo {
    param([string]$PointerPath)

    if (-not (Test-Path $PointerPath)) {
        throw "Latest task pointer was not found: $PointerPath"
    }

    $payload = Get-Content -Path $PointerPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $requiredPaths = @(
        @{ Label = "task directory"; Path = $payload.task_dir },
        @{ Label = "task manifest"; Path = $payload.manifest_path },
        @{ Label = "task prompt"; Path = $payload.prompt_path }
    )

    foreach ($requiredPath in $requiredPaths) {
        if ([string]::IsNullOrWhiteSpace($requiredPath.Path) -or
            -not (Test-Path $requiredPath.Path)) {
            throw "Latest task pointer references a missing $($requiredPath.Label): $($requiredPath.Path)"
        }
    }

    return $payload
}

function Get-TaskFieldValue {
    param(
        $TaskInfo,
        [string]$FieldName
    )

    switch ($FieldName) {
        "pointer_path" { return $TaskInfo.pointer_path }
        "task_id" { return $TaskInfo.task_id }
        "generated_at" { return $TaskInfo.generated_at }
        "mode" { return $TaskInfo.mode }
        "source_kind" { return $TaskInfo.source.kind }
        "source_path" { return $TaskInfo.source.path }
        "document_path" {
            if ($TaskInfo.document) {
                return $TaskInfo.document.path
            }
            return ""
        }
        "task_dir" { return $TaskInfo.task_dir }
        "prompt_path" { return $TaskInfo.prompt_path }
        "manifest_path" { return $TaskInfo.manifest_path }
        "review_result_path" { return $TaskInfo.review_result_path }
        "final_review_path" { return $TaskInfo.final_review_path }
        default { throw "Unsupported field name: $FieldName" }
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedTaskRoot = Resolve-TaskOutputRoot -RepoRoot $repoRoot -InputPath $TaskOutputRoot
$pointerPath = Get-LatestPointerPath -TaskRoot $resolvedTaskRoot -SourceKind $SourceKind
$taskInfo = Read-LatestTaskInfo -PointerPath $pointerPath
$taskInfo | Add-Member -NotePropertyName pointer_path -NotePropertyValue $pointerPath -Force

if ($OpenTaskDir) {
    Start-Process explorer.exe $taskInfo.task_dir | Out-Null
}

if ($OpenPrompt) {
    Start-Process notepad.exe $taskInfo.prompt_path | Out-Null
}

if ($AsJson) {
    ($taskInfo | ConvertTo-Json -Depth 6) | Write-Output
} elseif (-not [string]::IsNullOrWhiteSpace($PrintField)) {
    Write-Output (Get-TaskFieldValue -TaskInfo $taskInfo -FieldName $PrintField)
} elseif ($PromptOnly) {
    Get-Content -Path $taskInfo.prompt_path -Encoding UTF8
} else {
    Write-Host "Pointer file: $pointerPath"
    Write-Host "Task id: $($taskInfo.task_id)"
    Write-Host "Generated at: $($taskInfo.generated_at)"
    Write-Host "Mode: $($taskInfo.mode)"
    Write-Host "Source kind: $($taskInfo.source.kind)"
    Write-Host "Source: $($taskInfo.source.path)"
    if ($taskInfo.document -and $taskInfo.document.path) {
        Write-Host "Document: $($taskInfo.document.path)"
    }
    Write-Host "Task directory: $($taskInfo.task_dir)"
    Write-Host "Prompt: $($taskInfo.prompt_path)"
    Write-Host "Manifest: $($taskInfo.manifest_path)"
    Write-Host "Review result: $($taskInfo.review_result_path)"
    Write-Host "Final review: $($taskInfo.final_review_path)"
}

if ($PrintPrompt) {
    if (-not $AsJson) {
        Write-Host ""
        Write-Host "--- task_prompt.md ---"
    }
    Get-Content -Path $taskInfo.prompt_path -Encoding UTF8
}
