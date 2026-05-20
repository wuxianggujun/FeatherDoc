param(
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$OutputJson = "",
    [string]$CTestExecutable = "ctest",
    [switch]$Strict
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

function Resolve-PreferredBuildDir {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $requestedPath = Resolve-RepoPath -RepoRoot $RepoRoot -InputPath $InputPath
    if ($InputPath -ne ".bpdf-roundtrip-msvc" -or
        (Test-Path -LiteralPath $requestedPath -PathType Container)) {
        return [pscustomobject]@{
            Path = $requestedPath
            Source = "requested"
            RequestedPath = $requestedPath
        }
    }

    foreach ($candidate in @("build", "out\build")) {
        $candidatePath = Resolve-RepoPath -RepoRoot $RepoRoot -InputPath $candidate
        if (Test-Path -LiteralPath $candidatePath -PathType Container) {
            return [pscustomobject]@{
                Path = $candidatePath
                Source = "auto:$candidate"
                RequestedPath = $requestedPath
            }
        }
    }

    return [pscustomobject]@{
        Path = $requestedPath
        Source = "requested"
        RequestedPath = $requestedPath
    }
}

function New-CheckResult {
    param(
        [string]$Name,
        [string]$Status,
        [bool]$Required,
        [string]$Message,
        [string]$Path = "",
        [object]$Details = $null
    )

    $result = [ordered]@{
        name = $Name
        status = $Status
        required = $Required
        message = $Message
    }
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        $result.path = $Path
    }
    if ($null -ne $Details) {
        $result.details = $Details
    }
    return $result
}

