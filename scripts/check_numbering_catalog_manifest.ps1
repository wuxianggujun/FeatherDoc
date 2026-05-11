<#
.SYNOPSIS
Checks every registered numbering catalog baseline from a repository manifest.

.DESCRIPTION
Reads `baselines/numbering-catalog/manifest.json`, resolves each entry relative
to the repository root, then runs `check_numbering_catalog_baseline.ps1` for
every registered numbering catalog baseline. The script returns `0` only when
every entry matches and every committed catalog baseline is lint-clean; it
returns `1` on catalog drift or baseline lint failures.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_manifest.ps1 `
    -ManifestPath .\baselines\numbering-catalog\manifest.json `
    -BuildDir build-codex-clang-compat `
    -OutputDir .\output\numbering-catalog-manifest-checks `
    -SkipBuild
#>
param(
    [string]$ManifestPath = "baselines/numbering-catalog/manifest.json",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [string]$OutputDir = "output/numbering-catalog-manifest-checks",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[numbering-catalog-manifest] $Message"
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
        return "numbering-catalog-baseline"
    }

    return $sanitized
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
    Write-Step "No numbering catalog baseline entries are registered in $resolvedManifestPath"
    exit 0
}

$results = New-Object 'System.Collections.Generic.List[object]'
$hasDrift = $false
$hasDirtyBaseline = $false
$hasCheckIssues = $false
$powerShellPath = (Get-Process -Id $PID).Path
$baselineScriptPath = Join-Path $PSScriptRoot "check_numbering_catalog_baseline.ps1"

foreach ($entry in $entries) {
    $name = [string]$entry.name
    if ([string]::IsNullOrWhiteSpace($name)) {
        throw "Every manifest entry must provide a non-empty 'name'."
    }

    $inputDocx = Resolve-ManifestInputDocxPath -RepoRoot $repoRoot -ResolvedBuildDir $resolvedBuildDir -Entry $entry
    $catalogFileValue = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "catalog_file"
    if ([string]::IsNullOrWhiteSpace($catalogFileValue)) {
        throw "Manifest entry '$name' must set 'catalog_file'."
    }
    $catalogFile = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $catalogFileValue
    $generatedOutputValue = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "generated_output"
    $generatedOutput = if ([string]::IsNullOrWhiteSpace($generatedOutputValue)) {
        Join-Path $resolvedOutputDir ((Get-ManifestArtifactStem -Name $name) + ".generated.numbering-catalog.json")
    } else {
        Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $generatedOutputValue -AllowMissing
    }

    $outputDirectory = [System.IO.Path]::GetDirectoryName($generatedOutput)
    if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
        New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
    }

    Write-Step "Checking baseline '$name'"
    $scriptArgs = @(
        "-InputDocx", $inputDocx,
        "-CatalogFile", $catalogFile,
        "-GeneratedCatalogOutput", $generatedOutput,
        "-BuildDir", $BuildDir
    )
    if ($SkipBuild) {
        $scriptArgs += "-SkipBuild"
    }

    $commandOutput = @(& $powerShellPath -NoProfile -ExecutionPolicy Bypass -File $baselineScriptPath @scriptArgs 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    foreach ($line in $lines) {
        Write-Host $line
    }

    if ($exitCode -notin @(0, 1)) {
        throw "Baseline '$name' failed with unexpected exit code $exitCode."
    }

    $lintParsed = Get-TemplateSchemaCommandJsonObject `
        -Lines $lines `
        -Command "lint-numbering-catalog"
    $checkParsed = Get-TemplateSchemaCommandJsonObject `
        -Lines $lines `
        -Command "check-numbering-catalog"

    $results.Add([pscustomobject]@{
        name = $name
        input_docx = $inputDocx
        matches = [bool]$checkParsed.matches
        clean = [bool]$checkParsed.clean
        catalog_file = [string]$checkParsed.catalog_file
        catalog_lint_clean = [bool]$lintParsed.clean
        catalog_lint_issue_count = [int]$lintParsed.issue_count
        generated_output_path = [string]$checkParsed.generated_output_path
        baseline_issue_count = [int]$checkParsed.baseline_issue_count
        generated_issue_count = [int]$checkParsed.generated_issue_count
        added_definition_count = [int]$checkParsed.added_definition_count
        removed_definition_count = [int]$checkParsed.removed_definition_count
        changed_definition_count = [int]$checkParsed.changed_definition_count
    }) | Out-Null

    if (-not [bool]$checkParsed.matches) {
        $hasDrift = $true
    }
    if (-not [bool]$lintParsed.clean) {
        $hasDirtyBaseline = $true
    }
    if (-not [bool]$checkParsed.clean) {
        $hasCheckIssues = $true
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
    dirty_baseline_count = @($results | Where-Object { -not $_.catalog_lint_clean }).Count
    issue_entry_count = @($results | Where-Object { -not $_.clean }).Count
    passed = (-not $hasDrift) -and (-not $hasDirtyBaseline) -and (-not $hasCheckIssues)
    entries = $results
}

($summary | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Write-Step "Manifest summary: $summaryPath"

if ($hasDrift -or $hasDirtyBaseline -or $hasCheckIssues) {
    exit 1
}
