<#
.SYNOPSIS
Validates existing controlled PDF visual smoke artifacts.

.DESCRIPTION
This wrapper is intentionally read-only. It does not run CMake, CTest, Word,
LibreOffice, browser automation, PDF rendering, virtual environment creation,
or dependency installation. It only checks already-rendered PNG and text
evidence.
#>
param(
    [string]$Root = "output/pdf-controlled-visual-smoke-20260520",
    [string[]]$ExpectedCases = @("minimal", "rerun"),
    [string[]]$ExpectedText = @(
        "FeatherDoc Word visual smoke input",
        "This minimal document is generated for local visual validation preflight."
    ),
    [string]$OutputJson = "",
    [string]$PythonExecutable = "",
    [int]$MinWidth = 100,
    [int]$MinHeight = 100,
    [double]$MinNonWhiteRatio = 0.0001
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }
    return $resolved
}

function Resolve-Python {
    param([string]$RequestedPython)

    if (-not [string]::IsNullOrWhiteSpace($RequestedPython)) {
        if (Test-Path -LiteralPath $RequestedPython -PathType Leaf) {
            return (Resolve-Path $RequestedPython).Path
        }
        $requestedCommand = Get-Command $RequestedPython -ErrorAction SilentlyContinue
        if ($requestedCommand) {
            return $requestedCommand.Source
        }
        throw "Requested Python was not found: $RequestedPython"
    }

    if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_PDF_VISUAL_SMOKE_PYTHON_EXECUTABLE) -and
        (Test-Path -LiteralPath $env:FEATHERDOC_PDF_VISUAL_SMOKE_PYTHON_EXECUTABLE -PathType Leaf)) {
        return (Resolve-Path $env:FEATHERDOC_PDF_VISUAL_SMOKE_PYTHON_EXECUTABLE).Path
    }

    if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_PYTHON_EXECUTABLE) -and
        (Test-Path -LiteralPath $env:FEATHERDOC_PYTHON_EXECUTABLE -PathType Leaf)) {
        return (Resolve-Path $env:FEATHERDOC_PYTHON_EXECUTABLE).Path
    }

    foreach ($commandName in @("python", "python3", "py")) {
        $command = Get-Command $commandName -ErrorAction SilentlyContinue
        if ($command) {
            return $command.Source
        }
    }

    throw "Python was not found. Set -PythonExecutable, FEATHERDOC_PDF_VISUAL_SMOKE_PYTHON_EXECUTABLE, or FEATHERDOC_PYTHON_EXECUTABLE."
}

$repoRoot = Resolve-RepoRoot
$resolvedRoot = Resolve-RepoPath -RepoRoot $repoRoot -Path $Root -AllowMissing
$resolvedOutputJson = if ([string]::IsNullOrWhiteSpace($OutputJson)) {
    ""
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputJson -AllowMissing
}
$helperPath = Join-Path $repoRoot "scripts\check_pdf_controlled_visual_smoke.py"
if (-not (Test-Path -LiteralPath $helperPath -PathType Leaf)) {
    throw "Controlled PDF visual smoke helper was not found: $helperPath"
}

$python = Resolve-Python -RequestedPython $PythonExecutable
$invariantCulture = [System.Globalization.CultureInfo]::InvariantCulture
$arguments = @(
    $helperPath,
    "--root", $resolvedRoot,
    "--min-width", [string]$MinWidth,
    "--min-height", [string]$MinHeight,
    "--min-nonwhite-ratio", ([string]::Format($invariantCulture, "{0}", $MinNonWhiteRatio))
)

foreach ($caseName in @($ExpectedCases)) {
    if (-not [string]::IsNullOrWhiteSpace($caseName)) {
        $arguments += @("--case", $caseName)
    }
}

foreach ($text in @($ExpectedText)) {
    if (-not [string]::IsNullOrWhiteSpace($text)) {
        $arguments += @("--expect-text", $text)
    }
}

if (-not [string]::IsNullOrWhiteSpace($resolvedOutputJson)) {
    $arguments += @("--output", $resolvedOutputJson)
}

& $python @arguments
exit $LASTEXITCODE
