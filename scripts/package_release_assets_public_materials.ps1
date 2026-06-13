function Get-AssetDescriptor {
    param(
        [string]$Path,
        [string]$Label
    )

    $item = Get-Item -LiteralPath $Path
    $hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()

    return [ordered]@{
        label = $Label
        path = $item.FullName
        size_bytes = $item.Length
        sha256 = $hash
    }
}

function Get-TextLikeFiles {
    param([string[]]$RootPaths)

    $textExtensions = @(
        ".md",
        ".txt",
        ".json",
        ".xml",
        ".yml",
        ".yaml",
        ".csv",
        ".log",
        ".rst"
    )

    $results = [System.Collections.Generic.List[string]]::new()
    foreach ($rootPath in $RootPaths) {
        if ([string]::IsNullOrWhiteSpace($rootPath) -or -not (Test-Path -LiteralPath $rootPath)) {
            continue
        }

        foreach ($file in Get-ChildItem -LiteralPath $rootPath -Recurse -File) {
            if ($textExtensions -contains $file.Extension.ToLowerInvariant()) {
                [void]$results.Add($file.FullName)
            }
        }
    }

    return @($results | Sort-Object -Unique)
}

function Convert-RepoPathToRelative {
    param(
        [string]$Value,
        [string]$RepoRoot
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }

    $normalizedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $normalizedValue = $Value -replace '/', '\'
    try {
        if (-not [System.IO.Path]::IsPathRooted($normalizedValue)) {
            return $Value
        }

        $resolvedValue = [System.IO.Path]::GetFullPath($normalizedValue)
    } catch [System.ArgumentException] {
        return $Value
    } catch [System.NotSupportedException] {
        return $Value
    }

    if (-not $resolvedValue.StartsWith($normalizedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $Value
    }

    $relative = $resolvedValue.Substring($normalizedRepoRoot.Length).TrimStart('\', '/')
    if ([string]::IsNullOrWhiteSpace($relative)) {
        return "."
    }

    return ".\" + ($relative -replace '/', '\')
}

function Convert-EvidencePathToPublicDisplay {
    param(
        [string]$Value,
        [string]$RepoRoot,
        [switch]$PreferEvidenceAnchor
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }

    $normalized = $Value -replace '/', '\'
    if ($PreferEvidenceAnchor) {
        foreach ($anchor in @(
                "\output\",
                "\release-assets\",
                "\release-assets-ci\",
                "\build-msvc-install\",
                "\build-msvc-install"
            )) {
            $index = $normalized.LastIndexOf($anchor, [System.StringComparison]::OrdinalIgnoreCase)
            if ($index -ge 0) {
                $relative = $normalized.Substring($index + 1)
                if (-not [string]::IsNullOrWhiteSpace($relative)) {
                    return ".\" + $relative
                }
            }
        }
    }

    $repoDisplay = Convert-RepoPathToRelative -Value $Value -RepoRoot $RepoRoot
    if ($repoDisplay -ne $Value) {
        return $repoDisplay
    }

    foreach ($anchor in @(
            "\output\",
            "\release-assets\",
            "\release-assets-ci\",
            "\build-msvc-install\",
            "\build-msvc-install"
        )) {
        $index = $normalized.LastIndexOf($anchor, [System.StringComparison]::OrdinalIgnoreCase)
        if ($index -ge 0) {
            $relative = $normalized.Substring($index + 1)
            if (-not [string]::IsNullOrWhiteSpace($relative)) {
                return ".\" + $relative
            }
        }
    }

    return $Value
}

function Get-EvidenceObjectProperty {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return $Object[$Name]
        }

        return $null
    }

    return Get-OptionalPropertyObject -Object $Object -Name $Name
}

function Set-EvidenceObjectProperty {
    param(
        $Object,
        [string]$Name,
        $Value
    )

    if ($null -eq $Object) {
        return
    }

    if ($Object -is [System.Collections.IDictionary]) {
        $Object[$Name] = $Value
        return
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        $Object | Add-Member -NotePropertyName $Name -NotePropertyValue $Value
        return
    }

    $property.Value = $Value
}

