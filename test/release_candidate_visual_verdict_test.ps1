param(
    [string]$RepoRoot,
    [string]$WorkingDir
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
        throw $Message
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_release_candidate_checks.ps1"
$scriptText = Get-Content -Raw -LiteralPath $scriptPath

foreach ($name in @(
        "SmokeReviewVerdict",
        "FixedGridReviewVerdict",
        "SectionPageSetupReviewVerdict",
        "PageNumberFieldsReviewVerdict",
        "CuratedVisualReviewVerdict"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('[string]${0} = "undecided"' -f $name) `
        -Message ("Release preflight should expose {0}." -f $name)
}

foreach ($name in @(
        "SmokeReviewNote",
        "FixedGridReviewNote",
        "SectionPageSetupReviewNote",
        "PageNumberFieldsReviewNote",
        "CuratedVisualReviewNote"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('[string]${0} = ""' -f $name) `
        -Message ("Release preflight should expose {0}." -f $name)
}

foreach ($argument in @(
        "-SmokeReviewVerdict",
        "-FixedGridReviewVerdict",
        "-SectionPageSetupReviewVerdict",
        "-PageNumberFieldsReviewVerdict",
        "-CuratedVisualReviewVerdict"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('-VerdictArgument "{0}"' -f $argument) `
        -Message ("Release preflight should pass {0} through to the visual gate." -f $argument)
}

foreach ($argument in @(
        "-SmokeReviewNote",
        "-FixedGridReviewNote",
        "-SectionPageSetupReviewNote",
        "-PageNumberFieldsReviewNote",
        "-CuratedVisualReviewNote"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('-NoteArgument "{0}"' -f $argument) `
        -Message ("Release preflight should pass {0} through to the visual gate." -f $argument)
}

foreach ($prefix in @(
        "smoke",
        "fixed_grid",
        "section_page_setup",
        "page_number_fields"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('Add-GateFlowReviewSummary -Target $summary.steps.visual_gate -Prefix "{0}"' -f $prefix) `
        -Message ("Release summary should capture {0} review verdict metadata." -f $prefix)
}

Assert-ContainsText -Text $scriptText -ExpectedText '$summary.steps.visual_gate.curated_visual_regressions = $curatedVisualReviewEntries' `
    -Message "Release summary should capture curated visual review verdict metadata."

Write-Host "Release candidate visual verdict passthrough regression passed."
