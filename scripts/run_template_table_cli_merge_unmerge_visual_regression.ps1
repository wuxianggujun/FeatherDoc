param(
    [string]$BuildDir = "build-template-table-cli-merge-unmerge-visual-nmake",
    [string]$OutputDir = "output/template-table-cli-merge-unmerge-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[template-table-cli-merge-visual] $Message"
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

    $bestCandidate = $candidates |
        Sort-Object @{
            Expression = {
                ($_.FullName.Substring($resolvedBuildRoot.Length).TrimStart('\') -split '\\').Count
            }
        }, @{ Expression = { $_.LastWriteTime }; Descending = $true } |
        Select-Object -First 1
    return $bestCandidate.FullName
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

function Test-StringArraysEqual {
    param(
        [string[]]$Left,
        [string[]]$Right
    )

    if ($Left.Count -ne $Right.Count) {
        return $false
    }

    for ($index = 0; $index -lt $Left.Count; $index++) {
        if ($Left[$index] -ne $Right[$index]) {
            return $false
        }
    }

    return $true
}

function Assert-TableRows {
    param(
        [string]$RowsJsonPath,
        [string[][]]$ExpectedRows,
        [string]$Label
    )

    $json = Get-Content -Raw -LiteralPath $RowsJsonPath | ConvertFrom-Json
    $rows = @($json.rows)
    if ($rows.Count -ne $ExpectedRows.Count) {
        throw "$Label row count mismatch. Expected $($ExpectedRows.Count), got $($rows.Count)."
    }

    for ($index = 0; $index -lt $ExpectedRows.Count; $index++) {
        $actualCells = @($rows[$index].cell_texts | ForEach-Object { $_.ToString() })
        $expectedCells = @($ExpectedRows[$index])
        if (-not (Test-StringArraysEqual -Left $actualCells -Right $expectedCells)) {
            $expectedJoined = $expectedCells -join " | "
            $actualJoined = $actualCells -join " | "
            throw "$Label row $index mismatch. Expected '$expectedJoined', got '$actualJoined'."
        }
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

function Get-DocxEntryText {
    param(
        [string]$DocxPath,
        [string]$EntryName
    )

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        $entry = $archive.GetEntry($EntryName)
        if (-not $entry) {
            throw "Missing DOCX entry '$EntryName' in $DocxPath."
        }

        $reader = [System.IO.StreamReader]::new($entry.Open())
        try {
            return $reader.ReadToEnd()
        } finally {
            $reader.Dispose()
        }
    } finally {
        $archive.Dispose()
    }
}

function Assert-EntryPatterns {
    param(
        [string]$XmlText,
        [string[]]$MustContain,
        [string[]]$MustNotContain,
        [string]$Label
    )

    foreach ($pattern in $MustContain) {
        if (-not $XmlText.Contains($pattern)) {
            throw "$Label is missing XML pattern '$pattern'."
        }
    }

    foreach ($pattern in $MustNotContain) {
        if ($XmlText.Contains($pattern)) {
            throw "$Label unexpectedly contains XML pattern '$pattern'."
        }
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"

$cases = @(
    [ordered]@{
        id = "section-header-merge-right"
        sample_case_id = "section-header-merge-right"
        description = "Merge the purple section-header target table horizontally while the retained blue table and body text stay stable."
        command = "merge-template-table-cells"
        part = "section-header"
        part_args = @("--part", "section-header", "--section", "0", "--kind", "default")
        target_table_index = "1"
        row_index = "0"
        cell_index = "0"
        command_args = @("--direction", "right", "--count", "1")
        baseline_rows = @(
            @("Purple span anchor", "Merge source", "Green tail"),
            @("1400 cue", "2100 cue", "3300 cue")
        )
        mutated_rows = @(
            @("Purple span anchor", "Green tail"),
            @("1400 cue", "2100 cue", "3300 cue")
        )
        xml_entry = "word/header1.xml"
        baseline_xml_contains = @()
        baseline_xml_not_contains = @("w:gridSpan")
        mutated_xml_contains = @("w:gridSpan w:val=""2""")
        mutated_xml_not_contains = @()
        expected_visual_cues = @(
            "The retained blue header table stays unchanged above the purple target table.",
            "The purple header anchor cell becomes a visibly wider horizontal span while the green tail column remains separate.",
            "Body text still starts below the header tables with no overlap or clipping."
        )
    },
    [ordered]@{
        id = "section-footer-merge-down"
        sample_case_id = "section-footer-merge-down"
        description = "Merge the orange section-footer target table downward and keep the footer inside the page footer area."
        command = "merge-template-table-cells"
        part = "section-footer"
        part_args = @("--part", "section-footer", "--section", "0", "--kind", "default")
        target_table_index = "0"
        row_index = "0"
        cell_index = "0"
        command_args = @("--direction", "down", "--count", "2")
        baseline_rows = @(
            @("Orange pillar anchor", "Footer top"),
            @("Orange middle", "Footer middle"),
            @("Orange tail", "Footer bottom")
        )
        mutated_rows = @(
            @("Orange pillar anchor", "Footer top"),
            @("", "Footer middle"),
            @("", "Footer bottom")
        )
        xml_entry = "word/footer1.xml"
        baseline_xml_contains = @()
        baseline_xml_not_contains = @("w:vMerge")
        mutated_xml_contains = @("w:vMerge w:val=""restart""", "w:vMerge w:val=""continue""")
        mutated_xml_not_contains = @()
        expected_visual_cues = @(
            "The footer intro paragraph remains above the target table.",
            "The orange first-column cell becomes one vertical pillar spanning all three footer rows.",
            "The enlarged footer still stays inside the footer area and does not collide with the page body."
        )
    },
    [ordered]@{
        id = "body-unmerge-right"
        sample_case_id = "body-unmerge-right"
        description = "Split an already merged body-table span while the retained blue witness table remains unchanged."
        command = "unmerge-template-table-cells"
        part = "body"
        part_args = @("--part", "body")
        target_table_index = "1"
        row_index = "0"
        cell_index = "0"
        command_args = @("--direction", "right")
        baseline_rows = @(, @("Merged blue span", "Right tail"))
        mutated_rows = @(, @("Merged blue span", "", "Right tail"))
        xml_entry = "word/document.xml"
        baseline_xml_contains = @("w:gridSpan w:val=""2""")
        baseline_xml_not_contains = @()
        mutated_xml_contains = @()
        mutated_xml_not_contains = @("w:gridSpan")
        expected_visual_cues = @(
            "The retained blue body witness table remains unchanged above the target table.",
            "The blue merged span splits back into separate blue and restored middle cells while the green tail stays in place.",
            "The paragraph after the body target table remains readable and keeps its spacing."
        )
    },
    [ordered]@{
        id = "body-unmerge-down"
        sample_case_id = "body-unmerge-down"
        description = "Split an already merged body-table pillar while the retained blue witness table remains unchanged."
        command = "unmerge-template-table-cells"
        part = "body"
        part_args = @("--part", "body")
        target_table_index = "1"
        row_index = "1"
        cell_index = "0"
        command_args = @("--direction", "down")
        baseline_rows = @(
            @("Merged orange pillar", "Body top"),
            @("", "Body middle"),
            @("", "Body bottom")
        )
        mutated_rows = @(
            @("Merged orange pillar", "Body top"),
            @("", "Body middle"),
            @("", "Body bottom")
        )
        xml_entry = "word/document.xml"
        baseline_xml_contains = @("w:vMerge w:val=""restart""", "w:vMerge w:val=""continue""")
        baseline_xml_not_contains = @()
        mutated_xml_contains = @()
        mutated_xml_not_contains = @("w:vMerge")
        expected_visual_cues = @(
            "The retained blue body witness table remains unchanged above the orange target table.",
            "The orange pillar is split back into stacked standalone cells with visible horizontal borders.",
            "The paragraph after the body target table remains below the table and keeps its spacing."
        )
    }
)

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateFirstPagesDir = Join-Path $aggregateEvidenceDir "first-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
$summaryPath = Join-Path $resolvedOutputDir "summary.json"
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
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"

    Write-Step "Building CLI and merge/unmerge visual sample"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_template_table_cli_merge_unmerge_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_template_table_cli_merge_unmerge_visual"

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    cli_executable = $cliExecutable
    sample_executable = $sampleExecutable
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
    summary_json = $summaryPath
    review_checklist = $reviewChecklistPath
    final_review = $finalReviewPath
    cases = @()
}

$aggregateImagePaths = @()
$aggregateLabels = @()

foreach ($case in $cases) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    $baselineDocxPath = Join-Path $caseDir "$($case.id)-baseline.docx"
    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $baselineRowsJsonPath = Join-Path $caseDir "baseline_rows.json"
    $mutatedRowsJsonPath = Join-Path $caseDir "mutated_rows.json"
    $commandJsonPath = Join-Path $caseDir "command.json"
    $commandSequencePath = Join-Path $caseDir "command_sequence.md"
    $baselineVisualDir = Join-Path $caseDir "baseline-visual"
    $mutatedVisualDir = Join-Path $caseDir "mutated-visual"
    $caseContactSheetPath = Join-Path $caseDir "before_after_contact_sheet.png"
    $baselineVisualRelativeDir = Join-Path $OutputDir (Join-Path $case.id "baseline-visual")
    $mutatedVisualRelativeDir = Join-Path $OutputDir (Join-Path $case.id "mutated-visual")

    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    Write-Step "Generating baseline DOCX for case '$($case.id)'"
    & $sampleExecutable $case.sample_case_id $baselineDocxPath
    if ($LASTEXITCODE -ne 0) {
        throw "Sample execution failed for case '$($case.id)'."
    }

    Write-Step "Inspecting baseline rows for case '$($case.id)'"
    Invoke-CommandCapture `
        -ExecutablePath $cliExecutable `
        -Arguments (@("inspect-template-table-rows", $baselineDocxPath, $case.target_table_index) + @($case.part_args) + @("--json")) `
        -OutputPath $baselineRowsJsonPath `
        -FailureMessage "Failed to inspect baseline template rows for case '$($case.id)'."

    Assert-TableRows `
        -RowsJsonPath $baselineRowsJsonPath `
        -ExpectedRows $case.baseline_rows `
        -Label "baseline $($case.id) table"

    $baselineXml = Get-DocxEntryText -DocxPath $baselineDocxPath -EntryName $case.xml_entry
    Assert-EntryPatterns `
        -XmlText $baselineXml `
        -MustContain @($case.baseline_xml_contains) `
        -MustNotContain @($case.baseline_xml_not_contains) `
        -Label "baseline $($case.id) $($case.xml_entry)"

    Write-Step "Running $($case.command) for case '$($case.id)'"
    $commandArguments = @(
        $case.command
        $baselineDocxPath
        $case.target_table_index
        $case.row_index
        $case.cell_index
    ) + @($case.part_args) + @($case.command_args) + @("--output", $mutatedDocxPath, "--json")
    Invoke-CommandCapture `
        -ExecutablePath $cliExecutable `
        -Arguments $commandArguments `
        -OutputPath $commandJsonPath `
        -FailureMessage "Failed to mutate template table for case '$($case.id)'."

    $commandLines = [System.Collections.Generic.List[string]]::new()
    [void]$commandLines.Add("# Template table CLI merge/unmerge command sequence for $($case.id)")
    [void]$commandLines.Add("")
    [void]$commandLines.Add("Sample case id: `"$($case.sample_case_id)`"")
    [void]$commandLines.Add("")
    Add-CommandLine -Lines $commandLines -Command $case.command -Arguments (@("<baseline.docx>", $case.target_table_index, $case.row_index, $case.cell_index) + @($case.part_args) + @($case.command_args) + @("--output", "<mutated.docx>", "--json"))
    $commandLines | Set-Content -Path $commandSequencePath -Encoding UTF8

    Write-Step "Inspecting mutated rows for case '$($case.id)'"
    Invoke-CommandCapture `
        -ExecutablePath $cliExecutable `
        -Arguments (@("inspect-template-table-rows", $mutatedDocxPath, $case.target_table_index) + @($case.part_args) + @("--json")) `
        -OutputPath $mutatedRowsJsonPath `
        -FailureMessage "Failed to inspect mutated template rows for case '$($case.id)'."

    Assert-TableRows `
        -RowsJsonPath $mutatedRowsJsonPath `
        -ExpectedRows $case.mutated_rows `
        -Label "mutated $($case.id) table"

    $mutatedXml = Get-DocxEntryText -DocxPath $mutatedDocxPath -EntryName $case.xml_entry
    Assert-EntryPatterns `
        -XmlText $mutatedXml `
        -MustContain @($case.mutated_xml_contains) `
        -MustNotContain @($case.mutated_xml_not_contains) `
        -Label "mutated $($case.id) $($case.xml_entry)"

    $caseSummary = [ordered]@{
        id = $case.id
        sample_case_id = $case.sample_case_id
        description = $case.description
        command = $case.command
        part = $case.part
        target_table_index = [int]$case.target_table_index
        row_index = [int]$case.row_index
        cell_index = [int]$case.cell_index
        expected_visual_cues = @($case.expected_visual_cues)
        baseline_docx = $baselineDocxPath
        baseline_rows_json = $baselineRowsJsonPath
        baseline_rows_expected = $case.baseline_rows
        mutated_docx = $mutatedDocxPath
        mutated_rows_json = $mutatedRowsJsonPath
        mutated_rows_expected = $case.mutated_rows
        xml_entry = $case.xml_entry
        command_json = $commandJsonPath
        command_sequence = $commandSequencePath
    }

    if (-not $SkipVisual) {
        Write-Step "Rendering baseline Word evidence for case '$($case.id)'"
        & $wordSmokeScript `
            -BuildDir $BuildDir `
            -InputDocx $baselineDocxPath `
            -OutputDir $baselineVisualRelativeDir `
            -SkipBuild `
            -Dpi $Dpi `
            -KeepWordOpen:$KeepWordOpen.IsPresent `
            -VisibleWord:$VisibleWord.IsPresent
        if ($LASTEXITCODE -ne 0) {
            throw "Visual smoke failed for baseline document in case '$($case.id)'."
        }

        Write-Step "Rendering mutated Word evidence for case '$($case.id)'"
        & $wordSmokeScript `
            -BuildDir $BuildDir `
            -InputDocx $mutatedDocxPath `
            -OutputDir $mutatedVisualRelativeDir `
            -SkipBuild `
            -Dpi $Dpi `
            -KeepWordOpen:$KeepWordOpen.IsPresent `
            -VisibleWord:$VisibleWord.IsPresent
        if ($LASTEXITCODE -ne 0) {
            throw "Visual smoke failed for mutated document in case '$($case.id)'."
        }

        $baselineFirstPage = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
        $mutatedFirstPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
        if (-not (Test-Path $baselineFirstPage)) {
            throw "Missing baseline first-page PNG for case '$($case.id)': $baselineFirstPage"
        }
        if (-not (Test-Path $mutatedFirstPage)) {
            throw "Missing mutated first-page PNG for case '$($case.id)': $mutatedFirstPage"
        }

        $aggregateBaselinePage = Join-Path $aggregateFirstPagesDir "$($case.id)-baseline-page-01.png"
        $aggregateMutatedPage = Join-Path $aggregateFirstPagesDir "$($case.id)-mutated-page-01.png"
        Copy-Item -Path $baselineFirstPage -Destination $aggregateBaselinePage -Force
        Copy-Item -Path $mutatedFirstPage -Destination $aggregateMutatedPage -Force

        $renderPython = Ensure-RenderPython -RepoRoot $repoRoot
        Write-Step "Building before/after contact sheet for case '$($case.id)'"
        & $renderPython $contactSheetScript `
            --output $caseContactSheetPath `
            --columns 2 `
            --thumb-width 420 `
            --images $aggregateBaselinePage $aggregateMutatedPage `
            --labels "$($case.id)-baseline" "$($case.id)-mutated"
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to build case contact sheet for '$($case.id)'."
        }

        $aggregateImagePaths += $aggregateBaselinePage
        $aggregateImagePaths += $aggregateMutatedPage
        $aggregateLabels += "$($case.id)-baseline"
        $aggregateLabels += "$($case.id)-mutated"

        $caseSummary.visual_artifacts = [ordered]@{
            baseline_visual_output_dir = $baselineVisualDir
            baseline_first_page = $aggregateBaselinePage
            mutated_visual_output_dir = $mutatedVisualDir
            mutated_first_page = $aggregateMutatedPage
            before_after_contact_sheet = $caseContactSheetPath
        }
    }

    $summary.cases += $caseSummary
    $reviewManifest.cases += $caseSummary
}

if (-not $SkipVisual) {
    $renderPython = Ensure-RenderPython -RepoRoot $repoRoot
    Write-Step "Building aggregate before/after contact sheet"
    $contactSheetArgs = @(
        $contactSheetScript
        "--output"
        $aggregateContactSheetPath
        "--columns"
        "2"
        "--thumb-width"
        "420"
        "--images"
    ) + $aggregateImagePaths + @("--labels") + $aggregateLabels

    & $renderPython @contactSheetArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build aggregate before/after contact sheet."
    }

    $summary.aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        first_pages_dir = $aggregateFirstPagesDir
        contact_sheet = $aggregateContactSheetPath
    }
    $reviewManifest.aggregate_evidence = $summary.aggregate_evidence
}

