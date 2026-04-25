<#
.SYNOPSIS
Freezes a template schema baseline and registers or updates a manifest entry.

.DESCRIPTION
Resolves one repository DOCX or one build-relative generated fixture, optionally
prepares the fixture by running a sample target, freezes a normalized schema
baseline, verifies that the committed schema is lint-clean and matches the
DOCX, then writes or updates the matching entry in
`baselines/template-schema/manifest.json`.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\register_template_schema_manifest_entry.ps1 `
    -Name chinese-invoice-template `
    -InputDocx .\samples\chinese_invoice_template.docx

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\register_template_schema_manifest_entry.ps1 `
    -Name fill-bookmarks-visual-baseline `
    -InputDocxBuildRelative output\template-schema-manifest\fill_bookmarks_visual_baseline.docx `
    -PrepareSampleTarget featherdoc_sample_fill_bookmarks_visual `
    -BuildDir build-codex-clang-column-visual-verify
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$Name,
    [string]$ManifestPath = "baselines/template-schema/manifest.json",
    [string]$InputDocx = "",
    [string]$InputDocxBuildRelative = "",
    [string]$PrepareSampleTarget = "",
    [string]$SchemaFile = "",
    [ValidateSet("default", "section-targets", "resolved-section-targets")]
    [string]$TargetMode = "default",
    [string]$GeneratedOutput = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [switch]$SkipBuild,
    [switch]$SkipFreeze,
    [switch]$SkipBaselineCheck,
    [switch]$ReplaceExisting
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[template-schema-register] $Message"
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

    return $relativePath
}

function Get-DefaultTemplateSchemaFileStem {
    param([string]$Name)

    $trimmed = $Name.Trim()
    if ([string]::IsNullOrWhiteSpace($trimmed)) {
        throw "Entry name must not be empty."
    }

    return (($trimmed -replace '[^A-Za-z0-9._-]+', '-') -replace '-{2,}', '-').Trim('-')
}

function Resolve-InputSelection {
    param(
        [string]$RepoRoot,
        [string]$ResolvedBuildDir,
        [string]$InputDocx,
        [string]$InputDocxBuildRelative
    )

    if ([string]::IsNullOrWhiteSpace($InputDocx) -and [string]::IsNullOrWhiteSpace($InputDocxBuildRelative)) {
        throw "Provide either -InputDocx or -InputDocxBuildRelative."
    }

    if (-not [string]::IsNullOrWhiteSpace($InputDocx) -and
        -not [string]::IsNullOrWhiteSpace($InputDocxBuildRelative)) {
        throw "-InputDocx conflicts with -InputDocxBuildRelative."
    }

    if (-not [string]::IsNullOrWhiteSpace($InputDocxBuildRelative)) {
        if ([string]::IsNullOrWhiteSpace($ResolvedBuildDir)) {
            throw "-InputDocxBuildRelative requires -BuildDir."
        }

        return [pscustomobject]@{
            source_type = "build-relative-fixture"
            resolved_input_docx = Resolve-TemplateSchemaPath -RepoRoot $ResolvedBuildDir -InputPath $InputDocxBuildRelative -AllowMissing
            manifest_input_docx = ""
            manifest_input_docx_build_relative = $InputDocxBuildRelative -replace '\\', '/'
        }
    }

    $resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $InputDocx
    return [pscustomobject]@{
        source_type = "repository-docx"
        resolved_input_docx = $resolvedInputDocx
        manifest_input_docx = Convert-ToPortableRelativePath -BasePath $RepoRoot -TargetPath $resolvedInputDocx
        manifest_input_docx_build_relative = ""
    }
}

