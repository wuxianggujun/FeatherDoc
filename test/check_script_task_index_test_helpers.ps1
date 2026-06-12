function Resolve-DefaultRepoRoot {
    if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
        return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    }

    return (Resolve-Path $RepoRoot).Path
}

function Resolve-DefaultWorkingDir {
    if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
        return [System.IO.Path]::GetFullPath((Join-Path $resolvedRepoRoot "output\check-script-task-index-test"))
    }

    return [System.IO.Path]::GetFullPath($WorkingDir)
}

function Write-Utf8NoBomFile {
    param(
        [string]$Path,
        [string]$Text
    )

    $parentDir = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parentDir)) {
        New-Item -ItemType Directory -Path $parentDir -Force | Out-Null
    }

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Assert-FileHasNoBom {
    param([string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        throw "File must be UTF-8 without BOM: $Path"
    }
}

function Assert-RawJsonUtcTimestamp {
    param(
        [string]$JsonText,
        [string]$FieldName
    )

    $fieldPattern = '"{0}"\s*:\s*"\d{{4}}-\d{{2}}-\d{{2}}T\d{{2}}:\d{{2}}:\d{{2}}Z"' -f [regex]::Escape($FieldName)
    if ($JsonText -notmatch $fieldPattern) {
        throw "Expected JSON field '$FieldName' to use UTC timestamp format."
    }
}

function Assert-FileContainsText {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Message
    )

    $text = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path
    if ($text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
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

function Assert-ArrayContains {
    param(
        [object[]]$Values,
        [string]$ExpectedValue,
        [string]$Message
    )

    foreach ($value in $Values) {
        if ($value -eq $ExpectedValue) {
            return
        }
    }

    throw $Message
}

function Invoke-IndexCheck {
    param(
        [string]$Root,
        [string]$SummaryJson,
        [string]$ReportMarkdown,
        [switch]$Quiet,
        [switch]$FailOnUnindexed
    )

    $parameters = @{
        RepoRoot = $Root
        SummaryJson = $SummaryJson
    }
    if (-not [string]::IsNullOrWhiteSpace($ReportMarkdown)) {
        $parameters.ReportMarkdown = $ReportMarkdown
    }
    if ($Quiet) {
        $parameters.Quiet = $true
    }
    if ($FailOnUnindexed) {
        $parameters.FailOnUnindexed = $true
    }

    return @(& $checkerScript @parameters *>&1)
}
