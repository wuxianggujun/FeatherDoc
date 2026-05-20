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

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_reused_build_check.ps1"
$scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath

foreach ($expectedText in @(
    "[int]`$MinFreeMemoryMB = 2048",
    "[switch]`$SkipMemoryGuard",
    "Get-LocalMemorySnapshot",
    "Assert-FreeMemoryAvailable",
    "Get-CimInstance Win32_OperatingSystem",
    "FreePhysicalMemory",
    "Refusing to start because free physical memory",
    "Close unrelated applications or rerun with -SkipMemoryGuard",
    "Assert-FreeMemoryAvailable -MinimumFreeMemoryMB `$MinFreeMemoryMB"
)) {
    Assert-ContainsText -Text $scriptText -ExpectedText $expectedText `
        -Message "run_reused_build_check.ps1 should keep resource guard contract marker '$expectedText'."
}

& powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File $scriptPath `
    -ListProcessesOnly | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "run_reused_build_check.ps1 -ListProcessesOnly should stay lightweight and exit successfully."
}

Write-Host "Reused build check resource guard contract passed."