function Ensure-SampleFixture {
    param(
        [string]$RepoRoot,
        [string]$ResolvedBuildDir,
        [string]$Generator,
        [string]$TargetName,
        [string]$ResolvedInputDocx,
        [switch]$SkipBuild
    )

    if ([string]::IsNullOrWhiteSpace($TargetName)) {
        return
    }

    if ([string]::IsNullOrWhiteSpace($ResolvedBuildDir)) {
        throw "-PrepareSampleTarget requires -BuildDir."
    }

    $targetBinary = Find-TemplateSchemaTargetBinary -SearchRoot $ResolvedBuildDir -TargetName $TargetName
    if (-not $targetBinary) {
        if ($SkipBuild) {
            throw "Could not find built sample target '$TargetName' under $ResolvedBuildDir."
        }

        $vcvarsPath = Get-TemplateSchemaVcvarsPath
        Write-Step "Configuring build tree for sample target '$TargetName'"
        Invoke-TemplateSchemaMsvcCommand -VcvarsPath $vcvarsPath -CommandText (
            "cmake -S `"$RepoRoot`" -B `"$ResolvedBuildDir`" -G `"$Generator`" " +
            "-DBUILD_CLI=ON -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON")
        Write-Step "Building sample target '$TargetName'"
        Invoke-TemplateSchemaMsvcCommand -VcvarsPath $vcvarsPath -CommandText (
            "cmake --build `"$ResolvedBuildDir`" --target $TargetName")

        $targetBinary = Find-TemplateSchemaTargetBinary -SearchRoot $ResolvedBuildDir -TargetName $TargetName
        if (-not $targetBinary) {
            throw "Built sample target '$TargetName' but could not locate its executable under $ResolvedBuildDir."
        }
    }

    $inputDirectory = [System.IO.Path]::GetDirectoryName($ResolvedInputDocx)
    if (-not [string]::IsNullOrWhiteSpace($inputDirectory)) {
        New-Item -ItemType Directory -Path $inputDirectory -Force | Out-Null
    }

    Write-Step "Preparing fixture via sample target '$TargetName'"
    $prepareOutput = @(& $targetBinary $ResolvedInputDocx 2>&1)
    $prepareExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    foreach ($line in @($prepareOutput | ForEach-Object { $_.ToString() })) {
        Write-Host $line
    }

    if ($prepareExitCode -ne 0) {
        throw "Sample target '$TargetName' failed with exit code $prepareExitCode."
    }

    if (-not (Test-Path -LiteralPath $ResolvedInputDocx)) {
        throw "Sample target '$TargetName' did not produce $ResolvedInputDocx."
    }
}

function Resolve-TargetModeArgs {
    param([string]$TargetMode)

    switch ($TargetMode) {
        "default" { return @() }
        "section-targets" { return @("-SectionTargets") }
        "resolved-section-targets" { return @("-ResolvedSectionTargets") }
        default { throw "Unsupported target mode '$TargetMode'." }
    }
}

function Invoke-BaselineGate {
    param(
        [string]$InputDocx,
        [string]$SchemaFile,
        [string]$GeneratedOutput,
        [string]$RepairedOutput,
        [string]$BuildDir,
        [string]$TargetMode,
        [switch]$SkipBuild
    )

    $scriptArgs = @(
        "-InputDocx"
        $InputDocx
        "-SchemaFile"
        $SchemaFile
        "-GeneratedSchemaOutput"
        $GeneratedOutput
        "-RepairedSchemaOutput"
        $RepairedOutput
        "-BuildDir"
        $BuildDir
    )
    if ($SkipBuild) {
        $scriptArgs += "-SkipBuild"
    }
    $scriptArgs += @(Resolve-TargetModeArgs -TargetMode $TargetMode)

    Write-Step "Verifying frozen schema baseline before manifest write"
    $checkOutput = @(& powershell.exe -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "check_template_schema_baseline.ps1") @scriptArgs 2>&1)
    $checkExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $checkLines = @($checkOutput | ForEach-Object { $_.ToString() })
    foreach ($line in $checkLines) {
        Write-Host $line
    }

    if ($checkExitCode -notin @(0, 1)) {
        throw "check_template_schema_baseline.ps1 failed with unexpected exit code $checkExitCode."
    }

    $lintSummary = Get-TemplateSchemaCommandJsonObject `
        -Lines $checkLines `
        -Command "lint-template-schema"
    $checkSummary = Get-TemplateSchemaCommandJsonObject `
        -Lines $checkLines `
        -Command "check-template-schema"

    if ($checkExitCode -ne 0) {
        if (-not [bool]$lintSummary.clean -and -not [bool]$checkSummary.matches) {
            throw "Refusing to write manifest entry because the schema baseline has lint issues and drift."
        }
        if (-not [bool]$lintSummary.clean) {
            throw "Refusing to write manifest entry because the schema baseline has lint issues."
        }
        if (-not [bool]$checkSummary.matches) {
            throw "Refusing to write manifest entry because the schema baseline drifts from the DOCX."
        }
        throw "Refusing to write manifest entry because the schema baseline check failed."
    }

    return [pscustomobject]@{
        Lint = $lintSummary
        Check = $checkSummary
    }
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedManifestPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $ManifestPath -AllowMissing
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    ""
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
}

$inputSelection = Resolve-InputSelection `
    -RepoRoot $repoRoot `
    -ResolvedBuildDir $resolvedBuildDir `
    -InputDocx $InputDocx `
    -InputDocxBuildRelative $InputDocxBuildRelative

if (-not [string]::IsNullOrWhiteSpace($PrepareSampleTarget) -and
    $inputSelection.source_type -ne "build-relative-fixture") {
    throw "-PrepareSampleTarget only applies to -InputDocxBuildRelative entries."
}

Ensure-SampleFixture `
    -RepoRoot $repoRoot `
    -ResolvedBuildDir $resolvedBuildDir `
    -Generator $Generator `
    -TargetName $PrepareSampleTarget `
    -ResolvedInputDocx $inputSelection.resolved_input_docx `
    -SkipBuild:$SkipBuild

if (-not (Test-Path -LiteralPath $inputSelection.resolved_input_docx)) {
    throw "Input DOCX does not exist: $($inputSelection.resolved_input_docx)"
}

$fileStem = Get-DefaultTemplateSchemaFileStem -Name $Name
$resolvedSchemaFile = if ([string]::IsNullOrWhiteSpace($SchemaFile)) {
    Join-Path $repoRoot ("baselines/template-schema/{0}.schema.json" -f $fileStem)
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $SchemaFile -AllowMissing
}
$resolvedGeneratedOutput = if ([string]::IsNullOrWhiteSpace($GeneratedOutput)) {
    Join-Path $repoRoot ("output/template-schema-manifest-checks/{0}.generated.schema.json" -f $fileStem)
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $GeneratedOutput -AllowMissing
}
$generatedOutputDirectory = [System.IO.Path]::GetDirectoryName($resolvedGeneratedOutput)
$resolvedRepairedOutput = if ([string]::IsNullOrWhiteSpace($generatedOutputDirectory)) {
    Join-Path $repoRoot ("output/template-schema-manifest-checks/{0}.repaired.schema.json" -f $fileStem)
} else {
    Join-Path $generatedOutputDirectory ("{0}.repaired.schema.json" -f $fileStem)
}

if (-not $SkipFreeze) {
    Write-Step "Freezing schema baseline"
    $freezeArgs = @(
        "-InputDocx"
        $inputSelection.resolved_input_docx
        "-SchemaOutput"
        $resolvedSchemaFile
        "-BuildDir"
        $BuildDir
    )
    if ($SkipBuild) {
        $freezeArgs += "-SkipBuild"
    }
    if ($TargetMode -eq "section-targets") {
        $freezeArgs += "-SectionTargets"
    } elseif ($TargetMode -eq "resolved-section-targets") {
        $freezeArgs += "-ResolvedSectionTargets"
    }

    $freezeOutput = @(& powershell.exe -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "freeze_template_schema_baseline.ps1") @freezeArgs 2>&1)
    $freezeExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    foreach ($line in @($freezeOutput | ForEach-Object { $_.ToString() })) {
        Write-Host $line
    }
    if ($freezeExitCode -ne 0) {
        throw "freeze_template_schema_baseline.ps1 failed with exit code $freezeExitCode."
    }
}

if (-not (Test-Path -LiteralPath $resolvedSchemaFile)) {
    throw "Schema file does not exist after registration flow: $resolvedSchemaFile"
}

$baselineGate = $null
if (-not $SkipBaselineCheck) {
    $baselineGate = Invoke-BaselineGate `
        -InputDocx $inputSelection.resolved_input_docx `
        -SchemaFile $resolvedSchemaFile `
        -GeneratedOutput $resolvedGeneratedOutput `
        -RepairedOutput $resolvedRepairedOutput `
        -BuildDir $BuildDir `
        -TargetMode $TargetMode `
        -SkipBuild:$SkipBuild
}

$manifestDirectory = Split-Path -Parent $resolvedManifestPath
if (-not [string]::IsNullOrWhiteSpace($manifestDirectory)) {
    New-Item -ItemType Directory -Path $manifestDirectory -Force | Out-Null
}

$manifest = if (Test-Path -LiteralPath $resolvedManifestPath) {
    Get-Content -Raw -LiteralPath $resolvedManifestPath | ConvertFrom-Json
} else {
    [pscustomobject]@{ entries = @() }
}
$existingEntries = @($manifest.entries)
$replacementEntry = [ordered]@{
    name = $Name
}
if ($inputSelection.source_type -eq "repository-docx") {
    $replacementEntry.input_docx = $inputSelection.manifest_input_docx
} else {
    $replacementEntry.input_docx_build_relative = $inputSelection.manifest_input_docx_build_relative
    if (-not [string]::IsNullOrWhiteSpace($PrepareSampleTarget)) {
        $replacementEntry.prepare_sample_target = $PrepareSampleTarget
    }
}
$replacementEntry.schema_file = Convert-ToPortableRelativePath -BasePath $repoRoot -TargetPath $resolvedSchemaFile
$replacementEntry.target_mode = $TargetMode
$replacementEntry.generated_output = Convert-ToPortableRelativePath -BasePath $repoRoot -TargetPath $resolvedGeneratedOutput

$updatedEntries = New-Object 'System.Collections.Generic.List[object]'
$foundExisting = $false
foreach ($existingEntry in $existingEntries) {
    $existingNameProperty = $existingEntry.PSObject.Properties["name"]
    $existingName = if ($null -eq $existingNameProperty -or $null -eq $existingNameProperty.Value) {
        ""
    } else {
        [string]$existingNameProperty.Value
    }

    if ($existingName -eq $Name) {
        if (-not $ReplaceExisting) {
            throw "Manifest already contains an entry named '$Name'. Re-run with -ReplaceExisting to overwrite it."
        }

        $updatedEntries.Add([pscustomobject]$replacementEntry) | Out-Null
        $foundExisting = $true
        continue
    }

    $updatedEntries.Add($existingEntry) | Out-Null
}

if (-not $foundExisting) {
    $updatedEntries.Add([pscustomobject]$replacementEntry) | Out-Null
}

$manifestToWrite = [ordered]@{
    entries = $updatedEntries.ToArray()
}
($manifestToWrite | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $resolvedManifestPath -Encoding UTF8

Write-Step "Manifest entry written to $resolvedManifestPath"
Write-Host "name: $Name"
Write-Host "source_type: $($inputSelection.source_type)"
Write-Host "resolved_input_docx: $($inputSelection.resolved_input_docx)"
Write-Host "schema_file: $resolvedSchemaFile"
Write-Host "target_mode: $TargetMode"
Write-Host "generated_output: $resolvedGeneratedOutput"
if ($baselineGate) {
    Write-Host "baseline_check_matches: $($baselineGate.Check.matches)"
    Write-Host "baseline_schema_lint_clean: $($baselineGate.Lint.clean)"
    Write-Host "baseline_schema_lint_issue_count: $($baselineGate.Lint.issue_count)"
}
Write-Host "manifest_path: $resolvedManifestPath"
