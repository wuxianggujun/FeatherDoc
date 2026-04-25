param(
    [string]$BuildDir = "build-paragraph-list-visual-nmake",
    [string]$OutputDir = "output/paragraph-list-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[paragraph-list-visual] $Message"
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

function Assert-Contains {
    param(
        [string]$Path,
        [string]$Expected,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if (-not $content.Contains($Expected)) {
        throw "$Label did not contain expected text: $Expected"
    }
}

function Assert-ParagraphState {
    param(
        [string]$JsonPath,
        [int]$ExpectedIndex,
        [string]$ExpectedText,
        [bool]$ExpectNumbering,
        [object]$ExpectedLevel,
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

function Assert-NumberingInstanceState {
    param(
        [string]$JsonPath,
        [string]$ExpectedDefinitionName,
        [int]$ExpectedInstanceId,
        [int]$ExpectedOverrideCount,
        [bool]$ExpectRestartOverride,
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

    if ($ExpectRestartOverride) {
        if ($overrides.Count -lt 1) {
            throw "$Label expected a restart override entry."
        }

        $override = $overrides[0]
        if ([int]$override.level -ne 0) {
            throw "$Label expected restart override level 0, got $($override.level)."
        }
        if ([int]$override.start_override -ne 1) {
            throw "$Label expected start_override 1, got $($override.start_override)."
        }
        if ($null -ne $override.level_definition) {
            throw "$Label expected null level_definition for the restart override."
        }
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

    Write-Step "Building featherdoc_cli and paragraph list visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_paragraph_list_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_paragraph_list_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$setTargetText = "Set target: this plain paragraph should gain a bullet marker when the CLI applies set-paragraph-list."
$restartWitnessText = "Restart witness: the next two paragraphs already belong to one decimal list, so the restart target should render as a fresh item 1."
$restartExistingFirstText = "Existing decimal item A stays in the original list instance."
$restartExistingSecondText = "Existing decimal item B should remain item 2 in the same list."
$restartTargetText = "Restart target: this plain paragraph should restart as item 1 when the CLI applies restart-paragraph-list."
$clearTargetText = "Clear target: this paragraph starts as a bullet item and should fall back to normal body text when the CLI clears the list."

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared paragraph list baseline sample."

$baselineTargets = @(
    [ordered]@{
        id = "set-target"
        paragraph_index = 1
        text = $setTargetText
        expect_numbering = $false
    },
    [ordered]@{
        id = "restart-existing-first"
        paragraph_index = 3
        text = $restartExistingFirstText
        expect_numbering = $true
        expected_level = 0
        expected_definition_name = "FeatherDocDecimalList"
    },
    [ordered]@{
        id = "restart-existing-second"
        paragraph_index = 4
        text = $restartExistingSecondText
        expect_numbering = $true
        expected_level = 0
        expected_definition_name = "FeatherDocDecimalList"
    },
    [ordered]@{
        id = "restart-target"
        paragraph_index = 5
        text = $restartTargetText
        expect_numbering = $false
    },
    [ordered]@{
        id = "clear-target"
        paragraph_index = 6
        text = $clearTargetText
        expect_numbering = $true
        expected_level = 0
        expected_definition_name = "FeatherDocBulletList"
    }
)

$summaryBaselineTargets = @()
$baselineNumberingIds = @{}

foreach ($target in $baselineTargets) {
    $inspectJsonPath = Join-Path $resolvedOutputDir "baseline-$($target.id)-paragraph.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-paragraphs", $baselineDocxPath, "--paragraph", "$($target.paragraph_index)", "--json") `
        -OutputPath $inspectJsonPath `
        -FailureMessage "Failed to inspect baseline paragraph '$($target.id)'."

    $numberingId = Assert-ParagraphState `
        -JsonPath $inspectJsonPath `
        -ExpectedIndex $target.paragraph_index `
        -ExpectedText $target.text `
        -ExpectNumbering $target.expect_numbering `
        -ExpectedLevel $target.expected_level `
        -Label "baseline-$($target.id)"

    $summaryTarget = [ordered]@{
        id = $target.id
        paragraph_index = $target.paragraph_index
        text = $target.text
        inspect_paragraph_json = $inspectJsonPath
        expect_numbering = $target.expect_numbering
    }

    if ($target.expect_numbering) {
        $numberingJsonPath = Join-Path $resolvedOutputDir "baseline-$($target.id)-numbering.json"
        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @("inspect-numbering", $baselineDocxPath, "--instance", "$numberingId", "--json") `
            -OutputPath $numberingJsonPath `
            -FailureMessage "Failed to inspect baseline numbering instance '$($target.id)'."

        Assert-NumberingInstanceState `
            -JsonPath $numberingJsonPath `
            -ExpectedDefinitionName $target.expected_definition_name `
            -ExpectedInstanceId $numberingId `
            -ExpectedOverrideCount 0 `
            -ExpectRestartOverride $false `
            -Label "baseline-$($target.id)"

        $baselineNumberingIds[$target.id] = [int]$numberingId
        $summaryTarget.instance_id = [int]$numberingId
        $summaryTarget.definition_name = $target.expected_definition_name
        $summaryTarget.inspect_numbering_instance_json = $numberingJsonPath
    }

    $summaryBaselineTargets += $summaryTarget
}

if ($baselineNumberingIds["restart-existing-first"] -ne $baselineNumberingIds["restart-existing-second"]) {
    throw "Baseline decimal witness items were expected to share one numbering instance."
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
        id = "set-paragraph-list-visual"
        command = "set-paragraph-list"
        paragraph_index = 1
        command_args = @("--kind", "bullet")
        expected_text = $setTargetText
        expected_has_numbering = $true
        expected_level = 0
        expected_definition_name = "FeatherDocBulletList"
        expected_override_count = 0
        expect_restart_override = $false
        mutation_json_asserts = @(
            '"command":"set-paragraph-list"',
            '"paragraph_index":1',
            '"kind":"bullet"',
            '"level":0'
        )
        expected_visual_cues = @(
            "The set target gains a bullet marker and shifts into list indentation.",
            "The restart witness block keeps its existing decimal 1 and 2 markers.",
            "The clear target remains a bullet item in this case, and the trailing anchor stays on the same page."
        )
    },
    [ordered]@{
        id = "restart-paragraph-list-visual"
        command = "restart-paragraph-list"
        paragraph_index = 5
        command_args = @("--kind", "decimal")
        expected_text = $restartTargetText
        expected_has_numbering = $true
        expected_level = 0
        expected_definition_name = "FeatherDocDecimalList"
        expected_override_count = 1
        expect_restart_override = $true
        mutation_json_asserts = @(
            '"command":"restart-paragraph-list"',
            '"paragraph_index":5',
            '"kind":"decimal"',
            '"level":0'
        )
        expected_visual_cues = @(
            "The restart target becomes a fresh decimal item 1 even though decimal items 1 and 2 already appear above it.",
            "The two existing decimal witness paragraphs keep their original numbering instance and stay visually unchanged.",
            "The clear-target bullet and trailing anchor remain on the same page."
        )
    },
    [ordered]@{
        id = "clear-paragraph-list-visual"
        command = "clear-paragraph-list"
        paragraph_index = 6
        command_args = @()
        expected_text = $clearTargetText
        expected_has_numbering = $false
        expected_level = $null
        expected_definition_name = $null
        expected_override_count = 0
        expect_restart_override = $false
        mutation_json_asserts = @(
            '"command":"clear-paragraph-list"',
            '"paragraph_index":6'
        )
        expected_visual_cues = @(
            "The clear target drops its bullet marker and returns to plain body alignment.",
            "The restart witness block still shows its existing decimal items above the cleared paragraph.",
            "The page remains single-page with the trailing anchor still below the list targets."
        )
    }
)

$summaryCases = @()
$aggregateImages = @()
$aggregateLabels = @()

foreach ($case in $caseDefinitions) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $mutationJsonPath = Join-Path $caseDir "mutation_result.json"
    $inspectParagraphJsonPath = Join-Path $caseDir "inspect_paragraph.json"
    $inspectNumberingJsonPath = Join-Path $caseDir "inspect_numbering_instance.json"
    $visualDir = Join-Path $caseDir "mutated-visual"
    $witnessArtifacts = @()
    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    $arguments = @($case.command, $baselineDocxPath, "$($case.paragraph_index)")
    $arguments += $case.command_args
    $arguments += @("--output", $mutatedDocxPath, "--json")

    Write-Step "Running case '$($case.id)'"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments $arguments `
        -OutputPath $mutationJsonPath `
        -FailureMessage "Failed to run case '$($case.id)'."

    foreach ($expected in $case.mutation_json_asserts) {
        Assert-Contains -Path $mutationJsonPath -Expected $expected -Label $case.id
    }

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-paragraphs", $mutatedDocxPath, "--paragraph", "$($case.paragraph_index)", "--json") `
        -OutputPath $inspectParagraphJsonPath `
        -FailureMessage "Failed to inspect case '$($case.id)'."

    $targetNumberingId = Assert-ParagraphState `
        -JsonPath $inspectParagraphJsonPath `
        -ExpectedIndex $case.paragraph_index `
        -ExpectedText $case.expected_text `
        -ExpectNumbering $case.expected_has_numbering `
        -ExpectedLevel $case.expected_level `
        -Label $case.id

    if ($case.expected_has_numbering) {
        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @("inspect-numbering", $mutatedDocxPath, "--instance", "$targetNumberingId", "--json") `
            -OutputPath $inspectNumberingJsonPath `
            -FailureMessage "Failed to inspect numbering instance for case '$($case.id)'."

        Assert-NumberingInstanceState `
            -JsonPath $inspectNumberingJsonPath `
            -ExpectedDefinitionName $case.expected_definition_name `
            -ExpectedInstanceId $targetNumberingId `
            -ExpectedOverrideCount $case.expected_override_count `
            -ExpectRestartOverride $case.expect_restart_override `
            -Label $case.id
    } else {
        $inspectNumberingJsonPath = $null
    }

    if ($case.id -eq "restart-paragraph-list-visual") {
        $restartFirstWitnessJson = Join-Path $caseDir "restart-witness-first.json"
        $restartSecondWitnessJson = Join-Path $caseDir "restart-witness-second.json"

        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @("inspect-paragraphs", $mutatedDocxPath, "--paragraph", "3", "--json") `
            -OutputPath $restartFirstWitnessJson `
            -FailureMessage "Failed to inspect first restart witness paragraph."
        $restartFirstNumId = Assert-ParagraphState `
            -JsonPath $restartFirstWitnessJson `
            -ExpectedIndex 3 `
            -ExpectedText $restartExistingFirstText `
            -ExpectNumbering $true `
            -ExpectedLevel 0 `
            -Label "$($case.id)-restart-witness-first"

        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @("inspect-paragraphs", $mutatedDocxPath, "--paragraph", "4", "--json") `
            -OutputPath $restartSecondWitnessJson `
            -FailureMessage "Failed to inspect second restart witness paragraph."
        $restartSecondNumId = Assert-ParagraphState `
            -JsonPath $restartSecondWitnessJson `
            -ExpectedIndex 4 `
            -ExpectedText $restartExistingSecondText `
            -ExpectNumbering $true `
            -ExpectedLevel 0 `
            -Label "$($case.id)-restart-witness-second"

        if ($restartFirstNumId -ne $baselineNumberingIds["restart-existing-first"]) {
            throw "Restart case changed the first witness numbering instance unexpectedly."
        }
        if ($restartSecondNumId -ne $baselineNumberingIds["restart-existing-second"]) {
            throw "Restart case changed the second witness numbering instance unexpectedly."
        }
        if ($targetNumberingId -eq $restartFirstNumId) {
            throw "Restart case reused the existing decimal instance instead of creating a fresh restart."
        }

        $witnessArtifacts += $restartFirstWitnessJson
        $witnessArtifacts += $restartSecondWitnessJson
    }

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
        command = $case.command
        source_docx = $baselineDocxPath
        mutated_docx = $mutatedDocxPath
        mutation_json = $mutationJsonPath
        inspect_paragraph_json = $inspectParagraphJsonPath
        inspect_numbering_instance_json = $inspectNumberingJsonPath
        witness_paragraph_json = $witnessArtifacts
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

Write-Step "Completed paragraph list visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
