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

function Assert-NotContainsText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Message
    )

    if ($Text -match [regex]::Escape($UnexpectedText)) {
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

$requiredRtlTableSamples = @(
    @{
        Name = "rtl-table-cell-hebrew-mixed-boundaries"
        Pdf = "featherdoc-hebrew-rtl-table-cell-boundary.pdf"
    },
    @{
        Name = "rtl-table-cell-persian-digit-boundaries"
        Pdf = "featherdoc-hebrew-rtl-table-cell-persian-digit-boundary.pdf"
    },
    @{
        Name = "rtl-table-cell-arabic-core-boundaries"
        Pdf = "featherdoc-arabic-rtl-table-cell-boundary.pdf"
    },
    @{
        Name = "rtl-table-cell-split-mixed-bidi-raw-spans"
        Pdf = "featherdoc-hebrew-rtl-table-cell-split-mixed-bidi-boundary.pdf"
    },
    @{
        Name = "rtl-table-cell-split-pure-rtl-raw-spans"
        Pdf = "featherdoc-hebrew-rtl-table-cell-split-pure-rtl-boundary.pdf"
    },
    @{
        Name = "rtl-table-cell-arabic-split-pure-rtl-normalized"
        Pdf = "featherdoc-arabic-rtl-table-cell-split-pure-rtl-boundary.pdf"
    },
    @{
        Name = "rtl-table-cell-hebrew-combining-mark-raw-spans"
        Pdf = "featherdoc-hebrew-rtl-table-cell-combining-mark-boundary.pdf"
    },
    @{
        Name = "rtl-table-cell-arabic-combining-mark-raw-spans"
        Pdf = "featherdoc-arabic-rtl-table-cell-combining-mark-boundary.pdf"
    }
)

foreach ($sample in $requiredRtlTableSamples) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('name = "{0}"' -f $sample.Name) `
        -Message "PDF visual release gate should render RTL table cell baseline sample '$($sample.Name)'."
    Assert-ContainsText -Text $scriptText -ExpectedText $sample.Pdf `
        -Message "PDF visual release gate should point at the generated RTL table cell PDF for '$($sample.Name)'."
    Assert-ContainsText -Text $scriptText -ExpectedText ('output = Join-Path $baselineDir "{0}"' -f $sample.Name) `
        -Message "PDF visual release gate should write a baseline folder for '$($sample.Name)'."
}

Assert-ContainsText -Text $scriptText -ExpectedText "rtl-table-cell-normalization" `
    -Message "PDF visual release gate should tag RTL table cell visual baselines."
Assert-ContainsText -Text $scriptText -ExpectedText "rtl-table-cell-raw-preservation" `
    -Message "PDF visual release gate should tag complex RTL table cell raw-preservation baselines."
Assert-ContainsText -Text $scriptText -ExpectedText "pdf_unicode_font_roundtrip" `
    -Message "PDF visual release gate should ensure generated RTL table cell PDFs exist."
Assert-ContainsText -Text $scriptText -ExpectedText '-LogPath $unicodeLog' `
    -Message "PDF visual release gate should capture the unicode visual regression log when it runs."
Assert-ContainsText -Text $scriptText -ExpectedText '$logPaths["pdf_unicode_roundtrip"] = $unicodeRoundtripLog' `
    -Message "PDF visual release gate should report the roundtrip log only when the unicode visual baseline is skipped."
Assert-ContainsText -Text $scriptText -ExpectedText '$logPaths["unicode_font"] = $unicodeLog' `
    -Message "PDF visual release gate should report the unicode visual log when that baseline runs."
Assert-ContainsText -Text $scriptText -ExpectedText 'logs = $logPaths' `
    -Message "PDF visual release gate summary should use the conditionally assembled log map."
Assert-NotContainsText -Text $scriptText -UnexpectedText 'pdf_unicode_roundtrip = $unicodeRoundtripLog' `
    -Message "PDF visual release gate should not unconditionally publish the roundtrip log path."
Assert-NotContainsText -Text $scriptText -UnexpectedText 'unicode_font = $unicodeLog' `
    -Message "PDF visual release gate should not unconditionally publish the unicode visual log path."

Write-Host "PDF visual release gate text shaping baseline regression passed."
