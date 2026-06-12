function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $RepoRoot $Path
    }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }

    return $resolved
}

function Get-DisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $rootWithSeparator = $RepoRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    if ($fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        return ".\" + $fullPath.Substring($rootWithSeparator.Length)
    }

    return $fullPath
}

function Read-JsonFile {
    param([string]$Path)

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Write-Utf8NoBomFile {
    param(
        [string]$Path,
        [string]$Text
    )

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Get-OptionalPropertyValue {
    param($Object, [string]$Name)

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Test-IntegerEquals {
    param($Value, [int]$Expected)

    if ($null -eq $Value) {
        return $false
    }

    try {
        return ([int]$Value -eq $Expected)
    } catch {
        return $false
    }
}

function Test-IntegerAtLeast {
    param($Value, [int]$Minimum)

    if ($null -eq $Value) {
        return $false
    }

    try {
        return ([int]$Value -ge $Minimum)
    } catch {
        return $false
    }
}

function Convert-ToIntOrZero {
    param($Value)

    if ($null -eq $Value) {
        return 0
    }

    if ([string]::IsNullOrWhiteSpace([string]$Value)) {
        return 0
    }

    try {
        return [int]$Value
    } catch {
        return 0
    }
}

function Add-Check {
    param(
        [System.Collections.Generic.List[object]]$Checks,
        [string]$Name,
        [bool]$Passed,
        [string]$Message,
        [hashtable]$Details = @{}
    )

    $entry = [ordered]@{
        name = $Name
        status = if ($Passed) { "pass" } else { "fail" }
        required = $true
        message = $Message
    }

    if ($Details.Count -gt 0) {
        $entry.details = [ordered]@{}
        foreach ($key in ($Details.Keys | Sort-Object)) {
            $entry.details[$key] = $Details[$key]
        }
    }

    $Checks.Add([pscustomobject]$entry) | Out-Null
}

function Add-Warning {
    param(
        [System.Collections.Generic.List[object]]$Warnings,
        [string]$Id,
        [string]$Message,
        [hashtable]$Details = @{}
    )

    $entry = [ordered]@{
        id = $Id
        message = $Message
    }

    if ($Details.Count -gt 0) {
        $entry.details = [ordered]@{}
        foreach ($key in ($Details.Keys | Sort-Object)) {
            $entry.details[$key] = $Details[$key]
        }
    }

    $Warnings.Add([pscustomobject]$entry) | Out-Null
}

function Get-ManifestSamples {
    param($Manifest)

    $samples = Get-OptionalPropertyValue -Object $Manifest -Name "samples"
    if ($null -ne $samples) {
        return @($samples)
    }

    return @($Manifest)
}

function Resolve-ContactSheetPath {
    param(
        [string]$RepoRoot,
        $VisualSummary,
        [string]$VisualSummaryPath
    )

    $contactSheet = [string](Get-OptionalPropertyValue -Object $VisualSummary -Name "aggregate_contact_sheet")
    if ([string]::IsNullOrWhiteSpace($contactSheet)) {
        $reportDir = Split-Path -Parent $VisualSummaryPath
        return Join-Path $reportDir "aggregate-contact-sheet.png"
    }

    return Resolve-RepoPath -RepoRoot $RepoRoot -Path $contactSheet -AllowMissing
}
