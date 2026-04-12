<#
.SYNOPSIS
Safely syncs audited FeatherDoc release notes into a GitHub Release.

.DESCRIPTION
Reads a release-candidate summary, resolves the generated
`release_body.zh-CN.md`, audits it for public-release safety, and then updates
the matching GitHub Release body via `gh release edit --notes-file`.

By default the script only syncs notes. When `-Publish` is set, it also passes
`--draft=false` so an existing draft release is published with the same audited
body. Publishing requires a final local pass verdict and refuses CI-only
`visual_gate=skipped` summaries.

.PARAMETER SummaryJson
Path to the release-candidate summary JSON produced by
`run_release_candidate_checks.ps1`.

.PARAMETER ReleaseTag
Optional explicit GitHub release tag. Defaults to `v<release_version>`.

.PARAMETER ReleaseVersion
Optional explicit version override. Used only when deriving the default tag.

.PARAMETER BodyPath
Optional explicit release-body override. Defaults to
`summary.release_body_zh_cn` or `report/release_body.zh-CN.md`.

.PARAMETER Title
Optional explicit release title override.

.PARAMETER AllowIncomplete
Allows syncing notes even when `execution_status` / `visual_verdict` are not
final pass. This never relaxes the stricter `-Publish` checks.

.PARAMETER Publish
Also passes `--draft=false` so the target GitHub Release is published.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_github_release_notes.ps1 `
    -SummaryJson .\output\release-candidate-checks\report\summary.json

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_github_release_notes.ps1 `
    -SummaryJson .\output\release-candidate-checks\report\summary.json `
    -Publish
#>
param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$ReleaseTag = "",
    [string]$ReleaseVersion = "",
    [string]$BodyPath = "",
    [string]$Title = "",
    [switch]$AllowIncomplete,
    [switch]$Publish
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[sync-github-release-notes] $Message"
}

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
        throw "Missing ${Label}: $Path"
    }
}

function Get-OptionalPropertyValue {
    param(
        $Object,
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
        $Object,
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

function Get-ProjectVersion {
    param([string]$RepoRoot)

    $cmakePath = Join-Path $RepoRoot "CMakeLists.txt"
    if (-not (Test-Path -LiteralPath $cmakePath)) {
        return ""
    }

    $content = Get-Content -Raw -LiteralPath $cmakePath
    $match = [regex]::Match($content, 'VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)')
    if (-not $match.Success) {
        return ""
    }

    return $match.Groups[1].Value
}

function Get-ResolvedReleaseVersion {
    param(
        [string]$ExplicitVersion,
        $Summary,
        [string]$RepoRoot
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitVersion)) {
        return $ExplicitVersion
    }

    $summaryVersion = Get-OptionalPropertyValue -Object $Summary -Name "release_version"
    if (-not [string]::IsNullOrWhiteSpace($summaryVersion)) {
        return $summaryVersion
    }

    return Get-ProjectVersion -RepoRoot $RepoRoot
}

function Get-VisualVerdict {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $summaryVerdict = Get-OptionalPropertyValue -Object $Summary -Name "visual_verdict"
    if (-not [string]::IsNullOrWhiteSpace($summaryVerdict)) {
        return $summaryVerdict
    }

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    $stepVerdict = Get-OptionalPropertyValue -Object $visualGate -Name "visual_verdict"
    if (-not [string]::IsNullOrWhiteSpace($stepVerdict)) {
        return $stepVerdict
    }

    $gateSummaryPath = Get-OptionalPropertyValue -Object $visualGate -Name "summary_json"
    if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath)) {
        $resolvedGateSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $gateSummaryPath
        if (Test-Path -LiteralPath $resolvedGateSummaryPath) {
            $gateSummary = Get-Content -Raw -LiteralPath $resolvedGateSummaryPath | ConvertFrom-Json
            return Get-OptionalPropertyValue -Object $gateSummary -Name "visual_verdict"
        }
    }

    return ""
}

function Get-VisualGateStatus {
    param($Summary)

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    return Get-OptionalPropertyValue -Object $visualGate -Name "status"
}

function Normalize-ReleaseText {
    param([string]$Value)

    if ($null -eq $Value) {
        return ""
    }

    $normalized = $Value.Replace("`r`n", "`n").Replace("`r", "`n")
    return $normalized.TrimEnd("`n")
}

function Assert-LastCommandSucceeded {
    param([string]$Message)

    if (-not $?) {
        $exitCodeValue = $null
        $lastExitCodeVariable = Get-Variable -Name LASTEXITCODE -ErrorAction SilentlyContinue
        if ($null -ne $lastExitCodeVariable) {
            $exitCodeValue = $lastExitCodeVariable.Value
        }

        if ($null -eq $exitCodeValue) {
            throw $Message
        }

        throw ("{0} Exit code: {1}." -f $Message, $exitCodeValue)
    }
}

