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

Write-Host "Word visual release gate smoke verdict regression passed."
