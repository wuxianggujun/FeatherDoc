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

function Assert-DoesNotContainText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Message
    )

    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw $Message
    }
}

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-LineContainsAll {
    param(
        [string]$Text,
        [string[]]$Fragments,
        [string]$Message
    )

    foreach ($line in ($Text -split "\r?\n")) {
        $normalizedLine = $line -replace '/', '\'
        $lineMatches = $true
        foreach ($fragment in $Fragments) {
            $normalizedFragment = $fragment -replace '/', '\'
            if ($normalizedLine -notmatch [regex]::Escape($normalizedFragment)) {
                $lineMatches = $false
                break
            }
        }

        if ($lineMatches) {
            return
        }
    }

    throw $Message
}

function Assert-MarkdownListRunContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Fragments,
        [string]$Message
    )

    $lines = $Text -split "\r?\n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        if ($lines[$lineIndex] -notmatch [regex]::Escape($Anchor)) {
            continue
        }
        if ($lines[$lineIndex] -notmatch '^\s*-\s*') {
            continue
        }

        $runStart = $lineIndex
        while ($runStart -gt 0 -and $lines[$runStart - 1] -match '^\s*-\s*') {
            $runStart--
        }

        $runEnd = $lineIndex
        while ($runEnd + 1 -lt $lines.Count -and $lines[$runEnd + 1] -match '^\s*-\s*') {
            $runEnd++
        }

        $run = ($lines[$runStart..$runEnd]) -join "`n"
        $runMatches = $true
        foreach ($fragment in $Fragments) {
            if ($run -notmatch [regex]::Escape($fragment)) {
                $runMatches = $false
                break
            }
        }

        if ($runMatches) {
            return
        }
    }

    throw $Message
}

function Assert-MarkdownListBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Fragments,
        [string]$Message
    )

    $lines = $Text -split "\r?\n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        if ($lines[$lineIndex] -notmatch [regex]::Escape($Anchor)) {
            continue
        }
        if ($lines[$lineIndex] -notmatch '^(\s*)-\s*') {
            continue
        }

        $anchorIndent = $Matches[1].Length
        $blockEnd = $lineIndex
        for ($nextLineIndex = $lineIndex + 1; $nextLineIndex -lt $lines.Count; $nextLineIndex++) {
            $nextLine = $lines[$nextLineIndex]
            if ($nextLine -match '^(#+)\s+') {
                break
            }
            if ($nextLine -match '^(\s*)-\s*' -and $Matches[1].Length -le $anchorIndent) {
                break
            }

            $blockEnd = $nextLineIndex
        }

        $block = (($lines[$lineIndex..$blockEnd]) -join "`n") -replace '/', '\'
        $blockMatches = $true
        foreach ($fragment in $Fragments) {
            $normalizedFragment = $fragment -replace '/', '\'
            if ($block -notmatch [regex]::Escape($normalizedFragment)) {
                $blockMatches = $false
                break
            }
        }

        if ($blockMatches) {
            return
        }
    }

    throw $Message
}

function Assert-MarkdownSectionContainsAll {
    param(
        [string]$Text,
        [string]$Heading,
        [string[]]$Fragments,
        [string]$Message
    )

    $lines = $Text -split "\r?\n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        $line = $lines[$lineIndex].Trim()
        if ($line -ne $Heading -or $line -notmatch '^(#+)\s+') {
            continue
        }

        $headingLevel = $Matches[1].Length
        $sectionEnd = $lines.Count - 1
        for ($nextLineIndex = $lineIndex + 1; $nextLineIndex -lt $lines.Count; $nextLineIndex++) {
            if ($lines[$nextLineIndex] -match '^(#+)\s+' -and $Matches[1].Length -le $headingLevel) {
                $sectionEnd = $nextLineIndex - 1
                break
            }
        }

        $section = (($lines[$lineIndex..$sectionEnd]) -join "`n") -replace '/', '\'
        $sectionMatches = $true
        foreach ($fragment in $Fragments) {
            $normalizedFragment = $fragment -replace '/', '\'
            if ($section -notmatch [regex]::Escape($normalizedFragment)) {
                $sectionMatches = $false
                break
            }
        }

        if ($sectionMatches) {
            return
        }
    }

    throw $Message
}

function Write-TestJson {
    param(
        [string]$Path,
        [object]$Value
    )

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}
