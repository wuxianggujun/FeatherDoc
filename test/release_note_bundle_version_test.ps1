param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText': $Path"
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$reportDir = Join-Path $resolvedWorkingDir "report"
$installDir = Join-Path $resolvedWorkingDir "install"

New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $installDir -Force | Out-Null

$summaryPath = Join-Path $reportDir "summary.json"
$summary = [ordered]@{
    generated_at = "2026-04-11T12:00:00"
    workspace = $resolvedRepoRoot
    execution_status = "pass"
    release_version = "1.6.0"
    install_dir = $installDir
    steps = [ordered]@{
        configure = [ordered]@{ status = "completed" }
        build = [ordered]@{ status = "completed" }
        tests = [ordered]@{ status = "completed" }
        install_smoke = [ordered]@{
            status = "completed"
            install_prefix = ""
            consumer_document = ""
        }
        visual_gate = [ordered]@{
            status = "completed"
            summary_json = ""
            final_review = ""
        }
    }
}
($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$bundleScript = Join-Path $resolvedRepoRoot "scripts\write_release_note_bundle.ps1"
& $bundleScript -SummaryJson $summaryPath

$handoffPath = Join-Path $reportDir "release_handoff.md"
$bodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$shortPath = Join-Path $reportDir "release_summary.zh-CN.md"

Assert-Contains -Path $handoffPath -ExpectedText 'Project version: 1.6.0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_handoff.md'
Assert-Contains -Path $bodyPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText '`1.6.0`' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_summary.zh-CN.md'

$bodyContent = Get-Content -Raw -LiteralPath $bodyPath
if ($bodyContent -match 'v1\.6\.1') {
    throw "release_body.zh-CN.md unexpectedly referenced the current development version: $bodyPath"
}

Write-Host "Release note bundle version pinning passed."
