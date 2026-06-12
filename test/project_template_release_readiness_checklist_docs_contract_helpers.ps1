function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Assert-TextOrder {
    param(
        [string]$Text,
        [string[]]$ExpectedTexts,
        [string]$Message
    )

    $offset = 0
    foreach ($expectedText in $ExpectedTexts) {
        $index = $Text.IndexOf($expectedText, $offset, [System.StringComparison]::Ordinal)
        if ($index -lt 0) {
            throw "$Message Missing expected text after offset $offset. Missing='$expectedText'."
        }

        $offset = $index + $expectedText.Length
    }
}

function Assert-SourceSectionContainsAll {
    param(
        [string]$Text,
        [string]$StartText,
        [string]$EndText,
        [string[]]$ExpectedTexts,
        [string]$Message
    )

    $startIndex = $Text.IndexOf($StartText, [System.StringComparison]::Ordinal)
    if ($startIndex -lt 0) {
        throw "$Message Missing section start='$StartText'."
    }

    $endIndex = $Text.IndexOf($EndText, $startIndex + $StartText.Length, [System.StringComparison]::Ordinal)
    if ($endIndex -lt 0) {
        throw "$Message Missing section end='$EndText'."
    }

    $section = $Text.Substring($startIndex, $endIndex - $startIndex)
    foreach ($expectedText in $ExpectedTexts) {
        if ($section.IndexOf($expectedText, [System.StringComparison]::Ordinal) -lt 0) {
            throw "$Message Missing expected text in section. Missing='$expectedText'."
        }
    }
}

function Get-RepoFileText {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected contract file was not found: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}
