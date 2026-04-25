param(
    [string]$BuildDir = "build-table-row-repeat-header-visual-nmake",
    [string]$OutputDir = "output/table-row-repeat-header-visual-regression",
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
    Write-Host "[table-row-repeat-header-visual] $Message"
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

function Assert-TableRowState {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [int]$ExpectedCellCount,
        [AllowNull()][int]$ExpectedHeightTwips,
        [AllowNull()][string]$ExpectedHeightRule,
        [bool]$ExpectedCantSplit,
        [bool]$ExpectedRepeatsHeader,
        [string[]]$ExpectedCellTexts,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $row = $payload.row
    if ($null -eq $row) {
        throw "$Label expected single-row payload."
    }
    if ([int]$payload.table_index -ne $ExpectedTableIndex) {
        throw "$Label expected table_index $ExpectedTableIndex, got $($payload.table_index)."
    }
    if ([int]$row.row_index -ne $ExpectedRowIndex) {
        throw "$Label expected row_index $ExpectedRowIndex, got $($row.row_index)."
    }
    if ([int]$row.cell_count -ne $ExpectedCellCount) {
        throw "$Label expected cell_count $ExpectedCellCount, got $($row.cell_count)."
    }

    if ($null -eq $ExpectedHeightTwips) {
        if ($null -ne $row.height_twips) {
            throw "$Label expected height_twips=null, got $($row.height_twips)."
        }
    } elseif ([int]$row.height_twips -ne $ExpectedHeightTwips) {
        throw "$Label expected height_twips $ExpectedHeightTwips, got $($row.height_twips)."
    }

    if ($null -eq $ExpectedHeightRule) {
        if ($null -ne $row.height_rule) {
            throw "$Label expected height_rule=null, got '$($row.height_rule)'."
        }
    } elseif ([string]$row.height_rule -ne $ExpectedHeightRule) {
        throw "$Label expected height_rule '$ExpectedHeightRule', got '$($row.height_rule)'."
    }

    if ([bool]$row.cant_split -ne $ExpectedCantSplit) {
        throw "$Label expected cant_split=$ExpectedCantSplit, got $($row.cant_split)."
    }
    if ([bool]$row.repeats_header -ne $ExpectedRepeatsHeader) {
        throw "$Label expected repeats_header=$ExpectedRepeatsHeader, got $($row.repeats_header)."
    }

    if ($row.cell_texts.Count -ne $ExpectedCellTexts.Count) {
        throw "$Label expected $($ExpectedCellTexts.Count) cell_texts, got $($row.cell_texts.Count)."
    }

    for ($index = 0; $index -lt $ExpectedCellTexts.Count; $index++) {
        if ([string]$row.cell_texts[$index] -ne [string]$ExpectedCellTexts[$index]) {
            throw "$Label expected cell_text[$index] '$($ExpectedCellTexts[$index])', got '$($row.cell_texts[$index])'."
        }
    }
}

function Assert-SetRepeatHeaderResult {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "set-table-row-repeat-header") {
        throw "$Label expected command set-table-row-repeat-header, got '$($payload.command)'."
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
    if (-not [bool]$payload.repeats_header) {
        throw "$Label expected repeats_header=true."
    }
}

function Assert-ClearRepeatHeaderResult {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "clear-table-row-repeat-header") {
        throw "$Label expected command clear-table-row-repeat-header, got '$($payload.command)'."
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

function Get-BodyTableRowXmlRepeatHeaderState {
    param(
        [string]$DocxPath,
        [int]$TableIndex,
        [int]$RowIndex
    )

    $xmlText = Read-DocxEntryText -DocxPath $DocxPath -EntryName "word/document.xml"
    $xml = New-Object System.Xml.XmlDocument
    $xml.LoadXml($xmlText)

    $namespaceManager = New-Object System.Xml.XmlNamespaceManager($xml.NameTable)
    $namespaceManager.AddNamespace("w", $script:WordMlNamespace)

    $rowNode = $xml.SelectSingleNode("/w:document/w:body/w:tbl[$($TableIndex + 1)]/w:tr[$($RowIndex + 1)]", $namespaceManager)
    if ($null -eq $rowNode) {
        throw "Could not locate table $TableIndex row $RowIndex in $DocxPath."
    }

    $rowProperties = $rowNode.SelectSingleNode("w:trPr", $namespaceManager)
    $repeatHeader = $rowNode.SelectSingleNode("w:trPr/w:tblHeader", $namespaceManager)

    return [pscustomobject][ordered]@{
        has_row_properties = $null -ne $rowProperties
        has_repeat_header = $null -ne $repeatHeader
        value = if ($null -ne $repeatHeader) { [string]$repeatHeader.GetAttribute("val", $script:WordMlNamespace) } else { $null }
    }
}

function Assert-BodyTableRowXmlRepeatHeaderState {
    param(
        [pscustomobject]$State,
        [bool]$ExpectedHasRowProperties,
        [bool]$ExpectedHasRepeatHeader,
        [AllowNull()][string]$ExpectedValue,
        [string]$Label
    )

    if ([bool]$State.has_row_properties -ne $ExpectedHasRowProperties) {
        throw "$Label expected has_row_properties=$ExpectedHasRowProperties, got $($State.has_row_properties)."
    }
    if ([bool]$State.has_repeat_header -ne $ExpectedHasRepeatHeader) {
        throw "$Label expected has_repeat_header=$ExpectedHasRepeatHeader, got $($State.has_repeat_header)."
    }
    if ($null -eq $ExpectedValue) {
        if ($null -ne $State.value -and $State.value -ne "") {
            throw "$Label expected tblHeader value=null, got '$($State.value)'."
        }
        return
    }

    if ([string]$State.value -ne $ExpectedValue) {
        throw "$Label expected XML tblHeader value '$ExpectedValue', got '$($State.value)'."
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

    Write-Step "Building featherdoc_cli and table row repeat-header visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_table_row_repeat_header_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_table_row_repeat_header_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$baselineDefinitions = @(
    [pscustomobject][ordered]@{
        id = "set-repeat-header"
        mode = "set"
        baseline_docx = Join-Path $resolvedOutputDir "baseline-set-repeat-header.docx"
        baseline_visual_dir = Join-Path $resolvedOutputDir "baseline-set-repeat-header-visual"
        baseline_row_json = Join-Path $resolvedOutputDir "baseline-set-repeat-header-row.json"
        expected_height_twips = 780
        expected_height_rule = "exact"
        expected_repeats_header = $false
        expected_cell_texts = @("SET HEADER", "PAGE 2 GAINS THIS BAND", "ROW 0 TARGET")
        expected_xml = [ordered]@{
            has_row_properties = $true
            has_repeat_header = $false
            value = $null
        }
    },
    [pscustomobject][ordered]@{
        id = "clear-repeat-header"
        mode = "clear"
        baseline_docx = Join-Path $resolvedOutputDir "baseline-clear-repeat-header.docx"
        baseline_visual_dir = Join-Path $resolvedOutputDir "baseline-clear-repeat-header-visual"
        baseline_row_json = Join-Path $resolvedOutputDir "baseline-clear-repeat-header-row.json"
        expected_height_twips = 780
        expected_height_rule = "exact"
        expected_repeats_header = $true
        expected_cell_texts = @("CLEAR HEADER", "PAGE 2 LOSES THIS BAND", "ROW 0 TARGET")
        expected_xml = [ordered]@{
            has_row_properties = $true
            has_repeat_header = $true
            value = "1"
        }
    }
)

$baselineArtifacts = @{}
foreach ($baseline in $baselineDefinitions) {
    Write-Step "Generating baseline '$($baseline.id)'"
    & $sampleExecutable $baseline.baseline_docx $baseline.mode
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to generate baseline '$($baseline.id)'."
    }

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-rows", $baseline.baseline_docx, "0", "--row", "0", "--json") `
        -OutputPath $baseline.baseline_row_json `
        -FailureMessage "Failed to inspect baseline '$($baseline.id)' target row."
    Assert-TableRowState `
        -JsonPath $baseline.baseline_row_json `
        -ExpectedTableIndex 0 `
        -ExpectedRowIndex 0 `
        -ExpectedCellCount 3 `
        -ExpectedHeightTwips $baseline.expected_height_twips `
        -ExpectedHeightRule $baseline.expected_height_rule `
        -ExpectedCantSplit $false `
        -ExpectedRepeatsHeader $baseline.expected_repeats_header `
        -ExpectedCellTexts $baseline.expected_cell_texts `
        -Label "baseline-$($baseline.id)-row"

    $baselineXmlState = Get-BodyTableRowXmlRepeatHeaderState `
        -DocxPath $baseline.baseline_docx `
        -TableIndex 0 `
        -RowIndex 0
    Assert-BodyTableRowXmlRepeatHeaderState `
        -State $baselineXmlState `
        -ExpectedHasRowProperties $baseline.expected_xml.has_row_properties `
        -ExpectedHasRepeatHeader $baseline.expected_xml.has_repeat_header `
        -ExpectedValue $baseline.expected_xml.value `
        -Label "baseline-$($baseline.id)-xml"

    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $baseline.baseline_docx `
        -OutputDir $baseline.baseline_visual_dir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent
    Assert-VisualPageCount -VisualOutputDir $baseline.baseline_visual_dir -ExpectedCount 2 -Label "baseline-$($baseline.id)-visual"

    $baselineArtifacts[$baseline.id] = [pscustomobject][ordered]@{
        id = $baseline.id
        mode = $baseline.mode
        docx = $baseline.baseline_docx
        visual_dir = $baseline.baseline_visual_dir
        row_json = $baseline.baseline_row_json
        xml_state = $baselineXmlState
        page_2 = (Get-RenderedPagePath -VisualOutputDir $baseline.baseline_visual_dir -PageNumber 2 -Label "baseline-$($baseline.id)")
        expected_cell_texts = $baseline.expected_cell_texts
    }
}

$caseDefinitions = @(
    [pscustomobject][ordered]@{
        id = "set-table-row-repeat-header-visual"
        command = "set-table-row-repeat-header"
        baseline_id = "set-repeat-header"
        arguments = @("0", "0")
        expected_repeats_header = $true
        expected_cell_texts = @("SET HEADER", "PAGE 2 GAINS THIS BAND", "ROW 0 TARGET")
        expected_xml = [ordered]@{
            has_row_properties = $true
            has_repeat_header = $true
            value = "1"
        }
        expected_visual_cues = @(
            "Page 2 gains the blue header band at the top only after the mutation runs.",
            "The table body rows remain in the same order while the repeated header appears above them.",
            "The row height and table geometry stay fixed; only the repeat-header behavior changes."
        )
    },
    [pscustomobject][ordered]@{
        id = "clear-table-row-repeat-header-visual"
        command = "clear-table-row-repeat-header"
        baseline_id = "clear-repeat-header"
        arguments = @("0", "0")
        expected_repeats_header = $false
        expected_cell_texts = @("CLEAR HEADER", "PAGE 2 LOSES THIS BAND", "ROW 0 TARGET")
        expected_xml = [ordered]@{
            has_row_properties = $true
            has_repeat_header = $false
            value = $null
        }
        expected_visual_cues = @(
            "Page 2 loses the yellow repeated header band after the mutation runs.",
            "The table body rows remain in the same order and continue directly from page 1.",
            "The row height and table geometry stay fixed; only the repeat-header behavior changes."
        )
    }
)

$summaryCases = @()
$aggregateImages = @()
$aggregateLabels = @()

foreach ($case in $caseDefinitions) {
    $baseline = $baselineArtifacts[$case.baseline_id]
    $caseDir = Join-Path $resolvedOutputDir $case.id
    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $mutationJsonPath = Join-Path $caseDir "mutation_result.json"
    $targetRowJsonPath = Join-Path $caseDir "target_row.json"
    $visualDir = Join-Path $caseDir "mutated-visual"
    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    $arguments = @($case.command, $baseline.docx)
    $arguments += $case.arguments
    $arguments += @("--output", $mutatedDocxPath, "--json")

    Write-Step "Running case '$($case.id)'"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments $arguments `
        -OutputPath $mutationJsonPath `
        -FailureMessage "Failed to run case '$($case.id)'."

    if ($case.command -eq "set-table-row-repeat-header") {
        Assert-SetRepeatHeaderResult `
            -JsonPath $mutationJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 0 `
            -Label $case.id
    } elseif ($case.command -eq "clear-table-row-repeat-header") {
        Assert-ClearRepeatHeaderResult `
            -JsonPath $mutationJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 0 `
            -Label $case.id
    } else {
        throw "Unhandled case command '$($case.command)'."
    }

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-rows", $mutatedDocxPath, "0", "--row", "0", "--json") `
        -OutputPath $targetRowJsonPath `
        -FailureMessage "Failed to inspect mutated target row for case '$($case.id)'."
    Assert-TableRowState `
        -JsonPath $targetRowJsonPath `
        -ExpectedTableIndex 0 `
        -ExpectedRowIndex 0 `
        -ExpectedCellCount 3 `
        -ExpectedHeightTwips 780 `
        -ExpectedHeightRule "exact" `
        -ExpectedCantSplit $false `
        -ExpectedRepeatsHeader $case.expected_repeats_header `
        -ExpectedCellTexts $case.expected_cell_texts `
        -Label "$($case.id)-target-row"

    $targetXmlState = Get-BodyTableRowXmlRepeatHeaderState `
        -DocxPath $mutatedDocxPath `
        -TableIndex 0 `
        -RowIndex 0
    Assert-BodyTableRowXmlRepeatHeaderState `
        -State $targetXmlState `
        -ExpectedHasRowProperties $case.expected_xml.has_row_properties `
        -ExpectedHasRepeatHeader $case.expected_xml.has_repeat_header `
        -ExpectedValue $case.expected_xml.value `
        -Label "$($case.id)-xml"

    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $mutatedDocxPath `
        -OutputDir $visualDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent
    Assert-VisualPageCount -VisualOutputDir $visualDir -ExpectedCount 2 -Label "$($case.id)-visual"

    $mutatedPage2 = Get-RenderedPagePath -VisualOutputDir $visualDir -PageNumber 2 -Label $case.id
    $comparisonLabels = @(
        "$($case.id) before page 2",
        "$($case.id) after page 2"
    )
    $artifacts = Register-ComparisonArtifacts `
        -Python $renderPython `
        -ContactSheetScript $contactSheetScript `
        -AggregatePagesDir $aggregatePagesDir `
        -CaseOutputDir $caseDir `
        -CaseId $case.id `
        -Images @($baseline.page_2, $mutatedPage2) `
        -Labels $comparisonLabels

    $aggregateImages += $artifacts.selected_pages
    $aggregateLabels += $artifacts.labels
    $summaryCases += [ordered]@{
        id = $case.id
        command = $case.command
        source_docx = $baseline.docx
        mutated_docx = $mutatedDocxPath
        mutation_json = $mutationJsonPath
        target_row_json = $targetRowJsonPath
        target_row_repeat_header_xml = $targetXmlState
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
    baselines = @(
        [ordered]@{
            id = "set-repeat-header"
            docx = $baselineArtifacts["set-repeat-header"].docx
            visual_dir = $baselineArtifacts["set-repeat-header"].visual_dir
            page_2 = $baselineArtifacts["set-repeat-header"].page_2
            target_row_json = $baselineArtifacts["set-repeat-header"].row_json
            target_row_repeat_header_xml = $baselineArtifacts["set-repeat-header"].xml_state
        },
        [ordered]@{
            id = "clear-repeat-header"
            docx = $baselineArtifacts["clear-repeat-header"].docx
            visual_dir = $baselineArtifacts["clear-repeat-header"].visual_dir
            page_2 = $baselineArtifacts["clear-repeat-header"].page_2
            target_row_json = $baselineArtifacts["clear-repeat-header"].row_json
            target_row_repeat_header_xml = $baselineArtifacts["clear-repeat-header"].xml_state
        }
    )
    cases = $summaryCases
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 10) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed table row repeat-header visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
