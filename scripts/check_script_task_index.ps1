param(
    [string]$RepoRoot = "",
    [string]$SummaryJson = "output/script-task-index-check/summary.json",
    [string]$ReportMarkdown = "output/script-task-index-check/script_task_index_check.md",
    [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    param([string]$InputRoot)

    if ([string]::IsNullOrWhiteSpace($InputRoot)) {
        return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    }

    return (Resolve-Path $InputRoot).Path
}

function Resolve-RepoPath {
    param(
        [string]$Root,
        [string]$InputPath
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }
    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        return [System.IO.Path]::GetFullPath($InputPath)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $Root $InputPath))
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

function Read-Utf8Text {
    param([string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        throw "File must be UTF-8 without BOM: $Path"
    }

    return [System.Text.Encoding]::UTF8.GetString($bytes)
}

function Get-RepoRelativePath {
    param(
        [string]$BaseRoot,
        [string]$Path
    )

    $root = [System.IO.Path]::GetFullPath($BaseRoot)
    if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $root += [System.IO.Path]::DirectorySeparatorChar
    }

    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    $rootUri = New-Object System.Uri -ArgumentList $root
    $pathUri = New-Object System.Uri -ArgumentList $resolvedPath
    $relativeUri = $rootUri.MakeRelativeUri($pathUri)
    $relativePath = [System.Uri]::UnescapeDataString($relativeUri.ToString())
    return $relativePath.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
}

function Assert-FileExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing ${Label}: $Path"
    }
}

function Get-UniqueScriptReferences {
    param([string[]]$References)

    $uniqueReferences = New-Object 'System.Collections.Generic.List[string]'
    foreach ($relativePath in $References) {
        if (-not $uniqueReferences.Contains($relativePath)) {
            $uniqueReferences.Add($relativePath)
        }
    }

    return @($uniqueReferences | Sort-Object)
}

function Get-ScriptReferences {
    param([string]$Text)

    $references = New-Object 'System.Collections.Generic.List[string]'
    $matches = [regex]::Matches($Text, 'scripts/[A-Za-z0-9_.-]+\.(ps1|py)')
    foreach ($match in $matches) {
        $relativePath = $match.Value.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
        $references.Add($relativePath)
    }

    return @($references.ToArray())
}

function Get-DuplicateScriptReferences {
    param([string[]]$References)

    return @(
        $References |
            Group-Object |
            Where-Object { $_.Count -gt 1 } |
            Sort-Object Name |
            ForEach-Object {
                [pscustomobject]@{
                    relative_path = $_.Name
                    occurrence_count = $_.Count
                }
            }
    )
}

function Get-ScriptReferenceGroups {
    param([string]$Text)

    $lines = [regex]::Split($Text, "\r?\n")
    $currentGroup = "unsectioned"
    $groupMap = [ordered]@{}
    $scriptPattern = 'scripts/[A-Za-z0-9_.-]+\.(ps1|py)'

    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = [string]$lines[$index]
        if ($index + 1 -lt $lines.Count) {
            $nextLine = ([string]$lines[$index + 1]).Trim()
            if (-not [string]::IsNullOrWhiteSpace($line) -and
                $nextLine.Length -ge 3 -and
                $nextLine -match '^[=\-]+$') {
                $currentGroup = $line.Trim()
            }
        }

        $matches = [regex]::Matches($line, $scriptPattern)
        foreach ($match in $matches) {
            if (-not $groupMap.Contains($currentGroup)) {
                $groupMap[$currentGroup] = New-Object 'System.Collections.Generic.List[string]'
            }

            $relativePath = $match.Value.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
            $groupMap[$currentGroup].Add($relativePath)
        }
    }

    $groups = New-Object 'System.Collections.Generic.List[object]'
    foreach ($groupName in $groupMap.Keys) {
        $references = @($groupMap[$groupName].ToArray())
        $uniqueReferences = @(Get-UniqueScriptReferences -References $references)
        $duplicateReferences = @(Get-DuplicateScriptReferences -References $references)
        $groups.Add([pscustomobject]@{
            group_name = $groupName
            total_script_reference_count = $references.Count
            script_reference_count = $uniqueReferences.Count
            duplicate_script_reference_count = $duplicateReferences.Count
            script_references = $uniqueReferences
            duplicate_script_references = $duplicateReferences
        })
    }

    return @($groups.ToArray())
}

