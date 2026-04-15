param(
    [string]$BuildDir = "build-page-number-fields-regression-nmake",
    [string]$OutputDir = "output/page-number-fields-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[page-number-fields-regression] $Message"
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

function Get-DocxFieldSummary {
    param([string]$DocxPath)

    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $entrySummaries = @()
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        $entries = @(
            $archive.Entries |
                Where-Object {
                    $_.FullName -eq "word/document.xml" -or
                    $_.FullName -match "^word/header\d+\.xml$" -or
                    $_.FullName -match "^word/footer\d+\.xml$"
                } |
                Sort-Object FullName
        )

        foreach ($entry in $entries) {
            $reader = [System.IO.StreamReader]::new($entry.Open())
            try {
                $content = $reader.ReadToEnd()
            } finally {
                $reader.Dispose()
            }

            $pageFieldCount = ([regex]::Matches(
                    $content,
                    'w:instr="[^"]*\bPAGE\b[^"]*"')).Count
            $totalPagesFieldCount = ([regex]::Matches(
                    $content,
                    'w:instr="[^"]*\bNUMPAGES\b[^"]*"')).Count

            if ($pageFieldCount -gt 0 -or $totalPagesFieldCount -gt 0) {
                $entrySummaries += [pscustomobject][ordered]@{
                    entry_name = $entry.FullName
                    page_field_count = $pageFieldCount
                    total_pages_field_count = $totalPagesFieldCount
                }
            }
        }
    } finally {
        $archive.Dispose()
    }

    $totalPageFieldCount = 0
    $totalPagesFieldCount = 0
    if ($entrySummaries.Count -gt 0) {
        $totalPageFieldCount = [int](($entrySummaries |
                    Measure-Object -Property page_field_count -Sum).Sum)
        $totalPagesFieldCount = [int](($entrySummaries |
                    Measure-Object -Property total_pages_field_count -Sum).Sum)
    }

    return [ordered]@{
        docx_path = $DocxPath
        total_page_field_count = $totalPageFieldCount
        total_total_pages_field_count = $totalPagesFieldCount
        entries = $entrySummaries
    }
}

