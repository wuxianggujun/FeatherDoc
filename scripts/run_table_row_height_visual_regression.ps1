param(
    [string]$BuildDir = "build-table-row-height-visual-nmake",
    [string]$OutputDir = "output/table-row-height-visual-regression",
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
    Write-Host "[table-row-height-visual] $Message"
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

    if ($row.cell_texts.Count -ne $ExpectedCellTexts.Count) {
        throw "$Label expected $($ExpectedCellTexts.Count) cell_texts, got $($row.cell_texts.Count)."
    }

    for ($index = 0; $index -lt $ExpectedCellTexts.Count; $index++) {
        if ([string]$row.cell_texts[$index] -ne [string]$ExpectedCellTexts[$index]) {
            throw "$Label expected cell_text[$index] '$($ExpectedCellTexts[$index])', got '$($row.cell_texts[$index])'."
        }
    }
}

function Assert-SetTableRowHeightResult {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [int]$ExpectedHeightTwips,
        [string]$ExpectedHeightRule,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "set-table-row-height") {
        throw "$Label expected command set-table-row-height, got '$($payload.command)'."
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
    if ([int]$payload.height_twips -ne $ExpectedHeightTwips) {
        throw "$Label expected height_twips $ExpectedHeightTwips, got $($payload.height_twips)."
    }
    if ([string]$payload.height_rule -ne $ExpectedHeightRule) {
        throw "$Label expected height_rule '$ExpectedHeightRule', got '$($payload.height_rule)'."
    }
}

