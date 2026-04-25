param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$auditScript = Join-Path $resolvedRepoRoot "scripts\assert_release_material_safety.ps1"

$passDir = Join-Path $resolvedWorkingDir "pass"
$failDir = Join-Path $resolvedWorkingDir "fail"
New-Item -ItemType Directory -Path $passDir -Force | Out-Null
New-Item -ItemType Directory -Path $failDir -Force | Out-Null

$passFile = Join-Path $passDir "release_body.zh-CN.md"
Set-Content -LiteralPath $passFile -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 发布说明

- Release notes are ready for publishing.
- Evidence path: .\output\release-candidate-checks\report\release_handoff.md
"@

& $auditScript -Path $passFile

$passCliReferenceFile = Join-Path $passDir "cli_reference.md"
Set-Content -LiteralPath $passCliReferenceFile -Encoding UTF8 -Value @"
# CLI reference

- The template render wrappers expose ``-DraftPlanOutput``, ``-PatchedPlanOutput``, and ``-PatchPlanOutput`` options.
- These are stable option names for render-plan artifacts, not release-note draft markers.
"@

& $auditScript -Path $passCliReferenceFile

$badDraftFile = Join-Path $failDir "bad_draft.md"
Set-Content -LiteralPath $badDraftFile -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 发布说明草稿

- This file is still drafting and should not be public.
"@

$badPathFile = Join-Path $failDir "bad_path.md"
Set-Content -LiteralPath $badPathFile -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 发布说明

- Local path leak: C:\Users\wuxianggujun\CodeSpace\CMakeProjects\FeatherDoc\output\release-candidate-checks\report\release_body.zh-CN.md
"@

$failedAsExpected = $false
try {
    & $auditScript -Path @($badDraftFile, $badPathFile)
} catch {
    $failedAsExpected = $true
}

if (-not $failedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed forbidden release materials."
}

Write-Host "Release material safety audit regression passed."
