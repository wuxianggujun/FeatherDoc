param(
    [string]$BuildDir = "build-image-mutate-visual-nmake",
    [string]$OutputDir = "output/replace-remove-image-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[replace-remove-image-visual] $Message"
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

function Write-Base64Fixture {
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

    Write-Step "Building featherdoc_cli and floating image sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_floating_images"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_floating_images"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$sharedBaselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$replacementImagePath = Join-Path $resolvedOutputDir "replacement-red.png"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($sharedBaselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared replace/remove image baseline sample."

Write-Base64Fixture `
    -Path $replacementImagePath `
    -Base64 "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGP4z8AAAAMBAQDJ/pLvAAAAAElFTkSuQmCC"

$floatingFixturePath = Join-Path $resolvedOutputDir "sample_floating_image.bmp"
$inlineFixturePath = Join-Path $resolvedOutputDir "sample_inline_image.bmp"
if (-not (Test-Path $floatingFixturePath) -or -not (Test-Path $inlineFixturePath)) {
    throw "Expected sample_floating_images assets were not created next to the shared baseline."
}

$baselineInspectJsonPath = Join-Path $resolvedOutputDir "shared-baseline-images.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "inspect-images",
        $sharedBaselineDocxPath,
        "--json"
    ) `
    -OutputPath $baselineInspectJsonPath `
    -FailureMessage "Failed to inspect the shared image baseline."

Assert-ImageState `
    -JsonPath $baselineInspectJsonPath `
    -ExpectedCount 2 `
    -ExpectedFields @(
        '"part":"body"',
        '"count":2',
        '"placement":"anchored"',
        '"placement":"inline"'
    ) `
    -Label "shared-baseline"

$baselineImagesPayload = Get-Content -Raw -LiteralPath $baselineInspectJsonPath | ConvertFrom-Json
$anchoredImage = $baselineImagesPayload.images | Where-Object { $_.placement -eq "anchored" } | Select-Object -First 1
$inlineImage = $baselineImagesPayload.images | Where-Object { $_.placement -eq "inline" } | Select-Object -First 1
if ($null -eq $anchoredImage -or $null -eq $inlineImage) {
    throw "Expected one anchored image and one inline image in the shared baseline."
}

$sharedBaselineParagraphPath = Join-Path $resolvedOutputDir "shared-baseline-paragraphs.txt"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "inspect-template-paragraphs",
        $sharedBaselineDocxPath
    ) `
    -OutputPath $sharedBaselineParagraphPath `
    -FailureMessage "Failed to inspect shared baseline paragraphs."

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

$commonBodyText = @(
    "text=Floating image sample",
    "text=Expected render: cropped orange floating image near the upper-right.",
    "text=The blue inline image remains in normal flow below.",
    "text=This document exercises append_floating_image(...) and append_image(...) in the same body."
)

