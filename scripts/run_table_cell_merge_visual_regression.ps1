param(
    [string]$BuildDir = "build-table-cell-merge-visual-nmake",
    [string]$OutputDir = "output/table-cell-merge-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"
$script:WordMlNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"

Add-Type -AssemblyName System.IO.Compression.FileSystem

function Write-Step {
    param([string]$Message)
    Write-Host "[table-cell-merge-visual] $Message"
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

function Assert-TableCellState {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [int]$ExpectedCellIndex,
        [int]$ExpectedColumnSpan,
        [string]$ExpectedText,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $cell = $payload.cell
    if ($null -eq $cell) {
        throw "$Label expected single-cell payload."
    }
    if ([int]$payload.table_index -ne $ExpectedTableIndex) {
        throw "$Label expected table_index $ExpectedTableIndex, got $($payload.table_index)."
    }
    if ([int]$cell.row_index -ne $ExpectedRowIndex) {
        throw "$Label expected row_index $ExpectedRowIndex, got $($cell.row_index)."
    }
    if ([int]$cell.cell_index -ne $ExpectedCellIndex) {
        throw "$Label expected cell_index $ExpectedCellIndex, got $($cell.cell_index)."
    }
    if ([int]$cell.column_span -ne $ExpectedColumnSpan) {
        throw "$Label expected column_span $ExpectedColumnSpan, got $($cell.column_span)."
    }
    if ([string]$cell.text -ne $ExpectedText) {
        throw "$Label expected text '$ExpectedText', got '$($cell.text)'."
    }
}

function Assert-TableRowState {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [string[]]$ExpectedTexts,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $cells = @($payload.cells)
    if ([int]$payload.table_index -ne $ExpectedTableIndex) {
        throw "$Label expected table_index $ExpectedTableIndex, got $($payload.table_index)."
    }
    if ([int]$payload.row_index -ne $ExpectedRowIndex) {
        throw "$Label expected row_index $ExpectedRowIndex, got $($payload.row_index)."
    }
    if ([int]$payload.count -ne $ExpectedTexts.Count -or $cells.Count -ne $ExpectedTexts.Count) {
        throw "$Label expected $($ExpectedTexts.Count) cells, got payload count $($payload.count) and array count $($cells.Count)."
    }

    for ($index = 0; $index -lt $ExpectedTexts.Count; $index++) {
        if ([int]$cells[$index].cell_index -ne $index) {
            throw "$Label expected cell_index $index, got $($cells[$index].cell_index)."
        }
        if ([string]$cells[$index].text -ne [string]$ExpectedTexts[$index]) {
            throw "$Label cell $index expected text '$($ExpectedTexts[$index])', got '$($cells[$index].text)'."
        }
    }
}

function Assert-SetTableCellTextResult {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [int]$ExpectedCellIndex,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "set-table-cell-text") {
        throw "$Label expected command set-table-cell-text, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }
    if ([int]$payload.table_index -ne $ExpectedTableIndex) {
        throw "$Label expected table_index $ExpectedTableIndex, got $($payload.table_index)."
    }
    if ([int]$payload.row_index -ne $ExpectedRowIndex) {
        throw "$Label expected row_index $ExpectedRowIndex, got $($payload.row_index)."
    }
    if ([int]$payload.cell_index -ne $ExpectedCellIndex) {
        throw "$Label expected cell_index $ExpectedCellIndex, got $($payload.cell_index)."
    }
}

function Assert-MergeTableCellsResult {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [int]$ExpectedCellIndex,
        [int]$ExpectedCount,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "merge-table-cells") {
        throw "$Label expected command merge-table-cells, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }
    if ([int]$payload.table_index -ne $ExpectedTableIndex) {
        throw "$Label expected table_index $ExpectedTableIndex, got $($payload.table_index)."
    }
    if ([int]$payload.row_index -ne $ExpectedRowIndex) {
        throw "$Label expected row_index $ExpectedRowIndex, got $($payload.row_index)."
    }
    if ([int]$payload.cell_index -ne $ExpectedCellIndex) {
        throw "$Label expected cell_index $ExpectedCellIndex, got $($payload.cell_index)."
    }
    if ([string]$payload.direction -ne "right") {
        throw "$Label expected direction right, got '$($payload.direction)'."
    }
    if ([int]$payload.count -ne $ExpectedCount) {
        throw "$Label expected count $ExpectedCount, got $($payload.count)."
    }
}

function Assert-UnmergeTableCellsResult {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [int]$ExpectedCellIndex,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "unmerge-table-cells") {
        throw "$Label expected command unmerge-table-cells, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }
    if ([int]$payload.table_index -ne $ExpectedTableIndex) {
        throw "$Label expected table_index $ExpectedTableIndex, got $($payload.table_index)."
    }
    if ([int]$payload.row_index -ne $ExpectedRowIndex) {
        throw "$Label expected row_index $ExpectedRowIndex, got $($payload.row_index)."
    }
    if ([int]$payload.cell_index -ne $ExpectedCellIndex) {
        throw "$Label expected cell_index $ExpectedCellIndex, got $($payload.cell_index)."
    }
    if ([string]$payload.direction -ne "right") {
        throw "$Label expected direction right, got '$($payload.direction)'."
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

    return [pscustomobject][ordered]@{
        before_after_contact_sheet = $caseContactSheetPath
        selected_pages = $copiedImages
        labels = $Labels
    }
}

function Read-DocxEntryText {
    param(
        [string]$DocxPath,
        [string]$EntryName
    )

    $zip = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        $entry = $zip.GetEntry($EntryName)
        if ($null -eq $entry) {
            throw "Entry '$EntryName' was not found in $DocxPath."
        }

        $reader = New-Object System.IO.StreamReader($entry.Open())
        try {
            return $reader.ReadToEnd()
        } finally {
            $reader.Dispose()
        }
    } finally {
        $zip.Dispose()
    }
}

function Get-BodyTableRowXmlState {
    param(
        [string]$DocxPath,
        [int]$TableIndex,
        [int]$RowIndex
    )

    $documentXml = Read-DocxEntryText -DocxPath $DocxPath -EntryName "word/document.xml"
    $xmlDocument = New-Object System.Xml.XmlDocument
    $xmlDocument.LoadXml($documentXml)

    $namespaceManager = New-Object System.Xml.XmlNamespaceManager($xmlDocument.NameTable)
    $namespaceManager.AddNamespace("w", $script:WordMlNamespace)

    $tableNodes = $xmlDocument.SelectNodes("/w:document/w:body/w:tbl", $namespaceManager)
    if ($null -eq $tableNodes -or $TableIndex -ge $tableNodes.Count) {
        throw "Table index $TableIndex was not found in word/document.xml for $DocxPath."
    }

    $rowNodes = $tableNodes[$TableIndex].SelectNodes("w:tr", $namespaceManager)
    if ($null -eq $rowNodes -or $RowIndex -ge $rowNodes.Count) {
        throw "Row index $RowIndex in table $TableIndex was not found in word/document.xml for $DocxPath."
    }

    $cellNodes = $rowNodes[$RowIndex].SelectNodes("w:tc", $namespaceManager)
    $firstCellGridSpan = $null
    if ($cellNodes.Count -gt 0) {
        $gridSpanNode = $cellNodes[0].SelectSingleNode("w:tcPr/w:gridSpan", $namespaceManager)
        if ($null -ne $gridSpanNode) {
            $firstCellGridSpan = [int]$gridSpanNode.GetAttribute("val", $script:WordMlNamespace)
        }
    }

    return [pscustomobject][ordered]@{
        table_index = $TableIndex
        row_index = $RowIndex
        cell_count = $cellNodes.Count
        first_cell_grid_span = $firstCellGridSpan
    }
}

function Assert-BodyTableRowXmlState {
    param(
        [object]$State,
        [int]$ExpectedCellCount,
        [AllowNull()][int]$ExpectedFirstCellGridSpan,
        [string]$Label
    )

    if ([int]$State.cell_count -ne $ExpectedCellCount) {
        throw "$Label expected XML cell_count $ExpectedCellCount, got $($State.cell_count)."
    }

    if ($null -eq $ExpectedFirstCellGridSpan) {
        if ($null -ne $State.first_cell_grid_span) {
            throw "$Label expected XML first_cell_grid_span=null, got '$($State.first_cell_grid_span)'."
        }
    } elseif ([int]$State.first_cell_grid_span -ne $ExpectedFirstCellGridSpan) {
        throw "$Label expected XML first_cell_grid_span $ExpectedFirstCellGridSpan, got '$($State.first_cell_grid_span)'."
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

    Write-Step "Building featherdoc_cli and table cell/merge visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_table_cell_merge_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_table_cell_merge_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"
$baselineSetTextCellJson = Join-Path $resolvedOutputDir "shared-baseline-table-0-target-cell.json"
$baselineMergeRowJson = Join-Path $resolvedOutputDir "shared-baseline-table-1-row-0.json"
$baselineMergeAnchorJson = Join-Path $resolvedOutputDir "shared-baseline-table-1-anchor.json"
$baselineUnmergeRowJson = Join-Path $resolvedOutputDir "shared-baseline-table-2-row-0.json"
$baselineUnmergeAnchorJson = Join-Path $resolvedOutputDir "shared-baseline-table-2-anchor.json"

& $sampleExecutable $baselineDocxPath
if ($LASTEXITCODE -ne 0) {
    throw "Failed to generate shared table cell/merge baseline sample."
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-cells", $baselineDocxPath, "0", "--row", "1", "--cell", "1", "--json") `
    -OutputPath $baselineSetTextCellJson `
    -FailureMessage "Failed to inspect shared baseline table 0 target cell."
Assert-TableCellState `
    -JsonPath $baselineSetTextCellJson `
    -ExpectedTableIndex 0 `
    -ExpectedRowIndex 1 `
    -ExpectedCellIndex 1 `
    -ExpectedColumnSpan 1 `
    -ExpectedText "ORIGINAL BODY TARGET" `
    -Label "shared-baseline-table-0-target-cell"

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-cells", $baselineDocxPath, "1", "--row", "0", "--json") `
    -OutputPath $baselineMergeRowJson `
    -FailureMessage "Failed to inspect shared baseline table 1 row 0."
Assert-TableRowState `
    -JsonPath $baselineMergeRowJson `
    -ExpectedTableIndex 1 `
    -ExpectedRowIndex 0 `
    -ExpectedTexts @("MERGE ANCHOR", "SOURCE CELL", "TAIL CELL") `
    -Label "shared-baseline-table-1-row-0"

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-cells", $baselineDocxPath, "1", "--row", "0", "--cell", "0", "--json") `
    -OutputPath $baselineMergeAnchorJson `
    -FailureMessage "Failed to inspect shared baseline table 1 anchor cell."
Assert-TableCellState `
    -JsonPath $baselineMergeAnchorJson `
    -ExpectedTableIndex 1 `
    -ExpectedRowIndex 0 `
    -ExpectedCellIndex 0 `
    -ExpectedColumnSpan 1 `
    -ExpectedText "MERGE ANCHOR" `
    -Label "shared-baseline-table-1-anchor"

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-cells", $baselineDocxPath, "2", "--row", "0", "--json") `
    -OutputPath $baselineUnmergeRowJson `
    -FailureMessage "Failed to inspect shared baseline table 2 row 0."
Assert-TableRowState `
    -JsonPath $baselineUnmergeRowJson `
    -ExpectedTableIndex 2 `
    -ExpectedRowIndex 0 `
    -ExpectedTexts @("UNMERGE ANCHOR", "TAIL CELL") `
    -Label "shared-baseline-table-2-row-0"

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-cells", $baselineDocxPath, "2", "--row", "0", "--cell", "0", "--json") `
    -OutputPath $baselineUnmergeAnchorJson `
    -FailureMessage "Failed to inspect shared baseline table 2 anchor cell."
Assert-TableCellState `
    -JsonPath $baselineUnmergeAnchorJson `
    -ExpectedTableIndex 2 `
    -ExpectedRowIndex 0 `
    -ExpectedCellIndex 0 `
    -ExpectedColumnSpan 2 `
    -ExpectedText "UNMERGE ANCHOR" `
    -Label "shared-baseline-table-2-anchor"

$baselineMergeRowXmlState = Get-BodyTableRowXmlState -DocxPath $baselineDocxPath -TableIndex 1 -RowIndex 0
Assert-BodyTableRowXmlState `
    -State $baselineMergeRowXmlState `
    -ExpectedCellCount 3 `
    -ExpectedFirstCellGridSpan $null `
    -Label "shared-baseline-table-1-row-0-xml"

$baselineUnmergeRowXmlState = Get-BodyTableRowXmlState -DocxPath $baselineDocxPath -TableIndex 2 -RowIndex 0
Assert-BodyTableRowXmlState `
    -State $baselineUnmergeRowXmlState `
    -ExpectedCellCount 2 `
    -ExpectedFirstCellGridSpan 2 `
    -Label "shared-baseline-table-2-row-0-xml"

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
    [pscustomobject][ordered]@{
        id = "set-table-cell-text-body-visual"
        command = "set-table-cell-text"
        arguments = @("0", "1", "1", "--text", "UPDATED BODY TARGET")
        expected_visual_cues = @(
            "The yellow bottom-right body target cell changes text from ORIGINAL BODY TARGET to UPDATED BODY TARGET without changing table geometry.",
            "The amber merge target table remains a three-column grid in this case.",
            "The violet unmerge banner stays merged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "merge-table-cells-right-header-visual"
        command = "merge-table-cells"
        arguments = @("1", "0", "0", "--direction", "right", "--count", "1")
        expected_visual_cues = @(
            "The amber MERGE ANCHOR cell expands across the first two header columns and the SOURCE CELL label disappears.",
            "The green tail column remains visible as a separate third column.",
            "The set-table-cell-text and unmerge target tables remain structurally unchanged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "unmerge-table-cells-right-banner-visual"
        command = "unmerge-table-cells"
        arguments = @("2", "0", "0", "--direction", "right")
        expected_visual_cues = @(
            "The violet UNMERGE ANCHOR banner splits back into three visible columns.",
            "The restored middle column is blank but bordered, so the extra divider becomes visible again.",
            "The set-table-cell-text and merge target tables remain structurally unchanged in this case."
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
    $visualDir = Join-Path $caseDir "mutated-visual"
    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    $arguments = @($case.command, $baselineDocxPath)
    $arguments += $case.arguments
    $arguments += @("--output", $mutatedDocxPath, "--json")

    Write-Step "Running case '$($case.id)'"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments $arguments `
        -OutputPath $mutationJsonPath `
        -FailureMessage "Failed to run case '$($case.id)'."

    if ($case.command -eq "set-table-cell-text") {
        Assert-SetTableCellTextResult `
            -JsonPath $mutationJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 1 `
            -ExpectedCellIndex 1 `
            -Label $case.id
    } elseif ($case.command -eq "merge-table-cells") {
        Assert-MergeTableCellsResult `
            -JsonPath $mutationJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedCount 1 `
            -Label $case.id
    } elseif ($case.command -eq "unmerge-table-cells") {
        Assert-UnmergeTableCellsResult `
            -JsonPath $mutationJsonPath `
            -ExpectedTableIndex 2 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -Label $case.id
    } else {
        throw "Unsupported command '$($case.command)' in case '$($case.id)'."
    }

    $setTextCellJsonPath = Join-Path $caseDir "inspect-table-0-target-cell.json"
    $mergeRowJsonPath = Join-Path $caseDir "inspect-table-1-row-0.json"
    $mergeAnchorJsonPath = Join-Path $caseDir "inspect-table-1-anchor.json"
    $unmergeRowJsonPath = Join-Path $caseDir "inspect-table-2-row-0.json"
    $unmergeAnchorJsonPath = Join-Path $caseDir "inspect-table-2-anchor.json"

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-cells", $mutatedDocxPath, "0", "--row", "1", "--cell", "1", "--json") `
        -OutputPath $setTextCellJsonPath `
        -FailureMessage "Failed to inspect table 0 target cell for case '$($case.id)'."

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-cells", $mutatedDocxPath, "1", "--row", "0", "--json") `
        -OutputPath $mergeRowJsonPath `
        -FailureMessage "Failed to inspect table 1 row 0 for case '$($case.id)'."

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-cells", $mutatedDocxPath, "1", "--row", "0", "--cell", "0", "--json") `
        -OutputPath $mergeAnchorJsonPath `
        -FailureMessage "Failed to inspect table 1 anchor cell for case '$($case.id)'."

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-cells", $mutatedDocxPath, "2", "--row", "0", "--json") `
        -OutputPath $unmergeRowJsonPath `
        -FailureMessage "Failed to inspect table 2 row 0 for case '$($case.id)'."

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-cells", $mutatedDocxPath, "2", "--row", "0", "--cell", "0", "--json") `
        -OutputPath $unmergeAnchorJsonPath `
        -FailureMessage "Failed to inspect table 2 anchor cell for case '$($case.id)'."

    $mergeRowXmlState = Get-BodyTableRowXmlState -DocxPath $mutatedDocxPath -TableIndex 1 -RowIndex 0
    $unmergeRowXmlState = Get-BodyTableRowXmlState -DocxPath $mutatedDocxPath -TableIndex 2 -RowIndex 0

    if ($case.id -eq "set-table-cell-text-body-visual") {
        Assert-TableCellState `
            -JsonPath $setTextCellJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 1 `
            -ExpectedCellIndex 1 `
            -ExpectedColumnSpan 1 `
            -ExpectedText "UPDATED BODY TARGET" `
            -Label "$($case.id)-table-0-target-cell"
        Assert-TableRowState `
            -JsonPath $mergeRowJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedTexts @("MERGE ANCHOR", "SOURCE CELL", "TAIL CELL") `
            -Label "$($case.id)-table-1-row-0"
        Assert-TableCellState `
            -JsonPath $mergeAnchorJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedColumnSpan 1 `
            -ExpectedText "MERGE ANCHOR" `
            -Label "$($case.id)-table-1-anchor"
        Assert-TableRowState `
            -JsonPath $unmergeRowJsonPath `
            -ExpectedTableIndex 2 `
            -ExpectedRowIndex 0 `
            -ExpectedTexts @("UNMERGE ANCHOR", "TAIL CELL") `
            -Label "$($case.id)-table-2-row-0"
        Assert-TableCellState `
            -JsonPath $unmergeAnchorJsonPath `
            -ExpectedTableIndex 2 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedColumnSpan 2 `
            -ExpectedText "UNMERGE ANCHOR" `
            -Label "$($case.id)-table-2-anchor"
        Assert-BodyTableRowXmlState `
            -State $mergeRowXmlState `
            -ExpectedCellCount 3 `
            -ExpectedFirstCellGridSpan $null `
            -Label "$($case.id)-table-1-row-0-xml"
        Assert-BodyTableRowXmlState `
            -State $unmergeRowXmlState `
            -ExpectedCellCount 2 `
            -ExpectedFirstCellGridSpan 2 `
            -Label "$($case.id)-table-2-row-0-xml"
    } elseif ($case.id -eq "merge-table-cells-right-header-visual") {
        Assert-TableCellState `
            -JsonPath $setTextCellJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 1 `
            -ExpectedCellIndex 1 `
            -ExpectedColumnSpan 1 `
            -ExpectedText "ORIGINAL BODY TARGET" `
            -Label "$($case.id)-table-0-target-cell"
        Assert-TableRowState `
            -JsonPath $mergeRowJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedTexts @("MERGE ANCHOR", "TAIL CELL") `
            -Label "$($case.id)-table-1-row-0"
        Assert-TableCellState `
            -JsonPath $mergeAnchorJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedColumnSpan 2 `
            -ExpectedText "MERGE ANCHOR" `
            -Label "$($case.id)-table-1-anchor"
        Assert-TableRowState `
            -JsonPath $unmergeRowJsonPath `
            -ExpectedTableIndex 2 `
            -ExpectedRowIndex 0 `
            -ExpectedTexts @("UNMERGE ANCHOR", "TAIL CELL") `
            -Label "$($case.id)-table-2-row-0"
        Assert-TableCellState `
            -JsonPath $unmergeAnchorJsonPath `
            -ExpectedTableIndex 2 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedColumnSpan 2 `
            -ExpectedText "UNMERGE ANCHOR" `
            -Label "$($case.id)-table-2-anchor"
        Assert-BodyTableRowXmlState `
            -State $mergeRowXmlState `
            -ExpectedCellCount 2 `
            -ExpectedFirstCellGridSpan 2 `
            -Label "$($case.id)-table-1-row-0-xml"
        Assert-BodyTableRowXmlState `
            -State $unmergeRowXmlState `
            -ExpectedCellCount 2 `
            -ExpectedFirstCellGridSpan 2 `
            -Label "$($case.id)-table-2-row-0-xml"
    } elseif ($case.id -eq "unmerge-table-cells-right-banner-visual") {
        Assert-TableCellState `
            -JsonPath $setTextCellJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 1 `
            -ExpectedCellIndex 1 `
            -ExpectedColumnSpan 1 `
            -ExpectedText "ORIGINAL BODY TARGET" `
            -Label "$($case.id)-table-0-target-cell"
        Assert-TableRowState `
            -JsonPath $mergeRowJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedTexts @("MERGE ANCHOR", "SOURCE CELL", "TAIL CELL") `
            -Label "$($case.id)-table-1-row-0"
        Assert-TableCellState `
            -JsonPath $mergeAnchorJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedColumnSpan 1 `
            -ExpectedText "MERGE ANCHOR" `
            -Label "$($case.id)-table-1-anchor"
        Assert-TableRowState `
            -JsonPath $unmergeRowJsonPath `
            -ExpectedTableIndex 2 `
            -ExpectedRowIndex 0 `
            -ExpectedTexts @("UNMERGE ANCHOR", "", "TAIL CELL") `
            -Label "$($case.id)-table-2-row-0"
        Assert-TableCellState `
            -JsonPath $unmergeAnchorJsonPath `
            -ExpectedTableIndex 2 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedColumnSpan 1 `
            -ExpectedText "UNMERGE ANCHOR" `
            -Label "$($case.id)-table-2-anchor"
        Assert-BodyTableRowXmlState `
            -State $mergeRowXmlState `
            -ExpectedCellCount 3 `
            -ExpectedFirstCellGridSpan $null `
            -Label "$($case.id)-table-1-row-0-xml"
        Assert-BodyTableRowXmlState `
            -State $unmergeRowXmlState `
            -ExpectedCellCount 3 `
            -ExpectedFirstCellGridSpan $null `
            -Label "$($case.id)-table-2-row-0-xml"
    } else {
        throw "Unsupported case id '$($case.id)'."
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
        inspect_table_0_target_cell_json = $setTextCellJsonPath
        inspect_table_1_row_json = $mergeRowJsonPath
        inspect_table_1_anchor_json = $mergeAnchorJsonPath
        inspect_table_2_row_json = $unmergeRowJsonPath
        inspect_table_2_anchor_json = $unmergeAnchorJsonPath
        table_1_row_styles_xml = $mergeRowXmlState
        table_2_row_styles_xml = $unmergeRowXmlState
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
    shared_baseline_visual = [ordered]@{
        root = $baselineVisualDir
        page = (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label "shared-baseline")
    }
    shared_baseline_artifacts = [ordered]@{
        table_0_target_cell = $baselineSetTextCellJson
        table_1_row = $baselineMergeRowJson
        table_1_anchor = $baselineMergeAnchorJson
        table_2_row = $baselineUnmergeRowJson
        table_2_anchor = $baselineUnmergeAnchorJson
        table_1_row_styles_xml = $baselineMergeRowXmlState
        table_2_row_styles_xml = $baselineUnmergeRowXmlState
    }
    cases = $summaryCases
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 10) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed table cell/merge visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
