param(
    [string]$BuildDir = "build-codex-clang-compat",
    [string]$OutputDir = "output/template-bookmark-multiline-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[template-bookmark-multiline-visual] $Message"
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

function Add-CommandLine {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Command,
        [string[]]$Arguments
    )

    [void]$Lines.Add('```powershell')
    $escapedArgs = @()
    foreach ($argument in $Arguments) {
        if ($argument -match "\s") {
            $escapedArgs += "'$argument'"
        } else {
            $escapedArgs += $argument
        }
    }
    [void]$Lines.Add("& .\featherdoc_cli.exe $Command " + ($escapedArgs -join " "))
    [void]$Lines.Add('```')
    [void]$Lines.Add("")
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

    Write-Step "Building featherdoc_cli and multiline visual sample"
    & cmake --build $resolvedBuildDir --target featherdoc_cli featherdoc_sample_template_bookmark_multiline_visual -- -j4
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build multiline visual regression prerequisites."
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

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"

$sharedBaselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$sharedBaselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"
Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($sharedBaselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared multiline baseline sample."

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
        id = "body-bookmark-explicit-break-visual"
        description = "Body bookmark replacement should render a single paragraph with an explicit line break in the body flow."
        bookmark_name = "body_note"
        replacement_text = "North seed`npending approval"
        part_args = @("--part", "body")
        inspect_args = @("--part", "body", "--paragraph", "3")
        expected_paragraph_text = "North seed`npending approval"
        expected_visual_cues = @(
            "The header and footer retained lines remain unchanged.",
            "The body bookmark paragraph renders as two visible lines: North seed / pending approval.",
            "The retained body paragraph below stays in place and does not overlap the mutated paragraph."
        )
    },
    [ordered]@{
        id = "header-bookmark-explicit-break-visual"
        description = "Section header bookmark replacement should render two visible lines without disturbing the retained header paragraph."
        bookmark_name = "header_note"
        replacement_text = "Header title`nPending approval"
        part_args = @("--part", "section-header", "--section", "0")
        inspect_args = @("--part", "section-header", "--section", "0", "--paragraph", "1")
        expected_paragraph_text = "Header title`nPending approval"
        expected_visual_cues = @(
            "The retained header line remains unchanged.",
            "The target header bookmark renders as two visible lines: Header title / Pending approval.",
            "The body and footer baseline content remain unchanged."
        )
    },
    [ordered]@{
        id = "footer-bookmark-explicit-break-visual"
        description = "Section footer bookmark replacement should render two visible lines without disturbing the retained footer paragraph."
        bookmark_name = "footer_note"
        replacement_text = "Footer owner`nShanghai office"
        part_args = @("--part", "section-footer", "--section", "0")
        inspect_args = @("--part", "section-footer", "--section", "0", "--paragraph", "1")
        expected_paragraph_text = "Footer owner`nShanghai office"
        expected_visual_cues = @(
            "The retained footer line remains unchanged.",
            "The target footer bookmark renders as two visible lines: Footer owner / Shanghai office.",
            "The body and header baseline content remain unchanged."
        )
    }
)

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    summary_json = $summaryPath
    review_checklist = $reviewChecklistPath
    final_review = $finalReviewPath
    shared_baseline_docx = $sharedBaselineDocxPath
    cases = @()
}

$aggregateImagePaths = @()
$aggregateLabels = @()

