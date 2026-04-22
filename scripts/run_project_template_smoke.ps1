<#
.SYNOPSIS
Runs project-level template smoke checks from a manifest.

.DESCRIPTION
Reads a manifest of template fixtures, optionally prepares those fixtures via
built sample targets, then runs any combination of `validate-template`,
`validate-template-schema`, `check_template_schema_baseline.ps1`, and
`run_word_visual_smoke.ps1`.
#>
param(
    [string]$ManifestPath = "samples/project_template_smoke.manifest.json",
    [string]$BuildDir = "build-project-template-smoke-nmake",
    [string]$Generator = "NMake Makefiles",
    [string]$OutputDir = "output/project-template-smoke",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisualSmoke,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-smoke] $Message"
}

function Resolve-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Convert-ToSafeFileName {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "entry"
    }

    $sanitized = $Value -replace '[\\/:*?"<>|]', '-' -replace '\s+', '-'
    $sanitized = $sanitized.Trim('-')
    if ([string]::IsNullOrWhiteSpace($sanitized)) {
        return "entry"
    }

    return $sanitized.ToLowerInvariant()
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

function Get-ManifestArrayProperty {
    param(
        $Entry,
        [string]$Name
    )

    $property = $Entry.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return @()
    }

    return @($property.Value | Where-Object { $null -ne $_ })
}

function Get-OptionalObjectPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return ""
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name) -and $null -ne $Object[$Name]) {
            return [string]$Object[$Name]
        }

        return ""
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Resolve-ManifestPathValue {
    param(
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }

    return Resolve-TemplateSchemaPath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing:$AllowMissing
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
            throw "Entry '$([string]$Entry.name)' uses 'input_docx_build_relative' but no -BuildDir was provided."
        }

        return Resolve-TemplateSchemaPath -RepoRoot $ResolvedBuildDir -InputPath $buildRelativePath -AllowMissing
    }

    if ([string]::IsNullOrWhiteSpace($repoRelativePath)) {
        throw "Entry '$([string]$Entry.name)' must set either 'input_docx' or 'input_docx_build_relative'."
    }

    return Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $repoRelativePath -AllowMissing
}

function Resolve-PrepareArgumentPath {
    param(
        [string]$RepoRoot,
        [string]$ResolvedInputDocx,
        $Entry
    )

    $prepareArgument = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "prepare_argument"
    if ([string]::IsNullOrWhiteSpace($prepareArgument)) {
        return $ResolvedInputDocx
    }

    return Resolve-ManifestPathValue -RepoRoot $RepoRoot -InputPath $prepareArgument -AllowMissing
}

function Ensure-PathParent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $extension = [System.IO.Path]::GetExtension($Path)
    if ([string]::IsNullOrWhiteSpace($extension)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
}

function Convert-SlotSpecToCliString {
    param($SlotSpec)

    if ($SlotSpec -is [string]) {
        return $SlotSpec
    }

    $name = Resolve-OptionalManifestPropertyValue -Entry $SlotSpec -Name "bookmark"
    if ([string]::IsNullOrWhiteSpace($name)) {
        $name = Resolve-OptionalManifestPropertyValue -Entry $SlotSpec -Name "name"
    }
    $kind = Resolve-OptionalManifestPropertyValue -Entry $SlotSpec -Name "kind"

    if ([string]::IsNullOrWhiteSpace($name) -or [string]::IsNullOrWhiteSpace($kind)) {
        throw "Structured slot objects must provide 'bookmark'/'name' and 'kind'."
    }

    $segments = New-Object 'System.Collections.Generic.List[string]'
    $segments.Add($name)
    $segments.Add($kind)

    $requiredProperty = $SlotSpec.PSObject.Properties["required"]
    if ($null -ne $requiredProperty -and $null -ne $requiredProperty.Value) {
        $segments.Add((if ([bool]$requiredProperty.Value) { "required" } else { "optional" }))
    } else {
        $optionalProperty = $SlotSpec.PSObject.Properties["optional"]
        if ($null -ne $optionalProperty -and $null -ne $optionalProperty.Value) {
            $segments.Add((if ([bool]$optionalProperty.Value) { "optional" } else { "required" }))
        }
    }

    foreach ($occurrenceName in @("count", "min", "max")) {
        $property = $SlotSpec.PSObject.Properties[$occurrenceName]
        if ($null -ne $property -and $null -ne $property.Value -and
            -not [string]::IsNullOrWhiteSpace([string]$property.Value)) {
            $segments.Add("$occurrenceName=$([string]$property.Value)")
        }
    }

    return ($segments -join ":")
}

