<#
.SYNOPSIS
Checks every registered template schema baseline from a repository manifest.

.DESCRIPTION
Reads `baselines/template-schema/manifest.json`, resolves each entry relative to
the repository root, then runs `check_template_schema_baseline.ps1` for every
registered template baseline. The script returns `0` only when every entry
matches and every committed schema baseline is lint-clean; it returns `1` on
schema drift or baseline lint failures.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\check_template_schema_manifest.ps1 `
    -ManifestPath .\baselines\template-schema\manifest.json `
    -BuildDir build-codex-clang-column-visual-verify `
    -RepairedSchemaOutputDir .\output\template-schema-manifest-repairs `
    -SkipBuild
#>
param(
    [string]$ManifestPath = "baselines/template-schema/manifest.json",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [string]$OutputDir = "output/template-schema-manifest-checks",
    [string]$RepairedSchemaOutputDir = "",
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

function Get-ManifestArtifactStem {
    param([string]$Name)

    $invalidFileNameChars = [System.IO.Path]::GetInvalidFileNameChars()
    $sanitizedChars = foreach ($char in $Name.ToCharArray()) {
        if ($invalidFileNameChars -contains $char) {
            '-'
        } else {
            $char
        }
    }

    $sanitized = (-join $sanitizedChars).Trim().Trim('.')
    if ([string]::IsNullOrWhiteSpace($sanitized)) {
        return "template-schema-baseline"
    }

    return $sanitized
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
$resolvedRepairedSchemaOutputDir = if ([string]::IsNullOrWhiteSpace($RepairedSchemaOutputDir)) {
    ""
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $RepairedSchemaOutputDir -AllowMissing
}
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
if (-not [string]::IsNullOrWhiteSpace($resolvedRepairedSchemaOutputDir)) {
    New-Item -ItemType Directory -Path $resolvedRepairedSchemaOutputDir -Force | Out-Null
}

$manifest = Get-Content -Raw -LiteralPath $resolvedManifestPath | ConvertFrom-Json
$entries = @($manifest.entries)
if ($entries.Count -eq 0) {
    Write-Step "No template schema baseline entries are registered in $resolvedManifestPath"
    exit 0
}

$results = New-Object 'System.Collections.Generic.List[object]'
$hasDrift = $false
$hasDirtyBaseline = $false

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

    $repairedOutput = if ([string]::IsNullOrWhiteSpace($resolvedRepairedSchemaOutputDir)) {
        ""
    } else {
        Join-Path $resolvedRepairedSchemaOutputDir ((Get-ManifestArtifactStem -Name $name) + ".repaired.schema.json")
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
    if (-not [string]::IsNullOrWhiteSpace($repairedOutput)) {
        $scriptArgs += @(
            "-RepairedSchemaOutput"
            $repairedOutput
        )
    }
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

    $lintParsed = Get-TemplateSchemaCommandJsonObject `
        -Lines $lines `
        -Command "lint-template-schema"
    $checkParsed = Get-TemplateSchemaCommandJsonObject `
        -Lines $lines `
        -Command "check-template-schema"
    $repairParsed = $null
    if (-not [bool]$lintParsed.clean -and -not [string]::IsNullOrWhiteSpace($repairedOutput)) {
        $repairParsed = Get-TemplateSchemaCommandJsonObject `
            -Lines $lines `
            -Command "repair-template-schema"
    }
    $results.Add([pscustomobject]@{
        name = $name
        input_docx = $inputDocx
        matches = [bool]$checkParsed.matches
        schema_file = [string]$checkParsed.schema_file
        schema_lint_clean = [bool]$lintParsed.clean
        schema_lint_issue_count = [int]$lintParsed.issue_count
        schema_lint_duplicate_target_count = [int]$lintParsed.duplicate_target_count
        schema_lint_duplicate_slot_count = [int]$lintParsed.duplicate_slot_count
        schema_lint_target_order_issue_count = [int]$lintParsed.target_order_issue_count
        schema_lint_slot_order_issue_count = [int]$lintParsed.slot_order_issue_count
        schema_lint_entry_name_issue_count = [int]$lintParsed.entry_name_issue_count
        generated_output_path = [string]$checkParsed.generated_output_path
        repaired_schema_output_path = if ($repairParsed) { [string]$repairParsed.output_path } else { "" }
        added_target_count = [int]$checkParsed.added_target_count
        removed_target_count = [int]$checkParsed.removed_target_count
        changed_target_count = [int]$checkParsed.changed_target_count
    }) | Out-Null

    if (-not [bool]$checkParsed.matches) {
        $hasDrift = $true
    }
    if (-not [bool]$lintParsed.clean) {
        $hasDirtyBaseline = $true
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    manifest_path = $resolvedManifestPath
    workspace = $repoRoot
    build_dir = $resolvedBuildDir
    repaired_schema_output_dir = $resolvedRepairedSchemaOutputDir
    entry_count = $results.Count
    drift_count = @($results | Where-Object { -not $_.matches }).Count
    dirty_baseline_count = @($results | Where-Object { -not $_.schema_lint_clean }).Count
    passed = (-not $hasDrift) -and (-not $hasDirtyBaseline)
    entries = $results
}

($summary | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Write-Step "Manifest summary: $summaryPath"

if ($hasDrift -or $hasDirtyBaseline) {
    exit 1
}
