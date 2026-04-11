param(
    [string]$BuildDir = "build-fixed-grid-regression-nmake",
    [string]$OutputDir = "output/fixed-grid-merge-unmerge-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$PrepareReviewTask,
    [ValidateSet("review-only", "review-and-repair")]
    [string]$ReviewMode = "review-only",
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks",
    [switch]$OpenTaskDir,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[fixed-grid-regression] $Message"
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

function Find-SampleExecutable {
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

function Invoke-PrepareReviewTask {
    param(
        [string]$PrepareTaskScript,
        [string]$FixedGridRegressionRoot,
        [string]$Mode,
        [string]$TaskOutputRoot,
        [bool]$OpenTaskDir
    )

    $prepareTaskArgs = @(
        "-ExecutionPolicy"
        "Bypass"
        "-File"
        $PrepareTaskScript
        "-FixedGridRegressionRoot"
        $FixedGridRegressionRoot
        "-Mode"
        $Mode
        "-TaskOutputRoot"
        $TaskOutputRoot
    )
    if ($OpenTaskDir) {
        $prepareTaskArgs += "-OpenTaskDir"
    }

    $commandOutput = @(& powershell.exe @prepareTaskArgs 2>&1)
    $exitCode = $LASTEXITCODE
    foreach ($line in $commandOutput) {
        Write-Host $line
    }

    if ($exitCode -ne 0) {
        throw "Failed to prepare fixed-grid review task package."
    }

    $taskInfo = [ordered]@{
        mode = $Mode
        output_root = $TaskOutputRoot
        raw_output = @($commandOutput | ForEach-Object { $_.ToString() })
    }

    foreach ($line in $taskInfo.raw_output) {
        if ($line -match '^Task id:\s*(.+)$') {
            $taskInfo.task_id = $Matches[1].Trim()
        } elseif ($line -match '^Task directory:\s*(.+)$') {
            $taskInfo.task_dir = $Matches[1].Trim()
        } elseif ($line -match '^Prompt:\s*(.+)$') {
            $taskInfo.prompt_path = $Matches[1].Trim()
        } elseif ($line -match '^Manifest:\s*(.+)$') {
            $taskInfo.manifest_path = $Matches[1].Trim()
        } elseif ($line -match '^Review result:\s*(.+)$') {
            $taskInfo.review_result_path = $Matches[1].Trim()
        } elseif ($line -match '^Final review:\s*(.+)$') {
            $taskInfo.final_review_path = $Matches[1].Trim()
        } elseif ($line -match '^Latest task pointer:\s*(.+)$') {
            $taskInfo.latest_task_pointer_path = $Matches[1].Trim()
        } elseif ($line -match '^Latest source-kind task pointer:\s*(.+)$') {
            $taskInfo.latest_source_kind_task_pointer_path = $Matches[1].Trim()
        }
    }

    return $taskInfo
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"
$prepareTaskScript = Join-Path $repoRoot "scripts\prepare_word_review_task.ps1"
$preparedTaskInfo = $null

if ($PrepareReviewTask -and $SkipVisual) {
    throw "Cannot use -PrepareReviewTask together with -SkipVisual because the fixed-grid review task requires screenshot evidence."
}

$cases = @(
    [ordered]@{
        id = "merge-right"
        sample_name = "merge_right_fixed_grid"
        target = "featherdoc_sample_merge_right_fixed_grid"
        docx_name = "merge_right_fixed_grid.docx"
        description = "merge_right() fixed-grid width closure"
        expected_visual_cues = @(
            "The merged blue cell is visibly wider than the gray 1000-twip base cell below it.",
            "The merged blue cell remains narrower than the green 4100-twip tail column.",
            "The rightmost green tail column stays aligned and visibly widest."
        )
    },
    [ordered]@{
        id = "merge-down"
        sample_name = "merge_down_fixed_grid"
        target = "featherdoc_sample_merge_down_fixed_grid"
        docx_name = "merge_down_fixed_grid.docx"
        description = "merge_down() fixed-grid width closure"
        expected_visual_cues = @(
            "The orange first-column pillar spans both rows.",
            "The orange pillar remains the narrow 1000-twip first column.",
            "The yellow 1900 and green 4100 columns stay aligned across both rows."
        )
    },
    [ordered]@{
        id = "unmerge-right"
        sample_name = "unmerge_right_fixed_grid"
        target = "featherdoc_sample_unmerge_right_fixed_grid"
        docx_name = "unmerge_right_fixed_grid.docx"
        description = "unmerge_right() restores fixed-grid columns"
        expected_visual_cues = @(
            "The restored blue and yellow cells appear as two separate standalone cells.",
            "The blue cell is visibly narrower than the yellow cell.",
            "The green 4100-twip tail column remains clearly widest."
        )
    },
    [ordered]@{
        id = "unmerge-down"
        sample_name = "unmerge_down_fixed_grid"
        target = "featherdoc_sample_unmerge_down_fixed_grid"
        docx_name = "unmerge_down_fixed_grid.docx"
        description = "unmerge_down() restores fixed-grid rows"
        expected_visual_cues = @(
            "The orange top-left cell and blue lower-left cell appear as separate 1000-twip cells.",
            "A clean horizontal border is visible between the two restored left cells.",
            "The 1900 and 4100 columns stay unchanged after the vertical split."
        )
    }
)

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateFirstPagesDir = Join-Path $aggregateEvidenceDir "first-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "contact_sheet.png"
$reviewManifestPath = Join-Path $resolvedOutputDir "review_manifest.json"
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"

if (-not $SkipVisual) {
    New-Item -ItemType Directory -Path $aggregateEvidenceDir -Force | Out-Null
    New-Item -ItemType Directory -Path $aggregateFirstPagesDir -Force | Out-Null
}

if (-not $SkipBuild) {
    $vcvarsPath = Get-VcvarsPath
    Write-Step "Configuring dedicated build directory $resolvedBuildDir"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=OFF -DBUILD_TESTING=OFF"

    $targets = ($cases | ForEach-Object { $_.target }) -join " "
    Write-Step "Building fixed-grid regression samples"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target $targets"
}

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    build_dir = $resolvedBuildDir
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
        first_pages_dir = $aggregateFirstPagesDir
        contact_sheet = $aggregateContactSheetPath
    } }
    cases = @()
}

