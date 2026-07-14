param(
    [string]$BuildDir = "build-msvc-nmake",
    [string]$InstallDir = "build-msvc-install",
    [string]$ConsumerSourceDir = "test/install_find_package",
    [string]$ConsumerBuildDir = "build-msvc-install-consumer",
    [string]$PdfConsumerSourceDir = "test/install_find_package_pdf",
    [string]$ToolchainFile = "",
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
$resolvedPdfConsumerSourceDir = (Resolve-RepoPath $PdfConsumerSourceDir)
$pdfConsumerBuildDir = "$resolvedConsumerBuildDir-pdf"
$resolvedToolchainFile = ""
if ($ToolchainFile) {
    $resolvedToolchainFile = Resolve-RepoPath $ToolchainFile
    if (-not (Test-Path -LiteralPath $resolvedToolchainFile -PathType Leaf)) {
        throw "CMake toolchain file does not exist: $resolvedToolchainFile"
    }
}

function Convert-ToNativeProcessArgument {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyString()]
        [string]$Value
    )

    if ($Value.Length -gt 0 -and $Value -notmatch '[\s"]') {
        return $Value
    }

    $builder = [System.Text.StringBuilder]::new()
    [void]$builder.Append('"')
    $backslashCount = 0
    foreach ($character in $Value.ToCharArray()) {
        if ($character -eq [char]0x5C) {
            ++$backslashCount
            continue
        }

        if ($character -eq '"') {
            [void]$builder.Append([char]0x5C, 2 * $backslashCount + 1)
            [void]$builder.Append('"')
            $backslashCount = 0
            continue
        }

        if ($backslashCount -gt 0) {
            [void]$builder.Append([char]0x5C, $backslashCount)
            $backslashCount = 0
        }
        [void]$builder.Append($character)
    }

    if ($backslashCount -gt 0) {
        [void]$builder.Append([char]0x5C, 2 * $backslashCount)
    }
    [void]$builder.Append('"')
    return $builder.ToString()
}

function Write-CapturedProcessStream {
    param([AllowEmptyString()][string]$Text)

    foreach ($line in ($Text -split "`r?`n")) {
        if ($line.Length -gt 0) {
            Write-Host $line
        }
    }
}

function Invoke-ExpectedFailure {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [Parameter(Mandatory = $true)]
        [string]$UnexpectedSuccessMessage
    )

    # Keep the expected non-zero command outside PowerShell's native error
    # pipeline. Windows PowerShell 5.1 otherwise turns redirected native stderr
    # into a terminating NativeCommandError under ErrorActionPreference=Stop.
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Arguments[0]
    $startInfo.Arguments = ($Arguments[1..($Arguments.Length - 1)] |
        ForEach-Object { Convert-ToNativeProcessArgument -Value $_ }) -join " "
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        [void]$process.Start()
        $standardOutputTask = $process.StandardOutput.ReadToEndAsync()
        $standardErrorTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()

        $standardOutput = $standardOutputTask.Result
        $standardError = $standardErrorTask.Result
        $exitCode = $process.ExitCode
    } finally {
        $process.Dispose()
    }

    Write-CapturedProcessStream -Text $standardOutput
    Write-CapturedProcessStream -Text $standardError

    if ($exitCode -eq 0) {
        throw $UnexpectedSuccessMessage
    }
}
$missingPdfConsumerSourceDir =
    (Resolve-RepoPath "test/install_find_package_missing_pdf")
$missingPdfConsumerBuildDir = "$resolvedConsumerBuildDir-missing-pdf"

if (-not (Test-Path -LiteralPath $resolvedBuildDir)) {
    throw "Build directory does not exist: $resolvedBuildDir"
}