function Convert-EvidenceEntrypointsToPublicDisplay {
    param(
        $Contract,
        $SourceContract = $null,
        [string]$RepoRoot
    )

    if ($null -eq $Contract) {
        return
    }

    $source = if ($null -ne $SourceContract) { $SourceContract } else { $Contract }
    $sourceEntrypoints = @(Get-EvidenceObjectProperty -Object $source -Name "entrypoints")
    $targetEntrypoints = @(Get-EvidenceObjectProperty -Object $Contract -Name "entrypoints")
    $targetById = @{}
    foreach ($targetEntrypoint in $targetEntrypoints) {
        $targetId = [string](Get-EvidenceObjectProperty -Object $targetEntrypoint -Name "id")
        if (-not [string]::IsNullOrWhiteSpace($targetId)) {
            $targetById[$targetId] = $targetEntrypoint
        }
    }

    $convertedEntrypoints = New-Object 'System.Collections.Generic.List[object]'
    $index = 0
    foreach ($sourceEntrypoint in $sourceEntrypoints) {
        if ($null -eq $sourceEntrypoint) {
            $index++
            continue
        }

        $sourceId = [string](Get-EvidenceObjectProperty -Object $sourceEntrypoint -Name "id")
        $targetEntrypoint = if (-not [string]::IsNullOrWhiteSpace($sourceId) -and $targetById.ContainsKey($sourceId)) {
            $targetById[$sourceId]
        } elseif ($index -lt $targetEntrypoints.Count) {
            $targetEntrypoints[$index]
        } else {
            $null
        }

        if ($null -eq $targetEntrypoint) {
            $targetEntrypoint = $sourceEntrypoint
        }

        $publicEntrypoint = [ordered]@{}

        foreach ($fieldName in @("id", "required", "location")) {
            $fieldValue = Get-EvidenceObjectProperty -Object $sourceEntrypoint -Name $fieldName
            if ($null -eq $fieldValue) {
                $fieldValue = Get-EvidenceObjectProperty -Object $targetEntrypoint -Name $fieldName
            }
            if ($null -ne $fieldValue) {
                $publicEntrypoint[$fieldName] = $fieldValue
            }
        }

        $path = [string](Get-EvidenceObjectProperty -Object $sourceEntrypoint -Name "path")
        if ([string]::IsNullOrWhiteSpace($path)) {
            $path = [string](Get-EvidenceObjectProperty -Object $targetEntrypoint -Name "path")
        }
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            $publicEntrypoint["path"] = Convert-EvidencePathToPublicDisplay -Value $path -RepoRoot $RepoRoot
        }

        $pathDisplay = [string](Get-EvidenceObjectProperty -Object $sourceEntrypoint -Name "path_display")
        if ([string]::IsNullOrWhiteSpace($pathDisplay)) {
            $pathDisplay = [string](Get-EvidenceObjectProperty -Object $targetEntrypoint -Name "path_display")
        }
        if ([string]::IsNullOrWhiteSpace($pathDisplay)) {
            $pathDisplay = [string](Get-EvidenceObjectProperty -Object $sourceEntrypoint -Name "path")
        }
        if ([string]::IsNullOrWhiteSpace($pathDisplay)) {
            $pathDisplay = [string](Get-EvidenceObjectProperty -Object $targetEntrypoint -Name "path")
        }
        if (-not [string]::IsNullOrWhiteSpace($pathDisplay)) {
            $publicEntrypoint["path_display"] = Convert-EvidencePathToPublicDisplay -Value $pathDisplay -RepoRoot $RepoRoot -PreferEvidenceAnchor
        }

        $convertedEntrypoints.Add($publicEntrypoint) | Out-Null
        $index++
    }

    if ($convertedEntrypoints.Count -gt 0) {
        Set-EvidenceObjectProperty -Object $Contract -Name "entrypoints" -Value @($convertedEntrypoints.ToArray())
    }
}

function Convert-ReleaseTextToPublic {
    param(
        [string]$Value,
        [string]$RepoRoot
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }

    $result = $Value
    $result = $result.Replace($RepoRoot, ".")
    $result = $result.Replace(($RepoRoot -replace '\\', '/'), ".")
    $result = [regex]::Replace($result, '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|]+', '<windows-absolute-path>')
    $result = [regex]::Replace($result, '(?<!\w)/(?:Users|home)/[^\s"''`<>|]+', '<unix-absolute-path>')

    $zhReleaseNoteDraft = [string]::Concat(([char[]](0x53D1, 0x5E03, 0x8BF4, 0x660E, 0x8349, 0x7A3F)))
    $zhReleaseNotePreview = [string]::Concat(([char[]](0x53D1, 0x5E03, 0x8BF4, 0x660E, 0x9884, 0x89C8, 0x7248)))
    $zhFillBeforeRelease = [string]::Concat(([char[]](0x8BF7, 0x5728, 0x53D1, 0x5E03, 0x524D, 0x8865, 0x9F50)))
    $zhFillBeforePublicRelease = [string]::Concat(([char[]](0x8BF7, 0x5728, 0x516C, 0x5F00, 0x53D1, 0x5E03, 0x524D, 0x5B8C, 0x5584)))
    $zhDraft = [string]::Concat(([char[]](0x8349, 0x7A3F)))
    $zhPreview = [string]::Concat(([char[]](0x9884, 0x89C8, 0x7248)))

    $replacements = @(
        @{ Pattern = "(?i)$zhReleaseNoteDraft"; Replacement = $zhReleaseNotePreview }
        @{ Pattern = "(?i)$zhFillBeforeRelease"; Replacement = $zhFillBeforePublicRelease }
        @{ Pattern = '(?i)\brelease body draft\b'; Replacement = 'release body preview' }
        @{ Pattern = '(?i)\brelease-note drafts\b'; Replacement = 'release-note previews' }
        @{ Pattern = '(?i)\bpublic release drafts\b'; Replacement = 'public release previews' }
        @{ Pattern = '(?i)\brelease drafts\b'; Replacement = 'release previews' }
        @{ Pattern = '(?i)\bdraft review\b'; Replacement = 'release-note review' }
        @{ Pattern = '(?i)\bdraft boilerplate\b'; Replacement = 'placeholder boilerplate' }
        @{ Pattern = '(?i)\brelease-note drafting\b'; Replacement = 'release-note preparation' }
        @{ Pattern = '(?i)\bdraft=false\b'; Replacement = 'public-release-enabled' }
        @{ Pattern = '(?i)\bdrafts\b'; Replacement = 'previews' }
        @{ Pattern = '(?i)\bdrafting\b'; Replacement = 'preparation' }
        @{ Pattern = '(?i)\bdraft\b'; Replacement = 'preview' }
        @{ Pattern = $zhDraft; Replacement = $zhPreview }
    )

    foreach ($replacement in $replacements) {
        $result = [regex]::Replace($result, $replacement.Pattern, $replacement.Replacement)
    }

    return $result
}

