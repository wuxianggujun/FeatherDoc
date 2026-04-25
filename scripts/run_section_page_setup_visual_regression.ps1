param(
    [string]$BuildDir = "build-section-page-setup-regression-nmake",
    [string]$OutputDir = "output/section-page-setup-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[section-page-setup-visual] $Message"
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

    $candidates = Get-ChildItem -Path $BuildRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName } |
        Sort-Object LastWriteTime -Descending

    if (-not $candidates) {
        throw "Could not find built executable for target '$TargetName' under $BuildRoot."
    }

    return $candidates[0].FullName
}

function Resolve-BuildSearchRoot {
    param(
        [string]$RepoRoot,
        [string]$PreferredBuildRoot,
        [string[]]$TargetNames
    )

    $candidateRoots = @()
    if (Test-Path $PreferredBuildRoot) {
        $candidateRoots += (Resolve-Path $PreferredBuildRoot).Path
    }

    $candidateRoots += @(Get-ChildItem -Path $RepoRoot -Directory -Filter "build*" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -ExpandProperty FullName)

    foreach ($root in ($candidateRoots | Select-Object -Unique)) {
        $hasAllTargets = $true
        foreach ($targetName in $TargetNames) {
            $target = Get-ChildItem -Path $root -Recurse -File -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -ieq "$targetName.exe" -or $_.Name -ieq $targetName } |
                Select-Object -First 1
            if (-not $target) {
                $hasAllTargets = $false
                break
            }
        }

        if ($hasAllTargets) {
            return $root
        }
    }

    throw "Could not locate a build directory containing targets: $($TargetNames -join ', ')."
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

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        & $PythonCommand -c "import $ModuleName" 1> $null 2> $null
        return $LASTEXITCODE -eq 0
    } catch {
        return $false
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
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

function Get-RenderedPagePaths {
    param(
        [string]$VisualOutputDir,
        [string]$Label
    )

    $pagesDir = Join-Path $VisualOutputDir "evidence\pages"
    $pagePaths = @(Get-ChildItem -Path $pagesDir -Filter "page-*.png" -File -ErrorAction SilentlyContinue |
        Sort-Object Name |
        Select-Object -ExpandProperty FullName)
    if ($pagePaths.Count -eq 0) {
        throw "Missing rendered pages for $Label under $pagesDir."
    }

    return ,$pagePaths
}

function Invoke-CommandCapture {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments,
        [string]$OutputPath,
        [string]$FailureMessage
    )

    $commandOutput = @(& $ExecutablePath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
        $lines | Set-Content -Path $OutputPath -Encoding UTF8
    }
    foreach ($line in $lines) {
        Write-Host $line
    }
    if ($exitCode -ne 0) {
        throw $FailureMessage
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"
$buildSearchRoot = Resolve-BuildSearchRoot `
    -RepoRoot $repoRoot `
    -PreferredBuildRoot $resolvedBuildDir `
    -TargetNames @("featherdoc_sample_section_page_setup", "featherdoc_cli")

if ($buildSearchRoot -ne $resolvedBuildDir) {
    Write-Step "Using existing build directory $buildSearchRoot"
}

$cases = @(
    [ordered]@{
        id = "api-sample"
        description = "Sample output keeps page 1 portrait and page 2 landscape."
        expected_visual_cues = @(
            "Page 1 stays portrait with wider left and right margins around the opening copy.",
            "Page 2 rotates to landscape with a visibly wider canvas and tighter margins.",
            "Both pages remain readable without clipped text or unexpected pagination drift."
        )
    },
    [ordered]@{
        id = "cli-rewrite"
        description = "CLI rewrite rotates section 0 so both pages render in landscape."
        expected_visual_cues = @(
            "Page 1 switches from portrait to landscape after the CLI rewrite.",
            "Page 1 and page 2 now share the same 15840x12240 twip landscape geometry.",
            "The rendered PDF still has two pages with no clipped text or broken margins."
        )
    }
)

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateContactSheetsDir = Join-Path $aggregateEvidenceDir "contact-sheets"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "contact_sheet.png"
$aggregateSelectedPagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateBeforeAfterContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
$reviewManifestPath = Join-Path $resolvedOutputDir "review_manifest.json"
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"

if (-not $SkipVisual) {
    $renderPython = Ensure-RenderPython -RepoRoot $repoRoot
    New-Item -ItemType Directory -Path $aggregateEvidenceDir -Force | Out-Null
    New-Item -ItemType Directory -Path $aggregateContactSheetsDir -Force | Out-Null
    New-Item -ItemType Directory -Path $aggregateSelectedPagesDir -Force | Out-Null
}

if (-not $SkipBuild) {
    $vcvarsPath = Get-VcvarsPath
    Write-Step "Configuring dedicated build directory $resolvedBuildDir"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"

    Write-Step "Building section page setup regression targets"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_sample_section_page_setup featherdoc_cli"
}

$sampleExecutable = Find-BuildExecutable -BuildRoot $buildSearchRoot -TargetName "featherdoc_sample_section_page_setup"
$cliExecutable = Find-BuildExecutable -BuildRoot $buildSearchRoot -TargetName "featherdoc_cli"

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    build_dir = $buildSearchRoot
    requested_build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    review_manifest = $reviewManifestPath
    review_checklist = $reviewChecklistPath
    final_review = $finalReviewPath
    cases = @()
}

$reviewManifest = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    aggregate_evidence = if ($SkipVisual) { $null } else { [ordered]@{
        root = $aggregateEvidenceDir
        contact_sheets_dir = $aggregateContactSheetsDir
        contact_sheet = $aggregateContactSheetPath
    } }
    cases = @()
}

$apiCaseDir = Join-Path $resolvedOutputDir "api-sample"
$apiDocxPath = Join-Path $apiCaseDir "section_page_setup.docx"
$apiInspectPath = Join-Path $apiCaseDir "inspect_page_setup.json"
$apiVisualDir = Join-Path $apiCaseDir "visual"
$apiVisualRelativeDir = Join-Path $OutputDir (Join-Path "api-sample" "visual")

New-Item -ItemType Directory -Path $apiCaseDir -Force | Out-Null

Write-Step "Generating API sample document via $sampleExecutable"
& $sampleExecutable $apiDocxPath
if ($LASTEXITCODE -ne 0) {
    throw "Sample execution failed: featherdoc_sample_section_page_setup"
}

Write-Step "Inspecting API sample page setup through featherdoc_cli"
Invoke-CommandCapture `
    -ExecutablePath $cliExecutable `
    -Arguments @("inspect-page-setup", $apiDocxPath, "--json") `
    -OutputPath $apiInspectPath `
    -FailureMessage "Failed to inspect API sample page setup."

$apiCaseSummary = [ordered]@{
    id = "api-sample"
    description = $cases[0].description
    expected_visual_cues = @($cases[0].expected_visual_cues)
    sample_executable = $sampleExecutable
    inspector_executable = $cliExecutable
    docx_path = $apiDocxPath
    inspection_json = $apiInspectPath
    visual_output_dir = if ($SkipVisual) { $null } else { $apiVisualDir }
}

if (-not $SkipVisual) {
    Write-Step "Rendering Word evidence for api-sample"
    & $wordSmokeScript `
        -BuildDir $BuildDir `
        -InputDocx $apiDocxPath `
        -OutputDir $apiVisualRelativeDir `
        -SkipBuild `
        -Dpi $Dpi `
        -KeepWordOpen:$KeepWordOpen.IsPresent `
        -VisibleWord:$VisibleWord.IsPresent
    if ($LASTEXITCODE -ne 0) {
        throw "Visual smoke failed for api-sample."
    }

    $apiCaseSummary.visual_artifacts = [ordered]@{
        root = $apiVisualDir
        evidence_dir = Join-Path $apiVisualDir "evidence"
        pages_dir = Join-Path $apiVisualDir "evidence\pages"
        page_01 = Join-Path $apiVisualDir "evidence\pages\page-01.png"
        page_02 = Join-Path $apiVisualDir "evidence\pages\page-02.png"
        contact_sheet = Join-Path $apiVisualDir "evidence\contact_sheet.png"
        report_dir = Join-Path $apiVisualDir "report"
        summary_json = Join-Path $apiVisualDir "report\summary.json"
        review_result = Join-Path $apiVisualDir "report\review_result.json"
        final_review = Join-Path $apiVisualDir "report\final_review.md"
    }

    $apiRenderedPages = Get-RenderedPagePaths -VisualOutputDir $apiVisualDir -Label "api-sample"
    $apiCaseBeforeAfterContactSheetPath = Join-Path $apiCaseDir "before_after_contact_sheet.png"
    Build-ContactSheet `
        -Python $renderPython `
        -ScriptPath $contactSheetScript `
        -OutputPath $apiCaseBeforeAfterContactSheetPath `
        -Images $apiRenderedPages `
        -Labels (@($apiRenderedPages | ForEach-Object { "api-sample $([System.IO.Path]::GetFileNameWithoutExtension($_))" }))

    $apiCaseSummary.visual_artifacts.rendered_pages = $apiRenderedPages
    $apiCaseSummary.visual_artifacts.before_after_contact_sheet = $apiCaseBeforeAfterContactSheetPath
}

$summary.cases += $apiCaseSummary
$reviewManifest.cases += $apiCaseSummary

$cliCaseDir = Join-Path $resolvedOutputDir "cli-rewrite"
$cliDocxPath = Join-Path $cliCaseDir "section_page_setup_cli.docx"
$cliSetResultPath = Join-Path $cliCaseDir "set_section_page_setup.json"
$cliInspectPath = Join-Path $cliCaseDir "inspect_page_setup.json"
$cliVisualDir = Join-Path $cliCaseDir "visual"
$cliVisualRelativeDir = Join-Path $OutputDir (Join-Path "cli-rewrite" "visual")

New-Item -ItemType Directory -Path $cliCaseDir -Force | Out-Null

Write-Step "Rewriting section 0 through featherdoc_cli"
Invoke-CommandCapture `
    -ExecutablePath $cliExecutable `
    -Arguments @(
        "set-section-page-setup",
        $apiDocxPath,
        "0",
        "--orientation",
        "landscape",
        "--width",
        "15840",
        "--height",
        "12240",
        "--margin-top",
        "720",
        "--margin-bottom",
        "1080",
        "--margin-left",
        "1440",
        "--margin-right",
        "1440",
        "--margin-header",
        "360",
        "--margin-footer",
        "540",
        "--clear-page-number-start",
        "--output",
        $cliDocxPath,
        "--json"
    ) `
    -OutputPath $cliSetResultPath `
    -FailureMessage "Failed to rewrite section page setup through featherdoc_cli."

Write-Step "Inspecting CLI-rewritten page setup through featherdoc_cli"
Invoke-CommandCapture `
    -ExecutablePath $cliExecutable `
    -Arguments @("inspect-page-setup", $cliDocxPath, "--json") `
    -OutputPath $cliInspectPath `
    -FailureMessage "Failed to inspect CLI-rewritten page setup."

$cliCaseSummary = [ordered]@{
    id = "cli-rewrite"
    description = $cases[1].description
    expected_visual_cues = @($cases[1].expected_visual_cues)
    cli_executable = $cliExecutable
    input_docx_path = $apiDocxPath
    docx_path = $cliDocxPath
    set_result_json = $cliSetResultPath
    inspection_json = $cliInspectPath
    visual_output_dir = if ($SkipVisual) { $null } else { $cliVisualDir }
}

if (-not $SkipVisual) {
    Write-Step "Rendering Word evidence for cli-rewrite"
    & $wordSmokeScript `
        -BuildDir $BuildDir `
        -InputDocx $cliDocxPath `
        -OutputDir $cliVisualRelativeDir `
        -SkipBuild `
        -Dpi $Dpi `
        -KeepWordOpen:$KeepWordOpen.IsPresent `
        -VisibleWord:$VisibleWord.IsPresent
    if ($LASTEXITCODE -ne 0) {
        throw "Visual smoke failed for cli-rewrite."
    }

    $cliCaseSummary.visual_artifacts = [ordered]@{
        root = $cliVisualDir
        evidence_dir = Join-Path $cliVisualDir "evidence"
        pages_dir = Join-Path $cliVisualDir "evidence\pages"
        page_01 = Join-Path $cliVisualDir "evidence\pages\page-01.png"
        page_02 = Join-Path $cliVisualDir "evidence\pages\page-02.png"
        contact_sheet = Join-Path $cliVisualDir "evidence\contact_sheet.png"
        report_dir = Join-Path $cliVisualDir "report"
        summary_json = Join-Path $cliVisualDir "report\summary.json"
        review_result = Join-Path $cliVisualDir "report\review_result.json"
        final_review = Join-Path $cliVisualDir "report\final_review.md"
    }

    $cliRenderedPages = Get-RenderedPagePaths -VisualOutputDir $cliVisualDir -Label "cli-rewrite"
    $cliCaseBeforeAfterContactSheetPath = Join-Path $cliCaseDir "before_after_contact_sheet.png"
    Build-ContactSheet `
        -Python $renderPython `
        -ScriptPath $contactSheetScript `
        -OutputPath $cliCaseBeforeAfterContactSheetPath `
        -Images $cliRenderedPages `
        -Labels (@($cliRenderedPages | ForEach-Object { "cli-rewrite $([System.IO.Path]::GetFileNameWithoutExtension($_))" }))

    $cliCaseSummary.visual_artifacts.rendered_pages = $cliRenderedPages
    $cliCaseSummary.visual_artifacts.before_after_contact_sheet = $cliCaseBeforeAfterContactSheetPath
}

$summary.cases += $cliCaseSummary
$reviewManifest.cases += $cliCaseSummary

if (-not $SkipVisual) {
    $contactSheetPaths = @()
    $contactSheetLabels = @()
    $selectedPagePaths = @()
    $selectedPageLabels = @()

    foreach ($caseSummary in $summary.cases) {
        $contactSheetPath = $caseSummary.visual_artifacts.before_after_contact_sheet
        if (-not (Test-Path $contactSheetPath)) {
            throw "Missing contact sheet for case '$($caseSummary.id)': $contactSheetPath"
        }

        $aggregateCaseContactSheetPath = Join-Path $aggregateContactSheetsDir "$($caseSummary.id)-contact_sheet.png"
        Copy-Item -Path $contactSheetPath -Destination $aggregateCaseContactSheetPath -Force
        $contactSheetPaths += $aggregateCaseContactSheetPath
        $contactSheetLabels += $caseSummary.id

        foreach ($pagePath in $caseSummary.visual_artifacts.rendered_pages) {
            $pageStem = [System.IO.Path]::GetFileNameWithoutExtension($pagePath)
            $aggregateSelectedPagePath = Join-Path $aggregateSelectedPagesDir "$($caseSummary.id)-$pageStem.png"
            Copy-Item -Path $pagePath -Destination $aggregateSelectedPagePath -Force
            $selectedPagePaths += $aggregateSelectedPagePath
            $selectedPageLabels += "$($caseSummary.id) $pageStem"
        }
    }

    Build-ContactSheet `
        -Python $renderPython `
        -ScriptPath $contactSheetScript `
        -OutputPath $aggregateContactSheetPath `
        -Images $contactSheetPaths `
        -Labels $contactSheetLabels
    Build-ContactSheet `
        -Python $renderPython `
        -ScriptPath $contactSheetScript `
        -OutputPath $aggregateBeforeAfterContactSheetPath `
        -Images $selectedPagePaths `
        -Labels $selectedPageLabels

    $summary.aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        contact_sheets_dir = $aggregateContactSheetsDir
        contact_sheet = $aggregateContactSheetPath
        selected_pages_dir = $aggregateSelectedPagesDir
        before_after_contact_sheet = $aggregateBeforeAfterContactSheetPath
    }
    $reviewManifest.aggregate_evidence = $summary.aggregate_evidence
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 6) | Set-Content -Path $summaryPath -Encoding UTF8
($reviewManifest | ConvertTo-Json -Depth 6) | Set-Content -Path $reviewManifestPath -Encoding UTF8

$reviewChecklist = @"
# Section page setup visual review checklist

- Start with the aggregate before/after contact sheet when available:
  $aggregateBeforeAfterContactSheetPath
- Legacy case contact sheets remain available under:
  $aggregateContactSheetsDir
- Then inspect each case's before_after_contact_sheet.png, page-01.png, and page-02.png.

- api-sample: page 1 must remain portrait while page 2 is landscape.
- cli-rewrite: page 1 and page 2 must both render as landscape after the CLI rewrite.
- In both cases, text should remain readable with no clipped paragraphs, broken pagination, or unexpected margin drift.

Artifacts:

- Summary JSON: $summaryPath
- Review manifest: $reviewManifestPath
- Final review: $finalReviewPath
"@
$reviewChecklist | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReview = @"
# Section page setup final review

- Generated at: $(Get-Date -Format s)
- Summary JSON: $summaryPath
- Review manifest: $reviewManifestPath
- Aggregate before/after contact sheet: $aggregateBeforeAfterContactSheetPath
- Legacy aggregate contact sheet: $aggregateContactSheetPath

## Verdict

- Overall verdict:
- Notes:

## Case findings

- api-sample:
  Verdict:
  Notes:

- cli-rewrite:
  Verdict:
  Notes:
"@
$finalReview | Set-Content -Path $finalReviewPath -Encoding UTF8

Write-Step "Completed section page setup regression run"
Write-Host "Summary: $summaryPath"
Write-Host "Review manifest: $reviewManifestPath"
Write-Host "Review checklist: $reviewChecklistPath"
if (-not $SkipVisual) {
    Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
}
foreach ($case in $summary.cases) {
    Write-Host "Case: $($case.id)"
    Write-Host "  DOCX: $($case.docx_path)"
    Write-Host "  Inspection: $($case.inspection_json)"
    if ($case.visual_output_dir) {
        Write-Host "  Visual: $($case.visual_output_dir)"
    }
}
