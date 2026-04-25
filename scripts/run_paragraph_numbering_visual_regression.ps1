param(
    [string]$BuildDir = "build-paragraph-numbering-visual-nmake",
    [string]$OutputDir = "output/paragraph-numbering-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[paragraph-numbering-visual] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        return [System.IO.Path]::GetFullPath($InputPath)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath))
}

function Get-VcvarsPath {
    $candidates = @(
        "D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    throw "Could not locate vcvars64.bat for MSVC command-line builds."
}

function Invoke-MsvcCommand {
    param(
        [string]$VcvarsPath,
        [string]$CommandText
    )

    & cmd.exe /c "call `"$VcvarsPath`" && $CommandText"
    if ($LASTEXITCODE -ne 0) {
        throw "MSVC command failed: $CommandText"
    }
}

function Find-BuildExecutable {
    param(
        [string]$BuildRoot,
        [string]$TargetName
    )

    $resolvedBuildRoot = (Resolve-Path $BuildRoot).Path
    $candidates = @(Get-ChildItem -Path $resolvedBuildRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName })
    if ($candidates.Count -eq 0) {
        throw "Could not find built executable for target '$TargetName' under $BuildRoot."
    }

    $rootCandidate = $candidates |
        Where-Object { $_.DirectoryName -eq $resolvedBuildRoot } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($rootCandidate) {
        return $rootCandidate.FullName
    }

    return ($candidates | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}

function Get-BasePython {
    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($python) {
        return $python.Source
    }

    throw "Python was not found in PATH."
}

function Test-PythonImport {
    param(
        [string]$PythonCommand,
        [string]$ModuleName
    )

    & $PythonCommand -c "import $ModuleName" 1> $null 2> $null
    return $LASTEXITCODE -eq 0
}

function Ensure-RenderPython {
    param([string]$RepoRoot)

    $basePython = Get-BasePython
    if (Test-PythonImport -PythonCommand $basePython -ModuleName "PIL") {
        return $basePython
    }

    $venvDir = Join-Path $RepoRoot ".venv-word-visual-smoke"
    $venvPython = Join-Path $venvDir "Scripts\python.exe"
    if (-not (Test-Path $venvPython)) {
        Write-Step "Creating local Python environment at $venvDir"
        $null = & $basePython -m venv $venvDir
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to create local Python virtual environment."
        }
    }

    if (-not (Test-PythonImport -PythonCommand $venvPython -ModuleName "PIL")) {
        Write-Step "Installing Pillow into the local environment"
        & $venvPython -m pip install --disable-pip-version-check pillow 2>&1 |
            ForEach-Object { Write-Host $_ }
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to install Pillow into the local environment."
        }
    }

    return $venvPython
}

function Invoke-Capture {
    param(
        [string]$Executable,
        [string[]]$Arguments,
        [string]$OutputPath,
        [string]$FailureMessage
    )

    $lines = @(& $Executable @Arguments 2>&1 | ForEach-Object { $_.ToString() })
    if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
        $lines | Set-Content -Path $OutputPath -Encoding UTF8
    }
    foreach ($line in $lines) {
        Write-Host $line
    }
    if ($LASTEXITCODE -ne 0) {
        throw $FailureMessage
    }
}

function Invoke-WordSmoke {
    param(
        [string]$ScriptPath,
        [string]$BuildDir,
        [string]$DocxPath,
        [string]$OutputDir,
        [int]$Dpi,
        [bool]$KeepWordOpen,
        [bool]$VisibleWord
    )

    & $ScriptPath `
        -BuildDir $BuildDir `
        -InputDocx $DocxPath `
        -OutputDir $OutputDir `
        -SkipBuild `
        -Dpi $Dpi `
        -KeepWordOpen:$KeepWordOpen `
        -VisibleWord:$VisibleWord
    if ($LASTEXITCODE -ne 0) {
        throw "Word visual smoke failed for $DocxPath."
    }
}

function Build-ContactSheet {
    param(
        [string]$Python,
        [string]$ScriptPath,
        [string]$OutputPath,
        [string[]]$Images,
        [string[]]$Labels
    )

    & $Python $ScriptPath --output $OutputPath --columns 2 --thumb-width 420 --images $Images --labels $Labels
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build contact sheet at $OutputPath."
    }
}

