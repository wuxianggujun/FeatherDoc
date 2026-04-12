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
$fakeGhStatePath = Join-Path $resolvedWorkingDir "fake-gh-state.json"
$fakeGhLogPath = Join-Path $resolvedWorkingDir "fake-gh.log"

New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $fakeBinDir -Force | Out-Null

Set-Content -LiteralPath $releaseBodyPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 发布说明

## 核心变化
- 公开 release 正文已经切换到已审计的自动同步链路。

## 验证结论
- execution_status：pass
- visual_verdict：pass
"@

$summary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    release_version = "1.6.4"
    execution_status = "pass"
    visual_verdict = "pass"
    release_body_zh_cn = $releaseBodyPath
    steps = [ordered]@{
        visual_gate = [ordered]@{
            status = "completed"
            visual_verdict = "pass"
        }
    }
}
($summary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$fakeGhState = [ordered]@{
    tagName = "v1.6.4"
    name = "FeatherDoc v1.6.4"
    url = "https://example.invalid/releases/v1.6.4"
    isDraft = $true
    body = "old body"
}
($fakeGhState | ConvertTo-Json -Depth 5) | Set-Content -LiteralPath $fakeGhStatePath -Encoding UTF8
Set-Content -LiteralPath $fakeGhLogPath -Encoding UTF8 -Value ""

$fakeGhPath = Join-Path $fakeBinDir "gh.ps1"
Set-Content -LiteralPath $fakeGhPath -Encoding UTF8 -Value @'
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Args
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$statePath = $env:FEATHERDOC_FAKE_GH_STATE
$logPath = $env:FEATHERDOC_FAKE_GH_LOG
Add-Content -LiteralPath $logPath -Value ($Args -join " ")

if ($Args.Count -lt 2 -or $Args[0] -ne "release") {
    throw "Unexpected gh invocation: $($Args -join ' ')"
}

$state = Get-Content -Raw -LiteralPath $statePath | ConvertFrom-Json
switch ($Args[1]) {
    "view" {
        $payload = [ordered]@{
            tagName = $state.tagName
            name = $state.name
            url = $state.url
            isDraft = [bool]$state.isDraft
            body = [string]$state.body
        }
        $payload | ConvertTo-Json -Compress
    }
    "edit" {
        if ($Args[2] -ne $state.tagName) {
            throw "Unexpected tag: $($Args[2])"
        }

        for ($index = 3; $index -lt $Args.Count; $index++) {
            switch ($Args[$index]) {
                "--notes-file" {
                    $index++
                    $state.body = Get-Content -Raw -LiteralPath $Args[$index]
                }
                "--title" {
                    $index++
                    $state.name = $Args[$index]
                }
                "--draft=false" {
                    $state.isDraft = $false
                }
            }
        }

        ($state | ConvertTo-Json -Depth 5) | Set-Content -LiteralPath $statePath -Encoding UTF8
    }
    default {
        throw "Unexpected gh release subcommand: $($Args[1])"
    }
}
'@

$previousPath = $env:PATH
$env:FEATHERDOC_FAKE_GH_STATE = $fakeGhStatePath
$env:FEATHERDOC_FAKE_GH_LOG = $fakeGhLogPath
$env:PATH = "$fakeBinDir;$previousPath"

try {
    $syncScript = Join-Path $resolvedRepoRoot "scripts\sync_github_release_notes.ps1"
    & $syncScript -SummaryJson $summaryPath -Publish
} finally {
    $env:PATH = $previousPath
    Remove-Item Env:FEATHERDOC_FAKE_GH_STATE -ErrorAction SilentlyContinue
    Remove-Item Env:FEATHERDOC_FAKE_GH_LOG -ErrorAction SilentlyContinue
}

$finalState = Get-Content -Raw -LiteralPath $fakeGhStatePath | ConvertFrom-Json
$expectedBody = Get-Content -Raw -LiteralPath $releaseBodyPath
$logContent = Get-Content -Raw -LiteralPath $fakeGhLogPath

if ($finalState.body -ne $expectedBody) {
    throw "Fake GitHub release body did not match the audited release body after sync."
}

if (-not [bool](-not $finalState.isDraft)) {
    throw "Fake GitHub release was not published via --draft=false."
}

if ($logContent -notmatch [regex]::Escape("release edit v1.6.4 --notes-file $releaseBodyPath --draft=false")) {
    throw "Expected gh release edit invocation was not recorded."
}

Write-Host "GitHub release note sync regression passed."
