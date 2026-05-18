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
    "mixed-cjk-punctuation-text",
    "latin-ligature-text"
)

foreach ($sample in $requiredSamples) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('name = "{0}"' -f $sample) `
        -Message "PDF visual release gate should render text shaping baseline sample '$sample'."
    Assert-ContainsText -Text $scriptText -ExpectedText ('featherdoc-pdf-regression-{0}.pdf' -f $sample) `
        -Message "PDF visual release gate should point at the regression PDF for '$sample'."
    Assert-ContainsText -Text $scriptText -ExpectedText ('output = Join-Path $baselineDir "{0}"' -f $sample) `
        -Message "PDF visual release gate should write a baseline folder for '$sample'."
}

Assert-ContainsText -Text $scriptText -ExpectedText "scripts\check_pdf_text_layer.py" `
    -Message "PDF visual release gate should use the text-layer copy/search checker."
Assert-ContainsText -Text $scriptText -ExpectedText "cjk-copy-search" `
    -Message "PDF visual release gate should write CJK copy/search evidence."
Assert-ContainsText -Text $scriptText -ExpectedText "expect_cjk -eq `$true" `
    -Message "PDF visual release gate should select CJK manifest samples."
Assert-ContainsText -Text $scriptText -ExpectedText "matched_text" `
    -Message "PDF visual release gate should report matched text-layer snippets."
Assert-ContainsText -Text $scriptText -ExpectedText "missing_text" `
    -Message "PDF visual release gate should report missing text-layer snippets."

Write-Host "PDF visual release gate text shaping baseline regression passed."
