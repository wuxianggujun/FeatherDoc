param(
    [string]$BuildDir = "build-template-table-cli-column-visual-nmake",
    [string]$OutputDir = "output/template-table-cli-column-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[template-table-cli-column-visual] $Message"
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

    $steps = [System.Collections.Generic.List[object]]::new()
    $partArgs = @($CaseConfig.part_args)
    $postSteps = @($CaseConfig.post_steps)

    $firstOutputPath = $MutatedDocxPath
    if ($postSteps.Count -gt 0) {
        $firstOutputPath = Join-Path $CommandDir "step-01-$($CaseConfig.mutation_step_id).docx"
    }

    [void]$steps.Add([ordered]@{
            id = "01-$($CaseConfig.mutation_step_id)"
            command = $CaseConfig.command
            output = $firstOutputPath
            json = Join-Path $CommandDir "step-01-$($CaseConfig.mutation_step_id).json"
            args = @(
                $CaseConfig.target_table_index
                $CaseConfig.row_index
                $CaseConfig.cell_index
            ) + $partArgs
        })

    for ($index = 0; $index -lt $postSteps.Count; $index++) {
        $postStep = $postSteps[$index]
        $stepNumber = "{0:D2}" -f ($index + 2)
        $stepSlug = $postStep.id
        $isLastStep = $index -eq ($postSteps.Count - 1)
        $outputPath = if ($isLastStep) {
            $MutatedDocxPath
        } else {
            Join-Path $CommandDir "step-$stepNumber-$stepSlug.docx"
        }

        [void]$steps.Add([ordered]@{
                id = "$stepNumber-$stepSlug"
                command = "set-template-table-cell-text"
                output = $outputPath
                json = Join-Path $CommandDir "step-$stepNumber-$stepSlug.json"
                args = @(
                    $CaseConfig.target_table_index
                    $postStep.row_index
                    $postStep.cell_index
                ) + $partArgs + @("--text", $postStep.text)
            })
    }

    return @($steps)
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"

$cases = @(
    [ordered]@{
        id = "section-header"
        sample_case_id = "section-header-insert-before"
        description = "Insert one leading column into the purple section-header target table while keeping the retained blue table and body text stable."
        command = "insert-template-table-column-before"
        mutation_step_id = "insert-before"
        part = "section-header"
        part_args = @("--part", "section-header", "--section", "0", "--kind", "default")
        target_table_index = "1"
        row_index = "0"
        cell_index = "0"
        baseline_rows = @(
            @("Header Final", "Value"),
            @("Keep header seed", "Seed detail")
        )
        mutated_rows = @(
            @("Priority", "Header Final", "Value"),
            @("Elevated", "Keep header seed", "Seed detail")
        )
        post_steps = @(
            [ordered]@{
                id = "set-inserted-header"
                row_index = "0"
                cell_index = "0"
                text = "Priority"
            },
            [ordered]@{
                id = "set-inserted-body"
                row_index = "1"
                cell_index = "0"
                text = "Elevated"
            }
        )
        expected_visual_cues = @(
            "The purple section-header target table grows from two columns to three columns with a new leading Priority / Elevated column.",
            "The retained blue section-header table stays unchanged and is not accidentally mutated.",
            "Body text still starts below the header tables with no overlap or clipping."
        )
    },
    [ordered]@{
        id = "section-footer"
        sample_case_id = "section-footer-insert-after"
        description = "Insert one middle column into the orange section-footer target table while keeping the footer safely inside the page footer area."
        command = "insert-template-table-column-after"
        mutation_step_id = "insert-after"
        part = "section-footer"
        part_args = @("--part", "section-footer", "--section", "0", "--kind", "default")
        target_table_index = "0"
        row_index = "0"
        cell_index = "0"
        baseline_rows = @(
            @("Footer Final", "Value"),
            @("Keep footer seed", "Seed detail")
        )
        mutated_rows = @(
            @("Footer Final", "Status", "Value"),
            @("Keep footer seed", "Pinned", "Seed detail")
        )
        post_steps = @(
            [ordered]@{
                id = "set-inserted-header"
                row_index = "0"
                cell_index = "1"
                text = "Status"
            },
            [ordered]@{
                id = "set-inserted-body"
                row_index = "1"
                cell_index = "1"
                text = "Pinned"
            }
        )
        expected_visual_cues = @(
            "The orange section-footer target table grows from two columns to three columns with a new middle Status / Pinned column.",
            "The footer intro paragraph remains above the table and the footer content stays inside the footer area.",
            "The body content does not visibly collide with the expanded footer table."
        )
    },
    [ordered]@{
        id = "body"
        sample_case_id = "body-remove-column"
        description = "Remove the temporary middle column from the green body target table while keeping the retained blue table and surrounding paragraphs stable."
        command = "remove-template-table-column"
        mutation_step_id = "remove-column"
        part = "body"
        part_args = @("--part", "body")
        target_table_index = "1"
        row_index = "0"
        cell_index = "1"
        baseline_rows = @(
            @("Body Final", "Remove me", "Value"),
            @("Keep body seed", "Drop this cell", "Seed detail")
        )
        mutated_rows = @(
            @("Body Final", "Value"),
            @("Keep body seed", "Seed detail")
        )
        post_steps = @()
        expected_visual_cues = @(
            "The green body target table shrinks from three columns back to two columns after the yellow middle column is removed.",
            "The retained blue body table above the target stays unchanged.",
            "The paragraphs around the target table remain readable with stable spacing."
        )
    }
)

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateFirstPagesDir = Join-Path $aggregateEvidenceDir "first-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$reviewManifestPath = Join-Path $resolvedOutputDir "review_manifest.json"
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"

if (-not $SkipVisual) {
    New-Item -ItemType Directory -Path $aggregateEvidenceDir -Force | Out-Null
    New-Item -ItemType Directory -Path $aggregateFirstPagesDir -Force | Out-Null
}

if (-not $SkipBuild) {
    $vcvarsPath = Get-VcvarsPath
    Write-Step "Configuring dedicated build directory $resolvedBuildDir"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"

    Write-Step "Building CLI and column visual sample"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_template_table_cli_column_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_template_table_cli_column_visual"
$renderPython = $null
if (-not $SkipVisual) {
    $renderPython = Ensure-RenderPython -RepoRoot $repoRoot
}

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    cli_executable = $cliExecutable
    sample_executable = $sampleExecutable
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

foreach ($case in $cases) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    $commandDir = Join-Path $caseDir "commands"
    $baselineDocxPath = Join-Path $caseDir "$($case.id)-baseline.docx"
    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $baselineRowsJsonPath = Join-Path $caseDir "baseline_rows.json"
    $mutatedRowsJsonPath = Join-Path $caseDir "mutated_rows.json"
    $mutatedTablesJsonPath = Join-Path $caseDir "mutated_tables.json"
    $commandSequencePath = Join-Path $caseDir "command_sequence.md"
    $baselineVisualDir = Join-Path $caseDir "baseline-visual"
    $mutatedVisualDir = Join-Path $caseDir "mutated-visual"
    $caseContactSheetPath = Join-Path $caseDir "before_after_contact_sheet.png"
    $baselineVisualOutputArg = Join-Path $OutputDir (Join-Path $case.id "baseline-visual")
    $mutatedVisualOutputArg = Join-Path $OutputDir (Join-Path $case.id "mutated-visual")

    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
    New-Item -ItemType Directory -Path $commandDir -Force | Out-Null

    Write-Step "Generating baseline DOCX for case '$($case.id)'"
    & $sampleExecutable $case.sample_case_id $baselineDocxPath
    if ($LASTEXITCODE -ne 0) {
        throw "Sample execution failed for case '$($case.id)'."
    }

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
    [void]$commandLines.Add("# Template table CLI column command sequence for $($case.id)")
    [void]$commandLines.Add("")
    [void]$commandLines.Add("Sample case id: `"$($case.sample_case_id)`"")
    [void]$commandLines.Add("Target part: $($case.part), table index `"$($case.target_table_index)`".")
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
        sample_case_id = $case.sample_case_id
        description = $case.description
        command = $case.command
        part = $case.part
        target_table_index = [int]$case.target_table_index
        row_index = [int]$case.row_index
        cell_index = [int]$case.cell_index
        expected_visual_cues = @($case.expected_visual_cues)
        baseline_docx = $baselineDocxPath
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
        Write-Step "Rendering baseline Word evidence for case '$($case.id)'"
        & $wordSmokeScript `
            -BuildDir $BuildDir `
            -InputDocx $baselineDocxPath `
            -OutputDir $baselineVisualOutputArg `
            -SkipBuild `
            -Dpi $Dpi `
            -KeepWordOpen:$KeepWordOpen.IsPresent `
            -VisibleWord:$VisibleWord.IsPresent
        if ($LASTEXITCODE -ne 0) {
            throw "Visual smoke failed for baseline document in case '$($case.id)'."
        }

        Write-Step "Rendering mutated Word evidence for case '$($case.id)'"
        & $wordSmokeScript `
            -BuildDir $BuildDir `
            -InputDocx $mutatedDocxPath `
            -OutputDir $mutatedVisualOutputArg `
            -SkipBuild `
            -Dpi $Dpi `
            -KeepWordOpen:$KeepWordOpen.IsPresent `
            -VisibleWord:$VisibleWord.IsPresent
        if ($LASTEXITCODE -ne 0) {
            throw "Visual smoke failed for mutated document in case '$($case.id)'."
        }

        $baselineFirstPage = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
        $mutatedFirstPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
        if (-not (Test-Path $baselineFirstPage)) {
            throw "Missing baseline first-page PNG for case '$($case.id)': $baselineFirstPage"
        }
        if (-not (Test-Path $mutatedFirstPage)) {
            throw "Missing mutated first-page PNG for case '$($case.id)': $mutatedFirstPage"
        }

        $aggregateBaselinePage = Join-Path $aggregateFirstPagesDir "$($case.id)-baseline-page-01.png"
        $aggregateMutatedPage = Join-Path $aggregateFirstPagesDir "$($case.id)-mutated-page-01.png"
        Copy-Item -Path $baselineFirstPage -Destination $aggregateBaselinePage -Force
        Copy-Item -Path $mutatedFirstPage -Destination $aggregateMutatedPage -Force

        Write-Step "Building before/after contact sheet for case '$($case.id)'"
        & $renderPython $contactSheetScript `
            --output $caseContactSheetPath `
            --columns 2 `
            --thumb-width 420 `
            --images $aggregateBaselinePage $aggregateMutatedPage `
            --labels "$($case.id)-baseline" "$($case.id)-mutated"
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to build case contact sheet for '$($case.id)'."
        }

        $aggregateImagePaths += $aggregateBaselinePage
        $aggregateImagePaths += $aggregateMutatedPage
        $aggregateLabels += "$($case.id)-baseline"
        $aggregateLabels += "$($case.id)-mutated"

        $caseSummary.visual_artifacts = [ordered]@{
            baseline_visual_output_dir = $baselineVisualDir
            baseline_first_page = $aggregateBaselinePage
            mutated_visual_output_dir = $mutatedVisualDir
            mutated_first_page = $aggregateMutatedPage
            before_after_contact_sheet = $caseContactSheetPath
        }
    }

    $summary.cases += $caseSummary
    $reviewManifest.cases += $caseSummary
}

if (-not $SkipVisual) {
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
        first_pages_dir = $aggregateFirstPagesDir
        contact_sheet = $aggregateContactSheetPath
    }
    $reviewManifest.aggregate_evidence = $summary.aggregate_evidence
}

($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8
($reviewManifest | ConvertTo-Json -Depth 8) | Set-Content -Path $reviewManifestPath -Encoding UTF8

$reviewChecklistLines = [System.Collections.Generic.List[string]]::new()
[void]$reviewChecklistLines.Add("# Template table CLI column visual regression checklist")
[void]$reviewChecklistLines.Add("")
if (-not $SkipVisual) {
    [void]$reviewChecklistLines.Add("- Start with the aggregate before/after contact sheet:")
    [void]$reviewChecklistLines.Add("  $aggregateContactSheetPath")
    [void]$reviewChecklistLines.Add("- Then inspect each case's case-specific contact sheet and `page-01.png`.")
    [void]$reviewChecklistLines.Add("")
}
foreach ($case in $cases) {
    [void]$reviewChecklistLines.Add("- `"$($case.id)`":")
    foreach ($cue in $case.expected_visual_cues) {
        [void]$reviewChecklistLines.Add("  - $cue")
    }
}
[void]$reviewChecklistLines.Add("")
[void]$reviewChecklistLines.Add("Artifacts:")
[void]$reviewChecklistLines.Add("")
[void]$reviewChecklistLines.Add("- Summary JSON: $summaryPath")
[void]$reviewChecklistLines.Add("- Review manifest: $reviewManifestPath")
foreach ($caseSummary in $summary.cases) {
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) baseline DOCX: $($caseSummary.baseline_docx)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) mutated tables JSON: $($caseSummary.mutated_tables_json)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) command sequence: $($caseSummary.command_sequence)")
}
$reviewChecklistLines | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReviewLines = [System.Collections.Generic.List[string]]::new()
[void]$finalReviewLines.Add("# Template table CLI column final review")
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
    [void]$finalReviewLines.Add("")
}
$finalReviewLines | Set-Content -Path $finalReviewPath -Encoding UTF8

Write-Step "Completed template table CLI column visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Review manifest: $reviewManifestPath"
Write-Host "Review checklist: $reviewChecklistPath"
Write-Host "Final review: $finalReviewPath"
foreach ($caseSummary in $summary.cases) {
    Write-Host "$($caseSummary.id) baseline DOCX: $($caseSummary.baseline_docx)"
    Write-Host "$($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)"
}
if (-not $SkipVisual) {
    Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
}
