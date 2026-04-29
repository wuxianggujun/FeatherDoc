param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$HandoffOutputPath = "",
    [string]$BodyOutputPath = "",
    [string]$ShortOutputPath = "",
    [string]$GuideOutputPath = "",
    [string]$ChecklistOutputPath = "",
    [string]$StartHereOutputPath = "",
    [string]$ReleaseVersion = "",
    [switch]$SkipMaterialSafetyAudit
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

function Get-ProjectVersionFromHandoff {
    param([string]$HandoffPath)

    if ([string]::IsNullOrWhiteSpace($HandoffPath) -or -not (Test-Path -LiteralPath $HandoffPath)) {
        return ""
    }

    $content = Get-Content -Raw -LiteralPath $HandoffPath
    $match = [regex]::Match($content, 'Project version:\s*([0-9]+\.[0-9]+\.[0-9]+)')
    if (-not $match.Success) {
        return ""
    }

    return $match.Groups[1].Value
}

function Resolve-ReleaseVersion {
    param(
        [string]$RepoRoot,
        $Summary,
        [string]$ExplicitVersion = "",
        [string]$ExistingHandoffPath = ""
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitVersion)) {
        return $ExplicitVersion
    }

    $summaryVersion = Get-OptionalPropertyValue -Object $Summary -Name "release_version"
    if (-not [string]::IsNullOrWhiteSpace($summaryVersion)) {
        return $summaryVersion
    }

    $handoffVersion = Get-ProjectVersionFromHandoff -HandoffPath $ExistingHandoffPath
    if (-not [string]::IsNullOrWhiteSpace($handoffVersion)) {
        return $handoffVersion
    }

    return Get-ProjectVersion -RepoRoot $RepoRoot
}

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
if (-not (Test-Path -LiteralPath $resolvedSummaryPath)) {
    throw "Summary JSON does not exist: $resolvedSummaryPath"
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$reportDir = Split-Path -Parent $resolvedSummaryPath

$resolvedHandoffPath = if ([string]::IsNullOrWhiteSpace($HandoffOutputPath)) {
    $candidate = Get-OptionalPropertyValue -Object $summary -Name "release_handoff"
    if ([string]::IsNullOrWhiteSpace($candidate)) {
        Join-Path $reportDir "release_handoff.md"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $candidate
    }
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $HandoffOutputPath
}

$resolvedReleaseVersion = Resolve-ReleaseVersion `
    -RepoRoot $repoRoot `
    -Summary $summary `
    -ExplicitVersion $ReleaseVersion `
    -ExistingHandoffPath $resolvedHandoffPath

$resolvedBodyPath = if ([string]::IsNullOrWhiteSpace($BodyOutputPath)) {
    $candidate = Get-OptionalPropertyValue -Object $summary -Name "release_body_zh_cn"
    if ([string]::IsNullOrWhiteSpace($candidate)) {
        Join-Path $reportDir "release_body.zh-CN.md"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $candidate
    }
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $BodyOutputPath
}

$resolvedShortPath = if ([string]::IsNullOrWhiteSpace($ShortOutputPath)) {
    $candidate = Get-OptionalPropertyValue -Object $summary -Name "release_summary_zh_cn"
    if ([string]::IsNullOrWhiteSpace($candidate)) {
        Join-Path $reportDir "release_summary.zh-CN.md"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $candidate
    }
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ShortOutputPath
}

$resolvedGuidePath = if ([string]::IsNullOrWhiteSpace($GuideOutputPath)) {
    $candidate = Get-OptionalPropertyValue -Object $summary -Name "artifact_guide"
    if ([string]::IsNullOrWhiteSpace($candidate)) {
        Join-Path $reportDir "ARTIFACT_GUIDE.md"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $candidate
    }
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $GuideOutputPath
}

$resolvedChecklistPath = if ([string]::IsNullOrWhiteSpace($ChecklistOutputPath)) {
    $candidate = Get-OptionalPropertyValue -Object $summary -Name "reviewer_checklist"
    if ([string]::IsNullOrWhiteSpace($candidate)) {
        Join-Path $reportDir "REVIEWER_CHECKLIST.md"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $candidate
    }
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ChecklistOutputPath
}

$resolvedStartHerePath = if ([string]::IsNullOrWhiteSpace($StartHereOutputPath)) {
    $candidate = Get-OptionalPropertyValue -Object $summary -Name "start_here"
    if ([string]::IsNullOrWhiteSpace($candidate)) {
        Join-Path (Split-Path -Parent $reportDir) "START_HERE.md"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $candidate
    }
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $StartHereOutputPath
}

$handoffScript = Join-Path $repoRoot "scripts\write_release_artifact_handoff.ps1"
$bodyScript = Join-Path $repoRoot "scripts\write_release_body_zh.ps1"
$guideScript = Join-Path $repoRoot "scripts\write_release_artifact_guide.ps1"
$checklistScript = Join-Path $repoRoot "scripts\write_release_reviewer_checklist.ps1"
$startHereScript = Join-Path $repoRoot "scripts\write_release_metadata_start_here.ps1"
$releaseMaterialAuditScript = Join-Path $repoRoot "scripts\assert_release_material_safety.ps1"

$handoffParams = @{
    SummaryJson = $resolvedSummaryPath
    OutputPath = $resolvedHandoffPath
}
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseVersion)) {
    $handoffParams.ReleaseVersion = $resolvedReleaseVersion
}
& $handoffScript @handoffParams

$bodyParams = @{
    SummaryJson = $resolvedSummaryPath
    OutputPath = $resolvedBodyPath
    ShortOutputPath = $resolvedShortPath
}
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseVersion)) {
    $bodyParams.ReleaseVersion = $resolvedReleaseVersion
}
& $bodyScript @bodyParams

& $guideScript `
    -SummaryJson $resolvedSummaryPath `
    -OutputPath $resolvedGuidePath

& $checklistScript `
    -SummaryJson $resolvedSummaryPath `
    -OutputPath $resolvedChecklistPath

& $startHereScript `
    -SummaryJson $resolvedSummaryPath `
    -OutputPath $resolvedStartHerePath

if (-not $SkipMaterialSafetyAudit.IsPresent) {
    & $releaseMaterialAuditScript -Path @(
        $resolvedStartHerePath
        $resolvedGuidePath
        $resolvedChecklistPath
        $resolvedHandoffPath
        $resolvedBodyPath
        $resolvedShortPath
    )
}

Write-Host "Release handoff: $resolvedHandoffPath"
Write-Host "Release body output: $resolvedBodyPath"
Write-Host "Release summary output: $resolvedShortPath"
Write-Host "Artifact guide: $resolvedGuidePath"
Write-Host "Reviewer checklist: $resolvedChecklistPath"
Write-Host "Start here: $resolvedStartHerePath"
