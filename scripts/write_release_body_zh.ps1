param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$OutputPath = "",
    [string]$ShortOutputPath = "",
    [string]$ReleaseVersion = ""
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
    if ($null -eq $property) {
        return ""
    }

    if ($null -eq $property.Value) {
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

function Get-OptionalPropertyArray {
    param(
        $Object,
        [string]$Name
    )

    $propertyValue = Get-OptionalPropertyObject -Object $Object -Name $Name
    if ($null -eq $propertyValue) {
        return @()
    }

    return @($propertyValue)
}

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
        $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
        $resolvedValue = [System.IO.Path]::GetFullPath($Value)
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

function Get-VisualTaskVerdict {
    param(
        $VisualGateSummary,
        $GateSummary,
        [string]$TaskKey
    )

    $summaryVerdict = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_verdict" -f $TaskKey)
    if (-not [string]::IsNullOrWhiteSpace($summaryVerdict)) {
        return $summaryVerdict
    }

    $manualReview = Get-OptionalPropertyObject -Object $GateSummary -Name "manual_review"
    $tasks = Get-OptionalPropertyObject -Object $manualReview -Name "tasks"
    $taskReview = Get-OptionalPropertyObject -Object $tasks -Name $TaskKey
    return Get-OptionalPropertyValue -Object $taskReview -Name "verdict"
}

function Get-CuratedVisualReviewEntries {
    param(
        $VisualGateSummary,
        $GateSummary
    )

    $entryMap = @{}
    $entryOrder = New-Object 'System.Collections.Generic.List[string]'
    $fallbackIndex = 0

    $manualReview = Get-OptionalPropertyObject -Object $GateSummary -Name "manual_review"
    $manualTasks = Get-OptionalPropertyObject -Object $manualReview -Name "tasks"

    $sources = @(
        (Get-OptionalPropertyArray -Object $VisualGateSummary -Name "curated_visual_regressions"),
        (Get-OptionalPropertyArray -Object $manualTasks -Name "curated_visual_regressions"),
        (Get-OptionalPropertyArray -Object $GateSummary -Name "curated_visual_regressions")
    )

    foreach ($sourceGroup in $sources) {
        foreach ($source in $sourceGroup) {
            $fallbackIndex += 1

            $id = Get-OptionalPropertyValue -Object $source -Name "id"
            $displayLabel = Get-OptionalPropertyValue -Object $source -Name "display_label"
            $label = if (-not [string]::IsNullOrWhiteSpace($displayLabel)) {
                $displayLabel
            } else {
                Get-OptionalPropertyValue -Object $source -Name "label"
            }
            $key = if (-not [string]::IsNullOrWhiteSpace($id)) {
                $id
            } elseif (-not [string]::IsNullOrWhiteSpace($label) -and $label -notlike "curated:*") {
                $label
            } else {
                "__curated_{0}" -f $fallbackIndex
            }

            if (-not $entryMap.ContainsKey($key)) {
                $entryMap[$key] = [ordered]@{
                    id = ""
                    label = ""
                    verdict = ""
                }
                [void]$entryOrder.Add($key)
            }

            if (-not [string]::IsNullOrWhiteSpace($id)) {
                $entryMap[$key].id = $id
            }
            if (-not [string]::IsNullOrWhiteSpace($label) -and $label -notlike "curated:*") {
                $entryMap[$key].label = $label
            }

            $verdict = Get-OptionalPropertyValue -Object $source -Name "verdict"
            if (-not [string]::IsNullOrWhiteSpace($verdict)) {
                $entryMap[$key].verdict = $verdict
            }
        }
    }

    $entries = @()
    foreach ($key in $entryOrder) {
        $entry = $entryMap[$key]
        if ([string]::IsNullOrWhiteSpace($entry.label)) {
            if (-not [string]::IsNullOrWhiteSpace($entry.id)) {
                $entry.label = $entry.id
            } else {
                $entry.label = "Curated visual regression bundle"
            }
        }

        $entries += [pscustomobject]$entry
    }

    return $entries
}

function Get-CommandPathDisplayValue {
    param(
        [string]$RepoRoot,
        [string]$Value
    )

    $relative = Get-RepoRelativePath -RepoRoot $RepoRoot -Value $Value
    if ([string]::IsNullOrWhiteSpace($relative)) {
        return $Value
    }

    if ($relative.StartsWith(".\") -or $relative.StartsWith("./")) {
        return $relative
    }

    return ".\$relative"
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

function Normalize-ReleaseFacingText {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ""
    }

    $normalized = ($Text -replace '\s+', ' ').Trim()
    $normalized = $normalized.Replace("public release drafts", "public release notes")
    $normalized = $normalized.Replace("release drafts and reviewer handoff material", "release notes and reviewer handoff material")
    $normalized = $normalized.Replace("release-note drafts", "release notes")
    $normalized = $normalized.Replace("release-note drafting", "release-note preparation")
    $normalized = $normalized.Replace("release-body drafts", "release bodies")
    $normalized = $normalized.Replace("release-body draft", "release body")
    $normalized = $normalized.Replace("draft review", "release-note review")
    $normalized = $normalized.Replace("release drafts", "release notes")
    $normalized = $normalized.Replace("release draft", "release notes")
    $normalized = $normalized.Replace("draft's", "generated")
    $normalized = $normalized.Replace("`release_summary.zh-CN.md` draft", "`release_summary.zh-CN.md` summary")
    $normalized = $normalized.Replace("发布说明草稿", "发布说明")
    $normalized = $normalized.Replace("草稿", "说明")
    $normalized = $normalized.Replace("C:\Users\...", "Windows absolute paths")
    $normalized = $normalized.Replace("/Users/...", "Unix-style absolute paths")

    return $normalized
}

function Normalize-ChangelogBullet {
    param(
        [string]$Bullet,
        [string]$SectionName = ""
    )

    $normalizedBullet = Normalize-ReleaseFacingText -Text $Bullet
    switch ($SectionName) {
        "Added" { $normalizedBullet = $normalizedBullet -replace '^(Added|Updated)\s+', '' }
        "Changed" { $normalizedBullet = $normalizedBullet -replace '^(Changed|Updated)\s+', '' }
        "Fixed" { $normalizedBullet = $normalizedBullet -replace '^Fixed\s+', '' }
        "Validation" { $normalizedBullet = $normalizedBullet -replace '^Validated\s+', '' }
        "Dependencies" { $normalizedBullet = $normalizedBullet -replace '^Updated\s+', '' }
    }

    return $normalizedBullet.Trim()
}

function Get-NormalizedChangelogEntries {
    param($Sections)

    $entries = New-Object 'System.Collections.Generic.List[object]'
    if ($null -eq $Sections -or $Sections.Count -eq 0) {
        return $entries
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
        if ($null -eq $bullets) {
            continue
        }

        foreach ($bullet in $bullets) {
            $normalizedBullet = Normalize-ChangelogBullet -Bullet $bullet -SectionName $sectionName
            if ([string]::IsNullOrWhiteSpace($normalizedBullet)) {
                continue
            }

            [void]$entries.Add([pscustomobject]@{
                Section = $sectionName
                Text = $normalizedBullet
            })
        }
    }

    return $entries
}

function Test-ChangelogEntryMatch {
    param(
        [System.Collections.Generic.List[object]]$Entries,
        [string]$Pattern
    )

    foreach ($entry in $Entries) {
        if ($entry.Text -match $Pattern) {
            return $true
        }
    }

    return $false
}

function Get-ShortSummaryFallbackBullet {
    param(
        [string]$SectionName,
        [string]$Bullet
    )

    if ([string]::IsNullOrWhiteSpace($Bullet)) {
        return ""
    }

    $label = Get-ChineseSectionName -SectionName $SectionName
    $trimmed = Normalize-ReleaseFacingText -Text $Bullet
    if ($trimmed.Length -gt 92) {
        $trimmed = $trimmed.Substring(0, 89).TrimEnd() + "..."
    }

    return "$label：$trimmed"
}

function Get-ValidationSummaryBullet {
    param(
        [string]$ExecutionStatus,
        [string]$ConfigureStatus,
        [string]$BuildStatus,
        [string]$TestsStatus,
        [string]$InstallSmokeStatus,
        [string]$VisualGateStatus,
        [string]$VisualVerdict
    )

    $resolvedVerdict = if ([string]::IsNullOrWhiteSpace($VisualVerdict)) {
        "pending_manual_review"
    } else {
        $VisualVerdict
    }

    if ($ExecutionStatus -eq "pass" -and $resolvedVerdict -eq "pass") {
        return '本地 full release-preflight 当前为 pass：MSVC configure/build、ctest、install smoke 和 Word visual gate 均已通过，visual verdict 为 `pass`。'
    }

    if ($VisualGateStatus -eq "skipped") {
        return 'CI 侧 release-preflight 当前跳过了 Word visual gate；最终截图级结论仍需在本地 Windows + Microsoft Word 环境补齐。'
    }

    if ($ExecutionStatus -ne "pass") {
        return ('release-preflight 当前未完全通过：MSVC configure/build={0}/{1}，ctest={2}，install smoke={3}，visual gate={4}，visual verdict={5}。' -f `
            $ConfigureStatus, $BuildStatus, $TestsStatus, $InstallSmokeStatus, $VisualGateStatus, $resolvedVerdict)
    }

    return ('release-preflight 当前已完成，但 visual verdict 仍为 `{0}`；对外发布前还需要补齐最终人工复核。' -f $resolvedVerdict)
}

function Get-VisualValidationDetailBullet {
    param(
        [string]$SectionPageSetupVerdict,
        [string]$PageNumberFieldsVerdict,
        [object[]]$CuratedVisualReviewEntries
    )

    $parts = New-Object 'System.Collections.Generic.List[string]'

    if (-not [string]::IsNullOrWhiteSpace($SectionPageSetupVerdict)) {
        [void]$parts.Add('section page setup=`{0}`' -f $SectionPageSetupVerdict)
    }
    if (-not [string]::IsNullOrWhiteSpace($PageNumberFieldsVerdict)) {
        [void]$parts.Add('page number fields=`{0}`' -f $PageNumberFieldsVerdict)
    }

    $curatedWithVerdicts = @(
        $CuratedVisualReviewEntries |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_.verdict) }
    )
    $takeCount = [Math]::Min($curatedWithVerdicts.Count, 3)
    for ($index = 0; $index -lt $takeCount; $index++) {
        $entry = $curatedWithVerdicts[$index]
        [void]$parts.Add(('{0}=`{1}`' -f $entry.label, $entry.verdict))
    }
    if ($curatedWithVerdicts.Count -gt $takeCount) {
        [void]$parts.Add('其余 {0} 个 curated bundle 见 `release_body.zh-CN.md`' -f ($curatedWithVerdicts.Count - $takeCount))
    }

    if ($parts.Count -eq 0) {
        return ""
    }

    return 'Word visual gate 细分结论：{0}。' -f ($parts -join '，')
}

function Get-ShortSummaryBullets {
    param(
        $Sections,
        [string]$ChangelogSourceLabel = '`Unreleased` 区块',
        [string]$ExecutionStatus,
        [string]$ConfigureStatus,
        [string]$BuildStatus,
        [string]$TestsStatus,
        [string]$InstallSmokeStatus,
        [string]$VisualGateStatus,
        [string]$VisualVerdict,
        [string]$InstalledDataDir,
        [string]$TemplateSchemaManifestStatus,
        [string]$TemplateSchemaManifestPassed,
        [string]$TemplateSchemaManifestEntryCount,
        [string]$TemplateSchemaManifestDriftCount,
        [string]$SectionPageSetupVerdict,
        [string]$PageNumberFieldsVerdict,
        [object[]]$CuratedVisualReviewEntries
    )

    $bullets = New-Object 'System.Collections.Generic.List[string]'
    $entries = Get-NormalizedChangelogEntries -Sections $Sections

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'README\.zh-CN\.md|docs/index\.rst|VISUAL_VALIDATION(\.zh-CN)?\.md') {
        Add-UniqueLine -Lines $bullets -Line '文档首页、仓库首页和安装树入口现在统一展示完整 Word smoke contact sheet，以及 fixed-grid、merge/unmerge、纵排与 RTL/LTR/CJK 混排重点图。'
    }

    if (-not [string]::IsNullOrWhiteSpace($InstalledDataDir) -or
        (Test-ChangelogEntryMatch -Entries $entries -Pattern 'VISUAL_VALIDATION_QUICKSTART|RELEASE_ARTIFACT_TEMPLATE')) {
        Add-UniqueLine -Lines $bullets -Line '安装树 `share/FeatherDoc` 现在直接附带 quickstart、release 模板和预览 PNG，可从截图一路跳到复现脚本与 review task。'
    }

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'write_release_artifact_handoff\.ps1|write_release_body_zh\.ps1|release_body\.zh-CN\.md|release_handoff\.md') {
        Add-UniqueLine -Lines $bullets -Line ('release-preflight 现在会自动产出 `release_handoff.md`、`release_body.zh-CN.md` 和 `release_summary.zh-CN.md`，并从 `CHANGELOG.md` 的 {0} 预填核心变化。' -f $ChangelogSourceLabel)
    }

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'windows-msvc-release-metadata|windows-msvc\.yml') {
        Add-UniqueLine -Lines $bullets -Line 'Windows CI 现在会额外上传 `windows-msvc-release-metadata` artifact，把安装树文档入口和 release report 一并交给评审或发布人。'
    }

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'table_style_look|w:tblLook') {
        Add-UniqueLine -Lines $bullets -Line '现有表格现在可以直接编辑 `w:tblLook`，首末行列强调和行列 banding 可在不落回原始 XML 的情况下安全调节。'
    }

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'insert_cell_before|insert_cell_after|TableCell::remove\(\)|unmerge_right|unmerge_down|column_width_twips|set_column_width_twips|clear_column_width') {
        Add-UniqueLine -Lines $bullets -Line 'fixed-grid 表格现在补齐安全插列、删列、列宽编辑与 merge/unmerge 四象限能力，reopened fixed-layout 表格也能保持 `w:tblGrid` / `w:tcW` 一致。'
    }

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'restart_paragraph_list') {
        Add-UniqueLine -Lines $bullets -Line '段落列表现在支持安全重启编号，Word 渲染下可以稳定得到新的 `1.` 起始序列。'
    }

    foreach ($entry in $entries) {
        if ($bullets.Count -ge 7) {
            break
        }

        $fallbackBullet = Get-ShortSummaryFallbackBullet -SectionName $entry.Section -Bullet $entry.Text
        Add-UniqueLine -Lines $bullets -Line $fallbackBullet
    }

    if ($bullets.Count -gt 7) {
        while ($bullets.Count -gt 7) {
            $bullets.RemoveAt($bullets.Count - 1)
        }
    }

    Add-UniqueLine -Lines $bullets -Line (Get-ValidationSummaryBullet `
        -ExecutionStatus $ExecutionStatus `
        -ConfigureStatus $ConfigureStatus `
        -BuildStatus $BuildStatus `
        -TestsStatus $TestsStatus `
        -InstallSmokeStatus $InstallSmokeStatus `
        -VisualGateStatus $VisualGateStatus `
        -VisualVerdict $VisualVerdict)

    if (-not [string]::IsNullOrWhiteSpace($TemplateSchemaManifestStatus) -and
        $TemplateSchemaManifestStatus -ne "not_requested") {
        if ($TemplateSchemaManifestPassed -eq "True") {
            Add-UniqueLine -Lines $bullets -Line (
                '仓库级 template schema manifest 当前覆盖 {0} 份 baseline，漂移数为 {1}，并已纳入 release preflight。' -f `
                    $TemplateSchemaManifestEntryCount, $TemplateSchemaManifestDriftCount
            )
        } else {
            Add-UniqueLine -Lines $bullets -Line (
                '仓库级 template schema manifest 当前覆盖 {0} 份 baseline，但仍有 {1} 份漂移待处理。' -f `
                    $TemplateSchemaManifestEntryCount, $TemplateSchemaManifestDriftCount
            )
        }
    }

    $visualValidationDetailBullet = Get-VisualValidationDetailBullet `
        -SectionPageSetupVerdict $SectionPageSetupVerdict `
        -PageNumberFieldsVerdict $PageNumberFieldsVerdict `
        -CuratedVisualReviewEntries $CuratedVisualReviewEntries
    Add-UniqueLine -Lines $bullets -Line $visualValidationDetailBullet

    $preservedTailCount = 1
    if (-not [string]::IsNullOrWhiteSpace($visualValidationDetailBullet)) {
        $preservedTailCount = 2
    }
    $maxBulletCount = 7 + $preservedTailCount
    if ($bullets.Count -gt $maxBulletCount) {
        while ($bullets.Count -gt $maxBulletCount) {
            $removalIndex = [Math]::Max(0, $bullets.Count - $preservedTailCount - 1)
            $bullets.RemoveAt($removalIndex)
        }
    }

    return $bullets
}

function Get-ValidationNote {
    param(
        [string]$ExecutionStatus,
        [string]$VisualGateStatus,
        [string]$VisualVerdict
    )

    if ($ExecutionStatus -ne "pass") {
        return "当前 preflight 未完全通过，发布前先处理失败步骤，再重刷本文件。"
    }

    if ($VisualGateStatus -eq "skipped") {
        return "这份结果来自 CI 或跳过 visual gate 的本地检查；Word 截图级复核仍需在本地 Windows + Microsoft Word 环境补齐。"
    }

    if ($VisualVerdict -eq "pass") {
        return "本次 preflight 已补齐截图级 Word 复核，当前 visual verdict 为 pass。"
    }

    if ($VisualVerdict -eq "pending_manual_review" -or $VisualVerdict -eq "undecided" -or [string]::IsNullOrWhiteSpace($VisualVerdict)) {
        return "本次 preflight 已跑到 visual gate，但截图级人工复核尚未完成；发布前请先补写 visual verdict。"
    }

    if ($VisualVerdict -eq "fail") {
        return "截图级 Word 复核当前为 fail；发布前请先解决视觉回归。"
    }

    return "请根据当前验证结果确认是否可以对外发布。"
}

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
if (-not (Test-Path -LiteralPath $resolvedSummaryPath)) {
    throw "Summary JSON does not exist: $resolvedSummaryPath"
}

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path (Split-Path -Parent $resolvedSummaryPath) "release_body.zh-CN.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputPath
}

$resolvedShortOutputPath = if ([string]::IsNullOrWhiteSpace($ShortOutputPath)) {
    Join-Path (Split-Path -Parent $resolvedSummaryPath) "release_summary.zh-CN.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ShortOutputPath
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$reportDir = Split-Path -Parent $resolvedSummaryPath
$finalReviewPath = Join-Path $reportDir "final_review.md"
$releaseHandoffPath = Get-OptionalPropertyValue -Object $summary -Name "release_handoff"
if ([string]::IsNullOrWhiteSpace($releaseHandoffPath)) {
    $releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
}
$artifactGuidePath = Get-OptionalPropertyValue -Object $summary -Name "artifact_guide"
if ([string]::IsNullOrWhiteSpace($artifactGuidePath)) {
    $artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
}
$reviewerChecklistPath = Get-OptionalPropertyValue -Object $summary -Name "reviewer_checklist"
if ([string]::IsNullOrWhiteSpace($reviewerChecklistPath)) {
    $reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
}
$resolvedReleaseVersion = Resolve-ReleaseVersion `
    -RepoRoot $repoRoot `
    -Summary $summary `
    -ExplicitVersion $ReleaseVersion `
    -ExistingHandoffPath $releaseHandoffPath

$installPrefix = Get-OptionalPropertyValue -Object $summary.steps.install_smoke -Name "install_prefix"
$consumerDocument = Get-OptionalPropertyValue -Object $summary.steps.install_smoke -Name "consumer_document"
$gateSummaryPath = Get-OptionalPropertyValue -Object $summary.steps.visual_gate -Name "summary_json"
$gateFinalReviewPath = Get-OptionalPropertyValue -Object $summary.steps.visual_gate -Name "final_review"
$templateSchemaManifestSummary = Get-OptionalPropertyObject -Object $summary -Name "template_schema_manifest"
$templateSchemaManifestStep = Get-OptionalPropertyObject -Object $summary.steps -Name "template_schema_manifest"
$templateSchemaManifestStatus = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "status"
$templateSchemaManifestPassed = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "passed"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestPassed)) {
    $templateSchemaManifestPassed = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "passed"
}
$templateSchemaManifestEntryCount = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "entry_count"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestEntryCount)) {
    $templateSchemaManifestEntryCount = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "entry_count"
}
$templateSchemaManifestDriftCount = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "drift_count"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestDriftCount)) {
    $templateSchemaManifestDriftCount = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "drift_count"
}
$templateSchemaManifestSummaryPath = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "summary_json"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestSummaryPath)) {
    $templateSchemaManifestSummaryPath = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "summary_json"
}
$projectTemplateSmokeSummary = Get-OptionalPropertyObject -Object $summary -Name "project_template_smoke"
$projectTemplateSmokeStep = Get-OptionalPropertyObject -Object $summary.steps -Name "project_template_smoke"
$projectTemplateSmokeRequested = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "requested"
$projectTemplateSmokeStatus = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "status"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeStatus)) {
    $projectTemplateSmokeStatus = if ($projectTemplateSmokeRequested -eq "True") { "requested" } else { "not_requested" }
}
$projectTemplateSmokePassed = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "passed"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokePassed)) {
    $projectTemplateSmokePassed = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "passed"
}
$projectTemplateSmokeEntryCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "entry_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeEntryCount)) {
    $projectTemplateSmokeEntryCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "entry_count"
}
$projectTemplateSmokeFailedEntryCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "failed_entry_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeFailedEntryCount)) {
    $projectTemplateSmokeFailedEntryCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "failed_entry_count"
}
$projectTemplateSmokeVisualVerdict = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "visual_verdict"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeVisualVerdict)) {
    $projectTemplateSmokeVisualVerdict = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "visual_verdict"
}
$projectTemplateSmokePendingReviewCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "manual_review_pending_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokePendingReviewCount)) {
    $projectTemplateSmokePendingReviewCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "manual_review_pending_count"
}
$projectTemplateSmokeSummaryPath = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "summary_json"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeSummaryPath)) {
    $projectTemplateSmokeSummaryPath = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "summary_json"
}
$projectTemplateSmokeCandidateCoverage = Get-OptionalPropertyObject -Object $projectTemplateSmokeStep -Name "candidate_coverage"
if ($null -eq $projectTemplateSmokeCandidateCoverage) {
    $projectTemplateSmokeCandidateCoverage = Get-OptionalPropertyObject -Object $projectTemplateSmokeSummary -Name "candidate_coverage"
}
$projectTemplateSmokeRequireFullCoverage = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "require_full_coverage"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeRequireFullCoverage)) {
    $projectTemplateSmokeRequireFullCoverage = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "require_full_coverage"
}
$projectTemplateSmokeCandidateDiscoveryJson = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "candidate_discovery_json"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeCandidateDiscoveryJson)) {
    $projectTemplateSmokeCandidateDiscoveryJson = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "candidate_discovery_json"
}
$projectTemplateSmokeCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "candidate_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeCandidateCount)) {
    $projectTemplateSmokeCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "candidate_count"
}
$projectTemplateSmokeRegisteredCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "registered_candidate_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeRegisteredCandidateCount)) {
    $projectTemplateSmokeRegisteredCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "registered_candidate_count"
}
$projectTemplateSmokeUnregisteredCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "unregistered_candidate_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeUnregisteredCandidateCount)) {
    $projectTemplateSmokeUnregisteredCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "unregistered_candidate_count"
}
$projectTemplateSmokeExcludedCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "excluded_candidate_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeExcludedCandidateCount)) {
    $projectTemplateSmokeExcludedCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "excluded_candidate_count"
}

$visualVerdict = ""
$readmeGalleryStatus = ""
$readmeGalleryAssetsDir = ""
$gateSummary = $null
if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath) -and (Test-Path -LiteralPath $gateSummaryPath)) {
    $gateSummary = Get-Content -Raw $gateSummaryPath | ConvertFrom-Json
    $visualVerdict = Get-OptionalPropertyValue -Object $gateSummary -Name "visual_verdict"
    $readmeGallery = Get-OptionalPropertyObject -Object $gateSummary -Name "readme_gallery"
    $readmeGalleryStatus = Get-OptionalPropertyValue -Object $readmeGallery -Name "status"
    $readmeGalleryAssetsDir = Get-OptionalPropertyValue -Object $readmeGallery -Name "assets_dir"
}
$sectionPageSetupVerdict = Get-VisualTaskVerdict -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsVerdict = Get-VisualTaskVerdict -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "page_number_fields"
$curatedVisualReviewEntries = @(Get-CuratedVisualReviewEntries -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary)

$installedDataDir = ""
$installedQuickstartZh = ""
$installedTemplateZh = ""
$installedVisualDir = ""
if (-not [string]::IsNullOrWhiteSpace($installPrefix)) {
    $installedDataDir = Join-Path $installPrefix "share\FeatherDoc"
    $installedQuickstartZh = Join-Path $installedDataDir "VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
    $installedTemplateZh = Join-Path $installedDataDir "RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
    $installedVisualDir = Join-Path $installedDataDir "visual-validation"
}

$publicInstalledQuickstartZh = if ([string]::IsNullOrWhiteSpace($installedQuickstartZh)) {
    ""
} else {
    "share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
}
$publicInstalledTemplateZh = if ([string]::IsNullOrWhiteSpace($installedTemplateZh)) {
    ""
} else {
    "share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
}
$publicInstalledVisualDir = if ([string]::IsNullOrWhiteSpace($installedVisualDir)) {
    ""
} else {
    "share\FeatherDoc\visual-validation"
}

$publicSummaryPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $resolvedSummaryPath
$publicShortOutputPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $resolvedShortOutputPath
$publicFinalReviewPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $finalReviewPath
$publicReleaseHandoffPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $releaseHandoffPath
$publicArtifactGuidePath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $artifactGuidePath
$publicReviewerChecklistPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $reviewerChecklistPath
$publicGateSummaryPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $gateSummaryPath
$publicGateFinalReviewPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $gateFinalReviewPath
$publicReadmeGalleryAssetsDir = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $readmeGalleryAssetsDir
$publicConsumerDocument = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $consumerDocument
$publicProjectTemplateSmokeCandidateDiscoveryJson = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $projectTemplateSmokeCandidateDiscoveryJson

$validationNote = Get-ValidationNote `
    -ExecutionStatus $summary.execution_status `
    -VisualGateStatus $summary.steps.visual_gate.status `
    -VisualVerdict $visualVerdict
$changelogSelection = Resolve-ChangelogSelection -RepoRoot $repoRoot -ReleaseVersion $resolvedReleaseVersion
$changelogSections = $changelogSelection.Sections
$changelogSourceLabel = $changelogSelection.SourceLabel
$shortSummaryBullets = Get-ShortSummaryBullets `
    -Sections $changelogSections `
    -ChangelogSourceLabel $changelogSourceLabel `
    -ExecutionStatus $summary.execution_status `
    -ConfigureStatus $summary.steps.configure.status `
    -BuildStatus $summary.steps.build.status `
    -TestsStatus $summary.steps.tests.status `
    -InstallSmokeStatus $summary.steps.install_smoke.status `
    -VisualGateStatus $summary.steps.visual_gate.status `
    -VisualVerdict $visualVerdict `
    -InstalledDataDir $installedDataDir `
    -TemplateSchemaManifestStatus $templateSchemaManifestStatus `
    -TemplateSchemaManifestPassed $templateSchemaManifestPassed `
    -TemplateSchemaManifestEntryCount $templateSchemaManifestEntryCount `
    -TemplateSchemaManifestDriftCount $templateSchemaManifestDriftCount `
    -SectionPageSetupVerdict $sectionPageSetupVerdict `
    -PageNumberFieldsVerdict $pageNumberFieldsVerdict `
    -CuratedVisualReviewEntries $curatedVisualReviewEntries

$releaseChecksCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1"
$releaseGateCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1"
$refreshCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\write_release_note_bundle.ps1 -SummaryJson "{0}" -HandoffOutputPath "{1}" -BodyOutputPath "{2}" -ShortOutputPath "{3}"' -f `
    (Get-CommandPathDisplayValue -RepoRoot $repoRoot -Value $resolvedSummaryPath),
    (Get-CommandPathDisplayValue -RepoRoot $repoRoot -Value $releaseHandoffPath),
    (Get-CommandPathDisplayValue -RepoRoot $repoRoot -Value $resolvedOutputPath),
    (Get-CommandPathDisplayValue -RepoRoot $repoRoot -Value $resolvedShortOutputPath)
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseVersion)) {
    $refreshCommand += (' -ReleaseVersion "{0}"' -f $resolvedReleaseVersion)
}

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("# FeatherDoc v$(if ($resolvedReleaseVersion) { $resolvedReleaseVersion } else { '<版本号>' }) 发布说明")
[void]$lines.Add("")
[void]$lines.Add("## 核心变化")
Add-ChangelogSummaryLines -Lines $lines -Sections $changelogSections -SourceLabel $changelogSourceLabel
[void]$lines.Add("")
[void]$lines.Add("## 验证结论")
[void]$lines.Add("- 执行状态：$($summary.execution_status)")
[void]$lines.Add("- MSVC configure/build：$($summary.steps.configure.status) / $($summary.steps.build.status)")
[void]$lines.Add("- ctest：$($summary.steps.tests.status)")
[void]$lines.Add("- template schema manifest gate：$(Get-DisplayValue -Value $templateSchemaManifestStatus)")
[void]$lines.Add("- template schema manifest passed：$(Get-DisplayValue -Value $templateSchemaManifestPassed)")
[void]$lines.Add("- template schema manifest entries / drifts：$(Get-DisplayValue -Value ('{0}/{1}' -f $templateSchemaManifestEntryCount, $templateSchemaManifestDriftCount))")
[void]$lines.Add("- project template smoke gate：$(Get-DisplayValue -Value $projectTemplateSmokeStatus)")
[void]$lines.Add("- project template smoke passed：$(Get-DisplayValue -Value $projectTemplateSmokePassed)")
[void]$lines.Add("- project template smoke entries / failed：$(Get-DisplayValue -Value ('{0}/{1}' -f $projectTemplateSmokeEntryCount, $projectTemplateSmokeFailedEntryCount))")
[void]$lines.Add("- project template smoke candidates registered / unregistered / excluded：$(Get-DisplayValue -Value ('{0}/{1}/{2}' -f $projectTemplateSmokeRegisteredCandidateCount, $projectTemplateSmokeUnregisteredCandidateCount, $projectTemplateSmokeExcludedCandidateCount))")
[void]$lines.Add("- project template smoke full coverage required：$(Get-DisplayValue -Value $projectTemplateSmokeRequireFullCoverage)")
[void]$lines.Add("- project template smoke visual verdict：$(Get-DisplayValue -Value $projectTemplateSmokeVisualVerdict)")
[void]$lines.Add("- project template smoke pending reviews：$(Get-DisplayValue -Value $projectTemplateSmokePendingReviewCount)")
[void]$lines.Add("- install + find_package smoke：$($summary.steps.install_smoke.status)")
[void]$lines.Add("- Word visual release gate：$($summary.steps.visual_gate.status)")
[void]$lines.Add("- Visual verdict：$(if ($visualVerdict) { $visualVerdict } else { 'pending_manual_review' })")
[void]$lines.Add("- Section page setup verdict：$(Get-DisplayValue -Value $sectionPageSetupVerdict)")
[void]$lines.Add("- Page number fields verdict：$(Get-DisplayValue -Value $pageNumberFieldsVerdict)")
[void]$lines.Add("- Curated visual regression bundles：$($curatedVisualReviewEntries.Count)")
foreach ($curatedVisualReview in $curatedVisualReviewEntries) {
    [void]$lines.Add("- $($curatedVisualReview.label) verdict：$(Get-DisplayValue -Value $curatedVisualReview.verdict)")
}
[void]$lines.Add("- README 展示图刷新：$(if ($readmeGalleryStatus) { $readmeGalleryStatus } else { 'unknown' })")
[void]$lines.Add("- 说明：$validationNote")
[void]$lines.Add("")
[void]$lines.Add("## 安装包入口")
[void]$lines.Add("- 以下路径使用安装树相对位置，不包含本机绝对目录。")
[void]$lines.Add("- Quickstart：$(Get-DisplayValue -Value $publicInstalledQuickstartZh)")
[void]$lines.Add("- Release 模板：$(Get-DisplayValue -Value $publicInstalledTemplateZh)")
[void]$lines.Add("- 预览图目录：$(Get-DisplayValue -Value $publicInstalledVisualDir)")
[void]$lines.Add("")
[void]$lines.Add("## 复现与复核命令")
[void]$lines.Add('```powershell')
[void]$lines.Add($releaseChecksCommand)
[void]$lines.Add($releaseGateCommand)
[void]$lines.Add($refreshCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("## 证据文件")
[void]$lines.Add("- 以下路径默认使用仓库相对位置，不包含本机绝对目录。")
[void]$lines.Add("- Release candidate summary JSON：$(Get-DisplayValue -Value $publicSummaryPath)")
[void]$lines.Add("- Final review：$(Get-DisplayValue -Value $publicFinalReviewPath)")
[void]$lines.Add("- Release handoff：$(Get-DisplayValue -Value $publicReleaseHandoffPath)")
[void]$lines.Add("- Release short summary：$(Get-DisplayValue -Value $publicShortOutputPath)")
[void]$lines.Add("- Artifact guide：$(Get-DisplayValue -Value $publicArtifactGuidePath)")
[void]$lines.Add("- Reviewer checklist：$(Get-DisplayValue -Value $publicReviewerChecklistPath)")
[void]$lines.Add("- Template schema manifest summary：$(Get-DisplayValue -Value (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $templateSchemaManifestSummaryPath))")
[void]$lines.Add("- Project template smoke summary：$(Get-DisplayValue -Value (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $projectTemplateSmokeSummaryPath))")
[void]$lines.Add("- Project template smoke candidate discovery：$(Get-DisplayValue -Value $publicProjectTemplateSmokeCandidateDiscoveryJson)")
[void]$lines.Add("- Visual gate summary：$(Get-DisplayValue -Value $publicGateSummaryPath)")
[void]$lines.Add("- Visual gate final review：$(Get-DisplayValue -Value $publicGateFinalReviewPath)")
[void]$lines.Add("- README 展示图目录：$(Get-DisplayValue -Value $publicReadmeGalleryAssetsDir)")
[void]$lines.Add("- install smoke consumer docx：$(Get-DisplayValue -Value $publicConsumerDocument)")
[void]$lines.Add("")
[void]$lines.Add("## 补充说明")
[void]$lines.Add('- 如果 `Visual verdict` 不是 `pass`，请先完成本地 Word 截图级复核，再对外发布。')
[void]$lines.Add("- 如果这份结果来自 CI，请结合本地 Windows preflight 的最终结论后再对外发布。")

New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputPath) -Force | Out-Null
New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedShortOutputPath) -Force | Out-Null
($lines -join [Environment]::NewLine) | Set-Content -Path $resolvedOutputPath -Encoding UTF8

$shortLines = New-Object 'System.Collections.Generic.List[string]'
[void]$shortLines.Add("# FeatherDoc v$(if ($resolvedReleaseVersion) { $resolvedReleaseVersion } else { '<版本号>' }) 发布摘要")
[void]$shortLines.Add("")

if ($shortSummaryBullets.Count -eq 0) {
    [void]$shortLines.Add('- 未能自动提取短摘要，请改为参考 `release_body.zh-CN.md` 手工压缩。')
} else {
    foreach ($bullet in $shortSummaryBullets) {
        [void]$shortLines.Add("- $bullet")
    }
}

($shortLines -join [Environment]::NewLine) | Set-Content -Path $resolvedShortOutputPath -Encoding UTF8

Write-Host "Release body output: $resolvedOutputPath"
Write-Host "Release summary output: $resolvedShortOutputPath"
