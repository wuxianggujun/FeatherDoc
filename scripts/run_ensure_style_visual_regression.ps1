param(
    [string]$BuildDir = "build-ensure-style-visual-nmake",
    [string]$OutputDir = "output/ensure-style-visual-regression",
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
    Write-Host "[ensure-style-visual] $Message"
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

function Assert-ParagraphState {
    param(
        [string]$JsonPath,
        [int]$ExpectedIndex,
        [string]$ExpectedText,
        [AllowNull()][string]$ExpectedStyleId,
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
    if ([bool]$paragraph.section_break) {
        throw "$Label expected section_break=false."
    }
    if ($null -ne $paragraph.numbering) {
        throw "$Label expected direct paragraph numbering metadata to stay null."
    }

    if ($null -eq $ExpectedStyleId) {
        if ($null -ne $paragraph.style_id) {
            throw "$Label expected null style_id, got '$($paragraph.style_id)'."
        }
    } elseif ([string]$paragraph.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($paragraph.style_id)'."
    }
}

function Assert-RunState {
    param(
        [string]$JsonPath,
        [int]$ExpectedParagraphIndex,
        [int]$ExpectedRunIndex,
        [string]$ExpectedText,
        [AllowNull()][string]$ExpectedStyleId,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $run = $payload.run
    if ($null -eq $run) {
        throw "$Label expected run payload."
    }
    if ([int]$payload.paragraph_index -ne $ExpectedParagraphIndex) {
        throw "$Label expected paragraph_index $ExpectedParagraphIndex, got $($payload.paragraph_index)."
    }
    if ([int]$run.index -ne $ExpectedRunIndex) {
        throw "$Label expected run index $ExpectedRunIndex, got $($run.index)."
    }
    if ([string]$run.text -ne $ExpectedText) {
        throw "$Label expected run text '$ExpectedText', got '$($run.text)'."
    }

    if ($null -eq $ExpectedStyleId) {
        if ($null -ne $run.style_id) {
            throw "$Label expected null run style_id, got '$($run.style_id)'."
        }
    } elseif ([string]$run.style_id -ne $ExpectedStyleId) {
        throw "$Label expected run style_id '$ExpectedStyleId', got '$($run.style_id)'."
    }
}

function Assert-StyleCatalogState {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedName,
        [AllowNull()][string]$ExpectedBasedOn,
        [string]$ExpectedKind,
        [string]$ExpectedType,
        [bool]$ExpectedCustom,
        [bool]$ExpectedSemiHidden,
        [bool]$ExpectedUnhideWhenUsed,
        [bool]$ExpectedQuickFormat,
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

    if ($null -eq $ExpectedBasedOn) {
        if ($null -ne $style.based_on) {
            throw "$Label expected based_on=null, got '$($style.based_on)'."
        }
    } elseif ([string]$style.based_on -ne $ExpectedBasedOn) {
        throw "$Label expected based_on '$ExpectedBasedOn', got '$($style.based_on)'."
    }

    if ([string]$style.kind -ne $ExpectedKind) {
        throw "$Label expected kind '$ExpectedKind', got '$($style.kind)'."
    }
    if ([string]$style.type -ne $ExpectedType) {
        throw "$Label expected type '$ExpectedType', got '$($style.type)'."
    }
    if ([bool]$style.is_custom -ne $ExpectedCustom) {
        throw "$Label expected is_custom=$ExpectedCustom, got $($style.is_custom)."
    }
    if ([bool]$style.is_semi_hidden -ne $ExpectedSemiHidden) {
        throw "$Label expected is_semi_hidden=$ExpectedSemiHidden, got $($style.is_semi_hidden)."
    }
    if ([bool]$style.is_unhide_when_used -ne $ExpectedUnhideWhenUsed) {
        throw "$Label expected is_unhide_when_used=$ExpectedUnhideWhenUsed, got $($style.is_unhide_when_used)."
    }
    if ([bool]$style.is_quick_format -ne $ExpectedQuickFormat) {
        throw "$Label expected is_quick_format=$ExpectedQuickFormat, got $($style.is_quick_format)."
    }
    if ($null -ne $style.numbering) {
        throw "$Label expected numbering=null."
    }
}

function Assert-StyleInheritanceState {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedType,
        [string]$ExpectedKind,
        [AllowNull()][string]$ExpectedBasedOn,
        [string[]]$ExpectedInheritanceChain,
        [AllowNull()][string]$ExpectedFontFamilyValue,
        [AllowNull()][string]$ExpectedFontFamilySource,
        [AllowNull()][string]$ExpectedEastAsiaFontFamilyValue,
        [AllowNull()][string]$ExpectedEastAsiaFontFamilySource,
        [AllowNull()][string]$ExpectedLanguageValue,
        [AllowNull()][string]$ExpectedLanguageSource,
        [AllowNull()][string]$ExpectedEastAsiaLanguageValue,
        [AllowNull()][string]$ExpectedEastAsiaLanguageSource,
        [AllowNull()][string]$ExpectedBidiLanguageValue,
        [AllowNull()][string]$ExpectedBidiLanguageSource,
        [AllowNull()][object]$ExpectedRtlValue,
        [AllowNull()][string]$ExpectedRtlSource,
        [AllowNull()][object]$ExpectedParagraphBidiValue,
        [AllowNull()][string]$ExpectedParagraphBidiSource,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($payload.style_id)'."
    }
    if ([string]$payload.type -ne $ExpectedType) {
        throw "$Label expected type '$ExpectedType', got '$($payload.type)'."
    }
    if ([string]$payload.kind -ne $ExpectedKind) {
        throw "$Label expected kind '$ExpectedKind', got '$($payload.kind)'."
    }

    if ($null -eq $ExpectedBasedOn) {
        if ($null -ne $payload.based_on) {
            throw "$Label expected based_on=null, got '$($payload.based_on)'."
        }
    } elseif ([string]$payload.based_on -ne $ExpectedBasedOn) {
        throw "$Label expected based_on '$ExpectedBasedOn', got '$($payload.based_on)'."
    }

    $actualChain = @($payload.inheritance_chain)
    if ($actualChain.Count -ne $ExpectedInheritanceChain.Count) {
        throw "$Label expected inheritance_chain count $($ExpectedInheritanceChain.Count), got $($actualChain.Count)."
    }
    for ($index = 0; $index -lt $ExpectedInheritanceChain.Count; $index++) {
        if ([string]$actualChain[$index] -ne [string]$ExpectedInheritanceChain[$index]) {
            throw "$Label expected inheritance_chain[$index] '$($ExpectedInheritanceChain[$index])', got '$($actualChain[$index])'."
        }
    }

    $resolved = $payload.resolved_properties
    if ($null -eq $resolved) {
        throw "$Label expected resolved_properties payload."
    }

    $assertStringProperty = {
        param(
            [object]$Property,
            [AllowNull()][string]$ExpectedValue,
            [AllowNull()][string]$ExpectedSource,
            [string]$PropertyLabel
        )

        if ($null -eq $ExpectedValue) {
            if ($null -ne $Property.value) {
                throw "$Label expected $PropertyLabel.value=null, got '$($Property.value)'."
            }
        } elseif ([string]$Property.value -ne $ExpectedValue) {
            throw "$Label expected $PropertyLabel.value '$ExpectedValue', got '$($Property.value)'."
        }

        if ($null -eq $ExpectedSource) {
            if ($null -ne $Property.source_style_id) {
                throw "$Label expected $PropertyLabel.source_style_id=null, got '$($Property.source_style_id)'."
            }
        } elseif ([string]$Property.source_style_id -ne $ExpectedSource) {
            throw "$Label expected $PropertyLabel.source_style_id '$ExpectedSource', got '$($Property.source_style_id)'."
        }
    }

    $assertBoolProperty = {
        param(
            [object]$Property,
            [AllowNull()][object]$ExpectedValue,
            [AllowNull()][string]$ExpectedSource,
            [string]$PropertyLabel
        )

        if ($null -eq $ExpectedValue) {
            if ($null -ne $Property.value) {
                throw "$Label expected $PropertyLabel.value=null, got '$($Property.value)'."
            }
        } elseif ([bool]$Property.value -ne $ExpectedValue) {
            throw "$Label expected $PropertyLabel.value=$ExpectedValue, got $($Property.value)."
        }

        if ($null -eq $ExpectedSource) {
            if ($null -ne $Property.source_style_id) {
                throw "$Label expected $PropertyLabel.source_style_id=null, got '$($Property.source_style_id)'."
            }
        } elseif ([string]$Property.source_style_id -ne $ExpectedSource) {
            throw "$Label expected $PropertyLabel.source_style_id '$ExpectedSource', got '$($Property.source_style_id)'."
        }
    }

    & $assertStringProperty $resolved.font_family $ExpectedFontFamilyValue $ExpectedFontFamilySource "font_family"
    & $assertStringProperty $resolved.east_asia_font_family $ExpectedEastAsiaFontFamilyValue $ExpectedEastAsiaFontFamilySource "east_asia_font_family"
    & $assertStringProperty $resolved.language $ExpectedLanguageValue $ExpectedLanguageSource "language"
    & $assertStringProperty $resolved.east_asia_language $ExpectedEastAsiaLanguageValue $ExpectedEastAsiaLanguageSource "east_asia_language"
    & $assertStringProperty $resolved.bidi_language $ExpectedBidiLanguageValue $ExpectedBidiLanguageSource "bidi_language"
    & $assertBoolProperty $resolved.rtl $ExpectedRtlValue $ExpectedRtlSource "rtl"
    & $assertBoolProperty $resolved.paragraph_bidi $ExpectedParagraphBidiValue $ExpectedParagraphBidiSource "paragraph_bidi"
}

function Assert-EnsureParagraphStyleResult {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedName,
        [string]$ExpectedBasedOn,
        [bool]$ExpectedUnhideWhenUsed,
        [bool]$ExpectedQuickFormat,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "ensure-paragraph-style") {
        throw "$Label expected command ensure-paragraph-style, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }

    $style = $payload.style
    if ($null -eq $style) {
        throw "$Label expected nested style payload."
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
    if ([bool]$style.is_unhide_when_used -ne $ExpectedUnhideWhenUsed) {
        throw "$Label expected is_unhide_when_used=$ExpectedUnhideWhenUsed, got $($style.is_unhide_when_used)."
    }
    if ([bool]$style.is_quick_format -ne $ExpectedQuickFormat) {
        throw "$Label expected is_quick_format=$ExpectedQuickFormat, got $($style.is_quick_format)."
    }
}

function Assert-EnsureCharacterStyleResult {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedName,
        [string]$ExpectedBasedOn,
        [bool]$ExpectedQuickFormat,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "ensure-character-style") {
        throw "$Label expected command ensure-character-style, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }

    $style = $payload.style
    if ($null -eq $style) {
        throw "$Label expected nested style payload."
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
    if ([bool]$style.is_quick_format -ne $ExpectedQuickFormat) {
        throw "$Label expected is_quick_format=$ExpectedQuickFormat, got $($style.is_quick_format)."
    }
}

function Assert-EnsureTableStyleResult {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedName,
        [string]$ExpectedBasedOn,
        [bool]$ExpectedUnhideWhenUsed,
        [bool]$ExpectedQuickFormat,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "ensure-table-style") {
        throw "$Label expected command ensure-table-style, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }

    $style = $payload.style
    if ($null -eq $style) {
        throw "$Label expected nested style payload."
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
    if ([bool]$style.is_unhide_when_used -ne $ExpectedUnhideWhenUsed) {
        throw "$Label expected is_unhide_when_used=$ExpectedUnhideWhenUsed, got $($style.is_unhide_when_used)."
    }
    if ([bool]$style.is_quick_format -ne $ExpectedQuickFormat) {
        throw "$Label expected is_quick_format=$ExpectedQuickFormat, got $($style.is_quick_format)."
    }
}

function Assert-TableState {
    param(
        [string]$JsonPath,
        [int]$ExpectedIndex,
        [string]$ExpectedStyleId,
        [int]$ExpectedRowCount,
        [int]$ExpectedColumnCount,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $table = $payload.table
    if ($null -eq $table) {
        throw "$Label expected table payload."
    }
    if ([int]$table.index -ne $ExpectedIndex) {
        throw "$Label expected table index $ExpectedIndex, got $($table.index)."
    }
    if ([string]$table.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($table.style_id)'."
    }
    if ([int]$table.row_count -ne $ExpectedRowCount) {
        throw "$Label expected row_count $ExpectedRowCount, got $($table.row_count)."
    }
    if ([int]$table.column_count -ne $ExpectedColumnCount) {
        throw "$Label expected column_count $ExpectedColumnCount, got $($table.column_count)."
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

    $nameNode = $styleNode.SelectSingleNode("w:name", $namespaceManager)
    $basedOnNode = $styleNode.SelectSingleNode("w:basedOn", $namespaceManager)
    $nextNode = $styleNode.SelectSingleNode("w:next", $namespaceManager)
    $qFormatNode = $styleNode.SelectSingleNode("w:qFormat", $namespaceManager)
    $semiHiddenNode = $styleNode.SelectSingleNode("w:semiHidden", $namespaceManager)
    $unhideWhenUsedNode = $styleNode.SelectSingleNode("w:unhideWhenUsed", $namespaceManager)
    $paragraphBidiNode = $styleNode.SelectSingleNode("w:pPr/w:bidi", $namespaceManager)
    $outlineLevelNode = $styleNode.SelectSingleNode("w:pPr/w:outlineLvl", $namespaceManager)
    $fontAsciiNode = $styleNode.SelectSingleNode("w:rPr/w:rFonts", $namespaceManager)
    $langNode = $styleNode.SelectSingleNode("w:rPr/w:lang", $namespaceManager)
    $rtlNode = $styleNode.SelectSingleNode("w:rPr/w:rtl", $namespaceManager)

    $outlineLevel = $null
    if ($null -ne $outlineLevelNode) {
        $outlineLevel = [int]$outlineLevelNode.GetAttribute("val", $script:WordMlNamespace)
    }

    $fontAscii = $null
    if ($null -ne $fontAsciiNode) {
        $value = $fontAsciiNode.GetAttribute("ascii", $script:WordMlNamespace)
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $fontAscii = $value
        }
    }

    $langValue = $null
    $langBidi = $null
    if ($null -ne $langNode) {
        $value = $langNode.GetAttribute("val", $script:WordMlNamespace)
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $langValue = $value
        }
        $bidiValue = $langNode.GetAttribute("bidi", $script:WordMlNamespace)
        if (-not [string]::IsNullOrWhiteSpace($bidiValue)) {
            $langBidi = $bidiValue
        }
    }

    $rtlValue = $null
    if ($null -ne $rtlNode) {
        $value = $rtlNode.GetAttribute("val", $script:WordMlNamespace)
        if ([string]::IsNullOrWhiteSpace($value)) {
            $rtlValue = "1"
        } else {
            $rtlValue = $value
        }
    }

    return [pscustomobject][ordered]@{
        style_id = $StyleId
        type = $styleNode.GetAttribute("type", $script:WordMlNamespace)
        name = if ($null -ne $nameNode) { $nameNode.GetAttribute("val", $script:WordMlNamespace) } else { $null }
        based_on = if ($null -ne $basedOnNode) { $basedOnNode.GetAttribute("val", $script:WordMlNamespace) } else { $null }
        next_style = if ($null -ne $nextNode) { $nextNode.GetAttribute("val", $script:WordMlNamespace) } else { $null }
        has_qformat = ($null -ne $qFormatNode)
        has_semi_hidden = ($null -ne $semiHiddenNode)
        has_unhide_when_used = ($null -ne $unhideWhenUsedNode)
        has_paragraph_bidi = ($null -ne $paragraphBidiNode)
        outline_level = $outlineLevel
        font_ascii = $fontAscii
        lang_val = $langValue
        lang_bidi = $langBidi
        rtl_val = $rtlValue
    }
}

function Assert-StyleXmlState {
    param(
        [object]$State,
        [string]$ExpectedType,
        [string]$ExpectedName,
        [AllowNull()][string]$ExpectedBasedOn,
        [AllowNull()][string]$ExpectedNextStyle,
        [bool]$ExpectedQFormat,
        [bool]$ExpectedSemiHidden,
        [bool]$ExpectedUnhideWhenUsed,
        [bool]$ExpectedParagraphBidi,
        [AllowNull()][int]$ExpectedOutlineLevel,
        [AllowNull()][string]$ExpectedFontAscii,
        [AllowNull()][string]$ExpectedLangValue,
        [AllowNull()][string]$ExpectedLangBidi,
        [AllowNull()][string]$ExpectedRtlValue,
        [string]$Label
    )

    if ([string]$State.type -ne $ExpectedType) {
        throw "$Label expected XML type '$ExpectedType', got '$($State.type)'."
    }
    if ([string]$State.name -ne $ExpectedName) {
        throw "$Label expected XML name '$ExpectedName', got '$($State.name)'."
    }

    if ($null -eq $ExpectedBasedOn) {
        if ($null -ne $State.based_on) {
            throw "$Label expected XML based_on=null, got '$($State.based_on)'."
        }
    } elseif ([string]$State.based_on -ne $ExpectedBasedOn) {
        throw "$Label expected XML based_on '$ExpectedBasedOn', got '$($State.based_on)'."
    }

    if ($null -eq $ExpectedNextStyle) {
        if ($null -ne $State.next_style) {
            throw "$Label expected XML next_style=null, got '$($State.next_style)'."
        }
    } elseif ([string]$State.next_style -ne $ExpectedNextStyle) {
        throw "$Label expected XML next_style '$ExpectedNextStyle', got '$($State.next_style)'."
    }

    if ([bool]$State.has_qformat -ne $ExpectedQFormat) {
        throw "$Label expected XML has_qformat=$ExpectedQFormat, got $($State.has_qformat)."
    }
    if ([bool]$State.has_semi_hidden -ne $ExpectedSemiHidden) {
        throw "$Label expected XML has_semi_hidden=$ExpectedSemiHidden, got $($State.has_semi_hidden)."
    }
    if ([bool]$State.has_unhide_when_used -ne $ExpectedUnhideWhenUsed) {
        throw "$Label expected XML has_unhide_when_used=$ExpectedUnhideWhenUsed, got $($State.has_unhide_when_used)."
    }
    if ([bool]$State.has_paragraph_bidi -ne $ExpectedParagraphBidi) {
        throw "$Label expected XML has_paragraph_bidi=$ExpectedParagraphBidi, got $($State.has_paragraph_bidi)."
    }

    if ($null -eq $ExpectedOutlineLevel) {
        if ($null -ne $State.outline_level) {
            throw "$Label expected XML outline_level=null, got '$($State.outline_level)'."
        }
    } elseif ([int]$State.outline_level -ne $ExpectedOutlineLevel) {
        throw "$Label expected XML outline_level=$ExpectedOutlineLevel, got '$($State.outline_level)'."
    }

    if ($null -eq $ExpectedFontAscii) {
        if ($null -ne $State.font_ascii) {
            throw "$Label expected XML font_ascii=null, got '$($State.font_ascii)'."
        }
    } elseif ([string]$State.font_ascii -ne $ExpectedFontAscii) {
        throw "$Label expected XML font_ascii '$ExpectedFontAscii', got '$($State.font_ascii)'."
    }

    if ($null -eq $ExpectedLangValue) {
        if ($null -ne $State.lang_val) {
            throw "$Label expected XML lang_val=null, got '$($State.lang_val)'."
        }
    } elseif ([string]$State.lang_val -ne $ExpectedLangValue) {
        throw "$Label expected XML lang_val '$ExpectedLangValue', got '$($State.lang_val)'."
    }

    if ($null -eq $ExpectedLangBidi) {
        if ($null -ne $State.lang_bidi) {
            throw "$Label expected XML lang_bidi=null, got '$($State.lang_bidi)'."
        }
    } elseif ([string]$State.lang_bidi -ne $ExpectedLangBidi) {
        throw "$Label expected XML lang_bidi '$ExpectedLangBidi', got '$($State.lang_bidi)'."
    }

    if ($null -eq $ExpectedRtlValue) {
        if ($null -ne $State.rtl_val) {
            throw "$Label expected XML rtl_val=null, got '$($State.rtl_val)'."
        }
    } elseif ([string]$State.rtl_val -ne $ExpectedRtlValue) {
        throw "$Label expected XML rtl_val '$ExpectedRtlValue', got '$($State.rtl_val)'."
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

    Write-Step "Building featherdoc_cli and ensure-style visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_ensure_style_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_ensure_style_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$paragraphTargetText = "Paragraph style target: this paragraph already uses ReviewPara, so ensure-paragraph-style should rewrite the style definition and restyle this whole line without rebinding the paragraph."
$inheritedParagraphTargetText = "Inherited style target: this paragraph uses ReviewParaChild, so rewriting ReviewPara should flow through the basedOn chain and restyle this line without rebinding the child paragraph."
$runTargetText = "ALPHA 123 hello world"
$reviewTableStyleId = "ReviewTable"
$reviewTableStyleName = "Review Table"

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared ensure-style baseline sample."

$baselineParagraphJson = Join-Path $resolvedOutputDir "baseline-review-para-paragraph.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-paragraphs", $baselineDocxPath, "--paragraph", "1", "--json") `
    -OutputPath $baselineParagraphJson `
    -FailureMessage "Failed to inspect baseline ReviewPara paragraph."
Assert-ParagraphState `
    -JsonPath $baselineParagraphJson `
    -ExpectedIndex 1 `
    -ExpectedText $paragraphTargetText `
    -ExpectedStyleId "ReviewPara" `
    -Label "baseline-review-para"

$baselineInheritedParagraphJson = Join-Path $resolvedOutputDir "baseline-review-para-child-paragraph.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-paragraphs", $baselineDocxPath, "--paragraph", "2", "--json") `
    -OutputPath $baselineInheritedParagraphJson `
    -FailureMessage "Failed to inspect baseline ReviewParaChild paragraph."
Assert-ParagraphState `
    -JsonPath $baselineInheritedParagraphJson `
    -ExpectedIndex 2 `
    -ExpectedText $inheritedParagraphTargetText `
    -ExpectedStyleId "ReviewParaChild" `
    -Label "baseline-review-para-child"

$baselineRunJson = Join-Path $resolvedOutputDir "baseline-accent-marker-run.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-runs", $baselineDocxPath, "3", "--run", "1", "--json") `
    -OutputPath $baselineRunJson `
    -FailureMessage "Failed to inspect baseline AccentMarker run."
Assert-RunState `
    -JsonPath $baselineRunJson `
    -ExpectedParagraphIndex 3 `
    -ExpectedRunIndex 1 `
    -ExpectedText $runTargetText `
    -ExpectedStyleId "AccentMarker" `
    -Label "baseline-accent-marker-run"

$baselineReviewTableJson = Join-Path $resolvedOutputDir "baseline-review-table.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-tables", $baselineDocxPath, "--table", "0", "--json") `
    -OutputPath $baselineReviewTableJson `
    -FailureMessage "Failed to inspect baseline ReviewTable target."
Assert-TableState `
    -JsonPath $baselineReviewTableJson `
    -ExpectedIndex 0 `
    -ExpectedStyleId $reviewTableStyleId `
    -ExpectedRowCount 3 `
    -ExpectedColumnCount 2 `
    -Label "baseline-review-table"

$baselineReviewParaStyleJson = Join-Path $resolvedOutputDir "baseline-review-para-style.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-styles", $baselineDocxPath, "--style", "ReviewPara", "--json") `
    -OutputPath $baselineReviewParaStyleJson `
    -FailureMessage "Failed to inspect baseline ReviewPara style."
Assert-StyleCatalogState `
    -JsonPath $baselineReviewParaStyleJson `
    -ExpectedStyleId "ReviewPara" `
    -ExpectedName "Review Paragraph" `
    -ExpectedBasedOn "Normal" `
    -ExpectedKind "paragraph" `
    -ExpectedType "paragraph" `
    -ExpectedCustom $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedQuickFormat $true `
    -Label "baseline-review-para-style"

$baselineReviewParaChildStyleJson = Join-Path $resolvedOutputDir "baseline-review-para-child-style.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-styles", $baselineDocxPath, "--style", "ReviewParaChild", "--json") `
    -OutputPath $baselineReviewParaChildStyleJson `
    -FailureMessage "Failed to inspect baseline ReviewParaChild style."
Assert-StyleCatalogState `
    -JsonPath $baselineReviewParaChildStyleJson `
    -ExpectedStyleId "ReviewParaChild" `
    -ExpectedName "Review Paragraph Child" `
    -ExpectedBasedOn "ReviewPara" `
    -ExpectedKind "paragraph" `
    -ExpectedType "paragraph" `
    -ExpectedCustom $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedQuickFormat $true `
    -Label "baseline-review-para-child-style"

$baselineReviewParaChildInheritanceJson = Join-Path $resolvedOutputDir "baseline-review-para-child-inheritance.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-style-inheritance", $baselineDocxPath, "ReviewParaChild", "--json") `
    -OutputPath $baselineReviewParaChildInheritanceJson `
    -FailureMessage "Failed to inspect baseline ReviewParaChild inheritance."
Assert-StyleInheritanceState `
    -JsonPath $baselineReviewParaChildInheritanceJson `
    -ExpectedStyleId "ReviewParaChild" `
    -ExpectedType "paragraph" `
    -ExpectedKind "paragraph" `
    -ExpectedBasedOn "ReviewPara" `
    -ExpectedInheritanceChain @("ReviewParaChild", "ReviewPara", "Normal") `
    -ExpectedFontFamilyValue "Courier New" `
    -ExpectedFontFamilySource "ReviewPara" `
    -ExpectedEastAsiaFontFamilyValue $null `
    -ExpectedEastAsiaFontFamilySource $null `
    -ExpectedLanguageValue $null `
    -ExpectedLanguageSource $null `
    -ExpectedEastAsiaLanguageValue $null `
    -ExpectedEastAsiaLanguageSource $null `
    -ExpectedBidiLanguageValue $null `
    -ExpectedBidiLanguageSource $null `
    -ExpectedRtlValue $null `
    -ExpectedRtlSource $null `
    -ExpectedParagraphBidiValue $null `
    -ExpectedParagraphBidiSource $null `
    -Label "baseline-review-para-child-inheritance"

$baselineAccentMarkerStyleJson = Join-Path $resolvedOutputDir "baseline-accent-marker-style.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-styles", $baselineDocxPath, "--style", "AccentMarker", "--json") `
    -OutputPath $baselineAccentMarkerStyleJson `
    -FailureMessage "Failed to inspect baseline AccentMarker style."
Assert-StyleCatalogState `
    -JsonPath $baselineAccentMarkerStyleJson `
    -ExpectedStyleId "AccentMarker" `
    -ExpectedName "Accent Marker" `
    -ExpectedBasedOn "DefaultParagraphFont" `
    -ExpectedKind "character" `
    -ExpectedType "character" `
    -ExpectedCustom $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedQuickFormat $true `
    -Label "baseline-accent-marker-style"

$baselineReviewTableStyleJson = Join-Path $resolvedOutputDir "baseline-review-table-style.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-styles", $baselineDocxPath, "--style", $reviewTableStyleId, "--json") `
    -OutputPath $baselineReviewTableStyleJson `
    -FailureMessage "Failed to inspect baseline ReviewTable style."
Assert-StyleCatalogState `
    -JsonPath $baselineReviewTableStyleJson `
    -ExpectedStyleId $reviewTableStyleId `
    -ExpectedName $reviewTableStyleName `
    -ExpectedBasedOn "TableGrid" `
    -ExpectedKind "table" `
    -ExpectedType "table" `
    -ExpectedCustom $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedQuickFormat $true `
    -Label "baseline-review-table-style"

$baselineReviewParaXmlState = Get-StyleXmlState -DocxPath $baselineDocxPath -StyleId "ReviewPara"
Assert-StyleXmlState `
    -State $baselineReviewParaXmlState `
    -ExpectedType "paragraph" `
    -ExpectedName "Review Paragraph" `
    -ExpectedBasedOn "Normal" `
    -ExpectedNextStyle "ReviewPara" `
    -ExpectedQFormat $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedParagraphBidi $false `
    -ExpectedOutlineLevel $null `
    -ExpectedFontAscii "Courier New" `
    -ExpectedLangValue $null `
    -ExpectedLangBidi $null `
    -ExpectedRtlValue $null `
    -Label "baseline-review-para-xml"

$baselineReviewParaChildXmlState = Get-StyleXmlState -DocxPath $baselineDocxPath -StyleId "ReviewParaChild"
Assert-StyleXmlState `
    -State $baselineReviewParaChildXmlState `
    -ExpectedType "paragraph" `
    -ExpectedName "Review Paragraph Child" `
    -ExpectedBasedOn "ReviewPara" `
    -ExpectedNextStyle "ReviewParaChild" `
    -ExpectedQFormat $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedParagraphBidi $false `
    -ExpectedOutlineLevel $null `
    -ExpectedFontAscii $null `
    -ExpectedLangValue $null `
    -ExpectedLangBidi $null `
    -ExpectedRtlValue $null `
    -Label "baseline-review-para-child-xml"

$baselineAccentMarkerXmlState = Get-StyleXmlState -DocxPath $baselineDocxPath -StyleId "AccentMarker"
Assert-StyleXmlState `
    -State $baselineAccentMarkerXmlState `
    -ExpectedType "character" `
    -ExpectedName "Accent Marker" `
    -ExpectedBasedOn "DefaultParagraphFont" `
    -ExpectedNextStyle $null `
    -ExpectedQFormat $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedParagraphBidi $false `
    -ExpectedOutlineLevel $null `
    -ExpectedFontAscii "Courier New" `
    -ExpectedLangValue $null `
    -ExpectedLangBidi $null `
    -ExpectedRtlValue "0" `
    -Label "baseline-accent-marker-xml"

$baselineReviewTableXmlState = Get-StyleXmlState -DocxPath $baselineDocxPath -StyleId $reviewTableStyleId
Assert-StyleXmlState `
    -State $baselineReviewTableXmlState `
    -ExpectedType "table" `
    -ExpectedName $reviewTableStyleName `
    -ExpectedBasedOn "TableGrid" `
    -ExpectedNextStyle $null `
    -ExpectedQFormat $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedParagraphBidi $false `
    -ExpectedOutlineLevel $null `
    -ExpectedFontAscii $null `
    -ExpectedLangValue $null `
    -ExpectedLangBidi $null `
    -ExpectedRtlValue $null `
    -Label "baseline-review-table-xml"

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
        id = "ensure-paragraph-style-heading-visual"
        command = "ensure-paragraph-style"
        arguments = @(
            "ReviewPara",
            "--name", "Review Paragraph",
            "--based-on", "Heading2",
            "--next-style", "ReviewPara",
            "--custom", "true",
            "--semi-hidden", "false",
            "--unhide-when-used", "true",
            "--quick-format", "true",
            "--paragraph-bidi", "true",
            "--outline-level", "2"
        )
        expected_visual_cues = @(
            "The ReviewPara paragraph grows into a Heading 2 sized callout instead of staying monospaced body text.",
            "The paragraph keeps the same ReviewPara style binding; only the style definition changes.",
            "The ReviewParaChild paragraph also inherits the Heading 2 restyle through the basedOn chain instead of keeping the baseline Courier New body look.",
            "The AccentMarker run remains visually and structurally unchanged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "ensure-character-style-serif-visual"
        command = "ensure-character-style"
        arguments = @(
            "AccentMarker",
            "--name", "Accent Marker",
            "--based-on", "DefaultParagraphFont",
            "--custom", "true",
            "--semi-hidden", "false",
            "--unhide-when-used", "false",
            "--quick-format", "true",
            "--run-font-family", "Times New Roman",
            "--run-language", "fr-FR",
            "--run-rtl", "false"
        )
        expected_visual_cues = @(
            "The AccentMarker run drops the baseline Courier New look and switches to a proportional serif face.",
            "The prefix and suffix around the target run stay in the document default formatting.",
            "The ReviewPara and ReviewParaChild paragraphs remain visually and structurally unchanged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "materialize-style-run-properties-child-freeze-visual"
        command = "ensure-paragraph-style"
        steps = @(
            [pscustomobject][ordered]@{
                command = "materialize-style-run-properties"
                arguments = @("ReviewParaChild")
            },
            [pscustomobject][ordered]@{
                command = "ensure-paragraph-style"
                arguments = @(
                    "ReviewPara",
                    "--name", "Review Paragraph",
                    "--based-on", "Normal",
                    "--next-style", "ReviewPara",
                    "--custom", "true",
                    "--semi-hidden", "false",
                    "--unhide-when-used", "false",
                    "--quick-format", "true",
                    "--run-font-family", "Times New Roman"
                )
            }
        )
        expected_visual_cues = @(
            "The ReviewPara paragraph switches from the baseline Courier New face to Times New Roman.",
            "The ReviewParaChild paragraph keeps the materialized Courier New face even after its parent style changes.",
            "The AccentMarker run and ReviewTable target remain visually and structurally unchanged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "ensure-table-style-borderless-visual"
        command = "ensure-table-style"
        arguments = @(
            $reviewTableStyleId,
            "--name", $reviewTableStyleName,
            "--based-on", "TableNormal",
            "--custom", "true",
            "--semi-hidden", "false",
            "--unhide-when-used", "true",
            "--quick-format", "true"
        )
        expected_visual_cues = @(
            "The ReviewTable target table drops the baseline TableGrid borders and renders as a borderless TableNormal-derived layout.",
            "The table keeps the same ReviewTable style binding; only the style definition changes.",
            "The ReviewPara and ReviewParaChild paragraphs plus the AccentMarker run remain visually and structurally unchanged in this case."
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

    Write-Step "Running case '$($case.id)'"
    $mutationSteps = @()
    if ($null -ne $case.steps) {
        $currentInputDocx = $baselineDocxPath
        for ($stepIndex = 0; $stepIndex -lt $case.steps.Count; ++$stepIndex) {
            $step = $case.steps[$stepIndex]
            $stepJsonPath = Join-Path $caseDir ("mutation-step-{0:D2}.json" -f ($stepIndex + 1))
            $stepOutputDocxPath =
                if ($stepIndex -eq ($case.steps.Count - 1)) {
                    $mutatedDocxPath
                } else {
                    Join-Path $caseDir ("mutation-step-{0:D2}.docx" -f ($stepIndex + 1))
                }

            $stepArguments = @($step.command, $currentInputDocx)
            $stepArguments += $step.arguments
            $stepArguments += @("--output", $stepOutputDocxPath, "--json")

            Invoke-Capture `
                -Executable $cliExecutable `
                -Arguments $stepArguments `
                -OutputPath $stepJsonPath `
                -FailureMessage "Failed to run step $($stepIndex + 1) for case '$($case.id)'."

            $mutationSteps += [ordered]@{
                command = $step.command
                mutation_json = $stepJsonPath
                output_docx = $stepOutputDocxPath
            }
            $currentInputDocx = $stepOutputDocxPath
        }
        $mutationJsonPath = $mutationSteps[$mutationSteps.Count - 1].mutation_json
    } else {
        $arguments = @($case.command, $baselineDocxPath)
        $arguments += $case.arguments
        $arguments += @("--output", $mutatedDocxPath, "--json")

        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments $arguments `
            -OutputPath $mutationJsonPath `
            -FailureMessage "Failed to run case '$($case.id)'."

        $mutationSteps += [ordered]@{
            command = $case.command
            mutation_json = $mutationJsonPath
            output_docx = $mutatedDocxPath
        }
    }

    if ($case.id -eq "ensure-paragraph-style-heading-visual") {
        Assert-EnsureParagraphStyleResult `
            -JsonPath $mutationJsonPath `
            -ExpectedStyleId "ReviewPara" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Heading2" `
            -ExpectedUnhideWhenUsed $true `
            -ExpectedQuickFormat $true `
            -Label $case.id
    } elseif ($case.id -eq "materialize-style-run-properties-child-freeze-visual") {
        Assert-EnsureParagraphStyleResult `
            -JsonPath $mutationJsonPath `
            -ExpectedStyleId "ReviewPara" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Normal" `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label $case.id
    } elseif ($case.command -eq "ensure-character-style") {
        Assert-EnsureCharacterStyleResult `
            -JsonPath $mutationJsonPath `
            -ExpectedStyleId "AccentMarker" `
            -ExpectedName "Accent Marker" `
            -ExpectedBasedOn "DefaultParagraphFont" `
            -ExpectedQuickFormat $true `
            -Label $case.id
    } elseif ($case.command -eq "ensure-table-style") {
        Assert-EnsureTableStyleResult `
            -JsonPath $mutationJsonPath `
            -ExpectedStyleId $reviewTableStyleId `
            -ExpectedName $reviewTableStyleName `
            -ExpectedBasedOn "TableNormal" `
            -ExpectedUnhideWhenUsed $true `
            -ExpectedQuickFormat $true `
            -Label $case.id
    } else {
        throw "Unsupported command '$($case.command)' in case '$($case.id)'."
    }

    $paragraphJsonPath = Join-Path $caseDir "inspect-review-para-paragraph.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-paragraphs", $mutatedDocxPath, "--paragraph", "1", "--json") `
        -OutputPath $paragraphJsonPath `
        -FailureMessage "Failed to inspect ReviewPara paragraph for case '$($case.id)'."
    Assert-ParagraphState `
        -JsonPath $paragraphJsonPath `
        -ExpectedIndex 1 `
        -ExpectedText $paragraphTargetText `
        -ExpectedStyleId "ReviewPara" `
        -Label "$($case.id)-review-para"

    $inheritedParagraphJsonPath = Join-Path $caseDir "inspect-review-para-child-paragraph.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-paragraphs", $mutatedDocxPath, "--paragraph", "2", "--json") `
        -OutputPath $inheritedParagraphJsonPath `
        -FailureMessage "Failed to inspect ReviewParaChild paragraph for case '$($case.id)'."
    Assert-ParagraphState `
        -JsonPath $inheritedParagraphJsonPath `
        -ExpectedIndex 2 `
        -ExpectedText $inheritedParagraphTargetText `
        -ExpectedStyleId "ReviewParaChild" `
        -Label "$($case.id)-review-para-child"

    $runJsonPath = Join-Path $caseDir "inspect-accent-marker-run.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-runs", $mutatedDocxPath, "3", "--run", "1", "--json") `
        -OutputPath $runJsonPath `
        -FailureMessage "Failed to inspect AccentMarker run for case '$($case.id)'."
    Assert-RunState `
        -JsonPath $runJsonPath `
        -ExpectedParagraphIndex 3 `
        -ExpectedRunIndex 1 `
        -ExpectedText $runTargetText `
        -ExpectedStyleId "AccentMarker" `
        -Label "$($case.id)-accent-marker-run"

    $reviewTableJsonPath = Join-Path $caseDir "inspect-review-table.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-tables", $mutatedDocxPath, "--table", "0", "--json") `
        -OutputPath $reviewTableJsonPath `
        -FailureMessage "Failed to inspect ReviewTable target for case '$($case.id)'."
    Assert-TableState `
        -JsonPath $reviewTableJsonPath `
        -ExpectedIndex 0 `
        -ExpectedStyleId $reviewTableStyleId `
        -ExpectedRowCount 3 `
        -ExpectedColumnCount 2 `
        -Label "$($case.id)-review-table"

    $reviewParaStyleJsonPath = Join-Path $caseDir "inspect-review-para-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-styles", $mutatedDocxPath, "--style", "ReviewPara", "--json") `
        -OutputPath $reviewParaStyleJsonPath `
        -FailureMessage "Failed to inspect ReviewPara style for case '$($case.id)'."

    $reviewParaChildStyleJsonPath = Join-Path $caseDir "inspect-review-para-child-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-styles", $mutatedDocxPath, "--style", "ReviewParaChild", "--json") `
        -OutputPath $reviewParaChildStyleJsonPath `
        -FailureMessage "Failed to inspect ReviewParaChild style for case '$($case.id)'."

    $accentMarkerStyleJsonPath = Join-Path $caseDir "inspect-accent-marker-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-styles", $mutatedDocxPath, "--style", "AccentMarker", "--json") `
        -OutputPath $accentMarkerStyleJsonPath `
        -FailureMessage "Failed to inspect AccentMarker style for case '$($case.id)'."

    $reviewTableStyleJsonPath = Join-Path $caseDir "inspect-review-table-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-styles", $mutatedDocxPath, "--style", $reviewTableStyleId, "--json") `
        -OutputPath $reviewTableStyleJsonPath `
        -FailureMessage "Failed to inspect ReviewTable style for case '$($case.id)'."

    $reviewParaChildInheritanceJsonPath = Join-Path $caseDir "inspect-review-para-child-inheritance.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-style-inheritance", $mutatedDocxPath, "ReviewParaChild", "--json") `
        -OutputPath $reviewParaChildInheritanceJsonPath `
        -FailureMessage "Failed to inspect ReviewParaChild inheritance for case '$($case.id)'."

    $reviewParaXmlState = Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId "ReviewPara"
    $reviewParaChildXmlState = Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId "ReviewParaChild"
    $accentMarkerXmlState = Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId "AccentMarker"
    $reviewTableXmlState = Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId $reviewTableStyleId

    if ($case.id -eq "ensure-paragraph-style-heading-visual") {
        Assert-StyleCatalogState `
            -JsonPath $reviewParaStyleJsonPath `
            -ExpectedStyleId "ReviewPara" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Heading2" `
            -ExpectedKind "paragraph" `
            -ExpectedType "paragraph" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $true `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-para-style"
        Assert-StyleCatalogState `
            -JsonPath $reviewParaChildStyleJsonPath `
            -ExpectedStyleId "ReviewParaChild" `
            -ExpectedName "Review Paragraph Child" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedKind "paragraph" `
            -ExpectedType "paragraph" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-para-child-style"
        Assert-StyleCatalogState `
            -JsonPath $accentMarkerStyleJsonPath `
            -ExpectedStyleId "AccentMarker" `
            -ExpectedName "Accent Marker" `
            -ExpectedBasedOn "DefaultParagraphFont" `
            -ExpectedKind "character" `
            -ExpectedType "character" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-accent-marker-style"
        Assert-StyleCatalogState `
            -JsonPath $reviewTableStyleJsonPath `
            -ExpectedStyleId $reviewTableStyleId `
            -ExpectedName $reviewTableStyleName `
            -ExpectedBasedOn "TableGrid" `
            -ExpectedKind "table" `
            -ExpectedType "table" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-table-style"

        Assert-StyleXmlState `
            -State $reviewParaXmlState `
            -ExpectedType "paragraph" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Heading2" `
            -ExpectedNextStyle "ReviewPara" `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $true `
            -ExpectedParagraphBidi $true `
            -ExpectedOutlineLevel 2 `
            -ExpectedFontAscii $null `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-para-xml"
        Assert-StyleXmlState `
            -State $reviewParaChildXmlState `
            -ExpectedType "paragraph" `
            -ExpectedName "Review Paragraph Child" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedNextStyle "ReviewParaChild" `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii $null `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-para-child-xml"
        Assert-StyleInheritanceState `
            -JsonPath $reviewParaChildInheritanceJsonPath `
            -ExpectedStyleId "ReviewParaChild" `
            -ExpectedType "paragraph" `
            -ExpectedKind "paragraph" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedInheritanceChain @("ReviewParaChild", "ReviewPara", "Heading2", "Normal") `
            -ExpectedFontFamilyValue $null `
            -ExpectedFontFamilySource $null `
            -ExpectedEastAsiaFontFamilyValue $null `
            -ExpectedEastAsiaFontFamilySource $null `
            -ExpectedLanguageValue $null `
            -ExpectedLanguageSource $null `
            -ExpectedEastAsiaLanguageValue $null `
            -ExpectedEastAsiaLanguageSource $null `
            -ExpectedBidiLanguageValue $null `
            -ExpectedBidiLanguageSource $null `
            -ExpectedRtlValue $null `
            -ExpectedRtlSource $null `
            -ExpectedParagraphBidiValue $true `
            -ExpectedParagraphBidiSource "ReviewPara" `
            -Label "$($case.id)-review-para-child-inheritance"
        Assert-StyleXmlState `
            -State $accentMarkerXmlState `
            -ExpectedType "character" `
            -ExpectedName "Accent Marker" `
            -ExpectedBasedOn "DefaultParagraphFont" `
            -ExpectedNextStyle $null `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii "Courier New" `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue "0" `
            -Label "$($case.id)-accent-marker-xml"
        Assert-StyleXmlState `
            -State $reviewTableXmlState `
            -ExpectedType "table" `
            -ExpectedName $reviewTableStyleName `
            -ExpectedBasedOn "TableGrid" `
            -ExpectedNextStyle $null `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii $null `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-table-xml"
    } elseif ($case.id -eq "materialize-style-run-properties-child-freeze-visual") {
        Assert-StyleCatalogState `
            -JsonPath $reviewParaStyleJsonPath `
            -ExpectedStyleId "ReviewPara" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Normal" `
            -ExpectedKind "paragraph" `
            -ExpectedType "paragraph" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-para-style"
        Assert-StyleCatalogState `
            -JsonPath $reviewParaChildStyleJsonPath `
            -ExpectedStyleId "ReviewParaChild" `
            -ExpectedName "Review Paragraph Child" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedKind "paragraph" `
            -ExpectedType "paragraph" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-para-child-style"
        Assert-StyleCatalogState `
            -JsonPath $accentMarkerStyleJsonPath `
            -ExpectedStyleId "AccentMarker" `
            -ExpectedName "Accent Marker" `
            -ExpectedBasedOn "DefaultParagraphFont" `
            -ExpectedKind "character" `
            -ExpectedType "character" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-accent-marker-style"
        Assert-StyleCatalogState `
            -JsonPath $reviewTableStyleJsonPath `
            -ExpectedStyleId $reviewTableStyleId `
            -ExpectedName $reviewTableStyleName `
            -ExpectedBasedOn "TableGrid" `
            -ExpectedKind "table" `
            -ExpectedType "table" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-table-style"

        Assert-StyleXmlState `
            -State $reviewParaXmlState `
            -ExpectedType "paragraph" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Normal" `
            -ExpectedNextStyle "ReviewPara" `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii "Times New Roman" `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-para-xml"
        Assert-StyleXmlState `
            -State $reviewParaChildXmlState `
            -ExpectedType "paragraph" `
            -ExpectedName "Review Paragraph Child" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedNextStyle "ReviewParaChild" `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii "Courier New" `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-para-child-xml"
        Assert-StyleInheritanceState `
            -JsonPath $reviewParaChildInheritanceJsonPath `
            -ExpectedStyleId "ReviewParaChild" `
            -ExpectedType "paragraph" `
            -ExpectedKind "paragraph" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedInheritanceChain @("ReviewParaChild", "ReviewPara", "Normal") `
            -ExpectedFontFamilyValue "Courier New" `
            -ExpectedFontFamilySource "ReviewParaChild" `
            -ExpectedEastAsiaFontFamilyValue $null `
            -ExpectedEastAsiaFontFamilySource $null `
            -ExpectedLanguageValue $null `
            -ExpectedLanguageSource $null `
            -ExpectedEastAsiaLanguageValue $null `
            -ExpectedEastAsiaLanguageSource $null `
            -ExpectedBidiLanguageValue $null `
            -ExpectedBidiLanguageSource $null `
            -ExpectedRtlValue $null `
            -ExpectedRtlSource $null `
            -ExpectedParagraphBidiValue $null `
            -ExpectedParagraphBidiSource $null `
            -Label "$($case.id)-review-para-child-inheritance"
        Assert-StyleXmlState `
            -State $accentMarkerXmlState `
            -ExpectedType "character" `
            -ExpectedName "Accent Marker" `
            -ExpectedBasedOn "DefaultParagraphFont" `
            -ExpectedNextStyle $null `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii "Courier New" `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue "0" `
            -Label "$($case.id)-accent-marker-xml"
        Assert-StyleXmlState `
            -State $reviewTableXmlState `
            -ExpectedType "table" `
            -ExpectedName $reviewTableStyleName `
            -ExpectedBasedOn "TableGrid" `
            -ExpectedNextStyle $null `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii $null `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-table-xml"
    } elseif ($case.id -eq "ensure-character-style-serif-visual") {
        Assert-StyleCatalogState `
            -JsonPath $reviewParaStyleJsonPath `
            -ExpectedStyleId "ReviewPara" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Normal" `
            -ExpectedKind "paragraph" `
            -ExpectedType "paragraph" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-para-style"
        Assert-StyleCatalogState `
            -JsonPath $reviewParaChildStyleJsonPath `
            -ExpectedStyleId "ReviewParaChild" `
            -ExpectedName "Review Paragraph Child" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedKind "paragraph" `
            -ExpectedType "paragraph" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-para-child-style"
        Assert-StyleCatalogState `
            -JsonPath $accentMarkerStyleJsonPath `
            -ExpectedStyleId "AccentMarker" `
            -ExpectedName "Accent Marker" `
            -ExpectedBasedOn "DefaultParagraphFont" `
            -ExpectedKind "character" `
            -ExpectedType "character" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-accent-marker-style"
        Assert-StyleCatalogState `
            -JsonPath $reviewTableStyleJsonPath `
            -ExpectedStyleId $reviewTableStyleId `
            -ExpectedName $reviewTableStyleName `
            -ExpectedBasedOn "TableGrid" `
            -ExpectedKind "table" `
            -ExpectedType "table" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-table-style"

        Assert-StyleXmlState `
            -State $reviewParaXmlState `
            -ExpectedType "paragraph" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Normal" `
            -ExpectedNextStyle "ReviewPara" `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii "Courier New" `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-para-xml"
        Assert-StyleXmlState `
            -State $reviewParaChildXmlState `
            -ExpectedType "paragraph" `
            -ExpectedName "Review Paragraph Child" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedNextStyle "ReviewParaChild" `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii $null `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-para-child-xml"
        Assert-StyleInheritanceState `
            -JsonPath $reviewParaChildInheritanceJsonPath `
            -ExpectedStyleId "ReviewParaChild" `
            -ExpectedType "paragraph" `
            -ExpectedKind "paragraph" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedInheritanceChain @("ReviewParaChild", "ReviewPara", "Normal") `
            -ExpectedFontFamilyValue "Courier New" `
            -ExpectedFontFamilySource "ReviewPara" `
            -ExpectedEastAsiaFontFamilyValue $null `
            -ExpectedEastAsiaFontFamilySource $null `
            -ExpectedLanguageValue $null `
            -ExpectedLanguageSource $null `
            -ExpectedEastAsiaLanguageValue $null `
            -ExpectedEastAsiaLanguageSource $null `
            -ExpectedBidiLanguageValue $null `
            -ExpectedBidiLanguageSource $null `
            -ExpectedRtlValue $null `
            -ExpectedRtlSource $null `
            -ExpectedParagraphBidiValue $null `
            -ExpectedParagraphBidiSource $null `
            -Label "$($case.id)-review-para-child-inheritance"
        Assert-StyleXmlState `
            -State $accentMarkerXmlState `
            -ExpectedType "character" `
            -ExpectedName "Accent Marker" `
            -ExpectedBasedOn "DefaultParagraphFont" `
            -ExpectedNextStyle $null `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii "Times New Roman" `
            -ExpectedLangValue "fr-FR" `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue "0" `
            -Label "$($case.id)-accent-marker-xml"
        Assert-StyleXmlState `
            -State $reviewTableXmlState `
            -ExpectedType "table" `
            -ExpectedName $reviewTableStyleName `
            -ExpectedBasedOn "TableGrid" `
            -ExpectedNextStyle $null `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii $null `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-table-xml"
    } elseif ($case.id -eq "ensure-table-style-borderless-visual") {
        Assert-StyleCatalogState `
            -JsonPath $reviewParaStyleJsonPath `
            -ExpectedStyleId "ReviewPara" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Normal" `
            -ExpectedKind "paragraph" `
            -ExpectedType "paragraph" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-para-style"
        Assert-StyleCatalogState `
            -JsonPath $reviewParaChildStyleJsonPath `
            -ExpectedStyleId "ReviewParaChild" `
            -ExpectedName "Review Paragraph Child" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedKind "paragraph" `
            -ExpectedType "paragraph" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-para-child-style"
        Assert-StyleCatalogState `
            -JsonPath $accentMarkerStyleJsonPath `
            -ExpectedStyleId "AccentMarker" `
            -ExpectedName "Accent Marker" `
            -ExpectedBasedOn "DefaultParagraphFont" `
            -ExpectedKind "character" `
            -ExpectedType "character" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-accent-marker-style"
        Assert-StyleCatalogState `
            -JsonPath $reviewTableStyleJsonPath `
            -ExpectedStyleId $reviewTableStyleId `
            -ExpectedName $reviewTableStyleName `
            -ExpectedBasedOn "TableNormal" `
            -ExpectedKind "table" `
            -ExpectedType "table" `
            -ExpectedCustom $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $true `
            -ExpectedQuickFormat $true `
            -Label "$($case.id)-review-table-style"

        Assert-StyleXmlState `
            -State $reviewParaXmlState `
            -ExpectedType "paragraph" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Normal" `
            -ExpectedNextStyle "ReviewPara" `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii "Courier New" `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-para-xml"
        Assert-StyleXmlState `
            -State $reviewParaChildXmlState `
            -ExpectedType "paragraph" `
            -ExpectedName "Review Paragraph Child" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedNextStyle "ReviewParaChild" `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii $null `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-para-child-xml"
        Assert-StyleInheritanceState `
            -JsonPath $reviewParaChildInheritanceJsonPath `
            -ExpectedStyleId "ReviewParaChild" `
            -ExpectedType "paragraph" `
            -ExpectedKind "paragraph" `
            -ExpectedBasedOn "ReviewPara" `
            -ExpectedInheritanceChain @("ReviewParaChild", "ReviewPara", "Normal") `
            -ExpectedFontFamilyValue "Courier New" `
            -ExpectedFontFamilySource "ReviewPara" `
            -ExpectedEastAsiaFontFamilyValue $null `
            -ExpectedEastAsiaFontFamilySource $null `
            -ExpectedLanguageValue $null `
            -ExpectedLanguageSource $null `
            -ExpectedEastAsiaLanguageValue $null `
            -ExpectedEastAsiaLanguageSource $null `
            -ExpectedBidiLanguageValue $null `
            -ExpectedBidiLanguageSource $null `
            -ExpectedRtlValue $null `
            -ExpectedRtlSource $null `
            -ExpectedParagraphBidiValue $null `
            -ExpectedParagraphBidiSource $null `
            -Label "$($case.id)-review-para-child-inheritance"
        Assert-StyleXmlState `
            -State $accentMarkerXmlState `
            -ExpectedType "character" `
            -ExpectedName "Accent Marker" `
            -ExpectedBasedOn "DefaultParagraphFont" `
            -ExpectedNextStyle $null `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii "Courier New" `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue "0" `
            -Label "$($case.id)-accent-marker-xml"
        Assert-StyleXmlState `
            -State $reviewTableXmlState `
            -ExpectedType "table" `
            -ExpectedName $reviewTableStyleName `
            -ExpectedBasedOn "TableNormal" `
            -ExpectedNextStyle $null `
            -ExpectedQFormat $true `
            -ExpectedSemiHidden $false `
            -ExpectedUnhideWhenUsed $true `
            -ExpectedParagraphBidi $false `
            -ExpectedOutlineLevel $null `
            -ExpectedFontAscii $null `
            -ExpectedLangValue $null `
            -ExpectedLangBidi $null `
            -ExpectedRtlValue $null `
            -Label "$($case.id)-review-table-xml"
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
        mutation_steps = $mutationSteps
        source_docx = $baselineDocxPath
        mutated_docx = $mutatedDocxPath
        mutation_json = $mutationJsonPath
        inspect_review_paragraph_json = $paragraphJsonPath
        inspect_review_child_paragraph_json = $inheritedParagraphJsonPath
        inspect_accent_run_json = $runJsonPath
        inspect_review_table_json = $reviewTableJsonPath
        inspect_review_style_json = $reviewParaStyleJsonPath
        inspect_review_child_style_json = $reviewParaChildStyleJsonPath
        inspect_review_child_inheritance_json = $reviewParaChildInheritanceJsonPath
        inspect_accent_style_json = $accentMarkerStyleJsonPath
        inspect_review_table_style_json = $reviewTableStyleJsonPath
        review_para_styles_xml = $reviewParaXmlState
        review_para_child_styles_xml = $reviewParaChildXmlState
        accent_marker_styles_xml = $accentMarkerXmlState
        review_table_styles_xml = $reviewTableXmlState
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
        review_para_paragraph = $baselineParagraphJson
        review_para_child_paragraph = $baselineInheritedParagraphJson
        accent_marker_run = $baselineRunJson
        review_table = $baselineReviewTableJson
        review_para_style = $baselineReviewParaStyleJson
        review_para_child_style = $baselineReviewParaChildStyleJson
        review_para_child_inheritance = $baselineReviewParaChildInheritanceJson
        accent_marker_style = $baselineAccentMarkerStyleJson
        review_table_style = $baselineReviewTableStyleJson
        review_para_styles_xml = $baselineReviewParaXmlState
        review_para_child_styles_xml = $baselineReviewParaChildXmlState
        accent_marker_styles_xml = $baselineAccentMarkerXmlState
        review_table_styles_xml = $baselineReviewTableXmlState
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

Write-Step "Completed ensure-style visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
