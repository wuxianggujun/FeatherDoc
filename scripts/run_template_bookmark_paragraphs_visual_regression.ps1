param(
    [string]$BuildDir = "build-codex-clang-compat",
    [string]$OutputDir = "output/template-bookmark-paragraphs-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[template-bookmark-paragraphs-visual] $Message"
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

function Assert-ParagraphText {
    param(
        [string]$JsonPath,
        [string]$ExpectedText,
        [string]$Label
    )

    $json = Get-Content -Raw -LiteralPath $JsonPath | ConvertFrom-Json
    if ($null -eq $json.paragraph) {
        throw "$Label did not contain a paragraph payload."
    }
    if ($json.paragraph.text -ne $ExpectedText) {
        throw "$Label paragraph text mismatch. Expected '$ExpectedText', got '$($json.paragraph.text)'."
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"

if (-not $SkipBuild) {
    Write-Step "Configuring build directory $resolvedBuildDir"
    & cmake -S $repoRoot -B $resolvedBuildDir
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to configure the build directory."
    }

    Write-Step "Building featherdoc_cli and bookmark sample"
    & cmake --build $resolvedBuildDir --target featherdoc_cli featherdoc_sample_template_bookmark_multiline_visual -- -j4
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build bookmark paragraphs visual regression prerequisites."
    }
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_template_bookmark_multiline_visual"
$renderPython = if ($SkipVisual) { $null } else { Ensure-RenderPython -RepoRoot $repoRoot }

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$sharedBaselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($sharedBaselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared bookmark baseline sample."

$sharedBaselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"
$sharedBaselineFirstPage = $null
if (-not $SkipVisual) {
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
}

$cases = @(
    [ordered]@{
        id = "body-bookmark-paragraphs-visual"
        bookmark_name = "body_note"
        part_args = @("--part", "body")
        replace_args = @("--paragraph", "North seed", "--paragraph", "Pending approval")
        inspect_targets = @(
            [ordered]@{ paragraph_index = 3; expected_text = "North seed"; label = "Body first inserted paragraph" },
            [ordered]@{ paragraph_index = 4; expected_text = "Pending approval"; label = "Body second inserted paragraph" },
            [ordered]@{ paragraph_index = 5; expected_text = "This retained body paragraph should stay visually separate from the mutated body bookmark above."; label = "Body retained paragraph" }
        )
        expected_visual_cues = @(
            "The header and footer retained lines stay unchanged.",
            "The body bookmark expands into two separate body paragraphs: North seed / Pending approval.",
            "The retained body paragraph below stays in place without overlap."
        )
    },
    [ordered]@{
        id = "header-bookmark-paragraphs-visual"
        bookmark_name = "header_note"
        part_args = @("--part", "section-header", "--section", "0")
        replace_args = @("--paragraph", "Header title", "--paragraph", "Pending approval")
        inspect_targets = @(
            [ordered]@{ paragraph_index = 1; expected_text = "Header title"; label = "Header first inserted paragraph" },
            [ordered]@{ paragraph_index = 2; expected_text = "Pending approval"; label = "Header second inserted paragraph" }
        )
        expected_visual_cues = @(
            "The retained header line remains unchanged.",
            "The target header bookmark expands into two header paragraphs: Header title / Pending approval.",
            "The body and footer baseline content remain unchanged."
        )
    },
    [ordered]@{
        id = "footer-bookmark-paragraphs-visual"
        bookmark_name = "footer_note"
        part_args = @("--part", "section-footer", "--section", "0")
        replace_args = @("--paragraph", "Footer owner", "--paragraph", "Shanghai office")
        inspect_targets = @(
            [ordered]@{ paragraph_index = 1; expected_text = "Footer owner"; label = "Footer first inserted paragraph" },
            [ordered]@{ paragraph_index = 2; expected_text = "Shanghai office"; label = "Footer second inserted paragraph" }
        )
        expected_visual_cues = @(
            "The retained footer line remains unchanged.",
            "The target footer bookmark expands into two footer paragraphs: Footer owner / Shanghai office.",
            "The body and header baseline content remain unchanged."
        )
    }
)

$caseSummaries = @()
$aggregateImages = [System.Collections.Generic.List[string]]::new()
$aggregateLabels = [System.Collections.Generic.List[string]]::new()

foreach ($case in $cases) {
    $caseId = $case.id
    Write-Step "Running case '$caseId'"

    $caseOutputDir = Join-Path $resolvedOutputDir $caseId
    New-Item -ItemType Directory -Path $caseOutputDir -Force | Out-Null

    $mutatedDocxPath = Join-Path $caseOutputDir "$caseId-mutated.docx"
    $replaceJsonPath = Join-Path $caseOutputDir "replace_result.json"

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments (@(
                "replace-bookmark-paragraphs",
                $sharedBaselineDocxPath,
                $case.bookmark_name
            ) + @($case.part_args) + @($case.replace_args) + @(
                "--output",
                $mutatedDocxPath,
                "--json"
            )) `
        -OutputPath $replaceJsonPath `
        -FailureMessage "Failed to replace bookmark paragraphs for case '$caseId'."

    $inspectSummaries = @()
    foreach ($inspectTarget in $case.inspect_targets) {
        $paragraphIndex = [int]$inspectTarget.paragraph_index
        $inspectPath = Join-Path $caseOutputDir ("paragraph-{0:D2}.json" -f $paragraphIndex)
        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments (@(
                    "inspect-template-paragraphs",
                    $mutatedDocxPath
                ) + @($case.part_args) + @(
                    "--paragraph",
                    "$paragraphIndex",
                    "--json"
                )) `
            -OutputPath $inspectPath `
            -FailureMessage "Failed to inspect paragraph $paragraphIndex for case '$caseId'."
        Assert-ParagraphText `
            -JsonPath $inspectPath `
            -ExpectedText $inspectTarget.expected_text `
            -Label $inspectTarget.label
        $inspectSummaries += [ordered]@{
            paragraph_index = $paragraphIndex
            expected_text = $inspectTarget.expected_text
            json = $inspectPath
        }
    }

    $caseSummary = [ordered]@{
        id = $caseId
        command = "replace-bookmark-paragraphs"
        bookmark_name = $case.bookmark_name
        mutated_docx = $mutatedDocxPath
        replace_json = $replaceJsonPath
        inspected_paragraphs = $inspectSummaries
        expected_visual_cues = $case.expected_visual_cues
    }

    if (-not $SkipVisual) {
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

        [void]$aggregateImages.Add($baselinePagePath)
        [void]$aggregateLabels.Add("$caseId baseline")
        [void]$aggregateImages.Add($mutatedPagePath)
        [void]$aggregateLabels.Add("$caseId mutated")

        $caseSummary.visual_artifacts = [ordered]@{
            before_after_contact_sheet = $caseContactSheetPath
            baseline_page = $baselinePagePath
            mutated_page = $mutatedPagePath
        }
    }

    $caseSummaries += $caseSummary
}

if (-not $SkipVisual -and $aggregateImages.Count -gt 0) {
    Write-Step "Building aggregate before/after contact sheet"
    Build-ContactSheet `
        -Python $renderPython `
        -ScriptPath $contactSheetScript `
        -OutputPath $aggregateContactSheetPath `
        -Images $aggregateImages `
        -Labels $aggregateLabels
}

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual)
    shared_baseline_docx = $sharedBaselineDocxPath
    cases = $caseSummaries
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = if ($SkipVisual) { $null } else { $aggregateContactSheetPath }
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed template bookmark paragraphs visual regression"
Write-Host "Summary: $summaryPath"
if (-not $SkipVisual) {
    Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
}
