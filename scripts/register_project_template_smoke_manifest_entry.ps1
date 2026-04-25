<#
.SYNOPSIS
Registers or updates one project template smoke manifest entry.

.DESCRIPTION
Builds a `project_template_smoke` manifest entry from direct parameters and/or
JSON snippet files, validates the resulting manifest before writing it, then
adds or updates the named entry.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$Name,
    [string]$ManifestPath = "samples/project_template_smoke.manifest.json",
    [string]$InputDocx = "",
    [string]$InputDocxBuildRelative = "",
    [string]$PrepareSampleTarget = "",
    [string]$PrepareArgument = "",
    [string]$TemplateValidationsFile = "",
    [string]$SchemaValidationFile = "",
    [string]$SchemaValidationTargetsFile = "",
    [string]$SchemaBaselineFile = "",
    [ValidateSet("default", "section-targets", "resolved-section-targets")]
    [string]$SchemaBaselineTargetMode = "default",
    [string]$SchemaBaselineGeneratedOutput = "",
    [switch]$VisualSmoke,
    [string]$VisualSmokeOutputDir = "",
    [string]$EntryJsonFile = "",
    [string]$BuildDir = "",
    [switch]$ReplaceExisting,
    [switch]$CheckPaths
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "project_template_smoke_manifest_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-smoke-register] $Message"
}

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

function Convert-ToPortableRelativePath {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $resolvedBasePath = [System.IO.Path]::GetFullPath($BasePath)
    if (-not $resolvedBasePath.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $resolvedBasePath += [System.IO.Path]::DirectorySeparatorChar
    }
    $resolvedTargetPath = [System.IO.Path]::GetFullPath($TargetPath)

    $baseUri = [System.Uri]$resolvedBasePath
    $targetUri = [System.Uri]$resolvedTargetPath
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)
    $relativePath = [System.Uri]::UnescapeDataString($relativeUri.ToString()) -replace '\\', '/'

    if ($relativePath.StartsWith("../", [System.StringComparison]::Ordinal) -or
        $relativePath -eq "..") {
        throw "Target path '$resolvedTargetPath' is outside base path '$resolvedBasePath'."
    }

    if ($relativePath -notmatch '^(\./|\.\./)') {
        return "./$relativePath"
    }

    return $relativePath
}

function Convert-ToOrderedHashtable {
    param($Object)

    $result = [ordered]@{}
    if ($null -eq $Object) {
        return $result
    }

    if ($Object -is [System.Collections.IDictionary]) {
        foreach ($key in $Object.Keys) {
            $result[$key] = $Object[$key]
        }
        return $result
    }

    foreach ($property in $Object.PSObject.Properties) {
        $result[$property.Name] = $property.Value
    }

    return $result
}

function Read-JsonFile {
    param([string]$Path)

    return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
}

function Load-JsonArrayFile {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    $resolvedPath = Resolve-RepoPath -RepoRoot $RepoRoot -InputPath $Path
    return @(Read-JsonFile -Path $resolvedPath)
}

function Normalize-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $resolvedPath = Resolve-RepoPath -RepoRoot $RepoRoot -InputPath $Path -AllowMissing
    return Convert-ToPortableRelativePath -BasePath $RepoRoot -TargetPath $resolvedPath
}

function Normalize-BuildRelativePath {
    param(
        [string]$ResolvedBuildDir,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    if ([string]::IsNullOrWhiteSpace($ResolvedBuildDir)) {
        return ($Path -replace '\\', '/')
    }

    $resolvedPath = Resolve-RepoPath -RepoRoot $ResolvedBuildDir -InputPath $Path -AllowMissing
    return Convert-ToPortableRelativePath -BasePath $ResolvedBuildDir -TargetPath $resolvedPath
}

function Load-EntryBaseObject {
    param(
        [string]$RepoRoot,
        [string]$EntryJsonFile
    )

    if ([string]::IsNullOrWhiteSpace($EntryJsonFile)) {
        return [ordered]@{}
    }

    $resolvedEntryPath = Resolve-RepoPath -RepoRoot $RepoRoot -InputPath $EntryJsonFile
    return Convert-ToOrderedHashtable -Object (Read-JsonFile -Path $resolvedEntryPath)
}

function Get-ManifestSchemaReference {
    param(
        [string]$ManifestDirectory,
        [string]$SchemaPath
    )

    $resolvedBasePath = [System.IO.Path]::GetFullPath($ManifestDirectory)
    if (-not $resolvedBasePath.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $resolvedBasePath += [System.IO.Path]::DirectorySeparatorChar
    }
    $resolvedSchemaPath = [System.IO.Path]::GetFullPath($SchemaPath)

    $baseUri = [System.Uri]$resolvedBasePath
    $targetUri = [System.Uri]$resolvedSchemaPath
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)
    $relativePath = [System.Uri]::UnescapeDataString($relativeUri.ToString()) -replace '\\', '/'

    if ($relativePath -notmatch '^(\./|\.\./)') {
        return "./$relativePath"
    }

    return $relativePath
}

