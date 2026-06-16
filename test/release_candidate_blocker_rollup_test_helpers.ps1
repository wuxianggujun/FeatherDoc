function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Assert-MarkdownListBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$ExpectedFragments,
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

        $blockLines = @($lines[$lineIndex])
        for ($nextLineIndex = $lineIndex + 1; $nextLineIndex -lt $lines.Count; $nextLineIndex++) {
            $nextLine = $lines[$nextLineIndex]
            if ([string]::IsNullOrWhiteSpace($nextLine) -or
                $nextLine -match '^#{1,6}\s' -or
                ($nextLine -match '^\s*-\s*' -and $nextLine -notmatch '^\s{2,}-\s*')) {
                break
            }

            $blockLines += $nextLine
        }

        $block = $blockLines -join "`n"
        $hasAllFragments = $true
        foreach ($fragment in $ExpectedFragments) {
            if ($block -notmatch [regex]::Escape($fragment)) {
                $hasAllFragments = $false
                break
            }
        }

        if ($hasAllFragments) {
            return
        }
    }

    throw $Message
}

function Get-ItemById {
    param(
        [object[]]$Items,
        [string]$Id,
        [string]$Label
    )

    foreach ($item in @($Items)) {
        if ([string]$item.id -eq $Id) {
            return $item
        }
    }

    throw "$Label should include item id '$Id'."
}

function Write-JsonFile {
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}
