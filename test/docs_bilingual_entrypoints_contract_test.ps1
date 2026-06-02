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

$rootIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$englishIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\index.rst"
$chineseIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\index.rst"
$chineseApiIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\index.rst"
$readme = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "README.md"
$readmeZh = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "README.zh-CN.md"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

foreach ($marker in @(
        "Choose a language entry point",
        "en/index",
        "zh-CN/index",
        "Languages",
        "api/index",
        "getting_started"
    )) {
    Assert-ContainsText -Text $rootIndex -ExpectedText $marker `
        -Message "Root docs index should preserve bilingual landing markers."
}

foreach ($marker in @(
        "../zh-CN/index",
        "../api/index",
        "../getting_started",
        "../pdf_export"
    )) {
    Assert-ContainsText -Text $englishIndex -ExpectedText $marker `
        -Message "English docs entrypoint should preserve navigation marker."
}

foreach ($marker in @(
        "../en/index",
        "api/index",
        "../visual_validation_zh",
        "../governance_routes_zh",
        "../release_policy_zh"
    )) {
    Assert-ContainsText -Text $chineseIndex -ExpectedText $marker `
        -Message "Chinese docs entrypoint should preserve navigation marker."
}

foreach ($marker in @(
        "../../api/index",
        "featherdoc::Document",
        "../../api/document",
        "../../api/templates",
        "../../api/tables"
    )) {
    Assert-ContainsText -Text $chineseApiIndex -ExpectedText $marker `
        -Message "Chinese API entrypoint should preserve mirrored API marker."
}

foreach ($marker in @(
        "https://wuxianggujun.github.io/FeatherDoc/en/",
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/",
        "https://shop.input.im/?code=fbe6f3d5",
        "sponsor/zhifubao.jpg",
        "sponsor/weixin.png"
    )) {
    Assert-ContainsText -Text $readme -ExpectedText $marker `
        -Message "English README should preserve docs, promotion, and sponsor markers."
}

foreach ($marker in @(
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/",
        "https://wuxianggujun.github.io/FeatherDoc/en/",
        "https://shop.input.im/?code=fbe6f3d5",
        "sponsor/zhifubao.jpg",
        "sponsor/weixin.png"
    )) {
    Assert-ContainsText -Text $readmeZh -ExpectedText $marker `
        -Message "Chinese README should preserve docs, promotion, and sponsor markers."
}

foreach ($marker in @(
        "docs_bilingual_entrypoints_contract",
        "docs_bilingual_entrypoints_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;bilingual"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake should keep bilingual docs contract registered."
}

Write-Host "Docs bilingual entrypoints contract passed."
