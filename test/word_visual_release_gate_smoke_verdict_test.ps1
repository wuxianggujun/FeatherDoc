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

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_word_visual_release_gate.ps1"
$scriptText = Get-Content -Raw -LiteralPath $scriptPath

Assert-ContainsText -Text $scriptText -ExpectedText '[string]$SmokeReviewVerdict = "undecided"' `
    -Message "Release gate should expose SmokeReviewVerdict."
Assert-ContainsText -Text $scriptText -ExpectedText '[string]$SmokeReviewNote = ""' `
    -Message "Release gate should expose SmokeReviewNote."
Assert-ContainsText -Text $scriptText -ExpectedText '$smokeArgs += @("-ReviewVerdict", $SmokeReviewVerdict)' `
    -Message "Release gate should pass SmokeReviewVerdict into run_word_visual_smoke.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$smokeArgs += @("-ReviewNote", $SmokeReviewNote)' `
    -Message "Release gate should pass SmokeReviewNote into run_word_visual_smoke.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText 'review_verdict = Get-OptionalPropertyValue -Object $smokeReviewResult -Name "verdict"' `
    -Message "Release gate summary should capture the smoke review verdict."
Assert-ContainsText -Text $scriptText -ExpectedText 'Smoke review verdict:' `
    -Message "Release gate final review should surface the smoke review verdict."

Assert-ContainsText -Text $scriptText -ExpectedText '[string]$FixedGridReviewVerdict = "undecided"' `
    -Message "Release gate should expose FixedGridReviewVerdict."
Assert-ContainsText -Text $scriptText -ExpectedText '[string]$FixedGridReviewNote = ""' `
    -Message "Release gate should expose FixedGridReviewNote."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewVerdict", $FixedGridReviewVerdict)' `
    -Message "Release gate should pass FixedGridReviewVerdict into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewNote", $FixedGridReviewNote)' `
    -Message "Release gate should pass FixedGridReviewNote into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText 'review_verdict = Get-OptionalPropertyValue -Object $fixedGridTaskReview -Name "verdict"' `
    -Message "Release gate summary should capture the fixed-grid task review verdict."
Assert-ContainsText -Text $scriptText -ExpectedText 'Fixed-grid review verdict:' `
    -Message "Release gate final review should surface the fixed-grid review verdict."

Assert-ContainsText -Text $scriptText -ExpectedText '[string]$CuratedVisualReviewVerdict = "undecided"' `
    -Message "Release gate should expose CuratedVisualReviewVerdict."
Assert-ContainsText -Text $scriptText -ExpectedText '[string]$CuratedVisualReviewNote = ""' `
    -Message "Release gate should expose CuratedVisualReviewNote."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewVerdict", $CuratedVisualReviewVerdict)' `
    -Message "Release gate should pass CuratedVisualReviewVerdict into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewNote", $CuratedVisualReviewNote)' `
    -Message "Release gate should pass CuratedVisualReviewNote into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$bundleTaskReview = Read-ReviewResult -ReviewResultPath $bundleTaskInfo.review_result_path' `
    -Message "Release gate should read the curated task review result."
Assert-ContainsText -Text $scriptText -ExpectedText 'review_verdict = Get-OptionalPropertyValue -Object $bundleTaskReview -Name "verdict"' `
    -Message "Release gate summary should capture the curated task review verdict."
Assert-ContainsText -Text $scriptText -ExpectedText '$($_.label) review verdict:' `
    -Message "Release gate final review should surface curated task review verdicts."

Write-Host "Word visual release gate smoke verdict regression passed."
