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

function Assert-TextBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$ExpectedFragments,
        [string]$Message
    )

    $anchorIndex = $Text.IndexOf($Anchor, [StringComparison]::Ordinal)
    if ($anchorIndex -lt 0) {
        throw "$Message Missing anchor '$Anchor'."
    }

    $blockStart = $Text.LastIndexOf("`n   * ", $anchorIndex, [StringComparison]::Ordinal)
    if ($blockStart -lt 0) {
        $blockStart = 0
    } else {
        $blockStart += 1
    }

    $blockEnd = $Text.IndexOf("`n   * ", $anchorIndex + $Anchor.Length, [StringComparison]::Ordinal)
    if ($blockEnd -lt 0) {
        $blockEnd = $Text.Length
    }

    $block = $Text.Substring($blockStart, $blockEnd - $blockStart)
    foreach ($fragment in $ExpectedFragments) {
        if ($block.IndexOf($fragment, [StringComparison]::Ordinal) -lt 0) {
            throw "$Message Missing '$fragment' in block anchored by '$Anchor'."
        }
    }
}

function Assert-TextParagraphContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$ExpectedFragments,
        [string]$Message
    )

    $normalizedText = $Text -replace "`r`n", "`n"
    $anchorIndex = $normalizedText.IndexOf($Anchor, [StringComparison]::Ordinal)
    if ($anchorIndex -lt 0) {
        throw "$Message Missing anchor '$Anchor'."
    }

    $paragraphEnd = $normalizedText.IndexOf("`n`n", $anchorIndex, [StringComparison]::Ordinal)
    if ($paragraphEnd -lt 0) {
        $paragraphEnd = $normalizedText.Length
    }

    $paragraph = $normalizedText.Substring($anchorIndex, $paragraphEnd - $anchorIndex)
    foreach ($fragment in $ExpectedFragments) {
        if ($paragraph.IndexOf($fragment, [StringComparison]::Ordinal) -lt 0) {
            throw "$Message Missing '$fragment' in paragraph anchored by '$Anchor'."
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
