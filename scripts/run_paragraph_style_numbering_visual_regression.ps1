param(
    [string]$BuildDir = "build-paragraph-style-numbering-visual-nmake",
    [string]$OutputDir = "output/paragraph-style-numbering-visual-regression",
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
    Write-Host "[paragraph-style-numbering-visual] $Message"
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

function Assert-DefinitionLevels {
    param(
        [object[]]$ActualLevels,
        [object[]]$ExpectedLevels,
        [string]$Label
    )

    $resolvedActualLevels = @()
    if ($null -ne $ActualLevels) {
        $resolvedActualLevels = @($ActualLevels)
    }
    if ($resolvedActualLevels.Count -ne $ExpectedLevels.Count) {
        throw "$Label expected $($ExpectedLevels.Count) definition level(s), got $($resolvedActualLevels.Count)."
    }

    for ($index = 0; $index -lt $ExpectedLevels.Count; $index++) {
        $actual = $resolvedActualLevels[$index]
        $expected = $ExpectedLevels[$index]
        if ([int]$actual.level -ne [int]$expected.level) {
            throw "$Label expected level[$index]=$($expected.level), got $($actual.level)."
        }
        if ([string]$actual.kind -ne [string]$expected.kind) {
            throw "$Label expected kind[$index]='$($expected.kind)', got '$($actual.kind)'."
        }
        if ([int]$actual.start -ne [int]$expected.start) {
            throw "$Label expected start[$index]=$($expected.start), got $($actual.start)."
        }
        if ([string]$actual.text_pattern -ne [string]$expected.text_pattern) {
            throw "$Label expected text_pattern[$index]='$($expected.text_pattern)', got '$($actual.text_pattern)'."
        }
    }
}

function Assert-SetStyleNumberingResult {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedDefinitionName,
        [int]$ExpectedStyleLevel,
        [object[]]$ExpectedLevels,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "set-paragraph-style-numbering") {
        throw "$Label expected command set-paragraph-style-numbering, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }
    if ([string]$payload.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($payload.style_id)'."
    }
    if ([string]$payload.definition_name -ne $ExpectedDefinitionName) {
        throw "$Label expected definition_name '$ExpectedDefinitionName', got '$($payload.definition_name)'."
    }
    if ([int]$payload.style_level -ne $ExpectedStyleLevel) {
        throw "$Label expected style_level $ExpectedStyleLevel, got $($payload.style_level)."
    }

    Assert-DefinitionLevels `
        -ActualLevels $payload.definition_levels `
        -ExpectedLevels $ExpectedLevels `
        -Label $Label
}

function Assert-EnsureStyleLinkedNumberingResult {
    param(
        [string]$JsonPath,
        [string]$ExpectedDefinitionName,
        [object[]]$ExpectedLevels,
        [object[]]$ExpectedStyleLinks,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "ensure-style-linked-numbering") {
        throw "$Label expected command ensure-style-linked-numbering, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }
    if ([string]$payload.definition_name -ne $ExpectedDefinitionName) {
        throw "$Label expected definition_name '$ExpectedDefinitionName', got '$($payload.definition_name)'."
    }

    Assert-DefinitionLevels `
        -ActualLevels $payload.definition_levels `
        -ExpectedLevels $ExpectedLevels `
        -Label $Label

    if ($payload.style_links.Count -ne $ExpectedStyleLinks.Count) {
        throw "$Label expected $($ExpectedStyleLinks.Count) style links, got $($payload.style_links.Count)."
    }

    for ($index = 0; $index -lt $ExpectedStyleLinks.Count; $index++) {
        $actual = $payload.style_links[$index]
        $expected = $ExpectedStyleLinks[$index]
        if ([string]$actual.style_id -ne [string]$expected.style_id) {
            throw "$Label expected style_id[$index] '$($expected.style_id)', got '$($actual.style_id)'."
        }
        if ([int]$actual.level -ne [int]$expected.level) {
            throw "$Label expected level[$index] $($expected.level), got $($actual.level)."
        }
    }
}

function Assert-ClearStyleNumberingResult {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "clear-paragraph-style-numbering") {
        throw "$Label expected command clear-paragraph-style-numbering, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }
    if ([string]$payload.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($payload.style_id)'."
    }
}

function Assert-ParagraphStyleState {
    param(
        [string]$JsonPath,
        [int]$ExpectedIndex,
        [string]$ExpectedText,
        [string]$ExpectedStyleId,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $paragraph = $payload.paragraph
    if ($null -eq $paragraph) {
        throw "$Label expected paragraph payload."
    }
    if ([int]$paragraph.index -ne $ExpectedIndex) {
        throw "$Label expected paragraph index $ExpectedIndex, got $($paragraph.index)."
    }
    if ([int]$paragraph.section_index -ne 0) {
        throw "$Label expected section_index 0, got $($paragraph.section_index)."
    }
    if ([string]$paragraph.text -ne $ExpectedText) {
        throw "$Label expected text '$ExpectedText', got '$($paragraph.text)'."
    }
    if ([string]$paragraph.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($paragraph.style_id)'."
    }
    if ([bool]$paragraph.section_break) {
        throw "$Label expected section_break=false."
    }
    if ($null -ne $paragraph.numbering) {
        throw "$Label expected direct paragraph numbering metadata to remain null."
    }
}

function Assert-StyleState {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedName,
        [string]$ExpectedBasedOn,
        [bool]$ExpectNumbering,
        [AllowNull()][int]$ExpectedLevel,
        [AllowNull()][string]$ExpectedDefinitionName,
        [int]$ExpectedOverrideCount,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $style = $payload.style
    if ($null -eq $style) {
        throw "$Label expected style payload."
    }
    if ([string]$style.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($style.style_id)'."
    }
    if ([string]$style.name -ne $ExpectedName) {
        throw "$Label expected style name '$ExpectedName', got '$($style.name)'."
    }
    if ([string]$style.based_on -ne $ExpectedBasedOn) {
        throw "$Label expected based_on '$ExpectedBasedOn', got '$($style.based_on)'."
    }
    if ([string]$style.kind -ne "paragraph") {
        throw "$Label expected kind paragraph, got '$($style.kind)'."
    }
    if ([string]$style.type -ne "paragraph") {
        throw "$Label expected type paragraph, got '$($style.type)'."
    }
    if (-not [bool]$style.is_custom) {
        throw "$Label expected is_custom=true."
    }

    if ($ExpectNumbering) {
        if ($null -eq $style.numbering) {
            throw "$Label expected style numbering metadata."
        }
        $numbering = $style.numbering
        if ($null -eq $numbering.num_id) {
            throw "$Label expected num_id."
        }
        if ($null -eq $numbering.definition_id) {
            throw "$Label expected definition_id."
        }
        if ($null -eq $numbering.level) {
            throw "$Label expected numbering level."
        }
        if ([int]$numbering.level -ne [int]$ExpectedLevel) {
            throw "$Label expected numbering level $ExpectedLevel, got $($numbering.level)."
        }
        if ([string]$numbering.definition_name -ne $ExpectedDefinitionName) {
            throw "$Label expected definition_name '$ExpectedDefinitionName', got '$($numbering.definition_name)'."
        }

        $instance = $numbering.instance
        if ($null -eq $instance) {
            throw "$Label expected numbering instance payload."
        }
        if ([int]$instance.instance_id -ne [int]$numbering.num_id) {
            throw "$Label expected instance_id $($numbering.num_id), got $($instance.instance_id)."
        }

        $overrides = @()
        if ($null -ne $instance.level_overrides) {
            $overrides = @($instance.level_overrides)
        }
        if ($overrides.Count -ne $ExpectedOverrideCount) {
            throw "$Label expected $ExpectedOverrideCount level override(s), got $($overrides.Count)."
        }

        return [ordered]@{
            style_id = $ExpectedStyleId
            name = $ExpectedName
            based_on = $ExpectedBasedOn
            num_id = [int]$numbering.num_id
            definition_id = [int]$numbering.definition_id
            level = [int]$numbering.level
            definition_name = [string]$numbering.definition_name
            override_count = $overrides.Count
        }
    }

    if ($null -ne $style.numbering) {
        throw "$Label expected numbering=null."
    }

    return [ordered]@{
        style_id = $ExpectedStyleId
        name = $ExpectedName
        based_on = $ExpectedBasedOn
        num_id = $null
        definition_id = $null
        level = $null
        definition_name = $null
        override_count = 0
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

function Get-StyleXmlState {
    param(
        [string]$DocxPath,
        [string]$StyleId
    )

    $stylesXml = Read-DocxEntryText -DocxPath $DocxPath -EntryName "word/styles.xml"
    $xmlDocument = New-Object System.Xml.XmlDocument
    $xmlDocument.LoadXml($stylesXml)

    $namespaceManager = New-Object System.Xml.XmlNamespaceManager($xmlDocument.NameTable)
    $namespaceManager.AddNamespace("w", $script:WordMlNamespace)

    $styleNode = $xmlDocument.SelectSingleNode(
        "/w:styles/w:style[@w:styleId='$StyleId']",
        $namespaceManager)
    if ($null -eq $styleNode) {
        throw "Style '$StyleId' was not found in word/styles.xml for $DocxPath."
    }

    $numPrNode = $styleNode.SelectSingleNode("w:pPr/w:numPr", $namespaceManager)
    $bidiNode = $styleNode.SelectSingleNode("w:pPr/w:bidi", $namespaceManager)
    $numIdNode = $styleNode.SelectSingleNode("w:pPr/w:numPr/w:numId", $namespaceManager)
    $levelNode = $styleNode.SelectSingleNode("w:pPr/w:numPr/w:ilvl", $namespaceManager)

    $numId = $null
    if ($null -ne $numIdNode) {
        $numId = [int]$numIdNode.GetAttribute("val", $script:WordMlNamespace)
    }

    $level = $null
    if ($null -ne $levelNode) {
        $level = [int]$levelNode.GetAttribute("val", $script:WordMlNamespace)
    }

    return [ordered]@{
        style_id = $StyleId
        has_num_pr = ($null -ne $numPrNode)
        has_bidi = ($null -ne $bidiNode)
        num_id = $numId
        level = $level
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

    Write-Step "Building featherdoc_cli and paragraph style numbering visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_paragraph_style_numbering_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_paragraph_style_numbering_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$legalHeadingText = "Legal heading target: this paragraph already uses the LegalHeading style, so style numbering should appear only after the CLI links a numbering definition to that style."
$legalSubheadingText = "Legal subheading target: this paragraph already uses the LegalSubheading style and should become a nested style-numbered child only in the dual-style case."
$bodyNumberedText = "Body-numbered clear target: this paragraph starts with baseline style numbering and paragraph bidi enabled, so clearing the style numbering should remove only the visible marker."

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared paragraph style numbering baseline sample."

$paragraphExpectations = @(
    [ordered]@{
        id = "legal-heading"
        paragraph_index = 1
        style_id = "LegalHeading"
        text = $legalHeadingText
    },
    [ordered]@{
        id = "legal-subheading"
        paragraph_index = 2
        style_id = "LegalSubheading"
        text = $legalSubheadingText
    },
    [ordered]@{
        id = "body-numbered"
        paragraph_index = 3
        style_id = "BodyNumbered"
        text = $bodyNumberedText
    }
)

$styleExpectations = @(
    [ordered]@{
        style_id = "LegalHeading"
        name = "Legal Heading"
        based_on = "Heading1"
        expect_numbering = $false
        level = $null
        definition_name = $null
        override_count = 0
    },
    [ordered]@{
        style_id = "LegalSubheading"
        name = "Legal Subheading"
        based_on = "Heading2"
        expect_numbering = $false
        level = $null
        definition_name = $null
        override_count = 0
    },
    [ordered]@{
        style_id = "BodyNumbered"
        name = "Body Numbered"
        based_on = "Normal"
        expect_numbering = $true
        level = 0
        definition_name = "BodyStyleOutlineBaseline"
        override_count = 0
    }
)

$summaryBaselineParagraphs = @()
foreach ($paragraphExpectation in $paragraphExpectations) {
    $inspectParagraphJsonPath =
        Join-Path $resolvedOutputDir "baseline-$($paragraphExpectation.id)-paragraph.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @(
            "inspect-paragraphs",
            $baselineDocxPath,
            "--paragraph",
            "$($paragraphExpectation.paragraph_index)",
            "--json"
        ) `
        -OutputPath $inspectParagraphJsonPath `
        -FailureMessage "Failed to inspect baseline paragraph '$($paragraphExpectation.id)'."

    Assert-ParagraphStyleState `
        -JsonPath $inspectParagraphJsonPath `
        -ExpectedIndex $paragraphExpectation.paragraph_index `
        -ExpectedText $paragraphExpectation.text `
        -ExpectedStyleId $paragraphExpectation.style_id `
        -Label "baseline-$($paragraphExpectation.id)"

    $summaryBaselineParagraphs += [ordered]@{
        id = $paragraphExpectation.id
        paragraph_index = $paragraphExpectation.paragraph_index
        style_id = $paragraphExpectation.style_id
        text = $paragraphExpectation.text
        inspect_paragraph_json = $inspectParagraphJsonPath
    }
}

$baselineStyleStates = @{}
$summaryBaselineStyles = @()
foreach ($styleExpectation in $styleExpectations) {
    $inspectStyleJsonPath =
        Join-Path $resolvedOutputDir "baseline-$($styleExpectation.style_id)-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @(
            "inspect-styles",
            $baselineDocxPath,
            "--style",
            $styleExpectation.style_id,
            "--json"
        ) `
        -OutputPath $inspectStyleJsonPath `
        -FailureMessage "Failed to inspect baseline style '$($styleExpectation.style_id)'."

    $styleState = Assert-StyleState `
        -JsonPath $inspectStyleJsonPath `
        -ExpectedStyleId $styleExpectation.style_id `
        -ExpectedName $styleExpectation.name `
        -ExpectedBasedOn $styleExpectation.based_on `
        -ExpectNumbering $styleExpectation.expect_numbering `
        -ExpectedLevel $styleExpectation.level `
        -ExpectedDefinitionName $styleExpectation.definition_name `
        -ExpectedOverrideCount $styleExpectation.override_count `
        -Label "baseline-$($styleExpectation.style_id)"

    $baselineStyleStates[$styleExpectation.style_id] = $styleState
    $summaryBaselineStyles += [ordered]@{
        style_id = $styleExpectation.style_id
        inspect_style_json = $inspectStyleJsonPath
        state = $styleState
    }
}

$baselineBodyNumberedXmlState =
    Get-StyleXmlState -DocxPath $baselineDocxPath -StyleId "BodyNumbered"
if (-not $baselineBodyNumberedXmlState.has_num_pr) {
    throw "Baseline BodyNumbered style was expected to keep style-level numPr markup."
}
if (-not $baselineBodyNumberedXmlState.has_bidi) {
    throw "Baseline BodyNumbered style was expected to keep w:bidi markup."
}
if ([int]$baselineBodyNumberedXmlState.num_id -ne
    [int]$baselineStyleStates["BodyNumbered"].num_id) {
    throw "Baseline BodyNumbered XML numId did not match inspect-styles metadata."
}
if ([int]$baselineBodyNumberedXmlState.level -ne 0) {
    throw "Baseline BodyNumbered XML level was expected to be 0."
}

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

$headingOnlyLevels = @(
    [ordered]@{
        level = 0
        kind = "decimal"
        start = 5
        text_pattern = "(%1)"
    }
)

$nestedLevels = @(
    [ordered]@{
        level = 0
        kind = "decimal"
        start = 7
        text_pattern = "(%1)"
    },
    [ordered]@{
        level = 1
        kind = "decimal"
        start = 1
        text_pattern = "(%1.%2)"
    }
)

$caseDefinitions = @(
    [ordered]@{
        id = "set-style-numbering-legal-heading-visual"
        mutation_steps = @(
            [ordered]@{
                command = "set-paragraph-style-numbering"
                style_id = "LegalHeading"
                definition_name = "LegalHeadingStyleOutlineVisual"
                style_level = 0
                definition_levels = $headingOnlyLevels
            }
        )
        expected_styles = @(
            [ordered]@{
                style_id = "LegalHeading"
                name = "Legal Heading"
                based_on = "Heading1"
                expect_numbering = $true
                level = 0
                definition_name = "LegalHeadingStyleOutlineVisual"
                override_count = 0
                instance_group = "heading-outline"
            },
            [ordered]@{
                style_id = "LegalSubheading"
                name = "Legal Subheading"
                based_on = "Heading2"
                expect_numbering = $false
                level = $null
                definition_name = $null
                override_count = 0
            },
            [ordered]@{
                style_id = "BodyNumbered"
                name = "Body Numbered"
                based_on = "Normal"
                expect_numbering = $true
                level = 0
                definition_name = "BodyStyleOutlineBaseline"
                override_count = 0
                compare_to_baseline = $true
            }
        )
        expected_visual_cues = @(
            "The LegalHeading paragraph gains a visible decimal marker that starts at (5).",
            "The LegalSubheading paragraph stays unnumbered while the BodyNumbered clear target keeps its baseline [4] style marker.",
            "The page remains single-page with the trailing anchor still below the three styled paragraphs."
        )
    },
    [ordered]@{
        id = "set-style-numbering-nested-outline-visual"
        mutation_steps = @(
            [ordered]@{
                command = "set-paragraph-style-numbering"
                style_id = "LegalHeading"
                definition_name = "LegalHeadingNestedStyleOutlineVisual"
                style_level = 0
                definition_levels = $nestedLevels
            },
            [ordered]@{
                command = "set-paragraph-style-numbering"
                style_id = "LegalSubheading"
                definition_name = "LegalHeadingNestedStyleOutlineVisual"
                style_level = 1
                definition_levels = $nestedLevels
            }
        )
        expected_styles = @(
            [ordered]@{
                style_id = "LegalHeading"
                name = "Legal Heading"
                based_on = "Heading1"
                expect_numbering = $true
                level = 0
                definition_name = "LegalHeadingNestedStyleOutlineVisual"
                override_count = 0
                instance_group = "nested-outline"
            },
            [ordered]@{
                style_id = "LegalSubheading"
                name = "Legal Subheading"
                based_on = "Heading2"
                expect_numbering = $true
                level = 1
                definition_name = "LegalHeadingNestedStyleOutlineVisual"
                override_count = 0
                instance_group = "nested-outline"
            },
            [ordered]@{
                style_id = "BodyNumbered"
                name = "Body Numbered"
                based_on = "Normal"
                expect_numbering = $true
                level = 0
                definition_name = "BodyStyleOutlineBaseline"
                override_count = 0
                compare_to_baseline = $true
            }
        )
        expected_visual_cues = @(
            "The LegalHeading paragraph gains a root marker that starts at (7).",
            "The LegalSubheading paragraph renders as a nested child marker (7.1) under the same style-linked numbering instance.",
            "The BodyNumbered clear target keeps its baseline [4] marker so the nested-heading effect is isolated to the two heading styles."
        )
    },
    [ordered]@{
        id = "ensure-style-linked-numbering-nested-outline-visual"
        mutation_steps = @(
            [ordered]@{
                command = "ensure-style-linked-numbering"
                definition_name = "LegalHeadingNestedStyleOutlineBatchVisual"
                definition_levels = $nestedLevels
                style_links = @(
                    [ordered]@{ style_id = "LegalHeading"; level = 0 },
                    [ordered]@{ style_id = "LegalSubheading"; level = 1 }
                )
            }
        )
        expected_styles = @(
            [ordered]@{
                style_id = "LegalHeading"
                name = "Legal Heading"
                based_on = "Heading1"
                expect_numbering = $true
                level = 0
                definition_name = "LegalHeadingNestedStyleOutlineBatchVisual"
                override_count = 0
                instance_group = "nested-outline-batch"
            },
            [ordered]@{
                style_id = "LegalSubheading"
                name = "Legal Subheading"
                based_on = "Heading2"
                expect_numbering = $true
                level = 1
                definition_name = "LegalHeadingNestedStyleOutlineBatchVisual"
                override_count = 0
                instance_group = "nested-outline-batch"
            },
            [ordered]@{
                style_id = "BodyNumbered"
                name = "Body Numbered"
                based_on = "Normal"
                expect_numbering = $true
                level = 0
                definition_name = "BodyStyleOutlineBaseline"
                override_count = 0
                compare_to_baseline = $true
            }
        )
        expected_visual_cues = @(
            "The LegalHeading paragraph gains a root marker that starts at (7).",
            "The LegalSubheading paragraph renders as a nested child marker (7.1) under the same style-linked numbering instance even though the whole outline link is created in one CLI step.",
            "The BodyNumbered clear target keeps its baseline [4] marker so the batched style-linking effect is isolated to the two heading styles."
        )
    },
    [ordered]@{
        id = "clear-style-numbering-body-numbered-visual"
        mutation_steps = @(
            [ordered]@{
                command = "clear-paragraph-style-numbering"
                style_id = "BodyNumbered"
            }
        )
        expected_styles = @(
            [ordered]@{
                style_id = "LegalHeading"
                name = "Legal Heading"
                based_on = "Heading1"
                expect_numbering = $false
                level = $null
                definition_name = $null
                override_count = 0
            },
            [ordered]@{
                style_id = "LegalSubheading"
                name = "Legal Subheading"
                based_on = "Heading2"
                expect_numbering = $false
                level = $null
                definition_name = $null
                override_count = 0
            },
            [ordered]@{
                style_id = "BodyNumbered"
                name = "Body Numbered"
                based_on = "Normal"
                expect_numbering = $false
                level = $null
                definition_name = $null
                override_count = 0
            }
        )
        expected_visual_cues = @(
            "The BodyNumbered clear target drops its baseline [4] style marker and returns to plain body alignment.",
            "The paragraph still uses the BodyNumbered style, so the regression is about removing style numbering rather than clearing the paragraph style itself.",
            "The bidi markup on BodyNumbered stays in styles.xml even though the style-level numPr node is removed."
        )
        assert_body_numbered_xml_after_clear = $true
    }
)

$summaryCases = @()
$aggregateImages = @()
$aggregateLabels = @()

foreach ($case in $caseDefinitions) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $visualDir = Join-Path $caseDir "mutated-visual"
    $currentInputDocx = $baselineDocxPath
    $mutationArtifacts = @()

    Write-Step "Running case '$($case.id)'"

    for ($stepIndex = 0; $stepIndex -lt $case.mutation_steps.Count; $stepIndex++) {
        $step = $case.mutation_steps[$stepIndex]
        $isFinalStep = ($stepIndex -eq ($case.mutation_steps.Count - 1))
        $stepOutputDocx = if ($isFinalStep) {
            $mutatedDocxPath
        } else {
            Join-Path $caseDir ("mutation-step-{0:D2}.docx" -f ($stepIndex + 1))
        }
        $stepJsonPath = Join-Path $caseDir ("mutation-step-{0:D2}.json" -f ($stepIndex + 1))

        if ($step.command -eq "set-paragraph-style-numbering") {
            $arguments = @(
                "set-paragraph-style-numbering",
                $currentInputDocx,
                $step.style_id,
                "--definition-name",
                $step.definition_name
            )

            foreach ($definitionLevel in $step.definition_levels) {
                $arguments += @(
                    "--numbering-level",
                    "$($definitionLevel.level):$($definitionLevel.kind):$($definitionLevel.start):$($definitionLevel.text_pattern)"
                )
            }

            $arguments += @(
                "--style-level",
                "$($step.style_level)",
                "--output",
                $stepOutputDocx,
                "--json"
            )

            Invoke-Capture `
                -Executable $cliExecutable `
                -Arguments $arguments `
                -OutputPath $stepJsonPath `
                -FailureMessage "Failed to execute mutation step $($stepIndex + 1) for case '$($case.id)'."

            Assert-SetStyleNumberingResult `
                -JsonPath $stepJsonPath `
                -ExpectedStyleId $step.style_id `
                -ExpectedDefinitionName $step.definition_name `
                -ExpectedStyleLevel $step.style_level `
                -ExpectedLevels $step.definition_levels `
                -Label "$($case.id)-step-$($stepIndex + 1)"
        } elseif ($step.command -eq "ensure-style-linked-numbering") {
            $arguments = @(
                "ensure-style-linked-numbering",
                $currentInputDocx,
                "--definition-name",
                $step.definition_name
            )

            foreach ($definitionLevel in $step.definition_levels) {
                $arguments += @(
                    "--numbering-level",
                    "$($definitionLevel.level):$($definitionLevel.kind):$($definitionLevel.start):$($definitionLevel.text_pattern)"
                )
            }

            foreach ($styleLink in $step.style_links) {
                $arguments += @(
                    "--style-link",
                    "$($styleLink.style_id):$($styleLink.level)"
                )
            }

            $arguments += @(
                "--output",
                $stepOutputDocx,
                "--json"
            )

            Invoke-Capture `
                -Executable $cliExecutable `
                -Arguments $arguments `
                -OutputPath $stepJsonPath `
                -FailureMessage "Failed to execute mutation step $($stepIndex + 1) for case '$($case.id)'."

            Assert-EnsureStyleLinkedNumberingResult `
                -JsonPath $stepJsonPath `
                -ExpectedDefinitionName $step.definition_name `
                -ExpectedLevels $step.definition_levels `
                -ExpectedStyleLinks $step.style_links `
                -Label "$($case.id)-step-$($stepIndex + 1)"
        } elseif ($step.command -eq "clear-paragraph-style-numbering") {
            $arguments = @(
                "clear-paragraph-style-numbering",
                $currentInputDocx,
                $step.style_id,
                "--output",
                $stepOutputDocx,
                "--json"
            )

            Invoke-Capture `
                -Executable $cliExecutable `
                -Arguments $arguments `
                -OutputPath $stepJsonPath `
                -FailureMessage "Failed to execute mutation step $($stepIndex + 1) for case '$($case.id)'."

            Assert-ClearStyleNumberingResult `
                -JsonPath $stepJsonPath `
                -ExpectedStyleId $step.style_id `
                -Label "$($case.id)-step-$($stepIndex + 1)"
        } else {
            throw "Unsupported mutation command '$($step.command)' in case '$($case.id)'."
        }

        $mutationArtifacts += [ordered]@{
            step = $stepIndex + 1
            command = $step.command
            input_docx = $currentInputDocx
            output_docx = $stepOutputDocx
            mutation_json = $stepJsonPath
        }

        $currentInputDocx = $stepOutputDocx
    }

    $paragraphArtifacts = @()
    foreach ($paragraphExpectation in $paragraphExpectations) {
        $inspectParagraphJsonPath =
            Join-Path $caseDir "inspect-paragraph-$($paragraphExpectation.id).json"
        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @(
                "inspect-paragraphs",
                $mutatedDocxPath,
                "--paragraph",
                "$($paragraphExpectation.paragraph_index)",
                "--json"
            ) `
            -OutputPath $inspectParagraphJsonPath `
            -FailureMessage "Failed to inspect paragraph '$($paragraphExpectation.id)' for case '$($case.id)'."

        Assert-ParagraphStyleState `
            -JsonPath $inspectParagraphJsonPath `
            -ExpectedIndex $paragraphExpectation.paragraph_index `
            -ExpectedText $paragraphExpectation.text `
            -ExpectedStyleId $paragraphExpectation.style_id `
            -Label "$($case.id)-$($paragraphExpectation.id)"

        $paragraphArtifacts += [ordered]@{
            id = $paragraphExpectation.id
            paragraph_index = $paragraphExpectation.paragraph_index
            style_id = $paragraphExpectation.style_id
            inspect_paragraph_json = $inspectParagraphJsonPath
        }
    }

    $styleArtifacts = @()
    $caseStyleStates = @{}
    $groupStates = @{}
    foreach ($styleExpectation in $case.expected_styles) {
        $inspectStyleJsonPath =
            Join-Path $caseDir "inspect-style-$($styleExpectation.style_id).json"
        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @(
                "inspect-styles",
                $mutatedDocxPath,
                "--style",
                $styleExpectation.style_id,
                "--json"
            ) `
            -OutputPath $inspectStyleJsonPath `
            -FailureMessage "Failed to inspect style '$($styleExpectation.style_id)' for case '$($case.id)'."

        $styleState = Assert-StyleState `
            -JsonPath $inspectStyleJsonPath `
            -ExpectedStyleId $styleExpectation.style_id `
            -ExpectedName $styleExpectation.name `
            -ExpectedBasedOn $styleExpectation.based_on `
            -ExpectNumbering $styleExpectation.expect_numbering `
            -ExpectedLevel $styleExpectation.level `
            -ExpectedDefinitionName $styleExpectation.definition_name `
            -ExpectedOverrideCount $styleExpectation.override_count `
            -Label "$($case.id)-$($styleExpectation.style_id)"

        if ($styleExpectation.compare_to_baseline) {
            $baselineStyleState = $baselineStyleStates[$styleExpectation.style_id]
            if ($null -eq $baselineStyleState) {
                throw "Missing baseline style state for '$($styleExpectation.style_id)'."
            }
            if ($styleState.num_id -ne $baselineStyleState.num_id) {
                throw "$($case.id) unexpectedly changed num_id for style '$($styleExpectation.style_id)'."
            }
            if ($styleState.definition_id -ne $baselineStyleState.definition_id) {
                throw "$($case.id) unexpectedly changed definition_id for style '$($styleExpectation.style_id)'."
            }
            if ($styleState.level -ne $baselineStyleState.level) {
                throw "$($case.id) unexpectedly changed style level for '$($styleExpectation.style_id)'."
            }
            if ($styleState.definition_name -ne $baselineStyleState.definition_name) {
                throw "$($case.id) unexpectedly changed definition_name for '$($styleExpectation.style_id)'."
            }
        }

        if ($styleExpectation.instance_group) {
            if (-not $groupStates.ContainsKey($styleExpectation.instance_group)) {
                $groupStates[$styleExpectation.instance_group] = @()
            }
            $groupStates[$styleExpectation.instance_group] += $styleState
        }

        $caseStyleStates[$styleExpectation.style_id] = $styleState
        $styleArtifacts += [ordered]@{
            style_id = $styleExpectation.style_id
            inspect_style_json = $inspectStyleJsonPath
            state = $styleState
        }
    }

    foreach ($groupName in $groupStates.Keys) {
        $groupEntries = @($groupStates[$groupName])
        if ($groupEntries.Count -lt 2) {
            continue
        }

        $referenceEntry = $groupEntries[0]
        for ($index = 1; $index -lt $groupEntries.Count; $index++) {
            $candidateEntry = $groupEntries[$index]
            if ($candidateEntry.num_id -ne $referenceEntry.num_id) {
                throw "$($case.id) expected instance group '$groupName' to share one num_id."
            }
            if ($candidateEntry.definition_id -ne $referenceEntry.definition_id) {
                throw "$($case.id) expected instance group '$groupName' to share one definition_id."
            }
        }
    }

    $xmlAssertions = $null
    if ($case.assert_body_numbered_xml_after_clear) {
        $bodyNumberedXmlState =
            Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId "BodyNumbered"
        if ($bodyNumberedXmlState.has_num_pr) {
            throw "$($case.id) expected BodyNumbered w:numPr to be removed from styles.xml."
        }
        if (-not $bodyNumberedXmlState.has_bidi) {
            throw "$($case.id) expected BodyNumbered w:bidi to remain in styles.xml."
        }

        $xmlAssertions = [ordered]@{
            body_numbered_before = $baselineBodyNumberedXmlState
            body_numbered_after = $bodyNumberedXmlState
        }
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
        source_docx = $baselineDocxPath
        mutated_docx = $mutatedDocxPath
        mutation_steps = $mutationArtifacts
        paragraph_artifacts = $paragraphArtifacts
        style_artifacts = $styleArtifacts
        xml_assertions = $xmlAssertions
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
    shared_baseline_paragraphs = $summaryBaselineParagraphs
    shared_baseline_styles = $summaryBaselineStyles
    shared_baseline_styles_xml = [ordered]@{
        body_numbered = $baselineBodyNumberedXmlState
    }
    shared_baseline_visual = [ordered]@{
        root = $baselineVisualDir
        page = (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label "shared-baseline")
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

Write-Step "Completed paragraph style numbering visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