function Add-CheckResult {
    param(
        [System.Collections.ArrayList]$Checks,
        [string]$Name,
        [string]$Status,
        [bool]$Required,
        [string]$Message,
        [string]$Path = "",
        [object]$Details = $null
    )

    $Checks.Add((New-CheckResult `
        -Name $Name `
        -Status $Status `
        -Required $Required `
        -Message $Message `
        -Path $Path `
        -Details $Details)) | Out-Null
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

function Test-JsonBooleanProperty {
    param(
        [object]$Object,
        [string]$Name
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $false
    }
    return $property.Value -eq $true
}

function Get-JsonPropertyValue {
    param(
        [object]$Object,
        [string]$Name,
        [object]$DefaultValue = $null
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $DefaultValue
    }
    return $property.Value
}

function Get-BuildDirectorySnapshot {
    param([string]$Path)

    $exists = Test-Path -LiteralPath $Path -PathType Container
    $cmakeCachePath = Join-Path $Path "CMakeCache.txt"
    $ctestManifestPath = Join-Path $Path "CTestTestfile.cmake"
    $entries = @()
    $entryCount = 0

    if ($exists) {
        $allEntries = @(Get-ChildItem -LiteralPath $Path -Force -ErrorAction SilentlyContinue)
        $entryCount = $allEntries.Count
        $entries = @(
            $allEntries |
                Sort-Object Name |
                Select-Object -First 20 |
                ForEach-Object {
                    [ordered]@{
                        name = $_.Name
                        type = if ($_.PSIsContainer) { "directory" } else { "file" }
                        bytes = if ($_.PSIsContainer) { $null } else { $_.Length }
                    }
                }
        )
    }

    return [ordered]@{
        cmake_cache_path = $cmakeCachePath
        cmake_cache_exists = Test-Path -LiteralPath $cmakeCachePath -PathType Leaf
        ctest_manifest_path = $ctestManifestPath
        ctest_manifest_exists = Test-Path -LiteralPath $ctestManifestPath -PathType Leaf
        entry_count = $entryCount
        entries_preview = $entries
    }
}

$repoRoot = Resolve-RepoRoot
$buildDirSelection = Resolve-PreferredBuildDir -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedBuildDir = $buildDirSelection.Path
$checks = New-Object System.Collections.ArrayList

$buildDirExists = Test-Path -LiteralPath $resolvedBuildDir -PathType Container
$buildDirectorySnapshot = Get-BuildDirectorySnapshot -Path $resolvedBuildDir
Add-CheckResult `
    -Checks $checks `
    -Name "build_dir_exists" `
    -Status $(if ($buildDirExists) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($buildDirExists) { "Build directory exists." } else { "Build directory is missing; full PDF visual release gate would need a reusable PDF build." }) `
    -Path $resolvedBuildDir `
    -Details $buildDirectorySnapshot

$cmakeCachePath = Join-Path $resolvedBuildDir "CMakeCache.txt"
$cmakeCacheExists = Test-Path -LiteralPath $cmakeCachePath -PathType Leaf
Add-CheckResult `
    -Checks $checks `
    -Name "cmake_cache_exists" `
    -Status $(if ($cmakeCacheExists) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($cmakeCacheExists) { "CMakeCache.txt exists in the build directory." } else { "CMakeCache.txt is missing; the selected directory is not a reusable CMake build." }) `
    -Path $cmakeCachePath `
    -Details $buildDirectorySnapshot

$ctestFilePath = Join-Path $resolvedBuildDir "CTestTestfile.cmake"
$ctestFileExists = Test-Path -LiteralPath $ctestFilePath -PathType Leaf
Add-CheckResult `
    -Checks $checks `
    -Name "ctest_manifest_exists" `
    -Status $(if ($ctestFileExists) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($ctestFileExists) { "CTest manifest exists in the build directory." } else { "CTestTestfile.cmake is missing; ctest -N cannot prove PDF tests are registered." }) `
    -Path $ctestFilePath `
    -Details $buildDirectorySnapshot

$requiredCTestPatterns = @(
    "pdf_cli_export",
    "pdf_regression_",
    "pdf_visual_release_gate_style_baselines",
    "pdf_visual_release_gate_text_shaping_baselines"
)

if ($buildDirExists -and $cmakeCacheExists -and $ctestFileExists) {
    $ctestOutput = @()
    $ctestExitCode = 0
    try {
        $ctestOutput = @(& $CTestExecutable --test-dir $resolvedBuildDir -N 2>&1 | ForEach-Object { $_.ToString() })
        $ctestExitCode = $LASTEXITCODE
    } catch {
        $ctestExitCode = 1
        $ctestOutput = @($_.Exception.Message)
    }

    $ctestText = $ctestOutput -join "`n"
    $missingCTestPatterns = @(
        $requiredCTestPatterns | Where-Object { $ctestText -notmatch [regex]::Escape($_) }
    )
    Add-CheckResult `
        -Checks $checks `
        -Name "ctest_list_contains_pdf_gate_tests" `
        -Status $(if ($ctestExitCode -eq 0 -and $missingCTestPatterns.Count -eq 0) { "pass" } else { "missing" }) `
        -Required $true `
        -Message $(if ($ctestExitCode -eq 0 -and $missingCTestPatterns.Count -eq 0) { "ctest -N lists the PDF visual gate prerequisites." } else { "ctest -N did not list every PDF visual gate prerequisite." }) `
        -Details ([ordered]@{
            ctest_executable = $CTestExecutable
            exit_code = $ctestExitCode
            required_patterns = $requiredCTestPatterns
            missing_patterns = $missingCTestPatterns
        })
} else {
    Add-CheckResult `
        -Checks $checks `
        -Name "ctest_list_contains_pdf_gate_tests" `
        -Status "skipped" `
        -Required $true `
        -Message "Skipped ctest -N because the build directory, CMakeCache.txt, or CTest manifest is missing." `
        -Details ([ordered]@{
            required_patterns = $requiredCTestPatterns
        })
}

$requiredScripts = @(
    "scripts\run_pdf_visual_release_gate.ps1",
    "scripts\render_pdf_pages.py",
    "scripts\check_pdf_text_layer.py",
    "scripts\build_image_contact_sheet.py",
    "scripts\run_pdf_unicode_font_roundtrip_visual_regression.ps1"
)

$missingScripts = New-Object System.Collections.ArrayList
foreach ($relativePath in $requiredScripts) {
    $scriptPath = Join-Path $repoRoot $relativePath
    if (-not (Test-Path -LiteralPath $scriptPath -PathType Leaf)) {
        $missingScripts.Add($relativePath) | Out-Null
    }
}
Add-CheckResult `
    -Checks $checks `
    -Name "pdf_visual_gate_scripts_exist" `
    -Status $(if ($missingScripts.Count -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($missingScripts.Count -eq 0) { "All PDF visual gate helper scripts exist." } else { "Some PDF visual gate helper scripts are missing." }) `
    -Details ([ordered]@{
        required_scripts = $requiredScripts
        missing_scripts = @($missingScripts)
    })

