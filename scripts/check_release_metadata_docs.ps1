param(
    [string]$RepoRoot = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    param([string]$InputRoot)

    if ([string]::IsNullOrWhiteSpace($InputRoot)) {
        return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    }

    return (Resolve-Path $InputRoot).Path
}

function Read-Utf8Text {
    param([string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        throw "File must be UTF-8 without BOM: $Path"
    }

    return [System.Text.Encoding]::UTF8.GetString($bytes)
}

function Assert-FileExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Missing ${Label}: $Path"
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Label
    )

    if (-not $Text.Contains($ExpectedText)) {
        throw "$Label is missing expected text: $ExpectedText"
    }
}

function Assert-NoTrailingWhitespace {
    param(
        [string]$Text,
        [string]$Path
    )

    $lines = $Text -split "`r?`n"
    for ($index = 0; $index -lt $lines.Count; ++$index) {
        if ($lines[$index].TrimEnd() -ne $lines[$index]) {
            throw "Trailing whitespace in $Path line $($index + 1)."
        }
    }
}

function Assert-NoTabs {
    param(
        [string]$Text,
        [string]$Path
    )

    if ($Text.Contains("`t")) {
        throw "Tab character found in $Path."
    }
}

$resolvedRepoRoot = Resolve-RepoRoot -InputRoot $RepoRoot
$pipelinePath = Join-Path $resolvedRepoRoot "docs\release_metadata_pipeline_zh.rst"
$checklistPath = Join-Path $resolvedRepoRoot "docs\release_metadata_maintenance_checklist_zh.rst"
$policyPath = Join-Path $resolvedRepoRoot "docs\release_policy_zh.rst"

Assert-FileExists -Path $pipelinePath -Label "release metadata pipeline doc"
Assert-FileExists -Path $checklistPath -Label "release metadata maintenance checklist doc"
Assert-FileExists -Path $policyPath -Label "release policy doc"

$pipelineText = Read-Utf8Text -Path $pipelinePath
$checklistText = Read-Utf8Text -Path $checklistPath
$policyText = Read-Utf8Text -Path $policyPath

foreach ($doc in @(
        @{ Path = $pipelinePath; Text = $pipelineText },
        @{ Path = $checklistPath; Text = $checklistText },
        @{ Path = $policyPath; Text = $policyText }
    )) {
    Assert-NoTrailingWhitespace -Text $doc.Text -Path $doc.Path
    Assert-NoTabs -Text $doc.Text -Path $doc.Path
}

foreach ($expected in @(
        "run_word_visual_release_gate.ps1",
        "run_release_candidate_checks.ps1",
        "sync_visual_review_verdict.ps1",
        "write_release_note_bundle.ps1",
        "review_task_summary",
        "assert_release_material_safety.ps1",
        "-SkipMaterialSafetyAudit"
    )) {
    Assert-ContainsText -Text $pipelineText -ExpectedText $expected -Label "release metadata pipeline doc"
}

foreach ($expected in @(
        ':doc:`release_metadata_pipeline_zh`',
        "word_visual_release_gate_smoke_verdict",
        "release_candidate_visual_verdict",
        "sync_visual_review_verdict_(section_page_setup|page_number_fields|curated_visual_bundle)",
        "release_note_bundle_visual_verdict_fallback",
        "public_release_wording_regression_test.ps1",
        "git diff --check"
    )) {
    Assert-ContainsText -Text $checklistText -ExpectedText $expected -Label "release metadata maintenance checklist doc"
}

Assert-ContainsText -Text $policyText `
    -ExpectedText ':doc:`release_metadata_pipeline_zh`' `
    -Label "release policy doc"

Write-Host "Release metadata docs check passed."
