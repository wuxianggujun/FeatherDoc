param(
    [string]$BuildDir = "build-table-cell-margin-visual-nmake",
    [string]$OutputDir = "output/table-cell-margin-visual-regression",
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
    Write-Host "[table-cell-margin-visual] $Message"
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

function Assert-TableCellStableState {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [int]$ExpectedCellIndex,
        [int]$ExpectedColumnSpan,
        [int]$ExpectedWidthTwips,
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
    if ([int]$cell.width_twips -ne $ExpectedWidthTwips) {
        throw "$Label expected width_twips $ExpectedWidthTwips, got $($cell.width_twips)."
    }
    if ([string]$cell.text -ne $ExpectedText) {
        throw "$Label expected text '$ExpectedText', got '$($cell.text)'."
    }
}

function Assert-SetTableCellMarginResult {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [int]$ExpectedCellIndex,
        [string]$ExpectedEdge,
        [int]$ExpectedMarginTwips,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "set-table-cell-margin") {
        throw "$Label expected command set-table-cell-margin, got '$($payload.command)'."
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
    if ([string]$payload.edge -ne $ExpectedEdge) {
        throw "$Label expected edge '$ExpectedEdge', got '$($payload.edge)'."
    }
    if ([int]$payload.margin_twips -ne $ExpectedMarginTwips) {
        throw "$Label expected margin_twips $ExpectedMarginTwips, got $($payload.margin_twips)."
    }
}

function Assert-ClearTableCellMarginResult {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [int]$ExpectedCellIndex,
        [string]$ExpectedEdge,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "clear-table-cell-margin") {
        throw "$Label expected command clear-table-cell-margin, got '$($payload.command)'."
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
    if ([string]$payload.edge -ne $ExpectedEdge) {
        throw "$Label expected edge '$ExpectedEdge', got '$($payload.edge)'."
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

function Get-BodyTableCellXmlMarginState {
    param(
        [string]$DocxPath,
        [int]$TableIndex,
        [int]$RowIndex,
        [int]$CellIndex,
        [string]$EdgeName
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
    if ($null -eq $cellNodes -or $CellIndex -ge $cellNodes.Count) {
        throw "Cell index $CellIndex in table $TableIndex row $RowIndex was not found in word/document.xml for $DocxPath."
    }

    $marginsNode = $cellNodes[$CellIndex].SelectSingleNode("w:tcPr/w:tcMar", $namespaceManager)
    $hasMargins = $null -ne $marginsNode
    $edgeNode = $null
    if ($hasMargins) {
        $edgeNode = $marginsNode.SelectSingleNode("w:$EdgeName", $namespaceManager)
    }

    $hasEdge = $null -ne $edgeNode
    $type = $null
    $value = $null
    if ($hasEdge) {
        $typeAttribute = $edgeNode.GetAttribute("type", $script:WordMlNamespace)
        $valueAttribute = $edgeNode.GetAttribute("w", $script:WordMlNamespace)
        if (-not [string]::IsNullOrWhiteSpace($typeAttribute)) {
            $type = $typeAttribute
        }
        if (-not [string]::IsNullOrWhiteSpace($valueAttribute)) {
            $value = $valueAttribute
        }
    }

    return [pscustomobject][ordered]@{
        table_index = $TableIndex
        row_index = $RowIndex
        cell_index = $CellIndex
        edge = $EdgeName
        has_margins = $hasMargins
        has_edge = $hasEdge
        type = $type
        value = $value
    }
}

function Assert-BodyTableCellXmlMarginState {
    param(
        [object]$State,
        [bool]$ExpectedHasMargins,
        [bool]$ExpectedHasEdge,
        [AllowNull()][string]$ExpectedType,
        [AllowNull()][string]$ExpectedValue,
        [string]$Label
    )

    if ([bool]$State.has_margins -ne $ExpectedHasMargins) {
        throw "$Label expected has_margins=$ExpectedHasMargins, got $($State.has_margins)."
    }
    if ([bool]$State.has_edge -ne $ExpectedHasEdge) {
        throw "$Label expected has_edge=$ExpectedHasEdge, got $($State.has_edge)."
    }

    if (-not $ExpectedHasEdge) {
        return
    }

    if ([string]$State.type -ne $ExpectedType) {
        throw "$Label expected XML margin type '$ExpectedType', got '$($State.type)'."
    }
    if ([string]$State.value -ne $ExpectedValue) {
        throw "$Label expected XML margin value '$ExpectedValue', got '$($State.value)'."
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

    Write-Step "Building featherdoc_cli and table cell margin visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_table_cell_margin_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_table_cell_margin_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"
$baselineSetTargetJson = Join-Path $resolvedOutputDir "shared-baseline-table-0-target-cell.json"
$baselineClearTargetJson = Join-Path $resolvedOutputDir "shared-baseline-table-1-target-cell.json"

& $sampleExecutable $baselineDocxPath
if ($LASTEXITCODE -ne 0) {
    throw "Failed to generate shared table cell margin baseline sample."
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-cells", $baselineDocxPath, "0", "--row", "0", "--cell", "0", "--json") `
    -OutputPath $baselineSetTargetJson `
    -FailureMessage "Failed to inspect shared baseline table 0 target cell."
Assert-TableCellStableState `
    -JsonPath $baselineSetTargetJson `
    -ExpectedTableIndex 0 `
    -ExpectedRowIndex 0 `
    -ExpectedCellIndex 0 `
    -ExpectedColumnSpan 1 `
    -ExpectedWidthTwips 3200 `
    -ExpectedText "LEFT EDGE`nSHIFT ME" `
    -Label "shared-baseline-table-0-target-cell"

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-cells", $baselineDocxPath, "1", "--row", "0", "--cell", "0", "--json") `
    -OutputPath $baselineClearTargetJson `
    -FailureMessage "Failed to inspect shared baseline table 1 target cell."
Assert-TableCellStableState `
    -JsonPath $baselineClearTargetJson `
    -ExpectedTableIndex 1 `
    -ExpectedRowIndex 0 `
    -ExpectedCellIndex 0 `
    -ExpectedColumnSpan 1 `
    -ExpectedWidthTwips 3200 `
    -ExpectedText "CLEAR LEFT`nBACK TO EDGE" `
    -Label "shared-baseline-table-1-target-cell"

$baselineSetTargetXmlState = Get-BodyTableCellXmlMarginState `
    -DocxPath $baselineDocxPath `
    -TableIndex 0 `
    -RowIndex 0 `
    -CellIndex 0 `
    -EdgeName "left"
Assert-BodyTableCellXmlMarginState `
    -State $baselineSetTargetXmlState `
    -ExpectedHasMargins $false `
    -ExpectedHasEdge $false `
    -ExpectedType $null `
    -ExpectedValue $null `
    -Label "shared-baseline-table-0-target-left-margin-xml"

$baselineClearTargetXmlState = Get-BodyTableCellXmlMarginState `
    -DocxPath $baselineDocxPath `
    -TableIndex 1 `
    -RowIndex 0 `
    -CellIndex 0 `
    -EdgeName "left"
Assert-BodyTableCellXmlMarginState `
    -State $baselineClearTargetXmlState `
    -ExpectedHasMargins $true `
    -ExpectedHasEdge $true `
    -ExpectedType "dxa" `
    -ExpectedValue "720" `
    -Label "shared-baseline-table-1-target-left-margin-xml"

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
        id = "set-table-cell-margin-left-720-visual"
        command = "set-table-cell-margin"
        arguments = @("0", "0", "0", "left", "720")
        expected_visual_cues = @(
            "The blue table keeps the same outer width and border box while the text shifts right inside the cell.",
            "Both text lines in table 0 sit farther from the left border after the 720-twip left margin is applied.",
            "The yellow table in table 1 remains unchanged in this mutation."
        )
    },
    [pscustomobject][ordered]@{
        id = "clear-table-cell-margin-left-720-visual"
        command = "clear-table-cell-margin"
        arguments = @("1", "0", "0", "left")
        expected_visual_cues = @(
            "The yellow table keeps the same outer width and border box while the text moves back toward the left border.",
            "Removing the 720-twip left margin makes the two text lines in table 1 visibly less indented.",
            "The blue table in table 0 remains unchanged in this mutation."
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

    if ($case.command -eq "set-table-cell-margin") {
        Assert-SetTableCellMarginResult `
            -JsonPath $mutationJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedEdge "left" `
            -ExpectedMarginTwips 720 `
            -Label $case.id
    } elseif ($case.command -eq "clear-table-cell-margin") {
        Assert-ClearTableCellMarginResult `
            -JsonPath $mutationJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedEdge "left" `
            -Label $case.id
    } else {
        throw "Unsupported case command '$($case.command)'."
    }

    $setTargetJsonPath = Join-Path $caseDir "inspect-table-0-target-cell.json"
    $clearTargetJsonPath = Join-Path $caseDir "inspect-table-1-target-cell.json"

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-cells", $mutatedDocxPath, "0", "--row", "0", "--cell", "0", "--json") `
        -OutputPath $setTargetJsonPath `
        -FailureMessage "Failed to inspect table 0 target cell for case '$($case.id)'."

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-cells", $mutatedDocxPath, "1", "--row", "0", "--cell", "0", "--json") `
        -OutputPath $clearTargetJsonPath `
        -FailureMessage "Failed to inspect table 1 target cell for case '$($case.id)'."

    $setTargetXmlState = Get-BodyTableCellXmlMarginState `
        -DocxPath $mutatedDocxPath `
        -TableIndex 0 `
        -RowIndex 0 `
        -CellIndex 0 `
        -EdgeName "left"
    $clearTargetXmlState = Get-BodyTableCellXmlMarginState `
        -DocxPath $mutatedDocxPath `
        -TableIndex 1 `
        -RowIndex 0 `
        -CellIndex 0 `
        -EdgeName "left"

    if ($case.id -eq "set-table-cell-margin-left-720-visual") {
        Assert-TableCellStableState `
            -JsonPath $setTargetJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedColumnSpan 1 `
            -ExpectedWidthTwips 3200 `
            -ExpectedText "LEFT EDGE`nSHIFT ME" `
            -Label "$($case.id)-table-0-target-cell"
        Assert-TableCellStableState `
            -JsonPath $clearTargetJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedColumnSpan 1 `
            -ExpectedWidthTwips 3200 `
            -ExpectedText "CLEAR LEFT`nBACK TO EDGE" `
            -Label "$($case.id)-table-1-target-cell"
        Assert-BodyTableCellXmlMarginState `
            -State $setTargetXmlState `
            -ExpectedHasMargins $true `
            -ExpectedHasEdge $true `
            -ExpectedType "dxa" `
            -ExpectedValue "720" `
            -Label "$($case.id)-table-0-target-left-margin-xml"
        Assert-BodyTableCellXmlMarginState `
            -State $clearTargetXmlState `
            -ExpectedHasMargins $true `
            -ExpectedHasEdge $true `
            -ExpectedType "dxa" `
            -ExpectedValue "720" `
            -Label "$($case.id)-table-1-target-left-margin-xml"
    } elseif ($case.id -eq "clear-table-cell-margin-left-720-visual") {
        Assert-TableCellStableState `
            -JsonPath $setTargetJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedColumnSpan 1 `
            -ExpectedWidthTwips 3200 `
            -ExpectedText "LEFT EDGE`nSHIFT ME" `
            -Label "$($case.id)-table-0-target-cell"
        Assert-TableCellStableState `
            -JsonPath $clearTargetJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedCellIndex 0 `
            -ExpectedColumnSpan 1 `
            -ExpectedWidthTwips 3200 `
            -ExpectedText "CLEAR LEFT`nBACK TO EDGE" `
            -Label "$($case.id)-table-1-target-cell"
        Assert-BodyTableCellXmlMarginState `
            -State $setTargetXmlState `
            -ExpectedHasMargins $false `
            -ExpectedHasEdge $false `
            -ExpectedType $null `
            -ExpectedValue $null `
            -Label "$($case.id)-table-0-target-left-margin-xml"
        Assert-BodyTableCellXmlMarginState `
            -State $clearTargetXmlState `
            -ExpectedHasMargins $false `
            -ExpectedHasEdge $false `
            -ExpectedType $null `
            -ExpectedValue $null `
            -Label "$($case.id)-table-1-target-left-margin-xml"
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
        inspect_table_0_target_cell_json = $setTargetJsonPath
        inspect_table_1_target_cell_json = $clearTargetJsonPath
        table_0_target_left_margin_xml = $setTargetXmlState
        table_1_target_left_margin_xml = $clearTargetXmlState
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
        table_0_target_cell = $baselineSetTargetJson
        table_1_target_cell = $baselineClearTargetJson
        table_0_target_left_margin_xml = $baselineSetTargetXmlState
        table_1_target_left_margin_xml = $baselineClearTargetXmlState
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

Write-Step "Completed table cell margin visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
