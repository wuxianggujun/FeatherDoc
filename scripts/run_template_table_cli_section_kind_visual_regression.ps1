param(
    [string]$BuildDir = "build-template-table-cli-section-kind-visual-nmake",
    [string]$OutputDir = "output/template-table-cli-section-kind-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[template-table-cli-section-kind-visual] $Message"
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

    $bestCandidate = $candidates |
        Sort-Object @{
            Expression = {
                ($_.FullName.Substring($resolvedBuildRoot.Length).TrimStart('\') -split '\\').Count
            }
        }, @{ Expression = { $_.LastWriteTime }; Descending = $true } |
        Select-Object -First 1
    return $bestCandidate.FullName
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

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        & $PythonCommand -c "import $ModuleName" 1> $null 2> $null
        return $LASTEXITCODE -eq 0
    } catch {
        return $false
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
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

function Invoke-CommandCapture {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments,
        [string]$OutputPath,
        [string]$FailureMessage
    )

    $commandOutput = @(& $ExecutablePath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
        $lines | Set-Content -Path $OutputPath -Encoding UTF8
    }
    foreach ($line in $lines) {
        Write-Host $line
    }
    if ($exitCode -ne 0) {
        throw $FailureMessage
    }
}

function Test-StringArraysEqual {
    param(
        [string[]]$Left,
        [string[]]$Right
    )

    if ($Left.Count -ne $Right.Count) {
        return $false
    }

    for ($index = 0; $index -lt $Left.Count; $index++) {
        if ($Left[$index] -ne $Right[$index]) {
            return $false
        }
    }

    return $true
}

function Assert-TableRows {
    param(
        [string]$RowsJsonPath,
        [string[][]]$ExpectedRows,
        [string]$Label
    )

    $json = Get-Content -Raw -LiteralPath $RowsJsonPath | ConvertFrom-Json
    $rows = @($json.rows)
    if ($rows.Count -ne $ExpectedRows.Count) {
        throw "$Label row count mismatch. Expected $($ExpectedRows.Count), got $($rows.Count)."
    }

    for ($index = 0; $index -lt $ExpectedRows.Count; $index++) {
        $actualCells = @($rows[$index].cell_texts | ForEach-Object { $_.ToString() })
        $expectedCells = @($ExpectedRows[$index])
        if (-not (Test-StringArraysEqual -Left $actualCells -Right $expectedCells)) {
            $expectedJoined = $expectedCells -join " | "
            $actualJoined = $actualCells -join " | "
            throw "$Label row $index mismatch. Expected '$expectedJoined', got '$actualJoined'."
        }
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

function Add-CommandLine {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Command,
        [string[]]$Arguments
    )

    [void]$Lines.Add('```powershell')
    $escapedArgs = @()
    foreach ($argument in $Arguments) {
        if ($argument -match "\s") {
            $escapedArgs += "'$argument'"
        } else {
            $escapedArgs += $argument
        }
    }
    [void]$Lines.Add("& .\featherdoc_cli.exe $Command " + ($escapedArgs -join " "))
    [void]$Lines.Add('```')
    [void]$Lines.Add("")
}

function Build-CaseSteps {
    param($CaseConfig, [string]$MutatedDocxPath, [string]$CommandDir)

    $tableIndex = $CaseConfig.target_table_index
    $partArgs = @($CaseConfig.part_args)

    return @(
        [ordered]@{
            id = "01-set-original-left"
            command = "set-template-table-cell-text"
            output = Join-Path $CommandDir "step-01-set-original-left.docx"
            json = Join-Path $CommandDir "step-01-set-original-left.json"
            args = @($tableIndex, "1", "0") + $partArgs + @("--text", $CaseConfig.primary_left)
        },
        [ordered]@{
            id = "02-set-original-right"
            command = "set-template-table-cell-text"
            output = Join-Path $CommandDir "step-02-set-original-right.docx"
            json = Join-Path $CommandDir "step-02-set-original-right.json"
            args = @($tableIndex, "1", "1") + $partArgs + @("--text", $CaseConfig.primary_right)
        },
        [ordered]@{
            id = "03-append-row"
            command = "append-template-table-row"
            output = Join-Path $CommandDir "step-03-append-row.docx"
            json = Join-Path $CommandDir "step-03-append-row.json"
            args = @($tableIndex) + $partArgs
        },
        [ordered]@{
            id = "04-set-appended-left"
            command = "set-template-table-cell-text"
            output = Join-Path $CommandDir "step-04-set-appended-left.docx"
            json = Join-Path $CommandDir "step-04-set-appended-left.json"
            args = @($tableIndex, "2", "0") + $partArgs + @("--text", $CaseConfig.appended_left)
        },
        [ordered]@{
            id = "05-set-appended-right"
            command = "set-template-table-cell-text"
            output = Join-Path $CommandDir "step-05-set-appended-right.docx"
            json = Join-Path $CommandDir "step-05-set-appended-right.json"
            args = @($tableIndex, "2", "1") + $partArgs + @("--text", $CaseConfig.appended_right)
        },
        [ordered]@{
            id = "06-insert-after"
            command = "insert-template-table-row-after"
            output = Join-Path $CommandDir "step-06-insert-after.docx"
            json = Join-Path $CommandDir "step-06-insert-after.json"
            args = @($tableIndex, "1") + $partArgs
        },
        [ordered]@{
            id = "07-set-inserted-left"
            command = "set-template-table-cell-text"
            output = Join-Path $CommandDir "step-07-set-inserted-left.docx"
            json = Join-Path $CommandDir "step-07-set-inserted-left.json"
            args = @($tableIndex, "2", "0") + $partArgs + @("--text", $CaseConfig.inserted_left)
        },
        [ordered]@{
            id = "08-set-inserted-right"
            command = "set-template-table-cell-text"
            output = Join-Path $CommandDir "step-08-set-inserted-right.docx"
            json = Join-Path $CommandDir "step-08-set-inserted-right.json"
            args = @($tableIndex, "2", "1") + $partArgs + @("--text", $CaseConfig.inserted_right)
        },
        [ordered]@{
            id = "09-insert-before-for-remove-check"
            command = "insert-template-table-row-before"
            output = Join-Path $CommandDir "step-09-insert-before-for-remove-check.docx"
            json = Join-Path $CommandDir "step-09-insert-before-for-remove-check.json"
            args = @($tableIndex, "1") + $partArgs
        },
        [ordered]@{
            id = "10-remove-inserted-before"
            command = "remove-template-table-row"
            output = $MutatedDocxPath
            json = Join-Path $CommandDir "step-10-remove-inserted-before.json"
            args = @($tableIndex, "1") + $partArgs
        }
    )
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"

$cases = @(
    [ordered]@{
        id = "section-header-default"
        description = "Mutate the default section-header target table and verify the change only appears on the later odd page."
        part = "section-header"
        part_args = @("--part", "section-header", "--section", "0", "--kind", "default")
        target_table_index = "1"
        baseline_rows = @(
            @("Default Header Final", "Value"),
            @("Pending default header update", "Seed")
        )
        mutated_rows = @(
            @("Default Header Final", "Value"),
            @("Default header visual row", "13"),
            @("Default header inserted", "5"),
            @("Default header appended", "8")
        )
        primary_left = "Default header visual row"
        primary_right = "13"
        inserted_left = "Default header inserted"
        inserted_right = "5"
        appended_left = "Default header appended"
        appended_right = "8"
        visual_pages = @(
            [ordered]@{ number = 1; role = "control-first-page" },
            [ordered]@{ number = 3; role = "target-default-page" }
        )
        expected_visual_cues = @(
            "Page 1 still shows the first-page header control paragraph instead of the default header tables.",
            "Page 3 keeps the blue retained default-header table unchanged and expands the purple default-header target table to four rows with Default header visual row / 13, Default header inserted / 5, and Default header appended / 8.",
            "The page 3 body flow still begins below the header area without overlap or clipped text."
        )
    },
    [ordered]@{
        id = "section-header-even"
        description = "Mutate the even section-header target table and verify the change only appears on page 2."
        part = "section-header"
        part_args = @("--part", "section-header", "--section", "0", "--kind", "even")
        target_table_index = "1"
        baseline_rows = @(
            @("Even Header Final", "Value"),
            @("Pending even header update", "Seed")
        )
        mutated_rows = @(
            @("Even Header Final", "Value"),
            @("Even header visual row", "21"),
            @("Even header inserted", "6"),
            @("Even header appended", "9")
        )
        primary_left = "Even header visual row"
        primary_right = "21"
        inserted_left = "Even header inserted"
        inserted_right = "6"
        appended_left = "Even header appended"
        appended_right = "9"
        visual_pages = @(
            [ordered]@{ number = 2; role = "target-even-page" },
            [ordered]@{ number = 3; role = "control-default-page" }
        )
        expected_visual_cues = @(
            "Page 2 keeps the blue retained even-header table unchanged and expands the peach even-header target table to four rows with Even header visual row / 21, Even header inserted / 6, and Even header appended / 9.",
            "Page 3 still renders the default header tables instead of the even-header tables.",
            "The page 2 body flow still starts below the even header area with no overlap or clipping."
        )
    },
    [ordered]@{
        id = "section-footer-first"
        description = "Mutate the first-page section-footer target table and verify the change only appears on page 1."
        part = "section-footer"
        part_args = @("--part", "section-footer", "--section", "0", "--kind", "first")
        target_table_index = "1"
        baseline_rows = @(
            @("First Footer Final", "Value"),
            @("Pending first footer update", "Seed")
        )
        mutated_rows = @(
            @("First Footer Final", "Value"),
            @("First footer visual row", "15"),
            @("First footer inserted", "4"),
            @("First footer appended", "7")
        )
        primary_left = "First footer visual row"
        primary_right = "15"
        inserted_left = "First footer inserted"
        inserted_right = "4"
        appended_left = "First footer appended"
        appended_right = "7"
        visual_pages = @(
            [ordered]@{ number = 1; role = "target-first-page" },
            [ordered]@{ number = 3; role = "control-default-page" }
        )
        expected_visual_cues = @(
            "Page 1 keeps the blue retained first-footer table unchanged and expands the orange first-footer target table to four rows with First footer visual row / 15, First footer inserted / 4, and First footer appended / 7.",
            "Page 3 still renders the default footer tables instead of the first-page footer tables.",
            "The enlarged first-page footer stays inside the page area and does not visibly overlap the page 1 body paragraphs."
        )
    },
    [ordered]@{
        id = "section-footer-default"
        description = "Mutate the default section-footer target table and verify the change only appears on the later odd page."
        part = "section-footer"
        part_args = @("--part", "section-footer", "--section", "0", "--kind", "default")
        target_table_index = "1"
        baseline_rows = @(
            @("Default Footer Final", "Value"),
            @("Pending default footer update", "Seed")
        )
        mutated_rows = @(
            @("Default Footer Final", "Value"),
            @("Default footer visual row", "17"),
            @("Default footer inserted", "3"),
            @("Default footer appended", "6")
        )
        primary_left = "Default footer visual row"
        primary_right = "17"
        inserted_left = "Default footer inserted"
        inserted_right = "3"
        appended_left = "Default footer appended"
        appended_right = "6"
        visual_pages = @(
            [ordered]@{ number = 1; role = "control-first-page" },
            [ordered]@{ number = 3; role = "target-default-page" }
        )
        expected_visual_cues = @(
            "Page 1 still renders the first-page footer tables instead of the default footer tables.",
            "Page 3 keeps the blue retained default-footer table unchanged and expands the green default-footer target table to four rows with Default footer visual row / 17, Default footer inserted / 3, and Default footer appended / 6.",
            "The enlarged default footer stays inside the page area and does not visibly overlap the page 3 body paragraphs."
        )
    }
)

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$baselineDir = Join-Path $resolvedOutputDir "baseline"
$baselineDocxPath = Join-Path $baselineDir "template_table_cli_section_kind_visual_baseline.docx"
$baselineVisualDir = Join-Path $baselineDir "visual"
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateSelectedPagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$reviewManifestPath = Join-Path $resolvedOutputDir "review_manifest.json"
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"

New-Item -ItemType Directory -Path $baselineDir -Force | Out-Null
if (-not $SkipVisual) {
    New-Item -ItemType Directory -Path $aggregateEvidenceDir -Force | Out-Null
    New-Item -ItemType Directory -Path $aggregateSelectedPagesDir -Force | Out-Null
}

if (-not $SkipBuild) {
    $vcvarsPath = Get-VcvarsPath
    Write-Step "Configuring dedicated build directory $resolvedBuildDir"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"

    Write-Step "Building CLI and section-kind template table visual sample"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_template_table_cli_section_kind_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_template_table_cli_section_kind_visual"

Write-Step "Generating section-kind template table visual baseline via $sampleExecutable"
& $sampleExecutable $baselineDocxPath
if ($LASTEXITCODE -ne 0) {
    throw "Sample execution failed: featherdoc_sample_template_table_cli_section_kind_visual"
}

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    cli_executable = $cliExecutable
    sample_executable = $sampleExecutable
    baseline_docx = $baselineDocxPath
    review_manifest = $reviewManifestPath
    review_checklist = $reviewChecklistPath
    final_review = $finalReviewPath
    cases = @()
}

$reviewManifest = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    summary_json = $summaryPath
    review_checklist = $reviewChecklistPath
    final_review = $finalReviewPath
    cases = @()
}

$aggregateImagePaths = @()
$aggregateLabels = @()

if (-not $SkipVisual) {
    Write-Step "Rendering Word evidence for shared baseline document"
    & $wordSmokeScript `
        -BuildDir $BuildDir `
        -InputDocx $baselineDocxPath `
        -OutputDir $baselineVisualDir `
        -SkipBuild `
        -Dpi $Dpi `
        -KeepWordOpen:$KeepWordOpen.IsPresent `
        -VisibleWord:$VisibleWord.IsPresent
    if ($LASTEXITCODE -ne 0) {
        throw "Visual smoke failed for baseline document."
    }

    foreach ($case in $cases) {
        foreach ($pageConfig in $case.visual_pages) {
            $null = Get-RenderedPagePath `
                -VisualOutputDir $baselineVisualDir `
                -PageNumber ([int]$pageConfig.number) `
                -Label "baseline case '$($case.id)'"
        }
    }
}

foreach ($case in $cases) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    $commandDir = Join-Path $caseDir "commands"
    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $baselineRowsJsonPath = Join-Path $caseDir "baseline_rows.json"
    $mutatedRowsJsonPath = Join-Path $caseDir "mutated_rows.json"
    $mutatedTablesJsonPath = Join-Path $caseDir "mutated_tables.json"
    $commandSequencePath = Join-Path $caseDir "command_sequence.md"
    $mutatedVisualDir = Join-Path $caseDir "mutated-visual"
    $caseContactSheetPath = Join-Path $caseDir "before_after_contact_sheet.png"

    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
    New-Item -ItemType Directory -Path $commandDir -Force | Out-Null

    Write-Step "Inspecting baseline rows for case '$($case.id)'"
    Invoke-CommandCapture `
        -ExecutablePath $cliExecutable `
        -Arguments (@("inspect-template-table-rows", $baselineDocxPath, $case.target_table_index) + @($case.part_args) + @("--json")) `
        -OutputPath $baselineRowsJsonPath `
        -FailureMessage "Failed to inspect baseline template rows for case '$($case.id)'."

    Assert-TableRows `
        -RowsJsonPath $baselineRowsJsonPath `
        -ExpectedRows $case.baseline_rows `
        -Label "baseline $($case.id) table"

    $steps = Build-CaseSteps -CaseConfig $case -MutatedDocxPath $mutatedDocxPath -CommandDir $commandDir
    $commandLines = [System.Collections.Generic.List[string]]::new()
    [void]$commandLines.Add("# Template table CLI mutation sequence for $($case.id)")
    [void]$commandLines.Add("")
    [void]$commandLines.Add("Target part: $($case.part), table index `"$($case.target_table_index)`".")
    [void]$commandLines.Add("Selected pages: $(($case.visual_pages | ForEach-Object { ('page-{0:D2} ({1})' -f ([int]$_.number), $_.role) }) -join ', ')")
    [void]$commandLines.Add("")

    $currentInput = $baselineDocxPath
    foreach ($step in $steps) {
        Write-Step "Running $($step.command) for case '$($case.id)' step '$($step.id)'"
        $arguments = @($step.command, $currentInput) + @($step.args) + @("--output", $step.output, "--json")
        Invoke-CommandCapture `
            -ExecutablePath $cliExecutable `
            -Arguments $arguments `
            -OutputPath $step.json `
            -FailureMessage "Failed during template table CLI mutation step '$($step.id)' for case '$($case.id)'."

        [void]$commandLines.Add("## $($step.id)")
        Add-CommandLine -Lines $commandLines -Command $step.command -Arguments (@("<input.docx>") + @($step.args) + @("--output", "<output.docx>", "--json"))
        $currentInput = $step.output
    }

    $commandLines | Set-Content -Path $commandSequencePath -Encoding UTF8

    Write-Step "Inspecting mutated rows for case '$($case.id)'"
    Invoke-CommandCapture `
        -ExecutablePath $cliExecutable `
        -Arguments (@("inspect-template-tables", $mutatedDocxPath) + @($case.part_args) + @("--json")) `
        -OutputPath $mutatedTablesJsonPath `
        -FailureMessage "Failed to inspect mutated template tables for case '$($case.id)'."

    Invoke-CommandCapture `
        -ExecutablePath $cliExecutable `
        -Arguments (@("inspect-template-table-rows", $mutatedDocxPath, $case.target_table_index) + @($case.part_args) + @("--json")) `
        -OutputPath $mutatedRowsJsonPath `
        -FailureMessage "Failed to inspect mutated template rows for case '$($case.id)'."

    Assert-TableRows `
        -RowsJsonPath $mutatedRowsJsonPath `
        -ExpectedRows $case.mutated_rows `
        -Label "mutated $($case.id) table"

    $caseSummary = [ordered]@{
        id = $case.id
        description = $case.description
        part = $case.part
        target_table_index = [int]$case.target_table_index
        visual_page_plan = @($case.visual_pages)
        expected_visual_cues = @($case.expected_visual_cues)
        baseline_rows_json = $baselineRowsJsonPath
        baseline_rows_expected = $case.baseline_rows
        mutated_docx = $mutatedDocxPath
        mutated_tables_json = $mutatedTablesJsonPath
        mutated_rows_json = $mutatedRowsJsonPath
        mutated_rows_expected = $case.mutated_rows
        command_sequence = $commandSequencePath
        command_outputs_dir = $commandDir
    }

    if (-not $SkipVisual) {
        Write-Step "Rendering Word evidence for case '$($case.id)'"
        & $wordSmokeScript `
            -BuildDir $BuildDir `
            -InputDocx $mutatedDocxPath `
            -OutputDir $mutatedVisualDir `
            -SkipBuild `
            -Dpi $Dpi `
            -KeepWordOpen:$KeepWordOpen.IsPresent `
            -VisibleWord:$VisibleWord.IsPresent
        if ($LASTEXITCODE -ne 0) {
            throw "Visual smoke failed for mutated document in case '$($case.id)'."
        }

        $renderPython = Ensure-RenderPython -RepoRoot $repoRoot
        $caseImagePaths = @()
        $caseLabels = @()
        $selectedPageArtifacts = @()
        foreach ($pageConfig in $case.visual_pages) {
            $pageNumber = [int]$pageConfig.number
            $role = $pageConfig.role
            $baselinePage = Get-RenderedPagePath `
                -VisualOutputDir $baselineVisualDir `
                -PageNumber $pageNumber `
                -Label "baseline case '$($case.id)'"
            $mutatedPage = Get-RenderedPagePath `
                -VisualOutputDir $mutatedVisualDir `
                -PageNumber $pageNumber `
                -Label "mutated case '$($case.id)'"

            $aggregateBaselinePage = Join-Path $aggregateSelectedPagesDir ("{0}-baseline-page-{1:D2}.png" -f $case.id, $pageNumber)
            $aggregateMutatedPage = Join-Path $aggregateSelectedPagesDir ("{0}-mutated-page-{1:D2}.png" -f $case.id, $pageNumber)
            Copy-Item -Path $baselinePage -Destination $aggregateBaselinePage -Force
            Copy-Item -Path $mutatedPage -Destination $aggregateMutatedPage -Force

            $baselineLabel = "$($case.id)-baseline-page-$('{0:D2}' -f $pageNumber)-$role"
            $mutatedLabel = "$($case.id)-mutated-page-$('{0:D2}' -f $pageNumber)-$role"
            $caseImagePaths += $aggregateBaselinePage
            $caseImagePaths += $aggregateMutatedPage
            $caseLabels += $baselineLabel
            $caseLabels += $mutatedLabel
            $aggregateImagePaths += $aggregateBaselinePage
            $aggregateImagePaths += $aggregateMutatedPage
            $aggregateLabels += $baselineLabel
            $aggregateLabels += $mutatedLabel

            $selectedPageArtifacts += [ordered]@{
                page_number = $pageNumber
                role = $role
                baseline_page = $aggregateBaselinePage
                mutated_page = $aggregateMutatedPage
            }
        }

        Write-Step "Building before/after contact sheet for case '$($case.id)'"
        $caseContactSheetArgs = @(
            $contactSheetScript
            "--output"
            $caseContactSheetPath
            "--columns"
            "2"
            "--thumb-width"
            "420"
            "--images"
        ) + $caseImagePaths + @("--labels") + $caseLabels
        & $renderPython @caseContactSheetArgs
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to build case contact sheet for '$($case.id)'."
        }

        $caseSummary.visual_artifacts = [ordered]@{
            baseline_visual_output_dir = $baselineVisualDir
            mutated_visual_output_dir = $mutatedVisualDir
            selected_pages = $selectedPageArtifacts
            before_after_contact_sheet = $caseContactSheetPath
        }
    }

    $summary.cases += $caseSummary
    $reviewManifest.cases += $caseSummary
}