foreach ($case in $cases) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    $docxPath = Join-Path $caseDir $case.docx_name
    $visualRelativeDir = Join-Path $OutputDir (Join-Path $case.id "visual")
    $visualDir = Join-Path $caseDir "visual"

    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    $samplePath = Find-SampleExecutable -BuildRoot $resolvedBuildDir -TargetName $case.target
    Write-Step "Generating $($case.sample_name) via $samplePath"
    & $samplePath $docxPath
    if ($LASTEXITCODE -ne 0) {
        throw "Sample execution failed: $($case.target)"
    }

    $caseSummary = [ordered]@{
        id = $case.id
        sample_name = $case.sample_name
        target = $case.target
        description = $case.description
        expected_visual_cues = @($case.expected_visual_cues)
        sample_executable = $samplePath
        docx_path = $docxPath
        visual_output_dir = if ($SkipVisual) { $null } else { $visualDir }
    }

    if (-not $SkipVisual) {
        Write-Step "Rendering Word evidence for $($case.sample_name)"
        & $wordSmokeScript `
            -BuildDir $BuildDir `
            -InputDocx $docxPath `
            -OutputDir $visualRelativeDir `
            -SkipBuild `
            -Dpi $Dpi `
            -KeepWordOpen:$KeepWordOpen.IsPresent `
            -VisibleWord:$VisibleWord.IsPresent
        if ($LASTEXITCODE -ne 0) {
            throw "Visual smoke failed for $($case.sample_name)"
        }

        $caseSummary.visual_artifacts = [ordered]@{
            root = $visualDir
            evidence_dir = Join-Path $visualDir "evidence"
            pages_dir = Join-Path $visualDir "evidence\pages"
            first_page = Join-Path $visualDir "evidence\pages\page-01.png"
            contact_sheet = Join-Path $visualDir "evidence\contact_sheet.png"
            report_dir = Join-Path $visualDir "report"
            summary_json = Join-Path $visualDir "report\summary.json"
            checklist = Join-Path $visualDir "report\review_checklist.md"
            review_result = Join-Path $visualDir "report\review_result.json"
            final_review = Join-Path $visualDir "report\final_review.md"
        }
    }

    $summary.cases += $caseSummary
    $reviewManifest.cases += $caseSummary
}

if (-not $SkipVisual) {
    $renderPython = Ensure-RenderPython -RepoRoot $repoRoot
    $firstPagePaths = @()
    $firstPageLabels = @()

    foreach ($caseSummary in $summary.cases) {
        $firstPagePath = $caseSummary.visual_artifacts.first_page
        if (-not (Test-Path $firstPagePath)) {
            throw "Missing first-page PNG for case '$($caseSummary.id)': $firstPagePath"
        }

        $aggregateFirstPagePath = Join-Path $aggregateFirstPagesDir "$($caseSummary.id)-page-01.png"
        Copy-Item -Path $firstPagePath -Destination $aggregateFirstPagePath -Force

        $firstPagePaths += $aggregateFirstPagePath
        $firstPageLabels += $caseSummary.id
    }

    $contactSheetArgs = @(
        $contactSheetScript
        "--output"
        $aggregateContactSheetPath
        "--columns"
        "2"
        "--images"
    ) + $firstPagePaths + @("--labels") + $firstPageLabels

    & $renderPython @contactSheetArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build aggregate contact sheet."
    }

    $summary.aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        first_pages_dir = $aggregateFirstPagesDir
        contact_sheet = $aggregateContactSheetPath
    }
    $reviewManifest.aggregate_evidence = $summary.aggregate_evidence
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 6) | Set-Content -Path $summaryPath -Encoding UTF8
($reviewManifest | ConvertTo-Json -Depth 6) | Set-Content -Path $reviewManifestPath -Encoding UTF8

$reviewChecklist = @"
# Fixed-grid merge/unmerge visual review checklist

- Start with the aggregate contact sheet when available:
  $aggregateContactSheetPath
- Then inspect each case's ``page-01.png`` and compare it against the case-specific cues below.

- ``merge-right``: the blue merged cell must be wider than the gray ``1000`` base cell below it and still narrower than the green ``4100`` tail column.
- ``merge-down``: the orange pillar must span two rows while staying as the narrow first column; the ``1900/4100`` columns must remain aligned.
- ``unmerge-right``: the blue and yellow cells must return as separate standalone cells with distinct widths (``1000 < 1900 < 4100``).
- ``unmerge-down``: the orange top-left and blue lower-left cells must be separate standalone cells with a clean horizontal border between them.
- In all four cases, borders should stay continuous, fills should remain attached to the intended cells, and no cell should collapse or drift after reopen/merge/unmerge.

Artifacts:

- Summary JSON: $summaryPath
- Review manifest: $reviewManifestPath
- Final review: $finalReviewPath
"@
$reviewChecklist | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReview = @"
# Fixed-grid merge/unmerge final review

- Generated at: $(Get-Date -Format s)
- Summary JSON: $summaryPath
- Review manifest: $reviewManifestPath
- Aggregate contact sheet: $aggregateContactSheetPath

## Verdict

- Overall verdict:
- Notes:

## Case findings

- merge-right:
  Verdict:
  Notes:

- merge-down:
  Verdict:
  Notes:

- unmerge-right:
  Verdict:
  Notes:

- unmerge-down:
  Verdict:
  Notes:
"@
$finalReview | Set-Content -Path $finalReviewPath -Encoding UTF8

if ($PrepareReviewTask) {
    Write-Step "Preparing fixed-grid review task package"
    $preparedTaskInfo = Invoke-PrepareReviewTask `
        -PrepareTaskScript $prepareTaskScript `
        -FixedGridRegressionRoot $resolvedOutputDir `
        -Mode $ReviewMode `
        -TaskOutputRoot $TaskOutputRoot `
        -OpenTaskDir $OpenTaskDir.IsPresent

    $summary.review_task = $preparedTaskInfo
    $reviewManifest.review_task = $preparedTaskInfo
    ($summary | ConvertTo-Json -Depth 6) | Set-Content -Path $summaryPath -Encoding UTF8
    ($reviewManifest | ConvertTo-Json -Depth 6) | Set-Content -Path $reviewManifestPath -Encoding UTF8
}

Write-Step "Completed fixed-grid regression run"
Write-Host "Summary: $summaryPath"
Write-Host "Review manifest: $reviewManifestPath"
Write-Host "Review checklist: $reviewChecklistPath"
if (-not $SkipVisual) {
    Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
}
if ($PrepareReviewTask) {
    Write-Host "Review task mode: $ReviewMode"
    Write-Host "Review task root: $(Join-Path $repoRoot $TaskOutputRoot)"
    if ($preparedTaskInfo.task_dir) {
        Write-Host "Review task directory: $($preparedTaskInfo.task_dir)"
    }
}
foreach ($case in $summary.cases) {
    Write-Host "Case: $($case.id)"
    Write-Host "  DOCX: $($case.docx_path)"
    if ($case.visual_output_dir) {
        Write-Host "  Visual: $($case.visual_output_dir)"
    }
}
