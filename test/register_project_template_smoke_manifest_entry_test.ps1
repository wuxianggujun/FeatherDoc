param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\register_project_template_smoke_manifest_entry_test"
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)

    $dir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-RegisterScript {
    param([string[]]$Arguments)

    $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
    $powerShellPath = if ($powerShellCommand) { $powerShellCommand.Source } else { (Get-Process -Id $PID).Path }
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
        $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = (@($output | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
    }
}

function Get-ManifestEntry {
    param($Manifest, [string]$Name)

    return @($Manifest.entries | Where-Object { [string]$_.name -eq $Name }) | Select-Object -First 1
}

function Get-RepoRelativePortablePath {
    param([string]$RepoRoot, [string]$Path)

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    if (-not $resolvedRepoRoot.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $resolvedRepoRoot += [System.IO.Path]::DirectorySeparatorChar
    }

    $baseUri = [System.Uri]$resolvedRepoRoot
    $targetUri = [System.Uri]([System.IO.Path]::GetFullPath($Path))
    $relativePath = [System.Uri]::UnescapeDataString($baseUri.MakeRelativeUri($targetUri).ToString()) -replace '\\', '/'
    return "./$relativePath"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\register_project_template_smoke_manifest_entry.ps1"
$fixtureRoot = Join-Path $resolvedWorkingDir ("fixtures-" + [System.Guid]::NewGuid().ToString("N"))
$manifestPath = Join-Path $fixtureRoot "project_template_smoke.manifest.json"
$inputDocxPath = Join-Path $fixtureRoot "invoice.docx"
$schemaPath = Join-Path $fixtureRoot "invoice.schema.json"
$generatedSchemaPath = Join-Path $fixtureRoot "invoice.generated.schema.json"
$visualDir = Join-Path $fixtureRoot "visual-output"
$updatedVisualDir = Join-Path $fixtureRoot "updated-visual-output"
$entryJsonPath = Join-Path $fixtureRoot "entry-base.json"
$templateValidationsPath = Join-Path $fixtureRoot "template-validations.json"
$schemaTargetsPath = Join-Path $fixtureRoot "schema-targets.json"

New-Item -ItemType Directory -Path $fixtureRoot -Force | Out-Null
New-Item -ItemType Directory -Path $visualDir -Force | Out-Null
New-Item -ItemType Directory -Path $updatedVisualDir -Force | Out-Null
Set-Content -LiteralPath $inputDocxPath -Encoding UTF8 -Value "placeholder docx fixture"
Set-Content -LiteralPath $schemaPath -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath $generatedSchemaPath -Encoding UTF8 -Value "{}"

$expectedInputDocx = Get-RepoRelativePortablePath -RepoRoot $resolvedRepoRoot -Path $inputDocxPath
$expectedSchema = Get-RepoRelativePortablePath -RepoRoot $resolvedRepoRoot -Path $schemaPath
$expectedGeneratedSchema = Get-RepoRelativePortablePath -RepoRoot $resolvedRepoRoot -Path $generatedSchemaPath
$expectedVisualDir = Get-RepoRelativePortablePath -RepoRoot $resolvedRepoRoot -Path $visualDir
$expectedUpdatedVisualDir = Get-RepoRelativePortablePath -RepoRoot $resolvedRepoRoot -Path $updatedVisualDir

Write-JsonFile -Path $entryJsonPath -Value ([ordered]@{
    prepare_sample_target = "prepare_invoice_fixture"
    visual_smoke = [ordered]@{
        enabled = $false
        output_dir = "./old-visual-output"
    }
})

Write-JsonFile -Path $templateValidationsPath -Value @(
    [ordered]@{
        name = "body"
        part = "body"
        slots = @("customer_name:text", "line_items:table_rows")
    }
)

Write-JsonFile -Path $schemaTargetsPath -Value @(
    [ordered]@{
        part = "section-header"
        section = 0
        kind = "default"
        slots = @(
            [ordered]@{
                bookmark = "header_title"
                kind = "text"
                count = 1
            }
        )
    }
)

$registerResult = Invoke-RegisterScript -Arguments @(
    "-Name", "invoice-template",
    "-ManifestPath", $manifestPath,
    "-EntryJsonFile", $entryJsonPath,
    "-InputDocx", $inputDocxPath,
    "-TemplateValidationsFile", $templateValidationsPath,
    "-SchemaValidationFile", $schemaPath,
    "-SchemaValidationTargetsFile", $schemaTargetsPath,
    "-SchemaBaselineFile", $schemaPath,
    "-SchemaBaselineTargetMode", "section-targets",
    "-SchemaBaselineGeneratedOutput", $generatedSchemaPath,
    "-VisualSmokeOutputDir", $visualDir,
    "-CheckPaths"
)

Assert-Equal -Actual $registerResult.ExitCode -Expected 0 `
    -Message "Initial manifest registration should pass. Output: $($registerResult.Text)"
Assert-ContainsText -Text $registerResult.Text -ExpectedText "Manifest entry written" `
    -Message "Initial registration should announce the manifest write."
Assert-ContainsText -Text $registerResult.Text -ExpectedText "checks: template_validations, schema_validation, schema_baseline, visual_smoke" `
    -Message "Initial registration should summarize configured checks."

$manifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath | ConvertFrom-Json
Assert-Equal -Actual ([int]@($manifest.entries).Count) -Expected 1 `
    -Message "Initial registration should write one entry."
Assert-ContainsText -Text ([string]$manifest.'$schema') -ExpectedText "samples/project_template_smoke.manifest.schema.json" `
    -Message "New manifest should include a schema reference to the repository schema."

$entry = Get-ManifestEntry -Manifest $manifest -Name "invoice-template"
Assert-True -Condition ($null -ne $entry) `
    -Message "Registered manifest should include invoice-template."
Assert-Equal -Actual ([string]$entry.prepare_sample_target) -Expected "prepare_invoice_fixture" `
    -Message "EntryJsonFile base fields should be preserved."
Assert-Equal -Actual ([string]$entry.input_docx) -Expected $expectedInputDocx `
    -Message "InputDocx should be normalized relative to the repository."
Assert-Equal -Actual ([int]@($entry.template_validations).Count) -Expected 1 `
    -Message "Template validations file should be loaded."
Assert-Equal -Actual ([string]$entry.schema_validation.schema_file) -Expected $expectedSchema `
    -Message "Schema validation file should be normalized."
Assert-Equal -Actual ([int]@($entry.schema_validation.targets).Count) -Expected 1 `
    -Message "Schema validation targets file should be loaded."
Assert-Equal -Actual ([string]$entry.schema_baseline.schema_file) -Expected $expectedSchema `
    -Message "Schema baseline file should be normalized."
Assert-Equal -Actual ([string]$entry.schema_baseline.target_mode) -Expected "section-targets" `
    -Message "Schema baseline target mode should be written."
Assert-Equal -Actual ([string]$entry.schema_baseline.generated_output) -Expected $expectedGeneratedSchema `
    -Message "Schema baseline generated output should be normalized."
Assert-Equal -Actual ([bool]$entry.visual_smoke.enabled) -Expected $true `
    -Message "Visual smoke output should force visual smoke enabled."
Assert-Equal -Actual ([string]$entry.visual_smoke.output_dir) -Expected $expectedVisualDir `
    -Message "Visual smoke output dir should be normalized."

$duplicateResult = Invoke-RegisterScript -Arguments @(
    "-Name", "invoice-template",
    "-ManifestPath", $manifestPath,
    "-InputDocx", $inputDocxPath,
    "-VisualSmokeOutputDir", $visualDir
)
Assert-Equal -Actual $duplicateResult.ExitCode -Expected 1 `
    -Message "Duplicate registration without -ReplaceExisting should fail. Output: $($duplicateResult.Text)"
Assert-ContainsText -Text $duplicateResult.Text -ExpectedText "Manifest already contains an entry named 'invoice-template'" `
    -Message "Duplicate registration should explain how to update the entry."

$replaceResult = Invoke-RegisterScript -Arguments @(
    "-Name", "invoice-template",
    "-ManifestPath", $manifestPath,
    "-InputDocx", $inputDocxPath,
    "-VisualSmokeOutputDir", $updatedVisualDir,
    "-SchemaBaselineFile", $schemaPath,
    "-SchemaBaselineTargetMode", "resolved-section-targets",
    "-ReplaceExisting",
    "-CheckPaths"
)
Assert-Equal -Actual $replaceResult.ExitCode -Expected 0 `
    -Message "ReplaceExisting registration should pass. Output: $($replaceResult.Text)"

$updatedManifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath | ConvertFrom-Json
Assert-Equal -Actual ([int]@($updatedManifest.entries).Count) -Expected 1 `
    -Message "ReplaceExisting should update in place instead of appending."
$updatedEntry = Get-ManifestEntry -Manifest $updatedManifest -Name "invoice-template"
Assert-Equal -Actual ([string]$updatedEntry.schema_baseline.target_mode) -Expected "resolved-section-targets" `
    -Message "ReplaceExisting should update schema baseline target mode."
Assert-Equal -Actual ([string]$updatedEntry.visual_smoke.output_dir) -Expected $expectedUpdatedVisualDir `
    -Message "ReplaceExisting should update visual smoke output dir."
Assert-Equal -Actual ([string]$updatedEntry.prepare_sample_target) -Expected "prepare_invoice_fixture" `
    -Message "ReplaceExisting should preserve existing fields when not overridden."

Write-Host "Project template smoke manifest registration regression passed."
