param(
    [string]$BuildDir = "build-paragraph-run-style-visual-nmake",
    [string]$OutputDir = "output/paragraph-run-style-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[paragraph-run-style-visual] $Message"
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
        [AllowNull()][string]$ExpectedStyleId,
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
    if ([bool]$paragraph.section_break) {
        throw "$Label expected section_break=false."
    }

    if ($null -eq $ExpectedStyleId) {
        if ($null -ne $paragraph.style_id) {
            throw "$Label expected null style_id, got '$($paragraph.style_id)'."
        }
    } elseif ([string]$paragraph.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($paragraph.style_id)'."
    }
}

function Assert-RunState {
    param(
        [string]$JsonPath,
        [int]$ExpectedParagraphIndex,
        [int]$ExpectedRunIndex,
        [string]$ExpectedText,
        [AllowNull()][string]$ExpectedStyleId,
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
    if ($null -ne $run.font_family -or $null -ne $run.east_asia_font_family -or $null -ne $run.language) {
        throw "$Label expected no direct font/language overrides on the inspected run."
    }

    if ($null -eq $ExpectedStyleId) {
        if ($null -ne $run.style_id) {
            throw "$Label expected null style_id, got '$($run.style_id)'."
        }
    } elseif ([string]$run.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($run.style_id)'."
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

    Write-Step "Building featherdoc_cli and paragraph/run style visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_paragraph_run_style_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_paragraph_run_style_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$paragraphSetIndex = 1
$paragraphClearIndex = 2
$runSetParagraphIndex = 3
$runClearParagraphIndex = 4
$runTargetIndex = 1

$paragraphSetText = "Paragraph set target: this line starts as regular body text and should jump to Heading 2 when the CLI applies a paragraph style."
$paragraphClearText = "Paragraph clear target: this line starts as Heading 2 and should fall back to Normal when the CLI clears the paragraph style."
$runSetTargetText = "BOLD ME"
$runClearTargetText = "UNBOLD ME"

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"
$baselineParagraphSetJson = Join-Path $resolvedOutputDir "baseline-paragraph-set-target.json"
$baselineParagraphClearJson = Join-Path $resolvedOutputDir "baseline-paragraph-clear-target.json"
$baselineRunSetJson = Join-Path $resolvedOutputDir "baseline-run-set-target.json"
$baselineRunClearJson = Join-Path $resolvedOutputDir "baseline-run-clear-target.json"
$baselineTemplateRunSetJson = Join-Path $resolvedOutputDir "baseline-template-run-set-target.json"
$baselineTemplateRunClearJson = Join-Path $resolvedOutputDir "baseline-template-run-clear-target.json"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared paragraph/run style baseline sample."

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-paragraphs", $baselineDocxPath, "--paragraph", "$paragraphSetIndex", "--json") `
    -OutputPath $baselineParagraphSetJson `
    -FailureMessage "Failed to inspect baseline paragraph set target."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-paragraphs", $baselineDocxPath, "--paragraph", "$paragraphClearIndex", "--json") `
    -OutputPath $baselineParagraphClearJson `
    -FailureMessage "Failed to inspect baseline paragraph clear target."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-runs", $baselineDocxPath, "$runSetParagraphIndex", "--run", "$runTargetIndex", "--json") `
    -OutputPath $baselineRunSetJson `
    -FailureMessage "Failed to inspect baseline run set target."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-runs", $baselineDocxPath, "$runClearParagraphIndex", "--run", "$runTargetIndex", "--json") `
    -OutputPath $baselineRunClearJson `
    -FailureMessage "Failed to inspect baseline run clear target."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-runs", $baselineDocxPath, "$runSetParagraphIndex", "--run", "$runTargetIndex", "--json") `
    -OutputPath $baselineTemplateRunSetJson `
    -FailureMessage "Failed to inspect baseline template run set target."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-runs", $baselineDocxPath, "$runClearParagraphIndex", "--run", "$runTargetIndex", "--json") `
    -OutputPath $baselineTemplateRunClearJson `
    -FailureMessage "Failed to inspect baseline template run clear target."

Assert-ParagraphState -JsonPath $baselineParagraphSetJson -ExpectedIndex $paragraphSetIndex -ExpectedText $paragraphSetText -ExpectedStyleId $null -Label "baseline paragraph-set target"
Assert-ParagraphState -JsonPath $baselineParagraphClearJson -ExpectedIndex $paragraphClearIndex -ExpectedText $paragraphClearText -ExpectedStyleId "Heading2" -Label "baseline paragraph-clear target"
Assert-RunState -JsonPath $baselineRunSetJson -ExpectedParagraphIndex $runSetParagraphIndex -ExpectedRunIndex $runTargetIndex -ExpectedText $runSetTargetText -ExpectedStyleId $null -Label "baseline run-set target"
Assert-RunState -JsonPath $baselineRunClearJson -ExpectedParagraphIndex $runClearParagraphIndex -ExpectedRunIndex $runTargetIndex -ExpectedText $runClearTargetText -ExpectedStyleId "Strong" -Label "baseline run-clear target"
Assert-RunState -JsonPath $baselineTemplateRunSetJson -ExpectedParagraphIndex $runSetParagraphIndex -ExpectedRunIndex $runTargetIndex -ExpectedText $runSetTargetText -ExpectedStyleId $null -Label "baseline template run-set target"
Assert-RunState -JsonPath $baselineTemplateRunClearJson -ExpectedParagraphIndex $runClearParagraphIndex -ExpectedRunIndex $runTargetIndex -ExpectedText $runClearTargetText -ExpectedStyleId "Strong" -Label "baseline template run-clear target"

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

$summaryCases = @()
$aggregateImages = @()
$aggregateLabels = @()

$setParagraphCaseId = "set-paragraph-style-visual"
$setParagraphCaseDir = Join-Path $resolvedOutputDir $setParagraphCaseId
$setParagraphDocxPath = Join-Path $setParagraphCaseDir "$setParagraphCaseId-mutated.docx"
$setParagraphMutationJsonPath = Join-Path $setParagraphCaseDir "mutation_result.json"
$setParagraphInspectJsonPath = Join-Path $setParagraphCaseDir "inspect_paragraph.json"
$setParagraphVisualDir = Join-Path $setParagraphCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $setParagraphCaseDir -Force | Out-Null

Write-Step "Running case '$setParagraphCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "set-paragraph-style", $baselineDocxPath, "$paragraphSetIndex", "Heading2",
        "--output", $setParagraphDocxPath, "--json"
    ) `
    -OutputPath $setParagraphMutationJsonPath `
    -FailureMessage "Failed to set the paragraph style on the plain target."

foreach ($expected in @(
        '"command":"set-paragraph-style"',
        '"paragraph_index":1',
        '"style_id":"Heading2"'
    )) {
    Assert-Contains -Path $setParagraphMutationJsonPath -Expected $expected -Label $setParagraphCaseId
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-paragraphs", $setParagraphDocxPath, "--paragraph", "$paragraphSetIndex", "--json") `
    -OutputPath $setParagraphInspectJsonPath `
    -FailureMessage "Failed to inspect the set-paragraph-style result."
Assert-ParagraphState -JsonPath $setParagraphInspectJsonPath -ExpectedIndex $paragraphSetIndex -ExpectedText $paragraphSetText -ExpectedStyleId "Heading2" -Label $setParagraphCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $setParagraphDocxPath `
    -OutputDir $setParagraphVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $setParagraphVisualDir -ExpectedCount 1 -Label $setParagraphCaseId

$setParagraphArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $setParagraphCaseDir `
    -CaseId $setParagraphCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $setParagraphCaseId),
        (Get-RenderedPagePath -VisualOutputDir $setParagraphVisualDir -PageNumber 1 -Label $setParagraphCaseId)
    ) `
    -Labels @("$setParagraphCaseId baseline-page-01", "$setParagraphCaseId mutated-page-01")

$aggregateImages += $setParagraphArtifacts.selected_pages
$aggregateLabels += $setParagraphArtifacts.labels
$summaryCases += [ordered]@{
    id = $setParagraphCaseId
    command = "set-paragraph-style"
    source_docx = $baselineDocxPath
    mutated_docx = $setParagraphDocxPath
    mutation_json = $setParagraphMutationJsonPath
    inspect_paragraph_json = $setParagraphInspectJsonPath
    expected_visual_cues = @(
        "The plain paragraph-set target grows into Heading 2 with larger bold text.",
        "The clear target below it stays in Heading 2, so the page shows two heading-sized lines after mutation.",
        "The run-style paragraphs remain unchanged."
    )
    visual_artifacts = $setParagraphArtifacts
}

$clearParagraphCaseId = "clear-paragraph-style-visual"
$clearParagraphCaseDir = Join-Path $resolvedOutputDir $clearParagraphCaseId
$clearParagraphDocxPath = Join-Path $clearParagraphCaseDir "$clearParagraphCaseId-mutated.docx"
$clearParagraphMutationJsonPath = Join-Path $clearParagraphCaseDir "mutation_result.json"
$clearParagraphInspectJsonPath = Join-Path $clearParagraphCaseDir "inspect_paragraph.json"
$clearParagraphVisualDir = Join-Path $clearParagraphCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $clearParagraphCaseDir -Force | Out-Null

Write-Step "Running case '$clearParagraphCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "clear-paragraph-style", $baselineDocxPath, "$paragraphClearIndex",
        "--output", $clearParagraphDocxPath, "--json"
    ) `
    -OutputPath $clearParagraphMutationJsonPath `
    -FailureMessage "Failed to clear the paragraph style from the styled target."

foreach ($expected in @(
        '"command":"clear-paragraph-style"',
        '"paragraph_index":2'
    )) {
    Assert-Contains -Path $clearParagraphMutationJsonPath -Expected $expected -Label $clearParagraphCaseId
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-paragraphs", $clearParagraphDocxPath, "--paragraph", "$paragraphClearIndex", "--json") `
    -OutputPath $clearParagraphInspectJsonPath `
    -FailureMessage "Failed to inspect the clear-paragraph-style result."
Assert-ParagraphState -JsonPath $clearParagraphInspectJsonPath -ExpectedIndex $paragraphClearIndex -ExpectedText $paragraphClearText -ExpectedStyleId $null -Label $clearParagraphCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $clearParagraphDocxPath `
    -OutputDir $clearParagraphVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $clearParagraphVisualDir -ExpectedCount 1 -Label $clearParagraphCaseId

$clearParagraphArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $clearParagraphCaseDir `
    -CaseId $clearParagraphCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $clearParagraphCaseId),
        (Get-RenderedPagePath -VisualOutputDir $clearParagraphVisualDir -PageNumber 1 -Label $clearParagraphCaseId)
    ) `
    -Labels @("$clearParagraphCaseId baseline-page-01", "$clearParagraphCaseId mutated-page-01")

$aggregateImages += $clearParagraphArtifacts.selected_pages
$aggregateLabels += $clearParagraphArtifacts.labels
$summaryCases += [ordered]@{
    id = $clearParagraphCaseId
    command = "clear-paragraph-style"
    source_docx = $baselineDocxPath
    mutated_docx = $clearParagraphDocxPath
    mutation_json = $clearParagraphMutationJsonPath
    inspect_paragraph_json = $clearParagraphInspectJsonPath
    expected_visual_cues = @(
        "The Heading 2 clear target drops back to regular body text.",
        "The plain paragraph-set target above it stays unchanged, so only one line shrinks in the before/after pair.",
        "The run-style targets remain unchanged."
    )
    visual_artifacts = $clearParagraphArtifacts
}

$setRunCaseId = "set-run-style-visual"
$setRunCaseDir = Join-Path $resolvedOutputDir $setRunCaseId
$setRunDocxPath = Join-Path $setRunCaseDir "$setRunCaseId-mutated.docx"
$setRunMutationJsonPath = Join-Path $setRunCaseDir "mutation_result.json"
$setRunInspectJsonPath = Join-Path $setRunCaseDir "inspect_run.json"
$setRunVisualDir = Join-Path $setRunCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $setRunCaseDir -Force | Out-Null

Write-Step "Running case '$setRunCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "set-run-style", $baselineDocxPath, "$runSetParagraphIndex", "$runTargetIndex", "Strong",
        "--output", $setRunDocxPath, "--json"
    ) `
    -OutputPath $setRunMutationJsonPath `
    -FailureMessage "Failed to set the run style on the plain run target."

foreach ($expected in @(
        '"command":"set-run-style"',
        '"paragraph_index":3',
        '"run_index":1',
        '"style_id":"Strong"'
    )) {
    Assert-Contains -Path $setRunMutationJsonPath -Expected $expected -Label $setRunCaseId
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-runs", $setRunDocxPath, "$runSetParagraphIndex", "--run", "$runTargetIndex", "--json") `
    -OutputPath $setRunInspectJsonPath `
    -FailureMessage "Failed to inspect the set-run-style result."
Assert-RunState -JsonPath $setRunInspectJsonPath -ExpectedParagraphIndex $runSetParagraphIndex -ExpectedRunIndex $runTargetIndex -ExpectedText $runSetTargetText -ExpectedStyleId "Strong" -Label $setRunCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $setRunDocxPath `
    -OutputDir $setRunVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $setRunVisualDir -ExpectedCount 1 -Label $setRunCaseId

$setRunArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $setRunCaseDir `
    -CaseId $setRunCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $setRunCaseId),
        (Get-RenderedPagePath -VisualOutputDir $setRunVisualDir -PageNumber 1 -Label $setRunCaseId)
    ) `
    -Labels @("$setRunCaseId baseline-page-01", "$setRunCaseId mutated-page-01")

$aggregateImages += $setRunArtifacts.selected_pages
$aggregateLabels += $setRunArtifacts.labels
$summaryCases += [ordered]@{
    id = $setRunCaseId
    command = "set-run-style"
    source_docx = $baselineDocxPath
    mutated_docx = $setRunDocxPath
    mutation_json = $setRunMutationJsonPath
    inspect_run_json = $setRunInspectJsonPath
    expected_visual_cues = @(
        "Only the BOLD ME token inside the run-set target turns bold.",
        "The paragraph-level targets above stay unchanged, so the emphasis change is localized to one inline span.",
        "The prefix and suffix around the styled run stay regular body text."
    )
    visual_artifacts = $setRunArtifacts
}

$clearRunCaseId = "clear-run-style-visual"
$clearRunCaseDir = Join-Path $resolvedOutputDir $clearRunCaseId
$clearRunDocxPath = Join-Path $clearRunCaseDir "$clearRunCaseId-mutated.docx"
$clearRunMutationJsonPath = Join-Path $clearRunCaseDir "mutation_result.json"
$clearRunInspectJsonPath = Join-Path $clearRunCaseDir "inspect_run.json"
$clearRunVisualDir = Join-Path $clearRunCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $clearRunCaseDir -Force | Out-Null

Write-Step "Running case '$clearRunCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "clear-run-style", $baselineDocxPath, "$runClearParagraphIndex", "$runTargetIndex",
        "--output", $clearRunDocxPath, "--json"
    ) `
    -OutputPath $clearRunMutationJsonPath `
    -FailureMessage "Failed to clear the run style from the bold run target."

foreach ($expected in @(
        '"command":"clear-run-style"',
        '"paragraph_index":4',
        '"run_index":1'
    )) {
    Assert-Contains -Path $clearRunMutationJsonPath -Expected $expected -Label $clearRunCaseId
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-runs", $clearRunDocxPath, "$runClearParagraphIndex", "--run", "$runTargetIndex", "--json") `
    -OutputPath $clearRunInspectJsonPath `
    -FailureMessage "Failed to inspect the clear-run-style result."
Assert-RunState -JsonPath $clearRunInspectJsonPath -ExpectedParagraphIndex $runClearParagraphIndex -ExpectedRunIndex $runTargetIndex -ExpectedText $runClearTargetText -ExpectedStyleId $null -Label $clearRunCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $clearRunDocxPath `
    -OutputDir $clearRunVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $clearRunVisualDir -ExpectedCount 1 -Label $clearRunCaseId

$clearRunArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $clearRunCaseDir `
    -CaseId $clearRunCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $clearRunCaseId),
        (Get-RenderedPagePath -VisualOutputDir $clearRunVisualDir -PageNumber 1 -Label $clearRunCaseId)
    ) `
    -Labels @("$clearRunCaseId baseline-page-01", "$clearRunCaseId mutated-page-01")

$aggregateImages += $clearRunArtifacts.selected_pages
$aggregateLabels += $clearRunArtifacts.labels
$summaryCases += [ordered]@{
    id = $clearRunCaseId
    command = "clear-run-style"
    source_docx = $baselineDocxPath
    mutated_docx = $clearRunDocxPath
    mutation_json = $clearRunMutationJsonPath
    inspect_run_json = $clearRunInspectJsonPath
    expected_visual_cues = @(
        "Only the UNBOLD ME token inside the run-clear target returns to regular weight.",
        "The surrounding prefix and suffix stay unchanged, so the before/after pair isolates one inline style removal.",
        "The paragraph-level targets above stay unchanged."
    )
    visual_artifacts = $clearRunArtifacts
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
    shared_baseline_inspection = [ordered]@{
        paragraph_set = $baselineParagraphSetJson
        paragraph_clear = $baselineParagraphClearJson
        run_set = $baselineRunSetJson
        run_clear = $baselineRunClearJson
        template_run_set = $baselineTemplateRunSetJson
        template_run_clear = $baselineTemplateRunClearJson
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

Write-Step "Completed paragraph/run style visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