$manifestPath = Join-Path $repoRoot "test\pdf_regression_manifest.json"
$manifestExists = Test-Path -LiteralPath $manifestPath -PathType Leaf
Add-CheckResult `
    -Checks $checks `
    -Name "pdf_regression_manifest_exists" `
    -Status $(if ($manifestExists) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($manifestExists) { "PDF regression manifest exists." } else { "PDF regression manifest is missing." }) `
    -Path $manifestPath

$visualSamples = @()
$cjkSamples = @()
if ($manifestExists) {
    $manifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath | ConvertFrom-Json
    $visualSamples = @($manifest.samples | Where-Object { Test-JsonBooleanProperty -Object $_ -Name "expect_visual_baseline" })
    $cjkSamples = @($manifest.samples | Where-Object { Test-JsonBooleanProperty -Object $_ -Name "expect_cjk" })
}

$cliExpectedPdfs = @(
    "test\pdf_cli_export\font-map-source.pdf",
    "test\pdf_cli_export\cjk-font-source.pdf"
)
$missingCliPdfs = @(
    $cliExpectedPdfs | Where-Object {
        -not (Test-Path -LiteralPath (Join-Path $resolvedBuildDir $_) -PathType Leaf)
    }
)
Add-CheckResult `
    -Checks $checks `
    -Name "pdf_cli_export_baseline_pdfs_exist" `
    -Status $(if ($buildDirExists -and $missingCliPdfs.Count -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($buildDirExists -and $missingCliPdfs.Count -eq 0) { "CLI PDF export baseline PDFs exist." } else { "CLI PDF export baseline PDFs are not ready." }) `
    -Details ([ordered]@{
        required_paths = $cliExpectedPdfs
        missing_paths = $missingCliPdfs
    })

$missingVisualPdfs = New-Object System.Collections.ArrayList
foreach ($sample in $visualSamples) {
    $outputFile = [string](Get-JsonPropertyValue -Object $sample -Name "output_file" -DefaultValue "")
    if ([string]::IsNullOrWhiteSpace($outputFile)) {
        continue
    }
    $relativePath = "test\$outputFile"
    if (-not (Test-Path -LiteralPath (Join-Path $resolvedBuildDir $relativePath) -PathType Leaf)) {
        $missingVisualPdfs.Add($relativePath) | Out-Null
    }
}
Add-CheckResult `
    -Checks $checks `
    -Name "visual_baseline_manifest_pdfs_exist" `
    -Status $(if ($manifestExists -and $buildDirExists -and $missingVisualPdfs.Count -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($manifestExists -and $buildDirExists -and $missingVisualPdfs.Count -eq 0) { "All manifest visual baseline PDFs exist." } else { "Manifest visual baseline PDFs are not ready." }) `
    -Details ([ordered]@{
        sample_count = $visualSamples.Count
        missing_count = $missingVisualPdfs.Count
        missing_paths_preview = @($missingVisualPdfs | Select-Object -First 20)
    })

$missingCjkPdfs = New-Object System.Collections.ArrayList
foreach ($sample in $cjkSamples) {
    $outputFile = [string](Get-JsonPropertyValue -Object $sample -Name "output_file" -DefaultValue "")
    if ([string]::IsNullOrWhiteSpace($outputFile)) {
        continue
    }
    $relativePath = "test\$outputFile"
    if (-not (Test-Path -LiteralPath (Join-Path $resolvedBuildDir $relativePath) -PathType Leaf)) {
        $missingCjkPdfs.Add($relativePath) | Out-Null
    }
}
Add-CheckResult `
    -Checks $checks `
    -Name "cjk_text_layer_manifest_pdfs_exist" `
    -Status $(if ($manifestExists -and $buildDirExists -and $missingCjkPdfs.Count -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($manifestExists -and $buildDirExists -and $missingCjkPdfs.Count -eq 0) { "All manifest CJK text-layer PDFs exist." } else { "Manifest CJK text-layer PDFs are not ready." }) `
    -Details ([ordered]@{
        sample_count = $cjkSamples.Count
        missing_count = $missingCjkPdfs.Count
        missing_paths_preview = @($missingCjkPdfs | Select-Object -First 20)
    })