$repoRoot = Resolve-RepoRoot
$resolvedManifestPath = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $ManifestPath -AllowMissing
$manifestDirectory = Split-Path -Parent $resolvedManifestPath
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
}

if (-not [string]::IsNullOrWhiteSpace($InputDocx) -and -not [string]::IsNullOrWhiteSpace($InputDocxBuildRelative)) {
    throw "-InputDocx conflicts with -InputDocxBuildRelative."
}

if (-not [string]::IsNullOrWhiteSpace($InputDocxBuildRelative) -and [string]::IsNullOrWhiteSpace($resolvedBuildDir)) {
    throw "-InputDocxBuildRelative requires -BuildDir."
}

if (-not [string]::IsNullOrWhiteSpace($manifestDirectory)) {
    New-Item -ItemType Directory -Path $manifestDirectory -Force | Out-Null
}

$manifest = if (Test-Path -LiteralPath $resolvedManifestPath) {
    Read-JsonFile -Path $resolvedManifestPath
} else {
    [pscustomobject]@{ entries = @() }
}
$existingEntries = @(Get-ProjectTemplateSmokeArrayProperty -Object $manifest -Name "entries")
$existingEntry = $existingEntries | Where-Object {
    (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $_ -Name "name") -eq $Name
} | Select-Object -First 1

if ($null -ne $existingEntry -and -not $ReplaceExisting) {
    throw "Manifest already contains an entry named '$Name'. Re-run with -ReplaceExisting to update it."
}

$replacementEntry = if ($null -ne $existingEntry -and $ReplaceExisting) {
    Convert-ToOrderedHashtable -Object $existingEntry
} else {
    [ordered]@{}
}

$entryJsonBase = Load-EntryBaseObject -RepoRoot $repoRoot -EntryJsonFile $EntryJsonFile
foreach ($key in $entryJsonBase.Keys) {
    $replacementEntry[$key] = $entryJsonBase[$key]
}

$replacementEntry["name"] = $Name

if (-not [string]::IsNullOrWhiteSpace($InputDocx)) {
    $replacementEntry.Remove("input_docx_build_relative")
    $replacementEntry["input_docx"] = Normalize-RepoRelativePath -RepoRoot $repoRoot -Path $InputDocx
}

if (-not [string]::IsNullOrWhiteSpace($InputDocxBuildRelative)) {
    $replacementEntry.Remove("input_docx")
    $replacementEntry["input_docx_build_relative"] = Normalize-BuildRelativePath -ResolvedBuildDir $resolvedBuildDir -Path $InputDocxBuildRelative
}

if (-not [string]::IsNullOrWhiteSpace($PrepareSampleTarget)) {
    $replacementEntry["prepare_sample_target"] = $PrepareSampleTarget
}
if (-not [string]::IsNullOrWhiteSpace($PrepareArgument)) {
    $replacementEntry["prepare_argument"] = Normalize-RepoRelativePath -RepoRoot $repoRoot -Path $PrepareArgument
}

if (-not [string]::IsNullOrWhiteSpace($TemplateValidationsFile)) {
    $replacementEntry["template_validations"] = Load-JsonArrayFile -RepoRoot $repoRoot -Path $TemplateValidationsFile
}

if (-not [string]::IsNullOrWhiteSpace($SchemaValidationFile) -or
    -not [string]::IsNullOrWhiteSpace($SchemaValidationTargetsFile)) {
    $schemaValidation = Convert-ToOrderedHashtable -Object (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $replacementEntry -Name "schema_validation")
    if (-not [string]::IsNullOrWhiteSpace($SchemaValidationFile)) {
        $schemaValidation["schema_file"] = Normalize-RepoRelativePath -RepoRoot $repoRoot -Path $SchemaValidationFile
    }
    if (-not [string]::IsNullOrWhiteSpace($SchemaValidationTargetsFile)) {
        $schemaValidation["targets"] = Load-JsonArrayFile -RepoRoot $repoRoot -Path $SchemaValidationTargetsFile
    }
    $replacementEntry["schema_validation"] = [pscustomobject]$schemaValidation
}

