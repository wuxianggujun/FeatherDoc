param(
    [string]$BuildDir = "build-section-order-visual-nmake",
    [string]$OutputDir = "output/section-order-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[section-order-visual] $Message"
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

function Assert-BodyParagraphOrder {
    param(
        [string]$JsonPath,
        [string[]]$ExpectedTexts,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $paragraphs = @($payload.paragraphs | Where-Object { -not [string]::IsNullOrWhiteSpace($_.text) })
    if ($paragraphs.Count -ne $ExpectedTexts.Count) {
        throw "$Label non-empty paragraph array mismatch. Expected $($ExpectedTexts.Count), got $($paragraphs.Count)."
    }

    for ($index = 0; $index -lt $ExpectedTexts.Count; $index++) {
        if ($paragraphs[$index].text -ne $ExpectedTexts[$index]) {
            throw "$Label paragraph $index mismatch. Expected '$($ExpectedTexts[$index])', got '$($paragraphs[$index].text)'."
        }
    }
}

function Assert-ShowPartState {
    param(
        [string]$JsonPath,
        [bool]$ExpectedPresent,
        [string[]]$ExpectedParagraphs,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([bool]$payload.present -ne $ExpectedPresent) {
        throw "$Label expected present=$ExpectedPresent, got $($payload.present)."
    }

    $paragraphs = @($payload.paragraphs)
    if ($paragraphs.Count -ne $ExpectedParagraphs.Count) {
        throw "$Label expected $($ExpectedParagraphs.Count) shown paragraphs, got $($paragraphs.Count)."
    }

    for ($index = 0; $index -lt $ExpectedParagraphs.Count; $index++) {
        if ($paragraphs[$index] -ne $ExpectedParagraphs[$index]) {
            throw "$Label paragraph $index mismatch. Expected '$($ExpectedParagraphs[$index])', got '$($paragraphs[$index])'."
        }
    }
}

function Assert-SectionLayout {
    param(
        [string]$JsonPath,
        [int]$ExpectedSections,
        [int]$ExpectedHeaders,
        [int]$ExpectedFooters,
        [int]$SectionIndex,
        [bool]$HeaderDefault,
        [bool]$HeaderFirst,
        [bool]$HeaderEven,
        [bool]$FooterDefault,
        [bool]$FooterFirst,
        [bool]$FooterEven,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([int]$payload.sections -ne $ExpectedSections) {
        throw "$Label expected sections=$ExpectedSections, got $($payload.sections)."
    }
    if ([int]$payload.headers -ne $ExpectedHeaders) {
        throw "$Label expected headers=$ExpectedHeaders, got $($payload.headers)."
    }
    if ([int]$payload.footers -ne $ExpectedFooters) {
        throw "$Label expected footers=$ExpectedFooters, got $($payload.footers)."
    }

    $layout = @($payload.section_layouts) | Where-Object { [int]$_.index -eq $SectionIndex } | Select-Object -First 1
    if ($null -eq $layout) {
        throw "$Label did not contain section layout index $SectionIndex."
    }

    if ([bool]$layout.header.default -ne $HeaderDefault -or
        [bool]$layout.header.first -ne $HeaderFirst -or
        [bool]$layout.header.even -ne $HeaderEven -or
        [bool]$layout.footer.default -ne $FooterDefault -or
        [bool]$layout.footer.first -ne $FooterFirst -or
        [bool]$layout.footer.even -ne $FooterEven) {
        throw "$Label section[$SectionIndex] layout mismatch."
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

    Write-Step "Building featherdoc_cli and section order sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_section_order_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_section_order_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineInspectSectionsPath = Join-Path $resolvedOutputDir "shared-baseline-sections.json"
$baselineInspectParagraphsPath = Join-Path $resolvedOutputDir "shared-baseline-body.json"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"

$baselineBodyTexts = @(
    "SECTION 0 BODY PAGE",
    "Page 1 should show ALPHA HEADER and no footer in the baseline.",
    "SECTION 1 BODY PAGE",
    "Page 2 should show BETA HEADER and BETA FIRST FOOTER in the baseline.",
    "SECTION 2 BODY PAGE",
    "Page 3 should show no header and no footer in the baseline."
)

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared section order baseline sample."

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-sections", $baselineDocxPath, "--json") `
    -OutputPath $baselineInspectSectionsPath `
    -FailureMessage "Failed to inspect shared section baseline."

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-paragraphs", $baselineDocxPath, "--json") `
    -OutputPath $baselineInspectParagraphsPath `
    -FailureMessage "Failed to inspect shared baseline body paragraphs."

Assert-SectionLayout `
    -JsonPath $baselineInspectSectionsPath `
    -ExpectedSections 3 `
    -ExpectedHeaders 2 `
    -ExpectedFooters 1 `
    -SectionIndex 2 `
    -HeaderDefault $false `
    -HeaderFirst $false `
    -HeaderEven $false `
    -FooterDefault $false `
    -FooterFirst $false `
    -FooterEven $false `
    -Label "shared-baseline-sections"

Assert-BodyParagraphOrder -JsonPath $baselineInspectParagraphsPath -ExpectedTexts $baselineBodyTexts -Label "shared-baseline-body"

Write-Step "Rendering Word evidence for the shared baseline document"
Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $baselineDocxPath `
    -OutputDir $baselineVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

Assert-VisualPageCount -VisualOutputDir $baselineVisualDir -ExpectedCount 3 -Label "shared-baseline-visual"

$summaryCases = @()
$aggregateImages = @()
$aggregateLabels = @()

$insertCaseId = "insert-section-no-inherit-visual"
$insertCaseDir = Join-Path $resolvedOutputDir $insertCaseId
$insertDocxPath = Join-Path $insertCaseDir "$insertCaseId-mutated.docx"
$insertMutationJsonPath = Join-Path $insertCaseDir "mutation_result.json"
$insertInspectSectionsPath = Join-Path $insertCaseDir "inspect_sections.json"
$insertShowHeaderPath = Join-Path $insertCaseDir "show_header.json"
$insertShowFooterPath = Join-Path $insertCaseDir "show_footer.json"
$insertInspectParagraphsPath = Join-Path $insertCaseDir "inspect_body.json"
$insertVisualDir = Join-Path $insertCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $insertCaseDir -Force | Out-Null

Write-Step "Running case '$insertCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "insert-section", $baselineDocxPath, "1",
        "--no-inherit", "--output", $insertDocxPath, "--json"
    ) `
    -OutputPath $insertMutationJsonPath `
    -FailureMessage "Failed to insert a non-inherited section."

Assert-Contains -Path $insertMutationJsonPath -Expected '"command":"insert-section"' -Label $insertCaseId
Assert-Contains -Path $insertMutationJsonPath -Expected '"section":2' -Label $insertCaseId
Assert-Contains -Path $insertMutationJsonPath -Expected '"inherit_header_footer":false' -Label $insertCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-sections", $insertDocxPath, "--json") `
    -OutputPath $insertInspectSectionsPath `
    -FailureMessage "Failed to inspect inserted section layout."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("show-section-header", $insertDocxPath, "2", "--json") `
    -OutputPath $insertShowHeaderPath `
    -FailureMessage "Failed to show inserted section header."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("show-section-footer", $insertDocxPath, "2", "--json") `
    -OutputPath $insertShowFooterPath `
    -FailureMessage "Failed to show inserted section footer."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-paragraphs", $insertDocxPath, "--json") `
    -OutputPath $insertInspectParagraphsPath `
    -FailureMessage "Failed to inspect inserted section body order."

Assert-SectionLayout `
    -JsonPath $insertInspectSectionsPath `
    -ExpectedSections 4 `
    -ExpectedHeaders 2 `
    -ExpectedFooters 1 `
    -SectionIndex 2 `
    -HeaderDefault $false `
    -HeaderFirst $false `
    -HeaderEven $false `
    -FooterDefault $false `
    -FooterFirst $false `
    -FooterEven $false `
    -Label $insertCaseId
Assert-ShowPartState -JsonPath $insertShowHeaderPath -ExpectedPresent $false -ExpectedParagraphs @() -Label $insertCaseId
Assert-ShowPartState -JsonPath $insertShowFooterPath -ExpectedPresent $false -ExpectedParagraphs @() -Label $insertCaseId
Assert-BodyParagraphOrder -JsonPath $insertInspectParagraphsPath -ExpectedTexts $baselineBodyTexts -Label $insertCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $insertDocxPath `
    -OutputDir $insertVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $insertVisualDir -ExpectedCount 4 -Label $insertCaseId

$insertArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $insertCaseDir `
    -CaseId $insertCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 3 -Label $insertCaseId),
        (Get-RenderedPagePath -VisualOutputDir $insertVisualDir -PageNumber 3 -Label $insertCaseId)
    ) `
    -Labels @("$insertCaseId baseline-page-03", "$insertCaseId mutated-page-03")

$aggregateImages += $insertArtifacts.selected_pages
$aggregateLabels += $insertArtifacts.labels

$summaryCases += [ordered]@{
    id = $insertCaseId
    command = "insert-section"
    source_docx = $baselineDocxPath
    mutated_docx = $insertDocxPath
    mutation_json = $insertMutationJsonPath
    inspect_sections_json = $insertInspectSectionsPath
    shown_header_json = $insertShowHeaderPath
    shown_footer_json = $insertShowFooterPath
    inspect_body_json = $insertInspectParagraphsPath
    expected_visual_cues = @(
        "Page 3 changes from SECTION 2 BODY PAGE to a blank inserted section while still inheriting the previous BETA HEADER visually.",
        "The original section 2 content shifts to page 4 after the insertion.",
        "The inserted section keeps no local header/footer parts of its own."
    )
    visual_artifacts = $insertArtifacts
}

$copyCaseId = "copy-section-layout-into-inserted-section-visual"
$copyCaseDir = Join-Path $resolvedOutputDir $copyCaseId
$copyDocxPath = Join-Path $copyCaseDir "$copyCaseId-mutated.docx"
$copyMutationJsonPath = Join-Path $copyCaseDir "mutation_result.json"
$copyInspectSectionsPath = Join-Path $copyCaseDir "inspect_sections.json"
$copyShowHeaderPath = Join-Path $copyCaseDir "show_header.json"
$copyShowFooterPath = Join-Path $copyCaseDir "show_footer.json"
$copyInspectParagraphsPath = Join-Path $copyCaseDir "inspect_body.json"
$copyVisualDir = Join-Path $copyCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $copyCaseDir -Force | Out-Null

Write-Step "Running case '$copyCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "copy-section-layout", $insertDocxPath, "1", "2",
        "--output", $copyDocxPath, "--json"
    ) `
    -OutputPath $copyMutationJsonPath `
    -FailureMessage "Failed to copy section layout into the inserted section."

Assert-Contains -Path $copyMutationJsonPath -Expected '"command":"copy-section-layout"' -Label $copyCaseId
Assert-Contains -Path $copyMutationJsonPath -Expected '"source":1' -Label $copyCaseId
Assert-Contains -Path $copyMutationJsonPath -Expected '"target":2' -Label $copyCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-sections", $copyDocxPath, "--json") `
    -OutputPath $copyInspectSectionsPath `
    -FailureMessage "Failed to inspect copied section layout."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("show-section-header", $copyDocxPath, "2", "--json") `
    -OutputPath $copyShowHeaderPath `
    -FailureMessage "Failed to show copied section header."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("show-section-footer", $copyDocxPath, "2", "--kind", "first", "--json") `
    -OutputPath $copyShowFooterPath `
    -FailureMessage "Failed to show copied section first footer."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-paragraphs", $copyDocxPath, "--json") `
    -OutputPath $copyInspectParagraphsPath `
    -FailureMessage "Failed to inspect copied section body order."

Assert-SectionLayout `
    -JsonPath $copyInspectSectionsPath `
    -ExpectedSections 4 `
    -ExpectedHeaders 2 `
    -ExpectedFooters 1 `
    -SectionIndex 2 `
    -HeaderDefault $true `
    -HeaderFirst $false `
    -HeaderEven $false `
    -FooterDefault $false `
    -FooterFirst $true `
    -FooterEven $false `
    -Label $copyCaseId
Assert-ShowPartState -JsonPath $copyShowHeaderPath -ExpectedPresent $true -ExpectedParagraphs @("BETA HEADER") -Label $copyCaseId
Assert-ShowPartState -JsonPath $copyShowFooterPath -ExpectedPresent $true -ExpectedParagraphs @("BETA FIRST FOOTER") -Label $copyCaseId
Assert-BodyParagraphOrder -JsonPath $copyInspectParagraphsPath -ExpectedTexts $baselineBodyTexts -Label $copyCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $copyDocxPath `
    -OutputDir $copyVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $copyVisualDir -ExpectedCount 4 -Label $copyCaseId

$copyArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $copyCaseDir `
    -CaseId $copyCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $insertVisualDir -PageNumber 3 -Label $copyCaseId),
        (Get-RenderedPagePath -VisualOutputDir $copyVisualDir -PageNumber 3 -Label $copyCaseId)
    ) `
    -Labels @("$copyCaseId baseline-page-03", "$copyCaseId mutated-page-03")

$aggregateImages += $copyArtifacts.selected_pages
$aggregateLabels += $copyArtifacts.labels

$summaryCases += [ordered]@{
    id = $copyCaseId
    command = "copy-section-layout"
    source_docx = $insertDocxPath
    mutated_docx = $copyDocxPath
    mutation_json = $copyMutationJsonPath
    inspect_sections_json = $copyInspectSectionsPath
    shown_header_json = $copyShowHeaderPath
    shown_footer_json = $copyShowFooterPath
    inspect_body_json = $copyInspectParagraphsPath
    expected_visual_cues = @(
        "Page 3 stays blank in the body, keeps the inherited BETA HEADER, and now gains BETA FIRST FOOTER after copying section 1 layout into the inserted section.",
        "The copied layout does not change body paragraph order.",
        "Section 2 now exposes the same header/footer references as section 1."
    )
    visual_artifacts = $copyArtifacts
}

$reorderedBodyTexts = @(
    "SECTION 2 BODY PAGE",
    "Page 3 should show no header and no footer in the baseline.",
    "SECTION 0 BODY PAGE",
    "Page 1 should show ALPHA HEADER and no footer in the baseline.",
    "SECTION 1 BODY PAGE",
    "Page 2 should show BETA HEADER and BETA FIRST FOOTER in the baseline."
)

$moveCaseId = "move-section-three-to-zero-visual"
$moveCaseDir = Join-Path $resolvedOutputDir $moveCaseId
$moveDocxPath = Join-Path $moveCaseDir "$moveCaseId-mutated.docx"
$moveMutationJsonPath = Join-Path $moveCaseDir "mutation_result.json"
$moveInspectSectionsPath = Join-Path $moveCaseDir "inspect_sections.json"
$moveShowHeader0Path = Join-Path $moveCaseDir "show_header_section0.json"
$moveShowHeader1Path = Join-Path $moveCaseDir "show_header_section1.json"
$moveShowHeader2Path = Join-Path $moveCaseDir "show_header_section2.json"
$moveShowFooter2Path = Join-Path $moveCaseDir "show_footer_section2_first.json"
$moveInspectParagraphsPath = Join-Path $moveCaseDir "inspect_body.json"
$moveVisualDir = Join-Path $moveCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $moveCaseDir -Force | Out-Null

Write-Step "Running case '$moveCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "move-section", $copyDocxPath, "3", "0",
        "--output", $moveDocxPath, "--json"
    ) `
    -OutputPath $moveMutationJsonPath `
    -FailureMessage "Failed to move section 3 to the front."

Assert-Contains -Path $moveMutationJsonPath -Expected '"command":"move-section"' -Label $moveCaseId
Assert-Contains -Path $moveMutationJsonPath -Expected '"source":3' -Label $moveCaseId
Assert-Contains -Path $moveMutationJsonPath -Expected '"target":0' -Label $moveCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-sections", $moveDocxPath, "--json") `
    -OutputPath $moveInspectSectionsPath `
    -FailureMessage "Failed to inspect moved section layouts."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("show-section-header", $moveDocxPath, "0", "--json") `
    -OutputPath $moveShowHeader0Path `
    -FailureMessage "Failed to show moved section 0 header."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("show-section-header", $moveDocxPath, "1", "--json") `
    -OutputPath $moveShowHeader1Path `
    -FailureMessage "Failed to show moved section 1 header."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("show-section-header", $moveDocxPath, "2", "--json") `
    -OutputPath $moveShowHeader2Path `
    -FailureMessage "Failed to show moved section 2 header."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("show-section-footer", $moveDocxPath, "2", "--kind", "first", "--json") `
    -OutputPath $moveShowFooter2Path `
    -FailureMessage "Failed to show moved section 2 first footer."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-paragraphs", $moveDocxPath, "--json") `
    -OutputPath $moveInspectParagraphsPath `
    -FailureMessage "Failed to inspect moved section body order."

Assert-SectionLayout `
    -JsonPath $moveInspectSectionsPath `
    -ExpectedSections 4 `
    -ExpectedHeaders 2 `
    -ExpectedFooters 1 `
    -SectionIndex 0 `
    -HeaderDefault $false `
    -HeaderFirst $false `
    -HeaderEven $false `
    -FooterDefault $false `
    -FooterFirst $false `
    -FooterEven $false `
    -Label $moveCaseId
Assert-ShowPartState -JsonPath $moveShowHeader0Path -ExpectedPresent $false -ExpectedParagraphs @() -Label "$moveCaseId section0"
Assert-ShowPartState -JsonPath $moveShowHeader1Path -ExpectedPresent $true -ExpectedParagraphs @("ALPHA HEADER") -Label "$moveCaseId section1"
Assert-ShowPartState -JsonPath $moveShowHeader2Path -ExpectedPresent $true -ExpectedParagraphs @("BETA HEADER") -Label "$moveCaseId section2"
Assert-ShowPartState -JsonPath $moveShowFooter2Path -ExpectedPresent $true -ExpectedParagraphs @("BETA FIRST FOOTER") -Label "$moveCaseId section2-footer"
Assert-BodyParagraphOrder -JsonPath $moveInspectParagraphsPath -ExpectedTexts $reorderedBodyTexts -Label $moveCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $moveDocxPath `
    -OutputDir $moveVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $moveVisualDir -ExpectedCount 4 -Label $moveCaseId

$moveArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $moveCaseDir `
    -CaseId $moveCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $copyVisualDir -PageNumber 1 -Label $moveCaseId),
        (Get-RenderedPagePath -VisualOutputDir $moveVisualDir -PageNumber 1 -Label $moveCaseId)
    ) `
    -Labels @("$moveCaseId baseline-page-01", "$moveCaseId mutated-page-01")

$aggregateImages += $moveArtifacts.selected_pages
$aggregateLabels += $moveArtifacts.labels

$summaryCases += [ordered]@{
    id = $moveCaseId
    command = "move-section"
    source_docx = $copyDocxPath
    mutated_docx = $moveDocxPath
    mutation_json = $moveMutationJsonPath
    inspect_sections_json = $moveInspectSectionsPath
    shown_header_section0_json = $moveShowHeader0Path
    shown_header_section1_json = $moveShowHeader1Path
    shown_header_section2_json = $moveShowHeader2Path
    shown_footer_section2_first_json = $moveShowFooter2Path
    inspect_body_json = $moveInspectParagraphsPath
    expected_visual_cues = @(
        "Page 1 changes from SECTION 0 BODY PAGE with ALPHA HEADER to SECTION 2 BODY PAGE with no header after moving section 3 to the front.",
        "The body paragraph order becomes section 2, section 0, then section 1.",
        "The copied blank section remains at the end as page 4."
    )
    visual_artifacts = $moveArtifacts
}

$removeCaseId = "remove-last-section-visual"
$removeCaseDir = Join-Path $resolvedOutputDir $removeCaseId
$removeDocxPath = Join-Path $removeCaseDir "$removeCaseId-mutated.docx"
$removeMutationJsonPath = Join-Path $removeCaseDir "mutation_result.json"
$removeInspectSectionsPath = Join-Path $removeCaseDir "inspect_sections.json"
$removeShowHeader2Path = Join-Path $removeCaseDir "show_header_section2.json"
$removeShowFooter2Path = Join-Path $removeCaseDir "show_footer_section2_first.json"
$removeInspectParagraphsPath = Join-Path $removeCaseDir "inspect_body.json"
$removeVisualDir = Join-Path $removeCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $removeCaseDir -Force | Out-Null

Write-Step "Running case '$removeCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "remove-section", $moveDocxPath, "3",
        "--output", $removeDocxPath, "--json"
    ) `
    -OutputPath $removeMutationJsonPath `
    -FailureMessage "Failed to remove the last section."

Assert-Contains -Path $removeMutationJsonPath -Expected '"command":"remove-section"' -Label $removeCaseId
Assert-Contains -Path $removeMutationJsonPath -Expected '"section":3' -Label $removeCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-sections", $removeDocxPath, "--json") `
    -OutputPath $removeInspectSectionsPath `
    -FailureMessage "Failed to inspect removed section layouts."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("show-section-header", $removeDocxPath, "2", "--json") `
    -OutputPath $removeShowHeader2Path `
    -FailureMessage "Failed to show remaining section 2 header."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("show-section-footer", $removeDocxPath, "2", "--kind", "first", "--json") `
    -OutputPath $removeShowFooter2Path `
    -FailureMessage "Failed to show remaining section 2 first footer."
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-paragraphs", $removeDocxPath, "--json") `
    -OutputPath $removeInspectParagraphsPath `
    -FailureMessage "Failed to inspect removed section body order."

Assert-SectionLayout `
    -JsonPath $removeInspectSectionsPath `
    -ExpectedSections 3 `
    -ExpectedHeaders 2 `
    -ExpectedFooters 1 `
    -SectionIndex 2 `
    -HeaderDefault $true `
    -HeaderFirst $false `
    -HeaderEven $false `
    -FooterDefault $false `
    -FooterFirst $true `
    -FooterEven $false `
    -Label $removeCaseId
Assert-ShowPartState -JsonPath $removeShowHeader2Path -ExpectedPresent $true -ExpectedParagraphs @("BETA HEADER") -Label "$removeCaseId section2"
Assert-ShowPartState -JsonPath $removeShowFooter2Path -ExpectedPresent $true -ExpectedParagraphs @("BETA FIRST FOOTER") -Label "$removeCaseId section2-footer"
Assert-BodyParagraphOrder -JsonPath $removeInspectParagraphsPath -ExpectedTexts $reorderedBodyTexts -Label $removeCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $removeDocxPath `
    -OutputDir $removeVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $removeVisualDir -ExpectedCount 3 -Label $removeCaseId

$removeArtifacts = Register-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $removeCaseDir `
    -CaseId $removeCaseId `
    -Images @(
        (Get-RenderedPagePath -VisualOutputDir $moveVisualDir -PageNumber 3 -Label $removeCaseId),
        (Get-RenderedPagePath -VisualOutputDir $moveVisualDir -PageNumber 4 -Label $removeCaseId),
        (Get-RenderedPagePath -VisualOutputDir $removeVisualDir -PageNumber 3 -Label $removeCaseId)
    ) `
    -Labels @(
        "$removeCaseId baseline-page-03",
        "$removeCaseId baseline-page-04",
        "$removeCaseId mutated-page-03"
    )

$aggregateImages += $removeArtifacts.selected_pages
$aggregateLabels += $removeArtifacts.labels

$summaryCases += [ordered]@{
    id = $removeCaseId
    command = "remove-section"
    source_docx = $moveDocxPath
    mutated_docx = $removeDocxPath
    mutation_json = $removeMutationJsonPath
    inspect_sections_json = $removeInspectSectionsPath
    shown_header_section2_json = $removeShowHeader2Path
    shown_footer_section2_first_json = $removeShowFooter2Path
    inspect_body_json = $removeInspectParagraphsPath
    expected_visual_cues = @(
        "The final blank copied section disappears, shrinking the document from 4 pages to 3 pages.",
        "Page 3 remains SECTION 1 BODY PAGE with BETA HEADER and BETA FIRST FOOTER after the tail section is removed.",
        "The moved body order stays section 2, section 0, then section 1."
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
    shared_baseline_sections = $baselineInspectSectionsPath
    shared_baseline_body = $baselineInspectParagraphsPath
    cases = $summaryCases
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed section order visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
