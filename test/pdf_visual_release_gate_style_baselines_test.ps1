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
    Assert-ContainsText -Text $scriptText -ExpectedText ('name = "{0}"' -f $sample) `
        -Message "PDF visual release gate should render style baseline sample '$sample'."
}

$requiredFocusMarkers = @(
    "font-size",
    "bold-italic-underline",
    "underline-signature",
    "east-asia-font-mapping"
)

foreach ($marker in $requiredFocusMarkers) {
    Assert-ContainsText -Text $scriptText -ExpectedText $marker `
        -Message "PDF visual release gate should record style focus marker '$marker'."
}

Assert-ContainsText -Text $scriptText -ExpectedText "style_focus = @(`$StyleFocus)" `
    -Message "Rendered baseline summary should carry style_focus metadata."

Write-Host "PDF visual release gate style baseline regression passed."
