param(
    [string]$BuildDir = "build-table-row-visual-nmake",
    [string]$OutputDir = "output/table-row-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[table-row-visual] $Message"
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

function Assert-TableRows {
    param(
        [string]$JsonPath,
        [object[]]$ExpectedRows,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $rows = @($payload.rows)
    if ([int]$payload.count -ne $ExpectedRows.Count -or $rows.Count -ne $ExpectedRows.Count) {
        throw "$Label expected $($ExpectedRows.Count) rows, got payload count $($payload.count) and row array count $($rows.Count)."
    }

    for ($index = 0; $index -lt $ExpectedRows.Count; $index++) {
        $row = $rows[$index]
        $expected = $ExpectedRows[$index]
        $expectedTexts = @($expected.cell_texts)
        $actualTexts = @($row.cell_texts)

        if ([int]$row.row_index -ne $index) {
            throw "$Label row $index reported row_index $($row.row_index)."
        }
        if ([int]$row.cell_count -ne $expectedTexts.Count -or $actualTexts.Count -ne $expectedTexts.Count) {
            throw "$Label row $index expected $($expectedTexts.Count) cells, got cell_count=$($row.cell_count) and actual text count $($actualTexts.Count)."
        }

        if ($null -eq $expected.height_twips) {
            if ($null -ne $row.height_twips) {
                throw "$Label row $index expected null height_twips, got $($row.height_twips)."
            }
        } elseif ([int]$row.height_twips -ne [int]$expected.height_twips) {
            throw "$Label row $index expected height_twips $($expected.height_twips), got $($row.height_twips)."
        }

        $expectedRule = $expected.height_rule
        $actualRule = $row.height_rule
        if ($null -eq $expectedRule) {
            if ($null -ne $actualRule) {
                throw "$Label row $index expected null height_rule, got $actualRule."
            }
        } elseif ([string]$actualRule -ne [string]$expectedRule) {
            throw "$Label row $index expected height_rule '$expectedRule', got '$actualRule'."
        }

        if ([bool]$row.cant_split -ne [bool]$expected.cant_split) {
            throw "$Label row $index expected cant_split=$($expected.cant_split), got $($row.cant_split)."
        }

        for ($cellIndex = 0; $cellIndex -lt $expectedTexts.Count; $cellIndex++) {
            if ([string]$actualTexts[$cellIndex] -ne [string]$expectedTexts[$cellIndex]) {
                throw "$Label row $index cell $cellIndex mismatch. Expected '$($expectedTexts[$cellIndex])', got '$($actualTexts[$cellIndex])'."
            }
        }
    }
}

function Assert-BlankRowCells {
    param(
        [string]$JsonPath,
        [int]$ExpectedRowIndex,
        [int]$ExpectedCount,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $cells = @($payload.cells)
    if ([int]$payload.row_index -ne $ExpectedRowIndex) {
        throw "$Label expected row_index $ExpectedRowIndex, got $($payload.row_index)."
    }
    if ([int]$payload.count -ne $ExpectedCount -or $cells.Count -ne $ExpectedCount) {
        throw "$Label expected $ExpectedCount cells, got payload count $($payload.count) and array count $($cells.Count)."
    }

    for ($index = 0; $index -lt $cells.Count; $index++) {
        if ([int]$cells[$index].row_index -ne $ExpectedRowIndex) {
            throw "$Label cell $index reported row_index $($cells[$index].row_index)."
        }
        if ([int]$cells[$index].cell_index -ne $index) {
            throw "$Label cell $index reported cell_index $($cells[$index].cell_index)."
        }
        if (-not [string]::IsNullOrEmpty([string]$cells[$index].text)) {
            throw "$Label cell $index expected blank text, got '$($cells[$index].text)'."
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

    Write-Step "Building featherdoc_cli and table row visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_table_row_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_table_row_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineInspectRowsPath = Join-Path $resolvedOutputDir "shared-baseline-rows.json"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"

$baselineRows = @(
    [ordered]@{
        height_twips = 520
        height_rule = "exact"
        cant_split = $false
        cell_texts = @("ROW ROLE", "VISUAL CUE")
    },
    [ordered]@{
        height_twips = 1120
        height_rule = "at_least"
        cant_split = $true
        cell_texts = @(
            "ALPHA BLOCK",
            "Insert-before should create a blank amber row above this content."
        )
    },
    [ordered]@{
        height_twips = 1120
        height_rule = "at_least"
        cant_split = $false
        cell_texts = @(
            "OMEGA BLOCK",
            "Append should create a blank green row below this content."
        )
    }
)

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared table row baseline sample."

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-rows", $baselineDocxPath, "0", "--json") `
    -OutputPath $baselineInspectRowsPath `
    -FailureMessage "Failed to inspect shared table row baseline."

Assert-TableRows -JsonPath $baselineInspectRowsPath -ExpectedRows $baselineRows -Label "shared-baseline-rows"

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

$appendCaseId = "append-table-row-visual"
$appendCaseDir = Join-Path $resolvedOutputDir $appendCaseId
$appendDocxPath = Join-Path $appendCaseDir "$appendCaseId-mutated.docx"
$appendMutationJsonPath = Join-Path $appendCaseDir "mutation_result.json"
$appendInspectRowsPath = Join-Path $appendCaseDir "inspect_rows.json"
$appendInspectCellsPath = Join-Path $appendCaseDir "inspect_blank_row_cells.json"
$appendVisualDir = Join-Path $appendCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $appendCaseDir -Force | Out-Null

Write-Step "Running case '$appendCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "append-table-row", $baselineDocxPath, "0",
        "--output", $appendDocxPath, "--json"
    ) `
    -OutputPath $appendMutationJsonPath `
    -FailureMessage "Failed to append a body table row."

foreach ($expected in @(
        '"command":"append-table-row"',
        '"table_index":0',
        '"row_index":3',
        '"cell_count":2'
    )) {
    Assert-Contains -Path $appendMutationJsonPath -Expected $expected -Label $appendCaseId
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-rows", $appendDocxPath, "0", "--json") `
    -OutputPath $appendInspectRowsPath `
    -FailureMessage "Failed to inspect appended table rows."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-cells", $appendDocxPath, "0", "--row", "3", "--json") `
    -OutputPath $appendInspectCellsPath `
    -FailureMessage "Failed to inspect appended blank row cells."

Assert-TableRows `
    -JsonPath $appendInspectRowsPath `
    -ExpectedRows @(
        $baselineRows[0],
        $baselineRows[1],
        $baselineRows[2],
        [ordered]@{
            height_twips = $null
            height_rule = $null
            cant_split = $false
            cell_texts = @("", "")
        }
    ) `
    -Label $appendCaseId
Assert-BlankRowCells -JsonPath $appendInspectCellsPath -ExpectedRowIndex 3 -ExpectedCount 2 -Label $appendCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $appendDocxPath `
    -OutputDir $appendVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $appendVisualDir -ExpectedCount 1 -Label $appendCaseId

$appendArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $appendCaseDir `
    -CaseId $appendCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $appendCaseId),
        (Get-RenderedPagePath -VisualOutputDir $appendVisualDir -PageNumber 1 -Label $appendCaseId)
    ) `
    -Labels @("$appendCaseId baseline-page-01", "$appendCaseId mutated-page-01")

$aggregateImages += $appendArtifacts.selected_pages
$aggregateLabels += $appendArtifacts.labels
$summaryCases += [ordered]@{
    id = $appendCaseId
    command = "append-table-row"
    source_docx = $baselineDocxPath
    mutated_docx = $appendDocxPath
    mutation_json = $appendMutationJsonPath
    inspect_rows_json = $appendInspectRowsPath
    inspect_blank_row_cells_json = $appendInspectCellsPath
    expected_visual_cues = @(
        "A blank green row appears below OMEGA BLOCK while the trailing note stays below the table.",
        "The appended row keeps the same two-column grid but falls back to Word's default row height behavior.",
        "The baseline header and amber row stay unchanged."
    )
    visual_artifacts = $appendArtifacts
}

$beforeCaseId = "insert-table-row-before-visual"
$beforeCaseDir = Join-Path $resolvedOutputDir $beforeCaseId
$beforeDocxPath = Join-Path $beforeCaseDir "$beforeCaseId-mutated.docx"
$beforeMutationJsonPath = Join-Path $beforeCaseDir "mutation_result.json"
$beforeInspectRowsPath = Join-Path $beforeCaseDir "inspect_rows.json"
$beforeInspectCellsPath = Join-Path $beforeCaseDir "inspect_blank_row_cells.json"
$beforeVisualDir = Join-Path $beforeCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $beforeCaseDir -Force | Out-Null

Write-Step "Running case '$beforeCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "insert-table-row-before", $baselineDocxPath, "0", "1",
        "--output", $beforeDocxPath, "--json"
    ) `
    -OutputPath $beforeMutationJsonPath `
    -FailureMessage "Failed to insert a row before the amber row."

foreach ($expected in @(
        '"command":"insert-table-row-before"',
        '"table_index":0',
        '"row_index":1',
        '"inserted_row_index":1'
    )) {
    Assert-Contains -Path $beforeMutationJsonPath -Expected $expected -Label $beforeCaseId
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-rows", $beforeDocxPath, "0", "--json") `
    -OutputPath $beforeInspectRowsPath `
    -FailureMessage "Failed to inspect insert-before table rows."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-cells", $beforeDocxPath, "0", "--row", "1", "--json") `
    -OutputPath $beforeInspectCellsPath `
    -FailureMessage "Failed to inspect insert-before blank row cells."

Assert-TableRows `
    -JsonPath $beforeInspectRowsPath `
    -ExpectedRows @(
        $baselineRows[0],
        [ordered]@{
            height_twips = 1120
            height_rule = "at_least"
            cant_split = $true
            cell_texts = @("", "")
        },
        $baselineRows[1],
        $baselineRows[2]
    ) `
    -Label $beforeCaseId
Assert-BlankRowCells -JsonPath $beforeInspectCellsPath -ExpectedRowIndex 1 -ExpectedCount 2 -Label $beforeCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $beforeDocxPath `
    -OutputDir $beforeVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $beforeVisualDir -ExpectedCount 1 -Label $beforeCaseId

$beforeArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $beforeCaseDir `
    -CaseId $beforeCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $beforeCaseId),
        (Get-RenderedPagePath -VisualOutputDir $beforeVisualDir -PageNumber 1 -Label $beforeCaseId)
    ) `
    -Labels @("$beforeCaseId baseline-page-01", "$beforeCaseId mutated-page-01")

$aggregateImages += $beforeArtifacts.selected_pages
$aggregateLabels += $beforeArtifacts.labels
$summaryCases += [ordered]@{
    id = $beforeCaseId
    command = "insert-table-row-before"
    source_docx = $baselineDocxPath
    mutated_docx = $beforeDocxPath
    mutation_json = $beforeMutationJsonPath
    inspect_rows_json = $beforeInspectRowsPath
    inspect_blank_row_cells_json = $beforeInspectCellsPath
    expected_visual_cues = @(
        "A blank amber row appears between the blue header and ALPHA BLOCK.",
        "The inserted row keeps the amber row height and cant-split setting from the source row.",
        "OMEGA BLOCK remains below ALPHA BLOCK."
    )
    visual_artifacts = $beforeArtifacts
}

$afterCaseId = "insert-table-row-after-visual"
$afterCaseDir = Join-Path $resolvedOutputDir $afterCaseId
$afterDocxPath = Join-Path $afterCaseDir "$afterCaseId-mutated.docx"
$afterMutationJsonPath = Join-Path $afterCaseDir "mutation_result.json"
$afterInspectRowsPath = Join-Path $afterCaseDir "inspect_rows.json"
$afterInspectCellsPath = Join-Path $afterCaseDir "inspect_blank_row_cells.json"
$afterVisualDir = Join-Path $afterCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $afterCaseDir -Force | Out-Null

Write-Step "Running case '$afterCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "insert-table-row-after", $baselineDocxPath, "0", "0",
        "--output", $afterDocxPath, "--json"
    ) `
    -OutputPath $afterMutationJsonPath `
    -FailureMessage "Failed to insert a row after the blue header."

foreach ($expected in @(
        '"command":"insert-table-row-after"',
        '"table_index":0',
        '"row_index":0',
        '"inserted_row_index":1'
    )) {
    Assert-Contains -Path $afterMutationJsonPath -Expected $expected -Label $afterCaseId
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-rows", $afterDocxPath, "0", "--json") `
    -OutputPath $afterInspectRowsPath `
    -FailureMessage "Failed to inspect insert-after table rows."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-cells", $afterDocxPath, "0", "--row", "1", "--json") `
    -OutputPath $afterInspectCellsPath `
    -FailureMessage "Failed to inspect insert-after blank row cells."

Assert-TableRows `
    -JsonPath $afterInspectRowsPath `
    -ExpectedRows @(
        $baselineRows[0],
        [ordered]@{
            height_twips = 520
            height_rule = "exact"
            cant_split = $false
            cell_texts = @("", "")
        },
        $baselineRows[1],
        $baselineRows[2]
    ) `
    -Label $afterCaseId
Assert-BlankRowCells -JsonPath $afterInspectCellsPath -ExpectedRowIndex 1 -ExpectedCount 2 -Label $afterCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $afterDocxPath `
    -OutputDir $afterVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $afterVisualDir -ExpectedCount 1 -Label $afterCaseId

$afterArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $afterCaseDir `
    -CaseId $afterCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $afterCaseId),
        (Get-RenderedPagePath -VisualOutputDir $afterVisualDir -PageNumber 1 -Label $afterCaseId)
    ) `
    -Labels @("$afterCaseId baseline-page-01", "$afterCaseId mutated-page-01")

$aggregateImages += $afterArtifacts.selected_pages
$aggregateLabels += $afterArtifacts.labels
$summaryCases += [ordered]@{
    id = $afterCaseId
    command = "insert-table-row-after"
    source_docx = $baselineDocxPath
    mutated_docx = $afterDocxPath
    mutation_json = $afterMutationJsonPath
    inspect_rows_json = $afterInspectRowsPath
    inspect_blank_row_cells_json = $afterInspectCellsPath
    expected_visual_cues = @(
        "A blank blue row appears directly under the blue header, above ALPHA BLOCK.",
        "The inserted row keeps the header height instead of the taller amber row height.",
        "ALPHA BLOCK and OMEGA BLOCK remain in their original order below the insertion point."
    )
    visual_artifacts = $afterArtifacts
}

$removeCaseId = "remove-table-row-visual"
$removeCaseDir = Join-Path $resolvedOutputDir $removeCaseId
$removeDocxPath = Join-Path $removeCaseDir "$removeCaseId-mutated.docx"
$removeMutationJsonPath = Join-Path $removeCaseDir "mutation_result.json"
$removeInspectRowsPath = Join-Path $removeCaseDir "inspect_rows.json"
$removeVisualDir = Join-Path $removeCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $removeCaseDir -Force | Out-Null

Write-Step "Running case '$removeCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "remove-table-row", $baselineDocxPath, "0", "0",
        "--output", $removeDocxPath, "--json"
    ) `
    -OutputPath $removeMutationJsonPath `
    -FailureMessage "Failed to remove the blue header row."

foreach ($expected in @(
        '"command":"remove-table-row"',
        '"table_index":0',
        '"row_index":0'
    )) {
    Assert-Contains -Path $removeMutationJsonPath -Expected $expected -Label $removeCaseId
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-rows", $removeDocxPath, "0", "--json") `
    -OutputPath $removeInspectRowsPath `
    -FailureMessage "Failed to inspect remove-row table rows."

Assert-TableRows `
    -JsonPath $removeInspectRowsPath `
    -ExpectedRows @(
        $baselineRows[1],
        $baselineRows[2]
    ) `
    -Label $removeCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $removeDocxPath `
    -OutputDir $removeVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $removeVisualDir -ExpectedCount 1 -Label $removeCaseId

$removeArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $removeCaseDir `
    -CaseId $removeCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $removeCaseId),
        (Get-RenderedPagePath -VisualOutputDir $removeVisualDir -PageNumber 1 -Label $removeCaseId)
    ) `
    -Labels @("$removeCaseId baseline-page-01", "$removeCaseId mutated-page-01")

$aggregateImages += $removeArtifacts.selected_pages
$aggregateLabels += $removeArtifacts.labels
$summaryCases += [ordered]@{
    id = $removeCaseId
    command = "remove-table-row"
    source_docx = $baselineDocxPath
    mutated_docx = $removeDocxPath
    mutation_json = $removeMutationJsonPath
    inspect_rows_json = $removeInspectRowsPath
    expected_visual_cues = @(
        "The blue header row disappears completely, so the amber ALPHA BLOCK row becomes the top visible row.",
        "The green OMEGA BLOCK row moves up directly under ALPHA BLOCK.",
        "The table shrinks while the trailing note below it stays in place."
    )
    visual_artifacts = $removeArtifacts
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
    shared_baseline_rows_json = $baselineInspectRowsPath
    cases = $summaryCases
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed table row visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
