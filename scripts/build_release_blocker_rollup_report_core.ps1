function Write-Step {
    param([string]$Message)
    Write-Host "[release-blocker-rollup] $Message"
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

function Write-Utf8NoBomFile {
    param([string]$Path, [AllowNull()][string]$Text)

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, [string]$Text, $encoding)
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
    $summaryJson = Convert-ReleaseMaterialString -RepoRoot $repoRoot -Text $summaryJson
    Write-Utf8NoBomFile -Path $SummaryPath -Text ($summaryJson + [Environment]::NewLine)

    $markdown = @(New-ReportMarkdown -Summary $Summary) -join [Environment]::NewLine
    $markdown = Convert-ReleaseMaterialString -RepoRoot $repoRoot -Text $markdown
    Write-Utf8NoBomFile -Path $MarkdownPath -Text ($markdown + [Environment]::NewLine)
}

function Resolve-RepoPathFromDisplayPath {
    param([string]$RepoRoot, [string]$DisplayPath)

    if ([string]::IsNullOrWhiteSpace($RepoRoot) -or [string]::IsNullOrWhiteSpace($DisplayPath)) {
        return ""
    }

    $normalized = $DisplayPath -replace '/', '\'
    if ($normalized.StartsWith('.\')) {
        $normalized = $normalized.Substring(2)
    }

    if ([System.IO.Path]::IsPathRooted($normalized)) {
        return [System.IO.Path]::GetFullPath($normalized)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $normalized))
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

function Get-FirstJsonString {
    param($Object, [string[]]$Names, [string]$DefaultValue = "")

    foreach ($name in @($Names)) {
        $value = Get-JsonString -Object $Object -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }
    return $DefaultValue
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