function Read-JsonFile {
    param([string]$Path)

    return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
}

function Assert-EnsureDefinitionResult {
    param(
        [string]$JsonPath,
        [string]$ExpectedDefinitionName,
        [object[]]$ExpectedLevels,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "ensure-numbering-definition") {
        throw "$Label expected command ensure-numbering-definition, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }
    if ([string]$payload.definition_name -ne $ExpectedDefinitionName) {
        throw "$Label expected definition_name '$ExpectedDefinitionName', got '$($payload.definition_name)'."
    }

    $levels = @()
    if ($null -ne $payload.definition_levels) {
        $levels = @($payload.definition_levels)
    }
    if ($levels.Count -ne $ExpectedLevels.Count) {
        throw "$Label expected $($ExpectedLevels.Count) definition level(s), got $($levels.Count)."
    }

    for ($index = 0; $index -lt $ExpectedLevels.Count; $index++) {
        $actual = $levels[$index]
        $expected = $ExpectedLevels[$index]
        if ([int]$actual.level -ne [int]$expected.level) {
            throw "$Label expected level[$index]=$($expected.level), got $($actual.level)."
        }
        if ([string]$actual.kind -ne [string]$expected.kind) {
            throw "$Label expected kind[$index]='$($expected.kind)', got '$($actual.kind)'."
        }
        if ([int]$actual.start -ne [int]$expected.start) {
            throw "$Label expected start[$index]=$($expected.start), got $($actual.start)."
        }
        if ([string]$actual.text_pattern -ne [string]$expected.text_pattern) {
            throw "$Label expected text_pattern[$index]='$($expected.text_pattern)', got '$($actual.text_pattern)'."
        }
    }

    return [int]$payload.definition_id
}

function Assert-SetParagraphNumberingResult {
    param(
        [string]$JsonPath,
        [int]$ExpectedParagraphIndex,
        [int]$ExpectedDefinitionId,
        [int]$ExpectedLevel,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "set-paragraph-numbering") {
        throw "$Label expected command set-paragraph-numbering, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }
    if ([int]$payload.paragraph_index -ne $ExpectedParagraphIndex) {
        throw "$Label expected paragraph_index $ExpectedParagraphIndex, got $($payload.paragraph_index)."
    }
    if ([int]$payload.definition_id -ne $ExpectedDefinitionId) {
        throw "$Label expected definition_id $ExpectedDefinitionId, got $($payload.definition_id)."
    }
    if ([int]$payload.level -ne $ExpectedLevel) {
        throw "$Label expected level $ExpectedLevel, got $($payload.level)."
    }
}

function Assert-ParagraphState {
    param(
        [string]$JsonPath,
        [int]$ExpectedIndex,
        [string]$ExpectedText,
        [bool]$ExpectNumbering,
        [AllowNull()][int]$ExpectedLevel,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $paragraph = $payload.paragraph
    if ([int]$paragraph.index -ne $ExpectedIndex) {
        throw "$Label expected paragraph index $ExpectedIndex, got $($paragraph.index)."
    }
    if ([int]$paragraph.section_index -ne 0) {
        throw "$Label expected section_index 0, got $($paragraph.section_index)."
    }
    if ([string]$paragraph.text -ne $ExpectedText) {
        throw "$Label expected text '$ExpectedText', got '$($paragraph.text)'."
    }
    if ($null -ne $paragraph.style_id) {
        throw "$Label expected null style_id, got '$($paragraph.style_id)'."
    }
    if ([bool]$paragraph.section_break) {
        throw "$Label expected section_break=false."
    }

    if ($ExpectNumbering) {
        if ($null -eq $paragraph.numbering) {
            throw "$Label expected numbering metadata."
        }
        if ($null -ne $ExpectedLevel -and [int]$paragraph.numbering.level -ne [int]$ExpectedLevel) {
            throw "$Label expected numbering level $ExpectedLevel, got $($paragraph.numbering.level)."
        }
        return [int]$paragraph.numbering.num_id
    }

    if ($null -ne $paragraph.numbering) {
        throw "$Label expected null numbering metadata."
    }

    return $null
}

function Assert-NumberingDefinitionState {
    param(
        [string]$JsonPath,
        [int]$ExpectedDefinitionId,
        [string]$ExpectedDefinitionName,
        [object[]]$ExpectedLevels,
        [int]$ExpectedInstanceCount,
        [object[]]$ExpectedInstanceIds,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $definition = $payload.definition
    if ($null -eq $definition) {
        throw "$Label expected definition payload."
    }
    if ([int]$definition.definition_id -ne $ExpectedDefinitionId) {
        throw "$Label expected definition_id $ExpectedDefinitionId, got $($definition.definition_id)."
    }
    if ([string]$definition.name -ne $ExpectedDefinitionName) {
        throw "$Label expected definition name '$ExpectedDefinitionName', got '$($definition.name)'."
    }

    $levels = @()
    if ($null -ne $definition.levels) {
        $levels = @($definition.levels)
    }
    if ($levels.Count -ne $ExpectedLevels.Count) {
        throw "$Label expected $($ExpectedLevels.Count) level(s), got $($levels.Count)."
    }
    for ($index = 0; $index -lt $ExpectedLevels.Count; $index++) {
        $actual = $levels[$index]
        $expected = $ExpectedLevels[$index]
        if ([int]$actual.level -ne [int]$expected.level) {
            throw "$Label expected definition level[$index]=$($expected.level), got $($actual.level)."
        }
        if ([string]$actual.kind -ne [string]$expected.kind) {
            throw "$Label expected definition kind[$index]='$($expected.kind)', got '$($actual.kind)'."
        }
        if ([int]$actual.start -ne [int]$expected.start) {
            throw "$Label expected definition start[$index]=$($expected.start), got $($actual.start)."
        }
        if ([string]$actual.text_pattern -ne [string]$expected.text_pattern) {
            throw "$Label expected definition text_pattern[$index]='$($expected.text_pattern)', got '$($actual.text_pattern)'."
        }
    }

    $instanceIds = @()
    if ($null -ne $definition.instance_ids) {
        $instanceIds = @($definition.instance_ids | ForEach-Object { [int]$_ })
    }
    if ($instanceIds.Count -ne $ExpectedInstanceCount) {
        throw "$Label expected $ExpectedInstanceCount instance id(s), got $($instanceIds.Count)."
    }
    if ($ExpectedInstanceIds.Count -gt 0) {
        if ($instanceIds.Count -ne $ExpectedInstanceIds.Count) {
            throw "$Label expected $($ExpectedInstanceIds.Count) explicit instance id(s), got $($instanceIds.Count)."
        }
        for ($index = 0; $index -lt $ExpectedInstanceIds.Count; $index++) {
            if ([int]$instanceIds[$index] -ne [int]$ExpectedInstanceIds[$index]) {
                throw "$Label expected instance_ids[$index]=$($ExpectedInstanceIds[$index]), got $($instanceIds[$index])."
            }
        }
    }

    $instances = @()
    if ($null -ne $definition.instances) {
        $instances = @($definition.instances)
    }
    if ($instances.Count -ne $ExpectedInstanceCount) {
        throw "$Label expected $ExpectedInstanceCount instance payload(s), got $($instances.Count)."
    }

    return $instanceIds
}

function Assert-NumberingInstanceState {
    param(
        [string]$JsonPath,
        [string]$ExpectedDefinitionName,
        [int]$ExpectedInstanceId,
        [int]$ExpectedOverrideCount,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $instanceEnvelope = $payload.instance
    if ($null -eq $instanceEnvelope) {
        throw "$Label expected instance payload."
    }
    if ([string]$instanceEnvelope.definition_name -ne $ExpectedDefinitionName) {
        throw "$Label expected definition_name '$ExpectedDefinitionName', got '$($instanceEnvelope.definition_name)'."
    }

    $instance = $instanceEnvelope.instance
    if ($null -eq $instance) {
        throw "$Label expected nested numbering instance payload."
    }
    if ([int]$instance.instance_id -ne $ExpectedInstanceId) {
        throw "$Label expected instance_id $ExpectedInstanceId, got $($instance.instance_id)."
    }

    $overrides = @()
    if ($null -ne $instance.level_overrides) {
        $overrides = @($instance.level_overrides)
    }
    if ($overrides.Count -ne $ExpectedOverrideCount) {
        throw "$Label expected $ExpectedOverrideCount level override(s), got $($overrides.Count)."
    }
}

function Assert-VisualPageCount {
    param(
        [string]$VisualOutputDir,
        [int]$ExpectedCount,
        [string]$Label
    )

    $summaryPath = Join-Path $VisualOutputDir "report\summary.json"
    $payload = Read-JsonFile -Path $summaryPath
    if ([int]$payload.page_count -ne $ExpectedCount) {
        throw "$Label expected rendered page count $ExpectedCount, got $($payload.page_count)."
    }
}

function Get-RenderedPagePath {
    param(
        [string]$VisualOutputDir,
        [int]$PageNumber,
        [string]$Label
    )

    $pagePath = Join-Path $VisualOutputDir ("evidence\pages\page-{0:D2}.png" -f $PageNumber)
    if (-not (Test-Path $pagePath)) {
        throw "Missing rendered page for ${Label}: $pagePath"
    }

    return $pagePath
}

function Register-ComparisonArtifacts {
    param(
        [string]$Python,
        [string]$ContactSheetScript,
        [string]$AggregatePagesDir,
        [string]$CaseOutputDir,
        [string]$CaseId,
        [string[]]$Images,
        [string[]]$Labels
    )

    $copiedImages = @()
    for ($index = 0; $index -lt $Images.Count; $index++) {
        $copiedPath = Join-Path $AggregatePagesDir ("{0}-{1:D2}.png" -f $CaseId, ($index + 1))
        Copy-Item -Path $Images[$index] -Destination $copiedPath -Force
        $copiedImages += $copiedPath
    }

    $caseContactSheetPath = Join-Path $CaseOutputDir "before_after_contact_sheet.png"
    Build-ContactSheet `
        -Python $Python `
        -ScriptPath $ContactSheetScript `
        -OutputPath $caseContactSheetPath `
        -Images $copiedImages `
        -Labels $Labels

    return [ordered]@{
        before_after_contact_sheet = $caseContactSheetPath
        selected_pages = $copiedImages
        labels = $Labels
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"
$vcvarsPath = Get-VcvarsPath

if (-not $SkipBuild) {
    Write-Step "Configuring build directory $resolvedBuildDir"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"

    Write-Step "Building featherdoc_cli and paragraph numbering visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_paragraph_numbering_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_paragraph_numbering_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$rootTargetText = "Root target: this plain paragraph should become custom item (3) when the CLI applies a definition that starts at 3."
$nestedParentTargetText = "Nested parent target: this paragraph should become custom item (3) before a nested child paragraph renders as 3.1."
$nestedChildTargetText = "Nested child target: this paragraph should become custom item (3.1) beneath the nested parent when the CLI applies level 1 numbering."
$bulletTargetText = "Custom bullet target: this plain paragraph should gain a custom >> bullet marker when the CLI applies a bullet numbering definition."

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared paragraph numbering baseline sample."

$baselineTargets = @(
    [ordered]@{
        id = "root-target"
        paragraph_index = 1
        text = $rootTargetText
    },
    [ordered]@{
        id = "nested-parent-target"
        paragraph_index = 2
        text = $nestedParentTargetText
    },
    [ordered]@{
        id = "nested-child-target"
        paragraph_index = 3
        text = $nestedChildTargetText
    },
    [ordered]@{
        id = "bullet-target"
        paragraph_index = 4
        text = $bulletTargetText
    }
)

$summaryBaselineTargets = @()
foreach ($target in $baselineTargets) {
    $inspectJsonPath = Join-Path $resolvedOutputDir "baseline-$($target.id)-paragraph.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-paragraphs", $baselineDocxPath, "--paragraph", "$($target.paragraph_index)", "--json") `
        -OutputPath $inspectJsonPath `
        -FailureMessage "Failed to inspect baseline paragraph '$($target.id)'."

    Assert-ParagraphState `
        -JsonPath $inspectJsonPath `
        -ExpectedIndex $target.paragraph_index `
        -ExpectedText $target.text `
        -ExpectNumbering $false `
        -ExpectedLevel $null `
        -Label "baseline-$($target.id)" | Out-Null

    $summaryBaselineTargets += [ordered]@{
        id = $target.id
        paragraph_index = $target.paragraph_index
        text = $target.text
        inspect_paragraph_json = $inspectJsonPath
        expect_numbering = $false
    }
}

Write-Step "Rendering Word evidence for the shared baseline document"
Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $baselineDocxPath `
    -OutputDir $baselineVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $baselineVisualDir -ExpectedCount 1 -Label "shared-baseline-visual"

$caseDefinitions = @(
    [ordered]@{
        id = "custom-decimal-root-visual"
        definition_name = "LegalOutlineRootVisual"
        definition_levels = @(
            [ordered]@{ level = 0; kind = "decimal"; start = 3; text_pattern = "(%1)" },
            [ordered]@{ level = 1; kind = "decimal"; start = 1; text_pattern = "(%1.%2)" }
        )
        ensure_args = @(
            "--numbering-level", "0:decimal:3:(%1)",
            "--numbering-level", "1:decimal:1:(%1.%2)"
        )
        set_operations = @(
            [ordered]@{
                paragraph_index = 1
                level = $null
                expected_level = 0
                expected_text = $rootTargetText
            }
        )
        expected_numbered_paragraphs = @(
            [ordered]@{
                paragraph_index = 1
                text = $rootTargetText
                level = 0
                instance_group = "primary"
            }
        )
        expected_witness_paragraphs = @(
            [ordered]@{ paragraph_index = 2; text = $nestedParentTargetText },
            [ordered]@{ paragraph_index = 3; text = $nestedChildTargetText },
            [ordered]@{ paragraph_index = 4; text = $bulletTargetText }
        )
        expected_visual_cues = @(
            "The root target gains a custom decimal marker that starts at (3) instead of 1.",
            "The nested and bullet targets remain plain body paragraphs in this case.",
            "The trailing anchor stays on the same page below the four targets."
        )
    },
    [ordered]@{
        id = "custom-decimal-nested-visual"
        definition_name = "LegalOutlineNestedVisual"
        definition_levels = @(
            [ordered]@{ level = 0; kind = "decimal"; start = 3; text_pattern = "(%1)" },
            [ordered]@{ level = 1; kind = "decimal"; start = 1; text_pattern = "(%1.%2)" }
        )
        ensure_args = @(
            "--numbering-level", "0:decimal:3:(%1)",
            "--numbering-level", "1:decimal:1:(%1.%2)"
        )
        set_operations = @(
            [ordered]@{
                paragraph_index = 2
                level = $null
                expected_level = 0
                expected_text = $nestedParentTargetText
            },
            [ordered]@{
                paragraph_index = 3
                level = 1
                expected_level = 1
                expected_text = $nestedChildTargetText
            }
        )
        expected_numbered_paragraphs = @(
            [ordered]@{
                paragraph_index = 2
                text = $nestedParentTargetText
                level = 0
                instance_group = "primary"
            },
            [ordered]@{
                paragraph_index = 3
                text = $nestedChildTargetText
                level = 1
                instance_group = "primary"
            }
        )
        expected_witness_paragraphs = @(
            [ordered]@{ paragraph_index = 1; text = $rootTargetText },
            [ordered]@{ paragraph_index = 4; text = $bulletTargetText }
        )
        expected_visual_cues = @(
            "The nested parent gains a custom (3) marker and the child gains a (3.1) marker from level 1 numbering.",
            "Both numbered paragraphs share the same numbering instance instead of creating separate sequences.",
            "The root and bullet targets remain plain body paragraphs in this case."
        )
    },
    [ordered]@{
        id = "custom-bullet-visual"
        definition_name = "ArrowBulletVisual"
        definition_levels = @(
            [ordered]@{ level = 0; kind = "bullet"; start = 1; text_pattern = ">>" }
        )
        ensure_args = @(
            "--numbering-level", "0:bullet:1:>>"
        )
        set_operations = @(
            [ordered]@{
                paragraph_index = 4
                level = $null
                expected_level = 0
                expected_text = $bulletTargetText
            }
        )
        expected_numbered_paragraphs = @(
            [ordered]@{
                paragraph_index = 4
                text = $bulletTargetText
                level = 0
                instance_group = "primary"
            }
        )
        expected_witness_paragraphs = @(
            [ordered]@{ paragraph_index = 1; text = $rootTargetText },
            [ordered]@{ paragraph_index = 2; text = $nestedParentTargetText },
            [ordered]@{ paragraph_index = 3; text = $nestedChildTargetText }
        )
        expected_visual_cues = @(
            "The custom bullet target gains a literal >> marker from a custom bullet numbering definition.",
            "The decimal outline targets remain plain body paragraphs in this case.",
            "The document stays single-page with the trailing anchor still below the target block."
        )
    }
)

$summaryCases = @()
$aggregateImages = @()
$aggregateLabels = @()

foreach ($case in $caseDefinitions) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    $definedDocxPath = Join-Path $caseDir "$($case.id)-defined.docx"
    $ensureJsonPath = Join-Path $caseDir "ensure_numbering_definition.json"
    $definitionAfterEnsureJsonPath = Join-Path $caseDir "inspect_definition_after_ensure.json"
    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $definitionAfterApplyJsonPath = Join-Path $caseDir "inspect_definition_after_apply.json"
    $instanceInspectJsonPath = Join-Path $caseDir "inspect_numbering_instance.json"
    $visualDir = Join-Path $caseDir "mutated-visual"

    Write-Step "Running case '$($case.id)'"

    $ensureArguments = @("ensure-numbering-definition", $baselineDocxPath, "--definition-name", $case.definition_name)
    $ensureArguments += $case.ensure_args
    $ensureArguments += @("--output", $definedDocxPath, "--json")

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments $ensureArguments `
        -OutputPath $ensureJsonPath `
        -FailureMessage "Failed to run ensure-numbering-definition for case '$($case.id)'."

    $definitionId = Assert-EnsureDefinitionResult `
        -JsonPath $ensureJsonPath `
        -ExpectedDefinitionName $case.definition_name `
        -ExpectedLevels $case.definition_levels `
        -Label "$($case.id)-ensure"

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-numbering", $definedDocxPath, "--definition", "$definitionId", "--json") `
        -OutputPath $definitionAfterEnsureJsonPath `
        -FailureMessage "Failed to inspect ensured definition for case '$($case.id)'."

    Assert-NumberingDefinitionState `
        -JsonPath $definitionAfterEnsureJsonPath `
        -ExpectedDefinitionId $definitionId `
        -ExpectedDefinitionName $case.definition_name `
        -ExpectedLevels $case.definition_levels `
        -ExpectedInstanceCount 0 `
        -ExpectedInstanceIds @() `
        -Label "$($case.id)-definition-after-ensure" | Out-Null

    $currentDocPath = $definedDocxPath
    $setOperationSummaries = @()

    for ($index = 0; $index -lt $case.set_operations.Count; $index++) {
        $operation = $case.set_operations[$index]
        $isLast = $index -eq ($case.set_operations.Count - 1)
        $operationDocxPath = if ($isLast) {
            $mutatedDocxPath
        } else {
            Join-Path $caseDir ("set-operation-{0:D2}.docx" -f ($index + 1))
        }
        $setJsonPath = Join-Path $caseDir ("set_operation_{0:D2}.json" -f ($index + 1))
        $expectedLevel = [int]$operation.expected_level

        $setArguments = @(
            "set-paragraph-numbering",
            $currentDocPath,
            "$($operation.paragraph_index)",
            "--definition",
            "$definitionId"
        )
        if ($null -ne $operation.level) {
            $setArguments += @("--level", "$($operation.level)")
        }
        $setArguments += @("--output", $operationDocxPath, "--json")

        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments $setArguments `
            -OutputPath $setJsonPath `
            -FailureMessage "Failed to run set-paragraph-numbering step $($index + 1) for case '$($case.id)'."

        Assert-SetParagraphNumberingResult `
            -JsonPath $setJsonPath `
            -ExpectedParagraphIndex $operation.paragraph_index `
            -ExpectedDefinitionId $definitionId `
            -ExpectedLevel $expectedLevel `
            -Label "$($case.id)-set-$($index + 1)"

        $setOperationSummaries += [ordered]@{
            step_index = $index + 1
            paragraph_index = $operation.paragraph_index
            level = $expectedLevel
            output_docx = $operationDocxPath
            json = $setJsonPath
        }

        $currentDocPath = $operationDocxPath
    }

    $numberedParagraphArtifacts = @()
    $witnessParagraphArtifacts = @()
    $groupToNumId = @{}
    $firstInstanceId = $null

    foreach ($paragraphExpectation in $case.expected_numbered_paragraphs) {
        $inspectJsonPath = Join-Path $caseDir ("inspect_paragraph_{0:D2}.json" -f $paragraphExpectation.paragraph_index)
        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @("inspect-paragraphs", $mutatedDocxPath, "--paragraph", "$($paragraphExpectation.paragraph_index)", "--json") `
            -OutputPath $inspectJsonPath `
            -FailureMessage "Failed to inspect numbered paragraph $($paragraphExpectation.paragraph_index) for case '$($case.id)'."

        $numId = Assert-ParagraphState `
            -JsonPath $inspectJsonPath `
            -ExpectedIndex $paragraphExpectation.paragraph_index `
            -ExpectedText $paragraphExpectation.text `
            -ExpectNumbering $true `
            -ExpectedLevel ([int]$paragraphExpectation.level) `
            -Label "$($case.id)-paragraph-$($paragraphExpectation.paragraph_index)"

        if ($null -eq $firstInstanceId) {
            $firstInstanceId = [int]$numId
        }

        $groupName = [string]$paragraphExpectation.instance_group
        if ($groupToNumId.ContainsKey($groupName)) {
            if ([int]$groupToNumId[$groupName] -ne [int]$numId) {
                throw "$($case.id) expected numbered paragraphs in group '$groupName' to share one numId."
            }
        } else {
            $groupToNumId[$groupName] = [int]$numId
        }

        $numberedParagraphArtifacts += $inspectJsonPath
    }

    foreach ($paragraphExpectation in $case.expected_witness_paragraphs) {
        $inspectJsonPath = Join-Path $caseDir ("inspect_witness_paragraph_{0:D2}.json" -f $paragraphExpectation.paragraph_index)
        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @("inspect-paragraphs", $mutatedDocxPath, "--paragraph", "$($paragraphExpectation.paragraph_index)", "--json") `
            -OutputPath $inspectJsonPath `
            -FailureMessage "Failed to inspect witness paragraph $($paragraphExpectation.paragraph_index) for case '$($case.id)'."

        Assert-ParagraphState `
            -JsonPath $inspectJsonPath `
            -ExpectedIndex $paragraphExpectation.paragraph_index `
            -ExpectedText $paragraphExpectation.text `
            -ExpectNumbering $false `
            -ExpectedLevel $null `
            -Label "$($case.id)-witness-$($paragraphExpectation.paragraph_index)" | Out-Null

        $witnessParagraphArtifacts += $inspectJsonPath
    }

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-numbering", $mutatedDocxPath, "--definition", "$definitionId", "--json") `
        -OutputPath $definitionAfterApplyJsonPath `
        -FailureMessage "Failed to inspect final numbering definition for case '$($case.id)'."

    $finalInstanceIds = Assert-NumberingDefinitionState `
        -JsonPath $definitionAfterApplyJsonPath `
        -ExpectedDefinitionId $definitionId `
        -ExpectedDefinitionName $case.definition_name `
        -ExpectedLevels $case.definition_levels `
        -ExpectedInstanceCount 1 `
        -ExpectedInstanceIds @($firstInstanceId) `
        -Label "$($case.id)-definition-after-apply"

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-numbering", $mutatedDocxPath, "--instance", "$($finalInstanceIds[0])", "--json") `
        -OutputPath $instanceInspectJsonPath `
        -FailureMessage "Failed to inspect numbering instance for case '$($case.id)'."

    Assert-NumberingInstanceState `
        -JsonPath $instanceInspectJsonPath `
        -ExpectedDefinitionName $case.definition_name `
        -ExpectedInstanceId ([int]$finalInstanceIds[0]) `
        -ExpectedOverrideCount 0 `
        -Label "$($case.id)-instance"

    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $mutatedDocxPath `
        -OutputDir $visualDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent
    Assert-VisualPageCount -VisualOutputDir $visualDir -ExpectedCount 1 -Label $case.id

    $artifacts = Register-ComparisonArtifacts `
        -Python $renderPython `
        -ContactSheetScript $contactSheetScript `
        -AggregatePagesDir $aggregatePagesDir `
        -CaseOutputDir $caseDir `
        -CaseId $case.id `
        -Images @(
            (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $case.id),
            (Get-RenderedPagePath -VisualOutputDir $visualDir -PageNumber 1 -Label $case.id)
        ) `
        -Labels @("$($case.id) baseline-page-01", "$($case.id) mutated-page-01")

    $aggregateImages += $artifacts.selected_pages
    $aggregateLabels += $artifacts.labels
    $summaryCases += [ordered]@{
        id = $case.id
        source_docx = $baselineDocxPath
        defined_docx = $definedDocxPath
        mutated_docx = $mutatedDocxPath
        definition_id = $definitionId
        definition_name = $case.definition_name
        ensure_json = $ensureJsonPath
        inspect_definition_after_ensure_json = $definitionAfterEnsureJsonPath
        set_operations = $setOperationSummaries
        inspect_numbered_paragraph_json = $numberedParagraphArtifacts
        inspect_witness_paragraph_json = $witnessParagraphArtifacts
        inspect_definition_after_apply_json = $definitionAfterApplyJsonPath
        inspect_numbering_instance_json = $instanceInspectJsonPath
        expected_visual_cues = $case.expected_visual_cues
        visual_artifacts = $artifacts
    }
}

Build-ContactSheet `
    -Python $renderPython `
    -ScriptPath $contactSheetScript `
    -OutputPath $aggregateContactSheetPath `
    -Images $aggregateImages `
    -Labels $aggregateLabels

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = $true
    shared_baseline_docx = $baselineDocxPath
    shared_baseline_paragraphs = $summaryBaselineTargets
    shared_baseline_visual = [ordered]@{
        root = $baselineVisualDir
        page = (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label "shared-baseline")
    }
    cases = $summaryCases
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed paragraph numbering visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
