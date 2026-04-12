param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

$checks = @(
    @{ Path = "README.md"; Unexpected = "release-facing drafts" }
    @{ Path = "VISUAL_VALIDATION_QUICKSTART.md"; Unexpected = "release-facing drafts" }
    @{ Path = "VISUAL_VALIDATION_QUICKSTART.md"; Unexpected = "regenerate those drafts" }
    @{ Path = "VISUAL_VALIDATION_QUICKSTART.zh-CN.md"; Unexpected = "整套 release 草稿" }
    @{ Path = "VISUAL_VALIDATION_QUICKSTART.zh-CN.md"; Unexpected = "这几份草稿" }
    @{ Path = "RELEASE_ARTIFACT_TEMPLATE.md"; Unexpected = "release-facing drafts" }
    @{ Path = "RELEASE_ARTIFACT_TEMPLATE.md"; Unexpected = "trimming that draft" }
    @{ Path = "docs/index.rst"; Unexpected = "release-facing drafts" }
    @{ Path = "docs/release_policy_zh.rst"; Unexpected = "提供一份中文草稿" }
)

foreach ($check in $checks) {
    $path = Join-Path $resolvedRepoRoot $check.Path
    $content = Get-Content -Raw -LiteralPath $path
    if ($content.Contains([string]$check.Unexpected)) {
        throw "Unexpected public release wording '$($check.Unexpected)' still present in $($check.Path)."
    }
}

Write-Host "Public release wording regression passed."
