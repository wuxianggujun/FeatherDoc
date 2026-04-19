param(
    [string]$BuildDir = "build-template-table-cli-bookmark-visual-nmake",
    [string]$OutputDir = "output/template-table-cli-bookmark-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[template-table-cli-bookmark-visual] $Message"
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

function Write-Utf8NoBomTextFile {
    param(
        [string]$Path,
        [string]$Content
    )

    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Content, $utf8NoBom)
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
    [void]$Lines.Add("& .\featherdoc_cli.exe " + ($escapedArgs -join " "))
    [void]$Lines.Add('```')
    [void]$Lines.Add("")
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"

$baselineRows = @(
    @("Region", "Qty", "Amount"),
    @("North seed", "10", "100"),
    @("South seed", "20", "200")
)

$retainedBaselineRows = @(
    @("Retained", "Qty", "Amount"),
    @("keep-left", "4", "40")
)

$cases = @(
    [ordered]@{
        id = "row-texts-before-bookmark"
        description = "Replace two full data rows by selecting the target table from a bookmark paragraph immediately before it."
        command = "set-template-table-from-json"
        command_selector_args = @("--bookmark", "target_before_table")
        patch_json = @'
{
  "mode": "rows",
  "start_row": 1,
  "rows": [
    ["North ready", 12, 120],
    ["South ready", 18, 180]
  ]
}
'@
        row_assertions = @(
            [ordered]@{
                label = "target-before-table"
                inspect_args = @("--bookmark", "target_before_table")
                expected_rows = @(
                    @("Region", "Qty", "Amount"),
                    @("North ready", "12", "120"),
                    @("South ready", "18", "180")
                )
            }
        )
        expected_visual_cues = @(
            "The blue retained table remains unchanged above the target bookmark paragraph.",
            "The green target table shows North ready / 12 / 120 and South ready / 18 / 180 across the full two data rows.",
            "The paragraph below the target table stays readable with no overlap, clipping, or border loss."
        )
    },
    [ordered]@{
        id = "row-texts-after-text-header-selector"
        description = "Replace two full data rows by selecting the target table from the paragraph text immediately before it and the header row cells."
        command = "set-template-table-from-json"
        command_selector_args = @(
            "--after-text", "Bookmark paragraph immediately before the target table.",
            "--header-cell", "Region",
            "--header-cell", "Qty",
            "--header-cell", "Amount"
        )
        patch_json = @'
{
  "mode": "rows",
  "start_row": 1,
  "rows": [
    ["North selector", 22, 220],
    ["South selector", 28, 280]
  ]
}
'@
        row_assertions = @(
            [ordered]@{
                label = "target-after-text-header"
                inspect_args = @(
                    "--after-text", "Bookmark paragraph immediately before the target table.",
                    "--header-cell", "Region",
                    "--header-cell", "Qty",
                    "--header-cell", "Amount"
                )
                expected_rows = @(
                    @("Region", "Qty", "Amount"),
                    @("North selector", "22", "220"),
                    @("South selector", "28", "280")
                )
            }
        )
        expected_visual_cues = @(
            "The blue retained table above remains unchanged, confirming the text-and-header selector did not hit the wrong table.",
            "The green target table updates to North selector / 22 / 220 and South selector / 28 / 280 across both data rows.",
            "The paragraph immediately before the target table remains visible and the table keeps intact borders, widths, and spacing."
        )
    },
    [ordered]@{
        id = "cell-block-texts-inside-bookmark"
        description = "Replace only the Qty/Amount rectangle by selecting the target table from a bookmark inside the first data cell."
        command = "set-template-table-from-json"
        command_selector_args = @("--bookmark", "target_inside_table")
        patch_json = @'
{
  "command": "set-template-table-cell-block-texts",
  "start_row_index": 1,
  "start_cell_index": 1,
  "row_count": 2,
  "rows": [
    [12, 120],
    [18, 180]
  ]
}
'@
        row_assertions = @(
            [ordered]@{
                label = "target-inside-table"
                inspect_args = @("--bookmark", "target_inside_table")
                expected_rows = @(
                    @("Region", "Qty", "Amount"),
                    @("North seed", "12", "120"),
                    @("South seed", "18", "180")
                )
            }
        )
        expected_visual_cues = @(
            "The left Region column keeps North seed and South seed unchanged.",
            "Only the Qty and Amount cells in the two data rows update to 12 / 120 and 18 / 180.",
            "The target table keeps the same layout, widths, and visible borders after the inside-table bookmark mutation."
        )
    },
    [ordered]@{
        id = "batch-table-and-bookmark"
        description = "Replace the retained table by table index and the target table by bookmark in one batch command."
        command = "set-template-tables-from-json"
        command_selector_args = @()
        patch_json = @'
{
  "operations": [
    {
      "table_index": 0,
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["keep-batch", 6, 60]
      ]
    },
    {
      "bookmark": "target_before_table",
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["North batch", 14, 140],
        ["South batch", 16, 160]
      ]
    }
  ]
}
'@
        row_assertions = @(
            [ordered]@{
                label = "retained-table-index-0"
                inspect_args = @("0")
                expected_rows = @(
                    @("Retained", "Qty", "Amount"),
                    @("keep-batch", "6", "60")
                )
            },
            [ordered]@{
                label = "target-before-table"
                inspect_args = @("--bookmark", "target_before_table")
                expected_rows = @(
                    @("Region", "Qty", "Amount"),
                    @("North batch", "14", "140"),
                    @("South batch", "16", "160")
                )
            }
        )
        expected_visual_cues = @(
            "The retained table near the top updates its single data row to keep-batch / 6 / 60.",
            "The target table below the bookmark updates both data rows to North batch / 14 / 140 and South batch / 16 / 160.",
            "Both modified tables keep readable text, intact borders, and stable spacing on the same page after one batch mutation."
        )
    }
)

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$baselineDir = Join-Path $resolvedOutputDir "baseline"
$baselineDocxPath = Join-Path $baselineDir "template_table_cli_bookmark_visual_baseline.docx"
$baselineRowsJsonPath = Join-Path $baselineDir "baseline_rows.json"
$baselineVisualDir = Join-Path $baselineDir "visual"
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateFirstPagesDir = Join-Path $aggregateEvidenceDir "first-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$reviewManifestPath = Join-Path $resolvedOutputDir "review_manifest.json"
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"