function Get-ScriptReferenceExtensionSummary {
    param([string[]]$References)

    $extensionMap = [ordered]@{}
    foreach ($relativePath in $References) {
        $extension = [System.IO.Path]::GetExtension([string]$relativePath).ToLowerInvariant()
        if ([string]::IsNullOrWhiteSpace($extension)) {
            $extension = "(none)"
        }

        if (-not $extensionMap.Contains($extension)) {
            $extensionMap[$extension] = New-Object 'System.Collections.Generic.List[string]'
        }

        $extensionMap[$extension].Add($relativePath)
    }

    $extensionSummaries = New-Object 'System.Collections.Generic.List[object]'
    foreach ($extension in @($extensionMap.Keys | Sort-Object)) {
        $references = @($extensionMap[$extension].ToArray())
        $uniqueReferences = @(Get-UniqueScriptReferences -References $references)
        $duplicateReferences = @(Get-DuplicateScriptReferences -References $references)
        $extensionSummaries.Add([pscustomobject]@{
            extension = $extension
            total_script_reference_count = $references.Count
            script_reference_count = $uniqueReferences.Count
            duplicate_script_reference_count = $duplicateReferences.Count
            script_references = $uniqueReferences
            duplicate_script_references = $duplicateReferences
        })
    }

    return @($extensionSummaries.ToArray())
}

function Get-MissingMarkers {
    param(
        [string]$Text,
        [string[]]$Markers
    )

    return @(
        $Markers | Where-Object {
            -not $Text.Contains($_)
        }
    )
}

function New-ScriptCheckEntry {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    return [pscustomobject]@{
        relative_path = $RelativePath
        exists = (Test-Path -LiteralPath $path -PathType Leaf)
    }
}

function New-MarkdownReport {
    param(
        [object]$Summary,
        [object[]]$CheckedScripts,
        [string[]]$MissingScripts,
        [object[]]$DuplicateScriptReferences,
        [object[]]$ScriptReferenceGroups,
        [object[]]$ScriptReferenceExtensions,
        [object[]]$MissingMarkers
    )

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Script Task Index Check")
    $lines.Add("")
    $lines.Add("- schema: ``$($Summary.schema)``")
    $lines.Add("- status: ``$($Summary.status)``")
    $lines.Add("- checked_at_utc: ``$($Summary.checked_at_utc)``")
    $lines.Add("- checker: ``$($Summary.checker_name)``")
    $lines.Add("- script_index: ``$($Summary.script_index_relative_path)``")
    $lines.Add("- total_script_reference_count: ``$($Summary.total_script_reference_count)``")
    $lines.Add("- script_reference_count: ``$($Summary.script_reference_count)``")
    $lines.Add("- script_reference_group_count: ``$($Summary.script_reference_group_count)``")
    $lines.Add("- script_reference_extension_count: ``$($Summary.script_reference_extension_count)``")
    $lines.Add("- duplicate_script_reference_count: ``$($Summary.duplicate_script_reference_count)``")
    $lines.Add("- missing_script_count: ``$($Summary.missing_script_count)``")
    $lines.Add("- required_marker_count: ``$($Summary.required_marker_count)``")
    $lines.Add("- missing_marker_count: ``$($Summary.missing_marker_count)``")
    $lines.Add("- summary_json: ``$($Summary.summary_json_relative_path)``")
    $lines.Add("- report_markdown: ``$($Summary.report_markdown_relative_path)``")
    $lines.Add("")
    $lines.Add("## Checked Scripts")
    $lines.Add("")

    foreach ($script in $CheckedScripts) {
        $scriptStatus = if ([bool]$script.exists) { "ok" } else { "missing" }
        $lines.Add("- [$scriptStatus] ``$($script.relative_path)``")
    }

    $lines.Add("")
    $lines.Add("## Script Reference Groups")
    $lines.Add("")
    foreach ($group in $ScriptReferenceGroups) {
        $lines.Add("- ``$($group.group_name)``: $($group.script_reference_count) unique / $($group.total_script_reference_count) total")
    }

    $lines.Add("")
    $lines.Add("## Script Reference Extensions")
    $lines.Add("")
    foreach ($extension in $ScriptReferenceExtensions) {
        $lines.Add("- ``$($extension.extension)``: $($extension.script_reference_count) unique / $($extension.total_script_reference_count) total")
    }

    $lines.Add("")
    $lines.Add("## Duplicate Script References")
    $lines.Add("")
    if ($DuplicateScriptReferences.Count -eq 0) {
        $lines.Add("- none")
    } else {
        foreach ($script in $DuplicateScriptReferences) {
            $lines.Add("- ``$($script.relative_path)`` x$($script.occurrence_count)")
        }
    }

    $lines.Add("")
    $lines.Add("## Missing Scripts")
    $lines.Add("")
    if ($MissingScripts.Count -eq 0) {
        $lines.Add("- none")
    } else {
        foreach ($script in $MissingScripts) {
            $lines.Add("- ``$script``")
        }
    }

    $lines.Add("")
    $lines.Add("## Missing Markers")
    $lines.Add("")
    if ($MissingMarkers.Count -eq 0) {
        $lines.Add("- none")
    } else {
        foreach ($marker in $MissingMarkers) {
            $lines.Add("- ``$($marker.document)`` missing ``$($marker.marker)``")
        }
    }

    return (($lines.ToArray()) -join [Environment]::NewLine) + [Environment]::NewLine
}

