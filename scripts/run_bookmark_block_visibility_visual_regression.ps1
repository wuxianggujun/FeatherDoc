param(
    [string]$BuildDir = "build-codex-clang-compat",
    [string]$OutputDir = "output/bookmark-block-visibility-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[bookmark-block-visibility-visual] $Message"
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

function Assert-NotContains {
    param(
        [string]$Path,
        [string]$Unexpected,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content.Contains($Unexpected)) {
        throw "$Label unexpectedly contained text: $Unexpected"
    }
}

function Assert-TemplateTableState {
    param(
        [string]$JsonPath,
        [int]$ExpectedCount,
        [string[]]$ExpectedTexts,
        [string[]]$UnexpectedTexts,
        [string]$Label
    )

    $payload = Get-Content -Raw -LiteralPath $JsonPath | ConvertFrom-Json
    if ($null -eq $payload.count) {
        throw "$Label did not contain a table count."
    }
    if ([int]$payload.count -ne $ExpectedCount) {
        throw "$Label expected table count $ExpectedCount, got $($payload.count)."
    }

    $tableTexts = @()
    if ($null -ne $payload.tables) {
        $tableTexts = @($payload.tables | ForEach-Object { $_.text })
    } elseif ($null -ne $payload.table) {
        $tableTexts = @($payload.table.text)
    }

    foreach ($expected in $ExpectedTexts) {
        if (-not ($tableTexts -contains $expected)) {
            throw "$Label did not contain expected table text: $expected"
        }
    }
    foreach ($unexpected in $UnexpectedTexts) {
        if ($tableTexts -contains $unexpected) {
            throw "$Label unexpectedly contained table text: $unexpected"
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
    & cmake -S $repoRoot -B $resolvedBuildDir -DBUILD_SAMPLES=ON
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to configure the build directory."
    }

    Write-Step "Building featherdoc_cli and bookmark block visibility sample"
    & cmake --build $resolvedBuildDir --target featherdoc_cli featherdoc_sample_bookmark_block_visibility_visual -- -j4
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build bookmark block visibility visual regression prerequisites."
    }
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_bookmark_block_visibility_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

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
    -FailureMessage "Failed to generate shared bookmark block visibility baseline sample."

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

$cases = @(
    [ordered]@{
        id = "body-keep-bookmark-block-visual"
        command = "set-bookmark-block-visibility"
        mutation_args = @("keep_block", "--visible", "true")
        expected_json = @(
            '"command":"set-bookmark-block-visibility"',
            '"bookmark":{"bookmark_name":"keep_block"',
            '"visible":true',
            '"changed":1'
        )
        paragraph_contains = @(
            "part: body",
            "paragraphs: 7",
            "text=before",
            "text=Keep me",
            "text=middle",
            "text=Hide me",
            "text=after"
        )
        paragraph_not_contains = @()
        expected_table_count = 1
        expected_table_texts = @("Secret Cell")
        unexpected_table_texts = @()
        expected_visual_cues = @(
            "The keep_block marker paragraphs disappear while Keep me remains visible.",
            "The hidden block still renders its table and Hide me paragraph.",
            "The surrounding before / middle / after paragraphs keep their order."
        )
    },
    [ordered]@{
        id = "body-hide-bookmark-block-visual"
        command = "set-bookmark-block-visibility"
        mutation_args = @("hide_block", "--visible", "false")
        expected_json = @(
            '"command":"set-bookmark-block-visibility"',
            '"bookmark":{"bookmark_name":"hide_block"',
            '"visible":false',
            '"changed":1'
        )
        paragraph_contains = @(
            "part: body",
            "paragraphs: 6",
            "text=before",
            "text=Keep me",
            "text=middle",
            "text=after"
        )
        paragraph_not_contains = @("text=Hide me")
        expected_table_count = 0
        expected_table_texts = @()
        unexpected_table_texts = @("Secret Cell")
        expected_visual_cues = @(
            "The hide_block table and Hide me paragraph disappear completely.",
            "The keep_block content remains present with its original surrounding spacing.",
            "The before / middle / after paragraphs stay readable without overlap."
        )
    },
    [ordered]@{
        id = "body-apply-bookmark-block-visibility-visual"
        command = "apply-bookmark-block-visibility"
        mutation_args = @("--show", "keep_block", "--hide", "hide_block")
        expected_json = @(
            '"command":"apply-bookmark-block-visibility"',
            '"complete":true',
            '"requested":2',
            '"matched":2',
            '"kept":1',
            '"removed":1'
        )
        paragraph_contains = @(
            "part: body",
            "paragraphs: 4",
            "text=before",
            "text=Keep me",
            "text=middle",
            "text=after"
        )
        paragraph_not_contains = @("text=Hide me")
        expected_table_count = 0
        expected_table_texts = @()
        unexpected_table_texts = @("Secret Cell")
        expected_visual_cues = @(
            "The keep_block markers disappear while Keep me remains.",
            "The hide_block table and Hide me paragraph are both removed.",
            "The final page shows a compact before / Keep me / middle / after flow."
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
    $mutationJsonPath = Join-Path $caseOutputDir "mutation_result.json"
    $paragraphInspectPath = Join-Path $caseOutputDir "inspect_paragraphs.txt"
    $tableInspectPath = Join-Path $caseOutputDir "inspect_tables.json"

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments (@(
                $case.command,
                $sharedBaselineDocxPath
            ) + @($case.mutation_args) + @(
                "--output",
                $mutatedDocxPath,
                "--json"
            )) `
        -OutputPath $mutationJsonPath `
        -FailureMessage "Failed to mutate bookmark block visibility for case '$caseId'."

    foreach ($expected in $case.expected_json) {
        Assert-Contains -Path $mutationJsonPath -Expected $expected -Label $caseId
    }

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @(
            "inspect-template-paragraphs",
            $mutatedDocxPath
        ) `
        -OutputPath $paragraphInspectPath `
        -FailureMessage "Failed to inspect template paragraphs for case '$caseId'."

    foreach ($expected in $case.paragraph_contains) {
        Assert-Contains -Path $paragraphInspectPath -Expected $expected -Label $caseId
    }
    foreach ($unexpected in $case.paragraph_not_contains) {
        Assert-NotContains -Path $paragraphInspectPath -Unexpected $unexpected -Label $caseId
    }

    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @(
            "inspect-template-tables",
            $mutatedDocxPath,
            "--json"
        ) `
        -OutputPath $tableInspectPath `
        -FailureMessage "Failed to inspect template tables for case '$caseId'."

    Assert-TemplateTableState `
        -JsonPath $tableInspectPath `
        -ExpectedCount $case.expected_table_count `
        -ExpectedTexts $case.expected_table_texts `
        -UnexpectedTexts $case.unexpected_table_texts `
        -Label $caseId

    $caseSummary = [ordered]@{
        id = $caseId
        command = $case.command
        mutated_docx = $mutatedDocxPath
        mutation_json = $mutationJsonPath
        paragraph_inspect = $paragraphInspectPath
        table_inspect = $tableInspectPath
        expected_visual_cues = $case.expected_visual_cues
    }

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

    $caseSummaries += $caseSummary
}

if ($aggregateImages.Count -gt 0) {
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
    visual_enabled = $true
    shared_baseline_docx = $sharedBaselineDocxPath
    cases = $caseSummaries
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed bookmark block visibility visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
