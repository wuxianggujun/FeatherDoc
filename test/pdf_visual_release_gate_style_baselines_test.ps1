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

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_pdf_visual_release_gate.ps1"
$scriptText = Get-Content -Raw -LiteralPath $scriptPath
$manifestPath = Join-Path $resolvedRepoRoot "test\pdf_regression_manifest.json"
$manifestText = Get-Content -Raw -LiteralPath $manifestPath

$parseTokens = $null
$parseErrors = $null
[System.Management.Automation.Language.Parser]::ParseFile(
    $scriptPath,
    [ref]$parseTokens,
    [ref]$parseErrors
) | Out-Null
if ($parseErrors.Count -gt 0) {
    throw "PDF visual release gate script has parse errors."
}

$requiredSamples = @(
    "styled-text",
    "mixed-style-text",
    "underline-text",
    "document-contract-cjk-style",
    "document-eastasia-style-probe"
)

foreach ($sample in $requiredSamples) {
    Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sample) `
        -Message "PDF regression manifest should define style baseline sample '$sample'."
}

$requiredFocusMarkers = @(
    "font-size",
    "bold-italic-underline",
    "underline-signature",
    "east-asia-font-mapping"
)

foreach ($marker in $requiredFocusMarkers) {
    Assert-ContainsText -Text $manifestText -ExpectedText $marker `
        -Message "PDF regression manifest should record style focus marker '$marker'."
}

Assert-ContainsText -Text $scriptText -ExpectedText "style_focus = @(`$StyleFocus)" `
    -Message "Rendered baseline summary should carry style_focus metadata."
Assert-ContainsText -Text $scriptText -ExpectedText "expect_visual_baseline -eq `$true" `
    -Message "PDF visual release gate should select visual baseline samples from the manifest."
Assert-ContainsText -Text $scriptText -ExpectedText "visual_style_focus" `
    -Message "PDF visual release gate should carry manifest style focus metadata."
Assert-ContainsText -Text $manifestText -ExpectedText '"expect_visual_baseline": true' `
    -Message "PDF regression manifest should mark visual baseline samples."

Write-Host "PDF visual release gate style baseline regression passed."