if (-not [string]::IsNullOrWhiteSpace($SchemaBaselineFile) -or
    -not [string]::IsNullOrWhiteSpace($SchemaBaselineGeneratedOutput)) {
    $schemaBaseline = Convert-ToOrderedHashtable -Object (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $replacementEntry -Name "schema_baseline")
    if (-not [string]::IsNullOrWhiteSpace($SchemaBaselineFile)) {
        $schemaBaseline["schema_file"] = Normalize-RepoRelativePath -RepoRoot $repoRoot -Path $SchemaBaselineFile
    }
    $schemaBaseline["target_mode"] = $SchemaBaselineTargetMode
    if (-not [string]::IsNullOrWhiteSpace($SchemaBaselineGeneratedOutput)) {
        $schemaBaseline["generated_output"] = Normalize-RepoRelativePath -RepoRoot $repoRoot -Path $SchemaBaselineGeneratedOutput
    }
    $replacementEntry["schema_baseline"] = [pscustomobject]$schemaBaseline
}

if ($VisualSmoke.IsPresent -or -not [string]::IsNullOrWhiteSpace($VisualSmokeOutputDir)) {
    $visualSmokeEntry = Convert-ToOrderedHashtable -Object (Get-ProjectTemplateSmokeOptionalPropertyObject -Object $replacementEntry -Name "visual_smoke")
    $visualSmokeEntry["enabled"] = $true
    if (-not [string]::IsNullOrWhiteSpace($VisualSmokeOutputDir)) {
        $visualSmokeEntry["output_dir"] = Normalize-RepoRelativePath -RepoRoot $repoRoot -Path $VisualSmokeOutputDir
    }
    $replacementEntry["visual_smoke"] = [pscustomobject]$visualSmokeEntry
}

$schemaFilePath = Resolve-RepoPath -RepoRoot $repoRoot -InputPath "samples/project_template_smoke.manifest.schema.json"
$schemaReference = if ($manifest.PSObject.Properties["`$schema"] -and
    -not [string]::IsNullOrWhiteSpace([string]$manifest.PSObject.Properties["`$schema"].Value)) {
    [string]$manifest.PSObject.Properties["`$schema"].Value
} else {
    Get-ManifestSchemaReference -ManifestDirectory $manifestDirectory -SchemaPath $schemaFilePath
}

$updatedEntries = New-Object 'System.Collections.Generic.List[object]'
$foundExisting = $false
foreach ($existing in $existingEntries) {
    $existingName = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $existing -Name "name"
    if ($existingName -eq $Name) {
        $updatedEntries.Add([pscustomobject]$replacementEntry) | Out-Null
        $foundExisting = $true
        continue
    }

    $updatedEntries.Add($existing) | Out-Null
}

if (-not $foundExisting) {
    $updatedEntries.Add([pscustomobject]$replacementEntry) | Out-Null
}

$manifestToWrite = [ordered]@{
    '$schema' = $schemaReference
    entries = $updatedEntries.ToArray()
}

$temporaryManifestPath = Join-Path ([System.IO.Path]::GetTempPath()) ("project-template-smoke-manifest-" + [System.Guid]::NewGuid().ToString("N") + ".json")
try {
    ($manifestToWrite | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $temporaryManifestPath -Encoding UTF8
    $validation = Test-ProjectTemplateSmokeManifest `
        -RepoRoot $repoRoot `
        -ManifestPath $temporaryManifestPath `
        -BuildDir $resolvedBuildDir `
        -CheckPaths:$CheckPaths
    if (-not $validation.passed) {
        $errorLines = New-Object 'System.Collections.Generic.List[string]'
        $errorLines.Add("Project template smoke manifest validation failed:") | Out-Null
        foreach ($issue in $validation.errors) {
            $errorLines.Add("- $($issue.path): $($issue.message)") | Out-Null
        }
        throw ($errorLines -join [System.Environment]::NewLine)
    }
} finally {
    if (Test-Path -LiteralPath $temporaryManifestPath) {
        Remove-Item -LiteralPath $temporaryManifestPath -Force -ErrorAction SilentlyContinue
    }
}

($manifestToWrite | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedManifestPath -Encoding UTF8

$configuredChecks = New-Object 'System.Collections.Generic.List[string]'
if ($replacementEntry.Contains("template_validations")) {
    $configuredChecks.Add("template_validations") | Out-Null
}
if ($replacementEntry.Contains("schema_validation")) {
    $configuredChecks.Add("schema_validation") | Out-Null
}
if ($replacementEntry.Contains("schema_baseline")) {
    $configuredChecks.Add("schema_baseline") | Out-Null
}
if ($replacementEntry.Contains("visual_smoke")) {
    $configuredChecks.Add("visual_smoke") | Out-Null
}

Write-Step "Manifest entry written to $resolvedManifestPath"
Write-Host "name: $Name"
Write-Host "checks: $($configuredChecks -join ', ')"
Write-Host "manifest_path: $resolvedManifestPath"