function Add-PartSelectionArguments {
    param(
        [string[]]$Arguments,
        [string]$PartSwitch,
        $Selection
    )

    $part = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "part"
    if ([string]::IsNullOrWhiteSpace($part)) {
        throw "Every selection block must provide a non-empty 'part'."
    }

    $result = @($Arguments + @($PartSwitch, $part))

    $index = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "index"
    if (-not [string]::IsNullOrWhiteSpace($index)) {
        $result += @("--index", $index)
    }

    $section = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "section"
    if (-not [string]::IsNullOrWhiteSpace($section)) {
        $result += @("--section", $section)
    }

    $kind = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "kind"
    if (-not [string]::IsNullOrWhiteSpace($kind)) {
        $result += @("--kind", $kind)
    }

    return $result
}

function Write-CommandOutput {
    param(
        [string]$OutputPath,
        [string[]]$Lines
    )

    if ([string]::IsNullOrWhiteSpace($OutputPath)) {
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($OutputPath)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }

    $Lines | Set-Content -LiteralPath $OutputPath -Encoding UTF8
}

function Invoke-CliJsonCommand {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments,
        [string]$OutputPath,
        [string]$FailureLabel
    )

    $commandResult = Invoke-TemplateSchemaCli -ExecutablePath $ExecutablePath -Arguments $Arguments
    Write-CommandOutput -OutputPath $OutputPath -Lines $commandResult.Output

    if ($commandResult.ExitCode -ne 0) {
        $lines = if ($commandResult.Output.Count -gt 0) {
            ($commandResult.Output -join [System.Environment]::NewLine)
        } else {
            "(no output)"
        }
        throw "$FailureLabel failed with exit code $($commandResult.ExitCode). Output:`n$lines"
    }

    $jsonText = $commandResult.Text.Trim()
    if ([string]::IsNullOrWhiteSpace($jsonText)) {
        throw "$FailureLabel did not emit JSON output."
    }

    try {
        $parsed = $jsonText | ConvertFrom-Json
    } catch {
        throw "$FailureLabel emitted invalid JSON: $jsonText"
    }

    return [pscustomobject]@{
        ExitCode = $commandResult.ExitCode
        Text = $jsonText
        Output = $commandResult.Output
        Json = $parsed
    }
}

function Parse-CheckTemplateSchemaJsonLine {
    param([string[]]$Lines)

    $jsonLine = $Lines |
        Where-Object { $_ -match '^\{"command":"check-template-schema",' } |
        Select-Object -Last 1

    if ([string]::IsNullOrWhiteSpace($jsonLine)) {
        throw "check-template-schema wrapper did not emit a JSON result."
    }

    return $jsonLine | ConvertFrom-Json
}

function Invoke-BaselineCheck {
    param(
        [string]$ScriptPath,
        [string]$InputDocx,
        [string]$SchemaFile,
        [string]$GeneratedSchemaOutput,
        [string]$BuildDir,
        [string]$TargetMode,
        [bool]$SkipBuildRequested,
        [string]$LogPath
    )

    $scriptArgs = @(
        "-InputDocx"
        $InputDocx
        "-SchemaFile"
        $SchemaFile
        "-GeneratedSchemaOutput"
        $GeneratedSchemaOutput
        "-BuildDir"
        $BuildDir
    )
    if ($SkipBuildRequested) {
        $scriptArgs += "-SkipBuild"
    }

    switch ($TargetMode) {
        "" { }
        "default" { }
        "section-targets" { $scriptArgs += "-SectionTargets" }
        "resolved-section-targets" { $scriptArgs += "-ResolvedSectionTargets" }
        default { throw "Unsupported schema baseline target_mode '$TargetMode'." }
    }

    $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @scriptArgs 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    Write-CommandOutput -OutputPath $LogPath -Lines $lines

    if ($exitCode -notin @(0, 1)) {
        $joined = if ($lines.Count -gt 0) {
            $lines -join [System.Environment]::NewLine
        } else {
            "(no output)"
        }
        throw "check_template_schema_baseline.ps1 failed with exit code $exitCode. Output:`n$joined"
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Json = Parse-CheckTemplateSchemaJsonLine -Lines $lines
    }
}