$producerCache = Join-Path $resolvedBuildDir "CMakeCache.txt"
if (Test-Path -LiteralPath $producerCache) {
    $buildTypeEntry = Select-String `
        -LiteralPath $producerCache `
        -Pattern '^CMAKE_BUILD_TYPE:STRING=(.*)$' |
        Select-Object -First 1
    if ($buildTypeEntry -and $buildTypeEntry.Matches[0].Groups[1].Value) {
        $producerBuildType = $buildTypeEntry.Matches[0].Groups[1].Value
        if (-not $producerBuildType.Equals(
                $Config,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Requested install config '$Config' does not match the " +
                "single-config producer '$producerBuildType': $resolvedBuildDir"
        }
    }

    if (-not $resolvedToolchainFile) {
        $toolchainEntry = Select-String `
            -LiteralPath $producerCache `
            -Pattern '^CMAKE_TOOLCHAIN_FILE:(?:FILEPATH|UNINITIALIZED)=(.*)$' |
            Select-Object -First 1
        if ($toolchainEntry) {
            $producerToolchainFile =
                $toolchainEntry.Matches[0].Groups[1].Value.Trim()
            if ($producerToolchainFile -and
                (Test-Path -LiteralPath $producerToolchainFile -PathType Leaf)) {
                $resolvedToolchainFile =
                    [System.IO.Path]::GetFullPath($producerToolchainFile)
            }
        }
    }
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

# Intentionally do not pass the PDF dependency toolchain here. A consumer that
# requests only Core must configure and link without FreeType/PNG/HarfBuzz.
Invoke-Checked $configureArguments

$buildArguments = @(
    "cmake",
    "--build",
    $resolvedConsumerBuildDir,
    "--config",
    $Config
)

Invoke-Checked $buildArguments

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

$installedConfig = Join-Path $resolvedInstallDir `
    "lib/cmake/FeatherDoc/FeatherDocConfig.cmake"
if (-not (Test-Path -LiteralPath $installedConfig)) {
    throw "Installed package config was not found: $installedConfig"
}

$pdfComponentInstalled = Select-String `
    -LiteralPath $installedConfig `
    -SimpleMatch 'set(FeatherDoc_WITH_PDF "ON")' `
    -Quiet

if ($pdfComponentInstalled) {
    $pdfConfigureArguments = @(
        "cmake",
        "-S",
        $resolvedPdfConsumerSourceDir,
        "-B",
        $pdfConsumerBuildDir,
        "-DCMAKE_PREFIX_PATH=$resolvedInstallDir",
        "-DCMAKE_BUILD_TYPE=$Config"
    )
    if ($Generator) {
        $pdfConfigureArguments += @("-G", $Generator)
    }
    if ($resolvedToolchainFile) {
        $pdfConfigureArguments += "-DCMAKE_TOOLCHAIN_FILE=$resolvedToolchainFile"
    }

    Invoke-Checked $pdfConfigureArguments
    Invoke-Checked @(
        "cmake",
        "--build",
        $pdfConsumerBuildDir,
        "--config",
        $Config
    )

    $pdfExecutablePaths = @(
        (Join-Path $pdfConsumerBuildDir "featherdoc_install_pdf_smoke.exe"),
        (Join-Path (Join-Path $pdfConsumerBuildDir $Config) `
            "featherdoc_install_pdf_smoke.exe")
    )
    $pdfConsumerExecutable = $pdfExecutablePaths |
        Where-Object { Test-Path -LiteralPath $_ } |
        Select-Object -First 1
    if (-not $pdfConsumerExecutable) {
        throw "PDF consumer executable was not produced under $pdfConsumerBuildDir"
    }

    $unicodePdfName =
        "FeatherDoc-PDF-" +
        [char]0x4E2D + [char]0x6587 + "-" +
        [char]0x65E5 + [char]0x672C + [char]0x8A9E + "-" +
        [char]::ConvertFromUtf32(0x1F642) +
        ".pdf"
    $outputPdf = Join-Path $pdfConsumerBuildDir $unicodePdfName
    Invoke-Checked @($pdfConsumerExecutable, $outputPdf)

    if (-not (Test-Path -LiteralPath $outputPdf)) {
        throw "Expected smoke PDF was not created: $outputPdf"
    }
    $pdfBytes = [System.IO.File]::ReadAllBytes($outputPdf)
    if ($pdfBytes.Length -lt 5 -or
        [System.Text.Encoding]::ASCII.GetString($pdfBytes, 0, 5) -ne "%PDF-") {
        throw "Generated smoke output does not have a PDF signature: $outputPdf"
    }

    Write-Host "Installed Pdf component was consumed successfully."
    Write-Host "Consumer PDF: $outputPdf"
} else {
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
    Invoke-ExpectedFailure `
        -Arguments $missingPdfConfigureArguments `
        -UnexpectedSuccessMessage `
            "Core-only package unexpectedly satisfied the Pdf component."

    Write-Host "Unavailable Pdf component was rejected as expected."
}

Write-Host "Install + find_package smoke test passed."
Write-Host "Install prefix: $resolvedInstallDir"
Write-Host "Consumer document: $outputDocx"
