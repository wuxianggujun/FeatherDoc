<#
.SYNOPSIS
Discovers repository DOCX/DOTX files that can be registered for project template smoke.

.DESCRIPTION
Scans the repository for committed Word template candidates, compares them with
the current project-template-smoke manifest, and prints either a readable report
or JSON. The script does not edit the manifest; it emits ready-to-run
register_project_template_smoke_manifest_entry.ps1 commands for unregistered
candidates.
#>
param(
    [string]$ManifestPath = "samples/project_template_smoke.manifest.json",
    [string]$SearchRoot = ".",
    [string]$BuildDir = "",
    [string]$OutputPath = "",
    [switch]$Json,
    [switch]$IncludeGenerated,
    [switch]$IncludeRegistered,
    [switch]$FailOnUnregistered
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "project_template_smoke_manifest_common.ps1")

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    return Resolve-ProjectTemplateSmokePath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing:$AllowMissing
}

function Get-RepoRelativeDisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Convert-ToSafeManifestName {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "template"
    }

    $name = [System.IO.Path]::GetFileNameWithoutExtension($Value)
    $sanitized = $name -replace '[\\/:*?"<>|]', '-' -replace '\s+', '-'
    $sanitized = $sanitized.Trim('-').ToLowerInvariant()
    if ([string]::IsNullOrWhiteSpace($sanitized)) {
        return "template"
    }

    return $sanitized
}

function Get-UniqueSuggestedName {
    param(
        [string]$BaseName,
        [string]$DisplayPath,
        [hashtable]$UsedNames
    )

    $candidate = $BaseName
    if ([string]::IsNullOrWhiteSpace($candidate)) {
        $candidate = "template"
    }

    if (-not $UsedNames.ContainsKey($candidate.ToLowerInvariant())) {
        $UsedNames[$candidate.ToLowerInvariant()] = $true
        return $candidate
    }

    $relative = $DisplayPath -replace '^[.][\\/]', ''
    $parent = Split-Path -Parent $relative
    $parentLeaf = if ([string]::IsNullOrWhiteSpace($parent)) {
        ""
    } else {
        Split-Path -Leaf $parent
    }
    if (-not [string]::IsNullOrWhiteSpace($parentLeaf)) {
        $candidate = Convert-ToSafeManifestName -Value ("$parentLeaf-$BaseName")
        if (-not $UsedNames.ContainsKey($candidate.ToLowerInvariant())) {
            $UsedNames[$candidate.ToLowerInvariant()] = $true
            return $candidate
        }
    }

    $suffix = 2
    do {
        $candidate = "$BaseName-$suffix"
        $suffix += 1
    } while ($UsedNames.ContainsKey($candidate.ToLowerInvariant()))

    $UsedNames[$candidate.ToLowerInvariant()] = $true
    return $candidate
}

function Test-IsGeneratedPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    $relative = (Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $Path) -replace '^[.][\\/]', ''
    $segments = @($relative -split '[\\/]')
    if ($segments.Count -eq 0) {
        return $false
    }

    $first = $segments[0].ToLowerInvariant()
    if ($first -eq ".git" -or $first -eq "output" -or $first -eq "build") {
        return $true
    }
    if ($first.StartsWith("build-")) {
        return $true
    }

    return $false
}

function Resolve-ManifestEntryInputPath {
    param(
        [string]$RepoRoot,
        [string]$ResolvedBuildDir,
        $Entry
    )

    $repoRelativePath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Entry -Name "input_docx"
    $buildRelativePath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Entry -Name "input_docx_build_relative"

    if (-not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        if ([string]::IsNullOrWhiteSpace($ResolvedBuildDir)) {
            return ""
        }

        return Resolve-RepoPath -RepoRoot $ResolvedBuildDir -InputPath $buildRelativePath -AllowMissing
    }

    if ([string]::IsNullOrWhiteSpace($repoRelativePath)) {
        return ""
    }

    return Resolve-RepoPath -RepoRoot $RepoRoot -InputPath $repoRelativePath -AllowMissing
}

