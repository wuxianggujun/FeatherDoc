param(
    [string]$BuildDir = "build-codex-clang-compat",
    [string]$OutputDir = "output/template-bookmark-paragraphs-pagination-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[template-bookmark-paragraphs-pagination-visual] $Message"
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
        [string[]]$Labels,
        [int]$Columns = 2
    )

    & $Python $ScriptPath --output $OutputPath --columns $Columns --thumb-width 420 --images $Images --labels $Labels
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

function Assert-PageExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if (-not (Test-Path $Path)) {
        throw "$Label page image was not generated: $Path"
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

    Write-Step "Building featherdoc_cli and pagination sample"
    & cmake --build $resolvedBuildDir --target featherdoc_cli featherdoc_sample_template_bookmark_paragraphs_pagination_visual -- -j4
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build pagination visual regression prerequisites."
    }
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_template_bookmark_paragraphs_pagination_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate pagination baseline sample."

$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"
Write-Step "Rendering Word evidence for the pagination baseline document"
Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $baselineDocxPath `
    -OutputDir $baselineVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$baselinePage1 = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
$baselinePage2 = Join-Path $baselineVisualDir "evidence\pages\page-02.png"
Assert-PageExists -Path $baselinePage1 -Label "Baseline page 1"
Assert-PageExists -Path $baselinePage2 -Label "Baseline page 2"

$caseId = "body-bookmark-paragraphs-cross-page-visual"
$caseOutputDir = Join-Path $resolvedOutputDir $caseId
New-Item -ItemType Directory -Path $caseOutputDir -Force | Out-Null
$mutatedDocxPath = Join-Path $caseOutputDir "$caseId-mutated.docx"
$replaceJsonPath = Join-Path $caseOutputDir "replace_result.json"

$replaceArgs = @(
    "replace-bookmark-paragraphs",
    $baselineDocxPath,
    "body_note",
    "--part",
    "body",
    "--paragraph",
    "Expanded schedule item 1 keeps the first page denser.",
    "--paragraph",
    "Expanded schedule item 2 keeps the first page denser.",
    "--paragraph",
    "Expanded schedule item 3 keeps the first page denser.",
    "--paragraph",
    "Expanded schedule item 4 keeps the first page denser.",
    "--paragraph",
    "Expanded schedule item 5 keeps the first page denser.",
    "--paragraph",
    "Expanded schedule item 6 keeps the first page denser.",
    "--output",
    $mutatedDocxPath,
    "--json"
)

Write-Step "Running replace-bookmark-paragraphs for cross-page case"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments $replaceArgs `
    -OutputPath $replaceJsonPath `
    -FailureMessage "Failed to replace bookmark paragraphs for the cross-page case."

$inspectTargets = @(
    [ordered]@{ paragraph_index = 3; expected_text = "Expanded schedule item 1 keeps the first page denser."; label = "First inserted paragraph" },
    [ordered]@{ paragraph_index = 8; expected_text = "Expanded schedule item 6 keeps the first page denser."; label = "Last inserted paragraph" },
    [ordered]@{ paragraph_index = 9; expected_text = "This retained body paragraph should move downward but remain readable after the inserted paragraphs."; label = "Retained paragraph after inserted block" }
)

$inspectSummaries = @()
foreach ($inspectTarget in $inspectTargets) {
    $paragraphIndex = [int]$inspectTarget.paragraph_index
    $inspectPath = Join-Path $caseOutputDir ("paragraph-{0:D2}.json" -f $paragraphIndex)
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @(
            "inspect-template-paragraphs",
            $mutatedDocxPath,
            "--part",
            "body",
            "--paragraph",
            "$paragraphIndex",
            "--json"
        ) `
        -OutputPath $inspectPath `
        -FailureMessage "Failed to inspect paragraph $paragraphIndex for the cross-page case."
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

$mutatedVisualDir = Join-Path $caseOutputDir "mutated-visual"
Write-Step "Rendering Word evidence for the cross-page case"
Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $mutatedDocxPath `
    -OutputDir $mutatedVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent

$mutatedPage1 = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
$mutatedPage2 = Join-Path $mutatedVisualDir "evidence\pages\page-02.png"
Assert-PageExists -Path $mutatedPage1 -Label "Mutated page 1"
Assert-PageExists -Path $mutatedPage2 -Label "Mutated page 2"

$baselineSelectedPage1 = Join-Path $aggregatePagesDir "$caseId-baseline-page-01.png"
$mutatedSelectedPage1 = Join-Path $aggregatePagesDir "$caseId-mutated-page-01.png"
$baselineSelectedPage2 = Join-Path $aggregatePagesDir "$caseId-baseline-page-02.png"
$mutatedSelectedPage2 = Join-Path $aggregatePagesDir "$caseId-mutated-page-02.png"

Copy-Item -Path $baselinePage1 -Destination $baselineSelectedPage1 -Force
Copy-Item -Path $mutatedPage1 -Destination $mutatedSelectedPage1 -Force
Copy-Item -Path $baselinePage2 -Destination $baselineSelectedPage2 -Force
Copy-Item -Path $mutatedPage2 -Destination $mutatedSelectedPage2 -Force

$caseContactSheetPath = Join-Path $caseOutputDir "before_after_contact_sheet.png"
Build-ContactSheet `
    -Python $renderPython `
    -ScriptPath $contactSheetScript `
    -OutputPath $caseContactSheetPath `
    -Images @($baselineSelectedPage1, $mutatedSelectedPage1, $baselineSelectedPage2, $mutatedSelectedPage2) `
    -Labels @("$caseId baseline page 1", "$caseId mutated page 1", "$caseId baseline page 2", "$caseId mutated page 2") `
    -Columns 2

Copy-Item -Path $caseContactSheetPath -Destination $aggregateContactSheetPath -Force

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = $true
    shared_baseline_docx = $baselineDocxPath
    cases = @(
        [ordered]@{
            id = $caseId
            command = "replace-bookmark-paragraphs"
            bookmark_name = "body_note"
            mutated_docx = $mutatedDocxPath
            replace_json = $replaceJsonPath
            inspected_paragraphs = $inspectSummaries
            expected_visual_cues = @(
                "Page 1 replaces the single placeholder with six separate body paragraphs.",
                "Page 2 remains readable after the expanded block pushes later body paragraphs downward.",
                "The retained header and footer lines stay unchanged on both selected pages."
            )
            visual_artifacts = [ordered]@{
                before_after_contact_sheet = $caseContactSheetPath
                selected_pages = @(
                    [ordered]@{ page_number = 1; baseline_page = $baselineSelectedPage1; mutated_page = $mutatedSelectedPage1 },
                    [ordered]@{ page_number = 2; baseline_page = $baselineSelectedPage2; mutated_page = $mutatedSelectedPage2 }
                )
            }
        }
    )
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed template bookmark paragraphs pagination visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Case contact sheet: $caseContactSheetPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