function Convert-StructuredValueToPublic {
    param(
        $Value,
        [string]$RepoRoot,
        [switch]$PreferEvidenceAnchor
    )

    if ($null -eq $Value) {
        return $null
    }

    if ($Value -is [string]) {
        if ($PreferEvidenceAnchor) {
            $displayValue = Convert-EvidencePathToPublicDisplay `
                -Value $Value `
                -RepoRoot $RepoRoot `
                -PreferEvidenceAnchor
            if ($displayValue -ne $Value) {
                return Convert-ReleaseTextToPublic -Value $displayValue -RepoRoot $RepoRoot
            }
        }

        $relativeValue = Convert-RepoPathToRelative -Value $Value -RepoRoot $RepoRoot
        if (-not $PreferEvidenceAnchor -and
            $relativeValue -ne $Value -and
            $relativeValue -match '^\.[\\/]build[^\\/]*[\\/]test[\\/]') {
            if ([System.IO.Path]::DirectorySeparatorChar -eq '\') {
                return "<windows-absolute-path>"
            }

            return "<unix-absolute-path>"
        }

        return Convert-ReleaseTextToPublic -Value $relativeValue -RepoRoot $RepoRoot
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    if ($Value -is [System.Collections.IDictionary]) {
        $result = [ordered]@{}
        foreach ($key in $Value.Keys) {
            $result[$key] = Convert-StructuredValueToPublic `
                -Value $Value[$key] `
                -RepoRoot $RepoRoot `
                -PreferEvidenceAnchor:$PreferEvidenceAnchor
        }
        return $result
    }

    if ($Value -is [pscustomobject]) {
        $result = [ordered]@{}
        foreach ($property in $Value.PSObject.Properties) {
            $result[$property.Name] = Convert-StructuredValueToPublic `
                -Value $property.Value `
                -RepoRoot $RepoRoot `
                -PreferEvidenceAnchor:$PreferEvidenceAnchor
        }
        return $result
    }

    if ($Value -is [System.Collections.IEnumerable] -and -not ($Value -is [string])) {
        $result = @()
        foreach ($item in $Value) {
            $result += ,(Convert-StructuredValueToPublic `
                    -Value $item `
                    -RepoRoot $RepoRoot `
                    -PreferEvidenceAnchor:$PreferEvidenceAnchor)
        }
        return $result
    }

    return $Value
}

function Sanitize-StagedReleaseMaterials {
    param(
        [string]$RepoRoot,
        [string[]]$RootPaths
    )

    foreach ($filePath in Get-TextLikeFiles -RootPaths $RootPaths) {
        if ([System.IO.Path]::GetExtension($filePath).Equals(".json", [System.StringComparison]::OrdinalIgnoreCase)) {
            $content = Get-Content -Raw -LiteralPath $filePath | ConvertFrom-Json
            $sanitized = Convert-StructuredValueToPublic -Value $content -RepoRoot $RepoRoot
            ($sanitized | ConvertTo-Json -Depth 100) | Set-Content -LiteralPath $filePath -Encoding UTF8
            continue
        }

        $content = Get-Content -Raw -LiteralPath $filePath
        $content = Convert-ReleaseTextToPublic -Value $content -RepoRoot $RepoRoot
        Set-Content -LiteralPath $filePath -Encoding UTF8 -Value $content
    }
}
