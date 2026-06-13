function Get-ReleaseBlockerRollupInputList {
    param(
        [string[]]$InputJson,
        [string[]]$InputRoot
    )

    return @(
        @($InputJson) + @($InputRoot) |
            Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }
    )
}

function Select-UniqueReleaseBlockerRollupPathList {
    param([string[]]$Paths)

    $seen = @{}
    return @(
        foreach ($path in @($Paths)) {
            if ([string]::IsNullOrWhiteSpace($path)) { continue }
            $resolved = [System.IO.Path]::GetFullPath($path)
            $key = $resolved.ToLowerInvariant()
            if (-not $seen.ContainsKey($key)) {
                $seen[$key] = $true
                $resolved
            }
        }
    )
}

function Get-ReleaseGovernanceHandoffEffectiveInputJson {
    param(
        [string[]]$InputJson,
        [string]$ReleaseSummaryPath
    )

    return @(Select-UniqueReleaseBlockerRollupPathList `
            -Paths (@($InputJson) + @($ReleaseSummaryPath)))
}

function Get-ReleaseBlockerRollupAutoDiscoveredInputJson {
    param(
        [string]$RepoRoot,
        [string]$InputRoot
    )

    $candidateRelativePaths = @(
        "document-skeleton-governance-rollup/summary.json",
        "numbering-catalog-governance/summary.json",
        "table-layout-delivery-governance/summary.json",
        "content-control-data-binding-governance/summary.json",
        "project-template-delivery-readiness/summary.json",
        "schema-patch-confidence-calibration/summary.json",
        "docx-functional-smoke-readiness/summary.json"
    )
    $resolvedInputRoot = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $InputRoot

    return @(
        foreach ($relativePath in $candidateRelativePaths) {
            $candidate = Join-Path $resolvedInputRoot $relativePath
            if (Test-Path -LiteralPath $candidate) {
                $candidate
            }
        }
    )
}

function Invoke-ReleaseBlockerRollup {
    param(
        [string]$ScriptPath,
        [string[]]$InputJson,
        [string[]]$InputRoot,
        [string]$OutputDir,
        [string]$SummaryJson,
        [string]$ReportMarkdown,
        [bool]$FailOnBlocker,
        [bool]$FailOnWarning
    )

    $arguments = @()
    $cleanInputJson = @($InputJson | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $cleanInputRoot = @($InputRoot | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($cleanInputJson.Count -gt 0) {
        $arguments += "-InputJson"
        $arguments += ($cleanInputJson -join ",")
    }
    if ($cleanInputRoot.Count -gt 0) {
        $arguments += "-InputRoot"
        $arguments += ($cleanInputRoot -join ",")
    }
    $arguments += @(
        "-OutputDir"
        $OutputDir
        "-SummaryJson"
        $SummaryJson
        "-ReportMarkdown"
        $ReportMarkdown
    )
    if ($FailOnBlocker) {
        $arguments += "-FailOnBlocker"
    }
    if ($FailOnWarning) {
        $arguments += "-FailOnWarning"
    }

    Invoke-ChildPowerShell -ScriptPath $ScriptPath `
        -Arguments $arguments `
        -FailureMessage "Failed to build release blocker rollup."

    if (-not (Test-Path -LiteralPath $SummaryJson)) {
        throw "Release blocker rollup did not write summary JSON: $SummaryJson"
    }
    if (-not (Test-Path -LiteralPath $ReportMarkdown)) {
        throw "Release blocker rollup did not write Markdown report: $ReportMarkdown"
    }
}

function Read-ReleaseBlockerRollupSummary {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Invoke-ReleaseGovernanceHandoff {
    param(
        [string]$ScriptPath,
        [string]$InputRoot,
        [string[]]$InputJson,
        [string]$OutputDir,
        [string]$SummaryJson,
        [string]$ReportMarkdown,
        [string]$ExpectedReportProfile,
        [bool]$IncludeRollup,
        [bool]$FailOnMissing,
        [bool]$FailOnBlocker,
        [bool]$FailOnWarning
    )

    $arguments = @(
        "-InputRoot"
        $InputRoot
        "-OutputDir"
        $OutputDir
        "-SummaryJson"
        $SummaryJson
        "-ReportMarkdown"
        $ReportMarkdown
        "-ExpectedReportProfile"
        $ExpectedReportProfile
    )
    $cleanInputJson = @($InputJson | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($cleanInputJson.Count -gt 0) {
        $arguments += "-InputJson"
        $arguments += ($cleanInputJson -join ",")
    }
    if ($IncludeRollup) {
        $arguments += "-IncludeReleaseBlockerRollup"
    }
    if ($FailOnMissing) {
        $arguments += "-FailOnMissing"
    }
    if ($FailOnBlocker) {
        $arguments += "-FailOnBlocker"
    }
    if ($FailOnWarning) {
        $arguments += "-FailOnWarning"
    }

    Invoke-ChildPowerShell -ScriptPath $ScriptPath `
        -Arguments $arguments `
        -FailureMessage "Failed to build release governance handoff."

    if (-not (Test-Path -LiteralPath $SummaryJson)) {
        throw "Release governance handoff did not write summary JSON: $SummaryJson"
    }
    if (-not (Test-Path -LiteralPath $ReportMarkdown)) {
        throw "Release governance handoff did not write Markdown report: $ReportMarkdown"
    }
}

function Read-ReleaseGovernanceHandoffSummary {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}
