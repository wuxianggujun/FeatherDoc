param(
    [string]$RepoRoot = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Assert-ThrowsMessage {
    param([scriptblock]$ScriptBlock, [string]$ExpectedText, [string]$Message)

    try {
        & $ScriptBlock
    } catch {
        Assert-ContainsText -Text $_.Exception.Message -ExpectedText $ExpectedText -Message $Message
        return
    }

    throw "$Message Expected exception containing '$ExpectedText'."
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
} else {
    $RepoRoot = (Resolve-Path $RepoRoot).Path
}

. (Join-Path $RepoRoot "scripts\template_schema_cli_common.ps1")

$leadingWhitespaceJson = Get-TemplateSchemaCommandJsonObject `
    -Command "check-template-schema" `
    -Lines @(
        "noise before json",
        "  {`"command`":`"check-template-schema`",`"matches`":true}"
    )
Assert-Equal -Actual ([bool]$leadingWhitespaceJson.matches) -Expected $true `
    -Message "Command JSON parser should tolerate leading whitespace."

$latestJson = Get-TemplateSchemaCommandJsonObject `
    -Command "check-template-schema" `
    -Lines @(
        "{`"command`":`"check-template-schema`",`"matches`":false}",
        "{`"command`":`"other-command`",`"matches`":false}",
        "{`"command`":`"check-template-schema`",`"matches`":true}"
    )
Assert-Equal -Actual ([bool]$latestJson.matches) -Expected $true `
    -Message "Command JSON parser should keep using the last matching command result."

Assert-ThrowsMessage `
    -ExpectedText "emitted invalid JSON" `
    -Message "Command JSON parser should explain malformed matching JSON." `
    -ScriptBlock {
        Get-TemplateSchemaCommandJsonObject `
            -Command "check-template-schema" `
            -Lines @("{`"command`":`"check-template-schema`",") | Out-Null
    }

Write-Host "template schema CLI common regression passed."
