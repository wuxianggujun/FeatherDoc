param(
    [string]$BuildDir = "build-floating-image-z-order-visual-nmake",
    [string]$OutputDir = "output/floating-image-z-order-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[floating-image-z-order-visual] $Message"
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

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path $Path)) {
        throw "Expected $Label was not found: $Path"
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

    Write-Step "Building featherdoc_cli and floating-image z-order sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_floating_image_z_order_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_floating_image_z_order_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$sampleDocxPath = Join-Path $resolvedOutputDir "floating-image-z-order.docx"
$sampleLogPath = Join-Path $resolvedOutputDir "sample.log"
$backImagePath = Join-Path $resolvedOutputDir "floating_z_order_back.bmp"
$frontImagePath = Join-Path $resolvedOutputDir "floating_z_order_front.bmp"
$paragraphInspectPath = Join-Path $resolvedOutputDir "inspect_paragraphs.txt"
$imageInspectPath = Join-Path $resolvedOutputDir "inspect_images.json"
$extractBackPath = Join-Path $resolvedOutputDir "extracted-back.bmp"
$extractFrontPath = Join-Path $resolvedOutputDir "extracted-front.bmp"
$extractBackJsonPath = Join-Path $resolvedOutputDir "extract_back_result.json"
$extractFrontJsonPath = Join-Path $resolvedOutputDir "extract_front_result.json"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($sampleDocxPath) `
    -OutputPath $sampleLogPath `
    -FailureMessage "Failed to generate floating image z-order sample."

Assert-PathExists -Path $sampleDocxPath -Label "floating image z-order sample document"
Assert-PathExists -Path $backImagePath -Label "back image asset"
Assert-PathExists -Path $frontImagePath -Label "front image asset"

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "inspect-template-paragraphs",
        $sampleDocxPath
    ) `
    -OutputPath $paragraphInspectPath `
    -FailureMessage "Failed to inspect template paragraphs for the z-order sample."

foreach ($expected in @(
        "part: body",
        "text=Floating image z-order visual sample",
        "text=Expected render: the orange floating image should overlap and appear above the blue one.",
        "text=Both images use the same margin-based anchor family, but the orange image has a higher z-order.",
        "text=This body text stays in place so the overlap can be inspected directly in Word."
    )) {
    Assert-Contains -Path $paragraphInspectPath -Expected $expected -Label "floating-image-z-order"
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "inspect-images",
        $sampleDocxPath,
        "--json"
    ) `
    -OutputPath $imageInspectPath `
    -FailureMessage "Failed to inspect images for the z-order sample."

foreach ($expected in @(
        '"count":2',
        '"part":"body"',
        '"placement":"anchored"',
        '"content_type":"image/bmp"',
        '"width_px":180',
        '"height_px":120',
        '"horizontal_reference":"margin"',
        '"horizontal_offset_px":96',
        '"horizontal_offset_px":148',
        '"vertical_reference":"margin"',
        '"vertical_offset_px":120',
        '"vertical_offset_px":162',
        '"allow_overlap":true',
        '"z_order":16',
        '"z_order":64'
    )) {
    Assert-Contains -Path $imageInspectPath -Expected $expected -Label "floating-image-z-order"
}

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "extract-image",
        $sampleDocxPath,
        $extractBackPath,
        "--image",
        "0",
        "--json"
    ) `
    -OutputPath $extractBackJsonPath `
    -FailureMessage "Failed to extract the back floating image."

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @(
        "extract-image",
        $sampleDocxPath,
        $extractFrontPath,
        "--image",
        "1",
        "--json"
    ) `
    -OutputPath $extractFrontJsonPath `
    -FailureMessage "Failed to extract the front floating image."

foreach ($path in @($extractBackJsonPath, $extractFrontJsonPath)) {
    foreach ($expected in @(
            '"command":"extract-image"',
            '"part":"body"',
            '"placement":"anchored"',
            '"content_type":"image/bmp"'
        )) {
        Assert-Contains -Path $path -Expected $expected -Label "floating-image-z-order extract"
    }
}

