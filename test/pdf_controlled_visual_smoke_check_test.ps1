param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [string]$PythonExecutable = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Resolve-Python {
    param([string]$RequestedPython)

    if (-not [string]::IsNullOrWhiteSpace($RequestedPython)) {
        if (Test-Path -LiteralPath $RequestedPython -PathType Leaf) {
            return (Resolve-Path $RequestedPython).Path
        }
        $requestedCommand = Get-Command $RequestedPython -ErrorAction SilentlyContinue
        if ($requestedCommand) { return $requestedCommand.Source }
        throw "Requested Python is not available: $RequestedPython"
    }

    if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_PYTHON_EXECUTABLE) -and
        (Test-Path -LiteralPath $env:FEATHERDOC_PYTHON_EXECUTABLE -PathType Leaf)) {
        return (Resolve-Path $env:FEATHERDOC_PYTHON_EXECUTABLE).Path
    }

    foreach ($commandName in @("python", "python3", "py")) {
        $command = Get-Command $commandName -ErrorAction SilentlyContinue
        if ($command) { return $command.Source }
    }

    throw "Python is required for the controlled visual smoke contract test."
}

function Invoke-PowerShellScript {
    param([string]$ScriptPath, [string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellPath = (Get-Command powershell.exe -ErrorAction Stop).Source
    }

    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

function New-SmokeFixture {
    param(
        [string]$Python,
        [string]$Root,
        [switch]$BlankImages
    )

    $blankFlag = if ($BlankImages) { "1" } else { "0" }
    @'
import json
import struct
import sys
import zlib
from pathlib import Path

root = Path(sys.argv[1])
blank = sys.argv[2] == "1"
root.mkdir(parents=True, exist_ok=True)

def png_chunk(name, data):
    return struct.pack(">I", len(data)) + name + data + struct.pack(">I", zlib.crc32(name + data) & 0xFFFFFFFF)

def write_png(path, width=120, height=120):
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            if blank or (x < 20 or y < 20):
                raw.extend((255, 255, 255))
            else:
                raw.extend((20, 20, 20))
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"IDAT", zlib.compress(bytes(raw)))
        + png_chunk(b"IEND", b"")
    )

for case in ("minimal", "rerun"):
    case_dir = root / case
    pages_dir = case_dir / "pages"
    pages_dir.mkdir(parents=True, exist_ok=True)
    input_pdf = case_dir / "input.pdf"
    input_pdf.write_bytes(b"%PDF-1.4\n% fake controlled smoke input\n")
    page_png = pages_dir / "page-01.png"
    contact_sheet = case_dir / "contact-sheet.png"
    write_png(page_png)
    write_png(contact_sheet)
    summary = {
        "input_pdf": str(input_pdf),
        "page_count": 1,
        "pages": [str(page_png)],
        "contact_sheet": str(contact_sheet),
    }
    (case_dir / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    (case_dir / "text.txt").write_text(
        "=== Page 01 ===\nExpected Anchor\nStatus Preflight\n",
        encoding="utf-8",
    )
    text_summary = {
        "input_pdf": str(input_pdf),
        "page_count": 1,
        "expected_text": [],
        "matched_text": [],
        "missing_text": [],
        "output_text": str(case_dir / "text.txt"),
    }
    (case_dir / "text-summary.json").write_text(json.dumps(text_summary, indent=2), encoding="utf-8")
'@ | & $Python - $Root $blankFlag
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to create controlled visual smoke fixture."
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

$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_pdf_controlled_visual_smoke.ps1"
$helperPath = Join-Path $resolvedRepoRoot "scripts\check_pdf_controlled_visual_smoke.py"
$python = Resolve-Python -RequestedPython $PythonExecutable

$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count -gt 0) {
    throw "Controlled visual smoke PowerShell wrapper has parse errors."
}

$passRoot = Join-Path $resolvedWorkingDir "pass-smoke"
New-SmokeFixture -Python $python -Root $passRoot
$passSummaryPath = Join-Path $resolvedWorkingDir "pass-summary.json"
$passResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-Root", $passRoot,
    "-ExpectedText", "Expected Anchor",
    "-OutputJson", $passSummaryPath,
    "-PythonExecutable", $python,
    "-MinNonWhiteRatio", "0.001"
)
Assert-Equal -Actual $passResult.ExitCode -Expected 0 `
    -Message "Controlled visual smoke check should pass. Output: $($passResult.Text)"

$passSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $passSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$passSummary.schema) `
    -Expected "featherdoc.pdf_controlled_visual_smoke_check.v1" `
    -Message "Controlled visual smoke check should expose a stable schema."
Assert-Equal -Actual ([bool]$passSummary.passed) -Expected $true `
    -Message "Passing fixture should mark the summary as passed."
Assert-Equal -Actual ([int]$passSummary.case_count) -Expected 2 `
    -Message "Passing fixture should validate both smoke cases."
Assert-True -Condition ([double]$passSummary.cases[0].png_metrics[0].nonwhite_ratio -gt 0.001) `
    -Message "Passing fixture should report a non-blank PNG ratio."

$failRoot = Join-Path $resolvedWorkingDir "blank-smoke"
New-SmokeFixture -Python $python -Root $failRoot -BlankImages
$failSummaryPath = Join-Path $resolvedWorkingDir "fail-summary.json"
$failResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-Root", $failRoot,
    "-ExpectedCases", "minimal",
    "-ExpectedText", "Expected Anchor",
    "-OutputJson", $failSummaryPath,
    "-PythonExecutable", $python,
    "-MinNonWhiteRatio", "0.001"
)
Assert-True -Condition ($failResult.ExitCode -ne 0) `
    -Message "Blank fixture should fail controlled visual smoke check."

$failSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $failSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([bool]$failSummary.passed) -Expected $false `
    -Message "Blank fixture should mark the summary as failed."
$failCheckNames = ($failSummary.cases[0].checks | Where-Object { $_.status -eq "fail" } | ForEach-Object { [string]$_.name }) -join "`n"
Assert-True -Condition ($failCheckNames -match "png_nonblank") `
    -Message "Blank fixture should fail the png_nonblank check."

$helperText = Get-Content -Raw -Encoding UTF8 -LiteralPath $helperPath
foreach ($expectedText in @(
    "zlib.decompress",
    "png_nonblank",
    "expected_text_present",
    "featherdoc.pdf_controlled_visual_smoke_check.v1"
)) {
    Assert-True -Condition ($helperText -match [regex]::Escape($expectedText)) `
        -Message "Controlled visual smoke helper should keep marker '$expectedText'."
}

Write-Host "PDF controlled visual smoke check contract passed."
