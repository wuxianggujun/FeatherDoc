param(
    [string]$BuildDir = "build-codex-clang-compat",
    [string]$OutputDir = "output/template-table-cli-selector-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[template-table-cli-selector-visual] $Message"
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
        $null = & $basePython -m venv $venvDir
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to create local Python virtual environment."
        }
    }

    if (-not (Test-PythonImport -PythonCommand $venvPython -ModuleName "PIL")) {
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

function Assert-Rows {
    param(
        [string]$JsonPath,
        [string[][]]$ExpectedRows,
        [string]$Label
    )

    $json = Get-Content -Raw -LiteralPath $JsonPath | ConvertFrom-Json
    $rows = @($json.rows)
    if ($rows.Count -ne $ExpectedRows.Count) {
        throw "$Label row count mismatch. Expected $($ExpectedRows.Count), got $($rows.Count)."
    }

    for ($i = 0; $i -lt $ExpectedRows.Count; $i++) {
        $actual = @($rows[$i].cell_texts | ForEach-Object { $_.ToString() })
        $expected = @($ExpectedRows[$i])
        if (($actual -join "`n") -ne ($expected -join "`n")) {
            throw "$Label row $i mismatch. Expected '$($expected -join " | ")', got '$($actual -join " | ")'."
        }
    }
}

function Assert-Cells {
    param(
        [string]$JsonPath,
        [object[]]$ExpectedCells,
        [string]$Label
    )

    $json = Get-Content -Raw -LiteralPath $JsonPath | ConvertFrom-Json
    $cells = @($json.cells)
    if ($cells.Count -ne $ExpectedCells.Count) {
        throw "$Label cell count mismatch. Expected $($ExpectedCells.Count), got $($cells.Count)."
    }

    for ($i = 0; $i -lt $ExpectedCells.Count; $i++) {
        foreach ($key in $ExpectedCells[$i].Keys) {
            if ($cells[$i].$key -ne $ExpectedCells[$i][$key]) {
                throw "$Label cell $i field '$key' mismatch. Expected '$($ExpectedCells[$i][$key])', got '$($cells[$i].$key)'."
            }
        }
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

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"

if (-not $SkipBuild) {
    Write-Step "Building featherdoc_cli and selector visual sample"
    & cmake --build $resolvedBuildDir --target featherdoc_cli featherdoc_sample_template_table_cli_bookmark_visual -- -j4
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build selector visual regression prerequisites."
    }
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_template_table_cli_bookmark_visual"
$renderPython = if ($SkipVisual) { $null } else { Ensure-RenderPython -RepoRoot $repoRoot }

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateFirstPagesDir = Join-Path $aggregateEvidenceDir "first-pages"
$aggregateSelectedPagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "contact_sheet.png"
New-Item -ItemType Directory -Path $aggregateFirstPagesDir -Force | Out-Null
New-Item -ItemType Directory -Path $aggregateSelectedPagesDir -Force | Out-Null

$sharedBaselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$sharedBaselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"
Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($sharedBaselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared selector baseline sample."

$sharedBaselineFirstPage = $null
if (-not $SkipVisual) {
    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $sharedBaselineDocxPath `
        -OutputDir $sharedBaselineVisualDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent
    $sharedBaselineFirstPage = Join-Path $sharedBaselineVisualDir "evidence\pages\page-01.png"
}

$selectorArgs = @(
    "--after-text", "Bookmark paragraph immediately before the target table.",
    "--header-cell", "Region",
    "--header-cell", "Qty",
    "--header-cell", "Amount"
)

$retainedRows = @(
    @("Retained", "Qty", "Amount"),
    @("keep-left", "4", "40")
)

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    cases = @()
}

$aggregateImages = @()
$aggregateLabels = @()
$script:selectedPagesByCase = @{}

function Register-VisualPair {
    param(
        [string]$CaseId,
        [string]$BaselinePage,
        [string]$MutatedPage
    )

    $baselineCopy = Join-Path $aggregateFirstPagesDir "$CaseId-baseline-page-01.png"
    $mutatedCopy = Join-Path $aggregateFirstPagesDir "$CaseId-mutated-page-01.png"
    $baselineSelectedCopy = Join-Path $aggregateSelectedPagesDir "$CaseId-baseline-page-01.png"
    $mutatedSelectedCopy = Join-Path $aggregateSelectedPagesDir "$CaseId-mutated-page-01.png"
    Copy-Item -Path $BaselinePage -Destination $baselineCopy -Force
    Copy-Item -Path $MutatedPage -Destination $mutatedCopy -Force
    Copy-Item -Path $BaselinePage -Destination $baselineSelectedCopy -Force
    Copy-Item -Path $MutatedPage -Destination $mutatedSelectedCopy -Force
    $script:aggregateImages += $baselineCopy
    $script:aggregateImages += $mutatedCopy
    $script:aggregateLabels += "$CaseId-baseline"
    $script:aggregateLabels += "$CaseId-mutated"
    $script:selectedPagesByCase[$CaseId] = @(
        [ordered]@{
            page_number = 1
            role = "primary"
            baseline_page = $baselineSelectedCopy
            mutated_page = $mutatedSelectedCopy
        }
    )
}

function Write-SummaryArtifacts {
    param(
        [string]$SummaryPath,
        [bool]$SkipVisual,
        [string]$Python,
        [string]$ScriptPath,
        [string]$AggregateContactSheetPath
    )

    if (-not $SkipVisual) {
        foreach ($case in $script:summary.cases) {
            if ($script:selectedPagesByCase.ContainsKey($case.id) -and $case.Contains("visual")) {
                $case.visual["selected_pages"] = $script:selectedPagesByCase[$case.id]
            }
        }
        Build-ContactSheet -Python $Python -ScriptPath $ScriptPath -OutputPath $AggregateContactSheetPath -Images $script:aggregateImages -Labels $script:aggregateLabels
        $script:summary.aggregate_evidence = [ordered]@{
            root = $script:aggregateEvidenceDir
            first_pages_dir = $script:aggregateFirstPagesDir
            selected_pages_dir = $script:aggregateSelectedPagesDir
            contact_sheet = $AggregateContactSheetPath
        }
    }

    ($script:summary | ConvertTo-Json -Depth 8) | Set-Content -Path $SummaryPath -Encoding UTF8
    Write-Step "Selector visual regression artifacts written to $script:resolvedOutputDir"
}

# append-row-selector
$caseId = "append-row-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$appendDocx = Join-Path $caseDir "append.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$appendJson = Join-Path $caseDir "append.json"
$fillJson = Join-Path $caseDir "fill.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("append-template-table-row", $baselineDocx) + $selectorArgs + @("--output", $appendDocx, "--json")) `
    -OutputPath $appendJson `
    -FailureMessage "Append row selector mutation failed."
$appendResult = Get-Content -Raw -LiteralPath $appendJson | ConvertFrom-Json
$tableIndex = [int]$appendResult.table_index
$rowIndex = if ($appendResult.PSObject.Properties.Name -contains "row_index") { [int]$appendResult.row_index } else { 3 }

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("set-template-table-row-texts", $appendDocx, $tableIndex.ToString(), $rowIndex.ToString(), "--row", "West direct", "--cell", "30", "--cell", "300", "--output", $mutatedDocx, "--json") `
    -OutputPath $fillJson `
    -FailureMessage "Append row selector fill failed."

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") `
    -OutputPath $retainedRowsJson `
    -FailureMessage "Append row selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") `
    -OutputPath $targetRowsJson `
    -FailureMessage "Append row selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "10", "100"),
    @("South seed", "20", "200"),
    @("West direct", "30", "300")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($appendJson, $fillJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $mutatedDocx `
        -OutputDir $mutatedVisualDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# merge-right-selector
$caseId = "merge-right-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mergeJson = Join-Path $caseDir "merge.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetCellsJson = Join-Path $caseDir "target-cells.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture -Executable $cliExecutable -Arguments (@("merge-template-table-cells", $baselineDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $mutatedDocx, "--json")) -OutputPath $mergeJson -FailureMessage "Merge selector mutation failed."
$mergeResult = Get-Content -Raw -LiteralPath $mergeJson | ConvertFrom-Json
$tableIndex = [int]$mergeResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Merge selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-cells", $mutatedDocx, $tableIndex.ToString(), "--row", "1", "--json") -OutputPath $targetCellsJson -FailureMessage "Merge selector target-table inspect failed."
Assert-Cells -JsonPath $targetCellsJson -ExpectedCells @(
    [ordered]@{ cell_index = 0; column_index = 0; column_span = 2; text = "North seed"; width_twips = 4200 },
    [ordered]@{ cell_index = 1; column_index = 2; column_span = 1; text = "100"; width_twips = 2200 }
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mergeJson, $retainedRowsJson, $targetCellsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# merge-right-explicit-break-selector-visual
$caseId = "merge-right-explicit-break-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$sourceDocx = Join-Path $caseDir "source.docx"
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$prepareJson = Join-Path $caseDir "prepare-merge.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetCellsJson = Join-Path $caseDir "target-cells.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$baselineVisualDir = Join-Path $caseDir "baseline-visual"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $sourceDocx -Force

Invoke-Capture -Executable $cliExecutable -Arguments (@("merge-template-table-cells", $sourceDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $baselineDocx, "--json")) -OutputPath $prepareJson -FailureMessage "Failed to prepare merged baseline for merge explicit-break selector case."
$prepareResult = Get-Content -Raw -LiteralPath $prepareJson | ConvertFrom-Json
$tableIndex = [int]$prepareResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $baselineDocx, $tableIndex.ToString(), "1", "0", "--text", "North seed`npending approval", "--output", $mutatedDocx, "--json") -OutputPath $mutationJson -FailureMessage "Set cell-text after merge selector mutation failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-text after merge selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-cells", $mutatedDocx, $tableIndex.ToString(), "--row", "1", "--json") -OutputPath $targetCellsJson -FailureMessage "Set cell-text after merge selector target-table inspect failed."
Assert-Cells -JsonPath $targetCellsJson -ExpectedCells @(
    [ordered]@{ cell_index = 0; column_index = 0; column_span = 2; text = "North seed`npending approval"; width_twips = 4200 },
    [ordered]@{ cell_index = 1; column_index = 2; column_span = 1; text = "100"; width_twips = 2200 }
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($prepareJson, $mutationJson, $retainedRowsJson, $targetCellsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $baselineDocx -OutputDir $baselineVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $baselinePage = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($baselinePage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $baselinePage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $baselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# unmerge-right-selector
$caseId = "unmerge-right-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$sourceDocx = Join-Path $caseDir "source.docx"
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$prepareJson = Join-Path $caseDir "prepare-merge.json"
$unmergeJson = Join-Path $caseDir "unmerge.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetCellsJson = Join-Path $caseDir "target-cells.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$baselineVisualDir = Join-Path $caseDir "baseline-visual"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $sourceDocx -Force

Invoke-Capture -Executable $cliExecutable -Arguments (@("merge-template-table-cells", $sourceDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $baselineDocx, "--json")) -OutputPath $prepareJson -FailureMessage "Failed to prepare merged baseline for unmerge selector case."
Invoke-Capture -Executable $cliExecutable -Arguments (@("unmerge-template-table-cells", $baselineDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $mutatedDocx, "--json")) -OutputPath $unmergeJson -FailureMessage "Unmerge selector mutation failed."
$unmergeResult = Get-Content -Raw -LiteralPath $unmergeJson | ConvertFrom-Json
$tableIndex = [int]$unmergeResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Unmerge selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-cells", $mutatedDocx, $tableIndex.ToString(), "--row", "1", "--json") -OutputPath $targetCellsJson -FailureMessage "Unmerge selector target-table inspect failed."
Assert-Cells -JsonPath $targetCellsJson -ExpectedCells @(
    [ordered]@{ cell_index = 0; column_index = 0; column_span = 1; text = "North seed"; width_twips = 2600 },
    [ordered]@{ cell_index = 1; column_index = 1; column_span = 1; text = ""; width_twips = 1600 },
    [ordered]@{ cell_index = 2; column_index = 2; column_span = 1; text = "100"; width_twips = 2200 }
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($prepareJson, $unmergeJson, $retainedRowsJson, $targetCellsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $baselineDocx -OutputDir $baselineVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $baselinePage = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($baselinePage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $baselinePage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $baselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# unmerge-right-explicit-break-selector-visual
$caseId = "unmerge-right-explicit-break-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$sourceDocx = Join-Path $caseDir "source.docx"
$mergedDocx = Join-Path $caseDir "merged.docx"
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$prepareJson = Join-Path $caseDir "prepare-merge.json"
$unmergeJson = Join-Path $caseDir "prepare-unmerge.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetCellsJson = Join-Path $caseDir "target-cells.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$baselineVisualDir = Join-Path $caseDir "baseline-visual"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $sourceDocx -Force

Invoke-Capture -Executable $cliExecutable -Arguments (@("merge-template-table-cells", $sourceDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $mergedDocx, "--json")) -OutputPath $prepareJson -FailureMessage "Failed to prepare merged document for unmerge explicit-break selector case."
Invoke-Capture -Executable $cliExecutable -Arguments (@("unmerge-template-table-cells", $mergedDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $baselineDocx, "--json")) -OutputPath $unmergeJson -FailureMessage "Failed to prepare unmerged baseline for explicit-break selector case."
$unmergeResult = Get-Content -Raw -LiteralPath $unmergeJson | ConvertFrom-Json
$tableIndex = [int]$unmergeResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $baselineDocx, $tableIndex.ToString(), "1", "1", "--text", "10 units`nhold", "--output", $mutatedDocx, "--json") -OutputPath $mutationJson -FailureMessage "Set cell-text after unmerge selector mutation failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-text after unmerge selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-cells", $mutatedDocx, $tableIndex.ToString(), "--row", "1", "--json") -OutputPath $targetCellsJson -FailureMessage "Set cell-text after unmerge selector target-table inspect failed."
Assert-Cells -JsonPath $targetCellsJson -ExpectedCells @(
    [ordered]@{ cell_index = 0; column_index = 0; column_span = 1; text = "North seed"; width_twips = 2600 },
    [ordered]@{ cell_index = 1; column_index = 1; column_span = 1; text = "10 units`nhold"; width_twips = 1600 },
    [ordered]@{ cell_index = 2; column_index = 2; column_span = 1; text = "100"; width_twips = 2200 }
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($prepareJson, $unmergeJson, $mutationJson, $retainedRowsJson, $targetCellsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $baselineDocx -OutputDir $baselineVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $baselinePage = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($baselinePage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $baselinePage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $baselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# insert-column-before-selector
$caseId = "insert-column-before-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$insertDocx = Join-Path $caseDir "insert.docx"
$stepHeaderDocx = Join-Path $caseDir "step-header.docx"
$stepRow1Docx = Join-Path $caseDir "step-row-1.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$insertJson = Join-Path $caseDir "insert.json"
$setHeaderJson = Join-Path $caseDir "set-header.json"
$setRow1Json = Join-Path $caseDir "set-row-1.json"
$setRow2Json = Join-Path $caseDir "set-row-2.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("insert-template-table-column-before", $baselineDocx) + $selectorArgs + @("0", "1", "--output", $insertDocx, "--json")) `
    -OutputPath $insertJson `
    -FailureMessage "Insert column selector mutation failed."
$insertResult = Get-Content -Raw -LiteralPath $insertJson | ConvertFrom-Json
$tableIndex = [int]$insertResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $insertDocx, $tableIndex.ToString(), "0", "1", "--text", "Status", "--output", $stepHeaderDocx, "--json") -OutputPath $setHeaderJson -FailureMessage "Failed to set selector column header."
Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $stepHeaderDocx, $tableIndex.ToString(), "1", "1", "--text", "Open", "--output", $stepRow1Docx, "--json") -OutputPath $setRow1Json -FailureMessage "Failed to set selector row 1 column."
Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $stepRow1Docx, $tableIndex.ToString(), "2", "1", "--text", "Closed", "--output", $mutatedDocx, "--json") -OutputPath $setRow2Json -FailureMessage "Failed to set selector row 2 column."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Insert column selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Insert column selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Status", "Qty", "Amount"),
    @("North seed", "Open", "10", "100"),
    @("South seed", "Closed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($insertJson, $setHeaderJson, $setRow1Json, $setRow2Json, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# insert-column-after-selector
$caseId = "insert-column-after-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$insertDocx = Join-Path $caseDir "insert.docx"
$stepHeaderDocx = Join-Path $caseDir "step-header.docx"
$stepRow1Docx = Join-Path $caseDir "step-row-1.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$insertJson = Join-Path $caseDir "insert.json"
$setHeaderJson = Join-Path $caseDir "set-header.json"
$setRow1Json = Join-Path $caseDir "set-row-1.json"
$setRow2Json = Join-Path $caseDir "set-row-2.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("insert-template-table-column-after", $baselineDocx) + $selectorArgs + @("0", "1", "--output", $insertDocx, "--json")) `
    -OutputPath $insertJson `
    -FailureMessage "Insert column-after selector mutation failed."
$insertResult = Get-Content -Raw -LiteralPath $insertJson | ConvertFrom-Json
$tableIndex = [int]$insertResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $insertDocx, $tableIndex.ToString(), "0", "2", "--text", "Forecast", "--output", $stepHeaderDocx, "--json") -OutputPath $setHeaderJson -FailureMessage "Failed to set selector column-after header."
Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $stepHeaderDocx, $tableIndex.ToString(), "1", "2", "--text", "110", "--output", $stepRow1Docx, "--json") -OutputPath $setRow1Json -FailureMessage "Failed to set selector column-after row 1."
Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $stepRow1Docx, $tableIndex.ToString(), "2", "2", "--text", "220", "--output", $mutatedDocx, "--json") -OutputPath $setRow2Json -FailureMessage "Failed to set selector column-after row 2."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Insert column-after selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Insert column-after selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Forecast", "Amount"),
    @("North seed", "10", "110", "100"),
    @("South seed", "20", "220", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($insertJson, $setHeaderJson, $setRow1Json, $setRow2Json, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# insert-row-before-selector
$caseId = "insert-row-before-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$insertDocx = Join-Path $caseDir "insert.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$insertJson = Join-Path $caseDir "insert.json"
$fillJson = Join-Path $caseDir "fill.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("insert-template-table-row-before", $baselineDocx) + $selectorArgs + @("1", "--output", $insertDocx, "--json")) `
    -OutputPath $insertJson `
    -FailureMessage "Insert row selector mutation failed."
$insertResult = Get-Content -Raw -LiteralPath $insertJson | ConvertFrom-Json
$tableIndex = [int]$insertResult.table_index
$rowIndex = if ($insertResult.PSObject.Properties.Name -contains "inserted_row_index") { [int]$insertResult.inserted_row_index } else { 1 }

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("set-template-table-row-texts", $insertDocx, $tableIndex.ToString(), $rowIndex.ToString(), "--row", "Central direct", "--cell", "15", "--cell", "150", "--output", $mutatedDocx, "--json") `
    -OutputPath $fillJson `
    -FailureMessage "Insert row selector fill failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Insert row selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Insert row selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("Central direct", "15", "150"),
    @("North seed", "10", "100"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($insertJson, $fillJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# insert-row-after-selector
$caseId = "insert-row-after-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$insertDocx = Join-Path $caseDir "insert.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$insertJson = Join-Path $caseDir "insert.json"
$fillJson = Join-Path $caseDir "fill.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("insert-template-table-row-after", $baselineDocx) + $selectorArgs + @("1", "--output", $insertDocx, "--json")) `
    -OutputPath $insertJson `
    -FailureMessage "Insert row-after selector mutation failed."
$insertResult = Get-Content -Raw -LiteralPath $insertJson | ConvertFrom-Json
$tableIndex = [int]$insertResult.table_index
$rowIndex = if ($insertResult.PSObject.Properties.Name -contains "inserted_row_index") { [int]$insertResult.inserted_row_index } else { 2 }

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("set-template-table-row-texts", $insertDocx, $tableIndex.ToString(), $rowIndex.ToString(), "--row", "Follow-up direct", "--cell", "16", "--cell", "160", "--output", $mutatedDocx, "--json") `
    -OutputPath $fillJson `
    -FailureMessage "Insert row-after selector fill failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Insert row-after selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Insert row-after selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "10", "100"),
    @("Follow-up direct", "16", "160"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($insertJson, $fillJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# remove-row-selector
$caseId = "remove-row-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("remove-template-table-row", $baselineDocx) + $selectorArgs + @("1", "--output", $mutatedDocx, "--json")) `
    -OutputPath $mutationJson `
    -FailureMessage "Remove row selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Remove row selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Remove row selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-tables-from-json-batch-selector
$caseId = "set-tables-from-json-batch-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$patchPath = Join-Path $caseDir "patch.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

@'
{
  "operations": [
    {
      "header_cells": ["Retained", "Qty", "Amount"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["carry-right", "8", "80"]
      ]
    },
    {
      "after_text": "Bookmark paragraph immediately before the target table.",
      "header_cells": ["Region", "Qty", "Amount"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["Batch south", "23", "230"],
        ["Batch east", "45", "450"]
      ]
    }
  ]
}
'@ | Set-Content -Path $patchPath -Encoding UTF8

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("set-template-tables-from-json", $baselineDocx, "--patch-file", $patchPath, "--output", $mutatedDocx, "--json") `
    -OutputPath $mutationJson `
    -FailureMessage "Set-tables-from-json batch selector mutation failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set-tables-from-json batch retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows @(
    @("Retained", "Qty", "Amount"),
    @("carry-right", "8", "80")
) -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "1", "--json") -OutputPath $targetRowsJson -FailureMessage "Set-tables-from-json batch target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("Batch south", "23", "230"),
    @("Batch east", "45", "450")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($patchPath, $mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-tables-from-json-batch-longtext-selector-visual
$caseId = "set-tables-from-json-batch-longtext-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$patchPath = Join-Path $caseDir "patch.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

@'
{
  "operations": [
    {
      "header_cells": ["Retained", "Qty", "Amount"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        [
          "Carry-over items require archive review",
          "8 units pending approval",
          "80 total awaiting confirmation"
        ]
      ]
    },
    {
      "after_text": "Bookmark paragraph immediately before the target table.",
      "header_cells": ["Region", "Qty", "Amount"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        [
          "North region requires manual follow-up",
          "18 units pending approval",
          "240 total awaiting finance sign-off"
        ],
        [
          "South backlog schedules phased shipment",
          "27 units queued for packaging",
          "410 total under manager review"
        ]
      ]
    }
  ]
}
'@ | Set-Content -Path $patchPath -Encoding UTF8

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("set-template-tables-from-json", $baselineDocx, "--patch-file", $patchPath, "--output", $mutatedDocx, "--json") `
    -OutputPath $mutationJson `
    -FailureMessage "Set-tables-from-json batch longtext selector mutation failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set-tables-from-json batch longtext retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows @(
    @("Retained", "Qty", "Amount"),
    @("Carry-over items require archive review", "8 units pending approval", "80 total awaiting confirmation")
) -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "1", "--json") -OutputPath $targetRowsJson -FailureMessage "Set-tables-from-json batch longtext target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North region requires manual follow-up", "18 units pending approval", "240 total awaiting finance sign-off"),
    @("South backlog schedules phased shipment", "27 units queued for packaging", "410 total under manager review")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($patchPath, $mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# remove-column-selector
$caseId = "remove-column-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("remove-template-table-column", $baselineDocx) + $selectorArgs + @("0", "1", "--output", $mutatedDocx, "--json")) `
    -OutputPath $mutationJson `
    -FailureMessage "Remove column selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Remove column selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Remove column selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Amount"),
    @("North seed", "100"),
    @("South seed", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-from-json-selector
$caseId = "set-from-json-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$patchPath = Join-Path $caseDir "patch.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

@'
{
  "mode": "rows",
  "start_row": 1,
  "rows": [
    ["West refreshed", "31", "310"],
    ["East refreshed", "44", "440"]
  ]
}
'@ | Set-Content -Path $patchPath -Encoding UTF8

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-from-json", $baselineDocx) + $selectorArgs + @("--patch-file", $patchPath, "--output", $mutatedDocx, "--json")) `
    -OutputPath $mutationJson `
    -FailureMessage "Set-from-json selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set-from-json selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set-from-json selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("West refreshed", "31", "310"),
    @("East refreshed", "44", "440")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($patchPath, $mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-from-json-longtext-selector-visual
$caseId = "set-from-json-longtext-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$patchPath = Join-Path $caseDir "patch.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

@'
{
  "mode": "rows",
  "start_row": 1,
  "rows": [
    [
      "North region requires manual follow-up",
      "18 units pending approval",
      "240 total awaiting finance sign-off"
    ],
    [
      "South backlog schedules phased shipment",
      "27 units queued for packaging",
      "410 total under manager review"
    ]
  ]
}
'@ | Set-Content -Path $patchPath -Encoding UTF8

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-from-json", $baselineDocx) + $selectorArgs + @("--patch-file", $patchPath, "--output", $mutatedDocx, "--json")) `
    -OutputPath $mutationJson `
    -FailureMessage "Set-from-json longtext selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set-from-json longtext selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set-from-json longtext selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North region requires manual follow-up", "18 units pending approval", "240 total awaiting finance sign-off"),
    @("South backlog schedules phased shipment", "27 units queued for packaging", "410 total under manager review")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($patchPath, $mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-row-texts-selector-visual
$caseId = "set-row-texts-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-row-texts", $baselineDocx) + $selectorArgs + @(
            "1",
            "--row", "North region requires manual follow-up",
            "--cell", "18 units pending approval",
            "--cell", "240 total awaiting finance sign-off",
            "--row", "South backlog schedules phased shipment",
            "--cell", "27 units queued for packaging",
            "--cell", "410 total under manager review",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set row-texts selector visual mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set row-texts selector visual retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set row-texts selector visual target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North region requires manual follow-up", "18 units pending approval", "240 total awaiting finance sign-off"),
    @("South backlog schedules phased shipment", "27 units queued for packaging", "410 total under manager review")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-text-selector-visual
$caseId = "set-cell-text-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-text", $baselineDocx) + $selectorArgs + @(
            "1",
            "2",
            "--text", "100 total pending finance approval after manual audit",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-text selector visual mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-text selector visual retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-text selector visual target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "10", "100 total pending finance approval after manual audit"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-text-explicit-break-selector-visual
$caseId = "set-cell-text-explicit-break-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-text", $baselineDocx) + $selectorArgs + @(
            "1",
            "2",
            "--text", "240 total`nfinance review",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-text explicit-break selector visual mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-text explicit-break selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-text explicit-break selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "10", "240 total`nfinance review"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-block-texts-multiline-selector-visual
$caseId = "set-cell-block-texts-multiline-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-block-texts", $baselineDocx) + $selectorArgs + @(
            "1",
            "1",
            "--row", "18 units pending approval", "--cell", "240 total awaiting finance sign-off",
            "--row", "27 units queued for packaging", "--cell", "410 total under manager review",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-block-texts multiline selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-block-texts multiline selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-block-texts multiline selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "18 units pending approval", "240 total awaiting finance sign-off"),
    @("South seed", "27 units queued for packaging", "410 total under manager review")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-block-texts-explicit-break-selector-visual
$caseId = "set-cell-block-texts-explicit-break-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-block-texts", $baselineDocx) + $selectorArgs + @(
            "1",
            "1",
            "--row", "18 units`npending approval", "--cell", "240 total`nfinance review",
            "--row", "27 units`npackaging queue", "--cell", "410 total`nmanager review",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-block-texts explicit-break selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-block-texts explicit-break selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-block-texts explicit-break selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "18 units`npending approval", "240 total`nfinance review"),
    @("South seed", "27 units`npackaging queue", "410 total`nmanager review")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-block-texts-left-block-selector-visual
$caseId = "set-cell-block-texts-left-block-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-block-texts", $baselineDocx) + $selectorArgs + @(
            "1",
            "0",
            "--row", "North allocation awaiting warehouse audit", "--cell", "18 units held for approval",
            "--row", "South backlog routed to overflow storage", "--cell", "27 units queued for packaging",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-block-texts left-block selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-block-texts left-block selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-block-texts left-block selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North allocation awaiting warehouse audit", "18 units held for approval", "100"),
    @("South backlog routed to overflow storage", "27 units queued for packaging", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-block-texts-selector
$caseId = "set-cell-block-texts-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-block-texts", $baselineDocx) + $selectorArgs + @("1", "1", "--row", "18 units", "--cell", "240 total", "--output", $mutatedDocx, "--json")) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-block-texts selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-block-texts selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-block-texts selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "18 units", "240 total"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
Write-SummaryArtifacts `
    -SummaryPath $summaryPath `
    -SkipVisual $SkipVisual.IsPresent `
    -Python $renderPython `
    -ScriptPath $contactSheetScript `
    -AggregateContactSheetPath $aggregateContactSheetPath