foreach ($case in $cases) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $replaceJsonPath = Join-Path $caseDir "replace_result.json"
    $inspectJsonPath = Join-Path $caseDir "paragraph.json"
    $commandSequencePath = Join-Path $caseDir "command_sequence.md"
    $mutatedVisualDir = Join-Path $caseDir "mutated-visual"
    $caseContactSheetPath = Join-Path $caseDir "before_after_contact_sheet.png"

    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    Write-Step "Running replace-bookmark-text for case '$($case.id)'"
    $replaceArguments = @(
        "replace-bookmark-text",
        $sharedBaselineDocxPath,
        $case.bookmark_name
    ) + @($case.part_args) + @(
        "--text",
        $case.replacement_text,
        "--output",
        $mutatedDocxPath,
        "--json"
    )
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments $replaceArguments `
        -OutputPath $replaceJsonPath `
        -FailureMessage "Failed to replace bookmark text for case '$($case.id)'."

    Write-Step "Inspecting mutated paragraph for case '$($case.id)'"
    $inspectArguments = @(
        "inspect-template-paragraphs",
        $mutatedDocxPath
    ) + @($case.inspect_args) + @("--json")
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments $inspectArguments `
        -OutputPath $inspectJsonPath `
        -FailureMessage "Failed to inspect mutated paragraph for case '$($case.id)'."

    Assert-ParagraphText `
        -JsonPath $inspectJsonPath `
        -ExpectedText $case.expected_paragraph_text `
        -Label $case.id

    $commandLines = [System.Collections.Generic.List[string]]::new()
    [void]$commandLines.Add("# Template bookmark multiline mutation sequence for $($case.id)")
    [void]$commandLines.Add("")
    Add-CommandLine -Lines $commandLines -Command "replace-bookmark-text" -Arguments (@("<input.docx>", $case.bookmark_name) + @($case.part_args) + @("--text", $case.replacement_text, "--output", "<output.docx>", "--json"))
    Add-CommandLine -Lines $commandLines -Command "inspect-template-paragraphs" -Arguments (@("<output.docx>") + @($case.inspect_args) + @("--json"))
    $commandLines | Set-Content -Path $commandSequencePath -Encoding UTF8

    $caseSummary = [ordered]@{
        id = $case.id
        description = $case.description
        bookmark_name = $case.bookmark_name
        replacement_text = $case.expected_paragraph_text
        mutated_docx = $mutatedDocxPath
        replace_result_json = $replaceJsonPath
        paragraph_json = $inspectJsonPath
        expected_paragraph_text = $case.expected_paragraph_text
        expected_visual_cues = @($case.expected_visual_cues)
        command_sequence = $commandSequencePath
    }

    if (-not $SkipVisual) {
        Write-Step "Rendering Word evidence for case '$($case.id)'"
        Invoke-WordSmoke `
            -ScriptPath $wordSmokeScript `
            -BuildDir $BuildDir `
            -DocxPath $mutatedDocxPath `
            -OutputDir $mutatedVisualDir `
            -Dpi $Dpi `
            -KeepWordOpen $KeepWordOpen.IsPresent `
            -VisibleWord $VisibleWord.IsPresent

        $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
        Build-ContactSheet `
            -Python $renderPython `
            -ScriptPath $contactSheetScript `
            -OutputPath $caseContactSheetPath `
            -Images @($sharedBaselineFirstPage, $mutatedPage) `
            -Labels @("$($case.id)-baseline", "$($case.id)-mutated")

        $aggregateBaselinePage = Join-Path $aggregatePagesDir "$($case.id)-baseline-page-01.png"
        $aggregateMutatedPage = Join-Path $aggregatePagesDir "$($case.id)-mutated-page-01.png"
        Copy-Item -Path $sharedBaselineFirstPage -Destination $aggregateBaselinePage -Force
        Copy-Item -Path $mutatedPage -Destination $aggregateMutatedPage -Force
        $aggregateImagePaths += $aggregateBaselinePage
        $aggregateImagePaths += $aggregateMutatedPage
        $aggregateLabels += "$($case.id)-baseline"
        $aggregateLabels += "$($case.id)-mutated"

        $caseSummary.visual_artifacts = [ordered]@{
            baseline_visual_output_dir = $sharedBaselineVisualDir
            mutated_visual_output_dir = $mutatedVisualDir
            selected_pages = @(
                [ordered]@{
                    page_number = 1
                    role = "page-01"
                    baseline_page = $aggregateBaselinePage
                    mutated_page = $aggregateMutatedPage
                }
            )
            before_after_contact_sheet = $caseContactSheetPath
        }
    }

    $summary.cases += $caseSummary
}

if (-not $SkipVisual) {
    Write-Step "Building aggregate before/after contact sheet"
    Build-ContactSheet `
        -Python $renderPython `
        -ScriptPath $contactSheetScript `
        -OutputPath $aggregateContactSheetPath `
        -Images $aggregateImagePaths `
        -Labels $aggregateLabels

    $summary.aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
        shared_baseline_visual_output_dir = $sharedBaselineVisualDir
    }
}