Assert-BinaryEqual -ExpectedPath $backImagePath -ActualPath $extractBackPath -Label "back image extraction"
Assert-BinaryEqual -ExpectedPath $frontImagePath -ActualPath $extractFrontPath -Label "front image extraction"

$visualOutputDir = Join-Path $resolvedOutputDir "visual"
Write-Step "Rendering Word evidence for the z-order sample"
Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $sampleDocxPath `
    -OutputDir $visualOutputDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$renderedPagePath = Join-Path $visualOutputDir "evidence\pages\page-01.png"
Assert-PathExists -Path $renderedPagePath -Label "rendered first page"
$selectedPagePath = Join-Path $aggregatePagesDir "floating-image-z-order-page-01.png"
Copy-Item -Path $renderedPagePath -Destination $selectedPagePath -Force

Build-ContactSheet `
    -Python $renderPython `
    -ScriptPath $contactSheetScript `
    -OutputPath $aggregateContactSheetPath `
    -Images @($selectedPagePath) `
    -Labels @("floating-image-z-order overlap")

$expectedVisualCues = @(
    "The orange floating image overlaps the blue floating image on the first page.",
    "The orange floating image appears above the blue floating image instead of hiding behind it.",
    "The heading and retained explanatory body text stay readable while the overlap remains visible."
)

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = $true
    sample_docx = $sampleDocxPath
    sample_log = $sampleLogPath
    paragraph_inspect = $paragraphInspectPath
    image_inspect = $imageInspectPath
    extract_results = [ordered]@{
        back = [ordered]@{
            extract_json = $extractBackJsonPath
            extracted_image = $extractBackPath
            source_image = $backImagePath
        }
        front = [ordered]@{
            extract_json = $extractFrontJsonPath
            extracted_image = $extractFrontPath
            source_image = $frontImagePath
        }
    }
    cases = @(
        [ordered]@{
            id = "floating-image-z-order-overlap"
            command = "append_floating_image sample"
            document = $sampleDocxPath
            expected_visual_cues = @($expectedVisualCues)
            visual_artifacts = [ordered]@{
                visual_output_dir = $visualOutputDir
                rendered_pages = @($renderedPagePath)
                selected_pages = @($selectedPagePath)
            }
        }
    )
    aggregate_evidence = [ordered]@{
        selected_pages = @($selectedPagePath)
        aggregate_contact_sheet = $aggregateContactSheetPath
    }
}

$reviewManifest = [ordered]@{
    generated_at = $summary.generated_at
    output_dir = $resolvedOutputDir
    visual_enabled = $true
    review_focus = @($expectedVisualCues)
    cases = $summary.cases
    aggregate_evidence = $summary.aggregate_evidence
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$reviewManifestPath = Join-Path $resolvedOutputDir "review_manifest.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8
($reviewManifest | ConvertTo-Json -Depth 8) | Set-Content -Path $reviewManifestPath -Encoding UTF8

$reviewChecklistLines = @(
    "# Floating image z-order visual regression checklist",
    "",
    "- Summary: $summaryPath",
    "- Review manifest: $reviewManifestPath",
    "- Inspect the first rendered page: $selectedPagePath",
    "- Confirm the orange floating image overlaps the blue floating image.",
    "- Confirm the orange floating image appears above the blue floating image.",
    "- Confirm the heading and retained explanatory text still read cleanly around the overlap."
)
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$reviewChecklistLines | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReviewLines = @(
    "# Floating image z-order visual final review",
    "",
    "- Summary: $summaryPath",
    "- Review manifest: $reviewManifestPath",
    "- Visual enabled: true",
    "- Manual verdict: pending",
    "- Expected cues:",
    "- The orange floating image overlaps the blue floating image.",
    "- The orange floating image appears above the blue floating image.",
    "- The heading and retained explanatory text remain readable."
)
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"
$finalReviewLines | Set-Content -Path $finalReviewPath -Encoding UTF8

Write-Step "Completed floating image z-order visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Review manifest: $reviewManifestPath"
Write-Host "Checklist: $reviewChecklistPath"
Write-Host "Final review: $finalReviewPath"