$pythonCandidates = New-Object System.Collections.ArrayList
if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE)) {
    $pythonCandidates.Add([ordered]@{
        source = "FEATHERDOC_RENDER_PYTHON_EXECUTABLE"
        path = $env:FEATHERDOC_RENDER_PYTHON_EXECUTABLE
    }) | Out-Null
}
$pythonCandidates.Add([ordered]@{
    source = ".venv-word-visual-smoke"
    path = (Join-Path $repoRoot ".venv-word-visual-smoke\Scripts\python.exe")
}) | Out-Null
$pythonCandidates.Add([ordered]@{
    source = "tmp/render-venv"
    path = (Join-Path $repoRoot "tmp\render-venv\Scripts\python.exe")
}) | Out-Null
$pythonCandidates.Add([ordered]@{
    source = ".venv-pdf-visual-smoke-existing"
    path = (Join-Path $repoRoot ".venv-pdf-visual-smoke\Scripts\python.exe")
}) | Out-Null

$pythonCandidateDetails = New-Object System.Collections.ArrayList
$selectedPython = $null
foreach ($candidate in $pythonCandidates) {
    $candidatePath = [string]$candidate.path
    $exists = -not [string]::IsNullOrWhiteSpace($candidatePath) -and (Test-Path -LiteralPath $candidatePath -PathType Leaf)
    $hasPil = $false
    $hasFitz = $false
    if ($exists) {
        $resolvedCandidatePath = (Resolve-Path $candidatePath).Path
        $hasPil = Test-PythonImport -PythonCommand $resolvedCandidatePath -ModuleName "PIL"
        $hasFitz = Test-PythonImport -PythonCommand $resolvedCandidatePath -ModuleName "fitz"
        if ($null -eq $selectedPython -and $hasPil -and $hasFitz) {
            $selectedPython = [ordered]@{
                source = $candidate.source
                path = $resolvedCandidatePath
            }
        }
        $candidatePath = $resolvedCandidatePath
    }
    $pythonCandidateDetails.Add([ordered]@{
        source = $candidate.source
        path = $candidatePath
        exists = $exists
        has_pil = $hasPil
        has_fitz = $hasFitz
    }) | Out-Null
}
$renderPythonStatus = "missing"
$renderPythonMessage = "No existing render Python with PIL and fitz was found; preflight did not create or install anything."
if ($null -ne $selectedPython) {
    $renderPythonStatus = "pass"
    $renderPythonMessage = "A reusable render Python with PIL and fitz is available."
}

$selectedPythonSource = ""
$selectedPythonPath = ""
if ($null -ne $selectedPython) {
    $selectedPythonSource = [string]$selectedPython.source
    $selectedPythonPath = [string]$selectedPython.path
}
$checks.Add([ordered]@{
    name = "render_python_reusable"
    status = $renderPythonStatus
    required = $true
    message = $renderPythonMessage
    details = [ordered]@{
        selected_python_source = $selectedPythonSource
        selected_python_path = $selectedPythonPath
        candidates = @($pythonCandidateDetails)
    }
}) | Out-Null

$blockingChecks = @(
    $checks | Where-Object { $_.required -eq $true -and $_.status -ne "pass" }
)
$status = if ($blockingChecks.Count -eq 0) { "ready" } else { "not_ready" }
$blockingSummary = [ordered]@{
    required_check_count = @($checks | Where-Object { $_.required -eq $true }).Count
    blocking_check_count = $blockingChecks.Count
    missing_cli_pdf_count = $missingCliPdfs.Count
    visual_baseline_sample_count = $visualSamples.Count
    missing_visual_baseline_pdf_count = $missingVisualPdfs.Count
    cjk_text_layer_sample_count = $cjkSamples.Count
    missing_cjk_text_layer_pdf_count = $missingCjkPdfs.Count
    build_dir_entry_count = [int]$buildDirectorySnapshot.entry_count
    ctest_required_pattern_count = $requiredCTestPatterns.Count
}
$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    status = $status
    strict = [bool]$Strict
    repo_root = $repoRoot
    build_dir = $resolvedBuildDir
    build_dir_source = $buildDirSelection.Source
    requested_build_dir = $buildDirSelection.RequestedPath
    blocking_summary = $blockingSummary
    checks = @($checks)
    blocking_checks = @($blockingChecks | ForEach-Object { $_.name })
}

if (-not [string]::IsNullOrWhiteSpace($OutputJson)) {
    $resolvedOutputJson = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputJson
    $outputDir = Split-Path -Parent $resolvedOutputJson
    if (-not [string]::IsNullOrWhiteSpace($outputDir)) {
        New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    }
    ($summary | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8
}

$summary | ConvertTo-Json -Depth 16
if ($Strict -and $status -ne "ready") {
    exit 1
}
