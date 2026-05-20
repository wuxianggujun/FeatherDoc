param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Get-RepoFileText {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected contract file was not found: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

$statusDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\pdf_visual_validation_status_zh.rst"
$buildingPdfDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "BUILDING_PDF.md"
$preflightScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\check_pdf_visual_release_gate_preflight.ps1"
$governanceReportScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_pdf_visual_release_gate_preflight_governance_report.ps1"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
$doNotRunFullVisualGateMarker = [string]::Concat(@(
    [char]0x4E0D,
    [char]0x8981,
    [char]0x76F4,
    [char]0x63A5,
    [char]0x542F,
    [char]0x52A8,
    [char]0x5B8C,
    [char]0x6574,
    " visual gate"
))

$statusMarkers = @(
    "required_check_count = 11",
    "memory_guard_blocked = false",
    "blocking_check_count = 7",
    "workstation_free_memory_available",
    "free_memory_mb",
    "min_free_memory_mb",
    "memory_guard_skipped",
    "memory guard blocked=false",
    "memory guard skipped=false",
    "free memory MB",
    "minimum free memory MB",
    "2026-05-20",
    "release_blocker_count = 1",
    "action_item_count = 1",
    "build_dir_source = requested",
    "output_gap_count = 3",
    ".bpdf-roundtrip-msvc",
    "CMakeCache.txt",
    "CTestTestfile.cmake",
    "out\build",
    "-MinFreeMemoryMB",
    "-SkipMemoryGuard",
    "missing_output_count = 87",
    "fake-pdf-build",
    "fake ctest",
    "fake python",
    "test fixture",
    "reusable release build substitute",
    $doNotRunFullVisualGateMarker
)

foreach ($marker in $statusMarkers) {
    Assert-ContainsText -Text $statusDoc -ExpectedText $marker `
        -Message "PDF visual validation status doc should preserve memory-gate and blocker status marker."
}

$buildingPdfFixtureMarkers = @(
    "fake-pdf-build",
    "fake ctest",
    "fake python",
    "test fixture",
    "reusable release build substitute",
    "CMakeCache.txt",
    "CTestTestfile.cmake",
    "visual baseline PDF",
    "CJK text-layer PDF"
)

foreach ($marker in $buildingPdfFixtureMarkers) {
    Assert-ContainsText -Text $buildingPdfDoc -ExpectedText $marker `
        -Message "BUILDING_PDF.md should preserve the PDF preflight fixture boundary marker."
}

$scriptMarkers = @(
    "workstation_free_memory_available",
    "free_memory_mb",
    "min_free_memory_mb",
    "memory_guard_blocked",
    "memory_guard_skipped",
    "output_gap_count",
    "missing_output_count"
)

foreach ($marker in $scriptMarkers) {
    foreach ($entry in @(
        [ordered]@{ name = "PDF visual preflight script"; text = $preflightScript },
        [ordered]@{ name = "PDF visual preflight governance report script"; text = $governanceReportScript }
    )) {
        Assert-ContainsText -Text $entry.text -ExpectedText $marker `
            -Message "$($entry.name) should preserve memory-gate marker '$marker'."
    }
}

$releaseBlockerHelpers = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\release_blocker_metadata_helpers.ps1"
foreach ($marker in @(
    "memory guard blocked=",
    "memory guard skipped=",
    "free memory MB=",
    "minimum free memory MB="
)) {
    Assert-ContainsText -Text $releaseBlockerHelpers -ExpectedText $marker `
        -Message "Release blocker helpers should keep memory-gate fields visible in PDF preflight runbooks."
}

foreach ($marker in @(
    "pdf_visual_validation_status_docs_contract",
    "pdf_visual_validation_status_docs_contract_test.ps1",
    "TIMEOUT 60"
)) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep the PDF visual status docs contract wired."
}
