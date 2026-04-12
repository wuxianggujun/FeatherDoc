param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$summaryOutputDir = Join-Path $resolvedWorkingDir "output\release-candidate-checks"
$reportDir = Join-Path $summaryOutputDir "report"
$summaryPath = Join-Path $reportDir "summary.json"
$releaseBodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$fakeBinDir = Join-Path $resolvedWorkingDir "fake-bin"
$fakeGhLogPath = Join-Path $resolvedWorkingDir "fake-gh.log"

New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $fakeBinDir -Force | Out-Null

Set-Content -LiteralPath $releaseBodyPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 发布说明

## 核心变化
- 这是一份安全的 release 正文。
"@

$summary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    release_version = "1.6.4"
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    release_body_zh_cn = $releaseBodyPath
    steps = [ordered]@{
        visual_gate = [ordered]@{
            status = "skipped"
        }
    }
}
($summary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Set-Content -LiteralPath $fakeGhLogPath -Encoding UTF8 -Value ""

$fakeGhPath = Join-Path $fakeBinDir "gh.ps1"
Set-Content -LiteralPath $fakeGhPath -Encoding UTF8 -Value @'
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Args
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Add-Content -LiteralPath $env:FEATHERDOC_FAKE_GH_LOG -Value ($Args -join " ")
'@

$previousPath = $env:PATH
$env:FEATHERDOC_FAKE_GH_LOG = $fakeGhLogPath
$env:PATH = "$fakeBinDir;$previousPath"

$failedAsExpected = $false
$errorMessage = ""
try {
    $syncScript = Join-Path $resolvedRepoRoot "scripts\sync_github_release_notes.ps1"
    & $syncScript -SummaryJson $summaryPath -AllowIncomplete -Publish
} catch {
    $errorMessage = $_.Exception.Message
    $failedAsExpected = $_.Exception.Message -like "*Refusing to publish GitHub release*"
} finally {
    $env:PATH = $previousPath
    Remove-Item Env:FEATHERDOC_FAKE_GH_LOG -ErrorAction SilentlyContinue
}

if (-not $failedAsExpected) {
    throw "sync_github_release_notes.ps1 unexpectedly allowed publishing from an incomplete visual gate summary. Error: $errorMessage"
}

$logContent = Get-Content -Raw -LiteralPath $fakeGhLogPath
if (-not [string]::IsNullOrWhiteSpace($logContent)) {
    throw "GitHub CLI should not be invoked when publish guard checks fail."
}

Write-Host "GitHub release publish guard regression passed."
