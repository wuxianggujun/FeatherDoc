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

function New-TestFile {
    param([string]$Path, [datetime]$LastWriteTimeUtc)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    Set-Content -LiteralPath $Path -Value "" -Encoding UTF8
    (Get-Item -LiteralPath $Path).LastWriteTimeUtc = $LastWriteTimeUtc
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

$spacedJson = Get-TemplateSchemaCommandJsonObject `
    -Command "check-template-schema" `
    -Lines @(
        "noise before json",
        "  { `"command`" : `"check-template-schema`", `"matches`" : true }"
    )
Assert-Equal -Actual ([bool]$spacedJson.matches) -Expected $true `
    -Message "Command JSON parser should tolerate spaces around command JSON separators."

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

$portablePath = ConvertTo-TemplateSchemaPortableRelativePath `
    -BasePath (Join-Path $RepoRoot "output\style-merge") `
    -TargetPath (Join-Path $RepoRoot "output\style-merge\plans\rollback.json")
Assert-Equal -Actual $portablePath -Expected "plans/rollback.json" `
    -Message "Portable path helper should use forward-slash relative paths."

$parentPortablePath = ConvertTo-TemplateSchemaPortableRelativePath `
    -BasePath (Join-Path $RepoRoot "output\style-merge\summary") `
    -TargetPath (Join-Path $RepoRoot "output\style-merge\plans\rollback.json")
Assert-Equal -Actual $parentPortablePath -Expected "../plans/rollback.json" `
    -Message "Portable path helper should preserve parent traversal for sibling output folders."

$emptyPortablePath = ConvertTo-TemplateSchemaPortableRelativePath `
    -BasePath (Join-Path $RepoRoot "output\style-merge") `
    -TargetPath ""
Assert-Equal -Actual $emptyPortablePath -Expected "" `
    -Message "Portable path helper should preserve empty target paths."

$plainCommandLine = ConvertTo-TemplateSchemaCommandLine -Arguments @(
    "featherdoc_cli",
    "inspect-styles",
    ".\samples\invoice.docx",
    "--json"
)
Assert-Equal -Actual $plainCommandLine -Expected "featherdoc_cli inspect-styles .\samples\invoice.docx --json" `
    -Message "Command-line helper should keep plain arguments unquoted."

$quotedCommandLine = ConvertTo-TemplateSchemaCommandLine -Arguments @(
    "featherdoc_cli",
    "apply-style-refactor",
    "C:\Temp\input doc.docx",
    "--style",
    "Body/Text",
    "--label",
    'Owner "Review"'
)
Assert-Equal -Actual $quotedCommandLine -Expected 'featherdoc_cli apply-style-refactor "C:\Temp\input doc.docx" --style Body/Text --label "Owner \"Review\""' `
    -Message "Command-line helper should quote whitespace and embedded double quotes while preserving safe path separators."

$singleQuoteAndBraceCommandLine = ConvertTo-TemplateSchemaCommandLine -Arguments @(
    "featherdoc_cli",
    "set-content-control-form-state",
    "--alias",
    "Status Owner's Choice",
    "--data-binding-store-item-id",
    "{55555555-5555-5555-5555-555555555555}"
)
Assert-Equal -Actual $singleQuoteAndBraceCommandLine -Expected 'featherdoc_cli set-content-control-form-state --alias "Status Owner''s Choice" --data-binding-store-item-id "{55555555-5555-5555-5555-555555555555}"' `
    -Message "Command-line helper should quote single-quote text and brace-wrapped binding ids consistently."

$nullSkippedCommandLine = ConvertTo-TemplateSchemaCommandLine -Arguments @("cmd", $null, "next")
Assert-Equal -Actual $nullSkippedCommandLine -Expected "cmd next" `
    -Message "Command-line helper should skip null arguments."

$expandedArgumentList = Expand-TemplateSchemaArgumentList -Values @(
    "alpha, beta",
    " gamma ",
    "",
    " ,delta,, "
)
Assert-Equal -Actual ($expandedArgumentList -join "|") -Expected "alpha|beta|gamma|delta" `
    -Message "Argument-list helper should trim comma-delimited values and skip blanks."

$tempRoot = Join-Path $RepoRoot ".codex-temp\template-schema-cli-common-binary-test"
$resolvedTempRoot = [System.IO.Path]::GetFullPath($tempRoot)
$allowedTempRoot = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot ".codex-temp"))
if (-not $resolvedTempRoot.StartsWith($allowedTempRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing binary lookup fixture outside .codex-temp: $resolvedTempRoot"
}

if (Test-Path -LiteralPath $resolvedTempRoot) {
    Remove-Item -LiteralPath $resolvedTempRoot -Recurse -Force
}

try {
    New-TestFile `
        -Path (Join-Path $resolvedTempRoot "old\featherdoc_cli.exe") `
        -LastWriteTimeUtc ([datetime]"2024-01-01T00:00:00Z")
    $latestCli = Join-Path $resolvedTempRoot "new\featherdoc_cli.exe"
    New-TestFile `
        -Path $latestCli `
        -LastWriteTimeUtc ([datetime]"2024-01-02T00:00:00Z")
    $cliBinary = Find-TemplateSchemaCliBinary -SearchRoot $resolvedTempRoot
    Assert-Equal -Actual $cliBinary -Expected ([System.IO.Path]::GetFullPath($latestCli)) `
        -Message "CLI binary lookup should return the newest matching file."

    $sampleBinary = Join-Path $resolvedTempRoot "sample\featherdoc_sample.exe"
    New-TestFile `
        -Path $sampleBinary `
        -LastWriteTimeUtc ([datetime]"2024-01-03T00:00:00Z")
    $targetBinary = Find-TemplateSchemaTargetBinary `
        -SearchRoot $resolvedTempRoot `
        -TargetName "featherdoc_sample"
    Assert-Equal -Actual $targetBinary -Expected ([System.IO.Path]::GetFullPath($sampleBinary)) `
        -Message "Target binary lookup should return the newest target executable."

    $missingBinary = Find-TemplateSchemaTargetBinary `
        -SearchRoot $resolvedTempRoot `
        -TargetName "missing_target"
    Assert-Equal -Actual $missingBinary -Expected $null `
        -Message "Target binary lookup should return null for missing targets."
} finally {
    if (Test-Path -LiteralPath $resolvedTempRoot) {
        Remove-Item -LiteralPath $resolvedTempRoot -Recurse -Force
    }
}

Write-Host "template schema CLI common regression passed."