function Assert-ClearTableRowHeightResult {
    param(
        [string]$JsonPath,
        [int]$ExpectedTableIndex,
        [int]$ExpectedRowIndex,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "clear-table-row-height") {
        throw "$Label expected command clear-table-row-height, got '$($payload.command)'."
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

function Get-BodyTableRowXmlHeightState {
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

    $rowProperties = $rowNodes[$RowIndex].SelectSingleNode("w:trPr", $namespaceManager)
    $rowHeight = $rowNodes[$RowIndex].SelectSingleNode("w:trPr/w:trHeight", $namespaceManager)

    $hasRowProperties = $null -ne $rowProperties
    $hasRowHeight = $null -ne $rowHeight
    $value = $null
    $rule = $null
    if ($hasRowHeight) {
        $valueAttribute = $rowHeight.GetAttribute("val", $script:WordMlNamespace)
        $ruleAttribute = $rowHeight.GetAttribute("hRule", $script:WordMlNamespace)
        if (-not [string]::IsNullOrWhiteSpace($valueAttribute)) {
            $value = $valueAttribute
        }
        if (-not [string]::IsNullOrWhiteSpace($ruleAttribute)) {
            $rule = $ruleAttribute
        }
    }

    return [pscustomobject][ordered]@{
        table_index = $TableIndex
        row_index = $RowIndex
        has_row_properties = $hasRowProperties
        has_row_height = $hasRowHeight
        value = $value
        rule = $rule
    }
}

function Assert-BodyTableRowXmlHeightState {
    param(
        [object]$State,
        [bool]$ExpectedHasRowProperties,
        [bool]$ExpectedHasRowHeight,
        [AllowNull()][string]$ExpectedValue,
        [AllowNull()][string]$ExpectedRule,
        [string]$Label
    )

    if ([bool]$State.has_row_properties -ne $ExpectedHasRowProperties) {
        throw "$Label expected has_row_properties=$ExpectedHasRowProperties, got $($State.has_row_properties)."
    }
    if ([bool]$State.has_row_height -ne $ExpectedHasRowHeight) {
        throw "$Label expected has_row_height=$ExpectedHasRowHeight, got $($State.has_row_height)."
    }

    if (-not $ExpectedHasRowHeight) {
        return
    }

    if ([string]$State.value -ne $ExpectedValue) {
        throw "$Label expected XML trHeight value '$ExpectedValue', got '$($State.value)'."
    }
    if ([string]$State.rule -ne $ExpectedRule) {
        throw "$Label expected XML trHeight rule '$ExpectedRule', got '$($State.rule)'."
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

    Write-Step "Building featherdoc_cli and table row height visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_table_row_height_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_table_row_height_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"
$baselineSetTargetJson = Join-Path $resolvedOutputDir "shared-baseline-table-0-target-row.json"
$baselineClearTargetJson = Join-Path $resolvedOutputDir "shared-baseline-table-1-target-row.json"

& $sampleExecutable $baselineDocxPath
if ($LASTEXITCODE -ne 0) {
    throw "Failed to generate shared table row height baseline sample."
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-rows", $baselineDocxPath, "0", "--row", "0", "--json") `
    -OutputPath $baselineSetTargetJson `
    -FailureMessage "Failed to inspect shared baseline table 0 target row."
Assert-TableRowState `
    -JsonPath $baselineSetTargetJson `
    -ExpectedTableIndex 0 `
    -ExpectedRowIndex 0 `
    -ExpectedCellCount 1 `
    -ExpectedHeightTwips $null `
    -ExpectedHeightRule $null `
    -ExpectedCellTexts @("SET TARGET`nTALL EXACT") `
    -Label "shared-baseline-table-0-target-row"

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-table-rows", $baselineDocxPath, "1", "--row", "0", "--json") `
    -OutputPath $baselineClearTargetJson `
    -FailureMessage "Failed to inspect shared baseline table 1 target row."
Assert-TableRowState `
    -JsonPath $baselineClearTargetJson `
    -ExpectedTableIndex 1 `
    -ExpectedRowIndex 0 `
    -ExpectedCellCount 1 `
    -ExpectedHeightTwips 1800 `
    -ExpectedHeightRule "exact" `
    -ExpectedCellTexts @("CLEAR TARGET`nBACK TO AUTO") `
    -Label "shared-baseline-table-1-target-row"

$baselineSetTargetXmlState = Get-BodyTableRowXmlHeightState `
    -DocxPath $baselineDocxPath `
    -TableIndex 0 `
    -RowIndex 0
Assert-BodyTableRowXmlHeightState `
    -State $baselineSetTargetXmlState `
    -ExpectedHasRowProperties $false `
    -ExpectedHasRowHeight $false `
    -ExpectedValue $null `
    -ExpectedRule $null `
    -Label "shared-baseline-table-0-target-row-height-xml"

$baselineClearTargetXmlState = Get-BodyTableRowXmlHeightState `
    -DocxPath $baselineDocxPath `
    -TableIndex 1 `
    -RowIndex 0
Assert-BodyTableRowXmlHeightState `
    -State $baselineClearTargetXmlState `
    -ExpectedHasRowProperties $true `
    -ExpectedHasRowHeight $true `
    -ExpectedValue "1800" `
    -ExpectedRule "exact" `
    -Label "shared-baseline-table-1-target-row-height-xml"

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
        id = "set-table-row-height-exact-1800-visual"
        command = "set-table-row-height"
        arguments = @("0", "0", "1800", "exact")
        expected_visual_cues = @(
            "The blue target row becomes much taller while keeping the same one-cell table width and content.",
            "The exact 1800-twip row adds visible vertical whitespace around the two text lines.",
            "The yellow pre-seeded tall row remains unchanged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "clear-table-row-height-exact-1800-visual"
        command = "clear-table-row-height"
        arguments = @("1", "0")
        expected_visual_cues = @(
            "The yellow target row shrinks from the pre-seeded exact 1800-twip band back to natural content height.",
            "The row keeps the same table width and content while losing the extra vertical whitespace.",
            "The blue auto-height row remains unchanged in this case."
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

    if ($case.command -eq "set-table-row-height") {
        Assert-SetTableRowHeightResult `
            -JsonPath $mutationJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 0 `
            -ExpectedHeightTwips 1800 `
            -ExpectedHeightRule "exact" `
            -Label $case.id
    } elseif ($case.command -eq "clear-table-row-height") {
        Assert-ClearTableRowHeightResult `
            -JsonPath $mutationJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -Label $case.id
    } else {
        throw "Unsupported case command '$($case.command)'."
    }

    $setTargetJsonPath = Join-Path $caseDir "inspect-table-0-target-row.json"
    $clearTargetJsonPath = Join-Path $caseDir "inspect-table-1-target-row.json"

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-rows", $mutatedDocxPath, "0", "--row", "0", "--json") `
        -OutputPath $setTargetJsonPath `
        -FailureMessage "Failed to inspect table 0 target row for case '$($case.id)'."

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-table-rows", $mutatedDocxPath, "1", "--row", "0", "--json") `
        -OutputPath $clearTargetJsonPath `
        -FailureMessage "Failed to inspect table 1 target row for case '$($case.id)'."

    $setTargetXmlState = Get-BodyTableRowXmlHeightState `
        -DocxPath $mutatedDocxPath `
        -TableIndex 0 `
        -RowIndex 0
    $clearTargetXmlState = Get-BodyTableRowXmlHeightState `
        -DocxPath $mutatedDocxPath `
        -TableIndex 1 `
        -RowIndex 0

    if ($case.id -eq "set-table-row-height-exact-1800-visual") {
        Assert-TableRowState `
            -JsonPath $setTargetJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 0 `
            -ExpectedCellCount 1 `
            -ExpectedHeightTwips 1800 `
            -ExpectedHeightRule "exact" `
            -ExpectedCellTexts @("SET TARGET`nTALL EXACT") `
            -Label "$($case.id)-table-0-target-row"
        Assert-TableRowState `
            -JsonPath $clearTargetJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedCellCount 1 `
            -ExpectedHeightTwips 1800 `
            -ExpectedHeightRule "exact" `
            -ExpectedCellTexts @("CLEAR TARGET`nBACK TO AUTO") `
            -Label "$($case.id)-table-1-target-row"
        Assert-BodyTableRowXmlHeightState `
            -State $setTargetXmlState `
            -ExpectedHasRowProperties $true `
            -ExpectedHasRowHeight $true `
            -ExpectedValue "1800" `
            -ExpectedRule "exact" `
            -Label "$($case.id)-table-0-target-row-height-xml"
        Assert-BodyTableRowXmlHeightState `
            -State $clearTargetXmlState `
            -ExpectedHasRowProperties $true `
            -ExpectedHasRowHeight $true `
            -ExpectedValue "1800" `
            -ExpectedRule "exact" `
            -Label "$($case.id)-table-1-target-row-height-xml"
    } elseif ($case.id -eq "clear-table-row-height-exact-1800-visual") {
        Assert-TableRowState `
            -JsonPath $setTargetJsonPath `
            -ExpectedTableIndex 0 `
            -ExpectedRowIndex 0 `
            -ExpectedCellCount 1 `
            -ExpectedHeightTwips $null `
            -ExpectedHeightRule $null `
            -ExpectedCellTexts @("SET TARGET`nTALL EXACT") `
            -Label "$($case.id)-table-0-target-row"
        Assert-TableRowState `
            -JsonPath $clearTargetJsonPath `
            -ExpectedTableIndex 1 `
            -ExpectedRowIndex 0 `
            -ExpectedCellCount 1 `
            -ExpectedHeightTwips $null `
            -ExpectedHeightRule $null `
            -ExpectedCellTexts @("CLEAR TARGET`nBACK TO AUTO") `
            -Label "$($case.id)-table-1-target-row"
        Assert-BodyTableRowXmlHeightState `
            -State $setTargetXmlState `
            -ExpectedHasRowProperties $false `
            -ExpectedHasRowHeight $false `
            -ExpectedValue $null `
            -ExpectedRule $null `
            -Label "$($case.id)-table-0-target-row-height-xml"
        Assert-BodyTableRowXmlHeightState `
            -State $clearTargetXmlState `
            -ExpectedHasRowProperties $false `
            -ExpectedHasRowHeight $false `
            -ExpectedValue $null `
            -ExpectedRule $null `
            -Label "$($case.id)-table-1-target-row-height-xml"
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
        inspect_table_0_target_row_json = $setTargetJsonPath
        inspect_table_1_target_row_json = $clearTargetJsonPath
        table_0_target_row_height_xml = $setTargetXmlState
        table_1_target_row_height_xml = $clearTargetXmlState
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
        table_0_target_row = $baselineSetTargetJson
        table_1_target_row = $baselineClearTargetJson
        table_0_target_row_height_xml = $baselineSetTargetXmlState
        table_1_target_row_height_xml = $baselineClearTargetXmlState
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

Write-Step "Completed table row height visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
