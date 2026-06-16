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

function Write-Utf8NoBomFile {
    param(
        [string]$Path,
        [AllowEmptyString()][string]$Text
    )

    $content = if ($null -eq $Text) { "" } else { $Text }
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $content, $encoding)
}

. (Join-Path $PSScriptRoot "release_visual_metadata_helpers.ps1")
. (Join-Path $PSScriptRoot "release_blocker_metadata_helpers.ps1")

function Get-DisplayValue {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "(暂无)"
    }

    return $Value
}

function Get-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Value
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return ""
    }

    try {
        $candidate = if ([System.IO.Path]::IsPathRooted($Value)) {
            $Value
        } else {
            Join-Path $RepoRoot $Value
        }
        $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
        $resolvedValue = [System.IO.Path]::GetFullPath($candidate)
    } catch {
        return ""
    }

    if (-not $resolvedRepoRoot.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedRepoRoot.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedRepoRoot += [System.IO.Path]::DirectorySeparatorChar
    }

    try {
        $repoUri = [System.Uri]::new($resolvedRepoRoot)
        $valueUri = [System.Uri]::new($resolvedValue)
    } catch {
        return ""
    }

    if ($repoUri.Scheme -ne $valueUri.Scheme) {
        return ""
    }

    try {
        $relativeUri = $repoUri.MakeRelativeUri($valueUri)
    } catch {
        return ""
    }

    $relative = [System.Uri]::UnescapeDataString(
        $relativeUri.ToString()
    ).Replace('/', [System.IO.Path]::DirectorySeparatorChar)

    if ($relative -eq ".." -or
        $relative.StartsWith("..\") -or
        $relative.StartsWith("../")) {
        return ""
    }

    return $relative
}

function Get-PublicArtifactPath {
    param(
        [string]$RepoRoot,
        [string]$Value,
        [string]$Fallback = ""
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Fallback
    }

    $relative = Get-RepoRelativePath -RepoRoot $RepoRoot -Value $Value
    if (-not [string]::IsNullOrWhiteSpace($relative)) {
        return $relative
    }

    if (-not [string]::IsNullOrWhiteSpace($Fallback)) {
        return $Fallback
    }

    return Split-Path -Leaf $Value
}

function Get-ProjectVersion {
    param([string]$RepoRoot)

    $cmakePath = Join-Path $RepoRoot "CMakeLists.txt"
    if (-not (Test-Path -LiteralPath $cmakePath)) {
        return ""
    }

    $content = Get-Content -Raw $cmakePath
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

function Get-ChineseSectionName {
    param([string]$SectionName)

    switch ($SectionName) {
        "Added" { return "新增" }
        "Changed" { return "变更" }
        "Fixed" { return "修复" }
        "Validation" { return "验证" }
        "Performance" { return "性能" }
        "Dependencies" { return "依赖" }
        default { return $SectionName }
    }
}

function Get-ChangelogSectionsByHeader {
    param(
        [string]$RepoRoot,
        [string]$SectionHeader
    )

    $changelogPath = Join-Path $RepoRoot "CHANGELOG.md"
    if (-not (Test-Path -LiteralPath $changelogPath)) {
        return [ordered]@{}
    }

    $lines = Get-Content $changelogPath
    $escapedSectionHeader = [regex]::Escape($SectionHeader)
    $inSection = $false
    $currentSection = ""
    $sections = [ordered]@{}

    foreach ($rawLine in $lines) {
        $line = $rawLine.TrimEnd()

        if ($line -match ('^## \[' + $escapedSectionHeader + '\]')) {
            $inSection = $true
            continue
        }

        if (-not $inSection) {
            continue
        }

        if ($line -match '^## \[') {
            break
        }

        if ($line -match '^###\s+(.+)$') {
            $currentSection = $Matches[1].Trim()
            if (-not $sections.Contains($currentSection)) {
                $sections[$currentSection] = New-Object 'System.Collections.Generic.List[string]'
            }
            continue
        }

        if ([string]::IsNullOrWhiteSpace($currentSection)) {
            continue
        }

        if ($line -match '^- ') {
            $bullet = $line.Substring(2).Trim()
            [void]$sections[$currentSection].Add($bullet)
            continue
        }

        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }

        $sectionBullets = $sections[$currentSection]
        if ($sectionBullets.Count -gt 0) {
            $sectionBullets[$sectionBullets.Count - 1] = ($sectionBullets[$sectionBullets.Count - 1] + " " + $line.Trim())
        }
    }

    return $sections
}