function New-RegisterCommand {
    param(
        [string]$ManifestDisplayPath,
        [string]$InputDisplayPath,
        [string]$SuggestedName
    )

    $visualOutputDir = ".\output\project-template-smoke\$SuggestedName-visual"
    return 'pwsh -ExecutionPolicy Bypass -File .\scripts\register_project_template_smoke_manifest_entry.ps1 -Name "{0}" -ManifestPath "{1}" -InputDocx "{2}" -VisualSmokeOutputDir "{3}" -ReplaceExisting' -f `
        $SuggestedName,
        $ManifestDisplayPath,
        $InputDisplayPath,
        $visualOutputDir
}

function Get-TrackedWordTemplateFiles {
    param(
        [string]$RepoRoot,
        [string]$ResolvedSearchRoot
    )

    $relativeSearchRoot = Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $ResolvedSearchRoot
    $relativeSearchRoot = $relativeSearchRoot -replace '^[.][\\/]', ''
    if ($relativeSearchRoot -eq ".") {
        $relativeSearchRoot = ""
    }

    $trackedPaths = @(& git -C $RepoRoot ls-files "*.docx" "*.dotx" 2>$null)
    if ($LASTEXITCODE -ne 0) {
        return @(
            Get-ChildItem -LiteralPath $ResolvedSearchRoot -Recurse -File -Include *.docx, *.dotx |
                ForEach-Object { $_.FullName }
        )
    }

    return @(
        $trackedPaths |
            Where-Object {
                if ([string]::IsNullOrWhiteSpace($relativeSearchRoot)) {
                    return $true
                }

                $normalized = $_ -replace '/', '\'
                $prefix = $relativeSearchRoot -replace '/', '\'
                return ($normalized -eq $prefix -or $normalized.StartsWith($prefix.TrimEnd('\') + "\", [System.StringComparison]::OrdinalIgnoreCase))
            } |
            ForEach-Object { [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $_)) }
    )
}

$repoRoot = Resolve-RepoRoot
$resolvedManifestPath = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $ManifestPath
$resolvedSearchRoot = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SearchRoot
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
}

$manifest = Get-Content -Raw -LiteralPath $resolvedManifestPath | ConvertFrom-Json
$registeredLookup = @{}
$registeredEntries = New-Object 'System.Collections.Generic.List[object]'
$usedManifestNames = @{}
foreach ($entry in @(Get-ProjectTemplateSmokeArrayProperty -Object $manifest -Name "entries")) {
    $entryName = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "name"
    if (-not [string]::IsNullOrWhiteSpace($entryName)) {
        $usedManifestNames[$entryName.ToLowerInvariant()] = $true
    }
    $resolvedInputPath = Resolve-ManifestEntryInputPath -RepoRoot $repoRoot -ResolvedBuildDir $resolvedBuildDir -Entry $entry
    if (-not [string]::IsNullOrWhiteSpace($resolvedInputPath)) {
        $fullPath = [System.IO.Path]::GetFullPath($resolvedInputPath).ToLowerInvariant()
        if (-not $registeredLookup.ContainsKey($fullPath)) {
            $registeredLookup[$fullPath] = New-Object 'System.Collections.Generic.List[string]'
        }
        $registeredLookup[$fullPath].Add($entryName) | Out-Null
    }

    $registeredEntries.Add([pscustomobject]@{
        name = $entryName
        input_docx = $resolvedInputPath
        input_docx_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedInputPath
    }) | Out-Null
}

$candidateFiles = Get-TrackedWordTemplateFiles -RepoRoot $repoRoot -ResolvedSearchRoot $resolvedSearchRoot |
    ForEach-Object { Get-Item -LiteralPath $_ } |
    Where-Object {
        $includePath = $true
        if (-not $IncludeGenerated) {
            $includePath = -not (Test-IsGeneratedPath -RepoRoot $repoRoot -Path $_.FullName)
        }
        $includePath
    } |
    Sort-Object FullName

$manifestDisplayPath = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedManifestPath
$candidates = New-Object 'System.Collections.Generic.List[object]'
foreach ($file in $candidateFiles) {
    $fullPath = [System.IO.Path]::GetFullPath($file.FullName)
    $lookupKey = $fullPath.ToLowerInvariant()
    $registeredNames = if ($registeredLookup.ContainsKey($lookupKey)) {
        @($registeredLookup[$lookupKey].ToArray())
    } else {
        @()
    }
    $registered = @($registeredNames).Count -gt 0
    if ($registered -and -not $IncludeRegistered) {
        continue
    }

    $displayPath = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $fullPath
    $baseSuggestedName = Convert-ToSafeManifestName -Value $file.Name
    $suggestedName = if ($registered) {
        ""
    } else {
        Get-UniqueSuggestedName -BaseName $baseSuggestedName -DisplayPath $displayPath -UsedNames $usedManifestNames
    }
    $candidates.Add([pscustomobject]@{
        path = $fullPath
        display_path = $displayPath
        file_name = $file.Name
        extension = $file.Extension.ToLowerInvariant()
        size_bytes = $file.Length
        last_write_time = $file.LastWriteTime.ToString("s")
        registered = $registered
        registered_entry_names = [string[]]@($registeredNames)
        suggested_name = $suggestedName
        suggested_register_command = if ($registered) {
            ""
        } else {
            New-RegisterCommand `
                -ManifestDisplayPath $manifestDisplayPath `
                -InputDisplayPath $displayPath `
                -SuggestedName $suggestedName
        }
    }) | Out-Null
}

