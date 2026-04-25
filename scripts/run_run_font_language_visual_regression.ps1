param(
    [string]$BuildDir = "build-run-font-language-visual-nmake",
    [string]$OutputDir = "output/run-font-language-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[run-font-language-visual] $Message"
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

function Assert-RunState {
    param(
        [string]$JsonPath,
        [int]$ExpectedParagraphIndex,
        [int]$ExpectedRunIndex,
        [string]$ExpectedText,
        [AllowNull()][string]$ExpectedFontFamily,
        [AllowNull()][string]$ExpectedLanguage,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $run = $payload.run
    if ([int]$payload.paragraph_index -ne $ExpectedParagraphIndex) {
        throw "$Label expected paragraph_index $ExpectedParagraphIndex, got $($payload.paragraph_index)."
    }
    if ([int]$run.index -ne $ExpectedRunIndex) {
        throw "$Label expected run index $ExpectedRunIndex, got $($run.index)."
    }
    if ([string]$run.text -ne $ExpectedText) {
        throw "$Label expected text '$ExpectedText', got '$($run.text)'."
    }
    if ($null -ne $run.style_id) {
        throw "$Label expected null style_id, got '$($run.style_id)'."
    }
    if ($null -ne $run.east_asia_font_family) {
        throw "$Label expected null east_asia_font_family, got '$($run.east_asia_font_family)'."
    }

    if ($null -eq $ExpectedFontFamily) {
        if ($null -ne $run.font_family) {
            throw "$Label expected null font_family, got '$($run.font_family)'."
        }
    } elseif ([string]$run.font_family -ne $ExpectedFontFamily) {
        throw "$Label expected font_family '$ExpectedFontFamily', got '$($run.font_family)'."
    }

    if ($null -eq $ExpectedLanguage) {
        if ($null -ne $run.language) {
            throw "$Label expected null language, got '$($run.language)'."
        }
    } elseif ([string]$run.language -ne $ExpectedLanguage) {
        throw "$Label expected language '$ExpectedLanguage', got '$($run.language)'."
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

    Write-Step "Building featherdoc_cli and run font/language visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_run_font_language_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_run_font_language_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"

$baselineTargets = @(
    [ordered]@{
        id = "font-family-set"
        paragraph_index = 1
        run_index = 1
        text = "MONO SHIFT 0123456789 ABCD"
        font_family = $null
        language = $null
        inspect_json = Join-Path $resolvedOutputDir "baseline-run-font-family-set.json"
    },
    [ordered]@{
        id = "font-family-clear"
        paragraph_index = 2
        run_index = 1
        text = "MONO RESET 0123456789 ABCD"
        font_family = "Courier New"
        language = $null
        inspect_json = Join-Path $resolvedOutputDir "baseline-run-font-family-clear.json"
    },
    [ordered]@{
        id = "language-set"
        paragraph_index = 3
        run_index = 1
        text = "LANG TAG INSERT 2026 alpha beta"
        font_family = $null
        language = $null
        inspect_json = Join-Path $resolvedOutputDir "baseline-run-language-set.json"
    },
    [ordered]@{
        id = "language-clear"
        paragraph_index = 4
        run_index = 1
        text = "LANG TAG REMOVE 2026 alpha beta"
        font_family = $null
        language = "ja-JP"
        inspect_json = Join-Path $resolvedOutputDir "baseline-run-language-clear.json"
    }
)

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared run font/language baseline sample."

foreach ($target in $baselineTargets) {
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-runs", $baselineDocxPath, "$($target.paragraph_index)", "--run", "$($target.run_index)", "--json") `
        -OutputPath $target.inspect_json `
        -FailureMessage "Failed to inspect baseline target '$($target.id)'."
    Assert-RunState `
        -JsonPath $target.inspect_json `
        -ExpectedParagraphIndex $target.paragraph_index `
        -ExpectedRunIndex $target.run_index `
        -ExpectedText $target.text `
        -ExpectedFontFamily $target.font_family `
        -ExpectedLanguage $target.language `
        -Label "baseline-$($target.id)"
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
        id = "set-run-font-family-visual"
        command = "set-run-font-family"
        paragraph_index = 1
        run_index = 1
        mutation_value = "Courier New"
        expected_text = "MONO SHIFT 0123456789 ABCD"
        expected_font_family = "Courier New"
        expected_language = $null
        mutation_json_asserts = @(
            '"command":"set-run-font-family"',
            '"paragraph_index":1',
            '"run_index":1',
            '"font_family":"Courier New"'
        )
        expected_visual_cues = @(
            "The MONO SHIFT token turns into Courier New and visibly widens into a monospaced block.",
            "The other three target paragraphs stay in their baseline positions and styles.",
            "The trailing anchor remains below the four targets on the same page."
        )
    },
    [ordered]@{
        id = "clear-run-font-family-visual"
        command = "clear-run-font-family"
        paragraph_index = 2
        run_index = 1
        mutation_value = $null
        expected_text = "MONO RESET 0123456789 ABCD"
        expected_font_family = $null
        expected_language = $null
        mutation_json_asserts = @(
            '"command":"clear-run-font-family"',
            '"paragraph_index":2',
            '"run_index":1'
        )
        expected_visual_cues = @(
            "The MONO RESET token falls back from Courier New to the default proportional body font.",
            "The language-tag witness paragraphs remain visually unchanged.",
            "The page stays single-page with the trailing anchor still below the four targets."
        )
    },
    [ordered]@{
        id = "set-run-language-visual"
        command = "set-run-language"
        paragraph_index = 3
        run_index = 1
        mutation_value = "ja-JP"
        expected_text = "LANG TAG INSERT 2026 alpha beta"
        expected_font_family = $null
        expected_language = "ja-JP"
        mutation_json_asserts = @(
            '"command":"set-run-language"',
            '"paragraph_index":3',
            '"run_index":1',
            '"language":"ja-JP"'
        )
        expected_visual_cues = @(
            "The language-tag target should keep the same visible text, line breaks, and spacing as the baseline.",
            "The before/after screenshot pair is expected to look unchanged because only run metadata changes.",
            "The font-family targets above remain in their original visible states."
        )
    },
    [ordered]@{
        id = "clear-run-language-visual"
        command = "clear-run-language"
        paragraph_index = 4
        run_index = 1
        mutation_value = $null
        expected_text = "LANG TAG REMOVE 2026 alpha beta"
        expected_font_family = $null
        expected_language = $null
        mutation_json_asserts = @(
            '"command":"clear-run-language"',
            '"paragraph_index":4',
            '"run_index":1'
        )
        expected_visual_cues = @(
            "The language-tag removal target should stay visually unchanged while the ja-JP metadata disappears.",
            "The before/after screenshot pair is expected to remain stable with no new layout drift.",
            "The trailing anchor remains fixed below the four targets."
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
    $inspectJsonPath = Join-Path $caseDir "inspect_run.json"
    $visualDir = Join-Path $caseDir "mutated-visual"
    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    $arguments = @($case.command, $baselineDocxPath, "$($case.paragraph_index)", "$($case.run_index)")
    if ($null -ne $case.mutation_value) {
        $arguments += "$($case.mutation_value)"
    }
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
        -Arguments @("inspect-runs", $mutatedDocxPath, "$($case.paragraph_index)", "--run", "$($case.run_index)", "--json") `
        -OutputPath $inspectJsonPath `
        -FailureMessage "Failed to inspect case '$($case.id)'."
    Assert-RunState `
        -JsonPath $inspectJsonPath `
        -ExpectedParagraphIndex $case.paragraph_index `
        -ExpectedRunIndex $case.run_index `
        -ExpectedText $case.expected_text `
        -ExpectedFontFamily $case.expected_font_family `
        -ExpectedLanguage $case.expected_language `
        -Label $case.id

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
        inspect_run_json = $inspectJsonPath
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
    shared_baseline_runs = $baselineTargets
    cases = $summaryCases
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed run font/language visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