function Resolve-ChangelogSelection {
    param(
        [string]$RepoRoot,
        [string]$ReleaseVersion
    )

    if (-not [string]::IsNullOrWhiteSpace($ReleaseVersion)) {
        $versionSections = Get-ChangelogSectionsByHeader -RepoRoot $RepoRoot -SectionHeader $ReleaseVersion
        if ($versionSections.Count -gt 0) {
            return [pscustomobject]@{
                Sections = $versionSections
                SourceLabel = ('`{0}` 版本区块' -f $ReleaseVersion)
            }
        }
    }

    return [pscustomobject]@{
        Sections = (Get-ChangelogSectionsByHeader -RepoRoot $RepoRoot -SectionHeader "Unreleased")
        SourceLabel = '`Unreleased` 区块'
    }
}

function Add-ChangelogSummaryLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        $Sections,
        [string]$SourceLabel = '`Unreleased` 区块'
    )

    if ($null -eq $Sections -or $Sections.Count -eq 0) {
        [void]$Lines.Add(' - 本次发布以稳定性、安装验证和发布材料整理为主，详细变更请参考 `CHANGELOG.md`。'.TrimStart())
        return
    }

    $preferredOrder = @("Added", "Changed", "Fixed", "Validation", "Performance", "Dependencies")
    $orderedNames = New-Object 'System.Collections.Generic.List[string]'
    foreach ($name in $preferredOrder) {
        if ($Sections.Contains($name)) {
            [void]$orderedNames.Add($name)
        }
    }
    foreach ($property in $Sections.Keys) {
        if (-not $orderedNames.Contains($property)) {
            [void]$orderedNames.Add($property)
        }
    }

    foreach ($sectionName in $orderedNames) {
        $bullets = $Sections[$sectionName]
        if ($null -eq $bullets -or $bullets.Count -eq 0) {
            continue
        }

        [void]$Lines.Add("- $(Get-ChineseSectionName -SectionName $sectionName)：")

        $maxBullets = if ($sectionName -eq "Added") { 6 } else { 4 }
        $takeCount = [Math]::Min($bullets.Count, $maxBullets)
        for ($index = 0; $index -lt $takeCount; $index++) {
            $normalizedBullet = Normalize-ChangelogBullet -Bullet $bullets[$index] -SectionName $sectionName

            [void]$Lines.Add("  - $normalizedBullet")
        }

        if ($bullets.Count -gt $takeCount) {
            [void]$Lines.Add(('  - 其余 {0} 条请继续参考 `CHANGELOG.md`。' -f ($bullets.Count - $takeCount)))
        }
    }
}

function Add-UniqueLine {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Line
    )

    if ([string]::IsNullOrWhiteSpace($Line)) {
        return
    }

    if (-not $Lines.Contains($Line)) {
        [void]$Lines.Add($Line)
    }
}

function Get-FirstGovernanceReport {
    param(
        [AllowNull()]$Summary,
        [string]$Id,
        [string]$Schema
    )

    $rollup = Get-ReleaseGovernanceRollup -Summary $Summary
    $handoff = Get-ReleaseGovernanceHandoff -Summary $Summary
    foreach ($report in @(
            Get-ReleaseBlockerArrayProperty -Object $rollup -Name "source_reports"
            Get-ReleaseBlockerArrayProperty -Object $handoff -Name "reports"
        )) {
        $reportId = Get-ReleaseBlockerPropertyValue -Object $report -Name "id"
        $reportSchema = Get-ReleaseBlockerPropertyValue -Object $report -Name "schema"
        if ((-not [string]::IsNullOrWhiteSpace($Id) -and [string]::Equals($reportId, $Id, [System.StringComparison]::OrdinalIgnoreCase)) -or
            (-not [string]::IsNullOrWhiteSpace($Schema) -and [string]::Equals($reportSchema, $Schema, [System.StringComparison]::OrdinalIgnoreCase))) {
            return $report
        }
    }

    return $null
}

function Get-GovernanceSourceReportDisplay {
    param([AllowNull()]$Item)

    foreach ($propertyName in @("source_report_display", "path_display", "expected_summary_display", "source_report", "path", "expected_summary")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $propertyName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }

    return ""
}

function Get-GovernanceSourceJsonDisplay {
    param([AllowNull()]$Item)

    foreach ($propertyName in @("source_json_display", "source_json", "path_display", "expected_summary_display", "path", "expected_summary")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $propertyName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }

    return ""
}
