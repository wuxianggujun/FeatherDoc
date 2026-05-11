param(
    [string]$BuildDir = "build-codex-clang-compat",
    [string]$OutputDir = "output/section-text-multiline-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[section-text-multiline-visual] $Message"
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

function Remove-LeadingBom {
    param([AllowNull()][string]$Text)

    if ($null -eq $Text) {
        return ""
    }

    return $Text.TrimStart([char]0xFEFF)
}

function Assert-TextFileLines {
    param(
        [string]$Path,
        [string[]]$ExpectedLines,
        [string]$Label
    )

    $actualLines = @(Get-Content -LiteralPath $Path)
    if ($actualLines.Count -ne $ExpectedLines.Count) {
        throw "$Label line count mismatch. Expected $($ExpectedLines.Count), got $($actualLines.Count)."
    }

    for ($index = 0; $index -lt $ExpectedLines.Count; $index++) {
        $actual = Remove-LeadingBom -Text $actualLines[$index]
        $expected = Remove-LeadingBom -Text $ExpectedLines[$index]
        if ($actual -ne $expected) {
            throw "$Label line $index mismatch. Expected '$($ExpectedLines[$index])', got '$($actualLines[$index])'."
        }
    }
}

function Assert-ParagraphTexts {
    param(
        [string]$JsonPath,
        [string[]]$ExpectedTexts,
        [string]$Label
    )

    $json = Get-Content -Raw -LiteralPath $JsonPath | ConvertFrom-Json
    if ([int]$json.count -ne $ExpectedTexts.Count) {
        throw "$Label paragraph count mismatch. Expected $($ExpectedTexts.Count), got $($json.count)."
    }

    $paragraphs = @($json.paragraphs)
    if ($paragraphs.Count -ne $ExpectedTexts.Count) {
        throw "$Label paragraph array mismatch. Expected $($ExpectedTexts.Count), got $($paragraphs.Count)."
    }

    for ($index = 0; $index -lt $ExpectedTexts.Count; $index++) {
        $actual = Remove-LeadingBom -Text $paragraphs[$index].text
        $expected = Remove-LeadingBom -Text $ExpectedTexts[$index]
        if ($actual -ne $expected) {
            throw "$Label paragraph $index mismatch. Expected '$($ExpectedTexts[$index])', got '$($paragraphs[$index].text)'."
        }
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

    Write-Step "Building featherdoc_cli and section text visual sample"
    & cmake --build $resolvedBuildDir --target featherdoc_cli featherdoc_sample_section_text_multiline_visual -- -j4
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build section text visual regression prerequisites."
    }
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_section_text_multiline_visual"
$renderPython = if ($SkipVisual) { $null } else { Ensure-RenderPython -RepoRoot $repoRoot }

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateSelectedPagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregateSelectedPagesDir -Force | Out-Null

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"

Invoke-CommandCapture `
    -ExecutablePath $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared section text baseline sample."

if (-not $SkipVisual) {
    Write-Step "Rendering Word evidence for the shared baseline document"
    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $baselineDocxPath `
        -OutputDir $baselineVisualDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent
}

$cases = @(
    [ordered]@{
        id = "even-header-text-file-multiline-visual"
        description = "Use set-section-header --kind even --text-file to replace the even header with two paragraphs and verify page 2 in Word."
        command = "set-section-header"
        part = "section-header"
        section = 0
        kind = "even"
        page_number = 2
        page_role = "target-even-page"
        text_lines = @("Even header visual line 1", "Even header visual line 2")
        expected_visual_cues = @(
            "Page 2 replaces the single-line even header placeholder with two header paragraphs.",
            "Page 1 and page 3 keep the default header control line.",
            "The page 2 body flow still starts below the expanded header area without overlap or clipping."
        )
    },
    [ordered]@{
        id = "first-footer-text-file-multiline-visual"
        description = "Use set-section-footer --kind first --text-file to replace the first-page footer with two paragraphs and verify page 1 in Word."
        command = "set-section-footer"
        part = "section-footer"
        section = 0
        kind = "first"
        page_number = 1
        page_role = "target-first-page"
        text_lines = @("First footer visual line 1", "First footer visual line 2")
        expected_visual_cues = @(
            "Page 1 replaces the single-line first-page footer placeholder with two footer paragraphs.",
            "Page 2 and page 3 keep the default footer control line.",
            "The expanded footer stays inside the page area and does not visibly overlap the page 1 body paragraphs."
        )
    },
    [ordered]@{
        id = "default-header-text-file-multiline-visual"
        description = "Use set-section-header --kind default --text-file to replace the default header with two paragraphs and verify page 3 in Word."
        command = "set-section-header"
        part = "section-header"
        section = 0
        kind = "default"
        page_number = 3
        page_role = "target-default-header-page"
        text_lines = @("Default header visual line 1", "Default header visual line 2")
        expected_visual_cues = @(
            "Page 3 replaces the single-line default header control with two header paragraphs.",
            "Page 2 still shows the even header content instead of the default header paragraphs.",
            "The page 3 body flow still starts below the expanded header area without overlap or clipping."
        )
    },
    [ordered]@{
        id = "default-footer-text-file-multiline-visual"
        description = "Use set-section-footer --kind default --text-file to replace the default footer with two paragraphs and verify page 3 in Word."
        command = "set-section-footer"
        part = "section-footer"
        section = 0
        kind = "default"
        page_number = 3
        page_role = "target-default-footer-page"
        text_lines = @("Default footer visual line 1", "Default footer visual line 2")
        expected_visual_cues = @(
            "Page 3 replaces the single-line default footer control with two footer paragraphs.",
            "Page 1 still shows the first-page footer content instead of the default footer paragraphs.",
            "The expanded footer stays inside the page area and does not visibly overlap the page 3 body paragraphs."
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
    shared_baseline_docx = $baselineDocxPath
    cases = @()
}

$aggregateImagePaths = @()
$aggregateLabels = @()

foreach ($case in $cases) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    $commandDir = Join-Path $caseDir "commands"
    $textFilePath = Join-Path $caseDir "replacement.txt"
    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $showTextPath = Join-Path $caseDir "shown.txt"
    $paragraphJsonPath = Join-Path $caseDir "paragraphs.json"
    $commandSequencePath = Join-Path $caseDir "command_sequence.md"
    $mutatedVisualDir = Join-Path $caseDir "mutated-visual"
    $caseContactSheetPath = Join-Path $caseDir "before_after_contact_sheet.png"

    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
    New-Item -ItemType Directory -Path $commandDir -Force | Out-Null

    ($case.text_lines -join "`n") + "`n" | Set-Content -Path $textFilePath -Encoding UTF8

    Write-Step "Running $($case.command) for case '$($case.id)'"
    Invoke-CommandCapture `
        -ExecutablePath $cliExecutable `
        -Arguments @(
            $case.command,
            $baselineDocxPath,
            $case.section,
            "--kind",
            $case.kind,
            "--text-file",
            $textFilePath,
            "--output",
            $mutatedDocxPath,
            "--json"
        ) `
        -OutputPath (Join-Path $commandDir "$($case.command).json") `
        -FailureMessage "Failed to mutate section text for case '$($case.id)'."

    $showCommand = if ($case.command -eq "set-section-header") { "show-section-header" } else { "show-section-footer" }
    Write-Step "Showing mutated part text for case '$($case.id)'"
    Invoke-CommandCapture `
        -ExecutablePath $cliExecutable `
        -Arguments @(
            $showCommand,
            $mutatedDocxPath,
            $case.section,
            "--kind",
            $case.kind
        ) `
        -OutputPath $showTextPath `
        -FailureMessage "Failed to show section text for case '$($case.id)'."

    Assert-TextFileLines `
        -Path $showTextPath `
        -ExpectedLines $case.text_lines `
        -Label $case.id

    Write-Step "Inspecting mutated paragraphs for case '$($case.id)'"
    Invoke-CommandCapture `
        -ExecutablePath $cliExecutable `
        -Arguments @(
            "inspect-template-paragraphs",
            $mutatedDocxPath,
            "--part",
            $case.part,
            "--section",
            $case.section,
            "--kind",
            $case.kind,
            "--json"
        ) `
        -OutputPath $paragraphJsonPath `
        -FailureMessage "Failed to inspect mutated paragraphs for case '$($case.id)'."

    Assert-ParagraphTexts `
        -JsonPath $paragraphJsonPath `
        -ExpectedTexts $case.text_lines `
        -Label $case.id

    $commandLines = [System.Collections.Generic.List[string]]::new()
    [void]$commandLines.Add("# Section text multiline CLI mutation sequence for $($case.id)")
    [void]$commandLines.Add("")
    Add-CommandLine -Lines $commandLines -Command $case.command -Arguments @(
        "<input.docx>",
        $case.section,
        "--kind",
        $case.kind,
        "--text-file",
        "replacement.txt",
        "--output",
        "<output.docx>",
        "--json"
    )
    Add-CommandLine -Lines $commandLines -Command $showCommand -Arguments @(
        "<output.docx>",
        $case.section,
        "--kind",
        $case.kind
    )
    Add-CommandLine -Lines $commandLines -Command "inspect-template-paragraphs" -Arguments @(
        "<output.docx>",
        "--part",
        $case.part,
        "--section",
        $case.section,
        "--kind",
        $case.kind,
        "--json"
    )
    $commandLines | Set-Content -Path $commandSequencePath -Encoding UTF8

    $caseSummary = [ordered]@{
        id = $case.id
        description = $case.description
        command = $case.command
        section = $case.section
        kind = $case.kind
        page_number = $case.page_number
        replacement_text_file = $textFilePath
        shown_text = $showTextPath
        paragraph_json = $paragraphJsonPath
        command_sequence = $commandSequencePath
        mutated_docx = $mutatedDocxPath
        expected_paragraph_texts = @($case.text_lines)
        expected_visual_cues = @($case.expected_visual_cues)
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

        $baselinePage = Get-RenderedPagePath `
            -VisualOutputDir $baselineVisualDir `
            -PageNumber $case.page_number `
            -Label "baseline case '$($case.id)'"
        $mutatedPage = Get-RenderedPagePath `
            -VisualOutputDir $mutatedVisualDir `
            -PageNumber $case.page_number `
            -Label "mutated case '$($case.id)'"

        Build-ContactSheet `
            -Python $renderPython `
            -ScriptPath $contactSheetScript `
            -OutputPath $caseContactSheetPath `
            -Images @($baselinePage, $mutatedPage) `
            -Labels @("$($case.id)-baseline", "$($case.id)-mutated")

        $aggregateBaselinePage = Join-Path $aggregateSelectedPagesDir ("{0}-baseline-page-{1:D2}.png" -f $case.id, $case.page_number)
        $aggregateMutatedPage = Join-Path $aggregateSelectedPagesDir ("{0}-mutated-page-{1:D2}.png" -f $case.id, $case.page_number)
        Copy-Item -Path $baselinePage -Destination $aggregateBaselinePage -Force
        Copy-Item -Path $mutatedPage -Destination $aggregateMutatedPage -Force
        $aggregateImagePaths += $aggregateBaselinePage
        $aggregateImagePaths += $aggregateMutatedPage
        $aggregateLabels += "$($case.id)-baseline"
        $aggregateLabels += "$($case.id)-mutated"

        $caseSummary.visual_artifacts = [ordered]@{
            baseline_visual_output_dir = $baselineVisualDir
            mutated_visual_output_dir = $mutatedVisualDir
            selected_pages = @(
                [ordered]@{
                    page_number = $case.page_number
                    role = $case.page_role
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
        selected_pages_dir = $aggregateSelectedPagesDir
        contact_sheet = $aggregateContactSheetPath
        shared_baseline_visual_output_dir = $baselineVisualDir
    }
}

($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

$reviewChecklistLines = [System.Collections.Generic.List[string]]::new()
[void]$reviewChecklistLines.Add("# Section text multiline visual regression checklist")
[void]$reviewChecklistLines.Add("")
if (-not $SkipVisual) {
    [void]$reviewChecklistLines.Add("- Start with the aggregate before/after contact sheet:")
    [void]$reviewChecklistLines.Add("  $aggregateContactSheetPath")
    [void]$reviewChecklistLines.Add("")
}
foreach ($case in $cases) {
    [void]$reviewChecklistLines.Add("- `"$($case.id)`":")
    [void]$reviewChecklistLines.Add("  - Review page: page-$('{0:D2}' -f $case.page_number) ($($case.page_role))")
    foreach ($cue in $case.expected_visual_cues) {
        [void]$reviewChecklistLines.Add("  - $cue")
    }
}
[void]$reviewChecklistLines.Add("")
[void]$reviewChecklistLines.Add("Artifacts:")
[void]$reviewChecklistLines.Add("")
[void]$reviewChecklistLines.Add("- Summary JSON: $summaryPath")
[void]$reviewChecklistLines.Add("- Shared baseline DOCX: $baselineDocxPath")
foreach ($caseSummary in $summary.cases) {
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) shown text: $($caseSummary.shown_text)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) paragraph JSON: $($caseSummary.paragraph_json)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) command sequence: $($caseSummary.command_sequence)")
}
$reviewChecklistLines | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReviewLines = [System.Collections.Generic.List[string]]::new()
[void]$finalReviewLines.Add("# Section text multiline visual final review")
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
    [void]$finalReviewLines.Add("  Selected page: page-$('{0:D2}' -f $case.page_number) ($($case.page_role))")
    [void]$finalReviewLines.Add("")
}
$finalReviewLines | Set-Content -Path $finalReviewPath -Encoding UTF8

Write-Step "Completed section text multiline visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Review checklist: $reviewChecklistPath"
Write-Host "Final review: $finalReviewPath"
Write-Host "Shared baseline DOCX: $baselineDocxPath"
foreach ($caseSummary in $summary.cases) {
    Write-Host "$($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)"
}
if (-not $SkipVisual) {
    Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
}