$resolvedRepoRoot = Resolve-RepoRoot -InputRoot $RepoRoot
$summaryJsonPath = Resolve-RepoPath -Root $resolvedRepoRoot -InputPath $SummaryJson
$reportMarkdownPath = Resolve-RepoPath -Root $resolvedRepoRoot -InputPath $ReportMarkdown

$scriptIndexRelativePath = "docs\script_task_index_zh.rst"
$indexRelativePath = "docs\index.rst"
$maintenanceRelativePath = "docs\documentation_maintenance_zh.rst"
$scoreRelativePath = "docs\project_score_assessment_zh.rst"
$cmakeRelativePath = "test\CMakeLists.txt"

$scriptIndexPath = Join-Path $resolvedRepoRoot $scriptIndexRelativePath
$indexPath = Join-Path $resolvedRepoRoot $indexRelativePath
$maintenancePath = Join-Path $resolvedRepoRoot $maintenanceRelativePath
$scorePath = Join-Path $resolvedRepoRoot $scoreRelativePath
$cmakePath = Join-Path $resolvedRepoRoot $cmakeRelativePath

Assert-FileExists -Path $scriptIndexPath -Label "script task index doc"
Assert-FileExists -Path $indexPath -Label "sphinx index doc"
Assert-FileExists -Path $maintenancePath -Label "documentation maintenance doc"
Assert-FileExists -Path $scorePath -Label "project score assessment doc"
Assert-FileExists -Path $cmakePath -Label "test CMakeLists"

$scriptIndexText = Read-Utf8Text -Path $scriptIndexPath
$indexText = Read-Utf8Text -Path $indexPath
$maintenanceText = Read-Utf8Text -Path $maintenancePath
$scoreText = Read-Utf8Text -Path $scorePath
$cmakeText = Read-Utf8Text -Path $cmakePath

$allScriptReferences = @(Get-ScriptReferences -Text $scriptIndexText)
$scriptReferences = @(Get-UniqueScriptReferences -References $allScriptReferences)
if ($scriptReferences.Count -eq 0) {
    throw "Script task index does not contain any scripts/*.ps1 or scripts/*.py references."
}

$duplicateScriptReferences = @(Get-DuplicateScriptReferences -References $allScriptReferences)
$scriptReferenceGroups = @(Get-ScriptReferenceGroups -Text $scriptIndexText)
$scriptReferenceExtensions = @(Get-ScriptReferenceExtensionSummary -References $allScriptReferences)

$checkedScripts = @(
    $scriptReferences | ForEach-Object {
        New-ScriptCheckEntry -Root $resolvedRepoRoot -RelativePath $_
    }
)
$missingScripts = @(
    $checkedScripts | Where-Object {
        -not [bool]$_.exists
    } | ForEach-Object {
        $_.relative_path
    }
)

