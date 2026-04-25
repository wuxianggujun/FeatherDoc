param(
    [string]$BuildDir = "build-append-image-visual-nmake",
    [string]$OutputDir = "output/append-image-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[append-image-visual] $Message"
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

function Assert-NotContains {
    param(
        [string]$Path,
        [string]$Unexpected,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content.Contains($Unexpected)) {
        throw "$Label unexpectedly contained text: $Unexpected"
    }
}

function Assert-ImageState {
    param(
        [string]$JsonPath,
        [int]$ExpectedCount,
        [string[]]$ExpectedFields,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $JsonPath
    foreach ($expected in $ExpectedFields) {
        if (-not $content.Contains($expected)) {
            throw "$Label did not contain expected image field: $expected"
        }
    }

    $payload = $content | ConvertFrom-Json
    if ([int]$payload.count -ne $ExpectedCount) {
        throw "$Label expected image count $ExpectedCount, got $($payload.count)."
    }
}

function Assert-BinaryEqual {
    param(
        [string]$ExpectedPath,
        [string]$ActualPath,
        [string]$Label
    )

    $expectedBytes = [System.IO.File]::ReadAllBytes($ExpectedPath)
    $actualBytes = [System.IO.File]::ReadAllBytes($ActualPath)
    if ($expectedBytes.Length -ne $actualBytes.Length) {
        throw "$Label byte length mismatch."
    }
    for ($index = 0; $index -lt $expectedBytes.Length; $index++) {
        if ($expectedBytes[$index] -ne $actualBytes[$index]) {
            throw "$Label bytes differ at offset $index."
        }
    }
}

function Write-PngFixture {
    param(
        [string]$Path,
        [string]$Base64
    )

    [System.IO.File]::WriteAllBytes($Path, [System.Convert]::FromBase64String($Base64))
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

    Write-Step "Building featherdoc_cli and append-image sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_append_image_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_append_image_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$sharedBaselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$inlineFixtureImagePath = Join-Path $resolvedOutputDir "visible-red-pixel.png"
$floatingFixtureImagePath = Join-Path $resolvedOutputDir "visible-orange-pixel.png"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($sharedBaselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared append-image baseline sample."

Write-PngFixture `
    -Path $inlineFixtureImagePath `
    -Base64 "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGP4z8AAAAMBAQDJ/pLvAAAAAElFTkSuQmCC"
Write-PngFixture `
    -Path $floatingFixtureImagePath `
    -Base64 "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGP438AAAAQBAYDFKhhdAAAAAElFTkSuQmCC"

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
$sharedBaselineFirstPage = Join-Path $sharedBaselineVisualDir "evidence\pages\page-01.png"

$cases = @(
    [ordered]@{
        id = "body-append-inline-image-visual"
        image_path = $inlineFixtureImagePath
        mutation_args = @(
            "--width", "144",
            "--height", "48"
        )
        inspect_image_args = @()
        extract_args = @("--image", "0")
        expected_json = @(
            '"command":"append-image"',
            '"part":"body"',
            '"floating":false',
            '"placement":"inline"',
            '"width_px":144',
            '"height_px":48',
            '"content_type":"image/png"'
        )
        expected_image_json = @(
            '"part":"body"',
            '"count":1',
            '"placement":"inline"',
            '"content_type":"image/png"',
            '"width_px":144',
            '"height_px":48',
            '"floating_options":null'
        )
        expected_extract_json = @(
            '"command":"extract-image"',
            '"part":"body"',
            '"placement":"inline"',
            '"content_type":"image/png"'
        )
        expected_body_text = @(
            "text=Append image visual regression baseline.",
            "text=The body text stays fixed so appended inline and header floating image cases are easy to compare against the same baseline.",
            "text=Body inline case: a red image should appear below this retained paragraph block after the append-image command runs.",
            "text=Header floating case: the baseline starts with no header content, and the mutated render should show an orange badge near the top-right margin.",
            "text=Review note: appended image mutations should not reorder the existing body paragraphs."
        )
        expected_visual_cues = @(
            "A visible red inline image appears below the retained body text.",
            "The existing body paragraphs keep their order above the appended image.",
            "No header content is introduced in the inline body case."
        )
    },
    [ordered]@{
        id = "section-header-append-floating-image-visual"
        image_path = $floatingFixtureImagePath
        mutation_args = @(
            "--part", "section-header",
            "--section", "0",
            "--floating",
            "--width", "168",
            "--height", "72",
            "--horizontal-reference", "page",
            "--horizontal-offset", "300",
            "--vertical-reference", "margin",
            "--vertical-offset", "0",
            "--behind-text", "false",
            "--allow-overlap", "false",
            "--wrap-mode", "square",
            "--wrap-distance-left", "12",
            "--wrap-distance-right", "12"
        )
        inspect_image_args = @(
            "--part", "section-header",
            "--section", "0"
        )
        extract_args = @(
            "--part", "section-header",
            "--section", "0",
            "--image", "0"
        )
        related_parts_command = "inspect-header-parts"
        related_parts_expected = @(
            '"part":"header"',
            '"count":1',
            '"entry":"word/header1.xml"'
        )
        expected_json = @(
            '"command":"append-image"',
            '"part":"section-header"',
            '"section":0',
            '"kind":"default"',
            '"floating":true',
            '"placement":"anchored"',
            '"width_px":168',
            '"height_px":72',
            '"horizontal_reference":"page"',
            '"horizontal_offset_px":300',
            '"vertical_reference":"margin"',
            '"vertical_offset_px":0',
            '"behind_text":false',
            '"allow_overlap":false',
            '"wrap_mode":"square"'
        )
        expected_image_json = @(
            '"part":"section-header"',
            '"count":1',
            '"placement":"anchored"',
            '"content_type":"image/png"',
            '"width_px":168',
            '"height_px":72',
            '"floating_options":{"horizontal_reference":"page"',
            '"horizontal_offset_px":300',
            '"vertical_reference":"margin"',
            '"vertical_offset_px":0',
            '"behind_text":false',
            '"allow_overlap":false',
            '"wrap_mode":"square"',
            '"wrap_distance_left_px":12',
            '"wrap_distance_right_px":12',
            '"crop":null'
        )
        expected_extract_json = @(
            '"command":"extract-image"',
            '"part":"section-header"',
            '"placement":"anchored"',
            '"content_type":"image/png"'
        )
        expected_body_text = @(
            "text=Append image visual regression baseline.",
            "text=The body text stays fixed so appended inline and header floating image cases are easy to compare against the same baseline.",
            "text=Body inline case: a red image should appear below this retained paragraph block after the append-image command runs.",
            "text=Header floating case: the baseline starts with no header content, and the mutated render should show an orange badge near the top-right margin.",
            "text=Review note: appended image mutations should not reorder the existing body paragraphs."
        )
        expected_visual_cues = @(
            "The baseline page header is materialized and shows an orange floating badge near the top-right margin.",
            "The body text remains unchanged while the new header image appears above it.",
            "The header floating image preserves the configured anchored layout options."
        )
    }
)

$summaryCases = @()
$aggregateImages = @()
$aggregateLabels = @()

foreach ($case in $cases) {
    $caseId = $case.id
    Write-Step "Running case '$caseId'"

    $caseOutputDir = Join-Path $resolvedOutputDir $caseId
    New-Item -ItemType Directory -Path $caseOutputDir -Force | Out-Null

    $mutatedDocxPath = Join-Path $caseOutputDir "$caseId-mutated.docx"
    $mutationJsonPath = Join-Path $caseOutputDir "mutation_result.json"
    $paragraphInspectPath = Join-Path $caseOutputDir "inspect_paragraphs.txt"
    $imageInspectPath = Join-Path $caseOutputDir "inspect_images.json"
    $extractedImagePath = Join-Path $caseOutputDir "extracted-image.png"
    $extractJsonPath = Join-Path $caseOutputDir "extract_result.json"
    $relatedPartsPath = $null

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments (@(
                "append-image",
                $sharedBaselineDocxPath,
                $case.image_path
            ) + @($case.mutation_args) + @(
                "--output",
                $mutatedDocxPath,
                "--json"
            )) `
        -OutputPath $mutationJsonPath `
        -FailureMessage "Failed to append image for case '$caseId'."

    foreach ($expected in $case.expected_json) {
        Assert-Contains -Path $mutationJsonPath -Expected $expected -Label $caseId
    }

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @(
            "inspect-template-paragraphs",
            $mutatedDocxPath
        ) `
        -OutputPath $paragraphInspectPath `
        -FailureMessage "Failed to inspect body paragraphs for case '$caseId'."

    Assert-Contains -Path $paragraphInspectPath -Expected "part: body" -Label $caseId
    foreach ($expected in $case.expected_body_text) {
        Assert-Contains -Path $paragraphInspectPath -Expected $expected -Label $caseId
    }

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments (@(
                "inspect-images",
                $mutatedDocxPath
            ) + @($case.inspect_image_args) + @(
                "--json"
            )) `
        -OutputPath $imageInspectPath `
        -FailureMessage "Failed to inspect images for case '$caseId'."

    Assert-ImageState `
        -JsonPath $imageInspectPath `
        -ExpectedCount 1 `
        -ExpectedFields $case.expected_image_json `
        -Label $caseId

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments (@(
                "extract-image",
                $mutatedDocxPath,
                $extractedImagePath
            ) + @($case.extract_args) + @(
                "--json"
            )) `
        -OutputPath $extractJsonPath `
        -FailureMessage "Failed to extract appended image for case '$caseId'."

    foreach ($expected in $case.expected_extract_json) {
        Assert-Contains -Path $extractJsonPath -Expected $expected -Label $caseId
    }
    Assert-BinaryEqual -ExpectedPath $case.image_path -ActualPath $extractedImagePath -Label $caseId

    if ($case.Contains("related_parts_command")) {
        $relatedPartsPath = Join-Path $caseOutputDir "inspect_related_parts.json"
        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @(
                $case.related_parts_command,
                $mutatedDocxPath,
                "--json"
            ) `
            -OutputPath $relatedPartsPath `
            -FailureMessage "Failed to inspect related parts for case '$caseId'."

        foreach ($expected in $case.related_parts_expected) {
            Assert-Contains -Path $relatedPartsPath -Expected $expected -Label $caseId
        }
    }

    $visualOutputDir = Join-Path $caseOutputDir "mutated-visual"
    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $mutatedDocxPath `
        -OutputDir $visualOutputDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent

    $baselinePagePath = Join-Path $aggregatePagesDir "$caseId-baseline-page-01.png"
    $mutatedPagePath = Join-Path $aggregatePagesDir "$caseId-mutated-page-01.png"
    Copy-Item -Path $sharedBaselineFirstPage -Destination $baselinePagePath -Force
    Copy-Item -Path (Join-Path $visualOutputDir "evidence\pages\page-01.png") -Destination $mutatedPagePath -Force

    $caseContactSheetPath = Join-Path $caseOutputDir "before_after_contact_sheet.png"
    Build-ContactSheet `
        -Python $renderPython `
        -ScriptPath $contactSheetScript `
        -OutputPath $caseContactSheetPath `
        -Images @($baselinePagePath, $mutatedPagePath) `
        -Labels @("$caseId baseline", "$caseId mutated")

    $aggregateImages += $baselinePagePath
    $aggregateImages += $mutatedPagePath
    $aggregateLabels += "$caseId baseline"
    $aggregateLabels += "$caseId mutated"

    $caseSummary = [ordered]@{
        id = $caseId
        command = "append-image"
        mutated_docx = $mutatedDocxPath
        mutation_json = $mutationJsonPath
        paragraph_inspect = $paragraphInspectPath
        image_inspect = $imageInspectPath
        extract_json = $extractJsonPath
        extracted_image = $extractedImagePath
        expected_visual_cues = $case.expected_visual_cues
        visual_artifacts = [ordered]@{
            before_after_contact_sheet = $caseContactSheetPath
            baseline_page = $baselinePagePath
            mutated_page = $mutatedPagePath
        }
    }
    if ($null -ne $relatedPartsPath) {
        $caseSummary["related_parts_inspect"] = $relatedPartsPath
    }
    $summaryCases += $caseSummary
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
    fixtures = [ordered]@{
        body_inline = $inlineFixtureImagePath
        header_floating = $floatingFixtureImagePath
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

Write-Step "Completed append-image visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