New-Item -ItemType Directory -Path $baselineDir -Force | Out-Null
if (-not $SkipVisual) {
    New-Item -ItemType Directory -Path $aggregateEvidenceDir -Force | Out-Null
    New-Item -ItemType Directory -Path $aggregateFirstPagesDir -Force | Out-Null
}

if (-not $SkipBuild) {
    $vcvarsPath = Get-VcvarsPath
    Write-Step "Configuring dedicated build directory $resolvedBuildDir"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"

    Write-Step "Building CLI and bookmark visual sample"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_template_table_cli_bookmark_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_template_table_cli_bookmark_visual"

Write-Step "Generating bookmark visual baseline via $sampleExecutable"
& $sampleExecutable $baselineDocxPath
if ($LASTEXITCODE -ne 0) {
    throw "Sample execution failed: featherdoc_sample_template_table_cli_bookmark_visual"
}

Write-Step "Inspecting shared baseline rows"
Invoke-CommandCapture `
    -ExecutablePath $cliExecutable `
    -Arguments @("inspect-template-table-rows", $baselineDocxPath, "--bookmark", "target_before_table", "--json") `
    -OutputPath $baselineRowsJsonPath `
    -FailureMessage "Failed to inspect baseline template rows."

Assert-TableRows `
    -RowsJsonPath $baselineRowsJsonPath `
    -ExpectedRows $baselineRows `
    -Label "shared baseline table"

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    cli_executable = $cliExecutable
    sample_executable = $sampleExecutable
    baseline_docx = $baselineDocxPath
    baseline_rows_json = $baselineRowsJsonPath
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

$baselineFirstPage = $null
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

    $baselineFirstPage = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
    if (-not (Test-Path $baselineFirstPage)) {
        throw "Missing baseline first-page PNG: $baselineFirstPage"
    }
}

foreach ($case in $cases) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    $patchJsonPath = Join-Path $caseDir "patch.json"
    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $mutationJsonPath = Join-Path $caseDir "mutation.json"
    $mutatedRowsDir = Join-Path $caseDir "mutated-rows"
    $commandSequencePath = Join-Path $caseDir "command_sequence.md"
    $mutatedVisualDir = Join-Path $caseDir "mutated-visual"
    $caseContactSheetPath = Join-Path $caseDir "before_after_contact_sheet.png"

    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
    New-Item -ItemType Directory -Path $mutatedRowsDir -Force | Out-Null

    $commandLines = [System.Collections.Generic.List[string]]::new()
    [void]$commandLines.Add("# Template table bookmark CLI mutation sequence for $($case.id)")
    [void]$commandLines.Add("")
    $selectorDescription = if (@($case.command_selector_args).Count -gt 0) {
        (@($case.command_selector_args) | ForEach-Object { $_.ToString() }) -join " "
    } else {
        "<none>"
    }
    [void]$commandLines.Add("Command selector arguments: $selectorDescription")
    [void]$commandLines.Add("Patch JSON: `"$patchJsonPath`".")
    [void]$commandLines.Add("")
    [void]$commandLines.Add('```json')
    [void]$commandLines.Add($case.patch_json.Trim())
    [void]$commandLines.Add('```')
    [void]$commandLines.Add("")

    Write-Utf8NoBomTextFile -Path $patchJsonPath -Content $case.patch_json.Trim()

    $mutationArguments = @(
        $case.command,
        $baselineDocxPath
    ) + @($case.command_selector_args) + @(
        "--patch-file",
        $patchJsonPath,
        "--output",
        $mutatedDocxPath,
        "--json"
    )

    Write-Step "Running $($case.command) for case '$($case.id)'"
    Invoke-CommandCapture `
        -ExecutablePath $cliExecutable `
        -Arguments $mutationArguments `
        -OutputPath $mutationJsonPath `
        -FailureMessage "Failed during bookmark visual mutation for case '$($case.id)'."

    Add-CommandLine -Lines $commandLines -Arguments $mutationArguments
    $rowAssertionSummaries = @()
    foreach ($rowAssertion in $case.row_assertions) {
        $mutatedRowsJsonPath = Join-Path $mutatedRowsDir "$($rowAssertion.label).json"
        $inspectArguments = @(
            "inspect-template-table-rows",
            $mutatedDocxPath
        ) + @($rowAssertion.inspect_args) + @("--json")

        Write-Step "Inspecting mutated rows for case '$($case.id)' assertion '$($rowAssertion.label)'"
        Invoke-CommandCapture `
            -ExecutablePath $cliExecutable `
            -Arguments $inspectArguments `
            -OutputPath $mutatedRowsJsonPath `
            -FailureMessage "Failed to inspect mutated template rows for case '$($case.id)' assertion '$($rowAssertion.label)'."

        Assert-TableRows `
            -RowsJsonPath $mutatedRowsJsonPath `
            -ExpectedRows $rowAssertion.expected_rows `
            -Label "mutated $($case.id) assertion $($rowAssertion.label)"

        Add-CommandLine -Lines $commandLines -Arguments $inspectArguments
        $rowAssertionSummaries += [ordered]@{
            label = $rowAssertion.label
            inspect_args = @($rowAssertion.inspect_args)
            expected_rows = @($rowAssertion.expected_rows)
            rows_json = $mutatedRowsJsonPath
        }
    }

    $commandLines | Set-Content -Path $commandSequencePath -Encoding UTF8

    $caseSummary = [ordered]@{
        id = $case.id
        description = $case.description
        command = $case.command
        command_selector_args = @($case.command_selector_args)
        patch_json = $patchJsonPath
        row_assertions = $rowAssertionSummaries
        mutated_docx = $mutatedDocxPath
        mutation_json = $mutationJsonPath
        mutated_rows_dir = $mutatedRowsDir
        command_sequence = $commandSequencePath
        expected_visual_cues = @($case.expected_visual_cues)
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

        $mutatedFirstPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
        if (-not (Test-Path $mutatedFirstPage)) {
            throw "Missing mutated first-page PNG for case '$($case.id)': $mutatedFirstPage"
        }

        $aggregateBaselinePage = Join-Path $aggregateFirstPagesDir "$($case.id)-baseline-page-01.png"
        $aggregateMutatedPage = Join-Path $aggregateFirstPagesDir "$($case.id)-mutated-page-01.png"
        Copy-Item -Path $baselineFirstPage -Destination $aggregateBaselinePage -Force
        Copy-Item -Path $mutatedFirstPage -Destination $aggregateMutatedPage -Force

        $renderPython = Ensure-RenderPython -RepoRoot $repoRoot
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
        first_pages_dir = $aggregateFirstPagesDir
        contact_sheet = $aggregateContactSheetPath
        shared_baseline_visual_output_dir = $baselineVisualDir
    }
    $reviewManifest.aggregate_evidence = $summary.aggregate_evidence
}

($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8
($reviewManifest | ConvertTo-Json -Depth 8) | Set-Content -Path $reviewManifestPath -Encoding UTF8

$reviewChecklistLines = [System.Collections.Generic.List[string]]::new()
[void]$reviewChecklistLines.Add("# Template table bookmark CLI visual regression checklist")
[void]$reviewChecklistLines.Add("")
if (-not $SkipVisual) {
    [void]$reviewChecklistLines.Add("- Start with the aggregate before/after contact sheet:")
    [void]$reviewChecklistLines.Add("  $aggregateContactSheetPath")
    [void]$reviewChecklistLines.Add("- Then inspect each case contact sheet and page-01 screenshot.")
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
[void]$reviewChecklistLines.Add("- Shared baseline DOCX: $baselineDocxPath")
foreach ($caseSummary in $summary.cases) {
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) mutation JSON: $($caseSummary.mutation_json)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) mutated rows dir: $($caseSummary.mutated_rows_dir)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) command sequence: $($caseSummary.command_sequence)")
}
$reviewChecklistLines | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReviewLines = [System.Collections.Generic.List[string]]::new()
[void]$finalReviewLines.Add("# Template table bookmark CLI visual final review")
[void]$finalReviewLines.Add("")
[void]$finalReviewLines.Add("- Generated at: $(Get-Date -Format s)")
[void]$finalReviewLines.Add("- Summary JSON: $summaryPath")
[void]$finalReviewLines.Add("- Review manifest: $reviewManifestPath")
[void]$finalReviewLines.Add("- Visual enabled: $((-not $SkipVisual.IsPresent).ToString().ToLowerInvariant())")
if (-not $SkipVisual) {
    [void]$finalReviewLines.Add("- Aggregate contact sheet: $aggregateContactSheetPath")
}
[void]$finalReviewLines.Add("- Shared baseline DOCX: $baselineDocxPath")
foreach ($caseSummary in $summary.cases) {
    [void]$finalReviewLines.Add("- $($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)")
}
$finalReviewLines | Set-Content -Path $finalReviewPath -Encoding UTF8

Write-Step "Bookmark visual regression artifacts written to $resolvedOutputDir"
