param(
    [string]$BuildDir = "build-msvc-nmake",
    [string]$InstallDir = "build-msvc-install",
    [string]$ConsumerSourceDir = "test/install_find_package",
    [string]$ConsumerBuildDir = "build-msvc-install-consumer",
    [string]$Generator = "",
    [string]$Config = "Release"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    return Join-Path $repoRoot $Path
}

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    & $Arguments[0] $Arguments[1..($Arguments.Length - 1)]
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $($Arguments -join ' ')"
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

$resolvedBuildDir = (Resolve-RepoPath $BuildDir)
$resolvedInstallDir = (Resolve-RepoPath $InstallDir)
$resolvedConsumerSourceDir = (Resolve-RepoPath $ConsumerSourceDir)
$resolvedConsumerBuildDir = (Resolve-RepoPath $ConsumerBuildDir)
$missingPdfConsumerSourceDir =
    (Resolve-RepoPath "test/install_find_package_missing_pdf")
$missingPdfConsumerBuildDir = "$resolvedConsumerBuildDir-missing-pdf"

if (-not (Test-Path -LiteralPath $resolvedBuildDir)) {
    throw "Build directory does not exist: $resolvedBuildDir"
}

Invoke-Checked @(
    "cmake",
    "--install",
    $resolvedBuildDir,
    "--prefix",
    $resolvedInstallDir,
    "--config",
    $Config
)

$configureArguments = @(
    "cmake",
    "-S",
    $resolvedConsumerSourceDir,
    "-B",
    $resolvedConsumerBuildDir,
    "-DCMAKE_PREFIX_PATH=$resolvedInstallDir",
    "-DCMAKE_BUILD_TYPE=$Config"
)

if ($Generator) {
    $configureArguments += @("-G", $Generator)
}

Invoke-Checked $configureArguments

$buildArguments = @(
    "cmake",
    "--build",
    $resolvedConsumerBuildDir,
    "--config",
    $Config
)

Invoke-Checked $buildArguments

if (Test-Path -LiteralPath $missingPdfConsumerBuildDir) {
    Remove-Item -LiteralPath $missingPdfConsumerBuildDir -Recurse -Force
}

$missingPdfConfigureArguments = @(
    "cmake",
    "-S",
    $missingPdfConsumerSourceDir,
    "-B",
    $missingPdfConsumerBuildDir,
    "-DCMAKE_PREFIX_PATH=$resolvedInstallDir",
    "-DCMAKE_BUILD_TYPE=$Config"
)
if ($Generator) {
    $missingPdfConfigureArguments += @("-G", $Generator)
}

& $missingPdfConfigureArguments[0] `
    $missingPdfConfigureArguments[1..($missingPdfConfigureArguments.Length - 1)]
if ($LASTEXITCODE -eq 0) {
    throw "Core-only package unexpectedly satisfied the Pdf component."
}

$candidateExecutablePaths = @(
    (Join-Path $resolvedConsumerBuildDir "featherdoc_install_smoke.exe"),
    (Join-Path (Join-Path $resolvedConsumerBuildDir $Config) "featherdoc_install_smoke.exe")
)
$consumerExecutable = $candidateExecutablePaths |
    Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1

if (-not $consumerExecutable) {
    throw "Consumer executable was not produced under $resolvedConsumerBuildDir"
}

$unicodeOutputName =
    "FeatherDoc-" +
    [char]0x4E2D + [char]0x6587 + "-" +
    [char]0x65E5 + [char]0x672C + [char]0x8A9E + "-" +
    [char]::ConvertFromUtf32(0x1F642) +
    ".docx"
$outputDocx = Join-Path $resolvedConsumerBuildDir $unicodeOutputName
$installBinDir = Join-Path $resolvedInstallDir "bin"

$originalPath = $env:PATH
try {
    if (Test-Path -LiteralPath $installBinDir) {
        $env:PATH = "$installBinDir;$env:PATH"
    }

    Invoke-Checked @(
        $consumerExecutable,
        $outputDocx
    )
} finally {
    $env:PATH = $originalPath
}

if (-not (Test-Path -LiteralPath $outputDocx)) {
    throw "Expected smoke document was not created: $outputDocx"
}

Write-Host "Install + find_package smoke test passed."
Write-Host "Unavailable Pdf component was rejected as expected."
Write-Host "Install prefix: $resolvedInstallDir"
Write-Host "Consumer document: $outputDocx"
