<#
.SYNOPSIS
Checks every registered template schema baseline from a repository manifest.

.DESCRIPTION
Reads `baselines/template-schema/manifest.json`, resolves each entry relative to
the repository root, then runs `check_template_schema_baseline.ps1` for every
registered template baseline. The script returns `0` when every entry matches
and `1` when any entry drifts.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\check_template_schema_manifest.ps1 `
    -ManifestPath .\baselines\template-schema\manifest.json `
    -BuildDir build-codex-clang-column-visual-verify `
    -SkipBuild
#>
param(
    [string]$ManifestPath = "baselines/template-schema/manifest.json",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [string]$OutputDir = "output/template-schema-manifest-checks",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[template-schema-manifest] $Message"
}

function Resolve-TargetModeArgs {
    param([string]$TargetMode)

    switch ($TargetMode) {
        "" { return @() }
        "default" { return @() }
        "section-targets" { return @("-SectionTargets") }
        "resolved-section-targets" { return @("-ResolvedSectionTargets") }
        default { throw "Unsupported target_mode '$TargetMode'. Expected default, section-targets, or resolved-section-targets." }
    }
}

function Resolve-OptionalManifestPropertyValue {
    param(
        $Entry,
        [string]$Name
    )

    $property = $Entry.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Resolve-ManifestInputDocxPath {
    param(
        [string]$RepoRoot,
        [string]$ResolvedBuildDir,
        $Entry
    )

    $repoRelativePath = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "input_docx"
    $buildRelativePath = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "input_docx_build_relative"

    if (-not [string]::IsNullOrWhiteSpace($repoRelativePath) -and
        -not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        throw "Manifest entries must not set both 'input_docx' and 'input_docx_build_relative'."
    }

    if (-not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        if ([string]::IsNullOrWhiteSpace($ResolvedBuildDir)) {
            throw "Manifest entry '$([string]$Entry.name)' uses 'input_docx_build_relative' but no -BuildDir was provided."
        }

        return Resolve-TemplateSchemaPath -RepoRoot $ResolvedBuildDir -InputPath $buildRelativePath -AllowMissing
    }

    if ([string]::IsNullOrWhiteSpace($repoRelativePath)) {
        throw "Manifest entry '$([string]$Entry.name)' must set either 'input_docx' or 'input_docx_build_relative'."
    }

    return Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $repoRelativePath
}

function Invoke-ManifestSamplePreparation {
    param(
        [string]$Name,
        [string]$ResolvedBuildDir,
        [string]$TargetName,
        [string]$InputDocx
    )

    if ([string]::IsNullOrWhiteSpace($TargetName)) {
        return
    }

    if ([string]::IsNullOrWhiteSpace($ResolvedBuildDir)) {
        throw "Manifest entry '$Name' uses 'prepare_sample_target' but no -BuildDir was provided."
    }

    $samplePath = Find-TemplateSchemaTargetBinary -SearchRoot $ResolvedBuildDir -TargetName $TargetName
    if ([string]::IsNullOrWhiteSpace($samplePath)) {
        throw "Manifest entry '$Name' requested sample target '$TargetName' but no built executable was found under $ResolvedBuildDir."
    }

    $inputDirectory = Split-Path -Parent $InputDocx
    if (-not [string]::IsNullOrWhiteSpace($inputDirectory)) {
        New-Item -ItemType Directory -Path $inputDirectory -Force | Out-Null
    }

    Write-Step "Preparing fixture for '$Name' via sample target '$TargetName'"
    $prepareOutput = @(& $samplePath $InputDocx 2>&1)
    $prepareExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    foreach ($line in @($prepareOutput | ForEach-Object { $_.ToString() })) {
        Write-Host $line
    }

    if ($prepareExitCode -ne 0) {
        throw "Manifest entry '$Name' failed while preparing sample target '$TargetName' (exit code $prepareExitCode)."
    }

    if (-not (Test-Path -LiteralPath $InputDocx)) {
        throw "Manifest entry '$Name' ran sample target '$TargetName' but did not produce $InputDocx."
    }
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedManifestPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $ManifestPath
$resolvedOutputDir = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $OutputDir -AllowMissing
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    ""
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
}
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$manifest = Get-Content -Raw -LiteralPath $resolvedManifestPath | ConvertFrom-Json
$entries = @($manifest.entries)
if ($entries.Count -eq 0) {
    Write-Step "No template schema baseline entries are registered in $resolvedManifestPath"
    exit 0
}

$results = New-Object 'System.Collections.Generic.List[object]'
$hasDrift = $false

foreach ($entry in $entries) {
    $name = [string]$entry.name
    if ([string]::IsNullOrWhiteSpace($name)) {
        throw "Every manifest entry must provide a non-empty 'name'."
    }

    $inputDocx = Resolve-ManifestInputDocxPath -RepoRoot $repoRoot -ResolvedBuildDir $resolvedBuildDir -Entry $entry
    $prepareSampleTarget = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "prepare_sample_target"
    Invoke-ManifestSamplePreparation `
        -Name $name `
        -ResolvedBuildDir $resolvedBuildDir `
        -TargetName $prepareSampleTarget `
        -InputDocx $inputDocx
    $schemaFile = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath ([string]$entry.schema_file)
    $generatedOutput = if ([string]::IsNullOrWhiteSpace([string]$entry.generated_output)) {
        Join-Path $resolvedOutputDir ($name + ".generated.schema.json")
    } else {
        Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath ([string]$entry.generated_output) -AllowMissing
    }
    $targetModeArgs = Resolve-TargetModeArgs -TargetMode ([string]$entry.target_mode)

    $outputDirectory = [System.IO.Path]::GetDirectoryName($generatedOutput)
    if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
        New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
    }

    Write-Step "Checking baseline '$name'"
    $scriptArgs = @(
        "-InputDocx"
        $inputDocx
        "-SchemaFile"
        $schemaFile
        "-GeneratedSchemaOutput"
        $generatedOutput
        "-BuildDir"
        $BuildDir
    )
    if ($SkipBuild) {
        $scriptArgs += "-SkipBuild"
    }
    $scriptArgs += $targetModeArgs

    $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "check_template_schema_baseline.ps1") @scriptArgs 2>&1)
    $exitCode = $LASTEXITCODE
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    foreach ($line in $lines) {
        Write-Host $line
    }

    if ($exitCode -notin @(0, 1)) {
        throw "Baseline '$name' failed with unexpected exit code $exitCode."
    }

    $jsonLine = $lines | Where-Object { $_ -match '^\{"command":"check-template-schema",' } | Select-Object -Last 1
    if ([string]::IsNullOrWhiteSpace($jsonLine)) {
        throw "Baseline '$name' did not emit a check-template-schema JSON result."
    }

    $parsed = $jsonLine | ConvertFrom-Json
    $results.Add([pscustomobject]@{
        name = $name
        input_docx = $inputDocx
        matches = [bool]$parsed.matches
        schema_file = [string]$parsed.schema_file
        generated_output_path = [string]$parsed.generated_output_path
        added_target_count = [int]$parsed.added_target_count
        removed_target_count = [int]$parsed.removed_target_count
        changed_target_count = [int]$parsed.changed_target_count
    }) | Out-Null

    if ($exitCode -ne 0) {
        $hasDrift = $true
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    manifest_path = $resolvedManifestPath
    workspace = $repoRoot
    build_dir = $resolvedBuildDir
    entry_count = $results.Count
    drift_count = @($results | Where-Object { -not $_.matches }).Count
    passed = (-not $hasDrift)
    entries = $results
}

($summary | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Write-Step "Manifest summary: $summaryPath"

if ($hasDrift) {
    exit 1
}