$cases = @(
    [ordered]@{
        id = "body-replace-anchored-image-visual"
        command = "replace-image"
        mutation_args = @(
            "replace-image",
            $sharedBaselineDocxPath,
            $replacementImagePath,
            "--relationship-id",
            $anchoredImage.relationship_id
        )
        mutation_expected = @(
            '"command":"replace-image"',
            '"part":"body"',
            ('"replacement_path":"' + $replacementImagePath.Replace('\', '\\') + '"'),
            ('"filters":{"relationship_id":"' + $anchoredImage.relationship_id + '"}'),
            '"placement":"anchored"',
            '"content_type":"image/png"',
            '"width_px":144',
            '"height_px":48'
        )
        inspect_expected = @(
            '"part":"body"',
            '"count":2',
            '"placement":"anchored"',
            '"content_type":"image/png"',
            '"entry_name":"word/media/image1.png"',
            ('"relationship_id":"' + $anchoredImage.relationship_id + '"'),
            '"placement":"inline"',
            '"content_type":"image/bmp"'
        )
        extract_args = @(
            "extract-image",
            "<mutated.docx>",
            "<extracted>",
            "--relationship-id",
            $anchoredImage.relationship_id,
            "--json"
        )
        extract_expected = @(
            '"command":"extract-image"',
            '"part":"body"',
            '"placement":"anchored"',
            '"content_type":"image/png"'
        )
        expected_binary = $replacementImagePath
        expected_visual_cues = @(
            "The orange anchored body image becomes a vivid red anchored image in the same position.",
            "The blue inline image remains below and keeps its size.",
            "The surrounding body text order stays unchanged."
        )
    },
    [ordered]@{
        id = "body-remove-anchored-image-visual"
        command = "remove-image"
        mutation_args = @(
            "remove-image",
            $sharedBaselineDocxPath,
            "--relationship-id",
            $anchoredImage.relationship_id
        )
        mutation_expected = @(
            '"command":"remove-image"',
            '"part":"body"',
            ('"filters":{"relationship_id":"' + $anchoredImage.relationship_id + '"}'),
            '"placement":"anchored"',
            '"content_type":"image/bmp"'
        )
        inspect_expected = @(
            '"part":"body"',
            '"count":1',
            '"placement":"inline"',
            '"content_type":"image/bmp"',
            ('"relationship_id":"' + $inlineImage.relationship_id + '"'),
            '"floating_options":null'
        )
        extract_args = @(
            "extract-image",
            "<mutated.docx>",
            "<extracted>",
            "--image",
            "0",
            "--json"
        )
        extract_expected = @(
            '"command":"extract-image"',
            '"part":"body"',
            '"placement":"inline"',
            '"content_type":"image/bmp"'
        )
        expected_binary = $inlineFixturePath
        expected_visual_cues = @(
            "The orange anchored image disappears completely.",
            "The blue inline image remains in normal flow below the text.",
            "The retained body paragraphs keep their order and spacing."
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
    $extractJsonPath = Join-Path $caseOutputDir "extract_result.json"
    $extractedImagePath = Join-Path $caseOutputDir "extracted-image"

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments ($case.mutation_args + @(
                "--output",
                $mutatedDocxPath,
                "--json"
            )) `
        -OutputPath $mutationJsonPath `
        -FailureMessage "Failed to run $($case.command) for case '$caseId'."

    foreach ($expected in $case.mutation_expected) {
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
    foreach ($expected in $commonBodyText) {
        Assert-Contains -Path $paragraphInspectPath -Expected $expected -Label $caseId
    }

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @(
            "inspect-images",
            $mutatedDocxPath,
            "--json"
        ) `
        -OutputPath $imageInspectPath `
        -FailureMessage "Failed to inspect images for case '$caseId'."

    if ($case.command -eq "replace-image") {
        Assert-ImageState `
            -JsonPath $imageInspectPath `
            -ExpectedCount 2 `
            -ExpectedFields $case.inspect_expected `
            -Label $caseId
    } else {
        Assert-ImageState `
            -JsonPath $imageInspectPath `
            -ExpectedCount 1 `
            -ExpectedFields $case.inspect_expected `
            -Label $caseId
    }

    $extractExecutableArgs = @()
    foreach ($argument in $case.extract_args) {
        if ($argument -eq "<mutated.docx>") {
            $extractExecutableArgs += $mutatedDocxPath
        } elseif ($argument -eq "<extracted>") {
            $extractExecutableArgs += $extractedImagePath
        } else {
            $extractExecutableArgs += $argument
        }
    }

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments $extractExecutableArgs `
        -OutputPath $extractJsonPath `
        -FailureMessage "Failed to extract an image for case '$caseId'."

    foreach ($expected in $case.extract_expected) {
        Assert-Contains -Path $extractJsonPath -Expected $expected -Label $caseId
    }
    Assert-BinaryEqual -ExpectedPath $case.expected_binary -ActualPath $extractedImagePath -Label $caseId

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

    $summaryCases += [ordered]@{
        id = $caseId
        command = $case.command
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
    shared_baseline_images = $baselineInspectJsonPath
    shared_baseline_paragraphs = $sharedBaselineParagraphPath
    fixtures = [ordered]@{
        floating_bmp = $floatingFixturePath
        inline_bmp = $inlineFixturePath
        replacement_png = $replacementImagePath
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

Write-Step "Completed replace/remove image visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