($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

$reviewChecklistLines = [System.Collections.Generic.List[string]]::new()
[void]$reviewChecklistLines.Add("# Template bookmark multiline visual regression checklist")
[void]$reviewChecklistLines.Add("")
if (-not $SkipVisual) {
    [void]$reviewChecklistLines.Add("- Start with the aggregate before/after contact sheet:")
    [void]$reviewChecklistLines.Add("  $aggregateContactSheetPath")
    [void]$reviewChecklistLines.Add("")
}
foreach ($case in $cases) {
    [void]$reviewChecklistLines.Add("- `"$($case.id)`":")
    foreach ($cue in $case.expected_visual_cues) {
        [void]$reviewChecklistLines.Add("  - $cue")
    }
}
[void]$reviewChecklistLines.Add("")
[void]$reviewChecklistLines.Add("Artifacts:")
[void]$reviewChecklistLines.Add("")
[void]$reviewChecklistLines.Add("- Summary JSON: $summaryPath")
[void]$reviewChecklistLines.Add("- Shared baseline DOCX: $sharedBaselineDocxPath")
foreach ($caseSummary in $summary.cases) {
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) paragraph JSON: $($caseSummary.paragraph_json)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) command sequence: $($caseSummary.command_sequence)")
}
$reviewChecklistLines | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReviewLines = [System.Collections.Generic.List[string]]::new()
[void]$finalReviewLines.Add("# Template bookmark multiline visual final review")
[void]$finalReviewLines.Add("")
[void]$finalReviewLines.Add("- Generated at: $(Get-Date -Format s)")
[void]$finalReviewLines.Add("- Summary JSON: $summaryPath")
[void]$finalReviewLines.Add("- Visual enabled: $((-not $SkipVisual.IsPresent).ToString().ToLowerInvariant())")
if (-not $SkipVisual) {
    [void]$finalReviewLines.Add("- Aggregate contact sheet: $aggregateContactSheetPath")
}
[void]$finalReviewLines.Add("")
[void]$finalReviewLines.Add("## Verdict")
[void]$finalReviewLines.Add("")
[void]$finalReviewLines.Add("- Overall verdict:")
[void]$finalReviewLines.Add("- Notes:")
[void]$finalReviewLines.Add("")
[void]$finalReviewLines.Add("## Case findings")
[void]$finalReviewLines.Add("")
foreach ($case in $cases) {
    [void]$finalReviewLines.Add("- $($case.id):")
    [void]$finalReviewLines.Add("  Verdict:")
    [void]$finalReviewLines.Add("  Notes:")
    if (-not $SkipVisual) {
        [void]$finalReviewLines.Add("  Contact sheet: $(Join-Path (Join-Path $resolvedOutputDir $case.id) 'before_after_contact_sheet.png')")
    }
    [void]$finalReviewLines.Add("")
}
$finalReviewLines | Set-Content -Path $finalReviewPath -Encoding UTF8

Write-Step "Completed template bookmark multiline visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Review checklist: $reviewChecklistPath"
Write-Host "Final review: $finalReviewPath"
Write-Host "Shared baseline DOCX: $sharedBaselineDocxPath"
foreach ($caseSummary in $summary.cases) {
    Write-Host "$($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)"
}
if (-not $SkipVisual) {
    Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
}
