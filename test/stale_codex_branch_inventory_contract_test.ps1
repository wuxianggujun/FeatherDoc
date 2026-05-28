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

function New-TextFromCodePoints {
    param(
        [object[]]$Parts
    )

    $builder = New-Object System.Text.StringBuilder
    foreach ($part in $Parts) {
        if ($part -is [int]) {
            [void]$builder.Append([char]$part)
        } else {
            [void]$builder.Append([string]$part)
        }
    }

    return $builder.ToString()
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

$inventoryDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\stale_codex_branch_inventory_zh.rst"
$maintenanceDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\documentation_maintenance_zh.rst"
$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

$branchMarkers = @(
    "origin/codex/pdf-cjk-copy-search-gate",
    "origin/codex/pdf-cjk-bullet-fallback",
    "origin/codex/release-governance-rollup-details",
    "origin/codex/release-governance-warning-entrypoints"
)

foreach ($marker in $branchMarkers) {
    Assert-ContainsText -Text $inventoryDoc -ExpectedText $marker `
        -Message "Stale codex branch inventory should keep the remote reference branch marker."
}

$decisionMarkers = @(
    "manifest_ids_already_covered_in_dev",
    (New-TextFromCodePoints @(0x6D,0x61,0x6E,0x69,0x66,0x65,0x73,0x74," ",0x6837,0x4F8B," ID ",0x76F8,0x5BF9,0x5F53,0x524D,' ``dev`` ',0x7684,0x7F3A,0x53E3,0x4E3A," 0")),
    (New-TextFromCodePoints @(0x4E0D,0x4E3A,0x51CF,0x5C11,0x5206,0x652F,0x6570,0x91CF,0x800C,0x5220,0x9664,0x5168,0x90E8,0x65E7,0x5206,0x652F)),
    (New-TextFromCodePoints @(0x4E0D,0x6574,0x5206,0x652F,0x5408,0x5E76,0x3001,0x4E0D,0x5F3A,0x63A8,0x3001,0x4E0D,0x6539,0x5199,0x5386,0x53F2)),
    (New-TextFromCodePoints @(0x65E7," visual baseline",0x3001,0x91CD,0x578B," gate ",0x548C,0x5386,0x53F2,0x63D0,0x4EA4,0x8BC1,0x636E)),
    (New-TextFromCodePoints @("PDF preflight blocker ",0x5DF2,0x6E05,0x96F6)),
    (New-TextFromCodePoints @(0x5220,0x9664,0x524D,0x5148,0x4FDD,0x7559,0x5F52,0x6863,0x8BB0,0x5F55)),
    (New-TextFromCodePoints @(0x7528,0x6237,0x518D,0x6B21,0x660E,0x786E,0x8981,0x6C42,0x5220,0x9664,0x5177,0x4F53,0x5206,0x652F)),
    "PDF preflight",
    "not_ready",
    "ready",
    "3264183",
    "build_dir_source = requested",
    ".bpdf-roundtrip-msvc",
    "CMakeCache.txt",
    "CTestTestfile.cmake"
)

foreach ($marker in $decisionMarkers) {
    Assert-ContainsText -Text $inventoryDoc -ExpectedText $marker `
        -Message "Stale codex branch inventory should preserve the branch cleanup decision boundary."
}

foreach ($marker in @(
    "docs/stale_codex_branch_inventory_zh.rst",
    (New-TextFromCodePoints @(0x65E7,' ``codex/*`` ',0x5206,0x652F,0x7EE7,0x7EED,0x4F5C,0x4E3A,0x53EA,0x8BFB,0x53C2,0x8003,0x5E93,0x5B58,0x4FDD,0x7559)),
    (New-TextFromCodePoints @(0x540E,0x7EED,0x4E0D,0x505A,0x6574,0x5206,0x652F,0x5408,0x5E76))
)) {
    Assert-ContainsText -Text $maintenanceDoc -ExpectedText $marker `
        -Message "Documentation maintenance guide should keep the stale branch inventory entry and policy."
}

Assert-ContainsText -Text $indexDoc -ExpectedText "stale_codex_branch_inventory_zh" `
    -Message "Documentation index should keep the stale branch inventory page reachable."

foreach ($marker in @(
    "stale_codex_branch_inventory_contract",
    "stale_codex_branch_inventory_contract_test.ps1",
    "TIMEOUT 60"
)) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep the stale codex branch inventory contract wired."
}