$requiredMarkerGroups = @(
    [pscustomobject]@{
        document = $indexRelativePath
        markers = @("script_task_index_zh")
        text = $indexText
    },
    [pscustomobject]@{
        document = $maintenanceRelativePath
        markers = @("docs/script_task_index_zh.rst", "check_script_task_index.ps1")
        text = $maintenanceText
    },
    [pscustomobject]@{
        document = $scoreRelativePath
        markers = @("docs/script_task_index_zh.rst", "check_script_task_index.ps1")
        text = $scoreText
    },
    [pscustomobject]@{
        document = $cmakeRelativePath
        markers = @(
            "check_script_task_index",
            "check_script_task_index_test.ps1",
            "TIMEOUT 60",
            'LABELS "docs;smoke;governance;scripts"'
        )
        text = $cmakeText
    }
)

$missingMarkers = New-Object 'System.Collections.Generic.List[object]'
foreach ($group in $requiredMarkerGroups) {
    foreach ($marker in @(Get-MissingMarkers -Text $group.text -Markers $group.markers)) {
        $missingMarkers.Add([pscustomobject]@{
            document = $group.document
            marker = $marker
        })
    }
}

$status = if ($missingScripts.Count -eq 0 -and
    $duplicateScriptReferences.Count -eq 0 -and
    $missingMarkers.Count -eq 0) {
    "passed"
} else {
    "failed"
}

$checkedAtUtc = [DateTime]::UtcNow.ToString(
    "yyyy-MM-ddTHH:mm:ss'Z'",
    [System.Globalization.CultureInfo]::InvariantCulture
)
$summaryJsonRelativePath = Get-RepoRelativePath -BaseRoot $resolvedRepoRoot -Path $summaryJsonPath
$reportMarkdownRelativePath = Get-RepoRelativePath -BaseRoot $resolvedRepoRoot -Path $reportMarkdownPath
$requiredMarkerCount = [int](($requiredMarkerGroups | ForEach-Object { $_.markers.Count } | Measure-Object -Sum).Sum)
$missingMarkerEntries = @($missingMarkers.ToArray())
$duplicateScriptReferenceEntries = @($duplicateScriptReferences)
$scriptReferenceGroupEntries = @($scriptReferenceGroups)
$scriptReferenceExtensionEntries = @($scriptReferenceExtensions)

$summary = [ordered]@{
    summary_schema_version = 1
    schema = "featherdoc.script_task_index_check.v1"
    checker_name = "check_script_task_index.ps1"
    checked_at_utc = $checkedAtUtc
    powershell_version = $PSVersionTable.PSVersion.ToString()
    status = $status
    repo_root = $resolvedRepoRoot
    summary_json_path = $summaryJsonPath
    summary_json_relative_path = $summaryJsonRelativePath
    report_markdown_path = $reportMarkdownPath
    report_markdown_relative_path = $reportMarkdownRelativePath
    script_index_relative_path = $scriptIndexRelativePath
    total_script_reference_count = $allScriptReferences.Count
    script_reference_count = $scriptReferences.Count
    script_reference_group_count = $scriptReferenceGroups.Count
    script_reference_groups = $scriptReferenceGroupEntries
    script_reference_extension_count = $scriptReferenceExtensions.Count
    script_reference_extensions = $scriptReferenceExtensionEntries
    checked_scripts = @($checkedScripts)
    duplicate_script_reference_count = $duplicateScriptReferences.Count
    duplicate_script_references = $duplicateScriptReferenceEntries
    missing_script_count = $missingScripts.Count
    missing_scripts = @($missingScripts)
    required_marker_count = $requiredMarkerCount
    missing_marker_count = $missingMarkers.Count
    missing_markers = $missingMarkerEntries
}

Write-Utf8NoBomFile -Path $summaryJsonPath -Text (($summary | ConvertTo-Json -Depth 8) + [Environment]::NewLine)
Write-Utf8NoBomFile -Path $reportMarkdownPath -Text (New-MarkdownReport `
        -Summary ([pscustomobject]$summary) `
        -CheckedScripts $checkedScripts `
        -MissingScripts $missingScripts `
        -DuplicateScriptReferences $duplicateScriptReferenceEntries `
        -ScriptReferenceGroups $scriptReferenceGroupEntries `
        -ScriptReferenceExtensions $scriptReferenceExtensionEntries `
        -MissingMarkers $missingMarkerEntries)

if ($status -ne "passed") {
    throw "Script task index check failed. MissingScripts=$($missingScripts.Count); DuplicateScriptReferences=$($duplicateScriptReferences.Count); MissingMarkers=$($missingMarkers.Count)."
}

if (-not $Quiet) {
    Write-Host "Script task index check passed."
}