function Get-ReleaseView {
    param([string]$Tag)

    $releaseViewJson = & gh release view $Tag --json tagName,name,url,isDraft,body
    Assert-LastCommandSucceeded -Message ("gh release view failed for tag '{0}'." -f $Tag)

    return $releaseViewJson | ConvertFrom-Json
}

$repoRoot = Resolve-RepoRoot
$releaseMaterialAuditScript = Join-Path $repoRoot "scripts\assert_release_material_safety.ps1"
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
Assert-PathExists -Path $resolvedSummaryPath -Label "release summary JSON"

$summary = Get-Content -Raw -LiteralPath $resolvedSummaryPath | ConvertFrom-Json
$resolvedReleaseVersion = Get-ResolvedReleaseVersion -ExplicitVersion $ReleaseVersion -Summary $summary -RepoRoot $repoRoot
if ([string]::IsNullOrWhiteSpace($resolvedReleaseVersion)) {
    throw "Could not resolve the release version from the summary or CMakeLists.txt."
}

$executionStatus = Get-OptionalPropertyValue -Object $summary -Name "execution_status"
$visualVerdict = Get-VisualVerdict -RepoRoot $repoRoot -Summary $summary
$visualGateStatus = Get-VisualGateStatus -Summary $summary

if (-not $AllowIncomplete) {
    if ($executionStatus -ne "pass") {
        throw "Refusing to sync GitHub release notes because execution_status is '$executionStatus'. Use -AllowIncomplete to override."
    }

    if (-not [string]::IsNullOrWhiteSpace($visualVerdict) -and $visualVerdict -ne "pass") {
        throw "Refusing to sync GitHub release notes because visual_verdict is '$visualVerdict'. Use -AllowIncomplete to override."
    }
}

if ($Publish) {
    if ($executionStatus -ne "pass") {
        throw "Refusing to publish GitHub release '$resolvedReleaseVersion' because execution_status is '$executionStatus'."
    }

    if ([string]::IsNullOrWhiteSpace($visualVerdict) -or $visualVerdict -ne "pass") {
        throw "Refusing to publish GitHub release '$resolvedReleaseVersion' because visual_verdict is '$visualVerdict'."
    }

    if ($visualGateStatus -eq "skipped") {
        throw "Refusing to publish GitHub release '$resolvedReleaseVersion' from a summary with visual_gate=status skipped."
    }
}

$reportDir = Split-Path -Parent $resolvedSummaryPath
$resolvedBodyPath = if (-not [string]::IsNullOrWhiteSpace($BodyPath)) {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $BodyPath
} else {
    $summaryBodyPath = Get-OptionalPropertyValue -Object $summary -Name "release_body_zh_cn"
    if ([string]::IsNullOrWhiteSpace($summaryBodyPath)) {
        Join-Path $reportDir "release_body.zh-CN.md"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $summaryBodyPath
    }
}
Assert-PathExists -Path $resolvedBodyPath -Label "release body markdown"

if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    throw "GitHub CLI 'gh' is required to sync GitHub release notes."
}

Write-Step "Auditing release body before GitHub sync"
& $releaseMaterialAuditScript -Path @($resolvedBodyPath)

$resolvedReleaseTag = if (-not [string]::IsNullOrWhiteSpace($ReleaseTag)) {
    $ReleaseTag
} else {
    "v{0}" -f $resolvedReleaseVersion
}

$releaseViewBefore = Get-ReleaseView -Tag $resolvedReleaseTag
Write-Step "Syncing audited notes into GitHub release $resolvedReleaseTag"

$editArgs = @(
    "release"
    "edit"
    $resolvedReleaseTag
    "--notes-file"
    $resolvedBodyPath
)
if (-not [string]::IsNullOrWhiteSpace($Title)) {
    $editArgs += @("--title", $Title)
}
if ($Publish) {
    $editArgs += "--draft=false"
}

& gh @editArgs
Assert-LastCommandSucceeded -Message "gh release edit failed."

$releaseViewAfter = Get-ReleaseView -Tag $resolvedReleaseTag
$expectedBody = Normalize-ReleaseText -Value (Get-Content -Raw -LiteralPath $resolvedBodyPath)
$actualBody = Normalize-ReleaseText -Value (Get-OptionalPropertyValue -Object $releaseViewAfter -Name "body")
if ($expectedBody -ne $actualBody) {
    throw "GitHub release body for '$resolvedReleaseTag' does not match the audited local release body after sync."
}

if ($Publish -and $releaseViewAfter.isDraft) {
    throw "GitHub release '$resolvedReleaseTag' is still marked as draft after --draft=false."
}

Write-Host "Release tag: $($releaseViewAfter.tagName)"
Write-Host "Release URL: $($releaseViewAfter.url)"
Write-Host "Release draft: $($releaseViewAfter.isDraft)"
Write-Host "Release body source: $resolvedBodyPath"
if ($releaseViewBefore.isDraft -and -not $releaseViewAfter.isDraft) {
    Write-Step "Release transitioned from draft to published."
}
