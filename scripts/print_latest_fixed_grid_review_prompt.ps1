param(
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks"
)

$ErrorActionPreference = "Stop"

$delegateScript = Join-Path $PSScriptRoot "open_latest_fixed_grid_review_task.ps1"
if (-not (Test-Path $delegateScript)) {
    throw "Delegate script was not found: $delegateScript"
}

& $delegateScript -TaskOutputRoot $TaskOutputRoot -PromptOnly
