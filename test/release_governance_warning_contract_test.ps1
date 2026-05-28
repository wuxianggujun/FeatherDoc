param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-ScriptParses {
    param([string]$Path)

    $parseTokens = $null
    $parseErrors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$parseTokens, [ref]$parseErrors) | Out-Null
    if ($parseErrors.Count -gt 0) {
        throw "PowerShell script has parse errors: $Path"
    }
}

function Get-WarningBlocks {
    param([string]$Text)

    $lines = $Text -split "`r?`n"
    $blocks = New-Object 'System.Collections.Generic.List[object]'
    $capturing = $false
    $currentLines = $null
    $startLine = 0

    for ($index = 0; $index -lt $lines.Count; ++$index) {
        $line = $lines[$index]

        if (-not $capturing) {
            if ($line -match '^\s*\$warnings\.Add\(\[ordered\]@\{\s*$') {
                $capturing = $true
                $startLine = $index + 1
                $currentLines = New-Object 'System.Collections.Generic.List[string]'
                $currentLines.Add($line) | Out-Null
            }
            continue
        }

        $currentLines.Add($line) | Out-Null
        if ($line -match '^\s*\}\)\s*\|\s*Out-Null\s*$') {
            $bodyLines = @()
            if ($currentLines.Count -gt 2) {
                $bodyLines = $currentLines[1..($currentLines.Count - 2)]
            }

            $blocks.Add([pscustomobject]@{
                    start_line = $startLine
                    end_line = $index + 1
                    text = $currentLines -join "`n"
                    body = $bodyLines -join "`n"
                }) | Out-Null
            $capturing = $false
            $currentLines = $null
        }
    }

    if ($capturing) {
        throw "Unterminated warning block starting at line $startLine."
    }

    return @($blocks.ToArray())
}

function Assert-WarningBlockContract {
    param(
        [string]$Path,
        [object]$Block
    )

    foreach ($field in @("id", "action", "message", "source_schema")) {
        $fieldLine = @($Block.body -split "`r?`n" | Where-Object {
                $_ -match "^\s*$field\s*="
            } | Select-Object -First 1)
        if ($fieldLine.Count -eq 0) {
            throw "Warning block starting at line $($Block.start_line) in $Path is missing required field '$field'.`n$($Block.text)"
        }

        $fieldLineText = [string]$fieldLine[0]
        if ($fieldLineText -match '=\s*""\s*$' -or $fieldLineText -match '=\s*\$null\s*$') {
            throw "Warning block starting at line $($Block.start_line) in $Path assigns '$field' an empty value.`n$fieldLineText"
        }
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptsDir = Join-Path $resolvedRepoRoot "scripts"
$scriptCount = 0
$warningBlockCount = 0

foreach ($scriptInfo in @(Get-ChildItem -LiteralPath $scriptsDir -Filter *.ps1 -File | Sort-Object Name)) {
    $scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptInfo.FullName
    if ($scriptText -notmatch '\$warnings\.Add\(\[ordered\]@\{') {
        continue
    }

    $scriptCount++
    Assert-ScriptParses -Path $scriptInfo.FullName

    $blocks = @(Get-WarningBlocks -Text $scriptText)
    Assert-True -Condition ($blocks.Count -gt 0) `
        -Message "Expected warning blocks in $($scriptInfo.Name), but none were found."

    foreach ($block in $blocks) {
        $warningBlockCount++
        Assert-WarningBlockContract -Path $scriptInfo.FullName -Block $block
    }
}

Assert-True -Condition ($scriptCount -gt 0) `
    -Message "No scripts with literal warning blocks were found under scripts\."

Write-Host "Release governance warning contract regression passed. Scripts checked: $scriptCount; warning blocks: $warningBlockCount."
