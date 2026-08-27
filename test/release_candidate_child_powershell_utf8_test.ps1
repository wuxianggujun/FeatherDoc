param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Equal {
    param(
        [AllowNull()]$Actual,
        [AllowNull()]$Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected '$Expected', got '$Actual'."
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

. (Join-Path $resolvedRepoRoot "scripts\run_release_candidate_checks_core.ps1")
. (Join-Path $resolvedRepoRoot "scripts\run_release_candidate_checks_output_parsers.ps1")

$fakeVcvarsPath = Join-Path $resolvedWorkingDir "fake-vcvars.bat"
@'
@echo off
chcp 437 >NUL
exit /b 0
'@ | Set-Content -LiteralPath $fakeVcvarsPath -Encoding ASCII

$childScriptPath = Join-Path $resolvedWorkingDir "write-unicode-smoke-output.ps1"
@'
param(
    [string]$InstallPrefix,
    [string]$ConsumerDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Path $InstallPrefix -Force | Out-Null
New-Item -ItemType Directory -Path $ConsumerDir -Force | Out-Null

$unicodeOutputName =
    "FeatherDoc-" +
    [char]0x4E2D + [char]0x6587 + "-" +
    [char]0x65E5 + [char]0x672C + [char]0x8A9E + "-" +
    [char]::ConvertFromUtf32(0x1F642) +
    ".docx"
$consumerDocument = Join-Path $ConsumerDir $unicodeOutputName
Set-Content -LiteralPath $consumerDocument -Encoding UTF8 -Value "smoke"

Write-Output "Install prefix: $InstallPrefix"
Write-Output "Consumer document: $consumerDocument"
'@ | Set-Content -LiteralPath $childScriptPath -Encoding UTF8

$installPrefix = Join-Path $resolvedWorkingDir "install"
$consumerDir = Join-Path $resolvedWorkingDir "consumer"
$bootstrap = [ordered]@{
    mode = "vcvars"
    vcvars_path = $fakeVcvarsPath
}

$output = Invoke-ChildPowerShellInMsvcEnv `
    -MsvcBootstrap $bootstrap `
    -ScriptPath $childScriptPath `
    -Arguments @(
        "-InstallPrefix",
        $installPrefix,
        "-ConsumerDir",
        $consumerDir
    ) `
    -FailureMessage "Unicode child PowerShell smoke failed."
$parsed = Parse-InstallSmokeOutput -Lines $output

$expectedName =
    "FeatherDoc-" +
    [char]0x4E2D + [char]0x6587 + "-" +
    [char]0x65E5 + [char]0x672C + [char]0x8A9E + "-" +
    [char]::ConvertFromUtf32(0x1F642) +
    ".docx"
$expectedDocument = Join-Path $consumerDir $expectedName

Assert-Equal -Actual $parsed.install_prefix -Expected $installPrefix `
    -Message "Install prefix should survive the nested cmd/PowerShell output boundary."
Assert-Equal -Actual $parsed.consumer_document -Expected $expectedDocument `
    -Message "Unicode consumer path should survive the nested cmd/PowerShell output boundary."

Write-Host "Release candidate child PowerShell UTF-8 test passed."
