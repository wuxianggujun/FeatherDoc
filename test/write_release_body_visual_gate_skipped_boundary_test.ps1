param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = (Join-Path $PSScriptRoot "..")
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $resolvedRepoRoot "output\test\write-release-body-visual-gate-skipped-boundary"
}

$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$encoding = New-Object System.Text.UTF8Encoding($false)

function Get-Utf8FileText {
    param([string]$Path)

    return [System.IO.File]::ReadAllText($Path, $encoding)
}

function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-Utf8FileText -Path $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw ("{0} does not contain expected text '{1}': {2}" -f $Label, $ExpectedText, $Path)
    }
}

function Assert-NotContains {
    param(
        [string]$Path,
        [string]$UnexpectedText,
        [string]$Label
    )

    $content = Get-Utf8FileText -Path $Path
    if ($content -match [regex]::Escape($UnexpectedText)) {
        throw ("{0} unexpectedly contains '{1}': {2}" -f $Label, $UnexpectedText, $Path)
    }
}

function Invoke-SkippedVisualGateFixture {
    param([string]$VisualGateStatus)

    $caseRoot = Join-Path $resolvedWorkingDir $VisualGateStatus
    $reportDir = Join-Path $caseRoot "report"
    $gateReportDir = Join-Path $caseRoot "word-visual-release-gate\report"
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
    New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null

    $summaryPath = Join-Path $reportDir "summary.json"
    $bodyPath = Join-Path $reportDir "release_body.zh-CN.md"
    $shortPath = Join-Path $reportDir "release_summary.zh-CN.md"
    $gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"

    $gateSummary = [ordered]@{
        generated_at = "2026-06-03T12:00:00"
        workspace = $resolvedRepoRoot
        visual_verdict = "pass"
        readme_gallery = [ordered]@{
            status = "not_requested"
        }
    }
    [System.IO.File]::WriteAllText(
        $gateSummaryPath,
        ($gateSummary | ConvertTo-Json -Depth 8),
        $encoding
    )

    $summary = [ordered]@{
        generated_at = "2026-06-03T12:00:00"
        workspace = $resolvedRepoRoot
        release_version = "1.6.4"
        execution_status = "pass"
        steps = [ordered]@{
            configure = [ordered]@{ status = "completed" }
            build = [ordered]@{ status = "completed" }
            tests = [ordered]@{ status = "completed" }
            install_smoke = [ordered]@{ status = "completed" }
            visual_gate = [ordered]@{
                status = $VisualGateStatus
                summary_json = $gateSummaryPath
            }
        }
    }
    [System.IO.File]::WriteAllText(
        $summaryPath,
        ($summary | ConvertTo-Json -Depth 8),
        $encoding
    )

    $scriptPath = Join-Path $resolvedRepoRoot "scripts\write_release_body_zh.ps1"
    & $scriptPath -SummaryJson $summaryPath -OutputPath $bodyPath -ShortOutputPath $shortPath

    Assert-Contains `
        -Path $shortPath `
        -ExpectedText "Windows + Microsoft Word" `
        -Label "release_summary.zh-CN.md ($VisualGateStatus)"
    Assert-Contains `
        -Path $bodyPath `
        -ExpectedText "Windows + Microsoft Word" `
        -Label "release_body.zh-CN.md ($VisualGateStatus)"
    Assert-NotContains `
        -Path $shortPath `
        -UnexpectedText "full release-preflight" `
        -Label "release_summary.zh-CN.md ($VisualGateStatus)"
}

foreach ($status in @("skipped", "visual_gate_skipped")) {
    Invoke-SkippedVisualGateFixture -VisualGateStatus $status
}

Write-Host "Release body skipped visual gate boundary regression passed."
