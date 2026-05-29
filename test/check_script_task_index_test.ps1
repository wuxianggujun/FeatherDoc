param(
    [string]$RepoRoot = "",
    [string]$WorkingDir = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-DefaultRepoRoot {
    if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
        return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    }

    return (Resolve-Path $RepoRoot).Path
}

function Resolve-DefaultWorkingDir {
    if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
        return [System.IO.Path]::GetFullPath((Join-Path $resolvedRepoRoot "output\check-script-task-index-test"))
    }

    return [System.IO.Path]::GetFullPath($WorkingDir)
}

function Write-Utf8NoBomFile {
    param(
        [string]$Path,
        [string]$Text
    )

    $parentDir = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parentDir)) {
        New-Item -ItemType Directory -Path $parentDir -Force | Out-Null
    }

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Assert-FileHasNoBom {
    param([string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        throw "File must be UTF-8 without BOM: $Path"
    }
}

function Assert-ScriptParses {
    param([string]$Path)

    $parseTokens = $null
    $parseErrors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$parseTokens, [ref]$parseErrors) | Out-Null
    if ($parseErrors.Count -gt 0) {
        throw "PowerShell script has parse errors: $Path"
    }
}

function Assert-ArrayContains {
    param(
        [object[]]$Values,
        [string]$ExpectedValue,
        [string]$Message
    )

    foreach ($value in $Values) {
        if ($value -eq $ExpectedValue) {
            return
        }
    }

    throw $Message
}

function Invoke-IndexCheck {
    param(
        [string]$Root,
        [string]$SummaryJson,
        [switch]$Quiet
    )

    $parameters = @{
        RepoRoot = $Root
        SummaryJson = $SummaryJson
    }
    if ($Quiet) {
        $parameters.Quiet = $true
    }

    return @(& $checkerScript @parameters *>&1)
}

$resolvedRepoRoot = Resolve-DefaultRepoRoot
$resolvedWorkingDir = Resolve-DefaultWorkingDir
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$checkerScript = Join-Path $resolvedRepoRoot "scripts\check_script_task_index.ps1"
Assert-ScriptParses -Path $checkerScript

$passingSummaryJson = Join-Path $resolvedWorkingDir "passing-summary.json"
$passingOutput = Invoke-IndexCheck -Root $resolvedRepoRoot -SummaryJson $passingSummaryJson
$joinedPassingOutput = ($passingOutput | ForEach-Object { $_.ToString() }) -join "`n"
if ($joinedPassingOutput -notmatch [regex]::Escape("Script task index check passed.")) {
    throw "check_script_task_index.ps1 did not print the success marker."
}
if (-not (Test-Path -LiteralPath $passingSummaryJson -PathType Leaf)) {
    throw "check_script_task_index.ps1 did not write a passing summary."
}
Assert-FileHasNoBom -Path $passingSummaryJson
$passingSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $passingSummaryJson | ConvertFrom-Json
if ($passingSummary.status -ne "passed") {
    throw "Expected passing summary status, got: $($passingSummary.status)"
}
if ($passingSummary.schema -ne "featherdoc.script_task_index_check.v1") {
    throw "Unexpected script task index summary schema: $($passingSummary.schema)"
}
if ($passingSummary.checker_name -ne "check_script_task_index.ps1") {
    throw "Unexpected checker name: $($passingSummary.checker_name)"
}
if ($passingSummary.script_reference_count -lt 20) {
    throw "Expected at least 20 indexed scripts, got: $($passingSummary.script_reference_count)"
}
if ($passingSummary.missing_script_count -ne 0) {
    throw "Expected no missing scripts, got: $($passingSummary.missing_script_count)"
}
if ($passingSummary.missing_marker_count -ne 0) {
    throw "Expected no missing markers, got: $($passingSummary.missing_marker_count)"
}
Assert-ArrayContains `
    -Values @($passingSummary.checked_scripts | ForEach-Object { $_.relative_path }) `
    -ExpectedValue "scripts\check_script_task_index.ps1" `
    -Message "Summary should list the script index checker."
Assert-ArrayContains `
    -Values @($passingSummary.checked_scripts | ForEach-Object { $_.relative_path }) `
    -ExpectedValue "scripts\run_release_candidate_checks.ps1" `
    -Message "Summary should list the release candidate checks script."
Assert-ArrayContains `
    -Values @($passingSummary.checked_scripts | ForEach-Object { $_.relative_path }) `
    -ExpectedValue "scripts\check_pdf_release_readiness.ps1" `
    -Message "Summary should list the PDF release readiness script."

$quietSummaryJson = Join-Path $resolvedWorkingDir "quiet-summary.json"
$quietOutput = Invoke-IndexCheck -Root $resolvedRepoRoot -SummaryJson $quietSummaryJson -Quiet
$joinedQuietOutput = ($quietOutput | ForEach-Object { $_.ToString() }) -join "`n"
if ($joinedQuietOutput -match [regex]::Escape("Script task index check passed.")) {
    throw "check_script_task_index.ps1 printed the success marker in quiet mode."
}

$failingRoot = Join-Path $resolvedWorkingDir "failing-repo"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "docs\script_task_index_zh.rst") `
    -Text (@(
        "Script task index",
        "=================",
        "",
        "- ``scripts/check_script_task_index.ps1``",
        "- ``scripts/existing_tool.ps1``",
        "- ``scripts/missing_tool.ps1``"
    ) -join "`n")
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "docs\index.rst") `
    -Text "script_task_index_zh"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "docs\documentation_maintenance_zh.rst") `
    -Text "docs/script_task_index_zh.rst`ncheck_script_task_index.ps1"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "docs\project_score_assessment_zh.rst") `
    -Text "docs/script_task_index_zh.rst`ncheck_script_task_index.ps1"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "test\CMakeLists.txt") `
    -Text (@(
        "check_script_task_index",
        "check_script_task_index_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;scripts"'
    ) -join "`n")
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "scripts\check_script_task_index.ps1") `
    -Text "param()`n"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "scripts\existing_tool.ps1") `
    -Text "param()`n"

$failingSummaryJson = Join-Path $failingRoot "summary.json"
$failed = $false
$failureOutput = @()
try {
    $failureOutput = Invoke-IndexCheck -Root $failingRoot -SummaryJson $failingSummaryJson
} catch {
    $failed = $true
    $failureOutput += $_.Exception.Message
}
if (-not $failed) {
    throw "check_script_task_index.ps1 unexpectedly passed with a missing indexed script."
}
$joinedFailureOutput = ($failureOutput | ForEach-Object { $_.ToString() }) -join "`n"
if ($joinedFailureOutput -notmatch [regex]::Escape("MissingScripts=1")) {
    throw "Expected missing script count in failure output, got: $joinedFailureOutput"
}
if (-not (Test-Path -LiteralPath $failingSummaryJson -PathType Leaf)) {
    throw "check_script_task_index.ps1 did not write a failing summary."
}
Assert-FileHasNoBom -Path $failingSummaryJson
$failingSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $failingSummaryJson | ConvertFrom-Json
if ($failingSummary.status -ne "failed") {
    throw "Expected failing summary status, got: $($failingSummary.status)"
}
if ($failingSummary.missing_script_count -ne 1) {
    throw "Expected one missing script, got: $($failingSummary.missing_script_count)"
}
Assert-ArrayContains `
    -Values @($failingSummary.missing_scripts) `
    -ExpectedValue "scripts\missing_tool.ps1" `
    -Message "Failing summary should list the missing script."

Write-Host "Script task index checker regression passed."