function Invoke-VisualSmoke {
    param(
        [string]$ScriptPath,
        [string]$InputDocx,
        [string]$OutputDir,
        [int]$Dpi,
        [bool]$KeepWordOpenRequested,
        [bool]$VisibleWordRequested,
        [string]$LogPath
    )

    $scriptArgs = @(
        "-InputDocx"
        $InputDocx
        "-OutputDir"
        $OutputDir
        "-Dpi"
        ([string]$Dpi)
    )
    if ($KeepWordOpenRequested) {
        $scriptArgs += "-KeepWordOpen"
    }
    if ($VisibleWordRequested) {
        $scriptArgs += "-VisibleWord"
    }

    $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @scriptArgs 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    Write-CommandOutput -OutputPath $LogPath -Lines $lines

    if ($exitCode -ne 0) {
        $joined = if ($lines.Count -gt 0) {
            $lines -join [System.Environment]::NewLine
        } else {
            "(no output)"
        }
        throw "run_word_visual_smoke.ps1 failed with exit code $exitCode. Output:`n$joined"
    }

    $reportDir = Join-Path $OutputDir "report"
    $summaryPath = Join-Path $reportDir "summary.json"
    $reviewResultPath = Join-Path $reportDir "review_result.json"

    if (-not (Test-Path -LiteralPath $summaryPath)) {
        throw "Visual smoke summary was not created: $summaryPath"
    }
    if (-not (Test-Path -LiteralPath $reviewResultPath)) {
        throw "Visual smoke review_result was not created: $reviewResultPath"
    }

    return [pscustomobject]@{
        Output = $lines
        SummaryPath = $summaryPath
        ReviewResultPath = $reviewResultPath
        Summary = Get-Content -Raw -LiteralPath $summaryPath | ConvertFrom-Json
        ReviewResult = Get-Content -Raw -LiteralPath $reviewResultPath | ConvertFrom-Json
        ContactSheetPath = Join-Path (Join-Path $OutputDir "evidence") "contact_sheet.png"
    }
}