function Invoke-VisualSmokeCase {
    param(
        [string]$WordSmokeScript,
        [string]$BuildDirArgument,
        [string]$DocxPath,
        [string]$VisualRelativeDir,
        [int]$Dpi,
        [bool]$KeepWordOpen,
        [bool]$VisibleWord,
        [string]$FailureMessage,
        [string]$VisualDir
    )

    & $WordSmokeScript `
        -BuildDir $BuildDirArgument `
        -InputDocx $DocxPath `
        -OutputDir $VisualRelativeDir `
        -SkipBuild `
        -Dpi $Dpi `
        -KeepWordOpen:$KeepWordOpen `
        -VisibleWord:$VisibleWord
    if ($LASTEXITCODE -ne 0) {
        throw $FailureMessage
    }

    return [ordered]@{
        root = $VisualDir
        evidence_dir = Join-Path $VisualDir "evidence"
        pages_dir = Join-Path $VisualDir "evidence\pages"
        contact_sheet = Join-Path $VisualDir "evidence\contact_sheet.png"
        report_dir = Join-Path $VisualDir "report"
        summary_json = Join-Path $VisualDir "report\summary.json"
        review_result = Join-Path $VisualDir "report\review_result.json"
        final_review = Join-Path $VisualDir "report\final_review.md"
    }
}

function Invoke-FieldAppendMutation {
    param(
        [string]$CliExecutable,
        [string]$InputDocxPath,
        [string]$OutputDocxPath,
        [string]$Command,
        [string]$Part,
        [int]$Section,
        [string]$OutputJsonPath
    )

    Invoke-CommandCapture `
        -ExecutablePath $CliExecutable `
        -Arguments @(
            $Command,
            $InputDocxPath,
            "--part",
            $Part,
            "--section",
            $Section.ToString(),
            "--output",
            $OutputDocxPath,
            "--json"
        ) `
        -OutputPath $OutputJsonPath `
        -FailureMessage "Failed to run $Command for section $Section."

    return [ordered]@{
        command = $Command
        part = $Part
        section = $Section
        output_docx = $OutputDocxPath
        output_json = $OutputJsonPath
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"

$cases = @(
    [ordered]@{
        id = "api-sample"
        description = "API sample writes PAGE into the header and NUMPAGES into the footer."
        expected_visual_cues = @(
            "The header text should show 'Current page' followed by a rendered page number.",
            "The footer text should show 'Total pages' followed by the rendered total page count.",
            "The generated document should render cleanly without clipped header or footer text."
        )
    },
    [ordered]@{
        id = "cli-append"
        description = "CLI appends PAGE and NUMPAGES fields to both sections of the two-page sample."
        expected_visual_cues = @(
            "Page 1 and page 2 should both show visible header/footer field output after the CLI mutations.",
            "The rendered page numbers should differ across pages while total pages stays consistent.",
            "Adding fields through the CLI must not disturb the existing portrait/landscape section geometry."
        )
    }
)

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateContactSheetsDir = Join-Path $aggregateEvidenceDir "contact-sheets"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "contact_sheet.png"
$reviewManifestPath = Join-Path $resolvedOutputDir "review_manifest.json"
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"

if (-not $SkipVisual) {
    New-Item -ItemType Directory -Path $aggregateEvidenceDir -Force | Out-Null
    New-Item -ItemType Directory -Path $aggregateContactSheetsDir -Force | Out-Null
}

if (-not $SkipBuild) {
    $vcvarsPath = Get-VcvarsPath
    Write-Step "Configuring dedicated build directory $resolvedBuildDir"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"

    Write-Step "Building page number field regression targets"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_sample_page_number_fields featherdoc_sample_section_page_setup featherdoc_cli"
}

$pageNumberSampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_page_number_fields"
$sectionSampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_section_page_setup"
$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"

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
    aggregate_evidence = if ($SkipVisual) {
        $null
    } else {
        [ordered]@{
            root = $aggregateEvidenceDir
            contact_sheets_dir = $aggregateContactSheetsDir
            contact_sheet = $aggregateContactSheetPath
        }
    }
    cases = @()
}

$apiCaseDir = Join-Path $resolvedOutputDir "api-sample"
$apiDocxPath = Join-Path $apiCaseDir "page_number_fields.docx"
$apiFieldSummaryPath = Join-Path $apiCaseDir "field_summary.json"
$apiVisualDir = Join-Path $apiCaseDir "visual"
$apiVisualRelativeDir = Join-Path $OutputDir (Join-Path "api-sample" "visual")

New-Item -ItemType Directory -Path $apiCaseDir -Force | Out-Null

Write-Step "Generating API sample document via $pageNumberSampleExecutable"
& $pageNumberSampleExecutable $apiDocxPath
if ($LASTEXITCODE -ne 0) {
    throw "Sample execution failed: featherdoc_sample_page_number_fields"
}

$apiFieldSummary = Get-DocxFieldSummary -DocxPath $apiDocxPath
($apiFieldSummary | ConvertTo-Json -Depth 6) | Set-Content -Path $apiFieldSummaryPath -Encoding UTF8

$apiCaseSummary = [ordered]@{
    id = "api-sample"
    description = $cases[0].description
    expected_visual_cues = @($cases[0].expected_visual_cues)
    sample_executable = $pageNumberSampleExecutable
    docx_path = $apiDocxPath
    field_summary_json = $apiFieldSummaryPath
    visual_output_dir = if ($SkipVisual) { $null } else { $apiVisualDir }
}

if (-not $SkipVisual) {
    Write-Step "Rendering Word evidence for api-sample"
    $apiCaseSummary.visual_artifacts = Invoke-VisualSmokeCase `
        -WordSmokeScript $wordSmokeScript `
        -BuildDirArgument $BuildDir `
        -DocxPath $apiDocxPath `
        -VisualRelativeDir $apiVisualRelativeDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent `
        -FailureMessage "Visual smoke failed for api-sample." `
        -VisualDir $apiVisualDir
}

$summary.cases += $apiCaseSummary
$reviewManifest.cases += $apiCaseSummary

$cliCaseDir = Join-Path $resolvedOutputDir "cli-append"
$cliBaseDocxPath = Join-Path $cliCaseDir "section_page_setup_base.docx"
$cliStep1DocxPath = Join-Path $cliCaseDir "page_number_fields_section_0.docx"
$cliStep2DocxPath = Join-Path $cliCaseDir "page_number_fields_section_0_footer.docx"
$cliStep3DocxPath = Join-Path $cliCaseDir "page_number_fields_section_1.docx"
$cliDocxPath = Join-Path $cliCaseDir "page_number_fields_cli.docx"
$cliFieldSummaryPath = Join-Path $cliCaseDir "field_summary.json"
$cliVisualDir = Join-Path $cliCaseDir "visual"
$cliVisualRelativeDir = Join-Path $OutputDir (Join-Path "cli-append" "visual")

$cliAppendSection0PageJson = Join-Path $cliCaseDir "append_page_number_section_0.json"
$cliAppendSection0TotalJson = Join-Path $cliCaseDir "append_total_pages_section_0.json"
$cliAppendSection1PageJson = Join-Path $cliCaseDir "append_page_number_section_1.json"
$cliAppendSection1TotalJson = Join-Path $cliCaseDir "append_total_pages_section_1.json"

New-Item -ItemType Directory -Path $cliCaseDir -Force | Out-Null

Write-Step "Generating two-section base document via $sectionSampleExecutable"
& $sectionSampleExecutable $cliBaseDocxPath
if ($LASTEXITCODE -ne 0) {
    throw "Sample execution failed: featherdoc_sample_section_page_setup"
}

$cliMutations = @()
$cliMutations += Invoke-FieldAppendMutation `
    -CliExecutable $cliExecutable `
    -InputDocxPath $cliBaseDocxPath `
    -OutputDocxPath $cliStep1DocxPath `
    -Command "append-page-number-field" `
    -Part "section-header" `
    -Section 0 `
    -OutputJsonPath $cliAppendSection0PageJson
$cliMutations += Invoke-FieldAppendMutation `
    -CliExecutable $cliExecutable `
    -InputDocxPath $cliStep1DocxPath `
    -OutputDocxPath $cliStep2DocxPath `
    -Command "append-total-pages-field" `
    -Part "section-footer" `
    -Section 0 `
    -OutputJsonPath $cliAppendSection0TotalJson
$cliMutations += Invoke-FieldAppendMutation `
    -CliExecutable $cliExecutable `
    -InputDocxPath $cliStep2DocxPath `
    -OutputDocxPath $cliStep3DocxPath `
    -Command "append-page-number-field" `
    -Part "section-header" `
    -Section 1 `
    -OutputJsonPath $cliAppendSection1PageJson
$cliMutations += Invoke-FieldAppendMutation `
    -CliExecutable $cliExecutable `
    -InputDocxPath $cliStep3DocxPath `
    -OutputDocxPath $cliDocxPath `
    -Command "append-total-pages-field" `
    -Part "section-footer" `
    -Section 1 `
    -OutputJsonPath $cliAppendSection1TotalJson

$cliFieldSummary = Get-DocxFieldSummary -DocxPath $cliDocxPath
($cliFieldSummary | ConvertTo-Json -Depth 6) | Set-Content -Path $cliFieldSummaryPath -Encoding UTF8

$cliCaseSummary = [ordered]@{
    id = "cli-append"
    description = $cases[1].description
    expected_visual_cues = @($cases[1].expected_visual_cues)
    base_sample_executable = $sectionSampleExecutable
    cli_executable = $cliExecutable
    input_docx_path = $cliBaseDocxPath
    docx_path = $cliDocxPath
    field_summary_json = $cliFieldSummaryPath
    mutations = $cliMutations
    visual_output_dir = if ($SkipVisual) { $null } else { $cliVisualDir }
}

if (-not $SkipVisual) {
    Write-Step "Rendering Word evidence for cli-append"
    $cliCaseSummary.visual_artifacts = Invoke-VisualSmokeCase `
        -WordSmokeScript $wordSmokeScript `
        -BuildDirArgument $BuildDir `
        -DocxPath $cliDocxPath `
        -VisualRelativeDir $cliVisualRelativeDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent `
        -FailureMessage "Visual smoke failed for cli-append." `
        -VisualDir $cliVisualDir
}

$summary.cases += $cliCaseSummary
$reviewManifest.cases += $cliCaseSummary

if (-not $SkipVisual) {
    $renderPython = Ensure-RenderPython -RepoRoot $repoRoot
    $contactSheetPaths = @()
    $contactSheetLabels = @()

    foreach ($caseSummary in $summary.cases) {
        $contactSheetPath = $caseSummary.visual_artifacts.contact_sheet
        if (-not (Test-Path $contactSheetPath)) {
            throw "Missing contact sheet for case '$($caseSummary.id)': $contactSheetPath"
        }

        $aggregateCaseContactSheetPath = Join-Path $aggregateContactSheetsDir "$($caseSummary.id)-contact_sheet.png"
        Copy-Item -Path $contactSheetPath -Destination $aggregateCaseContactSheetPath -Force
        $contactSheetPaths += $aggregateCaseContactSheetPath
        $contactSheetLabels += $caseSummary.id
    }

    $contactSheetArgs = @(
        $contactSheetScript
        "--output"
        $aggregateContactSheetPath
        "--columns"
        "2"
        "--images"
    ) + $contactSheetPaths + @("--labels") + $contactSheetLabels

    & $renderPython @contactSheetArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build aggregate page number fields contact sheet."
    }

    $summary.aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        contact_sheets_dir = $aggregateContactSheetsDir
        contact_sheet = $aggregateContactSheetPath
    }
    $reviewManifest.aggregate_evidence = $summary.aggregate_evidence
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8
($reviewManifest | ConvertTo-Json -Depth 8) | Set-Content -Path $reviewManifestPath -Encoding UTF8

$reviewChecklistLines = @(
    "# Page number fields visual review checklist",
    "",
    "- Start with the aggregate contact sheet when available:",
    "  $aggregateContactSheetPath",
    "- Then inspect each case's contact_sheet.png plus the rendered page images.",
    "",
    "- api-sample: confirm the header renders 'Current page' with a page number and the footer renders 'Total pages' with the total page count.",
    "- cli-append: confirm both pages render visible header/footer field output after the CLI mutations.",
    "- In both cases, the field values should render cleanly with no clipped header/footer text or layout regressions.",
    "",
    "Artifacts:",
    "",
    "- Summary JSON: $summaryPath",
    "- Review manifest: $reviewManifestPath",
    "- Final review: $finalReviewPath"
)
$reviewChecklistLines | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReviewLines = @(
    "# Page number fields final review",
    "",
    "- Generated at: $(Get-Date -Format s)",
    "- Summary JSON: $summaryPath",
    "- Review manifest: $reviewManifestPath",
    "- Aggregate contact sheet: $aggregateContactSheetPath",
    "",
    "## Verdict",
    "",
    "- Overall verdict:",
    "- Notes:",
    "",
    "## Case findings",
    ""
)

foreach ($case in $summary.cases) {
    $finalReviewLines += "- $($case.id):"
    $finalReviewLines += "  Verdict:"
    $finalReviewLines += "  Notes:"
    $finalReviewLines += ""
}

$finalReviewLines | Set-Content -Path $finalReviewPath -Encoding UTF8

Write-Step "Completed page number fields regression run"
Write-Host "Summary: $summaryPath"
Write-Host "Review manifest: $reviewManifestPath"
Write-Host "Review checklist: $reviewChecklistPath"
if (-not $SkipVisual) {
    Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
}
foreach ($case in $summary.cases) {
    Write-Host "Case: $($case.id)"
    Write-Host "  DOCX: $($case.docx_path)"
    Write-Host "  Field summary: $($case.field_summary_json)"
    if ($case.visual_output_dir) {
        Write-Host "  Visual: $($case.visual_output_dir)"
    }
}
