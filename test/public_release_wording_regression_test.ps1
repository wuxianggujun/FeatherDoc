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
    @{ Path = "VISUAL_VALIDATION_QUICKSTART.zh-CN.md"; Unexpected = [string]::Concat(@([char]0x6574, [char]0x5957, " release ", [char]0x8349, [char]0x7A3F)) }
    @{ Path = "VISUAL_VALIDATION_QUICKSTART.zh-CN.md"; Unexpected = [string]::Concat(@([char]0x8FD9, [char]0x51E0, [char]0x4EFD, [char]0x8349, [char]0x7A3F)) }
    @{ Path = "RELEASE_ARTIFACT_TEMPLATE.md"; Unexpected = "release-facing drafts" }
    @{ Path = "RELEASE_ARTIFACT_TEMPLATE.md"; Unexpected = "trimming that draft" }
    @{ Path = "docs/index.rst"; Unexpected = "release-facing drafts" }
    @{ Path = "CHANGELOG.md"; Unexpected = "still blocked by missing reusable build and baseline outputs" }
    @{ Path = "docs/release_policy_zh.rst"; Unexpected = [string]::Concat(@([char]0x63D0, [char]0x4F9B, [char]0x4E00, [char]0x4EFD, [char]0x4E2D, [char]0x6587, [char]0x8349, [char]0x7A3F)) }
)

foreach ($check in $checks) {
    $path = Join-Path $resolvedRepoRoot $check.Path
    $content = Get-Content -Raw -LiteralPath $path
    if ($content.Contains([string]$check.Unexpected)) {
        throw "Unexpected public release wording '$($check.Unexpected)' still present in $($check.Path)."
    }
}

Write-Host "Public release wording regression passed."
