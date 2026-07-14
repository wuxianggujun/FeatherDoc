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

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    throw "WorkingDir is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot `
    "scripts\run_install_find_package_smoke.ps1"
$scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath

foreach ($expectedText in @(
    "function Convert-ToNativeProcessArgument",
    "function Invoke-ExpectedFailure",
    "[System.Diagnostics.ProcessStartInfo]::new()",
    "RedirectStandardOutput = `$true",
    "RedirectStandardError = `$true",
    "ReadToEndAsync()",
    "`$exitCode = `$process.ExitCode",
    "if (`$exitCode -eq 0)",
    "Invoke-ExpectedFailure",
    "-Arguments `$missingPdfConfigureArguments"
)) {
    Assert-ContainsText -Text $scriptText -ExpectedText $expectedText `
        -Message "Install smoke must preserve expected native-failure marker '$expectedText'."
}

Write-Host "Install find_package expected-failure contract passed."
