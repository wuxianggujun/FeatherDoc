param(
    [string]$BuildDir = "build-section-part-refs-visual-nmake",
    [string]$OutputDir = "output/section-part-refs-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[section-part-refs-visual] $Message"
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

function Read-JsonFile {
    param([string]$Path)

    return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
}

function Find-PartByParagraphText {
    param(
        [object]$Payload,
        [string]$ParagraphText,
        [string]$Label
    )

    foreach ($part in @($Payload.parts)) {
        if (@($part.paragraphs) -contains $ParagraphText) {
            return $part
        }
    }

    throw "$Label did not contain a part with paragraph text '$ParagraphText'."
}

function Get-PartIndexByParagraphText {
    param(
        [string]$JsonPath,
        [string]$ParagraphText,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $part = Find-PartByParagraphText -Payload $payload -ParagraphText $ParagraphText -Label $Label
    return [int]$part.index
}

function Assert-PartCount {
    param(
        [string]$JsonPath,
        [int]$ExpectedCount,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([int]$payload.count -ne $ExpectedCount) {
        throw "$Label expected part count $ExpectedCount, got $($payload.count)."
    }
}

function Assert-PartHasReference {
    param(
        [string]$JsonPath,
        [string]$ParagraphText,
        [int]$Section,
        [string]$Kind,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $part = Find-PartByParagraphText -Payload $payload -ParagraphText $ParagraphText -Label $Label
    foreach ($reference in @($part.references)) {
        if ([int]$reference.section -eq $Section -and $reference.kind -eq $Kind) {
            return
        }
    }

    throw "$Label part '$ParagraphText' did not reference section $Section kind '$Kind'."
}

function Assert-PartMissingReference {
    param(
        [string]$JsonPath,
        [string]$ParagraphText,
        [int]$Section,
        [string]$Kind,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $part = Find-PartByParagraphText -Payload $payload -ParagraphText $ParagraphText -Label $Label
    foreach ($reference in @($part.references)) {
        if ([int]$reference.section -eq $Section -and $reference.kind -eq $Kind) {
            throw "$Label part '$ParagraphText' unexpectedly referenced section $Section kind '$Kind'."
        }
    }
}

function Assert-PartMissingParagraph {
    param(
        [string]$JsonPath,
        [string]$ParagraphText,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    foreach ($part in @($payload.parts)) {
        if (@($part.paragraphs) -contains $ParagraphText) {
            throw "$Label unexpectedly still contained paragraph text '$ParagraphText'."
        }
    }
}

function Assert-ParagraphTexts {
    param(
        [string]$JsonPath,
        [string[]]$ExpectedTexts,
        [string]$Label
    )

    $json = Read-JsonFile -Path $JsonPath
    if ([int]$json.count -ne $ExpectedTexts.Count) {
        throw "$Label paragraph count mismatch. Expected $($ExpectedTexts.Count), got $($json.count)."
    }

    $paragraphs = @($json.paragraphs)
    if ($paragraphs.Count -ne $ExpectedTexts.Count) {
        throw "$Label paragraph array mismatch. Expected $($ExpectedTexts.Count), got $($paragraphs.Count)."
    }

    for ($index = 0; $index -lt $ExpectedTexts.Count; $index++) {
        if ($paragraphs[$index].text -ne $ExpectedTexts[$index]) {
            throw "$Label paragraph $index mismatch. Expected '$($ExpectedTexts[$index])', got '$($paragraphs[$index].text)'."
        }
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

function Add-ComparisonArtifacts {
    param(
        [string]$Python,
        [string]$ContactSheetScript,
        [string]$AggregatePagesDir,
        [string]$CaseOutputDir,
        [string]$CaseId,
        [string]$BaselinePagePath,
        [string]$MutatedPagePath,
        [string]$BaselineLabel,
        [string]$MutatedLabel
    )

    $copiedBaselinePagePath = Join-Path $AggregatePagesDir "$CaseId-baseline-page.png"
    $copiedMutatedPagePath = Join-Path $AggregatePagesDir "$CaseId-mutated-page.png"
    Copy-Item -Path $BaselinePagePath -Destination $copiedBaselinePagePath -Force
    Copy-Item -Path $MutatedPagePath -Destination $copiedMutatedPagePath -Force

    $caseContactSheetPath = Join-Path $CaseOutputDir "before_after_contact_sheet.png"
    Build-ContactSheet `
        -Python $Python `
        -ScriptPath $ContactSheetScript `
        -OutputPath $caseContactSheetPath `
        -Images @($copiedBaselinePagePath, $copiedMutatedPagePath) `
        -Labels @($BaselineLabel, $MutatedLabel)

    return [ordered]@{
        before_after_contact_sheet = $caseContactSheetPath
        baseline_page = $copiedBaselinePagePath
        mutated_page = $copiedMutatedPagePath
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

    Write-Step "Building featherdoc_cli and section part refs sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_section_part_refs_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_section_part_refs_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$sharedBaselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$sharedBaselineHeaderPartsPath = Join-Path $resolvedOutputDir "shared-baseline-header-parts.json"
$sharedBaselineFooterPartsPath = Join-Path $resolvedOutputDir "shared-baseline-footer-parts.json"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($sharedBaselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared section part refs baseline sample."

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-header-parts", $sharedBaselineDocxPath, "--json") `
    -OutputPath $sharedBaselineHeaderPartsPath `
    -FailureMessage "Failed to inspect shared baseline header parts."

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-footer-parts", $sharedBaselineDocxPath, "--json") `
    -OutputPath $sharedBaselineFooterPartsPath `
    -FailureMessage "Failed to inspect shared baseline footer parts."

Assert-PartCount -JsonPath $sharedBaselineHeaderPartsPath -ExpectedCount 2 -Label "shared-baseline-header-parts"
Assert-PartCount -JsonPath $sharedBaselineFooterPartsPath -ExpectedCount 2 -Label "shared-baseline-footer-parts"
Assert-PartHasReference -JsonPath $sharedBaselineHeaderPartsPath -ParagraphText "ALPHA SHARED HEADER" -Section 0 -Kind "default" -Label "shared-baseline-header-parts"
Assert-PartHasReference -JsonPath $sharedBaselineHeaderPartsPath -ParagraphText "BETA REMOVABLE HEADER" -Section 1 -Kind "default" -Label "shared-baseline-header-parts"
Assert-PartHasReference -JsonPath $sharedBaselineFooterPartsPath -ParagraphText "ALPHA SHARED FOOTER" -Section 0 -Kind "default" -Label "shared-baseline-footer-parts"
Assert-PartHasReference -JsonPath $sharedBaselineFooterPartsPath -ParagraphText "BETA FIRST FOOTER" -Section 1 -Kind "first" -Label "shared-baseline-footer-parts"

$sharedHeaderIndex = Get-PartIndexByParagraphText -JsonPath $sharedBaselineHeaderPartsPath -ParagraphText "ALPHA SHARED HEADER" -Label "shared-baseline-header-parts"
$removableHeaderIndex = Get-PartIndexByParagraphText -JsonPath $sharedBaselineHeaderPartsPath -ParagraphText "BETA REMOVABLE HEADER" -Label "shared-baseline-header-parts"
$sharedFooterIndex = Get-PartIndexByParagraphText -JsonPath $sharedBaselineFooterPartsPath -ParagraphText "ALPHA SHARED FOOTER" -Label "shared-baseline-footer-parts"
$removableFooterIndex = Get-PartIndexByParagraphText -JsonPath $sharedBaselineFooterPartsPath -ParagraphText "BETA FIRST FOOTER" -Label "shared-baseline-footer-parts"

$sharedBaselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"
Write-Step "Rendering Word evidence for the shared baseline document"
Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $sharedBaselineDocxPath `
    -OutputDir $sharedBaselineVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$summaryCases = @()
$aggregateImages = @()
$aggregateLabels = @()

$assignHeaderCaseId = "assign-section-header-even-reference-visual"
$assignHeaderCaseDir = Join-Path $resolvedOutputDir $assignHeaderCaseId
$assignHeaderDocxPath = Join-Path $assignHeaderCaseDir "$assignHeaderCaseId-mutated.docx"
$assignHeaderMutationJsonPath = Join-Path $assignHeaderCaseDir "mutation_result.json"
$assignHeaderParagraphJsonPath = Join-Path $assignHeaderCaseDir "inspect_paragraphs.json"
$assignHeaderPartsJsonPath = Join-Path $assignHeaderCaseDir "inspect_header_parts.json"
$assignHeaderVisualDir = Join-Path $assignHeaderCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $assignHeaderCaseDir -Force | Out-Null

Write-Step "Running case '$assignHeaderCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "assign-section-header", $sharedBaselineDocxPath, "2", $sharedHeaderIndex,
        "--kind", "even", "--output", $assignHeaderDocxPath, "--json"
    ) `
    -OutputPath $assignHeaderMutationJsonPath `
    -FailureMessage "Failed to assign section header reference."

Assert-Contains -Path $assignHeaderMutationJsonPath -Expected '"command":"assign-section-header"' -Label $assignHeaderCaseId
Assert-Contains -Path $assignHeaderMutationJsonPath -Expected '"kind":"even"' -Label $assignHeaderCaseId
Assert-Contains -Path $assignHeaderMutationJsonPath -Expected ('"part_index":' + $sharedHeaderIndex) -Label $assignHeaderCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "inspect-template-paragraphs", $assignHeaderDocxPath,
        "--part", "section-header", "--section", "2", "--kind", "even", "--json"
    ) `
    -OutputPath $assignHeaderParagraphJsonPath `
    -FailureMessage "Failed to inspect assigned section header paragraphs."

Assert-ParagraphTexts -JsonPath $assignHeaderParagraphJsonPath -ExpectedTexts @("ALPHA SHARED HEADER") -Label $assignHeaderCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-header-parts", $assignHeaderDocxPath, "--json") `
    -OutputPath $assignHeaderPartsJsonPath `
    -FailureMessage "Failed to inspect assigned section header parts."

Assert-PartCount -JsonPath $assignHeaderPartsJsonPath -ExpectedCount 2 -Label $assignHeaderCaseId
Assert-PartHasReference -JsonPath $assignHeaderPartsJsonPath -ParagraphText "ALPHA SHARED HEADER" -Section 2 -Kind "even" -Label $assignHeaderCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $assignHeaderDocxPath `
    -OutputDir $assignHeaderVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$assignHeaderArtifacts = Add-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $assignHeaderCaseDir `
    -CaseId $assignHeaderCaseId `
    -BaselinePagePath (Get-RenderedPagePath -VisualOutputDir $sharedBaselineVisualDir -PageNumber 4 -Label $assignHeaderCaseId) `
    -MutatedPagePath (Get-RenderedPagePath -VisualOutputDir $assignHeaderVisualDir -PageNumber 4 -Label $assignHeaderCaseId) `
    -BaselineLabel "$assignHeaderCaseId baseline" `
    -MutatedLabel "$assignHeaderCaseId mutated"

$aggregateImages += $assignHeaderArtifacts.baseline_page
$aggregateImages += $assignHeaderArtifacts.mutated_page
$aggregateLabels += "$assignHeaderCaseId baseline"
$aggregateLabels += "$assignHeaderCaseId mutated"

$summaryCases += [ordered]@{
    id = $assignHeaderCaseId
    command = "assign-section-header"
    source_docx = $sharedBaselineDocxPath
    mutated_docx = $assignHeaderDocxPath
    mutation_json = $assignHeaderMutationJsonPath
    paragraph_json = $assignHeaderParagraphJsonPath
    part_inspect_json = $assignHeaderPartsJsonPath
    expected_visual_cues = @(
        "Page 4 changes from no header to ALPHA SHARED HEADER after assigning the shared even header into section 2.",
        "Section 2 body copy remains in place while the new even-page header appears above it.",
        "The shared header part is still reused rather than cloned, and now references section 0 default plus section 2 even."
    )
    visual_artifacts = $assignHeaderArtifacts
}

$assignFooterCaseId = "assign-section-footer-default-reference-visual"
$assignFooterCaseDir = Join-Path $resolvedOutputDir $assignFooterCaseId
$assignFooterDocxPath = Join-Path $assignFooterCaseDir "$assignFooterCaseId-mutated.docx"
$assignFooterMutationJsonPath = Join-Path $assignFooterCaseDir "mutation_result.json"
$assignFooterParagraphJsonPath = Join-Path $assignFooterCaseDir "inspect_paragraphs.json"
$assignFooterPartsJsonPath = Join-Path $assignFooterCaseDir "inspect_footer_parts.json"
$assignFooterVisualDir = Join-Path $assignFooterCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $assignFooterCaseDir -Force | Out-Null

Write-Step "Running case '$assignFooterCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "assign-section-footer", $sharedBaselineDocxPath, "2", $removableFooterIndex,
        "--output", $assignFooterDocxPath, "--json"
    ) `
    -OutputPath $assignFooterMutationJsonPath `
    -FailureMessage "Failed to assign section footer reference."

Assert-Contains -Path $assignFooterMutationJsonPath -Expected '"command":"assign-section-footer"' -Label $assignFooterCaseId
Assert-Contains -Path $assignFooterMutationJsonPath -Expected '"kind":"default"' -Label $assignFooterCaseId
Assert-Contains -Path $assignFooterMutationJsonPath -Expected ('"part_index":' + $removableFooterIndex) -Label $assignFooterCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "inspect-template-paragraphs", $assignFooterDocxPath,
        "--part", "section-footer", "--section", "2", "--kind", "default", "--json"
    ) `
    -OutputPath $assignFooterParagraphJsonPath `
    -FailureMessage "Failed to inspect assigned section footer paragraphs."

Assert-ParagraphTexts -JsonPath $assignFooterParagraphJsonPath -ExpectedTexts @("BETA FIRST FOOTER") -Label $assignFooterCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-footer-parts", $assignFooterDocxPath, "--json") `
    -OutputPath $assignFooterPartsJsonPath `
    -FailureMessage "Failed to inspect assigned section footer parts."

Assert-PartCount -JsonPath $assignFooterPartsJsonPath -ExpectedCount 2 -Label $assignFooterCaseId
Assert-PartHasReference -JsonPath $assignFooterPartsJsonPath -ParagraphText "BETA FIRST FOOTER" -Section 2 -Kind "default" -Label $assignFooterCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $assignFooterDocxPath `
    -OutputDir $assignFooterVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$assignFooterArtifacts = Add-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $assignFooterCaseDir `
    -CaseId $assignFooterCaseId `
    -BaselinePagePath (Get-RenderedPagePath -VisualOutputDir $sharedBaselineVisualDir -PageNumber 3 -Label $assignFooterCaseId) `
    -MutatedPagePath (Get-RenderedPagePath -VisualOutputDir $assignFooterVisualDir -PageNumber 3 -Label $assignFooterCaseId) `
    -BaselineLabel "$assignFooterCaseId baseline" `
    -MutatedLabel "$assignFooterCaseId mutated"

$aggregateImages += $assignFooterArtifacts.baseline_page
$aggregateImages += $assignFooterArtifacts.mutated_page
$aggregateLabels += "$assignFooterCaseId baseline"
$aggregateLabels += "$assignFooterCaseId mutated"

$summaryCases += [ordered]@{
    id = $assignFooterCaseId
    command = "assign-section-footer"
    source_docx = $sharedBaselineDocxPath
    mutated_docx = $assignFooterDocxPath
    mutation_json = $assignFooterMutationJsonPath
    paragraph_json = $assignFooterParagraphJsonPath
    part_inspect_json = $assignFooterPartsJsonPath
    expected_visual_cues = @(
        "Page 3 changes from the inherited ALPHA SHARED FOOTER to BETA FIRST FOOTER after assigning a different footer part into section 2.",
        "Section 2 body copy keeps its layout while the newly assigned footer appears at the bottom margin.",
        "The BETA FIRST FOOTER part is reused rather than cloned, and now references section 1 first plus section 2 default."
    )
    visual_artifacts = $assignFooterArtifacts
}

$removeHeaderCaseId = "remove-section-header-even-reference-visual"
$removeHeaderCaseDir = Join-Path $resolvedOutputDir $removeHeaderCaseId
$removeHeaderDocxPath = Join-Path $removeHeaderCaseDir "$removeHeaderCaseId-mutated.docx"
$removeHeaderMutationJsonPath = Join-Path $removeHeaderCaseDir "mutation_result.json"
$removeHeaderPartsJsonPath = Join-Path $removeHeaderCaseDir "inspect_header_parts.json"
$removeHeaderVisualDir = Join-Path $removeHeaderCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $removeHeaderCaseDir -Force | Out-Null

Write-Step "Running case '$removeHeaderCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "remove-section-header", $assignHeaderDocxPath, "2",
        "--kind", "even", "--output", $removeHeaderDocxPath, "--json"
    ) `
    -OutputPath $removeHeaderMutationJsonPath `
    -FailureMessage "Failed to remove assigned even header reference."

Assert-Contains -Path $removeHeaderMutationJsonPath -Expected '"command":"remove-section-header"' -Label $removeHeaderCaseId
Assert-Contains -Path $removeHeaderMutationJsonPath -Expected '"kind":"even"' -Label $removeHeaderCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-header-parts", $removeHeaderDocxPath, "--json") `
    -OutputPath $removeHeaderPartsJsonPath `
    -FailureMessage "Failed to inspect header parts after removing the even header reference."

Assert-PartCount -JsonPath $removeHeaderPartsJsonPath -ExpectedCount 2 -Label $removeHeaderCaseId
Assert-PartMissingReference -JsonPath $removeHeaderPartsJsonPath -ParagraphText "ALPHA SHARED HEADER" -Section 2 -Kind "even" -Label $removeHeaderCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $removeHeaderDocxPath `
    -OutputDir $removeHeaderVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$removeHeaderArtifacts = Add-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $removeHeaderCaseDir `
    -CaseId $removeHeaderCaseId `
    -BaselinePagePath (Get-RenderedPagePath -VisualOutputDir $assignHeaderVisualDir -PageNumber 4 -Label $removeHeaderCaseId) `
    -MutatedPagePath (Get-RenderedPagePath -VisualOutputDir $removeHeaderVisualDir -PageNumber 4 -Label $removeHeaderCaseId) `
    -BaselineLabel "$removeHeaderCaseId baseline" `
    -MutatedLabel "$removeHeaderCaseId mutated"

$aggregateImages += $removeHeaderArtifacts.baseline_page
$aggregateImages += $removeHeaderArtifacts.mutated_page
$aggregateLabels += "$removeHeaderCaseId baseline"
$aggregateLabels += "$removeHeaderCaseId mutated"

$summaryCases += [ordered]@{
    id = $removeHeaderCaseId
    command = "remove-section-header"
    source_docx = $assignHeaderDocxPath
    mutated_docx = $removeHeaderDocxPath
    mutation_json = $removeHeaderMutationJsonPath
    part_inspect_json = $removeHeaderPartsJsonPath
    expected_visual_cues = @(
        "Page 4 loses ALPHA SHARED HEADER after removing the section 2 even-header reference.",
        "The section 2 body remains in place and does not shift when the header disappears.",
        "The shared header part still exists for section 0, but no longer references section 2 even pages."
    )
    visual_artifacts = $removeHeaderArtifacts
}

$removeFooterCaseId = "remove-section-footer-default-reference-visual"
$removeFooterCaseDir = Join-Path $resolvedOutputDir $removeFooterCaseId
$removeFooterDocxPath = Join-Path $removeFooterCaseDir "$removeFooterCaseId-mutated.docx"
$removeFooterMutationJsonPath = Join-Path $removeFooterCaseDir "mutation_result.json"
$removeFooterPartsJsonPath = Join-Path $removeFooterCaseDir "inspect_footer_parts.json"
$removeFooterVisualDir = Join-Path $removeFooterCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $removeFooterCaseDir -Force | Out-Null

Write-Step "Running case '$removeFooterCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "remove-section-footer", $assignFooterDocxPath, "2",
        "--output", $removeFooterDocxPath, "--json"
    ) `
    -OutputPath $removeFooterMutationJsonPath `
    -FailureMessage "Failed to remove assigned default footer reference."

Assert-Contains -Path $removeFooterMutationJsonPath -Expected '"command":"remove-section-footer"' -Label $removeFooterCaseId
Assert-Contains -Path $removeFooterMutationJsonPath -Expected '"kind":"default"' -Label $removeFooterCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-footer-parts", $removeFooterDocxPath, "--json") `
    -OutputPath $removeFooterPartsJsonPath `
    -FailureMessage "Failed to inspect footer parts after removing the default footer reference."

Assert-PartCount -JsonPath $removeFooterPartsJsonPath -ExpectedCount 2 -Label $removeFooterCaseId
Assert-PartMissingReference -JsonPath $removeFooterPartsJsonPath -ParagraphText "BETA FIRST FOOTER" -Section 2 -Kind "default" -Label $removeFooterCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $removeFooterDocxPath `
    -OutputDir $removeFooterVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$removeFooterArtifacts = Add-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $removeFooterCaseDir `
    -CaseId $removeFooterCaseId `
    -BaselinePagePath (Get-RenderedPagePath -VisualOutputDir $assignFooterVisualDir -PageNumber 3 -Label $removeFooterCaseId) `
    -MutatedPagePath (Get-RenderedPagePath -VisualOutputDir $removeFooterVisualDir -PageNumber 3 -Label $removeFooterCaseId) `
    -BaselineLabel "$removeFooterCaseId baseline" `
    -MutatedLabel "$removeFooterCaseId mutated"

$aggregateImages += $removeFooterArtifacts.baseline_page
$aggregateImages += $removeFooterArtifacts.mutated_page
$aggregateLabels += "$removeFooterCaseId baseline"
$aggregateLabels += "$removeFooterCaseId mutated"

$summaryCases += [ordered]@{
    id = $removeFooterCaseId
    command = "remove-section-footer"
    source_docx = $assignFooterDocxPath
    mutated_docx = $removeFooterDocxPath
    mutation_json = $removeFooterMutationJsonPath
    part_inspect_json = $removeFooterPartsJsonPath
    expected_visual_cues = @(
        "Page 3 falls back from BETA FIRST FOOTER to the inherited ALPHA SHARED FOOTER after removing the section 2 default-footer reference.",
        "The section 2 body remains in place and does not shift when the footer changes back.",
        "The BETA FIRST FOOTER part still exists for section 1 first page, but no longer references section 2 default pages."
    )
    visual_artifacts = $removeFooterArtifacts
}

$removeHeaderPartCaseId = "remove-header-part-section-1-visual"
$removeHeaderPartCaseDir = Join-Path $resolvedOutputDir $removeHeaderPartCaseId
$removeHeaderPartDocxPath = Join-Path $removeHeaderPartCaseDir "$removeHeaderPartCaseId-mutated.docx"
$removeHeaderPartMutationJsonPath = Join-Path $removeHeaderPartCaseDir "mutation_result.json"
$removeHeaderPartInspectJsonPath = Join-Path $removeHeaderPartCaseDir "inspect_header_parts.json"
$removeHeaderPartVisualDir = Join-Path $removeHeaderPartCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $removeHeaderPartCaseDir -Force | Out-Null

Write-Step "Running case '$removeHeaderPartCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "remove-header-part", $sharedBaselineDocxPath, $removableHeaderIndex,
        "--output", $removeHeaderPartDocxPath, "--json"
    ) `
    -OutputPath $removeHeaderPartMutationJsonPath `
    -FailureMessage "Failed to remove the dedicated section 1 header part."

Assert-Contains -Path $removeHeaderPartMutationJsonPath -Expected '"command":"remove-header-part"' -Label $removeHeaderPartCaseId
Assert-Contains -Path $removeHeaderPartMutationJsonPath -Expected ('"part_index":' + $removableHeaderIndex) -Label $removeHeaderPartCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-header-parts", $removeHeaderPartDocxPath, "--json") `
    -OutputPath $removeHeaderPartInspectJsonPath `
    -FailureMessage "Failed to inspect header parts after removing the dedicated section 1 header part."

Assert-PartCount -JsonPath $removeHeaderPartInspectJsonPath -ExpectedCount 1 -Label $removeHeaderPartCaseId
Assert-PartMissingParagraph -JsonPath $removeHeaderPartInspectJsonPath -ParagraphText "BETA REMOVABLE HEADER" -Label $removeHeaderPartCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $removeHeaderPartDocxPath `
    -OutputDir $removeHeaderPartVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$removeHeaderPartArtifacts = Add-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $removeHeaderPartCaseDir `
    -CaseId $removeHeaderPartCaseId `
    -BaselinePagePath (Get-RenderedPagePath -VisualOutputDir $sharedBaselineVisualDir -PageNumber 4 -Label $removeHeaderPartCaseId) `
    -MutatedPagePath (Get-RenderedPagePath -VisualOutputDir $removeHeaderPartVisualDir -PageNumber 4 -Label $removeHeaderPartCaseId) `
    -BaselineLabel "$removeHeaderPartCaseId baseline" `
    -MutatedLabel "$removeHeaderPartCaseId mutated"

$aggregateImages += $removeHeaderPartArtifacts.baseline_page
$aggregateImages += $removeHeaderPartArtifacts.mutated_page
$aggregateLabels += "$removeHeaderPartCaseId baseline"
$aggregateLabels += "$removeHeaderPartCaseId mutated"

$summaryCases += [ordered]@{
    id = $removeHeaderPartCaseId
    command = "remove-header-part"
    source_docx = $sharedBaselineDocxPath
    mutated_docx = $removeHeaderPartDocxPath
    mutation_json = $removeHeaderPartMutationJsonPath
    part_inspect_json = $removeHeaderPartInspectJsonPath
    expected_visual_cues = @(
        "Page 4 stops inheriting BETA REMOVABLE HEADER and falls back to ALPHA SHARED HEADER after the dedicated section 1 header part is pruned.",
        "The section 2 body copy stays in place while the inherited header source changes on the even page.",
        "Only the shared ALPHA SHARED HEADER part remains in the document after pruning."
    )
    visual_artifacts = $removeHeaderPartArtifacts
}

$removeFooterPartCaseId = "remove-footer-part-section-1-first-visual"
$removeFooterPartCaseDir = Join-Path $resolvedOutputDir $removeFooterPartCaseId
$removeFooterPartDocxPath = Join-Path $removeFooterPartCaseDir "$removeFooterPartCaseId-mutated.docx"
$removeFooterPartMutationJsonPath = Join-Path $removeFooterPartCaseDir "mutation_result.json"
$removeFooterPartInspectJsonPath = Join-Path $removeFooterPartCaseDir "inspect_footer_parts.json"
$removeFooterPartVisualDir = Join-Path $removeFooterPartCaseDir "mutated-visual"
New-Item -ItemType Directory -Path $removeFooterPartCaseDir -Force | Out-Null

Write-Step "Running case '$removeFooterPartCaseId'"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "remove-footer-part", $sharedBaselineDocxPath, $removableFooterIndex,
        "--output", $removeFooterPartDocxPath, "--json"
    ) `
    -OutputPath $removeFooterPartMutationJsonPath `
    -FailureMessage "Failed to remove the dedicated section 1 first-footer part."

Assert-Contains -Path $removeFooterPartMutationJsonPath -Expected '"command":"remove-footer-part"' -Label $removeFooterPartCaseId
Assert-Contains -Path $removeFooterPartMutationJsonPath -Expected ('"part_index":' + $removableFooterIndex) -Label $removeFooterPartCaseId

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-footer-parts", $removeFooterPartDocxPath, "--json") `
    -OutputPath $removeFooterPartInspectJsonPath `
    -FailureMessage "Failed to inspect footer parts after removing the dedicated section 1 first-footer part."

Assert-PartCount -JsonPath $removeFooterPartInspectJsonPath -ExpectedCount 1 -Label $removeFooterPartCaseId
Assert-PartMissingParagraph -JsonPath $removeFooterPartInspectJsonPath -ParagraphText "BETA FIRST FOOTER" -Label $removeFooterPartCaseId

Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $removeFooterPartDocxPath `
    -OutputDir $removeFooterPartVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$removeFooterPartArtifacts = Add-ComparisonArtifacts `
    -Python $renderPython `
    -ContactSheetScript $contactSheetScript `
    -AggregatePagesDir $aggregatePagesDir `
    -CaseOutputDir $removeFooterPartCaseDir `
    -CaseId $removeFooterPartCaseId `
    -BaselinePagePath (Get-RenderedPagePath -VisualOutputDir $sharedBaselineVisualDir -PageNumber 2 -Label $removeFooterPartCaseId) `
    -MutatedPagePath (Get-RenderedPagePath -VisualOutputDir $removeFooterPartVisualDir -PageNumber 2 -Label $removeFooterPartCaseId) `
    -BaselineLabel "$removeFooterPartCaseId baseline" `
    -MutatedLabel "$removeFooterPartCaseId mutated"

$aggregateImages += $removeFooterPartArtifacts.baseline_page
$aggregateImages += $removeFooterPartArtifacts.mutated_page
$aggregateLabels += "$removeFooterPartCaseId baseline"
$aggregateLabels += "$removeFooterPartCaseId mutated"

$summaryCases += [ordered]@{
    id = $removeFooterPartCaseId
    command = "remove-footer-part"
    source_docx = $sharedBaselineDocxPath
    mutated_docx = $removeFooterPartDocxPath
    mutation_json = $removeFooterPartMutationJsonPath
    part_inspect_json = $removeFooterPartInspectJsonPath
    expected_visual_cues = @(
        "Page 2 loses BETA FIRST FOOTER after the dedicated first-page footer part is pruned.",
        "The page 2 body copy stays in place and does not shift when the footer part disappears.",
        "Only the shared ALPHA SHARED FOOTER part remains in the document after pruning."
    )
    visual_artifacts = $removeFooterPartArtifacts
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
    shared_baseline_docx = $sharedBaselineDocxPath
    shared_baseline_header_parts = $sharedBaselineHeaderPartsPath
    shared_baseline_footer_parts = $sharedBaselineFooterPartsPath
    shared_part_indexes = [ordered]@{
        shared_header = $sharedHeaderIndex
        removable_header = $removableHeaderIndex
        shared_footer = $sharedFooterIndex
        removable_footer = $removableFooterIndex
    }
    cases = $summaryCases
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed section part refs visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
