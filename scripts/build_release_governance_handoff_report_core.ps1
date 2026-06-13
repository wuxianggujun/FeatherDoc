function Write-Step {
    param([string]$Message)
    Write-Host "[release-governance-handoff] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$Path, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }
    return $resolved
}

function Ensure-Directory {
    param([string]$Path)
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Get-DisplayPath {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $root = [System.IO.Path]::GetFullPath($RepoRoot)
    if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $root.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $root += [System.IO.Path]::DirectorySeparatorChar
    }
    $resolved = [System.IO.Path]::GetFullPath($Path)
    if ($resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolved.Substring($root.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
        return ".\" + ($relative -replace '/', '\')
    }
    return $resolved
}

function Convert-ReleaseMaterialString {
    param(
        [string]$RepoRoot,
        [AllowNull()][string]$Text
    )

    if ($null -eq $Text -or [string]::IsNullOrWhiteSpace($RepoRoot)) {
        return $Text
    }

    $normalized = [string]$Text
    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $repoRootForms = @(
        $resolvedRepoRoot,
        ($resolvedRepoRoot -replace '\\', '/'),
        ($resolvedRepoRoot -replace '\\', '\\')
    ) | Sort-Object -Unique

    foreach ($rootForm in $repoRootForms) {
        if ([string]::IsNullOrWhiteSpace($rootForm)) {
            continue
        }

        $pattern = [regex]::Escape($rootForm) + '(?<suffix>\\\\|[\\/]|(?=$|[\s"''`<>|;,)]+))'
        $normalized = [regex]::Replace(
            $normalized,
            $pattern,
            '.${suffix}',
            [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    }

    return $normalized.Replace('./', '.\')
}

function Write-ReleaseMaterialFiles {
    param(
        [AllowNull()]$Summary,
        [string]$SummaryPath,
        [string]$MarkdownPath,
        [int]$JsonDepth
    )

    $summaryJson = $Summary | ConvertTo-Json -Depth $JsonDepth
    (Convert-ReleaseMaterialString -RepoRoot $repoRoot -Text $summaryJson) |
        Set-Content -LiteralPath $SummaryPath -Encoding UTF8

    $markdown = @(New-ReportMarkdown -Summary $Summary) -join [Environment]::NewLine
    (Convert-ReleaseMaterialString -RepoRoot $repoRoot -Text $markdown) |
        Set-Content -LiteralPath $MarkdownPath -Encoding UTF8
}

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) { return $Object[$Name] }
        return $null
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Convert-JsonScalarToString {
    param($Value)

    if ($null -eq $Value) {
        return ""
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    return [string]$Value
}

function Get-JsonString {
    param($Object, [string]$Name, [string]$DefaultValue = "")

    $value = Get-JsonProperty -Object $Object -Name $Name
    $text = Convert-JsonScalarToString -Value $value
    if ([string]::IsNullOrWhiteSpace($text)) {
        return $DefaultValue
    }
    return $text
}

function Get-JsonBool {
    param($Object, [string]$Name, [bool]$DefaultValue = $false)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    if ($value -is [bool]) { return [bool]$value }
    $parsed = $false
    if ([bool]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $DefaultValue
}

function Get-JsonInt {
    param($Object, [string]$Name, [int]$DefaultValue = 0)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    $parsed = 0
    if ([int]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $DefaultValue
}

function Get-SummaryGroupMarkdownText {
    param(
        [object[]]$Items,
        [string]$NameProperty = "source_schema",
        [string]$CountProperty = "count"
    )

    $parts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($item in @($Items)) {
        $name = Get-JsonString -Object $item -Name $NameProperty
        if ([string]::IsNullOrWhiteSpace($name)) {
            continue
        }

        $count = Get-JsonString -Object $item -Name $CountProperty
        if ([string]::IsNullOrWhiteSpace($count)) {
            $count = "0"
        }

        [void]$parts.Add("${name}=${count}")
    }

    if ($parts.Count -eq 0) {
        return "(none)"
    }

    return ($parts.ToArray() -join ", ")
}

function Test-InformationalActionItem {
    param($Item)

    $category = Get-JsonString -Object $Item -Name "category"
    if ($category -in @("release_checklist", "manual_release_checklist", "informational")) {
        return $true
    }

    if (Get-JsonBool -Object $Item -Name "optional") {
        return $true
    }

    $releaseBlockingProperty = Get-JsonProperty -Object $Item -Name "release_blocking"
    if ($null -ne $releaseBlockingProperty -and -not (Get-JsonBool -Object $Item -Name "release_blocking" -DefaultValue $true)) {
        return $true
    }

    $id = Get-JsonString -Object $Item -Name "id"
    $action = Get-JsonString -Object $Item -Name "action"
    return ($id -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline") -or
        $action -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline"))
}

function Copy-ActionItemWithReleaseChecklistDefaults {
    param($Item)

    $copy = [ordered]@{}
    if ($Item -is [System.Collections.IDictionary]) {
        foreach ($key in @($Item.Keys)) {
            $copy[[string]$key] = $Item[$key]
        }
    } else {
        foreach ($property in @($Item.PSObject.Properties)) {
            $copy[$property.Name] = $property.Value
        }
    }

    if ([string]::IsNullOrWhiteSpace((Get-JsonString -Object $copy -Name "category"))) {
        $copy["category"] = "release_checklist"
    }
    if ([string]::IsNullOrWhiteSpace((Get-JsonString -Object $copy -Name "severity"))) {
        $copy["severity"] = "info"
    }
    $copy["release_blocking"] = $false
    $copy["optional"] = $true

    return $copy
}

function Get-JsonArray {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Get-FirstJsonProperty {
    param($Object, [string[]]$Names)

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Object -Name $name
        if ($null -eq $value) { continue }
        if ($value -is [string] -and [string]::IsNullOrWhiteSpace($value)) { continue }
        return $value
    }

    return $null
}