$report = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    manifest_path = $resolvedManifestPath
    manifest_path_display = $manifestDisplayPath
    search_root = $resolvedSearchRoot
    search_root_display = Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedSearchRoot
    include_generated = [bool]$IncludeGenerated
    include_registered = [bool]$IncludeRegistered
    fail_on_unregistered = [bool]$FailOnUnregistered
    registered_manifest_entry_count = $registeredEntries.Count
    candidate_count = $candidates.Count
    registered_candidate_count = @($candidates | Where-Object { $_.registered }).Count
    unregistered_candidate_count = @($candidates | Where-Object { -not $_.registered }).Count
    registered_entries = $registeredEntries.ToArray()
    candidates = $candidates.ToArray()
}

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputPath -AllowMissing
}

if ($Json) {
    $jsonText = $report | ConvertTo-Json -Depth 8
    if ([string]::IsNullOrWhiteSpace($resolvedOutputPath)) {
        Write-Output $jsonText
    } else {
        $directory = Split-Path -Parent $resolvedOutputPath
        if (-not [string]::IsNullOrWhiteSpace($directory)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
        }
        $jsonText | Set-Content -LiteralPath $resolvedOutputPath -Encoding UTF8
        Write-Host "[project-template-smoke-discover] Wrote JSON report to $resolvedOutputPath"
    }
    if ($FailOnUnregistered -and $report.unregistered_candidate_count -gt 0) {
        exit 1
    }

    exit 0
}

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("Project template smoke candidates")
[void]$lines.Add("Manifest: $manifestDisplayPath")
[void]$lines.Add("Search root: $(Get-RepoRelativeDisplayPath -RepoRoot $repoRoot -Path $resolvedSearchRoot)")
[void]$lines.Add("Registered manifest entries: $($report.registered_manifest_entry_count)")
[void]$lines.Add("Candidates shown: $($report.candidate_count)")
[void]$lines.Add("Unregistered candidates: $($report.unregistered_candidate_count)")
[void]$lines.Add("Fail on unregistered: $($report.fail_on_unregistered)")
[void]$lines.Add("")

foreach ($candidate in $candidates) {
    [void]$lines.Add("- $($candidate.display_path)")
    [void]$lines.Add("  registered: $($candidate.registered)")
    if ($candidate.registered) {
        [void]$lines.Add("  entry_names: $($candidate.registered_entry_names -join ', ')")
    } else {
        [void]$lines.Add("  suggested_name: $($candidate.suggested_name)")
        [void]$lines.Add("  register_command: $($candidate.suggested_register_command)")
    }
    [void]$lines.Add("")
}

if ($candidates.Count -eq 0) {
    [void]$lines.Add("- No candidates matched the current filters.")
}

$text = $lines -join [Environment]::NewLine
if ([string]::IsNullOrWhiteSpace($resolvedOutputPath)) {
    Write-Output $text
} else {
    $directory = Split-Path -Parent $resolvedOutputPath
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
    $text | Set-Content -LiteralPath $resolvedOutputPath -Encoding UTF8
    Write-Host "[project-template-smoke-discover] Wrote text report to $resolvedOutputPath"
}

if ($FailOnUnregistered -and $report.unregistered_candidate_count -gt 0) {
    exit 1
}