if (-not $SkipVisual) {
    $renderPython = Ensure-RenderPython -RepoRoot $repoRoot
    Write-Step "Building aggregate before/after contact sheet"
    $contactSheetArgs = @(
        $contactSheetScript
        "--output"
        $aggregateContactSheetPath
        "--columns"
        "2"
        "--thumb-width"
        "420"
        "--images"
    ) + $aggregateImagePaths + @("--labels") + $aggregateLabels

    & $renderPython @contactSheetArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build aggregate before/after contact sheet."
    }

    $summary.aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregateSelectedPagesDir
        contact_sheet = $aggregateContactSheetPath
        shared_baseline_visual_output_dir = $baselineVisualDir
    }
    $reviewManifest.aggregate_evidence = $summary.aggregate_evidence
}

($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8
($reviewManifest | ConvertTo-Json -Depth 8) | Set-Content -Path $reviewManifestPath -Encoding UTF8

$reviewChecklistLines = [System.Collections.Generic.List[string]]::new()
[void]$reviewChecklistLines.Add("# Template table CLI section-kind visual regression checklist")
[void]$reviewChecklistLines.Add("")
if (-not $SkipVisual) {
    [void]$reviewChecklistLines.Add("- Start with the aggregate before/after contact sheet:")
    [void]$reviewChecklistLines.Add("  $aggregateContactSheetPath")
    [void]$reviewChecklistLines.Add("- Then inspect each case's case-specific contact sheet and its selected page PNGs.")
    [void]$reviewChecklistLines.Add("")
}
foreach ($case in $cases) {
    [void]$reviewChecklistLines.Add("- `"$($case.id)`":")
    if (-not $SkipVisual) {
        $pageLabels = @($case.visual_pages | ForEach-Object {
            "page-{0:D2} ({1})" -f ([int]$_.number), $_.role
        })
        [void]$reviewChecklistLines.Add("  - Review pages: $($pageLabels -join ', ')")
    }
    foreach ($cue in $case.expected_visual_cues) {
        [void]$reviewChecklistLines.Add("  - $cue")
    }
}
[void]$reviewChecklistLines.Add("")
[void]$reviewChecklistLines.Add("Artifacts:")
[void]$reviewChecklistLines.Add("")
[void]$reviewChecklistLines.Add("- Summary JSON: $summaryPath")
[void]$reviewChecklistLines.Add("- Review manifest: $reviewManifestPath")
[void]$reviewChecklistLines.Add("- Shared baseline DOCX: $baselineDocxPath")
foreach ($caseSummary in $summary.cases) {
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) mutated rows JSON: $($caseSummary.mutated_rows_json)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) command sequence: $($caseSummary.command_sequence)")
}
$reviewChecklistLines | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReviewLines = [System.Collections.Generic.List[string]]::new()
[void]$finalReviewLines.Add("# Template table CLI section-kind visual final review")
[void]$finalReviewLines.Add("")
[void]$finalReviewLines.Add("- Generated at: $(Get-Date -Format s)")
[void]$finalReviewLines.Add("- Summary JSON: $summaryPath")
[void]$finalReviewLines.Add("- Review manifest: $reviewManifestPath")
[void]$finalReviewLines.Add("- Visual enabled: $((-not $SkipVisual.IsPresent).ToString().ToLowerInvariant())")
if (-not $SkipVisual) {
    [void]$finalReviewLines.Add("- Aggregate contact sheet: $aggregateContactSheetPath")
}
[void]$finalReviewLines.Add("")
[void]$finalReviewLines.Add("## Verdict")
[void]$finalReviewLines.Add("")
[void]$finalReviewLines.Add("- Overall verdict:")
[void]$finalReviewLines.Add("- Notes:")
[void]$finalReviewLines.Add("")
[void]$finalReviewLines.Add("## Case findings")
[void]$finalReviewLines.Add("")
foreach ($case in $cases) {
    [void]$finalReviewLines.Add("- $($case.id):")
    [void]$finalReviewLines.Add("  Verdict:")
    [void]$finalReviewLines.Add("  Notes:")
    if (-not $SkipVisual) {
        [void]$finalReviewLines.Add("  Contact sheet: $(Join-Path (Join-Path $resolvedOutputDir $case.id) 'before_after_contact_sheet.png')")
    }
    [void]$finalReviewLines.Add("  Selected pages: $(($case.visual_pages | ForEach-Object { ('page-{0:D2} ({1})' -f ([int]$_.number), $_.role) }) -join ', ')")
    [void]$finalReviewLines.Add("")
}
$finalReviewLines | Set-Content -Path $finalReviewPath -Encoding UTF8

Write-Step "Completed template table CLI section-kind visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Review manifest: $reviewManifestPath"
Write-Host "Review checklist: $reviewChecklistPath"
Write-Host "Final review: $finalReviewPath"
Write-Host "Shared baseline DOCX: $baselineDocxPath"
foreach ($caseSummary in $summary.cases) {
    Write-Host "$($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)"
}
if (-not $SkipVisual) {
    Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
}