($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8
($reviewManifest | ConvertTo-Json -Depth 8) | Set-Content -Path $reviewManifestPath -Encoding UTF8

$reviewChecklistLines = [System.Collections.Generic.List[string]]::new()
[void]$reviewChecklistLines.Add("# Template table CLI merge/unmerge visual regression checklist")
[void]$reviewChecklistLines.Add("")
if (-not $SkipVisual) {
    [void]$reviewChecklistLines.Add("- Start with the aggregate before/after contact sheet:")
    [void]$reviewChecklistLines.Add("  $aggregateContactSheetPath")
    [void]$reviewChecklistLines.Add("- Then inspect each case's case-specific contact sheet and `page-01.png`.")
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
[void]$reviewChecklistLines.Add("- Review manifest: $reviewManifestPath")
foreach ($caseSummary in $summary.cases) {
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) baseline DOCX: $($caseSummary.baseline_docx)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)")
    [void]$reviewChecklistLines.Add("- $($caseSummary.id) command sequence: $($caseSummary.command_sequence)")
}
$reviewChecklistLines | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReviewLines = [System.Collections.Generic.List[string]]::new()
[void]$finalReviewLines.Add("# Template table CLI merge/unmerge final review")
[void]$finalReviewLines.Add("")
[void]$finalReviewLines.Add("- Generated at: $(Get-Date -Format s)")
[void]$finalReviewLines.Add("- Summary JSON: $summaryPath")
[void]$finalReviewLines.Add("- Review manifest: $reviewManifestPath")
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

Write-Step "Completed template table CLI merge/unmerge visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Review manifest: $reviewManifestPath"
Write-Host "Review checklist: $reviewChecklistPath"
Write-Host "Final review: $finalReviewPath"
foreach ($caseSummary in $summary.cases) {
    Write-Host "$($caseSummary.id) baseline DOCX: $($caseSummary.baseline_docx)"
    Write-Host "$($caseSummary.id) mutated DOCX: $($caseSummary.mutated_docx)"
}
if (-not $SkipVisual) {
    Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
}
