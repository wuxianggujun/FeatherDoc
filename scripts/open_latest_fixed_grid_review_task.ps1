param(
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks",
    [switch]$OpenTaskDir,
    [switch]$OpenPrompt,
    [switch]$PrintPrompt,
    [switch]$PromptOnly,
    [ValidateSet("", "pointer_path", "task_id", "generated_at", "mode", "source_kind", "source_path", "document_path", "task_dir", "prompt_path", "manifest_path", "review_result_path", "final_review_path")]
    [string]$PrintField = "",
    [switch]$AsJson
)

$ErrorActionPreference = "Stop"

$delegateScript = Join-Path $PSScriptRoot "open_latest_word_review_task.ps1"
if (-not (Test-Path $delegateScript)) {
    throw "Delegate script was not found: $delegateScript"
}

$delegateArgs = @{
    TaskOutputRoot = $TaskOutputRoot
    SourceKind = "fixed-grid-regression-bundle"
}

if ($OpenTaskDir) {
    $delegateArgs.OpenTaskDir = $true
}
if ($OpenPrompt) {
    $delegateArgs.OpenPrompt = $true
}
if ($PrintPrompt) {
    $delegateArgs.PrintPrompt = $true
}
if ($PromptOnly) {
    $delegateArgs.PromptOnly = $true
}
if (-not [string]::IsNullOrWhiteSpace($PrintField)) {
    $delegateArgs.PrintField = $PrintField
}
if ($AsJson) {
    $delegateArgs.AsJson = $true
}

& $delegateScript @delegateArgs
