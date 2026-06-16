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