function Write-SummaryMarkdown {
    param(
        [string]$Path,
        [string]$RepoRoot,
        $Summary
    )

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Project Template Smoke")
    $lines.Add("")
    $lines.Add("- Generated at: $($Summary.generated_at)")
    $lines.Add("- Manifest: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $Summary.manifest_path)")
    $lines.Add("- Output directory: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $Summary.output_dir)")
    $lines.Add("- Overall status: $($Summary.overall_status)")
    $lines.Add("- Passed flag: $($Summary.passed)")
    $lines.Add("- Entry count: $($Summary.entry_count)")
    $lines.Add("- Failed entries: $($Summary.failed_entry_count)")
    $lines.Add("- Entries with pending visual review: $($Summary.manual_review_pending_count)")
    $lines.Add("")

    foreach ($entry in $Summary.entries) {
        $lines.Add("## $($entry.name)")
        $lines.Add("")
        $lines.Add("- Status: $($entry.status)")
        $lines.Add("- Input DOCX: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $entry.input_docx)")
        $lines.Add("- Entry artifact directory: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $entry.artifact_dir)")

        foreach ($validation in @($entry.checks.template_validations)) {
            $lines.Add("- Template validation '$($validation.name)': passed=$($validation.passed) output=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $validation.output_json)")
        }

        $schemaValidation = $entry.checks.schema_validation
        if ($schemaValidation.enabled) {
            $schemaValidationOutput = Get-OptionalObjectPropertyValue -Object $schemaValidation -Name "output_json"
            $lines.Add("- Schema validation: passed=$($schemaValidation.passed) output=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaValidationOutput)")
        }

        $schemaBaseline = $entry.checks.schema_baseline
        if ($schemaBaseline.enabled) {
            $schemaBaselineLog = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "log_path"
            $lines.Add("- Schema baseline: matches=$($schemaBaseline.matches) log=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaBaselineLog)")
        }

        $visualSmoke = $entry.checks.visual_smoke
        if ($visualSmoke.enabled) {
            $contactSheet = Get-OptionalObjectPropertyValue -Object $visualSmoke -Name "contact_sheet"
            $lines.Add("- Visual smoke: review_status=$($visualSmoke.review_status) contact_sheet=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $contactSheet)")
        }

        foreach ($issue in @($entry.issues)) {
            $lines.Add("- Issue: $issue")
        }

        $lines.Add("")
    }

    $lines | Set-Content -LiteralPath $Path -Encoding UTF8
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedManifestPath = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $ManifestPath
$resolvedOutputDir = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $OutputDir -AllowMissing
$resolvedBuildDir = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
$entriesOutputDir = Join-Path $resolvedOutputDir "entries"
$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$summaryMarkdownPath = Join-Path $resolvedOutputDir "summary.md"
$baselineScriptPath = Join-Path $PSScriptRoot "check_template_schema_baseline.ps1"
$visualSmokeScriptPath = Join-Path $PSScriptRoot "run_word_visual_smoke.ps1"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $entriesOutputDir -Force | Out-Null

$manifest = Get-Content -Raw -LiteralPath $resolvedManifestPath | ConvertFrom-Json
$entries = @($manifest.entries)
if ($entries.Count -eq 0) {
    $emptySummary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        manifest_path = $resolvedManifestPath
        workspace = $repoRoot
        build_dir = $resolvedBuildDir
        output_dir = $resolvedOutputDir
        entry_count = 0
        failed_entry_count = 0
        manual_review_pending_count = 0
        passed = $true
        overall_status = "passed"
        entries = @()
    }
    ($emptySummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
    Write-SummaryMarkdown -Path $summaryMarkdownPath -RepoRoot $repoRoot -Summary $emptySummary
    Write-Step "No entries were registered in $resolvedManifestPath"
    exit 0
}

$sampleTargets = @(
    $entries |
        ForEach-Object { Resolve-OptionalManifestPropertyValue -Entry $_ -Name "prepare_sample_target" } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Sort-Object -Unique
)

if (-not $SkipBuild) {
    $vcvarsPath = Get-TemplateSchemaVcvarsPath
    $buildSamplesValue = if ($sampleTargets.Count -gt 0) { "ON" } else { "OFF" }
    Write-Step "Configuring build directory $resolvedBuildDir"
    Invoke-TemplateSchemaMsvcCommand -VcvarsPath $vcvarsPath -CommandText (
        "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"$Generator`" " +
        "-DBUILD_CLI=ON -DBUILD_SAMPLES=$buildSamplesValue -DBUILD_TESTING=OFF")

    $buildTargets = @("featherdoc_cli") + $sampleTargets
    Write-Step "Building targets: $($buildTargets -join ', ')"
    Invoke-TemplateSchemaMsvcCommand -VcvarsPath $vcvarsPath -CommandText (
        "cmake --build `"$resolvedBuildDir`" --target " + ($buildTargets -join " "))
}

$cliPath = Find-TemplateSchemaCliBinary -SearchRoot $resolvedBuildDir
if (-not $cliPath) {
    $cliPath = Find-TemplateSchemaCliBinary -SearchRoot $repoRoot
}
if (-not $cliPath) {
    throw "Could not find featherdoc_cli under $resolvedBuildDir or $repoRoot."
}

$sampleTargetExecutables = @{}
foreach ($sampleTarget in $sampleTargets) {
    $binaryPath = Find-TemplateSchemaTargetBinary -SearchRoot $resolvedBuildDir -TargetName $sampleTarget
    if (-not $binaryPath) {
        $binaryPath = Find-TemplateSchemaTargetBinary -SearchRoot $repoRoot -TargetName $sampleTarget
    }
    if (-not $binaryPath) {
        throw "Could not find built sample target '$sampleTarget' under $resolvedBuildDir or $repoRoot."
    }
    $sampleTargetExecutables[$sampleTarget] = $binaryPath
}

$results = New-Object 'System.Collections.Generic.List[object]'
$failedEntryCount = 0
$manualReviewPendingCount = 0
$entryIndex = 0

foreach ($entry in $entries) {
    $entryIndex += 1
    $name = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "name"
    if ([string]::IsNullOrWhiteSpace($name)) {
        throw "Every manifest entry must provide a non-empty 'name'."
    }

    $entryDir = Join-Path $entriesOutputDir ("{0:00}-{1}" -f $entryIndex, (Convert-ToSafeFileName -Value $name))
    New-Item -ItemType Directory -Path $entryDir -Force | Out-Null

    $entryIssues = New-Object 'System.Collections.Generic.List[string]'
    $templateValidationResults = New-Object 'System.Collections.Generic.List[object]'
    $schemaValidationResult = [ordered]@{
        enabled = $false
    }
    $schemaBaselineResult = [ordered]@{
        enabled = $false
    }
    $visualSmokeResult = [ordered]@{
        enabled = $false
    }
    $entryPassed = $true
    $manualReviewPending = $false
    $resolvedInputDocx = ""

    Write-Step "Running entry '$name'"

    try {
        $resolvedInputDocx = Resolve-ManifestInputDocxPath `
            -RepoRoot $repoRoot `
            -ResolvedBuildDir $resolvedBuildDir `
            -Entry $entry

        $prepareSampleTarget = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "prepare_sample_target"
        if (-not [string]::IsNullOrWhiteSpace($prepareSampleTarget)) {
            $prepareArgument = Resolve-PrepareArgumentPath `
                -RepoRoot $repoRoot `
                -ResolvedInputDocx $resolvedInputDocx `
                -Entry $entry
            Ensure-PathParent -Path $prepareArgument
            Write-Step "Preparing '$name' via $prepareSampleTarget"
            $prepareOutput = @(& $sampleTargetExecutables[$prepareSampleTarget] $prepareArgument 2>&1)
            $prepareExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
            $prepareLines = @($prepareOutput | ForEach-Object { $_.ToString() })
            $prepareLogPath = Join-Path $entryDir "prepare.log"
            Write-CommandOutput -OutputPath $prepareLogPath -Lines $prepareLines
            if ($prepareExitCode -ne 0) {
                $joined = if ($prepareLines.Count -gt 0) {
                    $prepareLines -join [System.Environment]::NewLine
                } else {
                    "(no output)"
                }
                throw "Sample target '$prepareSampleTarget' failed with exit code $prepareExitCode. Output:`n$joined"
            }
        }

        if (-not (Test-Path -LiteralPath $resolvedInputDocx)) {
            throw "Input DOCX was not found after preparation: $resolvedInputDocx"
        }
    } catch {
        $entryIssues.Add($_.Exception.Message) | Out-Null
        $entryPassed = $false
    }

    if ($entryPassed) {
        $validationIndex = 0
        foreach ($validation in (Get-ManifestArrayProperty -Entry $entry -Name "template_validations")) {
            $validationIndex += 1
            try {
                $validationName = Resolve-OptionalManifestPropertyValue -Entry $validation -Name "name"
                if ([string]::IsNullOrWhiteSpace($validationName)) {
                    $validationName = "template-validation-$validationIndex"
                }

                $arguments = @("validate-template", $resolvedInputDocx)
                $arguments = Add-PartSelectionArguments -Arguments $arguments -PartSwitch "--part" -Selection $validation
                $slots = Get-ManifestArrayProperty -Entry $validation -Name "slots"
                if ($slots.Count -eq 0) {
                    throw "Template validation '$validationName' must provide at least one slot."
                }
                foreach ($slot in $slots) {
                    $arguments += @("--slot", (Convert-SlotSpecToCliString -SlotSpec $slot))
                }
                $arguments += "--json"

                $outputPath = Join-Path $entryDir ("template_validation_{0:00}.json" -f $validationIndex)
                $result = Invoke-CliJsonCommand `
                    -ExecutablePath $cliPath `
                    -Arguments $arguments `
                    -OutputPath $outputPath `
                    -FailureLabel "validate-template/$validationName"

                $templateValidationResults.Add([pscustomobject]@{
                    name = $validationName
                    passed = [bool]$result.Json.passed
                    output_json = $outputPath
                    result = $result.Json
                }) | Out-Null

                if (-not [bool]$result.Json.passed) {
                    $entryPassed = $false
                }
            } catch {
                $entryIssues.Add($_.Exception.Message) | Out-Null
                $entryPassed = $false
            }
        }

        $schemaValidation = $entry.PSObject.Properties["schema_validation"]
        if ($null -ne $schemaValidation -and $null -ne $schemaValidation.Value) {
            $schemaValidationResult.enabled = $true
            try {
                $arguments = @("validate-template-schema", $resolvedInputDocx)
                $schemaValidationBlock = $schemaValidation.Value
                $schemaFile = Resolve-OptionalManifestPropertyValue -Entry $schemaValidationBlock -Name "schema_file"
                if (-not [string]::IsNullOrWhiteSpace($schemaFile)) {
                    $resolvedSchemaFile = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $schemaFile
                    $arguments += @("--schema-file", $resolvedSchemaFile)
                    $schemaValidationResult.schema_file = $resolvedSchemaFile
                }

                $schemaTargets = Get-ManifestArrayProperty -Entry $schemaValidationBlock -Name "targets"
                foreach ($target in $schemaTargets) {
                    $arguments = Add-PartSelectionArguments -Arguments $arguments -PartSwitch "--target" -Selection $target
                    $targetSlots = Get-ManifestArrayProperty -Entry $target -Name "slots"
                    if ($targetSlots.Count -eq 0) {
                        throw "Every schema_validation target must provide at least one slot."
                    }
                    foreach ($slot in $targetSlots) {
                        $arguments += @("--slot", (Convert-SlotSpecToCliString -SlotSpec $slot))
                    }
                }

                if ([string]::IsNullOrWhiteSpace($schemaFile) -and $schemaTargets.Count -eq 0) {
                    throw "schema_validation must provide 'schema_file' or at least one target."
                }

                $arguments += "--json"
                $outputPath = Join-Path $entryDir "schema_validation.json"
                $result = Invoke-CliJsonCommand `
                    -ExecutablePath $cliPath `
                    -Arguments $arguments `
                    -OutputPath $outputPath `
                    -FailureLabel "validate-template-schema/$name"

                $schemaValidationResult.output_json = $outputPath
                $schemaValidationResult.passed = [bool]$result.Json.passed
                $schemaValidationResult.result = $result.Json
                if (-not [bool]$result.Json.passed) {
                    $entryPassed = $false
                }
            } catch {
                $entryIssues.Add($_.Exception.Message) | Out-Null
                $schemaValidationResult.passed = $false
                $entryPassed = $false
            }
        }

        $schemaBaseline = $entry.PSObject.Properties["schema_baseline"]
        if ($null -ne $schemaBaseline -and $null -ne $schemaBaseline.Value) {
            $schemaBaselineResult.enabled = $true
            try {
                $schemaBaselineBlock = $schemaBaseline.Value
                $schemaFile = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "schema_file"
                if ([string]::IsNullOrWhiteSpace($schemaFile)) {
                    throw "schema_baseline must provide a non-empty 'schema_file'."
                }
                $resolvedSchemaFile = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $schemaFile
                $generatedOutput = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "generated_output"
                if ([string]::IsNullOrWhiteSpace($generatedOutput)) {
                    $generatedOutput = Join-Path $entryDir "generated_template_schema.json"
                }
                $resolvedGeneratedOutput = Resolve-ManifestPathValue `
                    -RepoRoot $repoRoot `
                    -InputPath $generatedOutput `
                    -AllowMissing
                Ensure-PathParent -Path $resolvedGeneratedOutput

                $targetMode = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "target_mode"
                if ([string]::IsNullOrWhiteSpace($targetMode)) {
                    $targetMode = "default"
                }

                $logPath = Join-Path $entryDir "schema_baseline.log"
                $result = Invoke-BaselineCheck `
                    -ScriptPath $baselineScriptPath `
                    -InputDocx $resolvedInputDocx `
                    -SchemaFile $resolvedSchemaFile `
                    -GeneratedSchemaOutput $resolvedGeneratedOutput `
                    -BuildDir $BuildDir `
                    -TargetMode $targetMode `
                    -SkipBuildRequested ([bool]$SkipBuild.IsPresent) `
                    -LogPath $logPath

                $schemaBaselineResult.schema_file = $resolvedSchemaFile
                $schemaBaselineResult.generated_output = $resolvedGeneratedOutput
                $schemaBaselineResult.target_mode = $targetMode
                $schemaBaselineResult.matches = [bool]$result.Json.matches
                $schemaBaselineResult.log_path = $logPath
                $schemaBaselineResult.result = $result.Json
                if (-not [bool]$result.Json.matches) {
                    $entryPassed = $false
                }
            } catch {
                $entryIssues.Add($_.Exception.Message) | Out-Null
                $schemaBaselineResult.matches = $false
                $entryPassed = $false
            }
        }

        $visualSmoke = $entry.PSObject.Properties["visual_smoke"]
        if ($null -ne $visualSmoke -and $null -ne $visualSmoke.Value) {
            $visualSmokeEnabled = $true
            if ($visualSmoke.Value -is [bool]) {
                $visualSmokeEnabled = [bool]$visualSmoke.Value
            } else {
                $enabledValue = Resolve-OptionalManifestPropertyValue -Entry $visualSmoke.Value -Name "enabled"
                if (-not [string]::IsNullOrWhiteSpace($enabledValue)) {
                    $visualSmokeEnabled = [System.Convert]::ToBoolean($enabledValue)
                }
            }

            if ($visualSmokeEnabled) {
                $visualSmokeResult.enabled = $true
                if ($SkipVisualSmoke) {
                    $visualSmokeResult.review_status = "skipped"
                    $visualSmokeResult.reason = "Skipped by -SkipVisualSmoke."
                } else {
                    try {
                        $visualOutputDir = if ($visualSmoke.Value -is [bool]) {
                            Join-Path $entryDir "visual-smoke"
                        } else {
                            $configuredOutputDir = Resolve-OptionalManifestPropertyValue -Entry $visualSmoke.Value -Name "output_dir"
                            if ([string]::IsNullOrWhiteSpace($configuredOutputDir)) {
                                Join-Path $entryDir "visual-smoke"
                            } else {
                                Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $configuredOutputDir -AllowMissing
                            }
                        }

                        $logPath = Join-Path $entryDir "visual_smoke.log"
                        $result = Invoke-VisualSmoke `
                            -ScriptPath $visualSmokeScriptPath `
                            -InputDocx $resolvedInputDocx `
                            -OutputDir $visualOutputDir `
                            -Dpi $Dpi `
                            -KeepWordOpenRequested ([bool]$KeepWordOpen.IsPresent) `
                            -VisibleWordRequested ([bool]$VisibleWord.IsPresent) `
                            -LogPath $logPath

                        $visualSmokeResult.output_dir = $visualOutputDir
                        $visualSmokeResult.log_path = $logPath
                        $visualSmokeResult.summary_json = $result.SummaryPath
                        $visualSmokeResult.review_result_json = $result.ReviewResultPath
                        $visualSmokeResult.contact_sheet = $result.ContactSheetPath
                        $visualSmokeResult.page_count = [int]$result.Summary.page_count
                        $visualSmokeResult.review_status = [string]$result.ReviewResult.status
                        $visualSmokeResult.review_verdict = [string]$result.ReviewResult.verdict
                        $manualReviewPending = $true
                    } catch {
                        $entryIssues.Add($_.Exception.Message) | Out-Null
                        $visualSmokeResult.review_status = "failed"
                        $entryPassed = $false
                    }
                }
            }
        }
    }

    $status = if (-not $entryPassed) {
        "failed"
    } elseif ($manualReviewPending) {
        "passed_with_pending_visual_review"
    } else {
        "passed"
    }

    if (-not $entryPassed) {
        $failedEntryCount += 1
    }
    if ($manualReviewPending) {
        $manualReviewPendingCount += 1
    }

    $results.Add([pscustomobject]@{
        name = $name
        input_docx = $resolvedInputDocx
        artifact_dir = $entryDir
        status = $status
        passed = $entryPassed
        manual_review_pending = $manualReviewPending
        checks = [ordered]@{
            template_validations = $templateValidationResults.ToArray()
            schema_validation = $schemaValidationResult
            schema_baseline = $schemaBaselineResult
            visual_smoke = $visualSmokeResult
        }
        issues = $entryIssues.ToArray()
    }) | Out-Null
}

$summaryPassed = $failedEntryCount -eq 0
$overallStatus = if (-not $summaryPassed) {
    "failed"
} elseif ($manualReviewPendingCount -gt 0) {
    "passed_with_pending_visual_review"
} else {
    "passed"
}

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    manifest_path = $resolvedManifestPath
    workspace = $repoRoot
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    entry_count = $results.Count
    failed_entry_count = $failedEntryCount
    manual_review_pending_count = $manualReviewPendingCount
    passed = $summaryPassed
    overall_status = $overallStatus
    entries = $results
}

($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Write-SummaryMarkdown -Path $summaryMarkdownPath -RepoRoot $repoRoot -Summary $summary

Write-Step "Summary JSON: $summaryPath"
Write-Step "Summary Markdown: $summaryMarkdownPath"
Write-Step "Overall status: $overallStatus"

if (-not $summaryPassed) {
    exit 1
}

exit 0
